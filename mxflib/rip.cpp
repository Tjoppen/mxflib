/*! \file	rip.cpp
 *	\brief	Implementation of RIP class
 *
 *			The RIP class holds Random Index Pack data, either loaded from a real
 *			Random Index Pack in the file or built by discovering partitions.
 *
 *	\version $Id: rip.cpp,v 1.1 2004/04/26 18:27:48 asuraparaju Exp $
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

#include <mxflib/mxflib.h>


//! Random Index Pack constructor
/*! Sets the pack as generated (as it obviously hasn't been read yet)
*/
mxflib::RIP::RIP()
{
	isGenerated = true;
}


//! Random Index Pack destructor
/*! <this space for rent>
*/
mxflib::RIP::~RIP()
{
	debug("~RIP()\n");
}


//! Add a partition to a RIP
void mxflib::RIP::AddPartition(PartitionPtr Part,
						  Position Offset			/* = -1 */, 
						  Uint32 SID				/* = 0 */)
{
	debug("Adding a partition to a RIP\n");

	PartitionInfoPtr NewPI = new PartitionInfo(Part, Offset, SID);

	// Erase any existing partition at this place
	erase(Offset);

	// Add the new partition
	insert(PartitionInfoMap::value_type(Offset, NewPI));
}


//! PartitionInfo constructor
mxflib::PartitionInfo::PartitionInfo(PartitionPtr Part	/* = NULL */,
									 Position Offset			/* = -1 */, 
									 Uint32 SID					/* = 0 */)
	: ThePartition(Part), ByteOffset(Offset), BodySID(SID)
{
	debug("Generating new PartitionInfo\n");
}



