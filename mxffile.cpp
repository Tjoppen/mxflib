/*! \file	mxffile.cpp
 *	\brief	Implementation of MXFFile class
 *
 *			The MXFFile class holds data about an MXF file, either loaded 
 *          from a physical file or built in memory
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

// Required for strerror()
#include <string.h>


#include "mxflib.h"

using namespace mxflib;


//! Open the named MXF file
bool mxflib::MXFFile::Open(std::string FileName, bool ReadOnly /* = false */ )
{
	if(isOpen) Close();

	// Record the name
	Name = FileName;

	if(ReadOnly)
	{
		Handle = FileOpenRead(FileName.c_str());
	}
	else
	{
		Handle = FileOpen(FileName.c_str());
	}

	if(!FileValid(Handle)) return false;

	isOpen = true;

	return ReadRunIn();
}


//! Read the files run-in (if it exists)
/*! The run-in is placed in property run-in
 *	After this function the file pointer is at the start of the non-run in data
 */
bool mxflib::MXFFile::ReadRunIn()
{
	RunInSize = 0;
	RunIn.Resize(0);

	Seek(0);
	DataChunkPtr Key = Read(16);

	// If we couldn't read 16-bytes then this isn't a valid MXF file
	if(Key->Size != 16) return false;

	// Locate a closed header type for key compares
	MDOTypePtr BaseHeader = MDOType::Find("ClosedHeader");

	if(!BaseHeader)
	{
		error("Cannot find \"ClosedHeader\" in current dictionary\n");
		return false;
	}

	// Index the start of the key
	Uint8 *BaseKey = BaseHeader->GetDict()->Key;

	// If no run-in end now
	if(memcmp(BaseKey,Key->Data,11) == 0)
	{
		Seek(0);
		return true;
	}

	// Perform search in memory
	// Maximum size plus enough to test the following key
	Seek(0);
	DataChunkPtr Search = Read(0x10000 + 11);

	Uint64 Scan = Search->Size - 11;
	Uint8 *ScanPtr = Search->Data;
	while(Scan)
	{
		// Run-in ends when a vaid MXF key is found
		if(memcmp(BaseKey,ScanPtr,11) == 0) 
		{
			RunIn.Set(RunInSize, Search->Data);
			Seek(0);
			return true;
		}

		Scan--;
		ScanPtr++;
		RunInSize++;
	}

	error("Cannot find valid key in first 65536 bytes of file \"%s\"\n", Name);
	Seek(0);
	return false;
}


//! Close the file
bool mxflib::MXFFile::Close(void)
{
	if(isOpen) FileClose(Handle);

	isOpen = false;

	return true;
}


//! Read data from the file into a DataChunk
DataChunkPtr mxflib::MXFFile::Read(Uint64 Size)
{
	DataChunkPtr Ret = new DataChunk(Size);

	if(Size)
	{
		Uint64 Bytes = FileRead(Handle, Ret->Data, Size);

		// Handle errors
		if(Bytes == (Uint64)-1)
		{
			error("Error reading file \"%s\" at 0x%s- %s\n", Name.c_str(), Int64toHexString(Tell(), 8).c_str(), strerror(errno));
			Bytes = 0;
		}

		if(Bytes != Size) Ret->Resize(Bytes);
	}

	return Ret;
}

//! Get a RIP for the open MXF
/*! The RIP is read using ReadRIP() if possible.
 *  Otherwise it is Scanned using ScanRIP().
 *	If that fails it is built the hard way using BuildRIP().
 */
bool mxflib::MXFFile::GetRIP(Uint64 MaxScan /* = 1024*1024 */ )
{
	if(ReadRIP()) return true;
	if(ScanRIP(MaxScan)) return true;
	return BuildRIP();
}


//! Read the RIP from the end of the open MXF file
/*! The new RIP is placed in property FileRIP
 *  \note Partition packs will <b>not</b> be loaded. Partition pointers in the new RIP will be NULL
 *  \note This new RIP will represent what is in the physical
 *        file so any data in memory will not be considered
 *  \note The current contents of FileRIP will be destroyed
 */
