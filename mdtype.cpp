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

#include "mxflib.h"

using namespace mxflib;


//! Default traits for types without special handling
MDTraits DefaultTraits;


//! Add a definition for a basic type
/*! DRAGONS: Currently doesn't check for duplicates
 */
void MDType::AddBasic(std::string TypeName, int TypeSize)
{
	// Can't have a zero length basic type!
	ASSERT(TypeSize != 0);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, BASIC, &DefaultTraits);

	// Set the type size
	NewType->Size = TypeSize;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	NameLookup[TypeName] = NewType;
}


//! Add a definition for an interpretation type
/*! DRAGONS: Currently doesn't check for duplicates
 */
void MDType::AddInterpretation(std::string TypeName, MDTypePtr BaseType)
{
	// Can't base on nothing!
	ASSERT(BaseType);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, INTERPRETATION, BaseType->Traits);

	// Set the type size
	NewType->Size = BaseType->Size;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	NameLookup[TypeName] = NewType;
}


//! Add a definition for an array type
/*! DRAGONS: Currently doesn't check for duplicates
 */
void MDType::AddArray(std::string TypeName, MDTypePtr BaseType, int Size /* = 0 */)
{
	// Can't base on nothing!
	ASSERT(BaseType);

	// Create a new MDType to manage
	MDTypePtr NewType = new MDType(TypeName, TYPEARRAY, BaseType->Traits);

	// Set the array size
	NewType->Size = Size;

	// Add to the list of types
	Types.push_back(NewType);

	// Set the lookup
	NameLookup[TypeName] = NewType;
}


//! Find the MDType object that defines a named type
/*! /ret Pointer to the object
 *  /ret NULL if there is no type of that name
 */
MDTypePtr MDType::Find(const char *TypeName)
{
	MDTypePtr theType;

	std::map<std::string,MDTypePtr>::iterator it = NameLookup.find(std::string(TypeName));
	if(it == NameLookup.end())
	{
		return NULL;
	}

	theType = (*it).second;

	return theType;
}


//! MDValue named constructor
/*! Builds a "blank" variable of a named type
*/
MDValue::MDValue(const char *BaseType)
{
	Type = MDType::Find(BaseType);

	if(Type == NULL)
	{
		error("Metadata variable type \"%s\" doesn't exist\n", BaseType);
		// DRAGONS: Must sort this!!
	}

	// Initialise the new variable
	Init();
}


//! MDValue typed constructor
/*! Builds a "blank" variable of a specified type
*/
MDValue::MDValue(MDType *BaseType)
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
	// Start with no value
	Size = 0;
	Data = NULL;
}

//! Set a variable to be a certain size in bytes
/*!	The old data is NOT copied. This function assumes that this is a viable thing to do!
 */
void MDValue::MakeSize(int NewSize)
{
	if(Size == NewSize) return;
	
	if(Size < NewSize)
	{
		if(Size) delete[] Data;
		Data = new Uint8[NewSize];
		
		ASSERT(Data != NULL);
	}
	
	Size = NewSize;
}


//! Set a variable to be a certain size in bytes
/*!	The old data is NOT copied. This function assumes that this is a viable thing to do!
 */
void MDValue::SetValue(int ValSize, Uint8 *Val)
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


//! Add a child to an MDValue continer
/*! If the container is an array the index number of the new object can be
 *! specified. If the index number is specified and a child already exists
 *! with that number it is replaced. If the index number is specified and
 *! it is not the next index available, extra 'empty' objects are added to
 *! grow the array to the appropriate size.
 */
void MDValue::AddChild(MDValuePtr Child, int Index /* = -1 */)
{
	ASSERT( Type->Class == TYPEARRAY || Type->Class == COMPOUND );

	// Specific array index given
	if(Index >= 0)
	{
		// Can only specify an index for arrays
		ASSERT( Type->Class == TYPEARRAY );

		int Num = Children.size();

		// Replacing a current entry
		if(Num > Index)
		{
			MDValueList::iterator it = Children.begin();

			// Move to the index point
			while(Index--) it++;

			// Insert the new item at this point
			Children.insert(it, Child);

			// Remove the old entry, automatically deleting the object if required
			Children.erase(it);

			// All done for replace operation
			return;
		}
		else
		{
			// Entry padding items required
			if(Index > Num)
			{
				while(Index > Num)
				{
					// Insert a new item of the same type at the end
					Children.push_back(new MDValue(Child->Type));

					Num++;
				}
			}
		}
	}

	// Add to the list of children
	Children.push_back(Child);
};


//! Remove children from an MDValue continer
/*! Remove all but the first "Index" children. 
 *! Probably only useful for resizing arrays.
 */
void MDValue::TrimChildren(int Index)
{
	ASSERT( Type->Class == TYPEARRAY || Type->Class == COMPOUND );
	
	MDValueList::iterator it = Children.begin();

	// Move to the index point
	while(Index--) it++;

	// Remove the old entries, automatically deleting the objects if required
	Children.erase(it, Children.end());
}


//! Access array member within an MDValue array
/*! DRAGONS: The 
*/
MDValuePtr MDValue::operator[](int Index)
{
	MDValuePtr Ret = NULL;

	MDValueList::iterator it = Children.begin();

	while(Index--)
	{
		// End of list!
		if(++it == Children.end()) return Ret;
	}

	// Return a smart pointer to the object
	Ret = *(it);

	return Ret;
}



//std::string MDValue::GetString(void) { return std::string("Base"); };
//std::string MDValue_Int8::GetString(void) { return std::string("Int8"); };



//** Static Instantiations for MDType class **
//********************************************

MDTypeList MDType::Types;	//!< All types managed by the MDType class

//! Map for reverse lookups based on type name
std::map<std::string, MDTypePtr> MDType::NameLookup;
