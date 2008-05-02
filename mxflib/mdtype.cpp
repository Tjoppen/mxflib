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
 *	\version $Id: mdtype.cpp,v 1.16 2008/05/02 16:26:22 matt-beard Exp $
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
MDTypePtr MDType::AddBasic(std::string TypeName, ULPtr &UL, int TypeSize)
{
	// Can't have a zero length basic type!
	// But we can have a variable size (==0)
	// ASSERT(TypeSize != 0);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, BASIC, UL, DefaultTraits);

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
MDTypePtr MDType::AddInterpretation(std::string TypeName, MDTypePtr BaseType, ULPtr &UL, int Size /* = 0 */)
{
	// Can't base on nothing!
	ASSERT(BaseType);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, INTERPRETATION, UL, BaseType->Traits);

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
		ASSERT(BaseType->Size == 0);
		NewType->Size = Size;
	}

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
MDTypePtr MDType::AddArray(std::string TypeName, MDTypePtr BaseType, ULPtr &UL, int Size /* = 0 */)
{
	// Can't base on nothing!
	ASSERT(BaseType);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, TYPEARRAY, UL, BaseType->Traits);

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
MDTypePtr MDType::AddCompound(std::string TypeName, ULPtr &UL)
{
	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, COMPOUND, UL, DefaultTraits);

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
MDTypePtr MDType::AddEnum(std::string TypeName, MDTypePtr BaseType, ULPtr &UL)
{
	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, ENUM, UL, DefaultTraits);

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
 *  \return true if all OK
 */
bool MDType::AddEnumValue(std::string Name, MDValuePtr &Value)
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
			error("Attempted to add enumerated value named %s to type %s, which already has a value of this name\n", Name.c_str(), TypeName.c_str());
			return false;
		}

		if(*((*it).second) == *Value)
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
/*! DRAGONS: This actual value object will be added to the enumeration class - don't change the value after adding it!
 *  \return true if all OK
 */
bool MDType::AddEnumValue(std::string Name, std::string Value)
{
	MDValuePtr NewValue = new MDValue(Base);

	if(!NewValue) return false;

	NewValue->SetString(Value);

	return AddEnumValue(Name, NewValue);
}


//! Add a value to a definition for an enumeration type
/*! DRAGONS: This actual value object will be added to the enumeration class - don't change the value after adding it!
 *  \return true if all OK
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

	MDValuePtr NewValue = new MDValue(Base);

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


//! Report the effective type of this type
/*! \note Care must be taken using this function because
 *        it is easy to end up confused and read properties
 *        from the "effective" type that should be read
 *        from the interpretation instead (such as traits)
 */
MDTypePtr MDType::EffectiveType(void)
{
	// If we are an interpretation then see what of
	if(Class == INTERPRETATION || Class == ENUM)
	{
		ASSERT(Base);
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
		ASSERT(Base);
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
		ASSERT(Base);
		return Base->EffectiveBase();
	}

	return Base;
}


//! Report the effective reference type of this type
TypeRef MDType::EffectiveRefType(void) const
{
	if(RefType != TypeRefUndefined) return RefType;

	// If we are an interpretation then see what of
	if(Class == INTERPRETATION || Class == ENUM)
	{
		ASSERT(Base);
		return Base->EffectiveRefType();
	}

	return TypeRefNone;
}


//! Report the effective reference target of this type
std::string MDType::EffectiveRefTarget(void) const
{
	if(RefTarget.length() != 0) return RefTarget;

	// If we are an interpretation then see what of
	if(Class == INTERPRETATION || Class == ENUM)
	{
		ASSERT(Base);
		return Base->EffectiveRefTarget();
	}

	return "";
}


//! Report the effective size of this type
/*! \return The size in bytes of a single instance of this type, or 0 if variable size
 */
