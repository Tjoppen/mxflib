/*! \file	index.cpp
 *	\brief	Implementation of classes that handle index tables
 *
 *	\version $Id: index.cpp,v 1.15 2006/09/04 18:31:35 matt-beard Exp $
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


//! The lowest valid index position, used to flag omitted "start" parameters
/*! DRAGONS: Why isn't this initialized in the header file? Because MSVC 6 won't allow that!!
 */
const Position IndexTable::IndexLowest = (0 - UINT64_C(0x7fffffffffffffff));


//! Free memory by purging the specified range from the index
/*! DRAGONS: This function needs testing, also it could be improved to purge partial segments as well */
void IndexTable::Purge(UInt64 FirstPosition, UInt64 LastPosition)
{
	// Find the correct entry, or the nearest after it
	// DRAGONS: Is this inefficient?
	IndexSegmentMap::iterator it = SegmentMap.find(FirstPosition);
	if(it == SegmentMap.end()) { it = SegmentMap.lower_bound(FirstPosition); }

	// If the first position is after the end then do nothing
	if(it == SegmentMap.end()) return;

	// Erase all complete segments up to the last position
	while(it != SegmentMap.end())
	{
		if( (UInt64)((*it).first + (*it).second->EntryCount - 1) <= LastPosition)
			SegmentMap.erase(it++);
		else
			break;
	}
}


//! Get the segment containing a specified edit unit
/*! - If the edit unit exists within a segment that segment is returned
 *  - If the edit unit does not exist in a current edit unit, but it is the first edit unit
 *	  beyond the end of a segment then that segment is returned.
 *  - Otherwise a new segment is created starting with the specified edit unit and added to the index
 */
IndexSegmentPtr IndexTable::GetSegment(Position EditUnit)
{
	// Find the correct segment  - one starting with this edit unit, or the nearest before it
	IndexSegmentMap::iterator it = SegmentMap.find(EditUnit);
	if(it == SegmentMap.end()) 
	{ 
		it = SegmentMap.lower_bound(EditUnit); 
		if(it != SegmentMap.begin())
		{
			it--;
		}
		else
			// Flag that it is before the start
			it = SegmentMap.end();
	}

	// If this position is before the start of the index table we must add a new segment
	if((it == SegmentMap.end()) || ((*it).first > EditUnit))
	{
		return AddSegment(EditUnit);
	}

	// If this position is greater than the current free slot at the end of the segment we must add a new segment
	if(EditUnit > ((*it).second->StartPosition + (*it).second->EntryCount))
	{
		return AddSegment(EditUnit);
	}

	// This is the correct segment
	return (*it).second;
}


//! Add a single index entry creating segments as required
bool IndexTable::AddIndexEntry(Position EditUnit, Int8 TemporalOffset, Int8 KeyFrameOffset, UInt8 Flags, UInt64 StreamOffset, 
				   int SliceCount /*=0*/, UInt32 *SliceOffsets /*=NULL*/, int PosCount /*=0*/, Rational *PosTable /*=NULL*/)
{
	// Get the correct segment to use
	IndexSegmentPtr Segment = GetSegment(EditUnit);

	// If this position already exists in the segment we must replace it
	if(EditUnit < (Segment->StartPosition + Segment->EntryCount))
	{
//		@@Replace entry
		// DRAGONS: Need to add replace!
		error("Replacing index entries is not yet implemented\n");
	}

	// Add this entry to the end of the current segment
	if(Segment->AddIndexEntry(TemporalOffset, KeyFrameOffset, Flags, StreamOffset, SliceCount, SliceOffsets, PosCount, PosTable))
	{
		return true;
	}

	// Adding the entry didn't work - possibly because the segment is full
	// Try adding a new segment and adding the entry to it
	Segment = AddSegment(EditUnit);
	return Segment->AddIndexEntry(TemporalOffset, KeyFrameOffset, Flags, StreamOffset, SliceCount, SliceOffsets, PosCount, PosTable);
}


//! Perform an index table look-up
/*! Note that the return value is relative to the start of the EC in frame-wrapping,
 *  but relative to the start of the value of the first KLV in the first edit unit
 *  in the essence container in clip-wrapping
 */
