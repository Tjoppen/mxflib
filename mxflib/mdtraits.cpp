/*! \file	mdtraits.cpp
 *	\brief	Implementation of traits for MDType definitions
 *
 *	\version $Id: mdtraits.cpp,v 1.27 2011/01/10 10:42:09 matt-beard Exp $
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

#include <ios>
#include <sstream>

// Use mxflib by default in library source
using namespace mxflib;

// Standard library includes
//#include <stdexcept>

//! Soft limit for strings returned by MDTraits - Defaults to 10k
/*! \note This is a soft limit in that it is not enforced strictly.
 *        It is possible for string values to be returned that are longer than this value, but where
 *	      the string is built by several passes around a loop that loop should exit once this value
 *	      has been reached. The MDTrait may return a short text string to indicate that the limit
 *        would be exceeded by a full version (e.g. "Data exceeds <limit> bytes") or it may add
 *        an indication that the limit has been reached (e.g. "01 23 45 67 ..." for a very short limit).
 *        It is also permissable to simply stop at the limit (e.g. "A very long stri")
 */
UInt32 mxflib::MDTraits_StringLimit = 10240;


//! Flag to modify string behaviour to terminate all strings written
/*! \note This only works for UTF16 and ISO7 string SetString traits
 */
bool mxflib::TerminateStrings = false;

//! FIXME: Horrible fudge to fix unknown array size problem
int mxflib::IndexFudge_NSL = 0;

//! List of all traits that exist
mxflib::MDTraitsMap mxflib::MDTraits::AllTraits;

//! The current options for converting labels to strings
mxflib::LabelFormat mxflib::LabelFormatOption = LabelFormatText;


//! The last value allocated an an enumerated value, -1 if none ever issued
OutputFormatEnum MDTraitsEnum::LastAllocatedEnum = -1;



//! Add a new trait to the list of known traits
/*! \ret True is all went well, else false
	*/
bool MDTraits::Add(std::string Name, MDTraitsPtr Trait)
{
	MDTraitsMap::iterator it = AllTraits.find(Name);
	if(it != AllTraits.end())
	{
		error("Internal error - two traits defined with the name \"%s\"\n", Name.c_str());
	
		return false;
	}

	AllTraits[Name] = Trait;

	return true;
};


//! Replace the named trait in the list of known traits
/*! \ret True is all went well, else false
	*/
bool MDTraits::Replace(std::string Name, MDTraitsPtr Trait)
{
	bool Ret = true;

	MDTraitsMap::iterator it = AllTraits.find(Name);
	if(it == AllTraits.end())
	{
		error("Internal error - MDTraits::Replace(%s) called when no such traits exist\n", Name.c_str());

		Ret = false;
	}

	AllTraits[Name] = Trait;

	return Ret;
}


//! Locate a named trait in the list of known traits
/*! \ret A pointer to the named trait, or NULL if not found
	*/
MDTraitsPtr MDTraits::Find(std::string Name)
{
	MDTraitsMap::iterator it = AllTraits.find(Name);
	if(it == AllTraits.end())
	{
		return NULL;
	}

	return (*it).second;
}


// Default trait implementations
////////////////////////////////

void mxflib::MDTraits::SetInt(MDObject *Object, Int32 Val) { error("Called SetInt() on %s which has traits of %s and does not support SetInt()\n", Object->Name().c_str(), Name().c_str()); }
void mxflib::MDTraits::SetInt64(MDObject *Object, Int64 Val) { error("Called SetInt64() on %s which has traits of %s and does not support SetInt64()\n", Object->Name().c_str(), Name().c_str()); }
void mxflib::MDTraits::SetUInt(MDObject *Object, UInt32 Val) { error("Called SetUInt() on %s which has traits of %s and does not support SetUInt()\n", Object->Name().c_str(), Name().c_str()); }
void mxflib::MDTraits::SetUInt64(MDObject *Object, UInt64 Val) { error("Called SetUInt64() on %s which has traits of %s and does not support SetUInt64()\n", Object->Name().c_str(), Name().c_str()); }
void mxflib::MDTraits::SetString(MDObject *Object, std::string Val) { error("Called SetString() on %s which has traits of %s and does not support SetString()\n", Object->Name().c_str(), Name().c_str()); }
Int32 mxflib::MDTraits::GetInt(const MDObject *Object) const { error("Called GetInt() on %s which has traits of %s and does not support GetInt()\n", Object->Name().c_str(), Name().c_str()); return 0;}
Int64 mxflib::MDTraits::GetInt64(const MDObject *Object) const { error("Called GetInt64() on %s which has traits of %s and does not support GetInt64()\n", Object->Name().c_str(), Name().c_str()); return 0;}
UInt32 mxflib::MDTraits::GetUInt(const MDObject *Object) const { error("Called GetUInt() on %s which has traits of %s and does not support GetUInt()\n", Object->Name().c_str(), Name().c_str()); return 0;}
UInt64 mxflib::MDTraits::GetUInt64(const MDObject *Object) const { error("Called GetUInt64() on %s which has traits of %s and does not support GetUInt64()\n", Object->Name().c_str(), Name().c_str()); return 0;}
std::string mxflib::MDTraits::GetString(const MDObject *Object) const { error("Called GetString() on %s which has traits of %s and does not support GetString()\n", Object->Name().c_str(), Name().c_str()); return std::string("Base"); }

//! Support old capitalization of SetUInt
void MDTraits::SetUint(MDObject *Object, UInt32 Val) { SetUInt(Object, Val); }

//! Support old capitalization of SetUInt64
void MDTraits::SetUint64(MDObject *Object, UInt64 Val) { SetUInt64(Object, Val); }

//! Support old capitalization of GetUInt
UInt32 MDTraits::GetUint(const MDObject *Object) const { return GetUInt(Object); }

//! Support old capitalization of GetUInt64
UInt64 MDTraits::GetUint64(const MDObject *Object) const { return GetUInt64(Object); }


size_t MDTraits::ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	// If multiple items are found read them all "blindly"
	size_t FullSize = Size;
	if(Count) FullSize *= Count;

	// Try and make exactly the right amount of room
	// Some objects will not allow this and will return a different size
	size_t ObjSize = Object->MakeSize(FullSize);

	// If the object is too small, only read what we can
	if(ObjSize < FullSize)
	{
		Object->SetData(ObjSize, Buffer);
		return ObjSize;
	}

	// If the object is exactly the right size read it all in
	if(ObjSize == FullSize)
	{
		Object->SetData(FullSize, Buffer);
	}
	else
	{
		// If the object ends up too big we build a copy
		// of the data with zero padding
		UInt8 *Temp = new UInt8[ObjSize];

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
void mxflib::MDTraits_BasicInt::SetInt64(MDObject *Object, Int64 Val) { SetInt(Object, (Int32)Val); }

//! Set from a UInt32
void mxflib::MDTraits_BasicInt::SetUInt(MDObject *Object, UInt32 Val) { SetInt(Object, (Int32)Val); }

//! Set from a UInt64
void mxflib::MDTraits_BasicInt::SetUInt64(MDObject *Object, UInt64 Val) { SetInt(Object, (Int32)Val); }

//! Set from a string
void mxflib::MDTraits_BasicInt::SetString(MDObject *Object, std::string Val) 
{ 
	// Allow hex values in string
	if((Val.length() > 2) && (Val.at(1) == 'x') && (Val.at(0) == '0'))
	{
		std::stringstream Conv(Val);
		int IntVal;
		Conv >> std::hex >> IntVal;
		SetInt(Object, (Int32)IntVal);
	}
	else SetInt(Object, (Int32)atoi(Val.c_str())); 
}

//! Get Int64
Int64 mxflib::MDTraits_BasicInt::GetInt64(const MDObject *Object) const { return (Int64) GetInt(Object); }

//! Get UInt64
UInt64 mxflib::MDTraits_BasicInt::GetUInt64(const MDObject *Object) const { return (UInt64) GetUInt(Object); }

//!	Get string from an integer
std::string mxflib::MDTraits_BasicInt::GetString(const MDObject *Object) const 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%d", GetInt(Object));
	return std::string(Buffer);
}

size_t MDTraits_BasicInt::ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	// Limit the size attempted to be read to the size of the type
	UInt32 TypeSize = Object->GetValueType() ? Object->GetValueType()->Size : 0;
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
size_t mxflib::ReadValueUInt(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	// Limit the size attempted to be read to the size of the type
	UInt32 TypeSize = Object->GetValueType() ? Object->GetValueType()->Size : 0;
	if(TypeSize && (Size > TypeSize)) Size = TypeSize;

	if(Size >= 8)
	{
		Object->SetUInt64(GetU64(Buffer));
		return 8;
	}
	else if(Size >= 4)
	{
		Object->SetUInt64(GetU32(Buffer));
		return 4;
	}
	else if(Size >= 2)
	{
		Object->SetUInt64(GetU16(Buffer));
		return 2;
	}
	else if(Size >= 1)
	{
		Object->SetUInt64(GetU8(Buffer));
		return 1;
	}

	Object->SetUInt64(0);
	return 0;
}


/*****************************
**   Int8 Implementations   **
*****************************/

//! Set Int8 from an Int32
void mxflib::MDTraits_Int8::SetInt(MDObject *Object, Int32 Val) 
{ 
	if(Object->GetData().Size != 1)
	{
		Object->MakeSize(1);
		
		if(Object->GetData().Size != 1)
		{
			error("Tried to set an MDObject to a 1-byte value, but could not set length to 1\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	Int8 i = Val;
	Object->SetData(1, (UInt8*)&i);
}

//! Get Int32 from an Int8
Int32 mxflib::MDTraits_Int8::GetInt(const MDObject *Object) const
{
	size_t Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 1)
	{
		error("Tried to read a 1-byte value from an MDObject that has size %d\n", Size);
		return 0;
	}

	// Return the value promoted to 32-bits
	return (Int32) *((const Int8*)(Object->GetData().Data));
}

//! Get UInt32 from an Int8
/*! \note
 *	This function will return 128 through 255 for bit values 10000000 through 11111111
 *	even though an Int8 cannot store them. This is as opposed to the option of returning
 *  0xffffff80 through 0xffffffff for those values.
 */
UInt32 mxflib::MDTraits_Int8::GetUInt(const MDObject *Object) const
{ 
	size_t Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 1)
	{
		error("Tried to read a 1-byte value from an MDObject that has size %d\n", Size);
		return 0;
	}

	// Return the value promoted to 32-bits
	return (UInt32) *((const UInt8*)(Object->GetData().Data));
}


/******************************
**   UInt8 Implementations   **
******************************/

//!	Get string from a UInt8
std::string MDTraits_UInt8::GetString(const MDObject *Object) const
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%02x", GetUInt(Object));
	return std::string(Buffer);
}

