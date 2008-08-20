/*! \file	essence.cpp
 *	\brief	Implementation of classes that handle essence reading and writing
 *
 *	\version $Id: essence.cpp,v 1.37 2008/08/20 12:53:59 matt-beard Exp $
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

//! Template for all GC essence item keys
/*! DRAGONS: Version number is hard coded as 0 - must be overwritten */

namespace
{
	//! The standard Generic Container essence key root
	const UInt8 GCEssenceKey[16] =	{ 0x06, 0x0e, 0x2B, 0x34,
									  0x01, 0x02, 0x01, 0x00,
									  0x0d, 0x01, 0x03, 0x01,
									  0x00, 0x00, 0x00, 0x00  };

	//! A list of pointers to alternative essence key roots to treat as GC keys
	/*! This allows private or experimental essence keys to be treated as standard GC keys when reading 
	 */
	DataChunkList GCEssenceKeyAlternatives;


	//! The standard Generic Container system item key root
	const UInt8 GCSystemKey[16] =	{ 0x06, 0x0e, 0x2B, 0x34,
									  0x02, 0x00, 0x01, 0x00,
									  0x0d, 0x01, 0x03, 0x01,
									  0x00, 0x00, 0x00, 0x00  };

	//! A list of pointers to alternative system item key roots to treat as GC keys
	/*! This allows private or experimental system item keys to be treated as standard GC keys when reading 
	 */
	DataChunkList GCSystemKeyAlternatives;
}


//! Flag that allows faster clip wrapping using random access
bool mxflib::AllowFastClipWrap;


namespace
{
	//! Max nmumber of bytes that we will try and wrap in one go (stops clip-wrapping bursting our memory)
	const size_t MaxWrapChunkSize = 1024 * 1024 * 32;
}


//! Constructor
GCWriter::GCWriter(MXFFilePtr File, UInt32 BodySID /*=0*/, int Base /*=0*/)
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

	// All system items are assumed GC
	Stream->NonGC = false;

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

	// Use auto BER length size for system items
	Stream->LenSize = 0;

	// "Default" system item write order:
	//  00001000 s10SSSSS SSeeeeee ennnnnnn
	// Where:
	//   TTTTTTT = Type (GC types mapped to CP versions)
	//         s = 0 for CP, 1 for GC
	//	 SSSSSSS = Scheme ID
	//   eeeeeee = Element ID
	//	 nnnnnnn = Sub ID
	//

	if(CPCompatible) Stream->WriteOrder = 0x08400000; else Stream->WriteOrder = 0x08c00000;

	Stream->WriteOrder |= (Stream->SchemeOrCount << 14) | (Stream->Element << 7) | Stream->SubOrNumber;

	// Add this value to the map
	WriteOrderMap.insert(std::map<UInt32, GCStreamID>::value_type(Stream->WriteOrder, ID));

	return ID;
}


//! Define a new essence element for this container
GCStreamID GCWriter::AddEssenceElement(unsigned int EssenceType, unsigned int ElementType, int LenSize /*=0*/)
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

	// This is a GC essence stream
	Stream->NonGC = false;

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
	UInt8 Type = EssenceType;
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

	// Set BER length size for essence items
	Stream->LenSize = LenSize;

	// "Default" essence item write order:
	//  TTTTTTT0 s10eeeee ee000000 0nnnnnnn
	// Where:
	//   TTTTTTT = Type (GC types mapped to CP versions)
	//         s = 0 for CP, 1 for GC
	//   eeeeeee = Element ID
	//	 nnnnnnn = Element Number

	if(CPCompatible) Stream->WriteOrder = 0x00400000; else Stream->WriteOrder = 0x00c00000;

	Stream->WriteOrder |= (Type << 25) | (Stream->SchemeOrCount << 14) | Stream->SubOrNumber;

	// Add this value to the map
	WriteOrderMap.insert(std::map<UInt32, GCStreamID>::value_type(Stream->WriteOrder, ID));

	return ID;
}


//! Define a new essence element for this container, with a specified key
GCStreamID GCWriter::AddEssenceElement(DataChunkPtr &Key, int LenSize /*=0*/, bool NonGC /*=false*/)
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

	// Set the key details
	Stream->SpecifiedKey = Key;
	Stream->NonGC = NonGC;

	// Get the item type from the supplied key (should work OK for GCish types)
	Stream->Type = Key->Data[12];

	// Set the key items (which will probably never be used)
	Stream->RegVer = 1;
	Stream->RegDes = 0x02;		// All essence items are "essence items"

	if(!NonGC)
	{
		// Count the number of elements of this type
		int Count = 1;		// Start by counting us
		int i;
		for(i=0; i<ID; i++)
		{
			// DRAGONS: Should we allow duplicates for same essence types of different element types?
			if((StreamTable[i].Type == Stream->Type) /*&& (StreamTable[i].Element == ElementType)*/)
			{
				Count++;
			}
		}

		Stream->SchemeOrCount = Count+StreamBase;
		Stream->Element = Key->Data[14];
		Stream->SubOrNumber = Count+StreamBase;
		Stream->CountFixed = false;
	}

	bool CPCompatible = false;
	UInt8 Type = Stream->Type;
	if(!NonGC)
	{
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
	}

	// Initially we don't index this stream
	Stream->IndexMan = NULL;
	Stream->IndexFiller = 0;

	// Set BER length size for essence items
	Stream->LenSize = LenSize;

	// "Default" essence item write order:
	//  TTTTTTT0 s10eeeee ee000000 0nnnnnnn
	// Where:
	//   TTTTTTT = Type (GC types mapped to CP versions)
	//         s = 0 for CP, 1 for GC
	//   eeeeeee = Element ID
	//	 nnnnnnn = Element Number

	if(CPCompatible) Stream->WriteOrder = 0x00400000; else Stream->WriteOrder = 0x00c00000;

	Stream->WriteOrder |= (Type << 25) | (Stream->SchemeOrCount << 14) | Stream->SubOrNumber;

	// Add this value to the map
	WriteOrderMap.insert(std::map<UInt32, GCStreamID>::value_type(Stream->WriteOrder, ID));

	return ID;
}


//! Allow this data stream to be indexed and set the index manager
void GCWriter::AddStreamIndex(GCStreamID ID, IndexManagerPtr &IndexMan, int IndexSubStream, bool IndexFiller /*=false*/, bool IndexClip /*=false*/)
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
	Stream->IndexClip = IndexClip;
}


//! Add system item data to the current CP
void GCWriter::AddSystemData(GCStreamID ID, UInt64 Size, const UInt8 *Data)
{
	//! Template for all GC system item keys
	/*! DRAGONS: Version number is hard coded as 1 */
	static const UInt8 GCSystemKey[12] = { 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x00, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01 };
	
	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::AddSystemData()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	// Set up a new buffer big enough for the key, a huge BER length and the data
	UInt8 *Buffer = new UInt8[(size_t)(16 + 9 + Size)];

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
	size_t ValStart = 16 + BER->Size;

	// Copy the value into the buffer
	memcpy(&Buffer[ValStart], Data, (size_t)Size);

	// Add this item to the write queue (the writer will free the memory)
	WriteBlock WB;
	WB.Size = Size + ValStart;
	WB.Buffer = Buffer;
	WB.KLVSource = NULL;
	WB.FastClipWrap = false;
	WB.LenSize = Stream->LenSize;

	// Add the index data
	WB.IndexMan = Stream->IndexMan;
	if(WB.IndexMan)
	{
		WB.IndexSubStream = Stream->IndexSubStream;
		WB.IndexFiller = Stream->IndexFiller;
		WB.IndexClip = Stream->IndexClip;
	}
	else
		WB.IndexFiller = false;

	WriteQueue.insert(WriteQueueMap::value_type(Stream->WriteOrder, WB));
}


//! Add essence item data to the current CP
void GCWriter::AddEssenceData(GCStreamID ID, UInt64 Size, const UInt8 *Data)
{
	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::AddEssenceData()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	// Set up a new buffer big enough for the key, a huge BER length and the data
	UInt8 *Buffer = new UInt8[(size_t)(16 + 9 + Size)];

	if(Stream->SpecifiedKey)
	{
		memcpy(Buffer, Stream->SpecifiedKey->Data, 16);
	}
	else
	{
		// Copy in the key template
		memcpy(Buffer, GCEssenceKey, 12);

		// Set up the rest of the key
		Buffer[7] = Stream->RegVer;
		Buffer[12] = Stream->Type;
	}

	// Update the last three GC track number bytes unless it's not a GC KLV
	if(!Stream->NonGC)
	{
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
	}

	// Add the length and work out the start of the data field
	DataChunkPtr BER = MakeBER(Size);
	memcpy(&Buffer[16], BER->Data, BER->Size);
	size_t ValStart = 16 + BER->Size;

	// Copy the value into the buffer
	memcpy(&Buffer[ValStart], Data, (size_t)Size);

	// Add this item to the write queue (the writer will free the memory)
	WriteBlock WB;
	WB.Size = Size + ValStart;
	WB.Buffer = Buffer;
	WB.KLVSource = NULL;
	WB.FastClipWrap = false;
	WB.LenSize = Stream->LenSize;

	// Add the index data
	WB.IndexMan = Stream->IndexMan;
	if(WB.IndexMan)
	{
		WB.IndexSubStream = Stream->IndexSubStream;
		WB.IndexFiller = Stream->IndexFiller;
		WB.IndexClip = Stream->IndexClip;
	}
	else
		WB.IndexFiller = false;

	WriteQueue.insert(WriteQueueMap::value_type(Stream->WriteOrder, WB));
}


//! Add an essence item to the current CP with the essence to be read from an EssenceSource object
void GCWriter::AddEssenceData(GCStreamID ID, EssenceSourcePtr Source, bool FastClipWrap /*=false*/)
{
	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::AddEssenceData()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	// Set up a new buffer big enough for the key alone - the BER length and data will be added later
	UInt8 *Buffer = new UInt8[16];

	if(Stream->SpecifiedKey)
	{
		memcpy(Buffer, Stream->SpecifiedKey->Data, 16);
	}
	else
	{
		// Copy in the key template
		memcpy(Buffer, GCEssenceKey, 12);

		// Set up the rest of the key
		Buffer[7] = Stream->RegVer;
		Buffer[12] = Stream->Type;
	}

	// Update the last three GC track number bytes unless it's not a GC KLV
	if(!Stream->NonGC)
	{
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
	}

	// Add this item to the write queue (the writer will free the memory and the EssenceSource)
	WriteBlock WB;
	WB.Size = 16;
	WB.Buffer = Buffer;
	WB.Source = Source;
	WB.KLVSource = NULL;
	WB.FastClipWrap = FastClipWrap;
	WB.LenSize = Stream->LenSize;

	// Add the index data
	WB.IndexMan = Stream->IndexMan;
	if(WB.IndexMan)
	{
		WB.IndexSubStream = Stream->IndexSubStream;
		WB.IndexFiller = Stream->IndexFiller;
		WB.IndexClip = Stream->IndexClip;
	}
	else
		WB.IndexFiller = false;

	WriteQueue.insert(WriteQueueMap::value_type(Stream->WriteOrder, WB));
}



