/*! \file	esp_dvdif.h
 *	\brief	Definition of class that handles parsing of DV-DIF streams
 *
 *	\version $Id: esp_dvdif.h,v 1.11 2006/09/11 09:09:57 matt-beard Exp $
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

#ifndef MXFLIB__ESP_DVDIF_H
#define MXFLIB__ESP_DVDIF_H


#include <math.h>	// For "floor"

#define DV_DIF_BUFFERSIZE  (256 * 1024)

namespace mxflib
{
	//! Class that handles parsing of DV-DIf streams
	class DV_DIF_EssenceSubParser : public EssenceSubParserBase
	{
	protected:
		Rational NativeEditRate;							//!< The native edit rate of this essence
		Rational SelectedEditRate;							//!< Selected edit rate of this essence
		unsigned int EditRatio;								//!< Ratio of selected to native edit rate

		Position PictureNumber;								//!< Current picture number
		Position CurrentPos;								//!< Current position in the input file

		Position DIFStart;									//!< Byte offset of first byte of first DIF
		Position DIFEnd;									//!< Byte offset of last byte of last DIF + 1

		int SeqCount;										//!< Number of DIF sequences in a frame


		size_t CachedDataSize;								//!< The size of the next data to be read, or (size_t)-1 if not known

		// File buffering
		UInt8 *Buffer;										//!< Buffer for efficient file reading

		int BuffCount;										//!< Count of bytes still unread in Buffer
		UInt8 *BuffPtr;										//!< Pointer to next byte to read from Buffer

		MDObjectParent CurrentDescriptor;					//!< Pointer to the last essence descriptor we built
															/*!< This is used as a quick-and-dirty check that we know how to process this source */

		// Options
		// None (yet)

	public:
		//! Class for EssenceSource objects for parsing/sourcing DV-DIF essence
		class ESP_EssenceSource : public EssenceSubParserBase::ESP_EssenceSource
		{
		public:
			//! Construct and initialise for essence parsing/sourcing
			ESP_EssenceSource(EssenceSubParserPtr TheCaller, FileHandle InFile, UInt32 UseStream, UInt64 Count = 1/*, IndexTablePtr UseIndex = NULL*/)
				: EssenceSubParserBase::ESP_EssenceSource(TheCaller, InFile, UseStream, Count/*, UseIndex*/) 
			{
			};

			//! Get the size of the essence data in bytes
			/*! \note There is intentionally no support for an "unknown" response 
			 */
			virtual size_t GetEssenceDataSize(void) 
			{
				DV_DIF_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, DV_DIF_EssenceSubParser);
				return pCaller->ReadInternal(File, Stream, RequestedCount/*, Index*/);
			};

			//! Get the next "installment" of essence data
			/*! \return Pointer to a data chunk holding the next data or a NULL pointer when no more remains
			 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
			 *	\note If Size = 0 the object will decide the size of the chunk to return
			 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
			 */
			virtual DataChunkPtr GetEssenceData(size_t Size = 0, size_t MaxSize = 0)
			{
/*				// Allow us to differentiate the first call
				if(!Started)
				{
					// Move to the selected position
					DV_DIF_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, DV_DIF_EssenceSubParser);

					Started = true;
				}
*/

				return BaseGetEssenceData(Size, MaxSize);
			}

			//! Get the preferred BER length size for essence KLVs written from this source, 0 for auto
			virtual int GetBERSize(void) 
			{ 
				DV_DIF_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, DV_DIF_EssenceSubParser);

				if(pCaller->SelectedWrapping->ThisWrapType == WrappingOption::Clip) return 8;
				return 4;
			}
		};

		// Give our essence source class privilaged access
		friend class DV_DIF_EssenceSubParser::ESP_EssenceSource;

	public:
		DV_DIF_EssenceSubParser()
		{
			DIFStart = 0;
			DIFEnd = 0;
			SeqCount = 10;
			Buffer = NULL;

			CachedDataSize = static_cast<size_t>(-1);
		}

		~DV_DIF_EssenceSubParser()
		{
			// Free our buffer if we have allocated one
			if(Buffer) delete[] Buffer;
		}

		//! Build a new parser of this type and return a pointer to it
		virtual EssenceSubParserPtr NewParser(void) const;

		//! Report the extensions of files this sub-parser is likely to handle
		virtual StringList HandledExtensions(void)
		{
			StringList ExtensionList;

			ExtensionList.push_back("AVI");
			ExtensionList.push_back("DV");
			ExtensionList.push_back("DIF");

			return ExtensionList;
		}

		//! Examine the open file and return a list of essence descriptors
		virtual EssenceStreamDescriptorList IdentifyEssence(FileHandle InFile);

		//! Examine the open file and return the wrapping options known by this parser
		virtual WrappingOptionList IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor &Descriptor);

		//! Set a wrapping option for future Read and Write calls
		virtual void Use(UInt32 Stream, WrappingOptionPtr &UseWrapping);

		//! Set a non-native edit rate
		virtual bool SetEditRate(Rational EditRate);

		//! Get the current edit rate
		virtual Rational GetEditRate(void) { return SelectedEditRate; }

		//! Get BytesPerEditUnit, if Constant
		virtual UInt32 GetBytesPerEditUnit(UInt32 KAGSize = 1)
		{
			// FIXME: Assumes 25Mbps
			UInt32 Ret = (150 * 80 * SeqCount);

			if(SelectedWrapping->ThisWrapType == WrappingOption::Frame) 
			{
				// FIXME: This assumes that 4-byte BER coding will be used - this needs to be adjusted or forced to be true!!
				Ret += 16 + 4;

				// Adjust for whole KAGs if required
				if(KAGSize > 1)
				{
					// Work out how much short of the next KAG boundary we would be
					UInt32 Remainder = Ret % KAGSize;
					if(Remainder) Remainder = KAGSize - Remainder;

					// Round up to the start of the next KAG
					Ret += Remainder;

					// If there is not enough space to fit a filler in the remaining space an extra KAG will be required
					if((Remainder > 0) && (Remainder < 17)) Ret++;
				}
			}

			return Ret;
		}

		//! Get the current position in SetEditRate() sized edit units
		virtual Position GetCurrentPosition(void);

		//! Read a number of wrapping items from the specified stream and return them in a data chunk
		virtual DataChunkPtr Read(FileHandle InFile, UInt32 Stream, UInt64 Count = 1/*, IndexTablePtr Index = NULL*/);

		//! Build an EssenceSource to read a number of wrapping items from the specified stream
		virtual EssenceSourcePtr GetEssenceSource(FileHandle InFile, UInt32 Stream, UInt64 Count = 1/*, IndexTablePtr Index = NULL*/)
		{
			return new ESP_EssenceSource(this, InFile, Stream, Count/*, Index*/);
		};

		//! Write a number of wrapping items from the specified stream to an MXF file
		virtual Length Write(FileHandle InFile, UInt32 Stream, MXFFilePtr OutFile, UInt64 Count = 1/*, IndexTablePtr Index = NULL*/);

		//! Set a parser specific option
		/*! \return true if the option was successfully set */
		virtual bool SetOption(std::string Option, Int64 Param = 0);

		//! Get a unique name for this sub-parser
		/*! The name must be all lower case, and must be unique.
		 *  The recommended name is the part of the filename of the parser header after "esp_" and before the ".h".
		 *  If the parser has no name return "" (however this will prevent named wrapping option selection for this sub-parser)
		 */
		virtual std::string GetParserName(void) const { return "dvdif"; }

	protected:
		//! Read the header at the specified position in a DV file to build a video essence descriptor
		MDObjectPtr BuildCDCIEssenceDescriptor(FileHandle InFile, UInt64 Start = 0);

		//! Read the header at the specified position in a DV file to build an audio essence descriptor
		MDObjectPtr BuildSoundEssenceDescriptor(FileHandle InFile, UInt64 Start = 0);

		//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
		size_t ReadInternal(FileHandle InFile, UInt32 Stream, UInt64 Count/*, IndexTablePtr Index = NULL*/);
	};


	//! Factory class for making DV-DIF parsers
	class DV_DIF_EssenceSubParserFactory : public EssenceSubParserFactory
	{
	public:
		//! Build a new DV-DIF parser and return a pointer to it
		virtual EssenceSubParserPtr NewParser(void) const { return new DV_DIF_EssenceSubParser; }
	};

}

#endif // MXFLIB__ESP_DVDIF_H
