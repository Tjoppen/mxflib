/*! \file	types.h
 *	\brief	The main MXF data types
 *
 *	\version $Id: types.h,v 1.15 2004/01/06 14:18:55 terabrit Exp $
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

	typedef std::pair<Uint32, Uint32> U32Pair;
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
	template <int SIZE> class Identifier : public RefCount<Identifier<SIZE> >
	{
	protected:
		Uint8 Ident[SIZE];
	public:
		Identifier(const Uint8 *ID = NULL) { if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID, SIZE); };
		Identifier(const SmartPtr<Identifier> ID) { ASSERT(SIZE == ID->Size()); if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID->Ident, SIZE); };
		void Set(const Uint8 *ID = NULL) { if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID, SIZE); };
		const Uint8 *GetValue(void) const { return Ident; };
		int Size(void) const { return SIZE; };

		bool operator!(void) const
		{
			int i;
			for(i=0; i<SIZE; i++) if(Ident[i]) return false;
			return true;
		}

		bool operator<(const Identifier& Other) const
		{
			if(Other.Size() < SIZE ) return (memcmp(Ident, Other.Ident, Other.Size()) < 0);
			                    else return (memcmp(Ident, Other.Ident, SIZE) < 0);
		}

		bool operator==(const Identifier& Other) const
		{
			if(Other.Size() != SIZE ) return false;
								 else return (memcmp(Ident, Other.Ident, SIZE) == 0);
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


/*namespace mxflib
{
	//! Draft version of Vector base type (DRAGONS)
	template <class T> class Vector : public std::list<T>
	{
	private:
	public:
	};
}
*/

namespace mxflib
{
	typedef Identifier<16> UL;
//	typedef Vector<UL> ULVector;

	//! A smart pointer to a UL object
	typedef SmartPtr<UL> ULPtr;
	typedef std::list<ULPtr> ULList;
}

namespace mxflib
{
	typedef Identifier<16> Identifier16;
	class UUID : public Identifier16
	{
	public:
		UUID() { MakeUUID(Ident); };
		UUID(const Uint8 *ID) : Identifier16(ID) {};
		UUID(const SmartPtr<UUID> ID) { if(ID == NULL) memset(Ident,0,16); else memcpy(Ident,ID->Ident, 16); };
	};

	//! A smart pointer to a UUID object
	typedef SmartPtr<UUID> UUIDPtr;
}


namespace mxflib
{
	typedef Identifier<32> Identifier32;
	class UMID : public Identifier32
	{
	public:
		UMID(const Uint8 *ID = NULL) : Identifier32(ID) {};
		UMID(const SmartPtr<UMID> ID) { if(ID == NULL) memset(Ident,0,32); else memcpy(Ident,ID->Ident, 32); };

		//! Get the UMID's instance number
		/*! \note The number returned interprets the instance number as big-endian */
		Uint32 GetInstance(void)
		{
			return (Ident[13] << 16) | (Ident[14] << 8) | Ident[15];
		}

		//! Set the UMID's instance number
		/*! \note The number is set as big-endian */
		//	DRAGONS: Should add an option to generate a random instance number
		void SetInstance(int Instance, int Method = -1)
		{
			Uint8 Buffer[4];
			PutU32(Instance, Buffer);

			// Set the instance number
			memcpy(&Ident[13], &Buffer[1], 3);

			// Set the method if a new one is specified
			if(Method >= 0)
			{
				Ident[11] = (Ident[11] & 0xf0) | Method;
			}
		}

		//! Set the UMID's material number
		void SetMaterial( ULPtr aUL )
		{
			// Set the instance number
			memcpy(&Ident[16], aUL->GetValue(), 16);

			// DRAGONS: Is this the right method for a UL?
			Ident[11] = (Ident[11] & 0x0f) | 2<<4;
		}
	};

	//! A smart pointer to a UMID object
	typedef SmartPtr<UMID> UMIDPtr;
}


namespace mxflib
{
	//! Structure for holding fractions
	struct Rational
	{
		Int32 Numerator;
		Int32 Denominator;
	};
}

#endif // MXFLIB__TYPES_H