//! Add an essence item to the current CP with the essence to be read from a KLVObject
void GCWriter::AddEssenceData(GCStreamID ID, KLVObjectPtr Source, bool FastClipWrap /*=false*/)
{
	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::AddEssenceData()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	// Set up a new buffer big enough for the key alone - the BER length and data will be added later
	UInt8 *Buffer = new UInt8[16];

	if(Stream->SpecifiedKey)
	{
		memcpy(Buffer, Stream->SpecifiedKey->Data, 16);
	}
	else
	{
		// Copy in the key template
		memcpy(Buffer, GCEssenceKey, 12);

		// Set up the rest of the key
		Buffer[7] = Stream->RegVer;
		Buffer[12] = Stream->Type;
	}

	// Update the last three GC track number bytes unless it's not a GC KLV
	if(!Stream->NonGC)
	{
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
	}

	// Add this item to the write queue (the writer will free the memory and the EssenceSource)
	WriteBlock WB;
	WB.Size = 16;
	WB.Buffer = Buffer;
	WB.KLVSource = Source;
	WB.FastClipWrap = FastClipWrap;
	WB.LenSize = Stream->LenSize;

	// Add the index data
	WB.IndexMan = Stream->IndexMan;
	if(WB.IndexMan)
	{
		WB.IndexSubStream = Stream->IndexSubStream;
		WB.IndexFiller = Stream->IndexFiller;
		WB.IndexClip = Stream->IndexClip;
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
UInt32 GCWriter::GetTrackNumber(GCStreamID ID)
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
/*! \note Will return (2^64)-1 if the buffer contains a "FastClipWrap" item
 */
UInt64 GCWriter::CalcWriteSize(void)
{
	UInt64 Ret = 0;

	//! The last type written - KAG alignment is performed between different types
	UInt8 LastType = 0xff;

	WriteQueueMap::iterator it = WriteQueue.begin();
	while(it != WriteQueue.end())
	{
		// The most significant byte is basically the item type
		UInt8 ThisType = (*it).first >> 24;

		// Add the size of any filler
		if((ThisType != LastType) && (KAGSize > 1))
		{
			if(!LinkedFile->IsBlockAligned())
			{
				Ret += LinkedFile->FillerSize(ForceFillerBER4, KAGSize);
			}
			else
			{
				// Do nothing when block aligned
				// DRAGONS: Should we do something here?
			}
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
			// If any item is to be "FastClipWrapped" then return a huge size
			// to flag that we cannot know the size of the next write
			if((*it).second.FastClipWrap) return (UInt64)-1;

			size_t Size = (*it).second.Source->GetEssenceDataSize();
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
		if(!LinkedFile->IsBlockAligned())
		{
			Ret += LinkedFile->FillerSize(ForceFillerBER4, KAGSize);
		}
		else
		{
			// Do nothing when block aligned
			// DRAGONS: Should we do something here?
		}
	}

	return Ret;
}


//! GCWriter destructor - clean up all allocated memory
GCWriter::~GCWriter()
{
	// Clear the write-queue buffers
	WriteQueueMap::iterator it = WriteQueue.begin();
	while(it != WriteQueue.end())
	{
		delete[] (*it).second.Buffer;
		it++;
	}

	// Clear the stream table
	delete[] StreamTable;
}

//! Flush any remaining data
/*! \note It is important that any changes to this function are propogated to CalcWriteSize() */
void GCWriter::Flush(void)
{
	//! Stream offset of the first byte of the key for this KLV - this will later be turned into the size of the (Key+Length) once they are written
	Position KLSize = StreamOffset;

	//! The last type written - KAG alignment is performed between different types
	UInt8 LastType = 0xff;

	WriteQueueMap::iterator it = WriteQueue.begin();
	while(it != WriteQueue.end())
	{
		// The most significant byte is basically the item type
		UInt8 ThisType = (*it).first >> 24;

		// Align to the next KAG
		if((ThisType != LastType) && (KAGSize > 1))
		{
			// If we are indexing filler then send this offset to the index manager - even if we write 0 bytes 
			if((*it).second.IndexFiller)
			{
				// Send this stream offset to index stream -1 to signify filler
				if((*it).second.IndexMan) (*it).second.IndexMan->OfferOffset(-1, IndexEditUnit, StreamOffset);
			}

			if(!LinkedFile->IsBlockAligned())
			{
				UInt64 Pos = LinkedFile->Tell();
				StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
			}
			else
			{
				// Do nothing when block aligned
				// DRAGONS: Should we do something here?
			}
		}

		// Index this item (if we are indexing, but not if we are clip-wrap indexing)
		if((*it).second.IndexMan)
		{
			if(!(*it).second.IndexClip) (*it).second.IndexMan->OfferOffset((*it).second.IndexSubStream, IndexEditUnit, StreamOffset);
		}

		// Write the pre-formatted data and free its buffer
		StreamOffset += LinkedFile->Write((*it).second.Buffer, (UInt32)((*it).second.Size));
		delete[] (*it).second.Buffer;

		// Handle any KLVObject-buffered essence data
		if((*it).second.KLVSource)
		{
			UInt64 Size = (*it).second.KLVSource->GetLength();

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
			Position LenPosition = 0;
			UInt64 Size;

			// If we are fast clip wrapping flag the rest of the file as the value
			// and record the location of this length for later correction
			if((*it).second.FastClipWrap)
			{
				if((*it).second.LenSize == 4) Size = 0x00ffffff;
				else Size = UINT64_C(0x00ffffffffffffff);

				LenPosition = LinkedFile->Tell();
			}
			else Size = (*it).second.Source->GetEssenceDataSize();

			// Write out the length
			int LenSize = LinkedFile->WriteBER(Size, (*it).second.LenSize);
			StreamOffset += LenSize;

			// Fast access to IndexClip flag
			bool IndexClip = ((*it).second.IndexMan) && ((*it).second.IndexClip);
			Position LastEditUnit = -1;

			// Record the KLSize if we are doing value-relative indexing
			if(IndexClip && (*it).second.IndexMan->GetValueRelativeIndexing())
			{
				KLSize = StreamOffset - KLSize;
			}
			else
			{
				KLSize = 0;
			}

			// Write out all the data
			for(;;)
			{
				bool IndexThisItem = false;

				// If we are clip-wrap indexing we need to determine if this is the start of an edit unit
				// The EssenceSource is responsible for ensuring that each edit unit starts at the 
				// beginning of a new GetEssenceData() chunk
				if(IndexClip)
				{
					Position EditUnit = (*it).second.Source->GetCurrentPosition();
					if(EditUnit != LastEditUnit)
					{
						LastEditUnit = EditUnit;
						IndexThisItem = true;
					}
				}

				DataChunkPtr Data = (*it).second.Source->GetEssenceData(0, MaxWrapChunkSize);
				
				// Exit when no more data left
				if(!Data) break;

				if(Data->Size == 0)
				{
					warning("GetEssenceData returned zero bytes (request to try again later)\n");
					continue;
				}

				// Index this item if required (removing the KLSize if doing value-relative indexing)
				if(IndexThisItem) (*it).second.IndexMan->OfferOffset((*it).second.IndexSubStream, LastEditUnit, StreamOffset - KLSize);

				// Write the data
				StreamOffset += LinkedFile->Write(*Data);
			}

			// Now correct the length if we are fast clip wrapping
			if((*it).second.FastClipWrap)
			{
				Position ValueEnd = LinkedFile->Tell();

				// Calculate the size of the value taking into account the size of the BER length
				UInt64 ValueSize = (UInt64)(ValueEnd - (LenPosition + LenSize));

				// Write the true length over the (2^xx)-1 version
				LinkedFile->Seek(LenPosition);
				if((int)LinkedFile->WriteBER(ValueSize, LenSize) != LenSize)
				{
					// DRAGONS: At this point the file is broken - but there is no graceful solution!
					error("Clip wrapped essence item greater than will fit in the required %d-byte BER length\n", LenSize);
				}

				// Return to the current write point
				LinkedFile->Seek(ValueEnd);
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
		if(!LinkedFile->IsBlockAligned())
		{
			UInt64 Pos = LinkedFile->Tell();
			StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
		}
		else
		{
			// Do nothing when block aligned
			// DRAGONS: Should we do something here?
		}
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
void GCWriter::SetWriteOrder(GCStreamID ID, Int32 WriteOrder /*=-1*/, int Type /*=-1*/)
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
	UInt32 NewWriteOrder = (static_cast<UInt32>(Type) << 25) | ((static_cast<UInt32>(WriteOrder) & 0x0000ffff) << 5);

	// Add bits for CP/GC ordering
	if(!CPCompatible) NewWriteOrder |= 0x00800000;

	// Add bits to move the write order to after the "default" order if required
	if(WriteOrder & 0x8000) NewWriteOrder |= 0x00600000;

	/* De-deplicate write order */
	for(;;)
	{
		// See if this value is used yet
		std::map<UInt32, GCStreamID>::iterator it = WriteOrderMap.find(NewWriteOrder);

		// If not, all done
		if(it == WriteOrderMap.end()) break;

		// Else try the next position
		NewWriteOrder++;
	}

	// Add this value to the map
	WriteOrderMap.insert(std::map<UInt32, GCStreamID>::value_type(NewWriteOrder, ID));

	// Record the new value
	Stream->WriteOrder = NewWriteOrder;
}




// "Default" system item write order:
//  00001000 s10SSSSS SSeeeeee ennnnnnn
// Where:
//   TTTTTTT = Type (GC types mapped to CP versions)
//         s = 0 for CP, 1 for GC
//	 SSSSSSS = Scheme ID
//   eeeeeee = Element ID
//	 nnnnnnn = Sub ID
//

// "Default" essence item write order:
//  TTTTTTT0 s10eeeee ee000000 0nnnnnnn
// Where:
//   TTTTTTT = Type (GC types mapped to CP versions)
//         s = 0 for CP, 1 for GC
//   eeeeeee = Element ID
//	 nnnnnnn = Element Number

// "Default" essence item write order:
//  TTTTTTT0 s10eeeee ee000000 0nnnnnnn
// Where:
//   TTTTTTT = Type (GC types mapped to CP versions)
//         s = 0 for CP, 1 for GC
//   eeeeeee = Element ID
//	 nnnnnnn = Element Number

// Manually set write order:
//  TTTTTTT0 sXXWWWWW WWWWWWWW WWW00000
// Where:
//   TTTTTTT = Type (GC types mapped to CP versions)
//         s = 0 for CP, 1 for GC
//		  XX = MSB of Write Order (2 copies of)
//   WW...WW = Write order (as specified or last + 1 for auto)
//

// Special set write order:
//  TTTTTTT1 110WWWWW WWWWWWWW WWW00000
// Where:
//   TTTTTTT = Type (GC types mapped to CP versions)
//         s = 0 for CP, 1 for GC
//   WW...WW = Write order as calculated...
//

//! Set a write-order relative to all items of a specified type
/*! This allows a stream to be written "before all audio items" or "after all system items"
 *  rather than in its default position for its own type.
 *
 *  \param ID The ID of the stream whose write-order is being set
 *  \param Type The type to position this stream relative to
 *  \param Position A signed position indicating before (-ve) or after (+ve) and strength of positioning
 *
 * The value of Position (rather than its sign) is important when more than one stream is positioned
 * before or after the same general type. The further Position is from zero, the nearer it will be placed to
 * "immediately before" or "immediately after" all items of the specified type. For example the following
 * list shows the order of some system items and items specified as being before or after system items
 * with a specified relative-position:
 *
 *    Position = -1
 *    Position = -100
 *    Position = -25000
 *    Position = -32768
 *    First System Item
 *    ...
 *    Last System Item
 *    Position = 32767
 *    Position = 15000
 *    Position = 1
 *    Position = 0
 *
 * Although Position is a UInt32, only the sign bit, and the lowest 15 bits are significant.
 *
 * \note To set a stream's write order to be before all picture essence use Type = 0x05 and Position < 0, 
 *       using Type = 0x15 will place it between the CP picture essence and GC Picture essence. To set
 *       the write order to be after all picture essence use Type = 0x15 and Position >= 0. The same
 *       principle applies to other GC/CP types.
 */
void GCWriter::SetRelativeWriteOrder(GCStreamID ID, int Type, Int32 Position)
{
	// Index the data block for this stream
	if((ID < 0) || (ID >= StreamCount))
	{
		error("Unknown stream ID in GCWriter::SetRelativeWriteOrder()\n");
		return;
	}
	GCStreamData *Stream = &StreamTable[ID];

	UInt32 WriteOrder;

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

	if(Position >= 0)
	{
		WriteOrder = (static_cast<UInt32>(Type) << 25) | 0x01c00000 | ((static_cast<UInt32>(Position & 0xffff) ^ 0xffff) << 5);
	}
	else
	{
		WriteOrder = (static_cast<UInt32>(Type-1) << 25) | 0x01c00000 | ((static_cast<UInt32>(Position & 0xffff) ^ 0xffff) << 5);
	}

	// Add bits for CP/GC ordering
	if(!CPCompatible) Stream->WriteOrder |= 0x00800000;

	/* De-deplicate write order */
	for(;;)
	{
		// See if this value is used yet
		std::map<UInt32, GCStreamID>::iterator it = WriteOrderMap.find(WriteOrder);

		// If not, all done
		if(it == WriteOrderMap.end()) break;

		// Else try the next position away
		if(Position >= 0) WriteOrder++;
		else WriteOrder--;
	}

	// Add this value to the map
	WriteOrderMap.insert(std::map<UInt32, GCStreamID>::value_type(WriteOrder, ID));

	// Record the new value
	Stream->WriteOrder = WriteOrder;
}

//! Calculate how many bytes would be written if the specified object were written with WriteRaw()
Length GCWriter::CalcRawSize(KLVObjectPtr Object)
{
	Length Ret = 0;

	// Add the size of any filler
	if(KAGSize > 1)
	{
		if(!LinkedFile->IsBlockAligned())
		{
			Ret += LinkedFile->FillerSize(ForceFillerBER4, KAGSize);
		}
		else
		{
			// Do nothing when block aligned
			// DRAGONS: Should we do something here?
		}
	}

	// Add the chunk size
	Ret += Object->GetKLSize() + Object->GetLength();


	// DRAGONS: This is a bit of a fudge to cope with new partitions 
	//          being inserted after us and that causing a filler...

	// Align to the next KAG
	if(KAGSize > 1)
	{
		if(!LinkedFile->IsBlockAligned())
		{
			Ret += LinkedFile->FillerSize(ForceFillerBER4, KAGSize);
		}
		else
		{
			// Do nothing when block aligned
			// DRAGONS: Should we do something here?
		}
	}

	return Ret;
}


//! Write a raw KLVObject to the file - this is written immediately and not buffered in the WriteQueue
void GCWriter::WriteRaw(KLVObjectPtr Object)
{
	// Align to the next KAG
	if(KAGSize > 1)
	{
		if(!LinkedFile->IsBlockAligned())
		{
			UInt64 Pos = LinkedFile->Tell();
			StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
		}
		else
		{
			// Do nothing when block aligned
			// DRAGONS: Should we do something here?
		}
	}

	// Set this file and position as the destination for the KLVObject
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
		if(!LinkedFile->IsBlockAligned())
		{
			UInt64 Pos = LinkedFile->Tell();
			StreamOffset += LinkedFile->Align(ForceFillerBER4, KAGSize) - Pos;
		}
		else
		{
			// Do nothing when block aligned
			// DRAGONS: Should we do something here?
		}
	}

	return;
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
 *        to push the decrypted data back to the GCReader to pass to the appropriate handler
 *  \return true if all OK, false on error
 */
bool GCReader::HandleData(KLVObjectPtr Object)
{
	// First check is this KLV is a filler
	// We first check if byte 9 == 3 which is true for filler keys, but is
	// false for all GC sets and packs. Once this matches we can do a full memcmp.
	if(Object->GetUL()->GetValue()[8] == 3)
	{
		const UInt8 FillerKey[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x02, 0x10, 0x01, 0x00, 0x00, 0x00 };
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
			const UInt8 EncryptedKey[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x02, 0x04, 0x01, 0x07, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x7e, 0x01, 0x00 };
			if( memcmp(Object->GetUL()->GetValue(), EncryptedKey, 16) == 0 )
			{
				return EncryptionHandler->HandleData(this, Object);
			}
		}
	}

	// Get the track-number of this GC item (or zero if not GC)
	// Note that we don't bother if no handlers have been registered 
	// because we will have to use the defualt handler whatever!
	UInt32 TrackNumber; 
	if(!Handlers.size()) TrackNumber = 0; else TrackNumber = Object->GetGCTrackNumber();

	if( TrackNumber != 0 )
	{
		// See if we have a handler registered for this track
		std::map<UInt32, GCReadHandlerPtr>::iterator it = Handlers.find(TrackNumber);

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
Position BodyReader::Seek(UInt32 BodySID, Position Pos)
{
	// We <b>need</b> a RIP for this to work
	if(File->FileRIP.empty()) File->GetRIP();

	PartitionInfoPtr PartInfo = File->FileRIP.FindPartition(BodySID, Pos);

	if(!PartInfo)
	{
		error("BodyReader::Seek(%d, 0x%s) failed to locate the correct partition\n", BodySID, Int64toHexString(Pos).c_str());
		return -1;
	}

	// Get the stream offset of the start of this partition
	Position StreamOffset = PartInfo->GetStreamOffset();
	
	// If that was unknown we need to read the partition pack
	if(StreamOffset == -1)
	{
		File->Seek(PartInfo->GetByteOffset());
		PartInfo->ThePartition = File->ReadPartition();

		if(PartInfo->ThePartition)
		{
			StreamOffset = PartInfo->ThePartition->GetInt64("BodyOffset");
		}
	}

	Position EssenceStart = PartInfo->GetEssenceStart();
	if(EssenceStart == -1)
	{
		if(!PartInfo->ThePartition)
		{
			File->Seek(PartInfo->GetByteOffset());
			PartInfo->ThePartition = File->ReadPartition();
		}

		if(!PartInfo->ThePartition)
		{
			error("BodyReader::Seek(%d, 0x%s) failed to read the partition\n", BodySID, Int64toHexString(Pos).c_str());
			return -1;
		}
	
		if(!(PartInfo->ThePartition->SeekEssence()))
		{
			error("BodyReader::Seek(%d, 0x%s) failed to locate essence in the predicted partition\n", BodySID, Int64toHexString(Pos).c_str());
			return -1;
		}

		EssenceStart = File->Tell();
		PartInfo->SetEssenceStart(EssenceStart);
	}

	// Seek beyond end of file is a silent failure as this may be an incomplete file
	if(File->Seek(EssenceStart + (Pos - StreamOffset)) != 0)
	{
		return -1;
	}

	return File->Tell();
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
bool BodyReader::MakeGCReader(UInt32 BodySID, GCReadHandlerPtr DefaultHandler /*=NULL*/, GCReadHandlerPtr FillerHandler /*=NULL*/)
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
	bool Ret = false;
	GCReaderPtr Reader;

	// First check if we need to re-initialise
	if(NewPos)
	{
		// Start at the new location
		File->Seek(CurrentPos);

		// Use resync to locate the next partition pack
		// TODO: We could allow reinitializing within a partition if we can validate the offsets
		//       This would involve knowledge of the partition pack for this partition which could
		//       be found by a valid RIP or by scanning backwards from the current location
		if(!ReSync()) return false;

		PartitionPtr NewPartition;				// Pointer to the new partition pack
		for(;;)
		{
			// Read the partition pack to establish offsets and BodySID
			NewPartition = File->ReadPartition();
			if(!NewPartition) return false;

			CurrentBodySID = NewPartition->GetUInt(BodySID_UL);
			if(CurrentBodySID != 0) Reader = GetGCReader(CurrentBodySID);
		
			// All done when we have found a supported BodySID
			if(Reader) break;

			// Skip non-supported essence
			// We first index the start of the essence data, then the loop causes a re-sync
			// TODO: Add faster skipping of unwanted body partitions if we have enough RIP data...
			NewPartition->SeekEssence();
			CurrentPos = File->Tell();
			AtPartition = false;

			// Move to the next partition pack then return to caller for them to inspect this pack if required
			return ReSync();
		}

		// Set the stream offset
		Position StreamOffset = NewPartition->GetUInt64("BodyOffset");
		
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
		const UInt8 *Key = ThisUL->GetValue();
		if((Key[0] == 0x06) && (Key[1] == 0x0e) && (Key[2] == 0x2b) && (Key[3] == 0x34))
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

			CurrentPos = File->Tell() + Len;

			continue;
		}

		// At this point we have read a key that does not start with the same 4 bytes as standard MXF keys
		// This could mean that we have found a valid non-SMPTE key, or that we have encountered a portion
		// of the file where data is missing or corrupted.  We not try a byte-by-byte search for a partition
		// key

		for(;;)
		{
			// Scan 64k at a time
			const UInt64 BufferLen = 65536;
			DataChunkPtr Buffer = File->Read(BufferLen);
			
			if(Buffer->Size < 16) return false;

			size_t i;									// Loop variable
			size_t End = Buffer->Size - 15;				// End of loop - 15 bytes early to allow 16-byte compares
			UInt8 *p = Buffer->Data;					// Use moving pointer for faster compares
			for(i=0; i<End; i++)
			{
				// Only perform full partition key check if it looks promising
				if((*p == 0x06) && (p[1] == 0x0e))
				{
					if(IsPartitionKey(p))
					{
						CurrentPos += i;				// Move pointer to new partition pack
						File->Seek(CurrentPos);
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


//! Register an essence key to be treated as a GC essence key
/*! This allows private or experimental essence keys to be treated as standard GC keys when reading 
 *  /note If the size is less than 16-bytes, only that part of the key given will be compared (all the rest will be treated as wildcard bytes).
 *        Byte 8 is never compared (the version number byte)
 */
void mxflib::RegisterGCElementKey(DataChunkPtr &Key)
{
	if((Key->Size < 1) || (Key->Size > 16))
	{
		error("Invalid key size %d supplied to RegisterGCElementKey()\n", (int)Key->Size);
		return;
	}

	GCEssenceKeyAlternatives.push_back(Key);
}


//! Register a system item key to be treated as a GC system key
/*! This allows private or experimental system item keys to be treated as standard GC keys when reading 
 *  /note If the size is less than 16-bytes, only that part of the key given will be compared (all the rest will be treated as wildcard bytes).
 *        Byte 8 is never compared (the version number byte)
 */
void mxflib::RegisterGCSystemKey(DataChunkPtr &Key)
{
	if((Key->Size < 1) || (Key->Size > 16))
	{
		error("Invalid key size %d supplied to RegisterGCSystemKey()\n", (int)Key->Size);
		return;
	}

	GCSystemKeyAlternatives.push_back(Key);
}


//! Get a GCElementKind structure
GCElementKind mxflib::GetGCElementKind(const ULPtr TheUL)
{
	GCElementKind ret;

	// Assume it's not a valid key
	ret.IsValid = false;

	// Note that we first test the 11th byte as this where "Application = MXF Generic Container Keys"
	// is set and so is the same for all GC keys and different in the majority of non-CG keys
	// also, avoid testing the 8th byte (version number)
	if( ( TheUL->GetValue()[10] == GCEssenceKey[10] )
	 && ( TheUL->GetValue()[9]  == GCEssenceKey[9]  )
	 && ( TheUL->GetValue()[8]  == GCEssenceKey[8]  )
	 && ( memcmp(TheUL->GetValue(), GCEssenceKey, 7 ) == 0) )
	{
		ret.IsValid = true;
	}
	else
	{
		// Scan for any registered alternative GC-type essence keys
		if(GCEssenceKeyAlternatives.size())
		{
			DataChunkList::iterator it = GCEssenceKeyAlternatives.begin();
			while(it != GCEssenceKeyAlternatives.end())
			{
				size_t Size = (*it)->Size;
				
				if(Size > 8)
				{
					// Compare top bytes first as these are the most likely to differ
					if(memcmp(&TheUL->GetValue()[8], &(*it)->Data[8], Size - 8) == 0)
					{
						// DRAGONS: Don't test the version number byte
						if(memcmp(TheUL->GetValue(), (*it)->Data, 7) == 0)
						{
							ret.IsValid = true;
							break;
						}
					}
				}
				else
				{
					// There is no point comparing exactly 8 bytes
					if(Size == 8) Size = 7;

					if(memcmp(TheUL->GetValue(), (*it)->Data, Size) == 0)
					{
						ret.IsValid = true;
						break;
					}
				}

				it++;
			}
		}
	}

	if(ret.IsValid)
	{
		ret.Item =				(TheUL->GetValue())[12];
		ret.Count =				(TheUL->GetValue())[13];
		ret.ElementType =       (TheUL->GetValue())[14];
		ret.Number =			(TheUL->GetValue())[15];
	}

	return ret;
}


//! Determine if this is a system item
bool mxflib::IsGCSystemItem(const ULPtr TheUL)
{
	// Note that we first test the 11th byte as this where "Application = MXF Generic Container Keys"
	// is set and so is the same for all GC keys and different in the majority of non-CG keys
	// also, avoid testing the 6th byte (set or pack structure) and the 8th byte (version number)
	if( ( TheUL->GetValue()[10] == GCSystemKey[10] )
	 && ( TheUL->GetValue()[9]  == GCSystemKey[9]  )
	 && ( TheUL->GetValue()[8]  == GCSystemKey[8]  )
	 && ( TheUL->GetValue()[6]  == GCSystemKey[6]  )
	 && ( memcmp(TheUL->GetValue(), GCSystemKey, 5 ) == 0) )
	{
		return true;
	}
	else
	{
		// Scan for any registered alternative GC-type system item keys
		if(GCSystemKeyAlternatives.size())
		{
			DataChunkList::iterator it = GCSystemKeyAlternatives.begin();
			while(it != GCSystemKeyAlternatives.end())
			{
				size_t Size = (*it)->Size;
				
				if(Size > 8)
				{
					// Compare top bytes first as these are the most likely to differ
					if(memcmp(&TheUL->GetValue()[8], &(*it)->Data[8], Size - 8) == 0)
					{
						// DRAGONS: Don't test the bytes 6 or 8 (array index 5 and 7)
						if((TheUL->GetValue()[6] == (*it)->Data[6]) && (memcmp(TheUL->GetValue(), (*it)->Data, 5) == 0)) return true;
					}
				}
				else
				{
					// Test byte 7 if the test key is that long
					if((Size < 7) || (TheUL->GetValue()[6] == (*it)->Data[6]))
					{
						// Test up to 5 bytes of the start of the key
						if(memcmp(TheUL->GetValue(), (*it)->Data, (Size > 5) ? 5 : Size) == 0) return true;
					}
				}

				it++;
			}
		}
	}

	return false;
}



//! Get the track number of this essence key (if it is a GC Key)
/*! \return 0 if not a valid GC Key
 */
UInt32 mxflib::GetGCTrackNumber(const ULPtr TheUL)
{
	GCElementKind Info = GetGCElementKind(TheUL);

	if(!Info.IsValid) return 0;

	return    (static_cast<UInt32>(TheUL->GetValue()[12]) << 24) | (static_cast<UInt32>(TheUL->GetValue()[13]) << 16) 
			| (static_cast<UInt32>(TheUL->GetValue()[14]) << 8)  | static_cast<UInt32>(TheUL->GetValue()[15]);
}



//! Build a list of parsers with their descriptors for a given essence file
ParserDescriptorListPtr EssenceParser::IdentifyEssence(FileHandle InFile)
{
	// Ensure the EPList is initialized
	if(!Inited) Init();

	ParserDescriptorListPtr Ret = new ParserDescriptorList;

	EssenceSubParserFactoryList::iterator it = EPList.begin();
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


//! Produce a list of available wrapping options
EssenceParser::WrappingConfigList EssenceParser::ListWrappingOptions(bool AllowMultiples, FileHandle InFile, ParserDescriptorListPtr PDList, Rational ForceEditRate, WrappingOption::WrapType ForceWrap /*=WrappingOption::None*/)
{
	WrappingConfigList Ret;

	// No options!
	if((!PDList) || (PDList->empty())) return Ret;

	// Identify the wrapping options for each descriptor
	ParserDescriptorList::iterator pdit = PDList->begin();
	while(pdit != PDList->end())
	{
		EssenceStreamDescriptorList::iterator it = (*pdit).second.begin();
		while(it != (*pdit).second.end())
		{
			// Filter out the multiples, if not allowed
			if(AllowMultiples || (!(*it)->Descriptor->IsA(MultipleDescriptor_UL)))
			{
				WrappingOptionList WO = (*pdit).first->IdentifyWrappingOptions(InFile, *(*it));

				// Extract the valid options from those identified by the parser, and add them to Ret
				ExtractValidWrappingOptions(Ret, InFile, (*it), WO, ForceEditRate, ForceWrap);
			}

			it++;
		}
		pdit++;
	}

	return Ret;
}


//! Produce a list of available wrapping options
EssenceParser::WrappingConfigList EssenceParser::ListWrappingOptions(bool AllowMultiples, FileHandle InFile, Rational ForceEditRate, WrappingOption::WrapType ForceWrap /*=WrappingOption::None*/)
{
	// Build a list of parsers with their descriptors for this essence
	ParserDescriptorListPtr PDList = EssenceParser::IdentifyEssence(InFile);

	// Return the list of options for these parsers
	// DRAGONS: This called function will cope with an empty or NULL PDList
	return ListWrappingOptions(AllowMultiples, InFile, PDList, ForceEditRate, ForceWrap);
}


//! Take a list of wrapping options and validate them agains a specifier edit rate and wrapping type
/*! All valid options are built into a WrappingConfig object and added to a specified WrappingConfigList,
 *  which may already contain other items.
 */
void EssenceParser::ExtractValidWrappingOptions(WrappingConfigList &Ret, FileHandle InFile, EssenceStreamDescriptorPtr &ESDescriptor, WrappingOptionList &WO, Rational &ForceEditRate, WrappingOption::WrapType ForceWrap)
{
	WrappingOptionList::iterator it = WO.begin();
	while(it != WO.end())
	{
		// Only accept wrappings of the specified type
		if(ForceWrap != WrappingOption::None)
		{
			if((*it)->ThisWrapType != ForceWrap)
			{
				it++;
				continue;
			}
		}

		// Attempt to build a wrapping config for this wrapping option
		EssenceParser::WrappingConfigPtr Config = new WrappingConfig;

		Config->EssenceDescriptor = ESDescriptor->Descriptor;
		MDObjectPtr SampleRate = Config->EssenceDescriptor[SampleRate_UL];

		// Work out what edit rate to use
		if((!SampleRate) || (ForceEditRate.Numerator != 0))
		{
			Config->EditRate.Numerator = ForceEditRate.Numerator;
			Config->EditRate.Denominator = ForceEditRate.Denominator;
		}
		else
		{
			Rational Preferred = (*it)->Handler->GetPreferredEditRate();

			// No preferrred rate given so use the sample rate
			if(Preferred.Numerator == 0)
			{
				Config->EditRate.Numerator = SampleRate->GetInt("Numerator");
				Config->EditRate.Denominator = SampleRate->GetInt("Denominator");
			}
			else
			{
				Config->EditRate = Preferred;
			}
		}

		// Set the parser up to wrap this essence stream
		Config->Parser = (*it)->Handler;
		Config->WrapOpt = (*it);
		Config->Stream = ESDescriptor->ID;
		Config->Parser->Use(Config->Stream, Config->WrapOpt);

		// Check if the edit rate is acceptable
		if( Config->Parser->SetEditRate(Config->EditRate) )
		{
			/* All OK, including requested edit rate - now check for any sub-streams (from the same parser) */

			EssenceStreamDescriptorList::iterator subit = ESDescriptor->SubStreams.begin();
			while(subit != ESDescriptor->SubStreams.end())
			{
				WrappingOptionList SubWO = Config->Parser->IdentifyWrappingOptions(InFile, *(*subit));

				// Extract the valid options from those identified by the parser, and add them to Ret
				ExtractValidWrappingOptions(Config->SubStreams, InFile, (*subit), SubWO, Config->EditRate, (*it)->ThisWrapType);

				subit++;
			}

			// Add this option to the list
			Ret.push_back(Config);
		}

		it++;
	}
}


//! Select the best wrapping option
EssenceParser::WrappingConfigPtr EssenceParser::SelectWrappingOption(bool AllowMultiples, FileHandle InFile, Rational ForceEditRate, WrappingOption::WrapType ForceWrap /*=WrappingOption::None*/)
{
	// Build a list of parsers with their descriptors for this essence
	ParserDescriptorListPtr PDList = EssenceParser::IdentifyEssence(InFile);

	// Wrapping config to return
	EssenceParser::WrappingConfigPtr Ret;

	// Only select a wrapping option if we can identify the essence
	if(PDList && (!PDList->empty()))
	{
		Ret = SelectWrappingOption(AllowMultiples, InFile, PDList, ForceEditRate, ForceWrap);
	}

	return Ret;
}


//! Select the best wrapping option
EssenceParser::WrappingConfigPtr EssenceParser::SelectWrappingOption(bool AllowMultiples, FileHandle InFile, ParserDescriptorListPtr PDList, Rational ForceEditRate, WrappingOption::WrapType ForceWrap /*=WrappingOption::None*/)
{
	EssenceParser::WrappingConfigPtr Ret;

	// No options!
	if(PDList->empty()) return Ret;

	// Identify the wrapping options for each descriptor
	ParserDescriptorList::iterator pdit = PDList->begin();
	while(pdit != PDList->end())
	{
		EssenceStreamDescriptorList::iterator it = (*pdit).second.begin();
		while(it != (*pdit).second.end())
		{
			WrappingOptionList WO = (*pdit).first->IdentifyWrappingOptions(InFile, *(*it));

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

				// Skip multiples if not allowed
				if((!AllowMultiples) && (*it)->Descriptor->IsA(MultipleDescriptor_UL))
				{
					it2++;
					continue;
				}

				Ret = new WrappingConfig;

				// DRAGONS: Default to the first valid option!
				Ret->EssenceDescriptor = (*it)->Descriptor;
				MDObjectPtr SampleRate = Ret->EssenceDescriptor[SampleRate_UL];

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
				Ret->Stream = (*it)->ID;

				Ret->Parser->Use(Ret->Stream, Ret->WrapOpt);
				if( Ret->Parser->SetEditRate(Ret->EditRate) )
				{
					// All OK, including requested edit rate

					// Update the SampleRate in the Descriptor to the rate in use (which may be different than its native rate)
					if(!SampleRate) SampleRate = Ret->EssenceDescriptor->AddChild(SampleRate_UL);
					if(SampleRate)
					{
						SampleRate->SetInt("Numerator", Ret->EditRate.Numerator);
						SampleRate->SetInt("Denominator", Ret->EditRate.Denominator);
					}

					EssenceStreamDescriptorList::iterator subit = (*it)->SubStreams.begin();
					while(subit != (*it)->SubStreams.end())
					{
						// Make a mini Parser Descriptor list with only the current parser and the current sub-stream
						ParserDescriptorPair PDPair;
						PDPair.first = (*pdit).first;
						PDPair.second.push_back(*subit);

						ParserDescriptorListPtr MiniPDList = new ParserDescriptorList;
						MiniPDList->push_back(PDPair);

						// See if this works as a wrapping option
						EssenceParser::WrappingConfigPtr SubWrap = SelectWrappingOption(InFile, MiniPDList, Ret->EditRate, (*it2)->ThisWrapType);

						// If this sub-stream can be used, add its wrapping config to the sub-streams list
						if(SubWrap) Ret->SubStreams.push_back(SubWrap);

						subit++;
					}

					// FIXME: This does not take into account the KAG
					Ret->WrapOpt->BytesPerEditUnit = Ret->Parser->GetBytesPerEditUnit();

					// Set the EssenceContainer Label in the descriptor or descriptors
					if(Ret->EssenceDescriptor->IsA(MultipleDescriptor_UL))
					{
						// Set the EssenceContainer Label for the multiple descriptor
						Ret->EssenceDescriptor->SetValue(EssenceContainer_UL, DataChunk(16, GCMulti_Data));

						MDObjectPtr SubPtr = Ret->EssenceDescriptor[SubDescriptorUIDs_UL];
						if(SubPtr)
						{
							MDObject::iterator it = SubPtr->begin();
							while(it != SubPtr->end())
							{
								MDObjectPtr Link = (*it).second->GetLink();
								if(Link) Link->SetValue(EssenceContainer_UL, DataChunk(16,Ret->WrapOpt->WrappingUL->GetValue()));
								it++;
							}
						}
					}
					else
					{
						// Set the EssenceContainer Label in the single descriptor
						Ret->EssenceDescriptor->SetValue(EssenceContainer_UL, DataChunk(16,Ret->WrapOpt->WrappingUL->GetValue()));
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

	// Failed to select - return what will be NULL
	return Ret;
}


//! Select the specified wrapping options
void EssenceParser::SelectWrappingOption(EssenceParser::WrappingConfigPtr Config)
{
	// Ensure that these settings are in use by the parser
	Config->Parser->Use(Config->Stream, Config->WrapOpt);
	Config->Parser->SetEditRate(Config->EditRate);

	// Update the SampleRate in the Descriptor to the rate in use (which may be different than its native rate)
	MDObjectPtr SampleRate = Config->EssenceDescriptor[SampleRate_UL];
	if(!SampleRate) SampleRate = Config->EssenceDescriptor->AddChild(SampleRate_UL);
	if(SampleRate)
	{
		SampleRate->SetInt("Numerator", Config->EditRate.Numerator);
		SampleRate->SetInt("Denominator", Config->EditRate.Denominator);
	}

	// FIXME: This does not take into account the KAG
	Config->WrapOpt->BytesPerEditUnit = Config->Parser->GetBytesPerEditUnit();

	// Set the EssenceContainer Label in the descriptor or descriptors
	if(Config->EssenceDescriptor->IsA(MultipleDescriptor_UL))
	{
		// Set the EssenceContainer Label for the multiple descriptor
		Config->EssenceDescriptor->SetValue(EssenceContainer_UL, DataChunk(16, GCMulti_Data));

		MDObjectPtr SubPtr = Config->EssenceDescriptor[SubDescriptorUIDs_UL];
		if(SubPtr)
		{
			MDObject::iterator it = SubPtr->begin();
			while(it != SubPtr->end())
			{
				MDObjectPtr Link = (*it).second->GetLink();
				if(Link) Link->SetValue(EssenceContainer_UL, DataChunk(16,Config->WrapOpt->WrappingUL->GetValue()));
				it++;
			}
		}
	}
	else
	{
		// Set the EssenceContainer Label in the single descriptor
		Config->EssenceDescriptor->SetValue(EssenceContainer_UL, DataChunk(16,Config->WrapOpt->WrappingUL->GetValue()));
	}

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
	UInt32 UseKAG;
	if(Stream->GetKAG()) UseKAG = Stream->GetKAG(); else UseKAG = KAG;

	// If either setting is to force BER4 we will force it
	bool UseForceBER4 = ForceBER4 || Stream->GetForceBER4();

	// Set the KAG for this Essence Container
	Writer->SetKAG(UseKAG, UseForceBER4);

	// Get a pointer to the index manager (if there is one)
	IndexManagerPtr &IndexMan = Stream->GetIndexManager();

	// Set flag if we need to build a VBR index table
	bool VBRIndex;
	if((Stream->GetIndexType() != BodyStream::StreamIndexNone) && (IndexMan) && (!IndexMan->IsCBR()))
		VBRIndex = true;
	else
		VBRIndex = false;

	// Set flag if we need to add a sparse index entry this time (cleared once the sparse entry has been added).
	// If the previous pass for this stream left some data pending then it will already be indexed and so there
	// is no need to index the first entry of this pass.
	bool SparseIndex;
	if(VBRIndex && (!Stream->HasPendingData()) && (Stream->GetIndexType() & BodyStream::StreamIndexSparseFooter))
		SparseIndex = true;
	else
		SparseIndex = false;

	// Sort clip-wrap if that is what we are doing
	if(Stream->GetWrapType() == BodyStream::StreamWrapClip)
	{
		// Add essence from each sub-stream to the writer
		BodyStream::iterator it = Stream->begin();
		while(it != Stream->end())
		{
			// Get the stream ID for this sub-stream
			GCStreamID EssenceID = (*it)->GetStreamID();

			// Add the essence to write - using FastClipWrap if available
			Writer->AddEssenceData(EssenceID, (*it), GetFastClipWrap());
			it++;
		}

		// Write the current partition pack (there should be one pending if all is normal)
		if(PartitionWritePending) EndPartition();

		// Write the essence
		Writer->StartNewCP();

		// After writing the clip this must be the end of the stream
		Stream->SetEndOfStream(true);

		// DRAGONS: Is this the right thing to do?
		if(VBRIndex)
		{
			// Index the first edit unit of the essence for clip-wrap
			// FIXME: we need to do proper clip-wrap indexing!!
			Position EditUnit = IndexMan->AcceptProvisional();
			if(EditUnit == IndexTable::IndexLowest) EditUnit = IndexMan->GetLastNewEditUnit();
			Stream->SparseList.push_back(EditUnit);

			// If we are requested to add a "free space" index entry do so here
			if(Stream->GetFreeSpaceIndex())
			{
				// Remove the KLSize if doing ValueRelativeIndexing
				Position KLSize = 0;
				if( IndexMan->GetValueRelativeIndexing() ) KLSize = 0x19; // FIXME: don't know how to calculate

				// Add free space entry for each sub-stream
				BodyStream::iterator it = Stream->begin();
				while(it != Stream->end())
				{
					IndexMan->OfferOffset((*it)->GetIndexStreamID(), EditUnit + 1, Writer->GetStreamOffset() - KLSize);
					it++;
				}
			}
		}

		// FIXME: We don't yet count the duration of the clip wrapped essence
	}
	else
	{
		// We need to check the start of the partition on the first iteration
		bool FirstIteration = true;

		// Start of the current partition
		Position PartitionStart = 0;

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
				Length PrechargeSize = Stream->GetPrechargeSize();

				// Add this chunk of each essence sub-stream to the writer
				BodyStream::iterator it = Stream->begin();
				while(it != Stream->end())
				{
					// This data chunk
					DataChunkPtr Dat;

					// Do any required indexing (stream offset is updated when the content package is written)
					if(VBRIndex)
					{
						if(SparseIndex)
						{
							Position EditUnit;

							// If we are in the precharge, work out the -ve position (GetCurrentPosition may return 0 through the precharge)
							if(PrechargeSize) EditUnit = 0 - PrechargeSize;
							else EditUnit = (*it)->GetCurrentPosition();

							Stream->SparseList.push_back(EditUnit);
			
							// Sparce entry recorded for this partition
							SparseIndex = false;
						}
					}

					// Skip anything that currently doesn't have any precharge data to write
					if(PrechargeSize && (*it)->GetPrechargeSize() < PrechargeSize)
					{
						// Write an empty item for all skipped items
						Dat = NULL;
					}
					else
					{
						// Read the next data for this sub-stream
						Dat = (*it)->GetEssenceData();
					}

					// Get the stream ID for this sub-stream
					GCStreamID EssenceID = (*it)->GetStreamID();

					// Decide if we are actually writing anything this CP...
					if(Dat) DataWrittenThisCP = true;
					// ... if not, write an empty item 
					else Dat = new DataChunk;

					// Add this chunk of essence to the writer
					Writer->AddEssenceData(EssenceID, Dat);

					// Move to the next stream
					it++;
				}

				// Nothing remaining - all done
				if((!DataWrittenThisCP) && (PrechargeSize == 0))
				{
					Stream->SetEndOfStream(true);
					Stream->GetNextState();

					// If we are requested to add a "free space" index entry do so here
					if(Stream->GetFreeSpaceIndex())
					{
						Position EditUnit = IndexMan->AcceptProvisional();
						if(EditUnit == IndexTable::IndexLowest) EditUnit = IndexMan->GetLastNewEditUnit();

						if(EditUnit != IndexTable::IndexLowest)
						{
							// Remove the KLSize if doing ValueRelativeIndexing
							Position KLSize = 0;
							if( IndexMan->GetValueRelativeIndexing() ) KLSize = 0x19;  // FIXME: don't know how to calculate

							// Add free space entry for each sub-stream
							BodyStream::iterator it = Stream->begin();
							while(it != Stream->end())
							{
								IndexMan->OfferOffset((*it)->GetIndexStreamID(), EditUnit + 1, Writer->GetStreamOffset() - KLSize);
								it++;
							}
						}
					}

					return Ret;
				}

				// If we have been writing precharge, reduce the count
				if(PrechargeSize) Stream->DecrementPrecharge();

				// Now we have written something we must record the BodySID
				PartitionBodySID = CurrentBodySID;
			}

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
						if(VBRIndex && (Stream->GetIndexType() & BodyStream::StreamIndexSparseFooter))
						{
							// ...force the first edit unit of the new partition to be accepted, even if provisional,
							// and add it's edit unit to the sparse list
							Position EditUnit = IndexMan->AcceptProvisional();
							if(EditUnit == IndexTable::IndexLowest) EditUnit = IndexMan->GetLastNewEditUnit();
							Stream->SparseList.push_back(EditUnit);
						}

						// Flag that we have "stored" some essence for next partition
						Stream->SetPendingData();

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
		// It should now be safe to read the pre-charge size, so set it to the highest of any sub-streams
		// DRAGONS: No point in doing any of this if we are a single stream
		if(size() > 1) 
		{
			BodyStream::iterator it = begin();
			while(it != end())
			{
				Length PSize = Source->GetPrechargeSize();
				if(PSize > PrechargeSize) PrechargeSize = PSize;
				it++;
			}
		}

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
			if(StreamIndex & ( StreamIndexSparseFooter | StreamIndexCBRFooter | StreamIndexFullFooter ))
			{
				State = BodyStreamFootIndex;
			}
			// Check if we have any left-over sprinkles
			else if((StreamIndex & StreamIndexSprinkled)
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
			if(StreamIndex & ( StreamIndexSparseFooter | StreamIndexCBRFooter | StreamIndexFullFooter ))
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

	// Initialize any index managers required for this writer before we write the header
	InitIndexManagers();
	
	// Turn the partition into the correct type of header
	if(IsClosed)
		if(IsComplete)
			BasePartition->ChangeType(ClosedCompleteHeader_UL);
		else
			BasePartition->ChangeType(ClosedHeader_UL);
	else
		if(IsComplete)
			BasePartition->ChangeType(OpenCompleteHeader_UL);
		else
			BasePartition->ChangeType(OpenHeader_UL);

	// Initially there is no body data
	BasePartition->SetUInt(BodySID_UL, 0);
	BasePartition->SetUInt(BodyOffset_UL, 0);

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

	// TODO: We should really make two passes through the streams, the first pass writes any CBR index tables
	//       that are isolated and the second pass writes non-isolated index tables - this will allow the last
	//       non-isolated table to be followed by essence if possible

	// Find any streams that need a CBR index in the header
	StreamInfoList::iterator it = StreamList.begin();
	while(it != StreamList.end())
	{
		// Get a pointer to the stream
		BodyStreamPtr &Stream = (*it)->Stream;

		// Check the stream's state
		BodyStream::StateType State = Stream->GetState();

		// Only index if the stream is expecting it
		if(State == BodyStream::BodyStreamHeadIndex)
		{
			// Get the index type flags
			BodyStream::IndexType IndexFlags = Stream->GetIndexType();

			// Build an index table if required in the header
			if(IndexFlags & (BodyStream::StreamIndexCBRHeader | BodyStream::StreamIndexCBRHeaderIsolated))
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
					if(HeaderWritten) BasePartition->ChangeType(ClosedCompleteBodyPartition_UL);

					// Set the index SID
					BasePartition->SetUInt(IndexSID_UL,  Stream->GetIndexSID());

					// Record the index data to write
					PendingIndexData = IndexChunk;

					// Queue the write
					PartitionWritePending = true;

					// Now the header has been written (or at least is pending)
					HeaderWritten = true;

					// If this table is CBR set the partition done (so if we are last we will be isolated from essence by forcing a new partition)
					if(IndexFlags & BodyStream::StreamIndexCBRHeaderIsolated) PartitionDone = true;
					else PartitionDone = false;
				}
			}

			// Move the stream to the next state
			Stream->GetNextState();
		}

		it++;
	}

	// If no index table was written we will write a header with no index data
	if(!HeaderWritten)
	{
		// Flag no index data
		BasePartition->SetUInt(IndexSID_UL,  0);
		PendingIndexData = NULL;

		// Queue the write
		PartitionWritePending = true;
	}

	// Flush the partition if it is done
	if(PartitionDone) EndPartition();

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
			if(PartitionHandler) WriteMetadata = PartitionHandler->HandlePartition(BodyWriterPtr(this), CurrentBodySID, BasePartition->GetUInt(IndexSID_UL));
		}

		// FIXME: Need to force a separate partition pack if we are about to violate the metadata sharing rules

		if(PendingIndexData)
		{
			File->WritePartitionWithIndex(BasePartition, PendingIndexData, WriteMetadata, NULL, MinPartitionFiller, MinPartitionSize);
			
			// Clear the index data SID to prevent it being written again next time
			BasePartition->SetUInt(IndexSID_UL,  0);
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
		PartitionBodySID = 0;

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
	case BodyStream::BodyStreamPreBodyIndex:
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
			BasePartition->ChangeType(ClosedCompleteBodyPartition_UL);

			// There is no body data as we are isolated
			BasePartition->SetUInt(BodySID_UL, 0);
			BasePartition->SetUInt(BodyOffset_UL, 0);
			PartitionBodySID = 0;

			// Set the index SID
			BasePartition->SetUInt(IndexSID_UL,  Index->IndexSID);

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
	case BodyStream::BodyStreamBodyWithIndex:
		{
			// Ensure we don't share with metadata if not permitted
			if((!IndexSharesWithMetadata) && (!PartitionDone))
			{
				if(PartitionWritePending)
				{
					if(PendingHeader || PendingMetadata) PartitionDone = true;
				}
				else
				{
					if(BasePartition->GetUInt64(HeaderByteCount_UL) > 0) PartitionDone = true;
				}
			}

			// Fall through to no-index version
		}

	case BodyStream::BodyStreamBodyNoIndex:
		{
			// Ensure we don't share with metadata if not permitted
			if((!EssenceSharesWithMetadata) && (!PartitionDone))
			{
				if(PartitionWritePending)
				{
					if(PendingHeader || PendingMetadata) PartitionDone = true;
				}
				else
				{
					if(BasePartition->GetUInt64(HeaderByteCount_UL) > 0) PartitionDone = true;
				}
			}

			// It's OK to continue with the current partition if:
			//   1) The partition is not "done"
			// AND
			//   2) Either:
			//       a) The partition is of our BodySID
			//      OR 
			//       b) The partition is of no BodySID AND the partition pack has not yet been written (so we can set BodySID)
			//

			// Work out if we should continue the partition
			bool ContinuePartition = !PartitionDone;
			if(ContinuePartition && (PartitionBodySID != CurrentBodySID))
			{
				if((PartitionBodySID != 0) || (!PartitionWritePending)) ContinuePartition = false;
			}

			// Start a new partition if we can't continue the old one
			if(!ContinuePartition)
			{
				// Flush any previously pending partition
				if(PartitionWritePending) EndPartition();

				// Assume a closed complete body partition - may be changed by handler if metadata added
				BasePartition->ChangeType(ClosedCompleteBodyPartition_UL);

				// We now have a new partition pending
				PartitionWritePending = true;
			}

			// If there is a partition pending then update it and write it
			if(PartitionWritePending)
			{
				BasePartition->SetUInt(BodySID_UL, CurrentBodySID);
				BasePartition->SetUInt64(BodyOffset_UL, Stream->GetWriter()->GetStreamOffset());

				if(StreamState == BodyStream::BodyStreamBodyWithIndex)
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
						if(Stream->GetIndexType() & BodyStream::StreamIndexSprinkled)
						{
							Stream->SetNextSprinkled(Stream->GetNextSprinkled() + Count);
						}
					}

					// Write the index table
					DataChunkPtr IndexChunk = new DataChunk;
					Index->WriteIndex(*IndexChunk);

					// We will be a closed complete body partition unless the partition handler adds metadata
					if(!PendingHeader) BasePartition->ChangeType(ClosedCompleteBodyPartition_UL);

					// Set the index SID
					BasePartition->SetUInt(IndexSID_UL,  Index->IndexSID);

					// Record the index data to write
					PendingIndexData = IndexChunk;
				}
				else
				{
					// No index data
					BasePartition->SetUInt(IndexSID_UL,  0);
				}

				// Note: The partition will be written by the call to WriteEssence
			}

			// Write the Essence
			Ret += WriteEssence((*CurrentStream), Duration, MaxPartitionSize);

			// Check the new state for this stream
			StreamState = Stream->GetState();

			// If this stream has done a cycle - move to the next stream
			if(StreamState != BodyStream::BodyStreamPostBodyIndex) SetNextStream();

			break;
		}

	// Next action: Write an isolated index table after a body partition
	case BodyStream::BodyStreamPostBodyIndex:
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
				if(!(Stream->GetIndexType() & BodyStream::StreamIndexSprinkled))
				{
					Stream->SetNextSprinkled(Stream->GetNextSprinkled() + Count);
				}
			}

			// Write the index table
			DataChunkPtr IndexChunk = new DataChunk;
			Index->WriteIndex(*IndexChunk);

			// Isolated index tables generally live in closed complete body partitions
			BasePartition->ChangeType(ClosedCompleteBodyPartition_UL);

			// There is no body data as we are isolated
			BasePartition->SetUInt(BodySID_UL, 0);
			BasePartition->SetUInt(BodyOffset_UL, 0);
			PartitionBodySID = 0;

			// Set the index SID
			BasePartition->SetUInt(IndexSID_UL,  Index->IndexSID);

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
	case BodyStream::BodyStreamFootIndex:
	case BodyStream::BodyStreamDone:
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
	BasePartition->ChangeType(ClosedCompleteBodyPartition_UL);

	// There is no body data in a footer
	BasePartition->SetUInt(BodySID_UL, 0);
	BasePartition->SetUInt(BodyOffset_UL, 0);

	// Initially there is no index data
	BasePartition->SetUInt(IndexSID_UL,  0);

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
					  (  BodyStream::StreamIndexFullFooter  | BodyStream::StreamIndexSparseFooter
					   | BodyStream::StreamIndexSprinkled   | BodyStream::StreamIndexSprinkledIsolated
					   | BodyStream::StreamIndexCBRFooter ));

		/* Note: The index tables will be writen in such an order as to keep the footer
		 *       as small as possible (to allow it to be located easily).
		 *       The order is:
		 *           Any remaining body index tables (left-over sprinkles)
		 *           Full VBR index table
		 *           Sparse VBR index table
		 *           CBR index table
		 */

		// No more index data to write
		if(IndexFlags == BodyStream::StreamIndexNone)
		{
			// Clear the flags in case we are used to write another file (probably not a sane thing to do)
			Stream->SetIndexType(BodyStream::StreamIndexNone);

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
			IndexFlags = (BodyStream::IndexType) (IndexFlags & BodyStream::StreamIndexCBRFooter);

			// Check we are supposed to be writing a CBR index table
			ASSERT(IndexFlags);
		}
		// If the index table is VBR we need to build it
		else
		{
			// First off we write any remaining sprinkles
			if(IndexFlags & (BodyStream::StreamIndexSprinkled | BodyStream::StreamIndexSprinkledIsolated))
			{
				// Add any remaining entries to make a sprinkled index table
				Position EditUnit = IndexMan->GetLastNewEditUnit();
				IndexMan->AddEntriesToIndex(Index, Stream->GetNextSprinkled(), EditUnit);

				// We have now done the remaining sprinkles
				IndexFlags = (BodyStream::IndexType) (IndexFlags & (BodyStream::StreamIndexSprinkled | BodyStream::StreamIndexSprinkledIsolated));
			}
			else if(IndexFlags & BodyStream::StreamIndexFullFooter)
			{
				// Add all available edit units
				IndexMan->AddEntriesToIndex(Index);

				// We have now done the full index
				IndexFlags = BodyStream::StreamIndexFullFooter;
			}
			else if(IndexFlags & BodyStream::StreamIndexSparseFooter)
			{
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
//					IndexMan->AddEntriesToIndex(true, Index, (*it), (*it));
					IndexMan->AddEntriesToIndex(Index, (*it), (*it));
					it++;
				}

				// We have now done the sparse index
				IndexFlags = BodyStream::StreamIndexSparseFooter;
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

		// Don't write empty index partitions
		if(IndexChunk->Size)
		{
			// Set the index SID
			BasePartition->SetUInt(IndexSID_UL,  Index->IndexSID);

			// Record the index data to write
			PendingIndexData = IndexChunk;

			// Queue the write
			PartitionWritePending = true;
		}

		// Set the "done" flag for this index type
		// DRAGONS: Is this an MSVC funny or can we really not do bitmaths with enums without them becoming integers?
		BodyStream::IndexType Flags = Stream->GetFooterIndex();
		Stream->SetFooterIndex((BodyStream::IndexType) (Flags | IndexFlags) );
		if(Flags == Stream->GetFooterIndex())
		{
			error("Internal Error: Failed to clear footer index flag 0x%04x for BodySID 0x%04x\n", (int)IndexFlags, (int)CurrentBodySID);
			
			// Flag all indexing done
			Stream->SetFooterIndex(Stream->GetIndexType());
		}

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
		BasePartition->ChangeType(CompleteFooter_UL);
	else
		BasePartition->ChangeType(Footer_UL);

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
	int MaxIters = static_cast<int>(StreamList.size());


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
			if(StreamState == BodyStream::BodyStreamDone)
			{
				(*CurrentStream)->Active = false;
			}
			// If we are doing the header find a stream that is usable in the header
			else if(State == BodyStateHeader)
			{
				if(StreamState == BodyStream::BodyStreamHeadIndex)
				{
					// Set the BodySID and stop looking
					CurrentBodySID = (*CurrentStream)->Stream->GetBodySID();
					return;
				}
			}
			// If we are doing the footer find a stream that is usable in the footer
			else if(State == BodyStateFooter)
			{
				if(StreamState == BodyStream::BodyStreamFootIndex)
				{
					// Set the BodySID and stop looking
					CurrentBodySID = (*CurrentStream)->Stream->GetBodySID();
					return;
				}
			}
			// Se we must be in the body - find a body stream
			else
			{
				if((StreamState != BodyStream::BodyStreamHeadIndex) && (StreamState != BodyStream::BodyStreamFootIndex))
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
	UInt32 SID = Stream->GetBodySID();
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

	// Set this stream to use our KAG
	// TODO: This is probably not the best way - but is the only way to currently ensure correct CBR indexing!
	Stream->SetKAG(KAG);

	// Add at the end of the list
	StreamList.push_back(NewStream);

	// Ensure that this stream has a writer
	GCWriterPtr NewWriter = new GCWriter(File, SID);
	if(!Stream->GetWriter()) Stream->SetWriter(NewWriter);

	return true;
}




//! Add a new sub-stream
void BodyStream::AddSubStream(EssenceSourcePtr &SubSource, DataChunkPtr Key /*=NULL*/, bool NonGC /*=false*/) 
{
	// Add the new stream
	push_back(SubSource); 

	// If a key has been specified inform the source
	if(Key) SubSource->SetKey(Key, NonGC);

	// If the writer has already been defined add this stream to it
	if(StreamWriter)
	{
		GCStreamID EssenceID;

		if(Key)
			EssenceID = StreamWriter->AddEssenceElement(Key, SubSource->GetBERSize(), NonGC);
		else
			EssenceID = StreamWriter->AddEssenceElement(SubSource->GetGCEssenceType(), SubSource->GetGCElementType(), SubSource->GetBERSize());

		SubSource->SetStreamID(EssenceID);

		// Ensure the write-order is corrected if required
		if(SubSource->RelativeWriteOrder())
		{
			StreamWriter->SetRelativeWriteOrder(EssenceID, SubSource->RelativeWriteOrderType(), SubSource->RelativeWriteOrder());
		}
	}
}


//! Set the current GCWriter
void BodyStream::SetWriter(GCWriterPtr &Writer) 
{ 
	// Check that we haven't tried to add two writers
	if(StreamWriter)
	{
		error("BodyStream::SetWriter called - but this stream already has a GCWriter\n");
		return;
	}

	// Set the writer
	StreamWriter = Writer;

	// Add each existing stream to the new writer
	BodyStream::iterator it = begin();
	while(it != end())
	{
		GCStreamID EssenceID;
		
		if((*it)->GetKey())
			EssenceID = StreamWriter->AddEssenceElement((*it)->GetKey(), (*it)->GetBERSize(), (*it)->GetNonGC());
		else
			EssenceID = StreamWriter->AddEssenceElement((*it)->GetGCEssenceType(), (*it)->GetGCElementType(), (*it)->GetBERSize());

		(*it)->SetStreamID(EssenceID);

		// Ensure the write-order is corrected if required
		if((*it)->RelativeWriteOrder())
		{
			StreamWriter->SetRelativeWriteOrder(EssenceID, (*it)->RelativeWriteOrderType(), (*it)->RelativeWriteOrder());
		}

		it++;
	}
}

//! Parse a given multi-file name
void ListOfFiles::ParseFileName(std::string FileName)
{
	// Initialize settings
	ListOrigin = 0;
	ListIncrement = 1;
	ListNumber = -1;
	ListEnd = -1;
	RangeStart = -1;
	RangeEnd = -1;
	RangeDuration = -1;
	FileList = false;

	// Length of input string including terminating zero
	size_t InLength = FileName.size() + 1;

	// Build a buffer long enough for the longest base name
	char *NameBuffer = new char[InLength];

	// Build a buffer to receive a copy of the input string
	char *InBuffer = new char[InLength];

	// Make a safe copy of the input data so we can walk it for parsing
	memcpy(InBuffer, FileName.c_str(), InLength);

	char *pIn = InBuffer;
	char *pOut = NameBuffer;
	bool InBrackets = false;
	bool InCount = false;
	bool InStep = false;
	bool InEnd = false;

	while(*pIn)
	{
		if(!InBrackets)
		{
			if(*pIn == '[')
			{
				InBrackets = true;
				pIn++;
				continue;
			}
			else
			{
				// Handle escapes
				if(*pIn == '\\')
				{
					if(pIn[1] == '[') pIn++;
				}
				
				*pOut++ = *pIn++;
				continue;
			}
		}

		if(*pIn == ']')
		{
			InBrackets = false;
			FileList = true;
			pIn++;
			continue;
		}

		if(isdigit(*pIn))
		{
			if(InStep)
				ListIncrement = ListIncrement * 10 + (*pIn - '0');
			else if(InCount)
				ListNumber = ListNumber * 10 + (*pIn - '0');
			else if(InEnd)
				ListEnd = ListEnd * 10 + (*pIn - '0');
			else
				ListOrigin = ListOrigin * 10 + (*pIn - '0');
		}
		else
		{
			if(*pIn == '#')
			{
				InCount = true;
				InEnd = false;
				InStep = false;
				ListNumber = 0;
			}
			else if(*pIn == '+')
			{
				InCount = false;
				InEnd = false;
				InStep = true;
				ListIncrement = 0;
			}
			else if(*pIn == ':')
			{
				InCount = false;
				InEnd = true;
				InStep = false;
				ListEnd = 0;
			}
			else
			{
				error("Unknown pattern in multiple filename \"%s\"\n", InBuffer);
			}
		}

		pIn++;
	}

	// Set the base filename and initialize list
	*pOut = '\0';
	FileNumber = ListOrigin;

	// Calculate how many files we should read
	// Note that we stop at the earliest of either the count or the end file number
	FilesRemaining = ListNumber;
	if((ListNumber < 0) || ((ListEnd > 0) && (ListOrigin + (ListIncrement * FilesRemaining) > ListEnd)))
	{
		if(ListIncrement) FilesRemaining = ((ListEnd - FileNumber) / ListIncrement) + 1;
	}

	AtEOF = false;


	// Use the processed filename if we have identified a file list, otherwise leave unchanged
	if(FileList) BaseFileName = std::string(NameBuffer);
	else BaseFileName = FileName;

	// Free the name buffers
	delete[] NameBuffer;
	delete[] InBuffer;
}


//! Identify the essence type in the first file in the set of possible files
ParserDescriptorListPtr FileParser::IdentifyEssence(void)
{
	ParserDescriptorListPtr Ret;

	// If we haven't got a source file open - open it
	if(!CurrentFileOpen) 
	{
		if(!GetNextFile()) return Ret;
	}

	// Identify the options
	Ret = EssenceParser::IdentifyEssence(CurrentFile);

	return Ret;
}


//! Produce a list of available wrapping options
EssenceParser::WrappingConfigList FileParser::ListWrappingOptions(bool AllowMultiples, ParserDescriptorListPtr PDList, Rational ForceEditRate, WrappingOption::WrapType ForceWrap /*=WrappingOption::None*/)
{
	return EssenceParser::ListWrappingOptions(AllowMultiples, CurrentFile, PDList, ForceEditRate, ForceWrap);
}


//! Select the best wrapping option with a forced edit rate
EssenceParser::WrappingConfigPtr FileParser::SelectWrappingOption(bool AllowMultiples, ParserDescriptorListPtr PDList, Rational ForceEditRate, WrappingOption::WrapType ForceWrap /*=WrappingOption::None*/)
{
	EssenceParser::WrappingConfigPtr Ret = EssenceParser::SelectWrappingOption(AllowMultiples, CurrentFile, PDList, ForceEditRate, ForceWrap);

	// Record the selected stream and sub-parser
	if(Ret)
	{
		CurrentStream = Ret->Stream;
		SubParser = Ret->Parser;
		CurrentDescriptor = Ret->EssenceDescriptor;
	}
	else 
		SubParser = NULL;
	
	return Ret;
}


//! Select the specified wrapping options
void FileParser::SelectWrappingOption(EssenceParser::WrappingConfigPtr Config)
{
	EssenceParser::SelectWrappingOption(Config);

	// Record the selected stream and sub-parser
	CurrentStream = Config->Stream;
	SubParser = Config->Parser;
	CurrentDescriptor = Config->EssenceDescriptor;
}


//! Set a wrapping option for this essence
/*! IdentifyEssence() and IdentifyWrappingOptions() must have been called first
 */
void FileParser::Use(UInt32 Stream, WrappingOptionPtr &UseWrapping)
{
	if(!SubParser) return;
	
	if(Stream != CurrentStream)
	{
		error("FileParsers can currently only parse one stream at a time\n");
		return;
	}

	// Record the wrapping options
	CurrentWrapping = UseWrapping;

	SubParser->Use(Stream, UseWrapping);
}


//! Open the next file in the set of source files
/*! \return true if all OK, false if no file or error
 */
bool ListOfFiles::GetNextFile(void)
{
	// Close any currently open file
	if(IsFileOpen())
	{
		CloseFile();
	}

	if(FilesRemaining == 0)
	{
		// No more files - all done
		if(FollowingNames.empty())
		{
			AtEOF = true;
			return false;
		}

		// Get the next filename pattern from the list
		std::string NextName = FollowingNames.front();

		// Remove it from the list
		FollowingNames.pop_front();

		// Parse the pattern
		ParseFileName(NextName);
	}

	// Allocate a buffer to build the file name
	char *NameBuffer = new char[1024];

	// Build the first file name
	sprintf(NameBuffer, BaseFileName.c_str(), FileNumber);

	// Get the current name as a srting
	CurrentFileName = std::string(NameBuffer);

	// Free the name buffer
	delete[] NameBuffer;

	// Get the next file number
	FileNumber += ListIncrement;

	// Decrement the count if required
	if(FilesRemaining > 0) FilesRemaining--;

	// Inform our handler (who may change or even invalidate the file name)
	if(Handler) Handler->NewFile(CurrentFileName);

	// Validate the file open 
	if(!OpenFile())
	{
		AtEOF = true;
		return false;
	}

	return true;
}



//! Set the sequential source to use the EssenceSource from the currently open and identified source file
/*! \return true if all OK, false if no EssenceSource available
 */
bool FileParser::GetFirstSource(void)
{
	// We need to have a file open
	if(!CurrentFileOpen) return false;

	// Must have a sub-parser set already
	if(!SubParser) return false;

	// Set the new EssenceSource
	SequentialEssenceSource *Source = SmartPtr_Cast(SeqSource, SequentialEssenceSource);
	Source->SetSource(SubParser->GetEssenceSource(CurrentFile, CurrentStream));

	return true;
}


//! Set the sequential source to use an EssenceSource from the next available source file
/*! \return true if all OK, false if no EssenceSource available
 */
bool FileParser::GetNextSource(void)
{
	// Must have a sub-parser set already
	if(!SubParser) return false;

	// Open the next file, unless the current source is being requested
	if(!GetNextFile()) return false;

	// If this essence parser supports a quick re-validate of a new file, do it
	if(SubParser->CanReValidate())
	{
		if(!SubParser->ReValidate(CurrentFile, CurrentStream, CurrentDescriptor, CurrentWrapping))
		{
			error("File \"%s\" exists but is not the same essence type as the previous file\n", CurrentFileName.c_str());
			return false;
		}

		return true;
	}

	/** Must restart parsing the hard way **/

	// Sanity check that we don't have sub-streams
	// If we do the ESP is incorrect and could well do nasty things when we close the file!
	if(!SubStreams.empty())
	{
		error("EssenceSubParsers that extract sub-streams must support the ReValidate() method\n");
	}

	// Build a new parser of the same type as there is no guarantee that older parsers can be re-used
	EssenceSubParserPtr NewParser = SubParser->NewParser();

	// Identify the essence
	EssenceStreamDescriptorList ESDList = NewParser->IdentifyEssence(CurrentFile);

	// Scan for a matching wrapping
	bool FoundMatch = false;
	EssenceStreamDescriptorList::iterator it = ESDList.begin();
	while(it != ESDList.end())
	{
		if((*it)->ID == CurrentStream)
		{
			WrappingOptionList WOList = NewParser->IdentifyWrappingOptions(CurrentFile, *(*it));
		
			WrappingOptionList::iterator WO_it = WOList.begin();
			while(WO_it != WOList.end())
			{
				// If the mapping seems to match, use it
				if(    (*(*WO_it)->WrappingUL == *CurrentWrapping->WrappingUL)
					&& ((*WO_it)->ThisWrapType == CurrentWrapping->ThisWrapType) )
				{
					NewParser->Use(CurrentStream, (*WO_it));
					
					FoundMatch = true;

					break;
				}

				WO_it++;
			}
		}

		if(FoundMatch) break;

		it++;
	}

	if(!FoundMatch)
	{
		error("File \"%s\" exists but is not the same essence type as the previous file\n", CurrentFileName.c_str());
		return false;
	}

	// Switch to the new parser
	SubParser = NewParser;

	// Set the new EssenceSource
	SequentialEssenceSource *Source = SmartPtr_Cast(SeqSource, SequentialEssenceSource);
	Source->SetSource(SubParser->GetEssenceSource(CurrentFile, CurrentStream));

	return true;
}


//! Get the next "installment" of essence data
DataChunkPtr FileParser::SequentialEssenceSource::GetEssenceData(size_t Size /*=0*/, size_t MaxSize /*=0*/ )
{ 
	// We need a valid source to continue
	if(!ValidSource()) return NULL;

	// If we have emptied all files then exit now
	if(Outer->AtEOF) return NULL;

	// Get the next data from the current source
	DataChunkPtr Ret = CurrentSource->GetEssenceData(Size, MaxSize);

	// If no more data move to the next source file
	if(!Ret)
	{
		// Work out how much was read from this file
		Length CurrentSize = (Length)CurrentSource->GetCurrentPosition();

		if(Outer->GetNextSource())
		{
			// Add this length to the previous lengths
			PreviousLength += CurrentSize;

			return GetEssenceData(Size, MaxSize);
		}
	}

	return Ret;
}


//! Return the sequential EssenceSource for the main stream (already aquired internally, so no need to use the stream ID)
EssenceSourcePtr FileParser::GetEssenceSource(UInt32 Stream) 
{
	if(Stream != CurrentStream)
	{
		error("A stream of ID 0x%04x was requested from a file parser that is configured for ID 0x%04x\n", Stream, CurrentStream);
		return NULL;
	}

	// Ensure that the master source is installed - this is required if there are any sub-streams
	GetFirstSource();

	return SeqSource; 
}


//! Build an EssenceSource to read from the specified sub-stream
EssenceSourcePtr FileParser::GetSubSource(UInt32 Stream)
{
	// First check if there is already a source for this stream
	SubStreamList::iterator it = SubStreams.begin();
	while(it != SubStreams.end())
	{
		if((*it).StreamID == Stream) return (*it).Source;
		it++;
	}

	// If we don't have a parser for the main stream - quit now
	if(!SubParser) return NULL;


	//! Build a new info block
	SubStreamInfo Info;
	Info.StreamID = Stream;
	Info.Source = SubParser->GetEssenceSource(CurrentFile, Stream);

	// Only add if successful
	if(Info.Source) SubStreams.push_back(Info);

	// Return the result of the sub-get
	return Info.Source;
}


//! Initialize an index manager if required
void BodyStream::InitIndexManager(void)
{
	// Don't init if no indexing required
	if(StreamIndex == StreamIndexNone) return;

	// Don't init if no IndexSID set
	if(IndexSID == 0) return;

	// Don't init if no writer
	if(!StreamWriter) return;

	// Don't init if already done
	if(IndexMan) return; 

	// Nothing to initialise if we have no streams
	if(empty()) return;


	/* Build a write-order list of streams to ensure the index is built in the write order */
	
	std::map<UInt32, EssenceSourcePtr> IndexOrderMap;
	BodyStream::iterator it = begin();
	while(it != end())
	{
		std::pair<UInt32, EssenceSourcePtr> IndexOrder;
		IndexOrder.first = static_cast<UInt32>(StreamWriter->GetWriteOrder((*it)->GetStreamID()));
		IndexOrder.second = (*it);
		IndexOrderMap.insert(IndexOrder);

		it++;
	}


	/* Add each stream */

	std::map<UInt32, EssenceSourcePtr>::iterator MapIt = IndexOrderMap.begin();
	while(MapIt != IndexOrderMap.end())
	{
		// Add to the index manager (create the index manager on first pass)
		// TODO: Sort the PosTable!!
		int StreamID = 0;
		if(!IndexMan)
		{
			IndexMan = new IndexManager(0, (*MapIt).second->GetBytesPerEditUnit(KAG));
			IndexMan->SetBodySID(BodySID);
			IndexMan->SetIndexSID(IndexSID);
			IndexMan->SetEditRate((*MapIt).second->GetEditRate());
			IndexMan->SetValueRelativeIndexing(ValueRelativeIndexing);
		}
		else
			StreamID = IndexMan->AddSubStream(0, (*MapIt).second->GetBytesPerEditUnit(KAG));

		// Let the stream know about this index manager
		(*MapIt).second->SetIndexManager(IndexMan, StreamID);

		// TODO: Currently no support for filler indexing here - needs adding
		StreamWriter->AddStreamIndex((*MapIt).second->GetStreamID(), IndexMan, StreamID, false, (StreamWrap == StreamWrapClip));

		MapIt++;
	}

	// Set the sub-range offset, if required
	IndexMan->SetSubRangeOffset(front()->GetRangeStart());

	// Set the -ve indexing for pre-charge, if selected
	if(Feature(FeatureNegPrechargeIndex))
	{
		// Locate the highest pre-charge size, so we can set the -ve index edit unit correctly
		Length HighestPrechargeSize = 0;
		MapIt = IndexOrderMap.begin();
		while(MapIt != IndexOrderMap.end())
		{
			// Check if this is the largest precharge
			Length ThisSize = (*MapIt).second->GetPrechargeSize();
			if(ThisSize > HighestPrechargeSize) HighestPrechargeSize = ThisSize;

			MapIt++;
		}

		// Set the index edit unit -ve for the pre-charge, if required
		if(HighestPrechargeSize) 
		{
			// This is also the next edit unit that will be written to a sprinkled index
			NextSprinkled = 0 - HighestPrechargeSize;
			StreamWriter->SetIndexEditUnit(NextSprinkled);
		}
	}
}


//! Initialize all required index managers
void BodyWriter::InitIndexManagers(void)
{
	StreamInfoList::iterator it = StreamList.begin();
	while(it != StreamList.end())
	{
		(*it)->Stream->InitIndexManager();
		it++;
	}
}


//! Get the WriteOrder for the specified stream
/*! \return -1 if not found */
Int32 GCWriter::GetWriteOrder(GCStreamID ID)
{
	std::map<UInt32, GCStreamID>::iterator it = WriteOrderMap.begin();
	while(it != WriteOrderMap.end())
	{
		if((*it).second == ID) return static_cast<Int32>((*it).first);
		it++;
	}

	// Not found
	return -1;
}

