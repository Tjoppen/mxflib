/*! \file	mxfsplit.cpp
 *	\brief	Splitter (linear sequential unwrap program) for MXFLib
 *
 *	\version $Id: mxfsplit.cpp,v 1.22 2008/08/20 13:02:42 matt-beard Exp $
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

#include "mxflib/mxflib.h"

using namespace mxflib;

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>

using namespace std;

struct StreamFile
{
	FileHandle file;
	GCElementKind kind;
	EssenceSinkPtr Sink;
};

typedef map<string, StreamFile> FileMap;

//! Debug flag for KLVLib
int Verbose = 0;


// DRAGONS: static file-scope is deprecated
// TODO: remove static and replace with empty namespace

//! Debug flag for MXFLib
static bool Quiet = false;			// -q
static bool DebugMode = false;		// -v

static bool DumpAllHeader = false;	// -a
static bool SplitIndex = false;		// -i
static bool SplitGC = false;		// -g
static bool SplitWave = false;		// -w
static bool SplitMono = false;		// -m
static bool SplitStereo = false;	// -s
static bool SplitParts = false;		// -p
static bool FullIndex = false;		// -f dump full index
static bool DumpExtraneous = false;		// -x dump extraneous body elements

//static unsigned int SplitWaveChannels = 2;	// -w=n

//! Output Streams
FileMap theStreams;

//! DM Dictionaries
typedef list<std::string> DMFileList;
DMFileList DMDicts;

// not presently used
static void DumpObject(MDObjectPtr Object, std::string Prefix);

static void DumpHeader(PartitionPtr ThisPartition);
static void DumpIndex(PartitionPtr ThisPartition);
static void DumpBody(PartitionPtr ThisPartition);
static void WriteWaveHeader(FileHandle File, Int16 Channels, UInt32 SamplesPerSec, UInt16 BitsPerSample, UInt32 DataSize = 0 );
static bool UpdateWaveLengths(FileHandle File);


namespace
{
	//! Structure holding information about the essence in each body stream
	struct EssenceInfo
	{
		UMIDPtr PackageID;
		PackagePtr Package;
		MDObjectPtr Descriptor;
	};
	//! Map of EssenceInfo structures indexed by BodySID
	typedef std::map<UInt32, EssenceInfo> EssenceInfoMap;

	//! The map of essence info for this file
	EssenceInfoMap EssenceLookup;
};

//! Build an EssenceInfoMap for the essence in a given file
/*! \return True if al OK, else false
 */
bool BuildEssenceInfo(MXFFilePtr &File, EssenceInfoMap &EssenceLookup);

namespace mxflib
{
	//! EssenceSink that writes a raw file to the currently open file
	class RawFileSink : public EssenceSink
	{
	protected:
		FileHandle File;						//!< The file to write
		bool EndCalled;							//!< True once EndOfData is called

	private:
		// Prevent default construction
		RawFileSink();

	public:
		// Construct with required header values
		RawFileSink(FileHandle File) : File(File) 
		{
			EndCalled = false;
		};

		//! Clean up
		virtual ~RawFileSink() 
		{
			if(!EndCalled) EndOfData();
		};

		//! Receive the next "installment" of essence data
		/*! This will recieve a buffer containing thhe next bytes of essence data
		 *  \param Buffer The data buffer
		 *  \param BufferSize The number of bytes in the data buffer
		 *  \param EndOfItem This buffer is the last in this wrapping item
		 *  \return True if all is OK, else false
		 *  \note The first call may well fail if the sink has not been fully configured.
		 *	\note If false is returned the caller should make no more calls to this function, but the function should be implemented such that it is safe to do so
		 */
		virtual bool PutEssenceData(UInt8 *const Buffer, size_t BufferSize, bool EndOfItem = true)
		{
			// Write the buffer, returning true if all the bytes were written
			return BufferSize == FileWrite(File, Buffer, BufferSize);
		}

		//! Called once all data exhausted
		/*! \return true if all is OK, else false
		 *  \note This function must also be called from the derived class' destructor in case it is never explicitly called
		 */
		virtual bool EndOfData(void) { return true; }
	};


