/*! \file	endian.h
 *	\brief	Integer read and write functions header
 *
 *  Converted from the version in "klvlib"
 *
 */
/*
 *	Copyright (c) 2001 BBC R&D and 2003 Matt Beard
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


namespace mxflib
{
	/*
	** PutUxx() - Put unsigned xx-bit integer
	*/
	inline void PutU8(Uint8 x, unsigned char *dest) { *dest=(x); }
	inline void PutU16(Uint16 Data, unsigned char *Dest) { PutU8(Data >> 8, Dest); PutU8(Data & 0xff, &Dest[1]); }
	inline void PutU32(Uint32 Data, unsigned char *Dest) { PutU16(Data >> 16, Dest); PutU16(Data & 0xffff, &Dest[2]); }
	inline void PutU64(Uint64 Data, unsigned char *Dest) { PutU32(Data >> 32, Dest); PutU32(Data & 0xffffffff, &Dest[4]); }

	/*
	** PutIxx() - Signed versions of PutUxx()
	*/
	inline void PutI8(Int8 x, unsigned char *dest) { PutU8((Uint8)x,dest); }
	inline void PutI16(Int16 x, unsigned char *dest) { PutU16((Uint16)x,dest); }
	inline void PutI32(Int32 x, unsigned char *dest) { PutU32((Uint32)x,dest); }
	inline void PutI64(Int64 x, unsigned char *dest) { PutU64((Uint64)x,dest); }

	/*
	** GetUxx() - Get unsigned xx-bit integer
	*/
	inline Uint8 GetU8(unsigned char *src) { return (Uint8) *src; }
	inline Uint16 GetU16(unsigned char *src) { return (GetU8(src) << 8) | GetU8(&src[1]); }
	inline Uint32 GetU32(unsigned char *src) { return (GetU16(src) << 16) | GetU16(&src[2]); }
	inline Uint64 GetU64(unsigned char *src) { return (GetU32(src) << 32) | GetU32(&src[4]); }

	/*
	** GetIxx() - Signed versions of GetUxx()
	*/
	inline Int8 GetI8(unsigned char *src) { return (Int8)GetU8(src); }
	inline Int16 GetI16(unsigned char *src) { return (Int16)GetU16(src); }
	inline Int32 GetI32(unsigned char *src) { return (Int32)GetU32(src); }
	inline Int64 GetI64(unsigned char *src) { return (Int64)GetU64(src); }

	/*
	** GetUxx_LE() - Get LITTLE ENDIAN unsigned xx-bit integer
	*/
	inline Uint8 GetU8_LE(unsigned char *src) { return (Uint8) *src; }
	inline Uint16 GetU16_LE(unsigned char *src) { return GetU8_LE(src) | (GetU8_LE(&src[1]) << 8); }
	inline Uint32 GetU32_LE(unsigned char *src) { return GetU16_LE(src) | (GetU16_LE(&src[2]) << 16); }
	inline Uint64 GetU64_LE(unsigned char *src) { return GetU32_LE(src) | (GetU32_LE(&src[4]) << 32); }

	/*
	** GetIxx_LE() - Signed versions of GetUxx_LE()
	*/
	inline Int8 GetI8_LE(unsigned char *src) { return (Int8)GetU8_LE(src); }
	inline Int16 GetI16_LE(unsigned char *src) { return (Int16)GetU16_LE(src); }
	inline Int32 GetI32_LE(unsigned char *src) { return (Int32)GetU32_LE(src); }
	inline Int64 GetI64_LE(unsigned char *src) { return (Int64)GetU64_LE(src); }
}

