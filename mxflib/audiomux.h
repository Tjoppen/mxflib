/*! \file	audiomux.h
 *	\brief	Definition of classes that handle audio multiplexing and demultiplexing
 *
 *	\version $Id: audiomux.h,v 1.1 2011/01/10 10:42:09 matt-beard Exp $
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
#ifndef MXFLIB__AUDIOMUX_H
#define MXFLIB__AUDIOMUX_H

/* Define debug behaviour for AudioDemux */
namespace
{
	// Dummy function to force debug arguments to be ignored
	inline const char *Ignore(const char *fmt, ...) { return fmt; }
}

//#define AUDIODEMUX_DEBUG printf
//#define AUDIODEMUX_DEBUG debug
#define AUDIODEMUX_DEBUG Ignore


namespace mxflib
{
	// Forward declare the AudioDemux class
	class AudioDemux;

	// A Smart pointer to an AudioDemux object
	typedef SmartPtr<AudioDemux> AudioDemuxPtr;

	// A parent pointer to an AudioDemux object
	typedef ParentPtr<AudioDemux> AudioDemuxParent;

	// Forward declare the demux output source class
	class AudioDemuxSource;

	// A Smart pointer to an AudioDemuxSource object
	typedef SmartPtr<AudioDemuxSource> AudioDemuxSourcePtr;

	// A parent pointer to an AudioDemuxSource object
	typedef ParentPtr<AudioDemuxSource> AudioDemuxSourceParent;


	//! Audio demultiplexer class, splits a single multi-channel audio source into sources with less channels each
	class AudioDemux : public RefCount<AudioDemux>
	{
	protected:
		//! Structure holding data relating to a demultiplexed channel
		struct OutputData
		{
			EssenceSourceParent Source;		//!< Parent pointers for this channel's output EssenceSource, NULL if this channel not being output
			Position Pos;					//!< Sample position for this channel, holds the sample number for the next sample to output for this channel
			bool Eof;						//!< True once this channel has output all that it can
		};

		//! Structure holding data relating to old, but still active, data
		struct OldDataStruct
		{
			DataChunkPtr Data;				//!< The data chunk holding the data
			Position Start;					//!< The sample number if the first sample in the data buffer
			Length SampleCount;				//!< The number of samples in the data buffer
		};

		//! List of OldDataStructs
		typedef std::list<OldDataStruct> OldDataList;

	protected:
		EssenceSourcePtr Source;			//!< The audio essence source to demultiplex
		unsigned int SourceChannelCount;	//!< The number of channels in the source
		unsigned int SourceChannelBitSize;	//!< The size of each source sample of each channel, in bits
		unsigned int SourceSampleSize;		//!< The total size of a source sample, for all channels, in bytes
		unsigned int SourceAudioSampleRate; //!< The sample rate of the source file     

		unsigned int OutputBitSize;			//!< Output bits, if forcing a different bit-size (0 = same as input bits)

		bool Eof;							//!< The original source has ended

		OutputData *Outputs;				//!< Array of data relating to each channel being output

		DataChunkPtr CurrentData;			//!< Pointer to a chunk containing the current audio data
		Position CurrentStart;				//!< The sample number of the first sample in the CurrentData buffer
		Length CurrentSampleCount;			//!< The number of samples in the CurrentData buffer

		OldDataList OldData;				//!< List of data about chunks containing old, but active, data

		size_t MaxChunkSize;				//!< Maximum chunk size to read from our source, or zero for unrestricted

		Rational    VideoEditRate;          //!< Optionally used to calculate the size of a chunk  of audio
		int         FrameCount;				//!< Count frames for calculating size of individual buffers

	private:
		AudioDemux();						//!< Prevent default construction
		AudioDemux(AudioDemux&);			//!< Prevent copy construction

