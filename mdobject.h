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


// mxflib includes
#include "primer.h"


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDOType;

	//! A smart pointer to an MDOType object
	typedef SmartPtr<MDOType> MDOTypePtr;

	//! A list of smart pointers to MDOType objects
	typedef std::list<MDOTypePtr> MDOTypeList;

	typedef std::map<std::string, MDOTypePtr> MDOTypeMap;
}

namespace mxflib
{
	//! Holds the definition of a metadata object type
	class MDOType : public RefCount<MDOType>, public MDOTypeMap
	{
	private:
		//! The KLVLib dictionary entry
		DictEntry *Dict;

		MDContainerType ContainerType;

		// DRAGONS: Need to define non-KLVLib version
		DictRefType RefType;

		std::string RootName;			//!< Base name of this type

	public:
		MDTypePtr ValueType;			//!< Value type if this is an actual data item, else NULL
		MDOTypePtr Base;				//!< Base class if this is a derived class, else NULL
//		MDOTypeList Children;			//!< Types normally found inside this type
//		StringList ChildrenNames;		//!< Names for each entry in Children
//		MDOTypePtr ArrayType;			//!< Type of sub items if this is an array
		StringList ChildOrder;			//!< Child names in order for compound types
		MDOTypePtr Parent;				//!< Parent type if this is a child
		ULPtr TypeUL;					//!< The UL for this type, or NULL

		const DictEntry* GetDict(void) { return (const DictEntry*)Dict; };

		//! Access function for ContainerType
		const MDContainerType &GetContainerType(void) { return (const MDContainerType &)ContainerType; };

		//! Ref access function
		DictRefType GetRefType(void) const { return RefType; };

		//! Get the type name
		std::string Name(void)
		{
			if((Dict == NULL) || (Dict->Name == NULL)) return std::string("");
			return std::string(Dict->Name);
		}

		//! Get the full type name, including all parents
		std::string FullName(void)
		{
			if((Dict == NULL) || (Dict->Name == NULL)) return RootName;
			return RootName + Dict->Name;
		}

		//! Insert a new child type
		std::pair<iterator, bool> insert(MDOTypePtr NewType) 
		{ 
			std::string NewName = NewType->Name();
			std::pair<iterator, bool> Ret = MDOTypeMap::insert(MDOTypeMap::value_type(NewName, NewType));
			ChildOrder.push_back(NewName);
			return Ret;
		}

		//! Get the UL for this type
		// DRAGONS: When the KLVLib stub is no longer used this will return a ref to the contained UL
		ULPtr GetUL(void)
		{
			if(Dict == NULL) return new UL();
			if(Dict->GlobalKey == NULL) return new UL();
			if(Dict->GlobalKeyLen != 16) return new UL();
			return new UL(Dict->GlobalKey);
		}

	private:
		MDOType(DictEntry *RootDict);

	//** Static Dictionary Handling data and functions **
	//***************************************************
	private:
		static MDOTypeList	AllTypes;	//!< All types managed by this object
		static MDOTypeList	TopTypes;	//!< The top-level types managed by this object

		//! Map for UL lookups
		static std::map<UL, MDOTypePtr> ULLookup;
		
		//! Map for reverse lookups based on DictEntry pointer
		static std::map<DictEntry*, MDOTypePtr> DictLookup;

		//! Map for reverse lookups based on type name
		static std::map<std::string, MDOTypePtr> NameLookup;

// DRAGONS: This is public to allow us to build entries for dark items...
public:
		//! Add a KLVLib DictEntry definition to the managed types
		static void AddDict(DictEntry *Dict, MDOTypePtr ParentType = NULL);
private:

		//! Internal class to ensure dictionary is freed at application end
		class DictManager
		{
		public:
			DictEntry *MainDict;		//!< The KLVLib dictionary entry of the root entry
		public:
			DictManager() { MainDict = NULL; };
			void Load(const char *DictFile);
			PrimerPtr MakePrimer(void);
			~DictManager();
		};

		// Make the internal class able to access to private members
		friend class DictManager;

		static DictManager DictMan;		//!< Dictionary manager

	public:
		//! Load the dictionary
		static void LoadDict(const char *DictFile) { DictMan.Load(DictFile); };

		//! Build a primer
		static PrimerPtr MakePrimer(void) { return DictMan.MakePrimer(); };

