/*! \file	esp_wavepcm.h
 *	\brief	Definition of class that handles parsing of uncompressed pcm wave audio files
 *
 *	\version $Id: esp_wavepcm.h,v 1.1.2.3 2004/07/15 16:30:38 matt-beard Exp $
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

#ifndef MXFLIB__ESP_WAVEPCM_H
#define MXFLIB__ESP_WAVEPCM_H

#include <math.h>	// For "floor"


namespace mxflib
{
	class WAVE_PCM_EssenceSubParser : public EssenceSubParserBase
	{
	protected:
		WrappingOption::WrapType SelectedWrapping;			//!< The wrapping type selected

		Uint32 SampleRate;									//!< The sample rate of this essence

		Uint64 DataStart;									//!< Start of "data" chunk (value part)
		Uint64 DataSize;									//!< Size of "data" chunk (value part)
		Uint64 CurrentPos;									//!< Current position in the input file
															/*!< A value of 0 means the start of the data chunk,
															 *	 any other value is that position within the whole file.
															 *	 This means that a full rewind can be achieved by setting CurrentPos = 0
															 *	 \note Other functions may move the file
															 *         pointer between calls to our functions */

		int SampleSize;										//!< Size of each sample in bytes (includes all channels)
		Uint32 ConstSamples;								//!< Number of samples per edit unit (if constant, else zero)
		int SampleSequenceSize;								//!< Size of SampleSequence if used
		Uint32 *SampleSequence;								//!< Array of counts of samples per edit unit for non integer relationships between edit rate and sample rate
		int SequencePos;									//!< Current position in the sequence (i.e. next entry to use)

	public:
		//! Class for EssenceSource objects for parsing/sourcing MPEG-VES essence
		class ESP_EssenceSource : public EssenceSubParserBase::ESP_EssenceSource
		{
		protected:
			Uint64 EssenceBytePos;
			bool CountSet;
			Uint64 ByteCount;
			Uint64 Offset;

		public:
			//! Construct and initialise for essence parsing/sourcing
			ESP_EssenceSource(EssenceSubParserBase *TheCaller, FileHandle InFile, Uint32 UseStream, Uint64 Count = 1/*, IndexTablePtr UseIndex = NULL*/)
				: EssenceSubParserBase::ESP_EssenceSource(TheCaller, InFile, UseStream, Count/*, UseIndex*/) 
			{
				WAVE_PCM_EssenceSubParser *pCaller = (WAVE_PCM_EssenceSubParser*) TheCaller;
				EssenceBytePos = pCaller->CurrentPos;
				if(EssenceBytePos == 0) EssenceBytePos = pCaller->DataStart;

				CountSet = false;		// Flag unknown size
			};

			//! Get the size of the essence data in bytes
			/*! \note There is intentionally no support for an "unknown" response 
			 */
			virtual Uint64 GetEssenceDataSize(void) 
			{
				CountSet = true;
				Offset = 0;
				WAVE_PCM_EssenceSubParser *pCaller = (WAVE_PCM_EssenceSubParser*) Caller;
				ByteCount = pCaller->ReadInternal(File, Stream, RequestedCount);
				return ByteCount;
			};

			//! Get the next "installment" of essence data
			/*! \return Pointer to a data chunk holding the next data or a NULL pointer when no more remains
			 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
			 *	\note If Size = 0 the object will decide the size of the chunk to return
			 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
			 */
			virtual DataChunkPtr GetEssenceData(Uint64 Size = 0, Uint64 MaxSize = 0)
			{
				WAVE_PCM_EssenceSubParser *pCaller = (WAVE_PCM_EssenceSubParser*) Caller;

				// Allow us to differentiate the first call
				if(!Started)
				{
					if(!CountSet)
					{
						CountSet = true;
						ByteCount = pCaller->ReadInternal(File, Stream, RequestedCount);
						Offset = 0;
					}

					// Move to the selected position
					if(EssenceBytePos == 0) EssenceBytePos = pCaller->DataStart;
					pCaller->CurrentPos = EssenceBytePos;

					Started = true;
				}

				// Flag the end when no more to read
				if(ByteCount <= Offset) return NULL;

				// Decide how much to actually read
				if(Size == 0)
				{
					Size = ByteCount - Offset;
					if((MaxSize) && (Size > MaxSize)) Size = MaxSize;
				}

				// Read the data
				FileSeek(File, EssenceBytePos + Offset);
				DataChunkPtr Data = FileReadChunk(File, Size);

				// Update the position
				pCaller->CurrentPos += Data->Size;
				Offset += Data->Size;

				return Data;
			}
		};

		// Give our essence source class privilaged access
		friend class WAVE_PCM_EssenceSubParser::ESP_EssenceSource;

	public:
		WAVE_PCM_EssenceSubParser()
		{
			ConstSamples = 0;
			SampleSequenceSize = 0;
			SampleSequence = NULL;
			SequencePos = 0;
			DataStart = 0;
			DataSize = 0;
//			CurrentPos = 0;		// DRAGONS: Why was this removed?
		}

		//! Build a new parser of this type and return a pointer to it
		virtual EssenceSubParserBase *NewParser(void) const { return new WAVE_PCM_EssenceSubParser; }

		//! Report the extensions of files this sub-parser is likely to handle
		virtual StringList HandledExtensions(void)
		{
			StringList ExtensionList;

			ExtensionList.push_back("WAV");

			return ExtensionList;
		}

		//! Examine the open file and return a list of essence descriptors
		virtual EssenceStreamDescriptorList IdentifyEssence(FileHandle InFile);

		//! Examine the open file and return the wrapping options known by this parser
		virtual WrappingOptionList IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor Descriptor);

		//! Set a wrapping option for future Read and Write calls
		/*! \return true if this EditRate is acceptable with this wrapping */
		virtual void Use(Uint32 Stream, WrappingOptionPtr UseWrapping)
		{
			SelectedWrapping = UseWrapping->ThisWrapType;

			CurrentPos = 0;
		}


		//! Set a non-native edit rate
		/*! Must be called <b>after</b> Use()
		 *	\return true if this rate is acceptable 
		 */
		virtual bool SetEditRate(Uint32 Stream, Rational EditRate)
		{
			SequencePos = 0;
			return CalcWrappingSequence(EditRate);
		}

		//! Get BytesPerEditUnit, if Constant
		virtual Uint32 GetBytesPerEditUnit()
		{
			if(SelectedWrapping == WrappingOption::WrapType::Frame) 
			{
				// FIXME: This assumes that 4-byte BER coding will be used - this needs to be adjusted or forced to be true!!
				return SampleSize*ConstSamples + 16 + 4;
			}
			else return SampleSize*ConstSamples;
		}

		//! Get the current position in SetEditRate() sized edit units
		virtual Int64 GetCurrentPosition(void);

		//! Read a number of wrapping items from the specified stream and return them in a data chunk
		virtual DataChunkPtr Read(FileHandle InFile, Uint32 Stream, Uint64 Count = 1/*, IndexTablePtr Index = NULL*/);

		//! Build an EssenceSource to read a number of wrapping items from the specified stream
		virtual EssenceSubParserBase::ESP_EssenceSource *GetEssenceSource(FileHandle InFile, Uint32 Stream, Uint64 Count = 1/*, IndexTablePtr Index = NULL*/)
		{
			return new ESP_EssenceSource(this, InFile, Stream, Count/*, Index*/);
		};

		//! Write a number of wrapping items from the specified stream to an MXF file
		virtual Uint64 Write(FileHandle InFile, Uint32 Stream, MXFFilePtr OutFile, Uint64 Count = 1/*, IndexTablePtr Index = NULL*/);


	protected:
		//! Work out wrapping sequence
		bool CalcWrappingSequence(Rational EditRate);

		//! Read the sequence header at the specified position in an MPEG2 file to build an essence descriptor
		MDObjectPtr BuildWaveAudioDescriptor(FileHandle InFile, Uint64 Start = 0);

		//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
		Uint64 ReadInternal(FileHandle InFile, Uint32 Stream, Uint64 Count);

	};
}

#endif // MXFLIB__ESP_WAVEPCM_H
