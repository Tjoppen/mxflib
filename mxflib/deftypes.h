/*! \file	deftypes.h
 *	\brief	Definition of classes that load type and class dictionaries
 *	\version $Id: deftypes.h,v 1.14 2011/01/10 10:42:08 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2005, Matt Beard
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
#ifndef MXFLIB__DICTIONARY_H
#define MXFLIB__DICTIONARY_H
 
// STL Includes
#include <string>
#include <list>


namespace mxflib
{
	// DRAGONS: Why are there two different (but similar) in memory versions of type records?
	//          The answer is that as some types depend on others the loading process requires stacking
	//          part-processed entries for later completion. It is not easy to stack the simple struct
	//          version, partly because of the char pointers to strings and partly because of the fact
	//          that the compound sub-items are linked by their position in the array rather than a pointer
	//          giving ownership - however these same features are required in order to allow data to be
	//          built at compile-time which is highly desirable. When a type definition table made from
	//          the "constant" struct version is loaded it is first copied to a list of classes for easy
	//          manipulation during further processing.

	//! Enumeration identifying the type of an in-memory type definition entry to be processed by LoadTypes()
	enum TypeClass
	{
		TypeNULL = 0,						//!< Special token used to end a list of types
		TypeBasic,							//!< Basic type definition (the atomic definition of a type)
		TypeInterpretation,					//!< Interpretation - physically identical to another type, but with different symantics
		TypeMultiple,						//!< Multiple an array or batch of zero or more of another single type
		TypeCompound,						//!< Compound structure of two or more of another type, which may be different types, with a fixed layout
		TypeSub,							//!< An individual sub-item in a compound, or value in an enum
		TypeEnum,							//!< Enumeration type, with one or more named values
		TypeLabel,							//!< Define a label value
		TypeSymbolSpace						//!< Define the default symbol space for all types in this list
	};

	//! Referencing kinds for Types
	enum TypeRef
	{
		TypeRefUndefined = -1,				//!< Not defined yet for this type (used to differentiate not defined and specifically defined as "none")
		TypeRefNone = 0,					//!< Not a reference
		TypeRefStrong,						//!< A strong reference
		TypeRefWeak,						//!< A weak reference
		TypeRefMeta,						//!< A metadictionary reference (either a reference into a metadictionary, or just a value)
		TypeRefDict,						//!< A dictionary reference (either a reference into a dictionary, or just a value)
		TypeRefGlobal,						//!< A global reference - may be to a definition in the file, or to a published definition (outside the scope of the file)
		TypeRefTarget						//!< A target of a reference
	};

	//! Is a given reference type a reference source?
	bool inline IsRefSource(TypeRef Ref)
	{
		return (Ref == TypeRefStrong) || (Ref == TypeRefWeak) || (Ref == TypeRefMeta) || (Ref == TypeRefDict) || (Ref == TypeRefGlobal);
	}

	/* Bit values for Class.Flags */
	const UInt8 TypeFlags_Endian = 1;		//! Used with basic types: "1" if this type gets endian swapped on reading/writing on a little-endian platform
	const UInt8 TypeFlags_Baseline = 2;		//! This entry is a baseline type as defined in 377M or certain other specific standards (and so will not be added to the KXS metadictionary)

	//! Single entry for a type to be defined - can be stored as a compile-time built structure
	struct ConstTypeRecord
	{
		TypeClass Class;					//!< The class of type being defined by this entry
		const char *Type;					//!< The name of this type
		const char *Detail;					//!< The human readable description of this type
		const char *Base;					//!< The base type for an interpretation or multiple, or the type for a compound type sub-item
		const char *UL;						//!< The UL for this type (if known)
		const char *Value;					//!< Value if this is an enumerated value, or mask if this is a label
		int Size;							//!< The size in bytes of a basic type, or the number of entries in a multiple
		MDArrayClass ArrayClass;			//!< Used with multiple types to signify the class of array/batch/string
		TypeRef RefType;					//!< Reference type of this item (if a reference or target)
		const char *RefTarget;				//!< Type of the reference target (if this is a referencing type) or NULL
		const char *SymSpace;				//!< SymbolSpace for this type, or NULL if none specified (will inherit)
		UInt8 Flags;						//!< Flags for this entry as a bitfield
	};

	// Forward declare TypeRecord to allow TypeRecordPtr to be defined early
	class TypeRecord;

	//! A smart pointer to a TypeRecord
	typedef SmartPtr<TypeRecord> TypeRecordPtr;

	//! List of smart pointers to TypeRecords
	typedef std::list<TypeRecordPtr> TypeRecordList;

	//! List of TypeRecordLists
	typedef std::list<TypeRecordList> TypeRecordListList;

	//! Single entry for a type to be defined - built at run-time and can be stacked if required to allow out-of-order definitions
	class TypeRecord : public RefCount<TypeRecord>
	{
	public:
		TypeClass Class;					//!< The class of type being defined by this entry
		std::string Type;					//!< The name of this type
		std::string Detail;					//!< The human readable description of this type
		std::string Base;					//!< The base type for an interpretation or multiple, or the type for a compound type sub-item
		ULPtr UL;							//!< The UL for this type (NULL or "" if not known)
		std::string Value;					//!< Value if this is an enumerated value, or mask if this is a label
		int Size;							//!< The size in bytes of a basic type, or the number of entries in a multiple
		bool Endian;						//!< Used with basic types: "true" if this type gets endian swapped on reading/writing on a little-endian platform
		MDArrayClass ArrayClass;			//!< Used with multiple types to signify the class of array/batch/string
		TypeRef RefType;					//!< Reference type of this item (if a reference or target)
		std::string RefTarget;				//!< Type of the reference target (if this is a referencing type)
		SymbolSpacePtr SymSpace;			//!< SymbolSpace for this type, or NULL if none specified (will inherit)
		bool IsBaseline;					//!< True if this is a baseline type as defined in 377M or certain other specific standards (and so will not be added to the KXS metadictionary)
		TypeRecordList Children;			//!< Used with compound types: Sub-items within this compound

	public:
		//! Build an empty TypeRecord
		TypeRecord()
		{
			Class = TypeNULL;
			Size = 0;
			Endian = false;
			IsBaseline = false;
			ArrayClass = ARRAYIMPLICIT;
			RefType = TypeRefUndefined;
		}
	};

	//! Load types from the specified in-memory definitions
	/*! \note The last entry in the array must be a terminating entry with Class == TypeNULL
	 *  \return 0 if all OK
	 *  \return -1 on error
	 *  \note If any part of the dictionary loading fails the loading will continue unless FastFail is set to true
	 */
	int LoadTypes(const ConstTypeRecord *TypesData, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols);

	//! Load types from the specified in-memory definitions
	/*! \return 0 if all OK
	 *  \return -1 on error
	 *  \note If any part of the dictionary loading fails the loading will continue unless FastFail is set to true
	 */
	int LoadTypes(TypeRecordList &TypesData, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols);

	//! Find the traits for a given type - DEPRECATED
	/*! <b>DEPRECATED - Use MDTraits::Find()</B>
	 */
	inline MDTraitsPtr LookupTraits(const char* Name) { return MDTraits::Find(std::string(Name)); }

	//! Disable automatic loading of built-in traits
	/*! \note This needs to be called early as they may have already been loaded!
	 *  \ret false if the traits have already been loaded (or already disabled)
	 */
	bool DisableBuiltInTraits(void);


	/* Define macros for static type definitions */

