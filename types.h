/*! \file	types.h
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
	typedef Int64 Length;				//!< Lenth of an item in bytes
	typedef Int64 Position;				//!< Position within an MXF file

	typedef Uint16 Tag;					//!< 2-byte tag for local sets
}

// Some string conversion utilities
namespace mxflib
{
	// String version of a tag
	inline std::string Tag2String(Tag value)
	{
		char Buffer[8];
		sprintf(Buffer, "%02x.%02x", value >> 8, value & 0xff);
		return std::string(Buffer);
	}
}


namespace mxflib
{
	//! Draft version of Identifier base type (DRAGONS)
	template <int SIZE> class Identifier : public RefCount<Identifier>
	{
	private:
		Uint8 Ident[SIZE];
	public:
		Identifier(const Uint8 *ID = NULL) { if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID, SIZE); };
		Identifier(const Identifier *ID) { ASSERT(SIZE == ID->Size()); if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID->Ident, SIZE); };
		void Set(const Uint8 *ID = NULL) { if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID, SIZE); };
		const Uint8 *GetValue(void) const { return Ident; };
		int Size(void) const { return SIZE; };
		
		bool operator<(const Identifier& Other) const
		{
			if(Other.Size() < SIZE ) return (memcmp(Ident, Other.Ident, Other.Size()) < 0);
			                    else return (memcmp(Ident, Other.Ident, SIZE) < 0);
		}

		std::string GetString(void) const
		{
			std::string Ret;
			char Buffer[8];

			for(int i=0; i<SIZE; i++)
			{
				sprintf(Buffer, "%02x", Ident[i]);
				if(i!=0) Ret += " ";

				Ret += Buffer;
			}

			return Ret;
		}
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
	typedef Identifier<16> UUID;
	typedef Vector<UL> ULVector;

	//! A smart pointer to a UL object
	typedef SmartPtr<UL> ULPtr;

	//! A smart pointer to a UUID object
	typedef SmartPtr<UL> UUIDPtr;
}


#endif MXFLIB__TYPES_H
