/*! \file	helper.h
 *	\brief	Verious helper function declarations
 *
 *	\version $Id: helper.h,v 1.3 2004/11/12 09:20:44 matt-beard Exp $
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

#ifndef MXFLIB__HELPER_H
#define MXFLIB__HELPER_H


#include <time.h>
#include <string>

namespace mxflib
{
	//! Make a string containing a number
	inline std::string Int2String(int Num, int Digits = 0)
	{
		char Buffer[18];
		if(Digits > 16) Digits = 16;
		sprintf(Buffer, "%0*d", Digits, Num);
		return std::string(Buffer);
	}

	//! Make a string containing an unsigned number
	inline std::string Uint2String(int Num, int Digits = 0)
	{
		char Buffer[18];
		if(Digits > 16) Digits = 16;
		sprintf(Buffer, "%0*u", Digits, Num);
		return std::string(Buffer);
	}

	//! Make a hex string containing a number
	inline std::string Int2HexString(int Num, int Digits = 0)
	{
		char Buffer[18];
		if(Digits > 16) Digits = 16;
		sprintf(Buffer, "%0*x", Digits, Num);
		return std::string(Buffer);
	}

	//! Convert a time to an ISO 8601 string
	/*! \note ISO 8601 suggests "T" as a separator between date and time. 
	 *	To get this behaviour set StrictISO to true
	 *	\note ANSI-C doesn't seem to have a way to get milliseconds */
	inline std::string Time2String(full_time Time, bool StrictISO = false)
	{
		char Buffer[32];
		
		if(StrictISO)
			strftime(Buffer, 31, "%Y-%m-%dT%H:%M:%S.", localtime( &Time.time ));
		else
			strftime(Buffer, 31, "%Y-%m-%d %H:%M:%S.", localtime( &Time.time ));

		// Append the milliseconds
		sprintf(&Buffer[strlen(Buffer)], "%03d", Time.msBy4 * 4);

		return std::string(Buffer);
	}

	//! Get the current time as an ISO 8601 string
	/*! \note ISO 8601 suggests "T" as a separator between date and time. 
	 *	To get this behaviour set StrictISO to true */
	inline std::string Now2String(bool StrictISO = false)
	{
		full_time now = GetTime();
		
		return Time2String(now, StrictISO);
	}

	//! Build a BER length
	/*! \param Data		A pointer to the buffer to receive the length
	 *	\param MazSize	The maximum length that can be written to the buffer
	 *	\param Length	The length to be converted to BER
	 *	\param Size		The total number of bytes to use for BER length (or 0 for auto)
	 *	\return The number of bytes written
	 *	\note If the size is specified it will be overridden for lengths that will not fit in Size,
	 *        <b>providing</b> they will fit in MaxSize. However an error message will be produced.
	 */
	Uint32 MakeBER(Uint8 *Data, int MaxSize, Uint64 Length, Uint32 Size = 0);


	//! Build a BER length
	/*! \param Length	The length to be converted to BER
	 *	\param Size		The total number of bytes to use for BER length (or 0 for auto)
	 *	\note If the size is specified it will be overridden for lengths
	 *		  that will not fit. However an error message will be produced.
	 */
	inline DataChunkPtr MakeBER(Uint64 Length, Uint32 Size = 0)
	{
		// Buffer for building BER
		Uint8 Buff[9];

		Uint32 Bytes = MakeBER(Buff, 9, Length, Size);

		// Return as a DataChunk
		return new DataChunk(Bytes, Buff);
	}

	//! Read a BER length
	Length ReadBER(Uint8 **Data, int MaxSize);


	//! Encode a Uint64 as a BER OID subid (7 bits per byte)
	//! length > 0: length is maximum length of subid
	//! length == 0: as long as necessary
	//! length < 0: -length is exact length of subid
	//! returns number of bytes used
	int EncodeOID( Uint8* presult, Uint64 subid, int length );

	//! Build a new UMID
	UMIDPtr MakeUMID(int Type, const UUIDPtr AssetID = NULL);

	//! Read a "Chunk" from a non-MXF file
	DataChunkPtr FileReadChunk(FileHandle InFile, Uint64 Size);

	//! Read a RIFF chunk header (from an open file)
	/*! The Chunk ID is read as a big-endian Uint32 and returned as the first
	 *	part of the returned pair. The chunk size is read as a little-endian
	 *	number and returned as the second part of the returned pair
	 *	\return <0,0> if the header counld't be read
	 */
	inline U32Pair ReadRIFFHeader(FileHandle InFile)
	{
		U32Pair Ret;

		Uint8 Buffer[8];
		if(FileRead(InFile, Buffer, 8) < 8)
		{
			Ret.first = 0;
			Ret.second = 0;
			return Ret;
		}

		Ret.first = GetU32(Buffer);
		Ret.second = GetU32_LE(&Buffer[4]);

		return Ret;
	}

	//! Set a data chunk from a hex string
	DataChunkPtr Hex2DataChunk(std::string Hex);

	//! Set the search path to be used for dictionary files
	void SetDictionaryPath(std::string NewPath);

	//! Set the search path to be used for dictionary files
	inline void SetDictionaryPath(const char *NewPath) { SetDictionaryPath(std::string(NewPath)); }

	//! Search for a file of a specified name in the current dictionary search path
	/*! If the filname is either absolute, or relative to "." or ".." then the 
	 *  paths are not searched - just the location specified by that filename.
	 *  \return the full path and name of the file, or "" if not found
	 */
	std::string LookupDictionaryPath(const char *Filename);
	
	//! Search for a file of a specified name in the current dictionary search path
	inline std::string LookupDictionaryPath(std::string Filename) { return LookupDictionaryPath(Filename.c_str()); }

	//! Search a path list for a specified file
	/*! If the filname is either absolute, or relative to "." or ".." then the 
	 *  paths are not searched - just the location specified by that filename.
	 *  \return the full path and name of the file, or "" if not found
	 */
	std::string SearchPath(const char *Path, const char *Filename);

	//! Search a path list for a specified file
	inline std::string SearchPath(std::string Path, std::string Filename) { return SearchPath(Path.c_str(), Filename.c_str()); }


	// File read primitives

	//! Read 8-bit unsigned integer
	inline Uint8 ReadU8(FileHandle Handle) { unsigned char Buffer[1]; if(FileRead(Handle, Buffer, 1) == 1) return GetU8(Buffer); else return 0; }

	//! Read 16-bit unsigned integer
	inline Uint16 ReadU16(FileHandle Handle) { unsigned char Buffer[2]; if(FileRead(Handle, Buffer, 2) == 2) return GetU16(Buffer); else return 0; }

	//! Read 32-bit unsigned integer
	inline Uint32 ReadU32(FileHandle Handle) { unsigned char Buffer[4]; if(FileRead(Handle, Buffer, 4) == 4) return GetU32(Buffer); else return 0; }

	//! Read 64-bit unsigned integer
	inline Uint64 ReadU64(FileHandle Handle) { unsigned char Buffer[8]; if(FileRead(Handle, Buffer, 8) == 8) return GetU64(Buffer); else return 0; }

	//! Read 8-bit signed integer (casts from unsigned version)
	inline Int8 ReadI8(FileHandle Handle) { return (Int8)ReadU8(Handle); }

	//! Read 16-bit signed integer (casts from unsigned version)
	inline Int16 ReadI16(FileHandle Handle) { return (Int16)ReadU16(Handle); }
	
	//! Read 32-bit signed integer (casts from unsigned version)
	inline Int32 ReadI32(FileHandle Handle) { return (Int32)ReadU32(Handle); }
	
	//! Read 64-bit signed integer (casts from unsigned version)
	inline Int64 ReadI64(FileHandle Handle) { return (Int64)ReadU64(Handle); }


	// Is a given sequence of bytes a partition pack key?
	bool IsPartitionKey(const Uint8 *Key);
}

#endif // MXFLIB__HELPER_H