//! MXFLIB_TYPE_START - Use to start a type definition block
#define MXFLIB_TYPE_START(Name) const ConstTypeRecord Name[] = {

//! MXFLIB_TYPE_START_SYM - Use to start a type definition block and define a default symbol space
#define MXFLIB_TYPE_START_SYM(Name, Sym) const ConstTypeRecord Name[] = { { TypeSymbolSpace, "", "", "", "", "", 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, Sym, 0 },

//! MXFLIB_TYPE_BASIC - Use to define a "Basic" type
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Size The number of bytes used to store this type (must be > 0)
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Endian "true" if this type gets endian swapped on reading/writing on a little-endian platform
 *  \param IsCharacter "true" is this is a character type
 */
#define MXFLIB_TYPE_BASIC(Name, Detail, UL, Size, Endian, IsCharacter) { TypeBasic, Name, Detail, "", UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, Endian ? TypeFlags_Endian : 0 },

//! MXFLIB_TYPE_BASIC_EX - Use to define a "Basic" type with extended options
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Size The number of bytes used to store this type (must be > 0)
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param IsCharacter "true" is this is a character type
 *  \param Sym The name of the symbol space for this type
 *	\param Flags is the flags for this type
 */
#define MXFLIB_TYPE_BASIC_EX(Name, Detail, UL, Size, IsCharacter, Sym, Flags) { TypeBasic, Name, Detail, "", UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, TypeRefUndefined, NULL, Sym, Flags },

//! MXFLIB_TYPE_BASIC_SYM - Use to define a "Basic" type and override the default symbol space
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Size The number of bytes used to store this type (must be > 0)
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Endian "true" if this type gets endian swapped on reading/writing on a little-endian platform
 *  \param IsCharacter "true" is this is a character type
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_TYPE_BASIC_SYM(Name, Detail, UL, Size, Endian, IsCharacter, Sym) { TypeBasic, Name, Detail, "", UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, TypeRefUndefined, NULL, Sym, Endian ? TypeFlags_Endian : 0 },

//! MXFLIB_TYPE_BASIC_REF - Use to define a "Basic" type that is a reference source or target
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Size The number of bytes used to store this type (must be > 0)
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Endian "true" if this type gets endian swapped on reading/writing on a little-endian platform
 *  \param IsCharacter "true" is this is a character type
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 */
#define MXFLIB_TYPE_BASIC_REF(Name, Detail, UL, Size, Endian, IsCharacter, RefType, RefTarget) { TypeBasic, Name, Detail, "", UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, RefType, RefTarget, NULL, Endian ? TypeFlags_Endian : 0 },

//! MXFLIB_TYPE_BASIC_REF_SYM - Use to define a "Basic" type that is a reference source or target and override the default symbol space
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Size The number of bytes used to store this type (must be > 0)
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Endian "true" if this type gets endian swapped on reading/writing on a little-endian platform
 *  \param IsCharacter "true" is this is a character type
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_TYPE_BASIC_REF_SYM(Name, Detail, UL, Size, Endian, IsCharacter, RefType, RefTarget, Sym) { TypeBasic, Name, Detail, "", UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, RefType, RefTarget, Sym, Endian ? TypeFlags_Endian : 0 },

//! MXFLIB_TYPE_INTERPRETATION - Use to define an "Interpretation" type
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type that this is an interpretation of
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Size If non-zero this fixes the number of entries in the variable-length base array
 *  \param IsCharacter "true" is this is a character type
 */
#define MXFLIB_TYPE_INTERPRETATION(Name, Detail, Base, UL, Size, IsCharacter) { TypeInterpretation, Name, Detail, Base, UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, 0 },

//! MXFLIB_TYPE_INTERPRETATION_EX - Use to define an "Interpretation" type with extended options
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type that this is an interpretation of
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Size If non-zero this fixes the number of entries in the variable-length base array
 *  \param IsCharacter "true" is this is a character type
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 *  \param Sym The name of the symbol space for this type
 *	\param Flags is the flags for this type
 */
#define MXFLIB_TYPE_INTERPRETATION_EX(Name, Detail, Base, UL, Size, IsCharacter, RefType, RefTarget, Sym, Flags) { TypeInterpretation, Name, Detail, Base, UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, RefType, RefTarget, Sym, Flags },

//! MXFLIB_TYPE_INTERPRETATION_SYM - Use to define an "Interpretation" type and override the default symbol space
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type that this is an interpretation of
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Size If non-zero this fixes the number of entries in the variable-length base array
 *  \param IsCharacter "true" is this is a character type
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_TYPE_INTERPRETATION_SYM(Name, Detail, Base, UL, Size, IsCharacter, Sym) { TypeInterpretation, Name, Detail, Base, UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, TypeRefUndefined, NULL, Sym, 0 },

//! MXFLIB_TYPE_INTERPRETATION_REF - Use to define an "Interpretation" type that is a reference source or target
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type that this is an interpretation of
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Size If non-zero this fixes the number of entries in the variable-length base array
 *  \param IsCharacter "true" is this is a character type
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 */
#define MXFLIB_TYPE_INTERPRETATION_REF(Name, Detail, Base, UL, Size, IsCharacter, RefType, RefTarget) { TypeInterpretation, Name, Detail, Base, UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, RefType, RefTarget, NULL, 0 },

//! MXFLIB_TYPE_INTERPRETATION_REF_SYM - Use to define an "Interpretation" type that is a reference source or target and override the default symbol space
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type that this is an interpretation of
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Size If non-zero this fixes the number of entries in the variable-length base array
 *  \param IsCharacter "true" is this is a character type
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_TYPE_INTERPRETATION_REF_SYM(Name, Detail, Base, UL, Size, IsCharacter, RefType, RefTarget, Sym) { TypeInterpretation, Name, Detail, Base, UL, "", Size, IsCharacter ? ARRAYSTRING : ARRAYIMPLICIT, RefType, RefTarget, Sym,0 },

//! MXFLIB_TYPE_MULTIPLE - Use to define a "Multiple" type
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type of which this is a multiple
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param ArrayClass is one of ARRAYIMPLICIT, ARRAYEXPLICIT or ARRAYSTRING depending on the type of multiple
 *  \param Size If non-zero this fixes the number of entries, if zero the size is variable
 */
#define MXFLIB_TYPE_MULTIPLE(Name, Detail, Base, UL, ArrayClass, Size) { TypeMultiple, Name, Detail, Base, UL, "", Size, ArrayClass, TypeRefUndefined, NULL, NULL, 0 },

//! MXFLIB_TYPE_MULTIPLE_EX - Use to define a "Multiple" type with extended options
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type of which this is a multiple
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param ArrayClass is one of ARRAYIMPLICIT, ARRAYEXPLICIT or ARRAYSTRING depending on the type of multiple
 *  \param Size If non-zero this fixes the number of entries, if zero the size is variable
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 *  \param Sym The name of the symbol space for this type
 *	\param Flags is the flags for this type
 */
