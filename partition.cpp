/*! \file	partition.cpp
 *	\brief	Implementation of Partition class
 *
 *			The Partition class holds data about a partition, either loaded 
 *          from a partition in the file or built in memory
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

#if 0
//! Read the partition from a buffer
/*!	/ret Number of bytes read
 */
Uint32 Partition::ReadValue(const Uint8 *Buffer, Uint32 Size)
{
	// Start off empty
//##	clear();

	// Each entry in the primer is 18 bytes
	Uint32 Items = Size / 18;

	// Validate the size and only read whole items
	if((Items * 18) != Size)
	{
		error("Primer not an integer number of multiples of 18 bytes!\n");
		Size = Items * 18;
	}

	// Read each item
	while(Items--)
	{
		Tag ThisTag = GetU16(Buffer);
		Buffer += 2;

		UL ThisUL(Buffer);
		Buffer += 16;

		// Add this new entry to the primer
//##		insert(Primer::value_type(ThisTag, ThisUL));
	}

	// Return how many bytes we actually read
	return Size;
}

#endif 0


//! Add a metadata object to the header metadata belonging to a partition
void mxflib::Partition::AddMetadata(MDObjectPtr Object)
{
	// Start out without a target
	bool has_target = false;

	// Start out not (strong) reffed
	bool linked = false;

	// Add us to the list of all items
	AllMetadata.push_back(Object);

	// Add this object to the ref target list if it is one
	// Note: although nothing currently does it it is theoretically possible to
	//       have more than one target entry in a set
	MDObjectList::iterator it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		if((*it)->GetRefType() == DICT_REF_TARGET)
		{
			if((*it)->Value->size() != 16)
			{
				error("Metadata Object \"%s/%s\" should be a reference target (a UUID), but has size %d\n",
					  Object->Name().c_str(), (*it)->Name().c_str(), (*it)->Value->GetData().Size);
			}
			else
			{
				has_target = true;

				UUIDPtr ID = new UUID((*it)->Value->PutData().Data);
				RefTargets.insert(std::map<UUID, MDObjectPtr>::value_type(*ID, Object));
//printf("TARGET: %08x %s\n", Object.GetPtr(), Object->Name().c_str());
			
				// Try and satisfy all refs to this set
				for(;;)
				{
					std::multimap<UUID, MDObjectPtr>::iterator mit = UnmatchedRefs.find(*ID);
					
					// Exit when no more refs to this object
					if(mit == UnmatchedRefs.end()) break;

					// Sanity check!
					if((*mit).second->GetLink())
					{
						error("Internal error - Object in UnmatchedRefs but already linked!");
					}

					// Make the link
					(*mit).second->SetLink(Object);

					// If we are the tagert of a strong ref we won't get added to the top level
					if((*mit).second->GetRefType() == DICT_REF_STRONG) linked = true;

					// Remove from the unmatched refs map
					UnmatchedRefs.erase(mit);

					// loop for any more refs to this set
				}
			}
		}
		it++;
	}

	// If we are not yet (strong) reffed then we are top level
	if(!linked) TopLevelMetadata.push_back(Object);

	// Satisfy, or record as un-matched, all outgoing references
	ProcessChildRefs(Object);
}

//! Satisfy, or record as un-matched, all outgoing references
void mxflib::Partition::ProcessChildRefs(MDObjectPtr Object)
{
	MDObjectList::iterator it = Object->Children.begin();
	while(it != Object->Children.end())
	{
		DictRefType Ref = (*it)->GetRefType();
		if((Ref == DICT_REF_STRONG) || (Ref == DICT_REF_WEAK))
		{
			if((*it)->Value->size() != 16)
			{
				error("Metadata Object \"%s/%s\" should be a reference source (a UUID), but has size %d\n",
					  Object->Name().c_str(), (*it)->Name().c_str(), (*it)->Value->size());
			}
			else
			{
				UUIDPtr ID = new UUID((*it)->Value->PutData().Data);
//printf("SOURCE(%d): %08x %s\n", (*it)->GetRefType(), (*it).GetPtr(), (*it)->Name().c_str());
				std::map<UUID, MDObjectPtr>::iterator mit = RefTargets.find(*ID);

				if(mit == RefTargets.end())
				{
					// Not matched yet, so add to the list of outstanding refs
					UnmatchedRefs.insert(std::multimap<UUID, MDObjectPtr>::value_type(*ID, (*it)));
				}
				else
				{
					// Make the link
					(*it)->SetLink((*mit).second);

					// If we have made a strong ref, remove the target from the top level
					if(Ref == DICT_REF_STRONG) TopLevelMetadata.remove((*mit).second);
				}
			}
		}

		// Recurse to process sub-children if they exist
		if(!(*it)->Children.empty()) 	ProcessChildRefs((*it));

		it++;
	}
}

