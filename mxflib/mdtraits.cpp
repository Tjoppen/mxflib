/*! \file	mdtraits.cpp
 *	\brief	Implementation of traits for MDType definitions
 *
 *	\version $Id: mdtraits.cpp,v 1.25 2007/07/09 17:12:43 matt-beard Exp $
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


//! List of all traits that exist
mxflib::MDTraitsMap mxflib::MDTraits::AllTraits;

//! The current options for converting labels to strings
mxflib::LabelFormat mxflib::LabelFormatOption = LabelFormatText;


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

void mxflib::MDTraits::SetInt(MDValuePtr Object, Int32 Val) { error("Called SetInt() on %s which has traits of %s and does not support SetInt()\n", Object->Name().c_str(), Name().c_str()); }
void mxflib::MDTraits::SetInt64(MDValuePtr Object, Int64 Val) { error("Called SetInt64() on %s which has traits of %s and does not support SetInt64()\n", Object->Name().c_str(), Name().c_str()); }
void mxflib::MDTraits::SetUInt(MDValuePtr Object, UInt32 Val) { error("Called SetUInt() on %s which has traits of %s and does not support SetUInt()\n", Object->Name().c_str(), Name().c_str()); }
void mxflib::MDTraits::SetUInt64(MDValuePtr Object, UInt64 Val) { error("Called SetUInt64() on %s which has traits of %s and does not support SetUInt64()\n", Object->Name().c_str(), Name().c_str()); }
void mxflib::MDTraits::SetString(MDValuePtr Object, std::string Val) { error("Called SetString() on %s which has traits of %s and does not support SetString()\n", Object->Name().c_str(), Name().c_str()); }
Int32 mxflib::MDTraits::GetInt(MDValuePtr Object) { error("Called GetInt() on %s which has traits of %s and does not support GetInt()\n", Object->Name().c_str(), Name().c_str()); return 0;}
Int64 mxflib::MDTraits::GetInt64(MDValuePtr Object) { error("Called GetInt64() on %s which has traits of %s and does not support GetInt64()\n", Object->Name().c_str(), Name().c_str()); return 0;}
UInt32 mxflib::MDTraits::GetUInt(MDValuePtr Object) { error("Called GetUInt() on %s which has traits of %s and does not support GetUInt()\n", Object->Name().c_str(), Name().c_str()); return 0;}
UInt64 mxflib::MDTraits::GetUInt64(MDValuePtr Object) { error("Called GetUInt64() on %s which has traits of %s and does not support GetUInt64()\n", Object->Name().c_str(), Name().c_str()); return 0;}
std::string mxflib::MDTraits::GetString(MDValuePtr Object) { error("Called GetString() on %s which has traits of %s and does not support GetString()\n", Object->Name().c_str(), Name().c_str()); return std::string("Base"); }

size_t MDTraits::ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
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
void mxflib::MDTraits_BasicInt::SetInt64(MDValuePtr Object, Int64 Val) { SetInt(Object, (Int32)Val); }

//! Set from a UInt32
void mxflib::MDTraits_BasicInt::SetUInt(MDValuePtr Object, UInt32 Val) { SetInt(Object, (Int32)Val); }

//! Set from a UInt64
void mxflib::MDTraits_BasicInt::SetUInt64(MDValuePtr Object, UInt64 Val) { SetInt(Object, (Int32)Val); }

//! Set from a string
void mxflib::MDTraits_BasicInt::SetString(MDValuePtr Object, std::string Val) { SetInt(Object, (Int32)atoi(Val.c_str())); }

//! Get Int64
Int64 mxflib::MDTraits_BasicInt::GetInt64(MDValuePtr Object) { return (Int64) GetInt(Object); }

//! Get UInt64
UInt64 mxflib::MDTraits_BasicInt::GetUInt64(MDValuePtr Object) { return (UInt64) GetUInt(Object); }

//!	Get string from an integer
std::string mxflib::MDTraits_BasicInt::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%d", GetInt(Object));
	return std::string(Buffer);
}

size_t MDTraits_BasicInt::ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	// Limit the size attempted to be read to the size of the type
	UInt32 TypeSize = Object->GetType()->Size;
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
size_t mxflib::ReadValueUInt(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	// Limit the size attempted to be read to the size of the type
	UInt32 TypeSize = Object->GetType()->Size;
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
	Object->SetData(1, (UInt8*)&i);
}

//! Get Int32 from an Int8
Int32 mxflib::MDTraits_Int8::GetInt(MDValuePtr Object) 
{
	size_t Size = Object->GetData().Size;

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

//! Get UInt32 from an Int8
/*! \note
 *	This function will return 128 through 255 for bit values 10000000 through 11111111
 *	even though an Int8 cannot store them. This is as opposed to the option of returning
 *  0xffffff80 through 0xffffffff for those values.
 */
