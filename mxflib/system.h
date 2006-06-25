/*! \file	system.h
 *	\brief	System specifics
 *
 *  Items that are <b>required</b> to be defined for each platform/compiler:
 *  - Definions for signed and unsigned 64 bit integers (Int64 and UInt64)
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
 *	\version $Id: system.h,v 1.13 2006/06/25 14:46:06 matt-beard Exp $
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
#include <stdlib.h>						// Required for integer conversions

/************************************************/
/*           (Hopefully) Common types           */
/************************************************/
/* Defined here so they can be used in the rest */
/* of this file if required                     */
/************************************************/

namespace mxflib
{
	typedef unsigned int UInt32;		//!< Unsigned 32-bit integer
	typedef unsigned short int UInt16;	//!< Unsigned 16-bit integer
	typedef unsigned char UInt8;		//!< Unsigned 8-bit integer

	typedef int Int32;					//!< Signed 32-bit integer
	typedef short int Int16;			//!< Signed 16-bit integer
	typedef signed char Int8;			//!< Signed 8-bit integer

	struct full_time					//!< Structure for holding accurate time (to nearest 4ms)
	{
		time_t	time;
		int		msBy4;
	};


	// Runtime detection of endian-ness using detection code that executes once.
	// The variable littleEndian is then used to check if bytes read/written to
	// files need to be swapped.
	static bool IsLittleEndian()
	{
		unsigned int val = 0x12345678;
	    unsigned char *p;

   		p = (unsigned char *)&val;

    	if (p[0] == 0x12 && p[1] == 0x34 && p[2] == 0x56 && p[3] == 0x78)
        	return false;
    	else if (p[0] == 0x78 && p[1] == 0x56 && p[2] == 0x34 && p[3] == 0x12)
        	return true;

		return false;
	}

	static bool littleEndian = IsLittleEndian();
}



/************************************************/
/*             Microsoft Visual C++             */
/************************************************/

#ifdef _MSC_VER

// Visual studio 6 specifics
#if _MSC_VER < 1300
#pragma warning(disable:4786)			// Ignore "identifer > 255 characters" warning
										// This is produced from many STL class specialisations
										// Note: Not all these warnings go away (another MS-Bug??)

#ifndef UINT64_C
#define UINT64_C(c)	c					// for defining 64bit constants
#endif // UINT64_C
#ifndef INT64_C
#define INT64_C(c)	c					// for defining 64bit constants
#endif // INT64_C

#else  // _MSC_VER < 1300

#ifndef UINT64_C
#define UINT64_C(c)	c##ULL				// for defining 64bit constants
#endif // UINT64_C
#ifndef INT64_C
#define INT64_C(c)	c##ULL				// for defining 64bit constants
#endif // INT64_C

#endif // _MSC_VER < 1300

#include <crtdbg.h>						//!< Debug header
#include <string>						//!< Required for strings
#include <io.h>							//!< MSVC File I/O

namespace mxflib
{
	typedef __int64 Int64;				//!< Signed 64-bit integer
	typedef unsigned __int64 UInt64;	//!< Unsigned 64-bit integer

	/******** ENDIAN SWAPPING ********/
	inline UInt16 Swap(UInt16 Val) 
	{
		return ((Val & 0xff00) >> 8) | ((Val & 0x00ff) << 8); 
	};

	inline Int16 Swap(Int16 Val) { return (Int16)Swap((UInt16)Val); };
	
	inline UInt32 Swap(UInt32 Val) 
	{ 
		return ( ((Val & 0xff000000) >> 24)
			   | ((Val & 0x00ff0000) >> 8)
			   | ((Val & 0x0000ff00) << 8)
	           | ((Val & 0x000000ff) << 24) ); 
	};
	inline Int32 Swap(Int32 Val) { return (Int32)Swap((UInt32)Val); };

