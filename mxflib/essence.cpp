/*! \file	essence.cpp
 *	\brief	Implementation of classes that handle essence reading and writing
 *
 *	\version $Id: essence.cpp,v 1.1.2.13 2004/11/05 16:50:13 matt-beard Exp $
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


//! Constructor
GCWriter::GCWriter(MXFFilePtr File, Uint32 BodySID /*=0*/, int Base /*=0*/)
{
	LinkedFile = File;
	TheBodySID = BodySID;

	StreamCount = 0;
	StreamTableSize = 16;
	StreamTable = new GCStreamData[16];
	StreamBase = Base;

	IndexEditUnit = 0;
	StreamOffset = 0;

	KAGSize = 1;
	ForceFillerBER4 = false;

	NextWriteOrder = 0;
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
		int i;
		for(i=0; i<StreamTableSize; i++) NewTable[i] = StreamTable[i];

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

	// Initially we don't index this stream
	Stream->IndexMan = NULL;
	Stream->IndexFiller = 0;

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
		int i;
		for(i=0; i<StreamTableSize; i++) NewTable[i] = StreamTable[i];

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

	Stream->SchemeOrCount = Count+StreamBase;
	Stream->Element = ElementType;
	Stream->SubOrNumber = Count+StreamBase;
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

	// Initially we don't index this stream
	Stream->IndexMan = NULL;
	Stream->IndexFiller = 0;

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


//! Allow this data stream to be indexed and set the index manager
void GCWriter::AddStreamIndex(GCStreamID ID, IndexManagerPtr &IndexMan, int IndexSubStream, bool IndexFiller /*=false*/)
{
	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::AddStreamIndex()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	Stream->IndexMan = IndexMan;
	Stream->IndexSubStream = IndexSubStream;
	Stream->IndexFiller = IndexFiller;
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
	Uint8 *Buffer = new Uint8[(size_t)(16 + 9 + Size)];

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
	memcpy(&Buffer[ValStart], Data, (size_t)Size);

	// Add this item to the write queue (the writer will free the memory)
	WriteBlock WB;
	WB.Size = Size + ValStart;
	WB.Buffer = Buffer;
	WB.Source = NULL;
	WB.KLVSource = NULL;
	
	// Add the index data
	WB.IndexMan = Stream->IndexMan;
	if(WB.IndexMan)
	{
		WB.IndexSubStream = Stream->IndexSubStream;
		WB.IndexFiller = Stream->IndexFiller;
	}
	else
		WB.IndexFiller = false;

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
	Uint8 *Buffer = new Uint8[(size_t)(16 + 9 + Size)];

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

		Stream->SchemeOrCount = Count+StreamBase;
		Stream->SubOrNumber = Count+StreamBase;	// Could use Count-1, but this is clearer
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
	memcpy(&Buffer[ValStart], Data, (size_t)Size);

	// Add this item to the write queue (the writer will free the memory)
	WriteBlock WB;
	WB.Size = Size + ValStart;
	WB.Buffer = Buffer;
	WB.Source = NULL;
	WB.KLVSource = NULL;

	// Add the index data
	WB.IndexMan = Stream->IndexMan;
	if(WB.IndexMan)
	{
		WB.IndexSubStream = Stream->IndexSubStream;
		WB.IndexFiller = Stream->IndexFiller;
	}
	else
		WB.IndexFiller = false;

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

		Stream->SchemeOrCount = Count+StreamBase;
		Stream->SubOrNumber = Count+StreamBase;	// Could use Count-1, but this is clearer
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
	WB.KLVSource = NULL;

	// Add the index data
	WB.IndexMan = Stream->IndexMan;
	if(WB.IndexMan)
	{
		WB.IndexSubStream = Stream->IndexSubStream;
		WB.IndexFiller = Stream->IndexFiller;
	}
	else
		WB.IndexFiller = false;

	WriteQueue.insert(WriteQueueMap::value_type(Stream->WriteOrder, WB));
}



//! Add an essence item to the current CP with the essence to be read from a KLVObject
void GCWriter::AddEssenceData(GCStreamID ID, KLVObjectPtr Source)
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

		Stream->SchemeOrCount = Count+StreamBase;
		Stream->SubOrNumber = Count+StreamBase;	// Could use Count-1, but this is clearer
		Stream->CountFixed = true;
	}

	Buffer[13] = Stream->SchemeOrCount;
	Buffer[14] = Stream->Element;
	Buffer[15] = Stream->SubOrNumber;

	// Add this item to the write queue (the writer will free the memory and the EssenceSource)
	WriteBlock WB;
	WB.Size = 16;
	WB.Buffer = Buffer;
	WB.Source = NULL;
	WB.KLVSource = Source;

	// Add the index data
	WB.IndexMan = Stream->IndexMan;
	if(WB.IndexMan)
	{
		WB.IndexSubStream = Stream->IndexSubStream;
		WB.IndexFiller = Stream->IndexFiller;
	}
	else
		WB.IndexFiller = false;

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
		error("Unknown stream ID in GCWriter::GetTrackNumber()\n");
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

		Stream->SchemeOrCount = Count+StreamBase;
		Stream->CountFixed = true;
	}

	return (Stream->Type << 24) | (Stream->SchemeOrCount << 16) | (Stream->Element << 8) | Stream->SubOrNumber;
}


//! Start a new content package (and write out the prevous one if required)
void GCWriter::StartNewCP(void)
{
	Flush();
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

		// Add any KLVObject-buffered essence data
		if((*it).second.KLVSource)
		{
			Length Size = (*it).second.KLVSource->GetLength();
			DataChunkPtr BER = MakeBER(Size);
			Ret += BER->Size + Size;
		}
		// Add any non-buffered essence data
		else if((*it).second.Source)
		{
			Length Size = (*it).second.Source->GetEssenceDataSize();
			DataChunkPtr BER = MakeBER(Size);
			Ret += BER->Size + Size;
		}

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
			// If we are indexing filler then send this offset to the index manager - even if we write 0 bytes 
			if((*it).second.IndexFiller)
			{
				// Send this stream offset to index stream -1 to signify filler
				if((*it).second.IndexMan) (*it).second.IndexMan->OfferOffset(-1, IndexEditUnit, StreamOffset);
			}

			Uint64 Pos = LinkedFile->Tell();
			StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
		}

		// Index this item (if we are indexing)
		// TODO: This doesn't take account of clip-wrapping
		if((*it).second.IndexMan) (*it).second.IndexMan->OfferOffset((*it).second.IndexSubStream, IndexEditUnit, StreamOffset);

		// Write the pre-formatted data and free its buffer
		StreamOffset += LinkedFile->Write((*it).second.Buffer, (Uint32)((*it).second.Size));
		delete[] (*it).second.Buffer;

		// Handle any KLVObject-buffered essence data
		if((*it).second.KLVSource)
		{
			Uint64 Size = (*it).second.KLVSource->GetLength();

			// Write out the length
			DataChunkPtr BER = MakeBER(Size);
			StreamOffset += LinkedFile->Write(*BER);

			// Write out all the data
			Position Offset = 0;
			for(;;)
			{
				const int ReadChunkSize = 128 * 1024;
				Length Bytes = (*it).second.KLVSource->ReadDataFrom(Offset, ReadChunkSize);
				Offset += Bytes;

				// Exit when no more data left
				if(!Bytes) break;

				StreamOffset += LinkedFile->Write((*it).second.KLVSource->GetData());
			}
		}
		// Handle any non-buffered essence data
		else if((*it).second.Source)
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

	// DRAGONS: Note that we don't index the last filler - will this cause problems?

	// Align to the next KAG
	if(KAGSize > 1)
	{
		Uint64 Pos = LinkedFile->Tell();
		StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
	}

	// Increment edit unit
	// TODO: This doesn't take account of non-frame wrapping index calculations
	IndexEditUnit++;
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



//! Calculate how many bytes would be written if the specified object were written with WriteRaw()
Length GCWriter::CalcRawSize(KLVObjectPtr Object)
{
	Length Ret = 0;

	// Add the size of any filler
	if(KAGSize > 1)
	{
		Ret += LinkedFile->FillerSize(ForceFillerBER4, KAGSize);
	}

	// Add the chunk size
	Ret += Object->GetKLSize() + Object->GetLength();


	// DRAGONS: This is a bit of a fudge to cope with new partitions 
	//          being inserted after us and that causing a filler...

	// Align to the next KAG
	if(KAGSize > 1)
	{
		Ret += LinkedFile->FillerSize(ForceFillerBER4, KAGSize);
	}

	return Ret;
}


//! Write a raw KLVObject to the file - this is written immediately and not buffered in the WriteQueue
void GCWriter::WriteRaw(KLVObjectPtr Object)
{
	// Align to the next KAG
	if(KAGSize > 1)
	{
		Uint64 Pos = LinkedFile->Tell();
		StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
	}

	// Set this fila and position as the destination for the KLVObject
	Object->SetDestination(LinkedFile);

	// Write the KL
	StreamOffset += Object->WriteKL();

	// Write out all the data
	Position Offset = 0;
	for(;;)
	{
		const int ReadChunkSize = 128 * 1024;
		Length Bytes = Object->ReadDataFrom(Offset, ReadChunkSize);

		// Exit when no more data left
		if(!Bytes) break;

		StreamOffset += Object->WriteDataTo(Offset);
		Offset += Bytes;
	}

	// DRAGONS: This is a bit of a fudge to cope with new partitions 
	//          being inserted after us and that causing a filler...

	// Align to the next KAG
	if(KAGSize > 1)
	{
		Uint64 Pos = LinkedFile->Tell();
		StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
	}

	return;
}





