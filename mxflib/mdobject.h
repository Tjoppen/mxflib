/*! \file	mdobject.h
 *	\brief	Definition of classes that define metadata objects
 *
 *			Class MDObject holds info about a specific metadata object
 *<br><br>
 *			Class MDOType holds the definition of MDObjects derived from
 *			the XML dictionary.
 *<br><br>
 *
 *	\version $Id: mdobject.h,v 1.23 2011/01/10 10:42:09 matt-beard Exp $
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



namespace mxflib
{
	//! SymbolSpace used to translate a symbolic name to a UL
	class SymbolSpace : public std::map<std::string, ULPtr>, public RefCount<SymbolSpace>
	{
	protected:
		static SymbolSpaceMap AllSymbolSpaces;					//!< Map of all existing symbol spaces by name to allow full searching

		std::string SymName;									//!< The name of this symbol space

	private:
 		//! Prevent copy construction by NOT having an implementation to this copy constructor
		SymbolSpace(const SymbolSpace &rhs);

	public:
		//! Construct a new symbol space
		SymbolSpace(std::string Name) : SymName(Name)
		{
			SymbolSpaceMap::iterator it = AllSymbolSpaces.find(Name);

			if(it != AllSymbolSpaces.end())
			{
				error("Duplicate symbol space name \"%s\"\n");
			}

			AllSymbolSpaces.insert(SymbolSpaceMap::value_type(Name, this));
		};


		//! Add a new symbol to this symbol space
		/*! \return true if added OK, else false (most likely a duplicate symbol name)
		 */
		bool AddSymbol(std::string Symbol, ULPtr &UL)
		{
			iterator it = find(Symbol);
			if(it != end()) return false;

			insert(value_type(Symbol, UL));

			return true;
		}


		//! Locate the given symbol in this symbol space, optionally check all other spaces too
		ULPtr Find(std::string Symbol, bool SearchAll = false)
		{
			iterator it = find(Symbol);

			if(it != end()) return (*it).second;

			if(SearchAll)
			{
				SymbolSpaceMap::iterator map_it = AllSymbolSpaces.begin();
				while(map_it != AllSymbolSpaces.end())
				{
					it = (*map_it).second->find(Symbol);
					if(it != (*map_it).second->end()) return (*it).second;

					map_it++;
				}
			}

			return NULL;
		}

		//! Find the symbol space with a given name
		static SymbolSpacePtr FindSymbolSpace(std::string Name)
		{
			SymbolSpaceMap::iterator it = AllSymbolSpaces.find(Name);

			if(it != AllSymbolSpaces.end()) return (*it).second;

			return NULL;
		}

		//! Get the name of this symbol space
		const std::string &Name(void) const { return SymName; }
	};
}


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

	//! A map of MDOTypes by UL
	typedef std::map<UL, const MDOType *> MDOTypeULMap;

	// Forward declare the ObjectInterface
	class ObjectInterface;
}





namespace mxflib
{
	//! Holds the definition of a metadata object type
	class MDOType : public RefCount<MDOType>, public MDOTypeMap, 
				    public IMDTypeGetCommon, 
				    public IMDOTypeGet, 
					public IMDOTypeFind,
					public IMDOTypeSet, 
//					public IMDEffectiveType<MDOTypePtr, const MDOTypePtr&>, 
					public IMDKeyAccess, 
					public IMDTypeChild<MDOTypePtr, MDOTypeList>, 
					public IMDStructure, 
					public IMDTypeManip, 
					public IMDDict,
					public IMDOTypeDeprecated
	{
	protected:
		MDContainerType ContainerType;

		std::string RootName;			//!< Base name of this type

	protected:
		MDTypePtr ValueType;			//!< Value type if this is an actual data item, else NULL

	public:
		MDOTypeParent Base;				//!< Base class if this is a derived class, else NULL
		StringList MetadictOrder;		//!< AVMETA: Order to write properties in ClassDef of Avid Metadictionary

	protected:
		StringList ChildOrder;			//!< Child names in order, for packs	## DEPRECATED - Use ChildList ##
		MDOTypeList ChildList;			//!< Child types in order, for packs
		MDOTypeParent Parent;			//!< Parent type if this is a child
		ULPtr TypeUL;					//!< The UL for this type, or NULL
		Tag TypeTag;					//!< The 2-byte tag for this type, or 0

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
		ClassUsage		Use;			//!< Usage requirements
		DataChunk		Default;		//!< Default value (if one exists)
		DataChunk		DValue;			//!< Distinguished value (if one is defined)
		ClassRef		RefType;		//!< Reference type if this is a reference
		MDOTypeParent	RefTarget;		//!< Type (or base type) of item this ref source must target
		std::string		RefTargetName;	//!< Name of the type (or base type) of item this ref source must target
										/*!< \note This must only be used during dictionary parsing or for error reporting,
										 *         not for actual reference linking where RefTarget must be used
										 */

		bool			BaselineClass;	//!< True if this is a baseline class as defined in 377M or certain other specific standards (and so will not be added to the KXS metadictionary)

		//! Protected constructor so we can control creation of types
		MDOType();

	public:
		//! Public constructor - build a full type
		MDOType(MDContainerType ContainerType, std::string RootName, std::string Name, std::string Detail, MDTypePtr Type, 
				DictKeyFormat KeyFormat, DictLenFormat LenFormat, unsigned int minLen, unsigned int maxLen, DictUse Use)
			: ContainerType(ContainerType), RootName(RootName), ValueType(Type), TypeTag(0), DictName(Name), Detail(Detail),
			  KeyFormat(KeyFormat), LenFormat(LenFormat), minLength(minLen), maxLength(maxLen), Use(Use), BaselineClass(false)
		{
			// MaxLength = 0 is used for maxlength = unbounded
			if(maxLength == 0) maxLength = (unsigned int)-1;

			if(Type) TypeName = Type->Name();
			else TypeName = Name;

			// Set the name lookup - UL lookup set when key set
			NameLookup[RootName + Name] = this;

			// Start of with no referencing details
			RefType = ClassRefNone;
		};


		/* Interface IMDTypeGetCommon */
		/******************************/

		//! Get the name of this type
		const std::string &Name(void) const { return DictName; }

		//! Get the full name of this type, including all parents
		std::string FullName(void) const { return RootName + DictName; }

		//! Report the detailed description for this type
		const std::string &GetDetail(void) const { return Detail; };


		/* Interface IMDOTypeGet */
		/*************************/

		//! Get the type of this object (returns self if this is a type, may return NULL for sub-items of a complex type)
		const MDOType *GetType(void) const { return this; }

		//! Get the type of this object (returns self if this is a type, may return NULL for sub-items of a complex type)
		const MDOTypePtr GetType(void) { return this; }

		//! Get the type of the value for this object (returns NULL if a group rather than an element)
		const MDTypePtr &GetValueType(void) const { return ValueType; }

		//! Read-only access to KeyFormat
		const DictKeyFormat &GetKeyFormat(void) const { return KeyFormat; }

		//! Read-only access to LenFormat
		const DictLenFormat &GetLenFormat(void) const { return LenFormat; }

		//! Read-only access to the minLength value
		unsigned int GetMinLength(void) const { return minLength; }

		//! Read-only access to the maxnLength value
		unsigned int GetMaxLength(void) const { return maxLength; }

		//! Read-only access to default value
		const DataChunk &GetDefault(void) const { return Default; }

		//! Read-only access to distinguished value
		const DataChunk &GetDValue(void) const { return DValue; }

		//! Access function for ContainerType
		MDContainerType GetContainerType(void) const { return ContainerType; }

		//! Get the usage for this type
		ClassUsage GetUse(void) const { return Use; }

		//! Ref access function
		ClassRef GetRefType(void) const { return RefType; };

		//! Accessor for Reference Target
		const MDOTypeParent &GetRefTarget(void) const { return RefTarget; };

		//! Accessor for Reference Target Name
		/*!< \note This must only be used during dictionary parsing or for error reporting,
		 *         not for actual reference linking where RefTarget must be used
		 */
		std::string GetRefTargetName(void) const { return RefTargetName; };


		/* Interface IMDOTypeFind */
		/**************************/

	public:
		/* Static MDOType reading methods */

		//! Find a type in the default symbol space, optionally searching all others
		static MDOTypePtr Find(std::string BaseType, bool SearchAll = false) { return Find(BaseType, MXFLibSymbols, SearchAll); }
		
		//! Find a type in a specified symbol space, optionally searching all others
		static MDOTypePtr Find(std::string BaseType, SymbolSpacePtr &SymSpace, bool SearchAll = false);

		//! Find a type with a given UL
		static MDOTypePtr Find(const UL& BaseUL);

		//! Find a type via a tag
		static MDOTypePtr Find(Tag BaseTag, PrimerPtr BasePrimer);


		/* Interface IMDOTypeSet */
		/*************************/

		//! Insert a new child type
		/*! DRAGONS: This overloads the insert() methods of the base map */
		std::pair<MDOType::iterator, bool> insert(MDOTypePtr NewType);


		/* Interface IMDKeyAccess */
		/**************************/

		//! Set the UL for this type or this specific object
		void SetUL(ULPtr &Val) { TypeUL = Val; }

		//! Read-only access to the current UL (same as GetTypeUL for types, but may differ for actual objects)
		const ULPtr &GetUL(void) const { return TypeUL; }

		//! Read-only access to the type UL (the UL for the defined type, ignoring any UL set specifically for this object)
		const ULPtr &GetTypeUL(void) const { return TypeUL; }

		//! Set the tag for this type or this specific object
		void SetTag(Tag NewTag)
		{
			Key.Resize(2);
			Key.Data[0] = static_cast<UInt8>(NewTag >> 8);
			Key.Data[1] = static_cast<UInt8>(NewTag & 0xff);
		}

		//! Get the tag for this type or object
		Tag GetTag(void) const
		{
			if(Key.Size == 2) return (static_cast<Tag>(Key.Data[0]) << 8) | (static_cast<Tag>(Key.Data[1]));
			return 0;
		}


		/* Interface IMDOTypeChild */
		/**************************/

		//! Read-only access to ChildList
		const MDOTypeList &GetChildList(void) const { return ChildList; }

		//! Locate a named child
		MDOTypePtr Child(const std::string Name) const
		{
			MDOTypeMap::const_iterator it = find(Name);
			if(it != end()) return (*it).second;
			return NULL;
		}

		//! Locate a named child
		MDOTypePtr operator[](const std::string Name) const { return Child(Name); }

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		/* FIXME: Add this method */
		MDOTypePtr Child(int Index) const;

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MDOTypePtr operator[](int Index) const { return Child(Index); }

		//! Locate a child by UL
		MDOTypePtr Child(ULPtr &ChildType) const
		{
			MDOTypeMap::const_iterator it = begin();
			while(it != end()) 
			{ 
				if(((*it).second->TypeUL) && (((*it).second->TypeUL)->Matches(*ChildType))) return (*it).second; 
				it++;
			}
			return NULL;
		}

		//! Locate a child by UL
		MDOTypePtr operator[](ULPtr &ChildType) const { return Child(ChildType); }

		//! Locate a child by UL
		MDOTypePtr Child(const UL &ChildType) const
		{
			MDOTypeMap::const_iterator it = begin();
			while(it != end()) 
			{ 
				if(((*it).second->TypeUL) && (((*it).second->TypeUL)->Matches(ChildType))) return (*it).second; 
				it++;
			}
			return NULL;
		}

		//! Locate a child by UL
		MDOTypePtr operator[](const UL &ChildType) const { return Child(ChildType); }


		/* Interface IMDStructure */
		/**************************/

		//! Determine if this type is derived from a specified type (directly or indirectly)
		bool IsA(std::string BaseType) const;

		//! Determine if this type is derived from a specified type (directly or indirectly)
		bool IsA(MDOTypePtr &BaseType) const;
		
		//! Determine if this type is derived from a specified type (directly or indirectly)
		bool IsA(const UL &BaseType) const;

		//! Determine if this type is derived from a specified type (directly or indirectly)
		bool IsA(ULPtr &BaseType) const { return IsA(*BaseType); };

		//! Determine if this type is known to have a child with a given UL
		/*! This determines if the specified UL has been included as a child of this type in any loaded disctionary.
		 *  It may be valid for children of this UL to be included, even if this function returns false 
		 */
		bool HasA(const ULPtr &ChildType) const;

		//! Is the is baseline class, as defined in 377M or certain other specific standards (and so will not be added to the KXS metadictionary)
		bool IsBaseline(void) const
		{ 
			return BaselineClass; 
		}

		//! Get read-only access to the base type
		const MDOTypeParent &GetBase(void) const { return Base; }

	public:
		/* Static structure methods */

		//! Get the static primer (make one if required)
		static PrimerPtr GetStaticPrimer(void)
		{ 
			if( !StaticPrimer) MakePrimer(true); 
			return StaticPrimer;
		}


		/* Interface IMDTypeManip */
		/**************************/

		//! Derive this new entry from a base entry
		void Derive(MDOTypePtr &BaseEntry);

		//! Re-Derive sub-items from a base entry
		/*! Used when the base entry is being extended 
		 */
		void ReDerive(MDOTypePtr &BaseEntry);

		//! Redefine a sub-item in a container
		void ReDefine(std::string NewDetail, std::string NewBase, unsigned int NewMinSize, unsigned int NewMaxSize);
		
		//! Redefine a container
		void ReDefine(std::string NewDetail)
		{
			if(NewDetail.length()) Detail = NewDetail;
		}

	public:
		/* Static manipulation methods */

		//! Locate reference target types for any types not yet located
		static void LocateRefTypes(void);

		//! Build a primer from the current dictionary, optionally set as the static primer
		static PrimerPtr MakePrimer(bool SetStatic = false);


		/* Interface IMDDict */
		/*********************/

	public:
		/* Static dictionary methods */

		//! Load the dictionary
		static void LoadDict(const char *DictFile, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols)
		{
			LoadDictionary(DictFile, DefaultSymbolSpace);
		}


		//! Generate a new type based on a base type
		/*! DRAGONS: Use with great care - this is intended for library code that generates inferred classes */
		static MDOTypePtr DeriveCopy(MDOTypePtr &Base, std::string Name, std::string Detail);

		//! Define a class based on a ClassRecord
		static MDOTypePtr DefineClass(ClassRecordPtr &ThisClass, SymbolSpacePtr DefaultSymbolSpace, MDOTypePtr Parent)
		{
			return DefineClass(ThisClass, DefaultSymbolSpace, NULL, Parent);
		}

		// Define a class based on a ClassRecord with an optional symbol space
		static MDOTypePtr DefineClass(ClassRecordPtr &ThisClass, MDOTypePtr Parent = NULL, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols)
		{
			return DefineClass(ThisClass, DefaultSymbolSpace, NULL, Parent);
		}

		//! Define a class based on a ClassRecord - recursive version to allow out-of order definitions
		static MDOTypePtr DefineClass(ClassRecordPtr &ThisClass, SymbolSpacePtr DefaultSymbolSpace, ClassRecordList *Unresolved = NULL, MDOTypePtr Parent = NULL);

		/* Interface IMDOTypeDeprecated */
		/********************************/

		//! Read-only access to ChildOrder
		/*! \note DEPRECATED - Use GetChildList() instead
		 */
		const StringList &GetChildOrder(void) const { return ChildOrder; }


	/* Non-API methods */

	public:

		//! Set the KeyFormat
		void SetKeyFormat(DictKeyFormat Val) { KeyFormat = Val; }

		//! Set the LenFormat
		void SetLenFormat(DictLenFormat Val) { LenFormat = Val; }

		//! Read-only access to the key in use, in bytes, either local tag, or UL if a set/pack
		const DataChunk &GetKey(void) const { return Key; }

		//! Read-only access to the global key, in bytes
		const DataChunk &GetGlobalKey(void) const { return GlobalKey; }

		//! Get the parent of this type (if it has one)
		MDOType const *GetParent(void) const { return Parent; }


	//** Static Dictionary Handling data and functions **
	//***************************************************
	protected:
		static MDOTypeList	AllTypes;	//!< All types managed by this object
		static MDOTypeList	TopTypes;	//!< The top-level types managed by this object

		//! Map for UL lookups
		static std::map<UL, MDOTypePtr> ULLookup;
		
		//! Map for UL lookups - ignoring the version number (all entries use version = 1)
		static std::map<UL, MDOTypePtr> ULLookupVer1;

		//! Map for reverse lookups based on type name
		static MDOTypeMap NameLookup;

		//! Flag to show when we have loaded types and classes required for internal use
		static bool InternalsDefined;

	public:
		//! Bit masks for items that are to be set for this definition in BuildTypeFromDict
		/*! This allows some values to be inherited from the base class,
		 *  but some to be overridden by this definition.
		 *  \note The bits are actually passed in a UInt32.
		 *  \note Not all items need a bit as they can be validated.
		 */
		enum DictionaryItems
		{
			DICT_ITEM_USE = 1,
			DICT_ITEM_REFTYPE = 2,
			DICT_ITEM_CONTAINERTYPE = 4,
			DICT_ITEM_MINLENGTH = 8,
			DICT_ITEM_MAXLENGTH = 16,
			DICT_ITEM_KEYFORMAT = 32,
			DICT_ITEM_LENFORMAT = 64,
			DICT_ITEM_DVALUE = 128
		};

	protected:
		//! Basic primer for use when parsing non-primer partitions
		static PrimerPtr StaticPrimer;

	public:
		//! Build the built-in primer, optionally set as the static primer
		static PrimerPtr MakeBuiltInPrimer(bool SetStatic = false);
		
		//! Accessor for InternalsDefined
		static bool GetInternalsDefined(void) { return InternalsDefined; }

		//! Clear any loaded dictionary data
		/*! This can be used before loading a different dictionary, or to free allocated memory for debugging (such as memory leak detection) */
		static void ClearDict(void)
		{
			AllTypes.clear();
			TopTypes.clear();
			ULLookup.clear();
			ULLookupVer1.clear();
			NameLookup.clear();
			InternalsDefined = false;
		}

		//! Load types and classes required for internal use 
		static void DefineInternals(void);

		//! Determine if there is a valid MXF dictionary loaded
		/*! \return true if there is an MXF dictionary loaded */
		static bool IsDictLoaded(void)
		{
			MDOTypePtr Preface = MDOType::Find(Preface_UL);
			return (!Preface) ? false : true;
		}


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
		MDObjectPtr operator[](const char *ChildName) const;
		MDObjectPtr operator[](const MDOTypePtr &ChildType) const;
		MDObjectPtr operator[](const MDTypePtr &ChildType) const;
		MDObjectPtr operator[](int Index) const;
		MDObjectPtr operator[](const UL &ChildType) const;
		MDObjectPtr operator[](const ULPtr &ChildType) const;
	};

