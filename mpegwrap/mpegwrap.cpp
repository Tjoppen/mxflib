/*! \file	mpegwrap.cpp
 *	\brief	Basic MXF MPEG ES wrapper
 *
 *	\note	Although MPEG streams should have every start_code byte aligned
 *			not all seem to do this.  This code currently has some support
 *			for detecting non-byte aligned start_codes and it can be upgraded
 *			to work with these streams.
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


// Product GUID and version text for this release
Uint8 ProductGUID_Data[16] = { 0x84, 0x64, 0x1a, 0xf5, 0x27, 0xdd, 0xde, 0x40, 0x86, 0xdc, 0xe0, 0x99, 0xda, 0x7f, 0xd0, 0x52 };
std::string ProductVersion = "Release 0.1";

bool ParseCommandLine(int &argc, char **argv);
Int64 ProcessEssence(FileHandle InFile, MXFFilePtr &Out, Int64 PictureLimit = 0);
void WriteMPEG(FileHandle InFile, MXFFilePtr &Out, Int64 PictureStart, int PictureStartBitOffset, Int64 PictureEnd, int PictureEndBitOffset);
MDObjectPtr BuildMPEGEssenceDescriptor(FileHandle InFile);

// Options
Uint32 KAGSize = 1;						//!< The KAG Size for this file
char InFilenameSet[512];				//!< The set of input filenames
int InFileGangSize;						//!< The number of ganged files to process at a time
int InFileGangCount;					//!< The number of sets of ganged files to process
char InFilename[16][128];				//!< The list of input filenames
char OutFilename[128];					//!< The output filename

bool OPAtom = false;					//!< Is OP-Atom mode being forced?
bool StreamMode = false;				//!< Wrap in stream-mode
bool GOPAlign = false;					//!< Start new body partitions only at the start of a GOP

//! The mode of body partition insertion
enum { Body_None, Body_Duration, Body_Size } BodyMode = Body_None;
Uint32 BodyRate = 0;					//!< The rate of body partition insertion

Uint32 HeaderPadding = 0;				//!< The (minimum) number of bytes of padding to leave in the header


// Derived options
ULPtr OPUL;								//!< The UL of the OP for this file


// Operational Pattern Labels
// ==========================

// OP-Atom - #### DRAGONS: Qualifiers need work later!
Uint8 OPAtom_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00 };
ULPtr OPAtomUL = new UL(OPAtom_Data);

// OP1a - #### DRAGONS: Qualifiers may need work!
Uint8 OP1a_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x00 };
ULPtr OP1aUL = new UL(OP1a_Data);

// OP1b - #### DRAGONS: Qualifiers need work!
Uint8 OP1b_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x02, 0x05, 0x00 };
ULPtr OP1bUL = new UL(OP1b_Data);

// OP2a - #### DRAGONS: Qualifiers need work!
Uint8 OP2a_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x02, 0x01, 0x01, 0x00 };
ULPtr OP2aUL = new UL(OP2a_Data);

// OP2b - #### DRAGONS: Qualifiers need work!
Uint8 OP2b_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x02, 0x02, 0x05, 0x00 };
ULPtr OP2bUL = new UL(OP2b_Data);



//IT# // The index table
//IT# IndexTablePtr Table;

int main(int argc, char *argv[])
{
	printf("Simple MXF wrapping of MPEG\n\n");

//IT# 	// Clear the index table
//IT# 	Table = new IndexTable();

	// Parse command line options and exit on error
	if(!ParseCommandLine(argc, argv)) return -1;

	// Open the input file
	FileHandle InFile = FileOpenRead(InFilename[0]);
	if(!FileValid(InFile))
	{
		error("Can't open input file \"%s\"\n", InFilename[0]);
		return -2;
	}

	// Load the dictionaries
	LoadTypes("types.xml");
	MDOType::LoadDict("XMLDict.xml");

	// Create a set of header metadata
	MetadataPtr MData = new Metadata();
	ASSERT(MData);
	ASSERT(MData->Object);

	// Frame wrapped MPEG-ES
	// stream_id range starts at 0xe0 so byte 15 is 0x60
	Uint8 MPEG_VES_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x02, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x04, 0x60, 0x01 };
	ULPtr MPEG_VES = new UL(MPEG_VES_Data);
	MData->AddEssenceType(MPEG_VES);

	// Set the OP label
	// If we are writing OP-Atom we write the header as OP1a initially as another process
	// may try to read the file before it is complete and then it will NOT be a valid OP-Atom file
	if(OPAtom) MData->SetOP(OP1aUL); else MData->SetOP(OPUL); 

	// Build an essence descriptor
	MDObjectPtr EssenceDescriptor = BuildMPEGEssenceDescriptor(InFile);

	// Work out the edit rate from the descriptor
	bool DropFrame = 0;
	std::string EditRate = EssenceDescriptor->GetString("SampleRate");
	Uint32 FrameRate = atoi(EditRate.c_str());

	// If the numerator is large the denominator will be "1001" so we are in drop-frame
	if(FrameRate > 1000)
	{
		FrameRate /= 1000;
		DropFrame = true;
	}

	// Build the Material Package
	PackagePtr MaterialPackage = MData->AddMatarialPackage("Material Package");
	MData->SetPrimaryPackage(MaterialPackage);
	TrackPtr MPTimecodeTrack = MaterialPackage->AddTimecodeTrack(EditRate);
	TimecodeComponentPtr MPTimecodeComponent = MPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, 0);
	TrackPtr MPPictureTrack = MaterialPackage->AddPictureTrack(EditRate);
	SourceClipPtr MPClip = MPPictureTrack->AddSourceClip();

	// Build the File Package
	PackagePtr FilePackage = MData->AddFilePackage(1, "File Package");
	TrackPtr FPTimecodeTrack = FilePackage->AddTimecodeTrack(EditRate);
	TimecodeComponentPtr FPTimecodeComponent = FPTimecodeTrack->AddTimecodeComponent(25, false, 0);
	TrackPtr FPPictureTrack = FilePackage->AddPictureTrack(0x15010500, EditRate);		// Link to MPEG-ES picture stream 0
	SourceClipPtr FPClip = FPPictureTrack->AddSourceClip();

	// Add the file descriptor to the file package
	EssenceDescriptor->SetUint("LinkedTrackID", FPPictureTrack->GetUint("TrackID"));
	EssenceDescriptor->AddChild("EssenceContainer")->ReadValue(MPEG_VES->GetValue(), 16);
	FilePackage->AddChild("Descriptor")->MakeLink(EssenceDescriptor);

	// Link the MP to the FP
	MPClip->MakeLink(FPPictureTrack, 0);

	//
	// ** Write out the header **
	//

	PartitionPtr ThisPartition = new Partition("OpenHeader");
	ASSERT(ThisPartition);
	ThisPartition->SetKAG(KAGSize);			// Everything else can stay at default
	ThisPartition->SetUint("BodySID", 1);

	ThisPartition->AddMetadata(MData);

	// Build an Ident set describing us and link into the metadata
	MDObjectPtr Ident = new MDObject("Identification");
	Ident->SetString("CompanyName", "freeMXF.org");
	Ident->SetString("ProductName", "mpegwrap");
	Ident->SetString("VersionString", ProductVersion);
	UUIDPtr ProductUID = new mxflib::UUID(ProductGUID_Data);

	// DRAGONS: -- Need to set a proper GUID per released version
	//             Non-released versions currently use a random GUID
	//			   as they are not a stable version...
	Ident->SetValue("ProductUID", DataChunk(16,ProductUID->GetValue()));
	Ident->SetValue("ProductUID", DataChunk(16,ProductUID->GetValue()));

	// Link the new Ident set with all new metadata
	MData->UpdateGenerations(Ident);

	// Open the output file
	MXFFilePtr Out = new MXFFile;
	Out->OpenNew(OutFilename);

	// Write the header partition
	Out->WritePartition(ThisPartition, HeaderPadding);

	//
	// ** Process Essence **
	//

	Uint64 Duration = ProcessEssence(InFile, Out);
	printf("\nWrote %d pictures\n", (int)Duration);

	//
	// ** Write a footer (with updated durations) **
	//

	// If we are writing OP-Atom this is the first place we can claim it
	if(OPAtom) MData->SetOP(OPAtomUL);

	MData->SetTime();
	MPTimecodeComponent->SetDuration(Duration);
	MPClip->SetDuration(Duration);
	FPTimecodeComponent->SetDuration(Duration);
	FPClip->SetDuration(Duration);
	EssenceDescriptor->SetInt64("ContainerDuration",Duration);
	
	// Update the generation UIDs in the metadata to reflect the changes
	MData->UpdateGenerations(Ident);

	// Turn the header partition into a footer
	ThisPartition->ChangeType("CompleteFooter");

	// Make sure any new sets are linked in
	ThisPartition->UpdateMetadata(MData);

	// Actually write the footer
	Out->WritePartition(ThisPartition);

	// Add a RIP
	Out->WriteRIP();

	//
	// ** Update the header ** 
	//
	// For generalized OPs update the value of "FooterPartition" in the header pack
	// For OP-Atom re-write the entire header
	//

	Uint64 FooterPos = ThisPartition->GetUint64("FooterPartition");
	Out->Seek(0);
	if(OPAtom)
	{
		ThisPartition->ChangeType("ClosedCompleteHeader");
		ThisPartition->SetUint64("FooterPartition", FooterPos);
		Out->ReWritePartition(ThisPartition);
	}
	else
	{
		ThisPartition = Out->ReadPartition();
		ThisPartition->SetUint64("FooterPartition", FooterPos);
		Out->Seek(0);
		Out->WritePartitionPack(ThisPartition);
	}


	// Close the file - all done!
	Out->Close();

/*IT# 	//int i;
	for(i=0; i<8; i++)
	{
		int j;
		for(j=0; j<3; j++)
		{
			IndexPosPtr Pos = Table->Lookup(0 + i,j);
			printf("  EditUnit %d for stream %d is at 0x%s", (int)(0 + i), j, Int64toHexString(Pos->Location,8).c_str());
			printf(", Flags=%02x", Pos->Flags);
			if(Pos->Exact) printf("  *Exact*\n"); else printf("\n");
		}
	}
#IT#*/

	return 0;
}


