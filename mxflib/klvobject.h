/*! \file	klvobject.h
 *	\brief	Definition of classes that define basic KLV objects
 *
 *			Class KLVObject holds info about a KLV object
 *
 *	\version $Id: klvobject.h,v 1.1.2.1 2004/05/05 11:16:05 matt-beard Exp $
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
	//! Structure to hold information about each stream in a GC
	struct GCElementKind
	{
		bool	IsValid;					//!< true if this is a GC Element
		Uint8 Item;							//!< Item type - byte 13
		Uint8 Count;						//!< Element count - byte 14
		Uint8 ElementType;			//!< Element type - byte 15
		Uint8 Number;					  //!< Element number - byte 16
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
	//! KLV Object class
	/*! This class gives access to single KLV items within an MXFfile.
	 *  The normal use for this class is handling of essence data. Huge values can be safely 
	 *  handled by loading them a "chunk" at a time.  Data is also available to identify the
	 *  location of the value in an MXFFile so that MXFFile::Read() and MXFFile::Write can
	 *  be used for efficient access.
	 *  /note This class does <b>not</b> provide any interlock mechanism to ensure safe
	 *        concurrent access. So if modified data is held in the object's DataChunk,
	 *        but not yet written to the file, calls to KLVObject::ReadData() or
	 *        MXFFile::Read() will return the <b>unmodifief</b> data.
	 */
	class KLVObject : public RefCount<KLVObject>
	{
	private:
		bool IsConstructed;				//!< True if this object is constructed, false if read from a file or a parent object
		Uint64 SourceOffset;			//!< Offset from start of source object if read from file or memory
		Uint32 KLSize;					//!< Size of this objects KL if read from file or memory buffer
		MXFFilePtr SourceFile;			//!< Pointer to source file if read from a file
		ULPtr TheUL;					//!< The UL for this object (if known)

		std::string ObjectName;			//!< The name of this object (if known)

		DataChunkPtr Data;				//!< The raw data for this item (if available)

	public:
		KLVObject(ULPtr ObjectUL);
		void Init(void);
		~KLVObject() {};

		//! Set the source details when an object has been read from a file
		void SetSource(MXFFilePtr File, Uint64 Location, Uint32 NewKLSize, Length ValueLen)
		{
			IsConstructed = false;
			SourceOffset = Location;
			KLSize = NewKLSize;
			SourceFile = File;
		}

		//! Set the source details when an object has been read from memory
		void SetSource(Uint64 Location, Uint32 NewKLSize, Length ValueLen)
		{
			IsConstructed = false;
			SourceOffset = Location;
			KLSize = NewKLSize;
			SourceFile = NULL;
		}

		//! Get the object's UL
		ULPtr GetUL(void) { return TheUL; }

		//! Set the object's UL
		void SetUL(ULPtr NewUL) { TheUL = NewUL; }

		//! Get the location within the ultimate parent
		Uint64 GetLocation(void) { return SourceOffset; }

		//! Get text that describes where this item came from
		std::string GetSource(void);

		//! Get the size of the key and length (not of the value)
		Uint64 GetKLSize(void) { return KLSize; }

		//! Get a GCElementKind structure
		GCElementKind GetGCElementKind(void);

		//! Get the position of the first byte in the DataChunk as an offset into the file
		/*! /return -1 if the data has not been read from a file (or the offset cannot be determined) 
		 */
		Position GetDataBase(void);

		//! Set the position of the first byte in the DataChunk as an offset into the file
		/*! /note This function must be used with great care as data may will be written to this location
		 */
		void SetDataBase(Position NewBase);

		//! Read data from the KLVObject source into the DataChunk
		/*! If Size is zero an attempt will be made to read all available data
		 *  /note Any previously read data in the current DataChunk will be discarded before reading the new data
		 */
		Length ReadData(Position Start = 0, Length Size = 0);

		//! Write data from the current DataChunk to the source file
		/*! /note The data in the chunk will be written to the specified position 
		 *  <B>regardless of the position from where it was origanally read</b>
		 */
		Length WriteData(Position Start = 0, Length Size = 0);

		//! Function pointer type for KLVReader handlers
		typedef Length (*KLVObjectReadHandler) (KLVObjectPtr Object, Position Start, Length Size );

		//! Set a handler to supply data when a read is performed
		/*! /note If not set it will be read from the source file (if available) or cause an error message
		 */
		void SetReadHandler(KLVObjectReadHandler Handler);

		//! Get the length of the value field
		Length GetLength(void);

		//! Get a reference to the data chunk
		DataChunkPtr& GetData(void);

	private:
		// Some private helper functions
		//Uint32 ReadKey(KeyFormat Format, Uint32 Size, const Uint8 *Buffer, DataChunk& Key);
		//Uint32 ReadLength(LenFormat Format, Uint32 Size, const Uint8 *Buffer, Uint32& Length);
	};
}

#endif // MXFLIB__KLVOBJECT_H
