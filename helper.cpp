/*! \file	helper.cpp
 *	\brief	Verious helper functions
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

#include "mxflib.h"

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
	static const Uint64 Masks[9] = { 0xffffffffffffffff, 0xffffffffffffff00, 
									 0xffffffffffff0000, 0xffffffffff000000,
									 0xffffffff00000000, 0xffffff0000000000,
									 0xffff000000000000, 0xff00000000000000, 0 };
	if(Size > 9)
	{
		error("Maximum BER size is 9 bytes, however %d bytes specified in call to WriteBER()\n", Size);
		Size = 9;
	}

	// Validate size
	if(Size)
	{
		if(Length & Masks[Size-1])
		{
			error("BER size specified in call to WriteBER() is %d, however length 0x%s will not fit in that size\n",
				  Size, Int64toHexString(Length, 8).c_str());

			// Force a new size to be chosen
			Size = 0;
		}
	}

	// Determine the best BER size
	if(Size == 0)
	{
		if(Length < 0x01000000) Size = 4;
		else if(Length < 0x0100000000000000) Size = 8;
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


//! Build a new UMID
UMIDPtr mxflib::MakeUMID(int Type)
{
	static const Uint8 UMIDBase[10] = { 0x06, 0x0a, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
	Uint8 Buffer[32];

	// Set the non-varying base of the UMID
	memcpy(Buffer, UMIDBase, 10);

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
