/*! \file	rip.cpp
 *	\brief	Implementation of RIP class
 *
 *			The RIP class holds Random Index Pack data, either loaded from a real
 *			Random Index Pack in the file or built by discovering partitions.
 *
 *	\version $Id: rip.cpp,v 1.4 2007/01/19 14:39:16 matt-beard Exp $
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

using namespace mxflib;


//! Random Index Pack constructor
/*! Sets the pack as generated (as it obviously hasn't been read yet)
*/
RIP::RIP()
{
	isGenerated = true;
}


//! Random Index Pack destructor
RIP::~RIP()
{
	//debug("~RIP()\n");
}


//! Add a partition to a RIP
PartitionInfoPtr RIP::AddPartition(PartitionPtr Part,
						  Position Offset			/* = -1 */, 
						  UInt32 SID				/* = 0 */)
{
	debug("Adding a partition to a RIP\n");

	PartitionInfoPtr NewPI = new PartitionInfo(Part, Offset, SID);

	// Erase any existing partition at this place
	erase(Offset);

	// Add the new partition
	insert(PartitionInfoMap::value_type(Offset, NewPI));

	// Return the new item
	return NewPI;
}


//! PartitionInfo constructor
PartitionInfo::PartitionInfo(PartitionPtr Part	/* = NULL */,
									 Position Offset			/* = -1 */, 
									 UInt32 SID					/* = 0 */)
	: ThePartition(Part), ByteOffset(Offset), BodySID(SID),
	  IndexSID(0), KnownSIDs(false), StreamOffset(-1), StreamByteCount(-1), EstStreamOffset(-1), EstStreamByteCount(-1), EssenceStart(-1)
{
	debug("Generating new PartitionInfo\n");
}


//! Locate the previous partition to a given location
/*! Finds the nearest partition that is located before a given location
 *  \return Smart pointer to the PartitionInfo block, or NULL if no entries exist before the specified position
 */
PartitionInfoPtr RIP::FindPreviousPartition(Position Pos)
{
	// Nothing to return if the RIP is empty
	if(empty()) return NULL;

	// Find the current partition at this location, or the nearest after it
	RIP::iterator it = lower_bound(Pos);

	// If the first entry was returned then there is no preceeding entry - again we return NULL
	if(it == begin()) return NULL;

	// Index the previous entry
	it--;

	return (*it).second;
}


//! Locate the next partition to a given location
/*! Finds the nearest partition that is located after a given location
 *  \return Smart pointer to the PartitionInfo block, or NULL if no entries exist after the specified position
 */
PartitionInfoPtr RIP::FindNextPartition(Position Pos)
{
	// Nothing to return if the RIP is empty
	if(empty()) return NULL;

	// Find the first partition after this location
	RIP::iterator it = lower_bound(Pos + 1);

	// If no entry returned then there is no known partition after this position - again we return NULL
	if(it == end()) return NULL;

	return (*it).second;
}

//! Locate the partition <b>most likely</b> to contain the given stream offset for a SID
/*! This may or may not be the correct partition depending on what values are know about partitions,
 *  so it is up to the caller to verify this.
 *  \note If the RIP is incomplete you will get strange results
 *  \return A pointer to the PartitionInfo block, or NULL if there was a problem
 */
