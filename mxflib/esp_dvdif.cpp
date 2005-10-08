/*! \file	esp_dvdif.cpp
 *	\brief	Implementation of class that handles parsing of DV-DIF streams
 *
 *	\version $Id: esp_dvdif.cpp,v 1.6 2005/10/08 15:35:33 matt-beard Exp $
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

//! Local definitions
namespace
{
	//! Modified UUID for raw DV
	const UInt8 DV_DIF_RAW_Format[] = { 0x45, 0x54, 0x57, 0x62,  0xd6, 0xb4, 0x2e, 0x4e,  0xf3, 0xd2, 0xfa, 'R',  'A', 'W', 'D', 'V' };

	//! Modified UUID for AVI-wrapped DV
	const UInt8 DV_DIF_AVI_Format[] = { 0x45, 0x54, 0x57, 0x62,  0xd6, 0xb4, 0x2e, 0x4e,  0xf3, 0xd2, 0xfa, 'A',  'V', 'I', 'D', 'V' };
}


//! Examine the open file and return a list of essence descriptors
EssenceStreamDescriptorList DV_DIF_EssenceSubParser::IdentifyEssence(FileHandle InFile)
{
	int BufferBytes;

	EssenceStreamDescriptorList Ret;

	// Allocate a buffer if we don't have one
	if(!Buffer) Buffer = new UInt8[DV_DIF_BUFFERSIZE];

	// Read the first 12 bytes of the file to allow us to identify it
	FileSeek(InFile, 0);
	BufferBytes =(int) FileRead(InFile, Buffer, 12);

	// If the file is smaller than 12 bytes give up now!
	if(BufferBytes < 12) return Ret;

	// If the file starts with "RIFF" if could be an AVI DV file
	if((Buffer[0] == 'R') && (Buffer[1] == 'I') && (Buffer[2] == 'F') && (Buffer[3] == 'F'))
	{
		// Just because the file is a RIFF file doesn't mean it's a DV AVI file!
		if((Buffer[8] != 'A') || (Buffer[9] != 'V') || (Buffer[10] != 'I') || (Buffer[11] != ' ')) return Ret;

		// So its an AVI file.. but what type?
		const unsigned int ID_LIST = 0x4C495354;		//! "LIST"
		const unsigned int ID_hdrl = 0x6864726c;		//! "hdrl"
		
		FileSeek(InFile, 12);
		U32Pair Header = ReadRIFFHeader(InFile);

		// If the first item isn't a list then we are stumpted!
		if(Header.first != ID_LIST) return Ret;

		// Size of header section list
		int ListSize = Header.second;

		// Sanity check the list
		if(ListSize < 4) return Ret;

		// Must be an "hdrl" list
		if(ReadU32(InFile) != ID_hdrl) return Ret;
		ListSize -= 4;

		// Find the "strl" entry
		while(ListSize > 0)
		{
			const unsigned int ID_strl = 0x7374726c;		//! "strl"
			const unsigned int ID_strh = 0x73747268;		//! "strh"

			U32Pair Header = ReadRIFFHeader(InFile);
			ListSize -= 8;

			if(Header.first == ID_LIST)
			{
				ListSize -= 4;
				if(ReadU32(InFile) == ID_strl)
				{
					if(ReadRIFFHeader(InFile).first != ID_strh) return Ret;
					ListSize -= 8;

					ReadU32(InFile);
					UInt32 MediaType = ReadU32(InFile);
					ListSize -= 4;

					if(
					   (MediaType == 0x64767364)		// ! "dvsd"
					|| (MediaType == 0x44565344)		// ! "DVSD"
					)
					{
						error("Found a DV AVI file!!! - Code note yet implemented\n");
					
						FileSeek(InFile, FileTell(InFile) + ListSize);

//						MDObjectPtr DescObj = BuildWaveAudioDescriptor(InFile, 0);
						
						return Ret;
					}
				}
			}
		}

		return Ret;
	}


	// Is it a raw DIF file?
	// If is not easy to validate a raw DV-DIF file, the method used is as follows:
	// The header ID is read from the top 3 bits of each DIF block in what would be the first DIF sequence.
	// The probability of an arbitrary file having the correct values is, at first glance, in the order of 2^450:1 but
	// as the values follow a simple sequence the actual probability is somewhat less. It is still likely to be a good
	// enough method (maybe 2^50:1 at worst!)

	// The buffer must be big enough to hold an entire DIF sequence
	ASSERT(DV_DIF_BUFFERSIZE >= (80 * 150));

	// Read the first 80*150 bytes of the file, this should be the first DIF sequence
	FileSeek(InFile, 0);
	BufferBytes = (int)FileRead(InFile, Buffer, 80 * 150);

	// If we couldn't read the sequence give up now!
	if(BufferBytes < (80 * 150)) return Ret;

	// Validate the header section ID
	if((Buffer[0] & 0xe0) != 0x00) return Ret;

	// Validate the subcode section IDs
	if(((Buffer[80] & 0xe0) != 0x20) || ((Buffer[160] & 0xe0) != 0x20)) return Ret;

	// Validate the VAUX section IDs
	if(((Buffer[240] & 0xe0) != 0x40) || ((Buffer[320] & 0xe0) != 0x40) || ((Buffer[400] & 0xe0) != 0x40)) return Ret;

	// Validate the audio and video section IDs
	int i;
	for(i=0; i<144; i++)
	{
		// One in every 16 DIF blocks is audio
		if((i & 0x0f) == 0)
		{
			if ((Buffer[i * 80 + 480] & 0xe0) !=  0x60) return Ret;
		}
		else
		{
			if ((Buffer[i * 80 + 480] & 0xe0) !=  0x80) return Ret;
		}
	}

	// Attempt to parse the format
	MDObjectPtr DescObj = BuildCDCIEssenceDescriptor(InFile, 0);

	// Quit here if we couldn't build an essence descriptor
	if(!DescObj) return Ret;

	// Check the size (assume the entire file is DIF data)
	DIFStart = 0;
	FileSeekEnd(InFile);
	DIFEnd = FileTell(InFile);

	// Build a descriptor with a zero ID (we only support single stream files)
	EssenceStreamDescriptor Descriptor;
	Descriptor.ID = 0;
	Descriptor.Description = "DV-DIF audio/video essence";
	Descriptor.SourceFormat.Set(DV_DIF_RAW_Format);
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
WrappingOptionList DV_DIF_EssenceSubParser::IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor &Descriptor)
{
	UInt8 BaseUL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x02, 0x7f, 0x01 };
	WrappingOptionList Ret;

	// If the source format isn't RAW DV-DIFF then we can't wrap the essence
	if(memcmp(Descriptor.SourceFormat.GetValue(), DV_DIF_RAW_Format, 16) != 0) return Ret;

	// The identify step configures some member variables so we can only continue if we just identified this very source
	if((!CurrentDescriptor) || (Descriptor.Descriptor != CurrentDescriptor)) return Ret;

	// Build a WrappingOption for clip wrapping
	WrappingOptionPtr ClipWrap = new WrappingOption;

	ClipWrap->Handler = this;							// Set us as the handler
	ClipWrap->Description = "SMPTE 383M clip wrapping of DV-DIF video data";

	BaseUL[15] = 0x02;									// Clip wrapping
	ClipWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	ClipWrap->GCEssenceType = 0x18;						// GC Compound wrapping type
	ClipWrap->GCElementType = 0x02;						// Clip wrapped picture elemenet
	ClipWrap->ThisWrapType = WrappingOption::Clip;		// Clip wrapping
	ClipWrap->CanSlave = true;							// Can use non-native edit rate (clip wrap only!)
	ClipWrap->CanIndex = false;							// We can NOT currently index this essence in VBR mode
	ClipWrap->CBRIndex = true;							// This essence uses CBR indexing
	ClipWrap->BERSize = 0;								// No BER size forcing

	// Build a WrappingOption for frame wrapping
	WrappingOptionPtr FrameWrap = new WrappingOption;

	FrameWrap->Handler = this;							// Set us as the handler
	FrameWrap->Description = "SMPTE 383M frame wrapping of DV-DIF video data";

	BaseUL[15] = 0x01;									// Frame wrapping
	FrameWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	FrameWrap->GCEssenceType = 0x18;					// GC Compound wrapping type
	FrameWrap->GCElementType = 0x01;					// Frame wrapped picture elemenet
	FrameWrap->ThisWrapType = WrappingOption::Frame;	// Frame wrapping
	FrameWrap->CanSlave = false;						// Can only use the correct edit rate
	FrameWrap->CanIndex = false;						// We can NOT currently index this essence in VBR mode
	FrameWrap->CBRIndex = true;							// This essence uses CBR indexing
	FrameWrap->BERSize = 0;								// No BER size forcing

	// Add the two wrapping options
	Ret.push_back(ClipWrap);
	Ret.push_back(FrameWrap);

	return Ret;
}


//! Set a wrapping option for future Read and Write calls
void DV_DIF_EssenceSubParser::Use(UInt32 Stream, WrappingOptionPtr &UseWrapping)
{
	SelectedWrapping = UseWrapping;
	SelectedEditRate = NativeEditRate;

	// Select the DIF sequence size
	if(NativeEditRate.Numerator == 25) SeqCount = 12; else SeqCount = 10;

	EditRatio = 1;
	PictureNumber = 0;
	CurrentPos = 0;
}


//! Set a non-native edit rate
/*! \return true if this rate is acceptable */
bool DV_DIF_EssenceSubParser::SetEditRate(Rational EditRate)
{
	if(    (EditRate.Numerator == NativeEditRate.Numerator) 
		&& (EditRate.Denominator == NativeEditRate.Denominator) )return true;

	// We can clip-wrap at any rate!
	if(SelectedWrapping->ThisWrapType == WrappingOption::Clip)
	{
		SelectedEditRate = EditRate;
		return true;
	}

	// Prevent divide by zero
	if(NativeEditRate.Denominator == 0) return false;
	if(EditRate.Denominator == 0) return false;

	double FloatNative = double(NativeEditRate.Numerator) / double(NativeEditRate.Denominator);
	double FloatUse = double(EditRate.Numerator) / double(EditRate.Denominator);

	// Select the DIF sequence size
	if(FloatNative == 25) SeqCount = 12; else SeqCount = 10;

	// Different representation for the same edit rate
	// E.G. 25/1 and 50/2
	if(FloatNative == FloatUse)
	{
		SelectedEditRate = EditRate;
		return true;
	}

	if(FloatUse == 0) return false;

	// Integer multiples of the native edit rate are valid
	double Ratio = FloatNative / FloatUse;
	if(Ratio == floor(Ratio))
	{
		EditRatio = (unsigned int)(Ratio);

		SeqCount *= EditRatio;
		
		return true;
	}

	return false;
}