	public:
		//! Construct a new audio demux object - with an EssenceSource*
		AudioDemux(EssenceSource *AudioSource, 
			unsigned int ChannelCount, unsigned int ChannelBitSize,
			unsigned int AudioSampleRate) 
			: Source(AudioSource), SourceChannelCount(ChannelCount), SourceChannelBitSize(ChannelBitSize),
			SourceAudioSampleRate(AudioSampleRate)
		{
			Init();
		}

		//! Construct a new audio demux object - with an EssenceSourcePtr
			AudioDemux(EssenceSourcePtr &AudioSource, unsigned int ChannelCount, unsigned int ChannelBitSize,
			unsigned int AudioSampleRate) 
			: Source(AudioSource), SourceChannelCount(ChannelCount), SourceChannelBitSize(ChannelBitSize),
			SourceAudioSampleRate(AudioSampleRate)
			{
			Init();
		}

		//! Common part of constructors
		void Init()
		{
			AUDIODEMUX_DEBUG("Construct AudioDemux with %u %u-bit channels\n", SourceChannelCount, SourceChannelBitSize);

			// Initialize list of output sources and their positions
			Outputs = new OutputData[SourceChannelCount];

			// Clear the current start
			CurrentStart = 0;
			CurrentSampleCount = 0;

			// Normally we don't resize the samples
			OutputBitSize = 0;

			// Start with unlimited chunk size
			MaxChunkSize = 0;

			// Calculate the number of bytes per sample (all channels)
			SourceSampleSize = ((SourceChannelBitSize*SourceChannelCount)+7) / 8;

			// We start of not at EOF
			Eof = false;


			//varialbe for optionally controlling size fo frames
			VideoEditRate.Numerator=0;
			VideoEditRate.Denominator=0;
			FrameCount=0;
		};

		//! Clean up
		~AudioDemux()
		{
			AUDIODEMUX_DEBUG("Destruct AudioDemux with %u %u-bit channels\n", SourceChannelCount, SourceChannelBitSize);

			delete[] Outputs;
		}

		//! Get an essence source for reading data from the given channel number
		/*! \param Channel The number of the first channel to read with the new source (zero being the first in the outer source)
		 *  \param ChannelCount The number of channels to read at a time (e.g. ChannelCount = 2 give a stereo pair)
		 */
		EssenceSourcePtr GetSource(unsigned int Channel, unsigned int ChannelCount = 1);

		//! Set a maximum size for chunks read from the source
		void SetMaxChunkSize(size_t Max) { MaxChunkSize = Max; }

		//! Set the output bit size
		void SetOutputBitSize(unsigned int Bits) { OutputBitSize = Bits; }


		void SetVideoRate( Rational ER)
		{
			VideoEditRate.Numerator=ER.Numerator;
			VideoEditRate.Denominator=ER.Denominator;
		}

		// FIXME: Record the descriptor and pass to any new source as required (or possibly code the sources to read from here?)
		void SetDescriptor(MDObjectPtr Descriptor) { }

		// Allow our demultiplexed sources to access our internals
		friend class AudioDemuxSource;

	protected:
		//! Get data for a sub-source
		DataChunkPtr GetEssenceData(AudioDemuxSource *Caller, unsigned int Channel, unsigned int ChannelCount, size_t Size = 0, size_t MaxSize = 0);

		//! Get the size of a sub-sources essence data in bytes
		size_t GetEssenceDataSize(unsigned int Channel, unsigned int ChannelCount);

