/*! \file	system.h
 *	\brief	System specifics
 *
 *  Items that are <b>required</b> to be defined for each platform/compiler:
 *  - Definions for signed and unsigned 64 bit integers (Int64 and Uint64)
 *<br>
 *<br>
 *	Items that may need to be defined for each platform/compiler:
 *	- Turning warnings off
 *<br>
 *<br>
 *	Systems currently supported:
 *	- Microsoft Visual C++
 *<br>
 *<br>
 *	\note	File-I/O can be disabled to allow the functions to be supplied by the calling code by defining MXFLIB_NO_FILE_IO
 *
 *	\version $Id: system.h,v 1.1 2004/04/26 18:27:48 asuraparaju Exp $
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

#ifndef MXFLIB__SYSTEM_H
#define MXFLIB__SYSTEM_H

// Required headers for non-system specific bits
#include <time.h>


/************************************************/
/*           (Hopefully) Common types           */
/************************************************/
/* Defined here so they can be used in the rest */
/* of this file if required                     */
/************************************************/

namespace mxflib
{
	typedef unsigned int Uint32;		//!< Unsigned 32-bit integer
	typedef unsigned short int Uint16;	//!< Unsigned 16-bit integer
	typedef unsigned char Uint8;		//!< Unsigned 8-bit integer

	typedef int Int32;					//!< Signed 32-bit integer
	typedef short int Int16;			//!< Signed 16-bit integer
	typedef signed char Int8;			//!< Signed 8-bit integer

	struct full_time					//!< Structure for holding accurate time (to nearest 4ms)
	{
		time_t	time;
		int		msBy4;
	};
}



/************************************************/
/*             Microsoft Visual C++             */
/************************************************/

#ifdef _MSC_VER

#pragma warning(disable:4786)			// Ignore "identifer > 255 characters" warning
										// This is produced from many STL class specialisations
										// Note: Not all these warnings go away (another MS-Bug!!)

#include <crtdbg.h>						//!< Debug header
#include <stdlib.h>						//!< Required for integer conversions
#include <string>						//!< Required for strings
#include <windows.h>					//!< Required for system specifics (such as GUID)

namespace mxflib
{
	typedef __int64 Int64;				//!< Signed 64-bit integer
	typedef unsigned __int64 Uint64;	//!< Unsigned 64-bit integer

	/******** ENDIAN SWAPPING ********/
	inline Uint16 Swap(Uint16 Val) { return ((Val & 0xff00) >> 8) | ((Val & 0x00ff) << 8); };
	inline Int16 Swap(Int16 Val) { return (Int16)Swap((Uint16)Val); };
	
	inline Uint32 Swap(Uint32 Val) 
	{ 
		return ( ((Val & 0xff000000) >> 24)
			   | ((Val & 0x00ff0000) >> 8)
			   | ((Val & 0x0000ff00) << 8)
	           | ((Val & 0x000000ff) << 24) ); 
	};
	inline Int32 Swap(Int32 Val) { return (Int32)Swap((Uint32)Val); };

	inline Uint64 Swap(Uint64 Val) 
	{ 
		Uint32 MSW = (Uint32)((Val & 0xffffffff00000000) >> 32);
		Uint32 LSW = (Uint32)(Val & 0x00000000ffffffff);

		MSW = Swap(MSW);
		LSW = Swap(LSW);

		return (((Uint64)LSW) << 32) | ((Uint64)MSW);
	};
	inline Int64 Swap(Int64 Val) { return (Int64)Swap((Uint64)Val); };

	
	/******** Int64 Conversion ********/
	inline Int64 ato_Int64(const char *str) { return _atoi64(str); };

	inline std::string Int64toString(Int64 Val)
	{ 
		char Buffer[32];
		_i64toa(Val, Buffer, 10);
		return std::string(Buffer);
	};

	inline std::string Uint64toString(Uint64 Val)
	{ 
		char Buffer[32];
		_ui64toa(Val, Buffer, 10);
		return std::string(Buffer);
	};

	inline std::string Int64toHexString(Int64 Val, int Digits = 0)
	{
		char Buffer[32];
		if(Digits > 30) Digits = 30;
		sprintf(Buffer,"%0*I64x", Digits, Val );
		return std::string(Buffer);
	};

