/*! \file	defines.h
 *	\brief	The main MXF data types
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

#ifndef MXFLIB__TYPES_H
#define MXFLIB__TYPES_H

// Ensure NULL is available
#include <stdlib.h>

/*                        */
/* Basic type definitions */
/*                        */

namespace mxflib
{
	typedef unsigned int Uint32;		//!< Unsigned 32-bit integer
	typedef unsigned short int Uint16;	//!< Unsigned 16-bit integer
	typedef unsigned char Uint8;		//!< Unsigned 8-bit integer

	typedef int Int32;					//!< Signed 32-bit integer
	typedef short int Int16;			//!< Signed 16-bit integer
	typedef char Int8;					//!< Signed 8-bit integer

	typedef Int64 Length;				//!< Lenth of an item in bytes
	typedef Int64 Position;				//!< Position within an MXF file

	typedef Uint16 Tag;					//!< 2-byte tag for local sets
}

namespace mxflib
{
	//! Draft version of Identifier base type (DRAGONS)
	template <int SIZE> class Identifier
	{
	private:
		Uint8 Ident[SIZE];
	public:
		Identifier(Uint8 *ID = NULL) { if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID, SIZE); };
	};
}

#include <list>
namespace mxflib
{
	//! Draft version of Vector base type (DRAGONS)
	template <class T> class Vector : public std::list<T>
	{
	private:
	public:
	};
}


namespace mxflib
{
	typedef Identifier<16> UL;
	typedef Vector<UL> ULVector;
}

namespace mxflib
{
	//! Draft Variable Type Definition (VTypeDef) class (DRAGONS)
	class VTypeDef
	{
	private:
		char *TypeName;
		int Size;
		bool isVector;
	};
}

#endif MXFLIB__TYPES_H
