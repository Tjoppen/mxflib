/*! \file	primer.h
 *	\brief	Definition of Primer class
 *
 *			The Primer class holds data about the mapping between local
 *          tags in a partition and the UL that gives access to the full
 *			definition
 */
#ifndef MXFLIB__PRIMER_H
#define MXFLIB__PRIMER_H

#include "mxflib.h"

#include <map>

namespace mxflib
{
	//! Holds local tag to metadata definition UL mapping
	class Primer : public std::map<Tag, UL>
	{
	public:
	};
}


#endif MXFLIB__PRIMER_H

