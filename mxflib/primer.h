/*! \file	primer.h
 *	\brief	Definition of Primer class
 *
 *			The Primer class holds data about the mapping between local
 *          tags in a partition and the UL that gives access to the full
 *			definition
 *
 *	\version $Id: primer.h,v 1.5 2011/01/10 10:42:09 matt-beard Exp $
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
#ifndef MXFLIB__PRIMER_H
#define MXFLIB__PRIMER_H


#include <map>

namespace mxflib
{
	// Forward declare so the class can be used
	class Primer;

	//! A smart pointer to an Primer
	typedef SmartPtr<Primer> PrimerPtr;

	//! A list of smart pointers to Primer objects
	typedef std::list<PrimerPtr> PrimerList;
}


namespace mxflib
{
	// Required to make MSVC compile!
	typedef std::map<Tag, UL> Primer_Root;

	//! Holds local tag to metadata definition UL mapping
	class Primer : public Primer_Root, public RefCount<Primer>
	{
	protected:
		Tag NextDynamic;						//! Next dynamic tag to try
		std::map<UL, Tag> TagLookup;			//! Reverse lookup for locating a tag for a given UL

	public:
		Primer() { NextDynamic = 0xffff; };
		UInt32 ReadValue(const UInt8 *Buffer, UInt32 Size);

		//! Write this primer to a memory buffer
		UInt32 WritePrimer(DataChunkPtr &Buffer);

		//! Determine the tag to use for a given UL
		Tag Lookup(ULPtr ItemUL, Tag TryTag = 0);

		//! Determine the tag to use for a given UL - when no primer is availabe
		static Tag StaticLookup(ULPtr ItemUL, Tag TryTag = 0);

		//! Insert a new child type
		std::pair<iterator, bool> insert(value_type Val) 
		{ 
			TagLookup.insert(std::map<UL, Tag>::value_type(Val.second, Val.first));
			return Primer_Root::insert(Val);
		}
	};
}

#endif // MXFLIB__PRIMER_H
