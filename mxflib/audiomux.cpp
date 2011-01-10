/*! \file	audiomux.cpp
 *	\brief	Implementation of classes that handle audio multiplexing and demultiplexing
 *
 *	\version $Id: audiomux.cpp,v 1.1 2011/01/10 10:42:09 matt-beard Exp $
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

#include "mxflib/mxflib.h"

using namespace mxflib;


//! Calculate BytesPerEditUnit for a given KAGSize
void AudioDemuxSource::CalcBytesPerEditUnit(Uint32 KAGSize)
{
	// Store the KAGSize used
	BPEUKAGSize = KAGSize;

	// Test for constant sample count per edit unit by requesting sample size for KAG of 1 from original source
	if(Parent->Source->GetBytesPerEditUnit(1) == 0)
	{
		BytesPerEditUnit = 0;
		return;
	}

	// What bitsize will we be using?
	unsigned int BitSize = (Parent->OutputBitSize == 0) ? Parent->SourceChannelBitSize : Parent->OutputBitSize;

	BytesPerEditUnit = ((BitSize * ChannelCount) + 7) / 8;

	// Test for frame wrapping constant sample count per edit unit by requesting sample size for KAG of 1 from original source
	if(	Parent->Source->GetBytesPerEditUnit(1024*1024*1024) >= 1024*1024*1024)
	{
		// FIXME: This assumes that 4-byte BER coding will be used - this needs to be adjusted or forced to be true!!
		BytesPerEditUnit += 16 + 4;

		// Adjust for whole KAGs if required
		if(KAGSize > 1)
		{
			// Work out how much short of the next KAG boundary we would be
			UInt32 Remainder = static_cast<UInt32>(BytesPerEditUnit) % KAGSize;
			if(Remainder) Remainder = KAGSize - Remainder;

			// Round up to the start of the next KAG
			BytesPerEditUnit += Remainder;

			// If there is not enough space to fit a filler in the remaining space an extra KAG will be required
			// DRAGONS: For very small KAGSizes we may need to add several KAGs
			while((Remainder > 0) && (Remainder < 17))
			{
				BytesPerEditUnit += KAGSize;
				Remainder += KAGSize;
			}
		}
	}
}

//! Get data for a sub-source
DataChunkPtr AudioDemux::GetEssenceData(AudioDemuxSource *Caller, unsigned int Channel, unsigned int ChannelCount, size_t Size /*=0*/, size_t MaxSize /*=0*/)
{
	AUDIODEMUX_DEBUG("GetEssenceData(Caller, %u, %u, %s, %s)\n", Channel, ChannelCount, UInt64toString(Size).c_str(), UInt64toString(MaxSize).c_str());

	// The value we will return
	DataChunkPtr Ret;
	
	// Number of samples remaining in this chunk
	Length SamplesRemaining;

	// Number of samples we will demux this time
	Length SampleCount;

	// The start position of the first sample in the source buffer
	Length Start;

	// Sanity check the channel parameters
	if((Channel + ChannelCount) > SourceChannelCount) return Ret;
	mxflib_assert(Outputs[Channel].Source);

	if(Caller->GetLenToSend()!=-1 && Outputs[Channel].Pos>=Caller->GetLenToSend())
	{
		return Ret;
	}

	// What bitsize will we be using?
	unsigned int BitSize = (OutputBitSize == 0) ? SourceChannelBitSize : OutputBitSize;

	// Work out the number of bytes per sample for this number of channels
	unsigned int BytesPerSample = ((BitSize * ChannelCount) + 7) / 8;

	// Pointer to the current position in the source buffer
	UInt8 *BuffPtr;

	if(InCurrentBuffer(Channel))
	{
		SamplesRemaining = CurrentSampleCount - (Outputs[Channel].Pos - CurrentStart);
		Start = CurrentStart;

		// Initialize the buffer pointer
		BuffPtr = CurrentData->Data;
	}
	else if(Outputs[Channel].Eof)
	{
		if(Caller->GetLenToSend()!=-1 && Outputs[Channel].Pos<=Caller->GetLenToSend())
		{
			Ret = new DataChunk(static_cast<size_t>(Caller->GetLenToSend()-Outputs[Channel].Pos));
			memset(Ret->Data,0,Ret->Size);

			// Update the positions for each channel demuxed
			while(ChannelCount--)
			{
				Outputs[Channel].Pos += Ret->Size;
				Channel++;
			}
			printf("Wrote %d extra bytes for channel %d\n",(int)(Caller->GetLenToSend()-Outputs[Channel].Pos),Channel);
			return Ret;
		}
		// InCurrentBuffer() might have failed due to EOF when trying to get new data for this channel
		return Ret;
	}
	else
	{
		// Index the correct old buffer
		OldDataList::iterator it = GetOldBuffer(Channel);

		// If we can't get this data (such as at EOF) return nothing
		if(it == OldData.end()) return Ret;

		SamplesRemaining = (*it).SampleCount - (Outputs[Channel].Pos - (*it).Start);
		Start = (*it).Start;

		// Initialize the buffer pointer
		BuffPtr = (*it).Data->Data;
	}

	/* Allocate the demux buffer */

	if(true) // For future expansion...
	{
		// Initially assume that we will be demuxing all remaining samples for this chunk
		SampleCount = SamplesRemaining;
	}

	if(Caller->GetLenToSend()!=-1 && SampleCount>Caller->GetLenToSend())
		SampleCount=Caller->GetLenToSend();
	

	// Calculate the total size of this data
	size_t BufferSize = static_cast<size_t>(BytesPerSample * SampleCount);

	// If this would be bigger than the max size - reduce the count
	if(MaxSize && (BufferSize > MaxSize))
	{
		// Set to the maximum requested size (rounded down to the last whole sample)
		BufferSize = MaxSize - (MaxSize % BytesPerSample);
		
		// Calculate the number of samples to write for this buffer size
		SampleCount = MaxSize / BytesPerSample;
	}

	// Ensure that the caller's end-of-item flag is set if we will demux all remaining samples for this chunk
	Caller->SetEoi(SamplesRemaining == SampleCount);

	

	// Allocate the buffer
	Ret = new DataChunk(BufferSize);

//printf("Buffer holds %d*%d samples in %d bytes\n", (int)SampleCount, (int)ChannelCount, (int)BufferSize);

	// Pointer for output writing
	UInt8 *OutPtr = Ret->Data;

	// Record where we will leave the output pointers
	Position FinalPos = Outputs[Channel].Pos + SampleCount;


	/* Demux code - optimized versions for each common sample size (and a non-optimum one for resizing) */

	if((OutputBitSize != 0) && (OutputBitSize != SourceChannelBitSize))
	{
		UInt32 Sample;

		// Calculate how many bytes we skip
		unsigned int Skip = SourceSampleSize - ((ChannelCount * SourceChannelBitSize) + 7) / 8;;

		// Move the buffer pointer forward to the current position for this output channel
		BuffPtr += (Outputs[Channel].Pos - Start) * SourceChannelCount;

		// Move to the correct sub-channel in the source (as long as we step in full sample sizes this will keep correct)
		// DRAGONS: This only works if each demux set is an exact number of bytes
		BuffPtr += ((Channel * SourceChannelBitSize) + 7) / 8;

		while(SampleCount--)
		{
			unsigned int Channel = ChannelCount;
			while(Channel--)
			{
				if(SourceChannelBitSize == 8) Sample = *(BuffPtr++);
				else if(SourceChannelBitSize == 16) { Sample = GetU16_LE(BuffPtr); BuffPtr += 2; }
				else if(SourceChannelBitSize == 24) { Sample = (static_cast<UInt32>(BuffPtr[2]) << 16) + (static_cast<UInt32>(BuffPtr[1]) << 8) + *BuffPtr; BuffPtr += 3; }
				else if(SourceChannelBitSize == 32) { Sample = GetU32_LE(BuffPtr); BuffPtr += 4; }
				else
				{
					error("SourceChannelBitSize of %u not supported\n", SourceChannelBitSize);
					mxflib_assert(0);
				}

				// Adjust the bit size
				if(OutputBitSize > SourceChannelBitSize) Sample <<= (OutputBitSize - SourceChannelBitSize);
				else Sample >>= (SourceChannelBitSize - OutputBitSize);

				if(OutputBitSize == 8) *(OutPtr++) = Sample;
				else if(OutputBitSize == 16) { PutU16_LE(static_cast<UInt16>(Sample), OutPtr); OutPtr += 2; }
				else if(OutputBitSize == 24) { *OutPtr = static_cast<UInt8>(Sample); OutPtr[1] = static_cast<UInt8>(Sample >> 8); OutPtr[2] = static_cast<UInt8>(Sample >> 16); OutPtr += 3; }
				else if(OutputBitSize == 32) { PutU32_LE(Sample, OutPtr); OutPtr += 4; }
				else
				{
					error("OutputBitSize of %u not supported\n", OutputBitSize);
					mxflib_assert(0);
				}
			}

			// Skip the other channels
			BuffPtr += Skip;
		}
	}
	else if(SourceChannelBitSize == 8)
	{
		/* Do the demux for bytes */

		// Move the buffer pointer forward to the current position for this output channel
		BuffPtr += (Outputs[Channel].Pos - Start) * SourceChannelCount;

		// Move to the correct sub-channel in the source (as long as we step in full sample sizes this will keep correct)
		BuffPtr += Channel;

		if(ChannelCount == 1)
		{
			/* Single channel output */
			while(SampleCount--)
			{
				*(OutPtr++) = *BuffPtr;
				BuffPtr += SourceChannelCount;
			}
		}
		else
		{
			/* Optimized multi-channel version - copies ChannelCount channels, then skips to the next sample.
			 * Most compilers will make the inner loop very efficient.
			 */
			unsigned int ChanDiff = SourceChannelCount - ChannelCount;
			while(SampleCount--)
			{
				unsigned int Chan = ChannelCount;
				while(Chan--) *(OutPtr++) = *(BuffPtr++);
				BuffPtr += ChanDiff;
			}
		}
	}
	else if (SourceChannelBitSize == 16)
	{
		/* Do the demux for 16-bit words */

		// Move the buffer pointer forward to the current position for this output channel
		BuffPtr += (Outputs[Channel].Pos - Start) * SourceSampleSize;

		// Move to the correct sub-channel in the source (as long as we step in full sample sizes this will keep correct)
		BuffPtr += Channel * 2;

		if(ChannelCount == 1)
		{
			// The size of each sample in bytes  - less the number that we will have already incremented with post increment
			register unsigned int StepSize = SourceSampleSize - 1;

			/* Single channel output */
			while(SampleCount--)
			{
				*(OutPtr++) = *(BuffPtr++);
				*(OutPtr++) = *BuffPtr;
				BuffPtr += StepSize;
			}
		}
		else
		{
			/* Optimized multi-channel version - copies ChannelCount*2 bytes (ChannelCount channels), then skips to the next sample.
			 * Most compilers will make the inner loop very efficient.
			 */
			unsigned int ChanDiff = (SourceChannelCount - ChannelCount) * 2;
			unsigned int CopySize = ChannelCount * 2;
			while(SampleCount--)
			{
				unsigned int ChanSize = CopySize;
				while(ChanSize--) *(OutPtr++) = *(BuffPtr++);
				BuffPtr += ChanDiff;
			}
		}
	}
	else if (SourceChannelBitSize == 24)
	{
		/* Do the demux for 24-bit words */

		// Move the buffer pointer forward to the current position for this output channel
		BuffPtr += (Outputs[Channel].Pos - Start) * SourceSampleSize;

		// Move to the correct sub-channel in the source (as long as we step in full sample sizes this will keep correct)
		BuffPtr += Channel * 3;

		if(ChannelCount == 1)
		{
			// The size of each sample in bytes  - less the number that we will have already incremented with post increment
			register unsigned int StepSize = SourceSampleSize - 2;

			/* Single channel output */
			while(SampleCount--)
			{
				*(OutPtr++) = *(BuffPtr++);
				*(OutPtr++) = *(BuffPtr++);
				*(OutPtr++) = *BuffPtr;
				BuffPtr += StepSize;
			}
		}
		else
		{
			/* Optimized multi-channel version - copies ChannelCount*3 bytes (ChannelCount channels), then skips to the next sample.
			 * Most compilers will make the inner loop very efficient.
			 */
			unsigned int ChanDiff = (SourceChannelCount - ChannelCount) * 3;
			unsigned int CopySize = ChannelCount * 3;
			while(SampleCount--)
			{
				unsigned int ChanSize = CopySize;
				while(ChanSize--) *(OutPtr++) = *(BuffPtr++);
				BuffPtr += ChanDiff;
			}
		}
	}
	else if (SourceChannelBitSize == 32)
	{
		/* Do the demux for 32-bit words */

		// Move the buffer pointer forward to the current position for this output channel
		BuffPtr += (Outputs[Channel].Pos - Start) * SourceSampleSize;

		// Move to the correct sub-channel in the source (as long as we step in full sample sizes this will keep correct)
		BuffPtr += Channel * 4;

		if(ChannelCount == 1)
		{
			// The size of each sample in bytes  - less the number that we will have already incremented with post increment
			register unsigned int StepSize = SourceSampleSize;

			/* Single channel output */
			while(SampleCount--)
			{
				*(OutPtr++) = *(BuffPtr++);
				*(OutPtr++) = *(BuffPtr++);
				*(OutPtr++) = *(BuffPtr++);
				*(OutPtr++) = *BuffPtr;
				BuffPtr += StepSize;
			}
		}
		else
		{
			/* Optimized multi-channel version - copies ChannelCount*4 bytes (ChannelCount channels), then skips to the next sample.
			 * Most compilers will make the inner loop very efficient.
			 */
			unsigned int ChanDiff = (SourceChannelCount - ChannelCount) * 4;
			unsigned int CopySize = ChannelCount * 4;
			while(SampleCount--)
			{
				unsigned int ChanSize = CopySize;
				while(ChanSize--) *(OutPtr++) = *(BuffPtr++);
				BuffPtr += ChanDiff;
			}
		}
	}
	else if (SourceChannelBitSize == 12)
	{
		// Do the demux for other sizes
		error("SourceChannelBitSize of %u not supported\n", SourceChannelBitSize);
		mxflib_assert(0);
	}
	else
	{
		// Do the demux for other sizes
		error("SourceChannelBitSize of %u not supported\n", SourceChannelBitSize);
		mxflib_assert(0);
	}

	// Update the positions for each channel demuxed
	while(ChannelCount--)
	{
		Outputs[Channel].Pos = FinalPos;
		Channel++;
	}

	return Ret;
}


