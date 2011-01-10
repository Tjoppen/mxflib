/*! \file	mdtype.cpp
 *	\brief	Implementation of classes that define metadata type info
 *
 *			Class MDDict holds the overall dictionary definitions and
 *          manages loading them from a dictionary file and adding new
 *			metadata types.
 *<br><br>
 *			Class MDType holds info about a specific metadata type
 *<br><br>
 *			These classes are currently wrappers around KLVLib structures
 *
 *	\version $Id: mdtype.cpp,v 1.17 2011/01/10 10:42:09 matt-beard Exp $
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


#include "mxflib/mxflib.h"

using namespace mxflib;


//! Default traits for types without special handling
namespace
{
	MDTraitsPtr DefaultTraits = new MDTraits_DefaultTraits;
}


//! Map of type names to thair handling traits
MDType::TraitsMapType MDType::TraitsMap;

//! Map of type ULs to thair handling traits
MDType::TraitsULMapType MDType::TraitsULMap;


//! Add a given type to the lookups
void MDType::AddType(MDTypePtr &Type, ULPtr &TypeUL)
{
	// Add the name to the name lookup
	NameLookup[Type->TypeName] = Type;

	// Add the UL to the UL lookup
	ULLookup[*TypeUL] = Type;

	/* Add a version 1 UL for versionless compares */

	// Only add the version 1 lookup for SMPTE ULs (other labels may have other version rules)
	if((TypeUL->GetValue()[0] == 0x06) && (TypeUL->GetValue()[1] == 0x0e) && (TypeUL->GetValue()[2] == 0x2b) && (TypeUL->GetValue()[3] == 0x34))
	{
		// Make a version 1 copy of this UL
		ULPtr Ver1UL = new UL(TypeUL);
		Ver1UL->Set(7,1);

		// Insert it into the version 1 lookup
		ULLookupVer1[*Ver1UL] = Type;
	}
}


//! Add a definition for a basic type
/*! DRAGONS: Currently doesn't check for duplicates
 */
MDTypePtr MDType::AddBasic(std::string TypeName, std::string Detail, ULPtr &UL, int TypeSize)
{
	// Can't have a zero length basic type!
	// But we can have a variable size (==0)
	// mxflib_assert(TypeSize != 0);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, Detail, BASIC, UL, DefaultTraits);

	// Set no base type
	NewType->Base = NULL;

	// Set the type size
	NewType->Size = TypeSize;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	AddType(NewType, UL);

	// Return a pointer to the new type
	return NewType;
}


//! Add a definition for an interpretation type (With optional fixed size)
/*! DRAGONS: Currently doesn't check for duplicates
 */
MDTypePtr MDType::AddInterpretation(std::string TypeName, std::string Detail, MDTypePtr BaseType, ULPtr &UL, int Size /* = 0 */)
{
	// Can't base on nothing!
	mxflib_assert(BaseType);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, Detail, INTERPRETATION, UL, BaseType->Traits);

	// Set base type
	NewType->Base = BaseType;

	// Set the type size
	if(Size == 0)
	{
		// Inherit size from base
		NewType->Size = BaseType->Size;
	}
	else
	{
		// Force a new fixed size
		// Note: This is only valid if the base type is variable size!
		mxflib_assert(BaseType->Size == 0);
		NewType->Size = Size;
	}

	// Copy array type and reference details from base
	NewType->ArrayClass = BaseType->ArrayClass;
	NewType->RefType = BaseType->RefType;
	NewType->RefTarget = BaseType->RefTarget;
	NewType->RefTargetType = BaseType->RefTargetType;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	AddType(NewType, UL);

	// Return a pointer to the new type
	return NewType;
}


//! Add a definition for an array type
/*! DRAGONS: Currently doesn't check for duplicates
 */