		static MDOTypePtr Find(std::string BaseType);
		static MDOTypePtr Find(ULPtr BaseUL);
		static MDOTypePtr Find(Tag BaseTag, PrimerPtr BasePrimer);
	};
}


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDObject;
	class MDObjectPtr;

	//! A smart pointer to an MDObject object (with operator[] overloads)
	class MDObjectPtr : public SmartPtr<MDObject>
	{
	public:
		MDObjectPtr() : SmartPtr<MDObject>() {};
		MDObjectPtr(MDObject * ptr) : SmartPtr<MDObject>(ptr) {};
		
		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	//! A list of smart pointers to MDObject objects
	typedef std::list<MDObjectPtr> MDObjectList;
	typedef SmartPtr<MDObjectList> MDObjectListPtr;

	//! A list of smart pointers to MDObject objects with names
	typedef std::pair<std::string,MDObjectPtr> MDObjectNamedListItem;
	typedef std::list<MDObjectNamedListItem> MDObjectNamedList;
}


namespace mxflib
{
	//! Metadata Object class
	class MDObject : public RefCount<MDObject>, public MDObjectNamedList
	{
	protected:
		MDOTypePtr Type;

		MDObjectPtr Link;

		bool IsConstructed;				//!< True if this object is constructed, false if read from a file or a parent object
		Uint64 ParentOffset;			//!< Offset from start of parent object if read from file or object
		Uint32 KLSize;					//!< Size of this objects KL if read from file or parent object
		MDObjectPtr Parent;				//!< Pointer to parent if read from inside another object
		MXFFilePtr ParentFile;			//!< Pointer to parent file if read from a file
		ULPtr TheUL;					//!< The UL for this object (if known)
		Tag TheTag;						//!< The local tag used for this object (if known)

		std::string ObjectName;			//!< The name of this object (normally the name of the type)

	public:
//		StringList ChildrenNames;		//!< Names for each child

	public:
		MDValuePtr Value;

	public:
		MDObject(const char *BaseType);
		MDObject(MDOTypePtr BaseType);
		MDObject(ULPtr BaseUL);
		MDObject(Tag BaseTag, PrimerPtr BasePrimer);
		void Init(void);
		~MDObject();

		MDObjectPtr AddChild(std::string ChildName);
		MDObjectPtr AddChild(MDObjectPtr ChildObject, bool Replace = false);

		void RemoveChild(std::string ChildName);
		void RemoveChild(MDOTypePtr ChildType);
		void RemoveChild(MDObjectPtr ChildObject);