//! Get the size of a sub-sources essence data in bytes
size_t AudioDemux::GetEssenceDataSize(unsigned int Channel, unsigned int ChannelCount)
{
	AUDIODEMUX_DEBUG("GetEssenceDataSize(%u, %u)\n", Channel, ChannelCount);

	Length SampleCount;

	// Sanity check the channel parameters
	if((Channel + ChannelCount) > SourceChannelCount) return 0;
	mxflib_assert(Outputs[Channel].Source);

	/* Locate the buffer with the required data and read the sample count from it */
	if(InCurrentBuffer(Channel))
	{
		SampleCount = CurrentSampleCount;
	}
	else
	{
		if(Outputs[Channel].Eof) 
		{
			SampleCount = 0;
		}
        else
		{
			OldDataList::iterator it = GetOldBuffer(Channel);
			if(it == OldData.end()) SampleCount = 0;
			else SampleCount = (*it).SampleCount;
		}
	}

	// What bitsize will we be using?
	unsigned int BitSize = (OutputBitSize == 0) ? SourceChannelBitSize : OutputBitSize;

	// Work out the number of bytes per sample for this number of channels
	Length BytesPerSample = ((BitSize * ChannelCount) + 7) / 8;

	// Return the total size of this data
	return static_cast<size_t>(BytesPerSample * SampleCount);
}

