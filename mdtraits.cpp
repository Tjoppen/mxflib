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

#include <stdexcept>

#include "mxflib.h"

// Use mxflib by default in library source
using namespace mxflib;

// Default trait implementations
////////////////////////////////

void mxflib::MDTraits::SetInt(MDValuePtr Object, Int32 Val) { error("NO BODY!\n"); };
void mxflib::MDTraits::SetInt64(MDValuePtr Object, Int64 Val) { error("NO BODY!\n"); };
void mxflib::MDTraits::SetUint(MDValuePtr Object, Uint32 Val) { error("NO BODY!\n"); };
void mxflib::MDTraits::SetUint64(MDValuePtr Object, Uint64 Val) { error("NO BODY!\n"); };
void mxflib::MDTraits::SetString(MDValuePtr Object, std::string Val) { error("NO BODY!\n"); };
Int32 mxflib::MDTraits::GetInt(MDValuePtr Object) { error("NO BODY!\n"); return 0;};
Int64 mxflib::MDTraits::GetInt64(MDValuePtr Object) { error("NO BODY!\n"); return 0; };
Uint32 mxflib::MDTraits::GetUint(MDValuePtr Object) { error("NO BODY!\n"); return 0; };
Uint64 mxflib::MDTraits::GetUint64(MDValuePtr Object) { error("NO BODY!\n"); return 0; };
std::string mxflib::MDTraits::GetString(MDValuePtr Object) { return std::string("Base"); };


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
void mxflib::MDTraits_BasicInt::SetInt64(MDValuePtr Object, Int64 Val) { SetInt(Object, (Int32)Val); };

//! Set from a Uint32
void mxflib::MDTraits_BasicInt::SetUint(MDValuePtr Object, Uint32 Val) { SetInt(Object, (Int32)Val); };

//! Set from a Uint64
void mxflib::MDTraits_BasicInt::SetUint64(MDValuePtr Object, Uint64 Val) { SetInt(Object, (Int32)Val); };

//! Set from a string
void mxflib::MDTraits_BasicInt::SetString(MDValuePtr Object, std::string Val) { SetInt(Object, (Int32)atoi(Val.c_str())); };

//! Get Int64
Int64 mxflib::MDTraits_BasicInt::GetInt64(MDValuePtr Object) { return (Int64) GetInt(Object); };

//! Get Uint64
Uint64 mxflib::MDTraits_BasicInt::GetUint64(MDValuePtr Object) { return (Uint64) GetUint(Object); };

//!	Get string from an integer
std::string mxflib::MDTraits_BasicInt::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%d", GetInt(Object));
	return std::string(Buffer);
};


/*****************************
**   Int8 Implementations   **
*****************************/

//! Set Int8 from an Int32
void mxflib::MDTraits_Int8::SetInt(MDValuePtr Object, Int32 Val) 
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

//! Get Int32 from an Int8
Int32 mxflib::MDTraits_Int8::GetInt(MDValuePtr Object) 
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

//! Get Uint32 from an Int8
/*! \note
 *	This function will return 128 through 255 for bit values 10000000 through 11111111
 *	even though an Int8 cannot store them. This is as opposed to the option of returning
 *  0xffffff80 through 0xffffffff for those values.
 */
Uint32 mxflib::MDTraits_Int8::GetUint(MDValuePtr Object)
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



/******************************
**   Int16 Implementations   **
******************************/

//! Set Int16 from an Int32
void mxflib::MDTraits_Int16::SetInt(MDValuePtr Object, Int32 Val) 
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
	// Note that the swap is done in an unsigned int
	// to prevent any sign problems!
	Uint16 i = Swap((Uint16)Val);

	Object->SetValue(2, (Uint8*)&i);
};