MDTypePtr MDType::AddArray(std::string TypeName, std::string Detail, MDTypePtr BaseType, ULPtr &UL, int Size /* = 0 */)
{
	// Can't base on nothing!
	mxflib_assert(BaseType);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, Detail, TYPEARRAY, UL, DefaultTraits);

	// Set base type
	NewType->Base = BaseType;

	// Set the array size
	NewType->Size = Size;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	AddType(NewType, UL);

	// Return a pointer to the new type
	return NewType;
}


//! Add a definition for a compound type
/*! DRAGONS: Currently doesn't check for duplicates
 */
MDTypePtr MDType::AddCompound(std::string TypeName, std::string Detail, ULPtr &UL)
{
	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, Detail, COMPOUND, UL, DefaultTraits);

	// Set no base type
	NewType->Base = NULL;

	// Compounds have no size of thier own
	NewType->Size = 0;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	AddType(NewType, UL);

	// Return a pointer to the new type
	return NewType;
}


//! Add a definition for an enumeration type
/*! DRAGONS: Currently doesn't check for duplicates
 */
MDTypePtr MDType::AddEnum(std::string TypeName, std::string Detail, MDTypePtr BaseType, ULPtr &UL)
{
	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, Detail, ENUM, UL, DefaultTraits);

	// Set the base type
	NewType->Base = BaseType;

	// Inherit size
	NewType->Size = BaseType->Size;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	AddType(NewType, UL);

	// Return a pointer to the new type
	return NewType;
}


//! Add a value to a definition for an enumeration type
/*! DRAGONS: This actual value object will be added to the enumeration class - don't change the value after adding it!
 *  \return true if all OK (including if this is an exact duplicate)
 */
bool MDType::AddEnumValue(std::string Name, MDObjectPtr &Value)
{
	// Can only add enumerated values to an enumeration
	if(Class != ENUM) 
	{
		error("Attempted to add enumerated value %s to type %s, which is not an enumeration\n", Name.c_str(), TypeName.c_str());
		return false;
	}

	// Check for duplicate value
	NamedValueList::iterator it = EnumValues.begin();
	while(it != EnumValues.end())
	{
		if((*it).first == Name)
		{
			// Exact duplicates are allowed
			if(*((*it).second) == *Value) return true;

			error("Attempted to add enumerated value named %s to type %s, which already has a value of this name with a different value\n", Name.c_str(), TypeName.c_str());
			return false;
		}

		if(((*it).second)->GetString() == Value->GetString())
		{
			error("Attempted to add enumerated value of %s to type %s, which already has this value\n", Value->GetString().c_str(), TypeName.c_str());
			return false;
		}

		it++;
	}

	// Add the value to the list
	EnumValues.push_back(NamedValue(Name, Value));

	// Return success
	return true;
}


//! Add a value to a definition for an enumeration type
/*! \return true if all OK
 */
bool MDType::AddEnumValue(std::string Name, std::string Value)
{
	MDObjectPtr NewValue = new MDObject(Base);
	if(!NewValue) return false;

	NewValue->SetString(Value);

	return AddEnumValue(Name, NewValue);
}


//! Add a value to a definition for an enumeration type
/*! \return true if all OK
 */
bool MDType::AddEnumValue(std::string Name, ULPtr &Value)
{
	// Can only add enumerated values to an enumeration
	if(Class != ENUM) 
	{
		error("Attempted to add enumerated value %s to type %s, which is not an enumeration\n", Name.c_str(), TypeName.c_str());
		return false;
	}

	// String version of the value for comparison
	std::string ValueString = Value->GetString();

	// Check for duplicate value
	NamedValueList::iterator it = EnumValues.begin();
	while(it != EnumValues.end())
	{
		if((*it).first == Name)
		{
			error("Attempted to add enumerated value named %s to type %s, which already has a value of this name\n", Name.c_str(), TypeName.c_str());
			return false;
		}

		if((*it).second->GetString() == ValueString)
		{
			error("Attempted to add enumerated value of %s to type %s, which already has this value\n", ValueString.c_str(), TypeName.c_str());
			return false;
		}

		it++;
	}

	MDObjectPtr NewValue = new MDObject(Base);
	if(!NewValue) return false;

	NewValue->SetString(ValueString);

	// Add the value to the list
	EnumValues.push_back(NamedValue(Name, NewValue));

	// Return success
	return true;
}