// Build an essence parser with all known sub-parsers
EssenceParser::EssenceParser()
{
	//! Add one instance of all known essence parsers
	EPList.push_back(new MPEG2_VES_EssenceSubParser);
	EPList.push_back(new WAVE_PCM_EssenceSubParser);
	EPList.push_back(new DV_DIF_EssenceSubParser);
}


//! Create a new GCReader, optionally with a given default item handler and filler handler
/*! \note The default handler receives all KLVs without a specific handler (except fillers)
 *        The filler handler receives all filler KLVs
 */
GCReader::GCReader( MXFFilePtr File, GCReadHandlerPtr DefaultHandler /*=NULL*/, GCReadHandlerPtr FillerHandler /*=NULL*/ )
	: File(File), DefaultHandler(DefaultHandler), FillerHandler(FillerHandler)
{
	FileOffset = 0;

	StopNow = false;
	StopCalled = false;
	PushBackRequested = false;

	StreamOffset = 0;
}


//! Read from file
/*! All KLVs are dispatched to handlers
 *  Stops reading at the next partition pack unless SingleKLV is true when only one KLV is dispatched
 *  \return true if all went well, false end-of-file, an error occured or StopReading() was called
 */
bool GCReader::ReadFromFile(bool SingleKLV /*=false*/)
{
	// Seek to the offset of the "next" KLV
	ASSERT(File);
	File->Seek(FileOffset);

	// Force us to stop as soon as we have read a single KLV if requested
	StopNow = SingleKLV;
	StopCalled = false;

	// Read and dispatch until requested to stop
	do
	{
		// Get the next KLV
		KLVObjectPtr Object = File->ReadKLV();

		// Exit if we failed
		if(!Object)	return false;

		// Is this a partition pack?
		if(IsPartitionKey(Object->GetUL()->GetValue())) return true;

		// Handle the data
		bool Ret = HandleData(Object);
		
		// Perform a pushback (if requested) by seeking to the start of this KLV and not updating offsets
		if(StopCalled && PushBackRequested)
		{
			File->Seek(FileOffset);
		}
		else
		{
			// Advance to the start of the next KLV and update stream offset 
			Length Size = Object->GetKLSize() + Object->GetLength();
			FileOffset += Size;
			StreamOffset += Size;
		}

		// Abort if the handler errored
		if(!Ret) return false;

		// Seek to the next KLV
		File->Seek(FileOffset);

	} while(!StopNow);

	// We will drop out of the loop on two conditions:
	//    1) A single shot was requested by setting SingleKLV = true
	//    2) StopNow was set by a call to StopReading()

	// Return error status if StopReading() was called
	return StopCalled;
}


//! Force a KLVObject to be handled
/*! \note This is not the normal way that the GCReader is used, but allows the encryption handler
/*        to push the decrypted data back to the GCReader to pass to the appropriate handler
 *  \return true if all OK, false on error 
 */
bool GCReader::HandleData(KLVObjectPtr Object)
{
	// First check is this KLV is a filler
	// We first check if byte 9 == 3 which is true for filler keys, but is
	// false for all GC sets and packs. Once this matches we can do a full memcmp.
	if(Object->GetUL()->GetValue()[8] == 3)
	{
		const Uint8 FillerKey[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x02, 0x10, 0x01, 0x00, 0x00, 0x00 };
		if( memcmp(Object->GetUL()->GetValue(), FillerKey, 16) == 0 )
		{
			if(FillerHandler) return FillerHandler->HandleData(this, Object);
			else return true;
		}
	}

	// Next check if this KLV is encrypted essence data - but only if we have an encryption handler
	if(EncryptionHandler)
	{
		// We first check if byte 6 == 4 (variable pack) which is true for encrypted data keys,
		// but is false for standard GC sets and packs. Once this matches we can do a full memcmp.
		if(Object->GetUL()->GetValue()[5] == 4)
		{
			const Uint8 EncryptedKey[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x02, 0x04, 0x01, 0x07, 0x0f, 0x01, 0x03, 0x7f, 0x01, 0x00, 0x00, 0x00 };
			if( memcmp(Object->GetUL()->GetValue(), EncryptedKey, 16) == 0 )
			{
				return EncryptionHandler->HandleData(this, Object);
			}
		}
	}

	// Get the track-number of this GC item (or zero if not GC)
	// Note that we don't bother if no handlers have been registered 
	// because we will have to use the defualt handler whatever!
	Uint32 TrackNumber; 
	if(!Handlers.size()) TrackNumber = 0; else TrackNumber = Object->GetGCTrackNumber();

	if( TrackNumber != 0 )
	{
		// See if we have a handler registered for this track
		std::map<Uint32, GCReadHandlerPtr>::iterator it = Handlers.find(TrackNumber);

		// If so use that handler
		if(it != Handlers.end()) return (*it).second->HandleData(this, Object);
	}

	// By this point we only have the default handler left
	if(DefaultHandler) return DefaultHandler->HandleData(this, Object);

	// No available handler!  Discard the data
	return true;
}


//! Stop reading even though there appears to be valid data remaining
/*! This function can be called from a handler if it detects that the current KLV is either the last
 *  KLV in partition, or does not belong in this partition at all.  If the KLV belongs to another
 *  partition, or handling should be deferred for some reason, PushBackKLV can be set to true
 */
void GCReader::StopReading(bool PushBackKLV /*=false*/)
{
	StopNow = true;

	PushBackRequested = PushBackKLV;
}



//! Construct a body reader and associate it with an MXF file
BodyReader::BodyReader(MXFFilePtr File)
	: File(File)
{
	CurrentPos = 0;					// Start at the beginning
	NewPos = true;					// Force reading to be initialized
	
	AtPartition = false;			// We don't know if we are at a partition
	AtEOF = false;					// We don't know if we are at the end of the file

	CurrentBodySID = 0;				// We don't know what BodySID we are now in
};


//! Seek to a specific point in the file
/*! \return New location or -1 on seek error
 */
Position BodyReader::Seek(Position Pos /*=0*/)
{
	File->Seek(Pos);				// Move the file pointer
	CurrentPos = File->Tell();		// Find out where we ended up
	NewPos = true;					// Force reading to be reinitialized

	AtPartition = false;			// We don't know if we are at a partition
	AtEOF = false;					// We don't know if we are at the end of the file

	CurrentBodySID = 0;				// We don't know what BodySID we are now in
	
	return CurrentPos;				// Return the new position
}



//! Seek to a specific byte offset in a given stream
/*! \return New file offset or -1 on seek error
 */
Position BodyReader::Seek(Uint32 BodySID, Position Pos)
{
	error("BodyReader::Seek() per BodySID not currently supported\n");
	return -1;
}


//! Are we currently at the start of a partition pack?
bool BodyReader::IsAtPartition(void)
{
	// Return true if we know we are at a partition
	if(AtPartition) return true;

	// Otherwise read the next key to find out
	File->Seek(CurrentPos);
	ULPtr ThisUL = File->ReadKey();

	// Can't be true if we can't read a key
	if(!ThisUL) return false;

	// So - is this a partition pack?
	return IsPartitionKey(ThisUL->GetValue());
}


//! Are we currently at the end of the file?
bool BodyReader::Eof(void)
{
	// Return true if we know we are the end of the file
	if(AtEOF) return true;

	// Otherwise try and find out
	File->Seek(CurrentPos);
	
	// Did the seek set EOF?
	if(File->Eof()) 
	{
		AtEOF = true;
		return true;
	}

	// Did the seek place us earlier than we requested? (almost certainly because the file is shorter)
	if((Position)File->Tell() < CurrentPos) 
	{
		AtEOF = true;
		return true;
	}

	return false;
}



//! Make a GCReader for the specified BodySID
/*! \return true on success, false on error (such as there is already a GCReader for this BodySID)
 */
bool BodyReader::MakeGCReader(Uint32 BodySID, GCReadHandlerPtr DefaultHandler /*=NULL*/, GCReadHandlerPtr FillerHandler /*=NULL*/)
{
	// Don't try to make two readers for the same SID
	if(GetGCReader(BodySID)) return false;

	// Make the new reader
	GCReaderPtr Reader = new GCReader(File, DefaultHandler ? DefaultHandler : GCRDefaultHandler, FillerHandler ? FillerHandler : GCRFillerHandler);
	if(!Reader) return false;

	// Set the encryption handler if one is configured
	if(GCREncryptionHandler) Reader->SetEncryptionHandler(GCREncryptionHandler);
	
	// Insert into the map
	Readers[BodySID] = Reader;

	return true;
}



