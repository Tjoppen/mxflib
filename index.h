/*! \file	index.h
 *	\brief	Definition of classes that handle index tables
 *  \note	This index table system is far from efficient
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
#ifndef MXFLIB__INDEX_H
#define MXFLIB__INDEX_H

// Forward refs
namespace mxflib
{
	//! Smart pointer to an index table
	class IndexTable;
	typedef SmartPtr<IndexTable> IndexTablePtr;

	//! Smart pointer to an index table segment
	class IndexSegment;
	typedef SmartPtr<IndexSegment> IndexSegmentPtr;
	typedef std::list<IndexSegmentPtr> IndexSegmentList;
}


namespace mxflib
{
	//! Structure for holding the result of an index table look-up
	class IndexPos : public RefCount<IndexPos>
	{
	public:
		Uint64 ThisPos;			//!< The position (in file package edit units) of the data for which Location points to the start
		Uint64 Location;		//!< The location of the start of ThisPos edit unit in the essence container
		Rational PosOffset;		//!< The temporal offset for this edit unit (if Offset = true, otherwise undefined)
		bool Exact;				//!< true if ThisPos is the requested edit unit and the location is for the requested sub-item.
								/*!< false if it is a preceeding edit unit or the requested sub-item could not be identified */
		bool Offset;			//!< true if there is a temporal offset (stored in PosOffset, only set if Exact = true)
		Int8 KeyFrameOffset;	//!< The offset in edit units to the previous key frame
		Uint8 Flags;			//!< The flags for this edit unit (zero if ThisPos is not the requested edit unit)
	};

	//! Smart pointer to an IndexPos
	typedef SmartPtr<IndexPos> IndexPosPtr;

	//! Structure for holding delta entries
	/*! \note This must be s struct for efficiency - don't make into a (complex) class!! */
	struct DeltaEntry
	{
		Int8	PosTableIndex;
		Uint8	Slice;
		Uint32	ElementDelta;
	};

	//! Map of edit unit positions to index table segemnts
	typedef std::map<Position, IndexSegmentPtr> IndexSegmentMap;

	//! Class for holding index entries that may be out of order
	class IndexEntry : public RefCount<IndexEntry>
	{
	public:
		int TemporalOffset;
		int AnchorOffset;
		Uint8 Flags;
		Uint64 StreamOffset;
	};

	//! Smart pointer to index entry
	typedef SmartPtr<IndexEntry> IndexEntryPtr;

	// Map of edit unit with index entries
	typedef std::map<Int64, IndexEntryPtr> IndexEntryMap;
	//typedef SmartPtr<IndexEntryMap> IndexEntryMapPtr;

	//! Class that holds an index table
	class IndexTable : public RefCount<IndexTable>
	{
	public:
		Uint32 IndexSID;
		Uint32 BodySID;
		Rational EditRate;

		//! Byte count for each and every edit unit, if CBR, else zero
		Uint64 EditUnitByteCount;

		//! Number of entries in BaseDeltaArray
		int BaseDeltaCount;

		//! Deltas for CBR data and base delta array for VBR segments
		DeltaEntry *BaseDeltaArray;

		//! Map of edit unit position to index entry for VBR
		IndexSegmentMap SegmentMap;

		int NSL;			//!< NSL as defined in SMPTE-337M (number of slices minus 1)
		int NPE;			//!< NPE as defined in SMPTE-337M (number of PosTable entries)
		int IndexEntrySize;	//!< Size of each index entry (11 + 4*NSL + 8*NPE)

		//! Map of index entries that may be out of order
		/*! The entries will be built into segments by function xxx() */
		IndexEntryMap IndexOrderEntryMap;
		IndexEntryMap EssenceOrderEntryMap;
	
	public:
		//! Construct an IndexTable with no CBRDeltaArray
		IndexTable() : BaseDeltaCount(0), EditUnitByteCount(0) { EditRate.Numerator=0; EditUnitByteCount=0; NSL=0; NPE=0; IndexEntrySize=11; };

		//! Free any memory used by BaseDeltaArray when this IndexTable is destroyed
		~IndexTable() 
		{ 
//printf("Deleting IndexTable at 0x%08x: Count = %d, Array = 0x%08x\n", this, BaseDeltaCount, BaseDeltaArray);
			if(BaseDeltaCount) delete[] BaseDeltaArray; 
		};

		//! Define the base delta entry array from another delta entry array
		void DefineDeltaArray(int DeltaCount, DeltaEntry *DeltaArray)
		{
			if(BaseDeltaCount) delete[] BaseDeltaArray;

			BaseDeltaCount = DeltaCount;
			if(!DeltaCount) return;
			
			// Build the new array
			BaseDeltaArray = new DeltaEntry[DeltaCount];
			memcpy(BaseDeltaArray, DeltaArray, DeltaCount * sizeof(DeltaEntry));

			// Slice numbers start at zero, PosTable numbers start at 1
			NSL = 0;
			NPE = 0;
			int i;
			for(i=0; i<DeltaCount; i++)
			{
				if(BaseDeltaArray[i].PosTableIndex > NPE) NPE = BaseDeltaArray[i].PosTableIndex;
				if(BaseDeltaArray[i].Slice > NSL) NSL = BaseDeltaArray[i].Slice;
			}
		
			// Calculate the size of each IndexEntry
			IndexEntrySize = (11 + 4*NSL + 8*NPE);
		}

		//! Define the base delta entry array from an array of offsets
		/*! With this version of DefineDeltaArray Slice numbers are calculated and 
		 *	all PosTableIndex entries are set to 0. Whenever an Offset has the
		 *	value zero a new slice is started
		 */
		void DefineDeltaArray(int DeltaCount, Uint32 *DeltaArray)
		{
			if(BaseDeltaCount) delete[] BaseDeltaArray;

			BaseDeltaCount = DeltaCount;
			if(!DeltaCount) return;

			// Build the new array
			BaseDeltaArray = new DeltaEntry[DeltaCount];

			// Slice numbers start at zero, PosTable numbers start at 1
			NSL = 0;
			NPE = 0;
			int i;
			for(i=0; i<DeltaCount; i++)
			{
				// Start of a new slice?
				if((i != 0) && (DeltaArray[i] == 0)) NSL++;

				BaseDeltaArray[i].ElementDelta = DeltaArray[i];
//				BaseDeltaArray[i].PosTableIndex = 0;
//### HACK!!
BaseDeltaArray[i].PosTableIndex = -1;
				BaseDeltaArray[i].Slice = NSL;
			}

			// Calculate the size of each IndexEntry
			IndexEntrySize = (11 + 4*NSL + 8*NPE);
		}

		//! Add an index table segment from an "IndexSegment" MDObject
		IndexSegmentPtr AddSegment(MDObjectPtr Segment);

		//! Create a new empty index table segment
		IndexSegmentPtr AddSegment(Int64 StartPosition);

		//! Add a single index entry creating segments as required
		bool AddIndexEntry(Position EditUnit, Int8 TemporalOffset, Int8 KeyFrameOffset, Uint8 Flags, Uint64 StreamOffset, 
						   int SliceCount = 0, Uint32 *SliceOffsets = NULL, 
						   int PosCount = 0, Rational *PosTable = NULL);

//#####
//##### DRAGONS: Should Lookup also check the pending items?
//#####
		//! Perform an index table look-up
		IndexPosPtr Lookup(Position EditUnit, Uint32 SubItem = 0, bool Reorder = true);

		//! Fudge to correct index entry
		void Correct(Position EditUnit, Int8 TemporalOffset, Int8 KeyFrameOffset, Uint8 Flags);

		//! Free memory by purging the specified range from the index
		void Purge(Uint64 FirstPosition, Uint64 LastPosition);

		//! Write this index table to a memory buffer
		Uint32 WriteIndex(DataChunk &Buffer);

		//! Add a new index entry that may be out of order
		/*! The entry is added to IndexOrderEntryMap and EssenceOrderEntryMap */
		void AddNewEntry(Int64 IndexOrder, Int64 EssenceOrder, IndexEntryPtr NewEntry)
		{
			IndexOrderEntryMap.insert(IndexEntryMap::value_type(IndexOrder, NewEntry));
			EssenceOrderEntryMap.insert(IndexEntryMap::value_type(EssenceOrder, NewEntry));
		}

		//! Get the "new" index entry in IndexOrderEntryMap (i.e. by indexed order)
		IndexEntryPtr IndexEntryByIndexOrder(Int64 Pos)
		{
			IndexEntryMap::iterator it = IndexOrderEntryMap.find(Pos);

			if(it == IndexOrderEntryMap.end()) return NULL;

			return (*it).second;
		}

		//! Get the "new" index entry in EssenceOrderEntryMap (i.e. by essence order)
		IndexEntryPtr IndexEntryByEssenceOrder(Int64 Pos)
		{
			IndexEntryMap::iterator it = EssenceOrderEntryMap.find(Pos);

			if(it == EssenceOrderEntryMap.end()) return NULL;

			return (*it).second;
		}

		void CommitIndexEntries(void)
		{
			IndexEntryMap::iterator it = IndexOrderEntryMap.begin();

			while(it != IndexOrderEntryMap.end())
			{
				AddIndexEntry((*it).first, (*it).second->TemporalOffset, (*it).second->AnchorOffset, (*it).second->Flags, (*it).second->StreamOffset);
				IndexOrderEntryMap.erase(it);
				it = IndexOrderEntryMap.begin();
			}

			EssenceOrderEntryMap.clear();
		}
	};
}