	inline UInt64 Swap(UInt64 Val) 
	{ 
		UInt32 MSW = (UInt32)((Val & 0xffffffff00000000) >> 32);
		UInt32 LSW = (UInt32)(Val & 0x00000000ffffffff);

		MSW = Swap(MSW);
		LSW = Swap(LSW);

		return (((UInt64)LSW) << 32) | ((UInt64)MSW);
	};
	inline Int64 Swap(Int64 Val) { return (Int64)Swap((UInt64)Val); };

	
	/******** Int64 Conversion ********/
	inline Int64 ato_Int64(const char *str) { return _atoi64(str); };
	inline Int64 ato_UInt64(const char *str) { return (UInt64)_atoi64(str); };

	inline std::string Int64toString(Int64 Val)
	{ 
		char Buffer[32];
		_i64toa(Val, Buffer, 10);
		return std::string(Buffer);
	};

	inline std::string UInt64toString(UInt64 Val)
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

#define ASSERT _ASSERT					//!< Debug assert
#define strcasecmp(s1, s2) stricmp(s1, s2)

//! Allow command-line switches to be prefixed with '/' or '-'
#define IsCommandLineSwitchPrefix(x) ( (x == '/') || (x == '-'))

	//! Pause for user input (with prompt) e.g. for debugging purposes
	inline void PauseForInput(void)
	{
		printf("Press enter key...");
		getchar();
		printf("\n");
	}
}
#else // _MSC_VER
namespace mxflib
{
	typedef long long Int64;			//!< Signed 64-bit integer
	typedef unsigned long long UInt64;	//!< Unsigned 64-bit integer
}
#endif // _MSC_VER



// _WIN32 platform specific File I/O, GUID and time functions
#ifdef _WIN32

#include <windows.h>		//!< Required for system specifics (such as CoCreateGuid, GUID)
#include <fcntl.h>			//!< for _O_BINARY etc
#include <sys/stat.h>		//!< for _S_IREAD, _S_IWRITE
#include <sys/timeb.h>		//!< for _timeb

#define DIR_SEPARATOR		'\\'
#define PATH_SEPARATOR		';'
#ifndef DEFAULT_DICT_PATH
#define DEFAULT_DICT_PATH	".\\"
#endif //DEFAULT_DICT_PATH

namespace mxflib
{
	/******** 64-bit file-I/O ********/
#ifndef MXFLIB_NO_FILE_IO
	typedef int FileHandle;
	inline int FileSeek(FileHandle file, UInt64 offset) { return _lseeki64(file, offset, SEEK_SET) == -1 ? -1 : 0; }
	inline int FileSeekEnd(FileHandle file) { return _lseeki64(file, 0, SEEK_END) == -1 ? -1 : 0; }
	
	// DRAGONS: MSVC can't read or write more than 4Gb in one go currently
	inline UInt64 FileRead(FileHandle file, unsigned char *dest, UInt64 size) { return read(file, dest, (unsigned int)size); }
	inline UInt64 FileWrite(FileHandle file, const unsigned char *source, UInt64 size) { return write(file, source, (unsigned int)size); }

	inline int FileGetc(FileHandle file) { UInt8 c; return (FileRead(file, &c, 1) == 1) ? (int)c : EOF; }
	inline FileHandle FileOpen(const char *filename) { return open(filename, _O_BINARY | _O_RDWR ); }
	inline FileHandle FileOpenRead(const char *filename) { return open(filename, _O_BINARY | _O_RDONLY ); }
	inline FileHandle FileOpenNew(const char *filename) { return open(filename, _O_BINARY | _O_RDWR | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE); }
	inline bool FileValid(FileHandle file) { return (file >= 0); }
	inline bool FileEof(FileHandle file) { return eof(file) ? true : false; }
	inline UInt64 FileTell(FileHandle file) { return _telli64(file); }
	inline void FileClose(FileHandle file) { close(file); }
	inline bool FileExists(const char *filename) { struct stat buf; return stat(filename, &buf) == 0; }
#endif //MXFLIB_NO_FILE_IO