//! Parse the command line options
/*!	\return true if all parsed ok, false if an error or none supplied */
bool ParseCommandLine(int &argc, char **argv)
{
	int i;
	for(i=1; i<argc;)
	{
		if((argv[i][0] == '/') || (argv[i][0] == '-'))
		{
			char *p = &argv[i][1];					// The option less the '-' or '/'
			char Opt = tolower(*p);					// The option itself (in lower case)
			char *Val = "";							// Any value attached to the option
			if(strlen(p) > 2) Val = &p[2];			// Only set a value if one found

			if(Opt == 'a') OPAtom = true;
			else if(Opt == 'p')
			{
				// The value is further along as we are using a 2-byte option
				Val++;
				if(tolower(p[1]) == 'd')
				{
					char *temp;
					BodyMode = Body_Duration;
					BodyRate = strtoul(Val, &temp, 0);
				}
				else if(tolower(p[1]) == 's')
				{
					char *temp;
					BodyMode = Body_Size;
					BodyRate = strtoul(Val, &temp, 0);
				}
				else error("Unknown body partition mode '%c'\n", p[1]);
			}
			else if(Opt == 'g') GOPAlign = true;
			else if(Opt == 's') StreamMode = true;
			else if(Opt == 'v') DebugMode = true;
			else if(Opt == 'h') 
			{
				char *temp;
				HeaderPadding = strtoul(Val, &temp, 0);
			}
			else if(Opt == 'k') 
			{
				char *temp;
				KAGSize = strtoul(Val, &temp, 0);
			}
			else 
			{
				error("Unknown command-line option %s\n", argv[i]);
			}

			// Remove this option
			int j;
			for(j=i; j<(argc-1); j++) argv[j] = argv[j+1];
			argc--;
		}
		else 
		{
			// Move on to next option
			i++;
		}
	}

	if(argc < 3)
	{
		printf("Usage:    mpegwrap <inputfiles> <mxffile>\n\n");

		printf("Syntax for input files:\n");
		printf("         a,b = file a followed by file b\n");
		printf("         a+b = file a ganged with file b\n");
		printf("     a+b,c+d = file a ganged with file b\n");
		printf("               followed by file c ganged with file d\n\n");

		printf("Note: There must be the same number of ganged files in each sequential set\n");
		printf("      Also all files in each set must be the same duration\n\n");

		printf("Options:\n");
		printf("    -a        = Force OP-Atom\n");
		printf("    -g        = Only start body partitions at new GOP\n");
		printf("    -h=<size> = Leave at lease <size> bytes of expansion space in the header\n");
		printf("    -k=<size> = Set KAG size (default=1)\n");
		printf("   -pd=<dur>  = Body partition every <dur> frames\n");
		printf("   -ps=<size> = Body partition roughly every <size> bytes\n");
		printf("                (early rather than late)\n");
		printf("    -s        = Interleave essence containers for streaming\n");
		printf("    -v        = Verbose mode\n\n");

		return false;
	}

	InFileGangSize = 1;
	InFileGangCount = 1;

	strncpy(InFilenameSet, argv[1], 510);

	int InCount = 0;
	char *ps = InFilenameSet;
	for(;;)
	{
		char *LastDot = NULL;
		char *pd = InFilename[InCount];

		// Find the position of last dot in the input filename
		while(*ps)
		{
			if(*ps == '.') LastDot = ps;
			if(*ps == ',') { InFileGangCount++; break; }
			if(*ps == '+')
			{
				if(InFileGangCount == 1) InFileGangSize++;
				break;
			}
			*pd++ = *ps++;
		};
		*pd = '\0';
		InCount++;

		// If input filename specified no extension add ".mpg"
		if(LastDot == NULL)	strcpy(pd, ".mpg");

		// If all files progessed end scan
		if(*ps == '\0') break;

		// Otherwise we found '.' or '+' so skip it
		ps++;
	}

	strncpy(OutFilename, argv[2], 120);

	char *LastDot = NULL;
	ps = OutFilename;
	// Find the position of last dot in the output filename
	while(*ps)
	{
		if(*ps == '.') LastDot = ps;
		ps++;
	};

	// If output filename specified no extension add ".mxf"
	if(LastDot == NULL) strcat(OutFilename, ".mxf");

	
	// Detail the options

	debug("** Verbose Mode **\n\n");
	
	printf("KAGSize     = %u\n\n", KAGSize);
	
	if(InFileGangSize == 1)
	{
		if(InFileGangCount == 1) printf("Input file  = %s\n", InFilename[0]);
		else
		{
			printf("Input files = ");
			int i;
			for(i=0; i<InFileGangCount; i++) 
			{ 
				if(i != 0) printf(" then ");
				printf("%s", InFilename[i]);
			}
			printf("\n");
		}
	}
	else
	{
		printf("Input files = ");

		int i;
		for(i=0; i<InFileGangCount; i++) 
		{ 
			if(i != 0) printf(" followed by: ");
			int j;
			for(j=0; j<InFileGangSize; j++) 
			{ 
				if(j != 0) printf(" with ");
				printf("%s", InFilename[i*InFileGangSize + j]);
			}
			printf("\n");
		}
		if(InFileGangCount > 1) printf("\n");
	}

	printf("Output file = %s\n\n", OutFilename);

	if(OPAtom)
	{
		printf("Output OP = OP-Atom\n");
		
		// We will need some extra space in the header
		if(HeaderPadding == 0) HeaderPadding = 16384;

		OPUL = OPAtomUL;

		if((InFileGangCount * InFileGangSize) > 1) error("OP-Atom can only wrap a single file\n");
		
		if(BodyMode != Body_None) 
		{
			warning("Body partitions are forbidden in OP-Atom\n");
			BodyMode = Body_None;
		}

		warning("OP-Atom not yet fully supported\n");
	}
	else
	{
		if(InFileGangSize == 1)
		{
			if(InFileGangCount == 1) { printf("Output OP = OP1a\n"); OPUL = OP1aUL; }
			else { printf("Output OP = OP2a\n"); OPUL = OP2aUL; }
		}
		else
		{
			if(InFileGangCount == 1) { printf("Output OP = OP1b\n"); OPUL = OP1bUL; }
			else { printf("Output OP = OP2b\n"); OPUL = OP2bUL; }
		}

		if((InFileGangCount * InFileGangSize) > 1) error("Only OP1a currently supported\n");
	}

	if(StreamMode && (InFileGangSize == 1))
	{
		warning("Essence containers will not be interleaved for streaming as none are ganged\n");
		StreamMode = false;
	}

	if(StreamMode) 
	{
		printf("Essence containers will be interleaved for streaming\n");

		if(BodyMode != Body_None) 
		{
			warning("Body partitions will be inserted for interleaving - this overrides other body partitioning options\n");
			BodyMode = Body_None;
		}
		
		error("Stream mode not yet supported\n");
	}
	else
	{
		if(BodyMode == Body_Duration) 
		{
			if(GOPAlign)
				printf("A new body partition will be inserted at the first new GOP after each %d frame%s\n", BodyRate, BodyRate==1 ? "" : "s");
			else
				printf("A new body partition will be inserted every %d frame%s\n", BodyRate, BodyRate==1 ? "" : "s");
		}

		if(BodyMode == Body_Size) 
			printf("Partitions will be limited to %d byte%s (if possible)\n", BodyRate, BodyRate==1 ? "" : "s");
	}

	return true;
}


