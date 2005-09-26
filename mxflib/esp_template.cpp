/*! \file	esp_template.cpp
 *	\brief	Implementation of class that handles parsing of <File Type>
 *
 *	\version $Id: esp_template.cpp,v 1.1 2005/09/26 08:35:59 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2004, Matt Beard
 *
 *	This software is provided 'as-is', without any express or implied warranty.
 *	In no event will the authors be held liable for any damages arising from
 *	the use of this software.
 *
 *	Permission is granted to anyone to use this software for any purpose,
 *	including commercial applications, and to alter it and redistribute it
 *	freely, subject to the following restrictions:
 *
 *	  1. The origin of this software must not be misrepresented; you must
 *	     not claim that you wrote the original software. If you use this
 *	     software in a product, an acknowledgment in the product
 *	     documentation would be appreciated but is not required.
 *	
 *	  2. Altered source versions must be plainly marked as such, and must
 *	     not be misrepresented as being the original software.
 *	
 *	  3. This notice may not be removed or altered from any source
 *	     distribution.
 */

#include "mxflib/mxflib.h"

#include <math.h>	// For "floor"

using namespace mxflib;

//! Local definitions
namespace
{
	//! Modified UUID for <Source Type>
	// TODO: Fill in new UUID here
	const UInt8 TEMPLATE_Format[] = { 0x45, 0x54, 0x57, 0x62,  0xd6, 0xb4, 0x2e, 0x4e,  0xf3, 'x', 'x', 'x',  'x', 'x', 'x', 'x' };
}


//! Examine the open file and return a list of essence descriptors
/*! \note This call will modify properties SampleRate, DataStart and DataSize */
EssenceStreamDescriptorList mxflib::TEMPLATE_EssenceSubParser::IdentifyEssence(FileHandle InFile)
{
	int BufferBytes;
	UInt8 Buffer[ <xxx> ];

	EssenceStreamDescriptorList Ret;

	// Read the first <xxx> bytes of the file to allow us to identify it
	FileSeek(InFile, 0);
	BufferBytes = (int)FileRead(InFile, Buffer, <xxx>);

	// If the file is smaller than <xxx> bytes give up now!
	if(BufferBytes < <xxx>) return Ret;

	// If the file doesn't start with <...> if can't be a <File Type> file
	if( <Not our type> ) return Ret;

	MDObjectPtr DescObj = BuildDescriptor(InFile, 0);

	// Quit here if we couldn't build an essence descriptor
	if(!DescObj) return Ret;

	// Build a descriptor with a zero ID (we only support single stream files)
	EssenceStreamDescriptor Descriptor;
	Descriptor.ID = 0;
	Descriptor.Description = <File Type>;
	Descriptor.SourceFormat.Set(TEMPLATE_Format);
	Descriptor.Descriptor = DescObj;

	// Record a pointer to the descriptor so we can check if we are asked to process this source
	CurrentDescriptor = DescObj;

	// Set the single descriptor
	Ret.push_back(Descriptor);

	return Ret;
}


//! Examine the open file and return the wrapping options known by this parser
/*! \param InFile The open file to examine (if the descriptor does not contain enough info)
 *	\param Descriptor An essence stream descriptor (as produced by function IdentifyEssence)
 *		   of the essence stream requiring wrapping
 *	\note The options should be returned in an order of preference as the caller is likely to use the first that it can support
 */
WrappingOptionList mxflib::TEMPLATE_EssenceSubParser::IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor &Descriptor)
{
	// TODO: Fill in the base wrapping UL
	UInt8 BaseUL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	WrappingOptionList Ret;

	// If the source format isn't <Source Type> then we can't wrap the essence
	if(memcmp(Descriptor.SourceFormat.GetValue(), TEMPLATE_Format, 16) != 0) return Ret;

	// The identify step configures some member variables so we can only continue if we just identified this very source
	if((!CurrentDescriptor) || (Descriptor.Descriptor != CurrentDescriptor)) return Ret;

	// TODO: Only fill in the supported wrapping types

	// Build a WrappingOption for clip wrapping
	WrappingOptionPtr ClipWrap = new WrappingOption;

	ClipWrap->Handler = this;							// Set us as the handler
	ClipWrap->Description = "SMPTE xxxM clip wrapping of <File Type>";

	BaseUL[14] = 0x02;									// Clip wrapping
	ClipWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	ClipWrap->GCEssenceType = 0x<yy>;					// <xx> wrapping type
	ClipWrap->GCElementType = 0x<zz>;					// Clip wrapped elemenet
	ClipWrap->ThisWrapType = WrappingOption::Clip;		// Clip wrapping
	ClipWrap->CanSlave = true;							// Can use non-native edit rate
	ClipWrap->CanIndex = false;							// We CANNOT currently index this essence
	ClipWrap->CBRIndex = true;							// This essence uses CBR indexing
	ClipWrap->BERSize = 0;								// No BER size forcing

	// Build a WrappingOption for frame wrapping
	WrappingOptionPtr FrameWrap = new WrappingOption;

	FrameWrap->Handler = this;							// Set us as the handler
	FrameWrap->Description = "SMPTE xxxM frame wrapping of <File Type>";

	BaseUL[14] = 0x01;									// Frame wrapping
	FrameWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	FrameWrap->GCEssenceType = 0x<yy>;					// <xx> wrapping type
	FrameWrap->GCElementType = 0x<zz>;					// Frame wrapped elemenet
	FrameWrap->ThisWrapType = WrappingOption::Frame;	// Frame wrapping
	FrameWrap->CanSlave = true;							// Can use non-native edit rate
	FrameWrap->CanIndex = false;						// We CANNOT currently index this essence
	FrameWrap->CBRIndex = true;							// This essence uses CBR indexing
	FrameWrap->BERSize = 0;								// No BER size forcing

	// Add the two wrapping options 
	// Note: <aaa> wrapping is preferred as this works best for <This type>
	// TODO: Select the appropriate order for wrapping
	Ret.push_back(FrameWrap);
	Ret.push_back(ClipWrap);

	return Ret;
}