bool mxflib::MXFFile::ReadRIP(void)
{
	// Remove any old data
	FileRIP.clear();

	FileRIP.isGenerated = false;

	Uint64 FileEnd = SeekEnd();

	// File smaller than 20 bytes! No chance of a RIP
	if(FileEnd < 20) return false;

	Uint64 Location = Seek(FileEnd - 4);

	Uint32 RIPSize = ReadU32();

	// If the RIP size would be bigger than the file it can't be a valid RIP
	if(RIPSize > Location) return false;

	// If we have a valid RIP then RIPSize bytes back will be the RIP key
	Uint64 RIPStart = Seek(Location - RIPSize);
	DataChunkPtr RIPKey = Read(16);

	// Something went wrong with the read!
	if(RIPKey->Size != 16) return false;

	// Do a key lookup on this key
	MDOTypePtr KeyType = MDOType::Find(new UL(RIPKey->Data));

	// If not a known key type then not a valid RIP
	if(!KeyType) return false;

	// If it is a known type, but not a RIP then exit
	if(KeyType->Name() != "RandomIndexMetadata") return false;

	// Go back and read the RIP
	Seek(RIPStart);
	MDObjectPtr RIPObject = ReadObject();

	MDObjectPtr PartitionArray = RIPObject["PartitionArray"];
	
	MDObjectNamedList::iterator it = PartitionArray->begin();
	while(it != PartitionArray->end())
	{
		Uint32 BodySID = (*it).second->GetUint();

		it++;
		if(it == PartitionArray->end())
		{
			error("Unexpected end of pack in RIP at %s", RIPObject->GetSourceLocation().c_str());
			break;
		}

		Uint64 ByteOffset = (*it).second->GetUint64();
		
		debug("BodySID = 0x%04x, ByteOffset = %s\n", BodySID, Int64toString(ByteOffset).c_str());

		FileRIP.AddPartition(NULL, ByteOffset, BodySID);

		it++;
	}

	return true;
}


//! Build a RIP for the open MXF file by scanning partitions following links back from the footer
/*! The new RIP is placed in property FileRIP
 *  \note Each partition pack will be loaded and referenced from the new RIP
 *  \note This new RIP will represent what is in the physical
 *        file so any data in memory will not be considered
 *  \note The current contents of FileRIP will be destroyed
 *
 *  <h3>How the file is scanned</h3>
 *  An ideal file will not need scanning because it will contain a RIP (handled by ReadRIP).
 *  <br><br>
 *	The next best situation is for the header to hold the location of the footer in the 
 *	<i>FooterPartition</i> property. If this is the case the scan will happen as follows:
 *	-# The header is read to locate the footer
 *	-# The footer is read, and added to the RIP
 *	-# The <i>PreviousPartition</i> property is used to locate the previous partition
 *	-# The previous partition is read, and added to the RIP
 *  -# If the last partition processed was not the header then goto step 3
 *
 *	If the header doesn't hold the location of the footer then a search is performed to
 *  find the footer. This is somewhat optimised and is performed as follows:
 *	- 4Kb blocks of the file are read, starting with the last 4k, to a maximum specified (default 1Mb)
 *  - Each 4Kb block is scanned forwards (CPU optimisations work better scanning forwards) looking for bytes with the value 0x06
 *	- Each time 0x06 is found the next byte is checked for being 0x0e
 *	- When 0x06 0x0e is found 16 bytes are read starting at the 0x06 and a lookup is performed with MDOType::Find()
 *  - If the lookup shows this as a footer then the search is over and steps 2-5 of the above method are used
 *  - Otherwise the scan continues - if no footer is found within the maximum scan size then the scan is aborted
 *
 */
