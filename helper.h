/*! \file	helper.h
 *	\brief	Verious helper function declarations
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

#ifndef MXFLIB__HELPER_H
#define MXFLIB__HELPER_H

#include <string>

namespace mxflib
{
	//! Make a string containing a number
	inline std::string Int2String(int Num, int Digits = 0)
	{
		char Buffer[18];
		if(Digits > 16) Digits = 16;
		sprintf(Buffer, "%0*d", Digits, Num);
		return std::string(Buffer);
	}

	//! Make a string containing an unsigned number
	inline std::string Uint2String(int Num, int Digits = 0)
	{
		char Buffer[18];
		if(Digits > 16) Digits = 16;
		sprintf(Buffer, "%0*u", Digits, Num);
		return std::string(Buffer);
	}

	//! Make a hex string containing a number
	inline std::string Int2HexString(int Num, int Digits = 0)
	{
		char Buffer[18];
		if(Digits > 16) Digits = 16;
		sprintf(Buffer, "%0*x", Digits, Num);
		return std::string(Buffer);
	}
}

#endif MXFLIB__HELPER_H