IndexPosPtr IndexTable::Lookup(Position EditUnit, int SubItem /* =0 */, bool Reorder /* =true */)
{
	IndexPosPtr Ret = new IndexPos;

	// Deal with CBR first
	if(EditUnitByteCount)
	{
		// Start of edit unit
		Position Loc = EditUnit * (Position)EditUnitByteCount;

		if(SubItem == 0)
		{
			// If we are looking for the first sub-stream then all is fine
			Ret->Exact = true;
			Ret->OtherPos = false;
		}
		else
		{
			// Can't index a stream if we don't have a delta to it
			if(SubItem >= BaseDeltaCount)
			{
				Ret->Exact = false;
				Ret->OtherPos = false;
			}
			else
			{
				// Otherwise add the delta
				Ret->Exact = true;
				Ret->OtherPos = false;
				Loc += GetU32(BaseDeltaArray[SubItem].ElementDelta);
			}
		}

		Ret->ThisPos = EditUnit;
		Ret->Location = Loc;
		Ret->Offset = false;
		Ret->KeyFrameOffset = 0;
		Ret->KeyLocation = Ret->Location;
		Ret->Flags = 0;

		return Ret;
	}

	// Find the correct segment  - one starting with this edit unit, or the nearest before it
	IndexSegmentMap::iterator it = SegmentMap.find(EditUnit);
	if(it == SegmentMap.end()) 
	{ 
		if(!SegmentMap.empty()) 
		{ 
			it = SegmentMap.lower_bound(EditUnit); 
			if(it != SegmentMap.begin())
			{
				it--;
			}
			else
				// Flag that it is before the start
				it = SegmentMap.end();
		} 
	}

	// DRAGONS: will the above work as intended?

	// If this position is before the start of the index table, return the start of the essence
	if((it == SegmentMap.end()) || ((*it).first > EditUnit))
	{
		Ret->ThisPos = 0;
		Ret->Location = 0;
		Ret->Exact = false;
		Ret->Offset = false;
		Ret->KeyFrameOffset = 0;
		Ret->KeyLocation = 0;
		Ret->Flags = 0;

		return Ret;
	}

	// Build a segment pointer for ease of reading (very slight inefficiency)
	IndexSegmentPtr Segment = (*it).second;

	// Return start of file if we found a useless index entry (shouldn't happen!)
	if(Segment->EntryCount == 0)
	{
		error("IndexTableSegment contains no index entries!\n");

		Ret->ThisPos = 0;
		Ret->Location = 0;
		Ret->Exact = false;
		Ret->Offset = false;
		Ret->KeyFrameOffset = 0;
		Ret->KeyLocation = 0;
		Ret->Flags = 0;

		return Ret;
	}

	// If the nearest (or lower) index point is before this edit unit, set the result accordingly
	if((Segment->StartPosition + Segment->EntryCount - 1) < EditUnit)
	{
		Ret->ThisPos = Segment->StartPosition + Segment->EntryCount - 1;
		
		// Index the start of the index entry
		UInt8 *Ptr = &Segment->IndexEntryArray.Data[(Segment->EntryCount-1) * IndexEntrySize];

		// Skip the temporal and key-frame offsets and the flags as this is not an exact result
		Ptr += 3;

		// Read the location of the start of the edit unit
		Ret->Location = GetU64(Ptr);

		// Set non-exact values
		Ret->Exact = false;
		Ret->OtherPos = true;
		Ret->Offset = false;
		Ret->KeyFrameOffset = 0;
		Ret->KeyLocation = Ret->Location;
		Ret->Flags = 0;

		return Ret;
	}

	// Index the start of the correct index entry
	UInt8 *Ptr = &Segment->IndexEntryArray.Data[(EditUnit - Segment->StartPosition) * IndexEntrySize];

	// Read the temporal offset
	Int8 TemporalOffset = GetI8(Ptr);
	Ptr++;

	// Apply temporal re-ordering if we should, but only if we have details of the exact sub-item
	if(Reorder && (TemporalOffset != 0) && (SubItem < Segment->DeltaCount) && (Segment->DeltaArray[SubItem].PosTableIndex < 0))
	{
		return Lookup(EditUnit + TemporalOffset, SubItem, false);
	}

	// We are in the correct edit unit, so record the fact
	Ret->ThisPos = EditUnit;

	// Read the offset to the previous key-frame
	Ret->KeyFrameOffset = GetI8(Ptr);
	Ptr++;

	// Index the start of the keyframe index entry
	if( Ret->KeyFrameOffset > EditUnit - Segment->StartPosition )
	{
		// Key Frame is in a different Index Table Segment
		Ret->KeyLocation = ~0;
	}
	else
	{
		UInt8 *PKF = &Segment->IndexEntryArray.Data[(EditUnit - Segment->StartPosition - Ret->KeyFrameOffset) * IndexEntrySize];
		PKF += 3;
		Ret->KeyLocation = GetI64(PKF);
	}

	// Read the flags for this frame
	Ret->Flags = GetU8(Ptr);
	Ptr++;

	// Read the location of the start of the edit unit
	Ret->Location = GetU64(Ptr);
	Ptr += 8;

	// Note: At this point Ptr indexes the start of the SliceOffset array

	// If we don't have details of the exact sub-item return the start of the edit unit
	if(SubItem >= Segment->DeltaCount)
	{
		Ret->Exact = false;
		Ret->OtherPos = false;
		Ret->Offset = false;

		return Ret;
	}

	// We now have an exact match
	Ret->Exact = true;
	Ret->OtherPos = false;

	// Locate this sub-item in the edit unit
	if(SubItem > 0) 
	{
		// Locate the correct slice for this sub-item
		int Slice = Segment->DeltaArray[SubItem].Slice;
		if(Slice)
		{
			UInt8 *SlicePtr = Ptr + ((Slice - 1) * sizeof(UInt32));
			Ret->Location += GetU32(SlicePtr);
		}

		// Add the element delta
		Ret->Location += GetU32(Segment->DeltaArray[SubItem].ElementDelta);
	}

	// Sort the PosOffset if one is required
	int PosTableIndex = (int)Segment->DeltaArray[SubItem].PosTableIndex;
	if(PosTableIndex > 0)
	{
		// Index the correct PosTable entry for this sub-item
		UInt8 *PosPtr = Ptr + (NSL * sizeof(UInt32)) + ((PosTableIndex - 1) * (sizeof(UInt32)*2) );

		Ret->PosOffset.Numerator = GetI32(PosPtr);
		PosPtr += 4;
		Ret->PosOffset.Denominator = GetI32(PosPtr);
		Ret->Offset = true;
	}
	else
		Ret->Offset = false;

	return Ret;
}



//! Add an index table segment from an "IndexSegment" MDObject
/*! DRAGONS: Not the most efficient way to do this */
IndexSegmentPtr IndexTable::AddSegment(MDObjectPtr Segment)
{
	IndexSegmentPtr Ret;

	// DRAGONS: Must complete this!
//	warning("Index table reading not complete!\n");

	EditUnitByteCount = Segment->GetUInt(EditUnitByteCount_UL);

	// Set the index and body SIDs if not yet known
	// DRAGONS: Should we check that they match when loading later segments?
	if( IndexSID == 0 )
	{
		IndexSID = Segment->GetUInt(IndexSID_UL);
		BodySID = Segment->GetUInt(BodySID_UL);
	}

	if( EditUnitByteCount ) // CBR
	{
		MDObjectPtr	pEditRate = Segment[IndexEditRate_UL];
		if(pEditRate)
		{
			EditRate.Numerator = pEditRate->GetInt("Numerator");
			EditRate.Denominator = pEditRate->GetInt("Denominator");
		}

		MDObjectPtr Ptr = Segment[DeltaEntryArray_UL];
		if(Ptr)
		{
			// Free any old delta array
			if(BaseDeltaCount) delete[] BaseDeltaArray; 

			BaseDeltaCount = static_cast<int>(Ptr->size() / 3);		// << There are 3 items in each DeltaCount entry

			BaseDeltaArray = new DeltaEntry[BaseDeltaCount];

			int Delta = 0;
			MDObjectULList::iterator it = Ptr->begin();
			while(it != Ptr->end())
			{
				BaseDeltaArray[Delta].PosTableIndex = (*it).second->GetInt();
				if(++it == Ptr->end()) break;

				BaseDeltaArray[Delta].Slice = (*it).second->GetUInt();
				if(++it == Ptr->end()) break;

				PutU32((*it).second->GetUInt(), BaseDeltaArray[Delta].ElementDelta);
				it++;
				Delta++;
			}
			if(Delta != BaseDeltaCount)
			{
				error("Malformed DeltaEntryArray in %s at %s\n", Segment->FullName().c_str(), Segment->GetSourceLocation().c_str());
			}
		}
	}
	else // VBR
	{
		Int64 StartPosition = Segment->GetInt64(IndexStartPosition_UL);
		Ret = AddSegment(StartPosition);
		SegmentMap.insert(IndexSegmentMap::value_type(StartPosition, Ret));

		MDObjectPtr Ptr = Segment[DeltaEntryArray_UL];
		if(!Ptr)
		{
			Ret->DeltaCount = 0;
		}
		else
		{
			Ret->DeltaCount = static_cast<int>(Ptr->size() / 3);		// << There are 3 items in each DeltaCount entry

			Ret->DeltaArray = new DeltaEntry[Ret->DeltaCount];
			
			int Delta = 0;
			MDObjectULList::iterator it = Ptr->begin();
			while(it != Ptr->end())
			{
				Ret->DeltaArray[Delta].PosTableIndex = (*it).second->GetInt();
				if(++it == Ptr->end()) break;

				Ret->DeltaArray[Delta].Slice = (*it).second->GetUInt();
				if(++it == Ptr->end()) break;

				PutU32((*it).second->GetUInt(), Ret->DeltaArray[Delta].ElementDelta);
				it++;
				Delta++;
			}
			if(Delta != Ret->DeltaCount)
			{
				error("Malformed DeltaEntryArray in %s at %s\n", Segment->FullName().c_str(), Segment->GetSourceLocation().c_str());
			}
		}

		// Copy index entry bits...
		NSL = Segment->GetUInt(SliceCount_UL);
		NPE = Segment->GetUInt(PosTableCount_UL);
		// Calculate the size of each IndexEntry
		IndexEntrySize = (11 + 4*NSL + 8*NPE);

		// Copy the delta entries to the "base" is this is our first segment
		if((BaseDeltaCount == 0) && (Ret->DeltaCount != 0))
		{
			BaseDeltaCount = Ret->DeltaCount;
			BaseDeltaArray = new DeltaEntry[BaseDeltaCount];
			if(BaseDeltaCount) 
			{
				memcpy(BaseDeltaArray, Ret->DeltaArray, BaseDeltaCount * sizeof(DeltaEntry));
			}
		}
		else
		{
			// DRAGONS: We should validate this againts the current entries
			//#############################################################
		}

		Ptr = Segment[IndexEntryArray_UL];
		if(!Ptr)
		{
			Ret->EntryCount = 0;
			error("No IndexEntryArray in VBR index segment - is this supposed to be a CBR index?\n");
		}
		else
		{
			DataChunkPtr Entries = Ptr->WriteObject();

			if(Entries->Size >= 28)
			{
				UInt32 EntryCount = GetU32(&Entries->Data[20]);
				UInt32 EntrySize = GetU32(&Entries->Data[24]);

				if((Int32)EntrySize != IndexEntrySize)
				{
					error("IndexEntryArray items should be %d bytes, but are %d\n", IndexEntrySize, EntrySize);
				}
				else Ret->AddIndexEntries(EntryCount, IndexEntrySize, &Entries->Data[28]);
			}
		}
	} // CBR,VBR

	return Ret;
}