PartitionInfoPtr RIP::FindPartition(UInt32 SID, Position StreamOffset)
{
	// *****************************************************
	// DRAGONS: Simple algorithm, does not use any indexing
	// *****************************************************

	bool PositionAccurate = true;				//!< Cleared once we have made a guestimate of StreamPosition
	Position StreamPosition = 0;				//!< The real, or guestimated, stream offset of the current partition

	// Get the first partition
	RIP::iterator it = begin();

	// Details of the previous partition for this stream
	RIP::iterator PrevPartition = end();		// The previous partition for this SID, or end() if we haven't found one
	Position PrevStart = -1;					// The byte offset within the file of the previous partition, or -1 if not known
	Position PrevEnd = -1;						// The byte offset within the file of the start of the following partition (of any stream), or -1 if not known

	// The estimated size of the start of a partition (the partition pack and any other bits)
	// We will update this as we learn more about the file
	Length PartitionEstimate = 0;

	// The KAG used for this stream - if known
	Length StreamKAG = 0;

	while(it != end())
	{
		// If this is the first partition following "PrevPartition" record this position as the end of the previous partition
		if((PrevEnd == -1) && (PrevStart != -1)) PrevEnd = (*it).second->ByteOffset;

		if((*it).second->BodySID == 0)
		{
			// Should we read the partition to check?
		}

		// This partition is "one of ours"
		if((*it).second->BodySID == SID)
		{
			// We know the stream offset for this partition
			if((*it).second->GetStreamOffset() != -1)
			{
				// Update the stream position with the known value
				StreamPosition = (*it).second->GetStreamOffset();

				// If we overshoot we must retrun the previous partition - this is done by exiting the loop
				if(StreamPosition < StreamOffset) break;

				// If we have not over-shot the desired position, we have now updated our stream position with an accurate value
				PositionAccurate = true;
			}
			else
			{
				/* If we have a copy of the partition pack we can read the value from there (and other items used for later estimates) */
				if((*it).second->ThePartition)
				{
					/* We have a copy of the partition pack - this is going well */

					// Read the KAG size for this stream (should be constant through the stream)
					StreamKAG = (*it).second->ThePartition->GetInt64("KAGSize");

					// Estimate the size of the header metadata and index table segments
					PartitionEstimate = (*it).second->ThePartition->GetInt64("HeaderByteCount");
					PartitionEstimate += (*it).second->ThePartition->GetInt64("IndexByteCount");

					// Add in the size of this partition pack
					DataChunkPtr Pack = (*it).second->ThePartition->WriteObject();
					PartitionEstimate += Pack->Size;

					// Read the actual offset
					StreamPosition = (*it).second->ThePartition->GetInt64("BodyOffset");
					(*it).second->SetEstimatedStreamOffset(StreamPosition);
					PositionAccurate = true;
				}
				else
				{
					/* We must estimate a value */

					// If the last position was unknown then we must assume that this is the first partition for this stream
					if(PrevPartition == end())
					{
						// This still counts as an estimate!
						StreamPosition = 0;
						PositionAccurate = false;
					}
					else
					{
						if((PrevStart == -1) || (PrevEnd == -1))
						{
							// We have a problem as there was a previous partition for this stream, but it was not complete!
							// All we can really do is flag that we have an estimated value!
							PositionAccurate = false;
						}
						else
						{
							/* We know the start and end of the previous partition, so start estimating */

							// Use the previously calculated estimate (if we have one)
							if((*it).second->GetEstimatedStreamOffset())
							{
								StreamPosition = (*it).second->GetEstimatedStreamOffset();
								PositionAccurate = false;

								// If we overshoot we must retrun the previous partition - this is done by exiting the loop
								if(StreamPosition < StreamOffset) break;
							}
							else
							{
								// Start working out where stream data starts
								Position DataOffset = PartitionEstimate;

								// Round to the next KAG
								if(StreamKAG > 1)
								{
									// Calculate the number of bytes we are from the current KAG
									Position Align = DataOffset % StreamKAG;
									
									// Only align if not already aligned
									if(Align != 0)
									{
										// Work out the number of bytes to the next KAG
										Align = StreamKAG - Align;

										// We need to add extra if we can't fit a filler in - for small KAGs we may need to add several
										while(Align < 17) Align += StreamKAG;
								
										// Add the filler
										DataOffset += Align;
									}
								}

								// Add in the size of essence in the previous partition
								StreamPosition += (PrevEnd - (PrevStart + DataOffset));
								PositionAccurate = false;

								// Set this new estimated position (for later use)
								(*it).second->SetEstimatedStreamOffset(StreamPosition);
							}

							// If we overshoot we must retrun the previous partition - this is done by exiting the loop
							if(StreamPosition < StreamOffset) break;

							/* We should assume that only the header has header metadata, unless we have found some elsewhere,
							* so if we calculated PartitionEstimate from the header, remove the header byte count */
							if(((*it).first == 0) && ((*it).second->ThePartition))
							{
								PartitionEstimate -= (*PrevPartition).second->ThePartition->GetInt64("IndexByteCount");
							}
						} // End know size of previous partition
					} // End not the first partition of this stream
				} // End estimation block
			}

			// Record this as the previous one of our partitions
			PrevStart = (*it).second->ByteOffset;
			PrevPartition = it;
		}
	
		// Move on to the next partition
		it++;
	}

	// Return the last partition found for this stream, this will be the "previous partition" if we over-shot,
	// or the last partition if this stream if we ran off the end of the RIP

	// If we didn't find any partitions for this stream, return an error status
	if(PrevPartition == end()) return NULL;

	// Return a pointer to the PartitionInfo structure
	return (*PrevPartition).second;
}

