/*! \file	mdtraits.cpp
 *	\brief	Implementation of traits for MDType definitions
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

extern "C"
{
#include "Klv.h"						//!< The KLVLib header
}

#include <mxflib/mxflib.h>

extern "C"
{
#include "Endian.h"
}

// Use mxflib by default in library source
using namespace mxflib;

// Standard library includes
#include <stdexcept>


// Default trait implementations
////////////////////////////////

void mxflib::MDTraits::SetInt(MDValuePtr Object, Int32 Val) { error("NO BODY!\n"); }
void mxflib::MDTraits::SetInt64(MDValuePtr Object, Int64 Val) { error("NO BODY!\n"); }
void mxflib::MDTraits::SetUint(MDValuePtr Object, Uint32 Val) { error("NO BODY!\n"); }
void mxflib::MDTraits::SetUint64(MDValuePtr Object, Uint64 Val) { error("NO BODY!\n"); }
void mxflib::MDTraits::SetString(MDValuePtr Object, std::string Val) { error("NO BODY!\n"); }
Int32 mxflib::MDTraits::GetInt(MDValuePtr Object) { error("NO BODY!\n"); return 0;}
Int64 mxflib::MDTraits::GetInt64(MDValuePtr Object) { error("NO BODY!\n"); return 0; }
Uint32 mxflib::MDTraits::GetUint(MDValuePtr Object) { error("NO BODY!\n"); return 0; }
Uint64 mxflib::MDTraits::GetUint64(MDValuePtr Object) { error("NO BODY!\n"); return 0; }
std::string mxflib::MDTraits::GetString(MDValuePtr Object) { return std::string("Base"); }

Uint32 MDTraits::ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	// If multiple items are found read them all "blindly"
	Uint32 FullSize = Size;
	if(Count) FullSize *= Count;

	// Try and make exactly the right amount of room
	// Some objects will not allow this and will return a different size
	Uint32 ObjSize = Object->MakeSize(FullSize);

	// If the object is too small, only read what we can
	if(ObjSize < FullSize)
	{
		Object->SetData(ObjSize, Buffer);
		return ObjSize;
	};

	// If the object is exactly the right size read it all in
	if(ObjSize == FullSize)
	{
		Object->SetData(FullSize, Buffer);
	}
	else
	{
		// If the object ends up too big we build a copy
		// of the data with zero padding
		Uint8 *Temp = new Uint8[ObjSize];

		memcpy(Temp, Buffer, FullSize);
		memset(&Temp[FullSize], 0, ObjSize - FullSize);
		Object->SetData(ObjSize, Temp);

		delete[] Temp;
	}

	return FullSize;
}



// Extended trait implementations
/////////////////////////////////

/*************************************
**   Basic Integer Implementation   **
**                                  **
** Re-maps those functions that can **
** be re-mapped for 32-bit or less  **
** Some need to be overridden for   **
** 32-bit implementations to sort   **
** out signed/unsigned              **
*************************************/

//! Set from an Int64
void mxflib::MDTraits_BasicInt::SetInt64(MDValuePtr Object, Int64 Val) { SetInt(Object, (Int32)Val); }

//! Set from a Uint32
void mxflib::MDTraits_BasicInt::SetUint(MDValuePtr Object, Uint32 Val) { SetInt(Object, (Int32)Val); }

//! Set from a Uint64
void mxflib::MDTraits_BasicInt::SetUint64(MDValuePtr Object, Uint64 Val) { SetInt(Object, (Int32)Val); }

//! Set from a string
void mxflib::MDTraits_BasicInt::SetString(MDValuePtr Object, std::string Val) { SetInt(Object, (Int32)atoi(Val.c_str())); }

//! Get Int64
Int64 mxflib::MDTraits_BasicInt::GetInt64(MDValuePtr Object) { return (Int64) GetInt(Object); }

//! Get Uint64
Uint64 mxflib::MDTraits_BasicInt::GetUint64(MDValuePtr Object) { return (Uint64) GetUint(Object); }

//!	Get string from an integer
std::string mxflib::MDTraits_BasicInt::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%d", GetInt(Object));
	return std::string(Buffer);
}

