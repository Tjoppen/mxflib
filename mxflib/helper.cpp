/*! \file	helper.cpp
 *	\brief	Verious helper functions
 *
 *	\version $Id: helper.cpp,v 1.2.2.15 2004/11/08 10:09:32 matt-beard Exp $
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
/*! \param Data		A pointer to the buffer to receive the length
 *	\param MaxSize	The maximum length that can be written to the buffer
 *	\param Length	The length to be converted to BER
 *	\param Size		The total number of bytes to use for BER length (or 0 for auto)
 *	\return The number of bytes written
 *	\note If the size is specified it will be overridden for lengths that will not fit in Size,
 *        <b>providing</b> they will fit in MaxSize. However an error message will be produced.
 */
Uint32 mxflib::MakeBER(Uint8 *Data, int MaxSize, Uint64 Length, Uint32 Size /*=0*/)
{
	// Mask showing forbidden bits for various sizes
	static const Uint64 Masks[9] = { UINT64_C(0xffffffffffffff80), UINT64_C(0xffffffffffffff00), 
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
				  Size, Int64toHexString(Length).c_str());

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

	if(Size >(Uint32) MaxSize)
	{
		error("Buffer size given to MakeBER() is %d, however length 0x%s will not fit in that size\n",
			  MaxSize, Int64toHexString(Length).c_str());

		// This will produce an invalid size!!!!
		Size = MaxSize;
	}

	// Shortform encoding
	if(Size == 1)
	{
		Data[0] =(Uint8) Length;
		return 1;
	}

	// Buffer for building BER
	Data[0] = 0x80 + (Size-1);

	// Subscript to write next byte
	int i = Size-1;

	// More speed efficient to write backwards as no need to locate the start
	while(i)
	{
		Data[i] = (Uint8)Length & 0xff;
		Length >>= 8;
		i--;
	}

	// Return the number of bytes written
	return Size;
}


//! Read a BER length
/*! \param Data is a pointer to a pointer to the data so that the pointer will be updated to point to the first byte <b>after</b> the length.
 *  \param MaxSize is the maximum number of bytes available to read the BER length. This function never reads from more than 9 bytes as SMPTE 377M forbids vast BER lengths.
 *  \return The length, or -1 if the data was not a valid BER length
 *  \note MaxSize is signed to allow calling code to end up with -ve available bytes!
 */
Length mxflib::ReadBER(Uint8 **Data, int MaxSize)
{
	if(MaxSize <= 0) return -1;

	// Read the short-form length, or number of bytes for long-form
	int Bytes = *((*Data)++);

	// Return short-form length
	if(Bytes < 128) return (Length)Bytes;

	// 0x80 is not valid for MXF lengths (it is "length not specified" in BER)
	if(Bytes == 128) return -1;

	// Now we have the byte count
	Bytes -= 128;

	// Don't read passed the end of the available data!
	// (We use >= not > as we have already processed one byte)
	if(Bytes >= MaxSize) return -1;

	// Read in each byte
	Length Ret = 0;
	while(Bytes--)
	{
		Ret <<= 8;
		Ret += *((*Data)++);
	}

	return Ret;
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
		*prev++ = (Uint8)(subid & 0x7f) | 0x80; // set msb of every byte
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
UMIDPtr mxflib::MakeUMID(int Type, const UUIDPtr AssetID)
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

	/* Fill the material number with a UUID (no swapping) */

	// If no valid AssetID is provided, create a new one
	if( ( !AssetID ) || ( AssetID->Size() != 16 ) )
	{
		Uint8 UUIDbuffer[16];
		MakeUUID(UUIDbuffer);
		memcpy( &Buffer[16], &UUIDbuffer[0], 16 );
	}
	else
	{
		memcpy( &Buffer[16], AssetID->GetValue(), AssetID->Size() );
	}

	return new UMID(Buffer);
}


