/*! \file	vbi.h
 *	\brief	Definition of classes that handle Vertical Inerval Blanking data
 *
 *	\version $Id: vbi.h,v 1.2 2006/07/02 13:27:51 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2006, Matt Beard
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
#ifndef MXFLIB__VBI_H
#define MXFLIB__VBI_H


// Forward refs
namespace mxflib
{
}


namespace mxflib
{
	//! Wrapping type enumeration, as per SMPTE-436M
	enum VBIWrappingType
	{
		VBIFrame = 1,					//!< Interlaced or PsF frame
		VBIField1 = 2,					//!< Field 1 of an interlaced picture
		VBIField2 = 3,					//!< Field 1 of an interlaced picture
		VBIProgressive = 4				//!< Progressive frame
	};

	//! Sample coding enumeration, as per SMPTE-436M
	enum VBISampleCoding
	{
		Y1Bit = 1,						//!< Luma only, 1-bit per sample
		C1Bit = 2,						//!< Chroma only, 1-bit per sample
		YC1Bit = 3,						//!< Luma and Chroma, 1-bit per sample
		Y8Bit = 4,						//!< Luma only, 8-bits per sample
		C8Bit = 5,						//!< Chroma only, 8-bits per sample
		YC8Bit = 6,						//!< Luma and Chroma, 8-bits per sample
		Y10Bit = 7,						//!< Luma only, 10-bits per sample
		C10Bit = 8,						//!< Chroma only, 10-bits per sample
		YC10Bit = 9						//!< Luma and Chroma, 10-bits per sample
	};

	//! Class that holds a single line of VBI data
	class VBILine : public RefCount<VBILine>
	{
	protected:
		DataChunk Data;					//!< The VBI actual data bytes for this line, packed as per SMPTE-436M
		int LineNumber;					//!< The line number of this line in the frame, plus 0x4000 if the line is in field 2 of an interlaced picture
		VBIWrappingType WrappingType;	//!< The VBI Wrapping type for this line
		VBISampleCoding SampleCoding;	//!< SampleCoding for this line
		UInt16 SampleCount;				//!< Number of samples in this line

	public:
		//! Construct a VBILine with no data
		VBILine(int LineNumber, VBIWrappingType Wrapping, VBISampleCoding Coding)
			: LineNumber(LineNumber), WrappingType(Wrapping), SampleCoding(Coding) {};

		//! Construct a VBILine with no data, for an interlaced frame
		VBILine(int Field, int LineNumber, VBIWrappingType Wrapping, VBISampleCoding Coding)
			: WrappingType(Wrapping), SampleCoding(Coding) 
		{
			if(Field == 2) this->LineNumber = 0x4000 + LineNumber;
			else this->LineNumber = LineNumber;
		};

		//! Construct a VBILine with data
		/*! DRAGONS: The LineData chunk will be cleared by this operation */
		VBILine(int LineNumber, VBIWrappingType Wrapping, VBISampleCoding Coding, UInt16 SampleCount, DataChunkPtr &LineData)
			: LineNumber(LineNumber), WrappingType(Wrapping), SampleCoding(Coding), SampleCount(SampleCount)
		{
			// Take the buffer from LineData (and clear LineData)
			Data.TakeBuffer(LineData, true);
		}

		//! Construct a VBILine with data, for an interlaced frame
		/*! DRAGONS: The LineData chunk will be cleared by this operation */
		VBILine(int Field, int LineNumber, VBIWrappingType Wrapping, VBISampleCoding Coding, UInt16 SampleCount, DataChunkPtr &LineData)
			: LineNumber(LineNumber), WrappingType(Wrapping), SampleCoding(Coding), SampleCount(SampleCount)
		{
			if(Field == 2) this->LineNumber = 0x4000 + LineNumber;
			else this->LineNumber = LineNumber;

			// Take the buffer from LineData (and clear LineData)
			Data.TakeBuffer(LineData, true);
		}

		//! Set (or replace) the current line data
		/*! DRAGONS: The LineData chunk will be cleared by this operation */
		void SetData(UInt16 SampleCount, DataChunkPtr &LineData)
		{
			// Take the buffer from LineData (and clear LineData)
			Data.TakeBuffer(LineData, true);

			this->SampleCount = SampleCount;
		}

		//! Get the size of the data buffer, excluding the line number, wrapping type, sample coding and sample count bytes
		size_t GetDataSize(void) { return static_cast<size_t>(Data.Size); }

		//! Get the size of the data buffer, including the line number, wrapping type, sample coding and sample count bytes
		size_t GetFullDataSize(void) { return static_cast<size_t>(Data.Size) + 6; }

		//! Write the line of data into a buffer, including the line number, wrapping type, sample coding and sample count bytes
		/*! \note It is the caller's responsibility to ensure that the buffer has enough space - the number of bytes written <b>will be</b> GetFullDataSize()
		 */
		void WriteData(UInt8 *Buffer);
	};


	// Smart pointer to an VBILine object
	typedef SmartPtr<VBILine> VBILinePtr;

	// Map of smart pointer to VBILine objects, indexed by line number
	typedef std::map<int, VBILinePtr> VBILineMap;


	//! Class that holds the VBI data for a frame and supplies it as an EssenceSource
	class VBISource : public EssenceSource
	{
	protected:
		EssenceSourceParent MasterSource;	//!< The EssenceSource for the picture essence to which this VBI data relates

		VBILineMap Lines;					//!< Map of lines for this frame

		DataChunkList BufferedData;			//!< List of data items prepared and ready to be supplied in response to GetEssenceData() - next to be supplied is the head

		size_t BufferOffset;				//!< An offset into the current data buffer if we are returning a partial chunk in GetEssenceData()

	public:
		// Base constructor
		VBISource(EssenceSource *Master) : MasterSource(Master), BufferOffset(0) {};

		//! Virtual destructor to allow polymorphism
		virtual ~VBISource() {};

		// Add a line of data
		void AddLine(int LineNumber, VBIWrappingType Wrapping, VBISampleCoding Coding, UInt16 SampleCount, DataChunkPtr &LineData);

		// Add a line of data, for an interlaced frame
		void AddLine(int Field, int LineNumber, VBIWrappingType Wrapping, VBISampleCoding Coding, UInt16 SampleCount, DataChunkPtr &LineData)
		{
			if(Field == 2) LineNumber += 0x4000;
			AddLine(LineNumber, Wrapping, Coding, SampleCount, LineData);
		}

		//! Get the size of the essence data in bytes
		/*! \note There is intentionally no support for an "unknown" response */
		virtual size_t GetEssenceDataSize(void);

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
		virtual DataChunkPtr GetEssenceData(size_t Size = 0, size_t MaxSize = 0);

		//! Did the last call to GetEssenceData() return the end of a wrapping item
		/*! \return true if the last call to GetEssenceData() returned an entire wrapping unit.
		 *  \return true if the last call to GetEssenceData() returned the last chunk of a wrapping unit.
		 *  \return true if the last call to GetEssenceData() returned the end of a clip-wrapped clip.
		 *  \return false if there is more data pending for the current wrapping unit.
		 *  \return false if the source is to be clip-wrapped and there is more data pending for the clip
		 */
		virtual bool EndOfItem(void) { return (BufferOffset == 0); }

		//! Is all data exhasted?
		/*! \return true if a call to GetEssenceData() will return some valid essence data
		 */
		virtual bool EndOfData(void) 
		{ 
			if(!MasterSource) return true;
			return MasterSource->EndOfData();
		}

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual UInt8 GetGCEssenceType(void) { return 0x17; }

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual UInt8 GetGCElementType(void) { return 0x01; }

		//! Get the edit rate of this wrapping of the essence
		/*! \note This may not be the same as the original "native" edit rate of the
		 *        essence if this EssenceSource is wrapping to a different edit rate 
		 */
		virtual Rational GetEditRate(void)
		{ 
			if(!MasterSource) return Rational(1,1);
			return MasterSource->GetEditRate();
		}

		//! Get the current position in GetEditRate() sized edit units
		/*! This is relative to the start of the stream, so the first edit unit is always 0.
		 *  This is the same as the number of edit units read so far, so when the essence is 
		 *  exhausted the value returned shall be the size of the essence
		 */
		virtual Position GetCurrentPosition(void)
		{
			if(!MasterSource) return 0;
			return MasterSource->GetCurrentPosition();
		}

		//! Get the preferred BER length size for essence KLVs written from this source, 0 for auto
		virtual int GetBERSize(void) { return 4; }

		//! Is this picture essence?
		virtual bool IsPictureEssence(void) { return false; }

		//! Is this sound essence?
		virtual bool IsSoundEssence(void) { return false; }

		//! Is this data essence?
		virtual bool IsDataEssence(void) { return true; }

		//! Is this compound essence?
		virtual bool IsCompoundEssence(void) { return false; }

		//! An indication of the relative write order to use for this stream
		/*! Normally streams in a GC are ordered as follows:
		 *  - All the CP system items (in Scheme ID then Element ID order)
		 *  - All the GC system items (in Scheme ID then Element ID order)
		 *  - All the CP picture items (in Element ID then Element Number order)
		 *  - All the GC picture items (in Element ID then Element Number order)
		 *  - All the CP sound items (in Element ID then Element Number order)
		 *  - All the GC sound items (in Element ID then Element Number order)
		 *  - All the CP data items (in Element ID then Element Number order)
		 *  - All the GC data items (in Element ID then Element Number order)
		 *  - All the GC compound items (in Element ID then Element Number order) (no GC compound)
		 *
		 *  However, sometimes this order needs to be overridden - such as for VBI data preceding picture items.
		 *
		 *  The normal case for ordering of an essence stream is for RelativeWriteOrder to return 0,
		 *  indicating that the default ordering is to be used. Any other value indicates that relative
		 *  ordering is required, and this is used as the Position value for a SetRelativeWriteOrder()
		 *  call. The value of Type for that call is acquired from RelativeWriteOrderType()
		 *
		 * For example: to force a source to be written between the last GC sound item and the first CP data
		 *              item, RelativeWriteOrder() can return any -ve number, with RelativeWriteOrderType()
		 *				returning 0x07 (meaning before CP data). Alternatively RelativeWriteOrder() could
		 *				return a +ve number and RelativeWriteOrderType() return 0x16 (meaning after GC sound)
		 */
		virtual Int32 RelativeWriteOrder(void) 
		{ 
			// We need to be BEFORE the CP picture data
			return -1; 
		}

		//! The type for relative write-order positioning if RelativeWriteOrder() != 0
		/*! This method indicates the essence type to order this data before or after if reletive write-ordering is used 
		 */
		virtual int RelativeWriteOrderType(void) 
		{ 
			// We need to be before the CP PICTURE DATA
			return 0x05; 
		}

	protected:
		//! Build the VBI data for this frame in SMPTE-436M format
		DataChunkPtr BuildChunk(void);
	};
}

#endif // MXFLIB__VBI_H