//! Read a full set of header metadata from a buffer (including primer)
Uint64 mxflib::Partition::ReadMetadata(const Uint8 *Buffer, Uint64 Size)
{
	const Uint8 *BuffPtr = Buffer;
	Uint64 Bytes = 0;

	ClearMetadata();
//printf("Size = 0x%08x\n", (int)Size);

	while(Size)
	{
		if(Size < 16)
		{
			error("Less than 16-bytes available in Partition::ReadMetadata() after reading 0x%s bytes\n", Int64toHexString(Bytes, 8).c_str());
			break;
		}

		// Sanity check the keys
		if((BuffPtr[0] != 6) || (BuffPtr[1] != 0x0e))
		{
			error("Invalid KLV key found after reading 0x%s in Partition::ReadMetadata()\n", Int64toHexString(Bytes, 8).c_str());
			break;
		}

		// Build an object (it may come back as an "unknown")
		MDObjectPtr NewItem = new MDObject(new UL(BuffPtr));
		ASSERT(NewItem);

		BuffPtr += 16;
		Size -= 16;
		Bytes += 16;

		if(Size < 1)
		{
			error("Incomplete BER length after reading 0x%s in Partition::ReadMetadata()\n", Int64toHexString(Bytes, 8).c_str());
			break;
		}

		Uint64 Length = *BuffPtr++;
		Size--;
		Bytes++;
		if(Length >= 0x80)
		{
			Uint32 i = Length & 0x7f;
			if(Size < i)
			{
				error("Incomplete BER length after reading 0x%s in Partition::ReadMetadata()\n", Int64toHexString(Bytes, 8).c_str());
				break;
			}

			Length = 0;
			while(i--) 
			{
				Length = ((Length<<8) + *(BuffPtr++));
				Size--;
				Bytes++;
			}
		}
//printf("So Far %d\n", Bytes);

		// DRAGONS: KLV Size limit!!
		if(Length > 0xffffffff)
		{
			error("Current implementation KLV size limit of 0xffffffff bytes exceeded after reading 0x%s in Partition::ReadMetadata()\n", Int64toHexString(Bytes, 8).c_str());
			break;
		}

		if(Size < Length)
		{
			error("KLV length is %s but available data size is only %s after reading 0x%s in Partition::ReadMetadata()\n", 
				  Uint64toString(Length).c_str(), Uint64toString(Size).c_str(), Int64toHexString(Bytes, 8).c_str());

			// Try reading what we have
			Length = Size;
		}

		// Check for the primer until we have found it
		if(!PartitionPrimer)
		{
			if(NewItem->Name() == "Primer")
			{
				PartitionPrimer = new Primer;
				Uint32 ThisBytes = PartitionPrimer->ReadValue(BuffPtr, Length);
				Size -= ThisBytes;
				Bytes += ThisBytes;
				BuffPtr += ThisBytes;

				// Skip further processing for the primer
				continue;
			}
		}

		if(Length)
		{
			Uint32 ThisBytes = NewItem->ReadValue(BuffPtr, Length, PartitionPrimer);
/*if(NewItem->Value)
{
 printf("Read %s, size = %d, object size = %d (%d)\n", NewItem->Name().c_str(), ThisBytes, NewItem->Value->GetData().Size, NewItem->Value->size());
 int i;
 DataChunk Dat = NewItem->Value->PutData();
 for(i=0; i<Dat.Size; i++)
 {
   printf("%02x ", Dat.Data[i]);
 }
 printf("\n>%s\n", NewItem->Value->GetString().c_str());
}
else
 printf("Read %s, size = %d, Subs = %d)\n", NewItem->Name().c_str(), ThisBytes, NewItem->Children.size());
*/
			Size -= ThisBytes;
			Bytes += ThisBytes;
			BuffPtr += ThisBytes;
		}

		AddMetadata(NewItem);

//Uint32 Loc;
//printf("0x%08x %s\n",Loc, NewItem->Name().c_str());
//Loc = (int)BuffPtr - (int)Buffer;
	}

	return Bytes;
}


