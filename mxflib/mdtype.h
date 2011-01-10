/*! \file	mdtype.h
 *	\brief	Definition of classes that define metadata type info
 *
 *			Class MDDict holds the overall dictionary definitions and
 *          manages loading them from a dictionary file and adding new
 *			metadata types.
 *<br><br>
 *			Class MDType holds info about a specific metadata type
 *<br><br>
 *			These classes are currently wrappers around KLVLib structures
 *
 *	\version $Id: mdtype.h,v 1.14 2011/01/10 10:42:09 matt-beard Exp $
 *
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
#ifndef MXFLIB__MDTYPE_H
#define MXFLIB__MDTYPE_H

// STL Includes
#include <string>
#include <list>
#include <map>

namespace mxflib
{
	typedef std::list<std::string> StringList;
}


namespace mxflib
{
	//! Number/String duality object for index item in objects
	/*! Number facet is used for arrays, String facet for compounds
	 */
	class MapIndex
	{
	public:
		bool IsNum;
		UInt32 Number;
		std::string String;

	protected:
		MapIndex();

	public:
		MapIndex(UInt32 Num) { IsNum = true; Number = Num; String = Int2String(Num); };
		MapIndex(std::string Str) { IsNum = false; String = Str; };
		MapIndex(const MapIndex& Map) 
		{ IsNum = Map.IsNum; 
			if(IsNum) Number = Map.Number; 
			String = Map.String; 
		};

		bool operator<(const MapIndex& Other) const
		{
			if((IsNum==true) && (Other.IsNum==true)) return (Number < Other.Number);
			if((IsNum==false) && (Other.IsNum==false)) return (String < Other.String);
			
			// Numbers come before strings
			return Other.IsNum;
		}

		//! Allocator which does not change underlying type
		MapIndex& operator=(UInt32 Num) 
		{ 
			if(IsNum) { Number = Num; String = Int2String(Num); } 
			return *this;
		};

		//! Allocator which does not change underlying type
		MapIndex& operator=(std::string Str) 
		{ 
			if(IsNum) 
			{
				Number = atoi(Str.c_str());
				String = Int2String(Number);		// Reformat the string
			}
			else
			{
				String = Str;
			}
			return *this;
		};

		//! Allocator which does not change underlying type
		MapIndex& operator=(const MapIndex& Map)
		{ 
			if(IsNum)
			{
				Number = atoi(Map.String.c_str());	// Will be zero in !Map.IsNum
				String = Int2String(Number);		// Reformat the string
			}
			else
			{
				String = Map.String;
			}
			return *this;
		};

		//! Cast to a char string
		const char *c_str() const
		{
			return (const char *)String.c_str();
		}

		//! Equality check
		bool operator==(const MapIndex& Other) const
		{
			if(Other.IsNum != IsNum) return false;
			if(IsNum) return (Number == Other.Number);
			return (String == Other.String);
		}
	};
}


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDType;

	//! A smart pointer to an MDType object
	typedef SmartPtr<MDType> MDTypePtr;

	//! A parent pointer to an MDType object
	typedef ParentPtr<MDType> MDTypeParent;

	//! A list of smart pointers to MDType objects
	typedef std::list<MDTypePtr> MDTypeList;

	//! A map of MDTypes indexed by name
	typedef std::map<std::string, MDTypePtr> MDTypeMap;

	//! A map of MDTypes by UL
	typedef std::map<UL, MDTypePtr> MDTypeULMap;
}