#define MXFLIB_TYPE_MULTIPLE_EX(Name, Detail, Base, UL, ArrayClass, Size, RefType, RefTarget, Sym, Flags) { TypeMultiple, Name, Detail, Base, UL, "", Size, ArrayClass, RefType, RefTarget, Sym, Flags },

//! MXFLIB_TYPE_MULTIPLE_SYM - Use to define a "Multiple" type and override the default symbol space
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type of which this is a multiple
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param ArrayClass is one of ARRAYIMPLICIT, ARRAYEXPLICIT or ARRAYSTRING depending on the type of multiple
 *  \param Size If non-zero this fixes the number of entries, if zero the size is variable
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_TYPE_MULTIPLE_SYM(Name, Detail, Base, UL, ArrayClass, Size, Sym) { TypeMultiple, Name, Detail, Base, UL, "", Size, ArrayClass, TypeRefUndefined, NULL, Sym, 0 },

//! MXFLIB_TYPE_MULTIPLE_REF - Use to define a "Multiple" type that is a reference source or target
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type of which this is a multiple
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param ArrayClass is one of ARRAYIMPLICIT, ARRAYEXPLICIT or ARRAYSTRING depending on the type of multiple
 *  \param Size If non-zero this fixes the number of entries, if zero the size is variable
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 */
#define MXFLIB_TYPE_MULTIPLE_REF(Name, Detail, Base, UL, ArrayClass, Size, RefType, RefTarget) { TypeMultiple, Name, Detail, Base, UL, "", Size, ArrayClass, RefType, RefTarget, NULL, 0 },

//! MXFLIB_TYPE_MULTIPLE_REF_SYM - Use to define a "Multiple" type that is a reference source or target and override the default symbol space
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Base The type of which this is a multiple
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param ArrayClass is one of ARRAYIMPLICIT, ARRAYEXPLICIT or ARRAYSTRING depending on the type of multiple
 *  \param Size If non-zero this fixes the number of entries, if zero the size is variable
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_TYPE_MULTIPLE_REF_SYM(Name, Detail, Base, UL, ArrayClass, Size, RefType, RefTarget, Sym) { TypeMultiple, Name, Detail, Base, UL, "", Size, ArrayClass, RefType, RefTarget, Sym, 0 },

//! MXFLIB_TYPE_COMPOUND - Use to start the definition of a "Compound" type
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 */
#define MXFLIB_TYPE_COMPOUND(Name, Detail, UL) { TypeCompound, Name, Detail, "", UL, "", 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, 0 },

//! MXFLIB_TYPE_COMPOUND_EX - Use to start the definition of a "Compound" type with extended options
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Sym The name of the symbol space for this type
 *	\param Flags is the flags for this type
 */
#define MXFLIB_TYPE_COMPOUND_EX(Name, Detail, UL, Sym, Flags) { TypeCompound, Name, Detail, "", UL, "", 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, Sym, Flags },

//! MXFLIB_TYPE_COMPOUND_SYM - Use to start the definition of a "Compound" type and override the default symbol space
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param UL The UL, or endian-swapped UUID, for this type (or "" to force one to be generated)
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_TYPE_COMPOUND_SYM(Name, Detail, UL, Sym) { TypeCompound, Name, Detail, "", UL, "", 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, Sym, 0 },

//! MXFLIB_TYPE_COMPOUND_ITEM - Use to define an item within the current "Compound" type
/*! \param Name The name of the item being defined
 *  \param Detail A human readable description of the item
 *  \param Type The type of this item within the compound
 *  \param UL The UL, or endian-swapped UUID, for this item (or "" to force one to be generated)
 *  \param Size If non-zero this fixes the number of entries in a variable-length array
 */
#define MXFLIB_TYPE_COMPOUND_ITEM(Name, Detail, Type, UL, Size) { TypeSub, Name, Detail, Type, UL, "", Size, ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, 0 },

//! MXFLIB_TYPE_COMPOUND_END - Use to end definition of a "Compound" type
#define MXFLIB_TYPE_COMPOUND_END

//! MXFLIB_TYPE_ENUM - Use to start the definition of an "Enumeration" type
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Type The type of the values in this enumeration
 *  \param UL The UL, or endian-swapped UUID, for this item (or "" to force one to be generated)
 */
#define MXFLIB_TYPE_ENUM(Name, Detail, Type, UL) { TypeEnum, Name, Detail, Type, UL, "", 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, 0 },

//! MXFLIB_TYPE_ENUM_EX - Use to start the definition of an "Enumeration" type with extended options
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Type The type of the values in this enumeration
 *  \param UL The UL, or endian-swapped UUID, for this item (or "" to force one to be generated)
 *  \param Sym The name of the symbol space for this type
 *	\param Flags is the flags for this type
 */
#define MXFLIB_TYPE_ENUM_EX(Name, Detail, Type, UL, Sym, Flags) { TypeEnum, Name, Detail, Type, UL, "", 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, 0 },

//! MXFLIB_TYPE_ENUM_SYM - Use to start the definition of a "Enumeration" type and override the default symbol space
/*! \param Name The name of the type being defined
 *  \param Detail A human readable description of the type
 *  \param Type The type of the values in this enumeration
 *  \param UL The UL, or endian-swapped UUID, for this item (or "" to force one to be generated)
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_TYPE_ENUM_SYM(Name, Detail, Type, UL, Sym) { TypeEnum, Name, Detail, Type, UL, "", 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, Sym, 0 },

//! MXFLIB_TYPE_ENUM_VALUE - Use to define a value for the current "Enumeration" type
/*! \param Name The name of the value being defined
 *  \param Detail A human readable description of the value
 *  \param Value The value being defined
 */
#define MXFLIB_TYPE_ENUM_VALUE(Name, Detail, Value) { TypeSub, Name, Detail, "", "", Value, 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, 0 },

//! MXFLIB_TYPE_ENUM_END - Use to end definition of a "Enumeration" type
#define MXFLIB_TYPE_ENUM_END

//! MXFLIB_LABEL - Use to define a label
/*! \param Name The name of the label being defined
 *  \param Detail A human readable description of the label
 *  \param UL The UL, or end-swapped UUID of thsi label
 */
#define MXFLIB_LABEL(Name, Detail, UL) { TypeLabel, Name, Detail, "", UL, NULL, 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, 0 },

//! MXFLIB_LABEL_SYM - Use to define a label and override the default symbol space
/*! \param Name The name of the label being defined
 *  \param Detail A human readable description of the label
 *  \param UL The UL, or end-swapped UUID of thsi label
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_LABEL_SYM(Name, Detail, UL, Sym) { TypeLabel, Name, Detail, "", UL, NULL, 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, Sym, 0 },

//! MXFLIB_MASKED_LABEL - Use to define a masked label
/*! \param Name The name of the label being defined
 *  \param Detail A human readable description of the label
 *  \param UL The UL, or end-swapped UUID of thsi label
 *  \param Mask The mask of bits that can change in this label
 */
#define MXFLIB_MASKED_LABEL(Name, Detail, UL, Mask) { TypeLabel, Name, Detail, "", UL, Mask, 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, 0 },

