/*! \file	rip.h
 *	\brief	Definition of RIP class
 *
 *			The RIP class holds Random Index Pack data, either loaded from a real
 *			Random Index Pack in the file or built by discovering partitions.
 *
 *	\version $Id: rip.h,v 1.4 2007/01/19 14:39:16 matt-beard Exp $
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
	class PartitionInfo : public RefCount<PartitionInfo>
	{
	public:
		PartitionPtr ThePartition;	//!< The actual partition
									/*!< \note This is public for compatibility only <b>** use accessors **</b>
									 */

		Position ByteOffset;		//!< Byte offset into the file for the start of this partition
									/*!< \note This is public for compatibility only <b>** use accessors **</b>
									 *
									 *   \note
									 *	 Version 11 of the MXF spec uses a UInt64 for this
									 *         field but we are using a Position type here as it
									 *         makes more sense, and allows distingushed value -1
									 *   
									 *	 \note 
									 *	 Distinguished value -1 is used where the location
									 *         in the file is not known
									 */

		UInt32 BodySID;				//!< Stream ID of any essence in this partition (0 if none)
									/*!< \note This is public for compatibility only <b>** use accessors **</b>
									 *
									 *   \note 0 is also used if the existance of essence is
									 *         has not yet been determined
									 */

	protected:
		UInt32 IndexSID;			//!< Index SID of any index table in this partition (0 if none or not known)

		bool KnownSIDs;				//!< Set true once we know for sure what the SIDs are, including IndexSID
									/*!< This will be false when we have read a version 1 RIP as all that is known is the SID,
									 *   which could possibly be an index SID for index-only partitions, but will be true
									 *   once we have either parsed the partition pack itself, written one ourselves, or
									 *   read a version 2 RIP (complete with detailed partion layout)
									 */

		Position StreamOffset;		//!< Stream offset of the first byte of essence, or generic stream, data in the partition
									/*!< Set to -1 if not known
									 */
		Length StreamByteCount;		//!< Count of stream data bytes in the partition
									/*!< Set to -1 if not known
									 */

		Position EstStreamOffset;	//!< <b>Estimated</b> Stream offset of the first byte of essence, or generic stream, data in the partition
									/*!< Set to -1 if not known
									 */
		Length EstStreamByteCount;	//!< <b>Estimated</b> Count of stream data bytes in the partition
									/*!< Set to -1 if not known
									 */

		Position EssenceStart;		//!< Actual byte offset in the file where the essence starts for this partition, if known, else -1

	public:
		PartitionInfo(PartitionPtr Part = NULL, Position Offset = -1, UInt32 SID = 0);

		//! Comparison function to allow sorting
		bool operator< (PartitionInfo &Other) {	return (ByteOffset < Other.ByteOffset); }

		//! Get a pointer to the actual partition
		PartitionPtr &GetPartition(void) { return ThePartition; }

		//! Set the actual partition
		void SetPartition(PartitionPtr Val) { ThePartition = Val; }

		//! Get the BodySID
		UInt32 GetBodySID(void) const { return BodySID; }

		//! Set the BodySID
		void SetBodySID(UInt32 Val) { BodySID = Val; }
		
		//! Get the IndexSID
		UInt32 GetIndexSID(void) const { return IndexSID; }

		//! Set the IndexSID
		void SetIndexSID(UInt32 Val) { IndexSID = Val; }

		//! Get KnownSIDs flag
		bool SIDsKnown(void) const { return KnownSIDs; }

		//! Set KnownSIDs flag
		void SetKnownSIDs(bool Val) { KnownSIDs = Val; }

		//! Set BodySID and IndexSID, and set KnownSIDs to true
		void SetSIDs(UInt32 NewBodySID, UInt32 NewIndexSID)
		{
			BodySID = NewBodySID;
			IndexSID = NewIndexSID;
			KnownSIDs = true;
		}

		//! Get the byte offset of this partition pack in the file (if known), or -1 if not known
		Position GetByteOffset(void) const { return ByteOffset; }

		//! Set the byte offset of this partition pack in the file (if known), or -1 if not known
		void SetByteOffset(Position Val)  { ByteOffset = Val; }

		//! Get the stream offset of the first data byte in this partition (if known), or -1 if not known
		Position GetStreamOffset(void) const { return StreamOffset; }

		//! Set the stream offset of the first data byte in this partition (if known), or -1 if not known
		void SetStreamOffset(Position Val)  { StreamOffset = Val; }

		//! Get the <b>estimated</b> stream offset of the first data byte in this partition (if known), or -1 if no idea
		Position GetEstimatedStreamOffset(void) const { return EstStreamOffset; }

		//! Set the <b>estimated</b> stream offset of the first data byte in this partition (if known), or -1 if no idea
		void SetEstimatedStreamOffset(Position Val)  { EstStreamOffset = Val; }

		//! Get the essence start as a byte offset in the file (if known), or -1 if not known
		Position GetEssenceStart(void) const { return EssenceStart; }

		//! Set the essence start as a byte offset in the file (if known), or -1 if not known
		void SetEssenceStart(Position Val)  { EssenceStart = Val; }
	};

	//! A smart pointer to a PartitionInfo object
	typedef SmartPtr<PartitionInfo> PartitionInfoPtr;

	//! A map of file location against smart pointers to PartitionInfo objects
	typedef std::map<Position, PartitionInfoPtr> PartitionInfoMap;
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

		//! Add a partition at the specified location
		/*! \note Replaces any existing entry at the same location
		 */
		PartitionInfoPtr AddPartition(PartitionPtr Part, Position Offset = -1, UInt32 SID = 0);

		//! Locate the previous partition to a given location
		/*! Finds the nearest partition that is located before a given location
		 */
		PartitionInfoPtr FindPreviousPartition(Position Pos);

		//! Locate the next partition to a given location
		/*! Finds the nearest partition that is located after a given location
		 */
		PartitionInfoPtr FindNextPartition(Position Pos);

		//! Locate the partition <b>most likely</b> to contain the given stream offset for a SID
		/*! This may or may not be the correct partition depending on what values are know about partitions,
		 *  so it is up to the caller to verify this.
		 *  \note If the RIP is incomplete you will get strange results
		 */
		PartitionInfoPtr FindPartition(UInt32 SID, Position StreamOffset);
	};
}

#endif // MXFLIB__RIP_H
