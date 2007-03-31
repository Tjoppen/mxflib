/*! \file	esp_dvdif.cpp
 *	\brief	Implementation of class that handles parsing of DV-DIF streams
 *
 *	\version $Id: esp_dvdif.cpp,v 1.19 2007/03/31 14:29:42 matt-beard Exp $
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

#include <mxflib/esp_dvdif.h>


//! Local definitions
namespace
{
	//! Modified UUID for raw DV
	const UInt8 DV_DIF_RAW_Format[] = { 0x45, 0x54, 0x57, 0x62,  0xd6, 0xb4, 0x2e, 0x4e,  0xf3, 0xd2, 0xfa, 'R',  'A', 'W', 'D', 'V' };

	//! Modified UUID for AVI-wrapped DV
	const UInt8 DV_DIF_AVI_Format[] = { 0x45, 0x54, 0x57, 0x62,  0xd6, 0xb4, 0x2e, 0x4e,  0xf3, 0xd2, 0xfa, 'A',  'V', 'I', 'D', 'V' };

	// AVI FOURCC codes
	
	const UInt32 ID_RIFF = 0x52494646;		//! "RIFF"
	const UInt32 ID_AVIX = 0x41564958;		//! "AVIX"

	const UInt32 ID_LIST = 0x4C495354;		//! "LIST"
	const UInt32 ID_hdrl = 0x6864726c;		//! "hdrl"

	const UInt32 ID_avih = 0x61766968;		//! "avih"

	const UInt32 ID_strl = 0x7374726c;		//! "strl"
	const UInt32 ID_strh = 0x73747268;		//! "strh"
	const UInt32 ID_strf = 0x73747266;		//! "strf"
	const UInt32 ID_indx = 0x696e6478;		//! "indx"

	const UInt32 ID_dvsd = 0x64767364;		//! "dvsd"
	const UInt32 ID_DVSD = 0x44565344;		//! "DVSD"

	const UInt32 ID_dvhd = 0x64766864;		//! "dvhd"
	const UInt32 ID_DVHD = 0x44564844;		//! "DVHD"

	const UInt32 ID_dvsl = 0x6476736c;		//! "dvsl"
	const UInt32 ID_DVSL = 0x4456534c;		//! "DVSL"

	const UInt32 ID_movi = 0x6d6f7669;		//! "DVSL"

	const UInt32 ID_odml = 0x6f646d6c;		//! "odml"
	const UInt32 ID_dmlh = 0x646d6c68;		//! "dmlh"

	const UInt32 ID_00db = 0x30306462;		//! "00db" - the base for video streams
}


//! Local functions
namespace
{
	//! Decrement a number from a UInt32, stopping at 0
	inline void Decrement(UInt32 &Val, UInt32 Subtract) { if(Val <= Subtract) Val = 0; else Val -= Subtract; }
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
		FileSeek(InFile, 12);
		U32Pair Header = ReadRIFFHeader(InFile);

		// If the first item isn't a list then we are stumpted!
		if(Header.first != ID_LIST) return Ret;

		// Size of header section list
		UInt32 ListSize = Header.second;

		// Sanity check the list
		if(ListSize < 4) return Ret;

		// Must be an "hdrl" list
		if(ReadU32(InFile) != ID_hdrl) return Ret;
		Decrement(ListSize,4);

		// Initialize the video stream number
		StreamNumber = 0;

		// Find the "strl" entry
		while((ListSize > 0) && !FileEof(InFile))
		{
			U32Pair Header = ReadRIFFHeader(InFile);
			Decrement(ListSize,Header.second + 8);

			if(Header.first != ID_LIST)
			{
				// Grab the frame count as we pass
				if(Header.first == ID_avih)
				{
					FileSeek(InFile, FileTell(InFile) + 16);
					UInt8 Buffer[4];
					FileRead(InFile, Buffer, 4);
					AVIFrameCount = GetU32_LE(Buffer);
					
					// Reduce the amount that we skip forwards as we have already moved 20 bytes
					Header.second -= 20;
				}

	 			// Skip anything that is not a list
				FileSeek(InFile, FileTell(InFile) + Header.second);
			}
			else
			{
				// Work out where the end of this list is
				Position ListEnd = FileTell(InFile) + Header.second;

				// Read the list type (we are only interested in stream info lists)
				if(ReadU32(InFile) == ID_strl)
				{
					// We only support file with a stream header at the start of each strl list
					if(ReadRIFFHeader(InFile).first != ID_strh) return Ret;

					// Skip the fccType
					ReadU32(InFile);

					UInt32 MediaType = ReadU32(InFile);

					if( (MediaType == ID_dvsd) || (MediaType == ID_DVSD) )
					{
						// Record where the actual essence starts (for building descriptors)
						Position EssenceStart = FileTell(InFile) - 28;

						// Send the location of the list to the descriptor builder
						MDObjectPtr VideoDescObj = BuildCDCIEssenceDescriptorFromAVI(InFile, EssenceStart);

						// Quit here if we couldn't build an essence descriptor
						if(!VideoDescObj) return Ret;

						// Build a descriptor with a zero ID (we only support single stream files)
						EssenceStreamDescriptorPtr Descriptor = new EssenceStreamDescriptor;
						Descriptor->ID = 0;
						Descriptor->Description = "DV-DIF audio/video essence (AVI Wrapped)";
						Descriptor->SourceFormat.Set(DV_DIF_AVI_Format);
						Descriptor->Descriptor = VideoDescObj;

						MDObjectPtr AudioDescObj = BuildSoundEssenceDescriptorFromAVI(InFile, EssenceStart);

						// Return to the start of the DIF data (building the audio descriptor will probably have moved the file pointer)
						FileSeek(InFile, DIFStart);

						// Don't build the multiplex version if we failed to build the sound descriptor (or the mux descriptor)
						if(AudioDescObj)
						{
							UInt32 ChannelCount = AudioDescObj->GetUInt(ChannelCount_UL);

							MDObjectPtr MuxDescObj;

							/* If we have 2-16 channels, our preferred method is to make separate tracks for each */
							if((ChannelCount > 1) && (ChannelCount <= 16))
							{
								MuxDescObj = new MDObject(MultipleDescriptor_UL);
								if(MuxDescObj)
								{
									// Make a copy of the multi-track descriptor
									MDObjectPtr MonoDesc = AudioDescObj->MakeCopy();
									
									// Turn it into a mono descriptor
									MonoDesc->SetUInt(ChannelCount_UL, 1);
									MDObjectPtr AvgBps = MonoDesc->Child(AvgBps_UL);
									if(AvgBps) AvgBps->SetUInt(AvgBps->GetUInt() / ChannelCount);

									// Copy up the video edit rate
									MuxDescObj->SetString(SampleRate_UL, VideoDescObj->GetString(SampleRate_UL));

									MDObjectPtr SubDescriptors = MuxDescObj->AddChild(SubDescriptorUIDs_UL);
									if(SubDescriptors)
									{
										MDObjectPtr Ptr = SubDescriptors->AddChild();
										if(Ptr) Ptr->MakeRef(VideoDescObj);

										// Add one copy of the mono descriptor per channel
										while(ChannelCount--)
										{
											Ptr = SubDescriptors->AddChild();

											MDObjectPtr NewCopy = MonoDesc->MakeCopy();
											if(Ptr) Ptr->MakeRef(NewCopy);
										}
									
										// Build a descriptor with a zero ID (we only support single stream files)
										EssenceStreamDescriptorPtr MuxDescriptor = new EssenceStreamDescriptor;
										MuxDescriptor->ID = 0;
										MuxDescriptor->Description = "DV-DIF audio/video essence (AVI Wrapped)";
										MuxDescriptor->SourceFormat.Set(DV_DIF_RAW_Format);
										MuxDescriptor->Descriptor = MuxDescObj;

										// Add the multiple descriptor
										Ret.push_back(MuxDescriptor);
									}
								}
							}

							MuxDescObj = new MDObject(MultipleDescriptor_UL);
							if(MuxDescObj)
							{
								// Copy up the video edit rate
								MuxDescObj->SetString(SampleRate_UL, VideoDescObj->GetString(SampleRate_UL));

								MDObjectPtr SubDescriptors = MuxDescObj->AddChild(SubDescriptorUIDs_UL);
								if(SubDescriptors)
								{
									MDObjectPtr Ptr = SubDescriptors->AddChild();
									if(Ptr) Ptr->MakeRef(VideoDescObj);
									Ptr = SubDescriptors->AddChild();
									if(Ptr) Ptr->MakeRef(AudioDescObj);
								
									// Build a descriptor with a zero ID (we only support single stream files)
									EssenceStreamDescriptorPtr MuxDescriptor = new EssenceStreamDescriptor;
									MuxDescriptor->ID = 0;
									MuxDescriptor->Description = "DV-DIF audio/video essence (AVI Wrapped)";
									MuxDescriptor->SourceFormat.Set(DV_DIF_RAW_Format);
									MuxDescriptor->Descriptor = MuxDescObj;

									// Add the multiple descriptor
									Ret.push_back(MuxDescriptor);
								}
							}
						}

						// Add the single descriptor last so that the multiple one will be selected in preference, if allowed
						Ret.push_back(Descriptor);

						// Attempt to parse the format
						// Record a pointer to the video descriptor so we can check if we are asked to process this source
						CurrentDescriptor = VideoDescObj;

						return Ret;
					}

					if( (MediaType == ID_dvhd) || (MediaType == ID_DVHD) )
					{
						warning("HD DV formats not currently supported by esp_dvdif\n");
						return Ret;
					}
					if( (MediaType == ID_dvsl) || (MediaType == ID_DVSL) )
					{
						warning("High-Compression DV formats not currently supported by esp_dvdif\n");
						return Ret;
					}

					// We have skipped a stream, so increment the stream number
					StreamNumber++;
				}
				// Skip what is left of this list
				FileSeek(InFile, ListEnd);
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
	MDObjectPtr VideoDescObj = BuildCDCIEssenceDescriptor(InFile, 0);

	// Quit here if we couldn't build an essence descriptor
	if(!VideoDescObj) return Ret;

	// Check the size (assume the entire file is DIF data)
	DIFStart = 0;
	FileSeekEnd(InFile);
	DIFEnd = FileTell(InFile);

	// Seek to the start of the DIF data
	FileSeek(InFile, DIFStart);

	// Build a descriptor with a zero ID (we only support single stream files)
	EssenceStreamDescriptorPtr Descriptor = new EssenceStreamDescriptor;
	Descriptor->ID = 0;
	Descriptor->Description = "DV-DIF audio/video essence";
	Descriptor->SourceFormat.Set(DV_DIF_RAW_Format);
	Descriptor->Descriptor = VideoDescObj;

	MDObjectPtr AudioDescObj = BuildSoundEssenceDescriptor(InFile, 0);

	// Return to the start of the DIF data (building the audio descriptor will probably have moved the file pointer)
	FileSeek(InFile, DIFStart);

	// Don't build the multiplex version if we failed to build the sound descriptor (or the mux descriptor)
	if(AudioDescObj)
	{
		UInt32 ChannelCount = AudioDescObj->GetUInt(ChannelCount_UL);

		MDObjectPtr MuxDescObj;

		/* If we have 2-16 channels, our preferred method is to make separate tracks for each */
		if((ChannelCount > 1) && (ChannelCount <= 16))
		{
			MuxDescObj = new MDObject(MultipleDescriptor_UL);
			if(MuxDescObj)
			{
				// Make a copy of the multi-track descriptor
				MDObjectPtr MonoDesc = AudioDescObj->MakeCopy();
				
				// Turn it into a mono descriptor
				MonoDesc->SetUInt(ChannelCount_UL, 1);
				MDObjectPtr AvgBps = MonoDesc->Child(AvgBps_UL);
				if(AvgBps) AvgBps->SetUInt(AvgBps->GetUInt() / ChannelCount);

				// Copy up the video edit rate
				MuxDescObj->SetString(SampleRate_UL, VideoDescObj->GetString(SampleRate_UL));

				MDObjectPtr SubDescriptors = MuxDescObj->AddChild(SubDescriptorUIDs_UL);
				if(SubDescriptors)
				{
					MDObjectPtr Ptr = SubDescriptors->AddChild();
					if(Ptr) Ptr->MakeRef(VideoDescObj);
					
					// Add one copy of the mono descriptor per channel
					while(ChannelCount--)
					{
						Ptr = SubDescriptors->AddChild();

						MDObjectPtr NewCopy = MonoDesc->MakeCopy();

						if(Ptr) Ptr->MakeRef(NewCopy);
					}

					// Build a descriptor with a zero ID (we only support single stream files)
					EssenceStreamDescriptorPtr MuxDescriptor = new EssenceStreamDescriptor;
					MuxDescriptor->ID = 0;
					MuxDescriptor->Description = "DV-DIF audio/video essence";
					MuxDescriptor->SourceFormat.Set(DV_DIF_RAW_Format);
					MuxDescriptor->Descriptor = MuxDescObj;

					// Add the multiple descriptor
					Ret.push_back(MuxDescriptor);
				}
			}
		}

		MuxDescObj = new MDObject(MultipleDescriptor_UL);
		if(MuxDescObj)
		{
			// Copy up the video edit rate
			MuxDescObj->SetString(SampleRate_UL, VideoDescObj->GetString(SampleRate_UL));

			MDObjectPtr SubDescriptors = MuxDescObj->AddChild(SubDescriptorUIDs_UL);
			if(SubDescriptors)
			{
				MDObjectPtr Ptr = SubDescriptors->AddChild();
				if(Ptr) Ptr->MakeRef(VideoDescObj);
				Ptr = SubDescriptors->AddChild();
				if(Ptr) Ptr->MakeRef(AudioDescObj);
			
				// Build a descriptor with a zero ID (we only support single stream files)
				EssenceStreamDescriptorPtr MuxDescriptor = new EssenceStreamDescriptor;
				MuxDescriptor->ID = 0;
				MuxDescriptor->Description = "DV-DIF audio/video essence";
				MuxDescriptor->SourceFormat.Set(DV_DIF_RAW_Format);
				MuxDescriptor->Descriptor = MuxDescObj;

				// Add the multiple descriptor
				Ret.push_back(MuxDescriptor);
			}
		}
	}

	// Add the single descriptor last so that the multiple one will be selected in preference, if allowed
	Ret.push_back(Descriptor);

	// Attempt to parse the format
	// Record a pointer to the video descriptor so we can check if we are asked to process this source
	CurrentDescriptor = VideoDescObj;

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

	// Correct for IEC or DV-BAsed
	if(isS314M) BaseUL[14] = 0x3f;

	WrappingOptionList Ret;

	// If the source format isn't RAW DV-DIFF or AVI-DV then we can't wrap the essence
	if(   (memcmp(Descriptor.SourceFormat.GetValue(), DV_DIF_RAW_Format, 16) != 0)
	   && (memcmp(Descriptor.SourceFormat.GetValue(), DV_DIF_AVI_Format, 16) != 0)) return Ret;

	// The identify step configures some member variables so we can only continue if we just identified this very source
	bool DescriptorMatch = false;
	bool MuxDescriptor = false;
	size_t AudioChannels = 1;

	if(Descriptor.Descriptor == CurrentDescriptor) DescriptorMatch = true;
	else if(Descriptor.Descriptor->IsA(MultipleDescriptor_UL))
	{
		// Check if the first sub of a multiple is our video descriptor
		MDObjectPtr Ptr = Descriptor.Descriptor[SubDescriptorUIDs_UL];
		if(Ptr) AudioChannels = Ptr->size() - 1;
		if(Ptr) Ptr = Ptr->front().second;
		if(Ptr) Ptr = Ptr->GetLink();
		if(Ptr == CurrentDescriptor)
		{
			DescriptorMatch = true;
			MuxDescriptor = true;
		}
	}

	// We didn't build this descriptor, so we can't wrap the essence
	if(!DescriptorMatch) return Ret;

	// Build a WrappingOption for clip wrapping
	WrappingOptionPtr ClipWrap = new WrappingOption;

	ClipWrap->Handler = this;							// Set us as the handler

	if(MuxDescriptor)
	{
		if(AudioChannels > 1)
		{
			ClipWrap->Description = "SMPTE 383M clip wrapping of DV-DIF video and audio data - multiple mono audio tracks";
			ClipWrap->Name = "clip-multi";				// Set the wrapping name
		}
		else
		{
			ClipWrap->Description = "SMPTE 383M clip wrapping of DV-DIF video and audio data";
			ClipWrap->Name = "clip-av";					// Set the wrapping name
		}
	}
	else
	{
		ClipWrap->Description = "SMPTE 383M clip wrapping of DV-DIF video data";
		ClipWrap->Name = "clip";						// Set the wrapping name
	}

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

	if(MuxDescriptor)
	{
		if(AudioChannels > 1)
		{
			FrameWrap->Description = "SMPTE 383M frame wrapping of DV-DIF video and audio data - multiple mono audio tracks";
			FrameWrap->Name = "frame-multi";			// Set the wrapping name
		}
		else
		{
			FrameWrap->Description = "SMPTE 383M frame wrapping of DV-DIF video and audio data";
			FrameWrap->Name = "frame-av";				// Set the wrapping name
		}
	}
	else
	{
		FrameWrap->Description = "SMPTE 383M frame wrapping of DV-DIF video data";
		FrameWrap->Name = "frame";						// Set the wrapping name
	}

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
	// Either use the cached value, or scan the stream and find out how many bytes to read
	if((CachedDataSize == static_cast<size_t>(-1)) || (CachedCount != Count)) ReadInternal(InFile, Stream, Count);

	// Record, then clear, the data size
	size_t Bytes = CachedDataSize;
	CachedDataSize = static_cast<size_t>(-1);

	// If this is not an AVI file read the data and return
	if(DIFEnd != -1)
	{
		// Read the data
		return FileReadChunk(InFile, Bytes);
	}

	// Read the bytes from the AVI data
	return AVIRead(InFile, Bytes);
}


