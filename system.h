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

#ifndef MXFLIB__SYSTEM_H
#define MXFLIB__SYSTEM_H

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
}


/************************************************/
/*             Microsoft Visual C++             */
/************************************************/

#ifdef _MSC_VER

#include <crtdbg.h>						//!< Debug header

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
}

#pragma warning(disable:4786)			// Ignore "identifer > 255 characters" warning
										// This is produced from many STL class specialisations
										// Note: Not all these warnings go away (another MS-Bug!!)

#define ASSERT _ASSERT					//!< Debug assert

#endif _MSC_VER

/************************************************/
/************************************************/


#endif MXFLIB__SYSTEM_H
