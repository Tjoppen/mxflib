/*! \file	esp_mpeg2ves.cpp
 *	\brief	Implementation of class that handles parsing of uncompressed pcm wave audio files
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

//! Examine the open file and return a list of essence descriptors
/*! \note This call will modify properties SampleRate, DataStart and DataSize */
EssenceStreamDescriptorList mxflib::WAVE_PCM_EssenceSubParser::IdentifyEssence(FileHandle InFile)
{
	int BufferBytes;
	Uint8 Buffer[12];

	EssenceStreamDescriptorList Ret;

	// Read the first 12 bytes of the file to allow us to identify it
	FileSeek(InFile, 0);
	BufferBytes = FileRead(InFile, Buffer, 12);

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
	EssenceStreamDescriptor Descriptor;
	Descriptor.ID = 0;
	Descriptor.Description = "Wave audio essence";
	Descriptor.Descriptor = DescObj;

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
WrappingOptionList mxflib::WAVE_PCM_EssenceSubParser::IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor Descriptor)
{
	Uint8 BaseUL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x02, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x06, 0x01, 0x00 };
	WrappingOptionList Ret;

	// If the supplied descriptor isn't a wave audio descriptor then we can't wrap the essence
	if(Descriptor.Descriptor->Name() != "WaveAudioDescriptor") return Ret;

	// Build a WrappingOption for clip wrapping
	WrappingOptionPtr ClipWrap = new WrappingOption;

	ClipWrap->Handler = this;							// Set us as the handler
	ClipWrap->Description = "SMPTE 382M clip wrapping of wave audio";

	BaseUL[14] = 0x02;									// Clip wrapping
	ClipWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	ClipWrap->GCEssenceType = 0x16;						// GP Sound wrapping type
	ClipWrap->GCElementType = 0x02;						// Wave clip wrapped elemenet
	ClipWrap->ThisWrapType = WrappingOption::Clip;		// Clip wrapping
	ClipWrap->CanSlave = true;							// Can use non-native edit rate
	ClipWrap->BERSize = 0;								// No BER size forcing

	// Build a WrappingOption for frame wrapping
	WrappingOptionPtr FrameWrap = new WrappingOption;

	FrameWrap->Handler = this;							// Set us as the handler
	FrameWrap->Description = "SMPTE 382M frame wrapping of wave audio";

	BaseUL[14] = 0x01;									// Frame wrapping
	FrameWrap->WrappingUL = new UL(BaseUL);				// Set the UL
	FrameWrap->GCEssenceType = 0x16;					// GP Sound wrapping type
	FrameWrap->GCElementType = 0x01;					// Wave frame wrapped elemenet
	FrameWrap->ThisWrapType = WrappingOption::Frame;	// Frame wrapping
	FrameWrap->CanSlave = true;							// Can use non-native edit rate
	FrameWrap->BERSize = 0;								// No BER size forcing

	// Add the two wrapping options 
	// Note: clip wrapping is preferred as this works best for audio-only files
	Ret.push_back(ClipWrap);
	Ret.push_back(FrameWrap);

	return Ret;
}




//! Read a number of wrapping items from the specified stream and return them in a data chunk
/*! If frame or line mapping is used the parameter Count is used to
 *	determine how many items are read. In frame wrapping it is in
 *	units of EditRate, as specified in the call to Use(), which may
 *  not be the frame rate of this essence
 *	\note This is going to take a lot of memory in clip wrapping! 
 */
