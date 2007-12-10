/*! \file	esp_jp2k.cpp
 *	\brief	Implementation of class that handles parsing of JPEG 2000 files
 *
 *	\version $Id: esp_jp2k.cpp,v 1.14 2007/12/10 23:34:25 terabrit Exp $
 *
 */
/*
 *	Copyright (c) 2005, Matt Beard
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

#include <mxflib/esp_jp2k.h>


//! Local definitions
namespace
{
	//! Modified UUID for <Source Type>
	const UInt8 JP2K_Format[] = { 0x45, 0x54, 0x57, 0x62,  0xd6, 0xb4, 0x2e, 0x4e,  0xf3, 'j', 'p', '2',  'k', 0x00, 0x00, 0x00 };

	//! Array of known, or presumed, marker segments
	/*! All other markers followed by another marker
	 */
	static bool MarkerSegments[256] =
	{
		// 0xff00 is never used
		false,

		// 0xff01 is the temporary marker
			   false, 
		
		// 0xff02 to 0xff2f are reserved, but assumed to be marker segments
					  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  

		// 0xff30 to 0xff3f are reserved for non-segment margers
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, 

		// 0xff40 to 0xff6f, JPEG 2000 markers - all assumed to be marker segments unless specified otherwise
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false,  
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  

		// 0xff70 to 0xff8f are reserved, but assumed to be marker segments
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  
		
		// 0xff90 to 0xff93, JPEG 2000 markers
		true,  true,  false, false,  

		// 0xff94 to 0xffbf are reserved, but assumed to be marker segments
									true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  

		// 0xffc0 to 0xffcf, ISO/IEC 10918-1 marker segments
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  

		// 0xffd0 to 0xffd9, ISO/IEC 10918-1 non-segment markers
		false, false, false, false, false, false, false, false, false, false, 

		// 0xffda to 0xfffe, ISO/IEC 10918-1 marker segments
																			  true,  true,  true,  true,  true,  true,  
		true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  
		
		// 0xfff0 to 0xfff6, ISO/IEC 10918-3 marker segments
		true,  true,  true,  true,  true,  true,  

		// 0xfff7 to 0xfff8, ISO/IEC 14495-1 marker segments
		                                          true,  true,
		
		// 0xff02 to 0xff2f are reserved, but assumed to be marker segments
															    true,  true,  true,  true,  true,  true,  true,  

		// 0xffff is never used
																												 false
	};
}


