/*! \file	esp_wavepcm.cpp
 *	\brief	Implementation of class that handles parsing of uncompressed pcm wave audio files
 *
 *	\version $Id: esp_wavepcm.cpp,v 1.14 2007/10/09 12:51:52 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2003, Matt Beard
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

#include <mxflib/mxflib.h>

#include <math.h>	// For "floor"

using namespace mxflib;

#include <mxflib/esp_wavepcm.h>



//! Local definitions
namespace
{
	//! Modified UUID for RIFF-wrapped wave PCM audio
	const UInt8 WAVE_PCM_RIFF_Format[] = { 0x45, 0x54, 0x57, 0x62,  0xd6, 0xb4, 0x2e, 0x4e,  0xf3, 'R', 'I', 'F',  'F', 'W', 'A', 'V' };
}


//! Examine the open file and return a list of essence descriptors
/*! \note This call will modify properties SampleRate, DataStart and DataSize */
EssenceStreamDescriptorList mxflib::WAVE_PCM_EssenceSubParser::IdentifyEssence(FileHandle InFile)
{
	int BufferBytes;
	UInt8 Buffer[12];

	EssenceStreamDescriptorList Ret;

	// Read the first 12 bytes of the file to allow us to identify it
	FileSeek(InFile, 0);
	BufferBytes = (int)FileRead(InFile, Buffer, 12);

	// If the file is smaller than 12 bytes give up now!
	if(BufferBytes < 12) return Ret;

	// If the file doesn't start with "RIFF" if can't be a wave file
	if((Buffer[0] != 'R') || (Buffer[1] != 'I') || (Buffer[2] != 'F') || (Buffer[3] != 'F')) return Ret;

	// Just because the file is a RIFF file doesn't mean it's a wave file!
	if((Buffer[8] != 'W') || (Buffer[9] != 'A') || (Buffer[10] != 'V') || (Buffer[11] != 'E')) return Ret;

	MDObjectPtr DescObj = BuildWaveAudioDescriptor(InFile, 0);

	// Quit here if we couldn't build an essence descriptor
	if(!DescObj) return Ret;

	// Build a descriptor with a zero ID (we only support single stream files)
	EssenceStreamDescriptorPtr Descriptor = new EssenceStreamDescriptor;
	Descriptor->ID = 0;
	Descriptor->Description = "Wave audio essence";
	Descriptor->SourceFormat.Set(WAVE_PCM_RIFF_Format);
	Descriptor->Descriptor = DescObj;

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
WrappingOptionList mxflib::WAVE_PCM_EssenceSubParser::IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor &Descriptor)
{
	UInt8 BaseUL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x02, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x06, 0x01, 0x00 };
	WrappingOptionList Ret;

	// If the source format isn't RIFF-wrapped wave PCM then we can't wrap the essence
	if(memcmp(Descriptor.SourceFormat.GetValue(), WAVE_PCM_RIFF_Format, 16) != 0) return Ret;

	// The identify step configures some member variables so we can only continue if we just identified this very source
	if((!CurrentDescriptor) || (Descriptor.Descriptor != CurrentDescriptor)) return Ret;

	// Build a WrappingOption for clip wrapping
	WrappingOptionPtr ClipWrap = new WrappingOption;

	ClipWrap->Handler = this;							// Set us as the handler
	ClipWrap->Description = "SMPTE 382M clip wrapping of wave audio";

	BaseUL[14] = 0x02;									// Clip wrapping
	ClipWrap->Name = "clip";							// Set the wrapping name
	ClipWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	ClipWrap->GCEssenceType = 0x16;						// GP Sound wrapping type
	ClipWrap->GCElementType = 0x02;						// Wave clip wrapped elemenet
	ClipWrap->ThisWrapType = WrappingOption::Clip;		// Clip wrapping
	ClipWrap->CanSlave = true;							// Can use non-native edit rate
	ClipWrap->CanIndex = false;							// We CANNOT currently index this essence
	ClipWrap->CBRIndex = true;							// This essence uses CBR indexing
	ClipWrap->BERSize = 0;								// No BER size forcing

	// Build a WrappingOption for frame wrapping
	WrappingOptionPtr FrameWrap = new WrappingOption;

	FrameWrap->Handler = this;							// Set us as the handler
	FrameWrap->Description = "SMPTE 382M frame wrapping of wave audio";

	BaseUL[14] = 0x01;									// Frame wrapping
	FrameWrap->Name = "frame";							// Set the wrapping name
	FrameWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	FrameWrap->GCEssenceType = 0x16;					// GP Sound wrapping type
	FrameWrap->GCElementType = 0x01;					// Wave frame wrapped elemenet
	FrameWrap->ThisWrapType = WrappingOption::Frame;	// Frame wrapping
	FrameWrap->CanSlave = true;							// Can use non-native edit rate
	FrameWrap->CanIndex = false;						// We CANNOT currently index this essence
	FrameWrap->CBRIndex = true;							// This essence uses CBR indexing
	FrameWrap->BERSize = 0;								// No BER size forcing

	// Add the two wrapping options 
	// Note: clip wrapping is preferred as this works best for audio-only files
	Ret.push_back(ClipWrap);
	Ret.push_back(FrameWrap);

	return Ret;
}