Uint32 MDTraits_BasicInt::ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	// Limit the size attempted to be read to the size of the type
	Uint32 TypeSize = Object->GetType()->Size;
	if(TypeSize && (Size > TypeSize)) Size = TypeSize;

	if(Size >= 8)
	{
		Object->SetInt64(GetI64(Buffer));
		return 8;
	}
	else if(Size >= 4)
	{
		Object->SetInt64(GetI32(Buffer));
		return 4;
	}
	else if(Size >= 2)
	{
		Object->SetInt64(GetI16(Buffer));
		return 2;
	}
	else if(Size >= 1)
	{
		Object->SetInt64(GetI8(Buffer));
		return 1;
	}

	Object->SetInt64(0);
	return 0;
}

//! Special unsigned integer version of ReadValue - called by all basic unsigned integers
Uint32 mxflib::ReadValueUint(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	// Limit the size attempted to be read to the size of the type
	Uint32 TypeSize = Object->GetType()->Size;
	if(TypeSize && (Size > TypeSize)) Size = TypeSize;

	if(Size >= 8)
	{
		Object->SetUint64(GetU64(Buffer));
		return 8;
	}
	else if(Size >= 4)
	{
		Object->SetUint64(GetU32(Buffer));
		return 4;
	}
	else if(Size >= 2)
	{
		Object->SetUint64(GetU16(Buffer));
		return 2;
	}
	else if(Size >= 1)
	{
		Object->SetUint64(GetU8(Buffer));
		return 1;
	}

	Object->SetUint64(0);
	return 0;
}


/*****************************
**   Int8 Implementations   **
*****************************/