bool mxflib::MXFFile::ScanRIP(Uint64 MaxScan /* = 1024*1024 */ )
{
	// Size of scan chunk when looking for footer key
	static const ScanChunkSize = 4096;

	// Remove any old data
	FileRIP.clear();

	FileRIP.isGenerated = true;

	// Read the header
	Seek(0);
	PartitionPtr Header = ReadPartition();

	// Header not found (might not be an error - the file could be empty)
	if(!Header) return false;

	Uint64 FooterPos = Header->GetInt64("FooterPartition");
	if(FooterPos == 0)
	{
		// If too small a scan range is given we can't scan!
		if(MaxScan < 20) return false;

		Uint64 ScanLeft = MaxScan;			// Number of bytes left to scan
		Uint64 FileEnd = SeekEnd();			// The file end
		Uint64 ScanPos = FileEnd;			// Last byte of the current scan chunk

		while(ScanLeft)
		{
			Uint64 ThisScan;				// Number of bytes to scan this time
			
			// Scan the number of bytes left, limited to the chunk size
			if(ScanLeft > ScanChunkSize) ThisScan = ScanChunkSize; else ThisScan = ScanLeft;
			
			// Don't scan off the start of the file
			if(ThisScan > (ScanPos+1)) ThisScan = (ScanPos+1);

			// Quit if we ran out of bytes to scan
			if(ThisScan == 0) return false;

			// Read this chunk
//printf("Scanning %d bytes at %d, file size = %d\n", int(ThisScan), int(ScanPos - ThisScan), int(FileEnd));
			Seek(ScanPos - ThisScan);
			DataChunkPtr Chunk = Read(ThisScan);
//printf("Read %d bytes\n", int(Chunk->Size));

			// Quit if the read failed
			if(Chunk->Size != ThisScan) return false;

			unsigned char *p = Chunk->Data;
			int i;
			for(i=0; i<ThisScan; i++)
			{
				if(*p == 0x06)
				{
					// Find the byte following the 0x06
					unsigned char next;
					if(i < (ThisScan-1)) 
						// Next byte in the buffer
						next = p[1]; 
					else 
					{
						// Next byte is not in the buffer - read it from the file
						Seek(ScanPos);
						next = ReadI8();
					}

					// Matched 0x06 0x0e - could be a key...
					if(next == 0x0e)
					{
						// Locate the 0x06 in the file
						Uint64 Location = Seek(ScanPos - (ThisScan - i));
						DataChunkPtr Key = Read(16);

						if(Key->Size == 16)
						{
							MDOTypePtr Type = MDOType::Find(new UL(Key->Data));
							if(Type)
							{
								if(Type->Name().find("Footer") != std::string::npos)
								{
									debug("Found %s at 0x%s\n", Type->Name().c_str(), Int64toHexString(Location, 8).c_str());
									
									// Flag that the footer has been found, and record its location
									FooterPos = Location;
									break;
								}
							}
						}
					}
				}
				p++;
			}

			// If we found the footer in this chunk then scan no more
			if(FooterPos != 0) break;

			// Move back through the file
			if(ScanPos > ThisScan) ScanPos -= ThisScan; else return false;
		}

		// If the scan failed exit now
		if(FooterPos == 0) return false;
	}


	// Store the footer in the RIP and loop back through all other partitions
	Uint64 PartitionPos = FooterPos;
	for(;;)
	{
		Seek(PartitionPos);
		PartitionPtr ThisPartition = ReadPartition();

		// If any partition read fails abort the scan
		if(!ThisPartition) return false;

		Uint32 BodySID = ThisPartition->GetUint("BodySID");

		debug("Adding %s for BodySID 0x%04x at 0x%s\n", ThisPartition->Name().c_str(), BodySID, Int64toHexString(PartitionPos, 8).c_str());

		// Add the new partition
		FileRIP.AddPartition(ThisPartition, PartitionPos, BodySID);

		// Stop once we have added the header
		if(PartitionPos == 0) break;

		Uint64 NewPos = ThisPartition->GetUint64("PreviousPartition");
		if(NewPos >= PartitionPos)
		{
			error("%s/PreviousPartition in partition pack at %s is 0x%s, but this cannot be the location of the previous partition\n", 
				  ThisPartition->FullName().c_str(), ThisPartition->GetSourceLocation().c_str(), Int64toHexString(NewPos,8).c_str());
			return false;
		}

		PartitionPos = NewPos;
	}

	return true;
}


//! Build a RIP for the open MXF file by scanning the entire file for partitions
/*! The new RIP is placed in property FileRIP
 *  \note Each partition pack will be loaded and referenced from the new RIP
 *  \note This new RIP will represent what is in the physical
 *        file so any data in memory will not be considered
 *  \note The current contents of FileRIP will be destroyed
 */