//! Get the next "installment" of essence data
/*! \return Pointer to a data chunk holding the next data or a NULL pointer when no more remains
 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
 *	\note If Size = 0 the object will decide the size of the chunk to return
 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
 */
DataChunkPtr WAVE_PCM_EssenceSubParser::ESP_EssenceSource::GetEssenceData(size_t Size /*=0*/, size_t MaxSize /*=0*/)
{
	WAVE_PCM_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, WAVE_PCM_EssenceSubParser);

	// Allow us to differentiate the first call
	if(!Started)
	{
		Started = true;

		// Move to the selected position
		if(pCaller->BytePosition == 0) pCaller->BytePosition = pCaller->DataStart;
	}

	if(!BytesRemaining)
	{
		// Either use the cached value, or scan the stream and find out how many bytes to read
		if((pCaller->CachedDataSize == static_cast<size_t>(-1)) || (pCaller->CachedCount != RequestedCount)) pCaller->ReadInternal(File, Stream, RequestedCount);

		// Record, then clear, the data size
		BytesRemaining = pCaller->CachedDataSize;
		pCaller->CachedDataSize = static_cast<size_t>(-1);

		// Flag all done when no more to read
		if(BytesRemaining == 0)
		{
			AtEndOfData = true;
			return NULL;
		}
	}

	// Decide how many bytes to read this time - start by trying to read them all
	size_t Bytes = BytesRemaining;

	// Hard limit to MaxSize
	if((MaxSize != 0) && (Bytes > MaxSize))
	{
		Bytes = MaxSize;
	}

	// Also limit to Size
	if((Size != 0) && (Bytes > Size))
	{
		Bytes = Size;
	}

	// Remove this number of bytes from the remaining count
	BytesRemaining -= Bytes;

	// Seek to the current position
	FileSeek(File, pCaller->BytePosition);

	// Read the data
	DataChunkPtr Ret = FileReadChunk(File, Bytes);

	// Update the file pointer
	pCaller->BytePosition = FileTell(File);

	// Move the edit unit pointer forward by the number of edit units read (if the last part of a read)
	if(!BytesRemaining)	
	{
		// Only do a simple add if not reading the whole clip, and if the read succeeded
		if((pCaller->SelectedWrapping->ThisWrapType != WrappingOption::Clip) && (Ret->Size == Bytes)) pCaller->CurrentPosition += RequestedCount;
		// ... otherwise calculate the new position
		else pCaller->CurrentPosition = pCaller->CalcCurrentPosition();
	}

	return Ret;
}


//! Read a number of wrapping items from the specified stream and return them in a data chunk
/*! If frame or line mapping is used the parameter Count is used to
 *	determine how many items are read. In frame wrapping it is in
 *	units of EditRate, as specified in the call to Use(), which may
 *  not be the frame rate of this essence
 *	\note This is going to take a lot of memory in clip wrapping! 
 */
