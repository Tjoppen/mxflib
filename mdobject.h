/*! \file	mdobject.h
 *	\brief	Definition of classes that define metadata objects
 *
 *			Class MDObject holds info about a specific metadata object
 *<br><br>
 *			Class MDOType holds the definition of MDObjects derived from
 *			the XML dictionary.
 *<br><br>
 *	\note
 *			These classes are currently wrappers around KLVLib structures
 */
/*
 *	Copyright (c) 2002, Matt Beard
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
#ifndef MXFLIB__MDOBJECT_H
#define MXFLIB__MDOBJECT_H

// Include the KLVLib header
extern "C"
{
#include "KLV.h"						//!< The KLVLib header
}

// STL Includes
#include <string>
#include <list>
#include <map>

namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDOType;

	//! A smart pointer to an MDOType object
	typedef SmartPtr<MDOType> MDOTypePtr;

	//! A list of smart pointers to MDOType objects
	typedef std::list<MDOTypePtr> MDOTypeList;
}

namespace mxflib
{
	//! Holds the definition of a metadata object type
	class MDOType
	{
	private:
		//! The KLVLib dictionary entry
		DictEntry *Dict;

		MDContainerType ContainerType;

	public:
		MDOTypePtr Base;				//!< Base class if this is a derived class, else NULL
		MDOTypeList Children;			//!< Types normally found inside this type

		const DictEntry* GetDict(void) { return (const DictEntry*)Dict; };

		//! Access function for ContainerType
		const MDContainerType &GetContainerType(void) { return (const MDContainerType &)ContainerType; };

	private:
		MDOType(DictEntry *RootDict);

	//** Static Dictionary Handling data and functions **
	//***************************************************
	private:
		static MDOTypeList	AllTypes;	//!< All types managed by this object
		static MDOTypeList	TopTypes;	//!< The top-level types managed by this object

		//! Map for reverse lookups based on DictEntry pointer
		static std::map<DictEntry*, MDOTypePtr> DictLookup;

		//! Map for reverse lookups based on type name
		static std::map<std::string, MDOTypePtr> NameLookup;

		//! Add a KLVLib DictEntry definition to the managed types
		static void AddDict(DictEntry *Dict, MDOType *Parent = NULL);

		//! Internal class to ensure dictionary is freed at application end
		class DictManager
		{
		public:
			DictEntry *MainDict;		//!< The KLVLib dictionary entry of the root entry
		public:
			DictManager() { MainDict = NULL; };
			void Load(const char *DictFile);
			~DictManager();
		};

		// Make the internal class access to private members
		friend class DictManager;

		static DictManager DictMan;		//!< Dictionary manager

	public:
		// Load the dictionary
		static void LoadDict(const char *DictFile) { DictMan.Load(DictFile); };

		static MDOType *Find(const char *BaseType);
	};
}


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDObject;

	//! A smart pointer to an MDObject object
	typedef SmartPtr<MDObject> MDObjectPtr;

	//! A list of smart pointers to MDObject objects
	typedef std::list<MDObjectPtr> MDObjectList;
}


namespace mxflib
{
	//! Metadata Object class
	class MDObject
	{
	private:
		MDOType *Type;
		int Size;
		Uint8 *Data;

		MDObjectList Children;

	public:
		MDObject(const char *BaseType);
		MDObject(MDOType *BaseType);
		void Init(void);
		~MDObject();

		void AddChild(MDObject *Child);

		void SetInt8(Int8 Val) { ASSERT(Size==1); if(Size==1) PutI8(Val, Data); };
		void SetUint8(Uint8 Val) { ASSERT(Size==1); if(Size==1) PutU8(Val, Data); };
		void SetInt16(Int16 Val) { ASSERT(Size==2); if(Size==2) PutI16(Val, Data); };
		void SetUint16(Uint16 Val) { ASSERT(Size==2); if(Size==2) PutU16(Val, Data); };
		void SetInt32(Int32 Val) { ASSERT(Size==4); if(Size==4) PutI32(Val, Data); };
		void SetUint32(Uint32 Val) { ASSERT(Size==4); if(Size==4) PutU32(Val, Data); };
		void SetInt64(Int64 Val) { ASSERT(Size==8); if(Size==8) PutI64(Val, Data); };
		void SetUint64(Uint64 Val) { ASSERT(Size==8); if(Size==8) PutU64(Val, Data); };
		
		Int8 GetInt8(void) { ASSERT(Size==1); return (Size==1) ? GetI8(Data) : 0; };
		Uint8 GetUint8(void) { ASSERT(Size==1); return (Size==1) ? GetU8(Data) : 0; };
		Int16 GetInt16(void) { ASSERT(Size==2); return (Size==2) ? GetI16(Data) : 0; };
		Uint16 GetUint16(void) { ASSERT(Size==2); return (Size==2) ? GetU16(Data) : 0; };
		Int32 GetInt32(void) { ASSERT(Size==4); return (Size==4) ? GetI32(Data) : 0; };
		Uint32 GetUint32(void) { ASSERT(Size==4); return (Size==4) ? GetU32(Data) : 0; };
		Int64 GetInt64(void) { ASSERT(Size==8); return (Size==8) ? GetI64(Data) : 0; };
		Uint64 GetUint64(void) { ASSERT(Size==8); return (Size==8) ? GetU64(Data) : 0; };

		void SetData(int ValSize, Uint8 *Val);
	};
}


#endif MXFLIB__MDOBJECT_H

