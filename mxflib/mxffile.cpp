/*! \file	mxffile.cpp
 *	\brief	Implementation of MXFFile class
 *
 *			The MXFFile class holds data about an MXF file, either loaded 
 *          from a physical file or built in memory
 *
 *	\version $Id: mxffile.cpp,v 1.2 2004/11/12 09:20:44 matt-beard Exp $
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

// Required for strerror()
#include <string.h>
#include <errno.h>

#include <mxflib/mxflib.h>

using namespace mxflib;


//! Open the named MXF file
bool mxflib::MXFFile::Open(std::string FileName, bool ReadOnly /* = false */ )
{
	if(isOpen) Close();

	// Set to be a normal file
	isMemoryFile = false;

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


//! Create and open the named MXF file
bool mxflib::MXFFile::OpenNew(std::string FileName)
{
	if(isOpen) Close();

	// Set to be a normal file
	isMemoryFile = false;

	// Record the name
	Name = FileName;

	Handle = FileOpenNew(FileName.c_str());

	if(!FileValid(Handle)) return false;

	isOpen = true;

	// No run-in yet
	RunInSize = 0;

	return true;
}


bool mxflib::MXFFile::OpenMemory(DataChunkPtr Buff /*=NULL*/, Uint64 Offset /*=0*/)
{
	if(isOpen) Close();

	// Set to be a memory file
	isMemoryFile = true;
	Name = "Memory File";

	// No run-in currently allowed on memory files
	RunInSize = 0;

	if(Buff) Buffer = Buff;
	else Buffer = new DataChunk();

	// If no granularity set use 64k
	if(!Buffer->GetGranularity()) Buffer->SetGranularity(64 * 1024);

	BufferOffset = Offset;

	// Start at the start of the stream
	BufferCurrentPos = 0;

	isOpen = true;

	return true;
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

	// Index the start of the key (with a sanity check first!)
	if(BaseHeader->GetKey().Size < 16) return false;
	Uint8 *BaseKey = BaseHeader->GetKey().Data;

	// If no run-in end now
	if(memcmp(BaseKey,Key->Data,11) == 0)
	{
		Seek(0);
		return true;
	}

	// Perform search in memory
	// Maximum size plus enough to test the following key
	// TODO: What should we do here to ensure that this doesn't break stream sources?
	Seek(0);
	DataChunkPtr Search = Read(0x10000 + 11);

	Uint32 Scan = Search->Size - 11;
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

	error("Cannot find valid key in first 65536 bytes of file \"%s\"\n", Name.c_str());
	Seek(0);
	return false;
}


//! Close the file
bool mxflib::MXFFile::Close(void)
{
	if(isOpen) 
	{
		if(isMemoryFile)
		{
			Buffer = NULL;
		}
		else
		{
			FileClose(Handle);
		}
	}

	isOpen = false;

	return true;
}


//! Read data from the file into a DataChunk
DataChunkPtr mxflib::MXFFile::Read(Uint64 Size)
{
	DataChunkPtr Ret = new DataChunk(Size);

	if(Size)
	{
		Uint64 Bytes;

		if(isMemoryFile)
		{
			if(Size > 0xffffffff)
			{
				error("Memory file reading is limited to 4Gb\n");
				Size = 0xffffffff;
			}

			Bytes = MemoryRead(Ret->Data, (Uint32)Size);
		}
		else
		{
			Bytes = FileRead(Handle, Ret->Data, Size);
		}

		// Handle errors
		if(Bytes == (Uint64)-1)
		{
			error("Error reading file \"%s\" at 0x%s - %s\n", Name.c_str(), Int64toHexString(Tell(), 8).c_str(), strerror(errno));
			Bytes = 0;
		}

		if(Bytes != Size) Ret->Resize((Uint32)Bytes);
	}

	return Ret;
}


//! Read data from the file into a supplied buffer
Uint64 mxflib::MXFFile::Read(Uint8 *Buffer, Uint64 Size)
{
	Uint64 Ret = 0;

	if(Size)
	{
		if(isMemoryFile)
		{
			if(Size > 0xffffffff)
			{
				error("Memory file reading is limited to 4Gb\n");
				Size = 0xffffffff;
			}

			Ret = MemoryRead(Buffer,(Uint32) Size);
		}
		else
		{
			Ret = FileRead(Handle, Buffer, (Uint32)Size);
		}

		// Handle errors
		if(Ret == (Uint64)-1)
		{
			error("Error reading file \"%s\" at 0x%s - %s\n", Name.c_str(), Int64toHexString(Tell(), 8).c_str(), strerror(errno));
			Ret = 0;
		}
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

	SeekEnd();
	Uint64 FileEnd = Tell();

	// File smaller than 20 bytes! No chance of a RIP
	if(FileEnd < 20) return false;

	Seek(FileEnd - 4);
	Uint64 Location = Tell();

	Uint32 RIPSize = ReadU32();

	// If the RIP size would be bigger than the file it can't be a valid RIP
	if(RIPSize > Location) return false;

	// If we have a valid RIP then RIPSize bytes from back the end of the file will be the RIP key
	Seek(FileEnd - RIPSize);
	Uint64 RIPStart = Tell();
	DataChunkPtr RIPKey = Read(16);

	// Something went wrong with the read!
	if(RIPKey->Size != 16) return false;

	// Do a key lookup on this key
	MDOTypePtr KeyType = MDOType::Find(UL(RIPKey->Data));

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
			error("Unexpected end of pack in RIP at %s\n", RIPObject->GetSourceLocation().c_str());
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
 *  TODO: There should be some way to scan without loading every partition into memory (it could be millions!)
 *  TODO: We could read the IndexSID as we go and build an enhanced RIP or Greater-RIP
 */
bool mxflib::MXFFile::ScanRIP(Uint64 MaxScan /* = 1024*1024 */ )
{
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
		FooterPos = ScanRIP_FindFooter(MaxScan);
		if(FooterPos == 0) return false;
	}

	// Store the footer in the RIP and loop back through all other partitions
	Uint64 PartitionPos = FooterPos;
	
	bool AllOK = true;
	for(;;)
	{
		Seek(PartitionPos);
		PartitionPtr ThisPartition = ReadPartition();

		// If any partition read fails abort the scan
		// But attempt to store the header first
		if(!ThisPartition)
		{
			// Header read failed - things are bad!
			if(PartitionPos == 0) return false;
			
			// Try and read the header, then return failure
			AllOK = false;
			PartitionPos = 0;
			continue;
		}

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

	return AllOK;
}


//! Scan for the footer
/*! \return The location of the footer, or 0 if scan failed */
Uint64 MXFFile::ScanRIP_FindFooter(Uint64 MaxScan)
{
	Uint64 FooterPos = 0;

	// Size of scan chunk when looking for footer key
	static const unsigned int ScanChunkSize = 4096;

	// If too small a scan range is given we can't scan!
	if(MaxScan < 20) return 0;

	Uint64 ScanLeft = MaxScan;			// Number of bytes left to scan
	SeekEnd();
	Uint64 FileEnd = Tell();			// The file end
	Uint64 ScanPos = FileEnd;			// Last byte of the current scan chunk

	while(ScanLeft)
	{
		Uint64 ThisScan;				// Number of bytes to scan this time
		
		// Scan the number of bytes left, limited to the chunk size
		if(ScanLeft > ScanChunkSize) ThisScan = ScanChunkSize; else ThisScan = ScanLeft;
		
		// Don't scan off the start of the file
		if(ThisScan > ScanPos) ThisScan = ScanPos;

		// Quit if we ran out of bytes to scan
		if(ThisScan == 0) return 0;

		// Read this chunk
		Seek(ScanPos - ThisScan);
		DataChunkPtr Chunk = Read(ThisScan);

		// Quit if the read failed
		if(Chunk->Size != ThisScan) return 0;

		unsigned char *p = Chunk->Data;
		Uint64 i;
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
					Seek(ScanPos - (ThisScan - i));
					Uint64 Location = Tell();
					DataChunkPtr Key = Read(16);

					if(Key->Size == 16)
					{
						MDOTypePtr Type = MDOType::Find(UL(Key->Data));
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
		if(ScanPos > ThisScan) ScanPos -= ThisScan; else return 0;
	}

	return FooterPos;
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

	// Check that the first KLV is a partition pack
	// DRAGONS: What if the first KLV is a filler? - This shouldn't be valid as it would look like a run-in!
	if(!(ThisPartition->GetType()->Base)
		|| ( ThisPartition->GetType()->Base->Name() != "PartitionMetadata") )
	{
		error("First KLV in file \"%s\" is not a known partition type\n", Name.c_str());
		return false;
	}

	for(;;)
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
			Seek(NextPos);
			if( Tell() != NextPos)
			{
				error("Unexpected end of file in partition starting at 0x%s in file \"%s\" (Trying to skip from 0x%s to 0x%s)\n",
					  Int64toHexString(Location,8).c_str(), Name.c_str(), Int64toHexString(PreSkip,8).c_str(), Int64toHexString(NextPos,8).c_str());
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
				if(ThisType->Base)
				{
					if(ThisType->Base->Name() == "PartitionMetadata")
					{
						break;
					}
				}
			}

			Skip = ReadBER();
			Uint64 NextPos = Tell() + Skip;
			Seek(NextPos);
			if( (Skip < 0) || (Tell() != NextPos))
			{
				error("Unexpected end of file in KLV starting at 0x%s in file \"%s\" (Trying to skip from 0x%s to 0x%s)\n",
					  Int64toHexString(Location,8).c_str(), Name.c_str(), Int64toHexString(NextPos - Skip,8).c_str(), Int64toHexString(NextPos,8).c_str());
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
/*! \return -1 on error
 */
Length mxflib::MXFFile::ReadBER(void)
{
	DataChunkPtr Len = Read(1);
	if(Len->Size < 1)
	{
		error("Incomplete BER length in file \"%s\" at 0x%s\n", Name.c_str(), Int64toHexString(Tell(),8).c_str());
		return -1;
	}

	Length Ret = Len->Data[0];
	if(Ret >= 0x80)
	{
		Uint32 i = (Uint32)Ret & 0x7f;
		Len = Read(i);
		if(Len->Size != i)
		{
			error("Incomplete BER length in file \"%s\" at 0x%s\n", Name.c_str(), Int64toHexString(Tell(),8).c_str());
			return -1;
		}

		Ret = 0;
		Uint8 *p = Len->Data;
		while(i--) Ret = ((Ret<<8) + *(p++));
	}

	return Ret;
}


//! Read a Key length from the open file
ULPtr mxflib::MXFFile::ReadKey(void)
{
	ULPtr Ret;

//	Uint64 Location = Tell();
	DataChunkPtr Key = Read(16);

	// If we couldn't read 16-bytes then bug out (this may be valid)
	if(Key->Size != 16) return Ret;

/*
	// Sanity check the keys
	if((Key->Data[0] != 6) || (Key->Data[1] != 0x0e))
	{
		error("Invalid KLV key found at 0x%s in file \"%s\"\n", Int64toHexString(Location, 8).c_str(), Name.c_str());
		return Ret;
	}
*/
	// Build the UL
	Ret = new UL(Key->Data);

	return Ret;
}



//! Write a partition pack to the file
/*! The property "ThisPartition" is updated to reflect the correct location in the file
 *	\note This function only writes the partition pack, unlike WritePartition which 
 *	      writes the metadata (and index table segments as well - possibly)
 */
void MXFFile::WritePartitionPack(PartitionPtr ThisPartition, PrimerPtr UsePrimer /*=NULL*/) 
{ 
	DataChunk Buffer;
	Int64 CurrentPosition = Tell();
	ThisPartition->SetInt64("ThisPartition", CurrentPosition);

	// Adjust properties for a footer
	if(ThisPartition->Name().find("Footer") != std::string::npos)
	{
		ThisPartition->SetInt64("FooterPartition", CurrentPosition);
		ThisPartition->SetUint("BodySID", 0);
		ThisPartition->SetUint64("BodyOffset", 0);
	}

	// Find the current partition at this location, or the nearest after it
	RIP::iterator it = FileRIP.lower_bound(CurrentPosition);

	// Now get the previous partition in the RIP
	if(!FileRIP.empty()) it--;

	// Set the position of the previous partition
	// DRAGONS: Is there some way to know that we don't know the previous position?
	if(it == FileRIP.end()) ThisPartition->SetInt64("PreviousPartition", 0);
	else ThisPartition->SetInt64("PreviousPartition", (*it).first);

	// Add this partition to the RIP, but don't store the partition as we don't
	// own it and therefore cannot prevent changes after writing
	FileRIP.AddPartition(NULL, CurrentPosition, ThisPartition->GetUint("BodySID"));

	ThisPartition->WriteObject(Buffer, UsePrimer);

	Write(Buffer.Data, Buffer.Size);
}


//! Calculate the size of a filler to align to a specified KAG
/*! Note: Maximum supported filler size is 0x00ffffff bytes */
Uint32 MXFFile::FillerSize(bool ForceBER4, Uint64 FillPos, Uint32 KAGSize, Uint32 MinSize /*=0*/)
{
	if(KAGSize == 0) KAGSize = 1;

	// Work out how far into a KAG we are
	Uint32 Offset = (Uint32)(FillPos % KAGSize);

	// Don't insert anything if we are already aligned and not padding
	if((Offset == 0) && (MinSize == 0)) return 0;

	// Work out the required filler size
	Uint32 Fill = KAGSize - Offset;

	// Fix to minimum size;
	while(Fill < MinSize) Fill += KAGSize;

	// Adjust so that the filler can fit
	// Note that for very small KAGs the filler may be several KAGs long
	if(ForceBER4)
	{
		while(Fill < 20) Fill += KAGSize;
	}
	else
	{
		while(Fill < 17) Fill += KAGSize;
	}

	if(Fill > 0x00ffffff)
	{
		error("Maximum supported filler is 0x00ffffff bytes long, "
			  "but attempt to fill from 0x%s to KAG of 0x%08x with "
			  "MinSize=0x%08x requires a filler of size 0x%08x\n",
			  Int64toHexString(FillPos, 8).c_str(), KAGSize, MinSize, Fill);
		Fill = 0x00ffffff;
	}

	return Fill;
}


//! Write a filler to align to a specified KAG
/*! \return The position after aligning
 */
Uint64 MXFFile::Align(bool ForceBER4, Uint32 KAGSize, Uint32 MinSize /*=0*/)
{
	if(KAGSize == 0) KAGSize = 1;

	// Work out how big a filler we need
	Uint32 Fill = FillerSize(ForceBER4, Tell(), KAGSize, MinSize);

	// Nothing to do!
	if(Fill == 0) return Tell();

	// The filler type - don't perform the lookup each time!
	static MDOTypePtr FillType = MDOType::Find("KLVFill");
	ASSERT(FillType);

	// Write the filler key
	Write(FillType->GetGlobalKey());

	// Calculate filler length for shortform BER length
	Fill -= 17;
	if((!ForceBER4) && (Fill < 3))
	{
		WriteU8(Fill);
	}
	else
	{
		// Adjust for 4-byte BER length
		Fill -= 3;
		Write(*MakeBER(Fill, 4));
	}

	// Write the filler value
	// DRAGONS: Only moderately efficient
	if(Fill > 128)
	{
		Uint8 Buffer[128];
		memset(Buffer, 0, 128);
		while(Fill > 128)
		{
			Write(Buffer, 128); 
			Fill -= 128;
		}
	}
	while(Fill--) WriteU8(0);

	return Tell();
}


//! Write or re-write a partition pack and associated metadata (and index table segments?)
/*! \note Partition properties are updated from the linked metadata
 *	\return true if (re-)write was successful, else false
 */
bool MXFFile::WritePartitionInternal(bool ReWrite, PartitionPtr ThisPartition, bool IncludeMetadata, DataChunkPtr IndexData, PrimerPtr UsePrimer, Uint32 Padding, Uint32 MinPartitionSize)
{
	PrimerPtr ThisPrimer;
	if(UsePrimer) ThisPrimer = UsePrimer; else ThisPrimer = new Primer;

	DataChunk PrimerBuffer;
	DataChunk MetaBuffer;

	// Is this a footer?
	bool IsFooter = (ThisPartition->Name().find("Footer") != std::string::npos);

	// Write all objects
	MDObjectList::iterator it = ThisPartition->TopLevelMetadata.begin();
	while(it != ThisPartition->TopLevelMetadata.end())
	{
		if(IncludeMetadata) (*it)->WriteLinkedObjects(MetaBuffer, ThisPrimer);

		const Uint8 PrefaceUL_Data[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x2F, 0x00 };
		const UL PrefaceUL = UL(PrefaceUL_Data);

		// Update partition pack settings from the preface (if we find one)
		// if((*it)->Name() == "Preface" || (*it)->Name() == "Header" )
		if( *(*it)->GetType()->GetTypeUL() == PrefaceUL )
		{
			// Update OP label
			MDObjectPtr DstPtr = ThisPartition["OperationalPattern"];
			MDObjectPtr SrcPtr = (*it)["OperationalPattern"];
			if((SrcPtr) && (DstPtr))
			{
				DstPtr->ReadValue(SrcPtr->Value->PutData());
			}

			// Update essence containers
			DstPtr = ThisPartition["EssenceContainers"];
			if(DstPtr)
			{
				DstPtr->clear();
				SrcPtr = (*it)["EssenceContainers"];

				if(SrcPtr)
				{
					MDObjectNamedList::iterator it2 = SrcPtr->begin();
					while(it2 != SrcPtr->end())
					{
						DstPtr->AddChild("EssenceContainer", false)->ReadValue((*it2).second->Value->PutData());
						it2++;
					}
				}
			}
		}

		it++;
	}

	// Get the KAG size
	Uint32 KAGSize = ThisPartition->GetUint("KAGSize");

	// Align if required (not if re-writing)
	if((!ReWrite) && (KAGSize > 1)) Align(KAGSize);

	// Initialy we have no header data
	Uint64 HeaderByteCount = 0;
	
	if(IncludeMetadata)
	{
		// Build the primer
		ThisPrimer->WritePrimer(PrimerBuffer);

		// Set size of header metadata (including the primer)
		HeaderByteCount = PrimerBuffer.Size + MetaBuffer.Size;

		if(ReWrite)
		{
			Uint64 Pos = Tell();
			PartitionPtr OldPartition = ReadPartition();

			if(!OldPartition)
			{
				error("Failed to read old partition pack in MXFFile::ReWritePartition()\n"); 
				return false;
			}

			// Move back to re-write partition pack
			Seek(Pos);

			Uint64 OldHeaderByteCount = OldPartition->GetUint64("HeaderByteCount");

			// Record the required padding size to make the new partition match the old one
			Padding =(Uint32)( OldHeaderByteCount - HeaderByteCount);
			
			// We can't obey a MinPartitionSize request
			MinPartitionSize = 0;

			// Minimum possible filler size is 17 bytes
			if((HeaderByteCount > OldHeaderByteCount) || (Padding < 17))
			{
				error("Not enough space to re-write updated header at position 0x%s in %s\n", Int64toHexString(Tell(), 8).c_str(), Name.c_str());
				return false;
			}

			HeaderByteCount += Padding;
		}
		else
		{
			// Padding follows the header if no index data
			if(((Padding > 0) || (MinPartitionSize > HeaderByteCount)) && (!IndexData))
			{
				// Work out which of the two padding methods requires the greater padding
				Length UsePadding = (Length)MinPartitionSize - (Length)HeaderByteCount;
				if(UsePadding > (Length)Padding) Padding = (Uint32)UsePadding;

				HeaderByteCount += FillerSize(HeaderByteCount, KAGSize, Padding);
			}
			// Otherwise just pad to the KAG
			else if((!IsFooter) || (IndexData))
			{
				HeaderByteCount += FillerSize(HeaderByteCount, KAGSize);
			}
		}

		ThisPartition->SetUint64("HeaderByteCount", HeaderByteCount);
	}
	else
	{
		ThisPartition->SetUint64("HeaderByteCount", 0);
	}

	// Calculate the index byte size
	if(IndexData)
	{
		Uint64 IndexByteCount = IndexData->Size;

		if( (!IsFooter) || (Padding > 0) || (MinPartitionSize > HeaderByteCount) ) 
		{
			// Work out which of the two padding methods requires the greater padding
			Length UsePadding = (Length)MinPartitionSize - (Length)(HeaderByteCount + IndexByteCount);
			if(UsePadding > (Length)Padding) Padding = (Uint32)UsePadding;

			IndexByteCount += FillerSize(IndexByteCount, KAGSize, Padding);
		}

		ThisPartition->SetUint64("IndexByteCount", IndexByteCount);
	}
	else
	{
		ThisPartition->SetUint("IndexSID", 0);
		ThisPartition->SetUint64("IndexByteCount", 0);
	}

	// Write the pack
	WritePartitionPack(ThisPartition);

	if(IncludeMetadata)
	{
		// Align if required
		if(KAGSize > 1) Align(KAGSize);

		// Write the primer
		Write(PrimerBuffer);

		// Write the other header metadata
		Write(MetaBuffer);
	}

	if(IndexData)
	{
		// Align if required
		if(KAGSize > 1) Align(KAGSize);

		// Write the index data
		Write(IndexData);
	}

	// If not a footer align to the KAG (add padding if requested even if it is a footer)
	if( (!IsFooter) || (Padding > 0))
	{
		if((KAGSize > 1) || (Padding > 0)) Align(KAGSize, Padding);
	}

	return true;
}


Uint32 MXFFile::MemoryWrite(Uint8 const *Data, Uint32 Size)
{
	if(BufferCurrentPos < BufferOffset)
	{
		error("Cannot currently write to a memory file before the buffer start\n");
		return 0;
	}

	// Copy the data to the buffer
	Buffer->Set((Uint32)Size, Data, (Uint32)(BufferCurrentPos - BufferOffset));

	// Update the pointer
	BufferCurrentPos += Size;

	// Return the number of bytes written
	return Size;
}


Uint32 MXFFile::MemoryRead(Uint8 *Data, Uint32 Size)
{
	if(BufferCurrentPos < BufferOffset)
	{
		error("Cannot currently read from a memory file before the buffer start\n");
		return 0;
	}

	if((BufferCurrentPos - BufferOffset) >= Buffer->Size)
	{
		error("Cannot currently read beyond the end of a memory file buffer\n");
		return 0;
	}

	// Work out how many bytes we can read
	unsigned int MaxBytes =(Uint32)( Buffer->Size - (Uint32)(BufferCurrentPos - BufferOffset));

	// Limit our read to the max available
	if(Size > MaxBytes) Size = MaxBytes;

debug("Copy %d bytes from 0x%08x to 0x%08x\n", Size, &Buffer->Data[BufferCurrentPos - BufferOffset], Data);
	// Copy the data from the buffer
if(Buffer->Data != 0)
	memcpy(Data, &Buffer->Data[BufferCurrentPos - BufferOffset], Size);
else Size = 0;

	// Update the pointer
	BufferCurrentPos += Size;

	// Return the number of bytes read
	return Size;
}



//! Read a KLVObject from the file
KLVObjectPtr MXFFile::ReadKLV(void)
{
	KLVObjectPtr Ret = new KLVObject();

	// Set the the value details
	Ret->SetSource(this);

	// Read the key and length - returning NULL if no more valid KLVs
	if(Ret->ReadKL() < 17) return NULL;

	return Ret;
}