//! Read value from memory buffer
size_t MDTraits_UInt8::ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	return ReadValueUInt(Object, Buffer, Size, Count);
}


/******************************
**   Int16 Implementations   **
******************************/

//! Set Int16 from an Int32
void mxflib::MDTraits_Int16::SetInt(MDObject *Object, Int32 Val) 
{ 
	if(Object->GetData().Size != 2)
	{
		Object->MakeSize(2);
		
		if(Object->GetData().Size != 2)
		{
			error("Tried to set an MDObject to a 2-byte value, but could not set length to 2\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	// But as this is a multi-byte value we may need
	// to byte swap it...
	// Note that the swap is done in an unsigned int
	// to prevent any sign problems!
	UInt16 i = Swap((UInt16)Val);

	Object->SetData(2, (UInt8*)&i);
}

//! Get Int32 from an Int16
Int32 mxflib::MDTraits_Int16::GetInt(const MDObject *Object) const
{ 
	size_t Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 2)
	{
		error("Tried to read a 2-byte value from an MDObject that has size %d\n", Size);
		return 0;
	}

	// Build the 16-bit value
	Int16 Val = ((Object->GetData().Data)[0] << 8) | (Object->GetData().Data)[1];

	// Return that value cast up to 32-bit
	return static_cast<Int32>(Val);
}

//! Get UInt32 from an Int16
UInt32 mxflib::MDTraits_Int16::GetUInt(const MDObject *Object) const
{ 
	size_t Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 2)
	{
		error("Tried to read a 2-byte value from an MDObject that has size %d\n", Size);
		return 0;
	}

	// Build the 16-bit value
	return ((Object->GetData().Data)[0] << 8) | (Object->GetData().Data)[1];
}


/*******************************
**   UInt16 Implementations   **
*******************************/

//!	Get string from a UInt16
std::string MDTraits_UInt16::GetString(const MDObject *Object) const
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", GetUInt(Object));
	return std::string(Buffer);
}

//! Read value from memory buffer
size_t MDTraits_UInt16::ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	return ReadValueUInt(Object, Buffer, Size, Count);
}


/******************************
**   Int32 Implementations   **
******************************/

//! Set Int32 from an Int32
void mxflib::MDTraits_Int32::SetInt(MDObject *Object, Int32 Val)
{ 
	if(Object->GetData().Size != 4)
	{
		Object->MakeSize(4);
		
		if(Object->GetData().Size != 4)
		{
			error("Tried to set an MDObject to a 4-byte value, but could not set length to 4\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	// But as this is a multi-byte value we may need
	// to byte swap it...
	// Note that the swap is done in an unsigned int
	// to prevent any sign problems!
	UInt32 i = Swap((UInt32)Val);

	Object->SetData(4, (UInt8*)&i);
}

//! Get Int32 from an Int32
Int32 mxflib::MDTraits_Int32::GetInt(const MDObject *Object) const
{ 
	size_t Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 4)
	{
		error("Tried to read a 4-byte value from an MDObject that has size %d\n", Size);
		return 0;
	}

	// Build the 32-bit value
	return ((Object->GetData().Data)[0] << 24) | ((Object->GetData().Data)[1] << 16)
		 | ((Object->GetData().Data)[2] << 8) | (Object->GetData().Data)[3];
}

//! Get UInt32 from an Int32
UInt32 mxflib::MDTraits_Int32::GetUInt(const MDObject *Object) const
{
	// As the return value is the same size as our working variables
	// the signed to unsigned conversion should be safe like this
	return (UInt32)GetInt(Object);
}


/*******************************
**   UInt32 Implementations   **
*******************************/

//!	Get string from a UInt32
std::string MDTraits_UInt32::GetString(const MDObject *Object) const
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", GetUInt(Object));
	return std::string(Buffer);
}

//! Read value from memory buffer
size_t MDTraits_UInt32::ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	return ReadValueUInt(Object, Buffer, Size, Count);
}



/******************************
**   Int64 Implementations   **
******************************/

//! Set Int64 from an Int64
void mxflib::MDTraits_Int64::SetInt64(MDObject *Object, Int64 Val) 
{ 
	if(Object->GetData().Size != 8)
	{
		Object->MakeSize(8);
		
		if(Object->GetData().Size != 8)
		{
			error("Tried to set an MDObject to a 8-byte value, but could not set length to 8\n");
			return;
		}
	}

	// Now we know the value will fit, set it
	// But as this is a multi-byte value we may need
	// to byte swap it...
	// Note that the swap is done in an unsigned int
	// to prevent any sign problems!
	UInt64 i = Swap((UInt64)Val);

	Object->SetData(8, (UInt8*)&i);
}

//! Set from an Int32
void mxflib::MDTraits_Int64::SetInt(MDObject *Object, Int32 Val) { SetInt64(Object, (Int64)Val); }

//! Set from a UInt32
void mxflib::MDTraits_Int64::SetUInt(MDObject *Object, UInt32 Val) { SetUInt64(Object, (UInt64)Val); }

//! Set from a UInt64
/*! DRAGONS: Will this always work? This relies on the UInt64 -> Int64 -> UInt64
 *           conversion being valid for all values!
 */
void mxflib::MDTraits_Int64::SetUInt64(MDObject *Object, UInt64 Val) { SetInt64(Object, (UInt64)Val); }

//! Set from a string
void mxflib::MDTraits_Int64::SetString(MDObject *Object, std::string Val) { SetInt64(Object, ato_Int64(Val.c_str())); }

//!	Get string from an integer
std::string mxflib::MDTraits_Int64::GetString(const MDObject *Object) const
{ 
	return Int64toString(GetInt64(Object));
}

//! Get Int
Int32 mxflib::MDTraits_Int64::GetInt(const MDObject *Object) const { return (Int32) GetInt64(Object); }

//! Get UInt
UInt32 mxflib::MDTraits_Int64::GetUInt(const MDObject *Object) const { return (UInt32) GetUInt64(Object); }

//! Get Int64
Int64 mxflib::MDTraits_Int64::GetInt64(const MDObject *Object) const
{ 
	size_t Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 8)
	{
		error("Tried to read an 8-byte value from an MDObject that has size %d\n", Size);
		return 0;
	}

	// Build the 64-bit value using UInt32s to prevent possible sign problems
	UInt32 HiVal = ((Object->GetData().Data)[0] << 24) | ((Object->GetData().Data)[1] << 16)
		         | ((Object->GetData().Data)[2] << 8) | (Object->GetData().Data)[3];
	UInt32 LoVal = ((Object->GetData().Data)[4] << 24) | ((Object->GetData().Data)[5] << 16)
		         | ((Object->GetData().Data)[6] << 8) | (Object->GetData().Data)[7];

	return (UInt64(HiVal) << 32) | LoVal;
}

//! Get UInt64
UInt64 mxflib::MDTraits_Int64::GetUInt64(const MDObject *Object) const
{
	// As the return value is the same size as our working variables
	// the signed to unsigned conversion should be safe like this
	return (UInt64)GetInt64(Object);
}


/*******************************
**   UInt64 Implementations   **
*******************************/

//!	Get string from an integer
std::string mxflib::MDTraits_UInt64::GetString(const MDObject *Object) const
{ 
	return UInt64toString(GetUInt64(Object));
}

//! Read value from memory buffer
size_t MDTraits_UInt64::ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	return ReadValueUInt(Object, Buffer, Size, Count);
}


/***************************************
**   ISO 7-bit char Implementations   **
***************************************/

//!	Get string from an ISO7
std::string MDTraits_ISO7::GetString(const MDObject *Object) const
{ 
	char Buffer[2];					
	Buffer[0]= GetInt(Object);
	Buffer[1]='\0';
	return std::string(Buffer);
}

//! Set an ISO7 from a string
void MDTraits_ISO7::SetString(MDObject *Object, std::string Val)
{
	const char *StringData = Val.c_str();
	SetInt(Object, *StringData);
}


/************************************
**   UTF-16 char Implementations   **
************************************/

//!	Get string from a UTF16 - returns UTF8 coded version of a single UTF16 code unit - surrogates are not converted!
/*! Unicode Table 3-5. UTF-8 Bit Distribution
Unicode                     1st Byte 2nd Byte 3rd Byte 4th Byte
00000000 0xxxxxxx           0xxxxxxx
00000yyy yyxxxxxx           110yyyyy 10xxxxxx
zzzzyyyy yyxxxxxx           1110zzzz 10yyyyyy 10xxxxxx
000uuuuu zzzzyyyy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
*/
std::string MDTraits_UTF16::GetString(const MDObject *Object) const
{ 
	char Buffer[4];

	UInt16 Value = GetInt(Object);

	// Is this a simple 7-bit character?
	if((Value & (UInt16)(~0x7f)) == 0)
	{
		Buffer[0] = (char)Value;
		Buffer[1] = 0;
	}
	// How about a value that can be represented in 2 UTF8 bytes?
	else if((Value & (UInt16)(~0x7ff)) == 0)
	{
		Buffer[0] = (char)(0xc0 | (Value >> 6));
		Buffer[1] = (char)(0x80 | (Value & 0x3f));
		Buffer[2] = 0;
	}
	// Otherwise it will take 3 bytes
	else
	{
		Buffer[0] = (char)(0xe0 | (Value >> 12));
		Buffer[1] = (char)(0x80 | ((Value >> 6) & 0x3f));
		Buffer[2] = (char)(0x80 | (Value & 0x3f));
		Buffer[3] = 0;
	}

	return std::string(Buffer);
}

