/*! \file	test.cpp
 *	\brief	Test program for MXFLib
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

#include "..\mxflib.h"

using namespace mxflib;

#include <stdio.h>
#include <iostream>

using namespace std;


//! Debug flag for KLVLib
extern "C" int Verbose = 0;

//! Debug flag for MXFLib
bool DebugMode = false;

void DumpObject(MDObjectPtr Object, std::string Prefix);


int main(int argc, char *argv[])
{
	printf("Test Program for MXFLib\n");

	if(argc < 2)
	{
		printf("\nUsage:  Test <filename>\n");
		return -1;
	}

	int i;
	for(i=1; i<argc; i++)
	{
		if((argv[i][0] == '/') || (argv[i][0] == '-')) 
			if((argv[i][1] == 'v') || (argv[i][1] == 'V'))
				DebugMode = true;
	}

	LoadTypes("types.xml");
	MDOType::LoadDict("XMLDict.xml");

	MXFFilePtr TestFile = new MXFFile;
	TestFile->Open(argv[1], true);

	// Get a RIP (however possible)
	TestFile->GetRIP();

	RIP::iterator it = TestFile->FileRIP.begin();
	while(it != TestFile->FileRIP.end())
	{
		printf("\nPartition at 0x%s is for BodySID 0x%04x\n", Int64toHexString((*it)->ByteOffset,8).c_str(), (*it)->BodySID);

		TestFile->Seek((*it)->ByteOffset);
		PartitionPtr ThisPartition = TestFile->ReadPartition();
		if(ThisPartition)
		{
			DumpObject(MDObjectPtr(ThisPartition),"");

			if(ThisPartition->ReadMetadata() == 0)
			{
				printf("No header metadata in this partition\n");
			}
			else
			{
				printf("\nHeader Metadata:\n");
				
				MDObjectList::iterator it2 = ThisPartition->TopLevelMetadata.begin();
				while(it2 != ThisPartition->TopLevelMetadata.end())
				{
					DumpObject(*it2,"  ");
					it2++;
				}
			}

			// Read any index table segments!
			MDObjectListPtr Segments = ThisPartition->ReadIndex();
			if(Segments->empty())
			{
				printf("No index table in this partition\n");
			}
			else
			{
				IndexTablePtr Table = new IndexTable;

				MDObjectList::iterator it = Segments->begin();

				while(it != Segments->end())
				{
					Table->AddSegment(*it);
				
					// Demonstrate this new segment
					
					Uint32 Streams = 1;
					MDObjectPtr DeltaEntryArray = (*it)["DeltaEntryArray"];
					if(DeltaEntryArray && DeltaEntryArray->GetType()->size())
					{
						Streams = DeltaEntryArray->size() / DeltaEntryArray->GetType()->size();
						if(Streams == 0) Streams = 1;	// Fix for bad DeltaEntryArray
					}

					Position Start = (*it)->GetInt64("IndexStartPosition");
					Length Duration = (*it)->GetInt64("IndexDuration");
					
					if(Duration == 0) printf("CBR Index Table Segment (covering whole Essense Container) :\n");
					else printf("\nIndex Table Segment (first edit unit = %s, duration = %s) :\n", Int64toString(Start).c_str(), Int64toString(Duration).c_str());

					if(Duration < 1) Duration = 4;		// Could be CBR
					if(Duration > 10) Duration = 10;	// Don't go mad!

					int i;
					for(i=0; i<Duration; i++)
					{
						int j;
						for(j=0; j<Streams; j++)
						{
							IndexPosPtr Pos = Table->Lookup(Start + i,j);
							printf("  EditUnit %d for stream %d is at 0x%s", (int)(Start + i), j, Int64toHexString(Pos->Location,8).c_str());
							printf(", Flags=%02x", Pos->Flags);
							if(Pos->Exact) printf("  *Exact*\n"); else printf("\n");
						}
					}

					it++;
				}
			}
		}

		it++;
	}

	if(TestFile->ReadRIP())
	{
		printf("\nRead RIP\n");
		PartitionInfoList::iterator it = TestFile->FileRIP.begin();
		while(it != TestFile->FileRIP.end())
		{
			printf("  BodySID 0x%04x is at 0x%s", (*it)->BodySID, Int64toHexString((*it)->ByteOffset,8).c_str());

			if((*it)->ThePartition)
				printf(" is a %s\n", (*it)->ThePartition->Name().c_str());
			else
				printf(" is not loaded\n");

			it++;
		}
	}

	if(TestFile->ScanRIP())
	{
		printf("\nScanned RIP\n");
		PartitionInfoList::iterator it = TestFile->FileRIP.begin();
		while(it != TestFile->FileRIP.end())
		{
			printf("  BodySID 0x%04x is at 0x%s", (*it)->BodySID, Int64toHexString((*it)->ByteOffset,8).c_str());

			if((*it)->ThePartition)
				printf(" is a %s\n", (*it)->ThePartition->Name().c_str());
			else
				printf(" is not loaded\n");

			it++;
		}
	}

	if(TestFile->BuildRIP())
	{
		printf("\nBuilt RIP\n");
		PartitionInfoList::iterator it = TestFile->FileRIP.begin();
		while(it != TestFile->FileRIP.end())
		{
			printf("  BodySID 0x%04x is at 0x%s", (*it)->BodySID, Int64toHexString((*it)->ByteOffset,8).c_str());

			if((*it)->ThePartition)
				printf(" is a %s\n", (*it)->ThePartition->Name().c_str());
			else
				printf(" is not loaded\n");

			it++;
		}
	}

	TestFile->Close();

	return 0;
}


//! Dump an object and any physical or logical children
void DumpObject(MDObjectPtr Object, std::string Prefix)
{
//	printf("0x%s in %s : ", Int64toHexString(Object->GetLocation(),8).c_str(), Object->GetSource().c_str());

	if(Object->GetLink())
	{
		if(Object->GetRefType() == DICT_REF_STRONG)
		{
			printf("%s%s = %s\n", Prefix.c_str(), Object->Name().c_str(), Object->GetString().c_str());
			printf("%s%s -> Strong Reference to %s\n", Prefix.c_str(), Object->Name().c_str(), Object->GetLink()->Name().c_str());
			DumpObject(Object->GetLink(), Prefix + "  ");
		}
		else
		{
			printf("%s%s -> Weak Reference to %s\n", Prefix.c_str(), Object->Name().c_str(), Object->GetLink()->Name().c_str());
		}
	}
	else
	{
		if(Object->Value)
			printf("%s%s = %s\n", Prefix.c_str(), Object->Name().c_str(), Object->GetString().c_str());
		else
			printf("%s%s\n", Prefix.c_str(), Object->Name().c_str());

		MDObjectNamedList::iterator it = Object->begin();
		while(it != Object->end())
		{
			DumpObject((*it).second, Prefix + "  ");
			it++;
		}
	}

	return;
}




// Debug and error messages
#include <stdarg.h>

#ifdef MXFLIB_DEBUG
//! Display a general debug message
void mxflib::debug(const char *Fmt, ...)
{
	if(!DebugMode) return;

	va_list args;

	va_start(args, Fmt);
	vprintf(Fmt, args);
	va_end(args);
}
#endif MXFLIB_DEBUG

//! Display a warning message
void mxflib::warning(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("Warning: ");
	vprintf(Fmt, args);
	va_end(args);
}

//! Display an error message
void mxflib::error(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("ERROR: ");
	vprintf(Fmt, args);
	va_end(args);
}

