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
 *	$Id: mdobject.h,v 1.15 2003/11/25 18:44:45 stuart_hc Exp $
 *
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
#include <Klv.h>						//!< The KLVLib header
#include <Dict.h>
}


// STL Includes
#include <string>
#include <list>
#include <map>


// mxflib includes
#include <mxflib/primer.h>


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
		StringList ChildOrder;			//!< Child names in order for packs
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
		protected:
			static PrimerPtr StaticPrimer;
		public:
			DictEntry *MainDict;		//!< The KLVLib dictionary entry of the root entry
		public:
			DictManager() { MainDict = NULL; };
			void Load(const char *DictFile);
			PrimerPtr MakePrimer(void);
			PrimerPtr GetStaticPrimer(void) { return StaticPrimer; };
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
		static PrimerPtr GetStaticPrimer(void) { return DictMan.GetStaticPrimer(); };

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

		bool Modified;					//!< True if this object has been modified since being "read"
										/*!< This is used to automatically update the GenerationUID when writing the object */

	public:
		MDValuePtr Value;

	public:
		MDObject(std::string BaseType);
		MDObject(MDOTypePtr BaseType);
		MDObject(ULPtr BaseUL);
		MDObject(Tag BaseTag, PrimerPtr BasePrimer);
		void Init(void);
		~MDObject();

		MDObjectPtr AddChild(std::string ChildName, bool Replace = true);
		MDObjectPtr AddChild(MDOTypePtr ChildType, bool Replace = true);
		MDObjectPtr AddChild(MDObjectPtr ChildObject, bool Replace = false);

	protected:
		MDObjectPtr AddChildInternal(MDObjectPtr ChildObject, bool Replace = false);
	public:

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

		void SetInt(Int32 Val) { SetModified(true); if (Value) Value->SetInt(Val); };
		void SetInt64(Int64 Val) { SetModified(true); if (Value) Value->SetInt64(Val); };
		void SetUint(Uint32 Val) { SetModified(true); if (Value) Value->SetUint(Val); };
		void SetUint64(Uint64 Val) { SetModified(true); if (Value) Value->SetUint64(Val); };
		void SetString(std::string Val)	{ SetModified(true); if (Value) Value->SetString(Val); };
		bool SetDValue(void);
		void SetValue(DataChunk &Source) { ReadValue(Source); }
		void SetValue(MDObjectPtr Source) { ReadValue(Source->Value->PutData()); }
		Int32 GetInt(Int32 Default = 0) { if (Value) return Value->GetInt(); else return Default; };
		Int64 GetInt64(Int64 Default = 0) { if (Value) return Value->GetInt64(); else return Default; };
		Uint32 GetUint(Uint32 Default = 0) { if (Value) return Value->GetUint(); else return Default; };
		Uint64 GetUint64(Uint64 Default = 0) { if(Value) return Value->GetUint64(); else return Default; };
		std::string GetString(std::string Default = "")	{ if(Value) return Value->GetString(); else return Default; };
		bool IsDValue(void);

		//! Set the default value for this object
		/*! \return true is a default value is set, else false */
		bool SetDefault(void)
		{
			if(!Value) return false;

			ASSERT(Type);

			const DictEntry *Dict = Type->GetDict();

			if(!Dict) return false;
			if(!Dict->HasDefault) return false;
			if(!Dict->Default) return false;
			
			Value->ReadValue(Dict->Default, Dict->DefaultLen);

			return true;
		}

		/* Child value access */
		// For set functions AddChild is used (without replace option)
		// to ensure that the child exists and to set the modified flag
		void SetInt(const char *ChildName, Int32 Val) 
		{ 
			MDObjectPtr Ptr = AddChild(ChildName);
			if (Ptr) Ptr->SetInt(Val); else if(Value) Value->SetInt(ChildName, Val);
		};
		void SetInt64(const char *ChildName, Int64 Val) 
		{ 
			MDObjectPtr Ptr = AddChild(ChildName);
			if (Ptr) Ptr->SetInt64(Val); else if(Value) Value->SetInt64(ChildName, Val);
		};
		void SetUint(const char *ChildName, Uint32 Val) 
		{ 
			MDObjectPtr Ptr = AddChild(ChildName);
			if (Ptr) Ptr->SetUint(Val); else if(Value) Value->SetUint(ChildName, Val);
		};
		void SetUint64(const char *ChildName, Uint64 Val) 
		{ 
			MDObjectPtr Ptr = AddChild(ChildName);
			if (Ptr) Ptr->SetUint64(Val); else if(Value) Value->SetUint64(ChildName, Val);
		};
		void SetString(const char *ChildName, std::string Val) 
		{ 
			MDObjectPtr Ptr = AddChild(ChildName);
			if (Ptr) Ptr->SetString(Val); else if(Value) Value->SetString(ChildName, Val);
		};

		bool SetDValue(const char *ChildName) { MDObjectPtr Ptr = AddChild(ChildName); if (Ptr) return Ptr->SetDValue(); else return false; };
		
		void SetValue(const char *ChildName, const DataChunk &Source) 
		{ 
			MDObjectPtr Ptr = AddChild(ChildName); 
			if (Ptr) Ptr->ReadValue(Source); else if(Value) Value->ReadValue(ChildName, Source);
		};
		void SetValue(const char *ChildName, MDObjectPtr Source) 
		{ 
			MDObjectPtr Ptr = AddChild(ChildName); 
			if (Ptr) Ptr->ReadValue(Source->Value->PutData()); else if(Value) Value->ReadValue(ChildName, Source->Value->PutData());
		};
		
		Int32 GetInt(const char *ChildName, Int32 Default = 0) 
		{ 
			MDObjectPtr Ptr = operator[](ChildName);
			if (Ptr) return Ptr->GetInt(); else if(Value) return Value->GetInt(ChildName, Default); else return Default; 
		};

		Int64 GetInt64(const char *ChildName, Int64 Default = 0) 
		{ 
			MDObjectPtr Ptr = operator[](ChildName); 
			if (Ptr) return Ptr->GetInt64(); else if(Value) return Value->GetInt64(ChildName, Default); else return Default; 
		};

		Uint32 GetUint(const char *ChildName, Uint32 Default = 0) 
		{ 
			MDObjectPtr Ptr = operator[](ChildName); 
			if (Ptr) return Ptr->GetUint(); else if(Value) return Value->GetUint(ChildName, Default); else return Default; 
		};

		Uint64 GetUint64(const char *ChildName, Uint64 Default = 0) 
		{ 
			MDObjectPtr Ptr = operator[](ChildName); 
			if (Ptr) return Ptr->GetUint64(); else if(Value) return Value->GetUint64(ChildName, Default); else return Default; 
		};

		std::string GetString(const char *ChildName, std::string Default = 0)
		{ 
			MDObjectPtr Ptr = operator[](ChildName); 
			if (Ptr) return Ptr->GetString(); else if(Value) return Value->GetString(ChildName, Default); else return Default; 
		};

		bool IsDValue(const char *ChildName) { MDObjectPtr Ptr = operator[](ChildName); if (Ptr) return Ptr->IsDValue(); else return false; };

		void SetInt(MDOTypePtr ChildType, Int32 Val) { SetModified(true); MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetInt(Val); };
		void SetInt64(MDOTypePtr ChildType, Int64 Val) { SetModified(true); MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetInt64(Val); };
		void SetUint(MDOTypePtr ChildType, Uint32 Val) { SetModified(true); MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetUint(Val); };
		void SetUint64(MDOTypePtr ChildType, Uint64 Val) { SetModified(true); MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetUint64(Val); };
		void SetString(MDOTypePtr ChildType, std::string Val) { SetModified(true); MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetString(Val); };
		bool SetDValue(MDOTypePtr ChildType) { MDObjectPtr Ptr = AddChild(ChildType); if (Ptr) return Ptr->SetDValue(); else return false; };
		void SetValue(MDOTypePtr ChildType, MDObjectPtr Source) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->ReadValue(Source->Value->PutData()); }
		void SetValue(MDOTypePtr ChildType, DataChunk &Source) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->ReadValue(Source); }
		Int32 GetInt(MDOTypePtr ChildType, Int32 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetInt(); else return Default; };
		Int64 GetInt64(MDOTypePtr ChildType, Int64 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetInt64(); else return Default; };
		Uint32 GetUint(MDOTypePtr ChildType, Uint32 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUint(); else return Default; };
		Uint64 GetUint64(MDOTypePtr ChildType, Uint64 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUint64(); else return Default; };
		std::string GetString(MDOTypePtr ChildType, std::string Default = "") { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetString(); else return Default; };
		bool IsDValue(MDOTypePtr ChildType) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->IsDValue(); else return false; };

		/* Child raw data access */
		//! Get a reference to the data chunk (const to prevent setting!!)
		const DataChunk& GetData(void) { ASSERT(Value); return Value->GetData(); };

		//! Build a data chunk with all this items data (including child data)
		const DataChunk PutData(void) { if(Value) return Value->PutData(); else return DataChunk(); };

		//! Read the object's value from a data chunk
		Uint32 ReadValue(const DataChunk &Chunk) { return ReadValue(Chunk.Data, Chunk.Size); };

		//! Read the object's value from a memory buffer
		Uint32 ReadValue(const Uint8 *Buffer, Uint32 Size, PrimerPtr UsePrimer = NULL);

		//! Write this object to a memory buffer
		Uint32 WriteObject(DataChunk &Buffer, MDObjectPtr ParentObject, PrimerPtr UsePrimer);

		//! Write this top level object to a memory buffer
		/*! The object must be at the outer or top KLV level. The object is appended to the buffer
		 *	\return The number of bytes written
		 */
		Uint32 WriteObject(DataChunk &Buffer, PrimerPtr UsePrimer = NULL)
		{
			return WriteObject(Buffer, NULL, UsePrimer);
		}

		//! Write this object, and any strongly linked sub-objects, to a memory buffer
		Uint32 WriteLinkedObjects(DataChunk &Buffer, PrimerPtr UsePrimer = NULL);

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

		//! Make a link from this reference source to the specified target set
		bool MakeLink(MDObjectPtr TargetSet);
		
		//! Record that a link exists (not the same as making a link - see MakeLink)
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

		//! Access function for ParentFile
		MXFFilePtr GetParentFile(void) { return ParentFile; };

		//! Set the object's UL
		void SetUL(ULPtr NewUL) { TheUL = NewUL; }

		//! Set the object's tag
		void SetTag(Tag NewTag) { TheTag = NewTag; }

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		bool ChangeType(std::string NewType)
		{
			MDOTypePtr Ptr = MDOType::Find(NewType);

			if(!Ptr) return false;

			Type = Ptr;
			ObjectName = Type->Name();
			TheUL = Type->GetUL();
			TheTag = 0;

			return true;
		}

		//! Make a copy of this object
		MDObjectPtr MakeCopy(void);

	protected:
		//! Sets the modification state of this object
		/*! \note This function should be used rather than setting "Modified" as a 
		 *        future revision may "bubble" this up from sub-items to sets and packs
		 */
		void SetModified(bool State) { Modified = State; }

	public:
		//! Set the GenerationUID of an object iff it has been modified
		/*! \return true if the GenerationUID has been set, otherwise false
		 *  \note If the object does not have a GenerationUID property false is returned!
		 */
		bool SetGenerationUID(UUIDPtr UID);

		//! Has this object (including any child objects) been modified?
		bool IsModified(void);

		//! Clear the modified flag on this object and any contained objects
		void ClearModified(void);

		//! Get the location within the ultimate parent
		Uint64 GetLocation(void);

		//! Get text that describes where this item came from
		std::string GetSource(void);

		//! Get text that describes exactly where this item came from
		std::string GetSourceLocation(void) 
		{
			return std::string("0x") + Int64toHexString(GetLocation(),8) + std::string(" in ") + GetSource();
		}

	protected:
		// Some private helper functions
		static Uint32 ReadKey(DictKeyFormat Format, Uint32 Size, const Uint8 *Buffer, DataChunk& Key);
		static Uint32 ReadLength(DictLenFormat Format, Uint32 Size, const Uint8 *Buffer, Uint32& Length);
		Uint32 WriteKey(DataChunk &Buffer, DictKeyFormat Format, PrimerPtr UsePrimer = NULL);
		static Uint32 WriteLength(DataChunk &Buffer, Uint64 Length, DictLenFormat Format, Uint32 Size = 0);
	};
}


