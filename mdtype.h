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
#ifndef MXFLIB__MDTYPE_H
#define MXFLIB__MDTYPE_H

#include "mxflib.h"

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

