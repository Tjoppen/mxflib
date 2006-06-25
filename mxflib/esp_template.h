/*! \file	esp_template.h
 *	\brief	Definition of class that handles parsing of <File Type>
 *
 *	\version $Id: esp_template.h,v 1.2 2006/06/25 14:14:12 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2004, Matt Beard
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

#ifndef MXFLIB__ESP_TEMPLATE_H
#define MXFLIB__ESP_TEMPLATE_H

#include <math.h>	// For "floor"


namespace mxflib
{
	class TEMPLATE_EssenceSubParser : public EssenceSubParserBase
	{
	protected:
		UInt32 SampleRate;									//!< The sample rate of this essence
		Rational UseEditRate;								//!< The edit rate to use for wrapping this essence

		Position DataStart;									//!< Start of essence data within the file
		Length DataSize;									//!< Total size of the essence data within the file
		Position CurrentPos;								//!< Current position in the input file (in bytes)
															/*!< A value of 0 means the start of the data chunk,
															 *	 any other value is that position within the whole file.
															 *	 This means that a full rewind can be achieved by setting CurrentPos = 0
															 *	 \note Other functions may move the file
															 *         pointer between calls to our functions */

		int SampleSize;										//!< Size of each sample in bytes (if constant)
		UInt32 ConstSamples;								//!< Number of samples per edit unit (if constant, else zero)
		int SampleSequenceSize;								//!< Size of SampleSequence if used
		UInt32 *SampleSequence;								//!< Array of counts of samples per edit unit for non integer relationships between edit rate and sample rate
		int SequencePos;									//!< Current position in the sequence (i.e. next entry to use)

		MDObjectParent CurrentDescriptor;					//!< Pointer to the last essence descriptor we built
															/*!< This is used as a quick-and-dirty check that we know how to process this source */

	public:
		//! Class for EssenceSource objects for parsing/sourcing <Type> essence
		class ESP_EssenceSource : public EssenceSubParserBase::ESP_EssenceSource
		{
		protected:
			Position EssenceBytePos;
			bool CountSet;
			Length ByteCount;
			Position Offset;

		public:
			//! Construct and initialise for essence parsing/sourcing
			ESP_EssenceSource(EssenceSubParserPtr TheCaller, FileHandle InFile, UInt32 UseStream, UInt64 Count = 1/*, IndexTablePtr UseIndex = NULL*/)
				: EssenceSubParserBase::ESP_EssenceSource(TheCaller, InFile, UseStream, Count/*, UseIndex*/) 
			{
				TEMPLATE_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, TEMPLATE_EssenceSubParser);
				EssenceBytePos = pCaller->CurrentPos;
				if(EssenceBytePos == 0) EssenceBytePos = pCaller->DataStart;

				CountSet = false;		// Flag unknown size
			};

			//! Get the size of the essence data in bytes
			/*! \note There is intentionally no support for an "unknown" response 
			 */
			virtual Length GetEssenceDataSize(void) 
			{
				CountSet = true;
				Offset = 0;
				TEMPLATE_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, TEMPLATE_EssenceSubParser);
				ByteCount = pCaller->ReadInternal(File, Stream, RequestedCount);
				return ByteCount;
			};

			//! Get the next "installment" of essence data
			/*! \return Pointer to a data chunk holding the next data or a NULL pointer when no more remains
			 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
			 *	\note If Size = 0 the object will decide the size of the chunk to return
			 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
			 */
			virtual DataChunkPtr GetEssenceData(UInt64 Size = 0, UInt64 MaxSize = 0)
			{
				// Allow us to differentiate the first call
				if(!Started)
				{
					Started = true;

					TEMPLATE_EssenceSubParser *pCaller = SmartPtr_Cast(Caller, TEMPLATE_EssenceSubParser);

					// Move to the selected position
					if(EssenceBytePos == 0) EssenceBytePos = pCaller->DataStart;
					pCaller->CurrentPos = EssenceBytePos;
				}

				return BaseGetEssenceData(Size, MaxSize);
			}
		};

		// Give our essence source class privilaged access
		friend class TEMPLATE_EssenceSubParser::ESP_EssenceSource;

	public:
		TEMPLATE_EssenceSubParser()
		{
			SampleRate = 1;
			ConstSamples = 0;
			SampleSequenceSize = 0;
			SampleSequence = NULL;
			SequencePos = 0;
			DataStart = 0;
			DataSize = 0;
			CurrentPos = 0;

			// Use a sensible default if no edit rate is set - not ideal, but better than one sample!
			// It should always be possible to wrap at this rate, but the end of the data may not be a whole edit unit
			UseEditRate.Numerator = 1;
			UseEditRate.Denominator = 1;
		}

		//! Report the extensions of files this sub-parser is likely to handle
		virtual StringList HandledExtensions(void)
		{
			StringList ExtensionList;

			// TODO: Add any supported extentions here
			//       This is used as a hint to the overall essence parser to decide which sub-parsers to try.
			//       Calls may still be made to this sub-parser for files of different extensions, but this is a starting point.

			// ExtensionList.push_back("XXX");

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
			// See if we can figure out a sequence for this rate
			bool Ret = CalcWrappingSequence(EditRate);

			// If we can then set the rate
			if(Ret)
			{
				SequencePos = 0;
				UseEditRate = EditRate;
			}

			return Ret;
		}

		//! Get the current edit rate
		virtual Rational GetEditRate(void) { return UseEditRate; }

		//! Get the preferred edit rate (if one is known)
		/*! \return The prefered edit rate or 0/0 if note known
		 */
		virtual Rational GetPreferredEditRate(void);

		//! Get BytesPerEditUnit, if Constant
		virtual UInt32 GetBytesPerEditUnit(UInt32 KAGSize = 1)
		{
			// If we haven't determined the sample sequence we do it now
			if((ConstSamples == 0) && (SampleSequenceSize == 0)) CalcWrappingSequence(UseEditRate);

			UInt32 Ret = SampleSize*ConstSamples;

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
		virtual DataChunkPtr Read(FileHandle InFile, UInt32 Stream, UInt64 Count = 1);

		//! Build an EssenceSource to read a number of wrapping items from the specified stream
		virtual EssenceSubParserBase::ESP_EssenceSource *GetEssenceSource(FileHandle InFile, UInt32 Stream, UInt64 Count = 1)
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
		// TODO: Fill in the parser name
		virtual std::string GetParserName(void) const { return "template"; }


	protected:
		//! Work out wrapping sequence
		bool CalcWrappingSequence(Rational EditRate);

		//! Read the essence information at the specified position in the source file and build an essence descriptor
		MDObjectPtr BuildDescriptor(FileHandle InFile, UInt64 Start = 0);

		//! Scan the essence to calculate how many bytes to transfer for the given edit unit count
		Length ReadInternal(FileHandle InFile, UInt32 Stream, UInt64 Count);
	};


	//! Factory class for making <Type> parsers
	class TEMPLATE_EssenceSubParserFactory : public EssenceSubParserFactory
	{
	public:
		//! Build a new <Type> parser and return a pointer to it
		virtual EssenceSubParserPtr NewParser(void) const { return new TEMPLATE_EssenceSubParser; }
	};
}

#endif // MXFLIB__ESP_TEMPLATE_H