UInt32 mxflib::MDTraits_Int8::GetUInt(MDValuePtr Object)
{ 
	size_t Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 1)
	{
		error("Tried to read a 1-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Return the value promoted to 32-bits
	return (UInt32) *((const UInt8*)(Object->GetData().Data));
}


/******************************
**   UInt8 Implementations   **
******************************/

//!	Get string from a UInt8
std::string MDTraits_UInt8::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", GetUInt(Object));
	return std::string(Buffer);
}

//! Read value from memory buffer
size_t MDTraits_UInt8::ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	return ReadValueUInt(Object, Buffer, Size, Count);
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
	UInt16 i = Swap((UInt16)Val);

	Object->SetData(2, (UInt8*)&i);
}

//! Get Int32 from an Int16
Int32 mxflib::MDTraits_Int16::GetInt(MDValuePtr Object) 
{ 
	size_t Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 2)
	{
		error("Tried to read a 2-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 16-bit value
	Int16 Val = ((Object->GetData().Data)[0] << 8) | (Object->GetData().Data)[1];

	// Return that value cast up to 32-bit
	return static_cast<Int32>(Val);
}

//! Get UInt32 from an Int16
UInt32 mxflib::MDTraits_Int16::GetUInt(MDValuePtr Object) 
{ 
	size_t Size = Object->GetData().Size;

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
**   UInt16 Implementations   **
*******************************/

//!	Get string from a UInt16
std::string MDTraits_UInt16::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", GetUInt(Object));
	return std::string(Buffer);
}

//! Read value from memory buffer
size_t MDTraits_UInt16::ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	return ReadValueUInt(Object, Buffer, Size, Count);
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
	UInt32 i = Swap((UInt32)Val);

	Object->SetData(4, (UInt8*)&i);
}

//! Get Int32 from an Int32
Int32 mxflib::MDTraits_Int32::GetInt(MDValuePtr Object) 
{ 
	size_t Size = Object->GetData().Size;

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

//! Get UInt32 from an Int32
UInt32 mxflib::MDTraits_Int32::GetUInt(MDValuePtr Object)
{
	// As the return value is the same size as our working variables
	// the signed to unsigned conversion should be safe like this
	return (UInt32)GetInt(Object);
}


/*******************************
**   UInt32 Implementations   **
*******************************/

//!	Get string from a UInt32
std::string MDTraits_UInt32::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", GetUInt(Object));
	return std::string(Buffer);
}

//! Read value from memory buffer
size_t MDTraits_UInt32::ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	return ReadValueUInt(Object, Buffer, Size, Count);
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
	UInt64 i = Swap((UInt64)Val);

	Object->SetData(8, (UInt8*)&i);
}

//! Set from an Int32
void mxflib::MDTraits_Int64::SetInt(MDValuePtr Object, Int32 Val) { SetInt64(Object, (Int64)Val); }

//! Set from a UInt32
void mxflib::MDTraits_Int64::SetUInt(MDValuePtr Object, UInt32 Val) { SetUInt64(Object, (UInt64)Val); }

//! Set from a UInt64
/*! DRAGONS: Will this always work? This relies on the UInt64 -> Int64 -> UInt64
 *           conversion being valid for all values!
 */
void mxflib::MDTraits_Int64::SetUInt64(MDValuePtr Object, UInt64 Val) { SetInt64(Object, (UInt64)Val); }

//! Set from a string
void mxflib::MDTraits_Int64::SetString(MDValuePtr Object, std::string Val) { SetInt64(Object, ato_Int64(Val.c_str())); }

//!	Get string from an integer
std::string mxflib::MDTraits_Int64::GetString(MDValuePtr Object) 
{ 
	return Int64toString(GetInt64(Object));
}

//! Get Int
Int32 mxflib::MDTraits_Int64::GetInt(MDValuePtr Object) { return (Int32) GetInt64(Object); }

//! Get UInt
UInt32 mxflib::MDTraits_Int64::GetUInt(MDValuePtr Object) { return (UInt32) GetUInt64(Object); }