//! Get Int32 from an Int16
Int32 mxflib::MDTraits_Int16::GetInt(MDValuePtr Object) 
{ 
	int Size = Object->GetSize();

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 2)
	{
		error("Tried to read a 2-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 16-bit value in a Uint to prevent possible sign problems
	Uint16 Val = ((Object->GetData())[1] << 8) | (Object->GetData())[0];
	
	// Endian swap (if required) and convert to signed, then promote to 32-bit
	return (Int32) ((Int16)Swap(Val));
};

//! Get Uint32 from an Int16
Uint32 mxflib::MDTraits_Int16::GetUint(MDValuePtr Object) 
{ 
	int Size = Object->GetSize();

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 2)
	{
		error("Tried to read a 2-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 16-bit value
	Uint16 Val = ((Object->GetData())[1] << 8) | (Object->GetData())[0];
	
	// Endian swap (if required) then promote to 32-bit
	return (Int32) Swap(Val);
};


/******************************
**   Int32 Implementations   **
******************************/

//! Set Int32 from an Int32
void mxflib::MDTraits_Int32::SetInt(MDValuePtr Object, Int32 Val) 
{ 
	if(Object->GetSize() != 4)
	{
		Object->MakeSize(4);
		
		if(Object->GetSize() != 4)
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

	Object->SetValue(4, (Uint8*)&i);
};

//! Get Int32 from an Int32
Int32 mxflib::MDTraits_Int32::GetInt(MDValuePtr Object) 
{ 
	int Size = Object->GetSize();

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 4)
	{
		error("Tried to read a 4-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 32-bit value in a Uint to prevent possible sign problems
	Uint32 Val = ((Object->GetData())[3] << 24) | ((Object->GetData())[2] << 16)
		       | ((Object->GetData())[1] << 8) | (Object->GetData())[0];
	
	// Endian swap (if required) and convert to signed
	return (Int32) ((Int32)Swap(Val));
};

//! Get Uint32 from an Int32
Uint32 mxflib::MDTraits_Int32::GetUint(MDValuePtr Object)
{
	// As the return value is the same size as our working variables
	// the signed to unsigned conversion should be safe like this
	return (Uint32)GetInt(Object);
};


/*******************************
**   Uint32 Implementations   **
*******************************/

//!	Get string from a Uint32
std::string mxflib::MDTraits_Uint32::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", GetInt(Object));
	return std::string(Buffer);
};


/******************************
**   Int64 Implementations   **
******************************/

//! Set Int64 from an Int64
void mxflib::MDTraits_Int64::SetInt64(MDValuePtr Object, Int64 Val) 
{ 
	if(Object->GetSize() != 8)
	{
		Object->MakeSize(8);
		
		if(Object->GetSize() != 8)
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

	Object->SetValue(8, (Uint8*)&i);
};

//! Set from an Int32
void mxflib::MDTraits_Int64::SetInt(MDValuePtr Object, Int32 Val) { SetInt64(Object, (Int64)Val); };

//! Set from a Uint32
void mxflib::MDTraits_Int64::SetUint(MDValuePtr Object, Uint32 Val) { SetUint64(Object, (Uint64)Val); };

//! Set from a Uint64
/*! DRAGONS: Will this always work? This relies on the Uint64 -> Int64 -> Uint64
 *           conversion being valid for all values!
 */
void mxflib::MDTraits_Int64::SetUint64(MDValuePtr Object, Uint64 Val) { SetInt64(Object, (Uint64)Val); };

//! Set from a string
void mxflib::MDTraits_Int64::SetString(MDValuePtr Object, std::string Val) { SetInt64(Object, ato_Int64(Val.c_str())); };

//!	Get string from an integer
std::string mxflib::MDTraits_Int64::GetString(MDValuePtr Object) 
{ 
	return Int64toString(GetInt64(Object));
};

//! Get Int
Int32 mxflib::MDTraits_Int64::GetInt(MDValuePtr Object) { return (Int32) GetInt64(Object); };

//! Get Uint
Uint32 mxflib::MDTraits_Int64::GetUint(MDValuePtr Object) { return (Uint32) GetUint64(Object); };

//! Get Int64
Int64 mxflib::MDTraits_Int64::GetInt64(MDValuePtr Object) 
{ 
	int Size = Object->GetSize();

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 8)
	{
		error("Tried to read an 8-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 64-bit value using Uint32s to prevent possible sign problems
	Uint32 HiVal = ((Object->GetData())[7] << 24) | ((Object->GetData())[6] << 16)
		         | ((Object->GetData())[5] << 8) | (Object->GetData())[4];
	Uint32 LoVal = ((Object->GetData())[3] << 24) | ((Object->GetData())[2] << 16)
		         | ((Object->GetData())[1] << 8) | (Object->GetData())[0];

	Uint64 Val = (Uint64(HiVal) << 32) | LoVal;

	// Endian swap (if required) and convert to signed
	return (Int64) ((Int64)Swap(Val));
};

//! Get Uint64
Uint64 mxflib::MDTraits_Int64::GetUint64(MDValuePtr Object)
{
	// As the return value is the same size as our working variables
	// the signed to unsigned conversion should be safe like this
	return (Uint64)GetInt64(Object);
};


/*******************************
**   Uint64 Implementations   **
*******************************/

//!	Get string from an integer
std::string mxflib::MDTraits_Uint64::GetString(MDValuePtr Object) 
{ 
	return Uint64toString(GetUint64(Object));
};


/***************************************
**   ISO 7-bit char Implementations   **
***************************************/

//!	Get string from an ISO7
std::string MDTraits_ISO7::GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%c", GetInt(Object));
	return std::string(Buffer);
};

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
};

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
	MDValueList::iterator it;

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		(*it)->SetInt(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetInt64(MDValuePtr Object, Int64 Val)
{
	MDValueList::iterator it;

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		(*it)->SetInt64(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetUint(MDValuePtr Object, Uint32 Val)
{
	MDValueList::iterator it;

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		(*it)->SetUint(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetUint64(MDValuePtr Object, Uint64 Val)
{
	MDValueList::iterator it;

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		(*it)->SetUint64(Val);
		it++;
	}
}

void MDTraits_BasicArray::SetString(MDValuePtr Object, std::string Val)
{
	MDValueList::iterator it;

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		(*it)->SetString(Val);
		it++;
	}
}

Int32 MDTraits_BasicArray::GetInt(MDValuePtr Object)
{
	MDValueList::iterator it;

	it = Object->Children.begin();
	if(it == Object->Children.end())
	{
		return 0;
	}
	return (*it)->GetInt();
}

Int64 MDTraits_BasicArray::GetInt64(MDValuePtr Object)
{
	MDValueList::iterator it;

	it = Object->Children.begin();
	if(it == Object->Children.end())
	{
		return 0;
	}
	return (*it)->GetInt64();
}


Uint32 MDTraits_BasicArray::GetUint(MDValuePtr Object)
{
	MDValueList::iterator it;

	it = Object->Children.begin();
	if(it == Object->Children.end())
	{
		return 0;
	}
	return (*it)->GetUint();
}

Uint64 MDTraits_BasicArray::GetUint64(MDValuePtr Object)
{
	MDValueList::iterator it;

	it = Object->Children.begin();
	if(it == Object->Children.end())
	{
		return 0;
	}
	return (*it)->GetUint64();
}

std::string MDTraits_BasicArray::GetString(MDValuePtr Object)
{
	std::string Ret;

	MDValueList::iterator it;

	Ret = "";

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		if(Ret.length() > 0) Ret += ", ";
		Ret += (*it)->GetString();
		it++;
	}
	return Ret;
}


/*********************************************
**   Default String Array Implementations   **
**********************************************/

std::string MDTraits_BasicStringArray::GetString(MDValuePtr Object)
{
	std::string Ret;

	MDValueList::iterator it;

	Ret = "";

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		std::string Temp = (*it)->GetString();
		
		// Stop if a terminating zero was found
		if(Temp.length() == 0) break;

		Ret += Temp;
		it++;
	}
	return Ret;
}


void MDTraits_BasicStringArray::SetString(MDValuePtr Object, std::string Val)
{
	MDValueList::iterator it;

	int Size = Val.length();
	int Index = 0;
	
	Object->ResizeChildren(Size);

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		std::string Temp;
		char c;

		try
		{
			c = Val.at(Index++);
		}
		catch(std::out_of_range)
		{
			// Ignore string slice errors!! - Should never happen
		}

		// Stop at a terminating NULL
		if(c==0) 
		{
			Object->ResizeChildren(Size);
			break;
		}

		Temp = c;
		(*it)->SetString(Temp);

		it++;
	}
}


/****************************
**   Raw Implementations   **
****************************/

std::string MDTraits_Raw::GetString(MDValuePtr Object)
{
	std::string Ret;
	int Count = Object->GetSize();
	const Uint8 *Data = Object->GetData();

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
	int Count = Object->GetSize();
	int Value = -1;
	Uint8 *Data = new Uint8[Object->GetSize()];
	Uint8 *pD = Data;
	const char *p = Val.c_str();

	// During this loop Value = -1 when no digits of a number are mid-process
	// This stops a double space being regarded as a small zero in between two spaces
	while(Count)
	{
		int digit;
		
		if(*p == 0) Value = 0;

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

	Object->SetValue(Object->GetSize(), Data);
}


/**********************************
**   Raw Array Implementations   **
**********************************/

std::string MDTraits_RawArray::GetString(MDValuePtr Object)
{
	std::string Ret;

	MDValueList::iterator it;

	Ret = "";

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		char Buffer[32];
		if((*it)->GetSize() == 1) sprintf(Buffer, "%02x", (*it)->GetUint());
		else if((*it)->GetSize() == 2) sprintf(Buffer, "%04x", (*it)->GetUint());
		else if((*it)->GetSize() == 4) sprintf(Buffer, "%08x", (*it)->GetUint());
		else ASSERT(0);		// DRAGONS: We need a 64-bit hex print function!

		if(Ret.length() != 0) Ret += " ";
		Ret += Buffer;
		it++;
	}
	return Ret;
}


void MDTraits_RawArray::SetString(MDValuePtr Object, std::string Val)
{
	MDValueList::iterator it;

	int Index = 0;
	int Value = -1;
	const char *p = Val.c_str();

	it = Object->Children.begin();

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
			if(Object->GetSize() <= Index)
			{
				Object->ResizeChildren(Index+1);

				// Bug-out early if we hit the end of a fixed length array
				if(Object->GetSize() <= Index) break;

				it = Object->Children.end();
				it--;
			}

			(*it)->SetInt(Value);

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

	MDValueList::iterator it;

	Ret = "";

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		if(Ret.length() != 0) Ret += ", ";
		Ret += "{";
		Ret += (*it)->GetString();
		Ret += "}";
		it++;
	}
	return Ret;
}


void MDTraits_RawArrayArray::SetString(MDValuePtr Object, std::string Val)
{
	MDValueList::iterator it;

	int OpenBracket;
	int CloseBracket = -1;
	
	it = Object->Children.begin();

	for(;;)
	{
		OpenBracket = Val.find("{",CloseBracket+1);
		if(OpenBracket == std::string::npos) return;

		CloseBracket = Val.find("}",OpenBracket+1);
		if(CloseBracket == std::string::npos) return;

		// If we are already at the end of the list, add another
		if(it == Object->Children.end()) 
		{
			Object->ResizeChildren(Object->Children.size()+1);
			it = Object->Children.end();
			it--;
		}

		(*it)->SetString(std::string(Val,OpenBracket+1, (CloseBracket-OpenBracket)-1));

		it++;
	}
}


/************************************************
**   Array of Basic Compound Implementations   **
************************************************/

std::string MDTraits_BasicCompound::GetString(MDValuePtr Object)
{
	std::string Ret;

	int ChildNum = 0;

	MDValueList::iterator it;

	Ret = "";

	it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		if(Ret.length() != 0) Ret += ", ";
		Ret += (Object->ChildName(ChildNum++));
		Ret += "=\"";
		Ret += (*it)->GetString();
		Ret += "\"";
		it++;
	}
	return Ret;
}


void MDTraits_BasicCompound::SetString(MDValuePtr Object, std::string Val)
{
	MDValueList::iterator it;

	int OpenQuote;
	int CloseQuote = -1;

	it = Object->Children.begin();

	for(;;)
	{
		OpenQuote = Val.find("\"",CloseQuote+1);
		if(OpenQuote == std::string::npos) return;

		// DRAGONS: Should add code here to allow out-of-order items

		CloseQuote = Val.find("\"",OpenQuote+1);
		if(CloseQuote == std::string::npos) return;

		// If we are already at the end of the list, add another
		if(it == Object->Children.end()) 
		{
			warning("Extra parameters found parsing string in MDTraits_BasicCompound::SetString()\n");
			break;
		}

		(*it)->SetString(std::string(Val,OpenQuote+1, (CloseQuote-OpenQuote)-1));

		it++;
	}
}


/*	virtual void SetInt64(MDValuePtr Object, Int64 Val);
		virtual void SetUint(MDValuePtr Object, Uint32 Val);
		virtual void SetUint64(MDValuePtr Object, Uint64 Val);
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual Uint32 GetUint(MDValuePtr Object);
		virtual Uint64 GetUint64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);
*/
#if 0

// Default trait implementations
////////////////////////////////

void MDTraits::fSetInt(MDValuePtr Object, Int32 Val) { error("NO BODY!\n"); };
void MDTraits::fSetInt64(MDValuePtr Object, Int64 Val) { error("NO BODY!\n"); };
void MDTraits::fSetUint(MDValuePtr Object, Uint32 Val) { error("NO BODY!\n"); };
void MDTraits::fSetUint64(MDValuePtr Object, Uint64 Val) { error("NO BODY!\n"); };
void MDTraits::fSetString(MDValuePtr Object, std::string Val) { error("NO BODY!\n"); };
Int32 MDTraits::fGetInt(MDValuePtr Object) { error("NO BODY!\n"); return 0;};
Int64 MDTraits::fGetInt64(MDValuePtr Object) { error("NO BODY!\n"); return 0; };
Uint32 MDTraits::fGetUint(MDValuePtr Object) { error("NO BODY!\n"); return 0; };
Uint64 MDTraits::fGetUint64(MDValuePtr Object) { error("NO BODY!\n"); return 0; };
std::string MDTraits::fGetString(MDValuePtr Object) { return std::string("Base"); };


// Extended trait implementations
/////////////////////////////////

/*****************************
**   Int8 Implementations   **
*****************************/

//! Set Int8 from an Int32
void mxflib::Int8_SetInt(MDValuePtr Object, Int32 Val) 
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
void mxflib::Int8_SetInt64(MDValuePtr Object, Int64 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Int8 from a Uint32
void mxflib::Int8_SetUint(MDValuePtr Object, Uint32 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Int8 from a Uint64
void mxflib::Int8_SetUint64(MDValuePtr Object, Uint64 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Int8 from a string
void mxflib::Int8_SetString(MDValuePtr Object, std::string Val) {	Int8_SetInt(Object, (Int32)atoi(Val.c_str())); };

//! Get Int32 from an Int8
Int32 mxflib::Int8_GetInt(MDValuePtr Object) 
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
Int64 mxflib::Int8_GetInt64(MDValuePtr Object) { return (Int64) Int8_GetInt(Object); };

//! Get Uint32 from an Int8
/*! \note
 *	This function will return 128 through 255 for bit values 10000000 through 11111111
 *	even though an Int8 cannot store them. This is as opposed to the option of returning
 *  0xffffff80 through 0xffffffff for those values.
 */
Uint32 mxflib::Int8_GetUint(MDValuePtr Object)
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
Uint64 mxflib::Int8_GetUint64(MDValuePtr Object) { return (Uint64) Int8_GetUint(Object); };

//!	Get string from an Int8
std::string mxflib::Int8_GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%d", Int8_GetInt(Object));
	return std::string(Buffer);
};


/*****************************
**   Uint8 Implementations   **
*****************************/

//! Set Uint8 from an Int32
void mxflib::Uint8_SetInt(MDValuePtr Object, Int32 Val) { Int8_SetInt(Object, Val); };

//! Set Uint8 from an Int64
void mxflib::Uint8_SetInt64(MDValuePtr Object, Int64 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Uint8 from a Uint32
void mxflib::Uint8_SetUint(MDValuePtr Object, Uint32 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Uint8 from a Uint64
void mxflib::Uint8_SetUint64(MDValuePtr Object, Uint64 Val) { Int8_SetInt(Object, (Int32)Val); };

//! Set Uint8 from a string
void mxflib::Uint8_SetString(MDValuePtr Object, std::string Val) { Int8_SetInt(Object, (Int32)atoi(Val.c_str())); };

//! Get Int32 from a Uint8
Int32 mxflib::Uint8_GetInt(MDValuePtr Object) { return Int8_GetInt(Object); };

//! Get Int64 from a Uint8
Int64 mxflib::Uint8_GetInt64(MDValuePtr Object) { return Int8_GetInt64(Object); };

//! Get Uint32 from a Uint8
Uint32 mxflib::Uint8_GetUint(MDValuePtr Object) { return Int8_GetUint(Object); };

//! Get Uint64 from a Uint8
Uint64 mxflib::Uint8_GetUint64(MDValuePtr Object) { return Int8_GetUint64(Object); };

//!	Get string from a Uint8
std::string mxflib::Uint8_GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", Uint8_GetInt(Object));
	return std::string(Buffer);
};


/******************************
**   Int16 Implementations   **
******************************/

//! Set Int16 from an Int32
void mxflib::Int16_SetInt(MDValuePtr Object, Int32 Val) 
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
	// Note that the swap is done in an unsigned int
	// to prevent any sign problems!
	Uint16 i = Swap((Uint16)Val);

	Object->SetValue(2, (Uint8*)&i);
};

//! Set Int16 from an Int64
void mxflib::Int16_SetInt64(MDValuePtr Object, Int64 Val) { Int16_SetInt(Object, (Int32)Val); };

//! Set Int16 from a Uint32
void mxflib::Int16_SetUint(MDValuePtr Object, Uint32 Val) { Int16_SetInt(Object, (Int32)Val); };

//! Set Int16 from a Uint64
void mxflib::Int16_SetUint64(MDValuePtr Object, Uint64 Val) { Int16_SetInt(Object, (Int32)Val); };

//! Set Int16 from a string
void mxflib::Int16_SetString(MDValuePtr Object, std::string Val) { Int16_SetInt(Object, (Int32)atoi(Val.c_str())); };

//! Get Int32 from an Int16
Int32 mxflib::Int16_GetInt(MDValuePtr Object) 
{ 
	int Size = Object->GetSize();

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 2)
	{
		error("Tried to read a 2-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 16-bit value in a Uint to prevent possible sign problems
	Uint16 Val = ((Object->GetData())[1] << 8) | (Object->GetData())[0];
	
	// Endian swap (if required) and convert to signed, then promote to 32-bit
	return (Int32) ((Int16)Swap(Val));
};

//! Get Int64 from an Int16
Int64 mxflib::Int16_GetInt64(MDValuePtr Object) { return (Int64)Int16_GetInt(Object); };

//! Get Uint32 from an Int16
Uint32 mxflib::Int16_GetUint(MDValuePtr Object) 
{ 
	int Size = Object->GetSize();

	// Deal with a NULL variable
	if(Size == 0) return 0;

	if(Size != 2)
	{
		error("Tried to read a 2-byte value from an MDValue that has size %d\n", Size);
		return 0;
	}

	// Build the 16-bit value
	Uint16 Val = ((Object->GetData())[1] << 8) | (Object->GetData())[0];
	
	// Endian swap (if required) then promote to 32-bit
	return (Int32) Swap(Val);
};

//! Get Uint64 from an Int16
Uint64 mxflib::Int16_GetUint64(MDValuePtr Object) { return (Uint64)Int16_GetUint(Object); };

//!	Get string from an Int16
std::string mxflib::Int16_GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%d", Int16_GetInt(Object));
	return std::string(Buffer);
};


/*******************************
**   Uint16 Implementations   **
*******************************/

//! Set Uint16 from an Int32
void mxflib::Uint16_SetInt(MDValuePtr Object, Int32 Val) { Int16_SetInt(Object, Val); };

//! Set Uint16 from an Int64
void mxflib::Uint16_SetInt64(MDValuePtr Object, Int64 Val) { Int16_SetInt(Object, (Int32)Val); };

//! Set Uint16 from a Uint32
void mxflib::Uint16_SetUint(MDValuePtr Object, Uint32 Val) { Int16_SetInt(Object, (Int32)Val); };

//! Set Uint16 from a Uint64
void mxflib::Uint16_SetUint64(MDValuePtr Object, Uint64 Val) { Int16_SetInt(Object, (Int32)Val); };

//! Set Uint16 from a string
void mxflib::Uint16_SetString(MDValuePtr Object, std::string Val) { Int16_SetInt(Object, (Int32)atoi(Val.c_str())); };

//! Get Int32 from an Uint16
Int32 mxflib::Uint16_GetInt(MDValuePtr Object) { return Int16_GetInt(Object); };

//! Get Int64 from an Uint16
Int64 mxflib::Uint16_GetInt64(MDValuePtr Object) { return (Int64)Int16_GetInt(Object); };

//! Get Uint32 from an Uint16
Uint32 mxflib::Uint16_GetUint(MDValuePtr Object) { return Int16_GetUint(Object); };

//! Get Uint64 from an Uint16
Uint64 mxflib::Uint16_GetUint64(MDValuePtr Object) { return (Uint64)Int16_GetUint(Object); };

//!	Get string from an Uint16
std::string mxflib::Uint16_GetString(MDValuePtr Object) 
{ 
	char Buffer[32];					//!< Buffer to hold text version of the value (32 bytes must be enough!)
	sprintf(Buffer, "%u", Int16_GetUint(Object));
	return std::string(Buffer);
};
#endif 0

