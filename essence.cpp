/*! \file	essence.cpp
 *	\brief	Implementation of classes that handle essence reading and writing
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


//! Constructor
GCWriter::GCWriter(MXFFilePtr File, Uint32 BodySID /*=0*/)
{
	LinkedFile = File;
	TheBodySID = BodySID;

	StreamCount = 0;
	StreamTableSize = 16;
	StreamTable = new GCStreamData[16];

	KAGSize = 1;
	ForceFillerBER4 = false;

	NextWriteOrder = 0;

	// Don't use index tables unless requested
	UseIndex = false;
	EditUnit = 0;
	StreamOffset = 0;
}


//! Define a new system element for this container
GCStreamID GCWriter::AddSystemElement(bool CPCompatible, unsigned int RegistryDesignator, unsigned int SchemeID, unsigned int ElementID, unsigned int SubID /*=0*/)
{
	// This will be returned as an error if all goes wrong
	GCStreamID ID = -1;

	// Allocate a new ID and increase the count
	ID = StreamCount++;
	if(StreamTableSize <= ID)
	{
		// Build a new table double the current size
		GCStreamData *NewTable = new GCStreamData[StreamTableSize * 2];

		// Copy the old data into the new table
		memcpy(NewTable, StreamTable, StreamTableSize * sizeof(GCStreamData));

		// Delete the old table
		delete[] StreamTable;

		// Set the table and record the new size
		StreamTable = NewTable;
		StreamTableSize *= 2;
	}

	// Build a pointer to the new data block
	GCStreamData *Stream = &StreamTable[ID];

	// Set the item type
	if(CPCompatible) Stream->Type = 0x04; else Stream->Type = 0x14;

	// Set the key items
	Stream->RegVer = 1;
	Stream->RegDes = RegistryDesignator;

	Stream->SchemeOrCount = SchemeID;
	Stream->Element = ElementID;
	Stream->SubOrNumber = SubID;

	// Not used with system items
	Stream->CountFixed = false;

	// "Default" system item write order:
	//  0000100s 10SSSSSS Seeeeeee 0nnnnnnn
	// Where:
	//   TTTTTTT = Type (GC types mapped to CP versions)
	//         s = 0 for CP, 1 for GC
	//	 SSSSSSS = Scheme ID
	//   eeeeeee = Element ID
	//	 nnnnnnn = Sub ID
	//

	if(CPCompatible) Stream->WriteOrder = 0x08800000; else Stream->WriteOrder = 0x09800000;

	Stream->WriteOrder |= (Stream->SchemeOrCount << 15) | (Stream->Element << 8) | Stream->SubOrNumber;

	return ID;
}