	/******** 64-bit file-I/O ********/

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#ifndef MXFLIB_NO_FILE_IO
	typedef int FileHandle;
	inline int FileSeek(FileHandle file, Uint64 offset) { return _lseeki64(file, offset, SEEK_SET) == -1 ? -1 : 0; }
	inline int FileSeekEnd(FileHandle file) { return _lseeki64(file, 0, SEEK_END) == -1 ? -1 : 0; }
	inline Uint64 FileRead(FileHandle file, unsigned char *dest, Uint64 size) { return read(file, dest, size); }
	inline Uint64 FileWrite(FileHandle file, const unsigned char *source, Uint64 size) { return write(file, source, size); }
	inline Uint8 FileGetc(FileHandle file) { Uint8 c; FileRead(file, &c, 1); return c; }
	inline FileHandle FileOpen(const char *filename) { return open(filename, _O_BINARY | _O_RDWR ); }
	inline FileHandle FileOpenRead(const char *filename) { return open(filename, _O_BINARY | _O_RDONLY ); }
	inline FileHandle FileOpenNew(const char *filename) { return open(filename, _O_BINARY | _O_RDWR | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE); }
	inline bool FileValid(FileHandle file) { return (file >= 0); }
	inline bool FileEof(FileHandle file) { return eof(file) ? true : false; }
	inline Uint64 FileTell(FileHandle file) { return _telli64(file); }
	inline void FileClose(FileHandle file) { close(file); }
#endif //MXFLIB_NO_FILE_IO



	/********* Acurate time *********/
	inline full_time GetTime(void)
	{
		full_time Ret;
		_timeb tb;
		_ftime(&tb);
		Ret.time = tb.time;
		Ret.msBy4 = tb.millitm / 4;
		return Ret;
	}

	/******** UUID Generation ********/
	inline void MakeUUID(Uint8 *Buffer)
	{
		CoCreateGuid(reinterpret_cast<GUID*>(Buffer));
	}
}

//! Allow command-line switches to be prefixed with '/' or '-'
#define IsCommandLineSwitchPrefix(x) ( (x == '/') || (x == '-'))

#define UINT64_C(c)	c			// for defining 64bit constants

#define ASSERT _ASSERT					//!< Debug assert
#define strcasecmp(s1, s2) stricmp(s1, s2)

#else	/* for ISO C++ compilers */

#include <stdlib.h>
#include <string>
#include <sys/time.h>

namespace mxflib
{
	typedef long long Int64;			//!< Signed 64-bit integer
	typedef unsigned long long Uint64;	//!< Unsigned 64-bit integer

	/******** ENDIAN SWAPPING ********/
	inline Uint16 Swap(Uint16 Val) { return ((Val & 0xff00) >> 8) | ((Val & 0x00ff) << 8); }
	inline Int16 Swap(Int16 Val) { return (Int16)Swap((Uint16)Val); }
	
	inline Uint32 Swap(Uint32 Val) 
	{ 
		return ( ((Val & 0xff000000) >> 24)
			   | ((Val & 0x00ff0000) >> 8)
			   | ((Val & 0x0000ff00) << 8)
	           | ((Val & 0x000000ff) << 24) ); 
	}
	inline Int32 Swap(Int32 Val) { return (Int32)Swap((Uint32)Val); }

	inline Uint64 Swap(Uint64 Val) 
	{ 
		Uint32 MSW = (Uint32)((Val & 0xffffffff00000000LL) >> 32);
		Uint32 LSW = (Uint32)(Val & 0x00000000ffffffff);

		MSW = Swap(MSW);
		LSW = Swap(LSW);

		return (((Uint64)LSW) << 32) | ((Uint64)MSW);
	}
	inline Int64 Swap(Int64 Val) { return (Int64)Swap((Uint64)Val); }

	
	/******** Int64 Conversion ********/
	inline Int64 ato_Int64(const char *str) { return atoll(str); }

	inline std::string Int64toString(Int64 Val)
	{ 
		char Buffer[32];
		snprintf(Buffer, sizeof(Buffer) - 1, "%lld", Val);
		Buffer[31] = '\0';
		return std::string(Buffer);
	}

	inline std::string Uint64toString(Uint64 Val)
	{ 
		char Buffer[32];
		snprintf(Buffer, sizeof(Buffer) - 1, "%llu", Val);
		Buffer[31] = '\0';
		return std::string(Buffer);
	}

	inline std::string Int64toHexString(Int64 Val, int Digits = 0)
	{
		char Buffer[32];
		if(Digits > 30) Digits = 30;
		sprintf(Buffer,"%0*llx", Digits, Val );
		return std::string(Buffer);
	}