//! Set Int8 from an Int32
void mxflib::MDTraits_Int8::SetInt(MDValuePtr Object, Int32 Val) 
{ 
	if(Object->GetData().Size != 1)
	{
		Object->MakeSize(1);
		
		if(Object->GetData().Size != 1)
		{
			error("Tried to set an MDValue to a 1-byte value, but could not set length to 1\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	Int8 i = Val;
	Object->SetData(1, (Uint8*)&i);
}

//! Get Int32 from an Int8
Int32 mxflib::MDTraits_Int8::GetInt(MDValuePtr Object) 
{ 
	int Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 1)
	{
		error("Tried to read a 1-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Return the value promoted to 32-bits
	return (Int32) *((const Int8*)(Object->GetData().Data));
}

//! Get Uint32 from an Int8
/*! \note
 *	This function will return 128 through 255 for bit values 10000000 through 11111111
 *	even though an Int8 cannot store them. This is as opposed to the option of returning
 *  0xffffff80 through 0xffffffff for those values.
 */
Uint32 mxflib::MDTraits_Int8::GetUint(MDValuePtr Object)
{ 
	int Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 1)
	{
		error("Tried to read a 1-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Return the value promoted to 32-bits
	return (Uint32) *((const Uint8*)(Object->GetData().Data));
}


/******************************
**   Uint8 Implementations   **
******************************/

//!	Get string from a Uint8
std::string MDTraits_Uint8::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", GetUint(Object));
	return std::string(Buffer);
}

//! Read value from memory buffer
Uint32 MDTraits_Uint8::ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	return ReadValueUint(Object, Buffer, Size, Count);
}


/******************************
**   Int16 Implementations   **
******************************/

//! Set Int16 from an Int32
void mxflib::MDTraits_Int16::SetInt(MDValuePtr Object, Int32 Val) 
{ 
	if(Object->GetData().Size != 2)
	{
		Object->MakeSize(2);
		
		if(Object->GetData().Size != 2)
		{
			error("Tried to set an MDValue to a 2-byte value, but could not set length to 2\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	// But as this is a multi-byte value we may need
	// to byte swap it...
	// Note that the swap is done in an unsigned int
	// to prevent any sign problems!
	Uint16 i = Swap((Uint16)Val);

	Object->SetData(2, (Uint8*)&i);
}

//! Get Int32 from an Int16
Int32 mxflib::MDTraits_Int16::GetInt(MDValuePtr Object) 
{ 
	int Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 2)
	{
		error("Tried to read a 2-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 16-bit value
	return ((Object->GetData().Data)[0] << 8) | (Object->GetData().Data)[1];
}

//! Get Uint32 from an Int16
Uint32 mxflib::MDTraits_Int16::GetUint(MDValuePtr Object) 
{ 
	int Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 2)
	{
		error("Tried to read a 2-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 16-bit value
	return ((Object->GetData().Data)[0] << 8) | (Object->GetData().Data)[1];
}


/*******************************
**   Uint16 Implementations   **
*******************************/

//!	Get string from a Uint16
std::string MDTraits_Uint16::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", GetUint(Object));
	return std::string(Buffer);
}

//! Read value from memory buffer
Uint32 MDTraits_Uint16::ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	return ReadValueUint(Object, Buffer, Size, Count);
}


/******************************
**   Int32 Implementations   **
******************************/

//! Set Int32 from an Int32
void mxflib::MDTraits_Int32::SetInt(MDValuePtr Object, Int32 Val) 
{ 
	if(Object->GetData().Size != 4)
	{
		Object->MakeSize(4);
		
		if(Object->GetData().Size != 4)
		{
			error("Tried to set an MDValue to a 4-byte value, but could not set length to 4\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	// But as this is a multi-byte value we may need
	// to byte swap it...
	// Note that the swap is done in an unsigned int
	// to prevent any sign problems!
	Uint32 i = Swap((Uint32)Val);

	Object->SetData(4, (Uint8*)&i);
}

//! Get Int32 from an Int32
Int32 mxflib::MDTraits_Int32::GetInt(MDValuePtr Object) 
{ 
	int Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 4)
	{
		error("Tried to read a 4-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 32-bit value
	return ((Object->GetData().Data)[0] << 24) | ((Object->GetData().Data)[1] << 16)
		 | ((Object->GetData().Data)[2] << 8) | (Object->GetData().Data)[3];
}

//! Get Uint32 from an Int32
Uint32 mxflib::MDTraits_Int32::GetUint(MDValuePtr Object)
{
	// As the return value is the same size as our working variables
	// the signed to unsigned conversion should be safe like this
	return (Uint32)GetInt(Object);
}


/*******************************
**   Uint32 Implementations   **
*******************************/

//!	Get string from a Uint32
std::string MDTraits_Uint32::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", GetUint(Object));
	return std::string(Buffer);
}

//! Read value from memory buffer
Uint32 MDTraits_Uint32::ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	return ReadValueUint(Object, Buffer, Size, Count);
}



/******************************
**   Int64 Implementations   **
******************************/

//! Set Int64 from an Int64
void mxflib::MDTraits_Int64::SetInt64(MDValuePtr Object, Int64 Val) 
{ 
	if(Object->GetData().Size != 8)
	{
		Object->MakeSize(8);
		
		if(Object->GetData().Size != 8)
		{
			error("Tried to set an MDValue to a 8-byte value, but could not set length to 8\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	// But as this is a multi-byte value we may need
	// to byte swap it...
	// Note that the swap is done in an unsigned int
	// to prevent any sign problems!
	Uint64 i = Swap((Uint64)Val);

	Object->SetData(8, (Uint8*)&i);
}

//! Set from an Int32
void mxflib::MDTraits_Int64::SetInt(MDValuePtr Object, Int32 Val) { SetInt64(Object, (Int64)Val); }

//! Set from a Uint32
void mxflib::MDTraits_Int64::SetUint(MDValuePtr Object, Uint32 Val) { SetUint64(Object, (Uint64)Val); }

//! Set from a Uint64
/*! DRAGONS: Will this always work? This relies on the Uint64 -> Int64 -> Uint64
 *           conversion being valid for all values!
 */
void mxflib::MDTraits_Int64::SetUint64(MDValuePtr Object, Uint64 Val) { SetInt64(Object, (Uint64)Val); }

//! Set from a string
void mxflib::MDTraits_Int64::SetString(MDValuePtr Object, std::string Val) { SetInt64(Object, ato_Int64(Val.c_str())); }

//!	Get string from an integer
std::string mxflib::MDTraits_Int64::GetString(MDValuePtr Object) 
{ 
	return Int64toString(GetInt64(Object));
}

//! Get Int
Int32 mxflib::MDTraits_Int64::GetInt(MDValuePtr Object) { return (Int32) GetInt64(Object); }

//! Get Uint
Uint32 mxflib::MDTraits_Int64::GetUint(MDValuePtr Object) { return (Uint32) GetUint64(Object); }

//! Get Int64
Int64 mxflib::MDTraits_Int64::GetInt64(MDValuePtr Object) 
{ 
	int Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 8)
	{
		error("Tried to read an 8-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 64-bit value using Uint32s to prevent possible sign problems
	Uint32 HiVal = ((Object->GetData().Data)[0] << 24) | ((Object->GetData().Data)[1] << 16)
		         | ((Object->GetData().Data)[2] << 8) | (Object->GetData().Data)[3];
	Uint32 LoVal = ((Object->GetData().Data)[4] << 24) | ((Object->GetData().Data)[5] << 16)
		         | ((Object->GetData().Data)[6] << 8) | (Object->GetData().Data)[7];

	return (Uint64(HiVal) << 32) | LoVal;
}

//! Get Uint64
Uint64 mxflib::MDTraits_Int64::GetUint64(MDValuePtr Object)
{
	// As the return value is the same size as our working variables
	// the signed to unsigned conversion should be safe like this
	return (Uint64)GetInt64(Object);
}


/*******************************
**   Uint64 Implementations   **
*******************************/

//!	Get string from an integer
std::string mxflib::MDTraits_Uint64::GetString(MDValuePtr Object) 
{ 
	return Uint64toString(GetUint64(Object));
}

//! Read value from memory buffer
Uint32 MDTraits_Uint64::ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	return ReadValueUint(Object, Buffer, Size, Count);
}


/***************************************
**   ISO 7-bit char Implementations   **
***************************************/

//!	Get string from an ISO7
std::string MDTraits_ISO7::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%c", GetInt(Object));
	return std::string(Buffer);
}

//! Set an ISO7 from a string
void MDTraits_ISO7::SetString(MDValuePtr Object, std::string Val)
{
	const char *StringData = Val.c_str();
	SetInt(Object, *StringData);
}


/************************************
**   UTF-16 char Implementations   **
************************************/

//!	Get string from a UTF16
std::string MDTraits_UTF16::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%c", GetInt(Object));
	return std::string(Buffer);
}

//! Set a UTF16 from a string
void MDTraits_UTF16::SetString(MDValuePtr Object, std::string Val)
{
	const char *StringData = Val.c_str();
	SetInt(Object, *StringData);
}


/**************************************
**   Default Array Implementations   **
***************************************/

void MDTraits_BasicArray::SetInt(MDValuePtr Object, Int32 Val)
{
	MDValue::iterator it;

	it = Object->begin();
	while(it != Object->end())
	{
		(*it).second->SetInt(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetInt64(MDValuePtr Object, Int64 Val)
{
	MDValue::iterator it;

	it = Object->begin();
	while(it != Object->end())
	{
		(*it).second->SetInt64(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetUint(MDValuePtr Object, Uint32 Val)
{
	MDValue::iterator it;

	it = Object->begin();
	while(it != Object->end())
	{
		(*it).second->SetUint(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetUint64(MDValuePtr Object, Uint64 Val)
{
	MDValue::iterator it;

	it = Object->begin();
	while(it != Object->end())
	{
		(*it).second->SetUint64(Val);
		it++;
	}
}


//! Set string for basic array types
/*! \note This will fail if one of the array items includes a comma
 */
void MDTraits_BasicArray::SetString(MDValuePtr Object, std::string Val)
{
	MDValue::iterator it;

	int LastComma = -1;
	
	it = Object->begin();

	for(;;)
	{
		int Comma = Val.find(",",LastComma+1);

		// If we are already at the end of the list, add another
		if(it == Object->end()) 
		{
			Object->Resize(Object->size()+1);
			it = Object->end();
			it--;
		}

		if(Comma == (int)std::string::npos)
		{
			(*it).second->SetString(std::string(Val,LastComma+1, std::string::npos));
			return;
		}

		(*it).second->SetString(std::string(Val,LastComma+1, (Comma-LastComma)-1));

		it++;

		LastComma = Comma;
	}
}


Int32 MDTraits_BasicArray::GetInt(MDValuePtr Object)
{
	MDValue::iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetInt();
}

Int64 MDTraits_BasicArray::GetInt64(MDValuePtr Object)
{
	MDValue::iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetInt64();
}


Uint32 MDTraits_BasicArray::GetUint(MDValuePtr Object)
{
	MDValue::iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetUint();
}

Uint64 MDTraits_BasicArray::GetUint64(MDValuePtr Object)
{
	MDValue::iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetUint64();
}

std::string MDTraits_BasicArray::GetString(MDValuePtr Object)
{
	std::string Ret;

	MDValue::iterator it;

	Ret = "";

	it = Object->begin();
	while(it != Object->end())
	{
		if(Ret.length() > 0) Ret += ", ";
		Ret += (*it).second->GetString();
		it++;
	}
	return Ret;
}


Uint32 MDTraits_BasicArray::ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	// Start with no children in the object
	Object->clear();
	
	// If Count is 0 then the number of items is unknown
	bool UnknownCount;
	if(Count == 0)
	{
		UnknownCount = true;
		Count = 1;
	}
	else 
	{
		UnknownCount = false;
	}

	// Figure out the maximum number of items to read
	Uint32 MaxItems = Object->GetType()->Size;

	// Count of actual items read, and bytes read in doing so
	Uint32 ActualCount = 0;
	Uint32 Bytes = 0;

	// Either the size of each item to read, or the total size (for unknown count)
	Uint32 ThisSize = Size;

	while(Count)
	{
		Uint32 ThisBytes;

		MDValuePtr NewItem = new MDValue(Object->EffectiveBase());

		ThisBytes = NewItem->ReadValue(Buffer, ThisSize);
		Bytes += ThisBytes;
		Buffer += ThisBytes;

		if(UnknownCount)
		{
			// When we are on the last item keep scanning until...
			if(ThisBytes > ThisSize) ThisBytes = 0; else ThisSize -= ThisBytes;

			// ...we run out of data or until ReadValue stops taking data
			if((ThisBytes == 0) || (ThisSize == 0)) Count = 0;
		}
		else
		{
			Count--;
		}

		Object->AddChild(NewItem);
		ActualCount++;

		// Bug out if we run out of space
		if((MaxItems) && (ActualCount == MaxItems)) break;
	}

	// Force padding to be added if this is a fixed size array
	if(MaxItems) Object->Resize(MaxItems);

	return Bytes;
}


/*********************************************
**   Default String Array Implementations   **
**********************************************/

std::string MDTraits_BasicStringArray::GetString(MDValuePtr Object)
{
	std::string Ret;

	MDValue::iterator it;

	Ret = "";

	it = Object->begin();
	while(it != Object->end())
	{
		std::string Temp = (*it).second->GetString();
		
		// Stop if a terminating zero was found
		if(Temp.length() == 0) break;

		Ret += Temp;
		it++;
	}
	return Ret;
}


void MDTraits_BasicStringArray::SetString(MDValuePtr Object, std::string Val)
{
	MDValue::iterator it;

	int Size = Val.length();
	int Index = 0;
	
	Object->Resize(Size);

	it = Object->begin();
	while(it != Object->end())
	{
		std::string Temp;
		char c = '\0';

		try
		{
			c = Val.at(Index++);
		}
		catch(std::out_of_range)
		{
			// Ignore string slice errors!! - Should never happen
		}

		// Stop at a terminating NUL
		if(c == '\0') 
		{
			Object->Resize(Size);
			break;
		}

		Temp = c;
		(*it).second->SetString(Temp);

		it++;
	}
}


/****************************
**   Raw Implementations   **
****************************/

std::string MDTraits_Raw::GetString(MDValuePtr Object)
{
	std::string Ret;
	int Count = Object->GetData().Size;
	const Uint8 *Data = Object->GetData().Data;

	Ret = "";

	while(Count)
	{
		char Buffer[32];
		sprintf(Buffer, "%02x", *Data);

		if(Ret.length() != 0) Ret += " ";
		Ret += Buffer;
		
		Count--;
		Data++;
	}
	return Ret;
}


void MDTraits_Raw::SetString(MDValuePtr Object, std::string Val)
{
	int Count = Object->GetData().Size;
	int Value = -1;
	Uint8 *Data = new Uint8[Count];
	Uint8 *pD = Data;
	const char *p = Val.c_str();

	// During this loop Value = -1 when no digits of a number are mid-process
	// This stops a double space being regarded as a small zero in between two spaces
	while(Count)
	{
		int digit;
		
		if((*p == 0) && (Value == -1)) Value = 0;

		if(*p >= '0' && *p <='9') digit = (*p) - '0';
		else if(*p >= 'a' && *p <= 'f') digit = (*p) - 'a' + 10;
		else if(*p >= 'A' && *p <= 'F') digit = (*p) - 'A' + 10;
		else if(Value == -1)
		{
			// Skip second or subsiquent non-digit
			p++;
			continue;
		}
		else 
		{
			*pD = Value;
			*pD++;

			Count--;

			if(*p) p++;
			
			Value = -1;

			continue;
		}

		if(Value == -1) Value = 0; else Value <<=4;
		Value += digit;
		p++;
	};

	Object->SetData(Object->GetData().Size, Data);
}


/**********************************
**   Raw Array Implementations   **
**********************************/

std::string MDTraits_RawArray::GetString(MDValuePtr Object)
{
	std::string Ret;

	MDValue::iterator it;

	Ret = "";

	it = Object->begin();
	while(it != Object->end())
	{
		// Buffer for standard sizes
		char Buffer[32];

		if(Ret.length() != 0) Ret += " ";

		int Size = (*it).second->GetData().Size;
		
		// Invalidate buffer
		Buffer[0] = 0;

		if(Size == 1) sprintf(Buffer, "%02x", (*it).second->GetUint());
		else if(Size == 2) sprintf(Buffer, "%04x", (*it).second->GetUint());
		else if(Size == 4) sprintf(Buffer, "%08x", (*it).second->GetUint());
		else if(Size == 8) ASSERT(0);		// DRAGONS: We need a 64-bit hex print function!
		else
		{
			// Non-standard size!
			Ret += "{";
			Ret += GetString((*it).second);
			Ret += "}";
		}

		if(Buffer[0]) Ret += Buffer;
		it++;
	}
	return Ret;
}


void MDTraits_RawArray::SetString(MDValuePtr Object, std::string Val)
{
	MDValue::iterator it;

	unsigned int Index = 0;
	int Value = -1;
	const char *p = Val.c_str();

	it = Object->begin();

	// During this loop Value = -1 when no digits of a number are mid-process
	// This stops a double space being regarded as a small zero in between two spaces
	// It also stops a trailing zero being appended to the array if the last character
	// before the terminating byte is not a hex digit.
	do
	{
		int digit;
		if(*p >= '0' && *p <='9') digit = (*p) - '0';
		else if(*p >= 'a' && *p <= 'f') digit = (*p) - 'a' + 10;
		else if(*p >= 'A' && *p <= 'F') digit = (*p) - 'A' + 10;
		else if(Value == -1)
		{
			// Skip second or subsiquent non-digit
			continue;
		}
		else 
		{
			if(Object->size() <= Index)
			{
				Object->Resize(Index+1);

				// Bug-out early if we hit the end of a fixed length array
				if(Object->size() <= Index) break;

				it = Object->end();
				it--;
			}

			(*it).second->SetInt(Value);

			it++;

			Value = -1;
			Index++;

			continue;
		}

		if(Value == -1) Value = 0; else Value <<=4;
		Value += digit;
	
	// Note that the loop test is done in this way to force
	// a final cycle of the loop with *p == 0 to allow the last
	// number to be processed
	} while(*(p++));
}


/********************************************
**   Array of Raw Arrays Implementations   **
********************************************/

std::string MDTraits_RawArrayArray::GetString(MDValuePtr Object)
{
	std::string Ret;

	MDValue::iterator it;

	Ret = "";

	it = Object->begin();
	while(it != Object->end())
	{
		if(Ret.length() != 0) Ret += ", ";
		Ret += "{";
		Ret += (*it).second->GetString();
		Ret += "}";
		it++;
	}
	return Ret;
}


void MDTraits_RawArrayArray::SetString(MDValuePtr Object, std::string Val)
{
	MDValue::iterator it;

	int OpenBracket;
	int CloseBracket = -1;
	
	it = Object->begin();

	for(;;)
	{
		OpenBracket = Val.find("{",CloseBracket+1);
		if(OpenBracket == (int)std::string::npos) return;

		CloseBracket = Val.find("}",OpenBracket+1);
		if(CloseBracket == (int)std::string::npos) return;

		// If we are already at the end of the list, add another
		if(it == Object->end()) 
		{
			Object->Resize(Object->size()+1);
			it = Object->end();
			it--;
		}

		(*it).second->SetString(std::string(Val,OpenBracket+1, (CloseBracket-OpenBracket)-1));

		it++;
	}
}


/***************************************
**   Basic Compound Implementations   **
***************************************/

std::string MDTraits_BasicCompound::GetString(MDValuePtr Object)
{
	std::string Ret;

	StringList::iterator it = Object->EffectiveType()->ChildOrder.begin();
	StringList::iterator itend = Object->EffectiveType()->ChildOrder.end();

	while(it != itend)
	{
		MDValuePtr Value = Object[*it];

		if(!Value)
		{
			error("Missing sub-item %s in compound\n", (*it).c_str());
		}
		else
		{
			if(Ret.length() != 0) Ret += ", ";
			Ret += (*it);
			Ret += "=\"";
			Ret += Value->GetString();
			Ret += "\"";
		}
		it++;
	}
	return Ret;
}


void MDTraits_BasicCompound::SetString(MDValuePtr Object, std::string Val)
{
	int OpenQuote;
	int CloseQuote = -1;

	StringList::iterator it = Object->EffectiveType()->ChildOrder.begin();
	StringList::iterator itend = Object->EffectiveType()->ChildOrder.end();

	for(;;)
	{
		OpenQuote = Val.find("\"",CloseQuote+1);
		if(OpenQuote == (int)std::string::npos) return;

		// DRAGONS: Should add code here to allow out-of-order items

		CloseQuote = Val.find("\"",OpenQuote+1);
		if(CloseQuote == (int)std::string::npos) return;

		// If we are already at the end of the list, we have too much data!
		if(it == itend) 
		{
			warning("Extra parameters found parsing string in MDTraits_BasicCompound::SetString()\n");
			break;
		}

		MDValuePtr Value = Object[*it];
		if(!Value)
		{
			error("Missing sub-item %s in compound\n", (*it).c_str());
		}
		else
		{
			Value->SetString(std::string(Val,OpenQuote+1, (CloseQuote-OpenQuote)-1));
		}
		it++;
	}
}


//! Basic function to read a compound from a buffer
/*! \note Count is ignored in this function
 */
Uint32 MDTraits_BasicCompound::ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count /*=0*/)
{
	Uint32 Bytes = 0;

	StringList::iterator it = Object->EffectiveType()->ChildOrder.begin();
	StringList::iterator itend = Object->EffectiveType()->ChildOrder.end();

	while(Size)
	{
		// If we are already at the end of the list, we have too many bytes!
		if(it == itend) 
		{
			warning("Extra bytes found parsing buffer in MDTraits_BasicCompound::ReadValue()\n");
			break;
		}

		MDValuePtr Value = Object[*it];
		if(!Value)
		{
			error("Missing sub-item %s in compound\n", (*it).c_str());
		}
		else
		{
			Uint32 ThisBytes = Value->ReadValue(Buffer, Size);
			Bytes += ThisBytes;

			Buffer += ThisBytes;
			if(ThisBytes >= Size) Size = 0; else Size -= ThisBytes;
		}
		it++;
	}

	if(it != itend)
	{
		warning("Not enough bytes in buffer in MDTraits_BasicCompound::ReadValue()\n");
	}

	return Bytes;
}


/*********************************
**   Rational Implementations   **
*********************************/

std::string MDTraits_Rational::GetString(MDValuePtr Object)
{
	MDValuePtr Numerator = Object["Numerator"];
	MDValuePtr Denominator = Object["Denominator"];

	Uint32 Num = 0;
	Uint32 Den = 0;
	if(Numerator) Num = Numerator->GetUint();
	if(Denominator) Den = Denominator->GetUint();

	return Uint2String(Num) + "/" + Uint2String(Den);
}


void MDTraits_Rational::SetString(MDValuePtr Object, std::string Val)
{
	MDValuePtr Numerator = Object["Numerator"];
	MDValuePtr Denominator = Object["Denominator"];

	Uint32 Num = atoi(Val.c_str());

	Uint32 Den = 0;
	std::string::size_type Slash = Val.find("/");
	if(Slash != std::string::npos) Den = atoi(&(Val.c_str()[Slash+1]));

	if(Numerator) Numerator->SetUint(Num);
	if(Denominator) Denominator->SetUint(Den);
}


/**********************************
**   TimeStamp Implementations   **
**********************************/

std::string MDTraits_TimeStamp::GetString(MDValuePtr Object)
{
	MDValuePtr Year = Object["Year"];
	MDValuePtr Month = Object["Month"];
	MDValuePtr Day = Object["Day"];
	MDValuePtr Hours = Object["Hours"];
	MDValuePtr Minutes = Object["Minutes"];
	MDValuePtr Seconds = Object["Seconds"];
	MDValuePtr msBy4 = Object["msBy4"];

	Uint32 Y;
	Uint32 M;
	Uint32 D;
	Uint32 H;
	Uint32 Min;
	Uint32 S;
	Uint32 ms;

	if(Year) Y = Year->GetUint(); else Y = 0;
	if(Month) M = Month->GetUint(); else M = 0;
	if(Day) D = Day->GetUint(); else D = 0;
	if(Hours) H = Hours->GetUint(); else H = 0;
	if(Minutes) Min = Minutes->GetUint(); else Min = 0;
	if(Seconds) S = Seconds->GetUint(); else S = 0;
	if(msBy4) ms = msBy4->GetUint() * 4; else ms = 0;

	return Uint2String(Y) + "-" + Uint2String(M,2) + "-" + Uint2String(D,2) + " " +
		   Uint2String(H) + ":" + Uint2String(Min,2) + ":" + Uint2String(S,2) + "." + Uint2String(ms,3);
}


void MDTraits_TimeStamp::SetString(MDValuePtr Object, std::string Val)
{
	MDValuePtr Year = Object["Year"];
	MDValuePtr Month = Object["Month"];
	MDValuePtr Day = Object["Day"];
	MDValuePtr Hours = Object["Hours"];
	MDValuePtr Minutes = Object["Minutes"];
	MDValuePtr Seconds = Object["Seconds"];
	MDValuePtr msBy4 = Object["msBy4"];

	Uint32 Y;
	Uint32 M;
	Uint32 D;
	Uint32 H;
	Uint32 Min;
	Uint32 S;
	Uint32 ms;

	sscanf(Val.c_str(), "%d-%d-%d", &Y, &M, &D);
	std::string::size_type Pos = Val.find("T");
	if(Pos == std::string::npos) Pos = Val.find(" ");
	if(Pos != std::string::npos) sscanf(&(Val.c_str()[Pos]), "%d:%d:%d.%d", &H, &Min, &S, &ms);

	if(Year) Year->SetUint(Y);
	if(Month) Month->SetUint(M);
	if(Day) Day->SetUint(D);
	if(Hours) Hours->SetUint(H);
	if(Minutes) Minutes->SetUint(Min);
	if(Seconds) Seconds->SetUint(S);
	if(msBy4) msBy4->SetUint(ms / 4);
}