bool mxflib::MXFFile::BuildRIP(void)
{
	// Remove any old data
	FileRIP.clear();

	FileRIP.isGenerated = true;

	Seek(0);

	Uint64 Location = 0;

	// Locate a closed header type for key compares
	MDOTypePtr BaseHeader = MDOType::Find("ClosedHeader");
	if(!BaseHeader)
	{
		error("Cannot find \"ClosedHeader\" in current dictionary\n");
		return false;
	}

	// Read the first partition pack in the file
	PartitionPtr ThisPartition = ReadPartition();

	// If we couldn't read the first object then there are no partitions
	// Note that this is not stricty an error - the file could be empty!
	if(!ThisPartition) return true;

	// Check that the first KLV as a partition pack
	// DRAGONS: What if the first KLV is a filler? - This shouldn't be valid as it would look like a run-in!
	if(!(ThisPartition->GetType()->GetDict()->Base)
		|| ( strcmp(ThisPartition->GetType()->GetDict()->Base->Name, "PartitionMetadata") != 0) )
	{
		error("First KLV in file \"%s\" is not a known partition type\n", Name.c_str());
		return false;
	}

	while(!Eof())
	{
		Uint32 BodySID = 0;
		MDObjectPtr Ptr = ThisPartition["BodySID"];
		if(Ptr) BodySID = Ptr->Value->GetInt();

		FileRIP.AddPartition(ThisPartition, Location, BodySID);

		Uint64 Skip = 0;

		// Work out how far to skip ahead
		Ptr = ThisPartition["HeaderByteCount"];
		if(Ptr) Skip = Ptr->Value->GetInt64();
		Ptr = ThisPartition["IndexByteCount"];
		if(Ptr) Skip += Ptr->Value->GetInt64();

		if(Skip)
		{
			// Location before we skip
			Uint64 PreSkip = Tell();

			/* Check for version 10 HeaderByteCount and possible bug version */
			// Version 10 of MXF counts from the end of the partition pack, however
			// some version 10 code uses the version 11 counting so we check to 
			// see if the header is claimed to be an integer number of KAGs and
			// that there is a leading filler to take us to the start of the next KAG
			// In this case we are probably in a Version 10 HeaderByteCount bug situation...

			Uint16 Ver = 0;
			Ptr = ThisPartition["MinorVersion"];
			Ver = Ptr->Value->GetUint();

			bool SkipFiller = true;		// Version 11 behaviour

			if(Ver == 1)				// Ver = 1 for version 10 files
			{
				Uint32 KAGSize = 0;
				Ptr = ThisPartition["KAGSize"];
				if(Ptr) KAGSize = Ptr->GetInt();

				Uint64 HeaderByteCount = 0;
				Ptr = ThisPartition["HeaderByteCount"];
				if(Ptr) HeaderByteCount = Ptr->GetInt();

				if((KAGSize > 16) && (HeaderByteCount > 0))
				{
					Uint64 Pos = Tell();
					Uint64 U = HeaderByteCount / KAGSize;
					if((U * KAGSize) == HeaderByteCount)
					{
						MDObjectPtr First = ReadObject();
						if(!First)
						{
							// Can't tell what is next! This will probably cause an error later!
							SkipFiller = true;
							Seek(Pos);
						}
						else if(First->Name() == "KLVFill")
						{
							U = Tell() / KAGSize;
							if( (U * KAGSize) == Tell() )
							{
								warning("(Version 10 file) HeaderByteCount in %s at 0x%s in %s does not include the leading filler\n",
										ThisPartition->FullName().c_str(), Int64toHexString(ThisPartition->GetLocation(),8).c_str(), 
										ThisPartition->GetSource().c_str());

								// We are skipping filler - even though we have already done it!
								SkipFiller = true;
								PreSkip = Tell();
							}
							else
							{
								SkipFiller = false;
								Seek(Pos);
							}
						}
					}
				}
			}

			if(SkipFiller)
			{
				MDObjectPtr First = ReadObject();
				if(!First)
				{
					error("Error reading first KLV after %s at 0x%s in %s\n", ThisPartition->FullName().c_str(), 
						  Int64toHexString(ThisPartition->GetLocation(),8).c_str(), ThisPartition->GetSource().c_str());
					return false;
				}

				if(First->Name() == "KLVFill")
					PreSkip = Tell();
				else if(First->Name() != "Primer")
				{
					error("First KLV following a partition pack (and any trailing filler) must be a Primer, however %s found at 0x%s in %s\n", 
						  ThisPartition->FullName().c_str(), Int64toHexString(ThisPartition->GetLocation(),8).c_str(), 
						  ThisPartition->GetSource().c_str());
				}
			}

			// Skip over header
			Uint64 NextPos = PreSkip + Skip;
			if( Seek(NextPos) != NextPos)
			{
				error("Unexpected end of file in partition starting at 0x%s in file \"%s\"\n",
					  Int64toHexString(Location,8).c_str(), Name.c_str());
				return false;
			}

			// Check that we ended up in a sane place after the skip
			DataChunkPtr Test = Read(2);
			if( Test->Size != 2)
			{
				// Less than 2 bytes after the declared end of this metadata
				// Could be that the count is valid and points us to the end of the file
				// but to be safe check through the header to see if there is anything else
				Seek(PreSkip);
			} 
			else if((Test->Data[0] != 6) || (Test->Data[1] != 0x0e))
			{
				error("Byte counts in partition pack at 0x%s in file \"%s\" are not valid\n", Int64toHexString(Location,8).c_str(), Name.c_str());
				
				// Move back to end of partition pack and scan through the header
				Seek(PreSkip);
			}
			else
			{
				// Move back (the test moved the pointer 2 bytes forwards)
				Seek(NextPos);
			}
		}

		// Now scan until the next partition
		ULPtr Key;
		for(;;)
		{
			Location = Tell();
			Key = ReadKey();
			if(!Key) break;

			// Identify what we have found
			MDOTypePtr ThisType = MDOType::Find(Key);

			// Only check for this being a partition pack if we know the type
			if(ThisType)
			{
				const DictEntry *Dict = ThisType->GetDict();
//printf("Found %s at 0x%08x\n", Dict->Name, (Uint32)Location);
				if(Dict && Dict->Base)
				{
					if(strcmp(Dict->Base->Name, "PartitionMetadata") == 0)
					{
						break;
					}
				}
			}
//else printf("Found %s at 0x%08x\n", UL(Key->Data).GetString().c_str(), (Uint32)Location);

			Skip = ReadBER();
			Uint64 NextPos = Tell() + Skip;
			if( Seek(NextPos) != NextPos)
			{
				error("Unexpected end of file in KLV starting at 0x%s in file \"%s\"\n",
					  Int64toHexString(Location,8).c_str(), Name.c_str());
				return false;
			}
			
			if(Eof()) break;
		}

		// Check if we found anything
		if(!Key) break;
		if(Eof()) break;

		// By this point we have found a partition pack
		// Read it ...
		Seek(Location);
		ThisPartition = ReadPartition();
		
		// ... then loop back to add it
		continue;
	}

	return true;
}