//! Read from file
/*! All KLVs are dispatched to handlers
 *  Stops reading at the next partition pack unless SingleKLV is true when only one KLV is dispatched
 *  \return true if all went well, false end-of-file, an error occured or StopReading() 
 *          was called on the current GCReader
 */
bool BodyReader::ReadFromFile(bool SingleKLV /*=false*/)
{
	bool Ret;
	GCReaderPtr Reader;

	// First check if we need to re-initialise
	if(NewPos)
	{
		// Start at the new location
		File->Seek(CurrentPos);

		PartitionPtr NewPartition;				// Pointer to the new partition pack
		for(;;)
		{
			// Use resync to locate the next partition pack
			// TODO: We could allow reinitializing within a partition if we can validate the offsets
			//       This would involve knowledge of the partition pack for this partition which could
			//       be found by a valid RIP or by scanning backwards from the current location
			if(!ReSync()) return false;

			// Read the partition pack to establish offsets and BodySID
			NewPartition = File->ReadPartition();
			if(!NewPartition) return false;

			CurrentBodySID = NewPartition->GetUint("BodySID");
			if(CurrentBodySID != 0) Reader = GetGCReader(CurrentBodySID);
		
			// All done when we have found a supported BodySID
			if(Reader) break;

			// Skip non-supported essence
			// We first index the start of the essence data, then the loop causes a re-sync
			// TODO: Add faster skipping of unwanted body partitions if we have enough RIP data...
			NewPartition->SeekEssence();
			CurrentPos = File->Tell();
			AtPartition = false;
		}

		// Set the stream offset
		Position StreamOffset = NewPartition->GetUint64("BodyOffset");
		
		// Index the start of the essence data
		NewPartition->SeekEssence();

		// Read and handle data
		Ret = Reader->ReadFromFile(File->Tell(), StreamOffset, SingleKLV);

		// We have now initialized the reader
		NewPos = false;
	}
	else
	{
		// Continue from the previous read 
		Reader = GetGCReader(CurrentBodySID);
		if(!Reader) return true;
		Ret = Reader->ReadFromFile(SingleKLV);
	}

	CurrentPos = Reader->GetFileOffset();
	AtPartition = false;						// We don't KNOW we are at a partition pack now

	// If the read failed (or was stopped) reinitialize next time around
	if(Ret) NewPos = true;
	else
	{
		// Also reinitialize next time if we are at the end of this partition
		File->Seek(CurrentPos);
		if(IsAtPartition()) NewPos = true;
	}

	return Ret;
}


//! Resync after possible loss or corruption of body data
/*! Searches for the next partition pack and moves file pointer to that point
 *  \return false if an error (or EOF found)
 */
bool BodyReader::ReSync()
{
	// Do we actually need to resync?
	if(IsAtPartition())
	{
		File->Seek(CurrentPos);
		return true;
	}

	// Loop around until we have re-synced
	for(;;)
	{
		// Read the next key to see if we are yet in sync
		File->Seek(CurrentPos);
		ULPtr ThisUL = File->ReadKey();

		// Fail if we can't read a key
		if(!ThisUL) return false;

		// Validate the start of the key (to see if it is a standard MXF key)
		const Uint8 *Key = ThisUL->GetValue();
		if((Key[0] == 0x06) && (Key[1] == 0x0e) && (Key[2] == 0x2b) && (Key[3] == 34))
		{
			// It seems to be a key - is it a partition pack key? If so we are bac in sync
			if(IsPartitionKey(Key))
			{
				File->Seek(CurrentPos);
				AtPartition= true;
				NewPos = true;							// Force read to be reinitialized
				return true;
			}

			// Skip over this key...
			Length Len = File->ReadBER();
			if(Len < 0) return false;

			CurrentPos += Len + 16;

			continue;
		}

		// At this point we have read a key that does not start with the same 4 bytes as standard MXF keys
		// This could mean that we have found a valid non-SMPTE key, or that we have encountered a portion
		// of the file where data is missing or corrupted.  We not try a byte-by-byte search for a partition
		// key

		for(;;)
		{
			// Scan 64k at a time
			const Uint64 BufferLen = 1024 * 65536;
			DataChunkPtr Buffer = File->Read(BufferLen);
			if(Buffer->Size < 16) return false;

			Int32 i;									// Loop variable
			Int32 End = Buffer->Size - 15;				// End of loop - 15 bytes early to allow 16-byte compares
			Uint8 *p = Buffer->Data;						// Use moving pointer for faster compares
			for(i=0; i<End; i++)
			{
				// Only perform full partition key check if it looks promising
				if((*p == 0x06) && (p[1] == 0x0e))
				{
					if(IsPartitionKey(p))
					{
						File->Seek(CurrentPos);
						CurrentPos += i;				// Move pointer to new partition pack
						NewPos = true;					// Force read to be reinitialized
						AtPartition= true;
		
						return true;
					}
				}
				p++;
			}

			CurrentPos += End;
			File->Seek(CurrentPos);
		}
	}
}


//! Initialize the per SID seek system
/*! To allow us to seek to byte offsets within a file we need to initialize 
 *  various structures - seeking is not always possible!!
 *  \return False if seeking could not be initialized (perhaps because the file is not seekable)
 */
// bool BodyReader::InitSeek(void);



//! Get a GCElementKind structure
GCElementKind mxflib::GetGCElementKind(ULPtr TheUL)
{
	GCElementKind ret;

	//! Base of all standard GC keys
	/*! DRAGONS: version number is hard-coded as 1 */
	const Uint8 DegenerateGCLabel[12] = { 0x06, 0x0E, 0x2B, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01 };
	
	// Note that we first test the 11th byte as this where "Application = MXF Generic Container Keys"
	// is set and so is the same for all GC keys and different in the majority of non-CG keys
	if( ( TheUL->GetValue()[10] == DegenerateGCLabel[10] ) && (memcmp(TheUL->GetValue(), DegenerateGCLabel, 12) == 0) )
	{
		ret.IsValid =			true;
		ret.Item =				(TheUL->GetValue())[12];
		ret.Count =				(TheUL->GetValue())[13];
		ret.ElementType = (TheUL->GetValue())[14];
		ret.Number =			(TheUL->GetValue())[15];
	}
	else
		ret.IsValid =			false;

	return ret;
}



//! Get the track number of this essence key (if it is a GC Key)
/*! \return 0 if not a valid GC Key
 */
Uint32 mxflib::GetGCTrackNumber(ULPtr TheUL)
{
	//! Base of all standard GC keys
	/*! DRAGONS: version number is hard-coded as 1 */
	const Uint8 DegenerateGCLabel[12] = { 0x06, 0x0E, 0x2B, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01 };
	
	// Note that we first test the 11th byte as this where "Application = MXF Generic Container Keys"
	// is set and so is the same for all GC keys and different in the majority of non-CG keys
	if( ( TheUL->GetValue()[10] == DegenerateGCLabel[10] ) && (memcmp(TheUL->GetValue(), DegenerateGCLabel, 12) == 0) )
	{
		return (Uint32(TheUL->GetValue()[12]) << 24) | (Uint32(TheUL->GetValue()[13]) << 16) 
			 | (Uint32(TheUL->GetValue()[14]) << 8) | Uint32(TheUL->GetValue()[15]);
	}
	else
		return 0;
}



//! Build a list of parsers with their descriptors for a given essence file
ParserDescriptorListPtr EssenceParser::IdentifyEssence(FileHandle InFile)
{
	ParserDescriptorListPtr Ret = new ParserDescriptorList;

	EssenceParserList::iterator it = EPList.begin();
	while(it != EPList.end())
	{
		EssenceSubParserPtr EP = (*it)->NewParser();
		EssenceStreamDescriptorList DescList = EP->IdentifyEssence(InFile);
		
		if(!DescList.empty())
		{
			Ret->push_back(ParserDescriptorPair(EP, DescList));
		}

		it++;
	}

	return Ret;
}