namespace mxflib
{
	//! Interface for any class containing an MDObject that needs to behave like an MDObject
	/*! This class is required to prevent the need for polymorphism
	 *	which doesn't really work with smart pointers
	 */
	class ObjectInterface
	{
	public:
		MDObjectPtr Object;				//!< The MDObject for this item

	public:
		// ** MDObject Interface **
		std::string Name(void) { return Object->Name(); };
		std::string FullName(void) { return Object->FullName(); };

		//! Access function for child values of compound items
		MDObjectPtr operator[](std::string ChildName) { return Object->operator[](ChildName); };
		MDObjectPtr Child(std::string ChildName) { return Object->operator[](ChildName); };
		MDObjectListPtr ChildList(std::string ChildName) { return Object->ChildList(ChildName); };
		MDObjectPtr operator[](MDOTypePtr ChildType) { return Object->operator[](ChildType); };
		MDObjectPtr Child(MDOTypePtr ChildType) { return Object->operator[](ChildType); };
		MDObjectListPtr ChildList(MDOTypePtr ChildType) { return Object->ChildList(ChildType); };

		MDObjectPtr AddChild(std::string ChildName, bool Replace = true) { return Object->AddChild(ChildName, Replace); };
		MDObjectPtr AddChild(MDObjectPtr ChildObject, bool Replace = false) { return Object->AddChild(ChildObject, Replace); };

