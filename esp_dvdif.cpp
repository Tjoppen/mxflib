/*! \file	esp_dvdif.cpp
 *	\brief	Implementation of class that handles parsing of DV-DIF streams
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

#include "mxflib.h"

#include <math.h>	// For "floor"

using namespace mxflib;



//! Examine the open file and return a list of essence descriptors
EssenceStreamDescriptorList DV_DIF_EssenceSubParser::IdentifyEssence(FileHandle InFile)
{
	int BufferBytes;
//	Uint8 Buffer[12]; // Use the object's buffer!

	EssenceStreamDescriptorList Ret;

	// Read the first 12 bytes of the file to allow us to identify it
	FileSeek(InFile, 0);
	BufferBytes = FileRead(InFile, Buffer, 12);

	// If the file is smaller than 12 bytes give up now!
	if(BufferBytes < 12) return Ret;

	// If the file starts with "RIFF" if could be an AVI DV file
	if((Buffer[0] == 'R') && (Buffer[1] == 'I') && (Buffer[2] == 'F') && (Buffer[3] == 'F'))
	{
		// Just because the file is a RIFF file doesn't mean it's a DV AVI file!
		if((Buffer[8] != 'A') || (Buffer[9] != 'V') || (Buffer[10] != 'I') || (Buffer[11] != ' ')) return Ret;

		// So its an AVI file.. but what type?
		const unsigned int ID_LIST = 0x4B495354;		//! "LIST"
		const unsigned int ID_hdrl = 0x6864726b;		//! "hdrl"
		
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
			const unsigned int ID_strl = 0x7374726b;		//! "strl"
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
					U32 MediaType = ReadU32(InFile);
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
	BufferBytes = FileRead(InFile, Buffer, 80 * 150);

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

printf("NOTE: We have a valid DV-DIF file!\n");

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
	Descriptor.Descriptor = DescObj;

	// Set the single descriptor
	Ret.push_back(Descriptor);

	return Ret;

/*	int BufferBytes;
	Uint8 Buffer[512];
	Uint8 *BuffPtr;

	EssenceStreamDescriptorList Ret;

	// Read the first 512 bytes of the file to allow us to investigate it
	FileSeek(InFile, 0);
	BufferBytes = FileRead(InFile, Buffer, 512);
	
	// If the file is smaller than 16 bytes give up now!
	if(BufferBytes < 16) return Ret;

	// If the file doesn't start with two zeros the it doesn't start
	// with a start code and so it can't be a valid MPEG2-VES file
	if((Buffer[0] != 0) || (Buffer[1] != 0)) return Ret;

	// Scan for the first start code
	BuffPtr = &Buffer[2];
	int StartPos = 0;						//!< Start position of sequence header (when found)
	int ScanLeft = BufferBytes - 3;			//!< Number of bytes left in buffer to scan
	while(!(*BuffPtr))
	{
		if(!--ScanLeft) break;
		StartPos++;
		BuffPtr++;
	}

	// If we haven't found a start code then quit
	if(*BuffPtr != 1) return Ret;

	// Check what type of start code we have found
	// Only accept MPEG2-VES which will always start with a sequence header
	BuffPtr++;
	if(*BuffPtr != 0xb3) return Ret;

	MDObjectPtr DescObj = BuildMPEG2VideoDescriptor(InFile, StartPos);
	
	// Quit here if we couldn't build an essence descriptor
	if(!DescObj) return Ret;

	// Build a descriptor with a zero ID (we only support single stream files)
	EssenceStreamDescriptor Descriptor;
	Descriptor.ID = 0;
	Descriptor.Description = "DV video essence";
	Descriptor.Descriptor = DescObj;

	// Set the single descriptor
	Ret.push_back(Descriptor);

	return Ret;
	*/
}


//! Examine the open file and return the wrapping options known by this parser
/*! \param InFile The open file to examine (if the descriptor does not contain enough info)
 *	\param Descriptor An essence stream descriptor (as produced by function IdentifyEssence)
 *		   of the essence stream requiring wrapping
 *	\note The options should be returned in an order of preference as the caller is likely to use the first that it can support
 */
WrappingOptionList DV_DIF_EssenceSubParser::IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor Descriptor)
{
	Uint8 BaseUL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x02, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x02, 0x7f, 0x01 };
	WrappingOptionList Ret;

	// If the supplied descriptor isn't an CDCI Essence Descriptor then we can't wrap the essence
	if(Descriptor.Descriptor->Name() != "CDCIEssenceDescriptor") return Ret;

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
	FrameWrap->BERSize = 0;								// No BER size forcing

	// Add the two wrapping options
	Ret.push_back(ClipWrap);
	Ret.push_back(FrameWrap);

	return Ret;
}