//! Create a new empty index table segment
IndexSegmentPtr IndexTable::AddSegment(Int64 StartPosition)
{
	IndexSegmentPtr Segment = IndexSegment::AddIndexSegmentToIndexTable(this, StartPosition);

	SegmentMap.insert(IndexSegmentMap::value_type(StartPosition, Segment));

	return Segment;
}


//! Add a single index entry
/*! \return true if the entry was added OK, false if an error occured or the segment would be too big (sizeof(IndexEntryArray) > 65535)
*/
bool IndexSegment::AddIndexEntry(Int8 TemporalOffset, Int8 KeyFrameOffset, UInt8 Flags, UInt64 StreamOffset, 
								 int SliceCount /*=0*/, UInt32 *SliceOffsets /*=NULL*/,
								 int PosCount /*=0*/, Rational *PosTable /*=NULL*/)
{
	ASSERT(Parent);

	if(SliceCount != Parent->NSL)
	{
		error("Current index table has NSL=%d, tried to add entry with NSL=%d\n", Parent->NSL, SliceCount);
		return false;
	}

	if(PosCount != Parent->NPE)
	{
		error("Current index table has NPE=%d, tried to add entry with NPE=%d\n", Parent->NPE, PosCount);
		return false;
	}

	// Calculate the new size to see if it is too big for a 2-byte local local set length
	int NewSize = ((EntryCount+1) * Parent->IndexEntrySize + 8);
	if(NewSize > 0xffff) return false;

	UInt8 *Buffer = new UInt8[Parent->IndexEntrySize];

	// Write the new entry
	Buffer[0] = (UInt8) TemporalOffset;
	Buffer[1] = (UInt8) KeyFrameOffset;
	Buffer[2] = Flags;
	PutU64(StreamOffset, &Buffer[3]);

	UInt8 *Ptr = &Buffer[11];
	UInt32 *SlicePtr = SliceOffsets;
	int i;
	for(i=0; i<SliceCount; i++)
	{
		PutU32(*SlicePtr, Ptr);
		SlicePtr++;
		Ptr += 4;
	}
	
	Rational *PosPtr = PosTable;
	for(i=0; i<PosCount; i++)
	{
		PutI32(PosPtr->Numerator, Ptr);
		PutI32(PosPtr->Denominator, Ptr);
		PosPtr++;
		Ptr += 8;
	}

	// Add this entry to the end of the Index Entry Array
	IndexEntryArray.Set(Parent->IndexEntrySize, Buffer, IndexEntryArray.Size);

	// Increment the count
	EntryCount++;

	// Free the buffer
	delete[] Buffer;

	return true;
}


//! Add multiple - pre-formed index entries
bool IndexSegment::AddIndexEntries(int Count, int Size, UInt8 *Entries)
{
	ASSERT(Parent);

	if(Size != (int)Parent->IndexEntrySize)
	{
		error("Current index table has entries of size %d, tried to add entries of size %d\n", Parent->IndexEntrySize, Size);
		return false;
	}

	// Calculate the new size to see if it is too big for a 2-byte local local set length
	int NewSize = (EntryCount * Parent->IndexEntrySize) + (Count * Size);
	if(NewSize > 0xffff) return false;

// diagnostics
#ifdef MXFLIB_DEBUG
	debug("\nAddIndexEntries() %d, %d:\n", Size, Count);
	UInt8 *p = (UInt8*)Entries;
	int i, j, k;
	for(i=0; i<Count && i<35; i++)
	{
		debug( " %3d: %2d %3d  0x%02x  0x", i, (int)(char)p[0], (int)(char)p[1], p[2] );

		for(j=3; j<11 && j<Size; j++) debug("%02x", p[j]);

		for(j=11; j<Size; j+=4)
		{
			debug(" 0x");
			for( k=0; k<4; k++) debug("%02x", p[j+k]);
		}

		p+=Size;
		debug("\n");
	}
#endif // MXFLIB_DEBUG

	IndexEntryArray.Set(Size * Count, Entries, IndexEntryArray.Size);

	// Increment the count
	EntryCount += Count;

	return true;
}


//! Index segment pseudo-constructor
/*! \note <b>Only</b> call this from IndexTable::AddSegment() because it adds the segment to its SegmentMap */
IndexSegmentPtr IndexSegment::AddIndexSegmentToIndexTable(IndexTablePtr ParentTable, Int64 IndexStartPosition)
{
	IndexSegmentPtr Segment = new IndexSegment();

	Segment->Parent = ParentTable;
	Segment->StartPosition = IndexStartPosition;
	Segment->DeltaCount = ParentTable->BaseDeltaCount;
	if(ParentTable->BaseDeltaCount) 
	{
		Segment->DeltaArray = new DeltaEntry[Segment->DeltaCount];
		memcpy(Segment->DeltaArray, ParentTable->BaseDeltaArray, ParentTable->BaseDeltaCount * sizeof(DeltaEntry));
	}

	return Segment;
}


