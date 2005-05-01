/*! \file	mdobject.h
 *	\brief	Definition of classes that define metadata objects
 *
 *			Class MDObject holds info about a specific metadata object
 *<br><br>
 *			Class MDOType holds the definition of MDObjects derived from
 *			the XML dictionary.
 *<br><br>
 *
 *	\version $Id: mdobject.h,v 1.7 2005/05/01 15:06:14 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2004, Matt Beard
 *  Portions copyright (c) 2002, BBC R&D
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

	//! A parent pointer to an MDOType object
	typedef ParentPtr<MDOType> MDOTypeParent;

	//! A list of smart pointers to MDOType objects
	typedef std::list<MDOTypePtr> MDOTypeList;

	//! A map of object type names to MDOType objects
	typedef std::map<std::string, MDOTypePtr> MDOTypeMap;

	// Forward declare the ObjectInterface
	class ObjectInterface;
}



namespace mxflib
{
#define MXFLIB_MAXDICTDEPTH 32

	//! Define version with old name for backwards compatibility
	typedef ClassUsage DictUse;

	//! Make old enum values work for backwards compatibility
	const ClassUsage DICT_USE_NONE = ClassUsageNULL;
	
	//! Make old enum values work for backwards compatibility
	const ClassUsage DICT_USE_REQUIRED = ClassUsageRequired;
	
	//! Make old enum values work for backwards compatibility
	const ClassUsage DICT_USE_ENCODER_REQUIRED = ClassUsageEncoderRequired;
	
	//! Make old enum values work for backwards compatibility
	const ClassUsage DICT_USE_DECODER_REQUIRED = ClassUsageDecoderRequired;
	
	//! Make old enum values work for backwards compatibility
	const ClassUsage DICT_USE_OPTIONAL = ClassUsageOptional;
	
	//! Make old enum values work for backwards compatibility
	const ClassUsage DICT_USE_DARK = ClassUsageDark;
	
	//! Make old enum values work for backwards compatibility
	const ClassUsage DICT_USE_TOXIC = ClassUsageToxic;

	//! Make old enum values work for backwards compatibility
	const ClassUsage DICT_USE_BEST_EFFORT = ClassUsageBestEffort;

	/*
	** Enumeration type for key formats
	*/
	typedef enum
	{
		DICT_KEY_NONE = 0,
		DICT_KEY_1_BYTE = 1,
		DICT_KEY_2_BYTE = 2,
		DICT_KEY_4_BYTE = 4,
		DICT_KEY_AUTO = 3
	} DictKeyFormat;


	/*
	** Enumeration type for length formats
	*/
	typedef enum
	{
		DICT_LEN_NONE = 0,
		DICT_LEN_1_BYTE = 1,
		DICT_LEN_2_BYTE = 2,
		DICT_LEN_4_BYTE = 4,
		DICT_LEN_BER = 3
	} DictLenFormat;


	//! Define version with old name for backwards compatibility
	typedef ClassRef DictRefType;

	//! Make old enum values work for backwards compatibility
	const ClassRef DICT_REF_NONE = ClassRefNone;

	//! Make old enum values work for backwards compatibility
	const ClassRef DICT_REF_STRONG = ClassRefStrong;

	//! Make old enum values work for backwards compatibility
	const ClassRef DICT_REF_WEAK = ClassRefWeak;

	//! Make old enum values work for backwards compatibility
	const ClassRef DICT_REF_TARGET = ClassRefTarget;


/* Notes about the structure of dictionaries...
   ============================================

   The dictionary is held as a list of MDOType objects, each of 
   which holds information about a 'type' held in the dictionary.

   The dictionary is generally tree structured, with some types
   being children of others (this matches the sets with child items
   of an MXF file) When an item is a child it contains a pointer to
   its parent.  Each parent item contain pointers to each child item
   through derivation from MDOTypeMap (maps child name to pointer to
   child item). Care should be taken iterating this map as the
   order is likely to be alphabetical rather than dictionary order
   so where dictionary order is importand (such as packs) iterate
   through the ChildOrder list property.

   Inheritance is supported where a type is regarded as a modified
   version of another (base) type. The mechanism for inheritance is
   that a derived MDOType will be a copy of the base MDOType
   with a link back to the base in 'Base'. The 'Children' lists is
   copied and new child types are added to these lists.
   If a child of a derived type has the same name as a child of
   the base it is regarded as a replacement. 
*/
}