//! Read a number of wrapping items from the specified stream and return them in a data chunk
/*! If frame or line mapping is used the parameter Count is used to
 *	determine how many items are read. In frame wrapping it is in
 *	units of EditRate, as specified in the call to Use(), which may
 *  not be the frame rate of this essence
 *	\note This is going to take a lot of memory in clip wrapping! 
 */
DataChunkPtr mxflib::TEMPLATE_EssenceSubParser::Read(FileHandle InFile, UInt32 Stream, UInt64 Count /*=1*/)
{
	// Move to the current position
	if(CurrentPos == 0) CurrentPos = DataStart;

	FileSeek(InFile, CurrentPos);
	
	// Find out how many bytes to read
	Length Bytes = ReadInternal(InFile, Stream, Count);

	// Make a datachunk with enough space
	DataChunkPtr Ret = new DataChunk;
	Ret->Resize((UInt32)Bytes);

	// Read the data
	FileRead(InFile, Ret->Data, Bytes);

	// Update the file pointer
	CurrentPos = FileTell(InFile);

	return Ret; 
}


//! Write a number of wrapping items from the specified stream to an MXF file
/*! If frame or line mapping is used the parameter Count is used to
 *	determine how many items are read. In frame wrapping it is in
 *	units of EditRate, as specified in the call to Use(), which may
 *  not be the frame rate of this essence stream
 *	\note This is the only safe option for clip wrapping
 *	\return Count of bytes transferred
 */
Length mxflib::TEMPLATE_EssenceSubParser::Write(FileHandle InFile, UInt32 Stream, MXFFilePtr OutFile, UInt64 Count /*=1*/)
{
	const unsigned int BUFFERSIZE = 32768;
	UInt8 *Buffer = new UInt8[BUFFERSIZE];

	// Move to the current position
	if(CurrentPos == 0) CurrentPos = DataStart;
	FileSeek(InFile, CurrentPos);
	
	// Find out how many bytes to transfer
	Length Bytes = ReadInternal(InFile, Stream, Count);
	Length Ret = Bytes;

	while(Bytes)
	{
		int ChunkSize;
		
		// Number of bytes to transfer in this chunk
		if(Bytes < BUFFERSIZE) ChunkSize =(int) Bytes; else ChunkSize = BUFFERSIZE;

		FileRead(InFile, Buffer, ChunkSize);
		OutFile->Write(Buffer, ChunkSize);

		Bytes -= ChunkSize;
	}

	// Update the file pointer
	CurrentPos = FileTell(InFile);

	return Ret; 
}


//! Get the preferred edit rate (if one is known)
/*! \return The prefered edit rate or 0/0 if note known
 */
Rational mxflib::TEMPLATE_EssenceSubParser::GetPreferredEditRate(void)
{
	// TODO: This code is designed for audio essence (probably the hardest case) 
	//       Rewrite or simplify for other essence types - generally it will be obvious what to use

	/* Pick a sensible edit rate */
	/*****************************/

	/* Try 24ms first */

	// Calculate the number of samples in a 24ms frame
	double Samples = ((double)SampleRate * 24.0) / 1000.0;

	// If this is an integer value then all is well
	if(Samples == floor(Samples))
	{
		// Return 24ms edit rate
		return Rational(1000,24);
	}

	/* Try 100ms next */
	// DRAGONS: Is there any point in doing this?

	// Calculate the number of samples in a 100ms frame
	Samples = ((double)SampleRate * 100.0) / 1000.0;

	// If this is an integer value then all is well
	if(Samples == floor(Samples))
	{
		// Return 100ms edit rate
		return Rational(1000,100);
	}

	// 1Hz will always work for <File Type>
	// TODO: Check this is true
	return Rational(1,1);
}


