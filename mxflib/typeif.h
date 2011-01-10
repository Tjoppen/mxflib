/*! \file	typeif.h
 *	\brief	Definition of interfaces for MDType, MDOType and MDObject
 *
 *	\version $Id: typeif.h,v 1.1 2011/01/10 10:45:50 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2008, Matt Beard
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
#ifndef MXFLIB__TYPEIF_H
#define MXFLIB__TYPEIF_H

//#define MXFLIB_TYPEIF_VIRTUAL virtual
//#define MXFLIB_TYPEIF_PURE =0
#define MXFLIB_TYPEIF_VIRTUAL
#define MXFLIB_TYPEIF_PURE

// STL Includes
#include <string>
#include <list>
#include <map>

namespace mxflib
{
	//! List of strings
	typedef std::list<std::string> StringList;

	// Forward declare
	class MDType;

	//! Smart Pointer to an MDType
	typedef SmartPtr<MDType> MDTypePtr;

	//! List of Smart Pointers to MDTypes
	typedef std::list<MDTypePtr> MDTypeList;


	// Forward declare
	class MDOType;

	//! Smart Pointer to an MDOType
	typedef SmartPtr<MDOType> MDOTypePtr;

	//! Parent pointer to an MDOType
	typedef ParentPtr<MDOType> MDOTypeParent;

	//! List of Smart Pointers to MDOTypes
	typedef std::list<MDOTypePtr> MDOTypeList;


	// Forward declare
	class MDObject;
	class MDObjectPtr;
	class MDObjectParent;
	class MDObjectList;
	typedef SmartPtr<MDObjectList> MDObjectListPtr;

	// Forward declare
	class ObjectInterface;
}

namespace mxflib
{
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
		DICT_KEY_AUTO = 3,
		DICT_KEY_4_BYTE = 4,
		DICT_KEY_GLOBAL = 5,
		DICT_KEY_UNDEFINED = 6
	} DictKeyFormat;


	/*
	** Enumeration type for length formats
	*/
	typedef enum
	{
		DICT_LEN_NONE = 0,
		DICT_LEN_1_BYTE = 1,
		DICT_LEN_2_BYTE = 2,
		DICT_LEN_BER = 3,
		DICT_LEN_4_BYTE = 4,
		DICT_LEN_UNDEFINED = 5
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
   so where dictionary order is important (such as packs) iterate
   through the ChildList property.

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
	//! Interface for getting type info for MDOTypes and MDTypes
	class IMDTypeGetCommon
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDTypeGetCommon() {};

		//! Get the name of this type
		MXFLIB_TYPEIF_VIRTUAL const std::string &Name(void) const MXFLIB_TYPEIF_PURE;

		//! Get the full name of this type, including all parents
		MXFLIB_TYPEIF_VIRTUAL std::string FullName(void) const MXFLIB_TYPEIF_PURE;

		//! Report the detailed description for this type
		MXFLIB_TYPEIF_VIRTUAL const std::string &GetDetail(void) const MXFLIB_TYPEIF_PURE;
	};

	//! Interface for getting type info for MDOTypes only
	class IMDOTypeGet
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDOTypeGet() {};

		//! Get the type of this object (returns self if this is a type, may return NULL for sub-items of a complex type)
		MXFLIB_TYPEIF_VIRTUAL const MDOType *GetType(void) const MXFLIB_TYPEIF_PURE;

		//! Get the type of this object (returns self if this is a type, may return NULL for sub-items of a complex type)
		MXFLIB_TYPEIF_VIRTUAL const MDOTypePtr GetType(void) MXFLIB_TYPEIF_PURE;

		//! Get the type of the value for this object (returns NULL if a group rather than an element)
		MXFLIB_TYPEIF_VIRTUAL const MDTypePtr &GetValueType(void) const MXFLIB_TYPEIF_PURE;

		//! Read-only access to KeyFormat
		MXFLIB_TYPEIF_VIRTUAL const DictKeyFormat &GetKeyFormat(void) const MXFLIB_TYPEIF_PURE;

		//! Read-only access to LenFormat
		MXFLIB_TYPEIF_VIRTUAL const DictLenFormat &GetLenFormat(void) const MXFLIB_TYPEIF_PURE;

		//! Read-only access to the minLength value
		MXFLIB_TYPEIF_VIRTUAL unsigned int GetMinLength(void) const MXFLIB_TYPEIF_PURE;

		//! Read-only access to the maxnLength value
		MXFLIB_TYPEIF_VIRTUAL unsigned int GetMaxLength(void) const MXFLIB_TYPEIF_PURE;

		//! Read-only access to default value
		MXFLIB_TYPEIF_VIRTUAL const DataChunk &GetDefault(void) const MXFLIB_TYPEIF_PURE;

		//! Read-only access to distinguished value
		MXFLIB_TYPEIF_VIRTUAL const DataChunk &GetDValue(void) const MXFLIB_TYPEIF_PURE;

		//! Access function for ContainerType
		MXFLIB_TYPEIF_VIRTUAL MDContainerType GetContainerType(void) const MXFLIB_TYPEIF_PURE;

		//! Get the usage for this type
		MXFLIB_TYPEIF_VIRTUAL ClassUsage GetUse(void) const MXFLIB_TYPEIF_PURE;

		//! Get the reference type
		MXFLIB_TYPEIF_VIRTUAL TypeRef GetRefType(void) const MXFLIB_TYPEIF_PURE;

		//! Get the reference target
		MXFLIB_TYPEIF_VIRTUAL const MDOTypeParent &GetRefTarget(void) const MXFLIB_TYPEIF_PURE;

		//! Accessor for Reference Target Name
		/*!< \note This must only be used during dictionary parsing or for error reporting,
		 *         not for actual reference linking where RefTarget must be used
		 */
		MXFLIB_TYPEIF_VIRTUAL std::string GetRefTargetName(void) const MXFLIB_TYPEIF_PURE;
	};

	//! Interface for finding MDOTypes
	class IMDOTypeFind
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDOTypeFind() {};

	public:
		/* Static MDOType finding methods */

		//! Find a type in the default symbol space, optionally searching all others
		static MDOTypePtr Find(std::string BaseType, bool SearchAll = false);
		
		//! Find a type in a specified symbol space, optionally searching all others
		static MDOTypePtr Find(std::string BaseType, SymbolSpacePtr &SymSpace, bool SearchAll = false);

		//! Find a type with a given UL
		static MDOTypePtr Find(const UL& BaseUL);

		//! Find a type via a tag
		static MDOTypePtr Find(Tag BaseTag, PrimerPtr BasePrimer);
	};


	//! Interface for getting type info for MDTypes only
	class IMDTypeGet
	{
	public:
		//! Name and value pair for enums
		typedef std::pair<std::string, MDObjectPtr> NamedValue;

		//! List of name and value pairs for enums
		typedef std::list<NamedValue> NamedValueList;

	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDTypeGet() {};

		//! Is this a "character" type
		MXFLIB_TYPEIF_VIRTUAL bool IsCharacter(void) const MXFLIB_TYPEIF_PURE;

		//! Endian access function (get)
		MXFLIB_TYPEIF_VIRTUAL bool GetEndian(void) const MXFLIB_TYPEIF_PURE;

		//! Get the size of this type, in bytes if basic, or items if a multiple
		/*! DRAGONS: This gets the defined size for this type, not the size of the current value */
		MXFLIB_TYPEIF_VIRTUAL int GetSize(void) const MXFLIB_TYPEIF_PURE;

		//! Get a const reference to the enum values
		MXFLIB_TYPEIF_VIRTUAL const NamedValueList &GetEnumValues(void) const MXFLIB_TYPEIF_PURE;

		//! Get the class of this type
		MXFLIB_TYPEIF_VIRTUAL MDTypeClass GetClass(void) const MXFLIB_TYPEIF_PURE;

		//! ArrayClass access function (get)
		MXFLIB_TYPEIF_VIRTUAL MDArrayClass GetArrayClass(void) const MXFLIB_TYPEIF_PURE;

		//! Get the reference type
		MXFLIB_TYPEIF_VIRTUAL TypeRef GetRefType(void) const MXFLIB_TYPEIF_PURE;

		//! Get the reference target
		MXFLIB_TYPEIF_VIRTUAL const MDOTypePtr &GetRefTarget(void) const MXFLIB_TYPEIF_PURE;

		//! Get the reference target
		MXFLIB_TYPEIF_VIRTUAL std::string GetRefTargetName(void) const MXFLIB_TYPEIF_PURE;
	};

	//! Interface for finding MDTypes
	class IMDTypeFind
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDTypeFind() {};

	public:
		/* Static MDType finding methods */

		//! Find a type in the default symbol space, optionally searching all others
		static MDTypePtr Find(std::string BaseType, bool SearchAll = false);

		//! Find a type in a specified symbol space, optionally searching all others
		static MDTypePtr Find(std::string BaseType, SymbolSpacePtr &SymSpace, bool SearchAll = false);

		//! Find a type with a given UL
		static MDTypePtr Find(const UL& BaseUL);

		//! Find a type by ULPtr
		static MDTypePtr Find(ULPtr &BaseUL);
	};


	//! Interface for setting type info for MDOTypes only
	class IMDOTypeSet
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDOTypeSet() {};

		//! Insert a new child type
		/*! DRAGONS: This overlaods the insert() methods of the base map */
