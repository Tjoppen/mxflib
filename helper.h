/*! \file	helper.h
 *	\brief	Verious helper function declarations
 */

#ifndef MXFLIB__HELPER_H
#define MXFLIB__HELPER_H

#include <string>

namespace mxflib
{
	//! Make a string containing a number
	inline std::string Int2String(int Num)
	{
		char Buffer[16];
		sprintf(Buffer, "%d", Num);
		return std::string(Buffer);
	}
}

#endif MXFLIB__HELPER_H
