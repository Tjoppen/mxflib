/*! \file	metadict.cpp
 *	\brief	Basic MXF metadictionary functions
 *
 *	\version $Id: metadict.h,v 1.1 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2010, Metaglue Corporation
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


#ifndef MXFLIB__METADICT_H
#define MXFLIB__METADICT_H

#include "mxflib/mxflib.h"
//using namespace mxflib;

namespace mxflib
{

	//! Load classes and types from a Metadictionary object
	/*! At the point where this function is called, you need to have all the component parts loaded and
	 *  all the strong references within the metadictionary need to be satisfied
	 */
	bool LoadMetadictionary(MDObjectPtr &Meta, SymbolSpacePtr &SymSpace);

	//! Build a metadictionary from current classes and types used in a given list of metadata trees
	/*! If Meta is supplied, then all classes in the trees strongly linked from it are written to the metadictionary, 
	 *  including all properties of those classes (whether used or not) and any types used by those properties
	 */
	MDObjectPtr BuildMetadictionary(MDObjectList &MetaList, Primer *UsePrimer);
}

#endif // MXFLIB__METADICT_H