//! Read a "Chunk" from a non-MXF file
DataChunkPtr mxflib::FileReadChunk(FileHandle InFile, Uint64 Size)
{
	DataChunkPtr Ret = new DataChunk;
	Ret->Resize((Uint32)Size);

	// Read the data (and shrink chunk to fit)
	Ret->Resize((Uint32)FileRead(InFile, Ret->Data,(Uint32) Size));

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


namespace mxflib
{
	//! The search path - note that "~" is used as a token for "not yet initialized"
	std::string DictionaryPath = "~";
}

//! Set the search path to be used for dictionary files
void mxflib::SetDictionaryPath(std::string NewPath)
{
	DictionaryPath = NewPath;
}

//! Search for a file of a specified name in the current dictionary search path
/*! If the filname is either absolute, or relative to "." or ".." then the 
 *  paths are not searched - just the location specified by that filename.
 *  \return the full path and name of the file, or "" if not found
 */
std::string mxflib::LookupDictionaryPath(const char *Filename)
{
	if(DictionaryPath == "~")
	{

#ifdef MXFDATADIR
		// environment variable name may be specified at build time
		char *env = getenv( MXFDATADIR );
#else
		// default environment variable name is MXFLIB_DATA_DIR
		char *env = getenv( "MXFLIB_DATA_DIR" );
#endif

		// if environment variable not specified, use the platform default
		if( !env ) DictionaryPath = std::string( DEFAULT_DICT_PATH );
		else DictionaryPath = std::string(env);
	}

	return SearchPath(DictionaryPath, Filename);
}


//! Search a path list for a specified file
/*! If the filname is either absolute, or relative to "." or ".." then the 
 *  paths are not searched - just the location specified by that filename.
 *  \return the full path and name of the file, or "" if not found
 */
std::string mxflib::SearchPath(const char *Path, const char *Filename)
{
	// First check to see if the filename is either relative to . (or ..)
	// or absolute in which case we don't search via the path
	
	bool NonPath = false;

	if(*Filename == '.')
	{
		if(Filename[1] == DIR_SEPARATOR) NonPath = true;
		else if((Filename[1] == '.') && (Filename[2] == DIR_SEPARATOR)) NonPath = true;
	}
	else if(IsAbsolutePath(Filename)) NonPath = true;

	// Check the file without path if we should
	if(NonPath)
	{
		if(FileExists(Filename)) return std::string(Filename);
		return "";
	}

	// Buffer bug enough for the full path, directory seperator, filename and a terminating zero
	char *Buffer = new char[strlen(Path) + strlen(Filename) + 2];

	// Start searching all paths
	const char *p = Path;
	while(p && *p)
	{
		char *sep = strchr(p, PATH_SEPARATOR);

		// No more path separators - this is the last path to check
		if(!sep)
		{
			// Copy the whole of the remaining path
			strcpy(Buffer, p );
			
			// Force the loop to stop at the end of this iteration
			p = NULL;
		}
		else
		{
			// Copy the section until the next separator
			strncpy(Buffer, p, sep - p);
			Buffer[sep-p]='\0';

			// Advance the pointer to the character following the separator
			p = sep;
			p++;
		}

		// Establish the length of this path
		int len = strlen(Buffer);

		// Don't search a null path
		if(len == 0) continue;

		// Add a directory separator if required
		if(Buffer[len-1] != DIR_SEPARATOR)
		{
			Buffer[len++] = DIR_SEPARATOR;
			Buffer[len] = '\0';
		}

		// Add the filename
		strcat(Buffer, Filename);

		if(FileExists(Buffer))
		{
			std::string Ret = std::string(Buffer);
			delete[] Buffer;
			return Ret;
		}
	}

	// File not found in any of the paths supplied
	delete[] Buffer;
	return "";
}


// Is a given sequence of bytes a partition pack key?
// We first check if byte 13 == 1 which will be true for all partition packs,
// but is false for all GC sets and packs. Once this matches we can do a full memcmp.
bool mxflib::IsPartitionKey(const Uint8 *Key)
{
	if(Key[12] != 1) return false;

	// DRAGONS: This has version 1 hard coded as byte 8
	const Uint8 DegeneratePartition[13] = { 0x06, 0x0E, 0x2B, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01 };
	if( memcmp(Key, DegeneratePartition, 13) == 0 )
	{
		return true;
	}

	return false;
}