//! Read a BER length from the open file
Uint64 mxflib::MXFFile::ReadBER(void)
{
	DataChunkPtr Length = Read(1);
	if(Length->Size < 1)
	{
		error("Incomplete BER length in file \"%s\" at 0x%s\n", Name.c_str(), Int64toHexString(Tell(),8).c_str());
		return false;
	}

	Uint64 Ret = Length->Data[0];
	if(Ret >= 0x80)
	{
		Uint32 i = Ret & 0x7f;
		Length = Read(i);
		if(Length->Size != i)
		{
			error("Incomplete BER length in file \"%s\" at 0x%s\n", Name.c_str(), Int64toHexString(Tell(),8).c_str());
			return false;
		}

		Ret = 0;
		Uint8 *p = Length->Data;
		while(i--) Ret = ((Ret<<8) + *(p++));
	}

	return Ret;
}


//! Read a Key length from the open file
ULPtr mxflib::MXFFile::ReadKey(void)
{
	ULPtr Ret;

	Uint64 Location = Tell();
	DataChunkPtr Key = Read(16);

	// If we couldn't read 16-bytes then bug out (this may be valid)
	if(Key->Size != 16) return Ret;

	// Sanity check the keys
	if((Key->Data[0] != 6) || (Key->Data[1] != 0x0e))
	{
		error("Invalid KLV key found at 0x%s in file \"%s\"\n", Int64toHexString(Location, 8).c_str(), Name.c_str());
		return Ret;
	}

	// Build the UL
	Ret = new UL(Key->Data);

	return Ret;
}