//! Set a UTF16 from a string containing a UTF8 character - surrogates are not converted!
/*! \note Invalid input strings (such as incomplete multi-byte UTF-8 sequences) will produce unpredictable results

Unicode Table 3-5. UTF-8 Bit Distribution
Unicode                     1st Byte 2nd Byte 3rd Byte 4th Byte
00000000 0xxxxxxx           0xxxxxxx
00000yyy yyxxxxxx           110yyyyy 10xxxxxx
zzzzyyyy yyxxxxxx           1110zzzz 10yyyyyy 10xxxxxx
000uuuuu zzzzyyyy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
*/
void MDTraits_UTF16::SetString(MDObject *Object, std::string Val)
{
	char Buffer[3];						//!< Buffer to hold the UTF-8 version
	int Value;							//!< Built UTF-16 version

	// Read the UTF-8 value
	strncpy(Buffer, Val.c_str(), 3);

	// Is is a simple 7-bit character?
	if((Buffer[0] & 0x80) == 0)
	{
		Value = (UInt16)Buffer[0];
	}
	// Or a 2-byte code?
	else if((Buffer[0] & 0xe0) == 0xc0)
	{
		Value = (((UInt16)(Buffer[0] & 0x1f)) << 6) | ((UInt16)(Buffer[1] & 0x3f));
	}
	// Or a 3-byte code?
	else if((Buffer[0] & 0xf0) == 0xe0)
	{
		Value = (((UInt16)(Buffer[0] & 0x0f)) << 12) | (((UInt16)(Buffer[1] & 0x3f)) << 6) | ((UInt16)(Buffer[2] & 0x3f));
	}
	// Anything that requires > 16 bits becomes a "replacement character"
	// Not strictly 100% valid Unicode perhaps but not much else is possible!
	else
	{
		Value = 0xfffd;
	}

	SetUInt(Object, Value);
}


/************************************
**  UTF-16 string Implementations  **
************************************/

//!	Get UTF8 string from a UTF16 string - surrogates are converted
/*! Unicode Table 3-5. UTF-8 Bit Distribution
Unicode                     1st Byte 2nd Byte 3rd Byte 4th Byte
00000000 0xxxxxxx           0xxxxxxx
00000yyy yyxxxxxx           110yyyyy 10xxxxxx
zzzzyyyy yyxxxxxx           1110zzzz 10yyyyyy 10xxxxxx
000uuuuu zzzzyyyy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx

Unicode Table 3-4. UTF-16 Bit Distribution
xxxxxxxx xxxxxxxx           xxxxxxxx xxxxxxxx
000uuuuu xxxxxxxx xxxxxxxx  110110ww wwxxxxxx 110111xx xxxxxxxx
Where wwww = uuuuu - 1.

So UTF16 (surrogate-pair) -> UTF8:
110110ww wwzzzzyy 110111yy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
*/
std::string MDTraits_UTF16String::GetString(const MDObject *Object) const
{ 
	std::string Ret;				//!< Return value being built
	char Buffer[5];					//!< Buffer for building each Unicode symbol
	UInt16 Surrogate = 0;			//!< First surrogate if processing a pair

	MDObject::const_iterator it = Object->begin();
	while(it != Object->end())
	{
		UInt16 Value = (*it).second->GetInt();

		// Exit when a null is found
		if(!Value) break;

		if(Surrogate)
		{
			UInt16 UValue = ((Surrogate >> 6) & 0x000f) + 1;
			Buffer[0] = (char)(0xf0 | (UValue >> 2));
			Buffer[1] = (char)(0x80 | ((UValue & 0x03) << 4) | ((Surrogate & 0x003c) >> 2));
			Buffer[2] = (char)(0x80 | ((Surrogate & 0x03) << 4) | ((Value & 0x03c0) >> 6));
			Buffer[3] = (char)(0x80 | (Value & 0x3f));
			Buffer[4] = 0;

			// End surrogate-pair processing
			Surrogate = 0;
		}
		else
		{
			// Is this a simple 7-bit character?
			if((Value & (UInt16)(~0x7f)) == 0)
			{
				Buffer[0] = (char)Value;
				Buffer[1] = 0;
			}
			// How about a value that can be represented in 2 UTF8 bytes?
			else if((Value & (UInt16)(~0x7ff)) == 0)
			{
				Buffer[0] = (char)(0xc0 | (Value >> 6));
				Buffer[1] = (char)(0x80 | (Value & 0x3f));
				Buffer[2] = 0;
			}
			// Is this a surrogate start ?
			else if((Value & 0xfc00) == 0xd800)
			{
				Surrogate = Value;
			}
			// Otherwise it will take 3 bytes
			else
			{
				Buffer[0] = (char)(0xe0 | (Value >> 12));
				Buffer[1] = (char)(0x80 | ((Value >> 6) & 0x3f));
				Buffer[2] = (char)(0x80 | (Value & 0x3f));
				Buffer[3] = 0;
			}
		}

		if(!Surrogate) Ret += std::string(Buffer);
		it++;
	}

	return Ret;
}


//! Set a UTF16 from a string containing a UTF8 character - surrogates are converted
/*! Unicode Table 3-5. UTF-8 Bit Distribution
Unicode                     1st Byte 2nd Byte 3rd Byte 4th Byte
00000000 0xxxxxxx           0xxxxxxx
00000yyy yyxxxxxx           110yyyyy 10xxxxxx
zzzzyyyy yyxxxxxx           1110zzzz 10yyyyyy 10xxxxxx
000uuuuu zzzzyyyy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx

Unicode Table 3-4. UTF-16 Bit Distribution
xxxxxxxx xxxxxxxx           xxxxxxxx xxxxxxxx
000uuuuu xxxxxxxx xxxxxxxx  110110ww wwxxxxxx 110111xx xxxxxxxx
Where wwww = uuuuu - 1.

So UTF8 -> UTF16 (surrogate-pair):
11110uuu 10uuzzzz 10yyyyyy 10xxxxxx  110110ww wwzzzzyy 110111yy yyxxxxxx
*/
void MDTraits_UTF16String::SetString(MDObject *Object, std::string Val)
{
	UInt32 Len;							//!< Length of the UTF-8 version
	char *Buffer;						//!< Buffer to hold the UTF-8 version
	UInt32 RetLen;						//!< Length of returned UTF-16 string (in 16-bit words)
	int Value;							//!< Built UTF-16 version of the current character

	// Read the UTF-8 value
	Len = static_cast<UInt32>(Val.size());
	Buffer = new char[Len + 1];
	strcpy(Buffer, Val.c_str());

	// Initially assume that the return string will be the same size (times two) as the input string.
	// It will be no longer as four-byte UTF-16 codes start as four UTF-8 bytes
	if(GetStringTermination())
		Object->Resize(Len + 1);
	else
		Object->Resize(Len);


	// Count the actual length
	RetLen = 0;

	char *p = Buffer;
	MDObject::iterator it = Object->begin();
	while((Len) && (it != Object->end()))
	{
		// Is is a simple 7-bit character?
		if((p[0] & 0x80) == 0)
		{
			Value = (UInt16)*p;
			p++;
			Len--;
		}
		// Or a 2-byte code?
		else if((Len >= 2) && ((p[0] & 0xe0) == 0xc0))
		{
			Value = (((UInt16)(p[0] & 0x1f)) << 6) | ((UInt16)(p[1] & 0x3f));
			p += 2;
			Len -= 2;
		}
		// Or a 3-byte code?
		else if((Len >= 3) && ((p[0] & 0xf0) == 0xe0))
		{
			Value = (((UInt16)(p[0] & 0x0f)) << 12) | (((UInt16)(p[1] & 0x3f)) << 6) | ((UInt16)(p[2] & 0x3f));
			p += 3;
			Len -= 3;
		}
		// Handle surrogate
		else if((Len >= 4) && ((p[0] & 0xf0) == 0xf0))
		{
			// Build leading surrogate
			UInt16 UValue = (((UInt16)(p[0] & 0x07)) << 2) | (((UInt16)(p[1] & 0x30)) >> 4);
			Value = 0xd800 | ((UValue - 1) << 6) | ((p[1] & 0x0f) << 2) | ((p[2] & 0x30) >> 4);

			(*it).second->SetUInt(Value);
			RetLen++;

			// Move forward - quit if no more entries available (could be fixed size array)
			it++;
			if(it == Object->end()) break;

			// Build trailing surrogate (will be added at end of loop below)
			Value = 0xdc00 | ((p[2] & 0x0f) << 6) | (p[3] & 0x3f);

			p += 4;
			Len -= 4;
		}
		// Errors get replaced with "replacement character"
		else
		{
			Value = 0xfffd;
		}

		// Set this value;
		(*it).second->SetUInt(Value);
		RetLen++;
		it++;
	}

	// Terminate the string if requested
	if(GetStringTermination())

	{
		if(it != Object->end())
		{
			(*it).second->SetUInt(0);
			RetLen++;
		}
	}


	// Shrink output array to the actual size required
	Object->Resize(RetLen);

	// Free our temporary buffer
	delete[] Buffer;
}


/***********************************
**  String Array Implementations  **
***********************************/