//! Set a wrapping option for future Read and Write calls
void DV_DIF_EssenceSubParser::Use(Uint32 Stream, WrappingOptionPtr UseWrapping)
{
	SelectedWrapping = UseWrapping->ThisWrapType;
	SelectedEditRate = NativeEditRate;
	EditRatio = 1;
	PictureNumber = 0;
//	AnchorFrame = 0;
	CurrentPos = 0;
//	GOPOffset = 0;
//	ClosedGOP = false;					// Start by assuming the GOP is closed
//	IndexMap.clear();
}


//! Set a non-native edit rate
/*! \return true if this rate is acceptable */
bool DV_DIF_EssenceSubParser::SetEditRate(Uint32 Stream, Rational EditRate)
{
	if(    (EditRate.Numerator == NativeEditRate.Numerator) 
		&& (EditRate.Denominator == NativeEditRate.Denominator) )return true;

	// We can clip-wrap at any rate!
	if(SelectedWrapping == WrappingOption::Clip)
	{
		SelectedEditRate = EditRate;
		return true;
	}

	// Prevent divide by zero
	if(NativeEditRate.Denominator == 0) return false;
	if(EditRate.Denominator == 0) return false;

	double FloatNative = double(NativeEditRate.Numerator) / double(NativeEditRate.Denominator);
	double FloatUse = double(EditRate.Numerator) / double(EditRate.Denominator);

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
		return true;
	}

	return false;
}


//! Read a number of wrapping items from the specified stream and return them in a data chunk
/*! If frame or line mapping is used the parameter Count is used to
 *	determine how many items are read. In frame wrapping it is in
 *	units of EditRate, as specified in the call to Use(), which may
 *  not be the frame rate of this essence
 *	\note This is going to take a lot of memory in clip wrapping! 
 */
DataChunkPtr DV_DIF_EssenceSubParser::Read(FileHandle InFile, Uint32 Stream, Uint64 Count /*=1*/, IndexTablePtr Index /*=NULL*/) 
{ 
	// Scan the stream and find out how many bytes to read
	Uint64 Bytes = ReadInternal(InFile, Stream, Count, Index);

	// Read the data
	return FileReadChunk(InFile, Bytes);
}


//! Write a number of wrapping items from the specified stream to an MXF file
/*! If frame or line mapping is used the parameter Count is used to
 *	determine how many items are read. In frame wrapping it is in
 *	units of EditRate, as specified in the call to Use(), which may
 *  not be the frame rate of this essence stream
 *	\note This is the only safe option for clip wrapping
 *	\return Count of bytes transferred
 */
Uint64 DV_DIF_EssenceSubParser::Write(FileHandle InFile, Uint32 Stream, MXFFilePtr OutFile, Uint64 Count /*=1*/, IndexTablePtr Index /*=NULL*/)
{
	const unsigned int BUFFERSIZE = 32768;
	Uint8 *Buffer = new Uint8[BUFFERSIZE];

	// Scan the stream and find out how many bytes to transfer
	Uint64 Bytes = ReadInternal(InFile, Stream, Count, Index);
	Uint64 Ret = Bytes;

	while(Bytes)
	{
		Uint64 ChunkSize;
		
		// Number of bytes to transfer in this chunk
		if(Bytes < BUFFERSIZE) ChunkSize = Bytes; else ChunkSize = BUFFERSIZE;

		FileRead(InFile, Buffer, ChunkSize);
		OutFile->Write(Buffer, ChunkSize);

		Bytes -= ChunkSize;
	}

	return Ret; 
}