//! Write this index table to a memory buffer
size_t IndexTable::WriteIndex(DataChunk &Buffer)
{
	// If we don't have a delta array, but we have more than 1 slice
	if((NSL != 0) && (BaseDeltaCount == 0))
	{
		error("IndexTable::WriteIndex() called with NSL = %d, but no delta array defined - Index table will be invalid\n", NSL);
	}

	// Force a bit of space into the buffer then clear the size
	// TODO: Use allocation granularity
	Buffer.ResizeBuffer(4096);
	Buffer.Resize(0);

	if( EditUnitByteCount ) // CBR Index Table
	{
		MDObjectPtr ThisSegment = new MDObject(IndexTableSegment_UL);
		if(!ThisSegment)
		{
			error("Couldn't build \"IndexTableSegment\" - index table not written\n");
			return Buffer.Size;
		}

		// Even though it isn't used IndexTableSegments need an InstanceUID
		// as it is derived from InterchangeObject (A minor bug in the spec)
		MDObjectPtr Instance = ThisSegment->AddChild(InstanceUID_UL);
		UUIDPtr ThisInstance = new UUID;
		if(Instance) Instance->ReadValue(DataChunk(16, ThisInstance->GetValue()));

		MDObjectPtr Ptr;
		Ptr = ThisSegment->AddChild(IndexEditRate_UL);
		if(Ptr)
		{
			Ptr->SetInt("Numerator", EditRate.Numerator);
			Ptr->SetInt("Denominator", EditRate.Denominator);
		}

		ThisSegment->SetInt64(IndexStartPosition_UL, 0);
		ThisSegment->SetInt64(IndexDuration_UL, 0);
		ThisSegment->SetUInt(EditUnitByteCount_UL, EditUnitByteCount);
		ThisSegment->SetUInt(IndexSID_UL, IndexSID);
		ThisSegment->SetUInt(BodySID_UL, BodySID);

		// Add a delta entry array if we have anything meaningful
		if((BaseDeltaCount > 1) && (BaseDeltaArray != NULL))
		{
			// DRAGONS: A bit clunky!
			// DRAGONS: What if on this platform sizeof(DeltaEntry) != 6 ?
			ASSERT(sizeof(DeltaEntry) == 6);
			UInt8 Buf[8];
			DataChunk Deltas;
			PutU32(BaseDeltaCount, Buf);
			PutU32(sizeof(DeltaEntry), &Buf[4]);
			Deltas.Set(8, Buf);
			Deltas.Set(BaseDeltaCount * sizeof(DeltaEntry), (UInt8*)BaseDeltaArray, 8);

			ThisSegment->SetValue(DeltaEntryArray_UL, Deltas);
		}
		else
		{
			UInt8 Buf[8];
			DataChunk Deltas;
			PutU32(0, Buf);
			PutU32(sizeof(DeltaEntry), &Buf[4]);
			Deltas.Set(8, Buf);
			ThisSegment->SetValue(DeltaEntryArray_UL, Deltas);
		}

		// Add this segment to the buffer
		{
			DataChunkPtr Seg = ThisSegment->WriteObject(MDOType::GetStaticPrimer());
			Buffer.Set(Seg->Size, Seg->Data, Buffer.Size);
		}
	}
	else // VBR Index Table
	{
		IndexSegmentMap::iterator it = SegmentMap.begin();
		while(it != SegmentMap.end())
		{
			IndexSegmentPtr Segment = (*it).second;

			MDObjectPtr ThisSegment = new MDObject(IndexTableSegment_UL);
			if(!ThisSegment)
			{
				error("Couldn't build \"IndexTableSegment\" - index table not written\n");
				return Buffer.Size;
			}

			// Even though it isn't used IndexTableSegments need an InstanceUID
			// as it is derived from InterchangeObject (A minor bug in the spec)
			MDObjectPtr Instance = ThisSegment->AddChild(InstanceUID_UL);
			UUIDPtr ThisInstance = new UUID;
			if(Instance) Instance->ReadValue(DataChunk(16, ThisInstance->GetValue()));

			MDObjectPtr Ptr;
			Ptr = ThisSegment->AddChild(IndexEditRate_UL);
			if(Ptr)
			{
				Ptr->SetInt("Numerator", EditRate.Numerator);
				Ptr->SetInt("Denominator", EditRate.Denominator);
			}

			ThisSegment->SetInt64(IndexStartPosition_UL, Segment->StartPosition);
			ThisSegment->SetInt64(IndexDuration_UL, Segment->EntryCount);
			ThisSegment->SetUInt(EditUnitByteCount_UL, EditUnitByteCount);
			ThisSegment->SetUInt(IndexSID_UL, IndexSID);
			ThisSegment->SetUInt(BodySID_UL, BodySID);

			// DRAGONS: This assumes constant NSL and NPE...
			ThisSegment->SetUInt(SliceCount_UL, NSL);
			ThisSegment->SetUInt(PosTableCount_UL, NPE);

			// DRAGONS: A bit clunky!
			// DRAGONS: What if on this platform sizeof(DeltaEntry) != 6 ?
			ASSERT(sizeof(DeltaEntry) == 6);
			DataChunk Deltas;
			UInt8 Buf[8];
			PutU32(BaseDeltaCount, Buf);
			PutU32(sizeof(DeltaEntry), &Buf[4]);
			Deltas.Set(8, Buf);
			Deltas.Set(BaseDeltaCount * sizeof(DeltaEntry), (UInt8*)Segment->DeltaArray, 8);

			ThisSegment->SetValue(DeltaEntryArray_UL, Deltas);

			// DRAGONS: A bit clunky!
			DataChunk Entries;
			PutU32(Segment->EntryCount, Buf);
			PutU32(IndexEntrySize, &Buf[4]);
			Entries.Set(8, Buf);
			Entries.Set(IndexEntrySize * Segment->EntryCount, Segment->IndexEntryArray.Data, 8);

			ThisSegment->SetValue(IndexEntryArray_UL, Entries);

			// Add this segment to the buffer
			{
				DataChunkPtr Seg = ThisSegment->WriteObject(MDOType::GetStaticPrimer());
				Buffer.Set(Seg->Size, Seg->Data, Buffer.Size);
			}

			it++;
		}
	} // VBR

	return Buffer.Size;
}


//! Fudge to correct index entry
void IndexTable::Correct(Position EditUnit, Int8 TemporalOffset, Int8 KeyFrameOffset, UInt8 Flags)
{
	// Find the correct segment  - one starting with this edit unit, or the nearest before it
	IndexSegmentMap::iterator it = SegmentMap.find(EditUnit);
	if(it == SegmentMap.end()) 
	{ 
		if(!SegmentMap.empty()) 
		{ 
			it = SegmentMap.lower_bound(EditUnit); 
			if(it != SegmentMap.begin())
			{
				it--;
			}
			else
				// Flag that it is before the start
				it = SegmentMap.end();
		} 
	}

	// If this position is before the start of the index table do nothing
	if((it == SegmentMap.end()) || ((*it).first > EditUnit)) return;

	// Build a segment pointer for ease of reading (very slight inefficiency)
	IndexSegmentPtr Segment = (*it).second;

	// Do nothing if we found a useless index entry (shouldn't happen!)
	if(Segment->EntryCount == 0) return;

	// If the nearest (or lower) index point is before this edit unit, do nothing
	if((Segment->StartPosition + Segment->EntryCount - 1) < EditUnit) return;

	// Index the start of the correct index entry
	UInt8 *Ptr = &Segment->IndexEntryArray.Data[(EditUnit - Segment->StartPosition) * IndexEntrySize];

	// Write the new temporal offset
	PutI8(TemporalOffset, Ptr);
	Ptr++;

	// Write the new offset to the previous key-frame
	PutI8(KeyFrameOffset, Ptr);
	Ptr++;

	// Write the new flags for this frame
	PutU8(Flags,Ptr);
	
	return;
}



//! Update the Stream Offset of an index entry
void IndexTable::Update(Position EditUnit, UInt64 StreamOffset)
{
	// Find the correct segment  - one starting with this edit unit, or the nearest before it
	IndexSegmentMap::iterator it = SegmentMap.find(EditUnit);
	if(it == SegmentMap.end()) 
	{ 
		if(!SegmentMap.empty()) 
		{ 
			it = SegmentMap.lower_bound(EditUnit); 
			if(it != SegmentMap.begin())
			{
				it--;
			}
			else
				// Flag that it is before the start
				it = SegmentMap.end();
		} 
	}

	// If this position is before the start of the index table do nothing
	if((it == SegmentMap.end()) || ((*it).first > EditUnit)) return;

	// Update the entry in this segment
	(*it).second->Update(EditUnit, StreamOffset);
}