//!	Get UTF8 string from a StringArray - surrogates are converted
/*! Unicode Table 3-5. UTF-8 Bit Distribution
Unicode                     1st Byte 2nd Byte 3rd Byte 4th Byte
00000000 0xxxxxxx           0xxxxxxx
00000yyy yyxxxxxx           110yyyyy 10xxxxxx
zzzzyyyy yyxxxxxx           1110zzzz 10yyyyyy 10xxxxxx
000uuuuu zzzzyyyy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx

Unicode Table 3-4. UTF-16 Bit Distribution
xxxxxxxx xxxxxxxx           xxxxxxxx xxxxxxxx
000uuuuu xxxxxxxx xxxxxxxx  110110ww wwxxxxxx 110111xx xxxxxxxx
Where wwww = uuuuu - 1.

So UTF8 -> UTF16 (surrogate-pair):
11110uuu 10uuzzzz 10yyyyyy 10xxxxxx  110110ww wwzzzzyy 110111yy yyxxxxxx
*/std::string MDTraits_StringArray::GetString(const MDObject *Object) const
{
	std::string Ret;				//!< Return value being built
	char Buffer[5];					//!< Buffer for building each Unicode symbol
	UInt16 Surrogate = 0;			//!< First surrogate if processing a pair

	Ret = "\"";
	MDObject::const_iterator it = Object->begin();
	while(it != Object->end())
	{
		UInt16 Value = (*it).second->GetInt();

		// Split strings when a null is found
		if(!Value)
		{
			if(++it == Object->end()) Ret += "\""; else Ret += "\", \"";
			continue;
		}

		if(Surrogate)
		{
			UInt16 UValue = ((Surrogate >> 6) & 0x000f) + 1;
			Buffer[0] = (char)(0xf0 | (UValue >> 2));
			Buffer[1] = (char)(0x80 | ((UValue & 0x03) << 4) | ((Surrogate & 0x003c) >> 2));
			Buffer[2] = (char)(0x80 | ((Surrogate & 0x03) << 4) | ((Value & 0x03c0) >> 6));
			Buffer[3] = (char)(0x80 | (Value & 0x3f));
			Buffer[4] = 0;

			// End surrogate-pair processing
			Surrogate = 0;
		}
		else
		{
			// Is this a simple 7-bit character?
			if((Value & (UInt16)(~0x7f)) == 0)
			{
				Buffer[0] = (char)Value;
				Buffer[1] = 0;
			}
			// How about a value that can be represented in 2 UTF8 bytes?
			else if((Value & (UInt16)(~0x7ff)) == 0)
			{
				Buffer[0] = (char)(0xc0 | (Value >> 6));
				Buffer[1] = (char)(0x80 | (Value & 0x3f));
				Buffer[2] = 0;
			}
			// Is this a surrogate start ?
			else if((Value & 0xfc00) == 0xd800)
			{
				Surrogate = Value;
			}
			// Otherwise it will take 3 bytes
			else
			{
				Buffer[0] = (char)(0xe0 | (Value >> 12));
				Buffer[1] = (char)(0x80 | ((Value >> 6) & 0x3f));
				Buffer[2] = (char)(0x80 | (Value & 0x3f));
				Buffer[3] = 0;
			}
		}

		if(!Surrogate) Ret += std::string(Buffer);
		it++;
	}

	return Ret;
}


//! Set a StringArray from a string containing a UTF8 characters - surrogates are converted
void MDTraits_StringArray::SetString(MDObject *Object, std::string Val)
{
	error("MDTraits_StringArray::SetString() not supported!\n");
}


/**************************************
**   Default Array Implementations   **
***************************************/