namespace mxflib
{
	//! Holds the definition of a metadata object type
	class MDOType : public RefCount<MDOType>, public MDOTypeMap
	{
	protected:
		MDContainerType ContainerType;

		std::string RootName;			//!< Base name of this type

	protected:
		MDTypePtr ValueType;			//!< Value type if this is an actual data item, else NULL

	public:
		MDOTypeParent Base;				//!< Base class if this is a derived class, else NULL

	protected:
		StringList ChildOrder;			//!< Child names in order for packs
		MDOTypeParent Parent;			//!< Parent type if this is a child
		ULPtr TypeUL;					//!< The UL for this type, or NULL

		/* Dictionary data */
		DataChunk		Key;			//!< Main key field
		DataChunk		GlobalKey;		//!< Global key field (may be a copy of Key)
		std::string		DictName;		//!< Short (XML tag) name
		std::string		Detail;			//!< Full descriptive name
		std::string		TypeName;		//!< Data type name from dictionary (or built from UL found in file?)
		DictKeyFormat	KeyFormat;		//!< Format of key of sub-items
		DictLenFormat	LenFormat;		//!< Format of length of sub-items
		unsigned int	minLength;		//!< Minimum length of value field
		unsigned int	maxLength;		//!< Maximum length of value field
		DictUse			Use;			//!< Usage requirements
		DataChunk		Default;		//!< Default value (if one exists)
		DataChunk		DValue;			//!< Distinguished value (if one is defined)
		DictRefType		RefType;		//!< Reference type if this is a reference
		MDOTypeParent	RefTarget;		//!< Type (or base type) of item this ref source must target
		std::string		RefTargetName;	//!< Name of the type (or base type) of item this ref source must target

		//! Protected constructor so we can control creation of types
		MDOType();

	public:
		//! Public constructor - build a full type
		MDOType(MDContainerType ContainerType, std::string RootName, std::string Name, std::string Detail, MDTypePtr Type, 
				DictKeyFormat KeyFormat, DictLenFormat LenFormat, unsigned int minLength, unsigned int maxLength, DictUse Use)
			: ContainerType(ContainerType), RootName(RootName), DictName(Name), Detail(Detail), ValueType(Type),
			  KeyFormat(KeyFormat), LenFormat(LenFormat), minLength(minLength), maxLength(maxLength), Use(Use)
		{
			// MaxLength = 0 is used for maxlength = unbounded
			if(maxLength == 0) maxLength = (unsigned int)-1;

			if(Type) TypeName = Type->Name();
			else TypeName = Name;

			// Set the name lookup - UL lookup set when key set
			NameLookup[RootName + Name] = this;
		};

		//! Set the referencing details for this type
		void SetRef(DictRefType Type, MDOTypePtr Target, std::string TargetName)
		{
			RefType = Type;
			RefTarget = Target;
			RefTargetName = TargetName;
		}

		//! Derive this new entry from a base entry
		void Derive(MDOTypePtr BaseEntry);

		//! Access function for ContainerType
		const MDContainerType &GetContainerType(void) { return (const MDContainerType &)ContainerType; };

		//! Ref access function
		DictRefType GetRefType(void) const { return RefType; };

		//! Accessor for Reference Target
		MDOTypePtr GetRefTarget(void) const { return RefTarget; };

		//! Accessor for Reference Target Name
		std::string GetRefTargetName(void) const { return RefTargetName; };

		//! Get the type name
		std::string Name(void)
		{
			return DictName;
		}

		//! Get the full type name, including all parents
		std::string FullName(void)
		{
			return RootName + DictName;
		}

		//! Read-only access to default value
		const DataChunk &GetDefault(void) { return Default; }

		//! Read-only access to destinguished value
		const DataChunk &GetDValue(void) { return DValue; }

		//! Read-only access to the type UL
		const ULPtr &GetTypeUL(void) { return TypeUL; }

		//! Read-only access to the minLength value
		unsigned int GetMinLength(void) { return minLength; }

		//! Read-only access to the maxnLength value
		unsigned int GetMaxLength(void) { return maxLength; }

		//! Read-only access to ValueType
		const MDTypePtr &GetValueType(void) { return ValueType; }

		//! Read-only access to ChildOrder
		const StringList &GetChildOrder(void) { return ChildOrder; }

