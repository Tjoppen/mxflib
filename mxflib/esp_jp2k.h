/*! \file	esp_jp2k.h
 *	\brief	Definition of class that handles parsing of JPEG 2000 files
 *
 *	\version $Id: esp_jp2k.h,v 1.5 2006/07/02 13:27:50 matt-beard Exp $
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

#ifndef MXFLIB__ESP_JP2K_H
#define MXFLIB__ESP_JP2K_H

#include <math.h>	// For "floor"


namespace mxflib
{
	class JP2K_EssenceSubParser : public EssenceSubParserBase
	{
	protected:
		UInt32 SampleRate;									//!< The sample rate of this essence
		Rational UseEditRate;								//!< The edit rate to use for wrapping this essence

		Position PictureNumber;								//!< The picture number of the last picture read, zero before any read

		Position DataStart;									//!< Start of essence data within the file
		Length DataSize;									//!< Total size of the essence data within the file
		Position CurrentPos;								//!< Current position in the input file (in bytes)
															/*!< A value of 0 means the start of the data chunk,
															 *	 any other value is that position within the whole file.
															 *	 This means that a full rewind can be achieved by setting CurrentPos = 0
															 *	 \note Other functions may move the file
															 *         pointer between calls to our functions */

		MDObjectParent CurrentDescriptor;					//!< Pointer to the last essence descriptor we built
															/*!< This is used as a quick-and-dirty check that we know how to process this source */

	public:
		//! Class for EssenceSource objects for parsing/sourcing <Type> essence
		class ESP_EssenceSource : public EssenceSubParserBase::ESP_EssenceSource
		{
		protected:
			Position EssenceBytePos;						//!< The current byte offset within the input file
			bool CountSet;									//!< Set true once we know the size of the current item
			size_t ByteCount;								//!< The size of the current essence item (if known)

		public:
			//! Construct and initialise for essence parsing/sourcing
			ESP_EssenceSource(EssenceSubParserPtr TheCaller, FileHandle InFile, UInt32 UseStream, UInt64 Count = 1/*, IndexTablePtr UseIndex = NULL*/)
				: EssenceSubParserBase::ESP_EssenceSource(TheCaller, InFile, UseStream, Count/*, UseIndex*/) 
			{
				JP2K_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, JP2K_EssenceSubParser);
				EssenceBytePos = pCaller->CurrentPos;
				if(EssenceBytePos == 0) EssenceBytePos = pCaller->DataStart;

				CountSet = false;		// Flag unknown size
			};

			//! Get the size of the essence data in bytes
			/*! \note There is intentionally no support for an "unknown" response 
			 */
			virtual size_t GetEssenceDataSize(void) 
			{
				CountSet = true;
				JP2K_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, JP2K_EssenceSubParser);
				ByteCount = pCaller->ReadInternal(File, Stream, RequestedCount);
				return ByteCount;
			};

			//! Get the next "installment" of essence data
			/*! \return Pointer to a data chunk holding the next data or a NULL pointer when no more remains
			 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
			 *	\note If Size = 0 the object will decide the size of the chunk to return
			 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
			 */
			virtual DataChunkPtr GetEssenceData(size_t Size = 0, size_t MaxSize = 0)
			{
				// Allow us to differentiate the first call
				if(!Started)
				{
					Started = true;

					JP2K_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, JP2K_EssenceSubParser);

					// Move to the selected position
					if(EssenceBytePos == 0) EssenceBytePos = pCaller->DataStart;
					pCaller->CurrentPos = EssenceBytePos;
				}

				return BaseGetEssenceData(Size, MaxSize);
			}
		};

		// Give our essence source class privilaged access
		friend class JP2K_EssenceSubParser::ESP_EssenceSource;

	public:
		JP2K_EssenceSubParser()
		{
			PictureNumber = 0;

			SampleRate = 1;
			DataStart = 0;
			DataSize = 0;
			CurrentPos = 0;

			// Use a sensible default if no edit rate is set - not ideal, but better than one sample!
			// It should always be possible to wrap at this rate, but the end of the data may not be a whole edit unit
			UseEditRate.Numerator = 1;
			UseEditRate.Denominator = 1;
		}

		//! Build a new parser of this type and return a pointer to it
		virtual EssenceSubParserPtr NewParser(void) const { return new JP2K_EssenceSubParser; }

		//! Report the extensions of files this sub-parser is likely to handle
		virtual StringList HandledExtensions(void)
		{
			StringList ExtensionList;

			// TODO: Add any supported extentions here
			//       This is used as a hint to the overall essence parser to decide which sub-parsers to try.
			//       Calls may still be made to this sub-parser for files of different extensions, but this is a starting point.

			ExtensionList.push_back("JP2");

			return ExtensionList;
		}

		//! Examine the open file and return a list of essence descriptors
		virtual EssenceStreamDescriptorList IdentifyEssence(FileHandle InFile);

		//! Examine the open file and return the wrapping options known by this parser
		virtual WrappingOptionList IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor &Descriptor);

		//! Set a wrapping option for future Read and Write calls
		/*! \return true if this EditRate is acceptable with this wrapping */
		virtual void Use(UInt32 Stream, WrappingOptionPtr UseWrapping)
		{
			SelectedWrapping = UseWrapping;

			CurrentPos = 0;
		}

		//! Set a non-native edit rate
		/*! Must be called <b>after</b> Use()
		 *	\return true if this rate is acceptable 
		 */
		virtual bool SetEditRate(Rational EditRate)
		{
			UseEditRate = EditRate;

			// Pretend that the essence is sampled at whatever rate we are wrapping at
			MDObjectPtr Ptr;
			if(CurrentDescriptor) Ptr = CurrentDescriptor->AddChild(SampleRate_UL);
			if(Ptr)
			{
				Ptr->SetInt("Numerator", UseEditRate.Numerator);
				Ptr->SetInt("Denominator", UseEditRate.Denominator);
			}
			
			return true;
		}

		//! Get the current edit rate
		virtual Rational GetEditRate(void) { return UseEditRate; }

		//! Get the preferred edit rate
		/*! \return The prefered edit rate which is currently 24/1
		 */
		virtual Rational GetPreferredEditRate(void) { return Rational(24, 1); };

		//! Get the current position in SetEditRate() sized edit units
		virtual Position GetCurrentPosition(void) { return PictureNumber; }

		//! Read a number of wrapping items from the specified stream and return them in a data chunk
		virtual DataChunkPtr Read(FileHandle InFile, UInt32 Stream, UInt64 Count = 1);

		//! Build an EssenceSource to read a number of wrapping items from the specified stream
		virtual EssenceSourcePtr GetEssenceSource(FileHandle InFile, UInt32 Stream, UInt64 Count = 1)
		{
			return new ESP_EssenceSource(this, InFile, Stream, Count);
		};

		//! Write a number of wrapping items from the specified stream to an MXF file
		virtual Length Write(FileHandle InFile, UInt32 Stream, MXFFilePtr OutFile, UInt64 Count = 1);

		//! Get a unique name for this sub-parser
		/*! The name must be all lower case, and must be unique.
		 *  The recommended name is the part of the filename of the parser header after "esp_" and before the ".h".
		 *  If the parser has no name return "" (however this will prevent named wrapping option selection for this sub-parser)
		 */
		virtual std::string GetParserName(void) const { return "jp2k"; }


	protected:
		//! Type for multimap holding contents of the jp2 file header
		typedef std::multimap<std::string, DataChunkPtr> HeaderType;

		//! Multimap holding contents of the jp2 file header
		HeaderType Header;

		//! Work out wrapping sequence
		bool CalcWrappingSequence(Rational EditRate);

		//! Read the essence information from the codestream at the specified position in the source file and build an essence descriptor
		/*! \note This call will modify properties SampleRate, DataStart and DataSize */
		MDObjectPtr BuildDescriptorFromCodeStream(FileHandle InFile, Position Offset = 0 );

		//! Read the essence information at the start of the "JP2" format source file and build an essence descriptor
		/*! \note This call will modify properties SampleRate, DataStart and DataSize */
		MDObjectPtr BuildDescriptorFromJP2(FileHandle InFile);

		//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
		size_t ReadInternal(FileHandle InFile, UInt32 Stream, Length Count);

		//! Parse a JP2 header at the start of the specified file into items in the Header multimap
		bool ParseJP2Header(FileHandle InFile);

		//! Parse a JPEG 2000 header at the specified offset in a file into items in the Header multimap
		/*! This parsing includes the first tile-part header
		 */
		bool ParseJP2KCodestreamHeader(FileHandle InFile, Position Offset);


		//! Return the greatest common divisor of two numbers
		UInt32 GreatestCommonDivisor(UInt32 Large, UInt32 Small)
		{
			// Zero is never the GCD
			if(Large == 0) return 1;

			UInt32 Temp;

			// The rest of the algorithm assumes Large >= Small
			if(Large < Small)
			{
				Temp = Large;
				Large = Small;
				Small = Temp;
			}

			while (Small > 0)
			{
				Temp = Large % Small;
				Large = Small;
				Small = Temp;
			}
			
			return Large;
		}

		//! Reduce the complexity of a given rational made from a pair of UInt32s
		void ReduceRational(UInt32 &Numerator, UInt32 &Denominator)
		{
			if(Denominator == 1) return;

			UInt32 GCD = GreatestCommonDivisor(Numerator, Denominator);
			
			Numerator /= GCD;
			Denominator /= GCD;
		}
	};
}

#endif // MXFLIB__ESP_JP2K_H
