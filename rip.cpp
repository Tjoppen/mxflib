/*! \file	rip.cpp
 *	\brief	Implementation of RIP class
 *
 *			The RIP class holds Random Index Pack data, either loaded from a real
 *			Random Index Pack in the file or built by discovering partitions.
 */

#include "rip.h"


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
}


//! PartitionInfo constructor
mxflib::PartitionInfo::PartitionInfo(void *Part			/* = NULL */,
									 Position Offset	/* = -1 */, 
									 Uint32 SID			/* = 0 */)
	: Partition(Part), ByteOffset(Offset), BodySID(SID)
{
}