		//! Determine if the current buffer contains the data for the specified channel - reading more data if required
		/*! \return True if the CurrentData buffer contains the required data (even if we had to read new data to achieve this)
		 *  \return False if the required data is in a old buffer, or that channel is at its EOF
		 *  DRAGONS: The buffers may move during this call - so don't store the result of this function or of GetBuffer() across calls
		 */
		bool InCurrentBuffer(unsigned int Channel)
		{
			AUDIODEMUX_DEBUG("InCurrentBuffer(%u) - Pos=%s, CurrentStart=%s, CurrentSampleCount=%s\n", 
							 Channel, Int64toString(Outputs[Channel].Pos).c_str(),
							 Int64toString(CurrentStart).c_str(), Int64toString(CurrentSampleCount).c_str() );

			// Are we before the start of the current buffer (if so the current buffer can't be used)
			if(Outputs[Channel].Pos < CurrentStart)
			{
				AUDIODEMUX_DEBUG("Data is older than CurrentBuffer\n");
				return false;
			}

			// From now on we will be using the currnt buffer - unless the channel is at EOF
			bool Ret = true;

			// If we are not within the buffer, keep reading new data until we are
			while(Outputs[Channel].Pos >= (CurrentStart + CurrentSampleCount))
			{
				if(Eof)
				{
					Outputs[Channel].Eof = true;
					Ret = false;
					break;
				}

				// Get more data
				FillBuffer();
			}

			// The required data will now be in the current buffer - unless we have hit the EOF for this channel
			return Ret;
		}

		//! Read another chunk of data into the current buffer
		/*! If the contents of the current buffer are still required by any of the channels it will be added to the old data list
		 */
		void FillBuffer(void);

		//! Determine which of the old buffers to use for the given channel
		/*! \return iterator indexing the OutputData structure for the buffer, or OutData.end() if there is a problem
		 *  \note The caller must ensure that the channel number is valid and the channel is attached to an AudioDemuxSource before calling
		 */
		OldDataList::iterator GetOldBuffer(unsigned int Channel);

		//! Receive notification that one of our demultiplexed sources is being destroyed
		/*! This allows us to free any memory that is no longer required earlier than waiting for our destruction
		 */
		void DestructNotify(AudioDemuxSource *const Source)
		{
			// TODO: Release memory that is no longer required now this source has gone
		}
	};

	//! Essence source class to be used as the output of AudioDemux objects
	class AudioDemuxSource : public EssenceSource
	{
	protected:
		AudioDemuxParent Parent;			//!< The parent demux object
		int Channel;						//!< The number of our first channel (zero being the first in the outer source)
		int ChannelCount;					//!< The number of channels to read at a time (e.g. ChannelCount = 2 give a stereo pair)
		bool Eoi;							//!< True if the last GetEssenceData() call completed a wrapping item

		Length BytesPerEditUnit;			//!< The size of an edit unit, if constant, else zero. Set to -1 when not known
		UInt32 BPEUKAGSize;					//!< The KAGSize used to calculate BytesPerEditUnit

	private:
		AudioDemuxSource();					//!< Prevent default construction
		AudioDemuxSource(AudioDemux&);		//!< Prevent copy construction

	public:
		/*! \param Channel The number of the first channel to read with the new source (zero being the first in the outer source)
		 *  \param ChannelCount The number of channels to read at a time (e.g. ChannelCount = 2 give a stereo pair)
		 */
		AudioDemuxSource(AudioDemux *Parent, unsigned int Channel, unsigned int ChannelCount = 1) 
			: Parent(Parent), Channel(Channel), ChannelCount(ChannelCount)
		{
			Eoi = true;
			BytesPerEditUnit = -1;
		}

		//! Clean up on destruction
		~AudioDemuxSource()
		{
			// Let our parent know we are going
			if(Parent) Parent->DestructNotify(this);
		}

		//! Get the size of the essence data in bytes
		/*! \note There is intentionally no support for an "unknown" response */
		virtual size_t GetEssenceDataSize(void) { return Parent->GetEssenceDataSize(Channel, ChannelCount); }

		//! Get the next "installment" of essence data
		/*! This will attempt to return an entire wrapping unit (e.g. a full frame for frame-wrapping) but will return it in
		 *  smaller chunks if this would break the MaxSize limit. If a Size is specified then the chunk returned will end at
		 *  the first wrapping unit end encountered before Size. On no account will portions of two or more different wrapping
		 *  units be returned together. The mechanism for selecting a type of wrapping (e.g. frame, line or clip) is not 
		 *  (currently) part of the common EssenceSource interface.
		 *  \return Pointer to a data chunk holding the next data or a NULL pointer when no more remains
		 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
		 *	\note If Size = 0 the object will decide the size of the chunk to return
		 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
		 */
		virtual DataChunkPtr GetEssenceData(size_t Size = 0, size_t MaxSize = 0)
		{
			return Parent->GetEssenceData(this, Channel, ChannelCount, Size, MaxSize);
		}