namespace mxflib
{
	//! Holds the definition of a metadata type
	class MDType : public RefCount<MDType> , public MDTypeMap,
				   public IMDTypeGetCommon, 
				   public IMDTypeGet, 
				   public IMDTypeFind,
				   public IMDTypeSet, 
				   public IMDEffectiveType<MDType, MDTypePtr>, 
				   public IMDTypeChild<MDTypePtr, MDTypeList>,
				   public IMDTraitsAccess
	{
	protected:
		std::string TypeName;			//!< Name of this MDType
		std::string	Detail;				//!< Meaningful description
		MDTypeClass Class;				//!< Class of this MDType
		MDArrayClass ArrayClass;		//!< Sub-class of array
		MDTraitsPtr Traits;				//!< Traits for this MDType
		ULPtr TypeUL;					//!< The UL for this type
		bool Endian;					//!< Flag set to 'true' if this basic type should ever be byte-swapped
		TypeRef RefType;				//!< Reference type of this type (if a reference source or target), if TypeRefUndefined then inherit
		std::string RefTarget;			//!< Reference target of this type (if a reference source), if "" then inherit
		MDOTypePtr RefTargetType;		//!< The MDOType that is the target of this type (if known)
		bool Baseline;					//!< True if this is a baseline type as defined in 377M or certain other specific standards (and so will not be added to the KXS metadictionary)
		bool Character;					//!< True if this is a "character" type, this information is only required for the metadictionary

	protected:
		NamedValueList EnumValues;		//!< List of enumerated values, if this is an enum, each with its value name
		MDTypeList ChildList;			//!< Child names in order for compound types

	public:
		MDTypeParent Base;				//!< Base type if this is a derived type, else NULL
		StringList ChildOrder;			//!< DEPRECATED: Child names in order for compound types
		int Size;						//!< The size of the item in multiples of base class items, or 0 if it is variable
		bool Signed;

	protected:
		//!	Construct a basic MDType
		/*! This constructor is protected so the ONLY way to create
		 *	new MDTypes from outside this class is via AddBasic() etc.
		*/
		MDType(std::string TypeName, std::string Detail, MDTypeClass TypeClass, ULPtr &UL, MDTraitsPtr TypeTraits)
			: TypeName(TypeName), Detail(Detail), Class(TypeClass), ArrayClass(ARRAYIMPLICIT), Traits(TypeTraits), TypeUL(UL), Endian(false),
			  RefType(TypeRefUndefined), Baseline(false), Character(false)
		{ };
 
		//! Prevent auto construction by NOT having an implementation to this constructor
		MDType();

		//! Prevent copy construction by NOT having an implementation to this copy constructor
		MDType(const MDType &rhs);

		//! Add a sub to a compound type
		void AddSub(std::string SubName, MDTypePtr SubType);

	public:

		/* Interface IMDTypeGetCommon */
		/******************************/

		//! Get the name of this type
		const std::string &Name(void) const { return TypeName; }

		//! Get the full name of this type, including all parents
		std::string FullName(void) const
		{
			if((Class == COMPOUND) && Base) return Base->TypeName + "/" + TypeName;
			return TypeName;
		}

		//! Report the detailed description for this type
		const std::string &GetDetail(void) const { return Detail; }
		

		/* Interface IMDTypeGet */
		/************************/

		//! Is this a "character" type
		bool IsCharacter(void) const { return Character; }

		//! Endian access function (get)
		bool GetEndian(void) const { return Endian; }

		//! Baseline access function (get)
		bool IsBaseline(void) const { return Baseline; }

		//! Get the size of this type, in bytes if basic, or items if a multiple
		/*! DRAGONS: This gets the defined size for this type, not the size of the current value */
		int GetSize(void) const { return Size; }

		//! Get a const reference to the enum values
		const NamedValueList &GetEnumValues(void) const 
		{
			if(Class == INTERPRETATION) return Base->GetEnumValues();
			return EnumValues; 
		};

		//! Get the class of this type
		MDTypeClass GetClass(void) const { return Class; }

		//! ArrayClass access function (get)
		MDArrayClass GetArrayClass(void) const { return ArrayClass; };

		//! Get the reference type
		TypeRef GetRefType(void) const { return RefType; }

		//! Get the reference target
		const MDOTypePtr &GetRefTarget(void) const
		{
			return RefTargetType;
		}

		//! Get the reference target
		std::string GetRefTargetName(void) const
		{
			return RefTarget;
		}


		/* Interface IMDTypeFind */
		/*************************/

	public:
		/* Static MDType finding methods */

		//! Find a type in the default symbol space, optionally searching all others
		static MDTypePtr Find(std::string BaseType, bool SearchAll = false) { return Find(BaseType, MXFLibSymbols, SearchAll); }

		//! Find a type in a specified symbol space, optionally searching all others
		static MDTypePtr Find(std::string BaseType, SymbolSpacePtr &SymSpace, bool SearchAll = false);

		//! Find a type with a given UL
		static MDTypePtr Find(const UL& BaseUL);

		//! Find a type by ULPtr
		static MDTypePtr Find(ULPtr &BaseUL) { return Find(*BaseUL); }


		/* Interface IMDTypeSet */
		/************************/

		//! Set "character" type flag
		void SetCharacter(bool Val) { Character = Val; }

		//! Endian access function (set)
		void SetEndian(bool Val) { Endian = Val; }

		//! Baseline access function (set)
		void SetBaseline(bool Val = true) { Baseline = Val; }

		//! ArrayClass access function (set)
		void SetArrayClass(MDArrayClass Val) { ArrayClass = Val; }

		//! Set the reference type
		void SetRefType(TypeRef Val) { RefType = Val; }

		//! Set the reference target
		void SetRefTarget(std::string Val) { RefTarget = Val; }

		//! Insert a new child type
		/*! DRAGONS: This overlaods the insert() methods of the base map */
		std::pair<iterator, bool> insert(std::string Name, MDTypePtr NewType) 
		{
			// Add the child to the order lists
			ChildOrder.push_back(Name);
			ChildList.push_back(NewType);

			// Add this child item
			return MDTypeMap::insert(MDTypeMap::value_type(Name, NewType));
		}


		/* Interface IMDEffectiveType */
		/******************************/

		//! Report the effective type of this type
		const MDType *EffectiveType(void) const;

		//! Report the effective class of this type
		MDTypeClass EffectiveClass(void) const;

		//! Report the effective base type of this type
		MDTypePtr EffectiveBase(void) const;

		//! Report the effective reference type of this type
		TypeRef EffectiveRefType(void) const;

		//! Report the effective reference target of this type
		MDOTypePtr EffectiveRefTarget(void) const;

		//! Report the name of the effective reference target of this type
		/*! DRAGONS: To be used when loading dictionary only */
		std::string EffectiveRefTargetName(void) const;

		//! Report the effective size of this type
		/*! \return The size in bytes of a single instance of this type, or 0 if variable size
		 */
		UInt32 EffectiveSize(void) const { return EffectiveSizeInternal(); }

		//! Report the effective size of this type - internal recursive version
		UInt32 EffectiveSizeInternal(bool OverrideSize = false, int UseSize = 0) const;



		/* Interface IMDTypeChild */
		/**************************/

		//! Read-only access to ChildList
		const MDTypeList &GetChildList(void) const
		{
			return ChildList;
		}

		//! Locate a named child
		MDTypePtr Child(const std::string Name) const;

		//! Locate a named child
		MDTypePtr operator[](const std::string Name) const { return Child(Name); }

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MDTypePtr Child(int Index) const;

		//! Locate a numerically indexed child
		/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
		MDTypePtr operator[](int Index) const { return Child(Index); }

		//! Locate a child by UL
		MDTypePtr Child(ULPtr &ChildType) const;

		//! Locate a child by UL
		MDTypePtr operator[](ULPtr &ChildType) const { return Child(ChildType); };

		//! Locate a child by UL
		MDTypePtr Child(const UL &ChildType) const;

		//! Locate a child by UL
		MDTypePtr operator[](const UL &ChildType) const { return Child(ChildType); };


		/* Interface IMDTraits */
		/***********************/

		//! Set the traits for this type or object
		void SetTraits(MDTraitsPtr Tr) { Traits = Tr; };

		//! Access the traits for this type or object
		const MDTraitsPtr &GetTraits(void) const { return Traits; }

		//! Does this value's trait take control of all sub-data and build values in the our own DataChunk?
		/*! Normally any contained sub-types (such as array items or compound members) hold their own data */
		bool HandlesSubdata(void) const
		{
			if(Traits) return Traits->HandlesSubdata();
			return false;
		}


	public:
		/* Static traits methods */

		//! Add a mapping to be applied to all types of a given type name
		/*! \note This will act retrospectively
		 */
		static bool AddTraitsMapping(std::string TypeName, std::string TraitsName);

		//! Update an existing mapping and apply to any existing type of the given name
		static bool UpdateTraitsMapping(std::string TypeName, std::string TraitsName)
		{
			// DRAGONS: For the moment this does exactly the same ass AddTraitsMapping - it may differ in future versions
			return AddTraitsMapping(TypeName, TraitsName);
		}

		//! Add a mapping to be applied to all types of a given type UL
		/*! \note This will act retrospectively
		 */
		static bool AddTraitsMapping(const UL &TypeUL, std::string TraitsName);

		//! Update an existing mapping and apply to any existing type of the given UL
		static bool UpdateTraitsMapping(const UL &TypeUL, std::string TraitsName)
		{
			// DRAGONS: For the moment this does exactly the same ass AddTraitsMapping - it may differ in future versions
			return AddTraitsMapping(TypeUL, TraitsName);
		}

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



		/***********************************/


		//! Get the base of this type
		const MDTypeParent &GetBase(void) const { return Base; }

		//! Read-only access to the type UL
		const ULPtr &GetTypeUL(void) const { return TypeUL; }


	//** Static Dictionary Handling data and functions **
	//***************************************************
	protected:
		static MDTypeList Types;		//!< All types managed by this object

		//! Map for UL lookups
		static std::map<UL, MDTypePtr> ULLookup;
		
		//! Map for UL lookups - ignoring the version number (all entries use version = 1)
		static std::map<UL, MDTypePtr> ULLookupVer1;

		//! Map for reverse lookups based on type name
		static MDTypeMap NameLookup;

	public:
		//! Add a new basic type
		static MDTypePtr AddBasic(std::string TypeName, std::string Detail, ULPtr &UL, int TypeSize);

		//! Add a new basic type without detailed description ** DEPRECATED **
		static MDTypePtr AddBasic(std::string TypeName, ULPtr &UL, int TypeSize) { return MDType::AddBasic(TypeName, TypeName, UL, TypeSize); }

		//! Add a new interpretation type
		static MDTypePtr AddInterpretation(std::string TypeName, std::string Detail, MDTypePtr BaseType, ULPtr &UL, int Size = 0);

		//! Add a new interpretation type without detailed description ** DEPRECATED **
		static MDTypePtr AddInterpretation(std::string TypeName, MDTypePtr BaseType, ULPtr &UL, int Size = 0) { return MDType::AddInterpretation(TypeName, TypeName, BaseType, UL, Size); }

		//! Add a new array type
		static MDTypePtr AddArray(std::string TypeName, std::string Detail, MDTypePtr BaseType, ULPtr &UL, int ArraySize = 0);

		//! Add a new array type without detailed description ** DEPRECATED **
		static MDTypePtr AddArray(std::string TypeName, MDTypePtr BaseType, ULPtr &UL, int ArraySize = 0) { return MDType::AddArray(TypeName, TypeName, BaseType, UL, ArraySize); }

		//! Add a new compound type
		static MDTypePtr AddCompound(std::string TypeName, std::string Detail, ULPtr &UL);

		//! Add a new compound type without detailed description ** DEPRECATED **
		static MDTypePtr AddCompound(std::string TypeName, ULPtr &UL) { return MDType::AddCompound(TypeName, TypeName, UL); }

		//! Add a new enumerated value type
		static MDTypePtr AddEnum(std::string TypeName, std::string Detail, MDTypePtr BaseType, ULPtr &UL);

		//! Add a new enumerated value type without detailed description ** DEPRECATED **
		static MDTypePtr AddEnum(std::string TypeName, MDTypePtr BaseType, ULPtr &UL) { return MDType::AddEnum(TypeName, TypeName, BaseType, UL); }

		//! Add a new value to an enumerated value type
		bool AddEnumValue(std::string Name, MDObjectPtr &Value);

		//! Add a new value to an enumerated value type
		bool AddEnumValue(std::string Name, std::string Value);

		//! Add a new UL value to an enumerated value type
		bool AddEnumValue(std::string Name, ULPtr &Value);



	/* Traits handling */
	/*******************/

	protected:
		//! Type to map type names to their handling traits
		typedef std::map<std::string, MDTraitsPtr> TraitsMapType;

		//! Map of type names to thair handling traits
		static TraitsMapType TraitsMap;

		//! Type to map type ULs to their handling traits
		typedef std::map<UL, MDTraitsPtr> TraitsULMapType;

		//! Map of type ULs to thair handling traits
		static TraitsULMapType TraitsULMap;

	protected:
		//! Add a given type to the lookups
		static void AddType(MDTypePtr &Type, ULPtr &TypeUL);

	public:


		/* Allow MDObject class to view internals of this class */
		friend class MDObject;
	};
}

