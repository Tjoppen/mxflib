/*! \file	rip.h
 *	\brief	Definition of RIP class
 *
 *			The RIP class holds Random Index Pack data, either loaded from a real
 *			Random Index Pack in the file or built by discovering partitions.
 *
 *	\version $Id: rip.h,v 1.11 2003/12/18 17:51:56 matt-beard Exp $
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
#ifndef MXFLIB__RIP_H
#define MXFLIB__RIP_H

#include <mxflib/partition.h>


#include <list>

namespace mxflib
{
	//! Holds RIP data relating to a single partition
	class PartitionInfo
	{
	public:
		PartitionPtr ThePartition;	//!< The actual partition

		Position ByteOffset;		//!< Byte offset into the file for the start of this partition
									/*!< \note
									 *	 Version 11 of the MXF spec uses a Uint64 for this
									 *         field but we are using a Position type here as it
									 *         makes more sense, and allows distingushed value -1
									 *   
									 *	 \note 
									 *	 Distinguished value -1 is used where the location
									 *         in the file is not known
									 */

		Uint32 BodySID;				//!< Stream ID of any essence in this partition (0 if none)
									/*!< \note 0 is also used if the existance of essence is
									 *         has not yet been determined
									 */

	public:
		PartitionInfo(PartitionPtr Part = NULL, Position Offset = -1, Uint32 SID = 0);

		//! Comparison function to allow sorting
		bool operator< (PartitionInfo &Other) {	return (ByteOffset < Other.ByteOffset); }
	};

	//! A smart pointer to a PartitionInfo object
	typedef SmartPtr<PartitionInfo> PartitionInfoPtr;

	//! A map of file location against smart pointers to PartitionInfo objects
	typedef std::map<Int64, PartitionInfoPtr> PartitionInfoMap;
}


namespace mxflib
{
	//! Random Index Pack class
	/*! Holds Random Index Pack data, either loaded from a real
	 *	Random Index Pack in the file or built by discovering partitions.
	 */
	class RIP : public PartitionInfoMap
	{
	public:
		// DRAGONS: This should probably mutate into a "MatchedPysical" property
		bool isGenerated;				//!< If not generated then the RIP has been read from a file
	public:
		RIP();
		~RIP();

		void AddPartition(PartitionPtr Part, Position Offset = -1, Uint32 SID = 0);
	};
}


#endif // MXFLIB__RIP_H