//! Update the Stream Offset of an index entry
void IndexSegment::Update(Position EditUnit, UInt64 StreamOffset)
{
	// Ensure that this edit unit is within this segment
	if(EditUnit < StartPosition) return;

	// Ensure that this edit unit is within this segment
	if(EditUnit > (StartPosition + EntryCount - 1)) return;

	// Index the start of the correct index entry
	UInt8 *Ptr = &IndexEntryArray.Data[(EditUnit - StartPosition) * Parent->IndexEntrySize];

	// Skip over the temporal offset, the key frame offset and the flags
	Ptr += 3;

	// Write the stream offset for this frame
	PutU64(StreamOffset,Ptr);

	return;
}




//! Add a new entry to the table (setting flags and anchor offset)
bool ReorderIndex::SetEntry(Position Pos, UInt8 Flags, Int8 AnchorOffset, UInt8 *Tables /*=NULL*/ )
{
	// If this is the first entry we have added to the table set FirstPosition
	if(IndexEntries.Size == 0)
	{
		FirstPosition = Pos;
	}
	// Otherwise see if we are trying to add an entry before the start
	else if(Pos < FirstPosition)
	{
		// How far do we need to shift the data
		Int64 Shift = (FirstPosition - Pos) * IndexEntrySize;

		// Make enought room
		IndexEntries.Resize((UInt32)(IndexEntries.Size + Shift));

		// Shift the entries forwards
		memmove(&IndexEntries.Data[Shift], IndexEntries.Data, EntryCount * IndexEntrySize);

		// Adjust the counts
		if(CompleteEntryCount) CompleteEntryCount += (int)(FirstPosition - Pos);
		EntryCount += (int)(FirstPosition - Pos);
		
		// And the start position
		FirstPosition = Pos;
	}

	// Index this entry
	int Entry = int(Pos - FirstPosition);
	
	// Update the count
	if(Entry >= EntryCount)
	{
		IndexEntries.Resize((Entry+1) * IndexEntrySize);
		EntryCount = Entry + 1;
	}

	// And the complete count
	if(Entry >= CompleteEntryCount) CompleteEntryCount = Entry + 1;

	// Index the start of the entry
	UInt8 *EntryPtr = &IndexEntries.Data[Entry * IndexEntrySize];

	// Clear the temporal offset if it hasn't yet been set
	if(Entry >= EntryCount) *EntryPtr = 0;

	// Updata the data
	EntryPtr[1] = AnchorOffset;
	EntryPtr[2] = Flags;

	// Clear 8 bytes for the stream offset (should be efficient with most compilers)
	UInt8 *p = &EntryPtr[3];
	*(p++) = 0; *(p++) = 0; *(p++) = 0; *(p++) = 0;
	*(p++) = 0; *(p++) = 0; *(p++) = 0; *(p++) = 0;

	// Update the tables
	if((IndexEntrySize > 11) && Tables) memcpy(p, Tables, IndexEntrySize - 11);

	return true;
}


//! Add a new entry to the table
bool ReorderIndex::SetStreamOffset(Position Pos, Position StreamOffset)
{
	if(Pos < FirstPosition)
	{
		error("Tried to set the stream position of index entry for edit unit %s when the first entry in the ReorderIndex array is %s\n",
			  Int64toString(Pos).c_str(), Int64toString(FirstPosition).c_str());

		return false;
	}

	// Index this entry
	int Entry = int(Pos - FirstPosition);
	
	// Update the count
	if(Entry >= EntryCount)
	{
		error("Tried to set the stream position of index entry for edit unit %s when the last entry in the ReorderIndex array is %s\n",
			  Int64toString(Pos).c_str(), Int64toString(FirstPosition + EntryCount - 1).c_str());

		return false;
	}

	// Index the start of the entry
	UInt8 *EntryPtr = &IndexEntries.Data[Entry * IndexEntrySize];

	// Updata the data
	PutI64(StreamOffset, &EntryPtr[3]);

	return true;
}


//! Set the temporal offset for an entry in the table
bool ReorderIndex::SetTemporalOffset(Position Pos, Int8 TemporalOffset)
{
	// If this is the first entry we have added to the table set FirstPosition
	if(IndexEntries.Size == 0)
	{
		FirstPosition = Pos;
	}
	// Otherwise see if we are trying to add an entry before the start
	else if(Pos < FirstPosition)
	{
		// How far do we need to shift the data
		Int64 Shift = (FirstPosition - Pos) * IndexEntrySize;

		// Make enought room
		IndexEntries.Resize((UInt32)(IndexEntries.Size + Shift));

		// Shift the entries forwards
		memmove(&IndexEntries.Data[Shift], IndexEntries.Data, EntryCount * IndexEntrySize);

		// Adjust the counts
		if(CompleteEntryCount) CompleteEntryCount += (int)(FirstPosition - Pos);
		EntryCount +=(int) (FirstPosition - Pos);
		
		// And the start position
		FirstPosition = Pos;
	}

	// Index this entry
	int Entry = int(Pos - FirstPosition);
	
	// Update the count
	if(Entry >= EntryCount)
	{
		IndexEntries.Resize((Entry+1) * IndexEntrySize);
		EntryCount = Entry + 1;
	}
	
	// Index the start of the entry
	UInt8 *EntryPtr = &IndexEntries.Data[Entry * IndexEntrySize];

	// Set the temporal offset
	*EntryPtr = TemporalOffset;

	return true;
}


//! Commit entries to the specified index table
/*! If Count == -1 all entries are comitted
 *  \note There is no way for this function to know for sure which entries have their
 *        TemporalOffsets set so the caller must ensure it only asks us to commit those
 *		  entries that are certain to be totally complete. One possible strategy is to
 *		  always leave at least 128 entries in the table until the end of processing as
 *		  the temporal offsets cannot reach further than 128 backwards.
 */
Int32 ReorderIndex::CommitEntries(IndexTablePtr Index, Int32 Count /*=-1*/)
{
	IndexSegmentPtr Segment = Index->GetSegment(FirstPosition);

	// Note that we only commit complete entries
	if((Count < 0) || (Count > CompleteEntryCount)) Count = CompleteEntryCount;
	
	if(!Segment->AddIndexEntries(Count, IndexEntrySize, IndexEntries.Data))
	{
		// DRAGONS: This will happen when we burst the 64k limit!
		error("Problem in call to IndexSegment::AddIndexEntries from ReorderIndex::CommitEntries\n");

		return 0;
	}

	// Shuffle data back in data chunk (or clear it)
	if(EntryCount <= Count) 
	{
		EntryCount = 0;
		CompleteEntryCount = 0;
		FirstPosition = 0;
		IndexEntries.Resize(0);
	}
	else
	{
		// How far do we need to shift the data
		Int64 Shift = Count * IndexEntrySize;

		Int32 NewCount = EntryCount - Count;

		// Shift the entries backwards
		memmove(IndexEntries.Data, &IndexEntries.Data[Shift], NewCount * IndexEntrySize);

		// Adjust the counts
		if(CompleteEntryCount > NewCount) CompleteEntryCount -= NewCount; else CompleteEntryCount = 0;
		if(EntryCount > NewCount) EntryCount -= NewCount; else EntryCount = 0;
		
		// And the start position
		if(FirstPosition > NewCount) FirstPosition -= NewCount; else FirstPosition = 0;

		IndexEntries.Resize((EntryCount+1) * IndexEntrySize);
	}

	return Count;
}