	/********* Acurate time *********/
	//! Get the current <b>UTC</b> time including number of milliseconds / 4
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
	inline void MakeUUID(UInt8 *Buffer)
	{
		CoCreateGuid(reinterpret_cast<GUID*>(Buffer));
	}

	//! Determine if the specified filename refers to an absolute path
	inline bool IsAbsolutePath(const char *Filename)
	{
		if(*Filename == '\\') return true;
		if(*Filename == '/') return true;
		if(*Filename != '\0' && Filename[1] == ':') return true;
		return false;
	}
}
#endif // _WIN32


#ifndef _MSC_VER
// Support for all platforms with ISO C++ compilers using autoconf environment
// including the _WIN32 platform with Mingw, Cygwin or Intel compilers.

#include "config.h"		// generated by running ./configure
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#define uintptr_t long
#endif

#ifdef HAVE_UUID_GENERATE
#include <uuid/uuid.h>
#endif

namespace mxflib
{
	/******** ENDIAN SWAPPING ********/
	inline UInt16 Swap(UInt16 Val) 
	{ 
		if (!littleEndian)
			return Val;
		else
			return ((Val & 0xff00) >> 8) | ((Val & 0x00ff) << 8); 
	};

	inline Int16 Swap(Int16 Val) { return (Int16)Swap((UInt16)Val); }
	
	inline UInt32 Swap(UInt32 Val) 
	{ 
		if (!littleEndian)
			return Val;

		return ( ((Val & 0xff000000) >> 24)
			   | ((Val & 0x00ff0000) >> 8)
			   | ((Val & 0x0000ff00) << 8)
	           | ((Val & 0x000000ff) << 24) ); 
	}
	inline Int32 Swap(Int32 Val) { return (Int32)Swap((UInt32)Val); }

	inline UInt64 Swap(UInt64 Val) 
	{
		if (!littleEndian)
			return Val;

		UInt32 MSW = (UInt32)((Val & 0xffffffff00000000LL) >> 32);
		UInt32 LSW = (UInt32)(Val & 0x00000000ffffffff);

		MSW = Swap(MSW);
		LSW = Swap(LSW);

		return (((UInt64)LSW) << 32) | ((UInt64)MSW);
	}
	inline Int64 Swap(Int64 Val) { return (Int64)Swap((UInt64)Val); }

	
	/******** Int64 Conversion ********/
	inline Int64 ato_Int64(const char *str) { return strtoll(str, NULL, 10); }
	inline Int64 ato_UInt64(const char *str) { return strtoull(str, NULL, 10); }

	inline std::string Int64toString(Int64 Val)
	{ 
		char Buffer[32];
		snprintf(Buffer, sizeof(Buffer) - 1, "%lld", Val);
		Buffer[31] = '\0';
		return std::string(Buffer);
	}

	inline std::string UInt64toString(UInt64 Val)
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

	//! Pause for user input (with prompt) e.g. for debugging purposes
	inline void PauseForInput(void)
	{
		printf("Press enter key...");
		getchar();
		printf("\n");
	}

#ifndef _WIN32

#define DIR_SEPARATOR		'/'
#define PATH_SEPARATOR		':'
#ifndef DEFAULT_DICT_PATH
#define DEFAULT_DICT_PATH	"/usr/local/share/mxflib/"
#endif //DEFAULT_DICT_PATH