//! Work out wrapping sequence
/*! \return true if a sequence was found, otherwise false */
bool mxflib::TEMPLATE_EssenceSubParser::CalcWrappingSequence(Rational EditRate)
{
	// Delete any previous sequence data
	if(SampleSequence != NULL) 
	{
		delete[] SampleSequence;
		SampleSequence = NULL;
	}

	// Invalid edit rate!
	if(EditRate.Numerator == 0) return false;

	// Work out the desired number of samples per edit unit
	float SamplesPerEditUnit = (float(EditRate.Denominator) * float(SampleRate)) / float(EditRate.Numerator);

	// If we can acheive the desired number then it's simple!
	if(SamplesPerEditUnit == floor(SamplesPerEditUnit))
	{
		ConstSamples = (UInt32)SamplesPerEditUnit;
		return true;
	}

	// Work the shortest sequence that can be used
	for(SampleSequenceSize=2; SampleSequenceSize<10000; SampleSequenceSize++)
	{
		float SamplesPerSequence = (float(EditRate.Denominator) * float(SampleRate) * SampleSequenceSize) / float(EditRate.Numerator);

		if(SamplesPerSequence == floor(SamplesPerSequence)) break;
	}

	// Put a reasonable upper limit on the sequence length
	if(SampleSequenceSize >= 10000)
	{
		error("TEMPLATE_EssenceSubParser::CalcWrappingSequence could not find a sequence < 10000 edit units long!\n");
		return false;
	}

	// Allocate a sequence and flag that constant samples are not being used
	ConstSamples = 0;
	SampleSequence = new UInt32[SampleSequenceSize];

	// Calculate a sequence that allocates the nearest fit
	float Remain = 0;
	int i;
	for(i = 0; i < SampleSequenceSize; i++)
	{
		float f = SamplesPerEditUnit + Remain;
		UInt32 x = (UInt32)floor(f + 0.5);
		SampleSequence[i] = x;
		Remain = f - x;
	}

	return true;
}



//! Get the current position in SetEditRate() sized edit units
/*! \return 0 if position not known
 */
Position TEMPLATE_EssenceSubParser::GetCurrentPosition(void)
{
	if(SampleSize == 0) return 0;

	// Simple case where each edit unit has the same number of samples
	if(ConstSamples != 0)
	{
		return (CurrentPos-DataStart) / (SampleSize * ConstSamples);
	}

	// Work out how many samples are in a complete sequence
	UInt32 SeqSize = 0;
	int i;
	for(i=0; i < SampleSequenceSize; i++)
	{
		SeqSize += SampleSequence[i];
	}

	// Now work out how many complete sequences we are from the start of the essence
	Position CompleteSeq = (CurrentPos-DataStart) / SampleSize * SeqSize;

	// And The fractional part...
	Position FracSeq = (CurrentPos-DataStart) - (CompleteSeq * SeqSize);

	Position Ret = CompleteSeq * SeqSize;

	// Count back through the sequence to see how many edit units the fractional part is
	i = SequencePos;
	while(FracSeq)
	{
		// Step back through the sequence
		if(!i) i = SampleSequenceSize;
		i--;

		// Not a complete edit unit left
		if(FracSeq < SampleSequence[i]) break;

		Ret += SampleSequence[i];
		FracSeq -= SampleSequence[i];
	}

	return Ret;
}



//! Read the essence information at the specified position in the source file and build an essence descriptor
/*! \note This call will modify properties SampleRate, DataStart and DataSize */
MDObjectPtr mxflib::TEMPLATE_EssenceSubParser::BuildDescriptor(FileHandle InFile, UInt64 Start /*=0*/)
{
	MDObjectPtr Ret;

	// TODO: Build an essence descriptor

	return Ret;
}


//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
/*! \note The file position pointer is left at the start of the chunk at the end of 
 *		  this function
 */
Length mxflib::TEMPLATE_EssenceSubParser::ReadInternal(FileHandle InFile, UInt32 Stream, UInt64 Count) 
{ 
	Length Ret;
	UInt32 SamplesPerEditUnit;
	
	// If we haven't determined the sample sequence we do it now
	if((ConstSamples == 0) && (SampleSequenceSize == 0)) CalcWrappingSequence(UseEditRate);

	// Work out the maximum possible bytes to return
	if(CurrentPos == 0) CurrentPos = DataStart;		// Correct the start if we need to
	Length Max = (CurrentPos - DataStart);			// Where we are in the data
	if(Max >= DataSize) return 0;
	Max = DataSize - Max;							// How many bytes are left

	if(ConstSamples) 
	{
		SamplesPerEditUnit = ConstSamples;
	}
	else
	{
		if((SampleSequenceSize == 0) || (SampleSequence == NULL))
		{
			// If no edit rate has been set read single samples
			SamplesPerEditUnit = 1;
		}
		else
		{
			// Otherwise take the next in the sequence
			SamplesPerEditUnit = SampleSequence[SequencePos];
			SequencePos++;
			if(SequencePos >= SampleSequenceSize) SequencePos = 0;
		}
	}

	
	// Return anything we can find if in "unspecified" clip wrapping
	if((Count == 0) && (SelectedWrapping->ThisWrapType == WrappingOption::Clip)) Ret = Max;
	else Ret = Count * SamplesPerEditUnit * SampleSize;

	// Return no more than the maximum bytes available
	if(Ret > Max)
	{
		// DRAGONS: Can force no "partial" edit units here if required
		// while(Ret > Max) Ret -= (SamplesPerEditUnit * SampleSize)
		Ret = Max;
	}

	return Ret;
}