	/******** 64-bit file-I/O ********/

#ifndef MXFLIB_NO_FILE_IO
	typedef FILE *FileHandle;
	inline int FileSeek(FileHandle file, Uint64 offset) { return fseeko(file, offset, SEEK_SET); }
	inline int FileSeekEnd(FileHandle file) { return fseeko(file, 0, SEEK_END); }
	inline Uint64 FileRead(FileHandle file, unsigned char *dest, Uint64 size) { return fread(dest, 1, size, file); }
	inline Uint64 FileWrite(FileHandle file, const unsigned char *source, Uint64 size) { return fwrite(source, 1, size, file); }
	inline Uint8 FileGetc(FileHandle file) { Uint8 c; FileRead(file, &c, 1); return c; }
	inline FileHandle FileOpen(const char *filename) { return fopen(filename, "r+b" ); }
	inline FileHandle FileOpenRead(const char *filename) { return fopen(filename, "rb" ); }
	inline FileHandle FileOpenNew(const char *filename) { return fopen(filename, "w+b"); }
	inline bool FileValid(FileHandle file) { return (file != NULL); }
	inline bool FileEof(FileHandle file) { return feof(file); }
	inline Uint64 FileTell(FileHandle file) { return ftello(file); }
	inline void FileClose(FileHandle file) { fclose(file); }
#endif //MXFLIB_NO_FILE_IO



	/********* Acurate time *********/
	inline full_time GetTime(void)
	{
		full_time Ret;
		struct timeval	tv;

		gettimeofday(&tv, NULL);

		Ret.time = tv.tv_sec;
		// msBy4 is in units of 0.025 seconds
		Ret.msBy4 = (int)(tv.tv_usec / 4000 + 0.5);	// round to nearest 0.025s unit
		return Ret;
	}

	/******** UUID Generation ********/
#ifdef HAVE_UUID_GENERATE
#include <uuid/uuid.h>
	inline void MakeUUID(Uint8 *Buffer)
	{
		uuid_t u;
		uuid_generate(u);
		memcpy(Buffer, &u, sizeof(u));
	}
#else // HAVE_UUID_GENERATE
	inline void MakeUUID(Uint8 *Buffer)
	{
		static bool Inited = false;
		if(!Inited)
		{
			// Attempt a reasonably random seed to prevent duplicate UUIDs
			// The time is normally good enough as a seed, except multiple processes may run this code at the
			// same time across all machines in use at any time.  The address of the buffer will depend on the
			// target platform and other processes running on the same machine.  The value of clock() will
			// depend on how much CPU time has elapsed since the program started and is shifted to reduce the
			// chance that a system using the same granularity for this and time() will simply give the
			// program start time. Why also include a random number in the seed? Because if someone has already
			// seeded the generator with a decent random number this will be taken into account to prevent
			// degrading the randomness
			srand(((unsigned int) time(NULL)) ^ ((unsigned int)Buffer) ^ ((unsigned int)clock() << 2) ^ rand());
			Inited = true;
		}
		int i;
		for(i=0; i<16; i++)
		{
			Buffer[i] = (Uint8)rand();
		}

		// Set reserved bits (variant "10" = ISO/IEC 11578)
		Buffer[8] &= 0x3f;
		Buffer[8] |= 0x80;

		// Set version bits (varsion "0100" = random or pseudo-random)
		Buffer[7] &= 0x0f;
		Buffer[7] |= 0x40;
	}
#endif
}

//! Allow command-line switches to be prefixed only with '-'
#define IsCommandLineSwitchPrefix(x) ( x == '-' )

#ifndef UINT64_C
#define UINT64_C(c)	c##ULL		// for defining 64bit constants
#endif // UINT64_C

#include <assert.h>
#define ASSERT assert		// use -DNDEBUG

#endif // _MSC_VER

/************************************************/
/************************************************/


/*****************************************************/
/*     Declarations for client supplied file-I/O     */
/*****************************************************/
// If File-I/O is supplied by the caller FileHandle will be defined as a Uint32
// The caller may need to do something fancy to cope with this
//
#ifdef MXFLIB_NO_FILE_IO
namespace mxflib
{ 
	typedef Uint32 FileHandle;
	int FileSeek(FileHandle file, Uint64 offset);
	int FileSeekEnd(FileHandle file);
	Uint64 FileRead(FileHandle file, unsigned char *dest, Uint64 size);
	Uint64 FileWrite(FileHandle file, const unsigned char *source, Uint64 size);
	Uint8 FileGetc(FileHandle file);
	FileHandle FileOpen(const char *filename);
	FileHandle FileOpenRead(const char *filename);
	FileHandle FileOpenNew(const char *filename);
	bool FileValid(FileHandle file);
	bool FileEof(FileHandle file);
	Uint64 FileTell(FileHandle file);
	void FileClose(FileHandle file);
}
#endif // MXFLIB_NO_FILE_IO


#endif // MXFLIB__SYSTEM_H