//! Select the best wrapping option
EssenceParser::WrappingConfigPtr EssenceParser::SelectWrappingOption(FileHandle InFile, ParserDescriptorListPtr PDList, Rational ForceEditRate, WrappingOption::WrapType ForceWrap /*=WrappingOption::None*/)
{
	WrappingConfigPtr Ret;

	// No options!
	if(PDList->empty()) return Ret;

	// Identify the wrapping options for each descriptor
	ParserDescriptorList::iterator pdit = PDList->begin();
	while(pdit != PDList->end())
	{
		EssenceStreamDescriptorList::iterator it = (*pdit).second.begin();
		while(it != (*pdit).second.end())
		{
			WrappingOptionList WO = (*pdit).first->IdentifyWrappingOptions(InFile, (*it));

			WrappingOptionList::iterator it2 = WO.begin();
			while(it2 != WO.end())
			{
				// Only accept wrappings of the specified type
				if(ForceWrap != WrappingOption::None)
				{
					if((*it2)->ThisWrapType != ForceWrap)
					{
						it2++;
						continue;
					}
				}

				Ret = new WrappingConfig;

				// DRAGONS: Default to the first valid option!
				Ret->EssenceDescriptor = (*it).Descriptor;
				MDObjectPtr SampleRate = Ret->EssenceDescriptor["SampleRate"];

				if((!SampleRate) || (ForceEditRate.Numerator != 0))
				{
					Ret->EditRate.Numerator = ForceEditRate.Numerator;
					Ret->EditRate.Denominator = ForceEditRate.Denominator;
				}
				else
				{
					Rational Preferred = (*it2)->Handler->GetPreferredEditRate();

					// No preferrred rate given so use the sample rate
					if(Preferred.Numerator == 0)
					{
						Ret->EditRate.Numerator = SampleRate->GetInt("Numerator");
						Ret->EditRate.Denominator = SampleRate->GetInt("Denominator");
					}
					else
					{
						Ret->EditRate = Preferred;
					}
				}

				Ret->Parser = (*it2)->Handler;
				Ret->WrapOpt = (*it2);
				Ret->Stream = (*it).ID;

				Ret->Parser->Use(Ret->Stream, Ret->WrapOpt);
				if( Ret->Parser->SetEditRate(Ret->EditRate) )
				{
					// All OK, including requested edit rate

					// Update the SampleRate in the Descriptor to the rate in use (which may be different than its native rate)
					if(!SampleRate) SampleRate = Ret->EssenceDescriptor->AddChild("SampleRate");
					if(SampleRate)
					{
						SampleRate->SetInt("Numerator", Ret->EditRate.Numerator);
						SampleRate->SetInt("Denominator", Ret->EditRate.Denominator);
					}

					Ret->WrapOpt->BytesPerEditUnit = Ret->Parser->GetBytesPerEditUnit();

					return Ret;
				}

				// We failed to match - scrub the part made config
				Ret = NULL;

				it2++;
			}
			it++;
		}
		pdit++;
	}

	return Ret;
}



//! Write a complete partition's worth of essence
/*! Will stop if:
 *    Frame or "other" wrapping and the "StopAfter" reaches zero or "Duration" reaches zero
 *    Clip wrapping and the entire clip is wrapped
 */
Length BodyWriter::WriteEssence(StreamInfoPtr &Info, Length Duration /*=0*/, Length MaxPartitionSize /*=0*/)
{
	// Number of edit units processed
	Length Ret = 0;

	// Index the current stream
	BodyStreamPtr &Stream = Info->Stream;

	// Get the writer fro this stream
	GCWriterPtr &Writer = Stream->GetWriter();

	// Work out which KAG to use, either the default one or the stream specific
	Uint32 UseKAG;
	if(Stream->GetKAG()) UseKAG = Stream->GetKAG(); else UseKAG = KAG;

	// If either setting is to force BER4 we will force it
	bool UseForceBER4 = ForceBER4 || Stream->GetForceBER4();

	// Set the KAG for this Essence Container
	Writer->SetKAG(UseKAG, UseForceBER4);

	// Get a pointer to the index manager (if there is one)
	IndexManagerPtr &IndexMan = Stream->GetIndexManager();

	// Set flag if we need to build a VBR index table
	bool VBRIndex;
	if((Stream->GetIndexType() != BodyStream::IndexType::StreamIndexNone) && (IndexMan) && (!IndexMan->IsCBR()))
		VBRIndex = true;
	else
		VBRIndex = false;

	// Set flag if we need to add a sparse index entry this time (cleared once the sparse entry has been added).
	// If the previous pass for this stream left some data pending then it will already be indexed and so there
	// is no need to index the first entry of this pass.
	bool SparseIndex;
	if(VBRIndex && (!Stream->HasPendingData()) 
	   && (Stream->GetIndexType() 
	       & (BodyStream::IndexType::StreamIndexSparseFooter 
		      | BodyStream::IndexType::StreamIndexSparseFooterIsolated)))
		SparseIndex = true;
	else
		SparseIndex = false;

	// Sort clip-wrap if that is what we are doing
	if(Stream->GetWrapType() == BodyStream::WrapType::StreamWrapClip)
	{
		if(VBRIndex)
		{
			// Index the first edit unit of the essence for clip-wrap
			// FIXME: we need to do proper clip-wrap indexing!!
			Position EditUnit = IndexMan->AcceptProvisional();
			if(EditUnit == -1) EditUnit = IndexMan->GetLastNewEditUnit();
			Stream->SparseList.push_back(EditUnit);
		}

		// Add essence from each sub-stream to the writer
		BodyStream::iterator it = Stream->begin();
		while(it != Stream->end())
		{
			// Get the stream ID for this sub-stream
			GCStreamID EssenceID = (*it)->GetStreamID();

			Writer->AddEssenceData(EssenceID, (*it));
			it++;
		}

		// Write the current partition pack (there should be one pending if all is normal)
		if(PartitionWritePending) EndPartition();

		// Write the essence
		Writer->StartNewCP();

		// FIXME: We don't yet count the duration of the clip wrapped essence
	}
	else
	{
		// We need to check the start of the partition on the first iteration
		bool FirstIteration = true;

		// TODO: Re-write to not read a whole frame into memory if that is too big!!

		// We keep decrement a copy of the duration rather that the function parameter
		// so that we can destinguish between 0 meaning all done and 0 meaning unbounded
		Length RemainingDuration = Duration;

		// Two possible ways to exit the following loop
		bool ExitNow = false;					//!< Exit this iteration - no further checks required
		bool ExitASAP = false;					//!< Exit at the earliest valid time (for example the next edit point)

		// Loop for each frame, or field, or other wrapping-chunk
		while(!ExitNow)
		{
			// We can allow some sub-streams to finish first and still write the others, but we stop when all ended
			bool DataWrittenThisCP = false;

			// Add a chunk of essence data - unless there is already some pending
			if(!Stream->HasPendingData())
			{
				// Add this chunk of each essence sub-stream to the writer
				BodyStream::iterator it = Stream->begin();
				while(it != Stream->end())
				{
					// Read the next data for this sub-stream
					DataChunkPtr Dat = (*it)->GetEssenceData();

					// Skip any stream that is no longer returning data
					if((!Dat) || (Dat->Size == 0))
					{
						it++;
						continue;
					}

					// Do any required indexing (stream offset is updated when the content package is written)
					if(VBRIndex)
					{
						if(SparseIndex)
						{
							// Force the first edit unit to be accepted, even if provisional,
							// and add it's edit unit to the sparse list
							Position EditUnit = IndexMan->AcceptProvisional();
							if(EditUnit == -1) EditUnit = IndexMan->GetLastNewEditUnit();
							Stream->SparseList.push_back(EditUnit);
			
							// Sparce entry recorded for this partition
							SparseIndex = false;
						}
					}

					// Get the stream ID for this sub-stream
					GCStreamID EssenceID = (*it)->GetStreamID();

					// Add this chunk of essence to the writer
					Writer->AddEssenceData(EssenceID, Dat);
					DataWrittenThisCP = true;

					// Move to the next stream
					it++;
				}

				// Nothing remaining - all done
				if(!DataWrittenThisCP)
				{
					Stream->SetEndOfStream(true);
					Stream->GetNextState();
					return Ret;
				}

				// Now we have written something we must record the BodySID
				PartitionBodySID = CurrentBodySID;
			}

			// Start of the current partition
			Position PartitionStart;

			if(FirstIteration)
			{
				// If we have a pending write do it now
				if(PartitionWritePending) EndPartition();

				// Even if we didn't just write a partition pack the "template" pack should contain the start of the current partition
				PartitionStart = BasePartition->GetInt64("ThisPartition");
			}

			/* Work out if we should start a new partition before writing this data */
			/* Note that we ALWAYS write at least one iteration's worth of essence */
			if(MaxPartitionSize)
			{
				Length NewPartitionSize = (File->Tell() - PartitionStart) + Writer->CalcWriteSize();

				// Exit as soon as possible - if we are edit aligned or this is the first iteration we may burst the limit
				if(NewPartitionSize > MaxPartitionSize)
				{
					ExitASAP = true;
				}
			}

			//## FIXME: Currently we assume all wrapping is at edit rate
			Length ThisSize = 1;
			Ret += ThisSize;

			// If we are due to stop this stream after a specified duration...
			// DRAGONS: Is this right?
			// TODO: Decide if we should stop AFTER hitting the limit, or prevent us from bursting it...?
			// TODO: Also decide if we should edit-align these...
			if(Info->StopAfter)
			{
				// ... stop when we have reached the limit
				// DRAGONS: There is no "loop forever" check here, unlike the Duration version
				if(ThisSize > Info->StopAfter)
				{
					// Deactivate this stream
					Info->Active = false;

					// Flag that we have "stored" some essence for a later partition
					Stream->SetPendingData();

					// And exit as this partition is now done
					Stream->GetNextState();

					// Prevent this partition being "continued"
					PartitionDone = true;

					// Move this stream to the next state
					Stream->GetNextState();

					// Return the number of edit units processed
					return Ret;
				}

				// Otherwise keep counting down
				Info->StopAfter -= ThisSize;

				// If this has finished us then exit at the end of this iteration
				if(!Info->StopAfter)
				{
					// Prevent this partition being "continued"
					PartitionDone = true;
					ExitNow = true;
				}
			}

			// If we are due to stop this partition after a specified duration...
			if(Duration)
			{
				// ... stop when we have reached the limit...
				if(ThisSize > RemainingDuration)
				{
					// Prevent the duration counter from under-running when decremented later
					RemainingDuration = ThisSize;
					ExitASAP = true;
				}

				// Decrement the duration remaining
				RemainingDuration -= ThisSize;
			}

			// Should we exit yet?
			if(ExitASAP)
			{
				if(!FirstIteration)
				{
					if((!Stream->GetEditAlign()) || Stream->GetSource()->IsEditPoint())
					{
						// If we are building a sparse index table...
						if(VBRIndex && (Stream->GetIndexType() & (BodyStream::IndexType::StreamIndexSparseFooter | BodyStream::IndexType::StreamIndexSparseFooterIsolated)))
						{
							// ...force the first edit unit of the new partition to be accepted, even if provisional,
							// and add it's edit unit to the sparse list
							Position EditUnit = IndexMan->AcceptProvisional();
							if(EditUnit == -1) EditUnit = IndexMan->GetLastNewEditUnit();
							Stream->SparseList.push_back(EditUnit);
						}

						// Flag that we have "stored" some essence for next partition
						Stream->SetPendingData();

						// And exit as this partition is now done
						Stream->GetNextState();

						// Prevent this partition being "continued"
						PartitionDone = true;

						// Move this stream to the next state
						Stream->GetNextState();

						// Return the number of edit units processed
						return Ret;
					}
				}
			}

			// Write this chunk of essence
			Writer->StartNewCP();

			// Clear the pending flag
			Stream->SetPendingData(false);

			// First iteration is done
			FirstIteration = false;
		}
	}

	// Move this stream to the next state
	Stream->GetNextState();

	// Return the number of edit units processed
	return Ret;
}