	//! EssenceSink that writes a wave file to the currently open file
	class WaveFileSink : public EssenceSink
	{
	protected:
		FileHandle File;						//!< The file to write
		unsigned int ChannelCount;				//!< The number of audio channels
        UInt32 SamplesPerSec;					//!< The sample rate in smaples per second
        unsigned int BitsPerSample;				//!< The number of bits per sample, per channel
		UInt32 DataSize;						//!< The size of the entire data chunk of the finished wave file (if known), else 0
		bool HeaderWritten;						//!< Set true once the wave header has been written
		bool EndCalled;							//!< True once EndOfData is called

	private:
		// Prevent default construction
		WaveFileSink();

	public:
		// Construct with required header values
		WaveFileSink(FileHandle File, unsigned int Channels, UInt32 SamplesPerSec, unsigned int BitsPerSample, UInt32 DataSize = 0)
					: File(File), ChannelCount(Channels), SamplesPerSec(SamplesPerSec), BitsPerSample(BitsPerSample), DataSize(DataSize)
		{
			HeaderWritten = false;
			EndCalled = false;
		};

		//! Clean up
		virtual ~WaveFileSink()
		{
			if(!EndCalled) EndOfData();
		}

		//! Receive the next "installment" of essence data
		/*! This will recieve a buffer containing thhe next bytes of essence data
		 *  \param Buffer The data buffer
		 *  \param BufferSize The number of bytes in the data buffer
		 *  \param EndOfItem This buffer is the last in this wrapping item
		 *  \return True if all is OK, else false
		 *  \note The first call may well fail if the sink has not been fully configured.
		 *	\note If false is returned the caller should make no more calls to this function, but the function should be implemented such that it is safe to do so
		 */
		virtual bool PutEssenceData(UInt8 *const Buffer, size_t BufferSize, bool EndOfItem = true);