//! MXFLIB_MASKED_LABEL_SYM - Use to define a masked label and override the default symbol space
/*! \param Name The name of the label being defined
 *  \param Detail A human readable description of the label
 *  \param UL The UL, or end-swapped UUID of thsi label
 *  \param Mask The mask of bits that can change in this label
 *  \param Sym The name of the symbol space for this type
 */
#define MXFLIB_MASKED_LABEL_SYM(Name, Detail, UL, Mask, Sym) { TypeLabel, Name, Detail, "", UL, Mask, 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, Sym, 0 },


//! MXFLIB_TYPE_END - Use to end a type definition block
#define MXFLIB_TYPE_END { TypeNULL, "", "", "", "", "", 0, ARRAYIMPLICIT, TypeRefUndefined, NULL, NULL, 0 } };

//! UnknownType
const UInt8 UnknownType_UL_Data[16] = { 0x8f, 0x64, 0x35, 0x9b, 0xfe, 0x75, 0x36, 0x89, 0x8c, 0x4e, 0x57, 0x91, 0xcd, 0x68, 0x6c, 0xe4 };
const UL UnknownType_UL(UnknownType_UL_Data);


/* Example usage:
	MXFLIB_TYPE_START(TypeArray)
		MXFLIB_TYPE_BASIC("UInt8", "Unsigned 8 bit integer", "urn:x-ul:060E2B34.0104.0101.01010100.00000000", 1, false)
		MXFLIB_TYPE_BASIC("UInt16", "Unsigned 16 bit integer", "urn:x-ul:060E2B34.0104.0101.01010200.00000000", 2, true)
		MXFLIB_TYPE_BASIC("Int32", "32 bit integer", "urn:x-ul:060E2B34.0104.0101.01010700.00000000", 4, true)
		
		MXFLIB_TYPE_INTERPRETATION("VersionType", "Version number (created from major*256 + minor)", "UInt16", "urn:x-ul:060E2B34.0104.0101.03010300.00000000", 0)
		MXFLIB_TYPE_INTERPRETATION("UTF16", "Unicode UTF-16 coded character", "UInt16", "urn:x-ul:060E2B34.0104.0101.01100100.00000000", 0)
		MXFLIB_TYPE_INTERPRETATION("Boolean", "Boolean", "UInt8", "urn:x-ul:060E2B34.0104.0101.01040100.00000000", 0)

		MXFLIB_TYPE_MULTIPLE("UTF16String", "Unicode UTF-16 coded string", "UTF16", "urn:x-ul:060E2B34.0104.0101.01100200.00000000", false, 0)
		MXFLIB_TYPE_MULTIPLE("Int32Array", "Array of Int32 values", "Int32", "urn:x-ul:060E2B34.0104.0101.04010900.00000000", false, 0)
		MXFLIB_TYPE_MULTIPLE("Int32Batch", "Batch of Int32 values", "Int32", "urn:x-ul:060E2B34.0104.0101.04030200.00000000", true, 0)
		MXFLIB_TYPE_MULTIPLE("Int32Pair", "Pair of Int32 values", "Int32", "", false, 2)

		MXFLIB_TYPE_COMPOUND("Rational", "Rational", "urn:x-ul:060E2B34.0104.0101.03010100.00000000")
			MXFLIB_TYPE_COMPOUND_ITEM("Numerator", "Numerator", "Int32", "urn:x-ul:060E2B34.0104.0101.03010101.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Denominator", "Denominator", "Int32", "urn:x-ul:060E2B34.0104.0101.03010102.00000000", 0)
		MXFLIB_TYPE_COMPOUND_END
	MXFLIB_TYPE_END
*/
}


namespace mxflib
{
	//! Enumeration identifying the type of an in-memory type definition entry to be processed by LoadTypes()
	enum ClassType
	{
		ClassNULL = 0,						//!< Special token used to end a list of classes or the end of a set, pack or vector
		ClassPack,							//!< Pack definition
		ClassSet,							//!< Local set definition
		ClassVector,						//!< Vector (with 8-byte count and size header) definition
		ClassArray,							//!< Array definition
		ClassItem,							//!< Definition of an item in a set, pack, vector or array
		ClassRename,						//!< Rename of a set or pack without new children
		ClassExtend,						//!< Extend an existing set or pack definition
		ClassSymbolSpace					//!< Define the default symbol space for all classes in this list
	};

	//! Usage types for classes
	enum ClassUsage
	{
		ClassUsageNULL = 0,					//!< Not a usage - flag used to signify no usage supplied (or not required)
		ClassUsageOptional,					//!< Item is optional, if present the decoder may ignore it
		ClassUsageDecoderRequired,			//!< Item is optional, but if present the decoder must use this value
		ClassUsageEncoderRequired,			//!< Item must be encoded, but decoder may ignore the value
		ClassUsageRequired,					//!< Item must be encoded and the decoder must use this value
		ClassUsageBestEffort,				//!< As ClassUsageRequired, but an "Incomplete" set of metadata may signal unknown by using a distinguished value
		ClassUsageToxic,					//!< Item must not be encoded - kept for compatibility!
		ClassUsageDark						//!< Item is dark - no longer makes sense: kept for compatibility
	};

	//! Referencing types for classes - an exact copy of the type ref kinds
	typedef TypeRef ClassRef;

	//! ClassRef version of TypeRefUndefined
	const ClassRef ClassRefUndefined = TypeRefUndefined;

	//! ClassRef version of TypeRefNone
	const ClassRef ClassRefNone = TypeRefNone;

	//! ClassRef version of TypeRefStrong
	const ClassRef ClassRefStrong = TypeRefStrong;

	//! ClassRef version of TypeRefWeak
	const ClassRef ClassRefWeak = TypeRefWeak;

	//! ClassRef version of TypeRefMeta
	const ClassRef ClassRefMeta = TypeRefMeta;

	//! ClassRef version of TypeRefDict
	const ClassRef ClassRefDict = TypeRefDict;

	//! ClassRef version of TypeRefGlobal
	const ClassRef ClassRefGlobal = TypeRefGlobal;

	//! ClassRef version of TypeRefTarget
	const ClassRef ClassRefTarget = TypeRefTarget;

	/* Bit values for Class.Flags */
	const UInt8 ClassFlags_ExtendSubs = 1;	//! If this entry is extending a class, sub-classes should also be extended
	const UInt8 ClassFlags_Baseline = 2;	//! This entry is a baseline class as defined in 377M or certain other specific standards (and so will not be added to the KXS metadictionary)

	//! Single entry for a class to be defined - can be stored as a compile-time built structure
	struct ConstClassRecord
	{
		ClassType Class;					//!< The type of class being defined by this entry
		unsigned int MinSize;				//!< The minimum size of an item, or the tag size for a set (0 = no lower limit)
		unsigned int MaxSize;				//!< The maximum size of an item, or the length size/format for a set (0 = no upper limit)
		const char *Name;					//!< The name of this class
		const char *Detail;					//!< The human readable description of this type
		ClassUsage Usage;					//!< The usage type for this class
		const char *Base;					//!< The type of an item, or base type if redefining
		UInt16 Tag;							//!< The 2-byte tag, or zero
		const char *UL;						//!< The UL for this class or item (if known)
		const char *Default;				//!< Default value as a string, or NULL if none
		const char *DValue;					//!< Distinguished value as a string, or NULL if none
		ClassRef RefType;					//!< Reference type of this item (if a reference or target)
		const char *RefTarget;				//!< Type of the reference target (if this is a referencing type)
		const char *SymSpace;				//!< SymbolSpace for this class, or NULL if none specified (will inherit)
		UInt8 Flags;						//!< Flags for this entry as a bitfield
	};