//! Get an essence source for reading data from the given channel number
/*! \param Channel The number of the first channel to read with the new source (zero being the first in the outer source)
 *  \param ChannelCount The number of channels to read at a time (e.g. ChannelCount = 2 give a stereo pair)
 */
EssenceSourcePtr AudioDemux::GetSource(unsigned int Channel, unsigned int ChannelCount /*=1*/)
{
	EssenceSourcePtr Ret;

	// Validate that Channel range
	if((ChannelCount < 1) || ((Channel + ChannelCount) > SourceChannelCount)) return Ret;

	// Barf if we don't have the first sample any more
	if(CurrentStart > 0)
	{
		// No old data - so we don't have the first sample
		if(OldData.empty()) return Ret;

		// Only the first entry can hold the first sample
		if(OldData.front().Start > 0) return Ret;
	}

	// Make the new source
	Ret = new AudioDemuxSource(this, Channel, ChannelCount);

	/* Set the output data for each of our channels */
	while(ChannelCount--)
	{
		if(Outputs[Channel].Source)
		{
			warning("Audio channel %u is allocated twice in AudioDemux::GetSource() - this almost certainly won't work\n", Channel);
		}

		Outputs[Channel].Source = Ret;
		Outputs[Channel].Pos = 0;
		Outputs[Channel].Eof = false;

		Channel++;
	}

	return Ret;
}

