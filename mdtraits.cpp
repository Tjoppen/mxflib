/*! \file	mdtraits.cpp
 *	\brief	Implementation of traits for MDType definitions
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

// Use mxflib by default in library source
using namespace mxflib;


// Default trait implementations
////////////////////////////////

void MDTraits::fSetInt(MDValue *Object, Int32 Val) { error("NO BODY!\n"); };
void MDTraits::fSetInt64(MDValue *Object, Int64 Val) { error("NO BODY!\n"); };
void MDTraits::fSetUint(MDValue *Object, Uint32 Val) { error("NO BODY!\n"); };
void MDTraits::fSetUint64(MDValue *Object, Uint64 Val) { error("NO BODY!\n"); };
void MDTraits::fSetString(MDValue *Object, std::string Val) { error("NO BODY!\n"); };
Int32 MDTraits::fGetInt(MDValue *Object) { error("NO BODY!\n"); return 0;};
Int64 MDTraits::fGetInt64(MDValue *Object) { error("NO BODY!\n"); return 0; };
Uint32 MDTraits::fGetUint(MDValue *Object) { error("NO BODY!\n"); return 0; };
Uint64 MDTraits::fGetUint64(MDValue *Object) { error("NO BODY!\n"); return 0; };
std::string MDTraits::fGetString(MDValue *Object) { return std::string("Base"); };


// Extended trait implementations
/////////////////////////////////

/*****************************
**   Int8 Implementations   **
*****************************/

//! Set Int8 from an Int32
void mxflib::Int8_SetInt(MDValue *Object, Int32 Val) 
{ 
	if(Object->GetSize() != 1)
	{
		Object->MakeSize(1);
		
		if(Object->GetSize() != 1)
		{
			error("Tried to set an MDValue to a 1-byte value, but could not set length to 1\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	Int8 i = Val;
	Object->SetValue(1, (Uint8*)&i);
};

//! Set Int8 from an Int64
void mxflib::Int8_SetInt64(MDValue *Object, Int64 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Int8 from a Uint32
void mxflib::Int8_SetUint(MDValue *Object, Uint32 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Int8 from a Uint64
void mxflib::Int8_SetUint64(MDValue *Object, Uint64 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Int8 from a string
void mxflib::Int8_SetString(MDValue *Object, std::string Val) {	Int8_SetInt(Object, (Int32)atoi(Val.c_str())); };

//! Get Int32 from an Int8
Int32 mxflib::Int8_GetInt(MDValue *Object) 
{ 
	int Size = Object->GetSize();

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 1)
	{
		error("Tried to read a 1-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Return the value promoted to 32-bits
	return (Int32) *((const Int8*)(Object->GetData()));
};

//! Get Int64 from an Int8
Int64 mxflib::Int8_GetInt64(MDValue *Object) { return (Int64) Int8_GetInt(Object); };

//! Get Uint32 from an Int8
/*! \note
 *	This function will return 128 through 255 for bit values 10000000 through 11111111
 *	even though an Int8 cannot store them. This is as opposed to the option of returning
 *  0xffffff80 through 0xffffffff for those values.
 */
Uint32 mxflib::Int8_GetUint(MDValue *Object)
{ 
	int Size = Object->GetSize();

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 1)
	{
		error("Tried to read a 1-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Return the value promoted to 32-bits
	return (Uint32) *((const Uint8*)(Object->GetData()));
};

//! Get Uint64 from an Int8
/*! /note like mxflib::Int8_GetUint this function will returns 128 through 255 for -ve values
 */
Uint64 mxflib::Int8_GetUint64(MDValue *Object) { return (Uint64) Int8_GetUint(Object); };



std::string mxflib::Int8_GetString(MDValue *Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%d", Int8_GetInt(Object));
	return std::string(Buffer);
};

/*****************************
**   Uint8 Implementations   **
*****************************/

//! Set Uint8 from an Int32
void mxflib::Uint8_SetInt(MDValue *Object, Int32 Val) { Int8_SetInt(Object, Val); };

//! Set Uint8 from an Int64
void mxflib::Uint8_SetInt64(MDValue *Object, Int64 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Uint8 from a Uint32
void mxflib::Uint8_SetUint(MDValue *Object, Uint32 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Uint8 from a Uint64
void mxflib::Uint8_SetUint64(MDValue *Object, Uint64 Val) { Int8_SetInt(Object, (Int32)Val); };


/*****************************
**   Int16 Implementations   **
*****************************/

//! Set Int16 from an Int32
void mxflib::Int16_SetInt(MDValue *Object, Int32 Val) 
{ 
	if(Object->GetSize() != 2)
	{
		Object->MakeSize(2);
		
		if(Object->GetSize() != 2)
		{
			error("Tried to set an MDValue to a 2-byte value, but could not set length to 2\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	// But as this is a multi-byte value we may need
	// to byte swap it...
	Int16 i = Swap(Val);

	Object->SetValue(1, (Uint8*)&i);
};

//! Set Int16 from an Int64
void mxflib::Int16_SetInt64(MDValue *Object, Int64 Val) { Int16_SetInt(Object, (Int32)Val); };

//! Set Int16 from a Uint32
void mxflib::Int16_SetUint(MDValue *Object, Uint32 Val) { Int16_SetInt(Object, (Int32)Val); };

//! Set Int16 from a Uint64
void mxflib::Int16_SetUint64(MDValue *Object, Uint64 Val) { Int16_SetInt(Object, (Int32)Val); };