//! Find the MDType object that defines a named type
/*! \return Pointer to the object
 *  \return NULL if there is no type of that name
 *	\note If BaseType contains a qualified name of the format "symbolspace::name" then only 
 *        the specified symbolspace is searched
 */
MDTypePtr MDType::Find(std::string TypeName, SymbolSpacePtr &SymSpace, bool SearchAll /*=false*/)
{
	// Check for a symbol space given in the name
	size_type Pos = TypeName.find("::");

	if(Pos != std::string::npos)
	{
		SymbolSpacePtr Sym;

		// DRAGONS: A zero length namespace represents the default namespace
		if(Pos == 0) Sym = MXFLibSymbols;
		else Sym = SymbolSpace::FindSymbolSpace(TypeName.substr(0, Pos));

		if(Sym)
		{
			ULPtr ThisUL = Sym->Find(TypeName.substr(Pos+2), false);
			if(ThisUL) return MDType::Find(ThisUL);
		}
	}
	else
	{
		ULPtr ThisUL = SymSpace->Find(TypeName, SearchAll);
		if(ThisUL) return MDType::Find(ThisUL);
	}

	return NULL;
}



//! Find the MDType object that defines a type with a specified UL
/*! \return Pointer to the object
 *  \return NULL if there is no type with that UL
 */
MDTypePtr MDType::Find(const UL& BaseUL)
{
	MDTypePtr theType;

	std::map<UL, MDTypePtr>::iterator it = ULLookup.find(BaseUL);

	if(it != ULLookup.end())
	{
		theType = (*it).second;
	}
	else
	{
		// If the exact match is not found try a version-less lookup by changing the version number to 1
		if((BaseUL.GetValue()[0] == 0x06) && (BaseUL.GetValue()[1] == 0x0e) && (BaseUL.GetValue()[2] == 0x2b) && (BaseUL.GetValue()[3] == 0x34))
		{
			UL Ver1UL = BaseUL;
			Ver1UL.Set(7, 1);

			std::map<UL, MDTypePtr>::iterator it2 = ULLookupVer1.find(Ver1UL);
			if(it2 != ULLookupVer1.end())
			{
				theType = (*it2).second;
			}
		}
	}

	return theType;
}


//! Locate a named child
MDTypePtr MDType::Child(std::string Name) const
{
	MDType::const_iterator it = find(Name);
	if(it != end()) return (*it).second;

	return NULL;
}

//! Locate a numerically indexed child
/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
MDTypePtr MDType::Child(int Index) const
{
	/* No numeric index for types - try by count */
	if(size() > static_cast<size_t>(Index))
	{
		MDType::const_iterator it = begin();
		while(--Index) it++;
		return (*it).second;
	}

	return NULL;
}

//! Locate a child by UL
MDTypePtr MDType::Child(ULPtr &ChildType) const
{
	MDType::const_iterator it = begin();
	while(it != end())
	{
		if(((*it).second->TypeUL)->Matches(*ChildType)) return (*it).second;
		it++;
	}

	return NULL;
}


//! Locate a child by UL
MDTypePtr MDType::Child(const UL &ChildType) const
{
	MDType::const_iterator it = begin();
	while(it != end())
	{
		if(((*it).second->TypeUL)->Matches(ChildType)) return (*it).second;
		it++;
	}

	return NULL;
}


//! Report the effective type of this type
/*! \note Care must be taken using this function because
 *        it is easy to end up confused and read properties
 *        from the "effective" type that should be read
 *        from the interpretation instead (such as traits)
 *
 *  DRAGONS: We return a const* here because we can't build a smart pointer from a const 'this'.
 *		     We need this function to be const as it is used in other const methods.
 *			 As what we return is a const* it is pretty safe for this to not be smart, the only risk
 *			 is someone making a long-term copy of this pointer that will out-live the referenced
 *			 object, and that is very unlikely due to the nature of this funtion.
 */