		void RemoveChild(std::string ChildName) { Object->RemoveChild(ChildName); };
		void RemoveChild(MDOTypePtr ChildType) { Object->RemoveChild(ChildType); };
		void RemoveChild(MDObjectPtr ChildObject) { Object->RemoveChild(ChildObject); };

		void SetInt(const char *ChildName, Int32 Val) { Object->SetInt(ChildName, Val); };
		void SetInt64(const char *ChildName, Int64 Val) { Object->SetInt64(ChildName, Val); };
		void SetUint(const char *ChildName, Uint32 Val) { Object->SetUint(ChildName, Val); };
		void SetUint64(const char *ChildName, Uint64 Val) { Object->SetUint64(ChildName, Val); };
		void SetString(const char *ChildName, std::string Val) { Object->SetString(ChildName, Val); };
		bool SetDValue(const char *ChildName) { return Object->SetDValue(ChildName); };
		void SetValue(const char *ChildName, const DataChunk &Source) { Object->SetValue(ChildName, Source); }
		void SetValue(const char *ChildName, MDObjectPtr Source) { Object->SetValue(ChildName, Source); }
		Int32 GetInt(const char *ChildName, Int32 Default = 0) { return Object->GetInt(ChildName, Default); };
		Int64 GetInt64(const char *ChildName, Int64 Default = 0) { return Object->GetInt64(ChildName, Default); };
		Uint32 GetUint(const char *ChildName, Uint32 Default = 0) { return Object->GetUint(ChildName, Default); };
		Uint64 GetUint64(const char *ChildName, Uint64 Default = 0) { return Object->GetUint64(ChildName, Default); };
		std::string GetString(const char *ChildName, std::string Default = "") { return Object->GetString(ChildName, Default); };
		bool IsDValue(const char *ChildName) { return Object->IsDValue(ChildName); };
		void SetInt(MDOTypePtr ChildType, Int32 Val) { Object->SetInt(ChildType, Val); };
		void SetInt64(MDOTypePtr ChildType, Int64 Val) { Object->SetInt64(ChildType, Val); };
		void SetUint(MDOTypePtr ChildType, Uint32 Val) { Object->SetUint(ChildType, Val); };
		void SetUint64(MDOTypePtr ChildType, Uint64 Val) { Object->SetUint64(ChildType, Val); };
		void SetString(MDOTypePtr ChildType, std::string Val) { Object->SetString(ChildType, Val); };
		bool SetDValue(MDOTypePtr ChildType) { return Object->SetDValue(ChildType); };
		void SetValue(MDOTypePtr ChildType, DataChunk &Source) { Object->SetValue(ChildType, Source); }
		void SetValue(MDOTypePtr ChildType, MDObjectPtr Source) { Object->SetValue(ChildType, Source); }
		Int32 GetInt(MDOTypePtr ChildType, Int32 Default = 0) { return Object->GetInt(ChildType, Default); };
		Int64 GetInt64(MDOTypePtr ChildType, Int64 Default = 0) { return Object->GetInt64(ChildType, Default); };
		Uint32 GetUint(MDOTypePtr ChildType, Uint32 Default = 0) { return Object->GetUint(ChildType, Default); };
		Uint64 GetUint64(MDOTypePtr ChildType, Uint64 Default = 0) { return Object->GetUint64(ChildType, Default); };
		std::string GetString(MDOTypePtr ChildType, std::string Default = "") { return Object->GetString(ChildType, Default); };
		bool IsDValue(MDOTypePtr ChildType) { return Object->IsDValue(ChildType); };