//! Get the next state
/*! Sets State to the next state
 *  \return The next state (now the current state)
 */
BodyStream::StateType mxflib::BodyStream::GetNextState(void)
{
	switch(State)
	{
	default:
		// Reached an invalid state!!
		ASSERT(0);

	case BodyStreamStart:
		// Nothing yet done - do we need to write a header index table?
		if(StreamIndex & (StreamIndexCBRHeader | StreamIndexCBRHeaderIsolated))
		{
			State = BodyStreamHeadIndex;
			break;
		}

		// ... or a pre-body isolated CBR index table?
		if(StreamIndex & StreamIndexCBRPreIsolated)
		{
			State = BodyStreamPreBodyIndex;
			break;
		}

		/* We aren't starting with an index table, it must be essence - but with or without index? 
		 * We can't write VBR index data in the first body partition as we wont have any yet
		 * so the only test required is for CBR index data */

		if(StreamIndex & StreamIndexCBRBody) State = BodyStreamBodyWithIndex;
		else State = BodyStreamBodyNoIndex;

		break;

	case BodyStreamHeadIndex:
		// Do we need to write a pre-body isolated CBR index table?
		// DRAGONS: We currently can write a CBR index table in an isolated partition following
		//          the header (from StreamIndexCBRHeader or StreamIndexCBRHeaderIsolated) then
		//			another caused by StreamIndexCBRPreIsolated.  I think this is OK, but it
		//			may be perceived as undesired behaviour by some
		if(StreamIndex & StreamIndexCBRPreIsolated)
		{
			State = BodyStreamPreBodyIndex;
			break;
		}

		/* We aren't starting with an index table, it must be essence - but with or without index? 
		 * We can't write VBR index data in the first body partition as we wont have any yet
		 * so the only test required is for CBR index data */

		if(StreamIndex & StreamIndexCBRBody) State = BodyStreamBodyWithIndex;
		else State = BodyStreamBodyNoIndex;

		break;

	case BodyStreamPreBodyIndex:
		// We have just written a pre-body index table, it must now be the body - but with of without index?
		// DRAGONS: It is not currently possibe to have an isolated pre-body VBR index table
		//          but this may change in the future so we check all eventualities

		if(StreamIndex & StreamIndexCBRBody) State = BodyStreamBodyWithIndex;
		else if(StreamIndex & StreamIndexSprinkled) State = BodyStreamBodyWithIndex;
		else State = BodyStreamBodyNoIndex;

		break;

	case BodyStreamBodyWithIndex:
	case BodyStreamBodyNoIndex:
		// We have just written a body partition - what next?

		// How about a post-body index table?
		if(StreamIndex & (StreamIndexCBRIsolated | StreamIndexSprinkledIsolated))
		{
			State = BodyStreamPostBodyIndex;
			break;
		}

		// Maybe we are all done and need to go to the footer
		if(GetEndOfStream())
		{
			if(StreamIndex & ( StreamIndexSparseFooter | StreamIndexSparseFooterIsolated
				             | StreamIndexCBRFooter    | StreamIndexCBRFooterIsolated 
							 | StreamIndexFullFooter   | StreamIndexFullFooterIsolated ))
			{
				State = BodyStreamFootIndex;
			}
			// Check if we have any left-over sprinkles
			else if((StreamIndex & (StreamIndexSprinkled | StreamIndexSprinkledIsolated))
				    && (IndexMan && (IndexMan->GetLastNewEditUnit() >= GetNextSprinkled())))
			{
				State = BodyStreamFootIndex;
			}
			else
			{
				State = BodyStreamDone;
			}

			break;
		}

		// .. or a pre-body one for next time?
		if(StreamIndex & StreamIndexCBRPreIsolated)
		{
			State = BodyStreamPreBodyIndex;
			break;
		}

		// Must be another body partition - but do we need to enable indexing?
		// The first essence partition can't have any index segments, but later ones can
		if(State == BodyStreamBodyNoIndex)
		{
			if(StreamIndex & StreamIndexSprinkled) State = BodyStreamBodyWithIndex;
		}

		// Must be another body partition of the same type - leave it unchanged
		break;

	case BodyStreamPostBodyIndex:
		// We have just written a post-body index table - what next?

		// How about a pre-body one for next time?
		if(StreamIndex & StreamIndexCBRPreIsolated)
		{
			State = BodyStreamPreBodyIndex;
			break;
		}

		// Maybe we are all done and need to go to the footer
		if(GetEndOfStream())
		{
			if(StreamIndex & (StreamIndexFullFooter | StreamIndexFullFooterIsolated | StreamIndexCBRFooter | StreamIndexCBRFooterIsolated ))
				State = BodyStreamFootIndex;
			// Check if we have any left-over sprinkles
			else if((StreamIndex & (StreamIndexSprinkled | StreamIndexSprinkledIsolated))
				    && (IndexMan && (IndexMan->GetLastNewEditUnit() >= GetNextSprinkled())))
				State = BodyStreamFootIndex;
			else
				State = BodyStreamDone;

			break;
		}

		// Must be another body partition then - but with or without index?
		if(StreamIndex & StreamIndexCBRBody) State = BodyStreamBodyWithIndex;
		else if(StreamIndex & StreamIndexSprinkled) State = BodyStreamBodyWithIndex;
		else State = BodyStreamBodyNoIndex;

		break;

	case BodyStreamFootIndex:
	case BodyStreamDone:
		// If we have done the footer, or are already done, then all is done
		State = BodyStreamDone;

		break;
	}

	return State;
}


//! Write the file header
/*! No essence will be written, but CBR index tables will be written if required.
 *  The partition will not be "ended" if only the header partition is written
 *  meaning that essence will be added by the next call to WritePartition()
 */
void mxflib::BodyWriter::WriteHeader(bool IsClosed, bool IsComplete)
{
	if(!BasePartition)
	{
		error("No base partition pack defined before call to BodyWriter::WriteHeader()\n");
		return;
	}
	
	// Turn the partition into the correct type of header
	if(IsClosed)
		if(IsComplete)
			BasePartition->ChangeType("ClosedCompleteHeader");
		else
			BasePartition->ChangeType("ClosedHeader");
	else
		if(IsComplete)
			BasePartition->ChangeType("OpenCompleteHeader");
		else
			BasePartition->ChangeType("OpenHeader");

	// Initially there is no body data
	BasePartition->SetUint("BodySID", 0);
	BasePartition->SetUint("BodyOffset", 0);

	// Initially we haven't written any data
	PartitionBodySID = 0;

	// We have not yet written the header partition
	bool HeaderWritten = false;

	// Ensure that the first partition is a header
	PendingHeader = true;

	// If index data cannot share with metadata force the actual header to be flushed before any index data
	if(!IndexSharesWithMetadata)
	{
		// Queue the write
		PartitionWritePending = true;

		// Now the header has been written (or at least is pending)
		HeaderWritten = true;
	}

	// Find any streams that need a CBR index in the header
	StreamInfoList::iterator it = StreamList.begin();
	while(it != StreamList.end())
	{
		// Get a pointer to the stream
		BodyStreamPtr &Stream = (*it)->Stream;

		// Get the index type flags
		BodyStream::IndexType Index = Stream->GetIndexType();

		// Build an index table if required in the header
		if(Index & (BodyStream::IndexType::StreamIndexCBRHeader | BodyStream::IndexType::StreamIndexCBRHeaderIsolated))
		{
			// Get a pointer to the index manager
			IndexManagerPtr &IndexMan = Stream->GetIndexManager();

			if(IndexMan)
			{
				// Make an index table - will populate a CBR index
				IndexTablePtr Index = IndexMan->MakeIndex();

				// Write the index table
				DataChunkPtr IndexChunk = new DataChunk;
				Index->WriteIndex(*IndexChunk);
				
				// If we have a pending write do it now
				if(PartitionWritePending) EndPartition();

				// Work out this body SID
				PartitionBodySID = Stream->GetBodySID();

				// If we have already written the header partition this will be an isolated index
				if(HeaderWritten) BasePartition->ChangeType("ClosedCompleteBodyPartition");

				// Set the index SID
				BasePartition->SetUint("IndexSID",  Stream->GetIndexSID());

				// Record the index data to write
				PendingIndexData = IndexChunk;

				// Queue the write
				PartitionWritePending = true;

				// Now the header has been written (or at least is pending)
				HeaderWritten = true;
			}
		}

		it++;
	}

	// If no index table was written we will write a header with no index data
	if(!HeaderWritten)
	{
		// Flag no index data
		BasePartition->SetUint("IndexSID",  0);
		PendingIndexData = NULL;

		// Queue the write
		PartitionWritePending = true;
	}

	// Select the first post-header state
	State = BodyStateHeader;
	SetNextStream();
}



