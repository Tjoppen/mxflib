/*! \file	deftypes.h
 *	\brief	Header file for deftypes.cpp which loads type definitions
 *
 *	\version $Id: deftypes.h,v 1.1 2004/04/26 18:27:47 asuraparaju Exp $
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
#ifndef MXFLIB__DEFTYPES_H
#define MXFLIB__DEFTYPES_H

#include <mxflib/mdtraits.h>
#include <mxflib/mdtype.h>

// STL Includes
#include <string>
#include <list>
#include <map>

namespace mxflib
{
	//! Load types from the specified XML definitions
	int LoadTypes(char *TypesFile);
}
#endif // MXFLIB__DEFTYPES_H