namespace mxflib
{
	class IndexSegment : public RefCount<IndexSegment>
	{
	private:
		static PrimerPtr IndexPrimer;

	public:
		//! Table that owns this segment
		IndexTablePtr Parent;
		
		//! Edit unit of the first entry in this segment
		Int64 StartPosition;

		//! Number of entries in DeltaArray
		int DeltaCount;

		//! Deltas for this segment
		DeltaEntry *DeltaArray;

		//! Number of entries in IndexEntryArray
		int EntryCount;

		//! Index Entries for this segment
		/*! \note This can't be an array of structs because they are variable length */
		DataChunk IndexEntryArray;

	private:
		//! Private constructor to force construction via AddIndexSegmentToIndexTable()
		IndexSegment() : EntryCount(0) { };

	public:
		//! Destructor cleans up the segment
		~IndexSegment() { if(DeltaCount) delete[] DeltaArray; };

		//! Index segment pseudo-constructor
		/*! \note <b>Only</b> call this from IndexTable::AddSegment() because it adds the segment to its SegmentMap */
		static IndexSegmentPtr AddIndexSegmentToIndexTable(IndexTablePtr ParentTable, Int64 IndexStartPosition);

		//! Add a single index entry
		bool AddIndexEntry(Int8 TemporalOffset, Int8 KeyFrameOffset, Uint8 Flags, Uint64 StreamOffset, 
						   int SliceCount = 0, Uint32 *SliceOffsets = NULL, 
						   int PosCount = 0, Rational *PosTable = NULL);

		//! Add multiple - pre-formed index entries
		bool AddIndexEntries(int Count, int Size, Uint8 *Entries);
	};
}



#endif MXFLIB__INDEX_H