	/******** 64-bit file-I/O ********/
#ifndef MXFLIB_NO_FILE_IO
	typedef FILE *FileHandle;
	inline int FileSeek(FileHandle file, UInt64 offset) { return fseeko(file, offset, SEEK_SET); }
	inline int FileSeekEnd(FileHandle file) { return fseeko(file, 0, SEEK_END); }
	inline UInt64 FileRead(FileHandle file, unsigned char *dest, UInt64 size) { return fread(dest, 1, size, file); }
	inline UInt64 FileWrite(FileHandle file, const unsigned char *source, UInt64 size) { return fwrite(source, 1, size, file); }
	inline int FileGetc(FileHandle file) { UInt8 c; return (FileRead(file, &c, 1) == 1) ? (int)c : EOF; }
	inline FileHandle FileOpen(const char *filename) { return fopen(filename, "r+b" ); }
	inline FileHandle FileOpenRead(const char *filename) { return fopen(filename, "rb" ); }
	inline FileHandle FileOpenNew(const char *filename) { return fopen(filename, "w+b"); }
	inline bool FileValid(FileHandle file) { return (file != NULL); }
	inline bool FileEof(FileHandle file) { return feof(file); }
	inline UInt64 FileTell(FileHandle file) { return ftello(file); }
	inline void FileClose(FileHandle file) { fclose(file); }
	inline bool FileExists(const char *filename) { struct stat buf; return stat(filename, &buf) == 0; }
#endif //MXFLIB_NO_FILE_IO

	/********* Acurate time *********/
	//! Get the current <b>UTC</b> time including number of milliseconds / 4
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
	inline void MakeUUID(UInt8 *Buffer)
	{
		uuid_t u;
		uuid_generate(u);
		memcpy(Buffer, &u, sizeof(u));
	}
#else // HAVE_UUID_GENERATE
	inline void MakeUUID(UInt8 *Buffer)
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
			srand((time(NULL)) ^ ((uintptr_t)Buffer) ^ (clock() << 2) ^ rand());
			Inited = true;
		}
		int i;
		for(i=0; i<16; i++)
		{
			Buffer[i] = (UInt8)rand();
		}

		// Set reserved bits (variant "10" = ISO/IEC 11578)
		Buffer[8] &= 0x3f;
		Buffer[8] |= 0x80;

		// Set version bits (version "0100" = random or pseudo-random)
		Buffer[7] &= 0x0f;
		Buffer[7] |= 0x40;
	}
#endif // HAVE_UUID_GENERATE

	//! Determine if the specified filename refers to an absolute path
	inline bool IsAbsolutePath(const char *Filename)
	{
		if(*Filename == '/') return true;
		return false;
	}

#endif // _WIN32
}

//! Allow command-line switches to be prefixed only with '-'
#define IsCommandLineSwitchPrefix(x) ( x == '-' )

#ifndef UINT64_C
#define UINT64_C(c)	c##ULL		// for defining 64bit constants
#endif // UINT64_C
#ifndef INT64_C
#define INT64_C(c)	c##ULL		// for defining 64bit constants
#endif // INT64_C

#include <assert.h>
#define ASSERT assert		// use -DNDEBUG

#endif // not _WIN32

/************************************************/
/************************************************/

// Support old capitalization of unsigned integers
namespace mxflib
{
	typedef UInt8 Uint8;
	typedef UInt16 Uint16;
	typedef UInt32 Uint32;
	typedef UInt64 Uint64;
	inline std::string Uint64toString(Uint64 Val) { return UInt64toString(Val); }
}


/*****************************************************/
/*     Declarations for client supplied file-I/O     */
/*****************************************************/
// If File-I/O is supplied by the caller FileHandle will be defined as a UInt32
// The caller may need to do something fancy to cope with this
//
#ifdef MXFLIB_NO_FILE_IO
namespace mxflib
{ 
	typedef UInt32 FileHandle;
	int FileSeek(FileHandle file, UInt64 offset);
	int FileSeekEnd(FileHandle file);
	UInt64 FileRead(FileHandle file, unsigned char *dest, UInt64 size);
	UInt64 FileWrite(FileHandle file, const unsigned char *source, UInt64 size);
	int FileGetc(FileHandle file);
	FileHandle FileOpen(const char *filename);
	FileHandle FileOpenRead(const char *filename);
	FileHandle FileOpenNew(const char *filename);
	bool FileValid(FileHandle file);
	bool FileEof(FileHandle file);
	UInt64 FileTell(FileHandle file);
	void FileClose(FileHandle file);
	bool FileExists(const char *filename);
}
#endif // MXFLIB_NO_FILE_IO


#endif // MXFLIB__SYSTEM_H
