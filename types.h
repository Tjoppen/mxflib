/*! \file	types.h
 *	\brief	The main MXF data types
 */

#ifndef MXFLIB__TYPES_H
#define MXFLIB__TYPES_H


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
	template <int size> class Identifier
	{
	private:
		Uint8 Ident[size];
	public:
		Identifier(Uint8 *ID = NULL);
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