namespace mxflib
{
	//! Add a mapping to apply a given set of traits to a certain type
	/*! \ret The name of the traits
	 */
	/*template<class C>*/ inline std::string AddTraitsMapping(std::string TypeName, MDTraitsPtr Tr)
	{
//		MDTraitsPtr Tr = new C;
		MDTraitsPtr TrLookup = MDTraits::Find(Tr->Name());

		if(!TrLookup) MDTraits::Add(Tr->Name(), Tr);

		if(MDType::AddTraitsMapping(TypeName, Tr->Name()))
			return Tr->Name();
		else
			return "";
	}


	//! Update an existing mapping and apply to any existing type of the given name
	/*! \ret The name of the traits
	 */
	/*template<class C>*/ inline std::string UpdateTraitsMapping(std::string TypeName, MDTraitsPtr Tr)
	{
//		MDTraitsPtr Tr = new C;
		MDTraitsPtr TrLookup = MDTraits::Find(Tr->Name());

		if(!TrLookup) MDTraits::Add(Tr->Name(), Tr);

		if(MDType::UpdateTraitsMapping(TypeName, Tr->Name()))
			return Tr->Name();
		else
			return "";
	}


	//! Add a mapping to apply a given set of traits to a certain type
	/*! \ret The name of the traits
	 */
	/*template<class C>*/ inline std::string AddTraitsMapping(const UL &Type, MDTraitsPtr Tr)
	{
//		MDTraitsPtr Tr = new C;
		MDTraitsPtr TrLookup = MDTraits::Find(Tr->Name());

		if(!TrLookup) MDTraits::Add(Tr->Name(), Tr);

		if(MDType::AddTraitsMapping(Type, Tr->Name()))
			return Tr->Name();
		else
			return "";
	}


	//! Update an existing mapping and apply to any existing type of the given name
	/*! \ret The name of the traits
	 */
	/*template<class C>*/ inline std::string UpdateTraitsMapping(const UL &Type, MDTraitsPtr Tr)
	{
//		MDTraitsPtr Tr = new C;
		MDTraitsPtr TrLookup = MDTraits::Find(Tr->Name());

		if(!TrLookup) MDTraits::Add(Tr->Name(), Tr);

		if(MDType::UpdateTraitsMapping(Type, Tr->Name()))
			return Tr->Name();
		else
			return "";
	}
}

#endif // MXFLIB__MDTYPE_H
