/*! \file	klvobject.h
 *	\brief	Definition of classes that define basic KLV objects
 *
 *			Class KLVObject holds info about a KLV object
 *
 *	\version $Id: klvobject.h,v 1.1.2.4 2004/05/28 14:38:59 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2003, Matt Beard
 *	Portions Copyright (c) 2003, Metaglue Corporation
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
#ifndef MXFLIB__KLVOBJECT_H
#define MXFLIB__KLVOBJECT_H


// STL Includes
#include <string>
#include <list>
#include <map>


// Define some enums
namespace mxflib
{
	enum KeyFormat {
		KEY_NONE = 0,
		KEY_1_BYTE = 1,
		KEY_2_BYTE = 2,
		KEY_4_BYTE = 4,
		KEY_AUTO = 3
	};

	enum LenFormat
	{
		LEN_NONE = 0,
		LEN_1_BYTE = 1,
		LEN_2_BYTE = 2,
		LEN_4_BYTE = 4,
		LEN_BER = 3
	};
}


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class KLVObject;

	//! A smart pointer to a KLVObject object
	typedef SmartPtr<KLVObject> KLVObjectPtr;

	//! A list of smart pointers to KLVObject objects
	typedef std::list<KLVObjectPtr> KLVObjectList;

	typedef std::map<std::string, KLVObjectPtr> KLVObjectMap;
}


namespace mxflib
{
	//! Base class for GCReader handlers
	/*! \note Classes derived from this class <b>must not</b> include their own RefCount<> derivation
	 */
	class KLVReadHandler_Base : public RefCount<KLVReadHandler_Base>
	{
	public:
		//! Base destructor
		virtual ~KLVReadHandler_Base();

		//! Read data from the source into the KLVObject
		/*! \param Object KLVObject to receive the data
		 *  \param Start Offset from the start of the KLV value to start reading
		 *  \param Size Number of bytes to read, if zero all available bytes will be read (which gould be billions!)
		 *  \return The count of bytes read (may be less than Size if less available)
		 *  \note A call to ReadData must replace the current contents of the KLVObject's DataChunk
		 *        with the new data - no original data should be preserved
		 */
		virtual Length ReadData(KLVObjectPtr Object, Position Start = 0, Length Size = 0) = 0;
	};

	//! Smart pointer for the base KLVObject read handler
	typedef SmartPtr<KLVReadHandler_Base> KLVReadHandlerPtr;


	//! KLV Object class
	/*! This class gives access to single KLV items within an MXFfile.
	 *  The normal use for this class is handling of essence data. Huge values can be safely 
	 *  handled by loading them a "chunk" at a time.  Data is also available to identify the
	 *  location of the value in an MXFFile so that MXFFile::Read() and MXFFile::Write can
	 *  be used for efficient access.
	 *  \note This class does <b>not</b> provide any interlock mechanism to ensure safe
	 *        concurrent access. So if modified data is held in the object's DataChunk,
	 *        but not yet written to the file, calls to KLVObject::ReadData() or
	 *        MXFFile::Read() will return the <b>unmodifief</b> data.
	 */
	class KLVObject : public RefCount<KLVObject>
	{
	protected:
		bool IsConstructed;				//!< True if this object is constructed, false if read from a file or a parent object
		Position SourceOffset;			//!< The position of the first byte in the DataChunk as an offset into the file (-1 if not available)
		Uint32 KLSize;					//!< Size of this objects KL if read from file or memory buffer
		MXFFilePtr SourceFile;			//!< Pointer to source file if read from a file
		ULPtr TheUL;					//!< The UL for this object (if known)
		Length ValueLength;				//!< Length of the value field

		DataChunk Data;					//!< The raw data for this item (if available)

		KLVReadHandlerPtr ReadHandler;	//!< A read-handler to supply data in response to read requests. If NULL data will be read from SourceFile (if available)

		//## NOTE: Ensure any new properties are copied by the KLVObject --> KLVEObject copy constructor ##

		//@@@ Is this another MSVC bug?  KLVEObject can't access protected KLVObject properties from KLVEObject constructor!!
		friend class KLVEObject;

	public:
		KLVObject(ULPtr ObjectUL);
		virtual void Init(void);
		virtual ~KLVObject() {};		//!< Virtual to allow sub-classing and polymorphic pointers

		//! Set the source details when an object has been read from a file
		/*! \param File The source file of this KLVObject
		 *  \param Location The byte offset of the start of the <b>key</b> of the KLV from the start of the file
		 *  \param NewKLSize The total length of the key and the length field
		 *  \param ValueLen The length of the value field of the KLV
		 */
		virtual void SetSource(MXFFilePtr File, Position Location, Uint32 NewKLSize, Length ValueLen)
		{
			IsConstructed = false;
			SourceOffset = Location;
			KLSize = NewKLSize;
			SourceFile = File;
			ValueLength = ValueLen;
		}

		//! Set the source details when an object is build in memory
		/*! \param Location The byte offset of the start of the <b>key</b> of the KLV within the allocated memory block
		 *  \param NewKLSize The total length of the key and the length field
		 *  \param ValueLen The length of the value field of the KLV
		 */
		virtual void SetSource(Position Location, Uint32 NewKLSize, Length ValueLen)
		{
			IsConstructed = false;
			SourceOffset = Location;
			KLSize = NewKLSize;
			SourceFile = NULL;
			ValueLength = ValueLen;
		}

		//! Get the object's UL
		virtual ULPtr GetUL(void) { return TheUL; }

		//! Set the object's UL
		virtual void SetUL(ULPtr NewUL) { TheUL = NewUL; }

		//! Get the location within the ultimate parent
		virtual Uint64 GetLocation(void) { return SourceOffset; }

		//! Get text that describes where this item came from
		virtual std::string GetSource(void);

		//! Get the size of the key and length (not of the value)
		virtual Uint32 GetKLSize(void) { return KLSize; }

		//! Get a GCElementKind structure
		virtual GCElementKind GetGCElementKind(void) { return mxflib::GetGCElementKind(TheUL); }

		//! Get the track number of this KLVObject (if it is a GC KLV, else 0)
		virtual Uint32 GetGCTrackNumber(void) { return mxflib::GetGCTrackNumber(TheUL); };

		//! Get the position of the first byte in the DataChunk as an offset into the file
		/*! \return -1 if the data has not been read from a file (or the offset cannot be determined) 
		 */
		virtual Position GetDataBase(void) { return SourceFile ? SourceOffset : -1; };

		//! Set the position of the first byte in the DataChunk as an offset into the file
		/*! \note This function must be used with great care as data may will be written to this location
		 */
		virtual void SetDataBase(Position NewBase) { SourceOffset = NewBase; };

		//! Read data from the KLVObject source into the DataChunk
		/*! If Size is zero an attempt will be made to read all available data (which may be billions of bytes!)
		 *  \note Any previously read data in the current DataChunk will be discarded before reading the new data
		 *	\return Number of bytes read - zero if none could be read
		 */
		virtual Length ReadData(Position Start = 0, Length Size = 0);

		//! Write the key and length of the current DataChunk to the source file
		/*! The key and length will be written to the source file as set by SetSource.
		 *  If LenSize is zero the length will be formatted to match KLSize (if possible!)
		 *  \note If the length will not fit in KLSize then KLSize will be updated
		 */
		virtual Uint32 WriteKL(Uint32 LenSize = 0);

		//! Write data from the current DataChunk to the source file
		/*! \note The data in the chunk will be written to the specified position 
		 *  <B>regardless of the position from where it was origanally read</b>
		 */
		virtual Length WriteData(const Uint8 *Buffer, Position Start, Length Size);

		//! Write data from the current DataChunk to the source file
		/*! \note The data in the chunk will be written to the specified position 
		 *  <B>regardless of the position from where it was origanally read</b>
		 */
		Length WriteData(DataChunkPtr &Buffer, Position Start = 0, Length Size = 0) 
		{ 
			return WriteData(Buffer->Data, Start, ((Size == 0) || (Size > (Buffer->Size - Start))) ? Buffer->Size - Start : Size); 
		}

		//! Write data from the current DataChunk to the source file
		/*! \note The data in the chunk will be written to the specified position 
		 *  <B>regardless of the position from where it was origanally read</b>
		 */
		Length WriteData(Position Start = 0, Length Size = 0)
		{ 
			return WriteData(Data.Data, Start, ((Size == 0) || (Size > (Data.Size - Start))) ? Data.Size - Start : Size); 
		}

		//! Set a handler to supply data when a read is performed
		/*! \note If not set it will be read from the source file (if available) or cause an error message
		 */
		virtual void SetReadHandler(KLVReadHandlerPtr Handler) { ReadHandler = Handler; }

		//! Get the length of the value field
		virtual Length GetLength(void) { return ValueLength; }

		//! Get a reference to the data chunk
		virtual DataChunk& GetData(void) { return Data; }
	};
}

#endif // MXFLIB__KLVOBJECT_H