	// Forward declare ClassRecord to allow ClassRecordPtr to be defined early
	class ClassRecord;

	//! A smart pointer to a ClassRecord
	typedef SmartPtr<ClassRecord> ClassRecordPtr;

	//! List of smart pointers to ClassRecords
	typedef std::list<ClassRecordPtr> ClassRecordList;

	//! List of ClassRecordLists
	typedef std::list<ClassRecordList> ClassRecordListList;

	//! Single entry for a class to be defined - built at run-time and can be stacked if required to allow out-of-order definitions
	class ClassRecord : public RefCount<ClassRecord>
	{
	public:
		ClassType Class;					//!< The type of class being defined by this entry
		unsigned int MinSize;				//!< The minimum size of an item, or the tag size for a set (0 = no lower limit)
		unsigned int MaxSize;				//!< The maximum size of an item, or the length size/format for a set (0 = no upper limit)
		std::string Name;					//!< The name of this class
		std::string Detail;					//!< The human readable description of this type
		ClassUsage Usage;					//!< The usage type for this class
		std::string Base;					//!< The type of an item, or base type if redefining
		UInt16 Tag;							//!< The 2-byte tag, or zero
		ULPtr UL;							//!< The UL for this class or item (or NULL if not known)
		bool HasDefault;					//!< True if the item has a default value
		std::string Default;				//!< Default value as a string
		bool HasDValue;						//!< True if the item has a distinguished value
		std::string DValue;					//!< Distinguished value as a string
		ClassRecordList Children;			//!< Sub-items within this class (if it is a set or pack)
		ClassRef RefType;					//!< Reference type of this item (if a reference or target)
		std::string RefTarget;				//!< Type of the reference target (if this is a referencing type)
		SymbolSpacePtr SymSpace;			//!< SymbolSpace for this class, or NULL if none specified (will inherit)
		bool IsBaseline;					//!< True if this is a baseline class as defined in 377M or certain other specific standards (and so will not be added to the KXS metadictionary)
		bool ExtendSubs;					//!< If this entry is extending a class, should sub-classes also be extended?
		ULPtr Parent;						//!< The parent of an orphaned child item
											/*!< This property is only used when a parent set or pack was built, but this child could not
											 *   be added at the time and is saved for later building */

	public:
		//! Build an empty ClassRecord
		ClassRecord()
		{
			Class = ClassNULL;
			MinSize = 0;
			MaxSize = 0;
			Usage = ClassUsageNULL;
			Tag = 0;
			HasDefault = false;
			HasDValue = false;
			RefType = ClassRefUndefined;
			IsBaseline = false;
			ExtendSubs = true;
		}
	};


	//! Load classes from the specified in-memory definitions
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	int LoadClasses(ClassRecordList &ClassesData, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols);

	//! Load classeses from the specified in-memory definitions
	/*! \note There must be enough terminating entries (with Class == TypeNULL) to end the list
	 *  \return 0 if all OK
	 *  \return -1 on error
	 */
	int LoadClasses(const ConstClassRecord *ClassData, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols);