//! Process the essence, writing it as we go
Int64 ProcessEssence(FileHandle InFile, MXFFilePtr &Out, Int64 PictureLimit /*=0*/)
{
//IT# 	Int64 ECStart = Out->Tell();
//IT# 	Int64 EditUnit = 0;
	Int64 PictureCount = 0;

	// Flag that we haven't yet found the first picture
	Int64 PictureStart = -1;
	int PictureStartBitOffset = 0;
	Int64 PictureEnd = 0;
	int PictureEndBitOffset = 0;
	bool GOPStart = false;				//!< Set if this picture is the start of a GOP

	// Start of this partition(if partitioning by size)
	Uint64 PartitionStart = 0;

	// Size of this body partition in frames (if partitioning by frame)
	Uint32 BodySize = 0;

	// We haven't yet found the next picture start
	bool FoundEnd = false;

	// 32K buffer for scanning
	int ScanBufferSize = 32768;
	Uint8 *ScanBuffer = new Uint8[ScanBufferSize];

	// Offset in the file of the first byte in the buffer
	Int64 BufferStart = 0;

	for(;;)
	{
		// Read a chunk into the buffer
		FileSeek(InFile,BufferStart);
		int BufferCount = FileRead(InFile, ScanBuffer, ScanBufferSize);

//printf("Read %d\n", BufferCount);

		// If 4 bytes or less left in the input file then we can't have a start_code
		if(BufferCount <= 4) break;

		// Scan all but the last four bytes
		// These will be the first 4 bytes of the next scan - this allows us to look ahead
		int ScanPos = 0;
		int ScanLeft = BufferCount - 4;
		Uint8 *BuffPtr = ScanBuffer;
		while(ScanLeft)
		{
//printf("%02x ", *BuffPtr);

			if(*BuffPtr == 0)
			{
				if(BuffPtr[1] == 0)
				{
					// We know we have 16 zeros, but is it part of a start_code?
					// We check for this by testing the outer two bytes
					// Perform the test in this CPU's "favorite" integer type
					unsigned int Test;
					if(ScanPos == 0) Test = 0xff00 | BuffPtr[2];
					else Test = (BuffPtr[-1] << 8) | (BuffPtr[2]);

//printf("Found two zeros.. Test=0x%04x\n", Test);

					int Pos = 0;
					if((Test & 0x00ff) == 0x0001) Pos = 8;			// Test the most common first
					else if((Test & 0x7f80) == 0x0080) Pos = 1;
					else if((Test & 0x3fc0) == 0x0040) Pos = 2;
					else if((Test & 0x1fe0) == 0x0020) Pos = 3;
					else if((Test & 0x0ff0) == 0x0010) Pos = 4;
					else if((Test & 0x07f8) == 0x0008) Pos = 5;
					else if((Test & 0x03fc) == 0x0004) Pos = 6;
					else if((Test & 0x01fe) == 0x0002) Pos = 7;

					if(Pos)
					{
						Uint8 start_code;

						// We have a startcode... but which one?
						if(Pos == 8)
						{
							start_code = BuffPtr[3];
						}
						else
						{
							unsigned int Temp = (BuffPtr[2] << 8) | BuffPtr[3];
							start_code = (Temp >> (8-Pos));
						}

//printf("Found startcode 0x%02x\n", start_code);
						if(!FoundEnd)
						{
							// End of current picture is picture_start or sequence_header or group_start
							if((start_code == 0) || (start_code == 0xb3) || (start_code == 0xb8))
							{
								if(Pos == 8)
								{
									int Temp = int(&BuffPtr[0]) - int(ScanBuffer);
									PictureEnd = BufferStart + Temp;
									PictureEndBitOffset = 0;
								}
								else
								{
									int Temp = int(&BuffPtr[-1]) - int(ScanBuffer);
									PictureEnd = BufferStart + Temp;
									PictureEndBitOffset = Pos;
									error("Only byte aligned picture starts currently supported!\n");
								}
								FoundEnd = true;

								// This picture is the first in a GOP
								if(start_code == 0xb8) GOPStart = true;
							}
						}

						// If this is the picture start code write the previous picture
						if(start_code == 0)
						{
							// Don't write anything until we have found the first picture
							if(PictureStart >= 0)
							{
								// Align with the KAG is we are using one
								if(KAGSize > 1) Out->Align(KAGSize);

//IT# 								Table->AddEntry(EditUnit, Out->Tell() - ECStart);
//IT# 								EditUnit++;

								if(BodyMode == Body_Size)
								{
//printf("Start=0x%08x Here=0x%08x, Size=0x%08x\n", (int)PartitionStart, (int)Out->Tell(), (int)(Out->Tell() - PartitionStart));
									if(((Out->Tell() - PartitionStart) + (PictureEnd - PictureStart) + 20) > BodyRate)
									{
										PartitionStart = Out->Tell();
										PartitionPtr ThisPartition = new Partition("ClosedCompleteBodyPartition");
										ThisPartition->SetUint("BodySID",1);
										Out->WritePartition(ThisPartition);
									}
								}
								else if(BodyMode == Body_Duration)
								{
									if((BodySize >= BodyRate) && ((!GOPAlign) || GOPStart))
									{
										BodySize = 0;
										PartitionPtr ThisPartition = new Partition("ClosedCompleteBodyPartition");
										ThisPartition->SetUint("BodySID",1);
										Out->WritePartition(ThisPartition);
									}
									else BodySize++;
								}
								WriteMPEG(InFile, Out, PictureStart, PictureStartBitOffset, PictureEnd, PictureEndBitOffset);
								PictureCount++;
							}

							// Restart
							PictureStart = PictureEnd;
							PictureStartBitOffset = PictureEndBitOffset;
							FoundEnd = false;
							GOPStart = false;
						}
					}
				}
			}

			// Scan through the buffer
			BuffPtr++;
			ScanPos++;
			ScanLeft--;
		}

		// Get another chunk of buffer
		BufferStart += ScanPos;
	}
	
	// Don't write anything if we haven't found the first picture
	if(PictureStart >= 0)
	{
		// Align with the KAG is we are using one
		if(KAGSize > 1) Out->Align(KAGSize);

//IT# 				Table->AddEntry(EditUnit, Out->Tell() - ECStart);
//IT# 				EditUnit++;

		WriteMPEG(InFile, Out, PictureStart, PictureStartBitOffset, FileTell(InFile)+1, 0);
		PictureCount++;
	}

	return PictureCount;
}