//! Get the current position in SetEditRate() sized edit units
/*! \return 0 if position not known
 */
Position DV_DIF_EssenceSubParser::GetCurrentPosition(void)
{
	if((SelectedEditRate.Numerator == NativeEditRate.Numerator) && (SelectedEditRate.Denominator == NativeEditRate.Denominator))
	{
		return PictureNumber;
	}

	if((SelectedEditRate.Denominator == 0) || (NativeEditRate.Denominator || 0)) return 0;

	double Pos = (double)(PictureNumber * SelectedEditRate.Numerator * NativeEditRate.Denominator);
	Pos /= (SelectedEditRate.Denominator * NativeEditRate.Numerator);

	return (Position)floor(Pos + 0.5);
}


//! Read a number of wrapping items from the specified stream and return them in a data chunk
/*! If frame or line mapping is used the parameter Count is used to
 *	determine how many items are read. In frame wrapping it is in
 *	units of EditRate, as specified in the call to Use(), which may
 *  not be the frame rate of this essence
 *	\note This is going to take a lot of memory in clip wrapping! 
 */
DataChunkPtr DV_DIF_EssenceSubParser::Read(FileHandle InFile, UInt32 Stream, UInt64 Count /*=1*/) 
{ 
	// Scan the stream and find out how many bytes to read
	Length Bytes = ReadInternal(InFile, Stream, Count);

	// Read the data
	return FileReadChunk(InFile, Bytes);
};


