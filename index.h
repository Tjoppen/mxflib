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

namespace mxflib
{
	// Structure for holding the result of an index table look-up
	class IndexPos : public RefCount<IndexPos>
	{
	public:
		Uint64 ThisPos;			//!< The position (in file package edit units) of the data for which Location points to the start
		Uint64 Location;		//!< The location of the start of ThisPos edit unit in the essence container
		Rational PosOffset;		//!< The temporal offset for this edit unit (if Offset = true, otherwise undefined)
		bool Exact;				//!< true if ThisPos is the requested edit unit and the location is for the requested sub-item.
								/*!< false if it is a preceeding edit unit or the requested sub-item could not be identified */
		bool Offset;			//!< true if there is a temporal offset (stored in PosOffset, only set if Exact = true)
		Uint8 Flags;			//!< The flags for this edit unit (zero if ThisPos is not the requested edit unit)
	};

	//! Smart pointer to an IndexPos
	typedef SmartPtr<IndexPos> IndexPosPtr;

	struct IndexSubItem
	{
		Uint64 Location;		//!< The location of the start of this sub-item relative to the start of the essence container
		Rational PosOffset;		//!< The temporal offset for this sub-item (undefined if Offset = false)
		bool Offset;			//!< true if this sub-item has a PosOffset
		Int8 TemporalOffset;	//!< Offset in edit units from Display Order to Coded Order (0 if this sub-item not re-ordered)
		Int8 AnchorOffset;		//!< Offset in edit units to previous Anchor Frame. Zero if this is an anchor frame.
	};

	class IndexEntry : public RefCount<IndexEntry>
	{
	public:
		int	SubItemCount;
		IndexSubItem *SubItemArray;

/*		Uint64 Location;		//!< The location of the start of this edit unit in the essence container
		Uint32 DeltaCount;		//!< Number of deltas in DeltaArray
		Uint32 *DeltaArray;		//!< Deltas from the start of this edit unit to each sub-stream
		Uint32 PosCount;		//!< Number of PosTable entries
		Rational *PosTable;		//!< The temporal offset for sub-items in this edit unit
		Uint8 *PosTableIndex;	//!< As per index table PosTableIndex (one per DeltaArray entry)
								/*!< Values are as follows:
								 *   -  -1 = Apply temporal reordering
								 *   -   0 = Don't apply temporal reordering
								 *   - +ve = Index into PosOffset 
								 */
/*		Int8 TemporalOffset;	//!< Offset in edit units from Display Order to Coded Order
		Int8 AnchorOffset;		//!< Offset in edit units to previous Anchor Frame. Zero if this is an anchor frame.
*/
		Uint8 Flags;			//!< The flags for this edit unit

		//! Construct an IndexEntry with no SubItemArray
		IndexEntry() : SubItemCount(0) {};

		//! Free any memory used by DeltaArray when this IndexEntry is destroyed
		~IndexEntry() 
		{ 
			if(SubItemCount) delete[] SubItemArray; 
		};
	};

	//! Smart pointer to an index entry
	typedef SmartPtr<IndexEntry> IndexEntryPtr;

	//! Map of edit unit positions to index entries
	typedef std::map<Position, IndexEntryPtr> IndexEntryMap;

	class IndexTable : public RefCount<IndexTable>
	{
	public:
//		Uint32 IndexSID;
//		Uint32 BodySID;
		Rational EditRate;

		// Byte count for each and every edit unit, if CBR, else zero
		Uint64 EditUnitByteCount;

		// Number of delta entries for CBR data
		int CBRDeltaCount;

		// Deltas for CBR data
		Uint32 *CBRDeltaArray;

		// Map of edit unit position to index entry for VBR
		IndexEntryMap EntryMap;

	public:
		//! Construct an IndexTable with no CBRDeltaArray
		IndexTable() : CBRDeltaCount(0) {};

		//! Free any memory used by CBRDeltaArray when this IndexTable is destroyed
		~IndexTable() 
		{ 
			if(CBRDeltaCount) delete[] CBRDeltaArray; 
		};

		//! Add an index table segment
		void AddSegment(MDObjectPtr Segment);

		//! Perform an index table look-up
		IndexPosPtr Lookup(Position EditUnit, Uint32 SubItem = 0, bool Reorder = false);

		//! Free memory by purging the specified range from the index
		void Purge(Uint64 FirstPosition, Uint64 LastPosition);
	};

	//! Smart pointer to an index table
	typedef SmartPtr<IndexTable> IndexTablePtr;
}

#endif MXFLIB__INDEX_H