//! Construct with main stream details
IndexManager::IndexManager(int PosTableIndex, UInt32 ElementSize)
{
	// We haven't yet finalised the format
	FormatFixed = false;

	// Initially decide reordering based only on the main stream
	// (we will check each sub-stream as it is added)
	if(PosTableIndex < 0) UsesReordering = true; else UsesReordering = false;

	// Initially decide CBR flag based only on the main stream
	if(ElementSize > 0) DataIsCBR = true; else DataIsCBR = false;

	// Initialise arrays for up to 16 sub-streams - we will increase this if required
	StreamListSize = 16;
	PosTableList = new int[16];
	ElementSizeList = new UInt32[16];

	// Initialise the main stream
	StreamCount = 1;
	PosTableList[0] = PosTableIndex;
	ElementSizeList[0] = ElementSize;
	ManagedDataEntrySize = sizeof(IndexData);

	// So far we have no completed entries, but initialise the base to something safe
//	CompletedListBase = 0;

	// Initialise the index table values
	BodySID = 0;
	IndexSID = 0;
	EditRate.Numerator = 1;
	EditRate.Denominator = 1;

	// No entries added yet
	LastNewEditUnit = IndexTable::IndexLowest;

	// Initialise the provisional entry
	ProvisionalEntry = NULL;

	// Initialise log
	NextLogID = 0;
	LogWrapped = false;
	LogNextEntry = -1;

	// Initialise acceptance rules
	AcceptNextEntry = false;

	// Clear the value-relative indexing flag
	ValueRelativeIndexing = false;

	// Start with no sub-range offset
	SubRangeOffset = 0;
}


//! Add a sub-stream
/*! \ret Sub-stream ID or 0 if error */
int IndexManager::AddSubStream(int PosTableIndex, UInt32 ElementSize)
{
	// If we have finalised the format we can't add a new stream
	if(FormatFixed)
	{
		error("Tried to add a sub-stream to an IndexManager once the format has been finalised\n");
		return 0;
	}

	// Set reordering flag if this stream required it
	if(PosTableIndex < 0) UsesReordering = true;

	// Clear CBR flag if this stream is VBR
	if(ElementSize == 0) DataIsCBR = false;

	// Increase the size of the stream arrays if required
	if(StreamCount == StreamListSize)
	{
		int NewSize = StreamListSize + 16;
		int *NewPosTableList = new int[NewSize];
		UInt32 *NewElementSizeList = new UInt32[NewSize];
		
		memcpy(NewPosTableList, PosTableList, StreamListSize);
		memcpy(NewElementSizeList, ElementSizeList, StreamListSize);

		delete[] PosTableList;
		delete[] ElementSizeList;

		StreamListSize = NewSize;
		PosTableList = NewPosTableList;
		ElementSizeList = NewElementSizeList;
	}

	// Initialise this stream
	PosTableList[StreamCount] = PosTableIndex;
	ElementSizeList[StreamCount] = ElementSize;

	// Resize to accomodate new stream
	ManagedDataEntrySize = sizeof(IndexData) + (StreamCount * sizeof(UInt64));

	// Return this stream ID, them increment the count
	return StreamCount++;
}


//! Add an edit unit (of a stream) without a known offset
/*! \param SubStream	The stream number, 0 = main stream
	\param EditUnit		The position of the edit unit being set
	\param KeyOffset	The key frame offset for this edit unit (or 0 if not being set by this call)
	\param Flags		The flags for this edit unit (or -1 if not being set by this call)

	DRAGONS: The EditUnit supplied here is the absolute value from stream start, so will not start at 0 if sub-ranged
*/
void IndexManager::AddEditUnit(int SubStream, Position EditUnit, int KeyOffset /*=0*/, int Flags /*=-1*/)
{
	// No need for a CBR index table
	if(DataIsCBR) return;

	// Correct for sub-range offset
	EditUnit -= SubRangeOffset;

	// The entry we are using
	IndexData *ThisEntry = NULL;

	// Can't change the format once we are using the table
	FormatFixed = true;

	// Dump any outstanding provisional entry
	if(ProvisionalEntry) 
	{
		// If this is the same edit unit as the provisional entry we should
		// re-use it to keep any useful data already set
		if(ProvisionalEditUnit == EditUnit)
		{
			ThisEntry = ProvisionalEntry;
			
			// Add the entry to the managed data
			ManagedData.insert(std::pair<Position, IndexData*>(EditUnit, ThisEntry));
			LastNewEditUnit = EditUnit;
		}
		else
			delete[] (UInt8*)ProvisionalEntry;

		ProvisionalEntry = NULL;
	}

	// If we aren't re-using the provisional entry we need to locate or create one
	if(!ThisEntry)
	{
		// Locate the requested edit unit
		std::map<Position, IndexData*>::iterator it = ManagedData.find(EditUnit);

		// Not found - create a new one
		if(it == ManagedData.end())
		{
			ThisEntry = (IndexData*)(new UInt8[ManagedDataEntrySize]);
			
			// Initialise the new entry
			memset(ThisEntry, 0, ManagedDataEntrySize);

			// Can we satisfy an outstanding temporal offset?
			std::map<Position, int>::iterator it2 = UnsatisfiedTemporalOffsets.find(EditUnit);
			if(it2 != UnsatisfiedTemporalOffsets.end())
			{
				// Yes - so take the offset and clear the unsatisfied entry
				ThisEntry->TemporalOffset = (*it2).second;
				UnsatisfiedTemporalOffsets.erase(it2);
			}

			// Can we satisfy an outstanding temporal difference?
			std::map<Position, int>::iterator it3 = UnsatisfiedTemporalDiffs.find(EditUnit);
			if(it3 != UnsatisfiedTemporalDiffs.end())
			{
				// Yes - so take the offset and clear the unsatisfied entry
				ThisEntry->TemporalDiff = (*it3).second;
				UnsatisfiedTemporalDiffs.erase(it3);
			}

			// Add the entry
			ManagedData.insert(std::pair<Position, IndexData*>(EditUnit, ThisEntry));
			LastNewEditUnit = EditUnit;
		}
		else ThisEntry = (*it).second;
	}
	
	if(KeyOffset) ThisEntry->KeyOffset = KeyOffset;
	if(Flags != -1) ThisEntry->Flags = Flags;

	Log(EditUnit);
}


