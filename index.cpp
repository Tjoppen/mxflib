/*! \file	index.cpp
 *	\brief	Implementation of classes that handle index tables
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

#include "mxflib.h"

using namespace mxflib;


//! Free memory by purging the specified range from the index
/*! DRAGONS: This function needs testing, also it could be improved to purge partial segments as well */
void IndexTable::Purge(Uint64 FirstPosition, Uint64 LastPosition)
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
		if( ((*it).first + (*it).second->EntryCount - 1) <= LastPosition)
			it = SegmentMap.erase(it);
		else
			break;
	}
};


//! Add a single index entry creating segments as required
bool IndexTable::AddIndexEntry(Position EditUnit, Int8 TemporalOffset, Int8 KeyFrameOffset, Uint8 Flags, Uint64 StreamOffset, 
				   int SliceCount /*=0*/, Uint32 *SliceOffsets /*=NULL*/, int PosCount /*=0*/, Rational *PosTable /*=NULL*/)
{
	// Find the correct segment  - one starting with this edit unit, or the nearest before it
	IndexSegmentMap::iterator it = SegmentMap.find(EditUnit);
	if(it == SegmentMap.end()) 
	{ 
		it = SegmentMap.lower_bound(EditUnit); 
		if(!SegmentMap.empty())
		{
			it--;
		}
	}

	// DRAGONS: will the above work as intended?

	// Build a segment pointer for ease of working (very slight inefficiency)
	IndexSegmentPtr Segment;

	// If this position is before the start of the index table we must add a new segment
	if((it == SegmentMap.end()) || ((*it).first > EditUnit))
	{
		Segment = AddSegment(EditUnit);
	}
	else
	{
		Segment = (*it).second;
	}

	// If this position is greater than the current free slot at the end of the segment we must add a new segment
	if(EditUnit > (Segment->StartPosition + Segment->EntryCount))
	{
		Segment = AddSegment(EditUnit);
	}

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
IndexPosPtr IndexTable::Lookup(Position EditUnit, Uint32 SubItem /* =0 */, bool Reorder /* =true */)
{
	IndexPosPtr Ret = new IndexPos;

	// Deal with CBR first
	if(EditUnitByteCount)
	{
		// Start of edit unit
		Uint64 Loc = EditUnit * EditUnitByteCount;

		if(SubItem == 0)
		{
			// If we are looking for the first sub-stream then all is fine
			Ret->Exact = true;
		}
		else
		{
			// Can't index a stream if we don't have a delta to it
			if(SubItem >= BaseDeltaCount)
			{
				Ret->Exact = false;
			}
			else
			{
				// Otherwise add the delta
				Ret->Exact = true;
				Loc += BaseDeltaArray[SubItem].ElementDelta;
			}
		}

		Ret->ThisPos = EditUnit;
		Ret->Location = Loc;
		Ret->Offset = false;
		Ret->KeyFrameOffset = 0;
		Ret->Flags = 0;

		return Ret;
	}

	// Find the correct segment  - one starting with this edit unit, or the nearest before it
	IndexSegmentMap::iterator it = SegmentMap.find(EditUnit);
	if(it == SegmentMap.end()) { if(!SegmentMap.empty()) { it = SegmentMap.lower_bound(EditUnit); it--; } }

	// DRAGONS: will the above work as intended?

	// If this position is before the start of the index table, return the start of the essence
	if((it == SegmentMap.end()) || ((*it).first > EditUnit))
	{
		Ret->ThisPos = 0;
		Ret->Location = 0;
		Ret->Exact = false;
		Ret->Offset = false;
		Ret->KeyFrameOffset = 0;
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
		Ret->Flags = 0;

		return Ret;
	}

	// If the nearest (or lower) index point is before this edit unit, set the result accordingly
	if((Segment->StartPosition + Segment->EntryCount - 1) < EditUnit)
	{
		Ret->ThisPos = Segment->StartPosition + Segment->EntryCount - 1;
		
		// Index the start of the index entry
		Uint8 *Ptr = &Segment->IndexEntryArray.Data[(Segment->EntryCount-1) * IndexEntrySize];

		// Skip the temporal and key-frame offsets and the flags as this is not an exact result
		Ptr += 3;

		// Read the location of the start of the edit unit
		Ret->Location = GetI64(Ptr);

		// Set non-exact values
		Ret->Exact = false;
		Ret->Offset = false;
		Ret->KeyFrameOffset = 0;
		Ret->Flags = 0;

		return Ret;
	}

	// Index the start of the correct index entry
	Uint8 *Ptr = &Segment->IndexEntryArray.Data[(EditUnit - Segment->StartPosition) * IndexEntrySize];

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

	// Read the flags for this frame
	Ret->Flags = GetU8(Ptr);
	Ptr++;

	// Read the location of the start of the edit unit
	Ret->Location = GetI64(Ptr);
	Ptr += 8;

	// Note: At this point Ptr indexes the start of the SliceOffset array

	// If we don't have details of the exact sub-item return the start of the edit unit
	if(SubItem >= Segment->DeltaCount)
	{
		Ret->Exact = false;
		Ret->Offset = false;

		return Ret;
	}

	// We now have an exact match
	Ret->Exact = true;
	
	// Locate this sub-item in the edit unit
	if(SubItem > 0) 
	{
		// Index the correct slice for this sub-item
		Uint8 *SlicePtr = Ptr + ((Segment->DeltaArray[SubItem].Slice - 1) * sizeof(Uint32));

		Ret->Location += GetU32(SlicePtr);
		Ret->Location += Segment->DeltaArray[SubItem].ElementDelta;
	}

	// Sort the PosOffset if one is required
	if(TemporalOffset > 0)
	{
		// Index the correct PosTable entry for this sub-item
		Uint8 *PosPtr = Ptr + (NSL * sizeof(Uint32)) + ((TemporalOffset - 1) * (sizeof(Uint32)*2) );

		Ret->PosOffset.Numerator = GetI32(PosPtr);
		PosPtr += sizeof(Uint32);
		Ret->PosOffset.Denominator = GetI32(PosPtr);
	}
	else
		Ret->Offset = false;

	return Ret;
}


//! Add an index table segment
/*! DRAGONS: This section should check that the first delta entry is slice 0, offset 0
             and insert a spoof one if missing - this is because the spec has at times
			 suggested that the first one is omitted.
 
 *	\note Properties such as IndexEditRate are set by this function - if they are 
 *		  different from the current versions they will change.
 * 
 * DRAGONS: Should we bother updating per-index-table values if we already have valid ones?
 */
#if 0
 void IndexTable::AddSegment(MDObjectPtr Segment)
{
	// Pointer to allow easy manipulation of sub-objects
	MDObjectPtr Ptr;

	Ptr = Segment["IndexEditRate"];
	if(Ptr)
	{
		EditRate.Numerator = Ptr->GetInt("Numerator");
		EditRate.Denominator = Ptr->GetInt("Denominator");
	}

	EditUnitByteCount = Segment->GetInt64("EditUnitByteCount");

	if(EditUnitByteCount != 0)
	{
		Ptr = Segment["DeltaEntryArray"];
		if(Ptr && (Ptr->size() > 0))
		{
			// DRAGONS: Should we compare the two to validate what we find?
			MDObjectListPtr DeltaList = Ptr->ChildList("ElementDelta");
			int NewDeltaCount = DeltaList->size();
			if(BaseDeltaCount != NewDeltaCount)
			{
				if(BaseDeltaCount) delete[] BaseDeltaArray;
				BaseDeltaCount = NewDeltaCount;
				BaseDeltaArray = new DeltaEntry[BaseDeltaCount];

				int Delta = 0;
				MDObjectList::iterator it = DeltaList->begin();
				while(it != DeltaList->end())
				{
					BaseDeltaArray[Delta].PosTableIndex = 0;			// Must be 0 for CBR
					BaseDeltaArray[Delta].Slice = 0;					// Must be 0 for CBR
					BaseDeltaArray[Delta++].ElementDelta = (*it)->GetUint();
					it++;
				}
			}
		}

		// CBR entries done
		return;
	}


	Position StartPos = Segment->GetInt64("IndexStartPosition", -1);
	if(StartPos == -1)
	{
		error("No IndexStartPosition in  %s at %s\n", Segment->FullName().c_str(), Segment->GetSourceLocation().c_str());
		return;
	}

	// Work out how many delta entries there are
	Uint32 Delta_size = 0;
	MDObjectPtr DeltaEntryArray = Segment["DeltaEntryArray"];
	if(DeltaEntryArray && DeltaEntryArray->GetType()->size()) 
	{
		Delta_size = DeltaEntryArray->size() / DeltaEntryArray->GetType()->size();
	}

	if(Delta_size = 0)
	{
		warning("No entries in DeltaEntryArray in %s at %s\n", Segment->FullName().c_str(), Segment->GetSourceLocation().c_str());
	}


	// Note: 32-bit integers used when parsing index tables because most
	//       modern CPUs are more efficient working with 32-bit values
	Uint32 SliceCount = Segment->GetUint("SliceCount");
	Uint32 PosTableCount = Segment->GetUint("PosTableCount");

	Ptr = Segment["IndexEntryArray"];
	if(!Ptr)
	{
		error("VBR Index table segment at %s does not have an IndexEntryArray\n", Segment->GetSourceLocation().c_str());
		return;
	}

	Position ThisPos = StartPos;
	MDObjectNamedList::iterator it = Ptr->begin();
	MDObjectNamedList::iterator itend = Ptr->end();
	while(it != itend)
	{
		IndexEntryPtr NewEntry = new IndexEntry;

		ASSERT((*it).first == "TemporalOffset");
		Int32 TemporalOffset = (*it).second->GetInt();

		it++;
		if(it == itend) break;
		ASSERT((*it).first == "AnchorOffset");
		Int32 AnchorOffset = (*it).second->GetInt();

		it++;
		if(it == itend) break;
		ASSERT((*it).first == "Flags");
		NewEntry->Flags = (*it).second->GetInt();

		it++;
		if(it == itend) break;
		ASSERT((*it).first == "StreamOffset");
		Uint64 StreamOffset = (*it).second->GetUint64();

		it++;
		if(it == itend) break;
		ASSERT((*it).first == "SliceOffsetArray");

		if(Delta_size == 0) NewEntry->SubItemCount = 1; else NewEntry->SubItemCount = Delta_size;
		NewEntry->SubItemArray = new IndexSubItem[NewEntry->SubItemCount];

		// No delta entry array - not strictly valid, but we can work it out
		if(Delta_size == 0)
		{
			NewEntry->SubItemArray[0].Location = StreamOffset;
			NewEntry->SubItemArray[0].Offset = false;

			// Even though there is no PosTableIndex to be < 0, there is no point having an offset if we don't use it!
			NewEntry->SubItemArray[0].TemporalOffset = TemporalOffset;

			NewEntry->SubItemArray[0].AnchorOffset = AnchorOffset;

			it++;
			if(it == itend) break;
			ASSERT((*it).first == "PosTableArray");
		}
		else
		{
			MDObjectPtr SliceOffsetArray = (*it).second;
			// Build an iterator for the SliceOffsetArray
			MDObjectNamedList::iterator Slice_it = SliceOffsetArray->begin();
			int Slice_size = SliceOffsetArray->size();
			int Slice_index = 0;

			it++;
			if(it == itend) break;
			ASSERT((*it).first == "PosTableArray");
			MDObjectPtr PosTableArray = (*it).second;
			
			// Build an iterator for the PosTableArray
			MDObjectNamedList::iterator Pos_it = PosTableArray->begin();
			int Pos_size = PosTableArray->size();
			int Pos_index = 0;

			// Loop through each sub-item for this edit unit
			int Delta_index = 0;
			MDObjectNamedList::iterator Delta_it = DeltaEntryArray->begin();
			while(Delta_it != DeltaEntryArray->end())
			{
				ASSERT((*Delta_it).first == "PosTableIndex");
				Int32 PosTableIndex = (*Delta_it).second->GetInt();

				if(PosTableIndex < 0)
				{
					NewEntry->SubItemArray[Delta_index].TemporalOffset = TemporalOffset;
					NewEntry->SubItemArray[Delta_index].Offset = false;
				}
				else
				{
					NewEntry->SubItemArray[Delta_index].TemporalOffset = 0;

					if(PosTableIndex > 0 && PosTableIndex <= Pos_size)
					{
						NewEntry->SubItemArray[Delta_index].Offset = true;

						// PosTableIndex starts at 1, we need it to start at 0
						PosTableIndex--;
						if(PosTableIndex < Pos_index)
						{
							Pos_it = PosTableArray->begin();
							Pos_index = 0;
						}
						while(PosTableIndex > Pos_index)
						{
							Pos_it++;
							Pos_index++;
						}

						// Transfer the PosOffset
						NewEntry->SubItemArray[Delta_index].PosOffset.Numerator = (*Pos_it).second->GetInt("Numerator");
						NewEntry->SubItemArray[Delta_index].PosOffset.Denominator = (*Pos_it).second->GetInt("Denominator");
					}
					else 
					{
						NewEntry->SubItemArray[Delta_index].Offset = false;
					}
				}

				NewEntry->SubItemArray[Delta_index].AnchorOffset = AnchorOffset;

				// Process the slice
				Delta_it++;
				ASSERT((*Delta_it).first == "Slice");
				Uint32 Slice = (*Delta_it).second->GetUint();
				
				Uint32 SliceOffset = 0;
				if(Slice > 0)
				{
					// SliceOffsetArray starts at 1, we need it to start at 0
					Slice--;
					if(Slice < Slice_index)
					{
						Slice_it = SliceOffsetArray->begin();
						Slice_index = 0;
					}
					while(Slice > Slice_index)
					{
						Slice_it++;
						Slice_index++;
					}

					Slice = (*Slice_it).second->GetUint();
				}

				// Calculate the element location
				Delta_it++;
				ASSERT((*Delta_it).first == "ElementDelta");
			
				NewEntry->SubItemArray[Delta_index].Location = StreamOffset + Slice + (*Delta_it).second->GetUint();

				Delta_it++;	
			}
		}
	
		// Add this new IndexEntry
		EntryMap.insert(IndexEntryMap::value_type(ThisPos, NewEntry));

		// Process next IndexEntry
		ThisPos++;
		it++;
	}
}
#endif // 0


//! Add an index table segment from an "IndexSegment" MDObject
/*! DRAGONS: Not the most efficient way to do this */
IndexSegmentPtr IndexTable::AddSegment(MDObjectPtr Segment)
{
	IndexSegmentPtr Ret;

	// DRAGONS: Must complete this!
	warning("Index table reading not complete!\n");

	Int64 StartPosition = Segment->GetInt64("IndexStartPosition");
	Ret = AddSegment(StartPosition);
	SegmentMap.insert(IndexSegmentMap::value_type(StartPosition, Ret));

	MDObjectPtr Ptr = Segment["DeltaEntryArray"];
	if(!Ptr)
	{
		Ret->DeltaCount = 0;
	}
	else
	{
		Ret->DeltaCount = Ptr->size();

		error("DeltaEntryArray reading not yet supported - assuming simple version\n");
{
	Uint32 ZeroDelta = 0;
	DefineDeltaArray(1, &ZeroDelta);
}

	}

	// DRAGONS - must copy delta array into parent

	// Copy index entry bits...
	NSL = Segment->GetUint("SliceCount");
	NPE = Segment->GetUint("PosTableCount");
	// Calculate the size of each IndexEntry
	IndexEntrySize = (11 + 4*NSL + 8*NPE);

	Ptr = Segment["IndexEntryArray"];
	if(!Ptr)
	{
		Ret->EntryCount = 0;
		error("No IndexEntryArray - is this a CBR index? (not currently supported)\n");
	}
	else
	{
		DataChunk Entries;
		Ptr->WriteObject(Entries);

		if(Entries.Size >= 28)
		{
			Ret->EntryCount = GetU32(&Entries.Data[20]);
			Uint32 EntrySize = GetU32(&Entries.Data[24]);
			
			if(EntrySize != IndexEntrySize)
			{
				error("IndexEntryArray items should be %d bytes, but are %d\n", IndexEntrySize, EntrySize);
			}
			else Ret->AddIndexEntries(Ret->EntryCount, IndexEntrySize, &Entries.Data[28]);
		}
	}

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
bool IndexSegment::AddIndexEntry(Int8 TemporalOffset, Int8 KeyFrameOffset, Uint8 Flags, Uint64 StreamOffset, 
								 int SliceCount /*=0*/, Uint32 *SliceOffsets /*=NULL*/,
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
	int NewSize = (EntryCount+1) * Parent->IndexEntrySize;
	if(NewSize > 0xffff) return false;

	Uint8 *Buffer = new Uint8[Parent->IndexEntrySize];

	// Write the new entry
	Buffer[0] = (Uint8) TemporalOffset;
	Buffer[1] = (Uint8) KeyFrameOffset;
	Buffer[2] = Flags;
	PutU64(StreamOffset, &Buffer[3]);

	Uint8 *Ptr = &Buffer[11];
	Uint32 *SlicePtr = SliceOffsets;
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

	return true;
}


//! Add multiple - pre-formed index entries
bool IndexSegment::AddIndexEntries(int Count, int Size, Uint8 *Entries)
{
	ASSERT(Parent);

	if(Size != Parent->IndexEntrySize)
	{
		error("Current index table has entries of size %d, tried to add entries of size %d\n", Parent->IndexEntrySize, Size);
		return false;
	}

	// Calculate the new size to see if it is too big for a 2-byte local local set length
	int NewSize = (EntryCount * Parent->IndexEntrySize) + (Count * Size);
	if(NewSize > 0xffff) return false;

	// Add this entry to the end of the Index Entry Array
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
	Segment->DeltaArray = new DeltaEntry[Segment->DeltaCount];
	if(ParentTable->BaseDeltaCount) 
	{
		memcpy(Segment->DeltaArray, ParentTable->BaseDeltaArray, ParentTable->BaseDeltaCount * sizeof(DeltaEntry));
	}

	return Segment;
};


//! Write this index table to a memory buffer
Uint32 IndexTable::WriteIndex(DataChunk &Buffer)
{
	// Force a bit of space into the buffer then clear the size
	Buffer.ResizeBuffer(4096);
	Buffer.Resize(0);

	IndexSegmentMap::iterator it = SegmentMap.begin();
	while(it != SegmentMap.end())
	{
		IndexSegmentPtr Segment = (*it).second;

		MDObjectPtr ThisSegment = new MDObject("IndexTableSegment");
		if(!ThisSegment)
		{
			error("Couldn't build \"IndexTableSegment\" - index table not written\n");
			return Buffer.Size;
		}

		MDObjectPtr Ptr;
		Ptr = ThisSegment->AddChild("IndexEditRate");
		if(Ptr)
		{
			Ptr->SetInt("Numerator", EditRate.Numerator);
			Ptr->SetInt("Denominator", EditRate.Denominator);
		}

		ThisSegment->SetInt64("IndexStartPosition", Segment->StartPosition);
		ThisSegment->SetInt64("IndexDuration", Segment->EntryCount);
		ThisSegment->SetUint("EditUnitByteCount", EditUnitByteCount);
		ThisSegment->SetUint("IndexSID", IndexSID);
		ThisSegment->SetUint("BodySID", BodySID);

		// DRAGONS: This assumes constant NSL and NPE...
		ThisSegment->SetUint("SliceCount", NSL);
		ThisSegment->SetUint("PosTableCount", NPE);

		// DRAGONS: A bit clunky!
		// DRAGONS: What if on this platform sizeof(DeltaEntry) != 6 ?
		Uint8 Buf[8];
		DataChunk Deltas;
		PutU32(NSL, Buf);
		PutU32(sizeof(DeltaEntry), &Buf[4]);
		Deltas.Set(8, Buf);
		Deltas.Set(NSL * sizeof(DeltaEntry), (Uint8*)Segment->DeltaArray, 8);

		ThisSegment->SetValue("DeltaEntryArray", Deltas);

		// DRAGONS: A bit clunky!
		DataChunk Entries;
		PutU32(Segment->EntryCount, Buf);
		PutU32(IndexEntrySize, &Buf[4]);
		Entries.Set(8, Buf);
		Entries.Set(IndexEntrySize * Segment->EntryCount, Segment->IndexEntryArray.Data, 8);

		ThisSegment->SetValue("IndexEntryArray", Entries);

		// Add this segment to the buffer
		{
			DataChunk Seg;
			ThisSegment->WriteObject(Seg, MDOType::GetStaticPrimer());
			Buffer.Set(Seg.Size, Seg.Data, Buffer.Size);
		}

		it++;
	}

	return Buffer.Size;
}


//! Fudge to correct index entry
void IndexTable::Correct(Position EditUnit, Int8 TemporalOffset, Int8 KeyFrameOffset, Uint8 Flags)
{
	IndexPosPtr Ret = new IndexPos;

	// Find the correct segment  - one starting with this edit unit, or the nearest before it
	IndexSegmentMap::iterator it = SegmentMap.find(EditUnit);
	if(it == SegmentMap.end()) { if(!SegmentMap.empty()) { it = SegmentMap.lower_bound(EditUnit); it--; } }

	// If this position is before the start of the index table do nothing
	if((it == SegmentMap.end()) || ((*it).first > EditUnit)) return;

	// Build a segment pointer for ease of reading (very slight inefficiency)
	IndexSegmentPtr Segment = (*it).second;

	// Do nothing if we found a useless index entry (shouldn't happen!)
	if(Segment->EntryCount == 0) return;

	// If the nearest (or lower) index point is before this edit unit, do nothing
	if((Segment->StartPosition + Segment->EntryCount - 1) < EditUnit) return;

	// Index the start of the correct index entry
	Uint8 *Ptr = &Segment->IndexEntryArray.Data[(EditUnit - Segment->StartPosition) * IndexEntrySize];

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