//! Define a new essence element for this container
GCStreamID GCWriter::AddEssenceElement(unsigned int EssenceType, unsigned int ElementType)
{
	// This will be returned as an error if all goes wrong
	GCStreamID ID = -1;

	// Allocate a new ID and increase the count
	ID = StreamCount++;
	if(StreamTableSize <= ID)
	{
		// Build a new table double the current size
		GCStreamData *NewTable = new GCStreamData[StreamTableSize * 2];

		// Copy the old data into the new table
		memcpy(NewTable, StreamTable, StreamTableSize * sizeof(GCStreamData));

		// Delete the old table
		delete[] StreamTable;

		// Set the table and record the new size
		StreamTable = NewTable;
		StreamTableSize *= 2;
	}

	// Build a pointer to the new data block
	GCStreamData *Stream = &StreamTable[ID];

	// Set the item type
	Stream->Type = EssenceType;

	// Set the key items
	Stream->RegVer = 1;
	Stream->RegDes = 0x02;		// All essence items are "essence items"

	// Count the number of elements of this type
	int Count = 1;		// Start by counting us
	int i;
	for(i=0; i<ID; i++)
	{
		// DRAGONS: Should we allow duplicates for same essence types of different element types?
		if((StreamTable[i].Type == EssenceType) /*&& (StreamTable[i].Element == ElementType)*/)
		{
			Count++;
		}
	}

	Stream->SchemeOrCount = Count;
	Stream->Element = ElementType;
	Stream->SubOrNumber = Count;
	Stream->CountFixed = false;

	bool CPCompatible = false;
	Uint8 Type = EssenceType;
	switch(Type)
	{
		case 0x04: CPCompatible = true; break;		// CP System
		case 0x05: CPCompatible = true; break;		// CP Picture
		case 0x06: CPCompatible = true; break;		// CP Sound
		case 0x07: CPCompatible = true; break;		// CP Data
		case 0x14: Type = 0x04; break;				// Treat GC System as "System"
		case 0x15: Type = 0x05; break;				// Treat GC Picture as "Picture"
		case 0x16: Type = 0x06; break;				// Treat GC Sound as "Sound"
		case 0x17: Type = 0x07; break;				// Treat GC Data as "Data"
		case 0x18: Type = 0x08; break;				// Treat GC Compound as "Compound"
													// (even though there is no CP-Compound)
	}

	// "Default" essence item write order:
	//  TTTTTTTs 10eeeeee e0000000 0nnnnnnn
	// Where:
	//   TTTTTTT = Type (GC types mapped to CP versions)
	//         s = 0 for CP, 1 for GC
	//   eeeeeee = Element ID
	//	 nnnnnnn = Element Number

	if(CPCompatible) Stream->WriteOrder = 0x00800000; else Stream->WriteOrder = 0x01800000;

	Stream->WriteOrder |= (Type << 25) | (Stream->SchemeOrCount << 15) | Stream->SubOrNumber;

	return ID;
}


//! Add system item data to the current CP
void GCWriter::AddSystemData(GCStreamID ID, Uint64 Size, const Uint8 *Data)
{
	//! Template for all GC system item keys
	/*! DRAGONS: Version number is hard coded as 1 */
	static const Uint8 GCSystemKey[12] = { 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x00, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01 };

	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::AddSystemData()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	// Set up a new buffer big enough for the key, a huge BER length and the data
	Uint8 *Buffer = new Uint8[16 + 9 + Size];

	// Copy in the key template
	memcpy(Buffer, GCSystemKey, 12);

	// Set up the rest of the key
	Buffer[5] = Stream->RegDes;
	Buffer[7] = Stream->RegVer;
	Buffer[12] = Stream->Type;
	Buffer[13] = Stream->SchemeOrCount;
	Buffer[14] = Stream->Element;
	Buffer[15] = Stream->SubOrNumber;

	// Add the length and work out the start of the data field
	DataChunkPtr BER = MakeBER(Size);
	memcpy(&Buffer[16], BER->Data, BER->Size);
	int ValStart = 16 + BER->Size;

	// Copy the value into the buffer
	memcpy(&Buffer[ValStart], Data, Size);

	// Add this item to the write queue (the writer will free the memory)
	WriteBlock WB;
	WB.Size = Size + ValStart;
	WB.Buffer = Buffer;
	WB.Source = NULL;

	WriteQueue.insert(WriteQueueMap::value_type(Stream->WriteOrder, WB));
}