//! Write a number of wrapping items from the specified stream to an MXF file
/*! If frame or line mapping is used the parameter Count is used to
 *	determine how many items are read. In frame wrapping it is in
 *	units of EditRate, as specified in the call to Use(), which may
 *  not be the frame rate of this essence stream
 *	\note This is the only safe option for clip wrapping
 *	\return Count of bytes transferred
 */
Length DV_DIF_EssenceSubParser::Write(FileHandle InFile, UInt32 Stream, MXFFilePtr OutFile, UInt64 Count /*=1*/)
{
	const unsigned int BUFFERSIZE = 32768;
	UInt8 *Buffer = new UInt8[BUFFERSIZE];

	// Scan the stream and find out how many bytes to transfer
	Length Bytes = ReadInternal(InFile, Stream, Count);
	Length Ret = Bytes;

	while(Bytes)
	{
		Length ChunkSize;
		
		// Number of bytes to transfer in this chunk
		if(Bytes < BUFFERSIZE) ChunkSize = Bytes; else ChunkSize = BUFFERSIZE;

		FileRead(InFile, Buffer, ChunkSize);
		OutFile->Write(Buffer, (UInt32)ChunkSize);

		Bytes -= ChunkSize;
	}

	return Ret; 
}


//! Read the header at the specified position in a DV file to build an essence descriptor
/*! DRAGONS: Currently rather scrappy */
MDObjectPtr DV_DIF_EssenceSubParser::BuildCDCIEssenceDescriptor(FileHandle InFile, UInt64 Start /*=0*/)
{
	MDObjectPtr Ret;
	UInt8 Buffer[80];

	// Read the header DIF block
	FileSeek(InFile, Start);
	if(FileRead(InFile, Buffer, 80) < 80) return Ret;

	// Set 625/50 flag from the header
	bool is625 = ((Buffer[3] & 0x80) == 0x80);

	// Set SMPTE-314M flag by assuming the APT value will only be 001 or 111 if we are in SMPTE-314M
	bool isS314M = ((Buffer[4] & 0x07) == 0x01) || ((Buffer[4] & 0x07) == 0x07);

	// Bug out if the video is flagged as invalid
	if((Buffer[6] & 0x80) != 0) return Ret;


	// Build the essence descriptor, filling in all known values

	Ret = new MDObject(CDCIEssenceDescriptor_UL);
	if(!Ret) return Ret;

	if(!is625)
	{
		Ret->SetString(SampleRate_UL, "30000/1001");

		NativeEditRate.Numerator = 30000;
		NativeEditRate.Denominator = 1001;

		SeqCount = 10;
	}
	else
	{
		Ret->SetString(SampleRate_UL, "25/1");

		NativeEditRate.Numerator = 25;
		NativeEditRate.Denominator = 1;

		SeqCount = 12;
	}

//DRAGONS: printf("Assumed interleaved...\n");
//	if(Progressive) Ret->SetInt("FrameLayout", 0); else Ret->SetInt("FrameLayout", 1);
	Ret->SetInt(FrameLayout_UL, 1);

	if(is625)
	{
		Ret->SetUInt(StoredWidth_UL, 720);
		Ret->SetUInt(StoredHeight_UL, 288);
	}
	else	
	{
		Ret->SetUInt(StoredWidth_UL, 720);
		Ret->SetUInt(StoredHeight_UL, 240);
	}

//DRAGONS: printf("Assumed 4:3...\n");
//	if(Aspect) Ret->SetString("AspectRatio", Aspect); else Ret->SetDValue("AspectRatio");
	Ret->SetString(AspectRatio_UL, "4/3");

	MDObjectPtr Ptr = Ret->AddChild(VideoLineMap_UL);
	if(Ptr)
	{
		int F1 = 0;
		int F2 = 0;

		if(is625) { F1 = 1; F2 = 313; }
		else { F1 = 4; F2 = 266; }

		Ptr->AddChild()->SetUInt(F1);
		Ptr->AddChild()->SetUInt(F2);
	}

	Ret->SetUInt(ComponentDepth_UL, 8);

	if(!is625)
	{
		Ret->SetUInt(HorizontalSubsampling_UL, 4);
		Ret->SetUInt(VerticalSubsampling_UL, 1);
	}
	else
	{
		if(isS314M)
		{
			Ret->SetUInt(HorizontalSubsampling_UL, 4);
			Ret->SetUInt(VerticalSubsampling_UL, 1);
		}
		else
		{
			Ret->SetUInt(HorizontalSubsampling_UL, 2);
			Ret->SetUInt(VerticalSubsampling_UL, 2);
		}
	}

	Ret->SetUInt(ColorSiting_UL, 0);				// Co-sited

	return Ret;
}