const MDType *MDType::EffectiveType(void) const
{
	// If we are an interpretation then see what of
	if(Class == INTERPRETATION || Class == ENUM)
	{
		mxflib_assert(Base);
		return Base->EffectiveType();
	}

	return this;
}


//! Report the effective class of this type
MDTypeClass MDType::EffectiveClass(void) const
{
	// If we are an interpretation then see what of
	if(Class == INTERPRETATION || Class == ENUM)
	{
		mxflib_assert(Base);
		return Base->EffectiveClass();
	}

	return Class;
}


//! Report the effective base type of this type
MDTypePtr MDType::EffectiveBase(void) const
{
	// If we are an interpretation then see what of
	if(Class == INTERPRETATION || Class == ENUM)
	{
		mxflib_assert(Base);
		return Base->EffectiveBase();
	}

	return Base;
}


//! Report the effective reference type of this type
TypeRef MDType::EffectiveRefType(void) const
{
	if(RefType != TypeRefUndefined) return RefType;

	// If we are an interpretation then see what of
	if(Class == INTERPRETATION || Class == ENUM || Class == TYPEARRAY)
	{
		mxflib_assert(Base);
		return Base->EffectiveRefType();
	}

	return TypeRefNone;
}


//! Report the effective reference target of this type
MDOTypePtr MDType::EffectiveRefTarget(void) const
{
	if(RefTargetType) return RefTargetType;

	// If we are an interpretation then see what of
	if(Class == INTERPRETATION || Class == ENUM || Class == TYPEARRAY)
	{
		mxflib_assert(Base);
		return Base->EffectiveRefTarget();
	}

	return NULL;
}


//! Report the name of the effective reference target of this type
/*! DRAGONS: To be used when loading dictionary only */
std::string MDType::EffectiveRefTargetName(void) const
{
	if(!RefTarget.empty()) return RefTarget;

	// If we are an interpretation then see what of
	if(Class == INTERPRETATION || Class == ENUM || Class == TYPEARRAY)
	{
		mxflib_assert(Base);
		return Base->EffectiveRefTargetName();
	}

	return "";
}


//! Report the effective size of this type - internal recursive version
/*! \return The size in bytes of a single instance of this type, or 0 if variable size
 *  This function is recursive and at any stage the "size" value may be overridden.
 *  This means that an array defined as variable size may have its size "fixed" by an interpretation.
 *	It is even possible for this to be re-interpreted with a new size.
 *  For example, UTF16 is a 2-byte integer used to hold a Unicode character; UTF16String is a variable length
 *  string of UTF16 characters (size = 0) and CharPair could be an interpretation of UTF16String with a fixed
 *  size of 2. This could then be modified by making CharTriple an interpretation of CharPair with size = 3.
 */
UInt32 MDType::EffectiveSizeInternal(bool OverrideSize /*=false*/, int UseSize /*=0*/) const
{
	// If we are an interpretation then see what of
	if(Class == INTERPRETATION)
	{
		mxflib_assert(Base);
		// DRAGONS: The outermost overriden size is the one we stick with
		return Base->EffectiveSizeInternal(true, OverrideSize ? UseSize : Size);
	}

	// If we are an array calculate the total array size (will be zero if either is undefined)
	if(Class == TYPEARRAY)
	{
		mxflib_assert(Base);
		return Base->EffectiveSize() * OverrideSize ? UseSize : Size;
	}

	// If we are a compound calculate the size of the compound
	if(Class == COMPOUND)
	{
		UInt32 Ret = 0;

		MDType::const_iterator it;
		it = this->begin();

		while(it != end())
		{
			UInt32 ItemSize = (*it).second->EffectiveSize();

			// If any item is variable then we are variable
			if(ItemSize == 0) return 0;

			Ret += ItemSize;
			it++;
		}

		return Ret;
	}

	return Size;
}


