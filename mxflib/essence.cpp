/*! \file	essence.cpp
 *	\brief	Implementation of classes that handle essence reading and writing
 *
 *	\version $Id: essence.cpp,v 1.1.2.7 2004/06/26 17:59:33 matt-beard Exp $
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
	WB.KLVSource = NULL;

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
	memcpy(&Buffer[ValStart], Data, Size);

	// Add this item to the write queue (the writer will free the memory)
	WriteBlock WB;
	WB.Size = Size + ValStart;
	WB.Buffer = Buffer;
	WB.Source = NULL;
	WB.KLVSource = NULL;

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
			Uint64 Pos = LinkedFile->Tell();
			StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
		}

		// Write the pre-formatted data and free its buffer
		StreamOffset += LinkedFile->Write((*it).second.Buffer, (*it).second.Size);
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
	if(File->Tell() < CurrentPos) 
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
		EssenceSubParserBase *EP = (*it)->NewParser();
		EssenceStreamDescriptorList DescList = EP->IdentifyEssence(InFile);
		
		if(DescList.empty())
		{
			delete EP;
		}
		else
		{
			Ret->push_back(ParserDescriptorPair(EP, DescList));
		}

		it++;
	}

	return Ret;
}


//! Select the best wrapping option
/*! DRAGONS: Currently destroys PDList to preserve the essence handler
 */
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
				MDObjectPtr Ptr = Ret->EssenceDescriptor["SampleRate"];

				if((!Ptr) || (ForceEditRate.Numerator != 0))
				{
					Ret->EditRate.Numerator = ForceEditRate.Numerator;
					Ret->EditRate.Denominator = ForceEditRate.Denominator;
				}
				else
				{
					std::string Rate = Ptr->GetString();
					Ret->EditRate.Numerator = Ptr->GetInt("Numerator");
					Ret->EditRate.Denominator = Ptr->GetInt("Denominator");
				}

				Ret->WrapOpt = (*it2);
				Ret->Stream = (*it).ID;

				Ret->WrapOpt->Handler->Use(Ret->Stream, Ret->WrapOpt);
				if( Ret->WrapOpt->Handler->SetEditRate(0, Ret->EditRate) )
				{
					// All OK, including requested edit rate
					
					Ret->WrapOpt->BytesPerEditUnit = Ret->WrapOpt->Handler->GetBytesPerEditUnit();

					// Remove all entries that index this handler to prevent it being deleted
					ParserDescriptorList::iterator pdit2 = PDList->begin();
					while(pdit2 != PDList->end())
					{
						if((*pdit2).first == Ret->WrapOpt->Handler)
						{
							pdit2 = PDList->erase(pdit2);
						}
					}

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