	//! Construct an MDObjectPtr from an MDObjectParent
	//  Defined here as both classes need to be defined before this
	inline MDObjectPtr::MDObjectPtr(MDObjectParent ptr) : SmartPtr<MDObject>(ptr) {};

	//! A list of smart pointers to MDObject objects
	class MDObjectList : public RefCount<MDObjectList>, public std::list<MDObjectPtr> {};
	typedef SmartPtr<MDObjectList> MDObjectListPtr;

	//! A list of smart pointers to MDObject objects with names
	typedef std::pair<std::string,MDObjectPtr> MDObjectNamedListItem;
	typedef std::list<MDObjectNamedListItem> MDObjectNamedList;

	//! A list of smart pointers to MDObject objects with ULs
	typedef std::pair<UL,MDObjectPtr> MDObjectULListItem;
	typedef std::list<MDObjectULListItem> MDObjectULList;

	//! A map of MDObject pointers by UL
	typedef std::map<UL, MDObjectPtr> MDObjectMap;
}


namespace mxflib
{
	//! Metadata Object class
	class MDObject : public RefCount<MDObject>, public MDObjectULList, 
					 public IMDTypeGetCommon,
					 public IMDOTypeGet,
					 public IMDTypeGet,
					 public IMDKeyAccess,
					 public IMDEffectiveType<MDType, MDTypePtr>,
					 public IMDTraitsAccess,
					 public IMDChildAccess,
					 public IMDValueGet,
					 public IMDValueSet,
					 public IMDValueIO
	{
	protected:
		MDOTypePtr Type;				//!< The type of this object
		MDTypePtr ValueType;			//!< The type of the value held in this object, if it is a value
		bool IsValue;					//!< True if we are a value, rather than a container for other MDObjects
		DataChunk Data;					//!< The actual data for this object, if we are a value
		bool IsSubItem;					//!< True if this is a sub-item of a metadata item, not an item in it's own right

		MDObjectParent Link;			//!< The target of this reference if we are a weak or strong reference
		MDObjectPtr OwningLink;			//!< The target of this reference if we are a strong reference - will own the target and keep it from being deleted whicle we exist

		bool IsConstructed;				//!< True if this object is constructed, false if read from a file or a parent object
		Position ParentOffset;			//!< Offset from start of parent object if read from file or object
		UInt32 KLSize;					//!< Size of this objects KL if read from file or parent object
		MDObjectParent Parent;			//!< Pointer to parent if read from inside another object
		MXFFileParent ParentFile;		//!< Pointer to parent file if read from a file
		ULPtr TheUL;					//!< The UL for this object (if known)
		Tag TheTag;						//!< The local tag used for this object (if known)

		std::string ObjectName;			//!< The name of this object (normally the name of the type)


		MDTraitsPtr Traits;				//! The current traits for reading and modifying this value (if IsValue is true)

		bool Modified;					//!< True if this object has been modified since being "read"
										/*!< This is used to automatically update the GenerationUID when writing the object */

		ObjectInterface *Outer;			//!< Pointer to outer object if this is a sub-object of an ObjectInterface derived object

	public:
		//! Pointer to a translator function to translate unknown ULs to object names
		typedef std::string (*ULTranslator)(ULPtr,const Tag *);
		
		//! Pointer to a translator function to manufacture a type for an object name
		typedef MDOTypePtr (*ULTypeMaker)(ULPtr,const Tag *);

	protected:
		//! Translator function to translate unknown ULs to object names
		static ULTranslator UL2NameFunc;

		//! Translator function to translate unknown ULs to object names
		static ULTypeMaker TypeMaker;

		//! Static flag to say if dark metadata sets that appear to be valid KLV 2x2 sets should be parsed
		static bool ParseDark;

	public:
		//! Our value - this will either by ourself (if we represent a value) or NULL
		/*! DRAGONS: This is for legacy support as Value used to point to an MDValue object
		 */
		MDObject *Value;

	protected:
		//! MDObject UL based constructor body
		/*! Builds a "blank" metadata object of a specified type
		 *	\note packs are built with default values
		 *
		 *  \note TheUL must be set before calling
		 */
		void ULCtor(void);

		//! MDObject "unknown" object constructor body
		/*! Builds a "blank" metadata object with an unknown type based on a given UL
		*	\note packs are built with default values
		*
		*  \note TheUL and TheTag must be set before calling
		*/
		void UnknownCtor(void);

	public:
		//! Construct a new MDObject of the specified type
		/*! BaseType is a symbol to be located in the given SymbolSpace - if no SymbolSpace is specifed the default MXFLib space is used 
		 */
		MDObject(std::string BaseType, SymbolSpacePtr &SymSpace = MXFLibSymbols );

		//! Construct a new MDObject of the specified type
		MDObject(MDOTypePtr BaseType);

		//! Construct a new MDObject of the type with the specified UL
		MDObject(const UL &BaseUL) { TheUL = new UL(BaseUL); ULCtor(); }

		//! Construct a new MDObject of the type with the specified UL
		MDObject(const ULPtr &BaseUL) { TheUL = BaseUL; ULCtor(); }

		//! Construct a new MDObject of the type with the specified UL
		MDObject(Tag BaseTag, PrimerPtr BasePrimer);

		//! Construct a generic MDObject with given value type
		MDObject(const MDType *ValType);

		//! Construct a generic MDObject with given traits
		MDObject(MDTraitsPtr &Tr);

		//! Generic constructor - called after each specific constructor has done its specific bits
		void Init(void);

		//! Destroy an MDObject
		~MDObject();


		/* Interface IMDTypeGetCommon */
		/******************************/

		//! Get the name of this type
		const std::string &Name(void) const { return ObjectName; }

		//! Get the full name of this type, including all parents
		std::string FullName(void) const
		{
			if(Parent) return Parent->FullName() + "/" + ObjectName;
			else return ObjectName; 
		};

		//! Report the detailed description for this type
		const std::string &GetDetail(void) const { return Type->GetDetail(); }


		/* Interface IMDOTypeGet */
		/*************************/

		//! Get the type of this object (returns self if this is a type, may return NULL for sub-items of a complex type)
		const MDOType *GetType(void) const { return Type; }

		//! Get the type of this object (returns self if this is a type, may return NULL for sub-items of a complex type)
		const MDOTypePtr GetType(void) { return Type; }

		//! Get the type of the value for this object (returns NULL if a group rather than an element)
		const MDTypePtr &GetValueType(void) const { return ValueType; }

		//! Read-only access to KeyFormat
		const DictKeyFormat &GetKeyFormat(void) const { return Type->GetKeyFormat(); }

		//! Read-only access to LenFormat
		const DictLenFormat &GetLenFormat(void) const { return Type->GetLenFormat(); }

		//! Read-only access to the minLength value
		unsigned int GetMinLength(void) const { return Type->GetMinLength(); }

		//! Read-only access to the maxnLength value
		unsigned int GetMaxLength(void) const { return Type->GetMaxLength(); }

		//! Read-only access to default value
		const DataChunk &GetDefault(void) const { return Type->GetDefault(); }

		//! Read-only access to distinguished value
		const DataChunk &GetDValue(void) const { return Type->GetDValue(); }

		//! Access function for ContainerType
		MDContainerType GetContainerType(void) const 
		{ 
			if(IsValue)
			{
				if(ValueType->GetClass() == TYPEARRAY)
				{
					if(ValueType->GetArrayClass() == ARRAYEXPLICIT) return BATCH;
					return ARRAY;
				}
				return NONE;
			}
			if(!Type) return NONE;
			return Type->GetContainerType(); 
		}

		//! Get the usage for this type
		ClassUsage GetUse(void) const { return Type->GetUse(); }

		//! Get the reference type
		TypeRef GetRefType(void) const
		{ 
			if(IsSubItem || (!Type))
			{
				mxflib_assert(ValueType);
				return ValueType->EffectiveRefType();
			}
			if(!Type) return TypeRefUndefined;
			return Type->GetRefType(); 
		}

		//! Get the reference target
		const MDOTypeParent &GetRefTarget(void) const
		{ 
			if(IsSubItem || (!Type))
			{
				mxflib_assert(ValueType);
error("UNSUPPORTED: Trying to determine ref target of %s\n", FullName().c_str());
//				return MDOType::Find(ValueType->EffectiveRefTarget());
			}
			mxflib_assert(Type);
			return Type->GetRefTarget();
		}

		//! Accessor for Reference Target Name
		/*!< \note This must only be used during dictionary parsing or for error reporting,
		 *         not for actual reference linking where RefTarget must be used
		 */
		std::string GetRefTargetName(void) const
		{ 
			if(IsSubItem || (!Type))
			{
				mxflib_assert(ValueType); 
				return ValueType->GetRefTargetName();
			}
			mxflib_assert(Type);
			return Type->GetRefTargetName();
		}


		/* Interface IMDTypeGet */
		/************************/

		//! Is this a "character" type
		bool IsCharacter(void) const { return ValueType ? ValueType->IsCharacter() : false; }

		//! Endian access function (get)
		bool GetEndian(void) const { return ValueType ? ValueType->GetEndian() : false; }

		//! Get the size of this type, in bytes if basic, or items if a multiple
		/*! DRAGONS: This gets the defined size for this type, not the size of the current value */
		int GetSize(void) const { return ValueType ? ValueType->GetSize() : 0; }

		//! Get a const reference to the enum values
		const NamedValueList &GetEnumValues(void) const 
		{
			static NamedValueList Null;
			return ValueType ? ValueType->GetEnumValues() : Null; 
		}

		//! Get the class of this type
		MDTypeClass GetClass(void) const { return ValueType ? ValueType->GetClass() : BASIC; }

		//! ArrayClass access function (get)
		MDArrayClass GetArrayClass(void) const { return ValueType ? ValueType->GetArrayClass() : ARRAYIMPLICIT; }


		/* Interface IMDKeyAccess */
		/**************************/

		//! Set the UL for this type or this specific object
		void SetUL(ULPtr &Val) 
		{ 
			TheUL = Val;
		}

		//! Read-only access to the current UL (same as GetTypeUL for types, but may differ for actual objects)
		const ULPtr &GetUL(void) const { return TheUL; }

		//! Read-only access to the type UL (the UL for the defined type, ignoring any UL set specifically for this object)
		const ULPtr &GetTypeUL(void) const { return Type->GetTypeUL(); }

		//! Set the tag for this type or this specific object
		void SetTag(Tag NewTag) { TheTag = NewTag; }

		//! Get the tag for this type or object
		Tag GetTag(void) const { return TheTag; }


		/* Interface IMDEffectiveType */
		/******************************/

		//! Report the effective type of this type
		const MDType *EffectiveType(void) const { return ValueType ? ValueType->EffectiveType() : NULL; }

		//! Report the effective class of this type
		MDTypeClass EffectiveClass(void) const { return ValueType ? ValueType->EffectiveClass() : BASIC; }

		//! Report the effective base type of this type
		MDTypePtr EffectiveBase(void) const { return ValueType ? ValueType->EffectiveBase() : NULL; }

		//! Report the effective reference type of this type
		TypeRef EffectiveRefType(void) const { return ValueType ? ValueType->EffectiveRefType() : TypeRefUndefined; }

		//! Report the effective reference target of this type
		MDOTypePtr EffectiveRefTarget(void) const { return ValueType ? ValueType->EffectiveRefTarget() : NULL; }

		//! Report the name of the effective reference target of this type
		/*! DRAGONS: To be used when loading dictionary only */
		std::string EffectiveRefTargetName(void) const { return ValueType ? ValueType->EffectiveRefTargetName() : ""; };

		//! Report the effective size of this type
		/*! \return The size in bytes of a single instance of this type, or 0 if variable size
		 */
		UInt32 EffectiveSize(void) const { return ValueType ? ValueType->EffectiveSize() : 0; }


		/* Interface IMDTraitsAccess */
		/*****************************/
	
		//! Set the traits for this type or object
		void SetTraits(MDTraitsPtr Tr) { Traits = Tr; }

		//! Access the traits for this type or object
		const MDTraitsPtr &GetTraits(void) const
		{
			// Get the traits from the instance or base value type or type
			if( Traits ) return Traits;
			else if( ValueType ) return ValueType->GetTraits();
			else if( Type && Type->GetValueType() ) return Type->GetValueType()->GetTraits();
			else return Traits; // which is NULL - but this avoids a compiler Warning
		}

		//! Does this value's trait take control of all sub-data and build values in the our own DataChunk?
		/*! Normally any contained sub-types (such as array items or compound members) hold their own data */
		bool HandlesSubdata(void) const { return Traits ? Traits->HandlesSubdata() : false; }

	public:
		/* Static traits methods */

		//! Add a mapping to be applied to all types of a given type name
		/*! \note This will act retrospectively
		 */
		static bool AddTraitsMapping(std::string TypeName, std::string TraitsName) { return MDType::AddTraitsMapping(TypeName, TraitsName); }

		//! Update an existing mapping and apply to any existing type of the given name
		static bool UpdateTraitsMapping(std::string TypeName, std::string TraitsName) { return MDType::UpdateTraitsMapping(TypeName, TraitsName); }

		//! Add a mapping to be applied to all types of a given type UL
		/*! \note This will act retrospectively
		 */
		static bool AddTraitsMapping(const UL &TypeUL, std::string TraitsName) { return MDType::AddTraitsMapping(TypeUL, TraitsName); }

		//! Update an existing mapping and apply to any existing type of the given UL
		static bool UpdateTraitsMapping(const UL &TypeUL, std::string TraitsName) { return MDType::UpdateTraitsMapping(TypeUL, TraitsName); }

		//! Lookup the traits for a specified type name
		/*! If no traits have been defined for the specified type the traits with the name given in DefaultTraitsName is used (if specified)
		 */
		static MDTraitsPtr LookupTraitsMapping(std::string TypeName, std::string DefaultTraitsName = "") { return MDType::LookupTraitsMapping(TypeName, DefaultTraitsName); }

		//! Lookup the traits for a specified type UL
		/*! If no traits have been defined for the specified type the traits with the UL given in DefaultTraitsName is used
		*/
		static MDTraitsPtr LookupTraitsMapping(const UL &TypeUL, const UL &DefaultTraitsUL) { return MDType::LookupTraitsMapping(TypeUL, DefaultTraitsUL); }

		//! Lookup the traits for a specified type name
		/*! If no traits have been defined for the specified type the traits with the UL given in DefaultTraitsName is used
		 */
		static MDTraitsPtr LookupTraitsMapping(std::string TypeName, const UL &DefaultTraitsUL) { return MDType::LookupTraitsMapping(TypeName, DefaultTraitsUL); }

		//! Lookup the traits for a specified type UL
		/*! If no traits have been defined for the specified type the traits with the name given in DefaultTraitsName is used (if specified)
		*/
		static MDTraitsPtr LookupTraitsMapping(const UL &TypeUL, std::string DefaultTraitsName = "") { return MDType::LookupTraitsMapping(TypeUL, DefaultTraitsName); }


		/* Interface IMDChildAccess */
		/****************************/

		//! Read-only access to ChildList
		const MDOTypeList &GetChildList(void) const { return Type->GetChildList(); }

		//! Locate a named child
		MDObjectPtr Child(const std::string Name) const { return operator[](Name); }

		//! Locate a named child
		MDObjectPtr operator[](const std::string Name) const;

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MDObjectPtr Child(int Index) const { return operator[](Index); }

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MDObjectPtr operator[](int Index) const;

		//! Locate a child by UL
		MDObjectPtr Child(ULPtr &ChildType) const { return operator[](*ChildType); }

		//! Locate a child by UL
		MDObjectPtr operator[](ULPtr &ChildType) const { return operator[](*ChildType); }

		//! Locate a child by UL
		MDObjectPtr Child(const UL &ChildType) const { return operator[](ChildType); }

		//! Locate a child by UL
		MDObjectPtr operator[](const UL &ChildType) const;

		//! Locate a child by object type
		MDObjectPtr Child(const MDOTypePtr &ChildType) const { return operator[](ChildType); }

		//! Locate a child by object type
		MDObjectPtr operator[](const MDOTypePtr &ChildType) const;

		//! Locate a child by value type
		MDObjectPtr Child(const MDTypePtr &ChildType) const { return operator[](ChildType); }

		//! Locate a child by value type
		MDObjectPtr operator[](const MDTypePtr &ChildType) const;

		//! Add a new child MDObject of the specified type
		/*! ChildName is a symbol to be located in default MXFLib SymbolSpace
		 */
		MDObjectPtr AddChild(std::string ChildName, bool Replace) { return AddChild(ChildName, MXFLibSymbols, Replace); }

		//! Add a new child MDObject of the specified type
		/*! ChildName is a symbol to be located in the given SymbolSpace - if no SymbolSpace is specifed the default MXFLib space is used 
		 */
		MDObjectPtr AddChild(std::string ChildName, SymbolSpacePtr &SymSpace = MXFLibSymbols, bool Replace = true);

		//! Add a new child MDObject of the specified type
		MDObjectPtr AddChild(MDOTypePtr ChildType, bool Replace = true);
		
		//! Add a new child MDObject to a vector
		/*! \note The type of the object added is automatic. 
		 *        If the vector is of multiple members the next type will be chosen by the number of members currently
		 *        in the array, so if there are 3 sub member types the 7th entry will be type 1 [ 7 = (2*3) + 1 ]
		 *
		 *  \note This version of AddChild will <b>not</b> replace duplicates, it always appends
		 */
		MDObjectPtr AddChild(void);

		//! Add a new child MDObject of the specified type
		MDObjectPtr AddChild(const UL &ChildType, bool Replace = true);
		
		//! Add a new child MDObject of the specified type
		MDObjectPtr AddChild(ULPtr &ChildType, bool Replace = true) { return AddChild(*ChildType, Replace); }

		//! Add the given child object
		/*! \ret false if unable to add this child */
		bool AddChild(MDObjectPtr &ChildObject, bool Replace = false);

//		//! Add the given child object at a specific numerical index
//		/*! \ret false if unable to add this child at the specified location (for example if not numerically indexed) */
//		bool AddChild(MDObjectPtr &Child, int Index);

		//! Remove the (first) child of the specified type
		void RemoveChild(std::string ChildName);

		//! Remove the (first) child of the specified type
		void RemoveChild(MDOTypePtr &ChildType);

		//! Remove the (first) child of the specified type
		void RemoveChild(ULPtr &ChildType)
		{
			MDOTypePtr CType = MDOType::Find(ChildType);
			if(CType) RemoveChild(CType);
		}

		void RemoveChild(const UL &ChildType)
		{
			MDOTypePtr CType = MDOType::Find(ChildType);
			if(CType) RemoveChild(CType);
		}

		//! Remove the specified child
		void RemoveChild(MDObjectPtr ChildObject);

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const std::string ChildName) const;

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const UL &ChildType) const;

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const ULPtr &ChildType) const { return ChildList(*ChildType); };

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const MDOTypePtr &ChildType) const;

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const MDTypePtr &ChildType) const;


		/* Interface IMDValueGet */
		/*************************/

		/* Get the value of this object */

		//! Get the 32-bit signed integer version of value
		Int32 GetInt(Int32 Default = 0) const { if( Value && GetTraits() ) return GetTraits()->GetInt(Value); else return Default; }

		//! Get the 64-bit signed integer version of value
		Int64 GetInt64(Int64 Default = 0) const { if( Value && GetTraits() ) return GetTraits()->GetInt64(Value); else return Default; }

		//! Get the 32-bit unsigned integer version of value
		UInt32 GetUInt(UInt32 Default = 0) const { if( Value && GetTraits() ) return GetTraits()->GetUInt(Value); else return Default; }

		//! Get the 64-bit unsigned integer version of value
		UInt64 GetUInt64(UInt64 Default = 0) const { if( Value && GetTraits() ) return GetTraits()->GetUInt64(Value); else return Default; }

		//! Get the UTF-8 string version of value
		std::string GetString(std::string Default = "", OutputFormatEnum Format = -1) const { if( Value && GetTraits() ) return GetTraits()->GetString(Value, Format); else return Default; }

		//! Get the UTF-8 string version of value
		std::string GetString(OutputFormatEnum Format) const { if( Value && GetTraits() ) return GetTraits()->GetString(Value, Format); else return ""; }


		//! Is this a Best Effort property that is set to its distinguished value?
		bool IsDValue(void) const;


		/* Get the value of a child object by name */

		//! Get the 32-bit signed integer version of value of named child
		Int32 GetInt(const char *ChildName, Int32 Default = 0) const
		{ 
			MDObjectPtr Ptr = operator[](ChildName);
			if (Ptr) return Ptr->GetInt(); else return Default; 
		}

		//! Get the 64-bit signed integer version of value of named child
		Int64 GetInt64(const char *ChildName, Int64 Default = 0) const
		{ 
			MDObjectPtr Ptr = operator[](ChildName); 
			if (Ptr) return Ptr->GetInt64(); else return Default;
		}

		//! Get the 32-bit unsigned integer version of value of named child
		UInt32 GetUInt(const char *ChildName, UInt32 Default = 0) const
		{ 
			MDObjectPtr Ptr = operator[](ChildName); 
			if (Ptr) return Ptr->GetUInt(); else return Default;
		}

		//! Get the 64-bit unsigned integer version of value of named child
		UInt64 GetUInt64(const char *ChildName, UInt64 Default = 0) const
		{ 
			MDObjectPtr Ptr = operator[](ChildName); 
			if (Ptr) return Ptr->GetUInt64(); else return Default;
		}

		//! Get the UTF-8 string version of value of named child
		std::string GetString(const char *ChildName, std::string Default = "", OutputFormatEnum Format = -1) const
		{ 
			MDObjectPtr Ptr = operator[](ChildName); 
			if (Ptr) return Ptr->GetString(Default, Format); else return Default;
		}

		//! Get the UTF-8 string version of value of named child
		std::string GetString(const char *ChildName, OutputFormatEnum Format) const
		{ 
			MDObjectPtr Ptr = operator[](ChildName); 
			if (Ptr) return Ptr->GetString(Format); else return "";
		}

		//! Is the named child a Best Effort property that is set to its distinguished value?
		bool IsDValue(const char *ChildName) const
		{ 
			MDObjectPtr Ptr = operator[](ChildName);
			if (Ptr) return Ptr->IsDValue(); else return false; 
		}


		/* Get the value of a child object by UL */

		//! Get the 32-bit signed integer version of value of UL identified child
		Int32 GetInt(const UL &Child, Int32 Default = 0) const
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if (Ptr) return Ptr->GetInt(); else return Default;
		}

		//! Get the 64-bit signed integer version of value of UL identified child
		Int64 GetInt64(const UL &Child, Int64 Default = 0) const
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if (Ptr) return Ptr->GetInt64(); else return Default;
		}

		//! Get the 32-bit unsigned integer version of value of UL identified child
		UInt32 GetUInt(const UL &Child, UInt32 Default = 0) const
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if (Ptr) return Ptr->GetUInt(); else return Default;
		}

		//! Get the 64-bit unsigned integer version of value of UL identified child
		UInt64 GetUInt64(const UL &Child, UInt64 Default = 0) const
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if (Ptr) return Ptr->GetUInt64(); else return Default;
		}

		//! Get the UTF-8 string version of value of UL identified child
		std::string GetString(const UL &Child, std::string Default = "", OutputFormatEnum Format = -1) const
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if (Ptr) return Ptr->GetString(Default, Format); else return Default;
		}

		//! Get the UTF-8 string version of value of UL identified child
		std::string GetString(const UL &Child, OutputFormatEnum Format) const
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if (Ptr) return Ptr->GetString(Format); else return "";
		}

		//! Is the UL identified child a Best Effort property that is set to its distinguished value?
		bool IsDValue(const UL &Child) const
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if (Ptr) return Ptr->IsDValue(); else return false; 
		}


		/* Access the raw data value */

		//! Get a reference to the data chunk (const to prevent setting!!)
		const DataChunk& GetData(void) const { return Data; }

		//! Build a data chunk with all this items data (including child data)
		DataChunkPtr PutData(PrimerPtr UsePrimer = NULL) const;


		/* Misc value getting methods */

		//! Access function for Parent
		MDObjectParent GetParent(void) const { return Parent; }

		//! Access function for ParentFile
		MXFFilePtr GetParentFile(void) const { return ParentFile; }

		//! Make a copy of this object
		MDObjectPtr MakeCopy(void) const;

		//! Has this object (including any child objects) been modified?
		bool IsModified(void) const;

		//! Get the location within the ultimate parent
		Position GetLocation(void) const;

		//! Get text that describes where this item came from
		std::string GetSource(void) const;

		//! Get text that describes exactly where this item came from
		std::string GetSourceLocation(void) const
		{
			return std::string("0x") + Int64toHexString(GetLocation(),8) + std::string(" in ") + GetSource();
		}

		//! Get pointer to Outer object
		ObjectInterface *GetOuter(void) { return Outer; }


		/* Interface IMDValueSet */
		/*************************/

		/* Set the value of this object */

		//! Set the value from a 32-bit signed integer
		void SetInt(Int32 Val) { SetModified(true); if (Value && Traits) Traits->SetInt(Value, Val); }

		//! Set the value from a 64-bit signed integer
		void SetInt64(Int64 Val) { SetModified(true); if (Value && Traits) Traits->SetInt64(Value, Val); }

		//! Set the value from a 32-bit unsigned integer
		void SetUInt(UInt32 Val) { SetModified(true); if (Value && Traits) Traits->SetUInt(Value, Val); }

		//! Set the value from a 64-bit unsigned integer
		void SetUInt64(UInt64 Val) { SetModified(true); if (Value && Traits) Traits->SetUInt64(Value, Val); }

		//! Set the value from a UTF-8 string
		void SetString(std::string Val)	{ SetModified(true); if (Value && Traits) Traits->SetString(Value, Val); }

		//! Set this object to its distinguished value
		/*! \return true if distinguished value set, else false */
		bool SetDValue(void);

		//! Set the default value for this object
		/*! \return true is a default value is set, else false */
		bool SetDefault(void)
		{
			if(!IsValue && Traits) return false;

			mxflib_assert(Type);

			if(Type->GetDefault().Size == 0) return false;

			ReadValue(Type->GetDefault());
			SetModified(true);

			return true;
		}


		/* Set the value of a child object by name */

		//! Set the value of named child from a 32-bit signed integer
		void SetInt(const char *ChildName, Int32 Val)
		{ 
			MDObjectPtr Ptr = Child(ChildName);
			if(!Ptr) Ptr = AddChild(ChildName);
			if (Ptr) Ptr->SetInt(Val); 
			else { error("Unable to locate or add child %s of MDObject %s\n", ChildName, FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the value of named child from a 64-bit signed integer
		void SetInt64(const char *ChildName, Int64 Val)
		{ 
			MDObjectPtr Ptr = Child(ChildName);
			if(!Ptr) Ptr = AddChild(ChildName);
			if (Ptr) Ptr->SetInt64(Val); 
			else { error("Unable to locate or add child %s of MDObject %s\n", ChildName, FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the value of named child from a 32-bit unsigned integer
		void SetUInt(const char *ChildName, UInt32 Val)
		{ 
			MDObjectPtr Ptr = Child(ChildName);
			if(!Ptr) Ptr = AddChild(ChildName);
			if (Ptr) Ptr->SetUInt(Val); 
			else { error("Unable to locate or add child %s of MDObject %s\n", ChildName, FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the value of named child from a 32-bit unsigned integer
		void SetUInt64(const char *ChildName, UInt64 Val)
		{ 
			MDObjectPtr Ptr = Child(ChildName);
			if(!Ptr) Ptr = AddChild(ChildName);
			if (Ptr) Ptr->SetUInt64(Val); 
			else { error("Unable to locate or add child %s of MDObject %s\n", ChildName, FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the value of named child from a UTF-8 string
		void SetString(const char *ChildName, std::string Val)
		{ 
			MDObjectPtr Ptr = Child(ChildName);
			if(!Ptr) Ptr = AddChild(ChildName);
			if(Ptr) Ptr->SetString(Val); 
			else { error("Unable to locate or add child %s of MDObject %s\n", ChildName, FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the named child to its distinguished value
		/*! \return true if distinguished value set, else false */
		bool SetDValue(const char *ChildName)
		{ 
			MDObjectPtr Ptr = Child(ChildName);
			if(!Ptr) Ptr = AddChild(ChildName);
			if (Ptr) return Ptr->SetDValue();
			error("Unable to locate or add child %s of MDObject %s\n", ChildName, FullName().c_str());
			mxflib_assert(0);
			return false;
		}

		//! Set the default value for the named child
		/*! \return true is a default value is set, else false */
		bool SetDefault(const char *ChildName)
		{ 
			MDObjectPtr Ptr = Child(ChildName);
			if(!Ptr) Ptr = AddChild(ChildName);
			if (Ptr) return Ptr->SetDefault(); 
			error("Unable to locate or add child %s of MDObject %s\n", ChildName, FullName().c_str());
			mxflib_assert(0);
			return false;
		};


		/* Set the value of a child object by UL */

		//! Set the value of UL identified child from a 32-bit signed integer
		void SetInt(const UL &Child, Int32 Val)
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if(!Ptr) Ptr = AddChild(Child);
			if (Ptr) Ptr->SetInt(Val);
			else { error("Unable to locate or add child %s of MDObject %s\n", Child.GetString().c_str(), FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the value of UL identified child from a 64-bit signed integer
		void SetInt64(const UL &Child, Int64 Val)
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if(!Ptr) Ptr = AddChild(Child);
			if (Ptr) Ptr->SetInt64(Val);
			else { error("Unable to locate or add child %s of MDObject %s\n", Child.GetString().c_str(), FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the value of UL identified child from a 32-bit unsigned integer
		void SetUInt(const UL &Child, UInt32 Val)
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if(!Ptr) Ptr = AddChild(Child);
			if (Ptr) Ptr->SetUInt(Val);
			else { error("Unable to locate or add child %s of MDObject %s\n", Child.GetString().c_str(), FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the value of UL identified child from a 32-bit unsigned integer
		void SetUInt64(const UL &Child, UInt64 Val)
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if(!Ptr) Ptr = AddChild(Child);
			if (Ptr) Ptr->SetUInt64(Val);
			else { error("Unable to locate or add child %s of MDObject %s\n", Child.GetString().c_str(), FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the value of UL identified child from a UTF-8 string
		void SetString(const UL &Child, std::string Val)
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if(!Ptr) Ptr = AddChild(Child);
			if (Ptr) Ptr->SetString(Val);
			else { error("Unable to locate or add child %s of MDObject %s\n", Child.GetString().c_str(), FullName().c_str()); mxflib_assert(0); }
		}

		//! Set the UL identified child to its distinguished value
		/*! \return true if distinguished value set, else false */
		bool SetDValue(const UL &Child)
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if(!Ptr) Ptr = AddChild(Child);
			if (Ptr) return Ptr->SetDValue();
			error("Unable to locate or add child %s of MDObject %s\n", Child.GetString().c_str(), FullName().c_str());
			mxflib_assert(0);
			return false;
		}

		//! Set the default value for the UL identified child
		/*! \return true is a default value is set, else false */
		bool SetDefault(const UL &Child)
		{ 
			MDObjectPtr Ptr = operator[](Child);
			if(!Ptr) Ptr = AddChild(Child);
			if (Ptr) return Ptr->SetDefault();
			error("Unable to locate or add child %s of MDObject %s\n", Child.GetString().c_str(), FullName().c_str());
			mxflib_assert(0);
			return false;
		}


		/* Misc value setting methods */

		//! Set data into the datachunk
		// DRAGONS: This is dangerous as it bypasses any traits!!
		void SetData(size_t MemSize, const UInt8 *Buffer)
		{ 
			Data.Resize(MemSize); 
			Data.Set(MemSize, Buffer); 
		};

		//! Inset a new child object - overloads the existing MDObjectList versions
		void insert(MDObjectPtr NewObject)
		{
			if(NewObject->TheUL)
				push_back(MDObjectULList::value_type(*(NewObject->TheUL), NewObject));
			else
				push_back(MDObjectULList::value_type(Null_UL, NewObject));

			NewObject->Parent = this;
		}

		//! Set the parent details when an object has been read from a file
		void SetParent(MXFFilePtr &File, Position Location, UInt32 NewKLSize)
		{
			IsConstructed = false;
			ParentOffset = Location;
			KLSize = NewKLSize;
			Parent = NULL;
			ParentFile = File;
		}

		//! Set the parent details when an object has been read from memory
		void SetParent(MDObjectPtr &Object, Position Location, UInt32 NewKLSize)
		{
			IsConstructed = false;
			ParentOffset = Location;
			KLSize = NewKLSize;
			Parent = Object;
			ParentFile = NULL;
		}

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		bool ChangeType(const UL &NewType);

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		bool ChangeType(ULPtr &NewType) { return ChangeType(*NewType); }

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		bool ChangeType(std::string NewType)
		{
			MDOTypePtr Ptr = MDOType::Find(NewType);

			if(!Ptr) return false;

			Type = Ptr;
			ObjectName = Type->Name();
			TheUL = Type->GetUL();
			if(TheUL) TheTag = MDOType::GetStaticPrimer()->Lookup(TheUL);
			else TheTag = 0;

			return true;
		}

		//! Set the GenerationUID of an object iff it has been modified
		/*! \return true if the GenerationUID has been set, otherwise false
		 *  \note If the object does not have a GenerationUID property false is returned!
		 */
		bool SetGenerationUID(UUIDPtr UID);

		//! Clear the modified flag on this object and any contained objects
		void ClearModified(void);

		//! Set pointer to Outer object
		void SetOuter(ObjectInterface *NewOuter) { Outer = NewOuter; }


		/* Static value setting methods */

	public:
		//! Set a translator function to translate unknown ULs to object names
		static void SetULTranslator(ULTranslator Trans) { UL2NameFunc = Trans; }

		//! Set the "attempt to parse dark metadata" flag
		static void SetParseDark(bool Value) { ParseDark = Value; }


		/* Interface IMDValueIO */
		/************************/

		//! Read the object's value from a data chunk
		size_t ReadValue(const DataChunk &Chunk, PrimerPtr UsePrimer = NULL) { return ReadValue(Chunk.Data, Chunk.Size, UsePrimer); }

		//! Read the object's value from a data chunk pointer
		/*! DRAGONS: This will clear the modified flag for this object - Use SetValue() if you want it set */
		size_t ReadValue(DataChunkPtr &Chunk, PrimerPtr UsePrimer = NULL) { return ReadValue(Chunk->Data, Chunk->Size, UsePrimer); }

		//! Read the object's value from a memory buffer
		/*! DRAGONS: This will clear the modified flag for this object - Use SetValue() if you want it set */
		size_t ReadValue(const UInt8 *Buffer, size_t Size, PrimerPtr UsePrimer = NULL);

		//! Write this object to a new memory buffer
		DataChunkPtr WriteObject(const MDObject *ParentObject, UInt32 BERSize = 0) const
		{
			DataChunkPtr Ret = new DataChunk;
			WriteObject(Ret, ParentObject, NULL, BERSize);
			return Ret;
		}

		//! Write this object to a new memory buffer
		DataChunkPtr WriteObject(const MDObject *ParentObject, PrimerPtr UsePrimer, UInt32 BERSize = 0) const
		{
			DataChunkPtr Ret = new DataChunk;
			WriteObject(Ret, ParentObject, UsePrimer, BERSize);
			return Ret;
		}

		//! Append this object to a memory buffer
		size_t WriteObject(DataChunkPtr &Buffer, const MDObject *ParentObject = NULL, UInt32 BERSize = 0) const
		{
			return WriteObject(Buffer, ParentObject, NULL, BERSize);
		}

		//! Append this object to a memory buffer
		size_t WriteObject(DataChunkPtr &Buffer, const MDObject *ParentObject, PrimerPtr UsePrimer, UInt32 BERSize = 0) const;

		//! Write this top level object to a new memory buffer
		/*! The object must be at the outer or top KLV level
		 *	\return The new buffer
		 */
		DataChunkPtr WriteObject(UInt32 BERSize = 0) const
		{
			DataChunkPtr Ret = new DataChunk;
			WriteObject(Ret, NULL, NULL, BERSize);
			return Ret;
		}


		//! Write this top level object to a new memory buffer
		/*! The object must be at the outer or top KLV level
		 *	\return The new buffer
		 */
		DataChunkPtr WriteObject(PrimerPtr UsePrimer, UInt32 BERSize = 0) const
		{
			DataChunkPtr Ret = new DataChunk;
			WriteObject(Ret, NULL, UsePrimer, BERSize);
			return Ret;
		}

		//! Append this top level object to a memory buffer
		/*! The object must be at the outer or top KLV level. The object is appended to the buffer
		 *	\return The number of bytes written
		 */
		size_t WriteObject(DataChunkPtr &Buffer, PrimerPtr UsePrimer, UInt32 BERSize = 0) const
		{
			return WriteObject(Buffer, NULL, UsePrimer, BERSize);
		}

		//! Append this top level object, and any strongly linked sub-objects, to a memory buffer
		size_t WriteLinkedObjects(DataChunkPtr &Buffer, PrimerPtr UsePrimer = NULL);


		/* Interface IMDValueRef */
		/************************/

		//! Access the target of a reference link
		MDObjectParent GetRef(void) const { return Link; };

		//! Access the target of a reference link child property
		MDObjectParent GetRef(std::string ChildType) const
		{
			MDObjectPtr Ptr = Child(ChildType);
			if(Ptr) return Ptr->GetRef();
			return NULL;
		}

		//! Access the target of a reference link child property
		MDObjectParent GetRef(const UL &ChildType) const
		{
			MDObjectPtr Ptr = Child(ChildType);
			if(Ptr) return Ptr->GetRef();
			return NULL;
		}

		//! Access the target of a reference link child property
		MDObjectParent GetRef(ULPtr &ChildType) const { return GetRef(*ChildType); }

		//! Make a link from this reference source to the specified target set
		bool MakeRef(MDObjectPtr &TargetSet, bool ForceLink = false) { return MakeRef(TargetSet, InstanceUID_UL, ForceLink); }

		//! Make a link from this reference source to the specified target set via the given target property
		bool MakeRef(MDObjectPtr &TargetSet, const UL &Target, bool ForceLink = false);

		//! Make a link from the given source child of this set to the specified target set, adding a new child if required
		bool MakeRef(const UL &Source, MDObjectPtr &TargetSet, bool ForceLink = false)
		{
			MDObjectPtr Ptr = Child(Source);
			if(!Ptr) Ptr = AddChild(Source);
			if(Ptr) return Ptr->MakeRef(TargetSet, InstanceUID_UL, ForceLink);
			return false;
		}

		//! Add a new source child to the specified property of this set and link it to the specified target set
		/*! This is used for adding new reference entries to batches or arrays in this set */
		bool AddRef(const UL &Source, MDObjectPtr &TargetSet, bool ForceLink = false)
		{
			MDObjectPtr Ptr = Child(Source);
			if(!Ptr) Ptr = AddChild(Source);
			if(Ptr) Ptr = Ptr->AddChild();
			if(Ptr) return Ptr->MakeRef(TargetSet, InstanceUID_UL, ForceLink);
			return false;
		}

	
	
	protected:
		//! Add a new child MDObject of the specified type
		/*! Same as AddChild(), but does not set "Modified"
		 */
		MDObjectPtr AddChildInternal(MDObjectPtr ChildObject, bool Replace = false);

		//! Write any items strongly linked from sub-items
		size_t WriteLinkedSubObjects(DataChunkPtr &Buffer, PrimerPtr UsePrimer);

	public:

		void SetUint(UInt32 Val) { SetUInt(Val); }
		void SetUint64(UInt64 Val) { SetUInt64(Val); }

		void SetValue(const UInt8 *Buffer, size_t BufferSize) { ReadValue(Buffer, BufferSize); SetModified(true); }
		void SetValue(const DataChunk &Source) { ReadValue(Source); SetModified(true); }
		void SetValue(MDObjectPtr Source) { ReadValue(Source->PutData()); SetModified(true); }
		UInt32 GetUint(UInt32 Default = 0) const { return GetUInt(Default); }
		UInt64 GetUint64(UInt64 Default = 0) const { return GetUInt64(Default); }


		//! Compare the exact value of two MDObjects
		bool operator==(const MDObject &RVal) const;

		//! Is this a value rather than a container?
		bool IsAValue(void) const { return IsValue; }

		//! Value copy
		MDObject &operator=(const MDObject &RHS)
		{
			// Do a bit-copy it the types are the same
			if(ValueType && RHS.ValueType && (ValueType->EffectiveType() == RHS.ValueType->EffectiveType()))
			{
				Data.Set(RHS.Data);
			}
			// ... otherwise copy by string value!
			else
			{
				SetString(RHS.GetString());
			}

			return *this;
		}

		//! Add or Remove children from an MDObject continer to make a fixed size
		/*! Probably only useful for resizing arrays.
		 */
		void Resize(UInt32 Count);

		//! Set a variable to be a certain size in bytes
		/*!	This function assumes that this is a viable thing to do!
		*  \return The size of the resized item
		*/
		size_t MakeSize(size_t NewSize);

		void SetUint(const char *ChildName, UInt32 Val) { SetUInt(ChildName, Val); }
		void SetUint64(const char *ChildName, UInt64 Val) { SetUInt64(ChildName, Val); }

	
		void SetValue(const char *ChildName, const DataChunk &Source) 
		{ 
			MDObjectPtr Ptr = Child(ChildName);
			if(!Ptr) Ptr = AddChild(ChildName);
			if (Ptr) { Ptr->ReadValue(Source); Ptr->SetModified(true); }
			else { error("Unable to locate or add child %s of MDObject %s\n", ChildName, FullName().c_str()); mxflib_assert(0); }
		};

		void SetValue(const char *ChildName, MDObjectPtr Source) 
		{ 
			MDObjectPtr Ptr = Child(ChildName);
			if(!Ptr) Ptr = AddChild(ChildName);
			if (Ptr) { Ptr->ReadValue(Source->PutData()); Ptr->SetModified(true); }
			else { error("Unable to locate or add child %s of MDObject %s\n", ChildName, FullName().c_str()); mxflib_assert(0); }
		};



		UInt32 GetUint(const char *ChildName, UInt32 Default = 0) { return GetUInt(ChildName, Default); }
		UInt64 GetUint64(const char *ChildName, UInt64 Default = 0)  { return GetUInt64(ChildName, Default); }


		void SetInt(ULPtr &ChildType, Int32 Val) { SetInt(*ChildType, Val); }
		void SetInt64(ULPtr &ChildType, Int64 Val) { SetInt64(*ChildType, Val); }
		void SetUInt(ULPtr &ChildType, UInt32 Val) { SetUInt(*ChildType, Val); }
		void SetUInt64(ULPtr &ChildType, UInt64 Val) { SetUInt64(*ChildType, Val); }
		void SetString(ULPtr &ChildType, std::string Val) { SetString(*ChildType, Val); }

		bool SetDValue(ULPtr &ChildType) { return SetDValue(*ChildType); }
		
		void SetValue(const UL &ChildType, const DataChunk &Source) 
		{ 
			MDObjectPtr Ptr = AddChild(ChildType); 
			if(Ptr) { Ptr->ReadValue(Source); Ptr->SetModified(true); }
		};
		void SetValue(ULPtr &ChildType, const DataChunk &Source) { SetValue(*ChildType, Source); }

		void SetValue(const UL &ChildType, MDObjectPtr Source) 
		{ 
			MDObjectPtr Ptr = AddChild(ChildType); 
			if(Ptr) { Ptr->ReadValue(Source->PutData()); Ptr->SetModified(true); }
		};
		void SetValue(ULPtr &ChildType, MDObjectPtr Source) { SetValue(*ChildType, Source); }
		
		Int32 GetInt(ULPtr &ChildType, Int32 Default = 0) { return GetInt(*ChildType, Default); }

		Int64 GetInt64(ULPtr &ChildType, Int64 Default = 0) { return GetInt64(*ChildType, Default); }

		UInt32 GetUInt(ULPtr &ChildType, UInt32 Default = 0) { return GetUInt(*ChildType, Default); }

		UInt64 GetUInt64(ULPtr &ChildType, UInt64 Default = 0) { return GetUInt64(*ChildType, Default); }

		std::string GetString(ULPtr &ChildType, std::string Default = "", OutputFormatEnum Format = -1) { return GetString(*ChildType, Default, Format); }
		std::string GetString(ULPtr &ChildType, OutputFormatEnum Format) { return GetString(*ChildType, Format); }

		bool IsDValue(ULPtr &ChildType) { return IsDValue(*ChildType); }

		void SetInt(MDOTypePtr ChildType, Int32 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetInt(Val); };
		void SetInt64(MDOTypePtr ChildType, Int64 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetInt64(Val); };
		void SetUInt(MDOTypePtr ChildType, UInt32 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetUInt(Val); };
		void SetUInt64(MDOTypePtr ChildType, UInt64 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetUInt64(Val); };
		void SetUint(MDOTypePtr ChildType, UInt32 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetUInt(Val); };
		void SetUint64(MDOTypePtr ChildType, UInt64 Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetUInt64(Val); };
		void SetString(MDOTypePtr ChildType, std::string Val) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) Ptr->SetString(Val); };
		bool SetDValue(MDOTypePtr ChildType) { MDObjectPtr Ptr = AddChild(ChildType); if (Ptr) return Ptr->SetDValue(); else return false; };
		void SetValue(MDOTypePtr ChildType, MDObjectPtr Source) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) { Ptr->ReadValue(Source->PutData()); Ptr->SetModified(true); } }
		void SetValue(MDOTypePtr ChildType, const DataChunk &Source) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) { Ptr->ReadValue(Source); Ptr->SetModified(true); } }
		Int32 GetInt(MDOTypePtr ChildType, Int32 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetInt(); else return Default; };
		Int64 GetInt64(MDOTypePtr ChildType, Int64 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetInt64(); else return Default; };
		UInt32 GetUInt(MDOTypePtr ChildType, UInt32 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUInt(); else return Default; };
		UInt64 GetUInt64(MDOTypePtr ChildType, UInt64 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUInt64(); else return Default; };
		UInt32 GetUint(MDOTypePtr ChildType, UInt32 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUInt(); else return Default; };
		UInt64 GetUint64(MDOTypePtr ChildType, UInt64 Default = 0) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetUInt64(); else return Default; };
		std::string GetString(MDOTypePtr ChildType, std::string Default = "", OutputFormatEnum Format = -1) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetString(Default, Format); else return Default; };
		std::string GetString(MDOTypePtr ChildType, OutputFormatEnum Format) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->GetString(Format); else return ""; };
		bool IsDValue(MDOTypePtr ChildType) { MDObjectPtr Ptr = operator[](ChildType); if (Ptr) return Ptr->IsDValue(); else return false; };


		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(std::string BaseType) const;

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(MDOTypePtr &BaseType) const;
		
		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(const UL &BaseType) const;

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(ULPtr &BaseType) const { return IsA(*BaseType); }

		//! Is the object from a baseline class, as defined in 377M or certain other specific standards (and so will not be added to the KXS metadictionary)
		bool IsBaseline(void) const
		{
			bool Ret = Type->IsBaseline();

			// If the type claims to be baseline, is this instance really baseline?
			if(Ret)
			{
				// If we are based on "Unknown" then we must be an extension of some kind
				if(Type->IsA(Unknown_UL)) Ret = false;

				// Otherwise, see if we are a modified version of a known type
				else if(TheUL && (!Type->GetTypeUL()->Matches(*TheUL))) Ret = false;
			}

			return Ret;
		}

		//! Link access functions
		/*! DEPRECATED: Use GetRef() instead */
		MDObjectParent GetLink(void) const { return GetRef(); };

		//! DEPRECATED: Make a link from this reference source to the specified target set
		/*! \note This method is deprecated - use MakeRef() instead */
		bool MakeLink(MDObjectPtr &TargetSet, bool ForceLink = false) { return MakeRef(TargetSet, ForceLink); }

		//! Record that a link exists (not the same as making a link - see MakeRef)
		void SetLink(MDObjectPtr NewLink) 
		{ 
			Link = NewLink;
			if(GetRefType() == ClassRefStrong) OwningLink = NewLink;
		}


	protected:
		//! Sets the modification state of this object
		/*! \note This function should be used rather than setting "Modified" as a 
		 *        future revision may "bubble" this up from sub-items to sets and packs
		 */
		void SetModified(bool State) { Modified = State; }
//		void SetModified(bool State) { printf("%s Modified set to %s\n", FullName().c_str(), State ? "true" : "false"); Modified = State; }

	public:
		static void SetULTypeMaker(ULTypeMaker Trans) { TypeMaker = Trans; }

	protected:
		// Some private helper functions
		static UInt32 ReadKey(DictKeyFormat Format, size_t Size, const UInt8 *Buffer, DataChunk& Key);
		static UInt32 ReadLength(DictLenFormat Format, size_t Size, const UInt8 *Buffer, Length& Length);
		UInt32 WriteKey(DataChunkPtr &Buffer, DictKeyFormat Format, PrimerPtr UsePrimer = NULL) const;
		static UInt32 WriteLength(DataChunkPtr &Buffer, Length Length, DictLenFormat Format, UInt32 Size = 0);
	};
}


namespace mxflib
{
	//! Interface for any class containing an MDObject that needs to behave like an MDObject
	/*! This class is required to prevent the need for polymorphism
	 *	which doesn't really work with smart pointers
	 */
	class ObjectInterface : public IMDTypeGetCommon,
							public IMDOTypeGet,
							public IMDTypeGet,
							public IMDKeyAccess,
							public IMDEffectiveType<MDType, MDTypePtr>,
							public IMDTraitsAccess,
							public IMDChildAccess,
							public IMDValueGet,
							public IMDValueSet,
							public IMDValueIO
	{
	protected:
		//! Protected constructor used to create from an existing MDObject
		ObjectInterface(MDObjectPtr BaseObject) : Object(BaseObject) { BaseObject->SetOuter(this); }

	public:
		MDObjectPtr Object;					//!< The MDObject for this item

	public:
		ObjectInterface() {};				//!< Build a basic ObjectInterface
		virtual ~ObjectInterface() {};		//!< Virtual destructor to allow polymorphism
		

		/* Interface IMDTypeGetCommon */
		/******************************/

		//! Get the name of this type
		const std::string &Name(void) const { return Object->Name(); }

		//! Get the full name of this type, including all parents
		std::string FullName(void) const { return Object->FullName(); }

		//! Report the detailed description for this type
		const std::string &GetDetail(void) const { return Object->GetDetail(); }


		/* Interface IMDOTypeGet */
		/*************************/

		//! Get the type of this object (returns self if this is a type, may return NULL for sub-items of a complex type)
		const MDOType *GetType(void) const { return Object->GetType(); }

		//! Get the type of this object (returns self if this is a type, may return NULL for sub-items of a complex type)
		const MDOTypePtr GetType(void) { return Object->GetType(); }

		//! Get the type of the value for this object (returns NULL if a group rather than an element)
		const MDTypePtr &GetValueType(void) const { return Object->GetValueType(); }

		//! Read-only access to KeyFormat
		const DictKeyFormat &GetKeyFormat(void) const { return Object->GetKeyFormat(); }

		//! Read-only access to LenFormat
		const DictLenFormat &GetLenFormat(void) const { return Object->GetLenFormat(); }

		//! Read-only access to the minLength value
		unsigned int GetMinLength(void) const { return Object->GetMinLength(); }

		//! Read-only access to the maxnLength value
		unsigned int GetMaxLength(void) const { return Object->GetMaxLength(); }

		//! Read-only access to default value
		const DataChunk &GetDefault(void) const { return Object->GetDefault(); }

		//! Read-only access to distinguished value
		const DataChunk &GetDValue(void) const { return Object->GetDValue(); }

		//! Access function for ContainerType
		MDContainerType GetContainerType(void) const { return Object->GetContainerType(); }

		//! Get the usage for this type
		ClassUsage GetUse(void) const { return Object->GetUse(); }

		//! Get the reference type
		TypeRef GetRefType(void) const { return Object->GetRefType(); }

		//! Get the reference target
		const MDOTypeParent &GetRefTarget(void) const { return Object->GetRefTarget(); }

		//! Accessor for Reference Target Name
		/*!< \note This must only be used during dictionary parsing or for error reporting,
		 *         not for actual reference linking where RefTarget must be used
		 */
		std::string GetRefTargetName(void) const { return Object->GetRefTargetName(); }


		/* Interface IMDTypeGet */
		/************************/

		//! Is this a "character" type
		bool IsCharacter(void) const { return Object->IsCharacter(); }

		//! Endian access function (get)
		bool GetEndian(void) const { return Object->GetEndian(); }

		//! Get the size of this type, in bytes if basic, or items if a multiple
		/*! DRAGONS: This gets the defined size for this type, not the size of the current value */
		int GetSize(void) const { return Object->GetSize(); }

		//! Get a const reference to the enum values
		const NamedValueList &GetEnumValues(void) const { return Object->GetEnumValues(); }

		//! Get the class of this type
		MDTypeClass GetClass(void) const { return Object->GetClass(); }

		//! ArrayClass access function (get)
		MDArrayClass GetArrayClass(void) const { return Object->GetArrayClass(); }


		/* Interface IMDKeyAccess */
		/**************************/

		//! Set the UL for this type or this specific object
		void SetUL(ULPtr &Val) { Object->SetUL(Val); }

		//! Read-only access to the current UL (same as GetTypeUL for types, but may differ for actual objects)
		const ULPtr &GetUL(void) const { return Object->GetUL(); }

		//! Read-only access to the type UL (the UL for the defined type, ignoring any UL set specifically for this object)
		const ULPtr &GetTypeUL(void) const { return Object->GetTypeUL(); }

		//! Set the tag for this type or this specific object
		void SetTag(Tag NewTag) { Object->SetTag(NewTag); }

		//! Get the tag for this type or object
		Tag GetTag(void) const { return Object->GetTag(); }


		/* Interface IMDEffectiveType */
		/******************************/

		//! Report the effective type of this type
		const MDType *EffectiveType(void) const { return Object->EffectiveType(); }

		//! Report the effective class of this type
		MDTypeClass EffectiveClass(void) const { return Object->EffectiveClass(); }

		//! Report the effective base type of this type
		MDTypePtr EffectiveBase(void) const { return Object->EffectiveBase(); }

		//! Report the effective reference type of this type
		TypeRef EffectiveRefType(void) const { return Object->EffectiveRefType(); }

		//! Report the effective reference target of this type
		MDOTypePtr EffectiveRefTarget(void) const { return Object->EffectiveRefTarget(); }

		//! Report the name of the effective reference target of this type
		/*! DRAGONS: To be used when loading dictionary only */
		std::string EffectiveRefTargetName(void) const { return Object->EffectiveRefTargetName(); };

		//! Report the effective size of this type
		/*! \return The size in bytes of a single instance of this type, or 0 if variable size
		 */
		UInt32 EffectiveSize(void) const { return Object->EffectiveSize(); }


		/* Interface IMDTraitsAccess */
		/*****************************/
	
		//! Set the traits for this type or object
		void SetTraits(MDTraitsPtr Tr) { Object->SetTraits(Tr); }

		//! Access the traits for this type or object
		const MDTraitsPtr &GetTraits(void) const { return Object->GetTraits(); }

		//! Does this value's trait take control of all sub-data and build values in the our own DataChunk?
		/*! Normally any contained sub-types (such as array items or compound members) hold their own data */
		bool HandlesSubdata(void) const { return Object->HandlesSubdata(); }

	public:
		/* Static traits methods */

		//! Add a mapping to be applied to all types of a given type name
		/*! \note This will act retrospectively
		 */
		static bool AddTraitsMapping(std::string TypeName, std::string TraitsName) { return MDType::AddTraitsMapping(TypeName, TraitsName); }

		//! Update an existing mapping and apply to any existing type of the given name
		static bool UpdateTraitsMapping(std::string TypeName, std::string TraitsName) { return MDType::UpdateTraitsMapping(TypeName, TraitsName); }

		//! Add a mapping to be applied to all types of a given type UL
		/*! \note This will act retrospectively
		 */
		static bool AddTraitsMapping(const UL &TypeUL, std::string TraitsName) { return MDType::AddTraitsMapping(TypeUL, TraitsName); }

		//! Update an existing mapping and apply to any existing type of the given UL
		static bool UpdateTraitsMapping(const UL &TypeUL, std::string TraitsName) { return MDType::UpdateTraitsMapping(TypeUL, TraitsName); }

		//! Lookup the traits for a specified type name
		/*! If no traits have been defined for the specified type the traits with the name given in DefaultTraitsName is used (if specified)
		 */
		static MDTraitsPtr LookupTraitsMapping(std::string TypeName, std::string DefaultTraitsName = "") { return MDType::LookupTraitsMapping(TypeName, DefaultTraitsName); }

		//! Lookup the traits for a specified type UL
		/*! If no traits have been defined for the specified type the traits with the UL given in DefaultTraitsName is used
		*/
		static MDTraitsPtr LookupTraitsMapping(const UL &TypeUL, const UL &DefaultTraitsUL) { return MDType::LookupTraitsMapping(TypeUL, DefaultTraitsUL); }

		//! Lookup the traits for a specified type name
		/*! If no traits have been defined for the specified type the traits with the UL given in DefaultTraitsName is used
		 */
		static MDTraitsPtr LookupTraitsMapping(std::string TypeName, const UL &DefaultTraitsUL) { return MDType::LookupTraitsMapping(TypeName, DefaultTraitsUL); }

		//! Lookup the traits for a specified type UL
		/*! If no traits have been defined for the specified type the traits with the name given in DefaultTraitsName is used (if specified)
		*/
		static MDTraitsPtr LookupTraitsMapping(const UL &TypeUL, std::string DefaultTraitsName = "") { return MDType::LookupTraitsMapping(TypeUL, DefaultTraitsName); }


		/* Interface IMDChildAccess */
		/****************************/

		//! Read-only access to ChildList
		const MDOTypeList &GetChildList(void) const { return Object->GetChildList(); }

		//! Locate a named child
		MDObjectPtr Child(const std::string Name) const { return Object->operator[](Name); }

		//! Locate a named child
		MDObjectPtr operator[](const std::string Name) const { return Object->operator[](Name); }

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MDObjectPtr Child(int Index) const { return Object->operator[](Index); }

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MDObjectPtr operator[](int Index) const { return Object->operator[](Index); }

		//! Locate a child by UL
		MDObjectPtr Child(ULPtr &ChildType) const { return Object->operator[](*ChildType); }

		//! Locate a child by UL
		MDObjectPtr operator[](ULPtr &ChildType) const { return Object->operator[](*ChildType); }

		//! Locate a child by UL
		MDObjectPtr Child(const UL &ChildType) const { return Object->operator[](ChildType); }

		//! Locate a child by UL
		MDObjectPtr operator[](const UL &ChildType) const { return Object->operator[](ChildType); }

		//! Locate a child by object type
		MDObjectPtr Child(const MDOTypePtr &ChildType) const { return Object->operator[](ChildType); }

		//! Locate a child by object type
		MDObjectPtr operator[](const MDOTypePtr &ChildType) const { return Object->operator[](ChildType); }

		//! Locate a child by value type
		MDObjectPtr Child(const MDTypePtr &ChildType) const { return Object->operator[](ChildType); }

		//! Locate a child by value type
		MDObjectPtr operator[](const MDTypePtr &ChildType) const { return Object->operator[](ChildType); }

		//! Add a new child MDObject of the specified type
		/*! ChildName is a symbol to be located in default MXFLib SymbolSpace
		 */
		MDObjectPtr AddChild(std::string ChildName, bool Replace) { return Object->AddChild(ChildName, MXFLibSymbols, Replace); }

		//! Add a new child MDObject of the specified type
		/*! ChildName is a symbol to be located in the given SymbolSpace - if no SymbolSpace is specifed the default MXFLib space is used 
		 */
		MDObjectPtr AddChild(std::string ChildName, SymbolSpacePtr &SymSpace = MXFLibSymbols, bool Replace = true) { return Object->AddChild(ChildName, SymSpace, Replace); }

		//! Add a new child MDObject of the specified type
		MDObjectPtr AddChild(MDOTypePtr ChildType, bool Replace = true) { return Object->AddChild(ChildType, Replace); }
		
		//! Add a new child MDObject to a vector
		/*! \note The type of the object added is automatic. 
		 *        If the vector is of multiple members the next type will be chosen by the number of members currently
		 *        in the array, so if there are 3 sub member types the 7th entry will be type 1 [ 7 = (2*3) + 1 ]
		 *
		 *  \note This version of AddChild will <b>not</b> replace duplicates, it always appends
		 */
		MDObjectPtr AddChild(void) { return Object->AddChild(); }

		//! Add a new child MDObject of the specified type
		MDObjectPtr AddChild(const UL &ChildType, bool Replace = true) { return Object->AddChild(ChildType, Replace); }
		
		//! Add a new child MDObject of the specified type
		MDObjectPtr AddChild(ULPtr &ChildType, bool Replace = true) { return Object->AddChild(*ChildType, Replace); }

		//! Add the given child object
		/*! \ret false if unable to add this child */
		bool AddChild(MDObjectPtr &ChildObject, bool Replace = false) { return Object->AddChild(ChildObject, Replace); }

//		//! Add the given child object at a specific numerical index
//		/*! \ret false if unable to add this child at the specified location (for example if not numerically indexed) */
//		bool AddChild(MDObjectPtr &Child, int Index);

		//! Remove the (first) child of the specified type
		void RemoveChild(std::string ChildName) { Object->RemoveChild(ChildName); }

		//! Remove the (first) child of the specified type
		void RemoveChild(MDOTypePtr &ChildType) { Object->RemoveChild(ChildType); }

		//! Remove the (first) child of the specified type
		void RemoveChild(ULPtr &ChildType) { Object->RemoveChild(ChildType); }

		//! Remove the specified child
		void RemoveChild(MDObjectPtr ChildObject) { Object->RemoveChild(ChildObject); }

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const std::string ChildName) const { return Object->ChildList(ChildName); }

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const UL &ChildType) const { return Object->ChildList(ChildType); }

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const ULPtr &ChildType) const { return Object->ChildList(ChildType); }

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const MDOTypePtr &ChildType) const { return Object->ChildList(ChildType); }

		//! Get a list of all child items of a specified type
		MDObjectListPtr ChildList(const MDTypePtr &ChildType) const { return Object->ChildList(ChildType); }


		/* Interface IMDValueGet */
		/*************************/

		/* Get the value of this object */

		//! Get the 32-bit signed integer version of value
		Int32 GetInt(Int32 Default = 0) const { return Object->GetInt(Default); }

		//! Get the 64-bit signed integer version of value
		Int64 GetInt64(Int64 Default = 0) const { return Object->GetInt64(Default); }

		//! Get the 32-bit unsigned integer version of value
		UInt32 GetUInt(UInt32 Default = 0) const { return Object->GetUInt(Default); }

		//! Get the 64-bit unsigned integer version of value
		UInt64 GetUInt64(UInt64 Default = 0) const { return Object->GetUInt64(Default); }

		//! Get the UTF-8 string version of value
		std::string GetString(std::string Default = "", OutputFormatEnum Format = -1) const { return Object->GetString(Default, Format); }

		//! Get the UTF-8 string version of value
		std::string GetString(OutputFormatEnum Format) const { return Object->GetString(Format); }

		//! Is this a Best Effort property that is set to its distinguished value?
		bool IsDValue(void) const { return Object->IsDValue(); }


		/* Get the value of a child object by name */

		//! Get the 32-bit signed integer version of value of named child
		Int32 GetInt(const char *ChildName, Int32 Default = 0) const { return Object->GetInt(ChildName, Default); }

		//! Get the 64-bit signed integer version of value of named child
		Int64 GetInt64(const char *ChildName, Int64 Default = 0) const { return Object->GetInt64(ChildName, Default); }

		//! Get the 32-bit unsigned integer version of value of named child
		UInt32 GetUInt(const char *ChildName, UInt32 Default = 0) const { return Object->GetUInt(ChildName, Default); }

		//! Get the 64-bit unsigned integer version of value of named child
		UInt64 GetUInt64(const char *ChildName, UInt64 Default = 0) const { return Object->GetUInt64(ChildName, Default); }

		//! Get the UTF-8 string version of value of named child
		std::string GetString(const char *ChildName, std::string Default = "", OutputFormatEnum Format = -1) const { return Object->GetString(ChildName, Default, Format); }

		//! Get the UTF-8 string version of value of named child
		std::string GetString(const char *ChildName, OutputFormatEnum Format) const { return Object->GetString(ChildName, Format); }

		//! Is the named child a Best Effort property that is set to its distinguished value?
		bool IsDValue(const char *ChildName) const { return Object->IsDValue(ChildName); }


		/* Get the value of a child object by UL */

		//! Get the 32-bit signed integer version of value of UL identified child
		Int32 GetInt(const UL &Child, Int32 Default = 0) const { return Object->GetInt(Child, Default); }

		//! Get the 64-bit signed integer version of value of UL identified child
		Int64 GetInt64(const UL &Child, Int64 Default = 0) const { return Object->GetInt64(Child, Default); }

		//! Get the 32-bit unsigned integer version of value of UL identified child
		UInt32 GetUInt(const UL &Child, UInt32 Default = 0) const { return Object->GetUInt(Child, Default); }

		//! Get the 64-bit unsigned integer version of value of UL identified child
		UInt64 GetUInt64(const UL &Child, UInt64 Default = 0) const { return Object->GetUInt64(Child, Default); }

		//! Get the UTF-8 string version of value of UL identified child
		std::string GetString(const UL &Child, std::string Default = "", OutputFormatEnum Format = -1) const { return Object->GetString(Child, Default, Format); }

		//! Get the UTF-8 string version of value of UL identified child
		std::string GetString(const UL &Child, OutputFormatEnum Format) const { return Object->GetString(Child, Format); }

		//! Is the UL identified child a Best Effort property that is set to its distinguished value?
		bool IsDValue(const UL &Child) const { return Object->IsDValue(Child); }


		/* Access the raw data value */

		//! Get a reference to the data chunk (const to prevent setting!!)
		const DataChunk& GetData(void) const { return Object->GetData(); }

		//! Build a data chunk with all this items data (including child data)
		DataChunkPtr PutData(PrimerPtr UsePrimer = NULL) const { return Object->PutData(); }


		/* Misc value getting methods */

		//! Access function for Parent
		MDObjectParent GetParent(void) const { return Object->GetParent(); }

		//! Access function for ParentFile
		MXFFilePtr GetParentFile(void) const { return Object->GetParentFile(); }

		//! Make a copy of this object
		MDObjectPtr MakeCopy(void) const 
		{ 
			// DRAGONS: Each derived class should provide thier own version which builds an outer object too
			warning("Copy made of higher-level object %s with ObjectInterface::MakeCopy() - new outer not built\n", FullName().c_str());
			return Object->MakeCopy(); 
		}

		//! Has this object (including any child objects) been modified?
		bool IsModified(void) const { return Object->IsModified(); }

		//! Get the location within the ultimate parent
		Position GetLocation(void) const { return Object->GetLocation(); }

		//! Get text that describes where this item came from
		std::string GetSource(void) const { return Object->GetSource(); }

		//! Get text that describes exactly where this item came from
		std::string GetSourceLocation(void) const { return Object->GetSourceLocation(); }

		//! Get pointer to Outer object
		ObjectInterface *GetOuter(void) { return this; }


		/* Interface IMDValueSet */
		/*************************/

		/* Set the value of this object */

		//! Set the value from a 32-bit signed integer
		void SetInt(Int32 Val) { Object->SetInt(Val); }

		//! Set the value from a 64-bit signed integer
		void SetInt64(Int64 Val) { Object->SetInt64(Val); }

		//! Set the value from a 32-bit unsigned integer
		void SetUInt(UInt32 Val) { Object->SetUInt(Val); }

		//! Set the value from a 64-bit unsigned integer
		void SetUInt64(UInt64 Val) { Object->SetUInt64(Val); }

		//! Set the value from a UTF-8 string
		void SetString(std::string Val)	 { Object->SetString(Val); }

		//! Set this object to its distinguished value
		/*! \return true if distinguished value set, else false */
		bool SetDValue(void) { return Object->SetDValue(); }

		//! Set the default value for this object
		/*! \return true is a default value is set, else false */
		bool SetDefault(void) { return Object->SetDefault(); }


		/* Set the value of a child object by name */

		//! Set the value of named child from a 32-bit signed integer
		void SetInt(const char *ChildName, Int32 Val) { Object->SetInt(ChildName, Val); }

		//! Set the value of named child from a 64-bit signed integer
		void SetInt64(const char *ChildName, Int64 Val) { Object->SetInt64(ChildName, Val); }

		//! Set the value of named child from a 32-bit unsigned integer
		void SetUInt(const char *ChildName, UInt32 Val) { Object->SetUInt(ChildName, Val); }

		//! Set the value of named child from a 32-bit unsigned integer
		void SetUInt64(const char *ChildName, UInt64 Val) { Object->SetUInt64(ChildName, Val); }

		//! Set the value of named child from a UTF-8 string
		void SetString(const char *ChildName, std::string Val) { Object->SetString(ChildName, Val); }

		//! Set the named child to its distinguished value
		/*! \return true if distinguished value set, else false */
		bool SetDValue(const char *ChildName) { return Object->SetDValue(ChildName); }

		//! Set the default value for the named child
		/*! \return true is a default value is set, else false */
		bool SetDefault(const char *ChildName) { return Object->SetDefault(ChildName); }


		/* Set the value of a child object by UL */

		//! Set the value of UL identified child from a 32-bit signed integer
		void SetInt(const UL &Child, Int32 Val) { Object->SetInt(Child, Val); }

		//! Set the value of UL identified child from a 64-bit signed integer
		void SetInt64(const UL &Child, Int64 Val) { Object->SetInt64(Child, Val); }

		//! Set the value of UL identified child from a 32-bit unsigned integer
		void SetUInt(const UL &Child, UInt32 Val) { Object->SetUInt(Child, Val); }

		//! Set the value of UL identified child from a 32-bit unsigned integer
		void SetUInt64(const UL &Child, UInt64 Val) { Object->SetUInt64(Child, Val); }

		//! Set the value of UL identified child from a UTF-8 string
		void SetString(const UL &Child, std::string Val) { Object->SetString(Child, Val); }

		//! Set the UL identified child to its distinguished value
		/*! \return true if distinguished value set, else false */
		bool SetDValue(const UL &Child) { return Object->SetDValue(Child); }

		//! Set the default value for the UL identified child
		/*! \return true is a default value is set, else false */
		bool SetDefault(const UL &Child) { return Object->SetDefault(Child); }


		/* Misc value setting methods */

		//! Set data into the datachunk
		// DRAGONS: This is dangerous as it bypasses any traits!!
		void SetData(size_t MemSize, const UInt8 *Buffer) { Object->SetData(MemSize, Buffer); }

		//! Inset a new child object - overloads the existing MDObjectList versions
		void insert(MDObjectPtr NewObject)  { Object->insert(NewObject); }

		//! Set the parent details when an object has been read from a file
		void SetParent(MXFFilePtr &File, Position Location, UInt32 NewKLSize) { Object->SetParent(File, Location, NewKLSize); }

		//! Set the parent details when an object has been read from memory
		void SetParent(MDObjectPtr &Object, Position Location, UInt32 NewKLSize) { Object->SetParent(Object, Location, NewKLSize); }

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		bool ChangeType(const UL &NewType) { return Object->ChangeType(NewType); }

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		bool ChangeType(ULPtr &NewType) { return Object->ChangeType(NewType); }

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		bool ChangeType(std::string NewType) { return Object->ChangeType(NewType); }

		//! Set the GenerationUID of an object iff it has been modified
		/*! \return true if the GenerationUID has been set, otherwise false
		 *  \note If the object does not have a GenerationUID property false is returned!
		 */
		bool SetGenerationUID(UUIDPtr UID) { return Object->SetGenerationUID(UID); }

		//! Clear the modified flag on this object and any contained objects
		void ClearModified(void) { Object->ClearModified(); }

		//! Set pointer to Outer object
		void SetOuter(ObjectInterface *NewOuter) { Object->SetOuter(NewOuter); }


		/* Interface IMDValueIO */
		/************************/

		//! Read the object's value from a data chunk
		size_t ReadValue(const DataChunk &Chunk, PrimerPtr UsePrimer = NULL) { return Object->ReadValue(Chunk.Data, Chunk.Size, UsePrimer); }

		//! Read the object's value from a data chunk pointer
		size_t ReadValue(DataChunkPtr &Chunk, PrimerPtr UsePrimer = NULL) { return Object->ReadValue(Chunk->Data, Chunk->Size, UsePrimer); }

		//! Read the object's value from a memory buffer
		size_t ReadValue(const UInt8 *Buffer, size_t Size, PrimerPtr UsePrimer = NULL) { return Object->ReadValue(Buffer, Size, UsePrimer); }

		//! Write this object to a new memory buffer
		DataChunkPtr WriteObject(const MDObject *ParentObject, UInt32 BERSize = 0) const { return Object->WriteObject(ParentObject, BERSize); }

		//! Write this object to a new memory buffer
		DataChunkPtr WriteObject(const MDObject *ParentObject, PrimerPtr UsePrimer, UInt32 BERSize = 0) const 
		{ 
			return Object->WriteObject(ParentObject, UsePrimer, BERSize); 
		}

		//! Append this object to a memory buffer
		size_t WriteObject(DataChunkPtr &Buffer, const MDObject *ParentObject = NULL, UInt32 BERSize = 0) const
		{
			return Object->WriteObject(Buffer, ParentObject, NULL, BERSize);
		}

		//! Append this object to a memory buffer
		size_t WriteObject(DataChunkPtr &Buffer, const MDObject *ParentObject, PrimerPtr UsePrimer, UInt32 BERSize = 0) const
		{
			return Object->WriteObject(Buffer, ParentObject, UsePrimer, BERSize);
		}

		//! Write this top level object to a new memory buffer
		/*! The object must be at the outer or top KLV level
		 *	\return The new buffer
		 */
		DataChunkPtr WriteObject(UInt32 BERSize = 0) const
		{
			return Object->WriteObject(BERSize);
		}


		//! Write this top level object to a new memory buffer
		/*! The object must be at the outer or top KLV level
		 *	\return The new buffer
		 */
		DataChunkPtr WriteObject(PrimerPtr UsePrimer, UInt32 BERSize = 0) const
		{
			return Object->WriteObject(UsePrimer, BERSize);
		}

		//! Append this top level object to a memory buffer
		/*! The object must be at the outer or top KLV level. The object is appended to the buffer
		 *	\return The number of bytes written
		 */
		size_t WriteObject(DataChunkPtr &Buffer, PrimerPtr UsePrimer, UInt32 BERSize = 0) const
		{
			return Object->WriteObject(Buffer, NULL, UsePrimer, BERSize);
		}

		//! Append this top level object, and any strongly linked sub-objects, to a memory buffer
		size_t WriteLinkedObjects(DataChunkPtr &Buffer, PrimerPtr UsePrimer = NULL)
		{
			return Object->WriteLinkedObjects(Buffer, UsePrimer);
		}



		// ** MDObject Interface **

		bool AddChild(MDObjectPtr ChildObject, bool Replace = false) { return Object->AddChild(ChildObject, Replace); };

		void RemoveChild(MDOTypePtr ChildType) { Object->RemoveChild(ChildType); };

		void SetUint(const char *ChildName, UInt32 Val) { Object->SetUInt(ChildName, Val); };
		void SetUint64(const char *ChildName, UInt64 Val) { Object->SetUInt64(ChildName, Val); };
		void SetValue(const char *ChildName, const DataChunk &Source) { Object->SetValue(ChildName, Source); }
		void SetValue(const char *ChildName, MDObjectPtr Source) { Object->SetValue(ChildName, Source); }
		Int32 GetInt(const char *ChildName, Int32 Default = 0) { return Object->GetInt(ChildName, Default); };
		Int64 GetInt64(const char *ChildName, Int64 Default = 0) { return Object->GetInt64(ChildName, Default); };
		UInt32 GetUInt(const char *ChildName, UInt32 Default = 0) { return Object->GetUInt(ChildName, Default); };
		UInt64 GetUInt64(const char *ChildName, UInt64 Default = 0) { return Object->GetUInt64(ChildName, Default); };
		UInt32 GetUint(const char *ChildName, UInt32 Default = 0) { return Object->GetUInt(ChildName, Default); };
		UInt64 GetUint64(const char *ChildName, UInt64 Default = 0) { return Object->GetUInt64(ChildName, Default); };
		std::string GetString(const char *ChildName, std::string Default = "", OutputFormatEnum Format = -1) { return Object->GetString(ChildName, Default, Format); };
		std::string GetString(const char *ChildName, OutputFormatEnum Format) { return Object->GetString(ChildName, Format); };
		bool IsDValue(const char *ChildName) { return Object->IsDValue(ChildName); };
		
		void SetInt(ULPtr &ChildType, Int32 Val) { Object->SetInt(*ChildType, Val); };
		void SetInt64(ULPtr &ChildType, Int64 Val) { Object->SetInt64(*ChildType, Val); };
		void SetUInt(ULPtr &ChildType, UInt32 Val) { Object->SetUInt(*ChildType, Val); };
		void SetUInt64(ULPtr &ChildType, UInt64 Val) { Object->SetUInt64(*ChildType, Val); };
		void SetString(ULPtr &ChildType, std::string Val) { Object->SetString(*ChildType, Val); };
		bool SetDValue(ULPtr &ChildType) { return Object->SetDValue(*ChildType); };
		void SetValue(const UL &ChildType, const DataChunk &Source) { Object->SetValue(ChildType, Source); }
		void SetValue(ULPtr &ChildType, const DataChunk &Source) { Object->SetValue(*ChildType, Source); }
		void SetValue(const UL &ChildType, MDObjectPtr Source) { Object->SetValue(ChildType, Source); }
		void SetValue(ULPtr &ChildType, MDObjectPtr Source) { Object->SetValue(*ChildType, Source); }
		Int32 GetInt(const UL &ChildType, Int32 Default = 0) { return Object->GetInt(ChildType, Default); };
		Int32 GetInt(ULPtr &ChildType, Int32 Default = 0) { return Object->GetInt(*ChildType, Default); };
		Int64 GetInt64(const UL &ChildType, Int64 Default = 0) { return Object->GetInt64(ChildType, Default); };
		Int64 GetInt64(ULPtr &ChildType, Int64 Default = 0) { return Object->GetInt64(*ChildType, Default); };
		UInt32 GetUInt(const UL &ChildType, UInt32 Default = 0) { return Object->GetUInt(ChildType, Default); };
		UInt32 GetUInt(ULPtr &ChildType, UInt32 Default = 0) { return Object->GetUInt(*ChildType, Default); };
		UInt64 GetUInt64(const UL &ChildType, UInt64 Default = 0) { return Object->GetUInt64(ChildType, Default); };
		UInt64 GetUInt64(ULPtr &ChildType, UInt64 Default = 0) { return Object->GetUInt64(*ChildType, Default); };
		std::string GetString(const UL &ChildType, std::string Default = "", OutputFormatEnum Format = -1) { return Object->GetString(ChildType, Default, Format); };
		std::string GetString(const UL &ChildType, OutputFormatEnum Format) { return Object->GetString(ChildType, Format); };
		std::string GetString(ULPtr &ChildType, std::string Default = "", OutputFormatEnum Format = -1) { return Object->GetString(*ChildType, Default, Format); };
		std::string GetString(ULPtr &ChildType, OutputFormatEnum Format) { return Object->GetString(*ChildType, Format); };
		bool IsDValue(const UL &ChildType) { return Object->IsDValue(ChildType); };
		bool IsDValue(ULPtr &ChildType) { return Object->IsDValue(*ChildType); };

		void SetInt(MDOTypePtr ChildType, Int32 Val) { Object->SetInt(ChildType, Val); };
		void SetInt64(MDOTypePtr ChildType, Int64 Val) { Object->SetInt64(ChildType, Val); };
		void SetUInt(MDOTypePtr ChildType, UInt32 Val) { Object->SetUInt(ChildType, Val); };
		void SetUInt64(MDOTypePtr ChildType, UInt64 Val) { Object->SetUInt64(ChildType, Val); };
		void SetUint(MDOTypePtr ChildType, UInt32 Val) { Object->SetUInt(ChildType, Val); };
		void SetUint64(MDOTypePtr ChildType, UInt64 Val) { Object->SetUInt64(ChildType, Val); };
		void SetString(MDOTypePtr ChildType, std::string Val) { Object->SetString(ChildType, Val); };
		bool SetDValue(MDOTypePtr ChildType) { return Object->SetDValue(ChildType); };
		void SetValue(MDOTypePtr ChildType, const DataChunk &Source) { Object->SetValue(ChildType, Source); }
		void SetValue(MDOTypePtr ChildType, MDObjectPtr Source) { Object->SetValue(ChildType, Source); }
		Int32 GetInt(MDOTypePtr ChildType, Int32 Default = 0) { return Object->GetInt(ChildType, Default); };
		Int64 GetInt64(MDOTypePtr ChildType, Int64 Default = 0) { return Object->GetInt64(ChildType, Default); };
		UInt32 GetUInt(MDOTypePtr ChildType, UInt32 Default = 0) { return Object->GetUInt(ChildType, Default); };
		UInt64 GetUInt64(MDOTypePtr ChildType, UInt64 Default = 0) { return Object->GetUInt64(ChildType, Default); };
		UInt32 GetUint(MDOTypePtr ChildType, UInt32 Default = 0) { return Object->GetUInt(ChildType, Default); };
		UInt64 GetUint64(MDOTypePtr ChildType, UInt64 Default = 0) { return Object->GetUInt64(ChildType, Default); };
		std::string GetString(MDOTypePtr ChildType, std::string Default = "", OutputFormatEnum Format = -1) { return Object->GetString(ChildType, Default, Format); };
		std::string GetString(MDOTypePtr ChildType, OutputFormatEnum Format) { return Object->GetString(ChildType, Format); };
		bool IsDValue(MDOTypePtr ChildType) { return Object->IsDValue(ChildType); };

		//! Make a reference link - DEPRECATED
		/*! DEPRECATED: Use MakeRef() */
		void SetLink(MDObjectPtr NewLink) { Object->SetLink(NewLink); };

		//! Get Reference Link - DEPRECATED
		/*! DEPRECATED: Use GetRef() */
		const MDObjectParent GetLink(void) const { return Object->GetLink(); };

		//! Access the target of a reference link
		MDObjectParent GetRef(void) const { return Object->GetRef(); };

		//! Access the target of a reference link child property
		MDObjectParent GetRef(std::string ChildType) const { return Object->GetRef(ChildType); }

		//! Access the target of a reference link child property
		MDObjectParent GetRef(const UL &ChildType) const { return Object->GetRef(ChildType); }

		//! Make a link from this reference source to the specified target set
		bool MakeRef(MDObjectPtr &TargetSet, bool ForceLink = false) { return MakeRef(TargetSet, InstanceUID_UL, ForceLink); }

		//! Make a link from this reference source to the specified target set via the given target property
		bool MakeRef(MDObjectPtr &TargetSet, const UL &Target, bool ForceLink = false) { return Object->MakeRef(TargetSet, Target, ForceLink); }

		//! Make a link from the given source child of this set to the specified target set, adding a new child if required
		bool MakeRef(const UL &Source, MDObjectPtr &TargetSet, bool ForceLink = false) { return Object->MakeRef(Source, TargetSet, ForceLink); }

		//! Add a new source child to the specified property of this set and link it to the specified target set
		/*! This is used for adding new reference entries to batches or arrays in this set */
		bool AddRef(const UL &Source, MDObjectPtr &TargetSet, bool ForceLink = false) { return Object->AddRef(Source, TargetSet, ForceLink); }

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(std::string BaseType) { return Object->IsA(BaseType); }

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(MDOTypePtr BaseType) { return Object->IsA(BaseType); }

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(const UL &BaseType) { return Object->IsA(BaseType); }

		//! Determine if this object is derived from a specified type (directly or indirectly)
		bool IsA(ULPtr &BaseType) { return Object->IsA(*BaseType); }
	};
}


/* Compatibility for old MDValue class */
namespace mxflib
{
	//! DEPRECATED: Compatibility for old MDValue class
	typedef MDObject MDValue;

	//! DEPRECATED: Compatibility for smart pointer to old MDValue objects
	typedef MDObjectPtr MDValuePtr;

	//! DEPRECATED: Compatibility for list of smart pointers to old MDValue objects
	typedef MDObjectList MDValueList;
}


// These simple inlines need to be defined after MDObject
namespace mxflib
{
inline MDObjectPtr MDObjectPtr::operator[](const char *ChildName) const { return GetPtr()->operator[](ChildName); }
inline MDObjectPtr MDObjectPtr::operator[](const MDOTypePtr &ChildType) const { return GetPtr()->operator[](ChildType); }
inline MDObjectPtr MDObjectPtr::operator[](const MDTypePtr &ChildType) const { return GetPtr()->operator[](ChildType); }
inline MDObjectPtr MDObjectPtr::operator[](const UL &ChildType) const { return GetPtr()->operator[](ChildType); }
inline MDObjectPtr MDObjectPtr::operator[](const ULPtr &ChildType) const { return GetPtr()->operator[](ChildType); }
inline MDObjectPtr MDObjectPtr::operator[](int Index) const { return GetPtr()->operator[](Index); }
inline MDObjectPtr MDObjectParent::operator[](const char *ChildName) const { return GetPtr()->operator[](ChildName); }
inline MDObjectPtr MDObjectParent::operator[](const MDOTypePtr &ChildType) const { return GetPtr()->operator[](ChildType); }
inline MDObjectPtr MDObjectParent::operator[](const MDTypePtr &ChildType) const { return GetPtr()->operator[](ChildType); }
inline MDObjectPtr MDObjectParent::operator[](const UL &ChildType) const { return GetPtr()->operator[](ChildType); }
inline MDObjectPtr MDObjectParent::operator[](const ULPtr &ChildType) const { return GetPtr()->operator[](ChildType); }
inline MDObjectPtr MDObjectParent::operator[](int Index) const { return GetPtr()->operator[](Index); }
}

#endif // MXFLIB__MDOBJECT_H