//! End the current partition
/*! Once "ended" no more essence will be added, even if otherwise valid.
 *  A new partition will be started by the next call to WritePartition()
 *  \note This function will also flush any pending partition writes
 */
void mxflib::BodyWriter::EndPartition(void)
{
	// If we have a pending write do it now
	if(PartitionWritePending)
	{
		// By default we only write metadata in the header
		bool WriteMetadata = PendingHeader || PendingMetadata;

		if((!PendingHeader) && (!PendingFooter))
		{
			// If we have a body partition handler call it and allow it to ask us to write metadata
			if(PartitionHandler) WriteMetadata = PartitionHandler->HandlePartition(BodyWriterPtr(this), CurrentBodySID, BasePartition->GetUint("IndexSID"));
		}

		// FIXME: Need to force a separate partition pack if we are about to violate the metadata sharing rules

		if(PendingIndexData)
		{
			File->WritePartitionWithIndex(BasePartition, PendingIndexData, WriteMetadata, NULL, MinPartitionFiller, MinPartitionSize);
			
			// Clear the index data SID to prevent it being written again next time
			BasePartition->SetUint("IndexSID",  0);
			PendingIndexData = NULL;
		}
		else
		{
			File->WritePartition(BasePartition, WriteMetadata, NULL, MinPartitionFiller, MinPartitionSize);
		}

		// Clear the pending data
		PartitionWritePending = false;
		PendingHeader = false;
		PendingFooter = false;
		PendingMetadata = false;
		PartitionDone = false;

		// Reset the partition size limits
		MinPartitionFiller = 0;
		MinPartitionSize = 0;
	}
}


//! Write stream data
/*! \param Duration - If > 0 the stop writing at the earliest opportunity after (at least) this number of edit units have been written for each stream
 *  \note Streams that have finished or hit thier own StopAfter value will be regarded as having written enough when judging whether to stop
 */
void mxflib::BodyWriter::WriteBody(Length Duration /*=0*/, Length MaxPartitionSize /*=0*/)
{
	while(State != BodyStateFooter)
	{
		Length ThisChunk = WritePartition(Duration, MaxPartitionSize);

		// If we are doing a limited duration then check if we have done it all
		if(Duration)
		{
			if(ThisChunk >= Duration) return;
			Duration -= ThisChunk;
		}
	}
}


//! Write the next partition or continue the current one (if not complete)
/*! Will stop at the point where the next partition will start, or (if Duration > 0) at the earliest opportunity after (at least) Duration edit units have been written
 */
Length mxflib::BodyWriter::WritePartition(Length Duration /*=0*/, Length MaxPartitionSize /*=0*/)
{
	// Number of edit units processed
	Length Ret = 0;

	// We must have written the header already
	if(State == BodyStateStart)
	{
		error("BodyWriter::WritePartition() called without first calling BodyWriter::WriteHeader()\n");
		return Ret;
	}

	// Exit if we are already finished
	if(State == BodyStateFooter || State == BodyStateDone ) return 0;

	// If there are no more streams - all done (should never happen!)
 	if(!CurrentBodySID) return Ret;

	// Index the stream info
	BodyStreamPtr Stream = (*CurrentStream)->Stream;

	// Get the current state
	BodyStream::StateType StreamState = Stream->GetState();

	switch(StreamState)
	{
	default:
		// Got to an unknown or invalid state!
		ASSERT(0);

	// Next action: Write an isolated index table before the next body partition
	case BodyStream::StateType::BodyStreamPreBodyIndex:
		{
			// If we have a pending write do it now
			if(PartitionWritePending) EndPartition();

			// Make an index table - will populate a CBR index
			IndexManagerPtr &IndexMan = (*CurrentStream)->Stream->GetIndexManager();
			if(!IndexMan)
			{
				error("Attempted to index a stream with no index manager\n");
				break;
			}

			IndexTablePtr Index = IndexMan->MakeIndex();

			// If the index table is VBR we need to build it
			if(!IndexMan->IsCBR()) 
			{
				// DRAGONS: This code is not currently used as we don't yet have a valid pre-isolated VBR index table option.
				//          However it will probably be required in the future

				// DRAGONS: If it is ever required to add full (growing) VBR index tables in the body here is where to do it (part 1)

				// Add any remaining entries to make a sprinkled index table
				Position EditUnit = IndexMan->GetLastNewEditUnit();
				int Count = IndexMan->AddEntriesToIndex(Index, Stream->GetNextSprinkled(), EditUnit);

				// TODO: Clear any used entries from the index table if not writing a full table anywhere else (or another sprinkled one?)

				// We have written a sprinkled index table - next sprinkled table will start from the next edit unit in sequence
				Stream->SetNextSprinkled(Stream->GetNextSprinkled() + Count);
			}

			// Write the index table
			DataChunkPtr IndexChunk = new DataChunk;
			Index->WriteIndex(*IndexChunk);

			// Isolated index tables live in closed complete body partitions
			BasePartition->ChangeType("ClosedCompleteBodyPartition");

			// There is no body data as we are isolated
			BasePartition->SetUint("BodySID", 0);
			BasePartition->SetUint("BodyOffset", 0);
			PartitionBodySID = 0;

			// Set the index SID
			BasePartition->SetUint("IndexSID",  Index->IndexSID);

			// Record the index data to write
			PendingIndexData = IndexChunk;

			// Queue the write
			PartitionWritePending = true;

			// This partition is isolated, so flush it now
			EndPartition();

			// Move the stream to the next state
			Stream->GetNextState();

			// Job done for this partition
			break;
		}

	// Next action: Write a body partition with/without an index table
	case BodyStream::StateType::BodyStreamBodyWithIndex:
		{
			// Ensure we don't share with metadata if not permitted
			if((!IndexSharesWithMetadata) && (!PartitionDone))
			{
				if(PartitionWritePending)
					if(PendingHeader || PendingMetadata) PartitionDone = true;
				else
					if(BasePartition->GetUint64("HeaderByteCount") > 0) PartitionDone = true;
			}

			// Fall through to no-index version
		}

	case BodyStream::StateType::BodyStreamBodyNoIndex:
		{
			// Ensure we don't share with metadata if not permitted
			if((!EssenceSharesWithMetadata) && (!PartitionDone))
			{
				if(PartitionWritePending)
					if(PendingHeader || PendingMetadata) PartitionDone = true;
				else
					if(BasePartition->GetUint64("HeaderByteCount") > 0) PartitionDone = true;
			}

			// If we are currently in a different BodySID we need to start a new partition
			if(PartitionDone || ((PartitionBodySID != 0) && (PartitionBodySID != CurrentBodySID)))
			{
				// Flush any previously pending partition
				if(PartitionWritePending) EndPartition();

				// Assume a closed complete body partition - may be changed by handler if metadata added
				BasePartition->ChangeType("ClosedCompleteBodyPartition");

				// We now have a new partition pending
				PartitionWritePending = true;
			}

			// If there is a partition pending then update it and write it
			if(PartitionWritePending)
			{
				BasePartition->SetUint("BodySID", CurrentBodySID);
				BasePartition->SetUint64("BodyOffset", Stream->GetWriter()->GetStreamOffset());

				if(StreamState == BodyStream::StateType::BodyStreamBodyWithIndex)
				{
					// Make an index table - will populate a CBR index
					IndexManagerPtr &IndexMan = (*CurrentStream)->Stream->GetIndexManager();
					if(!IndexMan)
					{
						error("Attempted to index a stream with no index manager\n");
						break;
					}

					IndexTablePtr Index = IndexMan->MakeIndex();

					// If the index table is VBR we need to build it
					if(!IndexMan->IsCBR()) 
					{
						// DRAGONS: If it is ever required to add full (growing) VBR index tables in the body here is where to do it (part 2)

						// Add any remaining entries to make a Sprinkled index table
						Position EditUnit = IndexMan->GetLastNewEditUnit();
						int Count = IndexMan->AddEntriesToIndex(Index, Stream->GetNextSprinkled(), EditUnit);

						// TODO: Clear any used entries from the index table if not writing a full table anywhere else (or another sprinkled one?)

						// We have written a sprinkled index table - next sprinkled table will start from the next edit unit in sequence
						// Note that we DONT do this if we are also doing sprinkled non-isolated so that we get
						// identical copies in each place (DRAGONS: which may or may not be legal!)
						if(Stream->GetIndexType() & BodyStream::IndexType::StreamIndexSprinkled)
						{
							Stream->SetNextSprinkled(Stream->GetNextSprinkled() + Count);
						}
					}

					// Write the index table
					DataChunkPtr IndexChunk = new DataChunk;
					Index->WriteIndex(*IndexChunk);

					// We will be a closed complete body partition unless the partition handler adds metadata
					BasePartition->ChangeType("ClosedCompleteBodyPartition");

					// Set the index SID
					BasePartition->SetUint("IndexSID",  Index->IndexSID);

					// Record the index data to write
					PendingIndexData = IndexChunk;
				}
				else
				{
					// No index data
					BasePartition->SetUint("IndexSID",  0);
				}

				// Note: The partition will be written by the call to WriteEssence
			}

			// Write the Essence
			Ret += WriteEssence((*CurrentStream), Duration, MaxPartitionSize);

			// Check the new state for this stream
			StreamState = Stream->GetState();

			// If this stream has done a cycle - move to the next stream
			if(StreamState != BodyStream::StateType::BodyStreamPostBodyIndex) SetNextStream();

			break;
		}

	// Next action: Write an isolated index table after a body partition
	case BodyStream::StateType::BodyStreamPostBodyIndex:
		{
			// If we have a pending write do it now (should not be possible!)
			if(PartitionWritePending) EndPartition();

			// Make an index table - will populate a CBR index
			IndexManagerPtr &IndexMan = (*CurrentStream)->Stream->GetIndexManager();
			if(!IndexMan)
			{
				error("Attempted to index a stream with no index manager\n");
				break;
			}

			IndexTablePtr Index = IndexMan->MakeIndex();

			// If the index table is VBR we need to build it
			if(!IndexMan->IsCBR()) 
			{
				// DRAGONS: If it is ever required to add full (growing) VBR index tables in the body here is where to do it (part 3)

				// Add any remaining entries to make a Sprinkled index table
				Position EditUnit = IndexMan->GetLastNewEditUnit();
				int Count = IndexMan->AddEntriesToIndex(Index, Stream->GetNextSprinkled(), EditUnit);

				// TODO: Clear any used entries from the index table if not writing a full table anywhere else (or another sprinkled one?)

				// We have written a sprinkled index table - next sprinkled table will start from the next edit unit in sequence
				// Note that we DONT do this if we are also doing sprinkled non-isolated so that we get
				// identical copies in each place (DRAGONS: which may or may not be legal!)
				if(!(Stream->GetIndexType() & BodyStream::IndexType::StreamIndexSprinkled))
				{
					Stream->SetNextSprinkled(Stream->GetNextSprinkled() + Count);
				}
			}

			// Write the index table
			DataChunkPtr IndexChunk = new DataChunk;
			Index->WriteIndex(*IndexChunk);

			// Isolated index tables generally live in closed complete body partitions
			BasePartition->ChangeType("ClosedCompleteBodyPartition");

			// There is no body data as we are isolated
			BasePartition->SetUint("BodySID", 0);
			BasePartition->SetUint("BodyOffset", 0);
			PartitionBodySID = 0;

			// Set the index SID
			BasePartition->SetUint("IndexSID",  Index->IndexSID);

			// Record the index data to write
			PendingIndexData = IndexChunk;

			// Queue the write
			PartitionWritePending = true;

			// This partition is isolated, so flush it now
			EndPartition();

			// Move the stream to the next state
			Stream->GetNextState();

			// This stream has done a cycle - move to the next stream
			SetNextStream();

			// Job done for this partition
			break;
		}

	// All done - no more body actions required
	case BodyStream::StateType::BodyStreamFootIndex:
	case BodyStream::StateType::BodyStreamDone:
		{
			// This stream has done - move to the next stream
			SetNextStream();
		}
	}

	// All done - return the number of edit units processed
	return Ret;
}


