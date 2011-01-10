/*! \file	vbi.h
 *	\brief	Definition of classes that handle ANC or VBI data as per SMPTE 436M
 *
 *	\version $Id: vbi.h,v 1.5 2011/01/10 10:42:09 matt-beard Exp $
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
	//! ANC wrapping type enumeration, as per SMPTE-436M
	enum ANCWrappingType
	{
		VANCFrame = 1,					//!< VANC Data: Interlaced or PsF frame
		VANCField1 = 2,					//!< VANC Data: Field 1 of an interlaced picture
		VANCField2 = 3,					//!< VANC Data: Field 1 of an interlaced picture
		VANCProgressive = 4,			//!< VANC Data: Progressive frame
		HANCFrame = 0x11,				//!< HANC Data: Interlaced or PsF frame
		HANCField1 = 0x12,				//!< HANC Data: Field 1 of an interlaced picture
		HANCField2 = 0x13,				//!< HANC Data: Field 1 of an interlaced picture
		HANCProgressive = 0x14			//!< HANC Data: Progressive frame
	};

	//! VBI Wrapping type enumeration, as per SMPTE-436M
	typedef ANCWrappingType VBIWrappingType;

	/* Versions of the ANC enums for VBI use */
	const ANCWrappingType VBIFrame = VANCFrame;				//!< Interlaced or PsF frame
	const ANCWrappingType VBIField1 = VANCField1;			//!< Field 1 of an interlaced picture
	const ANCWrappingType VBIField2 = VANCField2;			//!< Field 1 of an interlaced picture
	const ANCWrappingType VBIProgressive = VANCProgressive;	//!< Progressive frame

	//! Sample coding enumeration, as per SMPTE-436M
	enum ANCSampleCoding
	{
		Y1Bit = 1,						//!< Luma only, 1-bit per sample
		C1Bit = 2,						//!< Chroma only, 1-bit per sample
		YC1Bit = 3,						//!< Luma and Chroma, 1-bit per sample
		Y8Bit = 4,						//!< Luma only, 8-bits per sample
		C8Bit = 5,						//!< Chroma only, 8-bits per sample
		YC8Bit = 6,						//!< Luma and Chroma, 8-bits per sample
		Y10Bit = 7,						//!< Luma only, 10-bits per sample
		C10Bit = 8,						//!< Chroma only, 10-bits per sample
		YC10Bit = 9,					//!< Luma and Chroma, 10-bits per sample
		Y8BitErr = 10,					//!< Luma only, 8-bits per sample, with a parity error!
		C8BitErr = 11,					//!< Chroma only, 8-bits per sample, with a parity error!
		YC8BitErr = 12,					//!< Luma and Chroma, 8-bits per sample, with a parity error!
	};

	//! VBI Wrapping type enumeration, as per SMPTE-436M
	typedef ANCSampleCoding VBISampleCoding;

	//! Class that holds a single line of ANC or VBI data
	class ANCVBILine : public RefCount<ANCVBILine>
	{
	protected:
		DataChunk Data;					//!< The actual data bytes for this line, packed as per SMPTE-436M
		int LineNumber;					//!< The line number of this line in the frame
		ANCWrappingType WrappingType;	//!< The wrapping type for this line
		ANCSampleCoding SampleCoding;	//!< SampleCoding for this line
		UInt16 SampleCount;				//!< Number of samples in this line

	public:
		//! Construct a VBILine with no data
		ANCVBILine(int LineNumber, ANCWrappingType Wrapping, ANCSampleCoding Coding)
			: LineNumber(LineNumber), WrappingType(Wrapping), SampleCoding(Coding) {};

		//! Construct a VBILine with no data, for an interlaced frame
		ANCVBILine(int Field, int LineNumber, ANCWrappingType Wrapping, ANCSampleCoding Coding)
			: WrappingType(Wrapping), SampleCoding(Coding) 
		{
			if(Field == 2) this->LineNumber = 0x4000 + LineNumber;
			else this->LineNumber = LineNumber;
		};

		//! Construct an ANCVBILine with data
		ANCVBILine(int LineNumber, ANCWrappingType Wrapping, ANCSampleCoding Coding, DataChunkPtr LineData, int DID = -1, int SDID = -1)
			: LineNumber(LineNumber), WrappingType(Wrapping), SampleCoding(Coding)
		{
			SetData(LineData, DID, SDID);
		}

		//! Construct an ANCVBILine with data, for an interlaced frame
		ANCVBILine(int Field, int LineNumber, ANCWrappingType Wrapping, ANCSampleCoding Coding, DataChunkPtr LineData, int DID = -1, int SDID = -1)
			: LineNumber(LineNumber), WrappingType(Wrapping), SampleCoding(Coding)
		{
			if(Field == 2) this->LineNumber = 0x4000 + LineNumber;
			else this->LineNumber = LineNumber;

			SetData(LineData, DID, SDID);
		}

		//! Set (or replace) the current line data
		void SetData(DataChunkPtr &LineData, int DID = -1, int  SDID = -1)
		{
			if(DID == -1)
			{
				// Round to next UInt32 boundary
				size_t Size = ((LineData->Size + 3) / 4) * 4;
				Data.Resize(Size);
				
				// Set the line data
				Data.Set(LineData);

				// Pad with zeros if required
				if(LineData->Size < Size) memset(&Data.Data[LineData->Size], 0, Size - LineData->Size);
			}
			else
			{
				// ANC packets need to start DID, SDID, DataCount
				size_t Size = 3 + LineData->Size;
				
				// Round to next UInt32 boundary
				Size = ((Size + 3) / 4) * 4;
				Data.Resize(Size);

				// Set the DID, SDID and DataCount
				Data.Data[0] = static_cast<UInt8>(DID);
				Data.Data[1] = static_cast<UInt8>(SDID);
				Data.Data[2] = static_cast<UInt8>(LineData->Size);

				// Increase the VBI line size by DID, SDID, DataCount
				SampleCount = 3 + static_cast<UInt16>(LineData->Size);

				// Set the rest of the buffer from LineData
				Data.Set(LineData, 3);

				// Pad with zeros if required
				if((LineData->Size + 3)< Size) memset(&Data.Data[LineData->Size + 3], 0, Size - (LineData->Size + 3));
			}
		}

		//! Get the size of the data buffer, excluding the line number, wrapping type, sample coding, sample count bytes and array header
		size_t GetDataSize(void) { return static_cast<size_t>(Data.Size); }

		//! Get the size of the data buffer, including the line number, wrapping type, sample coding, sample count bytes and array header
		size_t GetFullDataSize(void) { return static_cast<size_t>(Data.Size) + 14; }

		//! Write the line of data into a buffer, including the line number, wrapping type, sample coding and sample count bytes
		/*! \note It is the caller's responsibility to ensure that the buffer has enough space - the number of bytes written <b>will be</b> GetFullDataSize()
		 */
		void WriteData(UInt8 *Buffer);
	};

	//! Alias for ANC usage of ANCVBILine
	typedef ANCVBILine ANCLine;

	//! Alias for VBI usage of ANCVBILine
	typedef ANCVBILine VBILine;


	//! Smart pointer to an ANCVBILine object
	typedef SmartPtr<ANCVBILine> ANCVBILinePtr;

	//! Alias for ANC usage of ANCVBILinePtr
	typedef ANCVBILinePtr ANCLinePtr;

	//! Alias for VBI usage of ANCVBILinePtr
	typedef ANCVBILinePtr VBILinePtr;


	//! Map of smart pointer to ANCVBILine objects, indexed by line number
	typedef std::map<int, ANCVBILinePtr> ANCVBILineMap;

	//! Alias for ANC usage of ANCVBILineMap
	typedef ANCVBILineMap ANCLineMap;

	//! Alias for VBI usage of ANCVBILineMap
	typedef ANCVBILineMap VBILineMap;

	//! Superclass for objects that supply data to be wrapped by an ANCVBISource
	class ANCVBILineSource : public RefCount<ANCVBILineSource>
	{
	protected:
		EssenceSourceParent Parent;						//!< The ANCVBISource that owns this line source

	public:
		//! Get the field number for the supplied ANC/VBI line
		virtual int GetField(void) = 0;

		//! Get the line number within the field for the supplied ANC/VBI line
		virtual int GetLineNumber(void) = 0;

		//! Get the SMPTE 436M wrapping type for this line
		virtual ANCWrappingType GetWrappingType(void) = 0;

		//! Get the SMPTE 436M sample coding for this line
		virtual ANCSampleCoding GetSampleCoding(void) = 0;

		//! Get the next line of data to wrap
		virtual DataChunkPtr GetLineData(void) = 0;

		//! Determine if this line-source is able to be used when slaved from a master with the given wrapping configuration
		/*! \return Simple and short text description of the line being wrapped if OK (e.g. "Fixed AFD of 0x54") or empty string if not valid
		 */
		virtual std::string ValidateConfig(WrappingConfigPtr MasterCfg) = 0;

		//! Get the DID value for this ANC or -1 for VBI data
		virtual int GetDID(void) = 0;

		//! Get the SDID value for this ANC or -1 for VBI data
		virtual int GetSDID(void) = 0;
	
		//! Set the owning ANCVBISource
		void SetParent(EssenceSource *ParentSource) { Parent = ParentSource; }
	};

	//! Smart pointer to an ANCVBILineSource
	typedef SmartPtr<ANCVBILineSource> ANCVBILineSourcePtr;

	//! Parent pointer to an ANCVBILineSource
	typedef ParentPtr<ANCVBILineSource> ANCVBILineSourceParent;

	//! List of smart pointers to ANCVBILineSource objects
	typedef std::list<ANCVBILineSourcePtr> ANCVBILineSourceList;


	//! Class that holds the ANC or VBI data for a frame and supplies it as an EssenceSource
	class ANCVBISource : public EssenceSubSource
	{
	protected:
		ANCVBILineSourceList Sources;		//!< List of line sources used to build lines

		ANCVBILineMap Lines;				//!< Map of lines built for this frame

		DataChunkList BufferedData;			//!< List of data items prepared and ready to be supplied in response to GetEssenceData() - next to be supplied is the head

		size_t BufferOffset;				//!< An offset into the current data buffer if we are returning a partial chunk in GetEssenceData()

		Position CurrentPosition;			//!< Our current position

		int F2Offset;						//!< The offset to add to field 2 line numbers (0 if no field 2, -1 if unknown)

	public:
		//! Base constructor
		ANCVBISource(EssenceSource *Master = NULL) : EssenceSubSource(Master), BufferOffset(0), CurrentPosition(0), F2Offset(-1) {};

		//! Virtual destructor to allow polymorphism
		virtual ~ANCVBISource() {};

		//! Add a new line source
		void AddLineSource(ANCVBILineSourcePtr LineSource)
		{
			Sources.push_back(LineSource);
			LineSource->SetParent(this);
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

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual UInt8 GetGCEssenceType(void) { return 0x17; }

		//! Get the current position in GetEditRate() sized edit units
		/*! This is relative to the start of the stream, so the first edit unit is always 0.
		 *  This is the same as the number of edit units read so far, so when the essence is 
		 *  exhausted the value returned shall be the size of the essence
		 */
		virtual Position GetCurrentPosition(void)
		{
			return CurrentPosition;
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

		//! Determine if this sub-source can slave from a source with the given wrapping configuration, if so build the sub-config
		/*! \return A smart pointer to the new WrappingConfig for this source as a sub-stream of the specified master, or NULL if not a valid combination
		 */
		virtual EssenceParser::WrappingConfigPtr MakeWrappingConfig(WrappingConfigPtr MasterCfg);

		//! Get the offset to add to lines in field 2
		int Field2Offset(void);

	protected:
		//! Build the ANC or VBI data for this frame in SMPTE-436M format
		DataChunkPtr BuildChunk(void);

		//! Get the wrapping UL to use
		virtual ULPtr GetWrappingUL(void) = 0;
	};

	//! Version for ANC usage of ANCVBISource
	class ANCSource : public ANCVBISource
	{
	public:
		ANCSource(EssenceSource *Master = NULL) : ANCVBISource(Master) {};

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual UInt8 GetGCElementType(void) { return 0x02; }

	protected:
		//! Get the wrapping UL to use
		virtual ULPtr GetWrappingUL(void)
		{
			const UInt8 WrappingUL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x09, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0e, 0x00, 0x00 };
			return new UL(WrappingUL);
		}
	};

	//! Version for VBI usage of ANCVBISource
	class VBISource : public ANCVBISource
	{
	public:
		VBISource(EssenceSource *Master = NULL) : ANCVBISource(Master) {};

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual UInt8 GetGCElementType(void) { return 0x01; }

	protected:
		//! Get the wrapping UL to use
		virtual ULPtr GetWrappingUL(void)
		{
			const UInt8 WrappingUL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x09, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0d, 0x00, 0x00 };
			return new UL(WrappingUL);
		}
	};


	//! Smart pointer to ANCVBISource
	typedef SmartPtr<ANCVBISource> ANCVBISourcePtr;

	//! Alias for ANC usage of ANCVBISourcePtr
	typedef ANCVBISourcePtr ANCSourcePtr;

	//! Alias for VBI usage of ANCVBISourcePtr
	typedef ANCVBISourcePtr VBISourcePtr;


	//! Simple AFD line source
	class SimpleAFDSource : public ANCVBILineSource
	{
	protected:
		UInt8 CurrentAFD;								//!< Current value of the AFD, will insert this value each frame
		int FieldNum;									//!< Field number in which to insert this AFD
		int LineNum;									//!< Line number in field to insert this AFD

	public:
		//! Construct from binary value
		SimpleAFDSource(UInt8 AFDValue, int Field, int Line) : CurrentAFD(AFDValue), FieldNum(Field), LineNum(Line) {};

		//! Construct from text value
		SimpleAFDSource(std::string AFDText, int Field, int Line) : FieldNum(Field), LineNum(Line) 
		{
			CurrentAFD = TextToAFD(AFDText);
		};

		//! Allow polymorphic destructors
		virtual ~SimpleAFDSource() {};

		//! Get the field number for the supplied ANC/VBI line
		int GetField(void) { return FieldNum; }

		//! Get the line number within the field for the supplied ANC/VBI line
		int GetLineNumber(void) { return LineNum; }

		//! Get the SMPTE 436M wrapping type for this line
		ANCWrappingType GetWrappingType(void) { return FieldNum == 1 ? VANCField1 : VANCField2; }

		//! Get the SMPTE 436M sample coding for this line
		ANCSampleCoding GetSampleCoding(void) { return Y8Bit; }

		//! Get the next line of data to wrap
		DataChunkPtr GetLineData(void)
		{
			// Build a simple payload with just the AFD and no bar data
			DataChunkPtr Payload = new DataChunk(8);
			Payload->Data[0] = CurrentAFD;
			memset(&Payload->Data[1], 0, 7);
			return Payload;
		}

		//! Determine if this line-source is able to be used when slaved from a master with the given wrapping configuration
		/*! \return Simple and short text description of the line being wrapped if OK (e.g. "Fixed AFD of 0x54") or empty string if not valid
		 */
		virtual std::string ValidateConfig(WrappingConfigPtr MasterCfg);

		//! Get the DID value for this ANC or -1 for VBI data
		virtual int GetDID(void) { return 0x41; }

		//! Get the SDID value for this ANC or -1 for VBI data
		virtual int GetSDID(void) { return 0x05; }

	protected:
		//! Convert a binary AFD value string, with optional 'w' suffix for 16:9 image, to an AFD value as per SMPTE 2016-1-2007
		static UInt8 TextToAFD(std::string);
	};
}

#endif // MXFLIB__VBI_H
