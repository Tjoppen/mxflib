/*! \file	mdtype.h
 *	\brief	Definition of classes that define metadata type info
 *
 *			Class MDDict holds the overall dictionary definitions and
 *          manages loading them from a dictionary file and adding new
 *			metadata types.
 *<br><br>
 *			Class MDType holds info about a specific metadata type
 *<br><br>
 *			These classes are currently wrappers around KLVLib structures
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
#ifndef MXFLIB__MDTYPE_H
#define MXFLIB__MDTYPE_H

// Include the KLVLib header
extern "C"
{
#include "KLV.h"						//!< The KLVLib header
}


namespace mxflib
{
	//! Holds the definition of a metadata type
	class MDType
	{
	private:
		DictEntry *Dict;				//!< The KLVLib dictionary entry
	public:
	};
}

#include <list>
namespace mxflib
{
	//! A list of pointers to MDType objects
	typedef std::list<MDType *> MDTypeList;
}

namespace mxflib
{
	//! Holds metadata dictionary definitions and manages the dictionary
	class MDDict
	{
	private:
		DictEntry	*MainDict;			//!< The KLVLib dictionary entry of the root entry
		MDTypeList	Types;				//!< The top-level types managed by this object
	public:
		MDDict(const char *DictFile);
		~MDDict();
	};
}



#endif MXFLIB__MDTYPE_H