//! Add essence item data to the current CP
void GCWriter::AddEssenceData(GCStreamID ID, Uint64 Size, const Uint8 *Data)
{
	//! Template for all GC essence item keys
	/*! DRAGONS: Version number is hard coded as 1 */
	static const Uint8 GCSystemKey[12] = { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x00, 0x0d, 0x01, 0x03, 0x01 };

	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::AddEssenceData()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	// Set up a new buffer big enough for the key, a huge BER length and the data
	Uint8 *Buffer = new Uint8[16 + 9 + Size];

	// Copy in the key template
	memcpy(Buffer, GCSystemKey, 12);

	// Set up the rest of the key
	Buffer[7] = Stream->RegVer;
	Buffer[12] = Stream->Type;

	// If we have't yet fixed the count then update it and fix it
	if(!Stream->CountFixed)
	{
		// Count the number of elements of this type
		int Count = 1;		// Start by counting us
		int i;
		for(i=0; i<ID; i++)
		{
			// DRAGONS: Should we allow duplicates for same essence types of different element types?
			if((StreamTable[i].Type == StreamTable[ID].Type) /*&& (StreamTable[i].Element == StreamTable[ID].Element)*/)
			{
				Count++;
			}
		}

		Stream->SchemeOrCount = Count;
		Stream->SubOrNumber = Count;	// Could use Count-1, but this is clearer
		Stream->CountFixed = true;
	}

	Buffer[13] = Stream->SchemeOrCount;
	Buffer[14] = Stream->Element;
	Buffer[15] = Stream->SubOrNumber;

	// Add the length and work out the start of the data field
	DataChunkPtr BER = MakeBER(Size);
	memcpy(&Buffer[16], BER->Data, BER->Size);
	int ValStart = 16 + BER->Size;

	// Copy the value into the buffer
	memcpy(&Buffer[ValStart], Data, Size);

	// Add this item to the write queue (the writer will free the memory)
	WriteBlock WB;
	WB.Size = Size + ValStart;
	WB.Buffer = Buffer;
	WB.Source = NULL;

	WriteQueue.insert(WriteQueueMap::value_type(Stream->WriteOrder, WB));
}


//! Add an essence item to the current CP with the essence to be read from an EssenceSource object
void GCWriter::AddEssenceData(GCStreamID ID, EssenceSource* Source)
{
	//! Template for all GC essence item keys
	/*! DRAGONS: Version number is hard coded as 1 */
	static const Uint8 GCSystemKey[12] = { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x00, 0x0d, 0x01, 0x03, 0x01 };

	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::AddEssenceData()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	// Set up a new buffer big enough for the key alone - the BER length and data will be added later
	Uint8 *Buffer = new Uint8[16];

	// Copy in the key template
	memcpy(Buffer, GCSystemKey, 12);

	// Set up the rest of the key
	Buffer[7] = Stream->RegVer;
	Buffer[12] = Stream->Type;

	// If we have't yet fixed the count then update it and fix it
	if(!Stream->CountFixed)
	{
		// Count the number of elements of this type
		int Count = 1;		// Start by counting us
		int i;
		for(i=0; i<ID; i++)
		{
			// DRAGONS: Should we allow duplicates for same essence types of different element types?
			if((StreamTable[i].Type == StreamTable[ID].Type) /*&& (StreamTable[i].Element == StreamTable[ID].Element)*/)
			{
				Count++;
			}
		}

		Stream->SchemeOrCount = Count;
		Stream->SubOrNumber = Count;	// Could use Count-1, but this is clearer
		Stream->CountFixed = true;
	}

	Buffer[13] = Stream->SchemeOrCount;
	Buffer[14] = Stream->Element;
	Buffer[15] = Stream->SubOrNumber;

	// Add this item to the write queue (the writer will free the memory and the EssenceSource)
	WriteBlock WB;
	WB.Size = 16;
	WB.Buffer = Buffer;
	WB.Source = Source;

	WriteQueue.insert(WriteQueueMap::value_type(Stream->WriteOrder, WB));
}



//! Get the track number associated with the specified essence stream
/*! \note Once this function has been called for a stream, or an element
 *		  of the stream has been written, the value of "EssenceElementCount"
 *		  will be frozen, even if new elements are added. 
 *		  (See SMPTE-379M section 7.1)
 *	\note Unusual results are likely if called with the ID of a system item!
 */
Uint32 GCWriter::GetTrackNumber(GCStreamID ID)
{
	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::AddEssenceData()\n");
		return 0;
	}
	GCStreamData *Stream = &StreamTable[ID];

	// If we have't yet fixed the count then update it and fix it
	if(!Stream->CountFixed) 
	{
		// Count the number of elements of this type
		int Count = 1;		// Start by counting us
		int i;
		for(i=0; i<ID; i++)
		{
			// DRAGONS: Should we allow duplicates for same essence types of different element types?
			if((StreamTable[i].Type == StreamTable[ID].Type) /*&& (StreamTable[i].Element == StreamTable[ID].Element)*/)
			{
				Count++;
			}
		}

		Stream->SchemeOrCount = Count;
		Stream->CountFixed = true;
	}

	return (Stream->Type << 24) | (Stream->SchemeOrCount << 16) | (Stream->Element << 8) | Stream->SubOrNumber;
}