	/* Define macros for static type definitions */

//! MXFLIB_CLASS_START - Use to start a class definition block
#define MXFLIB_CLASS_START(Name) const ConstClassRecord Name[] = {

//! MXFLIB_CLASS_START_SYM - Use to start a class definition block and define a default symbol space
#define MXFLIB_CLASS_START_SYM(Name, Sym) const ConstClassRecord Name[] = { { ClassSymbolSpace, 0, 0, "", "", ClassUsageNULL, "", 0, "", NULL, NULL, ClassRefUndefined, "", Sym, 0 },

//! MXFLIB_CLASS_SET - Use to define a local set that has 2-byte tags and 2-byte lengths
/*! \param Name The name of the set being defined
 *  \param Detail A human readable description of the set
 *  \param Base The base class if this set is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_SET(Name, Detail, Base, UL) { ClassSet, 2, 2, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_SET_EX - Use to define a local set with extended options
/*! \param Name The name of the set being defined
 *  \param Detail A human readable description of the set
 *  \param Base The base class if this set is a derived class, else ""
 *	\param TagSize the size of the local tag for each contained item
 *	\param LenSize the size of the length for each contained item, or 3 for BER
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *	\param Sym The name of the symbol space for this set
 *	\param Flags for this set
*/
#define MXFLIB_CLASS_SET_EX(Name, Detail, Base, TagSize, LenSize, UL, Sym, Flags) { ClassSet, TagSize, LenSize, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, Flags },

//! MXFLIB_CLASS_SET_SYM - Use to define a local set that has 2-byte tags and 2-byte lengths and override the default symbol space
/*! \param Name The name of the set being defined
 *  \param Detail A human readable description of the set
 *  \param Base The base class if this set is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *  \param Sym The name of the symbol space for this set
 */
#define MXFLIB_CLASS_SET_SYM(Name, Detail, Base, UL, Sym) { ClassSet, 2, 2, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_SET_NOSUB - Use to extend a local set that has 2-byte tags and 2-byte lengths, without extending sub-classes
/*! \param Name The name of the set being extended
 *  \param Detail A human readable description of the set
 *  \param Base The base class if this set is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_SET_NOSUB(Name, Detail, Base, UL) { ClassSet, 2, 2, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", NULL, 0 },

//! MXFLIB_CLASS_SET_NOSUB_SYM - Use to extend a local set that has 2-byte tags and 2-byte lengths, without extending sub-classes, and override the default symbol space
/*! \param Name The name of the set being extended
 *  \param Detail A human readable description of the set
 *  \param Base The base class if this set is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *  \param Sym The name of the symbol space for this set
 */
#define MXFLIB_CLASS_SET_NOSUB_SYM(Name, Detail, Base, UL, Sym) { ClassSet, 2, 2, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, 0 },

//! MXFLIB_CLASS_SET_END - Use to end a set definition
#define MXFLIB_CLASS_SET_END { ClassNULL, 0, 0, "", "", ClassUsageNULL, "", 0, "", NULL, NULL, ClassRefUndefined, "", NULL, 0 },

//! MXFLIB_CLASS_FIXEDPACK - Use to define a fixed length pack (defined length pack)
/*! \param Name The name of the pack being defined
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_FIXEDPACK(Name, Detail, Base, UL) { ClassPack, 0, 0, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_FIXEDPACK_EX - Use to define a fixed length pack (defined length pack) with extended options
/*! \param Name The name of the pack being defined
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *	\param Sym The name of the symbol space for this pack
 *	\param Flags for this pack 
 */
#define MXFLIB_CLASS_FIXEDPACK_EX(Name, Detail, Base, UL, Sym, Flags) { ClassPack, 0, 0, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, Flags },

//! MXFLIB_CLASS_FIXEDPACK_SYM - Use to define a fixed length pack (defined length pack) and override the default symbol space
/*! \param Name The name of the pack being defined
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *  \param Sym The name of the symbol space for this set
 */
#define MXFLIB_CLASS_FIXEDPACK_SYM(Name, Detail, Base, UL, Sym) { ClassPack, 0, 0, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_FIXEDPACK - Use to extend a fixed length pack (defined length pack), without extending sub-classes
/*! \param Name The name of the pack being extended
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_FIXEDPACK_NOSUB(Name, Detail, Base, UL) { ClassPack, 0, 0, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", NULL, 0 },

//! MXFLIB_CLASS_FIXEDPACK_SYM - Use to extend a fixed length pack (defined length pack), without extending sub-classes, and override the default symbol space
/*! \param Name The name of the pack being extended
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *  \param Sym The name of the symbol space for this set
 */
#define MXFLIB_CLASS_FIXEDPACK_NOSUB_SYM(Name, Detail, Base, UL, Sym) { ClassPack, 0, 0, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, 0 },

//! MXFLIB_CLASS_FIXEDPACK_END - Use to end a pack definition
#define MXFLIB_CLASS_FIXEDPACK_END { ClassNULL, 0, 0, "", "", ClassUsageNULL, "", 0, "", NULL, NULL, ClassRefUndefined, "", NULL, 0 },

//! MXFLIB_CLASS_VARIABLEPACK - Use to define a variable length pack (defined length pack)
/*! \param Name The name of the pack being defined
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_VARIABLEPACK(Name, Detail, LenSize, Base, UL) { ClassPack, 0, LenSize, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_VARIABLEPACK_EX - Use to define a variable length pack (defined length pack) with extended options
/*! \param Name The name of the pack being defined
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *	\param Sym The name of the symbol space for this pack
 *	\param Flags for this pack 
 */
#define MXFLIB_CLASS_VARIABLEPACK_EX(Name, Detail, LenSize, Base, UL, Sym, Flags) { ClassPack, 0, LenSize, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, Flags },

//! MXFLIB_CLASS_VARIABLEPACK_SYM - Use to define a variable length pack (defined length pack) and override the default symbol space
/*! \param Name The name of the pack being defined
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *  \param Sym The name of the symbol space for this pack
 */
#define MXFLIB_CLASS_VARIABLEPACK_SYM(Name, Detail, LenSize, Base, UL, Sym) { ClassPack, 0, LenSize, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_VARIABLEPACK - Use to extend a variable length pack (defined length pack), without extending sub-classes
/*! \param Name The name of the pack being defined
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_VARIABLEPACK_NOSUB(Name, Detail, LenSize, Base, UL) { ClassPack, 0, LenSize, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", NULL, 0 },

//! MXFLIB_CLASS_VARIABLEPACK_SYM - Use to extend a variable length pack (defined length pack), without extending sub-classes, and override the default symbol space
/*! \param Name The name of the pack being defined
 *  \param Detail A human readable description of the pack
 *  \param Base The base class if this pack is a derived class, else ""
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *  \param Sym The name of the symbol space for this pack
 */
#define MXFLIB_CLASS_VARIABLEPACK_NOSUB_SYM(Name, Detail, LenSize, Base, UL, Sym) { ClassPack, 0, LenSize, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, 0 },

//! MXFLIB_CLASS_VARIABLEPACK_END - Use to end a pack definition
#define MXFLIB_CLASS_VARIABLEPACK_END { ClassNULL, 0, 0, "", "", ClassUsageNULL, "", 0, "", NULL, NULL, ClassRefUndefined, "", NULL, 0 },

//! MXFLIB_CLASS_EXTEND - Use to define a set or pack as a derived class 
/*! \param Name The name of the pack being extended
 *  \param Detail A human readable description of the class (if sub-classing rather than adding to)
 *  \param Base The base class if this pack is a derived class
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_EXTEND(Name, Detail, Base, UL) { ClassExtend, DICT_KEY_UNDEFINED, DICT_LEN_UNDEFINED, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_EXTEND_EX - Use to define a set or pack extention with extended options
/*! \param Name The name of the pack being extended
 *  \param Detail A human readable description of the class (if sub-classing rather than adding to)
 *  \param Base The base class if this pack is a derived class
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *  \param Sym The name of the symbol space of the pack being extended
 *	\param Flags to apply during extension process (not all will be applicable)
 */
#define MXFLIB_CLASS_EXTEND_EX(Name, Detail, Base, UL, Sym, Flags) { ClassExtend, DICT_KEY_UNDEFINED, DICT_LEN_UNDEFINED, Name, Detail, ClassUsageNULL, Base, 0, UL, NULL, NULL, ClassRefUndefined, "", Sym, Flags },

//! MXFLIB_CLASS_EXTEND_END - Use to end a set or pack extention
#define MXFLIB_CLASS_EXTEND_END { ClassNULL, 0, 0, "", "", ClassUsageNULL, "", 0, "", NULL, NULL, ClassRefUndefined, "", NULL, 0 },

//! MXFLIB_CLASS_ITEM - Use to define a single item in a set or pack
/*! \param Name The name of the item being defined
 *  \param Detail A human readable description of the item
 *  \param Usage The usage type of this item (ClassUsageRequired, ClassUsageOptional, etc.)
 *  \param Type The type of this item
 *  \param Tag The tag for this item as a string of hex bytes e.g. "03 2b" (if in a set, else "")
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *	\param Default The default value for this item as a string (or NULL if none)
 *	\param DValue The distinguished value for this item as a string (or NULL if none)
 */
#define MXFLIB_CLASS_ITEM(Name, Detail, Usage, Type, MinSize, MaxSize, Tag, UL, Default, DValue) { ClassItem, MinSize, MaxSize, Name, Detail, Usage, Type, Tag, UL, Default, DValue, ClassRefUndefined, "", NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_ITEM_EX - Use to define a single item in a set or pack, with extended options
/*! \param Name The name of the item being defined
 *  \param Detail A human readable description of the item
 *  \param Usage The usage type of this item (ClassUsageRequired, ClassUsageOptional, etc.)
 *  \param Type The type of this item
 *  \param Tag The tag for this item as a string of hex bytes e.g. "03 2b" (if in a set, else "")
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 *	\param Default The default value for this item as a string (or NULL if none)
 *	\param DValue The distinguished value for this item as a string (or NULL if none)
 *  \param Sym The name of the symbol space for this item
 *	\param Flags The flags for this item
 */
#define MXFLIB_CLASS_ITEM_EX(Name, Detail, Usage, Type, MinSize, MaxSize, Tag, UL, RefType, RefTarget, Default, DValue, Sym, Flags) { ClassItem, MinSize, MaxSize, Name, Detail, Usage, Type, Tag, UL, Default, DValue, RefType, RefTarget, Sym, Flags },

//! MXFLIB_CLASS_ITEM_REF - Use to define a single item in a set or pack that is a reference source or target
/*! \param Name The name of the item being defined
 *  \param Detail A human readable description of the item
 *  \param Usage The usage type of this item (ClassUsageRequired, ClassUsageOptional, etc.)
 *  \param Type The type of this item
 *  \param Tag The tag for this item as a string of hex bytes e.g. "03 2b" (if in a set, else "")
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 *	\param Default The default value for this item as a string (or NULL if none)
 *	\param DValue The distinguished value for this item as a string (or NULL if none)
 */
#define MXFLIB_CLASS_ITEM_REF(Name, Detail, Usage, Type, MinSize, MaxSize, Tag, UL, RefType, RefTarget, Default, DValue) { ClassItem, MinSize, MaxSize, Name, Detail, Usage, Type, Tag, UL, Default, DValue, RefType, RefTarget, NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_ITEM_SYM - Use to define a single item in a set or pack and override the default symbol space
/*! \param Name The name of the item being defined
 *  \param Detail A human readable description of the item
 *  \param Usage The usage type of this item (ClassUsageRequired, ClassUsageOptional, etc.)
 *  \param Type The type of this item
 *  \param Tag The tag for this item as a string of hex bytes e.g. "03 2b" (if in a set, else "")
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *	\param Default The default value for this item as a string (or NULL if none)
 *	\param DValue The distinguished value for this item as a string (or NULL if none)
 *  \param Sym The name of the symbol space for this item
 */
#define MXFLIB_CLASS_ITEM_SYM(Name, Detail, Usage, Type, MinSize, MaxSize, Tag, UL, Default, DValue, Sym) { ClassItem, MinSize, MaxSize, Name, Detail, Usage, Type, Tag, UL, Default, DValue, ClassRefUndefined, "", Sym, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_ITEM_REF_SYM - Use to define a single item in a set or pack that is a reference source or target and override the default symbol space
/*! \param Name The name of the item being defined
 *  \param Detail A human readable description of the item
 *  \param Usage The usage type of this item (ClassUsageRequired, ClassUsageOptional, etc.)
 *  \param Type The type of this item
 *  \param Tag The tag for this item as a string of hex bytes e.g. "03 2b" (if in a set, else "")
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 *	\param Default The default value for this item as a string (or NULL if none)
 *	\param DValue The distinguished value for this item as a string (or NULL if none)
 *  \param Sym The name of the symbol space for this item
 */
#define MXFLIB_CLASS_ITEM_REF_SYM(Name, Detail, Usage, Type, MinSize, MaxSize, Tag, UL, RefType, RefTarget, Default, DValue, Sym) { ClassItem, MinSize, MaxSize, Name, Detail, Usage, Type, Tag, UL, Default, DValue, RefType, RefTarget, Sym, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_VECTOR - Use to define a vector holding items
/*! \param Name The name of the vector being defined
 *  \param Detail A human readable description of the vector
 *  \param Usage The usage type of this vector (ClassUsageRequired, ClassUsageOptional, etc.)
 *  \param Tag The tag for this vector as a string of hex bytes e.g. "03 2b" (if in a set, else "")
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_VECTOR(Name, Detail, Usage, Tag, UL) { ClassVector, 0, 0, Name, Detail, Usage, "", Tag, UL, NULL, NULL, ClassRefUndefined, "", NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_VECTOR_REF - Use to define a vector holding items that are reference sources or targets
/*! \param Name The name of the vector being defined
 *  \param Detail A human readable description of the vector
 *  \param Usage The usage type of this vector (ClassUsageRequired, ClassUsageOptional, etc.)
 *  \param Tag The tag for this vector as a string of hex bytes e.g. "03 2b" (if in a set, else "")
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 *	\param RefType The reference type (RefWeak, RefStrong or RefTarget)
 *  \param RefTarget The type of the reference target
 */
#define MXFLIB_CLASS_VECTOR_REF(Name, Detail, Usage, Tag, UL, RefType, RefTarget) { ClassVector, 0, 0, Name, Detail, Usage, "", Tag, UL, NULL, NULL, RefType, RefTarget, NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_VECTOR_END - Use to end a vector definition
#define MXFLIB_CLASS_VECTOR_END { ClassNULL, 0, 0, "", "", ClassUsageNULL, "", 0, "", NULL, NULL, ClassRefUndefined, "", NULL, 0 },

//! MXFLIB_CLASS_ARRAY - Use to define an array holding items
/*! \param Name The name of the array being defined
 *  \param Detail A human readable description of the array
 *  \param Usage The usage type of this array (ClassUsageRequired, ClassUsageOptional, etc.)
 *  \param Tag The tag for this array as a string of hex bytes e.g. "03 2b" (if in a set, else "")
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_ARRAY(Name, Detail, Usage, Tag, UL) { ClassArray, 0, 0, Name, Detail, Usage, "", Tag, UL, NULL, NULL, ClassRefUndefined, "", NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_ARRAY - Use to define an array holding items
/*! \param Name The name of the array being defined
 *  \param Detail A human readable description of the array
 *  \param Usage The usage type of this array (ClassUsageRequired, ClassUsageOptional, etc.)
 *  \param Tag The tag for this array as a string of hex bytes e.g. "03 2b" (if in a set, else "")
 *  \param UL The UL of this class as a hex string e.g. "06 0e 2b 34 etc." (if one exists, else "")
 */
#define MXFLIB_CLASS_ARRAY_REF(Name, Detail, Usage, Tag, UL, RefType, RefTarget) { ClassArray, 0, 0, Name, Detail, Usage, "", Tag, UL, NULL, NULL, RefType, RefTarget, NULL, ClassFlags_ExtendSubs },

//! MXFLIB_CLASS_ARRAY_END - Use to end a array definition
#define MXFLIB_CLASS_ARRAY_END { ClassNULL, 0, 0, "", "", ClassUsageNULL, "", 0, "", NULL, NULL, ClassRefUndefined, "", NULL, 0 },

//! MXFLIB_CLASS_END - Use to end a class definition block
#define MXFLIB_CLASS_END { ClassNULL, 0, 0, "", "", ClassUsageNULL, "", 0, "", NULL, NULL, ClassRefUndefined, "", NULL, 0 } };

//! Unknown_UL
const UInt8 Unknown_UL_Data[16] = { 0xe0, 0x72, 0xd2, 0x17, 0xef, 0x1e, 0xfb, 0x6e, 0x9b, 0x9e, 0xe1, 0xa8, 0x83, 0xb6, 0x4b, 0x3c };
const UL Unknown_UL(Unknown_UL_Data);

//! UnknownSet_UL
const UInt8 UnknownSet_UL_Data[16] = { 0xe0, 0x72, 0xd2, 0x17, 0xef, 0x1e, 0xfb, 0x6e, 0x9b, 0x9e, 0xe1, 0xa8, 0x83, 0xb6, 0x4b, 0x3d };
const UL UnknownSet_UL(UnknownSet_UL_Data);

/* Example usage:
	MXFLIB_CLASS_RENAME("KLVFill", "KLV Filler packet", "RAW", "06 0E 2B 34 01 01 01 01 03 01 02 10 01 00 00 00")
	MXFLIB_CLASS_FIXEDPACK("PartitionMetadata", "Identifies a Partition Pack", "")
		MXFLIB_CLASS_ITEM("MajorVersion", "Major Version number of MXF byte-level format (non-backwards compatible version number)", ClassUsageRequired, "UInt16", 2, 2, "06 0e 2b 34 01 01 01 04  03 01 02 01 06 00 00 00", "", "0001", "")
		MXFLIB_CLASS_ITEM("MinorVersion", "Minor Version number of MXF byte-level format (backwards compatible version number)", ClassUsageRequired, "UInt16", 2, 2, "06 0e 2b 34 01 01 01 04  03 01 02 01 06 00 00 00", "", "0001", "")
...
		MXFLIB_CLASS_ITEM_REF("PrimaryPackage", "The primary Package in this file", ClassUsageOptional, "UUID", 16, 16, "06 0e 2b 34 01 01 01 04  06 01 01 04 01 08 00 00", "3b 08", "", "", RefWeak, "GenericPackage")
...
		MXFLIB_CLASS_VECTOR("EssenceContainers", "The unordered batch of Universal Labels of Essence Containers used in or referenced by this file", ClassUsageRequired, "06 0e 2b 34 01 01 01 05  01 02 02 10 02 01 00 00", "", "")
			MXFLIB_CLASS_ITEM("EssenceContainer", "Universal Labels of Essence Container", ClassUsageRequired, "Label", 16, 16, "", "", "", "")
		MXFLIB_CLASS_VECTOR_END
	MXFLIB_CLASS_END

	MXFLIB_CLASS_RENAME("OpenHeader", "Open Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 01 00")
	MXFLIB_CLASS_RENAME("OpenCompleteHeader", "Open Complete Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 03 00")

 <PrimaryPackage detail="The primary Package in this file" use="optional" type="UUID" minLength="16" maxLength="16" key="3b 08" globalKey="06 0e 2b 34 01 01 01 04  06 01 01 04 01 08 00 00" ref="weak" target="GenericPackage"/>
*/
}


