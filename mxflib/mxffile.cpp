/*! \file	mxffile.cpp
 *	\brief	Implementation of MXFFile class
 *
 *			The MXFFile class holds data about an MXF file, either loaded 
 *          from a physical file or built in memory
 *
 *	\version $Id: mxffile.cpp,v 1.24 2011/01/10 10:42:09 matt-beard Exp $
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
#include <sstream>
#include <iomanip>

#include "mxflib/mxflib.h"

using namespace mxflib;

//! Open the named MXF file
bool mxflib::MXFFile::Open(std::string FileName, bool ReadOnly /* = false */ )
{
	if(isOpen) Close();

	// Set to be a normal file
	isMemoryFile = false;
	isHandleFile = false;

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
	isHandleFile = false;

	// Record the name
	Name = FileName;

	Handle = FileOpenNew(FileName.c_str());

	if(!FileValid(Handle)) return false;

	isOpen = true;

	// No run-in yet
	RunInSize = 0;

	return true;
}


bool mxflib::MXFFile::OpenMemory(DataChunkPtr Buff /*=NULL*/, Position Offset /*=0*/)
{
	if(isOpen) Close();

	// Set to be a memory file
	isMemoryFile = true;
	isHandleFile = false;
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


//! Open an MXFFile for an existing, open, file handle
/*! DRAGONS: Once the file handle given here is closed by the caller, all further I/O will fail! */
bool mxflib::MXFFile::OpenFromHandle(FileHandle Handle)
{
	if(isOpen) Close();

	// Set to be a normal file, but with external handle management
	isMemoryFile = false;
	isHandleFile = true;

	// Record the name
	Name = "Existing Open File";

	// Set up our file handle
	this->Handle = Handle;

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

	// Use the closed header for key compares
	const UInt8 *BaseKey = ClosedHeader_UL.GetValue();

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

	UInt32 Scan = static_cast<UInt32>(Search->Size - 11);
	UInt8 *ScanPtr = Search->Data;
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
			if(!isHandleFile) FileClose(Handle);
		}
	}

	isOpen = false;

	return true;
}


//! Read data from the file into a DataChunk
DataChunkPtr mxflib::MXFFile::Read(size_t Size)
{
	DataChunkPtr Ret = new DataChunk(Size);

	if(Size)
	{
		size_t Bytes;

		if(isMemoryFile)
		{
			Bytes = MemoryRead(Ret->Data, Size);
		}
		else
		{
			Bytes = FileRead(Handle, Ret->Data, Size);
		}

		// Handle errors
		if(Bytes == static_cast<size_t>(-1))
		{
			error("Error reading file \"%s\" at 0x%s - %s\n", Name.c_str(), Int64toHexString(Tell(), 8).c_str(), strerror(errno));
			Bytes = 0;
		}

		if(Bytes != Size) Ret->Resize(Bytes);
	}

	return Ret;
}