//! Set the offset for a particular edit unit of a stream
//! DRAGONS: does NOT adjust for multiple substreams in a single GC
/*! \param SubStream	The stream number, 0 = main stream
	\param EditUnit		The position of the edit unit being set
	\param Offset		The stream offset of this edit unit
	\param KeyOffset	The key frame offset for this edit unit (or 0 if not being set by this call)
	\param Flags		The flags for this edit unit (or -1 if not being set by this call)
    
	DRAGONS: The EditUnit supplied here is relative to the sub-range, so it will start at 0 if sub-ranged (or be -ve for pre-charge)
*/
void IndexManager::SetOffset(int SubStream, Position EditUnit, UInt64 Offset, int KeyOffset /*=0*/, int Flags /*=-1*/)
{
	// No need for a CBR index table
	if(DataIsCBR) return;

	// The entry we are using
	IndexData *ThisEntry = NULL;

	// Can't change the format once we are using the table
	FormatFixed = true;

	// Dump any outstanding provisional entry
	if(ProvisionalEntry) 
	{
		// If this is the same edit unit as the provisional entry we should
		// re-use it to keep any useful data already set
		if(ProvisionalEditUnit == EditUnit)
		{
			ThisEntry = ProvisionalEntry;
			
			// Add the entry to the managed data
			ManagedData.insert(std::pair<Position, IndexData*>(EditUnit, ThisEntry));
			LastNewEditUnit = EditUnit;
		}
		else
			delete[] (UInt8*)ProvisionalEntry;

		ProvisionalEntry = NULL;
	}

	// If we aren't re-using the provisional entry we need to locate or create one
	if(!ThisEntry)
	{
		// Locate the requested edit unit
		std::map<Position, IndexData*>::iterator it = ManagedData.find(EditUnit);

		// Not found - create a new one
		if(it == ManagedData.end())
		{
			ThisEntry = (IndexData*)(new UInt8[ManagedDataEntrySize]);
			
			// Initialise the new entry
			memset(ThisEntry, 0, ManagedDataEntrySize);

			// Can we satisfy an outstanding temporal offset?
			std::map<Position, int>::iterator it2 = UnsatisfiedTemporalOffsets.find(EditUnit);
			if(it2 != UnsatisfiedTemporalOffsets.end())
			{
				// Yes - so take the offset and clear the unsatisfied entry
				ThisEntry->TemporalOffset = (*it2).second;
				UnsatisfiedTemporalOffsets.erase(it2);
			}

			// Can we satisfy an outstanding temporal difference?
			std::map<Position, int>::iterator it3 = UnsatisfiedTemporalDiffs.find(EditUnit);
			if(it3 != UnsatisfiedTemporalDiffs.end())
			{
				// Yes - so take the offset and clear the unsatisfied entry
				ThisEntry->TemporalDiff = (*it3).second;
				UnsatisfiedTemporalDiffs.erase(it3);
			}

			// Add the entry
			ManagedData.insert(std::pair<Position, IndexData*>(EditUnit, ThisEntry));
			LastNewEditUnit = EditUnit;
		}
		else ThisEntry = (*it).second;
	}

	// Set the offset
	ThisEntry->Status |= 0x01;
	ThisEntry->StreamOffset[SubStream] = Offset;

	if(KeyOffset) ThisEntry->KeyOffset = KeyOffset;
	if(Flags != -1) ThisEntry->Flags = Flags;

	Log(EditUnit);
}


//! Accept or decline an offered edit unit (of a stream) without a known offset
/*! DRAGONS: The EditUnit supplied here is the absolute value from stream start, so will not start at 0 if sub-ranged */
bool IndexManager::OfferEditUnit(int SubStream, Position EditUnit, int KeyOffset /*=0*/, int Flags /*=-1*/)
{
	// DRAGONS: Currently we accept all offered entries

	AddEditUnit(SubStream, EditUnit, KeyOffset, Flags);

//	Log(EditUnit); -- already done in AddEditUnit

	return true;
}


//! Accept or decline an offered offset for a particular edit unit of a stream
/*!	DRAGONS: The EditUnit supplied here is relative to the sub-range, so it will start at 0 if sub-ranged (or be -ve for pre-charge) */
bool IndexManager::OfferOffset(int SubStream, Position EditUnit, UInt64 Offset, int KeyOffset /*=0*/, int Flags /*=-1*/)
{
	// DRAGONS: Currently we accept all offered entries

	SetOffset(SubStream, EditUnit, Offset, KeyOffset, Flags);

	return true;
}


//! Set the temporal offset for a particular edit unit
/*! DRAGONS: The EditUnit supplied here is the absolute value from stream start, so will not start at 0 if sub-ranged */
void IndexManager::SetTemporalOffset(Position EditUnit, int Offset)
{
	// No need for a CBR index table
	if(DataIsCBR) return;

	// Correct for sub-range offset
	EditUnit -= SubRangeOffset;

	// Check the provisional entry first (quite likely and an easy test)
	if((ProvisionalEntry) && (EditUnit == ProvisionalEditUnit))
	{
		ProvisionalEntry->Status |= 0x02;
		ProvisionalEntry->TemporalOffset = Offset;
	}
	else
	{
		// Locate the requested edit unit in the managed data map
		std::map<Position, IndexData*>::iterator it = ManagedData.find(EditUnit);

		// Found - record the offset
		if(it != ManagedData.end())
		{
			(*it).second->Status |= 0x02;
			(*it).second->TemporalOffset = Offset;
		}
		else
		{
			// Else record it as being unsatisfied
			UnsatisfiedTemporalOffsets.insert(std::pair<Position, int>(EditUnit, Offset));
		}
	}

	// Now set the reverse offset (TemporalDiff)

	// Check the provisional entry first (quite likely and an easy test)
	if((ProvisionalEntry) && ((EditUnit + Offset) == ProvisionalEditUnit))
	{
		ProvisionalEntry->Status |= 0x04;
		ProvisionalEntry->TemporalDiff = -Offset;
	}
	else
	{
		// Locate the requested edit unit in the managed data map
		std::map<Position, IndexData*>::iterator it = ManagedData.find(EditUnit + Offset);

		// Found - record the offset
		if(it != ManagedData.end())
		{
			(*it).second->Status |= 0x04;
			(*it).second->TemporalDiff = -Offset;
		}
		else
		{
			// Else record it as being unsatisfied
			UnsatisfiedTemporalDiffs.insert(std::pair<Position, int>(EditUnit, -Offset));
		}
	}
}


//! Accept or decline an offered temporal offset for a particular edit unit
/*! DRAGONS: The EditUnit supplied here is the absolute value from stream start, so will not start at 0 if sub-ranged */
bool IndexManager::OfferTemporalOffset(Position EditUnit, int Offset)
{
	// DRAGONS: Currently we accept all offered entries

	SetTemporalOffset(EditUnit, Offset);

	return true;
}


//! Set the key-frame offset for a particular edit unit
/*! DRAGONS: The EditUnit supplied here is the absolute value from stream start, so will not start at 0 if sub-ranged */
void IndexManager::SetKeyOffset(Position EditUnit, int Offset)
{
	// No need for a CBR index table
	if(DataIsCBR) return;

	// Correct for sub-range offset
	EditUnit -= SubRangeOffset;

	// Check the provisional entry first (quite likely and an easy test)
	if((ProvisionalEntry) && (EditUnit == ProvisionalEditUnit))
	{
		ProvisionalEntry->KeyOffset = Offset;
	}
	else
	{
		// Locate the requested edit unit in the managed data map
		std::map<Position, IndexData*>::iterator it = ManagedData.find(EditUnit);

		// Found - record the offset
		if(it != ManagedData.end())
		{
			(*it).second->KeyOffset = Offset;
		}
		else
		{
			error("Attempted to set the KeyOffset for an unknown edit unit in IndexManager::SetKeyOffset()\n");
		}
	}
}