//! Write a frame of MPEG
void WriteMPEG(FileHandle InFile, MXFFilePtr &Out, Int64 PictureStart, int PictureStartBitOffset, Int64 PictureEnd, int PictureEndBitOffset)
{
	//printf("Write: 0x%08x:%d -> 0x%08x:%d\n", (int)PictureStart, PictureStartBitOffset, (int)PictureEnd, PictureEndBitOffset);

	if((PictureStartBitOffset != 0) || (PictureEndBitOffset != 0))
	{
		error("Only byte aligned picture writing currently supported!\n");
		return;
	}

	Int64 Size = PictureEnd - PictureStart;
	Uint8 *Buffer = new Uint8[Size];
	FileSeek(InFile, PictureStart);
	FileRead(InFile, Buffer, Size);

	// Note that the version number of 1 as the registry is not yet available!
	static Uint8 MPEG_ES_Key[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x15, 0x01, 0x05, 0x00 };

	Out->Write(MPEG_ES_Key, 16);
	Out->WriteBER(Size);
	Out->Write(Buffer, Size);
}


//! Read the start of the essence file and parse the sequence header to build an essence descriptor
/*! DRAGONS: Currently rather scrappy */
MDObjectPtr BuildMPEGEssenceDescriptor(FileHandle InFile)
{
	MDObjectPtr Ret;
	Uint8 Buffer[128];

	// Read the start of the file
	FileRead(InFile, Buffer, 128);

	if((Buffer[0] != 0) || (Buffer[1] != 0) || (Buffer[2] != 1) || (Buffer[3] != 0xb3))
	{
		error("Current implementation only supports essence starting with a sequence header\n");
		return Ret;
	}

	Uint32 HSize = (Buffer[4] << 4) | (Buffer[5] >> 4);
	Uint32 VSize = ((Buffer[5] & 0x0f) << 8) | (Buffer[6]);

	char *Aspect;
	switch(Buffer[7] & 0xf0)
	{
	default: Aspect = NULL; break;
	case 0x10: Aspect = "1/1"; break;
	case 0x20: Aspect = "4/3"; break;
	case 0x30: Aspect = "16/9"; break;
	case 0x40: Aspect = "221/100"; break;
	}

	int FrameRate = 0;
	bool DropFrame = false;
	switch(Buffer[7] & 0x0f)
	{
	case 0x01: FrameRate = 24; DropFrame = true; break;
	case 0x02: FrameRate = 24; break;
	case 0x03: FrameRate = 25; break;
	case 0x04: FrameRate = 30; DropFrame = true; break;
	case 0x05: FrameRate = 30; break;
	case 0x06: FrameRate = 50; break;
	case 0x07: FrameRate = 60; DropFrame = true; break;
	case 0x08: FrameRate = 60; break;
	}

	if(FrameRate == 0) error("Unknown frame rate!\n");

	Uint32 BitRate = (Buffer[8] << 10) | (Buffer[9] << 2) | (Buffer[10] >> 6);

	if(BitRate = 0x3ffff) warning("bit_rate = -1\n");

	// Assume some values if no extension found
	Uint8 PandL = 0;
	bool Progressive = true;
	int HChromaSub = 2;
	int VChromaSub = 2;
	bool LowDelay = false;

	Uint8 LoadIntra = Buffer[11] & 0x04;
	Uint8 LoadNonIntra;
	if(LoadIntra == 0)
	{
		LoadNonIntra = Buffer[11] & 0x02;
	}
	else
	{
		LoadNonIntra = Buffer[11 + 64] & 0x02;
	}

	int ExtPos = 12;
	if(LoadIntra) ExtPos += 64;
	if(LoadNonIntra) ExtPos += 64;

	FileSeek(InFile, ExtPos);
	// Read the sequence extention
	FileRead(InFile, Buffer, 128);

	if((Buffer[0] != 0) || (Buffer[1] != 0) || (Buffer[2] != 1) || (Buffer[3] != 0xb5))
	{
		error("Sequence extension does not follow sequence header - some assumptions made\n");
	}
	else
	{
		PandL = (Buffer[4] << 4) | (Buffer[5] >> 4);
		
		if(Buffer[5] & 0x08) Progressive = true; else Progressive = false;

		int Sub = ((Buffer[5] & 0x01) << 1) | (Buffer[6] >> 7);
		if(Sub >= 2) VChromaSub = 1;
		if(Sub == 3) HChromaSub = 1;

		HSize |= ((Buffer[5] & 0x01) << 13) | ((Buffer[6] & 0x80) << 5);
		VSize |= ((Buffer[6] & 0x60) << 7);
		BitRate |= ((Buffer[6] & 0x1f) << 25) | ((Buffer[7] & 0xfe) << 17);

		if(Buffer[9] & 0x80) LowDelay = true;

		int FR_n = ((Buffer[9] & 0x60) >> 5) + 1;
		int FR_d = (Buffer[9] & 0x1f) + 1;

		FrameRate *= FR_n;
		FrameRate /= FR_d;
	}

/*
if(PandL) printf("Profile-and-Level = 0x%02x\n", PandL); else printf("Profile-and-Level unknown!!\n");
printf("HSize = %d\n", HSize);
printf("VSize = %d\n", VSize);
if(Aspect) printf("Aspect Ratio = %s\n", Aspect); else printf("Aspect Ratio unknown!!\n");
printf("BitRate = %u\n", BitRate * 400);
printf("Low Delay mode %s\n", LowDelay ? "is used" : "is not used");
printf("FrameRate = %d%s\n", FrameRate, DropFrame ? " * 1000/1001" : "");
printf("Frame Layout = %s\n", Progressive ? "Progressive" : "Interlaced");
printf("Chroma horizontal sub-sampling = %d\n", HChromaSub);
printf("Chroma vertical sub-sampling = %d\n", VChromaSub);
*/

	// Build the essence descriptor, filling in all known values

	Ret = new MDObject("MPEG2VideoDescriptor");
	if(!Ret) return Ret;

	char Buff[32];
	if(DropFrame)
	{
		sprintf(Buff, "%d000/1001", FrameRate);
		Ret->SetString("SampleRate", Buff);
	}
	else
	{
		sprintf(Buff, "%d/1", FrameRate);
		Ret->SetString("SampleRate", Buff);
	}

	if(Progressive) Ret->SetInt("FrameLayout", 0); else Ret->SetInt("FrameLayout", 1);

	Ret->SetUint("StoredWidth", HSize);
	Ret->SetUint("StoredHeight", VSize);

	if(Aspect) Ret->SetString("AspectRatio", Aspect); else Ret->SetDValue("AspectRatio");

	MDObjectPtr Ptr = Ret->AddChild("VideoLineMap");
	if(Ptr)
	{
		int F1 = 0;
		int F2 = 0;

		if(VSize == 576) { F1 = 1; F2 = 313; }
		else if(VSize == 480) { F1 = 4; F2 = 266; }
//		else if(VSize == 720) { F1 = 1; F2 = 0; }
//		else if((VSize == 1080) && Progressive) { F1 = 1; F2 = 0; }
//		else if(VSize == 1080) { F1 = 1; F2 = 564; }

		if((F1 == 0) & (F2 == 0))
		{
			Ptr->AddChild("VideoLineMapEntry", false)->SetDValue();
			Ptr->AddChild("VideoLineMapEntry", false)->SetDValue();
		}
		else
		{
			Ptr->AddChild("VideoLineMapEntry", false)->SetUint(F1);
			Ptr->AddChild("VideoLineMapEntry", false)->SetUint(F2);
		}
	}

	Ret->SetUint("ComponentDepth", 8);

	Ret->SetUint("HorizontalSubsampling", HChromaSub);
	Ret->SetUint("VerticalSubsampling", VChromaSub);

	if((HChromaSub == 2) && (VChromaSub == 2))
		Ret->SetUint("ColorSiting", 3);				// Quincunx 4:2:0
	else if((HChromaSub == 2) && (VChromaSub == 1))
		Ret->SetUint("ColorSiting", 4);				// Rec 601 style 4:2:2
	if((HChromaSub == 1) && (VChromaSub == 1))
		Ret->SetUint("ColorSiting", 0);				// 4:4:4

	if(Progressive)	Ret->SetUint("CodedContentType", 1); else 	Ret->SetUint("CodedContentType", 2);
	if(LowDelay)	Ret->SetUint("LowDelay", 1); else	Ret->SetUint("LowDelay", 0);

	if(BitRate != 0x3ffff) Ret->SetUint("BitRate", BitRate * 400);

	Ret->SetUint("ProfileAndLevel", PandL);

	return Ret;
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


