/*! \file	esp_mpeg2ves.h
 *	\brief	Definition of class that handles parsing of MPEG-2 video elementary streams
 *
 *	\version $Id: esp_mpeg2ves.h,v 1.4 2005/07/19 13:38:41 matt-beard Exp $
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

#ifndef MXFLIB__ESP_MPEG2VES_H
#define MXFLIB__ESP_MPEG2VES_H


#include <math.h>	// For "floor"

#define MPEG2_VES_BUFFERSIZE  8192

namespace mxflib
{
	//! Class that handles parsing of MPEG-2 video elementary streams
	class MPEG2_VES_EssenceSubParser : public EssenceSubParserBase
	{
	protected:
		Rational NativeEditRate;							//!< The native edit rate of this essence
		Rational SelectedEditRate;							//!< Selected edit rate of this essence
		unsigned int EditRatio;								//!< Ratio of selected to native edit rate

		Position PictureNumber;								//!< Current picture number
		Position AnchorFrame;								//!< Picture number of last "anchor frame"
		Position CurrentPos;								//!< Current position in the input file
															/*!< \note Other functions may move the file
															 *         pointer between calls to our functions */
		int GOPOffset;										//!< The stream position of this picture in the GOP
															/*!< The first picture in the GOP is picture 0 */

		bool ClosedGOP;										//!< True if the current GOP is flagged as closed

		// File buffering
		Uint8 Buffer[MPEG2_VES_BUFFERSIZE];					//!< Buffer for efficient file reading
		int BuffCount;										//!< Count of bytes still unread in Buffer
		Uint8 *BuffPtr;										//!< Pointer to next byte to read from Buffer

		bool EditPoint;										//!< Set true each time an edit point (sequence header of a closed GOP) and false for other frames
															/*!< \note This flag can be checked by calling SetOption("EditPoint") which will return the flag.
															 *         The flag tells only if the last frame is the start of a new sequence.
															 */
		bool SelectiveIndex;								//!< True if each index entry is stored and only added to index by a call to SetOption("AddIndexEntry");

		bool EndOfStream;									//!< True once the end of the stream has been read

		MDObjectParent CurrentDescriptor;					//!< Pointer to the last essence descriptor we built
															/*!< This is used as a quick-and-dirty check that we know how to process this source */

	public:
		//! Class for EssenceSource objects for parsing/sourcing MPEG-VES essence
		class ESP_EssenceSource : public EssenceSubParserBase::ESP_EssenceSource
		{
		protected:
			Position EssencePos;
			Position EssenceBytePos;
			Position ByteCount;
			Position Offset;

		public:
			//! Construct and initialise for essence parsing/sourcing
			ESP_EssenceSource(EssenceSubParserPtr TheCaller, FileHandle InFile, Uint32 UseStream, Uint64 Count = 1)
				: EssenceSubParserBase::ESP_EssenceSource(TheCaller, InFile, UseStream, Count) 
			{
				MPEG2_VES_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, MPEG2_VES_EssenceSubParser);
				EssencePos = pCaller->PictureNumber;
				EssenceBytePos = pCaller->CurrentPos;
			};

			//! Get the size of the essence data in bytes
			/*! \note There is intentionally no support for an "unknown" response 
			 */
			virtual Length GetEssenceDataSize(void) 
			{
				Offset = 0;
				MPEG2_VES_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, MPEG2_VES_EssenceSubParser);
				
				if(pCaller->SelectedWrapping->ThisWrapType == WrappingOption::Clip)
					ByteCount = pCaller->ReadInternal(File, Stream, 0);
				else
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
				// Allow us to differentiate the first call
				if(!Started)
				{
					MPEG2_VES_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, MPEG2_VES_EssenceSubParser);

					// Move to the selected position
					pCaller->PictureNumber = EssencePos;
					pCaller->CurrentPos = EssenceBytePos;

					Offset = 0;

					Started = true;
				}

				return BaseGetEssenceData(Size, MaxSize);
			}

			//! Is the last data read the start of an edit point?
			virtual bool IsEditPoint(void) 
			{
				MPEG2_VES_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, MPEG2_VES_EssenceSubParser);
				return pCaller->EditPoint; 
			}
		};

		// Give our essence source class privilaged access
		friend class MPEG2_VES_EssenceSubParser::ESP_EssenceSource;

	public:
		// TODO: Check why properties are not initialised here!
		MPEG2_VES_EssenceSubParser()
		{
			EditPoint = false;
			EndOfStream = false;
		}

		//! Build a new parser of this type and return a pointer to it
		virtual EssenceSubParserPtr NewParser(void) const { return new MPEG2_VES_EssenceSubParser; }

		//! Report the extensions of files this sub-parser is likely to handle
		virtual StringList HandledExtensions(void);

		//! Examine the open file and return a list of essence descriptors
		virtual EssenceStreamDescriptorList IdentifyEssence(FileHandle InFile);

		//! Examine the open file and return the wrapping options known by this parser
		virtual WrappingOptionList IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor &Descriptor);

		//! Set a wrapping option for future Read and Write calls
		virtual void Use(Uint32 Stream, WrappingOptionPtr &UseWrapping);

		//! Set a non-native edit rate
		virtual bool SetEditRate(Rational EditRate);

		//! Get the current edit rate
		virtual Rational GetEditRate(void) { return SelectedEditRate; }

		//! Get the current position in SetEditRate() sized edit units
		virtual Position GetCurrentPosition(void);

		// Index table functions
		
		//! Set the IndexManager for this essence stream (and the stream ID if we are not the main stream)
		/*! Also force reordering to be used */
		virtual void SetIndexManager(IndexManagerPtr &TheManager, int StreamID = 0)
		{
			EssenceSubParserBase::SetIndexManager(TheManager, StreamID);
			TheManager->SetPosTableIndex(StreamID, -1);
		}

		//! Read a number of wrapping items from the specified stream and return them in a data chunk
		virtual DataChunkPtr Read(FileHandle InFile, Uint32 Stream, Uint64 Count = 1);

		//! Build an EssenceSource to read a number of wrapping items from the specified stream
		virtual EssenceSubParserBase::ESP_EssenceSource *GetEssenceSource(FileHandle InFile, Uint32 Stream, Uint64 Count = 1)
		{
			return new ESP_EssenceSource(this, InFile, Stream, Count/*, Index*/);
		};

		//! Write a number of wrapping items from the specified stream to an MXF file
		virtual Length Write(FileHandle InFile, Uint32 Stream, MXFFilePtr OutFile, Uint64 Count = 1/*, IndexTablePtr Index = NULL*/);

		//! Set a parser specific option
		/*! \return true if the option was successfully set */
		virtual bool SetOption(std::string Option, Int64 Param = 0);

	protected:
		//! Read the sequence header at the specified position in an MPEG2 file to build an essence descriptor
		MDObjectPtr BuildMPEG2VideoDescriptor(FileHandle InFile, Uint64 Start = 0);

		//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
		Length ReadInternal(FileHandle InFile, Uint32 Stream, Uint64 Count);

		//! Get a byte from the current stream
		int BuffGetU8(FileHandle InFile);
	};

}


#endif // MXFLIB__ESP_MPEG2VES_H