//! Add a mapping to be applied to all types of a given type name
/*! \note This will act retrospectively - all existing traits will be updated as required
	*/
bool MDType::AddTraitsMapping(std::string TypeName, std::string TraitsName)
{
	MDTraitsPtr Traits;
	if(!TraitsName.empty()) Traits = MDTraits::Find(TraitsName);
	if(!Traits) Traits = DefaultTraits;
	
	TraitsMap[TypeName] = Traits;

	/* Apply these traits to any type that will need them */
	std::map<std::string, MDTypePtr>::iterator it = NameLookup.begin();
	while(it != NameLookup.end())
	{
		bool UpdateThis = false;

		// Exact name matches will be updated
		if((*it).first == TypeName) UpdateThis = true;
		else
		{
			// If the type is an interpretation type, and its base type matches this type name...
			if(((*it).second->Class == INTERPRETATION) && ((*it).second->EffectiveType()->Name() == TypeName))
			{
				// ...and it does not have a trait mapping itself, we will update it
				TraitsULMapType::iterator ULMap_it = TraitsULMap.find((*it).second->TypeUL);
				if(ULMap_it == TraitsULMap.end())
				{
					TraitsMapType::iterator Map_it = TraitsMap.find((*it).second->Name());
					if(Map_it == TraitsMap.end()) UpdateThis = true;
				}
			}
		}

		if(UpdateThis)
		{
			(*it).second->SetTraits(Traits);
		}

		it++;
	}

	return true;
}


//! Add a mapping to be applied to all types of a given type UL
/*! \note This will act retrospectively - all existing traits will be updated as required
	*/
bool MDType::AddTraitsMapping(const UL &TypeUL, std::string TraitsName)
{
	MDTraitsPtr Traits;
	if(!TraitsName.empty()) Traits = MDTraits::Find(TraitsName);
	if(!Traits) Traits = DefaultTraits;

	TraitsULMap[TypeUL] = Traits;

	/* Apply these traits to any type that will need them */
	std::map<UL, MDTypePtr>::iterator it = ULLookup.begin();
	while(it != ULLookup.end())
	{
		bool UpdateThis = false;

		// Exact matches will be updated
		if((*it).first == TypeUL) UpdateThis = true;
		else
		{
			// If the type is an interpretation type, and its base type matches this type UL...
			if(((*it).second->Class == INTERPRETATION) && (*(*it).second->EffectiveType()->TypeUL == TypeUL))
			{
				// ...and it does not have a trait mapping itself, we will update it
				TraitsULMapType::iterator ULMap_it = TraitsULMap.find((*it).second->TypeUL);
				if(ULMap_it == TraitsULMap.end())
				{
					TraitsMapType::iterator Map_it = TraitsMap.find((*it).second->Name());
					if(Map_it == TraitsMap.end()) UpdateThis = true;
				}
			}
		}

		if(UpdateThis)
		{
			(*it).second->SetTraits(Traits);
		}

		it++;
	}

	/* Apply these traits to any match with a version 1 UL */

	UL Ver1UL(TypeUL);
	Ver1UL.Set(7,1);

	it = ULLookupVer1.begin();
	while(it != ULLookupVer1.end())
	{
		bool UpdateThis = false;

		// Exact matches will be updated
		if((*it).first == TypeUL) UpdateThis = true;
		else
		{
			// If the type is an interpretation type, and its base type matches this type UL...
			if(((*it).second->Class == INTERPRETATION) && (*(*it).second->EffectiveType()->TypeUL == TypeUL))
			{
				// ...and it does not have a trait mapping itself, we will update it
				TraitsULMapType::iterator ULMap_it = TraitsULMap.find((*it).second->TypeUL);
				if(ULMap_it == TraitsULMap.end())
				{
					TraitsMapType::iterator Map_it = TraitsMap.find((*it).second->Name());
					if(Map_it == TraitsMap.end()) UpdateThis = true;
				}
			}
		}

		if(UpdateThis)
		{
			(*it).second->SetTraits(Traits);
		}

		it++;
	}

	return true;
}


