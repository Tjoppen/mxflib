/*! \file	MXFLib.h
 *	\brief	The main MXFLib header file
 *
 *			This file contains ...
 */

#ifndef MXFLIB__MXFLIB_H
#define MXFLIB__MXFLIB_H

/* MXF Type Definitions */

namespace mxflib
{
	typedef __int64 Length;
	typedef __int64 Position;

	typedef unsigned int Uint32;
	typedef unsigned short int Uint16;
	typedef unsigned char Uint8;

}

//! Draft version of Identifier base type
namespace mxflib
{
	template <int size> class Identifier
	{
	private:
		Uint8 Ident[size];
	public:
		Identifier(Uint8 *ID = NULL);
	};
}

#include <list>
//! Draft version of Vector base type
namespace mxflib
{
	template <class T> class Vector : private std::list<T>
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

//! Draft Variable Type Definition (VTypeDef) class
namespace mxflib
{
	class VTypeDef
	{
	private:
		char *TypeName;
		int Size;
		bool isVector;
	};
}

#endif MXFLIB__MXFLIB_H