/*! \file	klvobject.cpp
 *	\brief	Implementation of classes that define basic KLV objects
 *
 *			Class KLVObject holds info about a KLV object
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

#include "mxflib.h"

using namespace mxflib;

//! Build a new KLVObject
KLVObject::KLVObject(ULPtr ObjectUL)
{
	TheUL = ObjectUL;
	
	Init();
}


//! Initialise newly built KLVObject
void KLVObject::Init(void)
{
	IsConstructed = true;
	SourceOffset = 0;
	KLSize = 0;
	SourceFile = NULL;

	ObjectName = "";
}


//! Get text that describes where this item came from
std::string KLVObject::GetSource(void) 
{ 
	if(SourceFile) return SourceFile->Name; else return "memory buffer"; 
}
