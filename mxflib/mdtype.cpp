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
 *	\version $Id: mdtype.cpp,v 1.1 2004/04/26 18:27:47 asuraparaju Exp $
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


#include <mxflib/mxflib.h>

using namespace mxflib;


//! Default traits for types without special handling
static MDTraits DefaultTraits;


//! Add a definition for a basic type
/*! DRAGONS: Currently doesn't check for duplicates
 */
MDTypePtr MDType::AddBasic(std::string TypeName, int TypeSize)
{
	// Can't have a zero length basic type!
	ASSERT(TypeSize != 0);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, BASIC, &DefaultTraits);

	// Set no base type
	NewType->Base = NULL;

	// Set the type size
	NewType->Size = TypeSize;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	NameLookup[TypeName] = NewType;

	// Return a pointer to the new type
	return NewType;
}


//! Add a definition for an interpretation type (With optional fixed size)
/*! DRAGONS: Currently doesn't check for duplicates
 */
MDTypePtr MDType::AddInterpretation(std::string TypeName, MDTypePtr BaseType, int Size /* = 0 */)
{
	// Can't base on nothing!
	ASSERT(BaseType);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, INTERPRETATION, BaseType->Traits);

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
	NameLookup[TypeName] = NewType;

	// Return a pointer to the new type
	return NewType;
}


//! Add a definition for an array type
/*! DRAGONS: Currently doesn't check for duplicates
 */
MDTypePtr MDType::AddArray(std::string TypeName, MDTypePtr BaseType, int Size /* = 0 */)
{
	// Can't base on nothing!
	ASSERT(BaseType);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, TYPEARRAY, BaseType->Traits);

	// Set base type
	NewType->Base = BaseType;

	// Set the array size
	NewType->Size = Size;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	NameLookup[TypeName] = NewType;

	// Return a pointer to the new type
	return NewType;
}


//! Add a definition for a compound type
/*! DRAGONS: Currently doesn't check for duplicates
 */
MDTypePtr MDType::AddCompound(std::string TypeName)
{
	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, COMPOUND, &DefaultTraits);

	// Set no base type
	NewType->Base = NULL;

	// Compounds have no type
	NewType->Size = 0;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	NameLookup[TypeName] = NewType;

	// Return a pointer to the new type
	return NewType;
}


//! Find the MDType object that defines a named type
/*! \return Pointer to the object
 *  \return NULL if there is no type of that name
 */
MDTypePtr MDType::Find(const std::string& TypeName)
{
	MDTypePtr theType;

	std::map<std::string,MDTypePtr>::iterator it = NameLookup.find(TypeName);
	if(it == NameLookup.end())
	{
		return NULL;
	}

	theType = (*it).second;

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
	if(Class == INTERPRETATION)
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
	if(Class == INTERPRETATION)
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
	if(Class == INTERPRETATION)
	{
		ASSERT(Base);
		return Base->EffectiveBase();
	}

	return Base;
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

		Type = MDType::Find("Unknown");

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
	
	// If it's a basic type build an empty one
	if(Type->EffectiveClass() == BASIC)
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
Uint32 MDValue::MakeSize(Uint32 NewSize)
{
	// Enforce fixed size if one exists for this type
	if(Type->Size) NewSize = Type->Size;

	Data.Resize(NewSize);
	return NewSize;
}


/* //! Set the value of an object from a pre-formatted buffer
void MDValue::SetValue(int ValSize, const Uint8 *Val)
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

		int Num = size();

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
	insert(MDValue::value_type(size(), Child));
}


//! Add or Remove children from an MDValue continer to make a fixed size
/*! Probably only useful for resizing arrays.
 */
void MDValue::Resize(Uint32 Count)
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

	unsigned int Current = size();

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

	// Return a smart pointer to the object
	if(it != end()) Ret = (*it).second;

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
Uint32 MDValue::ReadValue(const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	return Type->Traits->ReadValue(this, Buffer, Size, Count);
}


//std::string MDValue::GetString(void) { return std::string("Base"); };
//std::string MDValue_Int8::GetString(void) { return std::string("Int8"); };



//** Static Instantiations for MDType class **
//********************************************

MDTypeList MDType::Types;	//!< All types managed by the MDType class

//! Map for reverse lookups based on type name
std::map<std::string, MDTypePtr> MDType::NameLookup;