//! Read another chunk of data into the current buffer
/*! If the contents of the current buffer are still required by any of the channels it will be added to the old data list
 */
void AudioDemux::FillBuffer(void)
{
	AUDIODEMUX_DEBUG("FillBuffer()\n");

	if(Eof) return;

	/* First work out if we need to keep the current data */

	// Only bother if there is some current data
	if(CurrentSampleCount)
	{
		// Start the lowest position at its highest possible value
		Position LowestPosition = INT64_C(0x7fffffffffffffff);

		unsigned int i;
		for(i=0; i<SourceChannelCount; i++)
		{
			// Only accept a lower value if it is attached to an AudioDemuxSource
			if((Outputs[i].Pos < LowestPosition) && (Outputs[i].Source)) LowestPosition = Outputs[i].Pos;
		}

		// If any of the channels in use still require the "current" data we must keep it
		// DRAGONS: If we don't keep the buffer it will be freed when we overwrite the CurrentData smart pointer as it will no longer be referenced
		if(LowestPosition < (CurrentStart + CurrentSampleCount))
		{
			OldDataStruct Old;

			Old.Data = CurrentData;
			Old.Start = CurrentStart;
			Old.SampleCount = CurrentSampleCount;

			OldData.push_back(Old);
		}

		AUDIODEMUX_DEBUG("Lowest required sample = %s\n", Int64toString(LowestPosition).c_str());

		// While we are managing the old data list we can check if it contains any data that is no longer required
		while(!OldData.empty())
		{
			AUDIODEMUX_DEBUG("Oldest buffer starts at %s, length %s\n", Int64toString(OldData.front().Start).c_str(), Int64toString(OldData.front().SampleCount).c_str());

			// If the first buffer is before the lowest required position we can discard it (and loop to check again)
			if((OldData.front().Start + OldData.front().SampleCount) < LowestPosition)
			{
				AUDIODEMUX_DEBUG("No longer required - discarding\n");
				OldData.pop_front();
			}
			else
				break;
		}
	}

	AUDIODEMUX_DEBUG("CurrentStart updated from %s to %s\n", Int64toString(CurrentStart).c_str(), Int64toString(CurrentStart + CurrentSampleCount).c_str());

	// Update the start pointer
	CurrentStart += CurrentSampleCount;

	// Get a new data chunk
	size_t MaxSize=0;
	if(VideoEditRate.Denominator!=0) //i.e.has been set
	{
		MaxSize=SourceAudioSampleRate*VideoEditRate.Denominator/VideoEditRate.Numerator;
		if( (VideoEditRate.Denominator/VideoEditRate.Numerator)==29 && FrameCount%5>2)
			MaxSize++; //allow for 29.97 rate 
		MaxSize*=SourceSampleSize;

	}
	FrameCount++;
	CurrentData = Source->GetEssenceData( MaxSize, MaxChunkSize);

	// Have we hit EOF?
	if(!CurrentData)
	{
		AUDIODEMUX_DEBUG("EOF on reading new data\n");
		Eof = true;
		CurrentSampleCount = 0;
	}
	else
	{
		AUDIODEMUX_DEBUG("New current buffer holds %s samples\n", Int64toString(CurrentData->Size / SourceSampleSize).c_str());

		// Set the sample count
		CurrentSampleCount = CurrentData->Size / SourceSampleSize;
	}
}