//! Read the header at the specified position in a DV file to build an essence descriptor
/*! DRAGONS: Currently rather scrappy */
MDObjectPtr DV_DIF_EssenceSubParser::BuildCDCIEssenceDescriptor(FileHandle InFile, Uint64 Start /*=0*/)
{
	MDObjectPtr Ret;
//	Uint8 Buffer[80];	Use existing buffer!

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

	Ret = new MDObject("CDCIEssenceDescriptor");
	if(!Ret) return Ret;

	if(!is625)
	{
		Ret->SetString("SampleRate", "30000/1001");

		NativeEditRate.Numerator = 30000;
		NativeEditRate.Denominator = 1001;
	}
	else
	{
		Ret->SetString("SampleRate", "25/1");

		NativeEditRate.Numerator = 25;
		NativeEditRate.Denominator = 1;
	}

printf("Assumed interleaved...\n");
//	if(Progressive) Ret->SetInt("FrameLayout", 0); else Ret->SetInt("FrameLayout", 1);
	Ret->SetInt("FrameLayout", 1);

	if(is625)
	{
		Ret->SetUint("StoredWidth", 720);
		Ret->SetUint("StoredHeight", 288);
	}
	else	
	{
		Ret->SetUint("StoredWidth", 720);
		Ret->SetUint("StoredHeight", 240);
	}

printf("Assumed 4:3...\n");
//	if(Aspect) Ret->SetString("AspectRatio", Aspect); else Ret->SetDValue("AspectRatio");
	Ret->SetString("AspectRatio", "4/3");

	MDObjectPtr Ptr = Ret->AddChild("VideoLineMap");
	if(Ptr)
	{
		int F1 = 0;
		int F2 = 0;

		if(is625) { F1 = 1; F2 = 313; }
		else { F1 = 4; F2 = 266; }

		Ptr->AddChild("VideoLineMapEntry", false)->SetUint(F1);
		Ptr->AddChild("VideoLineMapEntry", false)->SetUint(F2);
	}

	Ret->SetUint("ComponentDepth", 8);

	if(!is625)
	{
		Ret->SetUint("HorizontalSubsampling", 4);
		Ret->SetUint("VerticalSubsampling", 1);
	}
	else
	{
		if(isS314M)
		{
			Ret->SetUint("HorizontalSubsampling", 4);
			Ret->SetUint("VerticalSubsampling", 1);
		}
		else
		{
			Ret->SetUint("HorizontalSubsampling", 2);
			Ret->SetUint("VerticalSubsampling", 2);
		}
	}

	Ret->SetUint("ColorSiting", 0);				// Co-sited

	return Ret;
}


//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
/*! \note The file position pointer is moved to the start of the chunk at the end of 
 *		  this function, but CurrentPos points to the start of the next edit unit
 *
 *	\note PictureNumber is incremented for each picture 'read'
 *
 *  \note	Currently assumes 25Mbit
 */
Uint64 DV_DIF_EssenceSubParser::ReadInternal(FileHandle InFile, Uint32 Stream, Uint64 Count, IndexTablePtr Index /*=NULL*/)
{	
	// Return anything we can find if clip wrapping
	if((Count == 0) && (SelectedWrapping == WrappingOption::Clip)) return DIFEnd - DIFStart;

	// Simple version - we are working in our native edit rate
	if((SelectedEditRate.Denominator == NativeEditRate.Denominator) && (SelectedEditRate.Numerator = NativeEditRate.Numerator))
	{
printf("Reading %llu bytes at %llu:0x%08llx\n",(150 * 80 * Count), PictureNumber, (150 * 80 * PictureNumber));

		// Seek to the data position
		FileSeek(InFile, DIFStart + (150 * 80 * PictureNumber));

		PictureNumber += Count;

		return (Count * 150 * 80);
	}

	error("Non-native edit rate not yet supported\n");
	return 0;
}


//! Get a byte from the current stream
/*! \return -1 if end of file */
int DV_DIF_EssenceSubParser::BuffGetU8(FileHandle InFile)
{
	if(!BuffCount)
	{
		BuffCount = FileRead(InFile, Buffer, MPEG2_VES_BUFFERSIZE);
		if(BuffCount == 0) return -1;

		BuffPtr = Buffer;
	}

	BuffCount--;
	return *(BuffPtr++);
}


//! Set a parser specific option
/*! \return true if the option was successfully set */
bool DV_DIF_EssenceSubParser::SetOption(std::string Option, Int64 Param /*=0*/ )
{
/*
	if(Option == "GOPIndex")
	{
		if(Param == 0) GOPIndex = false; else GOPIndex = true;
		return true;
	}

	if(Option == "SelectiveIndex")
	{
		if(Param == 0) SelectiveIndex = false; else SelectiveIndex = true;
		return true;
	}

	if(Option == "SingleShotIndex")
	{
		if(Param == 0)
		{
			SingleShotIndex = false;
		}
		else 
		{
			SingleShotIndex = true;
			SingleShotPrimed = true;
		}
	}

	if(Option == "EditPoint") return EditPoint;

	if(Option == "AddIndexEntry")
	{
		if(ProvisionalIndexEntry)
		{
			WorkingIndex->AddNewEntry(ProvisionalEssencePos, ProvisionalIndexPos, ProvisionalIndexEntry);
			ProvisionalIndexEntry = NULL;
			return true;
		}

		// Nothing to add so return error state
		return false;
	}
*/
	return false; 
}