//! Read data from the file into a supplied buffer
size_t mxflib::MXFFile::Read(UInt8 *Buffer, size_t Size)
{
	size_t Ret = 0;

	if(Size)
	{
		if(isMemoryFile)
		{
			Ret = MemoryRead(Buffer, Size);
		}
		else
		{
			Ret = FileRead(Handle, Buffer, Size);
		}

		// Handle errors
		if(Ret == static_cast<size_t>(-1))
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
bool mxflib::MXFFile::GetRIP(Length MaxScan /* = 1024*1024 */ )
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
	UInt64 FileEnd = Tell();

	// File smaller than 20 bytes! No chance of a RIP
	if(FileEnd < 20) return false;

	Seek(FileEnd - 4);
	UInt64 Location = Tell();

	UInt32 RIPSize = ReadU32();

	// If the RIP size would be bigger than the file it can't be a valid RIP
	if(RIPSize > Location) return false;

	// If we have a valid RIP then RIPSize bytes from back the end of the file will be the RIP key
	Seek(FileEnd - RIPSize);
	UInt64 RIPStart = Tell();
	DataChunkPtr RIPKey = Read(16);

	// Something went wrong with the read!
	if(RIPKey->Size != 16) return false;

	// Do a key lookup on this key
	MDOTypePtr KeyType = MDOType::Find(UL(RIPKey->Data));

	// If not a RIP, then exit
	if((!KeyType) || (!KeyType->IsA(RandomIndexMetadata_UL))) return false;

	// Go back and read the RIP
	Seek(RIPStart);
	MDObjectPtr RIPObject = ReadObject();
	if(!RIPObject) return false;

	MDObjectPtr PartitionArray = RIPObject[PartitionArray_UL];
	if(!PartitionArray) return false;

	MDObject::iterator it = PartitionArray->begin();
	while(it != PartitionArray->Value->end())
	{
		UInt32 BodySID = (*it).second->Child(0)->GetUInt();

		UInt64 ByteOffset = (*it).second->Child(1)->GetUInt64();

		debug("BodySID = 0x%04x, ByteOffset = %s\n", BodySID, Int64toString(ByteOffset).c_str());

		FileRIP.AddPartition(NULL, ByteOffset, BodySID);

		it++;
	}
/*	MDObjectULList::iterator it = PartitionArray->begin();
	while(it != PartitionArray->end())
	{
		UInt32 BodySID = (*it).second->Value[0]->GetUInt();

		it++;
		if(it == PartitionArray->end())
		{
			error("Unexpected end of pack in RIP at %s\n", RIPObject->GetSourceLocation().c_str());
			break;
		}

		UInt64 ByteOffset = (*it).second->Value[1]->GetUInt64();

		debug("BodySID = 0x%04x, ByteOffset = %s\n", BodySID, Int64toString(ByteOffset).c_str());

		FileRIP.AddPartition(NULL, ByteOffset, BodySID);

		it++;
	}
*/
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
bool mxflib::MXFFile::ScanRIP(Length MaxScan /* = 1024*1024 */ )
{
	// Remove any old data
	FileRIP.clear();

	FileRIP.isGenerated = true;

	// Read the header
	Seek(0);
	PartitionPtr Header = ReadPartition();

	// Header not found (might not be an error - the file could be empty)
	if(!Header) return false;

	Position FooterPos = Header->GetInt64(FooterPartition_UL);
	
	if(FooterPos == 0)
	{
		FooterPos = ScanRIP_FindFooter(MaxScan);
		if(FooterPos == 0) return false;
	}

	// Store the footer in the RIP and loop back through all other partitions
	UInt64 PartitionPos = FooterPos;
	
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

		UInt32 BodySID = ThisPartition->GetUInt(BodySID_UL);

		debug("Adding %s for BodySID 0x%04x at 0x%s\n", ThisPartition->Name().c_str(), BodySID, Int64toHexString(PartitionPos, 8).c_str());

		// Add the new partition
		FileRIP.AddPartition(ThisPartition, PartitionPos, BodySID);

		// Stop once we have added the header
		if(PartitionPos == 0) break;

		UInt64 NewPos = ThisPartition->GetUInt64(PreviousPartition_UL);
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
Position MXFFile::ScanRIP_FindFooter(Length MaxScan)
{
	Position FooterPos = 0;

	// Size of scan chunk when looking for footer key
	static const unsigned int ScanChunkSize = 16384;

	// Check that the scan chunk size is valid
	mxflib_assert( (sizeof(size_t) >= 8) || (ScanChunkSize <= 0xffffffff));

	// If too small a scan range is given we can't scan!
	if(MaxScan < 20) return 0;

	Length ScanLeft = MaxScan;			// Number of bytes left to scan
	SeekEnd();
	Position FileEnd = Tell();			// The file end
	Position ScanPos = FileEnd;			// Last byte of the current scan chunk

	while(ScanLeft)
	{
		Length ThisScan;				// Number of bytes to scan this time
		
		// Scan the number of bytes left, limited to the chunk size
		if(ScanLeft > ScanChunkSize) ThisScan = ScanChunkSize; else ThisScan = ScanLeft;
		
		// Don't scan off the start of the file
		if(ThisScan > ScanPos) ThisScan = ScanPos;

		// Quit if we ran out of bytes to scan
		if(ThisScan == 0) return 0;

		// Read this chunk
		Seek(ScanPos - ThisScan);
		DataChunkPtr Chunk = Read(static_cast<size_t>(ThisScan));

		// Quit if the read failed
		if(Chunk->Size != ThisScan) return 0;

		// Reduce remaining scan size
		ScanLeft -= ThisScan;

		unsigned char *p = Chunk->Data;
		Position i;
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
					UInt64 Location = Tell();
					DataChunkPtr Key = Read(16);

					if(Key->Size == 16)
					{
						MDOTypePtr Type = MDOType::Find(UL(Key->Data));
						if(Type)
						{
							if(Type->IsA(CompleteFooter_UL) || Type->IsA(Footer_UL))
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

	UInt64 Location = 0;

	// Read the first partition pack in the file
	PartitionPtr ThisPartition = ReadPartition();

	// If we couldn't read the first object then there are no partitions
	// Note that this is not stricty an error - the file could be empty!
	if(!ThisPartition) return true;

	// Check that the first KLV is a partition pack
	// DRAGONS: What if the first KLV is a filler? - This shouldn't be valid as it would look like a run-in!
	if(!(ThisPartition->IsA(PartitionMetadata_UL)) )
	{
		error("First KLV in file \"%s\" is not a known partition type\n", Name.c_str());
		return false;
	}

	for(;;)
	{
		UInt32 BodySID = 0;
		MDObjectPtr Ptr = ThisPartition[BodySID_UL];
		if(Ptr) BodySID = Ptr->Value->GetInt();

		FileRIP.AddPartition(ThisPartition, Location, BodySID);

		Length Skip;

		// Work out how far to skip ahead
		Length HeaderByteCount = ThisPartition->GetInt64(HeaderByteCount_UL);
		Skip = HeaderByteCount + ThisPartition->GetInt64(IndexByteCount_UL);

		if(Skip)
		{
			// Location before we skip
			Position PreSkip = Tell();

#if MXF_VERSION_10_CHECK
			/* Check for version 10 HeaderByteCount and possible bug version */
			// Version 10 of MXF counts from the end of the partition pack, however
			// some version 10 code uses the version 11 counting so we check to 
			// see if the header is claimed to be an integer number of KAGs and
			// that there is a leading filler to take us to the start of the next KAG
			// In this case we are probably in a Version 10 HeaderByteCount bug situation...

			UInt16 Ver = 0;
			Ptr = ThisPartition[MinorVersion_UL];
			Ver = Ptr->Value->GetUInt();

			bool SkipFiller = true;		// Version 11 behaviour

			if(Ver == 1)				// Ver = 1 for version 10 files
			{
				UInt32 KAGSize = 0;
				Ptr = ThisPartition[KAGSize_UL];
				if(Ptr) KAGSize = Ptr->GetInt();

				UInt64 HeaderByteCount = 0;
				Ptr = ThisPartition[HeaderByteCount_UL];
				if(Ptr) HeaderByteCount = Ptr->GetInt();

				if((KAGSize > 16) && (HeaderByteCount > 0))
				{
					UInt64 Pos = Tell();
					UInt64 U = HeaderByteCount / KAGSize;
					if((U * KAGSize) == HeaderByteCount)
					{
						MDObjectPtr First = ReadObject();
						if(!First)
						{
							// Can't tell what is next! This will probably cause an error later!
							SkipFiller = true;
							Seek(Pos);
						}
						else if(First->IsA(KLVFill_UL))
						{
							U = Tell() / KAGSize;
							if( (U * KAGSize) == (UInt64)Tell() )
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
#endif // MXF_VERSION_10_CHECK
			{
				MDObjectPtr First = ReadObject();
				if(!First)
				{
					error("Error reading first KLV after %s at 0x%s in %s\n", ThisPartition->FullName().c_str(), 
						  Int64toHexString(ThisPartition->GetLocation(),8).c_str(), ThisPartition->GetSource().c_str());
					return false;
				}

				if(First->IsA(KLVFill_UL))
					PreSkip = Tell();
				else if(HeaderByteCount)
				{
					if(!First->IsA(Primer_UL))
					{
						error("When HeaderByteCount is not zero, the first KLV following a partition pack (and any trailing filler) must be a Primer, however %s found at 0x%s in %s\n", 
							First->FullName().c_str(), Int64toHexString(First->GetLocation(),8).c_str(), 
							First->GetSource().c_str());
					}
				}
				else if(!First->IsA(IndexTableSegment_UL))
				{
					error("When HeaderByteCount is zero, but IndexByteCount is not zero, the first KLV following a partition pack (and any trailing filler) must be an IndexTableSegment, however %s found at 0x%s in %s\n", 
						  First->FullName().c_str(), Int64toHexString(First->GetLocation(),8).c_str(), 
						  First->GetSource().c_str());
				}
			}

			// Skip over header
			Position NextPos = PreSkip + Skip;
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
					if(ThisType->IsA(PartitionMetadata_UL))
					{
						break;
					}
				}
			}

			Skip = ReadBER();
			Position NextPos = Tell() + Skip;

			// Test for badly damaged file - have we ended up in a sequence of zeros?
			if(Skip == 0)
			{
				// If a zero length is followed by 16 zero bytes - abandon all hope!
				UInt8 Buff[16];
				if(Read(Buff, 16) == 16)
				{
					UInt8 *p = Buff;
					int Count = 16;
					while(Count && (*(p++) == 0)) Count--;
					if(!Count)
					{
						error("Unexpected sequence of zero bytes where a KLV stream was expected starting at 0x%s in file \"%s\"\n",
							  Int64toHexString(NextPos,8).c_str(), Name.c_str() );
						return false;
					}
				}
			}

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
		UInt32 i = (UInt32)Ret & 0x7f;
		Len = Read(i);
		if(Len->Size != i)
		{
			error("Incomplete BER length in file \"%s\" at 0x%s\n", Name.c_str(), Int64toHexString(Tell(),8).c_str());
			return -1;
		}

		Ret = 0;
		UInt8 *p = Len->Data;
		while(i--) Ret = ((Ret<<8) + *(p++));
	}

	return Ret;
}


//! Read a Key length from the open file
ULPtr mxflib::MXFFile::ReadKey(void)
{
	ULPtr Ret;

//	UInt64 Location = Tell();
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
	Int64 CurrentPosition = Tell();
	ThisPartition->SetInt64(ThisPartition_UL, CurrentPosition);

	// Adjust properties for a footer
	if(ThisPartition->IsA(CompleteFooter_UL) || ThisPartition->IsA(Footer_UL))
	{
		ThisPartition->SetInt64(FooterPartition_UL, CurrentPosition);
		ThisPartition->SetUInt(BodySID_UL, 0);
		ThisPartition->SetUInt64(BodyOffset_UL, 0);
	}

	// Find the partition before the current partition
	PartitionInfoPtr PrevPart = FileRIP.FindPreviousPartition(CurrentPosition);

	// Set the previous partition location, or zero if not known
	if(PrevPart) ThisPartition->SetInt64(PreviousPartition_UL, PrevPart->ByteOffset);
	else ThisPartition->SetInt64(PreviousPartition_UL, 0);

	// Add this partition to the RIP, but don't store the partition as we don't
	// own it and therefore cannot prevent changes after writing
	FileRIP.AddPartition(NULL, CurrentPosition, ThisPartition->GetUInt(BodySID_UL));

	DataChunkPtr Buffer = ThisPartition->WriteObject(UsePrimer);

	Write(Buffer->Data, Buffer->Size);
}


//! Calculate the size of a filler to align to a specified KAG
/*! Note: Maximum supported filler size is 0x00ffffff bytes */
UInt32 MXFFile::FillerSize(bool ForceBER4, UInt64 FillPos, UInt32 KAGSize, UInt32 MinSize /*=0*/)
{
	if(KAGSize == 0) KAGSize = 1;

	// Work out how far into a KAG we are
	UInt32 Offset = (UInt32)(FillPos % KAGSize);

	// Don't insert anything if we are already aligned and not padding
	if((Offset == 0) && (MinSize == 0)) return 0;

	// Work out the required filler size
	UInt32 Fill = KAGSize - Offset;

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
UInt64 MXFFile::Align(bool ForceBER4, UInt32 KAGSize, UInt32 MinSize /*=0*/)
{
	if(KAGSize == 0) KAGSize = 1;

	// Work out how big a filler we need
	UInt32 Fill = FillerSize(ForceBER4, Tell(), KAGSize, MinSize);

	// Nothing to do!
	if(Fill == 0) return Tell();
//if(Fill>0x10000)
//printf("%d: Fill- %x\n",Handle, Fill );
	if(Feature(FeatureVersion1KLVFill))
	{
		// Write the version 1 filler key
		Write(KLVFill_UL.GetValue(), 7);
		WriteU8(1);
		Write(&KLVFill_UL.GetValue()[8], 8);
	}
	else
	{
	// Write the filler key
		Write(KLVFill_UL.GetValue(), 16);
	}

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
	UInt8 Buffer[256];
	if(Fill > 256)
	{
		memset(Buffer, 0, 256);
		while(Fill > 256)
		{
			Write(Buffer, 256); 
			Fill -= 256;
		}
	}
	else
		memset(Buffer, 0, Fill);

	Write(Buffer, Fill); 

	return Tell();
}


namespace
{

	//! Write a metadictionary, or KXS extensions, for a given partition, into a buffer
	// FIXME: Need to prevent adding a metadictionary to a file that has a metadictionary!
	void WriteMetadict(DataChunkPtr &Buffer, PartitionPtr &ThisPartition, PrimerPtr &UsePrimer)
	{

		if(Feature(FeatureFullMetadict))
		{
			warning("Full metadictionary of all known classes, properties and types, not currently supported\n");
		}

		// Build the metadictionary
		MDObjectPtr MetaDict = BuildMetadictionary(ThisPartition->AllMetadata, UsePrimer);

		// Return if the build failed
		if(!MetaDict) return;

		if(Feature(FeatureKXSMetadict))
		{
			MDObjectPtr Preface;

			// Locate the preface
			MDObjectList::iterator it = ThisPartition->TopLevelMetadata.begin();
			while(it != ThisPartition->TopLevelMetadata.end())
			{
				if((*it)->IsA(Preface_UL))
				{
					Preface = (*it);
					break;
				}

				if((*it)->IsA(Root_UL))
				{
					Preface = (*it)->GetRef(RootPreface_UL);
					break;
				}
				it++;
			}

			// Long scan if quick find failed
			if(!Preface)
			{
				MDObjectList::iterator it = ThisPartition->AllMetadata.begin();
				while(it != ThisPartition->AllMetadata.end())
				{
					if((*it)->IsA(Preface_UL))
					{
						Preface = (*it);
						break;
					}
					it++;
				}
			}

			if(Feature(FeatureKXSMetadict))
			{
				MDObjectPtr Root = new MDObject(Root_UL);
				if(Root)
				{
					// Add the single extension set
					// FIXME: Need to have a UL available!  Ptr = Root->AddChild(RootExtensions_UL);
					MDObjectPtr Ptr = Root->AddChild("RootExtensions");
					if(Ptr) Ptr = Ptr->AddChild();
					if(Ptr) Ptr->MakeRef(MetaDict);

					// Add the structure version number for KXS 1b
					Root->SetInt(RootFormatVersion_UL, 0x12);

					// Add reference to the preface
					// DRAGONS: This must be after all the other definitions as this will cause the 
					//          definitions to be written before the items that they define!
					Ptr = Root->AddChild(RootPreface_UL);
					if(Ptr && Preface) Ptr->MakeRef(Preface);

					// Add the root
					Root->WriteLinkedObjects(Buffer, UsePrimer);
				}
			}
		}
		else
		{
			MetaDict->WriteLinkedObjects(Buffer, UsePrimer);
		}
	}
}


//! Write or re-write a partition pack and associated metadata (and index table segments?)
/*! \note Partition properties are updated from the linked metadata
 *	\return true if (re-)write was successful, else false
 */
bool MXFFile::WritePartitionInternal(bool ReWrite, PartitionPtr ThisPartition, bool IncludeMetadata, DataChunkPtr IndexData, PrimerPtr UsePrimer, UInt32 Padding, UInt32 MinPartitionSize)
{
	//! Length of the partition pack for calculating block alignment
	Length PartitionPackSize = 0;

	//! Essence BodySID for calculating block alignment
	UInt32 BodySID = 0;

	//! Length of block alignment padding after the header metadata (if required)
	Length BlockAlignHeaderBytes = 0;

	//! Length of block alignment padding after the index table (if required)
	Length BlockAlignIndexBytes = 0;

	//! Previous partition if re-writing
	PartitionPtr OldPartition;

	//! Start from a Primer that contains only built-in knowledge
	//! and add other entries as encountered within the Metadata
	PrimerPtr ThisPrimer;
	if(UsePrimer) ThisPrimer = UsePrimer; else ThisPrimer = MDOType::MakeBuiltInPrimer();

	DataChunkPtr RootBuffer = new DataChunk;		//! for Root and MetaDict - remains empty for non Avid
	DataChunkPtr MetadataBuffer = new DataChunk;	//! for Header Metadata, aka Preface object
	MDObjectPtr Preface;
	MDObjectPtr MetaDictionary;

	DataChunkPtr PrimerBuffer = new DataChunk;		//! for Primer - filled AFTER RootBuffer and MetadataBuffer

	DataChunkPtr ObjectDirBuffer = new DataChunk;	//! for Avid Object Directory - remains empty for non Avid

	// Is this a footer?
	bool IsFooter = (ThisPartition->IsA(CompleteFooter_UL) || ThisPartition->IsA(Footer_UL));

	// Should we build a metadictionary / KXS root and extensions?
	bool WritePreface = true;
	if(IncludeMetadata && Feature(FeatureSaveMetadict))
	{
		WriteMetadict(MetadataBuffer, ThisPartition, ThisPrimer);
		
		// Disable writing of the preface set below if we wrote a metadictionary
		if(MetadataBuffer->Size != 0) WritePreface = false;
	}

	// Write all objects
	MDObjectList::iterator it = ThisPartition->TopLevelMetadata.begin();
	while(it != ThisPartition->TopLevelMetadata.end())
	{
		if(IncludeMetadata)
		{
			// FIXME: This is a fudge to prevent the preface being written twice with KXS
			//        We still need to write any other "top level" items
			if(WritePreface || (!(*it)->IsA(Preface_UL)))
				(*it)->WriteLinkedObjects(MetadataBuffer, ThisPrimer);
		}

		// See if we are dealing with the preface - if so, we need to extract some data
		if((*it)->IsA(Preface_UL)) Preface = (*it);

		if((*it)->IsA(MetaDictionary_UL)) MetaDictionary = (*it);

		it++;
	}

	if(Preface)
	{
		// Update OP label
		MDObjectPtr DstPtr = ThisPartition[OperationalPattern_UL];
		MDObjectPtr SrcPtr = Preface[OperationalPattern_UL];
		if((SrcPtr) && (DstPtr))
		{
			DstPtr->ReadValue(SrcPtr->Value->PutData());
		}

		// Update essence containers
		DstPtr = ThisPartition[EssenceContainers_UL];
		if(DstPtr)
		{
			DstPtr->clear();
			SrcPtr = Preface[EssenceContainers_UL];

			if(SrcPtr)
			{
				DstPtr->SetValue(SrcPtr);
			}
		}
	}

	// Get the KAG size
	UInt32 KAGSize = ThisPartition->GetUInt(KAGSize_UL);

	// Align if required (not if re-writing)
	if((!ReWrite) && (KAGSize > 1)) Align(KAGSize);

	// Work out the index size
	UInt64 IndexByteCount = 0;
	if(IndexData) IndexByteCount = IndexData->Size;

	// Initialy we have no header data
	UInt64 HeaderByteCount = 0;

	// Read the old partition pack if re-writing
	if(ReWrite)
	{
		Position CurrentPos = Tell();
		OldPartition = ReadPartition();
		PartitionPackSize = (Length)(Tell() - CurrentPos);
		
		if(!OldPartition)
		{
			error("Failed to read old partition pack in MXFFile::ReWritePartition()\n"); 
			return false;
		}

		// Move back to re-write partition pack
		Seek(CurrentPos);
	}

	// If we are going to be doing block alignment we will need to know the size of the partition pack
	else if(BlockAlign)
	{
		// Write the (currently incomplete) partition pack to determine its size
		Position CurrentPos = Tell();
		WritePartitionPack(ThisPartition);
		PartitionPackSize = (Length)(Tell() - CurrentPos);
		
		// Then move back so that we re-write it later
		Seek(CurrentPos);

		// Read the partition's body SID so we know if there is essence in this partition
		BodySID = ThisPartition->GetUInt(BodySID_UL);
	}

	if(IncludeMetadata)
	{

		// Serialize the primer as KLV
		ThisPrimer->WritePrimer(PrimerBuffer);

		// Set size of header metadata (including the Primer, Root, ObjectDir, and toplevel objects (Preface etc))
		HeaderByteCount = PrimerBuffer->Size + RootBuffer->Size + ObjectDirBuffer->Size + MetadataBuffer->Size;

		// TODO: We need to do these calculations even if only rewriting with an index table (it could happen!)
		if(ReWrite)
		{
			// Old sizes in existing partition - which include padding
			UInt64 OldHeaderByteCount = OldPartition->GetUInt64(HeaderByteCount_UL);
			UInt64 OldIndexByteCount = OldPartition->GetUInt64(IndexByteCount_UL);

			// Can we make the index data start exactly where it used to?
			// Criteria for re-using the old layout is:
			//   * The new header metadata must be no bigger than the old metadata plus its filler
			//   * The new filler following the new header metadata bust be at least 17 bytes (or exactly zero bytes)
			//   * The new index table must be no bigger than the old table plus its filler
			//   * The new filler following the new index table bust be at least 17 bytes (or exactly zero bytes)
			if(    (HeaderByteCount <= OldHeaderByteCount) 
				&& ( ((OldHeaderByteCount - HeaderByteCount) >= 17) || (OldHeaderByteCount == HeaderByteCount))
			    && (IndexByteCount <= OldIndexByteCount) 
				&& ( ((OldIndexByteCount - IndexByteCount) >= 17) || (OldIndexByteCount == IndexByteCount)) )
			{
				BlockAlignHeaderBytes = OldHeaderByteCount - HeaderByteCount;
				HeaderByteCount = OldHeaderByteCount;

				BlockAlignIndexBytes = OldIndexByteCount - IndexByteCount;
				IndexByteCount = OldIndexByteCount;
			}
			else
			{
				// We need to figure out a new layout - this may break any block alignment

				// Minimum new sizes to obey KAG rules (without padding)
				UInt64 NewHeaderByteCount = HeaderByteCount + FillerSize(HeaderByteCount, KAGSize);
				UInt64 NewIndexByteCount = 0;
				if(IndexData) NewIndexByteCount = IndexByteCount + FillerSize(IndexByteCount, KAGSize);

				// Record the required extra padding size to make the new partition match the old one
				// DRAGONS: If it won't fit this will burst the -ve, but the following error check will also fire so it doesn't matter
				Padding =(UInt32)( (OldHeaderByteCount+OldIndexByteCount) - (NewHeaderByteCount+NewIndexByteCount) );


				// Minimum possible filler size is 17 bytes
				// TODO: Actually the padding may be OK at < 17 if we are aligning to a KAG. Better checking could be done...
				if(((NewHeaderByteCount+NewIndexByteCount) > (OldHeaderByteCount+OldIndexByteCount)) || (Padding < 17))
				{
					warning("Not enough space to re-write updated header metadata at position 0x%s in %s\n", Int64toHexString(Tell(), 8).c_str(), Name.c_str());
					return false;
				}

				// Add the padding to the required section
				if(IndexData)

				{
					IndexByteCount = NewIndexByteCount + Padding;

					HeaderByteCount = NewHeaderByteCount;

				}
				else
					HeaderByteCount = NewHeaderByteCount + Padding;


				// Prevent further padding

				Padding = 0;

			}

			// We can't obey a MinPartitionSize request
			MinPartitionSize = 0;
		}
		else
		{
			if(BlockAlign && (IndexData || (BodySID != 0)))
			{
				// Allow block alignment padding (calculated later)
			}
			// Padding follows the header if no index data
			else if(((Padding > 0) || (MinPartitionSize > HeaderByteCount)) && (!IndexData))
			{
				// Work out which of the two padding methods requires the greater padding
				Length UsePadding = (Length)MinPartitionSize - (Length)HeaderByteCount;
				if(UsePadding > (Length)Padding) Padding = (UInt32)UsePadding;

				HeaderByteCount += FillerSize(HeaderByteCount, KAGSize, Padding);
			}
			// Otherwise just pad to the KAG
			else if((!IsFooter) || (IndexData))
			{
				HeaderByteCount += FillerSize(HeaderByteCount, KAGSize);
			}
		}

		ThisPartition->SetUInt64(HeaderByteCount_UL, HeaderByteCount);
	}
	else
	{
		ThisPartition->SetUInt64(HeaderByteCount_UL, 0);
	}

	// Some systems can run more efficiently if the essence and index data start on a block boundary in addition to any alignment provided by KAG
	// We perform this alignment here if the partition contains an index table or essence data
	if(!ReWrite && (BlockAlign && (IndexData || (BodySID != 0))))
	{
		// Work out how far off the block grid we are at the start of this partition
		Position BlockOffset = Tell() % BlockAlign;

		// Calculate how many bytes will be written before the start of the header metadata
		Length BytesBefore = PartitionPackSize;
		
		// There will be a filler between the partition pack and the header metadata if KAG > 1 and there is header metadata
		if((KAGSize > 1) && (HeaderByteCount > 0)) BytesBefore += FillerSize(PartitionPackSize, KAGSize);

		// Work out what alignment offset to use (+ve aligns us after the block boundary, -ve before)
		Int32 Offset;
		if(IndexData) Offset = BlockAlignIndexOffset; else Offset = BlockAlignEssenceOffset;

		/* So, (BlockOffset + BytesBefore + HeaderByteCount + RequiredPadding + Offset) must be an integer number of blocks */
		
		// Work out how many blocks are completely or partially used by everything except the padding				
		Length Blocks = ((BlockOffset + BytesBefore + HeaderByteCount + Offset - 1) / BlockAlign) + 1;
		
		// Preserve the old byte count
		UInt64 OldHeaderByteCount = HeaderByteCount;

		// Make the header size equal to the number of bytes in that many blocks, less the preceeding data
		HeaderByteCount = (Blocks * BlockAlign) - (BytesBefore + BlockOffset) + Offset;

		// Work out the number of padding bytes required
		BlockAlignHeaderBytes = HeaderByteCount - OldHeaderByteCount;

		// If this is too small for a BER-4 filler KLV keep adding blocks until it is big enough
		while(BlockAlignHeaderBytes < 20)
		{
			BlockAlignHeaderBytes += BlockAlign;
			HeaderByteCount += BlockAlign;
		}

		if(!IndexData)
		{
			// If this is less than the requested padding keep adding blocks until it is enough
			while(BlockAlignHeaderBytes < Padding)
			{
				BlockAlignHeaderBytes += BlockAlign;
				HeaderByteCount += BlockAlign;
			}

			// Ensure we meet the requested minimum partition size
			while((BytesBefore + OldHeaderByteCount + BlockAlignHeaderBytes) < MinPartitionSize)
			{
				BlockAlignHeaderBytes += BlockAlign;
				HeaderByteCount += BlockAlign;
			}
		}

		// If this padding will come after some header metadata, add its size to the header size
		// DRAGONS: If we will not be writing any header metadata this block padding will simply be the filler after the partition pack
		if(OldHeaderByteCount > 0) ThisPartition->SetUInt64(HeaderByteCount_UL, HeaderByteCount);
	}

	// Calculate the index byte size
	if(IndexData)
	{
		// Some systems can run more efficiently if the essence and index data start on a block boundary in addition to any alignment provided by KAG
		if((!ReWrite) && BlockAlign && (BodySID != 0))
		{
			// Work out how far off the block grid we are at the start of this partition
			Position BlockOffset = Tell() % BlockAlign;

			// Calculate how many bytes will be written before the start of the index
			Length BytesBefore = PartitionPackSize;
			if(KAGSize > 1) BytesBefore += FillerSize(PartitionPackSize, KAGSize);
			BytesBefore += HeaderByteCount;


			/* So, (BlockOffset + BytesBefore + IndexByteCount + RequiredPadding + BlockAlignEssenceOffset) must be an integer number of blocks */
			
			// Work out how many blocks are completely or partially used by everything except the padding				
			Length Blocks = ((BlockOffset + BytesBefore + IndexByteCount + BlockAlignEssenceOffset - 1) / BlockAlign) + 1;
			
			// Preserve the old byte count
			UInt64 OldIndexByteCount = IndexByteCount;

			// Make the index size equal to the number of bytes in that many blocks, less the preceeding data
			IndexByteCount = (Blocks * BlockAlign) - (BytesBefore + BlockOffset) + BlockAlignEssenceOffset;

			// Work out the number of padding bytes required
			BlockAlignIndexBytes = IndexByteCount - OldIndexByteCount;

			// If this is too small for a BER-4 filler KLV keep adding blocks until it is big enough
			while(BlockAlignIndexBytes < 20)
			{
				BlockAlignIndexBytes += BlockAlign;
				IndexByteCount += BlockAlign;
			}

			// If this is less than the requested padding keep adding blocks until it is enough
			while(BlockAlignIndexBytes < Padding)
			{
				BlockAlignIndexBytes += BlockAlign;
				IndexByteCount += BlockAlign;
			}

			// Ensure we meet the requested minimum partition size
			while((BytesBefore + OldIndexByteCount + BlockAlignIndexBytes) < MinPartitionSize)
			{
				BlockAlignIndexBytes += BlockAlign;
				IndexByteCount += BlockAlign;
			}
		}
		else if( (!IsFooter) || (Padding > 0) || (MinPartitionSize > HeaderByteCount) )
		{
			// If we are not block aligning then we work out what padding to use
			// DRAGONS: BlockAlignIndexBytes may have been set earlier than the above block
			if(!BlockAlignIndexBytes)
			{
				// Work out which of the normal two padding methods requires the greater padding
				Length UsePadding = (Length)MinPartitionSize - (Length)(HeaderByteCount + IndexByteCount);
				if(UsePadding > (Length)Padding) Padding = (UInt32)UsePadding;

				IndexByteCount += FillerSize(IndexByteCount, KAGSize, Padding);
			}
		}

		ThisPartition->SetUInt64(IndexByteCount_UL, IndexByteCount);

	}
	else
	{
		// If we are re-writing, but not writing an index, keep the old index settings
		if(ReWrite)
		{
			ThisPartition->SetUInt(IndexSID_UL, OldPartition->GetUInt(IndexSID_UL));
			ThisPartition->SetUInt64(IndexByteCount_UL, OldPartition->GetUInt64(IndexByteCount_UL));
		}
		else
		{
			ThisPartition->SetUInt(IndexSID_UL, 0);
			ThisPartition->SetUInt64(IndexByteCount_UL, 0);
		}

	}

	// Write the pack
	WritePartitionPack(ThisPartition);

	// If we will not be writting metadata or index, but padding has been requested, write the filler here

	if((!IncludeMetadata) && (!IndexData) && (Padding > 0))

	{

		Align(KAGSize, Padding);

	}

	else

	{

		// Align if required

		// All non-footer partitions pack are followed by KAG alignment

		// Any partition with metadata or index has a KAG alignment after the partition pack

		if((KAGSize > 1) && ((!IsFooter) || IncludeMetadata || IndexData) && !BlockAlign) Align(KAGSize);

	}



	// Ensure the correct size of filler for the already written header byte count - it is possible for duff values to force 2 fillers
	UInt32 HeaderPadding = static_cast<UInt32>(HeaderByteCount - (PrimerBuffer->Size + MetadataBuffer->Size));

	if(IncludeMetadata)
	{
		// Write the primer
		Write(PrimerBuffer);

		{
			// Write the Root if writing an Avid compatible metadictionary
			if(RootBuffer->Size)
			{
				// Write the Root Object
				Write(RootBuffer);
				HeaderPadding -= static_cast<UInt32>(RootBuffer->Size);
			}

			// Write the other header metadata
			Write(MetadataBuffer);
		}
	}



	// Write a filler of the required size
	if(HeaderPadding) Align((UInt32)1, HeaderPadding);

	if(IndexData)
	{
		// Write the index data
		Write(IndexData);

		// Ensure the correct size of filler for the already written index byte count

		UInt32 IndexPadding = static_cast<UInt32>(IndexByteCount - IndexData->Size);



		// Write a filler of the required size

		if(IndexPadding) Align((UInt32)1, IndexPadding);

	}


	return true;
}


size_t MXFFile::MemoryWrite(UInt8 const *Data, size_t Size)
{
	if(BufferCurrentPos < BufferOffset)
	{
		error("Cannot currently write to a memory file before the buffer start\n");
		return 0;
	}

	// Copy the data to the buffer
	Buffer->Set(Size, Data, static_cast<size_t>(BufferCurrentPos - BufferOffset));

	// Update the pointer
	BufferCurrentPos += Size;

	// Return the number of bytes written
	return Size;
}


size_t MXFFile::MemoryRead(UInt8 *Data, size_t Size)
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
	size_t MaxBytes = Buffer->Size - static_cast<size_t>(BufferCurrentPos - BufferOffset);

	// Limit our read to the max available
	if(Size > MaxBytes) Size = MaxBytes;

//debug("Copy %d bytes from 0x%08x to 0x%08x\n", Size, &Buffer->Data[BufferCurrentPos - BufferOffset], Data);
	// Copy the data from the buffer
	if(Buffer->Data != 0)
		memcpy(Data, &Buffer->Data[static_cast<size_t>(BufferCurrentPos - BufferOffset)], Size);
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



//! Locate and read a partition containing closed header metadata
/*! \ret NULL if none found
 * TODO: Fix slightly iffy assumptions about RIP
 * DRAGONS: A significant number fo invalid files exist with a closed incomplete header and a closed complete footer. CheckForCompleteFooter=true will return the footer for these files.
 */
PartitionPtr MXFFile::ReadMasterPartition(Length MaxScan, bool CheckForCompleteFooter /*=false*/)
{
	PartitionPtr Ret;
	PartitionPtr IncompleteHeader;

	// Start by checking the header
	Seek(0);

	// Read the header
	Ret = ReadPartition();
	if(!Ret) return Ret;

	// If the header is closed and complete return it
	if(Ret->IsA(ClosedCompleteHeader_UL)) return Ret;
	
	// If the header is closed and incomplete, see if this if OK, otherwise store and try the footer
	if(Ret->IsA(ClosedHeader_UL))
	{
		if (!CheckForCompleteFooter) return Ret;
		IncompleteHeader = Ret;
	}

	/* The header is open - so we must locate the footer */
	Position FooterPos = 0;

	// First we see if the header partition tells us where the footer is
	FooterPos = Ret->GetInt64(FooterPartition_UL);

	// If the footer position is not specified in the header we must look for it
	if(FooterPos == 0)
	{
		// If we don't already have a RIP read one if we can!
		if(FileRIP.size() < 2)
		{
			// If we didn't manage to read a RIP try scanning for the footer partition
			if(!ReadRIP())
			{
				FooterPos = ScanRIP_FindFooter(MaxScan);
				if(FooterPos == 0) return NULL;
			}
		}

		// We use the RIP, but not if we successfully scanned for the footer
		if(FooterPos == 0)
		{
			// Locate the last partition in the file
			RIP::iterator it = FileRIP.end();
			it--;
			FooterPos = (*it).second->ByteOffset;
		}
	}

	// Read the footer
	Seek(FooterPos);
	Ret = ReadPartition();

	// If it is a footer, and it contains metadata, see if this is what we want
	if(Ret)
	{
		if(Ret->GetInt64(HeaderByteCount_UL))
		{
			// Always return closed and complete header with metadata
			if(Ret->IsA(CompleteFooter_UL)) return Ret;

			// Closed but incomplete footer
			if(Ret->IsA(Footer_UL))
			{
				// If the header was also closed but incomplete, return the header as the master
				if(IncompleteHeader) return IncompleteHeader;

				// Otherwise the header must have been open, so return this footer (even though it is incomplete)
				return Ret;
			}
		}
	}

	// TODO: Search for any closed body metadata for files with no footer?!
	return NULL;
}


//! Locate and read the footer partition
/*! \ret NULL if not found
 * TODO: Fix slightly iffy assumptions about RIP
 */
PartitionPtr MXFFile::ReadFooterPartition(Length MaxScan /*=1024*1024*/)
{
	PartitionPtr Ret;

	// Start by checking the FooterPosition value in the header
	Seek(0);

	// Read the header
	Ret = ReadPartition();
	if(!Ret) return Ret;

	// Now try to locate the footer
	Position FooterPos = 0;

	// First we see if the header partition tells us where the footer is
	FooterPos = Ret->GetInt64(FooterPartition_UL);

	// If the footer position is not specified in the header we must look for it
	if(FooterPos == 0)
	{
		// If we don't already have a RIP read one if we can!
		if(FileRIP.size() < 2)
		{
			// If we didn't manage to read a RIP try scanning for the footer partition
			if(!ReadRIP())
			{
				FooterPos = ScanRIP_FindFooter(MaxScan);
				if(FooterPos == 0) return NULL;
			}
		}

		// We use the RIP, but not if we successfully scanned for the footer
		if(FooterPos == 0)
		{
			// Locate the last partition in the file
			RIP::iterator it = FileRIP.end();
			it--;
			FooterPos = (*it).second->ByteOffset;
		}
	}

	// Read the footer
	Seek(FooterPos);
	Ret = ReadPartition();

	// Return the footer partition
	return Ret;
}


//! Write the RIP
void MXFFile::WriteRIP(void)
{
	MDObjectPtr RIPObject = new MDObject(RandomIndexMetadata_UL);
	mxflib_assert(RIPObject);

	if(RIPObject)
	{
		MDObjectPtr PA = RIPObject->AddChild(PartitionArray_UL);

		mxflib_assert(PA);
		if(PA)
		{
			RIP::iterator it = FileRIP.begin();
			while(it != FileRIP.end())
			{
				MDObjectPtr ThisEntry = PA->AddChild();

				if(ThisEntry->size() != 2)
				{
					error("Unable to build valid Random Index Pack - is the dictionary wrong?\n");
					return;
				}

				ThisEntry[0]->SetUInt((*it).second->BodySID);
				ThisEntry[1]->SetUInt64((*it).second->ByteOffset);

				it++;
			}
		}

		// Calculate the pack length
		RIPObject->SetUInt(Length_UL, 16 + 4 + (static_cast<UInt32>(FileRIP.size()) * 12) + 4);

		DataChunkPtr Buffer = RIPObject->WriteObject();

		Write(Buffer->Data, Buffer->Size);
	}
}


//! Is this file truncated?
bool MXFFile::IsTruncated(std::string *Details /*=NULL*/)
{
	/* There are cases where the file ends with a sequence of bytes of the same value, such as 0x00 or 0xff,
	 * often because a transfer has failed after allocating the full file size on the disc. 
	 * We test for this by looking at the lat 1K of the file, and the first 1K following the last valid KLV
	 * found for truncated files. If these 2K bytes are all the same value then chances are that all the bytes
	 * from the last found KLV to the end of the file are the same. We don't test every byte as this could be
	 * many, many GB.
	 */
	int SameByteCount = -1;						// The number of bytes at the end of the file that are identical (-1 if not checked)
	const int SameByteLimit = 1024;				// The number of identical bytes to regard as enough for this test
	UInt8 SameByte = 0xcc;						// What byte value if the last 1K

	// If we already know - return the value
	if(TruncatedKnown && (Details == NULL)) return Truncated;

	// Start by inspecting the header (and getting the file size)
	Length FileSize = Size();
	Seek(0);
	PartitionPtr Header = ReadPartition();

	// Flag that we have not yet determined the file to be truncated, and have not yet found the last KLV
	Truncated = false;
	Position LastKLV = -1;

	// If we can't even read the header - then the file is broken
	if(!Header)
	{
		*Details = "File does not start with a valid Header Partition Pack\n";
		TruncatedKnown = true;
		Truncated = true;
		return true;
	}

	// Does the header tell us where the footer is?
	Position FooterPos = Header->GetInt64(FooterPartition_UL);
	if(FooterPos != 0)
	{
		// See if the footer is at or beyond the end of the file
		SeekEnd();
		if(Tell() <= FooterPos)
		{
			if(Details)
			{
				std::stringstream s;
				s << "The Footer should start at 0x" << std::hex << std::setw(8) << std::setfill('0') << FooterPos;
				s << ", but the file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << Tell();
				*Details = s.str();
			}
			TruncatedKnown = true;
			Truncated = true;
			return true;
		}
	}
	else
	// Otherwise we must seek the footer ourselves
	{
		// Easy method - if the file ends in a RIP, then it is not truncated
		if(ReadRIP())
		{
			TruncatedKnown = true;
			Truncated = false;
			return false;
		}


		/* Scan the hard way */

		Position ChunkPos = FileSize;

		size_t ChunkSize = 32 * 1024;						// First check only the last 32K as a significant number of files have a small footer
		const size_t BigChunkSize = 1024 * 1024;			// Then we check in 1M chunks
		
		// Buffer big enough to hold the biggest chunk + 15 bytes (to allow key to stradle chunks)
		DataChunkPtr Buffer = new DataChunk(BigChunkSize + 15);

		// This could be a small metadata-only file - in which case the first chunk will be the whole file
		if(ChunkPos < ChunkSize) ChunkSize = static_cast<size_t>(ChunkPos);
		
		// Read the small first chunk
		Buffer->Resize(ChunkSize);
		ChunkPos -= ChunkSize;
		Seek(ChunkPos);
		Read(Buffer->Data, ChunkSize);

		// Do a simple check to see if the file seems to end in padding bytes
		if(Buffer->Data[ChunkSize - 1] == Buffer->Data[ChunkSize - 2])
		{
			// The last two bytes are the same, start scanning at the 3rd to last
			SameByteCount = 2;
			SameByte = Buffer->Data[ChunkSize - 2];
			UInt8 *pSame = &Buffer->Data[ChunkSize - 3];
			int Limit = SameByteLimit - 2;
			while(Limit)
			{
				if(*(pSame--) == SameByte) 
				{
					SameByteCount++;
					Limit--;
				}
				else
					break;
			}
		}

		size_t BytesToScan = ChunkSize;
		UInt8 *pScan = &Buffer->Data[Buffer->Size - 1];
		for(;;)
		{
			// Is this a 0x06 - if so it could be the start of one of teh keys we are scanning for
			if(*pScan == 0x06)
			{
				// Only a possible key if the next bytes are 0x0e 0x2b 0x 34
				if((pScan[1] == 0x0e) && (pScan[2] == 0x2b) && (pScan[3] == 0x34))
				{
					// Keys of interest:
					//
					// 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x??, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x04, 0x02, 0x00 = Footer
					// 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x??, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x04, 0x04, 0x00 = CompleteFooter
					// 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x??, 0x01, 0x??, 0x0d, 0x01, 0x03, 0x01, 0x04, 0x??, 0x??, 0x?? = CP System Data
					// 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x??, 0x01, 0x??, 0x0d, 0x01, 0x03, 0x01, 0x14, 0x??, 0x??, 0x?? = GC System Data
					// 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x??, 0x0d, 0x01, 0x03, 0x01, 0x??, 0x??, 0x??, 0x?? = CP/GC Essence Data
					// 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? = Some Sort of Essence Data

					// Groups (Sets and Packs)
					if(pScan[4] == 0x02)
					{
						// DRAGONS: We will never actually use the first 5 bytes. The 2 or 4 (Footer or CompleteFooter) in byte 14 will be tested separately
						static const UInt8 FooterKey[14] = { 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0xFF, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x04 };

						// DRAGONS: We will never actually use the first 5 bytes. The 0x04 or 0x14 (CP or GC) in byte 12 will be tested separately
						static const UInt8 CPGPSystem[12] = { 0x06, 0x0e, 0x2b, 0x34, 0x02, 0xFF, 0x01, 0xFF, 0x0d, 0x01, 0x03, 0x01 };

						// Version-less compare for either Footer key
						if((pScan[5] == FooterKey[5]) && (pScan[6] == FooterKey[6]) && (memcmp(&pScan[8], &FooterKey[8], 6) == 0))
						{
							if((pScan[14] == 0x02) || (pScan[14] == 0x04))
							{
								// Found a valid footer key - calculate the start location of this key
								FooterPos = ChunkPos + BytesToScan;
								break;
							}
						}
						// Version-less compare for any CP or GC System key
						else if((pScan[6] == CPGPSystem[6]) && (memcmp(&pScan[8], &CPGPSystem[8], 4) == 0))
						{
							if((pScan[12] == 0x04) || (pScan[12] == 0x14))
							{
								if(Details)
								{
									std::stringstream s;
									s << "The file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << FileSize << std::endl;
									s << "The last KLV before the end of the file is a System Item" << std::endl;
									*Details = s.str();
								}

								// Found a valid CP or GC System Key - can't be valid, so the file must be truncated
								TruncatedKnown = true;
								Truncated = true;
								LastKLV = ChunkPos + (pScan - Buffer->Data);
								break;
							}
						}
					}
					// Essence
					else if((pScan[4] == 0x01) && (pScan[5] == 0x02))
					{
						if(Details)
						{
							std::stringstream s;
							s << "The file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << FileSize << std::endl;
							s << "The last KLV before the end of the file is an Essence Item" << std::endl;
							*Details = s.str();
						}

						// Essence means file must be truncated
						TruncatedKnown = true;
						Truncated = true;
						LastKLV = ChunkPos + (pScan - Buffer->Data);
						break;
					}
				}
			}

			/* Move on to next (i.e. previous) byte */

			// Time to move back a chunk?
			if((--BytesToScan) == 0)
			{
				// We hit the start of the file !?!
				if(ChunkPos == 0) break;

				// Check in case we hit the start of the file in this chunk!
				if(ChunkPos < BigChunkSize)
				{
					ChunkSize = static_cast<size_t>(ChunkPos);
					ChunkPos = 0;
				} 
				else
				{
					ChunkSize = BigChunkSize;
					ChunkPos -= BigChunkSize;
				}

				// Copy the last 15 bytes tested so we can add them to the end of the test chunk (in case the last byte of the chunk is 0x06)
				// DRAGONS: We use memmove in case the last chunk is < 30 bytes long!
				Buffer->Resize(ChunkSize + 15, true);
				memmove(&Buffer->Data[ChunkSize], Buffer->Data, 15);

				// Read this chunk
				Seek(ChunkPos);
				Read(Buffer->Data, ChunkSize);
				pScan = &Buffer->Data[ChunkSize - 1];

				// Scan from here
				BytesToScan = ChunkSize;
				continue;
			}

			// Move back a byte
			pScan--;
		}
	}

	/* Did we just determine that the file is truncated? If so do a bit more digging */
	if(Truncated && Details)
	{
		// Check if the file ends before the L of this KLV
		if((LastKLV + 16) >= FileSize)
		{
			std::stringstream s;
			s << "The file ends at the start of this KLV" << std::endl;
			*Details += s.str();
		}
		else
		{
			Length LastKLVLength;

			// Check if the file ends during the L of this KLV, if not, read the length
			Seek(LastKLV + 16);
			int LenLen = FileGetc(Handle);
			if(LenLen <= 0x80) LastKLVLength = LenLen;
			else
			{
				LenLen -= 0x80;
				if((LastKLV + 16 + LenLen) > FileSize)
				{
					std::stringstream s;
					s << "The file ends at the start of this KLV" << std::endl;
					*Details += s.str();
				}
				else
				{
					Seek(LastKLV + 16);
					LastKLVLength = ReadBER();
					Position LastValuePos = Tell();

					if((LastValuePos + LastKLVLength) == FileSize)
					{
						std::stringstream s;
						s << "The file ends exactly at the end of this KLV" << std::endl;
						*Details += s.str();
					}
					else if((LastValuePos + LastKLVLength) > FileSize)
					{
						std::stringstream s;
						s << "The file ends during the value of this KLV" << std::endl;
						*Details += s.str();
					}

					/* Now try and determine if the file seems to have ended with padding bytes of some kind */
					if(SameByteCount > 16)
					{
						// Have we already determined that everything from the end of this KLV is the same value?
						if((LastValuePos + LastKLVLength + SameByteCount) >= FileSize)
						{
							std::stringstream s;
							s << "The truncated file ends with a sequence of " << SameByteCount << " bytes with the value 0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(SameByte) << std::endl;
							*Details += s.str();
						}
						else if(SameByteCount == SameByteLimit)
						{
							// So we now have the last KLV, then an indeterminate amount of space, then the end of the file, which we know is topped of with repeats
							// We could just scan every byte from here to there - but that could be billions, and may be a waste, so let's just check the start of this chunk
							Seek(LastValuePos + LastKLVLength);
							DataChunkPtr Chunk = Read(SameByteLimit);

							if(Chunk && (Chunk->Size == SameByteLimit))
							{
								UInt8 *pData = Chunk->Data;
								int Count = SameByteLimit;
								while(Count && (*(pData++) == SameByte)) Count--;
								if(Count == 0)
								{
									std::stringstream s;
									s << "The truncated file appears to end with a sequence of " << (FileSize - (LastValuePos + LastKLVLength)) << " bytes with the value 0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(SameByte) << std::endl;
									*Details += s.str();
								}
							}
						}
					}
				}
			}
		}

		return true;
	}

	/* We have found a footer - but the file could still be truncated (in the footer partition pack or metadata or index) */

	// Must be enough space for the key and length
	if(FileSize - FooterPos < 17)
	{
		if(Details)
		{
			std::stringstream s;
			s << "The file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << FileSize;
			s << ", during the Footer Partition Pack key" << std::endl;
			*Details = s.str();
		}

		TruncatedKnown = true;
		Truncated = true;
		return true;
	}

	// Manually read the footer KLV length, validating as we go
	Seek(FooterPos + 16);
	Length Len = FileGetc(Handle);
	if(Len == 0x80)
	{
		// Treat invalid BER as truncated
		error("Footer uses unsupported BER Length value of 0x80 meaning \"Unknown\"\n");

		if(Details) *Details = "Footer uses unsupported BER Length value of 0x80 meaning \"Unknown\"\n";

		TruncatedKnown = true;
		Truncated = true;
		return true;
	}
	if(Len > 0x80)
	{
		// Check that there is enough data for the remaining part of the BER length
		if((FileSize - FooterPos) < (16 + 1 + (Len - 0x80)))
		{
			if(Details)
			{
				std::stringstream s;
				s << "The file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << FileSize;
				s << ", part way through the Footer Partition Pack length field" << std::endl;
				*Details = s.str();
			}

			TruncatedKnown = true;
			Truncated = true;
			return true;
		}

		// Read the long-form BER using our normal method
		Seek(FooterPos + 16);
		Len = ReadBER();
	}

	// Do we have enough bytes to read the actual partition pack?
	if(Len > (FileSize - Tell()))
	{
		if(Details)
		{
			std::stringstream s;
			s << "The file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << FileSize;
			s << ", part way through the Footer Partition Pack value" << std::endl;
			*Details = s.str();
		}

		TruncatedKnown = true;
		Truncated = true;
		return true;
	}

	/* OK, we now know that there are enough bytes to read the footer partition pack - so we move back and read it the normal way */
	Seek(FooterPos);
	PartitionPtr Footer = ReadPartition();

	/* Let's see if we have a filler to skip */
	if((FileSize - Tell()) > 16)
	{
		Position TestPos = Tell();

		ULPtr TestKey = ReadKey();
		if(TestKey->Matches(KLVFill_UL))
		{
			// Manually read the filler length, validating as we go
			Length Len = FileGetc(Handle);
			if(Len == 0x80)
			{
				// Treat invalid BER as truncated
				error("KLVFill in Footer uses unsupported BER Length value of 0x80 meaning \"Unknown\"\n");

				if(Details) *Details = "KLVFill in Footer uses unsupported BER Length value of 0x80 meaning \"Unknown\"\n";

				TruncatedKnown = true;
				Truncated = true;
				return true;
			}
			if(Len > 0x80)
			{
				// Check that there is enough data for the remaining part of the BER length
				if((FileSize - Tell()) < (Len - 0x80))
				{
					if(Details)
					{
						std::stringstream s;
						s << "The file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << FileSize;
						s << ", part way through the length field of the KLVFill following the Footer Partition Pack" << std::endl;
						*Details = s.str();
					}

					TruncatedKnown = true;
					Truncated = true;
					return true;
				}

				// Read the long-form BER using our normal method
				Seek(Tell() - 1);
				Len = ReadBER();
			}

			// Do we have enough bytes for the actual KLVFill value?
			if(Len > (FileSize - Tell()))
			{
				if(Details)
				{
					std::stringstream s;
					s << "The file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << FileSize;
					s << ", part way through the value of the KLVFill following the Footer Partition Pack" << std::endl;
					*Details = s.str();
				}

				TruncatedKnown = true;
				Truncated = true;
				return true;
			}

			// Skip over the value
			Seek(Tell() + Len);
		}
		else
		{
			Seek(TestPos);
		}
	}

	// Work out the size of the rest of the footer
	Length HeaderBytes = Footer->GetInt64(HeaderByteCount_UL);
	Length IndexBytes = Footer->GetInt64(IndexByteCount_UL);

	// Determine if the file is truncated during the footer
	TruncatedKnown = true;
	if(FileSize - (Tell() + HeaderBytes + IndexBytes) >= 0)
		Truncated = false;
	else
	{
		if(FileSize - (Tell() + HeaderBytes) >= 0)
		{
			if(Details)
			{
				std::stringstream s;
				s << "The file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << FileSize;
				s << ", int the area expected to hold the Footer Index Table" << std::endl;
				*Details = s.str();
			}
		}
		else
		{
			if(Details)
			{
				std::stringstream s;
				s << "The file ends unexpectedly at 0x" << std::hex << std::setw(8) << std::setfill('0') << FileSize;
				s << ", int the area expected to hold the Footer copy of the Header Metadata" << std::endl;
				*Details = s.str();
			}
		}

		Truncated = true;
	}

	return Truncated;
}


//! Determine the wrapping type for a given Essence Container in this file
/*! \param BodySID SID of the EssenceContainer to examine
 *  \param ThisPartition A Partition with metadata to use for determining wrapping type
 *  \retval ClipWrap - Wrapping is Clip Wrap or similar (Wrapping size > Edit Unit)
 *  \retval FrameWrap - Wrapping is Frame Wrap or similar (Wrapping size <= Edit Unit)
 *	\retval UnknownWrap - Unable to determine
 */
WrapType MXFFile::GetWrapType(UInt32 BodySID, PartitionPtr ThisPartition)
{
	// Default answer is 'unknown'
	WrapType Ret = UnknownWrap;

	// Record the current position so that this method does not move the pointer
	Position Pos = Tell();

	// Ensure that we have the metadata for this partition
	if(ThisPartition->AllMetadata.empty()) ThisPartition->ReadMetadata();

	// Parse the metadata (or retreive already parsed object)
	MetadataPtr MData = ThisPartition->ParseMetadata();
	if(!MData)
	{
		// Return to the original position and return 'unknown'
		Seek(Pos);
		return Ret;
	}

	// DataChunk that will hold the UMID of the desired package, once we find it
	DataChunkPtr ECPackageID;

	MDObjectPtr ContentStorage = MData->GetRef(ContentStorageObject_UL);
	MDObjectPtr EssenceDataObjects = ContentStorage ? ContentStorage->Child(EssenceDataObjects_UL) : NULL;
	UInt32 IndexSID = 0;
	if(EssenceDataObjects)
	{
		MDObject::iterator it = EssenceDataObjects->begin();
		while(it != EssenceDataObjects->end())
		{
			MDObjectPtr ECData = (*it).second->GetRef();
			if(ECData && (ECData->GetUInt(BodySID_UL) == BodySID))
			{
				MDObjectPtr Ptr = ECData->Child(LinkedPackageUID_UL);
				if(Ptr) ECPackageID = Ptr->PutData();

				// Also record the IndexSID in case we need to do more detailed tests
				IndexSID = ECData->GetUInt(IndexSID_UL);

				break;
			}

			it++;
		}
	}

	// Unable to find the PackageID - maybe the BodySID is not valid
	if((!ECPackageID) || (ECPackageID->Size < 32))
	{
		// Return to the original position and return 'unknown'
		Seek(Pos);
		return Ret;
	}

	// Try and locate the source package with this UMID
	PackageList::iterator Package_it = MData->Packages.begin();
	while(Package_it != MData->Packages.end())
	{
		MDObjectPtr Ptr = (*Package_it)->Child(PackageUID_UL);
		DataChunkPtr PackageID = Ptr ? Ptr->PutData() : NULL;
		if(PackageID && (PackageID->Size == 32) && (memcmp(ECPackageID->Data, PackageID->Data, 32) == 0))
		{
			// Located matching package
			break;
		}
		Package_it++;
	}

	// Failed to locate source package?
	if(Package_it == MData->Packages.end())
	{
		// Return to the original position and return 'unknown'
		Seek(Pos);
		return Ret;
	}

	MDObjectPtr Descriptor = (*Package_it)->GetRef(Descriptor_UL);
	if(Descriptor)
	{
		if(Descriptor->IsA(MultipleDescriptor_UL))
		{
			// Check for first known sub-descriptor wrapping type by ECLabel
			MDObjectPtr Subs = Descriptor->Child(FileDescriptors_UL);
			if(Subs)
			{
				MDObject::iterator it = Subs->begin();
				while(it != Subs->end())
				{
					MDObjectPtr Ptr = (*it).second->GetLink();
					if(Ptr) Ptr = Ptr->Child(EssenceContainer_UL);
					if(Ptr)
					{
						DataChunkPtr ECLabel = Ptr->PutData();
						if(ECLabel && (ECLabel->Size == 16)) Ret = mxflib::GetWrapType(ECLabel->Data);
						if(Ret != UnknownWrap)
						{
							// Return to the original position and return newly determined wrapping type
							Seek(Pos);
							return Ret;
						}
					}
					it++;
				}
			}
		}
		else
		{
			// See if the descriptor has a known wrapping type from the ECLabel
			MDObjectPtr Ptr = Descriptor->GetLink();
			if(Ptr) Ptr = Ptr->Child(EssenceContainer_UL);
			if(Ptr)
			{
				DataChunkPtr ECLabel = Ptr->PutData();
				if(ECLabel && (ECLabel->Size == 16)) Ret = mxflib::GetWrapType(ECLabel->Data);
				if(Ret != UnknownWrap)
				{
					// Return to the original position and return newly determined wrapping type
					Seek(Pos);
					return Ret;
				}
			}
		}
	}


	// Return to the original position
	Seek(Pos);

	return Ret;
}