namespace mxflib
{
	//! Enumeration identifying the type of an in-memory dictionary definition entry to be processed by LoadDictionary()
	enum DictionaryType
	{
		DictionaryNULL = 0,					//!< Special token used to end a list of dictionary entries
		DictionaryTypes,					//!< Types dictionary befinition
		DictionaryClasses					//!< Class dictionary definition
	};

	//! Single entry for a dictionary to be defined - can be stored as a compile-time built structure
	struct ConstDictionaryRecord
	{
		DictionaryType Type;				//!< The type of dictionary being defined by this entry
		const void *Dict;					//!< Pointer to either a ConstTypeRecord array or a ConstClassRecord array holding the dictionary to load
	};

	// Forward declare Dictionary to allow DictionaryPtr to be defined early
	class Dictionary;

	//! A smart pointer to a Dictionary
	typedef SmartPtr<Dictionary> DictionaryPtr;

	//! List of smart pointers to Dictionary objects
	typedef std::list<DictionaryPtr> DictionaryList;

	//! Run-time dictionary definition - built from other run-time record definitions
	class Dictionary : public RefCount<Dictionary>
	{
	public:
		TypeRecordListList Types;			//!< All the types to define
		ClassRecordListList Classes;		//!< All the classes to define
	};


	//! Load dictionary from the specified in-memory definitions with a default symbol space
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	int LoadDictionary(DictionaryPtr &DictionaryData, SymbolSpacePtr DefaultSymbolSpace, bool FastFail = false);