		//! Did the last call to GetEssenceData() return the end of a wrapping item
		/*! \return true if the last call to GetEssenceData() returned an entire wrapping unit.
		 *  \return true if the last call to GetEssenceData() returned the last chunk of a wrapping unit.
		 *  \return true if the last call to GetEssenceData() returned the end of a clip-wrapped clip.
		 *  \return false if there is more data pending for the current wrapping unit.
		 *  \return false if the source is to be clip-wrapped and there is more data pending for the clip
		 */
		virtual bool EndOfItem(void) { return Eoi; }

		//! Set this channel's end-of-item flag
		void SetEoi(bool Val) { Eoi = Val; }

		//! Is all data exhasted?
		/*! \return true if a call to GetEssenceData() will return some valid essence data
		 */
		virtual bool EndOfData(void) { return Parent->Outputs[Channel].Eof; }

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual Uint8 GetGCEssenceType(void) { return Parent->Source->GetGCEssenceType(); }

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual Uint8 GetGCElementType(void) { return Parent->Source->GetGCElementType(); }

		//! Get the edit rate of this wrapping of the essence
		/*! \note This may not be the same as the original "native" edit rate of the
		 *        essence if this EssenceSource is wrapping to a different edit rate 
		 */
		virtual Rational GetEditRate(void) { return Parent->Source->GetEditRate(); }

		//! Get the current position in GetEditRate() sized edit units
		/*! This is relative to the start of the stream, so the first edit unit is always 0.
		 *  This is the same as the number of edit units read so far, so when the essence is 
		 *  exhausted the value returned shall be the size of the essence
		 */
		virtual Position GetCurrentPosition(void) { 
			Rational ER=GetEditRate();
			if(Parent->SourceAudioSampleRate)
			{
				Position Ret;
				if(BytesPerEditUnit) //constant edit rate
				{	//Ret=(Parent->Outputs[Channel].Pos*ER.Numerator/ER.Denominator)/Parent->SourceAudioSampleRate/BytesPerEditUnit;

					//Parent->Outputs[Channel].Pos is already in edit units and therefor the /BytesPerEditUnit should not be needed
					Ret=(Parent->Outputs[Channel].Pos*ER.Numerator)/(Parent->SourceAudioSampleRate * ER.Denominator);
					
				}
				else
				{
					float SamplesPerFrame = (float(ER.Denominator) * float(Parent->SourceAudioSampleRate)) / float(ER.Numerator);
					float CurrentPos=(float)Parent->Outputs[Channel].Pos;
					Ret=(Position)(CurrentPos/SamplesPerFrame);
				}
				return Ret;
			}
			else  //all we can do is return the number of samples returned
				return Parent->Outputs[Channel].Pos;
		}

		//! Get BytesPerEditUnit if Constant, else 0
		/*! \note This value may be useful even if CanIndex() returns false
		 */
		virtual Uint32 GetBytesPerEditUnit(Uint32 KAGSize = 1)
		{
			if((BytesPerEditUnit == -1) || BPEUKAGSize != KAGSize) CalcBytesPerEditUnit(KAGSize);

			return static_cast<Uint32>(BytesPerEditUnit);
		}

		//! Can this stream provide indexing
		/*! If true then SetIndex Manager can be used to set the index manager that will receive indexing data
		 */
		virtual bool CanIndex(void) { return Parent->Source->CanIndex(); }

	protected:
		//! Calculate BytesPerEditUnit for a given KAGSize
		void CalcBytesPerEditUnit(Uint32 KAGSize);
	};
}

#endif // MXFLIB__AUDIOMUX_H

