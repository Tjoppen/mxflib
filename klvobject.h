/*! \file	klvobject.h
 *	\brief	Definition of classes that define basic KLV objects
 *
 *			Class KLVObject holds info about a KLV object
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
	//! KLV Object class
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
		void SetSource(MXFFilePtr File, Uint64 Location, Uint32 NewKLSize)
		{
			IsConstructed = false;
			SourceOffset = Location;
			KLSize = NewKLSize;
			SourceFile = File;
		}

		//! Set the source details when an object has been read from memory
		void SetSource(Uint64 Location, Uint32 NewKLSize)
		{
			IsConstructed = false;
			SourceOffset = Location;
			KLSize = NewKLSize;
			SourceFile = NULL;
		}

		//! Set the object's UL
		void SetUL(ULPtr NewUL) { TheUL = NewUL; }

		//! Get the location within the ultimate parent
		Uint64 GetLocation(void) { return SourceOffset; }

		//! Get text that describes where this item came from
		std::string GetSource(void);

	private:
		// Some private helper functions
		Uint32 ReadKey(KeyFormat Format, Uint32 Size, const Uint8 *Buffer, DataChunk& Key);
		Uint32 ReadLength(LenFormat Format, Uint32 Size, const Uint8 *Buffer, Uint32& Length);
	};
}

#endif // MXFLIB__KLVOBJECT_H
