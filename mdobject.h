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
	class MDOType : public RefCount<MDOType>
	{
	private:
		//! The KLVLib dictionary entry
		DictEntry *Dict;

		MDContainerType ContainerType;

	public:
		MDTypePtr ValueType;			//!< Value type if this is an actual data item, else NULL
		MDOTypePtr Base;				//!< Base class if this is a derived class, else NULL
		MDOTypeList Children;			//!< Types normally found inside this type
		StringList ChildrenNames;		//!< Names for each entry in Children
		MDOTypePtr Parent;				//!< Parent type if this is a child

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
		static void AddDict(DictEntry *Dict, MDOTypePtr ParentType = NULL);

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

		static MDOTypePtr Find(const char *BaseType);
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
		
		//! Child access operator that overcomes dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
	};

	//! A list of smart pointers to MDObject objects
	typedef std::list<MDObjectPtr> MDObjectList;
}


namespace mxflib
{
	//! Metadata Object class
	class MDObject : public RefCount<MDObject>
	{
	private:
		MDOTypePtr Type;

		MDObjectList Children;
		StringList ChildrenNames;		//!< Names for each entry in Children

	public:
		MDValuePtr Value;

	public:
		MDObject(const char *BaseType);
		MDObject(MDOTypePtr BaseType);
		void Init(void);
		~MDObject();

		MDObjectPtr AddChild(const char *ChildName);
// DRAGONS: RemoveChild() ?

		//! Access function for child values of compound items
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr Child(const char *ChildName) { return operator[](ChildName); };

		void SetInt(Int32 Val) { Value->SetInt(Val); };
		void SetInt64(Int64 Val) { Value->SetInt64(Val); };
		void SetUint(Uint32 Val) { Value->SetUint(Val); };
		void SetUint64(Uint64 Val) { Value->SetUint64(Val); };
		void SetString(std::string Val)	{ Value->SetString(Val); };
		Int32 GetInt(void) { return Value->GetInt(); };
		Int64 GetInt64(void) { return Value->GetInt64(); };
		Uint32 GetUint(void) { return Value->GetUint(); };
		Uint64 GetUint64(void) { return Value->GetUint64(); };
		std::string GetString(void)	{ return Value->GetString(); };
	};
}

// This simple inlines need to be defined after MDValue
namespace mxflib
{
inline MDObjectPtr MDObjectPtr::operator[](const char *ChildName) { return GetPtr()->operator[](ChildName); };
}


#endif MXFLIB__MDOBJECT_H