		//! Read-only access to KeyFormat
		DictKeyFormat &GetKeyFormat(void) { return KeyFormat; }

		//! Read-only access to LenFormat
		DictLenFormat &GetLenFormat(void) { return LenFormat; }

		//! Insert a new child type
		std::pair<iterator, bool> insert(MDOTypePtr NewType) 
		{ 
			std::string NewName = NewType->Name();
			std::pair<iterator, bool> Ret = MDOTypeMap::insert(MDOTypeMap::value_type(NewName, NewType));
			ChildOrder.push_back(NewName);
			return Ret;
		}

		//! Get the UL for this type
		ULPtr GetUL(void) { return TypeUL; }

		//! Read-only access to the key
		const DataChunk &GetKey(void) { return Key; }

		//! Read-only access to the global key
		const DataChunk &GetGlobalKey(void) { return GlobalKey; }

	//** Static Dictionary Handling data and functions **
	//***************************************************
	protected:
		static MDOTypeList	AllTypes;	//!< All types managed by this object
		static MDOTypeList	TopTypes;	//!< The top-level types managed by this object

		//! Map for UL lookups
		static std::map<UL, MDOTypePtr> ULLookup;
		
		//! Map for reverse lookups based on type name
		static MDOTypeMap NameLookup;

protected:
		//! Basic primer for use when parsing non-primer partitions
		static PrimerPtr StaticPrimer;

	public:
		//! Load the dictionary
		static void LoadDict(const char *DictFile);

		//! Build a primer
		static PrimerPtr MakePrimer(bool SetStatic = false);
		
		//! Get the static primer (make one if required)
		static PrimerPtr GetStaticPrimer(void) 
		{ 
			if( !StaticPrimer) MakePrimer(true); 
			return StaticPrimer;
		}

		static MDOTypePtr Find(std::string BaseType);
		static MDOTypePtr Find(const UL& BaseUL);
		static MDOTypePtr Find(Tag BaseTag, PrimerPtr BasePrimer);

		static MDOTypePtr MDOType::DefineClass(ClassRecordPtr &ThisClass, MDOTypePtr Parent = NULL);

