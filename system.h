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

/************************/
/* Microsoft Visual C++ */
/************************/

#ifdef _MSC_VER
namespace mxflib
{
	typedef __int64 Int64;				//!< Signed 64-bit integer
	typedef unsigned __int64 Uint64;	//!< Unsigned 64-bit integer

#pragma warning(disable:4786)			// Ignore "identifer > 255 characters" warning
										// This is produced from many STL class specialisations
										// Note: Not all these warnings go away (another MS-Bug!!)
}
#endif _MSC_VER

#endif MXFLIB__SYSTEM_H