//! Determine which of the old buffers to use for the given channel
/*! \return iterator indexing the OutputData structure for the buffer, or OutData.end() if there is a problem
 *  \note The caller must ensure that the channel number is valid and the channel is attached to an AudioDemuxSource before calling
 */
AudioDemux::OldDataList::iterator AudioDemux::GetOldBuffer(unsigned int Channel)
{
	AUDIODEMUX_DEBUG("GetOldBuffer(%u)\n", Channel);

	/* Scan the old buffers */
	OldDataList::iterator it = OldData.begin();

	AUDIODEMUX_DEBUG("Looking for sample %s\n", Int64toString(Outputs[Channel].Pos).c_str());

	while(it != OldData.end())
	{
		AUDIODEMUX_DEBUG("Old buffer start = %s, length = %s\n", Int64toString((*it).Start).c_str(), Int64toString((*it).SampleCount).c_str());

		// Are we after the start of this buffer ?
		if(Outputs[Channel].Pos >= (*it).Start)
		{
			// If we are before the end of the buffer, this is it
			if(Outputs[Channel].Pos < ((*it).Start + (*it).SampleCount))
			{
				AUDIODEMUX_DEBUG("Found\n");
				return it;
			}
		}

		it++;
	}

	AUDIODEMUX_DEBUG("Not Found\n");

	// Return the result - OldData.end() if we got here
	return it;
}