DataChunkPtr mxflib::WAVE_PCM_EssenceSubParser::Read(FileHandle InFile, Uint32 Stream, Uint64 Count /*=1*/, IndexTablePtr Index /*=NULL*/)
{
	// Move to the current position
	if(CurrentPos == 0) CurrentPos = DataStart;
	FileSeek(InFile, CurrentPos);
	
	// Find out how many bytes to read
	Uint64 Bytes = ReadInternal(InFile, Stream, Count);

	// Make a datachunk with enough space
	DataChunkPtr Ret = new DataChunk;
	Ret->Resize(Bytes);

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
Uint64 mxflib::WAVE_PCM_EssenceSubParser::Write(FileHandle InFile, Uint32 Stream, MXFFilePtr OutFile, Uint64 Count /*=1*/, IndexTablePtr Index /*=NULL*/)
{
	const unsigned int BUFFERSIZE = 32768;
	Uint8 *Buffer = new Uint8[BUFFERSIZE];

	// Move to the current position
	if(CurrentPos == 0) CurrentPos = DataStart;
	FileSeek(InFile, CurrentPos);
	
	// Find out how many bytes to transfer
	Uint64 Bytes = ReadInternal(InFile, Stream, Count);
	Uint64 Ret = Bytes;

	while(Bytes)
	{
		int ChunkSize;
		
		// Number of bytes to transfer in this chunk
		if(Bytes < BUFFERSIZE) ChunkSize = Bytes; else ChunkSize = BUFFERSIZE;

		FileRead(InFile, Buffer, ChunkSize);
		OutFile->Write(Buffer, ChunkSize);

		Bytes -= ChunkSize;
	}

	// Update the file pointer
	CurrentPos = FileTell(InFile);

	return Ret; 
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
		ConstSamples = (Uint32)SamplesPerEditUnit;
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
	SampleSequence = new Uint32[SampleSequenceSize];

	// Calculate a sequence that allocates the nearest fit
	float Remain = 0;
	int i;
	for(i = 0; i < SampleSequenceSize; i++)
	{
		float f = SamplesPerEditUnit + Remain;
		Uint32 x = (Uint32)floor(f + 0.5);
		SampleSequence[i] = x;
		Remain = f - x;
	}

	return true;
}



//! Read the sequence header at the specified position in an MPEG2 file to build an essence descriptor
/*! \note This call will modify properties SampleRate, DataStart and DataSize */
MDObjectPtr mxflib::WAVE_PCM_EssenceSubParser::BuildWaveAudioDescriptor(FileHandle InFile, Uint64 Start /*=0*/)
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

			Uint16 AudioFormat = GetU16_LE(&ChunkData->Data[0]);
			if(AudioFormat != 1) return Ret;

			Ret = new MDObject("WaveAudioDescriptor");
			if(!Ret) return Ret;

			// Set the sample rate
			char Buffer[32];
			SampleRate = GetU32_LE(&ChunkData->Data[4]);
			sprintf(Buffer, "%d/1", SampleRate);
			Ret->SetString("SampleRate", Buffer);
			Ret->SetString("AudioSamplingRate", Buffer);

			// Must assume not locked!
			Ret->SetUint("Locked", 0);

			// Set channel count
			Uint16 Chan = GetU16_LE(&ChunkData->Data[2]);
			Ret->SetUint("ChannelCount", Chan);

			// Set quantization bits
			Uint16 Quant = GetU16_LE(&ChunkData->Data[14]);
			Ret->SetUint("QuantizationBits", Quant);

			// Calculate the number of bytes per sample
			SampleSize = ((Quant+7) / 8) * Chan;

			// Set the block alignment
			Ret->SetUint("BlockAlign", GetU16_LE(&ChunkData->Data[12]));

			// Set the byte-rate
			Ret->SetUint("AvgBps", GetU32_LE(&ChunkData->Data[8]));
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
Uint64 mxflib::WAVE_PCM_EssenceSubParser::ReadInternal(FileHandle InFile, Uint32 Stream, Uint64 Count) 
{ 
	Uint64 Ret;
	Uint32 SamplesPerEditUnit;
	
	// Work out the maximum possible bytes to return
	Uint64 Max = (CurrentPos - DataStart);			// Where we are in the data
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
	if((Count == 0) && (SelectedWrapping == WrappingOption::Clip)) Ret = Max;
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