//! Start a new content package (and write out the prevous one if required)
void GCWriter::StartNewCP(void)
{
	Flush();

	EditUnit++;
}


//! Calculate how much data will be written if "Flush" is called now
Uint64 GCWriter::CalcWriteSize(void)
{
	Uint64 Ret = 0;

	//! The last type written - KAG alignment is performed between different types
	Uint8 LastType = 0xff;

	WriteQueueMap::iterator it = WriteQueue.begin();
	while(it != WriteQueue.end())
	{
		// The most significant byte is basically the item type
		Uint8 ThisType = (*it).first >> 24;

		// Add the size of any filler
		if((ThisType != LastType) && (KAGSize > 1))
		{
			Ret += LinkedFile->FillerSize(ForceFillerBER4, KAGSize);
		}

		// Add the chunk size
		Ret += (*it).second.Size;

		LastType = ThisType;

		it++;
	}

	// DRAGONS: This is a bit of a fudge to cope with new partitions 
	//          being inserted after us and that causing a filler...

	// Align to the next KAG
	if(KAGSize > 1)
	{
		Ret += LinkedFile->FillerSize(ForceFillerBER4, KAGSize);
	}

	return Ret;
}


//! Flush any remaining data
/*! \note It is important that any changes to this function are propogated to CalcWriteSize() */
void GCWriter::Flush(void)
{
	//! The last type written - KAG alignment is performed between different types
	Uint8 LastType = 0xff;

	WriteQueueMap::iterator it = WriteQueue.begin();
	while(it != WriteQueue.end())
	{
		// The most significant byte is basically the item type
		Uint8 ThisType = (*it).first >> 24;

		// Align to the next KAG
		if((ThisType != LastType) && (KAGSize > 1))
		{
			Uint64 Pos = LinkedFile->Tell();
			StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
		}

		// Build index table if requested
		// DRAGONS: Currently only a single stream can be indexed
		if(UseIndex)
		{
			if((ThisType != 0x04) && (ThisType != 0x14))
			{
				// If this is our first index entry we must configure the table
				if(Index->BaseDeltaCount == 0)
				{
					Uint32 ZeroDelta = 0;
					Index->DefineDeltaArray(1, &ZeroDelta);
				}

				// DRAGONS: Not correct yet!
//				Index->AddIndexEntry(EditUnit,0,0,0x80,StreamOffset);
			}
		}

		// Write the pre-formatted data and free its buffer
		StreamOffset += LinkedFile->Write((*it).second.Buffer, (*it).second.Size);
		delete[] (*it).second.Buffer;

		// Handle any non-buffered essence data
		if((*it).second.Source)
		{
			Uint64 Size = (*it).second.Source->GetEssenceDataSize();

			// Write out the length
			DataChunkPtr BER = MakeBER(Size);
			StreamOffset += LinkedFile->Write(*BER);

			// Write out all the data
			for(;;)
			{
				DataChunkPtr Data = (*it).second.Source->GetEssenceData();
				
				// Exit when no more data left
				if(!Data) break;

				if(Data->Size == 0)
				{
					warning("GetEssenceData returned zero bytes (request to try again later)\n");
					continue;
				}

				StreamOffset += LinkedFile->Write(*Data);
			}
		}

		WriteQueue.erase(it);
		it = WriteQueue.begin();

		LastType = ThisType;
	}


	// DRAGONS: This is a bit of a fudge to cope with new partitions 
	//          being inserted after us and that causing a filler...

	// Align to the next KAG
	if(KAGSize > 1)
	{
		Uint64 Pos = LinkedFile->Tell();
		StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
	}
}