//! Examine the open file and return a list of essence descriptors
/*! \note This call will modify properties SampleRate, DataStart and DataSize */
EssenceStreamDescriptorList mxflib::JP2K_EssenceSubParser::IdentifyEssence(FileHandle InFile)
{
	// ".JP2" Signature box
	const UInt8 JP2_Signature[] = { 0x00, 0x00, 0x00, 0x0c, 0x6a, 0x50, 0x20, 0x20, 0x0d, 0x0a, 0x87, 0x0a };

	// The first 4 bytes of a JPEG 2000 codestream are always the same and are a poor, but usable, signature
	const UInt8 J2C_Signature[] = { 0xff, 0x4f, 0xff, 0x51 };

	int BufferBytes;
	UInt8 Buffer[12];

	EssenceStreamDescriptorList Ret;

	// Read the first 12 bytes of the file to allow us to identify it
	FileSeek(InFile, 0);
	BufferBytes = (int)FileRead(InFile, Buffer, 12);

	// If the file is smaller than 12 bytes give up now!
	if(BufferBytes < 12) return Ret;

	// If the file doesn't start with the signature box if can't be a jp2 file
	if( memcmp(Buffer, JP2_Signature, 12) != 0 )
	{
		// But it could be a JPEG 2000 codestream file
		if(memcmp(Buffer, J2C_Signature, 4) != 0) return Ret;

		MDObjectPtr DescObj = BuildDescriptorFromCodeStream(InFile, 0);

		// Quit here if we couldn't build an essence descriptor
		if(!DescObj) return Ret;

		/* Note that this code is currently the same as that below the if block
		 * This may change if we process the the two formats differently
		 */

		// Build a descriptor with a zero ID (we only support single stream files)
		EssenceStreamDescriptorPtr Descriptor = new EssenceStreamDescriptor;
		Descriptor->ID = 0;
		Descriptor->Description = "JPEG 2000 Image data";
		Descriptor->SourceFormat.Set(JP2K_Format);
		Descriptor->Descriptor = DescObj;

		// Record a pointer to the descriptor so we can check if we are asked to process this source
		CurrentDescriptor = DescObj;

		// Set the single descriptor
		Ret.push_back(Descriptor);

		return Ret;
	}

	MDObjectPtr DescObj = BuildDescriptorFromJP2(InFile);

	// Quit here if we couldn't build an essence descriptor
	if(!DescObj) return Ret;

	// Build a descriptor with a zero ID (we only support single stream files)
	EssenceStreamDescriptorPtr Descriptor = new EssenceStreamDescriptor;
	Descriptor->ID = 0;
	Descriptor->Description = "JPEG 2000 Image data";
	Descriptor->SourceFormat.Set(JP2K_Format);
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
WrappingOptionList mxflib::JP2K_EssenceSubParser::IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor &Descriptor)
{
	// TODO: Fill in the base wrapping UL
	UInt8 BaseUL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x07, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0c, 0x01, 0x00 };
	WrappingOptionList Ret;

	// If the source format isn't <Source Type> then we can't wrap the essence
	if(memcmp(Descriptor.SourceFormat.GetValue(), JP2K_Format, 16) != 0) return Ret;

	// The identify step configures some member variables so we can only continue if we just identified this very source
	if((!CurrentDescriptor) || (Descriptor.Descriptor != CurrentDescriptor)) return Ret;

	// TODO: Only fill in the supported wrapping types

	// Build a WrappingOption for clip wrapping
	WrappingOptionPtr ClipWrap = new WrappingOption;

	ClipWrap->Handler = this;							// Set us as the handler
	ClipWrap->Description = "SMPTE 422M clip wrapping of JPEG 2000 image data";

	BaseUL[14] = 0x02;									// Clip wrapping
	ClipWrap->Name = "clip";							// Set the wrapping name
	ClipWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	ClipWrap->GCEssenceType = 0x15;						// GC Picture wrapping type
	ClipWrap->GCElementType = 0x09;						// Clip wrapped elemenet
	ClipWrap->ThisWrapType = WrappingOption::Clip;		// Clip wrapping
	ClipWrap->CanSlave = true;							// Can use non-native edit rate
	ClipWrap->CanIndex = true;							// We CANNOT currently index this essence
	ClipWrap->CBRIndex = false;							// This essence uses CBR indexing
	ClipWrap->BERSize = 0;								// No BER size forcing

	// Build a WrappingOption for frame wrapping
	WrappingOptionPtr FrameWrap = new WrappingOption;

	FrameWrap->Handler = this;							// Set us as the handler
	FrameWrap->Description = "SMPTE 422M frame wrapping of JPEG 2000 image data";

	BaseUL[14] = 0x01;									// Frame wrapping
	FrameWrap->Name = "frame";							// Set the wrapping name
	FrameWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	FrameWrap->GCEssenceType = 0x15;					// GC Picture wrapping type
	FrameWrap->GCElementType = 0x08;					// Frame wrapped elemenet
	FrameWrap->ThisWrapType = WrappingOption::Frame;	// Frame wrapping
	FrameWrap->CanSlave = true;							// Can use non-native edit rate
	FrameWrap->CanIndex = true;							// We CANNOT currently index this essence
	FrameWrap->CBRIndex = false;						// This essence uses CBR indexing
	FrameWrap->BERSize = 0;								// No BER size forcing

	// Add the two wrapping options 
	// Note: Frame wrapping is preferred
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
DataChunkPtr mxflib::JP2K_EssenceSubParser::Read(FileHandle InFile, UInt32 Stream, UInt64 Count /*=1*/)
{
	// Return value
	DataChunkPtr Ret;

	// Move to the current position
	if(CurrentPos == 0) CurrentPos = DataStart;

	FileSeek(InFile, CurrentPos);
	
	// Find out how many bytes to read
	size_t Bytes = ReadInternal(InFile, Stream, Count);

	// Clear the cached size as we are about to read it, so it will need to be recalculated
	CachedDataSize = static_cast<size_t>(-1);

	// If there is no data left return a NULL pointer as a signal
	if(!Bytes) return Ret;

	// Make a datachunk with enough space
	Ret = new DataChunk(Bytes);

	// Read the data
	FileRead(InFile, Ret->Data, Bytes);

	// Update the file pointer
	CurrentPos = FileTell(InFile);

	// Update the picture number
	PictureNumber++;

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
Length mxflib::JP2K_EssenceSubParser::Write(FileHandle InFile, UInt32 Stream, MXFFilePtr OutFile, UInt64 Count /*=1*/)
{
	const unsigned int BUFFERSIZE = 32768;
	UInt8 *Buffer = new UInt8[BUFFERSIZE];

	// Move to the current position
	if(CurrentPos == 0) CurrentPos = DataStart;
	FileSeek(InFile, CurrentPos);
	
	// Find out how many bytes to transfer
	size_t Bytes = ReadInternal(InFile, Stream, (Length)Count);
	Length Ret = static_cast<Length>(Bytes);

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
	CurrentPos = FileTell(InFile);

	// Free the buffer
	delete[] Buffer;

	return Ret; 
}


//! Read the essence information from the codestream at the specified position in the source file and build an essence descriptor
/*! \note This call will modify properties SampleRate, DataStart and DataSize */
MDObjectPtr mxflib::JP2K_EssenceSubParser::BuildDescriptorFromCodeStream(FileHandle InFile, Position Offset /*=0*/)
{
	MDObjectPtr Ret;

	// Is this codestream an RGB codestream (as opposed to CDCI) - assume so initially
	bool IsRGB = true;

	// Parse the header
	ParseJP2KCodestreamHeader(InFile, Offset);

	// Header iterator
	HeaderType::iterator it;

	// Header data size and pointer for item parsing
	size_t Size;
	UInt8 *p;

	it = Header.lower_bound("FF51");
	if(it == Header.upper_bound("FF51")) return Ret;

	// Get the data size and pointer
	Size = (size_t)(*it).second->Size;
	p = (*it).second->Data;

	if(Size < 34) return Ret;

	// Start to build the sub-descriptor
	MDObjectPtr SubDescriptor = new MDObject(JPEG2000PictureSubDescriptor_UL);
	if(!SubDescriptor) return Ret;

	// Image properties
	UInt32 Width = GetU32(&p[2]);
	UInt32 Height = GetU32(&p[6]);
	UInt32 XOsiz = GetU32(&p[10]);
	UInt32 YOsiz = GetU32(&p[14]);
	UInt32 XTOsiz = GetU32(&p[26]);
	UInt32 YTOsiz = GetU32(&p[30]);

	SubDescriptor->SetInt(Rsiz_UL, GetU16(&p[0]));
	SubDescriptor->SetInt(Xsiz_UL, Width);
	SubDescriptor->SetInt(Ysiz_UL, Height);
	SubDescriptor->SetInt(XOsiz_UL, XOsiz);
	SubDescriptor->SetInt(YOsiz_UL, YOsiz);
	SubDescriptor->SetInt(XTsiz_UL, GetU32(&p[18]));
	SubDescriptor->SetInt(YTsiz_UL, GetU32(&p[22]));
	SubDescriptor->SetInt(XTOsiz_UL, XTOsiz);
	SubDescriptor->SetInt(YTOsiz_UL, YTOsiz);

	int Components = GetU16(&p[34]);
	SubDescriptor->SetInt(Csiz_UL, Components);

	if((Size - 36) < (size_t)(Components * 3)) return Ret;

	// Index the start of the components
	p += 36;

	// Component bit-depths and relative sizes
	const int MaxComponents = 32;
	int CDepth[MaxComponents];
	int XRsiz[MaxComponents];
	int YRsiz[MaxComponents];
	int ComponentCount = 0;
	if(Components > MaxComponents) error("Maximum number of supported JPEG 2000 image components is %d. This image contains %d\n", MaxComponents, Components);

	// Add the component data
	MDObjectPtr Array = SubDescriptor->AddChild(PictureComponentSizing_UL);
	int ComponentsRemaining = Components;
	while(ComponentsRemaining)
	{
		MDObjectPtr Item = Array->AddChild();
		if(Item) 
		{
			// If any component is signed we assume it is CDCI rather than RGB
			if((*p) & 0x80) IsRGB = false;

			if(ComponentCount < MaxComponents)
			{
				CDepth[ComponentCount] = (*p) & 0x7f;
				XRsiz[ComponentCount] = p[1];
				YRsiz[ComponentCount] = p[2];
				ComponentCount++;
			}
			Item->SetInt("Ssiz", *(p++));
			Item->SetInt("XRsiz", *(p++));
			Item->SetInt("YRsiz", *(p++));
		}
		ComponentsRemaining--;
	}

	if(IsRGB)
	{
		Ret = new MDObject(RGBAEssenceDescriptor_UL);
		if(!Ret) return Ret;

		if(Components < 1) Ret->SetInt(ComponentDepth_UL, 0);
		else Ret->SetInt(ComponentDepth_UL, CDepth[0] + 1);

		MDObjectPtr PixelLayout = Ret->AddChild(PixelLayout_UL);
		if(PixelLayout)
		{
			DataChunk Buffer(ComponentCount*2);
			int Count = 0;
			UInt8 *p = Buffer.Data;
			while(Count < ComponentCount)
			{
				// TODO: Figure out a way to determine the proper component order
				char c = '?';
				if(Count == 0) c = 'R';
				else if(Count == 1) c = 'G';
				else if(Count == 2) c = 'B';
				else if(Count == 3) c = 'A';

				*(p++) = (UInt8)c;
				*(p++) = (UInt8)CDepth[Count] + 1;
				Count++;
			}
			PixelLayout->SetValue(Buffer);
		}
	}
	else
	{
		Ret = new MDObject(CDCIEssenceDescriptor_UL);
		if(!Ret) return Ret;

		if(Components < 1) Ret->SetInt(ComponentDepth_UL, 0);
		else Ret->SetInt(ComponentDepth_UL, CDepth[0] + 1);

		if((Components < 2) || (XRsiz[0] == 0)) Ret->SetInt(HorizontalSubsampling_UL, 0);
		else Ret->SetInt(HorizontalSubsampling_UL, XRsiz[1] / XRsiz[0]);

		if((Components >= 2) && (XRsiz[0] != 0)) Ret->SetInt(VerticalSubsampling_UL, XRsiz[1] / XRsiz[0]);

		// Assume component 4 is alpha
		if(Components >= 4) Ret->SetInt(AlphaSampleDepth_UL, CDepth[3] + 1);
	}


	/* File Descriptor items */

	// Set 24Hz as the default sample rate
	Ret->SetString(SampleRate_UL, "24/1");


	/* Picture Essence Descriptor Items */

	Ret->SetUInt(FrameLayout_UL, 0);

	Ret->SetUInt(StoredWidth_UL, Width-XTOsiz);
	Ret->SetUInt(StoredHeight_UL, Height-YTOsiz);
	Ret->SetUInt(SampledWidth_UL, Width);
	Ret->SetUInt(SampledHeight_UL, Height);
	Ret->SetUInt(SampledXOffset_UL, XTOsiz);
	Ret->SetUInt(SampledYOffset_UL, YTOsiz);
	Ret->SetUInt(DisplayWidth_UL, Width-XOsiz);
	Ret->SetUInt(DisplayHeight_UL, Height-YOsiz);
	Ret->SetUInt(DisplayXOffset_UL, XOsiz);
	Ret->SetUInt(DisplayYOffset_UL, YOsiz);

	MDObjectPtr AspectItem = Ret->AddChild(AspectRatio_UL);
	if(AspectItem)
	{
		// TODO: Find a way to compensate for non-square pixels

		UInt32 Aspect_n = Width-XOsiz;
		UInt32 Aspect_d = Height-YOsiz;

		ReduceRational(Aspect_n, Aspect_d);

		AspectItem->SetInt("Numerator", (Int32)Aspect_n);
		AspectItem->SetInt("Denominator", (Int32)Aspect_d);
	}

	MDObjectPtr VLMItem = Ret->AddChild(VideoLineMap_UL);
	if(VLMItem)
	{
		MDObjectPtr VLMChild = VLMItem->AddChild();
		if( VLMChild ) VLMChild->SetInt(1);
	}

	// TODO: Add alpha transparency?

	
	// Link the sub-descrioptor to the file descriptor
	MDObjectPtr Link = Ret->AddChild(SubDescriptors_UL);
	if(Link) Link = Link->AddChild();
	if(Link) Link->MakeRef(SubDescriptor);

	return Ret;
}


//! Read the essence information at the start of the "JP2" format source file and build an essence descriptor
/*! \note This call will modify properties SampleRate, DataStart and DataSize */
MDObjectPtr mxflib::JP2K_EssenceSubParser::BuildDescriptorFromJP2(FileHandle InFile)
{
	MDObjectPtr Ret;

	// Parse the header
	ParseJP2Header(InFile);

	// If we didn't find any codestream data there is no point going any further
	if(DataStart == 0) return Ret;

	// Header iterator
	HeaderType::iterator it;

	// Header data size and pointer for item parsing
	size_t Size;
	UInt8 *p;

	it = Header.lower_bound("ftyp");
	if(it == Header.upper_bound("ftyp")) return Ret;

	/* Check that we support this file type */

	Size = (size_t)(*it).second->Size;
	if(Size < 12) return Ret;

	// Scan the file types for "jp2 "
	// Note that we don't skip the minor version field so a minor version of 0x6a703220 would cause a false-positive!
	p = (*it).second->Data;
	for(;;)
	{
		if(memcmp(p, "jp2 ", 4) == 0) break;
		if(Size >= 4) Size -= 4;
		else return Ret;
	}
	
	Ret = BuildDescriptorFromCodeStream(InFile, DataStart);
	
	return Ret;
}


//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
/*! \note The file position pointer is left at the start of the chunk at the end of 
 *		  this function
 */
size_t mxflib::JP2K_EssenceSubParser::ReadInternal(FileHandle InFile, UInt32 Stream, Length Count) 
{ 
	// TODO: Add better error reporting - it currently just exits on most errors
	Length Ret = 0;

	// Return the cached value if we have not yet used it
	if(CachedDataSize != static_cast<size_t>(-1)) return CachedDataSize;

	// Move to the current position
	if(CurrentPos == 0) CurrentPos = DataStart;
	FileSeek(InFile, CurrentPos);

	// If the size is known (possible in a JP2 file) we return it
	if(DataSize != 0)
	{
		Ret = DataSize - (CurrentPos - DataStart);
		if(Ret < 0) Ret = 0;

		// Store so we don't have to calculate if called again without reading
		CachedDataSize =  static_cast<size_t>(Ret);
		
		return CachedDataSize;
	}

	// If the size is unknown we assume the rest of the file is data
	// DRAGONS: Should work even for JP2 files as an "unknown" length must be the last item in a JP2 file
	FileSeekEnd(InFile);
	Ret = static_cast<Length>(FileTell(InFile) - CurrentPos);
	
	// Move back to the current position
	FileSeek(InFile, CurrentPos);

	// If we have an index manager we need to perform indexing operations
	if((Ret != 0) && (Manager))
	{
		// Offer this index table data to the index manager
		Manager->OfferEditUnit(ManagedStreamID, PictureNumber, 0, 0x80);
	}

	// Validate the size
	if((sizeof(size_t) < 8) && (Ret > 0xffffffff))
	{
		error("This edit unit > 4GBytes, but this platform can only handle <= 4GByte chunks\n");
		Ret = 0;
	}

	// Store so we don't have to calculate if called again without reading
	CachedDataSize =  static_cast<size_t>(Ret);
	
	return CachedDataSize;

/* Codestream scanning code...

// Maximum chunk size for a single read
//	const size_t MaxChunkSize = 1 * 1024 * 1024;
const size_t MaxChunkSize = 77;

	DataChunkPtr Buffer = new DataChunk(MaxChunkSize);

	// Our processing position
	Position Pos;

	// Move to the current position
	if(CurrentPos == 0) CurrentPos = DataStart;
	Pos = CurrentPos;
	FileSeek(InFile, Pos);

	// Read the first chunk
	size_t Bytes = (size_t)FileRead(InFile, Buffer->Data, MaxChunkSize);
	if(Bytes < MaxChunkSize) Buffer->Resize(Bytes);
printf("@ 0x%08x Read 0x%08x bytes\n", (int)Pos, (int)Bytes);

	// Quit if less than 2 bytes available
	if(Bytes < 2) return Ret;

	// Verify that the first byte of the first marker is 0xff
	UInt8 *p = Buffer->Data;
	if((*p++) != 0xff) return Ret;

	// Read the first marker
	UInt8 Marker = *(p++);

	// Two bytes found already
	Ret += 2;
	Pos += 2;
	Bytes -= 2;
	for(;;)
	{
printf("Marker 0x%02x:\n", (int)Marker);
		// Parsing ends once we read EOC
		if(Marker == 0xd9) break;

		// If we have a segment skip over it
		if(MarkerSegments[Marker])
		{
printf("  Is a segment\n");
			// If there are not enough bytes left for the length, restart the scan at this marker
			if(Bytes < 2)
			{
printf("  Bytes remaining %d so will re-read\n", Bytes);
			// Seek to the current position
			FileSeek(InFile, Pos);

				// Read the next chunk starting at the byte following the current marker
				size_t Bytes = (size_t)FileRead(InFile, Buffer->Data, MaxChunkSize);
				if(Bytes < MaxChunkSize) Buffer->Resize(Bytes);
printf("  @ 0x%08x Read 0x%08x bytes\n", (int)Pos, (int)Bytes);

				// Quit if less than 2 bytes available
				if(Bytes < 2) return Ret;

				// Reset the pointer
				p = Buffer->Data;

				continue;
			}

			// Read the length
			UInt16 SegmentLength = GetU16(p);
			ASSERT(SegmentLength > 2);
printf("  Length = 0x%04x\n", SegmentLength);

			// Add the segment length to the byte count
			Ret += SegmentLength;

			// Skip over the value (and the length)
			Pos += SegmentLength;
printf("  Now @ 0x%08x\n", (int)Pos);

			// Reduce the count of bytes left in the buffer
			if(Bytes >= SegmentLength) 
			{
				Bytes-= SegmentLength;
				p += SegmentLength;
			}
			else Bytes = 0;
		}

		// If there are not enough bytes left for a marker, restart the scan at the current position
		if(Bytes < 2)
		{
printf("Bytes remaining %d so will re-read\n", Bytes);
			// Seek to the current position
			FileSeek(InFile, Pos);

			// Read the next chunk starting at the start of the next marker
			Bytes = (size_t)FileRead(InFile, Buffer->Data, MaxChunkSize);
			if(Bytes < MaxChunkSize) Buffer->Resize(Bytes);
printf("@ 0x%08x Read 0x%08x bytes\n", (int)Pos, (int)Bytes);

			// Quit if less than 2 bytes available
			if(Bytes < 2) return Ret;

			// Reset the pointer
			p = Buffer->Data;
		}

		// Validate the first byte of the marker
		if((*p++) != 0xff) return Ret;

		// Read the marker
		Marker = *(p++);

		// Add the next marker bytes
		Pos += 2;
		Ret += 2;
		Bytes -= 2;
	}

printf("Read Done\n");

	// TODO: Determine if this is the end of the file

	return Ret;
*/
}


//! Parse a JP2 header at the start of the specified file into items in the Header multimap
bool mxflib::JP2K_EssenceSubParser::ParseJP2Header(FileHandle InFile)
{
	// Known superboxes
	static const char *SuperBoxes[] = { "jp2h", "res ", "uinf", NULL };

	// Clear any existing header data
	Header.clear();

	// Clear the data pointers
	DataStart = 0;
	CurrentPos = 0;
	DataSize = 0;

	// The name of any parent box (including its parent names)
	std::string Parent = "";

	// List of parent lengths
	std::list<Length> RemainingStack;

	// Flag that the rest of the file is currently available
	Length Remaining = -1;

	FileSeek(InFile, 0);
	while(!FileEof(InFile))
	{
		int Bytes = 0;
		char BoxName[5];

		// Read the box length
		Length BoxLength = ReadU32(InFile);
		
		// Read the box name
		FileRead(InFile, (UInt8*)BoxName, 4);
		BoxName[4] = 0;

		// Count bytes used so far
		Bytes = 8;

		// Read extended length if used
		if(BoxLength == 1)
		{
			BoxLength = ReadI64(InFile);

			// Adjust byte count
			Bytes += 8;
		}

		// Adjust remaining byte count
		if(Remaining >= 0) 
		{
			Remaining -= Bytes;
			if(Remaining < 0) Remaining = 0;
		}

		// If the length is unknown treat it as all remaining bytes, otherwise adjust for the bytes used so far
		if(BoxLength == 0) BoxLength = Remaining; 
		else BoxLength -= Bytes;

		// Parsing ends once the first codestream is found
		if(strcmp(BoxName, "jp2c") == 0)
		{
			DataStart = FileTell(InFile);
			CurrentPos = DataStart;
			if(BoxLength < 2) DataSize = 0;
			else DataSize = BoxLength - 2;

			return true;
		}

		// See if this is a superbox
		bool SuperBox = false;
		const char **p = SuperBoxes;
		while(*p != NULL)
		{
			if(strcmp(BoxName, *p) == 0)
			{
				SuperBox = true;
				break;
			}
			p++;
		}

		if(SuperBox)
		{
			// Add us as a parent
			if(Parent.size() == 0) Parent = BoxName;
			else Parent += std::string("/") + BoxName;

			// Stack the number of bytes remaining after this box
			if(Remaining < 0) RemainingStack.push_back(Remaining);
			else RemainingStack.push_back(Remaining - BoxLength);

			// The new "remaining bytes" is the box length
			Remaining = BoxLength;
		}
		else
		{
			// DRAGONS: Currently limit to 1k
			// TODO: We should probably report errors if the box is bogger
			const int MaxBoxSize = 1024;

			int ReadLength;
			if((BoxLength > MaxBoxSize) || (BoxLength < 0)) ReadLength = MaxBoxSize;
			else ReadLength = (int)BoxLength;

			DataChunkPtr ThisData = new DataChunk(ReadLength);
			int Bytes = (int)FileRead(InFile, ThisData->Data, ReadLength);

			// Resize the value if not all bytes read
			if(Bytes != ReadLength) ThisData->Resize(Bytes);

			if(Remaining > 0)
			{
				Remaining -= Bytes;
				if(Remaining < 0) Remaining = 0;
			}

			std::string FullBoxName;
			if(Parent.size() == 0) FullBoxName = BoxName;
			else FullBoxName = Parent + std::string("/") + std::string(BoxName);

			Header.insert(HeaderType::value_type(FullBoxName, ThisData));
		}

		/* Are we done yet? */

		// EOF will end parsing completely
		if(FileEof(InFile)) break;

		// End parsing of this box if required
		if(Remaining == 0)
		{
			// Remove the last name from the parent list
			if(Parent.size() > 5) Parent = Parent.substr(0, Parent.size() - 6);
			else Parent = "";

			if(RemainingStack.size())
			{
				std::list<Length>::iterator it = RemainingStack.end();
				it--;
				Remaining = (*it);
				RemainingStack.erase(it);
			}
			else
			{
				Remaining = -1;
			}
		}
	}

	return true;
}


//! Parse a JPEG 2000 header at the specified offset in a file into items in the Header multimap
/*! This parsing includes the first tile-part header
 */
bool mxflib::JP2K_EssenceSubParser::ParseJP2KCodestreamHeader(FileHandle InFile, Position Offset)
{
	// Clear any existing header data
	Header.clear();

	FileSeek(InFile, Offset);
	
	// Verify that the first byte of the first marker if 0xff
	UInt8 Start = ReadU8(InFile);
	if(Start != 0xff) return false;

	// Read the first marker
	UInt8 Marker = ReadU8(InFile);

	while(!FileEof(InFile))
	{
		// Parsing ends once we read SOD
		if(Marker == 0x93)
		{
			return true;
		}

		bool isSegment = true;

		// Determine if there is a marker segment
		UInt16 SegmentLength = ReadU16(InFile);
		ASSERT(SegmentLength > 2);

		// Looks like no segment (or it could be a long length)
		if(SegmentLength >= 0xff00)
		{
			isSegment = MarkerSegments[Marker];				// Check with the marker segment array
		}

		// Build the segment name
		char SegmentName[5];
		sprintf(SegmentName, "FF%2X", Marker);

		// Data for this segment
		DataChunkPtr ThisData;

		if(isSegment)
		{
			SegmentLength -= 2;

			ThisData = new DataChunk(SegmentLength);
			int Bytes = (int)FileRead(InFile, ThisData->Data, SegmentLength);

			// Resize the value if not all bytes read
			if(Bytes != SegmentLength) ThisData->Resize(Bytes);
		}
		else
			ThisData = new DataChunk();

		// Insert this segment
		Header.insert(HeaderType::value_type(SegmentName, ThisData));

		if(isSegment)
		{
			// Verify that the first byte of the next marker if 0xff
			UInt8 Start = ReadU8(InFile);
			if(Start != 0xff) return false;

			// Read the next marker
			Marker = ReadU8(InFile);
		}
		else
		{
			// The next marker is currently in the low byte of the length (as this is not a segment it has no length)
			Marker = (UInt8)SegmentLength;
		}
	}

	return true;
}
