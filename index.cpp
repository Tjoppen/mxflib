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
void IndexTable::Purge(Uint64 FirstPosition, Uint64 LastPosition)
{
	IndexEntryMap::iterator it = EntryMap.lower_bound(FirstPosition);

	// If the first position is before the start, go from the start
	if(it == EntryMap.end()) it = EntryMap.begin();

	// Erase all entries up to the last position
	while(it != EntryMap.end() && (*it).first <= LastPosition) it = EntryMap.erase(it);
};


//! Perform an index table look-up
/*! Note that the return value is relative to the start of the EC in frame-wrapping,
 *  but relative to the start of the value of the first KLV in the first edit unit
 *  in the essence container in clip-wrapping
 */
IndexPosPtr IndexTable::Lookup(Position EditUnit, Uint32 SubItem /* =0 */, bool Reorder /* =false */)
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
			if(SubItem >= CBRDeltaCount)
			{
				Ret->Exact = false;
			}
			else
			{
				// Otherwise add the delta
				Ret->Exact = true;
				Loc += CBRDeltaArray[SubItem];
			}
		}

		Ret->ThisPos = EditUnit;
		Ret->Location = Loc;
		Ret->Offset = false;
		Ret->Flags = 0;

		return Ret;
	}

	// Find the correct entry, or the nearest before it
	IndexEntryMap::iterator it = EntryMap.lower_bound(EditUnit);

	// If the first position is before the start of the map, return the start of the essence
	// Also return this if we found a useless index entry (shouldn't happen!)
	if(it == EntryMap.end() || (*it).second->SubItemCount == 0)
	{
		Ret->ThisPos = 0;
		Ret->Location = 0;
		Ret->Exact = false;
		Ret->Offset = false;
		Ret->Flags = 0;

		return Ret;
	}

	// Record the location of what we have found
	Ret->ThisPos = (*it).first;

	// If it is a previous edit unit, set the result accordingly
	if((*it).first != EditUnit)
	{
		Ret->Location = (*it).second->SubItemArray[0].Location;
		Ret->Exact = false;
		Ret->Offset = false;
		Ret->Flags = 0;

		return Ret;
	}


	// If we don't have details of the exact sub-item return the start of the edit unit
	if(SubItem >= (*it).second->SubItemCount)
	{
		Ret->Location = (*it).second->SubItemArray[0].Location;
		Ret->Exact = false;
		Ret->Offset = false;
		Ret->Flags = (*it).second->Flags;

		return Ret;
	}

	// Apply temporal re-ordering if we should, but only if we have details of the exact sub-item
	if(Reorder)
	{
		if ((*it).second->SubItemArray[SubItem].TemporalOffset != 0)
		{
			return Lookup(EditUnit + (*it).second->SubItemArray[SubItem].TemporalOffset, SubItem, false);
		}
	}

	// We now have an exact match

	Ret->Exact = true;
	Ret->Location = (*it).second->SubItemArray[SubItem].Location;
	Ret->Flags = (*it).second->Flags;

	// Sort the PosOffset if one is required
	Ret->Offset = (*it).second->SubItemArray[SubItem].Offset;
	if( Ret->Offset )
	{
		Ret->PosOffset = (*it).second->SubItemArray[SubItem].PosOffset;
	}

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
			if(CBRDeltaCount != NewDeltaCount)
			{
				if(CBRDeltaCount) delete[] CBRDeltaArray;
				CBRDeltaCount = NewDeltaCount;
				CBRDeltaArray = new Uint32[CBRDeltaCount];

				int Delta = 0;
				MDObjectList::iterator it = DeltaList->begin();
				while(it != DeltaList->end())
				{
					CBRDeltaArray[Delta++] = (*it)->GetUint();
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