//! Set the WriteOrder for the specified stream
/*!	When a GC Content Package is written all elements of each type are written together,
 *	with the lowest "Type" number being written first. So all system items are written first,
 *	then all picture items, then all sound items etc. Within each type the "WriteOrder" is
 *	used to determine the order that each element is written. So the system item with the
 *	lowest "WriteOrder" will be written first, then the next lowest up to the system item
 *  with the highest "WriteOrder", then the picture item with the lowest "WriteOrder".
 *	\note There are GC and CP versions of system items, picture items, sound items and
 *		  data items. These are grouped with all CP system elements first, then all GC
 *		  system elements, then all CP picture elements, all GC picture elements etc.
 *
 *	\note Elements with a WriteOrder set to be < 0x8000 will be written before elements
 *		  of streams that have not had a write order set. Elements with a WriteOrder >= 0x8000
 *		  will be written after elements of streams that have not had a write order set.
 */
void GCWriter::SetWriteOrder(GCStreamID ID, int WriteOrder /*=-1*/, int Type /*=-1*/)
{
	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::SetWriteOrder()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	// Auto WriteOrder...
	if(WriteOrder == -1)
	{
		if(NextWriteOrder >= 0xffff)
		{
			error("Auto WriteOrder > 0xffff in GCWriter::SetWriteOrder()\n");
		}

		WriteOrder = NextWriteOrder++;
	}

	// Auto type order
	if(Type == -1) 
	{
		Type = Stream->Type;
	}

	// Sort the CP/GC ordering
	bool CPCompatible = false;
	switch(Type)
	{
		case 0x04: CPCompatible = true; break;		// CP System
		case 0x05: CPCompatible = true; break;		// CP Picture
		case 0x06: CPCompatible = true; break;		// CP Sound
		case 0x07: CPCompatible = true; break;		// CP Data
		case 0x14: Type = 0x04; break;				// Treat GC System as "System"
		case 0x15: Type = 0x05; break;				// Treat GC Picture as "Picture"
		case 0x16: Type = 0x06; break;				// Treat GC Sound as "Sound"
		case 0x17: Type = 0x07; break;				// Treat GC Data as "Data"
		case 0x18: Type = 0x08; break;				// Treat GC Compound as "Compound"
													// (even though there is no CP-Compound)
	}

	// Set the new write order
	Stream->WriteOrder = (Type << 25) | ((WriteOrder & 0x0000ffff) << 6);

	// Add bits for CP/GC ordering
	if(!CPCompatible) Stream->WriteOrder |= 0x01000000;

	// Add bits to move the write order to after the "default" order if required
	if(WriteOrder & 0x8000) Stream->WriteOrder |= 0x00c00000;
}


// Manually set write order:
//  TTTTTTTs XXWWWWWW WWWWWWWW WW000000
// Where:
//   TTTTTTT = Type (GC types mapped to CP versions)
//         s = 0 for CP, 1 for GC
//		  XX = MSB of Write Order (2 copies of)
//   WW...WW = Write order (as specified or last + 1 for auto)
//
// "Default" system item write order:
//  0000100s 10SSSSSS Seeeeeee 0nnnnnnn
// Where:
//   TTTTTTT = Type (GC types mapped to CP versions)
//         s = 0 for CP, 1 for GC
//	 SSSSSSS = Scheme ID
//   eeeeeee = Element ID
//	 nnnnnnn = Sub ID
//
// "Default" essence item write order:
//  TTTTTTTs 10eeeeee e0000000 0nnnnnnn
// Where:
//   TTTTTTT = Type (GC types mapped to CP versions)
//         s = 0 for CP, 1 for GC
//   eeeeeee = Element ID
//	 nnnnnnn = Element Number
//
// Note: Many items are 7-bit because they are short-form OID encoded in keys
//



// Build an essence parser with all known sub-parsers
EssenceParser::EssenceParser()
{
	//! Add one instance of all known essence parsers
	EPList.push_back(new MPEG2_VES_EssenceSubParser);
	EPList.push_back(new WAVE_PCM_EssenceSubParser);
	EPList.push_back(new DV_DIF_EssenceSubParser);
}