//! Lookup the traits for a specified type name
/*! If no traits have been defined for the specified type the traits with the name given in DefaultTraitsName is used (if specified)
 */
MDTraitsPtr MDType::LookupTraitsMapping(std::string TypeName, std::string DefaultTraitsName /*=""*/)
{
	MDTraitsPtr Ret;

	// First lookup this type's traits
	TraitsMapType::iterator it = TraitsMap.find(TypeName);
	if(it != TraitsMap.end())
	{
		Ret = (*it).second;
	}
	else
	{
		if(!DefaultTraitsName.empty())
		{
			it = TraitsMap.find(DefaultTraitsName);
			if(it != TraitsMap.end())
			{
				Ret = (*it).second;
			}
			else
			{
				// If that doesn't work, look up the named default traits
				Ret = MDTraits::Find(DefaultTraitsName);
			}
		}
	}

	return Ret;
}


//! Lookup the traits for a specified type name
/*! If no traits have been defined for the specified type the traits with the UL given in DefaultTraitsUL is used
 */
MDTraitsPtr MDType::LookupTraitsMapping(std::string TypeName, const UL &DefaultTraitsUL)
{
	MDTraitsPtr Ret;

	// First lookup this type's traits
	TraitsMapType::iterator it = TraitsMap.find(TypeName);
	if(it != TraitsMap.end())
	{
		Ret = (*it).second;
	}
	else
	{
		TraitsULMapType::iterator it2 = TraitsULMap.find(DefaultTraitsUL);
		if(it2 != TraitsULMap.end())
		{
			Ret = (*it2).second;
		}
	}

	return Ret;
}


//! Lookup the traits for a specified type UL
/*! If no traits have been defined for the specified type the traits with the UL given in DefaultTraitsUL is used
 */
MDTraitsPtr MDType::LookupTraitsMapping(const UL &TypeUL, const UL &DefaultTraitsUL )
{
	MDTraitsPtr Ret;

	// First lookup this type's traits
	TraitsULMapType::iterator it = TraitsULMap.find(TypeUL);
	if(it != TraitsULMap.end())
	{
		Ret = (*it).second;
	}
	else
	{
		it = TraitsULMap.find(DefaultTraitsUL);
		if(it != TraitsULMap.end())
		{
			Ret = (*it).second;
		}
	}

	return Ret;
}



//! Lookup the traits for a specified type UL
/*! If no traits have been defined for the specified type the traits with the UL given in DefaultTraitsName is used (if specified)
 */
MDTraitsPtr MDType::LookupTraitsMapping(const UL &TypeUL, std::string DefaultTraitsName /*=""*/)
{
	MDTraitsPtr Ret;

	// First lookup this type's traits
	TraitsULMapType::iterator it = TraitsULMap.find(TypeUL);
	if(it != TraitsULMap.end())
	{
		Ret = (*it).second;
	}
	else
	{
		if(!DefaultTraitsName.empty())
		{
			TraitsMapType::iterator it2 = TraitsMap.find(DefaultTraitsName);
			if(it2 != TraitsMap.end())
			{
				Ret = (*it2).second;
			}
			else
			{
				// If that doesn't work, look up the named default traits
				Ret = MDTraits::Find(DefaultTraitsName);
			}
		}
	}

	return Ret;
}





//** Static Instantiations for MDType class **
//********************************************

MDTypeList MDType::Types;	//!< All types managed by the MDType class

//! Map for UL lookups
std::map<UL, MDTypePtr> MDType::ULLookup;

//! Map for UL lookups - ignoring the version number (all entries use version = 1)
std::map<UL, MDTypePtr> MDType::ULLookupVer1;

//! Map for reverse lookups based on type name
MDTypeMap MDType::NameLookup;