UInt32 MDType::EffectiveSize(void) const
{
	// If we are an array calculate the total array size (will be zero if either is undefined)
	if(Class == TYPEARRAY)
	{
		ASSERT(Base);
		return Base->EffectiveSize() * Size;
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



//! MDValue named constructor
/*! Builds a "blank" variable of a named type
*/
MDValue::MDValue(const std::string &BaseType)
{
	Type = MDType::Find(BaseType);

	if(!Type)
	{
		error("Metadata variable type \"%s\" doesn't exist\n", BaseType.c_str());

		Type = MDType::Find("UnknownType");

		ASSERT(Type);
	}

	// Initialise the new variable
	Init();
}


//! MDValue typed constructor
/*! Builds a "blank" variable of a specified type
*/
MDValue::MDValue(MDTypePtr BaseType)
{
	Type = BaseType;

	// Initialise the new variable
	Init();
}


//! Second part of MDValue constructors
/*! Builds a "blank" variable
*/
void MDValue::Init(void)
{
	ASSERT(Type);
	
	// If it's a basic type (or handles its sub data) build an empty item
	if(Type->HandlesSubdata() || (Type->EffectiveClass() == BASIC))
	{
		if(Type->Size)
		{
			MakeSize(Type->Size);
			memset(Data.Data,0,Data.Size);
		}
	}

	// If it's a fixed size array build all items
	else if(Type->EffectiveClass() == TYPEARRAY)
	{
		if(Type->Size > 0)
		{
			// Build blank array
			Resize(Type->Size);
		}
	}

	// If it's a compound build all sub-items
	else if(Type->EffectiveClass() == COMPOUND)
	{
		MDType::iterator it;
		it = Type->begin();

		while(it != Type->end())
		{
			// Insert a new item of the appropriate type
			insert(MDValue::value_type((*it).first, new MDValue((*it).second)));
			it++;
		}
	}
}


//! Set a variable to be a certain size in bytes
/*!	The old data is NOT copied. 
 *  This function assumes that this is a viable thing to do!
 *  \return The size of the resized item
 */
size_t MDValue::MakeSize(size_t NewSize)
{
	// Enforce fixed size if one exists for this type
	if(Type->Size) NewSize = Type->Size;

	Data.Resize(NewSize);
	return NewSize;
}


/* //! Set the value of an object from a pre-formatted buffer
void MDValue::SetValue(int ValSize, const UInt8 *Val)
{
	if(ValSize > Size)
	{
		error("Tried to use MDValue::SetValue() to set %d bytes into %d\n", ValSize, Size);
		
		// Copy in what will fit!
		if(Size) memcpy(Data, Val, Size);
		
		return;
	}
	
	if(ValSize < Size)
	{
		warning("Used MDValue::SetValue() when Size = %d bytes, but new value only = %d\n", Size, ValSize);
	}

	memcpy(Data, Val, ValSize);
}
*/

//! Add a child to an MDValue continer
/*! If the container is an array the index number of the new object can be
 *! specified. If the index number is specified and a child already exists
 *! with that number it is replaced. If the index number is specified and
 *! it is not the next index available, extra 'empty' objects are added to
 *! grow the array to the appropriate size.
 */
void MDValue::AddChild(MDValuePtr Child, int Index /* = -1 */)
{
	MDTypeClass Class = Type->EffectiveClass();

	ASSERT( Class == TYPEARRAY || Class == COMPOUND );

	// Specific array index given
	if(Index >= 0)
	{
		// Can only specify an index for arrays
		ASSERT( Class == TYPEARRAY );

		int Num = static_cast<int>(size());

		// Replacing a current entry
		if(Num >= Index)
		{
			MDValue::iterator it = find(Index);

			if(it != end())
			{
				// Remove any old entry, automatically deleting the object if required
				erase(it);
			}

			// Insert the new item at this point
			insert(MDValue::value_type(Index, Child));

			// All done for replace operation
			return;
		}
		else
		{
			// Extra padding items required
			if(Index > (Num+1))
			{
				while(Index > (Num+1))
				{
					// Insert a new item of the same type at the end
					insert(MDValue::value_type(Num, new MDValue(Child->Type)));

					Num++;
				}
			}
		}
	}

	// Add to the list of children
	insert(MDValue::value_type(static_cast<UInt32>(size()), Child));
}


//! Add or Remove children from an MDValue continer to make a fixed size
/*! Probably only useful for resizing arrays.
 */
void MDValue::Resize(UInt32 Count)
{
	MDTypeClass Class = Type->EffectiveClass();

	ASSERT( Class == TYPEARRAY || Class == COMPOUND );
	
	// If this function is called for a fixed size array
	// simply validate the size
	if(Type->Size) Count = Type->Size;

	if(Count == 0)
	{
		clear();
		return;
	}

	unsigned int Current = static_cast<unsigned int>(size());

	// Extra padding items required
	if(Current < Count)
	{
		while(Current < Count)
		{
			// Insert a new item of the appropriate type
			insert(MDValue::value_type(Current, new MDValue(Type->EffectiveBase())));
			Current++;
		}
	}
	else if (Current > Count)
	{
		MDValue::iterator it = lower_bound(Count);

		// Remove the old entries, automatically deleting the objects if required
		if(it != end()) erase(it, end());
	}
}


//! Access array member within an MDValue array
/*! DRAGONS: This doesn't work well with SmartPtrs
 *           so member function Child() is also available
*/
MDValuePtr MDValue::operator[](int Index)
{
	MDValuePtr Ret = NULL;

	MDValue::iterator it = find(Index);

	// Did we find this index?
	if(it != end())
	{
		// Return a smart pointer to the object
		Ret = (*it).second;
	}
	else
	{
		// If not a match, it may be a valid attempt to index the nth member of a compound
		if(Type->EffectiveClass() == COMPOUND)
		{
			// Check the index bounds
			if((Index >= 0) && (Index < static_cast<int>(size())))
			{
				// No need to validate the map iteration as we have just checked the bounds
				it = begin();
				while(Index--) it++;
				Ret = (*it).second;
			}
		}
	}

	return Ret;
}


//! Access named sub-item within a compound MDValue
/*! DRAGONS: This doesn't work well with SmartPtrs
 *           so member function Child() is also available
*/
MDValuePtr MDValue::operator[](const std::string ChildName)
{
	MDValuePtr Ret = NULL;

	MDValue::iterator it = find(ChildName);

	// Return a smart pointer to the object
	if(it != end()) Ret = (*it).second;

	return Ret;
}


//! Access UL indexed sub-item within a compound MDValue
/*! DRAGONS: This doesn't work well with SmartPtrs
 *           so member function Child() is also available
*/
MDValuePtr MDValue::operator[](const UL &Child)
{
	// Scan all children for a matching UL
	MDValue::iterator it = begin();
	while(it != end())
	{
		if(*((*it).second->Type->GetTypeUL()) == Child) return (*it).second;
		it++;
	}

	/* Before giving up and saying no match - check again without comparing the version byte */
	it = begin();
	while(it != end())
	{
		if((*it).second->Type->GetTypeUL()->Matches(Child)) return (*it).second;
		it++;
	}

	return NULL;
}


/*std::string MDValue::ChildName(int Child)
{
	MDTypePtr EType = Type->EffectiveType();

	ASSERT(EType->EffectiveClass() == COMPOUND);
	
	if(EType->EffectiveClass() != COMPOUND) return "";
	
	MDValue::iterator it;
	it = begin();

	while(Child--)
	{
		if(it != end()) it++;
	}

	if(it == end()) return "";
	return (*it).second;
}
*/

//! Read value from a buffer
/*!
 *  \return Number of bytes read
 */
size_t MDValue::ReadValue(const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	return Type->Traits->ReadValue(this, Buffer, Size, Count);
}


//! Build a data chunk with all this items data (including child data)
DataChunkPtr MDValue::PutData(void) 
{
	DataChunkPtr Ret = new DataChunk;
	
	// True if we are a batch, but the size is not currently known
	bool BatchCorrection = false;
	UInt32 Count = static_cast<UInt32>(size());

	MDTypePtr EffType = EffectiveType();
	if(EffType->GetArrayClass() == ARRAYBATCH)
	{
		// Write the item count
		UInt8 Buffer[8];
		PutU32(Count, Buffer);

		// Write the item size - here we check to see if this is 'unknown' (flagged by zero), in which case we need to fix it at the end
		UInt32 Size = Type->EffectiveSize();
		if(Size == 0) BatchCorrection = true;
		PutU32(Size, &Buffer[4]);

		// Set the header
		Ret->Set(8, Buffer);
	}

	// If the size is zero we don't have any sub items
	// Otherwise we may not need to use them because the traits may build in our data
	if(size() == 0 || (Type->HandlesSubdata())) 
	{
		// If we are part of a batch this appends the data, otherwise it simply sets it to be the same
		Ret->Append(GetData());
	}
	else
	{
		// Compounds must be written in the correct order
		if(Type->EffectiveClass() == COMPOUND)
		{
			StringList::iterator it = Type->ChildOrder.begin();
			while(it != Type->ChildOrder.end())
			{
				DataChunkPtr SubItem = Child(*it)->PutData();
				Ret->Append(SubItem->Size, SubItem->Data);
				it++;
			}
		}
		else
		{
			MDValue::iterator it = begin();
			while(it != end())
			{
				DataChunkPtr SubItem = (*it).second->PutData();
				Ret->Append(SubItem->Size, SubItem->Data);
				it++;
			}
		}
	}

	// If this is a batch where we did not know the item size, set it now we have written all items
	if(BatchCorrection && (Count > 0) && (Ret->Size > 8))
	{
		UInt32 Size = static_cast<UInt32>((Ret->Size - 8) / Count);
		PutU32(Size, &Ret->Data[4]);
	}

	return Ret;
};

//std::string MDValue::GetString(void) { return std::string("Base"); };
//std::string MDValue_Int8::GetString(void) { return std::string("Int8"); };



//** Static Instantiations for MDType class **
//********************************************

MDTypeList MDType::Types;	//!< All types managed by the MDType class

//! Map for UL lookups
std::map<UL, MDTypePtr> MDType::ULLookup;

//! Map for UL lookups - ignoring the version number (all entries use version = 1)
std::map<UL, MDTypePtr> MDType::ULLookupVer1;

//! Map for reverse lookups based on type name
MDTypeMap MDType::NameLookup;
