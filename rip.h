/*! \file	Rip.h
 *	\brief	Definition of RIP class
 *
 *			The RIP class holds Random Index Pack data, either loaded from a real
 *			Random Index Pack in the file or built by discovering partitions.
 */
#ifndef MXFLIB__RIP_H
#define MXFLIB__RIP_H

#include "mxflib.h"
#include "partition.h"

#include <list>

namespace mxflib
{
	//! Holds RIP data relating to a single partition - DRAGONS: Currently dummy data
	class PartitionInfo
	{
	public:
		void *Partition;		//!< The actual partition - DRAGONS: Currently void pointer!!
		
		Position ByteOffset;	//!< Byte offset into the file for the start of this partition
								/*!< Note: Version 11 of the MXF spec uses a Uint64 for this
								 *         field but we are using a Position type here as it
								 *         makes more sense, and allows distingushed value -1
								 *   Note: Distinguished value -1 is used where the location
								 *         in the file is not known
								 */
		
		Uint32 BodySID;			//!< Stream ID of any essence in this partition (0 if none)
								/*!< Note: 0 is also used if the existance of essence is
								 *         has not yet been determined
								 */

	public:
		PartitionInfo(void *Part = NULL, Position Offset = -1, Uint32 SID = 0);

		bool operator< (PartitionInfo &Other);
								//!< Comparison function to allow sorting
	};

	//! A list of PartitionInfo items
	typedef std::list<PartitionInfo> PartitionList;
}


namespace mxflib
{
	//! Random Index Pack class
	/*! Holds Random Index Pack data, either loaded from a real
	 *	Random Index Pack in the file or built by discovering partitions.
	 */
	class RIP
	{
	private:
		bool			isGenerated;		//!< If not generated then the RIP has been read from a file
		PartitionList	Partitions;			//!< Details of each partition known about
	public:
		RIP();
		~RIP();

		AddPartition(void *Part = NULL, Position Offset = -1, Uint32 SID = 0);
	};
}


#endif MXFLIB__RIP_H