		//! Called once all data exhausted
		/*! \return true if all is OK, else false
		 *  \note This function must also be called from the derived class' destructor in case it is never explicitly called
		 */
		virtual bool EndOfData(void);
	};
}


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
			else if(Opt == 'd')
			{
				// DM Dictionary
				if(tolower(*(p+1))=='d')
				{
					const char *name=""; // default name
					if( '='==*(p+2) || ':'==*(p+2))	name=p+3; // explicit name
					else if( i+1<argc ) name=argv[++i]; // explicit name in next arg

					if( strlen(name) ) 
					{
						DMDicts.push_back( std::string(name) );
					}
				}
			}
			else if(Opt == 'f') FullIndex = true;
			else if(Opt == 'i')	SplitIndex = true;
			else if(Opt == 'g')	SplitGC = true;
			else if(Opt == 'p') SplitParts = true;
			else if(Opt == 'a') DumpAllHeader = true;
			else if(Opt == 'm') SplitMono = true;
			else if(Opt == 's') SplitStereo = true;
			else if(Opt == 'w') 
			{
//				int sub = 2;
				{
					SplitWave = true;
				}
//				if( argv[i][sub]==':' || argv[i][sub]=='=' )
//				{
//					SplitWaveChannels = (unsigned int)strtoul( argv[i]+3, NULL, 0 );
//				}
			}
			else if(Opt == 'x') DumpExtraneous = true;
		}
	}

	if((argc-num_options) < 2)
	{
		fprintf( stderr,"\nUsage:  mxfsplit [options] <filename> \n" );
		fprintf( stderr,"                       [-q] Quiet (default is Terse) \n" );
		fprintf( stderr,"                       [-v] Verbose (Debug) \n" );
		fprintf( stderr,"                       [-a] Dump all header metadata (and start of index)\n" );
		fprintf( stderr,"                       [-f] Dump Full Index \n" );
		fprintf( stderr,"                                    (where pattern is the filename pattern)\n");
		//fprintf( stderr,"                       [-i] Split Index Table Segments \n" );
		//fprintf( stderr,"                       [-g] Split Generic Containers into Elements \n" );
//		fprintf( stderr,"                     [-w:n] Split AESBWF Elements into n-channel wave files \n" );
		fprintf( stderr,"                       [-w] Split AESBWF audio elements into wave files \n" );
		//fprintf( stderr,"                       [-m] Subdivide AESBWF Elements into mono wave files \n" );
		//fprintf( stderr,"                       [-s] Subdivide AESBWF Elements into stereo wave files \n" );
		//fprintf( stderr,"                       [-p] Split Partitions \n");
		fprintf( stderr,"                       [-x] Dump Extraneous Body Elements \n" );
		fprintf( stderr,"                       [-z] Pause for input before final exit\n");
		fprintf( stderr,"             [-dd=filename] Use DM dictionary \n" );

		return 1;
	}

	// load any DM Dictionaries
	DMFileList::iterator dd_it = DMDicts.begin();
	while( dd_it != DMDicts.end() )
	{
		MDOType::LoadDict( (*dd_it).c_str() );
		dd_it++;
	}

	MXFFilePtr TestFile = new MXFFile;
	if (! TestFile->Open(argv[num_options+1], true))
	{
		perror(argv[num_options+1]);
		return 1;
	}

	// Get a RIP (however possible)
	TestFile->GetRIP();

	BuildEssenceInfo(TestFile, EssenceLookup);

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
			if(DumpAllHeader)
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
			}
			
			// Body Elements
			DumpBody( ThisPartition );
		}
		it++;
	} // while(it != TestFile->FileRIP.end())

	TestFile->Close();

	FileMap::iterator itFile = theStreams.begin();
	while( theStreams.end() != itFile )
	{
/*			if( SplitWave && 0x16==(*itFile).second.kind.Item )
		{
			UpdateWaveLengths((*itFile).second.file);
		}
*/
		if( !Quiet ) printf( "Closing %s, size 0x%s\n", (*itFile).first.c_str(), Int64toHexString(FileTell( (*itFile).second.file )).c_str() );

		if((*itFile).second.Sink) (*itFile).second.Sink->EndOfData();
		FileClose( (*itFile).second.file );

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
					if(sizeof(size_t) == 4)
                        printf("%s%s = RAW[0x%08x]", Prefix.c_str(), Object->Name().c_str(), (int)Object->Value->GetData().Size );
					else
						printf("%s%s = RAW[0x%s]", Prefix.c_str(), Object->Name().c_str(), Int64toHexString(Object->Value->GetData().Size, 8).c_str() );

					const unsigned char* p = Object->Value->GetData().Data;
					int i; for(i=0;i<3;i++)
					{
						printf("\n%s%*c      ", Prefix.c_str(), (int)strlen(Object->Name().c_str()), ' ');
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
				Streams = static_cast<Uint32>(DeltaEntryArray->size() / DeltaEntryArray->GetType()->size());
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
	UInt32 BodySID = ThisPartition->GetUInt( BodySID_UL );

	if( 0==BodySID )
	{
		if( !Quiet ) printf( "No Body in this Partition\n\n" );
	}
	else
	{
		if( !Quiet ) printf( "Elements for BodySID 0x%04x\n", BodySID );

		char filename[40] = "_12345678-Giiccttnn-Mcc-Ppppp.Stream";

		FileMap::iterator itFile;

		int limit=0;

		KLVObjectPtr anElement;
		ThisPartition->StartElements();
		while( anElement = ThisPartition->NextElement() )
		{
			// KLVFill is skipped already

			GCElementKind kind = anElement->GetGCElementKind();

			if( !kind.IsValid )
			{
				if(anElement->IsGCSystemItem())
				{
					if( !Quiet ) printf( "GC System: L=0x%s\n", Int64toHexString( anElement->GetLength(), 8 ).c_str()); 
				}
				else
				{
					if(limit < 35)
					{
						if( !Quiet ) printf( "EXTRANEOUS (non-GC) Element: K=%s L=0x%s\n", 
																	anElement->GetUL()->GetString().c_str(),
																	Int64toHexString( anElement->GetLength(), 8 ).c_str() );
						if( DumpExtraneous )
						{
							// anElement isa KLVObject
							//IDB the kludge with tmpUL ois to get it to compile with GCC>3.4.0
							// see http://www.gnu.org/software/gcc/gcc-3.4/changes.html
							ULPtr tmpUL=anElement->GetUL();
							MDObjectPtr anObj = new MDObject( tmpUL );

							// this may take a long time if we only want to report the size of a mystery KLV
							anElement->ReadData();

							DataChunk& theChunk = anElement->GetData();
							anObj->ReadValue( theChunk );

							DumpObject( anObj, "  " );
							printf( "\n" );

						}

						if( ++limit >= 35 )
						{
							printf( "Excessive Extraneous Elements in this Partition...skipping reporting the rest\n" );
						}
					}
				}
			}
			else
			{
				// The current file
				FileHandle ThisFile;
				EssenceSinkPtr ThisSink;

				// DRAGONS: wimpos sprintf not ISO 
				// DRAGONS: In what way??
				sprintf(	filename, "_%04x-G%02x%02x%02x%02x.Stream", 
									BodySID,
									kind.Item,
									kind.Count,
									kind.ElementType,
									kind.Number );

				if( !Quiet )
				{
					printf( "GC Element: L=0x%s", Int64toHexString( anElement->GetLength(), 8 ).c_str());
					printf(" File=%s",	filename );
				}

				bool StreamFound = false;
				itFile = theStreams.find( filename );
				if(itFile != theStreams.end()) 
				{
					ThisFile = (*itFile).second.file;
					ThisSink = (*itFile).second.Sink;
					StreamFound = true;
				}

				if( !StreamFound )
				{
					if(!Quiet) printf( " NEW" );

					// Open the file
					ThisFile = FileOpenNew(filename);

					if( !FileValid(ThisFile) ) if( !Quiet ) printf( " ERROR");

					if( FileValid(ThisFile) )
					{
						TrackPtr Track;					//!< Pointer to the top-level source package for this stream
						MDObjectPtr Descriptor;			//!< Pointer to the file descriptor for this stream

						EssenceInfoMap::iterator it = EssenceLookup.find(BodySID);
						if(it == EssenceLookup.end())
						{
							warning("BodySID %d not listed in header metadata\n", BodySID);

							// Add a dummy entry so we don't keep getting the same error
							EssenceInfo Dummy;
							EssenceLookup[BodySID] = Dummy;
						}
						else
						{
							if((*it).second.Package)
							{
								UInt32 TrackNumber = anElement->GetGCTrackNumber();
								int TrackPos = 0;			//!< The entry number in the tracks array, and possibly the descriptor list, of this track
								
								TrackList::iterator Track_it = (*it).second.Package->Tracks.begin();
								while(Track_it != (*it).second.Package->Tracks.end())
								{
									if((*Track_it)->GetUInt(TrackNumber_UL) == TrackNumber)
									{
										Track = (*Track_it);
										break;
									}

									// DRAGONS: We don't count timecode tracks as we assume that these don't have descriptors
									if(!(*Track_it)->IsTimecodeTrack()) TrackPos++;
									Track_it++;
								}

								if(!Track)
								{
									warning("Track Number 0x%08x for BodySID %d, not listed in header metadata\n", TrackNumber, BodySID);
									// TODO: Should we add something to stop a repeat of this error?
								}
								else
								{
									bool HasTrackID = false;
									UInt32 TrackID = 0;

									MDObjectPtr TrackIDObject = Track[TrackID_UL];
									if(TrackIDObject)
									{
										HasTrackID = true;
										TrackID = TrackIDObject->GetUInt();
									}

									if((*it).second.Descriptor)
									{
										// DRAGONS: If we don't have a multi-descriptor then this descriptor must describe anything we have
										Descriptor = (*it).second.Descriptor;

										if(Descriptor->IsA(MultipleDescriptor_UL))
										{
											MDObjectPtr DescriptorList = Descriptor[SubDescriptorUIDs_UL];
											if(DescriptorList)
											{
												int DescriptorPos = 0;

												MDObject::iterator it = DescriptorList->begin();
												while(it != DescriptorList->end())
												{
													MDObjectPtr SubDescriptor = (*it).second->GetLink();
													if(SubDescriptor)
													{
														if(!HasTrackID)
														{
															// Track has no TrackID parameter, fall-back to position linking
															if(DescriptorPos == TrackPos)
															{
																Descriptor = SubDescriptor;
																break;
															}
														}
														else
														{
															MDObjectPtr LinkedTrackIDObject = SubDescriptor->Child(LinkedTrackID_UL);
															if(LinkedTrackIDObject)
															{
																UInt32 LinkedTrackID = LinkedTrackIDObject->GetUInt();

																if(LinkedTrackID == TrackID)
																{
																	Descriptor = SubDescriptor;
																	break;
																}
															}
															else
															{
																// Descriptor has no LinkedTrackID parameter, fall-back to position linking
																if(DescriptorPos == TrackPos)
																{
																	Descriptor = SubDescriptor;
																	break;
																}
															}
														}
													}

													DescriptorPos++;
													it++;
												}
											}
										}
									}
								}
							}
						}

						if(!Descriptor)
						{
							// If we couldn't find a descriptor we can't get fancy with unwrapping the essence
							ThisSink = new RawFileSink(ThisFile);
						}
						else
						{
							if(SplitWave && Track && (Track->GetType() == Track::TypeSound) && (Descriptor->IsA(GenericSoundEssenceDescriptor_UL)))
							{
								UInt32 ChannelCount = Descriptor->GetUInt(ChannelCount_UL);
								UInt32 QuantizationBits = Descriptor->GetUInt(QuantizationBits_UL);
								
								// Assume 48k if we have problems!
								Int32 AudioSamplingRate = 48000;

								MDObjectPtr SamplingRate = Descriptor[AudioSamplingRate_UL];
								if(SamplingRate)
								{
									AudioSamplingRate = SamplingRate->GetInt("Numerator");
									Int32 Denom = SamplingRate->GetInt("Denominator");
									if(Denom != 0) AudioSamplingRate /= Denom;
								}

								// FIXME: Take into account "SplitWaveChannels"
								ThisSink = new WaveFileSink(ThisFile, (int)ChannelCount, AudioSamplingRate, (int)QuantizationBits);

								if( !Quiet ) printf( " Wave" );
							}
							else
							{
								ThisSink = new RawFileSink(ThisFile);
							}
						}

						StreamFile sf;
						sf.file = ThisFile; 
						sf.kind = kind;
						sf.Sink = ThisSink;
						theStreams.insert( FileMap::value_type(filename, sf) );
					}

/*					// if( -w && GCSound item) add an empty waveheader
					// DRAGONS: if a user says -w, we assume S382M
					if( FileValid(ThisFile) && SplitWave && 0x16==kind.Item )
					{
						if( !Quiet ) printf( " Wave" );
						// assume 48000, 24 bit at this stage
						WriteWaveHeader(ThisFile, SplitWaveChannels, 48000, 24);
					}
*/
				}

				if( !Quiet ) printf( "\n" );


				/* Copy the essence KLV to the output file in managable chunks */
	
				// Limit chunk size to 32Mb
				const Length MaxSize = 32 * 1024 * 1024;

				Position Offset = 0;
				for(;;)
				{
					// Work out the chunk-size
					Length CurrentSize = anElement->GetLength() - (Length)Offset;
					if(CurrentSize <= 0) break;
					if(CurrentSize > MaxSize) CurrentSize = MaxSize;

					size_t Bytes = anElement->ReadDataFrom(Offset, static_cast<size_t>(CurrentSize));
					if(!Bytes) break;
					Offset += Bytes;

					//if(FileValid(ThisFile)) FileWrite(ThisFile, anElement->GetData().Data, anElement->GetData().Size);
					// FIXME: Need to add end-of-element
					if(ThisSink) ThisSink->PutEssenceData(anElement->GetData());
				}
			}
		}
	} // if( 0==BodySID )
	return;
}


//! Write a basic wave fle header
void WriteWaveHeader(FileHandle File, Int16 Channels, UInt32 SamplesPerSec, UInt16 BitsPerSample, UInt32 DataSize /*=0*/)
{
	const UInt32 ID_RIFF = 0x52494646;		//! "RIFF"
	const UInt32 ID_WAVE = 0x57415645;		//! "WAVE"
	const UInt32 ID_fmt  = 0x666d7420;		//! "fmt "
	const UInt32 ID_data = 0x64617461;		//! "data"

	//! Buffer big enough to hold a basic Wave Header
	UInt8 Buffer[44];

	/*  The format written is as follows:

		fourcc		fRIFF;				// 0
		LEUInt32	RIFF_len;			// 4
		fourcc		fWAVE;				// 8

		fourcc		ffmt_;				// 12
		LEUInt32	fmt__len;			// 16

		LEUInt16	format;				// 20
		LEUInt16	nchannels;			// 22
		LEUInt32	samplespersec;		// 24
		LEUInt32	avgbps;				// 28
		LEUInt16	blockalign;			// 32
		LEUInt16	bitspersample;		// 34

		fourcc		data;				// 36
		LEUInt32	data_len;			// 40
										// 44
	*/

	//! Walking buffer pointer
	UInt8 *p = Buffer;

	// Write the initial RIFF FourCC
	PutU32(ID_RIFF, p);
	p+= 4;

	// Write the length of the file with only the header (excluding the first 8 bytes)
	PutU32_LE(38 + DataSize, p);
	p+= 4;

	// Write the WAVE FourCC
	PutU32(ID_WAVE, p);
	p+= 4;

	// Write the fmt_ FourCC
	PutU32(ID_fmt, p);
	p+= 4;

	// And the length of the fmt_ chunk
	PutU32_LE(16, p);
	p+= 4;

	/* Write the format chunk */

	// AudioFormat = PCM
	PutU16_LE(1, p);
	p += 2;

	// NumChannels
	PutU16_LE(Channels, p);
	p += 2;

	// SamplesRate
	PutU32_LE(SamplesPerSec, p);
	p += 4;

	// ByteRate
	PutU32_LE((SamplesPerSec * Channels * BitsPerSample)/8, p);
	p += 4;

	// BlockAlign
	PutU16_LE((Channels * BitsPerSample)/8, p);
	p += 2;

	// BitsPerSample
	PutU16_LE(BitsPerSample, p);
	p += 2;


	/* Write the data header */
	
	// Write the data FourCC
	PutU32(ID_data, p);
	p+= 4;

	// Write the length of the data
	PutU32_LE(DataSize, p);


	// Write this data to the file
	FileWrite(File, Buffer, 44);
}


//! Update the lengths in the header of the specified wave file
/*! \return true if updated OK */
bool UpdateWaveLengths(FileHandle File)
{
	const UInt32 ID_RIFF = 0x52494646;		//! "RIFF"
	const UInt32 ID_WAVE = 0x57415645;		//! "WAVE"
	const UInt32 ID_fmt  = 0x666d7420;		//! "fmt "
	const UInt32 ID_data = 0x64617461;		//! "data"

	//! Buffer for working values
	UInt8 Buffer[20];

	// Determine the size of the file (Note it can not be > 4Gb)
	FileSeekEnd(File);
	UInt32 FileSize = (UInt32)FileTell(File);

	// Read the start of the header
	FileSeek(File, 0);
	if(FileRead(File, Buffer, 20) != 20) return false;

	// Check the initial RIFF FourCC
	if(GetU32(Buffer) != ID_RIFF) return false;

	// Check the WAVE FourCC
	if(GetU32(&Buffer[8]) != ID_WAVE) return false;

	// Check the fmt_ FourCC
	if(GetU32(&Buffer[12]) != ID_fmt) return false;

	// Get the length of the format chunk
	UInt32 FormatLength = GetU32_LE(&Buffer[16]);

	// Read the following chunk
	FileSeek(File, FormatLength + 20);
	if(FileRead(File, Buffer, 4) != 4) return false;

	// Check the data FourCC (doesn't have to be here for a valid wave file, but it's all we support!)
	if(GetU32(Buffer) != ID_data) return false;

	// Write the file length (less the first 8 bytes)
	PutU32_LE(FileSize - 8, Buffer);
	FileSeek(File, 4);
	FileWrite(File, Buffer, 4);

	// Write the file length (less the first 28 bytes)
	PutU32_LE(FileSize - (FormatLength + 28), Buffer);
	FileSeek(File, FormatLength + 24);
	FileWrite(File, Buffer, 4);

	return true;
}


//! Receive the next "installment" of essence data
/*! This will recieve a buffer containing thhe next bytes of essence data
 *  \param Buffer The data buffer
 *  \param BufferSize The number of bytes in the data buffer
 *  \param EndOfItem This buffer is the last in this wrapping item
 *  \return True if all is OK, else false
 *  \note The first call may well fail if the sink has not been fully configured.
 *	\note If false is returned the caller should make no more calls to this function, but the function should be implemented such that it is safe to do so
 */
bool WaveFileSink::PutEssenceData(UInt8 *const Buffer, size_t BufferSize, bool EndOfItem /*=true*/)
{
	if(!HeaderWritten)
	{
		WriteWaveHeader(File, static_cast<Int16>(ChannelCount), SamplesPerSec, static_cast<Int16>(BitsPerSample), DataSize);
		HeaderWritten = true;
	}

	// Write the buffer, returning true if all the bytes were written
	return BufferSize == FileWrite(File, Buffer, BufferSize);
}

//! Called once all data exhausted
/*! \return true if all is OK, else false
	*/
bool WaveFileSink::EndOfData(void)
{
	bool Ret = true;

	if(DataSize == 0)
	{
		// Update the length fields if required
		Ret = UpdateWaveLengths(File);
	}

	EndCalled = true;

	return Ret;
}


//! Build an EssenceInfoMap for the essence in a given file
/*! \return True if al OK, else false
 */
bool BuildEssenceInfo(MXFFilePtr &File, EssenceInfoMap &EssenceLookup)
{
	// Empty any old data
	EssenceLookup.clear();

	// Get the master metadata set (or the header if we must)
	PartitionPtr MasterPartition = File->ReadMasterPartition();
	if(!MasterPartition)
	{
		File->Seek(0);
		MasterPartition = File->ReadPartition();
		warning("File %s does not contain a cloased copy of header metadata - using the open copy in the file header\n", File->Name.c_str());
	}

	if(!MasterPartition) 
	{
		error("Could not read header metadata from file %s\n", File->Name.c_str());
		return false;
	}

	// Read and parse the metadata
	MasterPartition->ReadMetadata();
	MetadataPtr HMeta = MasterPartition->ParseMetadata();
	
	if(!HMeta) 
	{
		error("Could not read header metadata from file %s\n", File->Name.c_str());
		return false;
	}

	/* Scan the Essence container data sets to get PackageID to BodySID mapping */
	MDObjectPtr ECDSet = HMeta[ContentStorage_UL];
	if(ECDSet) ECDSet = ECDSet->GetLink();
	if(ECDSet) ECDSet = ECDSet[EssenceContainerDataBatch_UL];
	if(!ECDSet)
	{
		error("Header metadata in file %s does not contain an EssenceContainerData set\n", File->Name.c_str());
		return false;
	}

	MDObject::iterator it = ECDSet->begin();
	while(it != ECDSet->end())
	{
		MDObjectPtr ThisECDSet = (*it).second->GetLink();
		MDObjectPtr PackageID;
		if(ThisECDSet) PackageID = ThisECDSet->Child(LinkedPackageUID_UL);
		if(PackageID)
		{
			EssenceInfo NewEI;
			NewEI.PackageID = new UMID(PackageID->PutData()->Data);

			// Inset the basic essence info - but not if this is external essence (BodySID == 0)
			UInt32 BodySID = ThisECDSet->GetUInt(BodySID_UL);
			if(BodySID) EssenceLookup[BodySID] = NewEI;
		}
		it++;
	}

	/* Now find the other items for the essence lookup map */
	if(EssenceLookup.size())
	{
		PackageList::iterator it = HMeta->Packages.begin();
		while(it != HMeta->Packages.end())
		{
			// Only Source Packages are of interest
			if((*it)->IsA(SourcePackage_UL))
			{
				MDObjectPtr Descriptor = (*it)->Child(Descriptor_UL);
				if(Descriptor) Descriptor = Descriptor->GetLink();

				if(Descriptor)
				{
					MDObjectPtr PackageID = (*it)->Child(PackageUID_UL);
					if(PackageID)
					{
						UMIDPtr TheID = new UMID(PackageID->PutData()->Data);
						
						/* Now do a lookup in the essence lookup map (it will need to be done the long way here */
						EssenceInfoMap::iterator EL_it = EssenceLookup.begin();
						while(EL_it != EssenceLookup.end())
						{
							if((*((*EL_it).second.PackageID)) == (*TheID))
							{
								// If found, set the missing items and stop searching
								(*EL_it).second.Package = (*it);
								(*EL_it).second.Descriptor = Descriptor;
								break;
							}
							EL_it++;
						}
					}
				}
			}

			it++;
		}
	}

	return true;
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



