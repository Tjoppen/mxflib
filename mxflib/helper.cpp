/*! \file	helper.cpp
 *	\brief	Verious helper functions
 *
 *	\version $Id: helper.cpp,v 1.2.2.2 2004/05/18 18:31:40 matt-beard Exp $
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


//! Build a BER length
/*! \param Length	The length to be converted to BER
 *	\param Size		The total number of bytes to use for BER length (or 0 for auto)
 *	\note If the size is specified it will be overridden for lengths
 *		  that will not fit. However an error message will be produced.
 */
DataChunkPtr mxflib::MakeBER(Uint64 Length, Uint32 Size /*=0*/)
{
	// Mask showing forbidden bits for various sizes
	static const Uint64 Masks[9] = { UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffff00), 
									 UINT64_C(0xffffffffffff0000), UINT64_C(0xffffffffff000000),
									 UINT64_C(0xffffffff00000000), UINT64_C(0xffffff0000000000),
									 UINT64_C(0xffff000000000000), UINT64_C(0xff00000000000000), 0 };
	if(Size > 9)
	{
		error("Maximum BER size is 9 bytes, however %d bytes specified in call to MakeBER()\n", Size);
		Size = 9;
	}

	// Validate size
	if(Size)
	{
		if(Length & Masks[Size-1])
		{
			error("BER size specified in call to MakeBER() is %d, however length 0x%s will not fit in that size\n",
				  Size, Int64toHexString(Length, 8).c_str());

			// Force a new size to be chosen
			Size = 0;
		}
	}

	// Determine the best BER size
	if(Size == 0)
	{
		if(Length < 0x01000000) Size = 4;
		else if(Length < UINT64_C(0x0100000000000000)) Size = 8;
		else Size = 9;
	}

	// Buffer for building BER
	Uint8 Buff[9];
	Buff[0] = 0x80 + (Size-1);
	
	// Subscript to write next byte
	int i = Size-1;
	
	// More speed efficient to write backwards as no need to locate the start
	while(i)
	{
		Buff[i] = Length & 0xff;
		Length >>= 8;
		i--;
	}

	// Return as a DataChunk
	return new DataChunk(Size, Buff);
}


//! Encode a Uint64 as a BER OID subid (7 bits per byte)
//! length > 0: length is maximum length of subid
//! length == 0: as long as necessary
//! length < 0: -length is EXACT length of subid
//! returns number of bytes UNUSED (-ve is error)
int mxflib::EncodeOID( Uint8* presult, Uint64 subid, int length )
{
	Uint8 rev[10];			// intermediate result (reverse byte order)
	Uint8 *prev = rev;
	int count = 0;			// bytes required to represent

	do
	{
		*prev++ = (subid & 0x7f) | 0x80; // set msb of every byte
		subid >>= 7;
		count++;
	}
	while( subid );

	rev[0] &= 0x7f; // clear msb of least significant byte

	if( length>0 && count<=length )
	{
		do *presult++ = *--prev; while( --count );		// copy result
		return length-count;
	}
	else if( length<0 )
	{
		int cm = count - (-length);
		if( cm<0 ) return cm;							// error
		while( cm-- ) *presult++ = 0x80;				// pad 
		do *presult++ = *--prev; while( --count );		// copy result
		return 0;										// i.e. none unused
	}
	else // any length
	{
		do *presult++ = *--prev; while( --count );		// copy result
		return 0;
	}
}


//! Build a new UMID
UMIDPtr mxflib::MakeUMID(int Type)
{
	static const Uint8 UMIDBase[10] = { 0x06, 0x0a, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
	Uint8 Buffer[32];

	// Set the non-varying base of the UMID
	memcpy(Buffer, UMIDBase, 10);

	// Correct to v5 dictionary for new (330M-2003) types
	if( Type > 4 ) Buffer[7] = 5;

	// Set the type
	Buffer[10] = Type;

	// We are using a GUID for material number, and no defined instance method
	Buffer[11] = 0x20;

	// Length of UMID "Value" is 19 bytes
	Buffer[12] = 0x13;

	// Set instance number to zero as this is the first instance of this material
	Buffer[13] = 0;
	Buffer[14] = 0;
	Buffer[15] = 0;

	// Fill the material number with a GUID
	MakeUUID(&Buffer[16]);

	return new UMID(Buffer);
}


//! Read a "Chunk" from a non-MXF file
DataChunkPtr mxflib::FileReadChunk(FileHandle InFile, Uint64 Size)
{
	DataChunkPtr Ret = new DataChunk;
	Ret->Resize(Size);

	// Read the data (and shrink chunk to fit)
	Ret->Resize(FileRead(InFile, Ret->Data, Size));

	return Ret;
}


//! Set a data chunk from a hex string
DataChunkPtr mxflib::Hex2DataChunk(std::string Hex)
{
	// Build the result chunk
	DataChunkPtr Ret = new DataChunk();
	
	// Use a granularity of 16 as most hex strings are likely to be 16 or 32 bytes
	// DRAGONS: We may want to revise this later
	Ret->SetGranularity(16);

	// Index the hex string
	char const *p = Hex.c_str();

	int Size = 0;
	int Value = -1;

	// During this loop Value = -1 when no digits of a number are mid-process
	// This stops a double space being regarded as a small zero in between two spaces
	// It also stops a trailing zero being appended to the data if the last character
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
			Size++;
			Ret->Resize(Size);
			Ret->Data[Size-1] = Value;

			Value = -1;
			continue;
		}

		if(Value == -1) Value = 0; else Value <<=4;
		Value += digit;

	// Note that the loop test is done in this way to force
	// a final cycle of the loop with *p == 0 to allow the last
	// number to be processed
	} while(*(p++));

	return Ret;
}


// Find the specified XML file by searching the MXFLIB_DATA_DIR directory
// then the configured MXFDATADIR path.
// If no matching file is found, return NULL.
// TODO: add an mxflib namespace global for command-line/runtime use.
char *mxflib::lookupDataFilePath(const char *filename)
{
	char *buf = new char[FILENAME_MAX];

	// TODO: trap buffer overflows

	// Try under MXFLIB_DATA_DIR env variable (if set)
	if (getenv("MXFLIB_DATA_DIR"))
	{
		sprintf(buf, "%s%c%s", getenv("MXFLIB_DATA_DIR"), DIR_SEPARATOR, filename);

		if (FileExists(buf))
			return buf;
	}

	// Try under the legacy MXFLIB_DICT_PATH env variable (if set)
	if (getenv("MXFLIB_DICT_PATH"))
	{
		sprintf(buf, "%s%c%s", getenv("MXFLIB_DICT_PATH"), DIR_SEPARATOR, filename);

		if (FileExists(buf))
			return buf;
	}

	// Try under MXFDATADIR compile-time macro
	sprintf(buf, "%s%c%s", MXFDATADIR, DIR_SEPARATOR, filename);

	if (FileExists(buf))
		return buf;

	return NULL;
}


// Is a given sequence of bytes a partition pack key?
// We first check if byte 13 == 1 which will be true for all partition packs,
// but is false for all GC sets and packs. Once this matches we can do a full memcmp.
bool mxflib::IsPartitionKey(const Uint8 *Key)
{
	if(Key[12] != 1) return false

	// DRAGONS: This has version 1 hard coded as byte 8
	const Uint8 DegeneratePartition[13] = { 0x06, 0x0E, 0x2B, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01 };
	if( memcmp(Key, DegeneratePartition, 13) == 0 )
	{
		return true;
	}

	return false;
}