	//! Load dictionary from the specified in-memory definitions
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	inline int LoadDictionary(DictionaryPtr &DictionaryData, bool FastFail = false, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols)
	{
		return LoadDictionary(DictionaryData, DefaultSymbolSpace, FastFail );
	}

	//! Load dictionary from the specified in-memory definitions with a default symbol space
	/*! \note There must be a terminating entry (with Type == DictionaryNULL) to end the list
	 *  \return 0 if all OK
	 *  \return -1 on error
	 */
	int LoadDictionary(const ConstDictionaryRecord *DictionaryData, SymbolSpacePtr DefaultSymbolSpace, bool FastFail = false);

	//! Load dictionary from the specified in-memory definitions
	/*! \note There must be a terminating entry (with Type == DictionaryNULL) to end the list
	 *  \return 0 if all OK
	 *  \return -1 on error
	 */
	inline int LoadDictionary(const ConstDictionaryRecord *DictionaryData, bool FastFail = false, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols)
	{
		return LoadDictionary(DictionaryData, DefaultSymbolSpace, FastFail);
	}

	//! Load dictionary from the specified XML definitions with a default symbol space
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	int LoadDictionary(const char *DictFile, SymbolSpacePtr DefaultSymbolSpace, std::string Application, bool FastFail = false);

	//! Load dictionary from the specified XML definitions with a default symbol space
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	inline int LoadDictionary(const char *DictFile, SymbolSpacePtr DefaultSymbolSpace, bool FastFail = false)
	{
		return LoadDictionary(DictFile, DefaultSymbolSpace, "", FastFail );
	}

	//! Load dictionary from the specified XML definitions with a default symbol space
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	inline int LoadDictionary(std::string DictFile, SymbolSpacePtr DefaultSymbolSpace, bool FastFail = false)
	{
		return LoadDictionary(DictFile.c_str(), DefaultSymbolSpace, "", FastFail );
	}

	//! Load dictionary from the specified XML definitions
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	inline int LoadDictionary(const char *DictFile, bool FastFail = false, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols)
	{
		return LoadDictionary(DictFile, DefaultSymbolSpace, "", FastFail);
	}

	//! Load dictionary from the specified XML definitions
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	inline int LoadDictionary(std::string DictFile, bool FastFail = false, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols)
	{
		return LoadDictionary(DictFile.c_str(), DefaultSymbolSpace, "", FastFail);
	}

	//! Load dictionary from the specified XML definitions with a default symbol space
	/*! \return 0 if all OK
	*  \return -1 on error
	*/
	int LoadDictionaryFromXML(std::string & strXML, std::string Application, bool FastFail = false);

	//! Load dictionary from the specified XML definitions with a default symbol space
	/*! \return 0 if all OK
	*  \return -1 on error
	*/
	inline int LoadDictionaryFromXML(std::string & strXML, bool FastFail = false)
	{
		return LoadDictionaryFromXML(strXML, "", FastFail);
	}


//! MXFLIB_DICTIONARY_START - Use to start a type definition block
#define MXFLIB_DICTIONARY_START(Name)		const ConstDictionaryRecord Name[] = {

//! MXFLIB_DICTIONARY_TYPES - Add a set of types to the current dictionary
#define MXFLIB_DICTIONARY_TYPES(Types)		{ DictionaryTypes, Types },

//! MXFLIB_DICTIONARY_CLASSES - Add a set of classes to the current dictionary
#define MXFLIB_DICTIONARY_CLASSES(Classes)	{ DictionaryClasses, Classes },

//! MXFLIB_DICTIONARY_END - Use to end a dictionary definition block
#define MXFLIB_DICTIONARY_END				{ DictionaryNULL, NULL } };

/* Example usage:
	MXFLIB_DICTIONARY_START(Dict)
		MXFLIB_DICTIONARY_TYPES(MyTypes)
		MXFLIB_DICTIONARY_CLASSES(MyClasses)
	MXFLIB_DICTIONARY_END
*/
}


// DRAGONS: namespace mxflib_deftypes is semi-privat eto allow legacytypes.cpp to access items
namespace mxflib_deftypes
{
	//! Set when built-in traits need to be loaded
	extern bool LoadBuiltInTraits;

	//! Build the map of all known traits
	void DefineTraits(void);
};

#endif // MXFLIB__DICTIONARY_H