	protected:
		static void XML_startElement(void *user_data, const char *name, const char **attrs);
		static void XML_endElement(void *user_data, const char *name);
		static void XML_warning(void *user_data, const char *msg, ...);
		static void XML_error(void *user_data, const char *msg, ...);
		static void XML_fatalError(void *user_data, const char *msg, ...);
	};
}


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDObject;
	class MDObjectPtr;
	class MDObjectParent;

	//! A smart pointer to an MDObject object (with operator[] overloads)
	class MDObjectPtr : public SmartPtr<MDObject>
	{
	public:
		MDObjectPtr() : SmartPtr<MDObject>() {};
//		MDObjectPtr(MDObject * ptr) : SmartPtr<MDObject>(ptr) {};
		MDObjectPtr(IRefCount<MDObject> * ptr) : SmartPtr<MDObject>(ptr) {};

		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	//! A parent pointer to an MDObject object (with operator[] overloads)
	class MDObjectParent : public ParentPtr<MDObject>
	{
	public:
		MDObjectParent() : ParentPtr<MDObject>() {};
		MDObjectParent(IRefCount<MDObject> * ptr) : ParentPtr<MDObject>(ptr) {};

		//! Set value from a smart pointer
		/*! \note Not a perfect operator= as no return value is created (too inefficient) */
		void operator=(const MDObjectPtr &sp) { SmartPtr<MDObject>::operator=(sp); }

		//! Set value from a pointer
		/*! \note Not a perfect operator= as no return value is created (too inefficient) */
		void operator=(RefCount<MDObject> *Ptr) { SmartPtr<MDObject>::operator=(Ptr); }

		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	//! A list of smart pointers to MDObject objects
	class MDObjectList : public RefCount<MDObjectList>, public std::list<MDObjectPtr> {};
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
//		MDObjectPtr Parent;				//!< Pointer to parent if read from inside another object
		MDObjectParent Parent;			//!< Pointer to parent if read from inside another object
//		MXFFilePtr ParentFile;			//!< Pointer to parent file if read from a file
		MXFFileParent ParentFile;		//!< Pointer to parent file if read from a file
		ULPtr TheUL;					//!< The UL for this object (if known)
		Tag TheTag;						//!< The local tag used for this object (if known)

		std::string ObjectName;			//!< The name of this object (normally the name of the type)

		bool Modified;					//!< True if this object has been modified since being "read"
										/*!< This is used to automatically update the GenerationUID when writing the object */

		ObjectInterface *Outer;			//!< Pointer to outer object if this is a sub-object of an ObjectInterface derived object

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
		void SetValue(const DataChunk &Source) { ReadValue(Source); }
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

			if(Type->GetDefault().Size == 0) return false;

			Value->ReadValue(Type->GetDefault());

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

		std::string GetString(const char *ChildName, std::string Default = "")
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
		void SetValue(MDOTypePtr ChildType, const DataChunk &Source) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->ReadValue(Source); }
		Int32 GetInt(MDOTypePtr ChildType, Int32 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetInt(); else return Default; };
		Int64 GetInt64(MDOTypePtr ChildType, Int64 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetInt64(); else return Default; };
		Uint32 GetUint(MDOTypePtr ChildType, Uint32 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUint(); else return Default; };
		Uint64 GetUint64(MDOTypePtr ChildType, Uint64 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUint64(); else return Default; };
		std::string GetString(MDOTypePtr ChildType, std::string Default = "") { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetString(); else return Default; };
		bool IsDValue(MDOTypePtr ChildType) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->IsDValue(); else return false; };

		/* Child raw data access */
		//! Get a reference to the data chunk (const to prevent setting!!)
		const DataChunk& GetData(void) { ASSERT(Value); return Value->GetData(); };

		//! Build a data chunk with all this item's data (including child data)
		const DataChunkPtr PutData(PrimerPtr UsePrimer = NULL);

		//! Read the object's value from a data chunk
		Uint32 ReadValue(const DataChunk &Chunk) { return ReadValue(Chunk.Data, Chunk.Size); };

		//! Read the object's value from a data chunk pointer
		Uint32 ReadValue(DataChunkPtr &Chunk) { return ReadValue(Chunk->Data, Chunk->Size); };

		//! Read the object's value from a memory buffer
		Uint32 ReadValue(const Uint8 *Buffer, Uint32 Size, PrimerPtr UsePrimer = NULL);

		//! Write this object to a new memory buffer
		DataChunkPtr WriteObject(MDObjectPtr ParentObject, PrimerPtr UsePrimer)
		{
			DataChunkPtr Ret = new DataChunk;
			WriteObject(Ret, ParentObject, UsePrimer);
			return Ret;
		}

		//! Write this object to a memory buffer
		Uint32 WriteObject(DataChunkPtr &Buffer, MDObjectPtr ParentObject, PrimerPtr UsePrimer);

		//! Write this top level object to a new memory buffer
		/*! The object must be at the outer or top KLV level
		 *	\return The new buffer
		 */
		DataChunkPtr WriteObject(PrimerPtr UsePrimer = NULL)
		{
			DataChunkPtr Ret = new DataChunk;
			WriteObject(Ret, NULL, UsePrimer);
			return Ret;
		}

		//! Write this top level object to a memory buffer
		/*! The object must be at the outer or top KLV level. The object is appended to the buffer
		 *	\return The number of bytes written
		 */
		Uint32 WriteObject(DataChunkPtr &Buffer, PrimerPtr UsePrimer = NULL)
		{
			return WriteObject(Buffer, NULL, UsePrimer);
		}

		//! Write this object, and any strongly linked sub-objects, to a memory buffer
		Uint32 WriteLinkedObjects(DataChunkPtr &Buffer, PrimerPtr UsePrimer = NULL);

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

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(std::string BaseType);

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(MDOTypePtr BaseType);
		
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

		//! Get pointer to Outer object
		ObjectInterface *GetOuter(void) { return Outer; };

		//! Set pointer to Outer object
		void SetOuter(ObjectInterface *NewOuter) { Outer = NewOuter; };

	protected:
		// Some private helper functions
		static Uint32 ReadKey(DictKeyFormat Format, Uint32 Size, const Uint8 *Buffer, DataChunk& Key);
		static Uint32 ReadLength(DictLenFormat Format, Uint32 Size, const Uint8 *Buffer, Uint32& Length);
		Uint32 WriteKey(DataChunkPtr &Buffer, DictKeyFormat Format, PrimerPtr UsePrimer = NULL);
		static Uint32 WriteLength(DataChunkPtr &Buffer, Uint64 Length, DictLenFormat Format, Uint32 Size = 0);
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
	protected:
		//! Protected constructor used to create from an existing MDObject
		ObjectInterface(MDObjectPtr BaseObject) : Object(BaseObject) {}

	public:
		MDObjectPtr Object;					//!< The MDObject for this item

	public:
		ObjectInterface() {};				//!< Build a basic ObjectInterface
		virtual ~ObjectInterface() {};		//!< Virtual destructor to allow polymorphism
		
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
		void SetValue(MDOTypePtr ChildType, const DataChunk &Source) { Object->SetValue(ChildType, Source); }
		void SetValue(MDOTypePtr ChildType, MDObjectPtr Source) { Object->SetValue(ChildType, Source); }
		Int32 GetInt(MDOTypePtr ChildType, Int32 Default = 0) { return Object->GetInt(ChildType, Default); };
		Int64 GetInt64(MDOTypePtr ChildType, Int64 Default = 0) { return Object->GetInt64(ChildType, Default); };
		Uint32 GetUint(MDOTypePtr ChildType, Uint32 Default = 0) { return Object->GetUint(ChildType, Default); };
		Uint64 GetUint64(MDOTypePtr ChildType, Uint64 Default = 0) { return Object->GetUint64(ChildType, Default); };
		std::string GetString(MDOTypePtr ChildType, std::string Default = "") { return Object->GetString(ChildType, Default); };
		bool IsDValue(MDOTypePtr ChildType) { return Object->IsDValue(ChildType); };

		//! Get a reference to the data chunk (const to prevent setting!!)
		const DataChunk& GetData(void) { ASSERT(Object); return Object->GetData(); };

		//! Build a data chunk with all this item's data (including child data)
		DataChunkPtr PutData(PrimerPtr UsePrimer = NULL) { if(Object) return Object->PutData(UsePrimer); else return new DataChunk(); };

		//! Read the object's value from a data chunk
		Uint32 ReadValue(const DataChunk &Chunk) { return Object->ReadValue(Chunk.Data, Chunk.Size); };

		//! Read the object's value from a data chunk pointer
		Uint32 ReadValue(const DataChunkPtr &Chunk) { return Object->ReadValue(Chunk->Data, Chunk->Size); };

		//! Read the object's value from a memory buffer
		Uint32 ReadValue(const Uint8 *Buffer, Uint32 Size, PrimerPtr UsePrimer = NULL) { return Object->ReadValue(Buffer, Size, UsePrimer); };

		//! Write this object to a new memory buffer
		DataChunkPtr WriteObject(MDObjectPtr ParentObject, PrimerPtr UsePrimer) { return Object->WriteObject(ParentObject, UsePrimer); };

		//! Write this object to a memory buffer
		Uint32 WriteObject(DataChunkPtr &Buffer, MDObjectPtr ParentObject, PrimerPtr UsePrimer) { return Object->WriteObject(Buffer, ParentObject, UsePrimer); };

		//! Write this top level object to a new memory buffer
		/*! The object must be at the outer or top KLV level
		 *	\return The number of bytes written
		 */
		DataChunkPtr WriteObject(PrimerPtr UsePrimer = NULL)
		{
			return Object->WriteObject(NULL, UsePrimer);
		}

		//! Write this top level object to a memory buffer
		/*! The object must be at the outer or top KLV level. The object is appended to the buffer
		 *	\return The number of bytes written
		 */
		Uint32 WriteObject(DataChunkPtr &Buffer, PrimerPtr UsePrimer = NULL)
		{
			return Object->WriteObject(Buffer, NULL, UsePrimer);
		}

		MDOTypePtr GetType(void) const { return Object->GetType(); };
		MDObjectPtr GetLink(void) const { return Object->GetLink(); };
		void SetLink(MDObjectPtr NewLink) { Object->SetLink(NewLink); };
		DictRefType GetRefType(void) const { return Object->GetRefType(); };

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(std::string BaseType) { return Object->IsA(BaseType); }

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(MDOTypePtr BaseType) { return Object->IsA(BaseType); }

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
inline MDObjectPtr MDObjectParent::operator[](const char *ChildName) { return GetPtr()->operator[](ChildName); }
inline MDObjectPtr MDObjectParent::operator[](MDOTypePtr ChildType) { return GetPtr()->operator[](ChildType); }
}

#endif // MXFLIB__MDOBJECT_H
