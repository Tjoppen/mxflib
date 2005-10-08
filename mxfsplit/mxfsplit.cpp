/*! \file	mxfsplit.cpp
 *	\brief	Splitter (linear sequential unwrap program) for MXFLib
 *
 *	\version $Id: mxfsplit.cpp,v 1.14 2005/10/08 15:32:30 matt-beard Exp $
 *
 */
/*
 *  Copyright (c) 2003, Metaglue Corporation
 *  Based on "test.cpp"	Copyright (c) 2003, Matt Beard
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
#include <mxflib/waveheader.h>

using namespace mxflib;

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>

using namespace std;

struct StreamFile
{
	FILE* file;
	GCElementKind kind;
};

typedef map<string, StreamFile> FileMap;

//! Debug flag for KLVLib
int Verbose = 0;

//! Debug flag for MXFLib
static bool Quiet = false;				// -q
static bool DebugMode = false;		// -v

static bool SplitIndex = false;		// -i
static bool SplitGC = false;			// -g
static bool SplitWave = false;		// -w
static bool SplitMono = false;		// -m
static bool SplitStereo = false;	// -s
static bool SplitParts = false;		// -p
static bool FullIndex = false;		// -f dump full index
static bool DumpExtraneous = false;		// -x dump extraneous body elements

static unsigned int SplitWaveChannels = 2;	// -w=n

#ifdef DMStiny
static char* DMStinyDict = NULL;						//!< Set to name of DMStiny xmldict
#endif

//! Output Streams
FileMap theStreams;

// not presently used
static void DumpObject(MDObjectPtr Object, std::string Prefix);

static void DumpHeader(PartitionPtr ThisPartition);
static void DumpIndex(PartitionPtr ThisPartition);
static void DumpBody(PartitionPtr ThisPartition);


//! Should we pause before exit?
bool PauseBeforeExit = false;

// Declare main process function
int main_process(int argc, char *argv[]);

//! Do the main processing and pause if required
int main(int argc, char *argv[]) 
{ 
	int Ret = main_process(argc, argv);

	if(PauseBeforeExit) PauseForInput();

	return Ret;
}

//! Do the main processing (less any pause before exit)
int main_process(int argc, char *argv[])
{
	printf("MXFlib File Splitter\n" );

	// Load the dictionaries
	LoadDictionary("dict.xml");

	int num_options = 0;
	for(int i=1; i<argc; i++)
	{
		if(argv[i][0] == '-')
		{
			num_options++;
			char *p = &argv[i][1];					// The option less the '-' or '/'
			char Opt = tolower(*p);					// The option itself (in lower case)
			if(Opt == 'q') Quiet = true;
			else if(Opt == 'v') DebugMode = true;
#ifdef DMStiny
			else if(Opt == 't')
			{
				// DMStiny Dictionary
				if(tolower(*(p+1))=='d')
				{
					char *name="DMStiny.xml"; // default name
					if( '='==*(p+2) )	name=p+3; // explicit name
					DMStinyDict = new char[ 1+strlen(name) ];
					strcpy( DMStinyDict, name );
				}
			}
#endif
			else if(Opt == 'f') FullIndex = true;
			else if(Opt == 'i')	SplitIndex = true;
			else if(Opt == 'g')	SplitGC = true;
			else if(Opt == 'p') SplitParts = true;
			else if(Opt == 'm') SplitMono = true;
			else if(Opt == 's') SplitStereo = true;
			else if(Opt == 'w') 
			{
				SplitWave = true;
				if( argv[i][2]==':' || argv[i][2]=='=' )
				{
					SplitWaveChannels = (unsigned int)strtoul( argv[i]+3, NULL, 0 );
				}
			}
			else if(Opt == 'x') DumpExtraneous = true;
		}
	}

	if((argc-num_options) < 2)
	{
		fprintf( stderr,"\nUsage:  mxfsplit [-qv] <filename> \n" );
		fprintf( stderr,"                       [-q] Quiet (default is Terse) \n" );
		fprintf( stderr,"                       [-v] Verbose (Debug) \n" );
		fprintf( stderr,"                       [-f] Dump Full Index \n" );
		//fprintf( stderr,"                       [-i] Split Index Table Segments \n" );
		//fprintf( stderr,"                       [-g] Split Generic Containers into Elements \n" );
		fprintf( stderr,"                     [-w:n] Split AESBWF Elements into n-channel wave files \n" );
		//fprintf( stderr,"                       [-m] Subdivide AESBWF Elements into mono wave files \n" );
		//fprintf( stderr,"                       [-s] Subdivide AESBWF Elements into stereo wave files \n" );
		//fprintf( stderr,"                       [-p] Split Partitions \n");
		fprintf( stderr,"                       [-x] Dump Extraneous Body Elements \n" );
		fprintf( stderr,"                       [-z] Pause for input before final exit\n");
#ifdef DMStiny
		fprintf( stderr,"                       [-td=filename] Use DMStiny dictionary \n" );
#endif

		return 1;
	}

#ifdef DMStiny
	// load the DMStiny Dictionary
	if( DMStinyDict ) MDOType::LoadDict( DMStinyDict );
#endif

	MXFFilePtr TestFile = new MXFFile;
	if (! TestFile->Open(argv[num_options+1], true))
	{
		perror(argv[num_options+1]);
		return 1;
	}

	// Get a RIP (however possible)
	TestFile->GetRIP();

	RIP::iterator it = TestFile->FileRIP.begin();
	UInt32 iPart = 0;
	while(it != TestFile->FileRIP.end())
	{
		iPart++;

		if( !Quiet ) printf("\nPartition %4d at 0x%s for BodySID 0x%04x\n\n",
													 iPart,
		                       Int64toHexString((*it).second->ByteOffset,8).c_str(),
													 (*it).second->BodySID );

		TestFile->Seek((*it).second->ByteOffset);
		PartitionPtr ThisPartition = TestFile->ReadPartition();
		if(ThisPartition)
		{
			// Dump Partition Pack
			if( !Quiet )
			{
				printf( "Partition Pack:\n" );
				DumpObject(ThisPartition->Object,"");
				printf("\n");
			}

			// Header Metadata
			DumpHeader( ThisPartition );

			// Index Segments
			DumpIndex( ThisPartition );

			// Body Elements
			DumpBody( ThisPartition );
		}
		it++;
	} // while(it != TestFile->FileRIP.end())

	TestFile->Close();

	FileMap::iterator itFile = theStreams.begin();
	while( theStreams.end() != itFile )
	{
		if( SplitWave && 0x16==(*itFile).second.kind.Item )
		{
			// update length fields in wave header
			// how much data?
			long datalen = ftell( (*itFile).second.file ) - sizeof( waveheader_t );

			if( !Quiet ) printf( "Updating wave data length = 0x%lx\n", datalen );

			// get wave header
			waveheader_t wavfmt;
			rewind( (*itFile).second.file );
			fread( (void *)&wavfmt, 1, sizeof(wavfmt), (*itFile).second.file );
			// update it
			wavfmt.SetDataLength( datalen );
			// put it back
			rewind( (*itFile).second.file );
			fwrite( (void *)&wavfmt, 1, sizeof(wavfmt), (*itFile).second.file );
			// go to end (to report correct length)
			fseek( (*itFile).second.file, 0, SEEK_END );
		}

		if( !Quiet ) printf( "Closing %s, size 0x%lx\n", (*itFile).first.c_str(), ftell( (*itFile).second.file ) );
		fclose( (*itFile).second.file );
		itFile++;
	}
	theStreams.clear();

	return 0;
}

// maximum value size to dump
// above this, dump will just state size
#define MAX_DUMPSIZE 128

//! Dump an object and any physical or logical children
void DumpObject(MDObjectPtr Object, std::string Prefix)
{
//	printf("0x%s in %s : ", Int64toHexString(Object->GetLocation(),8).c_str(), Object->GetSource().c_str());

	if(Object->IsModified()) printf("%s%s is *MODIFIED*\n", Object->FullName().c_str(), Prefix.c_str() );

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
		if(Object->IsDValue())
		{
			printf("%s%s = <Unknown>\n", Prefix.c_str(), Object->Name().c_str());
		}
		else
		{
//			const char* n=Object->Name().c_str();
			if(Object->Value)
			{
//				UInt32 sz=Object->Value->GetData().Size;
				if( Object->Value->GetData().Size > MAX_DUMPSIZE )
				{
					printf("%s%s = RAW[0x%08x]", Prefix.c_str(), Object->Name().c_str(), Object->Value->GetData().Size );

					const unsigned char* p = Object->Value->GetData().Data;
					int i; for(i=0;i<3;i++)
					{
						printf("\n%s%*c      ", Prefix.c_str(), strlen(Object->Name().c_str()), ' ');
						int j; for(j=0;j<4;j++)
						{
							int k; for(k=0;k<4;k++) printf("%02x", *p++);
							printf(" ");
						}
						if(i==2) printf( "...\n" );
					}
				}
				else
				printf("%s%s = %s\n", Prefix.c_str(), Object->Name().c_str(), Object->GetString().c_str());
			}
			else
				printf("%s%s\n", Prefix.c_str(), Object->Name().c_str());
		}

		MDObjectULList::iterator it = Object->begin();
		while(it != Object->end())
		{
			DumpObject((*it).second, Prefix + "  ");
			it++;
		}
	}
	return;
}


static void DumpHeader( PartitionPtr ThisPartition )
{
	if(ThisPartition->ReadMetadata() == 0)
	{
		if( !Quiet ) printf("No Header Metadata in this Partition\n\n");
	}
	else
	{
		if( !Quiet ) 
		{
			printf("\nHeader Metadata:\n");
			
			MDObjectList::iterator it2 = ThisPartition->TopLevelMetadata.begin();
			while(it2 != ThisPartition->TopLevelMetadata.end())
			{
				DumpObject(*it2,"  ");
				it2++;
			}
			printf("\n");
		}
	}
	return;
}


static void DumpIndex( PartitionPtr ThisPartition )
{
	// Read any index table segments!
	MDObjectListPtr Segments = ThisPartition->ReadIndex();
	if(Segments->empty())
	{
		if( !Quiet ) printf("No Index Table in this Partition\n\n");
	}
	else if( !DebugMode )
	{

		printf( "\nIndexTable:\n" );

		IndexTablePtr Table = new IndexTable;

		MDObjectList::iterator it = Segments->begin();

		while(it != Segments->end())
		{
			Table->AddSegment(*it);
		
			// Demonstrate this new segment
			
			UInt32 Streams = 1;
			MDObjectPtr DeltaEntryArray = (*it)[DeltaEntryArray_UL];
			if(DeltaEntryArray && DeltaEntryArray->GetType()->size())
			{
				Streams = DeltaEntryArray->size() / DeltaEntryArray->GetType()->size();
				if(Streams == 0) Streams = 1;	// Fix for bad DeltaEntryArray
			}

			Position Start = (*it)->GetInt64(IndexStartPosition_UL);
			Length Duration = (*it)->GetInt64(IndexDuration_UL);
			
			UInt32 IndexSID = (*it)->GetUInt(IndexSID_UL);
			UInt32 BodySID = (*it)->GetUInt(BodySID_UL);
			
			if(Duration == 0) printf("CBR Index Table Segment (covering whole Essence Container) :\n");
			else printf("\nIndex Table Segment (first edit unit = %s, duration = %s) :\n", Int64toString(Start).c_str(), Int64toString(Duration).c_str());

			printf("  Indexing BodySID 0x%04x from IndexSID 0x%04x\n", BodySID, IndexSID);

			if(Duration < 1) Duration = 6;		// Could be CBR
			if(!FullIndex && Duration > 35) Duration = 35;	// Don't go mad!

			int i;
			printf( "\n Bytestream Order:\n" );
			for(i=0; i<Duration; i++)
			{
				UInt32 j;
				for(j=0; j<Streams; j++)
				{
					IndexPosPtr Pos = Table->Lookup(Start + i,j,false);
					printf("  EditUnit %3s for stream %d is at 0x%s", Int64toString(Start + i).c_str(), j, Int64toHexString(Pos->Location,8).c_str());
					printf(", Flags=%02x", Pos->Flags);
					if(Pos->Exact) printf("  *Exact*\n"); else printf("\n");
				}
			}

			printf( "\n Presentation Order:\n" );
			for(i=0; i<Duration; i++)
			{
				UInt32 j;
				for(j=0; j<Streams; j++)
				{
					IndexPosPtr Pos = Table->Lookup(Start + i,j);
					printf("  EditUnit %3s for stream %d is at 0x%s", Int64toString(Start + i).c_str(), j, Int64toHexString(Pos->Location,8).c_str());
					printf(", Flags=%02x", Pos->Flags);
					if( Pos->KeyFrameOffset ) printf(", Keyframe is at 0x%s", Int64toHexString(Pos->KeyLocation,8).c_str() );

					if(Pos->Exact) printf("  *Exact*\n");
					else if(Pos->OtherPos) printf(" (Location of un-reordered position %s)\n", Int64toString(Pos->ThisPos).c_str());
					else printf("\n");
				}
			}
			it++;
		}
	}
	return;
} // DumpIndex


static void DumpBody( PartitionPtr ThisPartition )
{
	UInt32 BodySID = ThisPartition->GetUInt( "BodySID" );

	if( 0==BodySID )
	{
		if( !Quiet ) printf( "No Body in this Partition\n\n" );
	}
	else
	{
		if( !Quiet ) printf( "Elements for BodySID 0x%04x\n", BodySID );

		char filename[40] = "_12345678-Giiccttnn-Mcc-Ppppp.Stream";

		FileMap::iterator itFile;
		FILE* fp;

		int limit=0;

		KLVObjectPtr anElement;
		ThisPartition->StartElements();
		while( anElement = ThisPartition->NextElement() )
		{
			// KLVFill is skipped already

			GCElementKind kind = anElement->GetGCElementKind();

			if( !kind.IsValid )
			{
				if( !Quiet ) printf( "EXTRANEOUS (non-GC) Element: K=%s L=0x%s\n", 
															anElement->GetUL()->GetString().c_str(),
															Int64toHexString( anElement->GetLength(), 8 ).c_str() );
				if( DumpExtraneous )
				{
					// anElement isa KLVObject
					MDObjectPtr anObj = new MDObject( anElement->GetUL() );

					// this may take a long time if we only want to report the size of a mystery KLV
					anElement->ReadData();

					DataChunk& theChunk = anElement->GetData();
					anObj->ReadValue( theChunk );

					DumpObject( anObj, "  " );
					printf( "\n" );

				}

				if( ++limit >= 35 )
				{
					printf( "Excessive Extraneous Elements in this Partition...skipping the rest\n" );
					break;
				}
			}
			else
			{
				// DRAGONS: wimpos sprintf not ISO 
				sprintf(	filename, "_%04x-G%02x%02x%02x%02x.Stream", 
									BodySID,
									kind.Item,
									kind.Count,
									kind.ElementType,
									kind.Number );

				if( !Quiet ) printf( "GC Element: L=0x%s File=%s", 
															Int64toHexString( anElement->GetLength(), 8 ).c_str(),
															filename );

				itFile = theStreams.find( filename );
				if( theStreams.end() == itFile )
				{
					if( !Quiet ) printf( " NEW" );

                    // @modif 20/01/2004 | replaced "wb" by "w+b" | marcvdb@users.sourceforge.net
                    // This prevented the reloading of the files and thus always wrote the 
                    // the default wave header
					fp = fopen( filename, "w+b" );
					if( !fp ) if( !Quiet ) printf( " ERROR");

					StreamFile sf;
					sf.file = fp; sf.kind = kind;
					theStreams.insert( FileMap::value_type(filename, sf) );

					// if( -w && GCSound item) add an empty waveheader
					// DRAGONS: if a user says -w, we assume S382M
					if( SplitWave && 0x16==kind.Item )
					{
						if( !Quiet ) printf( " Wave" );
						// assume 48000, 24 bit at this stage
						waveheader_t wavfmt( SplitWaveChannels );
						fwrite( (void *)&wavfmt, 1, sizeof(wavfmt), fp );
					}

				}
				itFile = theStreams.find( filename );

				if( !Quiet ) printf( "\n" );

				// Read entire essence KLV
				// DRAGONS: This is likely to be messy with large KLVs such as clip-wrapped essence!
				anElement->ReadData();
				DataChunk &theEss = anElement->GetData();

				//diagnostics
				// printf( "  writing %x bytes to %s\n", theEss.Size, (*itFile).first.c_str() );

				fwrite( theEss.Data, 1, theEss.Size, (*itFile).second.file );

			}
		}
	} // if( 0==BodySID )
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
#endif // MXFLIB_DEBUG

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

