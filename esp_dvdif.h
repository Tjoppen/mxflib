/*! \file	esp_dvdif.h
 *	\brief	Definition of class that handles parsing of DV-DIF streams
 */
/*
 *	$Id: esp_dvdif.h,v 1.2 2003/12/04 13:55:21 stuart_hc Exp $
 *
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
	//! Class that handles parsing of MPEG-2 video elementary streams
	class DV_DIF_EssenceSubParser : public EssenceSubParserBase
	{
	protected:
		WrappingOption::WrapType SelectedWrapping;			//!< The wrapping type selected

		Rational NativeEditRate;							//!< The native edit rate of this essence
		Rational SelectedEditRate;							//!< Selected edit rate of this essence
		unsigned int EditRatio;								//!< Ratio of selected to native edit rate

		Uint64 PictureNumber;								//!< Current picture number
		Uint64 CurrentPos;									//!< Current position in the input file

		Uint64 DIFStart;									//!< Byte offset of first byte of first DIF
		Uint64 DIFEnd;										//!< Byte offset of last byte of last DIF + 1

		// File buffering
		Uint8 Buffer[DV_DIF_BUFFERSIZE];					//!< Buffer for efficient file reading
		int BuffCount;										//!< Count of bytes still unread in Buffer
		Uint8 *BuffPtr;										//!< Pointer to next byte to read from Buffer

		// Options
		// None (yet)

	public:
		//! Class for EssenceSource objects for parsing/sourcing DV-DIF essence
		class ESP_EssenceSource : public EssenceSubParserBase::ESP_EssenceSource
		{
		protected:
			Uint64 EssencePos;

		public:
			//! Construct and initialise for essence parsing/sourcing
			ESP_EssenceSource(EssenceSubParserBase *TheCaller, FileHandle InFile, Uint32 UseStream, Uint64 Count = 1, IndexTablePtr UseIndex = NULL)
				: EssenceSubParserBase::ESP_EssenceSource(TheCaller, InFile, UseStream, Count, UseIndex) 
			{
				DV_DIF_EssenceSubParser *pCaller = (DV_DIF_EssenceSubParser*) TheCaller;
				EssencePos = pCaller->PictureNumber;
			};

			//! Get the size of the essence data in bytes
			/*! \note There is intentionally no support for an "unknown" response 
			 */
			virtual Uint64 GetEssenceDataSize(void) 
			{
				DV_DIF_EssenceSubParser *pCaller = (DV_DIF_EssenceSubParser*) Caller;
				return pCaller->ReadInternal(File, Stream, RequestedCount, Index);
			};

			//! Get the next "installment" of essence data
			/*! \ret Pointer to a data chunk holding the next data or a NULL pointer when no more remains
			 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
			 *	\note If Size = 0 the object will decide the size of the chunk to return
			 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
			 */
			virtual DataChunkPtr GetEssenceData(Uint64 Size = 0, Uint64 MaxSize = 0)
			{
				// Allow us to differentiate the first call
				if(!Started)
				{
					// Move to the selected position
					DV_DIF_EssenceSubParser *pCaller = (DV_DIF_EssenceSubParser*) Caller;
					pCaller->PictureNumber = EssencePos;

					Started = true;
				}

				return BaseGetEssenceData(Size, MaxSize);
			}
		};

		// Give our essence source class privilaged access
		friend class DV_DIF_EssenceSubParser::ESP_EssenceSource;

	public:
		DV_DIF_EssenceSubParser()
		{
			DIFStart = 0;
			DIFEnd = 0;
		}

		//! Build a new parser of this type and return a pointer to it
		virtual EssenceSubParserBase *NewParser(void) const { return new DV_DIF_EssenceSubParser; }

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
		virtual WrappingOptionList IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor Descriptor);

		//! Set a wrapping option for future Read and Write calls
		virtual void Use(Uint32 Stream, WrappingOptionPtr UseWrapping);

		//! Set a non-native edit rate
		virtual bool SetEditRate(Uint32 Stream, Rational EditRate);

		//! Read a number of wrapping items from the specified stream and return them in a data chunk
		virtual DataChunkPtr Read(FileHandle InFile, Uint32 Stream, Uint64 Count = 1, IndexTablePtr Index = NULL);

		//! Build an EssenceSource to read a number of wrapping items from the specified stream
		virtual EssenceSubParserBase::ESP_EssenceSource *GetEssenceSource(FileHandle InFile, Uint32 Stream, Uint64 Count = 1, IndexTablePtr Index = NULL)
		{
			return new ESP_EssenceSource(this, InFile, Stream, Count, Index);
		};

		//! Write a number of wrapping items from the specified stream to an MXF file
		virtual Uint64 Write(FileHandle InFile, Uint32 Stream, MXFFilePtr OutFile, Uint64 Count = 1, IndexTablePtr Index = NULL);

		//! Set a parser specific option
		/*! \return true if the option was successfully set */
		virtual bool SetOption(std::string Option, Int64 Param = 0);

	protected:
		//! Read the header at the specified position in a DV file to build an essence descriptor
		MDObjectPtr BuildCDCIEssenceDescriptor(FileHandle InFile, Uint64 Start = 0);

		//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
		Uint64 ReadInternal(FileHandle InFile, Uint32 Stream, Uint64 Count, IndexTablePtr Index = NULL);

		//! Get a byte from the current stream
		int BuffGetU8(FileHandle InFile);
	
	};
}

#endif // MXFLIB__ESP_DVDIF_H