//! Write the file footer
/*! No essence will be written, but index tables will be written if required.
 */
void mxflib::BodyWriter::WriteFooter(bool WriteMetadata /*=false*/, bool IsComplete /*=true*/)
{
	// Sanity check
	if(WriteMetadata && (!BasePartition))
	{
		error("No base partition pack defined before call to BodyWriter::WriteFooter()\n");
		return;
	}

	// Sanity check
	if(State != BodyStateFooter)
	{
		error("BodyWriter::WriteFooter() called when the BodyWriter was not ready to write a footer\n");
		return;
	}

	// Turn the partition into a closed complete body or any pre-footer index only partitions
	BasePartition->ChangeType("ClosedCompleteBodyPartition");

	// There is no body data in a footer
	BasePartition->SetUint("BodySID", 0);
	BasePartition->SetUint("BodyOffset", 0);

	// Initially there is no index data
	BasePartition->SetUint("IndexSID",  0);

	// There will be no essence data
	PartitionBodySID = 0;

	/* There are a number of index tables that could be required at this point.
	 * We track what has been written already by the stream's FooterIndexFlags.
	 * These flags start off clear and when we write an index type, or determine it is not required, we set that bit.
	 * Once there are no outstanding index tables to write we bump the state to the next value and clear the flags (in case we write again)
	 */

	// Re-start the scan of body streams
	CurrentBodySID = 0;
	SetNextStream();

	// For each stream that claims to be ready for indexing
	while(CurrentBodySID)
	{
		// Index the stream info
		BodyStreamPtr Stream = (*CurrentStream)->Stream;

		// Read the index types and see what is requested
		BodyStream::IndexType IndexFlags = Stream->GetIndexType();
			
		// Clear any flags for types that have already been dealt with
		// DRAGONS: Is this an MSVC funny or can we really not do bitmaths with enums without them becoming integers?
		IndexFlags = (BodyStream::IndexType) (IndexFlags & ~(Stream->GetFooterIndex()));

		// Allow only those types that are of interest here
		// Note that sprinkled index tables are of interest as we have to drop any remaining sprinkle data
		// DRAGONS: Is this an MSVC funny or can we really not do bitmaths with enums without them becoming integers?
		IndexFlags = (BodyStream::IndexType) (IndexFlags & 
					  (  BodyStream::IndexType::StreamIndexFullFooter   | BodyStream::IndexType::StreamIndexFullFooterIsolated
					   | BodyStream::IndexType::StreamIndexSparseFooter | BodyStream::IndexType::StreamIndexSparseFooterIsolated
					   | BodyStream::IndexType::StreamIndexSprinkled    | BodyStream::IndexType::StreamIndexSprinkledIsolated
					   | BodyStream::IndexType::StreamIndexCBRFooter    | BodyStream::IndexType::StreamIndexCBRFooterIsolated));

		/* Note: The index tables will be writen in such an order as to keep the footer
		 *       as small as possible (to allow it to be located easily).
		 *       The order is:
		 *           Any remaining body index tables (left-over sprinkles)
		 *           Full VBR index table
		 *           Sparse VBR index table
		 *           CBR index table
		 */

		// No more index data to write
		if(IndexFlags == BodyStream::IndexType::StreamIndexNone)
		{
			// Clear the flags in case we are used to write another file (probably not a sane thing to do)
			Stream->SetIndexType(BodyStream::IndexType::StreamIndexNone);

			// Move this stream to the next state
			Stream->GetNextState();

			// Move to the next stream
			SetNextStream();

			// Process the next stream
			continue;
		}

		// Get and validate the index manager
		IndexManagerPtr &IndexMan = (*CurrentStream)->Stream->GetIndexManager();
		if(!IndexMan)
		{
			error("Attempted to index a stream with no index manager\n");

			// Move this stream to the next state
			Stream->GetNextState();

			// Move to the next stream
			SetNextStream();

			// Process the next stream
			continue;
		}

		// Make an index table - will populate a CBR index
		IndexTablePtr Index = IndexMan->MakeIndex();

		if(IndexMan->IsCBR()) 
		{
			// Select only the CBR flags
			// DRAGONS: Is this an MSVC funny or can we really not do bitmaths with enums without them becoming integers?
			IndexFlags = (BodyStream::IndexType) (IndexFlags & (BodyStream::IndexType::StreamIndexCBRFooter | BodyStream::IndexType::StreamIndexCBRFooterIsolated));

			// Check we are supposed to be writing a CBR index table
			ASSERT(IndexFlags);

			// If we are writing an isolated footer CBR do that first (in case we are doing both!)
			if(IndexFlags & BodyStream::IndexType::StreamIndexCBRFooterIsolated) IndexFlags = BodyStream::IndexType::StreamIndexCBRFooterIsolated;
		}
		// If the index table is VBR we need to build it
		else
		{
			// First off we write any remaining sprinkles
			if(IndexFlags & (BodyStream::IndexType::StreamIndexSprinkled | BodyStream::IndexType::StreamIndexSprinkledIsolated))
			{
				// Treat both left-over sprinkles the same - this means that there will only be
				// a single isolated "last sprinkled index" if both types are requested.
				// This is probably the most sensible way to do it as we can't add any essence
				// to this partition to make a non-isolated version so if we obeyed both flags
				// we would get two "isolated sprinkled index" partitions!
				// Setting both flags now will cause woth opetions to be flagged as "done" later
				// DRAGONS: Is this an MSVC funny or can we really not do bitmaths with enums without them becoming integers?
				IndexFlags = (BodyStream::IndexType) (IndexFlags & (BodyStream::IndexType::StreamIndexSprinkled | BodyStream::IndexType::StreamIndexSprinkledIsolated));

				// Add any remaining entries to make a sprinkled index table
				Position EditUnit = IndexMan->GetLastNewEditUnit();
				int Count = IndexMan->AddEntriesToIndex(Index, Stream->GetNextSprinkled(), EditUnit);
			}
			else if(IndexFlags & (BodyStream::IndexType::StreamIndexFullFooter | BodyStream::IndexType::StreamIndexFullFooterIsolated))
			{
				// If we are writing an isolated full index do that first (in case we are doing both!)
				if(IndexFlags & BodyStream::IndexType::StreamIndexFullFooterIsolated) IndexFlags = BodyStream::IndexType::StreamIndexFullFooterIsolated;

				// Add all available edit units
				Position EditUnit = IndexMan->GetLastNewEditUnit();
				int Count = IndexMan->AddEntriesToIndex(Index);
			}
			else if(IndexFlags & (BodyStream::IndexType::StreamIndexSparseFooter | BodyStream::IndexType::StreamIndexSparseFooterIsolated))
			{
				// If we are writing an isolated sparse index do that first (in case we are doing both!)
				if(IndexFlags & BodyStream::IndexType::StreamIndexSparseFooterIsolated) IndexFlags = BodyStream::IndexType::StreamIndexSparseFooterIsolated;

				// Force no re-ordering in the sparse index (to prevent unsatisfied links)
				int i;
				for(i=0; i<Index->BaseDeltaCount; i++)
				{
					if(Index->BaseDeltaArray[i].PosTableIndex < 0) Index->BaseDeltaArray[i].PosTableIndex = 0;
				}

				// Add each requested entry
				std::list<Position>::iterator it = (*CurrentStream)->Stream->SparseList.begin();
				while(it != (*CurrentStream)->Stream->SparseList.end())
				{
					IndexMan->AddEntriesToIndex(true, Index, (*it), (*it));
					it++;
				}
			}
			else
			{
				// This shouldn't be possible!
				ASSERT(0);
			}
		}

		// If we have a pending write do it now
		if(PartitionWritePending) EndPartition();

		// Write the index table
		DataChunkPtr IndexChunk = new DataChunk;
		Index->WriteIndex(*IndexChunk);

		// Set the index SID
		BasePartition->SetUint("IndexSID",  Index->IndexSID);

		// Record the index data to write
		PendingIndexData = IndexChunk;

		// Queue the write
		PartitionWritePending = true;

		// Set the "done" flag for this index type
		// DRAGONS: Is this an MSVC funny or can we really not do bitmaths with enums without them becoming integers?
		Stream->SetFooterIndex((BodyStream::IndexType) (Stream->GetFooterIndex() | IndexFlags) );

		// Move the stream to the next state
		Stream->GetNextState();

		// This stream has done a cycle - move to the next stream
		SetNextStream();
	}

	// If index data cannot share with metadata force any pending index data to be flushed before writing the actual footer
	if((!IndexSharesWithMetadata) && WriteMetadata && PartitionWritePending && PendingIndexData)
	{
		EndPartition();
	}

	// Turn the partition into the correct type of footer
	if(IsComplete)
		BasePartition->ChangeType("CompleteFooter");
	else
		BasePartition->ChangeType("Footer");

	// Flag that we have a partition to write
	PartitionWritePending = true;

	// And that it is a footer
	PendingFooter = true;

	// Set the metadata status
	PendingMetadata = WriteMetadata;

	// Write the footer now
	EndPartition();

	// Flag the writing as all done
	State = BodyStateDone;

	// Last thing we do is write the RIP
	// TODO: At the moment there is no way to not write a RIP which is probably the best way
	//       however if there is a good reason to allow it to be omitted...
	// Add a RIP (note that we have to manually KAG align as a footer can end off the KAG)
	if(KAG > 1) File->Align(KAG);
	File->WriteRIP();
}