DataChunkPtr mxflib::WAVE_PCM_EssenceSubParser::Read(FileHandle InFile, UInt32 Stream, UInt64 Count /*=1*/ /*, IndexTablePtr Index */ /*=NULL*/)
{
	// Move to the current position
	if(BytePosition == 0) BytePosition = DataStart;

	FileSeek(InFile, BytePosition);
	
	// Either use the cached value, or scan the stream and find out how many bytes to read
	if((CachedDataSize == static_cast<size_t>(-1)) || (CachedCount != Count)) ReadInternal(InFile, Stream, Count);

	// Record, then clear, the data size
	size_t Bytes = CachedDataSize;
	CachedDataSize = static_cast<size_t>(-1);

	// Make a datachunk with enough space
	DataChunkPtr Ret = new DataChunk(Bytes);

	// Read the data
	size_t BytesRead = static_cast<size_t>(FileRead(InFile, Ret->Data, Bytes));

	// Update the file pointer
	BytePosition = FileTell(InFile);

	// Move the edit unit pointer forward by the number of edit units read
	CurrentPosition += Count;

	// Cope with an early end-of-file
	if(BytesRead < Bytes)
	{
		DataSize = BytePosition - DataStart;
		Ret->Resize(BytesRead);

		// We need to work out where we actually ended
		CurrentPosition = CalcCurrentPosition();
	}

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
Length mxflib::WAVE_PCM_EssenceSubParser::Write(FileHandle InFile, UInt32 Stream, MXFFilePtr OutFile, UInt64 Count /*=1*/ /*, IndexTablePtr Index*/ /*=NULL*/)
{
	const unsigned int BUFFERSIZE = 32768;
	UInt8 *Buffer = new UInt8[BUFFERSIZE];

	// Move to the current position
	if(BytePosition == 0) BytePosition = DataStart;
	FileSeek(InFile, BytePosition);
	
	// Scan the stream and find out how many bytes to transfer
	// Either use the cached value, or scan the stream and find out how many bytes to read
	if((CachedDataSize == static_cast<size_t>(-1)) || (CachedCount != Count)) ReadInternal(InFile, Stream, Count);
	size_t Bytes = CachedDataSize;
	Length Ret = static_cast<Length>(Bytes);

	// Clear the cached size
	CachedDataSize = static_cast<size_t>(-1);

	while(Bytes)
	{
		size_t ChunkSize;
		
		// Number of bytes to transfer in this chunk
		if(Bytes < BUFFERSIZE) ChunkSize = Bytes; else ChunkSize = BUFFERSIZE;

		FileRead(InFile, Buffer, ChunkSize);
		OutFile->Write(Buffer, ChunkSize);

		Bytes -= ChunkSize;
	}

	// Update the file pointer
	BytePosition = FileTell(InFile);

	// Free the buffer
	delete[] Buffer;

	return Ret; 
}


//! Get the preferred edit rate (if one is known)
/*! \return The prefered edit rate or 0/0 if note known
 */
Rational mxflib::WAVE_PCM_EssenceSubParser::GetPreferredEditRate(void)
{
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

	// 1Hz will always work for Wave audio
	return Rational(1,1);
}


//! Work out wrapping sequence
/*! \return true if a sequence was found, otherwise false */
bool mxflib::WAVE_PCM_EssenceSubParser::CalcWrappingSequence(Rational EditRate)
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
		error("WAVE_EssenceSubParser::CalcWrappingSequence could not find a sequence < 10000 edit units long!\n");
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



//! Calculate the current position in SetEditRate() sized edit units from "BytePosition" in bytes
/*! \return 0 if position not known
 */
Position WAVE_PCM_EssenceSubParser::CalcCurrentPosition(void)
{
	if(SampleSize == 0) return 0;

	// Simple case where each edit unit has the same number of samples
	if(ConstSamples != 0)
	{
		return (BytePosition-DataStart) / (SampleSize * ConstSamples);
	}

	// Work out how many samples are in a complete sequence
	UInt32 SeqSize = 0;
	int i;
	for(i=0; i < SampleSequenceSize; i++)
	{
		SeqSize += SampleSequence[i];
	}

	// Now work out how many complete sequences we are from the start of the essence
	Position CompleteSeq = (BytePosition-DataStart) / SampleSize * SeqSize;

	// And The fractional part...
	Position FracSeq = (BytePosition-DataStart) - (CompleteSeq * SeqSize);

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



//! Read the sequence header at the specified position in a Wave file to build an essence descriptor
/*! \note This call will modify properties SampleRate, DataStart and DataSize */
MDObjectPtr mxflib::WAVE_PCM_EssenceSubParser::BuildWaveAudioDescriptor(FileHandle InFile, UInt64 Start /*=0*/)
{
	const unsigned int ID_RIFF = 0x52494646;		//! "RIFF"
	const unsigned int ID_fmt  = 0x666d7420;		//! "fmt "
	const unsigned int ID_data = 0x64617461;		//! "data"

	MDObjectPtr Ret;

	FileSeek(InFile, Start);
	U32Pair Header = ReadRIFFHeader(InFile);

	// Can't build a descriptor if it isn't a RIFF file!
	if(Header.first != ID_RIFF) return Ret;
	if(Header.second < 4) return Ret;

	// Read the RIFF file type (always 4 bytes)
	DataChunkPtr ChunkData = FileReadChunk(InFile, 4);
	
	// Can't build a descriptor if it isn't a WAVE file!
	if(memcmp(ChunkData->Data, "WAVE", 4) != 0) return Ret;

	// Scan the chunks within the RIFF file
	// DRAGONS: To do this properly we would check the file size in the RIFF chunk
	// DRAGONS: "LIST" chunks are "sets" and are not yet supported
	for(;;)
	{
		Header = ReadRIFFHeader(InFile);

		// End of file?
		if((Header.first == 0) && (Header.second == 0)) break;

		if(Header.first == ID_fmt)
		{
			ChunkData = FileReadChunk(InFile, Header.second);
			if(ChunkData->Size < 16) return Ret;

			UInt16 AudioFormat = GetU16_LE(&ChunkData->Data[0]);
			if(AudioFormat != 1) return Ret;

			Ret = new MDObject(WaveAudioDescriptor_UL);
			if(!Ret) return Ret;

			// Set the sample rate
			char Buffer[32];
			SampleRate = GetU32_LE(&ChunkData->Data[4]);
			sprintf(Buffer, "%d/1", SampleRate);
			Ret->SetString(SampleRate_UL, Buffer);
			Ret->SetString(AudioSamplingRate_UL, Buffer);

			// Must assume not locked!
			Ret->SetUInt(Locked_UL, 0);

			// Set channel count
			UInt16 Chan = GetU16_LE(&ChunkData->Data[2]);
			Ret->SetUInt(ChannelCount_UL, Chan);

			// Set quantization bits
			UInt16 Quant = GetU16_LE(&ChunkData->Data[14]);
			Ret->SetUInt(QuantizationBits_UL, Quant);

			// Calculate the number of bytes per sample
			SampleSize = ((Quant+7) / 8) * Chan;

			// Set the block alignment
			Ret->SetUInt(BlockAlign_UL, GetU16_LE(&ChunkData->Data[12]));

			// Set the byte-rate
			Ret->SetUInt(AvgBps_UL, GetU32_LE(&ChunkData->Data[8]));
		}
		else if(Header.first == ID_data)
		{
			// Record the location of the audio data
			DataStart = FileTell(InFile);
			DataSize = Header.second;

			// ...and skip the chunk value
			FileSeek(InFile, FileTell(InFile) + Header.second);
		}
		else
		{
			// Skip the chunk value
			FileSeek(InFile, FileTell(InFile) + Header.second);
		}
	}

	return Ret;
}


//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
/*! \note The file position pointer is left at the start of the chunk at the end of 
 *		  this function
 */
size_t mxflib::WAVE_PCM_EssenceSubParser::ReadInternal(FileHandle InFile, UInt32 Stream, UInt64 Count) 
{ 
	Length Ret;
	UInt32 SamplesPerEditUnit;
	
	// Return the cached value if we have not yet used it
	if((CachedDataSize != static_cast<size_t>(-1)) && (CachedCount == Count)) return CachedDataSize;

	// If we haven't determined the sample sequence we do it now
	if((ConstSamples == 0) && (SampleSequenceSize == 0)) CalcWrappingSequence(UseEditRate);

	// Work out the maximum possible bytes to return
	if(BytePosition == 0) BytePosition = DataStart;		// Correct the start if we need to
	Length Max = (BytePosition - DataStart);			// Where we are in the data
	if(Max >= DataSize) 
	{
		CachedDataSize = 0;
		return 0;
	}
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

	// Return anything we can find if in clip wrapping
	if(SelectedWrapping->ThisWrapType == WrappingOption::Clip) Ret = Max;
	else 
	{
		Ret = SamplesPerEditUnit * SampleSize;
		if(Count) Ret *= Count;
	}

	// Return no more than the maximum bytes available
	if(Ret > Max)
	{
		// DRAGONS: Can force no "partial" edit units here if required
		// while(Ret > Max) Ret -= (SamplesPerEditUnit * SampleSize)
		Ret = Max;
	}

	// Validate the size
	if((sizeof(size_t) < 8) && (Ret > 0xffffffff))
	{
		error("This edit unit > 4GBytes, but this platform can only handle <= 4GByte chunks\n");
		Ret = 0;
	}

	// Store so we don't have to calculate if called again without reading
	CachedDataSize =  static_cast<size_t>(Ret);
	CachedCount = Count;
	
	return CachedDataSize;
}