//! Get Int64
Int64 mxflib::MDTraits_Int64::GetInt64(MDValuePtr Object) 
{ 
	size_t Size = Object->GetData().Size;

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 8)
	{
		error("Tried to read an 8-byte value from an MDValue that has size %d\n", Size);
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
UInt64 mxflib::MDTraits_Int64::GetUInt64(MDValuePtr Object)
{
	// As the return value is the same size as our working variables
	// the signed to unsigned conversion should be safe like this
	return (UInt64)GetInt64(Object);
}


/*******************************
**   UInt64 Implementations   **
*******************************/

//!	Get string from an integer
std::string mxflib::MDTraits_UInt64::GetString(MDValuePtr Object) 
{ 
	return UInt64toString(GetUInt64(Object));
}

//! Read value from memory buffer
size_t MDTraits_UInt64::ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	return ReadValueUInt(Object, Buffer, Size, Count);
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

//!	Get string from a UTF16 - returns UTF8 coded version of a single UTF16 code unit - surrogates are not converted!
/*! Unicode Table 3-5. UTF-8 Bit Distribution
Unicode                     1st Byte 2nd Byte 3rd Byte 4th Byte
00000000 0xxxxxxx           0xxxxxxx
00000yyy yyxxxxxx           110yyyyy 10xxxxxx
zzzzyyyy yyxxxxxx           1110zzzz 10yyyyyy 10xxxxxx
000uuuuu zzzzyyyy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
*/
std::string MDTraits_UTF16::GetString(MDValuePtr Object) 
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
void MDTraits_UTF16::SetString(MDValuePtr Object, std::string Val)
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
std::string MDTraits_UTF16String::GetString(MDValuePtr Object) 
{ 
	std::string Ret;				//!< Return value being built
	char Buffer[5];					//!< Buffer for building each Unicode symbol
	UInt16 Surrogate = 0;			//!< First surrogate if processing a pair

	MDValue::iterator it = Object->begin();
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
void MDTraits_UTF16String::SetString(MDValuePtr Object, std::string Val)
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
	MDValue::iterator it = Object->begin();
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

void MDTraits_BasicArray::SetUInt(MDValuePtr Object, UInt32 Val)
{
	MDValue::iterator it;

	it = Object->begin();
	while(it != Object->end())
	{
		(*it).second->SetUInt(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetUInt64(MDValuePtr Object, UInt64 Val)
{
	MDValue::iterator it;

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
void MDTraits_BasicArray::SetString(MDValuePtr Object, std::string Val)
{
	MDValue::iterator it;

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


UInt32 MDTraits_BasicArray::GetUInt(MDValuePtr Object)
{
	MDValue::iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetUInt();
}

UInt64 MDTraits_BasicArray::GetUInt64(MDValuePtr Object)
{
	MDValue::iterator it;

	it = Object->begin();
	if(it == Object->end())
	{
		return 0;
	}
	return (*it).second->GetUInt64();
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


size_t MDTraits_BasicArray::ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
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

	// Number of bytes read
	size_t Bytes = 0;

	// If this object is a batch we need to read its header
	if(Object->GetType()->GetArrayClass() == ARRAYBATCH)
	{
		if(Size < 8)
		{
			error("Tried to read a batch of type %s but less than 8 bytes available\n", Object->Name().c_str());
			return 0;
		}

		UInt32 ItemCount = GetU32(Buffer);
		UInt32 ItemSize = GetU32(&Buffer[4]);

		Buffer += 8;
		Bytes += 8;
		Size -= 8;

		if(Count > (int)ItemCount)
		{
			error("Tried to read more items from batch of type %s than available - requested = %u, available = %u\n", Object->Name().c_str(), Count, ItemCount);
		}
		else
		{
			// Only update the count if it was unknown (this allows a valid request to read less than available)
			if(UnknownCount) Count = ItemCount;

			// Now the count IS known
			UnknownCount = false;
		}

		if((ItemCount * ItemSize) > Size)
		{
			error("Invalid batch of type %s - count = %u, item size = %u so 0x%08x bytes required but only 0x%08x available\n", Object->Name().c_str(), ItemCount, ItemSize, (ItemCount * ItemSize), Size);
			
			// Make the count safe
			Count = static_cast<int>(Size / ItemSize);
		}
	}

	// Figure out the maximum number of items to read, or zero if open-ended
	UInt32 MaxItems = Object->GetType()->Size;

	// Count of actual items read, and bytes read in doing so
	UInt32 ActualCount = 0;

	// Either the size of each item to read, or the total size (for unknown count)
	size_t ThisSize = Size;

	while(Count)
	{
		size_t ThisBytes;

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

Int32 MDTraits_Raw::GetInt(MDValuePtr Object)
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

UInt32 MDTraits_Raw::GetUInt(MDValuePtr Object)
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

Int64 MDTraits_Raw::GetInt64(MDValuePtr Object)
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

UInt64 MDTraits_Raw::GetUInt64(MDValuePtr Object)
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

std::string MDTraits_Raw::GetString(MDValuePtr Object)
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


void MDTraits_Raw::SetString(MDValuePtr Object, std::string Val)
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

std::string MDTraits_RawArray::GetString(MDValuePtr Object)
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

	MDValue::iterator it;

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


size_t MDTraits_Raw::ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
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


/*****************************
**   UUID Implementations	**
*****************************/

void MDTraits_UUID::SetString(MDValuePtr Object, std::string Val)
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

std::string MDTraits_UUID::GetString(MDValuePtr Object)
{
	char Buffer[100];

	ASSERT(Object->GetData().Size >= 16);
	const UInt8 *Ident = Object->GetData().Data;

	// Check which format should be used
	if( !(0x80&Ident[8]) )
	{	// Half-swapped UL packed into a UUID datatype
		// Return as compact SMPTE format [bbaa9988.ddcc.ffee.00010203.04050607]
		// Stored with upper/lower 8 bytes exchanged
		// Stored in the following 0-based index order: 88 99 aa bb cc dd ee ff 00 01 02 03 04 05 06 07
		sprintf (Buffer, "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x]",
						   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15],
						   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7]
		);
	}
	else
	{	// UUID
		// Stored in the following 0-based index order: 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
		// (i.e. network byte order)
		// Return as compact GUID format {00112233-4455-6677-8899-aabbccddeeff}
		sprintf (Buffer, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
						   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
						   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
				);
	}

	return std::string(Buffer);
}


/***********************************
**   Label Implementations        **
************************************/

void MDTraits_Label::SetString(MDValuePtr Object, std::string Val)
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

std::string MDTraits_Label::GetString(MDValuePtr Object)
{
	std::string Ret;

	ASSERT(Object->GetData().Size >= 16);
	const UInt8 *Ident = Object->GetData().Data;

	// If we are not simply returning the hex, lookup the string
	if(GetLabelFormat() != LabelFormatHex)
	{
		if(Object->GetData().Size == 16)
		{
			LabelPtr Label =  Label::Find(Ident);
			if(Label) 
			{
				Ret = Label->GetDetail();

				// If we are just getting the text - return it
				if(    (GetLabelFormat() == LabelFormatText) 
					|| ((GetLabelFormat() == LabelFormatTextHexMask) && !Label->HasMask())) return Ret;
			}
		}
	}

	// ...else emit underlying identifier

	char Buffer[100];

	// Check which format should be used
	if( !(0x80&Ident[0]) )
	{	
		// This is a UL rather than a half-swapped UUID
		// Return as compact SMPTE format [060e2b34.rrss.mmvv.ccs1s2s3.s4s5s6s7]
		// Stored in the following 0-based index order: 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
		// (i.e. network byte order)
		sprintf (Buffer, "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x]",
						   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
						   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
				);
	}
	else
	{	
		// Half-swapped UUID
		// Return as compact GUID format {8899aabb-ccdd-eeff-0011-223344556677}
		sprintf (Buffer, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
						   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15],
						   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7]
				);
	}

	if(Ret.length() == 0)
	{
		Ret = Buffer;
	}
	else
	{
		Ret += " ";
		Ret += Buffer;
	}

	return Ret;
};


/*****************************
**   UMID Implementations	**
*****************************/

void MDTraits_UMID::SetString(MDValuePtr Object, std::string Val)
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

		if(Value == -1) Value = 0; else Value <<=4;
		Value += digit;
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

std::string MDTraits_UMID::GetString(MDValuePtr Object)
{
	char Buffer[100];

	ASSERT(Object->GetData().Size >= 32);
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
	size_t OpenQuote;
	size_t CloseQuote = static_cast<size_t>(-1);

	StringList::iterator it = Object->EffectiveType()->ChildOrder.begin();
	StringList::iterator itend = Object->EffectiveType()->ChildOrder.end();

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
size_t MDTraits_BasicCompound::ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count /*=0*/)
{
	size_t Bytes = 0;

	StringList::iterator it = Object->EffectiveType()->ChildOrder.begin();
	StringList::iterator itend = Object->EffectiveType()->ChildOrder.end();

	// DRAGONS: Note that it is valid to have more bytes than we have read as the caller could be parsing an array of compounds
	while(Size && (it != itend))
	{
		MDValuePtr Value = Object[*it];
		if(!Value)
		{
			error("Missing sub-item %s in compound\n", (*it).c_str());
		}
		else
		{
			size_t ThisBytes = Value->ReadValue(Buffer, Size);
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

	UInt32 Num = 0;
	UInt32 Den = 1;
	if(Numerator) Num = Numerator->GetUInt();
	if(Denominator) Den = Denominator->GetUInt();

	return UInt2String(Num) + "/" + UInt2String(Den);
}


void MDTraits_Rational::SetString(MDValuePtr Object, std::string Val)
{
	MDValuePtr Numerator = Object["Numerator"];
	MDValuePtr Denominator = Object["Denominator"];

	UInt32 Num = atoi(Val.c_str());

	UInt32 Den = 1;
	std::string::size_type Slash = Val.find("/");
	if(Slash != std::string::npos) Den = atoi(&(Val.c_str()[Slash+1]));

	if(Numerator) Numerator->SetUInt(Num);
	if(Denominator) Denominator->SetUInt(Den);
}


/**********************************
**   TimeStamp Implementations   **
**********************************/

//! Write timestamp to ISO-8601 format string
std::string MDTraits_TimeStamp::GetString(MDValuePtr Object)
{
	MDValuePtr Year = Object["Year"];
	MDValuePtr Month = Object["Month"];
	MDValuePtr Day = Object["Day"];
	MDValuePtr Hours = Object["Hours"];
	MDValuePtr Minutes = Object["Minutes"];
	MDValuePtr Seconds = Object["Seconds"];
	MDValuePtr msBy4 = Object["msBy4"];

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


#if defined(AAF_DATES)
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
#else
	return UInt2String(Y) + "-" + UInt2String(M,2) + "-" + UInt2String(D,2) + " " +
		   UInt2String(H) + ":" + UInt2String(Min,2) + ":" + UInt2String(S,2) + "." + UInt2String(ms,3);
#endif
}


//! Read timestamp from ISO-8601 format string
void MDTraits_TimeStamp::SetString(MDValuePtr Object, std::string Val)
{
	MDValuePtr Year = Object["Year"];
	MDValuePtr Month = Object["Month"];
	MDValuePtr Day = Object["Day"];
	MDValuePtr Hours = Object["Hours"];
	MDValuePtr Minutes = Object["Minutes"];
	MDValuePtr Seconds = Object["Seconds"];
	MDValuePtr msBy4 = Object["msBy4"];

	UInt32 Y = 0;
	UInt32 M = 0;
	UInt32 D = 0;
	UInt32 H = 0;
	UInt32 Min = 0;
	UInt32 S = 0;
	UInt32 ms = 0;

	sscanf(Val.c_str(), "%d-%d-%d", &Y, &M, &D);
	std::string::size_type Pos = Val.find("T");
	if(Pos == std::string::npos) Pos = Val.find(" ");
	if(Pos != std::string::npos) sscanf(&(Val.c_str()[Pos]), "%d:%d:%d.%d", &H, &Min, &S, &ms);

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

std::string MDTraits_BasicEnum::GetString(MDValuePtr Object)
{
	MDType::NamedValueList::iterator it = Object->GetType()->GetEnumValues().begin();
	while(it != Object->GetType()->GetEnumValues().end())
	{
		if(*((*it).second) == *Object)
		{
			return (*it).first;
		}

		it++;
	}

	return "[Unknown Value " + Object->PutData()->GetString() + "]";
}


void MDTraits_BasicEnum::SetString(MDValuePtr Object, std::string Val)
{
	MDType::NamedValueList::const_iterator it = Object->GetType()->GetEnumValues().begin();
	while(it != Object->GetType()->GetEnumValues().end())
	{
		if((*it).first == Val)
		{
			*Object = *((*it).second);
			return;
		}

		it++;
	}

	// Let's see if we are setting the actual value
	MDValuePtr NewValue = new MDValue(Object->EffectiveType());
	if(NewValue)
	{
		// Set a value of the same type to validate this value
		NewValue->SetString(Val);

		it = Object->GetType()->GetEnumValues().begin();
		while(it != Object->GetType()->GetEnumValues().end())
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