//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
/*! \note The file position pointer is moved to the start of the chunk at the end of 
 *		  this function, but CurrentPos points to the start of the next edit unit
 *
 *	\note PictureNumber is incremented for each picture 'read'
 *
 *  TODO: Currently assumes 25Mbit - needs fixing
 */
Length DV_DIF_EssenceSubParser::ReadInternal(FileHandle InFile, UInt32 Stream, UInt64 Count)
{	
	// Return anything remaining if clip wrapping
	if((Count == 0) && (SelectedWrapping->ThisWrapType == WrappingOption::Clip)) Count = ((DIFEnd - DIFStart) / (150 * 80)) - PictureNumber;

	// Simple version - we are working in our native edit rate
	if((SelectedEditRate.Denominator == NativeEditRate.Denominator) && (SelectedEditRate.Numerator = NativeEditRate.Numerator))
	{
		// Seek to the data position
		Position ReadStart = DIFStart + (150 * 80 * SeqCount * PictureNumber);
		FileSeek(InFile, ReadStart);

		// Work out how many bytes to read
		Length Ret = (Length)(Count * 150 * 80 * SeqCount);
		PictureNumber += Count;

		// If this would read beyond the end of the file stop at the end
		if((Ret + ReadStart) > DIFEnd)
		{
			Ret = DIFEnd - ReadStart;
			PictureNumber = (DIFEnd - DIFStart) / (150 * 80 * SeqCount);
		}

		// Return the number of bytes to read
		return Ret;
	}

	error("Non-native edit rate not yet supported\n");
	return 0;
}


//! Set a parser specific option
/*! \return true if the option was successfully set */
bool DV_DIF_EssenceSubParser::SetOption(std::string Option, Int64 Param /*=0*/ )
{
	warning("DV_DIF_EssenceSubParser::SetOption(\"%s\", Param) not a known option\n", Option.c_str());

	return false; 
}