//! Move to the next active stream
/*! \note Will set State to BodyStateDone if nothing left to do
 */
void mxflib::BodyWriter::SetNextStream(void)
{
	// If we are already done then no more to do
	if(State == BodyStateDone) return;

	// As we will loop at the end of the list check that we don't loop forever
	int MaxIters = StreamList.size();


	/* Update the state if required */

	// If we haven't done anything yet we are to do the header
	if(State == BodyStateStart) State = BodyStateHeader;

	// See if we need to restart the list - otherwise move to the next stream
	if(!CurrentBodySID) CurrentStream = StreamList.begin();
	else
	{
		CurrentStream++;
		// Wrap when we go off the end
		if(CurrentStream == StreamList.end()) CurrentStream = StreamList.begin();
	}

	// Loop through as many stream as we have (from the current stream back again with wrapping)
	while(MaxIters--)
	{
		if((*CurrentStream)->Active)
		{
			BodyStream::StateType StreamState = (*CurrentStream)->Stream->GetState();

			// If we find a "done" stream deactivate it
			if(StreamState == BodyStream::StateType::BodyStreamDone)
			{
				(*CurrentStream)->Active = false;
			}
			// If we are doing the header find a stream that is usable in the header
			else if(State == BodyStateHeader)
			{
				if(StreamState == BodyStream::StateType::BodyStreamHeadIndex)
				{
					// Set the BodySID and stop looking
					CurrentBodySID = (*CurrentStream)->Stream->GetBodySID();
					return;
				}
			}
			// If we are doing the footer find a stream that is usable in the footer
			else if(State == BodyStateFooter)
			{
				if(StreamState == BodyStream::StateType::BodyStreamFootIndex)
				{
					// Set the BodySID and stop looking
					CurrentBodySID = (*CurrentStream)->Stream->GetBodySID();
					return;
				}
			}
			// Se we must be in the body - find a body stream
			else
			{
				if((StreamState != BodyStream::StateType::BodyStreamHeadIndex) && (StreamState != BodyStream::StateType::BodyStreamFootIndex))
				{
					// Set the BodySID and stop looking
					CurrentBodySID = (*CurrentStream)->Stream->GetBodySID();
					return;
				}
			}
		}

		// Try the next stream
		CurrentStream++;

		// Wrap when we go off the end
		if(CurrentStream == StreamList.end()) CurrentStream = StreamList.begin();
	}

	// So far we haven't found a valid stream...
	CurrentBodySID = 0;

	// Whe don't manually progress beyond the footer state
	// This is so when all body essence is exhasted we will move to the footer state
	// (irrespective of the number of streams with footer index data to write) but calls
	// from within WriteFooter() won't progress us to "done" that will be manually
	// determined within WriteFooter()
	if(State == BodyStateFooter) return;

	/* We haven't found a valid stream yet - but we are not all done there are more states to try */

	// Move to the next state
	if(State == BodyStateHeader) State = BodyStateBody;
	else if(State == BodyStateBody) State = BodyStateFooter;

	// Try finding a stream for that state (starting with the first stream)
	SetNextStream();
}


//! Add a stream to the list of those to write
/*! \param Stream - The stream to write
 *  \param StopAfter - If > 0 the writer will stop writing this stream at the earliest opportunity after (at least) this number of edit units have been written
 *  Streams will be written in the order that they were offered and the list is kept in this order.
 *	\return false if unable to add this stream (for example this BodySID already in use)
 */
bool mxflib::BodyWriter::AddStream(BodyStreamPtr &Stream, Length StopAfter /*=0*/)
{
	// Check if this BodySID is already used
	Uint32 SID = Stream->GetBodySID();
	StreamInfoList::iterator it = StreamList.begin();
	while(it != StreamList.end())
	{
		// Matching SID?
		if((*it)->Stream->GetBodySID() == SID)
		{
			error("Attempted to add two streams both with BodySID = %d to file %s\n", SID, File->Name.c_str());
			return false;
		}

		it++;
	}

	// Build the new info object
	StreamInfoPtr NewStream = new StreamInfo;
	NewStream->Active = true;
	NewStream->Stream = Stream;
	NewStream->StopAfter = StopAfter;

	// Add at the end of the list
	StreamList.push_back(NewStream);

	// Ensure that this stream has a writer
	if(!Stream->GetWriter()) Stream->SetWriter(GCWriterPtr(new GCWriter(File, SID)));

	return true;
}