		//! Access function for child values of compound items
		MDObjectPtr operator[](std::string ChildName);
		MDObjectPtr Child(std::string ChildName) { return operator[](ChildName); };
		MDObjectListPtr ChildList(std::string ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
		MDObjectPtr Child(MDOTypePtr ChildType) { return operator[](ChildType); };
		MDObjectListPtr ChildList(MDOTypePtr ChildType);

		void SetInt(Int32 Val) { if (Value) Value->SetInt(Val); };
		void SetInt64(Int64 Val) { if (Value) Value->SetInt64(Val); };
		void SetUint(Uint32 Val) { if (Value) Value->SetUint(Val); };
		void SetUint64(Uint64 Val) { if (Value) Value->SetUint64(Val); };
		void SetString(std::string Val)	{ if (Value) Value->SetString(Val); };
		Int32 GetInt(Int32 Default = 0) { if (Value) return Value->GetInt(); else return Default; };
		Int64 GetInt64(Int64 Default = 0) { if (Value) return Value->GetInt64(); else return Default; };
		Uint32 GetUint(Uint32 Default = 0) { if (Value) return Value->GetUint(); else return Default; };
		Uint64 GetUint64(Uint64 Default = 0) { if(Value) return Value->GetUint64(); else return Default; };
		std::string GetString(std::string Default = "")	{ if(Value) return Value->GetString(); else return Default; };

		/* Child value access */
		void SetInt(const char *ChildName, Int32 Val) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetInt(Val); };
		void SetInt64(const char *ChildName, Int64 Val) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetInt64(Val); };
		void SetUint(const char *ChildName, Uint32 Val) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetUint(Val); };
		void SetUint64(const char *ChildName, Uint64 Val) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetUint64(Val); };
		void SetString(const char *ChildName, std::string Val) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetString(Val); };
		Int32 GetInt(const char *ChildName, Int32 Default = 0) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetInt(); else return Default; };
		Int64 GetInt64(const char *ChildName, Int64 Default = 0) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetInt64(); else return Default; };
		Uint32 GetUint(const char *ChildName, Uint32 Default = 0) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetUint(); else return Default; };
		Uint64 GetUint64(const char *ChildName, Uint64 Default = 0) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetUint64(); else return Default; };
		std::string GetString(const char *ChildName, std::string Default = "") { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetString(); else return Default; };
		void SetInt(MDOTypePtr ChildType, Int32 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetInt(Val); };
		void SetInt64(MDOTypePtr ChildType, Int64 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetInt64(Val); };
		void SetUint(MDOTypePtr ChildType, Uint32 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetUint(Val); };
		void SetUint64(MDOTypePtr ChildType, Uint64 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetUint64(Val); };
		void SetString(MDOTypePtr ChildType, std::string Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetString(Val); };
		Int32 GetInt(MDOTypePtr ChildType, Int32 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetInt(); else return Default; };
		Int64 GetInt64(MDOTypePtr ChildType, Int64 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetInt64(); else return Default; };
		Uint32 GetUint(MDOTypePtr ChildType, Uint32 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUint(); else return Default; };
		Uint64 GetUint64(MDOTypePtr ChildType, Uint64 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUint64(); else return Default; };
		std::string GetString(MDOTypePtr ChildType, std::string Default = "") { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetString(); else return Default; };

		/* Child raw data access */
		//! Get a reference to the data chunk (const to prevent setting!!)
		const DataChunk& GetData(void) { ASSERT(Value); return Value->GetData(); };

		//! Build a data chunk with all this items data (including child data)
		const DataChunk PutData(void) { if(Value) return Value->PutData(); else return DataChunk(); };

		//! Read the object's value from a memory buffer
		Uint32 ReadValue(const Uint8 *Buffer, Uint32 Size, PrimerPtr UsePrimer = NULL);

		//! Report the name of this item (the name of its type)
		std::string Name(void) { return ObjectName; };

		//! Report the full name of this item (the full name of its type)
		std::string FullName(void) { ASSERT(Type); return Type->FullName(); };

		void insert(MDObjectPtr NewObject)
		{
			push_back(MDObjectNamedList::value_type(NewObject->Name(), NewObject));
		}
		
		//! Type access function
		MDOTypePtr GetType(void) const { return Type; };

		//! Link access functions
		MDObjectPtr GetLink(void) const { return Link; };
		void SetLink(MDObjectPtr NewLink) { Link = NewLink; };

		//! Ref access function
		DictRefType GetRefType(void) const { ASSERT(Type); return Type->GetRefType(); };

		//! Set the parent details when an object has been read from a file
		void SetParent(MXFFilePtr File, Uint64 Location, Uint32 NewKLSize)
		{
			IsConstructed = false;
			ParentOffset = Location;
			KLSize = NewKLSize;
			Parent = NULL;
			ParentFile = File;
		}

		//! Set the parent details when an object has been read from memory
		void SetParent(MDObjectPtr Object, Uint64 Location, Uint32 NewKLSize)
		{
			IsConstructed = false;
			ParentOffset = Location;
			KLSize = NewKLSize;
			Parent = Object;
			ParentFile = NULL;
		}

		//! Set the object's UL
		void SetUL(ULPtr NewUL) { TheUL = NewUL; }

		//! Set the object's tag
		void SetTag(Tag NewTag) { TheTag = NewTag; }

		//! Get the location within the ultimate parent
		Uint64 GetLocation(void);

		//! Get text that describes where this item came from
		std::string GetSource(void);

		//! Get text that describes exactly where this item came from
		std::string GetSourceLocation(void) 
		{
			return std::string("0x") + Int64toHexString(GetLocation(),8) + std::string(" in ") + GetSource();
		}

	private:
		// Some private helper functions
		Uint32 ReadKey(DictKeyFormat Format, Uint32 Size, const Uint8 *Buffer, DataChunk& Key);
		Uint32 ReadLength(DictLenFormat Format, Uint32 Size, const Uint8 *Buffer, Uint32& Length);
	};
}

// These simple inlines need to be defined after MDObject
namespace mxflib
{
inline MDObjectPtr MDObjectPtr::operator[](const char *ChildName) { return GetPtr()->operator[](ChildName); };
inline MDObjectPtr MDObjectPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->operator[](ChildType); };
}

#endif MXFLIB__MDOBJECT_H