//! Accept or decline an offered key-frame offset for a particular edit unit
/*! DRAGONS: The EditUnit supplied here is the absolute value from stream start, so will not start at 0 if sub-ranged */
bool IndexManager::OfferKeyOffset(Position EditUnit, int Offset)
{
	// DRAGONS: Currently we accept all offered entries

	SetKeyOffset(EditUnit, Offset);

	return true;
}



#define ManagedDataArrayGranularity 1024		// Number of extra entries to add when creating or extending-up the array

//! Flush index data to free memory
/*! \note The array is not resized (unless it is totally cleared) so this will not gain much when flushing the end of the array. 
 *        If the beginning of the array is flushed the data is shuffled down allowing more new entries to be added to the end before resizing.
 */
void IndexManager::Flush(Position FirstEditUnit, Position LastEditUnit)
{
	// No need for a CBR index table
	if(DataIsCBR) return;

	error("IndexManager::Flush() not yet implemented\n");
}



//! Generate a CBR index table or empty VBR index table for the managed index
IndexTablePtr IndexManager::MakeIndex(void)
{
	// Once we have made an index table the format is very definately fixed
	FormatFixed = true;

	// Build an empty index table
	IndexTablePtr Ret = new IndexTable;

	// Set the index table parameters
	Ret->IndexSID = IndexSID;
	Ret->BodySID = BodySID;
	Ret->EditRate = EditRate;

	// Build the delta array
	Ret->DefineDeltaArray(StreamCount, ElementSizeList);

	// Update the pos table index entries (not set by DefineDeltaArray())
	int i;
	for(i=0; i<StreamCount; i++)
	{
		Ret->BaseDeltaArray[i].PosTableIndex = PosTableList[i];
	}

	// Calculate length if CBR
	if( DataIsCBR )
	{
		UInt32 ByteCount = 0;
		for(i=0; i<StreamCount; i++)
		{
			ByteCount += ElementSizeList[i];
		}
		Ret->EditUnitByteCount = ByteCount;
	}

	// Return the newly built index table
	return Ret;
}


//! Add all complete entries in a range to the supplied index table
/*! \return Number of index entries added */
int IndexManager::AddEntriesToIndex(bool UndoReorder, IndexTablePtr Index, Position FirstEditUnit /*=IndexLowest*/, Position LastEditUnit /*=UINT64_C(0x7fffffffffffffff)*/)
{
	// Count of number of index table entries added
	int Ret = 0;

	// No need for a CBR index table
	if(DataIsCBR) return Ret;

	// Find the first entry, or the nearest after it
	// DRAGONS: Is this inefficient?
	std::map<Position, IndexData*>::iterator it = ManagedData.find(FirstEditUnit);
	if(it == ManagedData.end()) { it = ManagedData.lower_bound(FirstEditUnit); }

	// No data to add
	if((it == ManagedData.end()) || ((*it).first > LastEditUnit)) return Ret;

	// Set up SliceOffsets and PosTable arrays
	int NSL = Index->NSL;
	UInt32 *SliceOffsets = NULL;
	if(NSL) SliceOffsets = new UInt32[NSL];
	int NPE = Index->NPE;
	Rational *PosTable = NULL;
	if(NPE) PosTable = new Rational[NPE];

	// Undo any reordering set in the index table if requested to undo reordering
	if(UsesReordering && UndoReorder)
	{
		int i;
		for(i=0; i<Index->BaseDeltaCount; i++)
		{
			if(Index->BaseDeltaArray[i].PosTableIndex < 0) Index->BaseDeltaArray[i].PosTableIndex = 0;
		}
	}

	// DRAGONS: Not supporting PosTable yet!
	if(NPE)
	{
		error("PosTable not currently supported by IndexManager\n");
		NPE = 0;
		delete[] PosTable;
		PosTable = NULL;
	}

	// Which bits in the status word show we can use the entry?
	int StatusTest;
	if(UsesReordering) StatusTest = 0x03; else StatusTest = 0x01;
	if(UndoReorder) StatusTest |= 0x04;

	// Loop until out of entries
	while((*it).first <= LastEditUnit)
	{
		IndexData *ThisEntry = (*it).second;
		int Slice = 0;

		Position StreamPos = ThisEntry->StreamOffset[0];

		// Don't build an entry if it is not (yet) complete
		if((ThisEntry->Status & StatusTest) != StatusTest)
		{
			if(++it == ManagedData.end()) break;
			continue;
		}

		// Build the slice table
		int i;
		for(i=0; i<(StreamCount-1); i++)
		{
			if( ElementSizeList[i] == 0) // VBR - next Stream will be start of next Slice
			{
				Position NextPos = ThisEntry->StreamOffset[i+1];
				
				if(NextPos >= StreamPos) 
					SliceOffsets[Slice]=(UInt32)(NextPos - StreamPos);
				else
				{
					// Write zero in the slice offset of any missing entry
					// DRAGONS: this is not very good, but what else do we do
					// FIXME: Scan forwards to find the next indexed item to calculate the correct slice offset for a zero size object
					SliceOffsets[Slice]=0;
				}

				Slice++;
			}

			// DRAGONS: Not supporting PosTable yet!
		}

		// Determine the edit unit to add
		Position ThisEditUnit = (*it).first;
		if(UndoReorder) ThisEditUnit += ThisEntry->TemporalDiff;

		// Add this new entry (carry FirstEditUnit up with us as we go)
		Index->AddIndexEntry(ThisEditUnit, ThisEntry->TemporalOffset, ThisEntry->KeyOffset, ThisEntry->Flags, ThisEntry->StreamOffset[0], NSL, SliceOffsets, NPE, PosTable);

		// Maintain count of entries
		Ret++;

		// Move to the next entry
		if(++it == ManagedData.end()) break;
	}

	if(NSL) delete[] SliceOffsets;
	if(NPE) delete[] PosTable;

	return Ret;
}


//! Log next edit unit offered
/*! The next edit unit stored is recorded in the log.
 *  \return An ID used in a call to CheckLog() to get the EditUnit when available (-1 if error)
 */
int IndexManager::LogNext(void)
{
	// We are already logging the next entry - simply return the ID
	if(LogNextEntry >= 0) return LogNextEntry;

	// Something has gone wrong - abort
	if(NextLogID < 0) return -1;

	// Log the next entry
	LogNextEntry = NextLogID;

	// Now try and pick a new log ID
	if(!LogWrapped)
	{
		NextLogID++;

		// Not wrapped - all OK
		if(NextLogID > 0) return LogNextEntry;
		
		// Oops - we have wrapped! Must now search for free slots
		LogWrapped = true;
		NextLogID = -1;
	}

	// The log ID has wrapped - we need to search for an unused slot

	// Start searching from the next numerical slot
	int TryLogID = NextLogID + 1;

	// Search forward to end of range first
	while(TryLogID >= 0)
	{
		if((TryLogID != LogNextEntry) && (EntryLog.find(TryLogID) == EntryLog.end()))
		{
			NextLogID = TryLogID;
			return LogNextEntry;
		}
		TryLogID++;
	}

	// None available in upper end - scan lower end
	
	// No lower end exists - we are out of slots
	if(NextLogID < 0) return -1;

	TryLogID = 0;
	while(TryLogID < LogNextEntry)
	{
		if(EntryLog.find(TryLogID) == EntryLog.end())
		{
			NextLogID = TryLogID;
			return LogNextEntry;
		}
		TryLogID++;
	}

	// Scanned all - none free!
	return -1;
}