		//! Read the object's value from a data chunk
		Uint32 ReadValue(const DataChunk &Chunk) { return Object->ReadValue(Chunk.Data, Chunk.Size); };

		//! Read the object's value from a memory buffer
		Uint32 ReadValue(const Uint8 *Buffer, Uint32 Size, PrimerPtr UsePrimer = NULL) { return Object->ReadValue(Buffer, Size, UsePrimer); };

		//! Write this object to a memory buffer
		Uint32 WriteObject(DataChunk &Buffer, MDObjectPtr ParentObject, PrimerPtr UsePrimer) { return Object->WriteObject(Buffer, ParentObject, UsePrimer); };

		//! Write this top level object to a memory buffer
		/*! The object must be at the outer or top KLV level. The object is appended to the buffer
		 *	\return The number of bytes written
		 */
		Uint32 WriteObject(DataChunk &Buffer, PrimerPtr UsePrimer /*=NULL*/)
		{
			return Object->WriteObject(Buffer, NULL, UsePrimer);
		}

		MDOTypePtr GetType(void) const { return Object->GetType(); };
		MDObjectPtr GetLink(void) const { return Object->GetLink(); };
		void SetLink(MDObjectPtr NewLink) { Object->SetLink(NewLink); };
		DictRefType GetRefType(void) const { return Object->GetRefType(); };

		//! Set the parent details when an object has been read from a file
		void SetParent(MXFFilePtr File, Uint64 Location, Uint32 NewKLSize) { Object->SetParent(File, Location, NewKLSize); };

		//! Set the parent details when an object has been read from memory
		void SetParent(MDObjectPtr ParentObject, Uint64 Location, Uint32 NewKLSize) { Object->SetParent(ParentObject, Location, NewKLSize); };

		bool IsModified(void) { return Object->IsModified(); }

		//! Clear the modified flag on this object and any contained objects
		void ClearModified(void) { Object->ClearModified(); }

		Uint64 GetLocation(void) { return Object->GetLocation(); }
		std::string GetSource(void) { return Object->GetSource(); }
		std::string GetSourceLocation(void) { return Object->GetSourceLocation(); }

		bool ChangeType(std::string NewType) { return Object->ChangeType(NewType); };
	};
}


// These simple inlines need to be defined after MDObject
namespace mxflib
{
inline MDObjectPtr MDObjectPtr::operator[](const char *ChildName) { return GetPtr()->operator[](ChildName); }
inline MDObjectPtr MDObjectPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->operator[](ChildType); }
}

#endif // MXFLIB__MDOBJECT_H