//! Read data from AVI wrapped essence
/*! Parses the list and chunk structure - can recurse */
DataChunkPtr DV_DIF_EssenceSubParser::AVIRead(FileHandle InFile, size_t Bytes) 
{
	// Can we return all the data from the current chunk?
	if(AVIChunkRemaining >= Bytes)
	{
		Decrement(AVIChunkRemaining, static_cast<UInt32>(Bytes));
		Decrement(AVIListRemaining, static_cast<UInt32>(Bytes));
		return FileReadChunk(InFile, Bytes);
	}

	// Read anything left in the current chunk
	DataChunkPtr Ret;
	if(AVIChunkRemaining > 0)
	{
		Ret = FileReadChunk(InFile, AVIChunkRemaining);
		Bytes -= AVIChunkRemaining;
		Decrement(AVIListRemaining, AVIChunkRemaining);
	}

	while(!FileEof(InFile))
	{
		// Look for another essence stream chunk in this list
		while(AVIListRemaining && !FileEof(InFile))
		{
			U32Pair Header = ReadRIFFHeader(InFile);
			Decrement(AVIListRemaining, 8);
			
			if(Header.first == AVIStreamID)
			{
				AVIChunkRemaining = Header.second;
				
				// If we haven't yet read anything - simply read what is required
				if(!Ret) return AVIRead(InFile, Bytes);

				// Append the new data and return that
				Ret->Append(AVIRead(InFile, Bytes));
				return Ret;
			}

			// Skip this chunk
			FileSeek(InFile, FileTell(InFile) + Header.second);
			Decrement(AVIListRemaining, Header.second);
		}

		// If we have exhausted the current list, we need to locate the next list
		while(!FileEof(InFile))
		{
			U32Pair Header = ReadRIFFHeader(InFile);
			
			// Ensure we exit gracefully if we run out of valid data
			if((Header.first == 0) && (Header.second == 0)) break;

			if(Header.first == ID_LIST)
			{
				AVIListRemaining = Header.second;
				
				// Read and discard the list type
				ReadU32(InFile);

				break;
			}

			if(Header.first == ID_RIFF)
			{
				UInt32 RIFFType = ReadU32(InFile);

				if(RIFFType != ID_AVIX)
				{
					error("Found continuation RIFF of type 0x%08x - expected an AVIX chunk\n", RIFFType);
					return Ret;
				}

				// We have found an AVIX chunk - start parsing inside it, looking for our next list
				continue;
			}

			// Skip this chunk
			FileSeek(InFile, FileTell(InFile) + Header.second);
		}
	}

	// If we hit EOF, return what we have so far
	return Ret;
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
	size_t Bytes = ReadInternal(InFile, Stream, Count);
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

	// Free the buffer
	delete[] Buffer;

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
	isS314M = ((Buffer[4] & 0x07) == 0x01) || ((Buffer[4] & 0x07) == 0x07);

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

	// FIXME: Currently only supports SD DV
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


//! Read the header at the specified position in a DV file to build a sound essence descriptor
/*! DRAGONS: Currently rather scrappy */
MDObjectPtr DV_DIF_EssenceSubParser::BuildSoundEssenceDescriptor(FileHandle InFile, UInt64 Start /*=0*/)
{
	MDObjectPtr Ret;
	UInt8 Buffer[80];

	// Read the header DIF block
	FileSeek(InFile, Start);
	if(FileRead(InFile, Buffer, 80) < 80) return Ret;

	// Set 625/50 flag from the header
	bool is625 = ((Buffer[3] & 0x80) == 0x80);

	// Set SMPTE-314M flag by assuming the APT value will only be 001 or 111 if we are in SMPTE-314M
	isS314M = ((Buffer[4] & 0x07) == 0x01) || ((Buffer[4] & 0x07) == 0x07);

	// Bug out if the audio is flagged as invalid
	if((Buffer[5] & 0x80) != 0) return Ret;


	// Build the essence descriptor, filling in all known values

	Ret = new MDObject(GenericSoundEssenceDescriptor_UL);
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

	// FIXME: We currently assume 2 channel, 16-bit, 48kHz audio
	Ret->SetInt(ChannelCount_UL, 2);
	Ret->SetString(AudioSamplingRate_UL, "48000/1");
	Ret->SetInt(QuantizationBits_UL, 16);

	return Ret;
}


//! Read the header at the specified position in a DV-AVI file to build an essence descriptor
MDObjectPtr DV_DIF_EssenceSubParser::BuildCDCIEssenceDescriptorFromAVI(FileHandle InFile, UInt64 Start)
{
	MDObjectPtr Ret;

	// Re-read the header list
	FileSeek(InFile, Start);
	U32Pair Header = ReadRIFFHeader(InFile);
	UInt32 ListSize = Header.second;

	// Verify that this is a list
	if(Header.first != ID_LIST) return Ret;

	// Read the list type (we are only interested in stream info lists)
	if(ReadU32(InFile) != ID_strl) return Ret;
	Decrement(ListSize, 4);

	// We only support files with a stream header at the start of each strl list
	Header = ReadRIFFHeader(InFile);
	if(Header.first != ID_strh) return Ret;
	Decrement(ListSize, 8);

	// Read this chunk
	DataChunkPtr StreamHeader = FileReadChunk(InFile, Header.second);
	Decrement(ListSize, Header.second);

	// We only support files with a stream format following the stream header
	if(ListSize < 8) return Ret;
	Header = ReadRIFFHeader(InFile);
	if(Header.first != ID_strf) return Ret;
	Decrement(ListSize, 8);

	// Read this chunk
	DataChunkPtr StreamFormat = FileReadChunk(InFile, Header.second);
	Decrement(ListSize, Header.second);

	/* Build the stream ID that this essence uses - this is normally ##db where ## is the stream number in decimal */
	AVIStreamID = ID_00db;
	if(StreamNumber > 0) AVIStreamID += (StreamNumber % 10) << 16;
	if(StreamNumber > 9) AVIStreamID += (StreamNumber / 10) << 24;

	// Check if there is an index chunk - this will define the StreamID
	if(ListSize > 8)
	{
		Header = ReadRIFFHeader(InFile);
		Decrement(ListSize, 8);
		if(Header.first == ID_indx) 
		{
			// Read this chunk
			DataChunkPtr IndexChunk = FileReadChunk(InFile, Header.second);
			Decrement(ListSize, Header.second);

			if(IndexChunk->Size >= 12) AVIStreamID = GetU32(&IndexChunk->Data[8]);
		}
	}

	// DRAGONS: We now ignore all this info and build the data from the movi data
	//          We may make more use of the header in future

	// Start scanning for the movi list
	UInt64 Scan = FileTell(InFile) + ListSize;

	while(!FileEof(InFile))
	{
		// Seek to the next position (and check that we succeeded - if not we are beyond the EOF)
		FileSeek(InFile, Scan);
		if(FileTell(InFile) != Scan) return Ret;

		// Read the chunk header
		Header = ReadRIFFHeader(InFile);
		
		// Work out where this chunk ends
		UInt64 NextScan = FileTell(InFile) + Header.second;

		// Is this the movi list?
		if(Header.first == ID_LIST)
		{
			UInt32 ListID = ReadU32(InFile);
			if(ListID == ID_movi)
			{
				ListSize = Header.second;

				while(ListSize && !FileEof(InFile))
				{
					Header = ReadRIFFHeader(InFile);
					if(Header.first == AVIStreamID)
					{
						// Record the start of the data
						DIFStart = FileTell(InFile);
						DIFEnd = -1;
						
						// Record the outer list and current chunk remaining byte counts
						AVIListRemaining = ListSize;
						AVIChunkRemaining = Header.second;

						// Build the header from this data
						Ret = BuildCDCIEssenceDescriptor(InFile, static_cast<UInt64>(DIFStart));
						
						// Return to the start of the data
						FileSeek(InFile, DIFStart);

						return Ret;
					}

					// Skip over the contents of this chunk
					FileSeek(InFile, FileTell(InFile) + Header.second);

					// Remove the size of the chunk header and the chunk from the list size
					Decrement(ListSize, (Header.second + 8));
				}
			}
			// Have we found an ODML section?
			else if(ListID == ID_odml)
			{
				ListSize = Header.second;

				while(ListSize && !FileEof(InFile))
				{
					Header = ReadRIFFHeader(InFile);

					// Work out where this chunk in the list ends
					Position ChunkEnd = FileTell(InFile) + Header.second;

					if(Header.first == ID_dmlh)
					{
						AVIFrameCount = ReadU32_LE(InFile);
					}

					// Skip over the contents of this chunk
					FileSeek(InFile, ChunkEnd);

					// Remove the size of the chunk header and the chunk from the list size
					Decrement(ListSize, (Header.second + 8));
				}
			}
		}

		// Move to the end of this chunk
		Scan = NextScan;
	}

	return Ret;
}


//! Read the header at the specified position in a DV-AVI file to build an audio essence descriptor
MDObjectPtr DV_DIF_EssenceSubParser::BuildSoundEssenceDescriptorFromAVI(FileHandle InFile, UInt64 Start)
{
	MDObjectPtr Ret;

	// Re-read the header list
	FileSeek(InFile, Start);
	U32Pair Header = ReadRIFFHeader(InFile);
	UInt32 ListSize = Header.second;

	// Verify that this is a list
	if(Header.first != ID_LIST) return Ret;

	// Read the list type (we are only interested in stream info lists)
	if(ReadU32(InFile) != ID_strl) return Ret;
	Decrement(ListSize, 4);

	// We only support files with a stream header at the start of each strl list
	Header = ReadRIFFHeader(InFile);
	if(Header.first != ID_strh) return Ret;
	Decrement(ListSize, 8);

	// Read this chunk
	DataChunkPtr StreamHeader = FileReadChunk(InFile, Header.second);
	Decrement(ListSize, Header.second);

	// We only support files with a stream format following the stream header
	if(ListSize < 8) return Ret;
	Header = ReadRIFFHeader(InFile);
	if(Header.first != ID_strf) return Ret;
	Decrement(ListSize, 8);

	// Read this chunk
	DataChunkPtr StreamFormat = FileReadChunk(InFile, Header.second);
	Decrement(ListSize, Header.second);

	/* Build the stream ID that this essence uses - this is normally ##db where ## is the stream number in decimal */
	AVIStreamID = ID_00db;
	if(StreamNumber > 0) AVIStreamID += (StreamNumber % 10) << 16;
	if(StreamNumber > 9) AVIStreamID += (StreamNumber / 10) << 24;

	// Check if there is an index chunk - this will define the StreamID
	if(ListSize > 8)
	{
		Header = ReadRIFFHeader(InFile);
		Decrement(ListSize, 8);
		if(Header.first == ID_indx) 
		{
			// Read this chunk
			DataChunkPtr IndexChunk = FileReadChunk(InFile, Header.second);
			Decrement(ListSize, Header.second);

			if(IndexChunk->Size >= 12) AVIStreamID = GetU32(&IndexChunk->Data[8]);
		}
	}

	// DRAGONS: We now ignore all this info and build the data from the movi data
	//          We may make more use of the header in future

	// Start scanning for the movi list
	UInt64 Scan = FileTell(InFile) + ListSize;

	while(!FileEof(InFile))
	{
		// Seek to the next position (and check that we succeeded - if not we are beyond the EOF)
		FileSeek(InFile, Scan);
		if(FileTell(InFile) != Scan) return Ret;

		// Read the chunk header
		Header = ReadRIFFHeader(InFile);
		
		// Work out where this chunk ends
		UInt64 NextScan = FileTell(InFile) + Header.second;

		// Is this the movi list?
		if(Header.first == ID_LIST)
		{
			if(ReadU32(InFile) == ID_movi)
			{
				ListSize = Header.second;

				while(ListSize && !FileEof(InFile))
				{
					Header = ReadRIFFHeader(InFile);
					if(Header.first == AVIStreamID)
					{
						// Record the start of the data
						DIFStart = FileTell(InFile);
						DIFEnd = -1;
						
						// Record the outer list and current chunk remaining byte counts
						AVIListRemaining = ListSize;
						AVIChunkRemaining = Header.second;

						// Build the header from this data
						Ret = BuildSoundEssenceDescriptor(InFile, static_cast<UInt64>(DIFStart));
						
						// Return to the start of the data
						FileSeek(InFile, DIFStart);

						return Ret;
					}

					// Skip over the contents of this chunk
					FileSeek(InFile, FileTell(InFile) + Header.second);

					// Remove the size of the chunk header and the chunk from the list size
					Decrement(ListSize, (Header.second + 8));
				}
			}
		}

		// Move to the end of this chunk
		Scan = NextScan;
	}

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
size_t DV_DIF_EssenceSubParser::ReadInternal(FileHandle InFile, UInt32 Stream, UInt64 Count)
{	
	// Return the cached value if we have not yet used it
	if((CachedDataSize != static_cast<size_t>(-1)) && CachedCount == Count) return CachedDataSize;

	// Seek to the start of the essence on the first read
	if(PictureNumber == 0) FileSeek(InFile, DIFStart);

	// Return anything remaining if clip wrapping
	if((Count == 0) && (SelectedWrapping->ThisWrapType == WrappingOption::Clip))
	{
		if(DIFEnd == -1)
			Count = AVIFrameCount - PictureNumber;
		else
			Count = ((DIFEnd - DIFStart) / (150 * 80)) - PictureNumber;
	}

	// Simple version - we are working in our native edit rate
	if((SelectedEditRate.Denominator == NativeEditRate.Denominator) && (SelectedEditRate.Numerator = NativeEditRate.Numerator))
	{
		// Check for end of AVI essence, and adjust the count as required
		if(DIFEnd == -1)
		{
			if((PictureNumber + Count) > AVIFrameCount) Count = AVIFrameCount - PictureNumber;
		}

		// Work out how many bytes to read
		Length Ret = (Length)(Count * 150 * 80 * SeqCount);
		PictureNumber += Count;

		// If this would read beyond the end of the file stop at the end (don't test on AVI files)
		if((DIFEnd != -1) && ((Ret + static_cast<Position>(FileTell(InFile))) > DIFEnd))
		{
			Position SeqSize = (150 * 80 * SeqCount);

			Ret = DIFEnd - static_cast<Position>(FileTell(InFile));
			
			// Fix for an incomplete frame at the end of the previous read
			if(Ret < 0) Ret = 0;

			// Work out the picture number
			// DRAGONS: Add SeqSize-1 to ensure that a truncated edit unit is counted as a whole one (relies on '/' rounding down)
			PictureNumber = ((DIFEnd - DIFStart) + (SeqSize - 1)) / SeqSize;
		}

		// Return the number of bytes to read
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

	error("Non-native edit rate not yet supported\n");
	CachedDataSize = 0;
	return 0;
}


//! Set a parser specific option
/*! \return true if the option was successfully set */
bool DV_DIF_EssenceSubParser::SetOption(std::string Option, Int64 Param /*=0*/ )
{
	warning("DV_DIF_EssenceSubParser::SetOption(\"%s\", Param) not a known option\n", Option.c_str());

	return false; 
}



//! Build a new parser of this type and return a pointer to it
EssenceSubParserPtr DV_DIF_EssenceSubParser::NewParser(void) const 
{
	DV_DIF_EssenceSubParserFactory Factory;
	return Factory.NewParser(); 
}