//		std::pair<MDOType::iterator, bool> insert(MDOTypePtr NewType) 
	};


	//! Interface for setting type info for MDTypes only
	class IMDTypeSet
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDTypeSet() {};

		//! Set "character" type flag
		MXFLIB_TYPEIF_VIRTUAL void SetCharacter(bool Val) MXFLIB_TYPEIF_PURE;

		//! Endian access function (set)
		MXFLIB_TYPEIF_VIRTUAL void SetEndian(bool Val) MXFLIB_TYPEIF_PURE;

		//! ArrayClass access function (set)
		MXFLIB_TYPEIF_VIRTUAL void SetArrayClass(MDArrayClass Val) MXFLIB_TYPEIF_PURE;

		//! Set the reference type
		MXFLIB_TYPEIF_VIRTUAL void SetRefType(TypeRef Val) MXFLIB_TYPEIF_PURE;

		//! Set the reference target
		MXFLIB_TYPEIF_VIRTUAL void SetRefTarget(std::string Val) MXFLIB_TYPEIF_PURE;

		//! Insert a new child type
		/*! DRAGONS: This overlaods the insert() methods of the base map */
//		std::pair<iterator, bool> insert(MDTypePtr NewType) 
	};


	//! Interface for getting or setting key info for MDObjects and MDTypes
	class IMDKeyAccess
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDKeyAccess() {};

		//! Set the UL for this type or this specific object
		MXFLIB_TYPEIF_VIRTUAL void SetUL(ULPtr &Val) MXFLIB_TYPEIF_PURE;

		//! Read-only access to the current UL (same as GetTypeUL for types, but may differ for actual objects)
		MXFLIB_TYPEIF_VIRTUAL const ULPtr &GetUL(void) const MXFLIB_TYPEIF_PURE;

		//! Read-only access to the type UL (the UL for the defined type, ignoring any UL set specifically for this object)
		MXFLIB_TYPEIF_VIRTUAL const ULPtr &GetTypeUL(void) const MXFLIB_TYPEIF_PURE;

		//! Set the tag for this type or this specific object
		MXFLIB_TYPEIF_VIRTUAL void SetTag(Tag NewTag) MXFLIB_TYPEIF_PURE;

		//! Get the tag for this type or object
		MXFLIB_TYPEIF_VIRTUAL Tag GetTag(void) const MXFLIB_TYPEIF_PURE;
	};


	//! Interface for getting effective type info
	template<class Type, class TypePtr> class IMDEffectiveType
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDEffectiveType() {};

		//! Report the effective type of this type
		MXFLIB_TYPEIF_VIRTUAL const Type *EffectiveType(void) const MXFLIB_TYPEIF_PURE;

		//! Report the effective class of this type
		MXFLIB_TYPEIF_VIRTUAL MDTypeClass EffectiveClass(void) const MXFLIB_TYPEIF_PURE;

		//! Report the effective base type of this type
		MXFLIB_TYPEIF_VIRTUAL TypePtr EffectiveBase(void) const MXFLIB_TYPEIF_PURE;

		//! Report the effective reference type of this type
		MXFLIB_TYPEIF_VIRTUAL TypeRef EffectiveRefType(void) const MXFLIB_TYPEIF_PURE;

		//! Report the effective reference target of this type
		MXFLIB_TYPEIF_VIRTUAL MDOTypePtr EffectiveRefTarget(void) const MXFLIB_TYPEIF_PURE;

		//! Report the name of the effective reference target of this type
		/*! DRAGONS: To be used when loading dictionary only */
		MXFLIB_TYPEIF_VIRTUAL std::string EffectiveRefTargetName(void) const MXFLIB_TYPEIF_PURE;

		//! Report the effective size of this type
		/*! \return The size in bytes of a single instance of this type, or 0 if variable size
		 */
		MXFLIB_TYPEIF_VIRTUAL UInt32 EffectiveSize(void) const MXFLIB_TYPEIF_PURE;
	};


	//! Interface for accessing trait details
	class IMDTraitsAccess
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDTraitsAccess() {};
	
		//! Set the traits for this type or object
		MXFLIB_TYPEIF_VIRTUAL void SetTraits(MDTraitsPtr Tr) MXFLIB_TYPEIF_PURE;

		//! Access the traits for this type or object
		MXFLIB_TYPEIF_VIRTUAL const MDTraitsPtr &GetTraits(void) const MXFLIB_TYPEIF_PURE;

		//! Does this value's trait take control of all sub-data and build values in the our own DataChunk?
		/*! Normally any contained sub-types (such as array items or compound members) hold their own data */
		MXFLIB_TYPEIF_VIRTUAL bool HandlesSubdata(void) const MXFLIB_TYPEIF_PURE;

	public:
		/* Static traits methods */

		//! Add a mapping to be applied to all types of a given type name
		/*! \note This will act retrospectively
		 */
		static bool AddTraitsMapping(std::string TypeName, std::string TraitsName);

		//! Update an existing mapping and apply to any existing type of the given name
		static bool UpdateTraitsMapping(std::string TypeName, std::string TraitsName);

		//! Add a mapping to be applied to all types of a given type UL
		/*! \note This will act retrospectively
		 */
		static bool AddTraitsMapping(const UL &TypeUL, std::string TraitsName);

		//! Update an existing mapping and apply to any existing type of the given UL
		static bool UpdateTraitsMapping(const UL &TypeUL, std::string TraitsName);

		//! Lookup the traits for a specified type name
		/*! If no traits have been defined for the specified type the traits with the name given in DefaultTraitsName is used (if specified)
		 */
		static MDTraitsPtr LookupTraitsMapping(std::string TypeName, std::string DefaultTraitsName = "");

		//! Lookup the traits for a specified type UL
		/*! If no traits have been defined for the specified type the traits with the UL given in DefaultTraitsName is used
		*/
		static MDTraitsPtr LookupTraitsMapping(const UL &TypeUL, const UL &DefaultTraitsUL);

		//! Lookup the traits for a specified type name
		/*! If no traits have been defined for the specified type the traits with the UL given in DefaultTraitsName is used
		 */
		static MDTraitsPtr LookupTraitsMapping(std::string TypeName, const UL &DefaultTraitsUL);

		//! Lookup the traits for a specified type UL
		/*! If no traits have been defined for the specified type the traits with the name given in DefaultTraitsName is used (if specified)
		*/
		static MDTraitsPtr LookupTraitsMapping(const UL &TypeUL, std::string DefaultTraitsName = "");
	};

	//! Interface for accessing child types
	template<class TypePtr, class TypeList> class IMDTypeChild
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDTypeChild() {};

		//! Read-only access to ChildList
		MXFLIB_TYPEIF_VIRTUAL const TypeList &GetChildList(void) const MXFLIB_TYPEIF_PURE;

		//! Locate a named child
		MXFLIB_TYPEIF_VIRTUAL TypePtr Child(const std::string Name) const MXFLIB_TYPEIF_PURE;

		//! Locate a named child
		MXFLIB_TYPEIF_VIRTUAL TypePtr operator[](const std::string Name) const MXFLIB_TYPEIF_PURE;

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MXFLIB_TYPEIF_VIRTUAL TypePtr Child(int Index) const MXFLIB_TYPEIF_PURE;

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MXFLIB_TYPEIF_VIRTUAL TypePtr operator[](int Index) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by UL
		MXFLIB_TYPEIF_VIRTUAL TypePtr Child(ULPtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by UL
		MXFLIB_TYPEIF_VIRTUAL TypePtr operator[](ULPtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by UL
		MXFLIB_TYPEIF_VIRTUAL TypePtr Child(const UL &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by UL
		MXFLIB_TYPEIF_VIRTUAL TypePtr operator[](const UL &ChildType) const MXFLIB_TYPEIF_PURE;
	};


	//! Interface for accessing child objects
	class IMDChildAccess
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDChildAccess() {};

		//! Read-only access to ChildList
		MXFLIB_TYPEIF_VIRTUAL const MDOTypeList &GetChildList(void) const MXFLIB_TYPEIF_PURE;

		//! Locate a named child
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr Child(const std::string Name) const MXFLIB_TYPEIF_PURE;

		//! Locate a named child
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr operator[](const std::string Name) const MXFLIB_TYPEIF_PURE;

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr Child(int Index) const MXFLIB_TYPEIF_PURE;

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr operator[](int Index) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by UL
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr Child(ULPtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by UL
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr operator[](ULPtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by object type
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr Child(const MDOTypePtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by object type
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr operator[](const MDOTypePtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by value type
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr Child(const MDTypePtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Locate a child by value type
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr operator[](const MDTypePtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Add a new child MDObject of the specified type
		/*! ChildName is a symbol to be located in default MXFLib SymbolSpace
		 */
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr AddChild(std::string ChildName, bool Replace) MXFLIB_TYPEIF_PURE;

		//! Add a new child MDObject of the specified type
		/*! ChildName is a symbol to be located in the given SymbolSpace - if no SymbolSpace is specifed the default MXFLib space is used 
		 */
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr AddChild(std::string ChildName, SymbolSpacePtr &SymSpace = MXFLibSymbols, bool Replace = true) MXFLIB_TYPEIF_PURE;

	
		//! Add a new child MDObject of the specified type
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr AddChild(MDOTypePtr ChildType, bool Replace = true) MXFLIB_TYPEIF_PURE;
		
		//! Add a new child MDObject to a vector
		/*! \note The type of the object added is automatic. 
		 *        If the vector is of multiple members the next type will be chosen by the number of members currently
		 *        in the array, so if there are 3 sub member types the 7th entry will be type 1 [ 7 = (2*3) + 1 ]
		 *
		 *  \note This version of AddChild will <b>not</b> replace duplicates, it always appends
		 */
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr AddChild(void) MXFLIB_TYPEIF_PURE;

		//! Add a new child MDObject of the specified type
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr AddChild(const UL &ChildType, bool Replace = true) MXFLIB_TYPEIF_PURE;
		
		//! Add a new child MDObject of the specified type
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr AddChild(ULPtr &ChildType, bool Replace = true) MXFLIB_TYPEIF_PURE;

		//! Add the given child object
		/*! \ret false if unable to add this child */
		MXFLIB_TYPEIF_VIRTUAL bool AddChild(MDObjectPtr &ChildObject, bool Replace = false) MXFLIB_TYPEIF_PURE;

//		//! Add the given child object at a specific numerical index
//		/*! \ret false if unable to add this child at the specified location (for example if not numerically indexed) */
//		MXFLIB_TYPEIF_VIRTUAL bool AddChild(MDObjectPtr &Child, int Index) MXFLIB_TYPEIF_PURE;

		//! Remove the (first) child of the specified type
		MXFLIB_TYPEIF_VIRTUAL void RemoveChild(std::string ChildName) MXFLIB_TYPEIF_PURE;

		//! Remove the (first) child of the specified type
		MXFLIB_TYPEIF_VIRTUAL void RemoveChild(MDOTypePtr &ChildType) MXFLIB_TYPEIF_PURE;

		//! Remove the (first) child of the specified type
		MXFLIB_TYPEIF_VIRTUAL void RemoveChild(ULPtr &ChildType) MXFLIB_TYPEIF_PURE;

		//! Remove the specified child
		MXFLIB_TYPEIF_VIRTUAL void RemoveChild(MDObjectPtr ChildObject) MXFLIB_TYPEIF_PURE;

		//! Get a list of all child items of a specified type
		MXFLIB_TYPEIF_VIRTUAL MDObjectListPtr ChildList(const std::string ChildName) const MXFLIB_TYPEIF_PURE;

		//! Get a list of all child items of a specified type
		MXFLIB_TYPEIF_VIRTUAL MDObjectListPtr ChildList(const UL &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Get a list of all child items of a specified type
		MXFLIB_TYPEIF_VIRTUAL MDObjectListPtr ChildList(const ULPtr &ChildType) const MXFLIB_TYPEIF_PURE;;

		//! Get a list of all child items of a specified type
		MXFLIB_TYPEIF_VIRTUAL MDObjectListPtr ChildList(const MDOTypePtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Get a list of all child items of a specified type
		MXFLIB_TYPEIF_VIRTUAL MDObjectListPtr ChildList(const MDTypePtr &ChildType) const MXFLIB_TYPEIF_PURE;
	};

	//! Interface for getting value info for MDObjects
	class IMDValueGet
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDValueGet() {};

		/* Get the value of this object */

		//! Get the 32-bit signed integer version of value
		MXFLIB_TYPEIF_VIRTUAL Int32 GetInt(Int32 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the 64-bit signed integer version of value
		MXFLIB_TYPEIF_VIRTUAL Int64 GetInt64(Int64 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the 32-bit unsigned integer version of value
		MXFLIB_TYPEIF_VIRTUAL UInt32 GetUInt(UInt32 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the 64-bit unsigned integer version of value
		MXFLIB_TYPEIF_VIRTUAL UInt64 GetUInt64(UInt64 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the UTF-8 string version of value
		MXFLIB_TYPEIF_VIRTUAL std::string GetString(std::string Default = "")	const MXFLIB_TYPEIF_PURE;

		//! Is this a Best Effort property that is set to its distinguished value?
		MXFLIB_TYPEIF_VIRTUAL bool IsDValue(void) const MXFLIB_TYPEIF_PURE;


		/* Get the value of a child object by name */

		//! Get the 32-bit signed integer version of value of named child
		MXFLIB_TYPEIF_VIRTUAL Int32 GetInt(const char *ChildName, Int32 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the 64-bit signed integer version of value of named child
		MXFLIB_TYPEIF_VIRTUAL Int64 GetInt64(const char *ChildName, Int64 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the 32-bit unsigned integer version of value of named child
		MXFLIB_TYPEIF_VIRTUAL UInt32 GetUInt(const char *ChildName, UInt32 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the 64-bit unsigned integer version of value of named child
		MXFLIB_TYPEIF_VIRTUAL UInt64 GetUInt64(const char *ChildName, UInt64 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the UTF-8 string version of value of named child
		MXFLIB_TYPEIF_VIRTUAL std::string GetString(const char *ChildName, std::string Default = "") const MXFLIB_TYPEIF_PURE;

		//! Is the named child a Best Effort property that is set to its distinguished value?
		MXFLIB_TYPEIF_VIRTUAL bool IsDValue(const char *ChildName) const MXFLIB_TYPEIF_PURE;


		/* Get the value of a child object by UL */

		//! Get the 32-bit signed integer version of value of UL identified child
		MXFLIB_TYPEIF_VIRTUAL Int32 GetInt(const UL &Child, Int32 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the 64-bit signed integer version of value of UL identified child
		MXFLIB_TYPEIF_VIRTUAL Int64 GetInt64(const UL &Child, Int64 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the 32-bit unsigned integer version of value of UL identified child
		MXFLIB_TYPEIF_VIRTUAL UInt32 GetUInt(const UL &Child, UInt32 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the 64-bit unsigned integer version of value of UL identified child
		MXFLIB_TYPEIF_VIRTUAL UInt64 GetUInt64(const UL &Child, UInt64 Default = 0) const MXFLIB_TYPEIF_PURE;

		//! Get the UTF-8 string version of value of UL identified child
		MXFLIB_TYPEIF_VIRTUAL std::string GetString(const UL &Child, std::string Default = "") const MXFLIB_TYPEIF_PURE;

		//! Is the UL identified child a Best Effort property that is set to its distinguished value?
		MXFLIB_TYPEIF_VIRTUAL bool IsDValue(const UL &Child) const MXFLIB_TYPEIF_PURE;


		/* Access the raw data value */

		//! Get a reference to the data chunk (const to prevent setting!!)
		MXFLIB_TYPEIF_VIRTUAL const DataChunk& GetData(void) const MXFLIB_TYPEIF_PURE;

		//! Build a data chunk with all this items data (including child data)
		MXFLIB_TYPEIF_VIRTUAL DataChunkPtr PutData(PrimerPtr UsePrimer = NULL) const MXFLIB_TYPEIF_PURE;


		/* Misc value getting methods */

		//! Access function for Parent
		MXFLIB_TYPEIF_VIRTUAL MDObjectParent GetParent(void) const MXFLIB_TYPEIF_PURE;

		//! Access function for ParentFile
		MXFLIB_TYPEIF_VIRTUAL MXFFilePtr GetParentFile(void) const MXFLIB_TYPEIF_PURE;

		//! Make a copy of this object
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr MakeCopy(void) const MXFLIB_TYPEIF_PURE;

		//! Has this object (including any child objects) been modified?
		MXFLIB_TYPEIF_VIRTUAL bool IsModified(void) const MXFLIB_TYPEIF_PURE;

		//! Get the location within the ultimate parent
		MXFLIB_TYPEIF_VIRTUAL Position GetLocation(void) const MXFLIB_TYPEIF_PURE;

		//! Get text that describes where this item came from
		MXFLIB_TYPEIF_VIRTUAL std::string GetSource(void) const MXFLIB_TYPEIF_PURE;

		//! Get text that describes exactly where this item came from
		MXFLIB_TYPEIF_VIRTUAL std::string GetSourceLocation(void) const MXFLIB_TYPEIF_PURE;

		//! Get pointer to Outer object
		MXFLIB_TYPEIF_VIRTUAL ObjectInterface *GetOuter(void) MXFLIB_TYPEIF_PURE;
	};


	//! Interface for setting value info for MDObjects
	class IMDValueSet
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDValueSet() {};

		/* Set the value of this object */

		//! Set the value from a 32-bit signed integer
		MXFLIB_TYPEIF_VIRTUAL void SetInt(Int32 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value from a 64-bit signed integer
		MXFLIB_TYPEIF_VIRTUAL void SetInt64(Int64 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value from a 32-bit unsigned integer
		MXFLIB_TYPEIF_VIRTUAL void SetUInt(UInt32 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value from a 64-bit unsigned integer
		MXFLIB_TYPEIF_VIRTUAL void SetUInt64(UInt64 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value from a UTF-8 string
		MXFLIB_TYPEIF_VIRTUAL void SetString(std::string Val)	 MXFLIB_TYPEIF_PURE;

		//! Set this object to its distinguished value
		/*! \return true if distinguished value set, else false */
		MXFLIB_TYPEIF_VIRTUAL bool SetDValue(void) MXFLIB_TYPEIF_PURE;

		//! Set the default value for this object
		/*! \return true is a default value is set, else false */
		MXFLIB_TYPEIF_VIRTUAL bool SetDefault(void) MXFLIB_TYPEIF_PURE;


		/* Set the value of a child object by name */

		//! Set the value of named child from a 32-bit signed integer
		MXFLIB_TYPEIF_VIRTUAL void SetInt(const char *ChildName, Int32 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value of named child from a 64-bit signed integer
		MXFLIB_TYPEIF_VIRTUAL void SetInt64(const char *ChildName, Int64 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value of named child from a 32-bit unsigned integer
		MXFLIB_TYPEIF_VIRTUAL void SetUInt(const char *ChildName, UInt32 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value of named child from a 32-bit unsigned integer
		MXFLIB_TYPEIF_VIRTUAL void SetUInt64(const char *ChildName, UInt64 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value of named child from a UTF-8 string
		MXFLIB_TYPEIF_VIRTUAL void SetString(const char *ChildName, std::string Val) MXFLIB_TYPEIF_PURE;

		//! Set the named child to its distinguished value
		/*! \return true if distinguished value set, else false */
		MXFLIB_TYPEIF_VIRTUAL bool SetDValue(const char *ChildName) MXFLIB_TYPEIF_PURE;

		//! Set the default value for the named child
		/*! \return true is a default value is set, else false */
		MXFLIB_TYPEIF_VIRTUAL bool SetDefault(const char *ChildName) MXFLIB_TYPEIF_PURE;


		/* Set the value of a child object by UL */

		//! Set the value of UL identified child from a 32-bit signed integer
		MXFLIB_TYPEIF_VIRTUAL void SetInt(const UL &Child, Int32 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value of UL identified child from a 64-bit signed integer
		MXFLIB_TYPEIF_VIRTUAL void SetInt64(const UL &Child, Int64 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value of UL identified child from a 32-bit unsigned integer
		MXFLIB_TYPEIF_VIRTUAL void SetUInt(const UL &Child, UInt32 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value of UL identified child from a 32-bit unsigned integer
		MXFLIB_TYPEIF_VIRTUAL void SetUInt64(const UL &Child, UInt64 Val) MXFLIB_TYPEIF_PURE;

		//! Set the value of UL identified child from a UTF-8 string
		MXFLIB_TYPEIF_VIRTUAL void SetString(const UL &Child, std::string Val) MXFLIB_TYPEIF_PURE;

		//! Set the UL identified child to its distinguished value
		/*! \return true if distinguished value set, else false */
		MXFLIB_TYPEIF_VIRTUAL bool SetDValue(const UL &Child) MXFLIB_TYPEIF_PURE;

		//! Set the default value for the UL identified child
		/*! \return true is a default value is set, else false */
		MXFLIB_TYPEIF_VIRTUAL bool SetDefault(const UL &Child) MXFLIB_TYPEIF_PURE;


		/* Misc value setting methods */

		//! Set data into the datachunk
		// DRAGONS: This is dangerous as it bypasses any traits!!
		MXFLIB_TYPEIF_VIRTUAL void SetData(size_t MemSize, const UInt8 *Buffer) MXFLIB_TYPEIF_PURE;

		//! Inset a new child object - overloads the existing MDObjectList versions
		MXFLIB_TYPEIF_VIRTUAL void insert(MDObjectPtr NewObject) MXFLIB_TYPEIF_PURE;

		//! Set the parent details when an object has been read from a file
		MXFLIB_TYPEIF_VIRTUAL void SetParent(MXFFilePtr &File, Position Location, UInt32 NewKLSize) MXFLIB_TYPEIF_PURE;

		//! Set the parent details when an object has been read from memory
		MXFLIB_TYPEIF_VIRTUAL void SetParent(MDObjectPtr &Object, Position Location, UInt32 NewKLSize) MXFLIB_TYPEIF_PURE;

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		MXFLIB_TYPEIF_VIRTUAL bool ChangeType(const UL &NewType) MXFLIB_TYPEIF_PURE;

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		MXFLIB_TYPEIF_VIRTUAL bool ChangeType(ULPtr &NewType) MXFLIB_TYPEIF_PURE;

		//! Change the type of an MDObject
		/*! \note This may result in very wrong data - exercise great care! */
		MXFLIB_TYPEIF_VIRTUAL bool ChangeType(std::string NewType) MXFLIB_TYPEIF_PURE;

		//! Set the GenerationUID of an object iff it has been modified
		/*! \return true if the GenerationUID has been set, otherwise false
		 *  \note If the object does not have a GenerationUID property false is returned!
		 */
		MXFLIB_TYPEIF_VIRTUAL bool SetGenerationUID(UUIDPtr UID) MXFLIB_TYPEIF_PURE;

		//! Clear the modified flag on this object and any contained objects
		MXFLIB_TYPEIF_VIRTUAL void ClearModified(void) MXFLIB_TYPEIF_PURE;

		//! Set pointer to Outer object
		MXFLIB_TYPEIF_VIRTUAL void SetOuter(ObjectInterface *NewOuter) MXFLIB_TYPEIF_PURE;


		/* Static value setting methods */

	public:
		//! Pointer to a translator function to translate unknown ULs to object names
		typedef std::string (*ULTranslator)(ULPtr,const Tag *);

		//! Set a translator function to translate unknown ULs to object names
		static void SetULTranslator(ULTranslator Trans);

		//! Set the "attempt to parse dark metadata" flag
		static void SetParseDark(bool Value);
	};

	//! Interface for reading and writing value info for MDObjects
	class IMDValueIO
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDValueIO() {};

		//! Read the object's value from a data chunk
		MXFLIB_TYPEIF_VIRTUAL size_t ReadValue(const DataChunk &Chunk, PrimerPtr UsePrimer = NULL) MXFLIB_TYPEIF_PURE;

		//! Read the object's value from a data chunk pointer
		MXFLIB_TYPEIF_VIRTUAL size_t ReadValue(DataChunkPtr &Chunk, PrimerPtr UsePrimer = NULL) MXFLIB_TYPEIF_PURE;

		//! Read the object's value from a memory buffer
		MXFLIB_TYPEIF_VIRTUAL size_t ReadValue(const UInt8 *Buffer, size_t Size, PrimerPtr UsePrimer = NULL) MXFLIB_TYPEIF_PURE;

		//! Write this object to a new memory buffer
		MXFLIB_TYPEIF_VIRTUAL DataChunkPtr WriteObject(const MDObject *ParentObject, UInt32 BERSize = 0) const MXFLIB_TYPEIF_PURE;

		//! Write this object to a new memory buffer
		MXFLIB_TYPEIF_VIRTUAL DataChunkPtr WriteObject(const MDObject *ParentObject, PrimerPtr UsePrimer, UInt32 BERSize = 0) const MXFLIB_TYPEIF_PURE;

		//! Append this object to a memory buffer
		MXFLIB_TYPEIF_VIRTUAL size_t WriteObject(DataChunkPtr &Buffer, const MDObject *ParentObject = NULL, UInt32 BERSize = 0) const MXFLIB_TYPEIF_PURE;

		//! Append this object to a memory buffer
		MXFLIB_TYPEIF_VIRTUAL size_t WriteObject(DataChunkPtr &Buffer, const MDObject *ParentObject, PrimerPtr UsePrimer, UInt32 BERSize = 0) const MXFLIB_TYPEIF_PURE;

		//! Write this top level object to a new memory buffer
		/*! The object must be at the outer or top KLV level
		 *	\return The new buffer
		 */
		MXFLIB_TYPEIF_VIRTUAL DataChunkPtr WriteObject(UInt32 BERSize = 0) const MXFLIB_TYPEIF_PURE;

		//! Write this top level object to a new memory buffer
		/*! The object must be at the outer or top KLV level
		 *	\return The new buffer
		 */
		MXFLIB_TYPEIF_VIRTUAL DataChunkPtr WriteObject(PrimerPtr UsePrimer, UInt32 BERSize = 0) const MXFLIB_TYPEIF_PURE;

		//! Append this top level object to a memory buffer
		/*! The object must be at the outer or top KLV level. The object is appended to the buffer
		 *	\return The number of bytes written
		 */
		MXFLIB_TYPEIF_VIRTUAL size_t WriteObject(DataChunkPtr &Buffer, PrimerPtr UsePrimer, UInt32 BERSize = 0) const MXFLIB_TYPEIF_PURE;

		//! Append this top level object, and any strongly linked sub-objects, to a memory buffer
		MXFLIB_TYPEIF_VIRTUAL size_t WriteLinkedObjects(DataChunkPtr &Buffer, PrimerPtr UsePrimer = NULL) MXFLIB_TYPEIF_PURE;
	};


	//! Interface for setting or getting ref link info for MDTypes
	class IMDTypeRef
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDTypeRef() {};

		//! Set the referencing details for this type, with a UL target type
		MXFLIB_TYPEIF_VIRTUAL void SetRef(ClassRef Type, ULPtr &Target, std::string TargetName = "") MXFLIB_TYPEIF_PURE;

		//! Set the referencing details for this type, with an MDTypePtr target type
		MXFLIB_TYPEIF_VIRTUAL void SetRef(ClassRef Type, MDTypePtr &Target, std::string TargetName = "") MXFLIB_TYPEIF_PURE;
	};


	//! Interface for setting or getting ref link info for MDObjects
	class IMDValueRef
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDValueRef() {};

		//! Access the target of a reference link
		MXFLIB_TYPEIF_VIRTUAL MDObjectParent GetRef(void) const MXFLIB_TYPEIF_PURE;

		//! Access the target of a reference link child property
		MXFLIB_TYPEIF_VIRTUAL MDObjectParent GetRef(std::string ChildType) const MXFLIB_TYPEIF_PURE;

		//! Access the target of a reference link child property
		MXFLIB_TYPEIF_VIRTUAL MDObjectParent GetRef(const UL &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Access the target of a reference link child property
		MXFLIB_TYPEIF_VIRTUAL MDObjectParent GetRef(ULPtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Make a link from this reference source to the specified target set
		MXFLIB_TYPEIF_VIRTUAL bool MakeRef(MDObjectPtr &TargetSet, bool ForceLink = false) MXFLIB_TYPEIF_PURE;

		//! Make a link from this reference source to the specified target set via the given target property
		MXFLIB_TYPEIF_VIRTUAL bool MakeRef(MDObjectPtr &TargetSet, const UL &Target, bool ForceLink = false) MXFLIB_TYPEIF_PURE;

		//! Make a link from the given source child of this set to the specified target set, adding a new child if required
		MXFLIB_TYPEIF_VIRTUAL bool MakeRef(const UL &Source, MDObjectPtr &TargetSet, bool ForceLink = false) MXFLIB_TYPEIF_PURE;

		//! Add a new source child to the specified property of this set and link it to the specified target set
		/*! This is used for adding new reference entries to batches or arrays in this set */
		MXFLIB_TYPEIF_VIRTUAL bool AddRef(const UL &Source, MDObjectPtr &TargetSet, bool ForceLink = false) MXFLIB_TYPEIF_PURE;
	};


	//! Interface for structural MDOType / MDObject methods
	class IMDStructure
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDStructure() {};

		//! Determine if this type is derived from a specified type (directly or indirectly)
		MXFLIB_TYPEIF_VIRTUAL bool IsA(std::string BaseType) MXFLIB_TYPEIF_PURE;

		//! Determine if this type is derived from a specified type (directly or indirectly)
		MXFLIB_TYPEIF_VIRTUAL bool IsA(MDOTypePtr &BaseType) MXFLIB_TYPEIF_PURE;
		
		//! Determine if this type is derived from a specified type (directly or indirectly)
		MXFLIB_TYPEIF_VIRTUAL bool IsA(const UL &BaseType) MXFLIB_TYPEIF_PURE;

		//! Determine if this type is derived from a specified type (directly or indirectly)
		MXFLIB_TYPEIF_VIRTUAL bool IsA(ULPtr &BaseType) MXFLIB_TYPEIF_PURE;

		//! Determine if this type is known to have a child with a given UL
		/*! This determines if the specified UL has been included as a child of this type in any loaded disctionary.
		 *  It may be valid for children of this UL to be included, even if this function returns false 
		 */
		MXFLIB_TYPEIF_VIRTUAL bool HasA(const ULPtr &ChildType) const MXFLIB_TYPEIF_PURE;

		//! Get read-only access to the base type
		MXFLIB_TYPEIF_VIRTUAL const MDOTypeParent &GetBase(void) const MXFLIB_TYPEIF_PURE;


#ifdef OPTION3ENABLED
		//! Is the is baseline class, as defined in 377M
		/*! If the type is not a set then it is assumed to be baseline */
		MXFLIB_TYPEIF_VIRTUAL bool IsBaseline(void) const MXFLIB_TYPEIF_PURE;

		//! Determine the nearest baseline UL for this type
		MXFLIB_TYPEIF_VIRTUAL ULPtr GetBaselineUL(void) const MXFLIB_TYPEIF_PURE;
#endif // OPTION3ENABLED

	public:
		/* Static structure methods */

		//! Get the static primer (make one if required)
		static PrimerPtr GetStaticPrimer(void);
	};

/*
	//! Interface for structural MDType methods
	class IMDTypeStructure
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDTypeStructure() {};

		//! Determine if this type is derived from a specified type (directly or indirectly)
		MXFLIB_TYPEIF_VIRTUAL bool IsA(std::string BaseType) MXFLIB_TYPEIF_PURE;

		//! Determine if this type is derived from a specified type (directly or indirectly)
		MXFLIB_TYPEIF_VIRTUAL bool IsA(MDTypePtr &BaseType) MXFLIB_TYPEIF_PURE;
		
		//! Determine if this type is derived from a specified type (directly or indirectly)
		MXFLIB_TYPEIF_VIRTUAL bool IsA(const UL &BaseType) MXFLIB_TYPEIF_PURE;

		//! Determine if this type is derived from a specified type (directly or indirectly)
		MXFLIB_TYPEIF_VIRTUAL bool IsA(ULPtr &BaseType) MXFLIB_TYPEIF_PURE;

		//! Determine if this type is known to have a child with a given UL
		*! This determines if the specified UL has been included as a child of this type in any loaded disctionary.
		 *  It may be valid for children of this UL to be included, even if this function returns false 
		 *
		MXFLIB_TYPEIF_VIRTUAL bool HasA(const ULPtr &ChildType) const MXFLIB_TYPEIF_PURE;
	};
*/

	//! Interface for MDType dictionary handling
	class IMDDict
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDDict() {};

	public:
		/* Static dictionary methods */

		//! Load the dictionary
		static void LoadDict(const char *DictFile, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols);

		//! Generate a new type based on a base type
		/*! DRAGONS: Use with great care - this is intended for library code that generates inferred classes */
		static MDOTypePtr DeriveCopy(MDOTypePtr &Base, std::string Name, std::string Detail);

		//! Define a class based on a ClassRecord
		static MDOTypePtr DefineClass(ClassRecordPtr &ThisClass, SymbolSpacePtr DefaultSymbolSpace, MDOTypePtr Parent);

		// Define a class based on a ClassRecord with an optional symbol space
		static MDOTypePtr DefineClass(ClassRecordPtr &ThisClass, MDOTypePtr Parent = NULL, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols);

		//! Define a class based on a ClassRecord - recursive version to allow out-of order definitions
		static MDOTypePtr DefineClass(ClassRecordPtr &ThisClass, SymbolSpacePtr DefaultSymbolSpace, ClassRecordList *Unresolved = NULL, MDOTypePtr Parent = NULL);
	};


	//! Interface for manipulating the MDOType structure
	class IMDTypeManip
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDTypeManip() {};

		//! Derive this new entry from a base entry
		MXFLIB_TYPEIF_VIRTUAL void Derive(MDOTypePtr &BaseEntry) MXFLIB_TYPEIF_PURE;

		//! Re-Derive sub-items from a base entry
		/*! Used when the base entry is being extended 
		 */
		MXFLIB_TYPEIF_VIRTUAL void ReDerive(MDOTypePtr &BaseEntry) MXFLIB_TYPEIF_PURE;

		//! Redefine a sub-item in a container
		MXFLIB_TYPEIF_VIRTUAL void ReDefine(std::string NewDetail, std::string NewBase, unsigned int NewMinSize, unsigned int NewMaxSize) MXFLIB_TYPEIF_PURE;
		
		//! Redefine a container
		MXFLIB_TYPEIF_VIRTUAL void ReDefine(std::string NewDetail) MXFLIB_TYPEIF_PURE;

	public:
		/* Static manipulation methods */

		//! Locate reference target types for any types not yet located
		static void LocateRefTypes(void);

		//! Build a primer from the current dictionary, optionally set as the static primer
		static PrimerPtr MakePrimer(bool SetStatic = false);
	};


	//! Interface for deprecated MDObject methods
	class IMDDeprecated
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDDeprecated() {};

		//! Force the data buffer to be a given number of bytes
		/*! \return The number of entries in the resized value (may not be what was requested) 
		 *  DRAGONS: Use with care as this bypasses all checking and traits
		 */
		MXFLIB_TYPEIF_VIRTUAL size_t MakeSize(size_t NewSize) MXFLIB_TYPEIF_PURE;

		//! Link access functions
		/*! \note This method is deprecated - use GetRef() instead */
		MXFLIB_TYPEIF_VIRTUAL MDObjectPtr GetLink(void) const MXFLIB_TYPEIF_PURE;

		//! DEPRECATED: Make a link from this reference source to the specified target set
		/*! \note This method is deprecated - use MakeRef() instead */
		MXFLIB_TYPEIF_VIRTUAL bool MakeLink(MDObjectPtr &TargetSet, bool ForceLink = false) MXFLIB_TYPEIF_PURE;
	};


	//! Interface for deprecated MDOType methods
	class IMDOTypeDeprecated
	{
	public:
		//! Obligatory virtual destructor
		MXFLIB_TYPEIF_VIRTUAL ~IMDOTypeDeprecated() {};

		//! Read-only access to ChildOrder
		/*! \note DEPRECATED - Use GetChildList() instead
		 */
		MXFLIB_TYPEIF_VIRTUAL const StringList &GetChildOrder(void) const MXFLIB_TYPEIF_PURE;
	};
}


#endif // MXFLIB__TYPEIF_H