void MDTraits_BasicArray::SetInt(MDObject *Object, Int32 Val)
{
	MDObject::iterator it;

	it = Object->begin();
	while(it != Object->end())
	{
		(*it).second->SetInt(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetInt64(MDObject *Object, Int64 Val)
{
	MDObject::iterator it;

	it = Object->begin();
	while(it != Object->end())
	{
		(*it).second->SetInt64(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetUInt(MDObject *Object, UInt32 Val)
{
	MDObject::iterator it;

	it = Object->begin();
	while(it != Object->end())
	{
		(*it).second->SetUInt(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetUInt64(MDObject *Object, UInt64 Val)
{
	MDObject::iterator it;

	it = Object->begin();
	while(it != Object->end())
	{
		(*it).second->SetUInt64(Val);
		it++;
	}
}


//! Set string for basic array types
/*! \note This will fail if one of the array items includes a comma
 */
void MDTraits_BasicArray::SetString(MDObject *Object, std::string Val)
{
	MDObject::iterator it;

	size_t LastComma = std::string::npos;
	
	it = Object->begin();

	for(;;)
	{
		// If we have not yet seen a comma, search from the start
		size_t Comma = Val.find(",", (LastComma == std::string::npos ? 0 : LastComma + 1));

		// If we are already at the end of the list, add another
		if(it == Object->end()) 
		{
			// DRAGONS: This will fail for arrays of more that 4 billion entries
			Object->Resize(static_cast<UInt32>(Object->size() + 1));
			it = Object->end();
			it--;
		}

		if(Comma == std::string::npos)
		{
			(*it).second->SetString(std::string(Val,LastComma+1, std::string::npos));
			return;
		}

		(*it).second->SetString(std::string(Val,LastComma+1, (Comma-LastComma)-1));

		it++;

		LastComma = Comma;
	}
}


Int32 MDTraits_BasicArray::GetInt(const MDObject *Object) const
{
	MDObject::const_iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetInt();
}

Int64 MDTraits_BasicArray::GetInt64(const MDObject *Object) const
{
	MDObject::const_iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetInt64();
}


UInt32 MDTraits_BasicArray::GetUInt(const MDObject *Object) const
{
	MDObject::const_iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetUInt();
}

UInt64 MDTraits_BasicArray::GetUInt64(const MDObject *Object) const
{
	MDObject::const_iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetUInt64();
}

std::string MDTraits_BasicArray::GetString(const MDObject *Object) const
{
	std::string Ret;

	MDObject::const_iterator it;

	Ret = "";

	it = Object->begin();
	while(it != Object->end())
	{
#ifndef	FORMAT_ARRAY
#define FORMAT_ARRAY 0
#endif

#if FORMAT_ARRAY==0 // default legacy behaviour
		// dump as a comma separated list
		if(Ret.length() > 0) Ret += ", ";
		Ret += (*it).second->GetString();
#elif FORMAT_ARRAY==1
		// dump as a whitespaced list
		if(Ret.length() != 0) Ret += " ";
		Ret += (*it).second->GetString();
#else
		// dump as an array of elements
		if(Ret.length() != 0) Ret += "\n";
		Ret += "<";
		Ret += (*it).second->Name();
		Ret += ">";
		Ret += (*it).second->GetString();
		Ret += "</";
		Ret += (*it).second->Name();
		Ret += ">";
#endif
		it++;
	}
	return Ret;
}


size_t MDTraits_BasicArray::ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	const MDTypePtr &ValueType = Object->GetValueType();
	if(!ValueType) return 0;

	// Start with no children in the object
	Object->clear();

	// Either the size of each item to read, or the total size (for unknown count)
	size_t ThisSize = Size;

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
		ThisSize /= Count;

		if((ThisSize * Count) != Size)
		{
			error("MDTraits_BasicArray::ReadValue() requested to read %d items from %d bytes, not an integer number of items\n", (int)Count, (int)Size);
		}
	}

	// Number of bytes read
	size_t Bytes = 0;

	// If this object is a batch we need to read its header
	if((Object->GetContainerType() == BATCH) || (ValueType->GetArrayClass() == ARRAYEXPLICIT))
	{
		if(Size < 8)
		{
			error("Tried to read a batch of type %s but less than 8 bytes available\n", Object->Name().c_str());
			return 0;
		}

		UInt32 ItemCount = GetU32(Buffer);
		UInt32 ItemSize = GetU32(&Buffer[4]);
		ThisSize = static_cast<size_t>(ItemSize);

		Buffer += 8;
		Bytes += 8;
		Size -= 8;

		// Only update the count if it was unknown (this allows a valid request to read less than available)
		if(UnknownCount) 
		{
			Count = ItemCount;

			// Now the count IS known
			UnknownCount = false;
		}
		else
		{
			if(Count > (int)ItemCount)
			{
				error("Tried to read more items from batch of type %s than available - requested = %u, available = %u\n", Object->Name().c_str(), Count, ItemCount);
			}
		}

		if((ItemCount * ItemSize) > Size)
		{
			error("Invalid batch of type %s - count = %u, item size = %u so 0x%08x bytes required but only 0x%08x available\n", Object->Name().c_str(), ItemCount, ItemSize, (ItemCount * ItemSize), Size);
			
			// Make the count safe
			Count = static_cast<int>(Size / ItemSize);
		}
	}

	// Figure out the maximum number of items to read, or zero if open-ended
	UInt32 MaxItems = ValueType->Size;

	// Count of actual items read, and bytes read in doing so
	UInt32 ActualCount = 0;

	while(Count)
	{
		size_t ThisBytes;

		MDObjectPtr NewItem = new MDObject(ValueType->EffectiveBase());

		// DRAGONS: We need to add the new item before reading its value because some complex traits need to know the parent details
		Object->AddChild(NewItem);
		ActualCount++;

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

std::string MDTraits_BasicStringArray::GetString(const MDObject *Object) const
{
	std::string Ret;

	MDObject::const_iterator it;

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


void MDTraits_BasicStringArray::SetString(MDObject *Object, std::string Val)
{
	MDObject::iterator it;

	// TODO: Sanity check the range here
	UInt32 Size = static_cast<UInt32>(Val.length());
	size_t Index = 0;

	if(GetStringTermination())
		Object->Resize(Size + 1);
	else
		Object->Resize(Size);

	it = Object->begin();
	while(it != Object->end())
	{
		std::string Temp;
		char c = Val[Index];

		// Stop at a terminating NUL
		if(c=='\0') break;

		Temp = c;
		(*it).second->SetString(Temp);

		it++;
		Index++;
	}

	if(GetStringTermination())
	{
		if(it != Object->end())
		{
			(*it).second->SetUInt(0);
			it++;
			Index++;
		}
	}

	Object->Resize(static_cast<UInt32>(Index));
}


/****************************
**   Raw Implementations   **
****************************/
// TODO: The raw implementations should check "endian" and byte-swap if required (or should they?)

Int32 MDTraits_Raw::GetInt(const MDObject *Object) const
{
	if(Object->GetData().Size >= 8)
	{
		return static_cast<Int32>(GetI64(Object->GetData().Data));
	}
	else if(Object->GetData().Size >= 4)
	{
		return GetI32(Object->GetData().Data);
	}
	else if(Object->GetData().Size >= 2)
	{
		return static_cast<Int32>(GetI16(Object->GetData().Data));
	}
	else if(Object->GetData().Size >= 1)
	{
		return static_cast<Int32>(*(Object->GetData().Data));
	}

	return 0;
}

UInt32 MDTraits_Raw::GetUInt(const MDObject *Object) const
{
	if(Object->GetData().Size >= 8)
	{
		return static_cast<UInt32>(GetU64(Object->GetData().Data));
	}
	else if(Object->GetData().Size >= 4)
	{
		return GetU32(Object->GetData().Data);
	}
	else if(Object->GetData().Size >= 2)
	{
		return static_cast<UInt32>(GetU16(Object->GetData().Data));
	}
	else if(Object->GetData().Size >= 1)
	{
		return static_cast<UInt32>(*(Object->GetData().Data));
	}

	return 0;
}

Int64 MDTraits_Raw::GetInt64(const MDObject *Object) const
{
	if(Object->GetData().Size >= 8)
	{
		return GetI64(Object->GetData().Data);
	}
	else if(Object->GetData().Size >= 4)
	{
		return static_cast<Int64>(GetI32(Object->GetData().Data));
	}
	else if(Object->GetData().Size >= 2)
	{
		return static_cast<Int64>(GetI16(Object->GetData().Data));
	}
	else if(Object->GetData().Size >= 1)
	{
		return static_cast<Int64>(*(Object->GetData().Data));
	}

	return 0;
}

UInt64 MDTraits_Raw::GetUInt64(const MDObject *Object) const
{
	if(Object->GetData().Size >= 8)
	{
		return GetU64(Object->GetData().Data);
	}
	else if(Object->GetData().Size >= 4)
	{
		return static_cast<UInt64>(GetU32(Object->GetData().Data));
	}
	else if(Object->GetData().Size >= 2)
	{
		return static_cast<UInt64>(GetU16(Object->GetData().Data));
	}
	else if(Object->GetData().Size >= 1)
	{
		return static_cast<UInt64>(*(Object->GetData().Data));
	}

	return 0;
}

std::string MDTraits_Raw::GetString(const MDObject *Object) const
{
	std::string Ret;
	size_t Count = Object->GetData().Size;
	const UInt8 *Data = Object->GetData().Data;

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


void MDTraits_Raw::SetString(MDObject *Object, std::string Val)
{
	size_t Count = Object->GetData().Size;
	int Value = -1;
	UInt8 *Data = new UInt8[Count];
	UInt8 *pD = Data;
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
	}

	Object->SetData(Object->GetData().Size, Data);

	// Clean up
	delete[] Data;
}


/**********************************
**   Raw Array Implementations   **
**********************************/

std::string MDTraits_RawArray::GetString(const MDObject *Object) const
{
	std::string Ret;

	if( Object->size() > GetStringLimit() )
	{
		char Buffer[32];
		if(sizeof(size_t) == 4)
			sprintf( Buffer, "RAW[0x%08x]", (int)Object->size() );
		else
			sprintf( Buffer, "RAW[0x%s]", Int64toHexString(Object->size(),8).c_str() );

		Ret = Buffer;
		return Ret;
	}

	MDObject::const_iterator it;

	Ret = "";

	it = Object->begin();
	while(it != Object->end())
	{
		// Buffer for standard sizes
		char Buffer[32];

		if(Ret.length() != 0) Ret += " ";

		size_t Size = (*it).second->GetData().Size;
		
		// Invalidate buffer
		Buffer[0] = 0;

		if(Size == 1) sprintf(Buffer, "%02x", (*it).second->GetUInt());
		else if(Size == 2) sprintf(Buffer, "%04x", (*it).second->GetUInt());
		else if(Size == 4) sprintf(Buffer, "%08x", (*it).second->GetUInt());
		else if(Size == 8) strcpy( Buffer, Int64toHexString((*it).second->GetUInt64(), 8).c_str() );
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


size_t MDTraits_Raw::ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	// If multiple items are found read them all "blindly"
	size_t FullSize = Size;
	if(Count) FullSize *= Count;

	// Try and make exactly the right amount of room
	// Some objects will not allow this and will return a different size
	size_t ObjSize = Object->MakeSize(FullSize);

	// If the object is too small, only read what we can
	if(ObjSize < FullSize)
	{
		Object->SetData(ObjSize, Buffer);
		return ObjSize;
	}

	// If the object is exactly the right size read it all in
	if(ObjSize == FullSize)
	{
		Object->SetData(FullSize, Buffer);
	}
	else
	{
		// If the object ends up too big we build a copy
		// of the data with zero padding
		UInt8 *Temp = new UInt8[ObjSize];

		memcpy(Temp, Buffer, FullSize);
		memset(&Temp[FullSize], 0, ObjSize - FullSize);
		Object->SetData(ObjSize, Temp);

		delete[] Temp;
	}

	return FullSize;
}


void MDTraits_RawArray::SetString(MDObject *Object, std::string Val)
{
	MDObject::iterator it;

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


/*****************************
**   UUID Implementations	**
*****************************/

//! Current default output format for ULs
OutputFormatEnum MDTraits_UUID::DefaultFormat = -1;

void MDTraits_UUID::SetString(MDObject *Object, std::string Val)
{
	// Make a safe copy of the value that will not be cleaned-up by string manipulation
	const int VALBUFF_SIZE = 256;
	char ValueBuff[VALBUFF_SIZE];
	strncpy(ValueBuff, Val.c_str(), VALBUFF_SIZE -1);
	const char *p = ValueBuff;

	size_t DataSize = Object->GetData().Size;
	size_t Count = DataSize;
	int Value = -1;
	UInt8 *Data = new UInt8[Count];
	UInt8 *pD = Data;

	// Is this a UL than needs to be end-swapped
	bool EndSwap = false;

	// Is this an OID format, which will need converting, then end swapping
	bool OIDFormat = false;

	// Check for URN format
	if((tolower(*p) == 'u') && (tolower(p[1]) == 'r') && (tolower(p[2]) == 'n') && (tolower(p[3]) == ':'))
	{
		if(    (strcasecmp(Val.substr(0,7).c_str(), "urn:ul:") == 0) 
			|| (strcasecmp(Val.substr(0,13).c_str(), "urn:smpte-ul:") == 0) 
			|| (strcasecmp(Val.substr(0,13).c_str(), "urn:smpte:ul:") == 0) 
			|| (strcasecmp(Val.substr(0,9).c_str(), "urn:x-ul:") == 0) )
		{
			EndSwap = true;
		}
		else if(strcasecmp(Val.substr(0,8).c_str(), "urn:oid:") == 0)
		{
			OIDFormat = true;
		}

		p += Val.rfind(':') + 1;
	}

	// During this loop Value = -1 when no digits of a number are mid-process
	// This stops a double space being regarded as a small zero in between two spaces
	int DigitCount = 0;
	while(Count)
	{
		int Digit;

		if((*p == 0) && (Value == -1)) Value = 0;

		if(*p >= '0' && *p <='9') Digit = (*p) - '0';
		else if(*p >= 'a' && *p <= 'f') Digit = (*p) - 'a' + 10;
		else if(*p >= 'A' && *p <= 'F') Digit = (*p) - 'A' + 10;
		else
		{
			// If we meet "[" before any digits, this as a UL - which will need to be end-swapped
			if((*p == '[') && (Count == DataSize))
			{
				EndSwap = true;
			}

			if(Value == -1)
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
				DigitCount = 0;

				continue;
			}
		}

		if(Value == -1) Value = 0; else if(OIDFormat) Value *= 10; else Value <<=4;
		Value += Digit;
		p++;

		if(!DigitCount) 
			DigitCount++;
		else
		{
			*pD = Value;
			*pD++;

			Count--;
			
			Value = -1;
			DigitCount = 0;
		}
	}

	// DRAGONS: oids are encoded ULs, so we need to end swap during the decode!
	if(OIDFormat && (DataSize == 16))
	{
		if((Data[0] == 1) && (Data[1] == 3) && (Data[2] == 52))
		{
			UInt8 Temp[8];

			// Copy out the last 8 bytes of the UL (note that the oid is 1 byte shorter than a UL)
			memcpy(Temp, &Data[7], 8);

			// Copy what would be the 4th through 7th bytes of the UL to where they live in an end-swapped UL
			memcpy(&Data[12], &Data[3], 4);

			// Set the "first 4 bytes" of an end-swapped UL
			Data[8] = 0x06;
			Data[9] = 0x0e;
			Data[10] = 0x2b;
			Data[11] = 0x34;

			// Copy the last 8 bytes of the UL to the first 8 bytes of the UUID (end swapping!)
			memcpy(Data, Temp, 8);
		}
	}

	// If the value was a UL, end-swap it
	if(EndSwap && (DataSize == 16))
	{
		UInt8 Temp[8];
		memcpy(Temp, &Data[8], 8);
		memcpy(&Data[8], Data, 8);
		memcpy(Data, Temp, 8);
	}

	Object->SetData(DataSize, Data);

	// Clean up
	delete[] Data;
}


std::string MDTraits_UUID::GetString(const MDObject *Object, OutputFormatEnum Format) const
{
	mxflib_assert(Object->GetData().Size >= 16);
	if(Object->GetData().Size < 16) 
	{
		if(Object->GetData().Size == 0) return "<Zero-Length UUID>";
		else return "<Invalid-UUID = " + Object->GetData().GetString() + ">";
	}

	return mxflib::UUID::FormatString(Object->GetData().Data, (Format == -1) ? DefaultFormat : Format);
}


/***********************************
**   Label Implementations        **
************************************/

//! Current default output format for Labels
OutputFormatEnum MDTraits_Label::DefaultFormat = -1;

void MDTraits_Label::SetString(MDObject *Object, std::string Val)
{
	// Make a safe copy of the value that will not be cleaned-up by string manipulation
	const int VALBUFF_SIZE = 256;
	char ValueBuff[VALBUFF_SIZE];
	strncpy(ValueBuff, Val.c_str(), VALBUFF_SIZE -1);
	const char *p = ValueBuff;

	// Is this a UUID than needs to be end-swapped
	bool EndSwap = false;

	// Is this an OID format, which will need converting
	bool OIDFormat = false;

	// Check for URN format
	if((tolower(*p) == 'u') && (tolower(p[1]) == 'r') && (tolower(p[2]) == 'n') && (tolower(p[3]) == ':'))
	{
		if(strcasecmp(Val.substr(0,9).c_str(), "urn:uuid:") == 0)
		{
			EndSwap = true;
		}
		else if(strcasecmp(Val.substr(0,8).c_str(), "urn:oid:") == 0)
		{
			OIDFormat = true;
		}

		p += Val.rfind(':') + 1;
	}
	else
	{
		/* If not URN format, check for label name */

		// The label could be "annotated" with details after the first word, so only search for the first word
		std::string LabelName = Val;
		size_t Pos = Val.find(' ');
		if(Pos != std::string::npos) LabelName = Val.substr(0,Pos);
		
		ULPtr LabelUL = MXFLibSymbols->Find(LabelName, true);
		if(LabelUL)
		{
			Object->SetData(16, LabelUL->GetValue());
			return;
		}
	}

	// Allocate a working buffer - and a walking pointer into it
	size_t DataSize = Object->GetData().Size;
	size_t Count = DataSize;
	UInt8 *Data = new UInt8[Count];
	UInt8 *pD = Data;
	int Value = -1;

	// During this loop Value = -1 when no digits of a number are mid-process
	// This stops a double space being regarded as a small zero in between two spaces
	int DigitCount = 0;
	while(Count)
	{
		int Digit;
		
		if((*p == 0) && (Value == -1)) Value = 0;

		if(*p >= '0' && *p <='9') Digit = (*p) - '0';
		else if(*p >= 'a' && *p <= 'f') Digit = (*p) - 'a' + 10;
		else if(*p >= 'A' && *p <= 'F') Digit = (*p) - 'A' + 10;
		else
		{
			// If we meet "{" before any digits, this as a UUID - which will need to be end-swapped
			if((*p == '{') && (Count == DataSize))
			{
				EndSwap = true;
			}

			if(Value == -1)
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
				DigitCount = 0;

				continue;
			}
		}

		if(Value == -1) Value = 0; else if(OIDFormat) Value *= 10; else Value <<=4;
		Value += Digit;
		p++;

		if(!DigitCount) 
			DigitCount++;
		else
		{
			*pD = Value;
			*pD++;

			Count--;
			
			Value = -1;
			DigitCount = 0;
		}
	}

	// DRAGONS: oids can be encoded ULs
	if(OIDFormat && (DataSize == 16))
	{
		if((Data[0] == 1) && (Data[1] == 3) && (Data[2] == 52))
		{
			// Shift the last 12 bytes of the UL forwards 1 byte (note that the oid is 1 byte shorter than a UL)
			memmove(&Data[4], &Data[3], 12);

			// Set the first 4 bytes of a standard UL
			Data[0] = 0x06;
			Data[1] = 0x0e;
			Data[2] = 0x2b;
			Data[3] = 0x34;
		}
	}

	// If the value was a UUID, end-swap it
	if(EndSwap && (DataSize == 16))
	{
		UInt8 Temp[8];
		memcpy(Temp, &Data[8], 8);
		memcpy(&Data[8], Data, 8);
		memcpy(Data, Temp, 8);
	}

	Object->SetData(DataSize, Data);

	// Clean up
	delete[] Data;
}

std::string MDTraits_Label::GetString(const MDObject *Object, OutputFormatEnum Format) const
{
	mxflib_assert(Object->GetData().Size >= 16);
	if(Object->GetData().Size < 16) 
	{
		if(Object->GetData().Size == 0) return "<Zero-Length Label>";
		else return "<Invalid-Label = " + Object->GetData().GetString() + ">";
	}

	return UL::FormatString(Object->GetData().Data, (Format == -1) ? DefaultFormat : Format);
};


/*****************************
**   UMID Implementations	**
*****************************/

void MDTraits_UMID::SetString(MDObject *Object, std::string Val)
{
	// Make a safe copy of the value that will not be cleaned-up by string manipulation
	const int VALBUFF_SIZE = 256;
	char ValueBuff[VALBUFF_SIZE];
	strncpy(ValueBuff, Val.c_str(), VALBUFF_SIZE -1);
	const char *p = ValueBuff;

	size_t Count = Object->GetData().Size;
	int Value = -1;
	UInt8 *Data = new UInt8[Count];
	UInt8 *pD = Data;

	bool EndSwap = false;

	// During this loop Value = -1 when no digits of a number are mid-process
	// This stops a double space being regarded as a small zero in between two spaces
	while(Count)
	{
		int digit;
		
		if((*p == 0) && (Value == -1)) Value = 0;

		if(*p >= '0' && *p <='9') digit = (*p) - '0';
		else if(*p >= 'a' && *p <= 'f') digit = (*p) - 'a' + 10;
		else if(*p >= 'A' && *p <= 'F') digit = (*p) - 'A' + 10;
		else
		{
			// If we meet "[" before the digits for the material number, it is a UL - which will need to be end-swapped
			if((*p == '[') && (Count == (Object->GetData().Size - 16)))
			{
				EndSwap = true;
			}

			if(Value == -1)
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
		}

		if(Value == -1) Value = digit;
		else 
		{
			Value <<=4;
			Value += digit;

			*pD = Value;
			*pD++;

			Count--;

			if(*p) p++;
			
			Value = -1;

			continue;
		}

		p++;
	}

	// If the material number was a UL, end-swap it
	if(EndSwap && (Object->GetData().Size == 32))
	{
		UInt8 Temp[8];
		memcpy(Temp, &Data[24], 8);
		memcpy(&Data[24], &Data[16], 8);
		memcpy(&Data[16], Temp, 8);
	}

	Object->SetData(Object->GetData().Size, Data);

	// Clean up
	delete[] Data;
}

std::string MDTraits_UMID::GetString(const MDObject *Object) const
{
	char Buffer[100];

	mxflib_assert(Object->GetData().Size >= 32);
	const UInt8 *Ident = Object->GetData().Data;

	sprintf (Buffer, "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x]",
					  Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5],
					  Ident[6], Ident[7], Ident[8], Ident[9], Ident[10], Ident[11]
		    );
	
	// Start building the return value
	std::string Ret(Buffer);

	sprintf( Buffer, ",%02x,%02x,%02x,%02x,", Ident[12], Ident[13], Ident[14], Ident[15]);
	Ret += Buffer;

	// Decide how best to represent the material number
	const UInt8* Material = &Ident[16];
	if( !(0x80&Material[8]) )
	{	// Half-swapped UL packed into a UUID datatype
		// Return as compact SMPTE format [bbaa9988.ddcc.ffee.00010203.04050607]
		// Stored with upper/lower 8 bytes exchanged
		// Stored in the following 0-based index order: 88 99 aa bb cc dd ee ff 00 01 02 03 04 05 06 07
		sprintf (Buffer, "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x]",
						   Material[8], Material[9], Material[10], Material[11], Material[12], Material[13], Material[14], Material[15],
						   Material[0], Material[1], Material[2], Material[3], Material[4], Material[5], Material[6], Material[7]
				);
	}
	else
	{	// UUID
		// Stored in the following 0-based index order: 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
		// (i.e. network byte order)
		// Return as compact GUID format {00112233-4455-6677-8899-aabbccddeeff}
		sprintf (Buffer, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
						   Material[0], Material[1], Material[2], Material[3], Material[4], Material[5], Material[6], Material[7],
						   Material[8], Material[9], Material[10], Material[11], Material[12], Material[13], Material[14], Material[15]
				);
	}

	Ret += Buffer;

	return Ret;
};


/********************************************
**   Array of Raw Arrays Implementations   **
********************************************/

std::string MDTraits_RawArrayArray::GetString(const MDObject *Object) const
{
	std::string Ret;

	MDObject::const_iterator it;

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


void MDTraits_RawArrayArray::SetString(MDObject *Object, std::string Val)
{
	MDObject::iterator it;

	size_t OpenBracket;
	size_t CloseBracket = std::string::npos;
	
	it = Object->begin();

	for(;;)
	{
		OpenBracket = Val.find("{", CloseBracket == std::string::npos ? 0 : CloseBracket+1);
		if(OpenBracket == std::string::npos) return;

		CloseBracket = Val.find("}",OpenBracket+1);
		if(CloseBracket == std::string::npos) return;

		// If we are already at the end of the list, add another
		if(it == Object->end()) 
		{
			Object->Resize(static_cast<UInt32>(Object->size() + 1));
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

std::string MDTraits_BasicCompound::GetString(const MDObject *Object) const
{
	std::string Ret;

	const MDType *ValueType = Object->EffectiveType();
	if(!ValueType) return Ret;

	MDTypeList::const_iterator it = ValueType->GetChildList().begin();
	MDTypeList::const_iterator itend = ValueType->GetChildList().end();

	while(it != itend)
	{
		MDObjectPtr Value = Object->Child(*it);

		if(!Value)
		{
			error("Missing sub-item %s in compound\n", (*it)->Name().c_str());
		}
		else
		{
#ifndef FORMAT_COMPOUND
#define FORMAT_COMPOUND 1
#endif

#if FORMAT_COMPOUND==0 // default legacy behaviour
			// comma separated list
			if(Ret.length() != 0) Ret += ", ";
			Ret += Value->Name();
			Ret += "=\"";
			Ret += Value->GetString();
			Ret += "\"";
#elif FORMAT_COMPOUND==1
			// whitespace separated list
			if(Ret.length() != 0) Ret += " ";
			Ret += Value->Name();
			Ret += "=\"";
			Ret += Value->GetString();
			Ret += "\"";
#else
			// sequence of elements
			if(Ret.length() != 0) Ret += "\n";
			Ret += "<";
			Ret += Value->Name();
			Ret += ">";
			Ret += Value->GetString();
			Ret += "</";
			Ret += (*it)->Name();
			Ret += ">";
#endif
		}
		it++;
	}
	return Ret;
}


void MDTraits_BasicCompound::SetString(MDObject *Object, std::string Val)
{
	const MDType *ValueType = Object->EffectiveType();
	if(!ValueType) return;

	size_t OpenQuote;
	size_t CloseQuote = static_cast<size_t>(-1);

	MDTypeList::const_iterator it = ValueType->GetChildList().begin();
	MDTypeList::const_iterator itend = ValueType->GetChildList().end();

	for(;;)
	{
		// DRAGONS: We scan from the start on the first iter, then from the last close quote + 1
		OpenQuote = Val.find("\"", (CloseQuote == static_cast<size_t>(-1) ? 0 : CloseQuote + 1));
		if(OpenQuote == std::string::npos) return;

		// DRAGONS: Should add code here to allow out-of-order items
		// TODO: not updated to match new GetString() above

		CloseQuote = Val.find("\"",OpenQuote+1);
		if(CloseQuote == std::string::npos) return;

		// If we are already at the end of the list, we have too much data!
		if(it == itend) 
		{
			warning("Extra parameters found parsing string in MDTraits_BasicCompound::SetString()\n");
			break;
		}

		MDObjectPtr Value = Object->Child(*it);
		if(!Value)
		{
			error("Missing sub-item %s in compound\n", (*it)->Name().c_str());
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
size_t MDTraits_BasicCompound::ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	size_t Bytes = 0;

	const MDType *ValueType = Object->EffectiveType();
	if(!ValueType) return Bytes;

	MDTypeList::const_iterator it = ValueType->GetChildList().begin();
	MDTypeList::const_iterator itend = ValueType->GetChildList().end();

	// DRAGONS: Note that it is valid to have more bytes than we have read as the caller could be parsing an array of compounds
	while(Size && (it != itend))
	{
		MDObjectPtr Value = Object->Child(*it);
		if(!Value)
		{
			error("Missing sub-item %s in compound\n", (*it)->Name().c_str());
		}
		else
		{
			size_t ThisBytes;

			// Check for SliceOffsetArray which is an implicit array whose size requires knowledge of parents
			if((*it)->Name() == "SliceOffsetArray")
			{
				// Scan up through parents until we get to the IndexTableSegment
				MDObject *pSegment = &(*(Object->GetParent()));
				while(pSegment) 
				{
					if(pSegment->IsA(IndexTableSegment_UL)) break;
					pSegment = &(*(pSegment->GetParent()));
				}

				if(pSegment)
					ThisBytes = Value->ReadValue(Buffer, pSegment->GetInt(SliceCount_UL) * 4);
				else
					error("SliceOffsetArray not in IndexTableSegment in MDTraits_BasicCompound::ReadValue()\n");
			}
			else 
				ThisBytes = Value->ReadValue(Buffer, Size);

			Bytes += ThisBytes;

			Buffer += ThisBytes;
			if(ThisBytes >= Size) Size = 0; else Size -= ThisBytes;
		}
		it++;
	}

	while(it != itend)
	{
		MDObjectPtr Value = Object->Child(*it);
		if(Value->GetValueType()->GetSize() == 0)
		{
			debug("Compound ended with a variable length item %s\n", (*it)->Name().c_str());
			it++;
		}
		else
		{
			warning("Not enough bytes in buffer in MDTraits_BasicCompound::ReadValue()\n");
			break;
		}
	}

	return Bytes;
}


/*********************************
**   Rational Implementations   **
*********************************/

std::string MDTraits_Rational::GetString(const MDObject *Object) const
{
	MDObjectPtr Numerator = Object->Child("Numerator");
	MDObjectPtr Denominator = Object->Child("Denominator");

	UInt32 Num = 0;
	UInt32 Den = 1;
	if(Numerator) Num = Numerator->GetUInt();
	if(Denominator) Den = Denominator->GetUInt();

#ifndef FORMAT_RATIONAL
#define FORMAT_RATIONAL 0
#endif

#if FORMAT_RATIONAL==0 // default legacy behaviour
	return UInt2String(Num) + "/" + UInt2String(Den);
#elif FORMAT_RATIONAL==1
	return UInt2String(Num) + " " + UInt2String(Den);
#elif FORMAT_RATIONAL==2
	return UInt2String(Num) + "," + UInt2String(Den);
#else
	return "<Numerator>" + UInt2String(Num) + "</Numerator>\n" +
	       "<Denominator>" + UInt2String(Den) + "</Denominator>";
#endif
}


void MDTraits_Rational::SetString(MDObject *Object, std::string Val)
{
	MDObjectPtr Numerator = Object->Child("Numerator");
	MDObjectPtr Denominator = Object->Child("Denominator");

	UInt32 Num = atoi(Val.c_str());

	UInt32 Den = 1;
	
#ifndef FORMAT_RATIONAL
#define FORMAT_RATIONAL 0
#endif

#if FORMAT_RATIONAL==0 // default legacy behaviour
	// separator = '/'
	std::string::size_type Sep = Val.find("/");
	if(Sep != std::string::npos) Den = atoi(&(Val.c_str()[Sep+1]));

	if(Numerator) Numerator->SetUInt(Num);
	if(Denominator) Denominator->SetUInt(Den);
#elif FORMAT_RATIONAL==1
	// separator = space
	std::string::size_type Sep = Val.find(" ");
	if(Sep != std::string::npos) Den = atoi(&(Val.c_str()[Sep+1]));

	if(Numerator) Numerator->SetUInt(Num);
	if(Denominator) Denominator->SetUInt(Den);
#elif FORMAT_RATIONAL==2
	// separator = comma
	std::string::size_type Sep = Val.find(",");
	if(Sep != std::string::npos) Den = atoi(&(Val.c_str()[Sep+1]));

	if(Numerator) Numerator->SetUInt(Num);
	if(Denominator) Denominator->SetUInt(Den);
#else
#error need to implement to use uderlying MDTraits_BasicCompound::SetString
#endif
}


/**********************************
**   TimeStamp Implementations   **
**********************************/

//! Write timestamp to ISO-8601 format string
std::string MDTraits_TimeStamp::GetString(const MDObject *Object) const
{
	MDObject *Year;
	MDObject *Month;
	MDObject *Day;
	MDObject *Hours;
	MDObject *Minutes;
	MDObject *Seconds;
	MDObject *msBy4;

	MDObject *Date = Object->Child("Date");
	MDObject *Time = Object->Child("Time");

	// AVMETA: Use Avid style nested structure if applicable
	if(Date && Time)
	{
		Year = Date->Child("Year");
		Month = Date->Child("Month");
		Day = Date->Child("Day");

		Hours = Time->Child("Hours");
		Minutes = Time->Child("Minutes");
		Seconds = Time->Child("Seconds");
		msBy4 = Time->Child("msBy4");
	}
	else
	{
		Year = Object->Child("Year");
		Month = Object->Child("Month");
		Day = Object->Child("Day");
		Hours = Object->Child("Hours");
		Minutes = Object->Child("Minutes");
		Seconds = Object->Child("Seconds");
		msBy4 = Object->Child("msBy4");
	}

	UInt32 Y;
	UInt32 M;
	UInt32 D;
	UInt32 H;
	UInt32 Min;
	UInt32 S;
	UInt32 ms;

	if(Year) Y = Year->GetUInt(); else Y = 0;
	if(Month) M = Month->GetUInt(); else M = 0;
	if(Day) D = Day->GetUInt(); else D = 0;
	if(Hours) H = Hours->GetUInt(); else H = 0;
	if(Minutes) Min = Minutes->GetUInt(); else Min = 0;
	if(Seconds) S = Seconds->GetUInt(); else S = 0;
	if(msBy4) ms = msBy4->GetUInt() * 4; else ms = 0;


#ifndef FORMAT_DATES
#define FORMAT_DATES 0
#endif

#if FORMAT_DATES==0 // default behaviour
	// ISO-8601
	return UInt2String(Y) + "-" + UInt2String(M,2) + "-" + UInt2String(D,2) + " " +
		   UInt2String(H) + ":" + UInt2String(Min,2) + ":" + UInt2String(S,2) + "." + UInt2String(ms,3);
#else
	// AAF legacy format
	static const char * const monthNames[] =
	{
		    "Month0",
		    "Jan", "Feb", "Mar",
			"Apr", "May", "Jun",
			"Jul", "Aug", "Sep",
			"Oct", "Nov", "Dec"
	};

	std::string date = monthNames[ M ];

	return date + " "  + UInt2String(D,2)
	            + ", " + UInt2String(Y)
	            + " "  + UInt2String(H) + ":" + UInt2String(Min,2) + ":" + UInt2String(S,2)
				+ "."  + UInt2String(ms,3)
				+ " GMT";
#endif
}


//! Read timestamp from ISO-8601 format string
void MDTraits_TimeStamp::SetString(MDObject *Object, std::string Val)
{
	MDObject *Year;
	MDObject *Month;
	MDObject *Day;
	MDObject *Hours;
	MDObject *Minutes;
	MDObject *Seconds;
	MDObject *msBy4;

	MDObject *Date = Object->Child("Date");
	MDObject *Time = Object->Child("Time");

	// AVMETA: Use Avid style nested structure if applicable
	if(Date && Time)
	{
		Year = Date->Child("Year");
		Month = Date->Child("Month");
		Day = Date->Child("Day");

		Hours = Time->Child("Hours");
		Minutes = Time->Child("Minutes");
		Seconds = Time->Child("Seconds");
		msBy4 = Time->Child("msBy4");
	}
	else
	{
		Year = Object->Child("Year");
		Month = Object->Child("Month");
		Day = Object->Child("Day");
		Hours = Object->Child("Hours");
		Minutes = Object->Child("Minutes");
		Seconds = Object->Child("Seconds");
		msBy4 = Object->Child("msBy4");
	}

	UInt32 Y = 0;
	UInt32 M = 0;
	UInt32 D = 0;
	UInt32 H = 0;
	UInt32 Min = 0;
	UInt32 S = 0;
	UInt32 ms = 0;

#ifndef FORMAT_DATES
#define FORMAT_DATES 0
#endif

#if FORMAT_DATES==0 // default behaviour
	// ISO-8601
	sscanf(Val.c_str(), "%d-%d-%d", &Y, &M, &D);
	std::string::size_type Pos = Val.find("T");
	if(Pos == std::string::npos) Pos = Val.find(" ");
	if(Pos != std::string::npos) sscanf(&(Val.c_str()[Pos]), "%d:%d:%d.%d", &H, &Min, &S, &ms);
#else
#error non ISO-8601 dates not implemented
#endif

	if(Year) Year->SetUInt(Y);
	if(Month) Month->SetUInt(M);
	if(Day) Day->SetUInt(D);
	if(Hours) Hours->SetUInt(H);
	if(Minutes) Minutes->SetUInt(Min);
	if(Seconds) Seconds->SetUInt(S);
	if(msBy4) msBy4->SetUInt(ms / 4);
}


/***********************************
**   Basic Enum Implementations   **
***********************************/

//! Set from an Int32
void mxflib::MDTraits_BasicEnum::SetInt(MDObject *Object, Int32 Val)
{
	MDOType *OType = Object->GetType();
	if(OType)
	{
		const MDTypePtr &BaseType = OType->GetValueType();
		if(BaseType)
		{
			const MDTypeParent &Base = BaseType->GetBase();
			if(Base)
			{
				MDTraitsPtr BaseTraits = Base->GetTraits();
				if(BaseTraits)
				{
					BaseTraits->SetInt(Object, Val);
					return;
				}
			}
		}
	}
	error("Unable to SetInt() on base of enumerated type %s\n", Object->FullName().c_str());
}

//! Set from an Int64
void mxflib::MDTraits_BasicEnum::SetInt64(MDObject *Object, Int64 Val) 
{
	MDOType *OType = Object->GetType();
	if(OType)
	{
		const MDTypePtr &BaseType = OType->GetValueType();
		if(BaseType)
		{
			const MDTypeParent &Base = BaseType->GetBase();
			if(Base)
			{
				MDTraitsPtr BaseTraits = Base->GetTraits();
				if(BaseTraits)
				{
					BaseTraits->SetInt64(Object, Val);
					return;
				}
			}
		}
	}
	error("Unable to SetInt64() on base of enumerated type %s\n", Object->FullName().c_str());
}

//! Set from a UInt32
void mxflib::MDTraits_BasicEnum::SetUInt(MDObject *Object, UInt32 Val)
{
	MDOType *OType = Object->GetType();
	if(OType)
	{
		const MDTypePtr &BaseType = OType->GetValueType();
		if(BaseType)
		{
			const MDTypeParent &Base = BaseType->GetBase();
			if(Base)
			{
				MDTraitsPtr BaseTraits = Base->GetTraits();
				if(BaseTraits)
				{
					BaseTraits->SetUInt(Object, Val);
					return;
				}
			}
		}
	}
	error("Unable to SetUInt() on base of enumerated type %s\n", Object->FullName().c_str());
}

//! Set from a UInt64
void mxflib::MDTraits_BasicEnum::SetUInt64(MDObject *Object, UInt64 Val)
{
	MDOType *OType = Object->GetType();
	if(OType)
	{
		const MDTypePtr &BaseType = OType->GetValueType();
		if(BaseType)
		{
			const MDTypeParent &Base = BaseType->GetBase();
			if(Base)
			{
				MDTraitsPtr BaseTraits = Base->GetTraits();
				if(BaseTraits)
				{
					BaseTraits->SetUInt64(Object, Val);
					return;
				}
			}
		}
	}
	error("Unable to SetUInt64() on base of enumerated type %s\n", Object->FullName().c_str());
}

//! Get Int32
Int32 mxflib::MDTraits_BasicEnum::GetInt(const MDObject *Object) const
{
	const MDOType *OType = Object->GetType();
	if(OType)
	{
		const MDTypePtr &BaseType = OType->GetValueType();
		if(BaseType)
		{
			const MDTypeParent &Base = BaseType->GetBase();
			if(Base)
			{
				MDTraitsPtr BaseTraits = Base->GetTraits();
				if(BaseTraits) return BaseTraits->GetInt(Object);
			}
		}
	}
	error("Unable to GetInt() on base of enumerated type %s\n", Object->FullName().c_str());
	return 0;
}

//! Get Int64
Int64 mxflib::MDTraits_BasicEnum::GetInt64(const MDObject *Object) const
{
	const MDOType *OType = Object->GetType();
	if(OType)
	{
		const MDTypePtr &BaseType = OType->GetValueType();
		if(BaseType)
		{
			const MDTypeParent &Base = BaseType->GetBase();
			if(Base)
			{
				MDTraitsPtr BaseTraits = Base->GetTraits();
				if(BaseTraits) return BaseTraits->GetInt64(Object);
			}
		}
	}
	error("Unable to GetInt64() on base of enumerated type %s\n", Object->FullName().c_str());
	return 0;
}

//! Get UInt32
UInt32 mxflib::MDTraits_BasicEnum::GetUInt(const MDObject *Object) const
{
	const MDOType *OType = Object->GetType();
	if(OType)
	{
		const MDTypePtr &BaseType = OType->GetValueType();
		if(BaseType)
		{
			const MDTypeParent &Base = BaseType->GetBase();
			if(Base)
			{
				MDTraitsPtr BaseTraits = Base->GetTraits();
				if(BaseTraits) return BaseTraits->GetUInt(Object);
			}
		}
	}
	error("Unable to GetUInt() on base of enumerated type %s\n", Object->FullName().c_str());
	return 0;
}

//! Get UInt64
UInt64 mxflib::MDTraits_BasicEnum::GetUInt64(const MDObject *Object) const
{
	const MDOType *OType = Object->GetType();
	if(OType)
	{
		const MDTypePtr &BaseType = OType->GetValueType();
		if(BaseType)
		{
			const MDTypeParent &Base = BaseType->GetBase();
			if(Base)
			{
				MDTraitsPtr BaseTraits = Base->GetTraits();
				if(BaseTraits) return BaseTraits->GetUInt64(Object);
			}
		}
	}
	error("Unable to GetUInt64() on base of enumerated type %s\n", Object->FullName().c_str());
	return 0;
}

std::string MDTraits_BasicEnum::GetString(const MDObject *Object) const
{
	MDTypePtr EnumBase = Object->GetValueType();
	while(EnumBase->GetClass() != ENUM) EnumBase = EnumBase->Base;
	const MDType::NamedValueList &EnumValues = EnumBase->GetEnumValues();

	MDType::NamedValueList::const_iterator it = EnumValues.begin();
	while(it != EnumValues.end())
	{
		if(*((*it).second) == *Object)
		{
			return (*it).first;
		}

		it++;
	}

	return "[Unknown Value " + Object->PutData()->GetString() + "]";
}


void MDTraits_BasicEnum::SetString(MDObject *Object, std::string Val)
{
	MDTypePtr EnumBase = Object->GetValueType();
	while(EnumBase->GetClass() != ENUM) EnumBase = EnumBase->Base;
	const MDType::NamedValueList &EnumValues = EnumBase->GetEnumValues();

	MDType::NamedValueList::const_iterator it = EnumValues.begin();
	while(it != EnumValues.end())
	{
		if((*it).first == Val)
		{
			*Object = *((*it).second);
			return;
		}

		it++;
	}

	// Let's see if we are setting the actual value rather than its name
	const MDType *ValueType = EnumBase->EffectiveType();
	MDObjectPtr NewValue = new MDObject(ValueType);
	if(NewValue)
	{
		// Set a value of the same type to validate this value
		NewValue->SetString(Val);

		it = EnumBase->GetEnumValues().begin();
		while(it != EnumBase->GetEnumValues().end())
		{
			if(*((*it).second) == *NewValue)
			{
				*Object = *((*it).second);
				return;
			}

			it++;
		}
	}

	error("Attempted to set unknown value %s for enumerated value of type %s\n", Val.c_str(), Object->GetType()->Name().c_str());
}

/* ParentPtr versions */
void MDTraits::SetInt(MDObjectPtr Object, Int32 Val) { SetInt(Object.GetPtr(), Val); }
void MDTraits::SetInt64(MDObjectPtr Object, Int64 Val) { SetInt64(Object.GetPtr(), Val); }
void MDTraits::SetUInt(MDObjectPtr Object, UInt32 Val) { SetUInt(Object.GetPtr(), Val); }
void MDTraits::SetUInt64(MDObjectPtr Object, UInt64 Val) { SetUInt64(Object.GetPtr(), Val); }
void MDTraits::SetString(MDObjectPtr Object, std::string Val) { SetString(Object.GetPtr(), Val); }
void MDTraits::SetUint(MDObjectPtr Object, UInt32 Val) { SetUInt(Object.GetPtr(), Val); }
void MDTraits::SetUint64(MDObjectPtr Object, UInt64 Val) { SetUInt64(Object.GetPtr(), Val); }
Int32 MDTraits::GetInt(const MDObjectPtr Object) const { return GetInt(Object.GetPtr()); }
Int64  MDTraits::GetInt64(const MDObjectPtr Object) const { return GetInt64(Object.GetPtr()); }
UInt32  MDTraits::GetUInt(const MDObjectPtr Object) const { return GetUInt(Object.GetPtr()); }
UInt64  MDTraits::GetUInt64(const MDObjectPtr Object) const { return GetUInt64(Object.GetPtr()); }
UInt32  MDTraits::GetUint(const MDObjectPtr Object) const { return GetUInt(Object.GetPtr()); }
UInt64  MDTraits::GetUint64(const MDObjectPtr Object) const { return GetUInt64(Object.GetPtr()); }
std::string  MDTraits::GetString(const MDObjectPtr Object) const { return GetString(Object.GetPtr()); }
size_t MDTraits::ReadValue(MDObjectPtr Object, const UInt8 *Buffer, size_t Size, int Count) { return ReadValue(Object.GetPtr(), Buffer, Size, Count); }

