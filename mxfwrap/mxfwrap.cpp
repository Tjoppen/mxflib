/*! \file	mxfwrap.cpp
 *	\brief	Basic MXF essence wrapping utility
 */
/*
 *	Copyright (c) 2003, Matt Beard
 *	Portions Copyright (c) 2003, Metaglue Corporation
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

#include <stdio.h>
#include <iostream>

using namespace std;

// base library version
std::string ProductVersion = "Unreleased mxflib 0.3.3.2";

// DMStiny
#ifdef DMStiny
// DMStinyIDs.h contains Company,Product,GUID, and other stuff
#include "DMStinyIDs.h"
#include "DMStiny.h"
#else
// Product GUID and version text for this release
Uint8 ProductGUID_Data[16] = { 0x84, 0x66, 0x14, 0xf3, 0x27, 0xdd, 0xde, 0x40, 0x86, 0xdc, 0xe0, 0x99, 0xda, 0x7f, 0xd0, 0x53 };
std::string CompanyName = "FreeMXF.org";
std::string ProductName = "mxfwrap file wrapper";
#endif

//! Debug flag for KLVLib
int Verbose = 0;

//! Debug flag for MXFLib
static bool DebugMode = false;


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

// small utility functions that shoud be somewhere else
Int64 TCtoFrames(Uint16 FrameRate, bool DropFrame, Uint16 Hours, Uint16 Mins, Uint16 Secs, Uint16 Frames)
{
	Int64 f = Frames + FrameRate*( Secs + 60*( Mins + 60*Hours ) );

	if( (FrameRate == 30) && DropFrame )
	{
		Uint16 m = Mins + 60*Hours;
		f -= 2*( m - m/10 );
	}

	return f;
}

// non-exported forward declarations
bool ParseCommandLine(int &argc, char **argv);

int Process(	int OutFileNum,
							MXFFilePtr Out,
							EssenceParser::WrappingConfigList WrappingList,
							Rational EditRate,
							UMIDPtr *FPUMID
					 );

struct BodyWrapping
{
	GCWriterPtr Writer;
	GCStreamID EssenceID;
	EssenceParser::WrappingConfigPtr Config;
	FileHandle InFile;
	Uint32 BodySID;

	//! The mode of body partition insertion
	enum PartitionMode { Body_None, Body_Duration, Body_Size } BodyMode;

	Uint32 BodyRate;						//!< The rate of body partition insertion
};
typedef std::list<BodyWrapping> BodyWrappingList;

// OP Qualifier manipulators: ClearStream, SetStream, SetUniTrack, SetMultiTrack
void ClearStream(ULPtr &theUL);
void SetStream(ULPtr &theUL);
void SetUniTrack(ULPtr &theUL);
void SetMultiTrack(ULPtr &theUL);

namespace
{
	// Options
	char* DMStinyDict = NULL;						//!< Set to name of DMStiny xmldict
	char* DMStinyMaterial = NULL;				//!< Set to name of DMStiny Material instance data

	char InFilenameSet[512];				//!< The set of input filenames
	char InFilename[16][128];				//!< The list of input filenames
	int InFileGangSize;						//!< The number of ganged files to process at a time
	int InFileGangCount;					//!< The number of sets of ganged files to process
	char OutFilenameSet[512];				//!< The set of output filenames
	char OutFilename[16][128];				//!< The output filename
	int OutFileCount;						//!< The number of files to output

	FileHandle InFile[16];					//!< File handles
	Int64 Duration[16];						//!< Duration of each ganged section of essence

	bool OPAtom = false;					//!< Is OP-Atom mode being forced?
	bool UpdateHeader= false;				//!< Is the header going to be updated after writing the footer
	bool StreamMode = false;				//!< Wrap in stream-mode
	bool EditAlign = false;					//!< Start new body partitions only at the start of a GOP
	bool UseIndex	= false;				//!< Write complete index tables
	bool SparseIndex = false;				//!< Write sparse index tables (one entry per partition)
	bool SprinkledIndex = false;			//!< Write segmented index tables (one set per partition)
	bool IsolatedIndex = false;				//!< Don't write essence and index in same partition
	bool VeryIsolatedIndex = false;			//!< Don't write metadata and index in same partition

	Position LastEditUnit[128];				//!< Table of last edit units written in sparse index tables (per BodySID)


	// DRAGONS: Temporary option!
	bool FrameGroup = false;				//!< Group all as a frame-wrapped group (in one essence container)

	int IndexManCount;						//!< Number of index managers in use
	IndexManagerPtr IndexMan[16];			//!< Index managers per indexed essence container
	std::list<Position> SparseList[16];		//!< List of edit units to include in sparse index tables (per BodySID)

	Rational ForceEditRate;					//!< Edit rate to try and force

	//! The mode of body partition insertion
	//enum { Body_None, Body_Duration, Body_Size } 
	BodyWrapping::PartitionMode BodyMode = BodyWrapping::Body_None;
	Uint32 BodyRate = 0;					//!< The rate of body partition insertion

	Uint32 HeaderPadding = 0;				//!< The (minimum) number of bytes of padding to leave in the header

	Uint32 KAGSize = 1;						//!< The KAG Size for this file

	// Derived options
	ULPtr OPUL;								//!< The UL of the OP for this file
}


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




int main(int argc, char *argv[])
{
	printf("Simple MXF wrapping application\n\n");

	// Build an essence parser
	EssenceParser EssParse;

	// Load the dictionaries
	LoadTypes("types.xml");
	MDOType::LoadDict("xmldict.xml");

	// Parse command line options and exit on error
	ForceEditRate.Numerator = 0;
	if(!ParseCommandLine(argc, argv))
		return 1;

#ifdef DMStiny
	// load the DMStiny Dictionary
	if( DMStinyDict ) MDOType::LoadDict( DMStinyDict );
#endif

	EssenceParser::WrappingConfigList WrappingList;
	EssenceParser::WrappingConfigList::iterator WrappingList_it;

	// The edit rate for all tracks in this file
	Rational EditRate;

	// Identify the wrapping options
	// DRAGONS: Not flexible yet
	int i;
	int InCount = InFileGangSize * InFileGangCount;
	for(i=0; i< InCount; i++)
	{
		// Open the input file
		InFile[i] = FileOpenRead(InFilename[i]);
		if(!FileValid(InFile[i]))
		{
			error("Can't open input file \"%s\"\n", InFilename[i]);
			return 2;
		}

		// Build a list of parsers with their descriptors for this essence
		ParserDescriptorListPtr PDList = EssParse.IdentifyEssence(InFile[i]);

		if(PDList->empty())
		{
			error("Could not identify the essence in file \"%s\"\n", InFilename[i]);
			return 3;
		}

		EssenceParser::WrappingConfigPtr WCP;
		if(FrameGroup) WCP = EssParse.SelectWrappingOption(InFile[i], PDList, ForceEditRate, WrappingOption::Frame);
		else WCP = EssParse.SelectWrappingOption(InFile[i], PDList, ForceEditRate);

// Fixed now ? ## When PDList is deleted so is the essence parser...

		if(!WCP)
		{
			error("Could not identify a wrapping mode for the essence in file \"%s\"\n", InFilename[i]);
			return 4;
		}

		// Ensure the essence descriptor reflects the new wrapping
		WCP->EssenceDescriptor->SetValue("EssenceContainer", DataChunk(16,WCP->WrapOpt->WrappingUL->GetValue()));

		// Add this wrapping option
		WrappingList.push_back(WCP);

		// Edit rate for this file
		EditRate = WCP->EditRate;

		// DRAGONS: Once we have set the edit rate for the first file we force it on the rest
		ForceEditRate = EditRate;

		printf("\nSelected wrapping for file \"%s\" : %s\n", InFilename[i], WCP->WrapOpt->Description.c_str());
	}

	// Generate UMIDs for each file package
	UMIDPtr FPUMID[16];								//! UMIDs for each file package (internal or external)
	i = 0;				//  Essence container and track index
	WrappingList_it = WrappingList.begin();
	while(WrappingList_it != WrappingList.end())
	{
		switch((*WrappingList_it)->WrapOpt->GCEssenceType)
		{
		case 0x05: case 0x15:
			FPUMID[i] = MakeUMID(1);
			break;
		case 0x06: case 0x16:
			FPUMID[i] = MakeUMID(2);
			break;
		case 0x07: case 0x17:
			FPUMID[i] = MakeUMID(3);
			break;
		case 0x18: default:
			FPUMID[i] = MakeUMID(4);
			break;
		}

		WrappingList_it++;
		i++;
	}

	// Set any OP qualifiers
	if(!OPAtom)
	{
		if((FrameGroup) || (WrappingList.size() == 1))
		{
			SetUniTrack(OPUL);
			SetStream(OPUL);
		}
		else
		{
			SetMultiTrack(OPUL);
			if(StreamMode) SetStream(OPUL); else ClearStream(OPUL);
		}
	}

	int OutFileNum;
	for(OutFileNum=0; OutFileNum < OutFileCount ; OutFileNum++)
	{
		// Open the output file
		MXFFilePtr Out = new MXFFile;
		if(!Out->OpenNew(OutFilename[OutFileNum]))
		{
			error("Can't open output file \"%s\"\n", OutFilename[OutFileNum]);
			return 5;
		}

		printf( "\nProcessing output file \"%s\"\n", OutFilename[OutFileNum]);

		Process( OutFileNum, Out, WrappingList, EditRate, FPUMID );

		// Close the file - all done!
		Out->Close();
	}

	printf("\nDone\n");

	if( DebugMode ) { fprintf( stderr, "press enter to continue..."); getchar(); }
	return 0;
}


// OP Qualifier manipulators: ClearStream, SetStream, SetUniTrack, SetMultiTrack
void ClearStream(ULPtr &theUL)
{
	Uint8 Buffer[16];

	memcpy(Buffer, theUL->GetValue(), 16);

	if(Buffer[12] > 3) 
	{
		warning("ClearStream() called on specialized OP UL\n");
		return;
	}

	Buffer[14] |= 0x04;

	theUL->Set(Buffer);
}

void SetStream(ULPtr &theUL)
{
	Uint8 Buffer[16];

	memcpy(Buffer, theUL->GetValue(), 16);

	if(Buffer[12] > 3) 
	{
		warning("SetStream() called on specialized OP UL\n");
		return;
	}

	Buffer[14] &= ~0x04;

	theUL->Set(Buffer);
}


void SetUniTrack(ULPtr &theUL)
{
	Uint8 Buffer[16];

	memcpy(Buffer, theUL->GetValue(), 16);

	if(Buffer[12] > 3) 
	{
		warning("SetUniTrack() called on specialized OP UL\n");
		return;
	}

	Buffer[14] &= ~0x08;

	theUL->Set(Buffer);
}

void SetMultiTrack(ULPtr &theUL)
{
	Uint8 Buffer[16];

	memcpy(Buffer, theUL->GetValue(), 16);

	if(Buffer[12] > 3) 
	{
		warning("SetMultiTrack() called on specialized OP UL\n");
		return;
	}

	Buffer[14] |= 0x08;

	theUL->Set(Buffer);
}


//! Parse the command line options
/*!	\return true if all parsed ok, false if an error or none supplied */
bool ParseCommandLine(int &argc, char **argv)
{
	int i;
	for(i=1; i<argc;)
	{
		if (IsCommandLineSwitchPrefix(argv[i][0]))
		{
			char *p = &argv[i][1];					// The option less the '-'
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
					BodyMode = BodyWrapping::Body_Duration;
					BodyRate = strtoul(Val, &temp, 0);
				}
				else if(tolower(p[1]) == 's')
				{
					char *temp;
					BodyMode = BodyWrapping::Body_Size;
					BodyRate = strtoul(Val, &temp, 0);
				}
				else error("Unknown body partition mode '%c'\n", p[1]);
			}
			else if(Opt == 'e') EditAlign = true;
			else if(Opt == 'f') FrameGroup = true;
			else if(Opt == 's') StreamMode = true;
			else if(Opt == 'v') DebugMode = true;
			else if(Opt == 'i')
			{
				if(tolower(p[1]) == 'i')
				{
					IsolatedIndex = true;
					if(p[2] == '2') VeryIsolatedIndex = true;
				}
				else if(tolower(p[1]) == 'p')
				{
					SparseIndex = true;
				}
				else if(tolower(p[1]) == 's')
				{
					SprinkledIndex = true;
				}
				else
				{
					UseIndex = true;
				}
			}
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
			else if(Opt == 'r')
			{
				int N, D; // Use ints in case Int32 is not the same size as "int" on this platform
				if(sscanf(Val,"%d/%d", &N, &D) == 2)
				{
					ForceEditRate.Numerator = N;
					ForceEditRate.Denominator = D;
				}
				else
				{
					error("Invalid edit rate format \"%s\"\n", Val);
				}
			}
			else if(Opt == 'u') 
			{
				UpdateHeader = true;
			}
#ifdef DMStiny
			else if(Opt == 't')
			{
				// DMStiny Dictionary
				if(tolower(p[1])=='d')
				{
					char *name="DMStiny.xml"; // default name
					if( p[2]=='=' )	name=p[3]; // explicit name
					DMStinyDict = new char[ 1+strlen(name) ];
					strcpy( DMStinyDict, name );
				}
				// DMStiny Material instance data
				else if(tolower(p[1])=='m' )
				{
					char *name=""; // default name = none
					if( p[2]=='=' )	name=p[3]; // explicit name
					DMStinyMaterial = new char[ 1+strlen(name) ];
					strcpy( DMStinyMaterial, p[3] );
				}
			}
#endif
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
		printf("Usage:    mxfwrap [options] <inputfiles> <mxffile>\n\n");

		printf("Syntax for input files:\n");
		printf("         a,b = file a followed by file b\n");
		printf("         a+b = file a ganged with file b\n");
		printf("     a+b,c+d = file a ganged with file b\n");
		printf("               followed by file c ganged with file d\n\n");

		printf("Note: There must be the same number of ganged files in each sequential set\n");
		printf("      Also all files in each set must be the same duration\n\n");

		printf("Options:\n");
		printf("    -a         = Force OP-Atom\n");
		printf("    -e         = Only start body partitions at edit points\n");
		printf("    -f         = Frame-wrap and group in one container\n");
		printf("    -h=<size>  = Leave at least <size> bytes of expansion space in the header\n");
		printf("    -i         = Write index tables (at the end of the file)\n");
		printf("    -ip        = Write sparse index tables with one entry per partition\n");
		printf("    -is        = Write index tables sprinkled one section per partition\n");
		printf("    -ii        = Isolated index tables (don't share partition with essence)\n");
		printf("    -ii2       = Isolated index tables (don't share with essence or metadata)\n");
		printf("    -k=<size>  = Set KAG size (default=1)\n");
		printf("   -pd=<dur>   = Body partition every <dur> frames\n");
		printf("   -ps=<size>  = Body partition roughly every <size> bytes\n");
		printf("                 (early rather than late)\n");
		printf("    -r=<n>/<d> = Force edit rate (if possible)\n");
		printf("    -s         = Interleave essence containers for streaming\n");
		printf("    -u         = Update the header after writing footer\n\n");
		printf("    -v         = Verbose mode\n\n");

#ifdef DMStiny
		printf("   -td         = Enable DMStiny with default dictionary DMStiny.xml\n");
		printf("   -td=<name>  = Enable DMStiny with explicit dictionary\n");
		printf("   -tm         = Enable DMStiny Material metadata\n");
		printf("   -tm=<name>  = Enable DMStiny with explicit MAterial package instance metadata\n");
#endif

		return false;
	}

	InFileGangCount = 1;
	InFileGangSize = 1;

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

		// If all files progessed end scan
		if(*ps == '\0') break;

		// Otherwise we found ',' or '+' so skip it
		ps++;
	}

	strncpy(OutFilenameSet, argv[2], 510);
	OutFileCount = 0;
	ps = OutFilenameSet;
	for(;;)
	{
		char *LastDot = NULL;
		char *pd = OutFilename[OutFileCount];

		// Find the position of last dot in the input filename
		while(*ps)
		{
			if(*ps == '.') LastDot = ps;
			if(*ps == ',') { break; }
			if(*ps == '+') { break; }
			*pd++ = *ps++;
		};
		*pd = '\0';
		OutFileCount++;

		// If input filename specified no extension add ".mxf"
		if(LastDot == NULL)	strcpy(pd, ".mxf");

		// If all files progessed end scan
		if(*ps == '\0') break;

		// Otherwise we found ',' or '+' so skip it
		ps++;
	}


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

	if(OutFileCount == 1)
	{
		printf("Output file = %s\n\n", OutFilename[0]);
	}
	else
	{
		printf("Output files = ");
		int i;
		for(i=0; i<OutFileCount; i++) 
		{ 
			if(i != 0) printf(" with ");
			printf("%s", OutFilename[i]);
		}
		printf("\n");
	}

	if(OPAtom)
	{
		printf("Output OP = OP-Atom\n");
		
		// We will need to update the header
		UpdateHeader = true;

		OPUL = OPAtomUL;

		if((InFileGangCount * InFileGangSize) != OutFileCount) error("OP-Atom can only output a single essence container per file so requires as many output files as input files\n");
		
		if(BodyMode != BodyWrapping::Body_None) 
		{
			warning("Splitting essence across body partitions is forbidden in OP-Atom\n");
			BodyMode = BodyWrapping::Body_None;
		}

		// Force mandatory index table for OP-Atom
		UseIndex = true;
		IsolatedIndex = true;
	}
	else
	{
		if((FrameGroup) || (InFileGangSize == 1))
		{
			if(InFileGangCount == 1) { printf("Output OP = OP1a\n"); OPUL = OP1aUL; }
			else { printf("Output OP = OP2a\n"); OPUL = OP2aUL; }
		}
		else
		{
			if(InFileGangCount == 1) { printf("Output OP = OP1b\n"); OPUL = OP1bUL; }
			else { printf("Output OP = OP2b\n"); OPUL = OP2bUL; }
		}

		if(InFileGangCount > 1) error("Only OP1a and OP1b currently supported\n");
	}

	if(UpdateHeader)
	{
		// We will need some extra space in the header
		if(HeaderPadding == 0) HeaderPadding = 16384;

		printf("An updated header will be written after writing the footer\n");
	}

	if(HeaderPadding)
	{
		if(UpdateHeader) printf("At least %d padding bytes will be left after the initial writing of the header\n", HeaderPadding);
		else printf("At least %d padding bytes will be left after writing the header\n", HeaderPadding);
	}

	if(StreamMode && (InFileGangSize == 1))
	{
		warning("Essence containers will not be interleaved for streaming as none are ganged\n");
		StreamMode = false;
	}

	if(StreamMode) 
	{
		printf("Essence containers will be interleaved for streaming\n");

		if(BodyMode != BodyWrapping::Body_None) 
		{
			warning("Body partitions will be inserted for interleaving - this overrides other body partitioning options\n");
			BodyMode = BodyWrapping::Body_None;
		}

		error("Stream mode not yet supported\n");
	}
	else
	{
		if(BodyMode == BodyWrapping::Body_Duration)
		{
			if(EditAlign)
				printf("A new body partition will be inserted at the first new GOP after each %d frame%s\n", BodyRate, BodyRate==1 ? "" : "s");
			else
				printf("A new body partition will be inserted every %d frame%s\n", BodyRate, BodyRate==1 ? "" : "s");
		}

		if(BodyMode == BodyWrapping::Body_Size)
			printf("Partitions will be limited to %d byte%s (if possible)\n", BodyRate, BodyRate==1 ? "" : "s");
	}

	if(UseIndex) printf("Index tables will be written for each frame wrapped essence container\n");
	if(SprinkledIndex) 
	{
		if(UseIndex) printf("Index tables will also be sprinkled across partitions for each frame wrapped container\n");
		else		 printf("Index tables will be sprinkled across partitions for each frame wrapped essence container\n");
	}
	if(SparseIndex) 
	{
		if(UseIndex || SprinkledIndex) printf("Sparse index tables will also be written for each frame wrapped container\n");
		else						   printf("Sparse index tables will be written for each frame wrapped essence container\n");
	}

	if((UseIndex || SparseIndex || SprinkledIndex) && (IsolatedIndex))
	{
		if(VeryIsolatedIndex) printf("Index table segments will not share a partition with essence or metadata\n");
		else printf("Index table segments will not share a partition with essence\n");
	}

	// Check for stray parameters as a space in the wrong place can otherise cause us to overwrite input files!
	if(argc > 3)
	{
		printf("\nThere appear to be too many filenames on the command line\n");
		return false;
	}

	return true;
}


/// CUT HERE

// non-exported forward declarations
typedef std::list<BodyWrapping> BodyWrappingList;
Int64 WriteBody(MXFFilePtr Out, BodyWrappingList BodyWrapList, PartitionPtr ThisPartition, Int64 Duration = 0);

//! Short term hack to allow per-BodySID GCWriters
/*! DRAGONS: This needs to be tidied a little when there is time! */
GCWriterPtr AddGCWriter(std::map<int, GCWriterPtr> &Map, MXFFilePtr &File, int BodySID)
{
	// First try and return an existing GCWriter
	std::map<int, GCWriterPtr>::iterator it = Map.find(BodySID);
	if(it != Map.end()) return (*it).second;

	// Insert a new writer
	Map.insert(std::map<int, GCWriterPtr>::value_type(BodySID, new GCWriter(File, BodySID)));

	// Find and return the new entry (not hugely efficient!)
	it = Map.find(BodySID);

	return (*it).second;
}


int Process(	int OutFileNum,
							MXFFilePtr Out,
							EssenceParser::WrappingConfigList WrapCfgList,
							Rational EditRate,
							UMIDPtr *FPUMID 
					 )

{
	EssenceParser::WrappingConfigList::iterator WrapCfgList_it;

	int Ret = 0;

	///@step Create a set of header metadata

	MetadataPtr MData = new Metadata();
	ASSERT(MData);
	ASSERT(MData->Object);

	// DRAGONS: Why is this here? It unconditionally adds "Used to describe multiple wrappings not
	//          otherwise covered under the MXF Generic Container node" to all MXF files!!

	// REMOVED: // Assume we are doing GC
	// REMOVED: ULPtr GCUL = new UL( mxflib::GCMulti_Data );
	// REMOVED: MData->AddEssenceType( GCUL );

	// DMStiny
	#ifdef DMStiny
		// DMStinyIDs.h contains const char DMStinyFrameworkName = "name";
		if( DMStinyDict )	MData->AddDMScheme( MDOType::Find(DMStinyFrameworkName)->GetUL() );
	#endif

	// Set the OP label
	// If we are writing OP-Atom we write the header as OP1a initially as another process
	// may try to read the file before it is complete and then it will NOT be a valid OP-Atom file
	// DRAGONS: This should be OPxx which matches the number of file packages...
	if(OPAtom) MData->SetOP(OP1aUL); else MData->SetOP(OPUL);

	// Work out the edit rate from the descriptor
	bool DropFrame = 0;
	Uint32 FrameRate = EditRate.Numerator;

	// Use drop-frame for any non-integer frame rate
	if(EditRate.Denominator > 1)
	{
		// This is an integer equivalent of FrameRate = floor((FrameRate + 0.5) / Denominator)
		FrameRate += EditRate.Denominator - 1;
		FrameRate /= EditRate.Denominator;

		DropFrame = true;
	}

	// Build the Material Package
	// DRAGONS: We should really try and determine the UMID type rather than cop-out!
	UMIDPtr pUMID = MakeUMID( 0x0d ); // mixed type

	// DMStiny
	#ifdef DMStiny
		// DMStiny::AdjustMaterialUMID() conforms to any special material numbering scheme
		if( DMStinyMaterial )	AdjustMaterialUMID( pUMID );
	#endif

	PackagePtr MaterialPackage = MData->AddMaterialPackage("A Material Package", pUMID);

	MData->SetPrimaryPackage(MaterialPackage);		// This will be overwritten for OP-Atom

	// DMStiny
	#ifdef DMStiny
		// DMStiny::AddDMStiny() creates a DM track and populates it with DMSegments
		if( DMStinyMaterial )	AddDMStiny( MaterialPackage, DMStinyMaterial );
	#endif


	TrackPtr MPTimecodeTrack = MaterialPackage->AddTimecodeTrack(EditRate);
	TimecodeComponentPtr MPTimecodeComponent = MPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, 0 );

	// Writers for each BodySID
	std::map<int, GCWriterPtr> WriterMap;

	// Build the File Packages and all essence tracks

	// FP UMIDs are the same for all OutFiles, so they are supplied as a parameter
	GCWriterPtr Writer[16];							//! Writers for each essence container
	GCStreamID EssenceID[16];						//! Essence stream ID for each essence stream
	TimecodeComponentPtr FPTimecodeComponent[16];	//! Timecode component for each file package
	TrackPtr MPTrack[16];							//! Material Package track for each essence stream
	TrackPtr FPTrack[16];							//! File Package track for each essence stream
	SourceClipPtr MPClip[16];						//! Material Package SourceClip for each essence stream 
	SourceClipPtr FPClip[16];						//! File Package SourceClip for each essence stream 

	PackagePtr FilePackage;

	unsigned int PrevEssenceType = 0;

	int iTrack = 0;				//  Essence container and track index
	WrapCfgList_it = WrapCfgList.begin();
	while(WrapCfgList_it != WrapCfgList.end())
	{
		TrackPtr FPTimecodeTrack;

		// Write File Packages except for externally ref'ed essence in OP-Atom
		bool WriteFP = (!OPAtom) || (iTrack == OutFileNum);

		if( OPAtom )
		{
			if(WriteFP) // (iTrack == OutFileNum)
			{
				// Set up a writer for body SID (iTrack + 1)
				Writer[iTrack] = AddGCWriter(WriterMap, Out, iTrack + 1);

				// Set the KAG size and force 4-byte BER lengths (for maximum compatibility)
				Writer[iTrack]->SetKAG(KAGSize, true);

				// Add an essence element
				EssenceID[iTrack] = Writer[iTrack]->AddEssenceElement((*WrapCfgList_it)->WrapOpt->GCEssenceType, (*WrapCfgList_it)->WrapOpt->GCElementType);

				FilePackage = MData->AddFilePackage(iTrack+1, std::string("File Package: ") + (*WrapCfgList_it)->WrapOpt->Description, FPUMID[iTrack]);

				FPTimecodeTrack = FilePackage->AddTimecodeTrack(EditRate);
				FPTimecodeComponent[iTrack] = FPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, TCtoFrames( FrameRate, DropFrame, 1, 0, 0, 0 ) );
			}
		}
		else if( FrameGroup ) // !OPAtom
		{
			Writer[iTrack] = AddGCWriter(WriterMap, Out, 1);
			
			/* How to we acheive this now?? -- I think that it will be automatic, needs checking!!
			// if same EssenceType as previous track, construct GCWriter with non-zero StreamBase
			if( iTrack != 0 && WrapCfgList_it != WrapCfgList.begin() 
			&& (*WrapCfgList_it)->WrapOpt->GCEssenceType == PrevEssenceType )
			{
				Writer[iTrack] = new GCWriter(Out, 1, Writer[iTrack-1]->GetStreamCount());
			}
			else
			{
				// Set up a writer for body SID 1
				Writer[iTrack] = new GCWriter(Out, 1);
			}
			*/

			// Set the KAG size and force 4-byte BER lengths (for maximum compatibility)
			Writer[iTrack]->SetKAG(KAGSize, true);

			// Add an essence element
			EssenceID[iTrack] = Writer[iTrack]->AddEssenceElement((*WrapCfgList_it)->WrapOpt->GCEssenceType, (*WrapCfgList_it)->WrapOpt->GCElementType);

			PrevEssenceType = (*WrapCfgList_it)->WrapOpt->GCEssenceType;

			if( iTrack == 0 )
			{
				FilePackage = MData->AddFilePackage(iTrack+1, std::string("File Package: ") + (*WrapCfgList_it)->WrapOpt->Description, FPUMID[iTrack]);

				FPTimecodeTrack = FilePackage->AddTimecodeTrack(EditRate);
				FPTimecodeComponent[iTrack] = FPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, TCtoFrames( FrameRate, DropFrame, 1, 0, 0, 0 ) );
			}
		}
		else // !OPAtom, !FrameGroup
		{
			// Set up a writer for body SID (iTrack + 1)
			Writer[iTrack] = AddGCWriter(WriterMap, Out, iTrack + 1);

			// Set the KAG size and force 4-byte BER lengths (for maximum compatibility)
			Writer[iTrack]->SetKAG(KAGSize, true);

			// Add an essence element
			EssenceID[iTrack] = Writer[iTrack]->AddEssenceElement((*WrapCfgList_it)->WrapOpt->GCEssenceType, (*WrapCfgList_it)->WrapOpt->GCElementType);

			if( true )
			{
				FilePackage = MData->AddFilePackage(iTrack+1, std::string("File Package: ") + (*WrapCfgList_it)->WrapOpt->Description, FPUMID[iTrack]);

				FPTimecodeTrack = FilePackage->AddTimecodeTrack(EditRate);
				FPTimecodeComponent[iTrack] = FPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, TCtoFrames( FrameRate, DropFrame, 1, 0, 0, 0 ) );
			}
		}

		// Add the appropriate Track to the Material Package
		if(iTrack < InFileGangSize) // first gang only
		{
			switch((*WrapCfgList_it)->WrapOpt->GCEssenceType)
			{
			case 0x05: case 0x15:
				MPTrack[iTrack] = MaterialPackage->AddPictureTrack(EditRate);
				break;
			case 0x06: case 0x16:
				MPTrack[iTrack] = MaterialPackage->AddSoundTrack(EditRate);
				break;
			case 0x07: case 0x17: default:
				MPTrack[iTrack] = MaterialPackage->AddDataTrack(EditRate);
				break;
			}
		}

		// Add the track to the file package
		if(WriteFP) 
		{
			switch((*WrapCfgList_it)->WrapOpt->GCEssenceType)
			{
			case 0x05: case 0x15:
				FPTrack[iTrack] = FilePackage->AddPictureTrack(Writer[iTrack]->GetTrackNumber(EssenceID[iTrack]), EditRate);
				break;
			case 0x06: case 0x16:
				FPTrack[iTrack] = FilePackage->AddSoundTrack(Writer[iTrack]->GetTrackNumber(EssenceID[iTrack]), EditRate);
				break;
			case 0x07: case 0x17: default:
				FPTrack[iTrack] = FilePackage->AddDataTrack(Writer[iTrack]->GetTrackNumber(EssenceID[iTrack]), EditRate);
				break;
			}
		}


		// Locate the material package track this essence is in
		int TrackNumber = iTrack;
		while(TrackNumber >= InFileGangSize) TrackNumber -= InFileGangSize;

		// Add a single Component to this Track of the Material Package
		MPClip[iTrack] = MPTrack[TrackNumber]->AddSourceClip();

		// Add a single Component to this Track of the File Package
		if(WriteFP) FPClip[iTrack] = FPTrack[iTrack]->AddSourceClip();

		// Add the file descriptor to the file package
		// except for externally ref'ed essence in OP-Atom
		if( OPAtom )
		{
			// Write a File Descriptor only on the internally ref'ed Track 
			if( WriteFP ) // (iTrack == OutFileNum)
			{
				(*WrapCfgList_it)->EssenceDescriptor->SetUint("LinkedTrackID", FPTrack[iTrack]->GetUint("TrackID"));
				FilePackage->AddChild("Descriptor")->MakeLink((*WrapCfgList_it)->EssenceDescriptor);

				MData->AddEssenceType((*WrapCfgList_it)->WrapOpt->WrappingUL);

				// Link the MP to the FP
				MPClip[iTrack]->MakeLink(FPTrack[iTrack], 0);
			}
			else // (!WriteFP)
			{
				// Link the MP to the external FP
				// DRAGONS: We must assume what the linked track will be... track 2
				MPClip[iTrack]->MakeLink(FPUMID[iTrack], 2, 0);
			}
		}
		else if( FrameGroup ) // !OPAtom
		{
			// write a MultipleDescriptor only on the first Iteration
			if( iTrack == 0 )
			{
					MDObjectPtr MuxDescriptor = new MDObject("MultipleDescriptor");
					MuxDescriptor->AddChild("SampleRate")->SetInt("Numerator",(*WrapCfgList_it)->EssenceDescriptor["SampleRate"]->GetInt("Numerator"));
					MuxDescriptor->AddChild("SampleRate")->SetInt("Denominator",(*WrapCfgList_it)->EssenceDescriptor["SampleRate"]->GetInt("Denominator"));

					MuxDescriptor->AddChild("EssenceContainer",false)->SetValue(DataChunk(16,mxflib::GCMulti_Data));

					MuxDescriptor->AddChild("SubDescriptorUIDs");
					FilePackage->AddChild("Descriptor")->MakeLink(MuxDescriptor);
			}

			// Write a SubDescriptor
			(*WrapCfgList_it)->EssenceDescriptor->SetUint("LinkedTrackID", FPTrack[iTrack]->GetUint("TrackID"));
			
			MDObjectPtr MuxDescriptor = FilePackage["Descriptor"]->GetLink();

			MuxDescriptor["SubDescriptorUIDs"]->AddChild("SubDescriptorUID", false)->MakeLink((*WrapCfgList_it)->EssenceDescriptor);

			MData->AddEssenceType((*WrapCfgList_it)->WrapOpt->WrappingUL);

			// Link the MP to the FP
			MPClip[iTrack]->MakeLink(FPTrack[iTrack], 0);
		}
		else // !OPAtom, !FrameGroup
		{
			// Write a FileDescriptor
			// DRAGONS Can we ever need a MultipleDescriptor?
			(*WrapCfgList_it)->EssenceDescriptor->SetUint("LinkedTrackID", FPTrack[iTrack]->GetUint("TrackID"));
			FilePackage->AddChild("Descriptor")->MakeLink((*WrapCfgList_it)->EssenceDescriptor);

			MData->AddEssenceType((*WrapCfgList_it)->WrapOpt->WrappingUL);

			// Link the MP to the FP
			MPClip[iTrack]->MakeLink(FPTrack[iTrack], 0);
		}

		WrapCfgList_it++;
		iTrack++;
	}


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
	Ident->SetString("CompanyName", CompanyName);
	Ident->SetString("ProductName", ProductName);
	Ident->SetString("VersionString", ProductVersion);
	UUIDPtr ProductUID = new mxflib::UUID(ProductGUID_Data);

	// DRAGONS: -- Need to set a proper GUID per released version
	//             Non-released versions currently use a random GUID
	//			   as they are not a stable version...
	Ident->SetValue("ProductUID", DataChunk(16,ProductUID->GetValue()));

	// Link the new Ident set with all new metadata
	// Note that this is done even for OP-Atom as the 'dummy' header written first
	// could have been read by another device. This flags that items have changed.
	MData->UpdateGenerations(Ident);

	// Write the header partition
	Out->WritePartition(ThisPartition, HeaderPadding);


	//
	// ** Set up indexing **
	//

	IndexManCount = 0;
	if(UseIndex || SparseIndex || SprinkledIndex)
	{
		WrapCfgList_it = WrapCfgList.begin();
		iTrack=0;
		int ManagerID = 0;
		while(WrapCfgList_it != WrapCfgList.end())
		{
			// Currently we can only index frame wrapped essence
			if((*WrapCfgList_it)->WrapOpt->ThisWrapType == WrappingOption::Frame)
			{
				// Only index it if we can
				if((*WrapCfgList_it)->WrapOpt->CanIndex)
				{
					if((!OPAtom) || (iTrack == OutFileNum))
					{
						// FrameGroup will use a single multi-stream index...
						if(FrameGroup)
						{
							int StreamID = 0;
							if(IndexManCount == 0)
							{
								IndexMan[0] = new IndexManager(0, (*WrapCfgList_it)->WrapOpt->BytesPerEditUnit );
								IndexMan[0]->SetBodySID(1);
								IndexMan[0]->SetIndexSID(129);
								IndexMan[0]->SetEditRate((*WrapCfgList_it)->EditRate);
							}
							else StreamID = IndexMan[0]->AddSubStream(0, (*WrapCfgList_it)->WrapOpt->BytesPerEditUnit );
//printf("IndexMan[0] -> %d\n", StreamID);
							(*WrapCfgList_it)->WrapOpt->Handler->SetIndexManager(IndexMan[0], StreamID);
							IndexManCount = 1;
						}
						// ...otherwise one per stream
						else
						{
							IndexMan[ManagerID] = new IndexManager(0, (*WrapCfgList_it)->WrapOpt->BytesPerEditUnit );
							IndexMan[0]->SetBodySID(iTrack + 1);
							IndexMan[0]->SetIndexSID(iTrack + 129);
							IndexMan[0]->SetEditRate((*WrapCfgList_it)->EditRate);

							(*WrapCfgList_it)->WrapOpt->Handler->SetIndexManager(IndexMan[ManagerID], 0);
							IndexManCount++;
						}
						ManagerID++;
					}
				}
			}

			WrapCfgList_it++;
			iTrack++;
		}
	}


	//
	// ** Process Essence **
	//

	// Clear all section durations
	{
		int i;
		for(i=0; i<16; i++) Duration[i] = 0;
	}

	// Clear all sprinkled index starts
	{
		int i;
		for(i=0; i<128; i++) LastEditUnit[i] =0;
	}

	BodyWrappingList BodyWrapList;

	// Do all frame-wrappings first
	WrapCfgList_it = WrapCfgList.begin();
	iTrack = 0;
	while(WrapCfgList_it != WrapCfgList.end())
	{
// DRAGONS: Why has this gone?
//		// Calculate the section this essence is for
//		int Section = 0;

		if((*WrapCfgList_it)->WrapOpt->ThisWrapType == WrappingOption::Frame)
		{
			if((!OPAtom) || (iTrack == OutFileNum))
			{
				BodyWrapping BW;

				BW.Writer = Writer[iTrack];
				BW.EssenceID = EssenceID[iTrack];
				BW.Config = (*WrapCfgList_it);
				BW.InFile = InFile[iTrack];

				if(FrameGroup) BW.BodySID = 1;
				else BW.BodySID = iTrack+1;

				BW.BodyMode = BodyMode;
				BW.BodyRate = BodyRate;

				BodyWrapList.push_back(BW);
			}
		}

		WrapCfgList_it++;
		iTrack++;
	}

	// Write all frame-wrapped items
	if(!BodyWrapList.empty()) Duration[0] = WriteBody(Out, BodyWrapList, ThisPartition);


	// Non-clip-wrapped items
	BodyWrapList.clear();
	WrapCfgList_it = WrapCfgList.begin();
	iTrack=0;
	while(WrapCfgList_it != WrapCfgList.end())
	{
		if(    ((*WrapCfgList_it)->WrapOpt->ThisWrapType != WrappingOption::Frame)
			&& ((*WrapCfgList_it)->WrapOpt->ThisWrapType != WrappingOption::Clip))
		{
			if((!OPAtom) || (iTrack == OutFileNum))
			{
				BodyWrapping BW;

				BW.Writer = Writer[iTrack];
				BW.EssenceID = EssenceID[iTrack];
				BW.Config = (*WrapCfgList_it);
				BW.InFile = InFile[iTrack];
				BW.BodySID = iTrack+1;

				BodyWrapList.push_back(BW);
			}
		}

		WrapCfgList_it++;
		iTrack++;
	}

	// Write all non-frame- or clip-wrapped items
	if(!BodyWrapList.empty()) 
	{
		if(Duration[0]) WriteBody(Out, BodyWrapList, ThisPartition, Duration[0]);
		else Duration[0] = WriteBody(Out, BodyWrapList, ThisPartition);
	}

	// Clip wrappings
	BodyWrapList.clear();
	WrapCfgList_it = WrapCfgList.begin();
	iTrack=0;
	while(WrapCfgList_it != WrapCfgList.end())
	{
		if(((*WrapCfgList_it)->WrapOpt->ThisWrapType == WrappingOption::Clip))
		{
			if((!OPAtom) || (iTrack == OutFileNum))
			{
				BodyWrapping BW;

				BW.Writer = Writer[iTrack];
				BW.EssenceID = EssenceID[iTrack];
				BW.Config = (*WrapCfgList_it);
				BW.InFile = InFile[iTrack];
				BW.BodySID = iTrack+1;

				BodyWrapList.push_back(BW);
			}
		}

		WrapCfgList_it++;
		iTrack++;
	}

	// Write all clip-wrapped items
	if(!BodyWrapList.empty()) 
	{
		if(Duration[0]) WriteBody(Out, BodyWrapList, ThisPartition, Duration[0]);
		else Duration[0] = WriteBody(Out, BodyWrapList, ThisPartition);
	}


	//
	// Write out a set of index tables
	//

	DataChunkPtr IndexChunk = new DataChunk();
	Uint32 IndexSID = 0;

	if(UseIndex || SparseIndex || SprinkledIndex)
	{
		// Find all essence container data sets so we can update "IndexSID"
		MDObjectListPtr ECDataSets;
		MDObjectPtr Ptr = MData["ContentStorage"];
		if(Ptr) Ptr = Ptr->GetLink();
		if(Ptr) Ptr = Ptr["EssenceContainerData"];
		if(Ptr) ECDataSets = Ptr->ChildList("EssenceContainer");

		int iManager;
		for(iManager=0; iManager<IndexManCount; iManager++)
		{
			// ** Handle leftover-sprinkles first **
			// *************************************

			if(SprinkledIndex)
			{
				// Make an index table containing all available entries
				IndexTablePtr Index = IndexMan[iManager]->MakeIndex();

				// Add any remaining entries
				Position EditUnit = IndexMan[iManager]->GetLastNewEditUnit();
				int Count = IndexMan[iManager]->AddEntriesToIndex(Index, LastEditUnit[Index->BodySID], EditUnit-1);

				// Only write if there were any entries left
				if(Count)
				{
					// Flush any previous index table before starting a new one
					if(IndexChunk->Size)
					{
						// Write the index in a partition of its own

						ThisPartition->ChangeType("ClosedCompleteBodyPartition");
						ThisPartition->SetUint("BodySID", 0);
						ThisPartition->SetUint("BodyOffset", 0);
						ThisPartition->SetUint("IndexSID",  IndexSID);

						Out->WritePartitionWithIndex(ThisPartition, IndexChunk, false);
					}

					// Write the index table
					Index->WriteIndex(*IndexChunk);
					
					// Record the IndexSID for when the index is written
					IndexSID = Index->IndexSID;
				}
			}

			// ** Handle full index tables next **
			// ***********************************
			
			if(UseIndex)
			{
				// Flush any previous index table before starting a new one
				if(IndexChunk->Size)
				{
					// Write the index in a partition of its own

					ThisPartition->ChangeType("ClosedCompleteBodyPartition");
					ThisPartition->SetUint("BodySID", 0);
					ThisPartition->SetUint("BodyOffset", 0);
					ThisPartition->SetUint("IndexSID",  IndexSID);

					Out->WritePartitionWithIndex(ThisPartition, IndexChunk, false);
				}

				// Make an index table containing all available entries
				IndexTablePtr Index = IndexMan[iManager]->MakeIndex();
				IndexMan[iManager]->AddEntriesToIndex(Index);

				// Write the index table
				Index->WriteIndex(*IndexChunk);
				
				// Record the IndexSID for when the index is written
				IndexSID = Index->IndexSID;
			}


			// ** Handle sparse index tables next **
			// *************************************
			
			if(SparseIndex)
			{
				// Flush any previous index table before starting a new one
				if(IndexChunk->Size)
				{
					// Write the index in a partition of its own

					ThisPartition->ChangeType("ClosedCompleteBodyPartition");
					ThisPartition->SetUint("BodySID", 0);
					ThisPartition->SetUint("BodyOffset", 0);
					ThisPartition->SetUint("IndexSID",  IndexSID);

					Out->WritePartitionWithIndex(ThisPartition, IndexChunk, false);
				}

				// Make an empty index table
				IndexTablePtr Index = IndexMan[iManager]->MakeIndex();

				// Force no re-ordering in the sparse index (to prevent unsatisfied links)
				int i;
				for(i=0; i<Index->BaseDeltaCount; i++)
				{
					if(Index->BaseDeltaArray[i].PosTableIndex < 0) Index->BaseDeltaArray[i].PosTableIndex = 0;
				}

				// Add each requested entry
				Uint32 BodySID = IndexMan[iManager]->GetBodySID();
				ASSERT(BodySID);
				std::list<Position>::iterator it = SparseList[BodySID-1].begin();
				while(it != SparseList[BodySID-1].end())
				{
					IndexMan[iManager]->AddEntriesToIndex(true, Index, (*it), (*it));
					it++;
				}

				// Write the index table
				Index->WriteIndex(*IndexChunk);
				
				// Record the IndexSID for when the index is written
				IndexSID = Index->IndexSID;
			}

			// Update IndexSID in essence container data set
			if(ECDataSets)
			{
				MDObjectList::iterator ECD_it = ECDataSets->begin();
				while(ECD_it != ECDataSets->end())
				{
					if((*ECD_it)->GetLink())
					{
						if((*ECD_it)->GetLink()->GetUint("BodySID") == IndexMan[iManager]->GetBodySID())
						{
							(*ECD_it)->GetLink()->SetUint("IndexSID", IndexMan[iManager]->GetIndexSID());
							break;
						}
					}
					ECD_it++;
				}
			}
		}
	}


	//
	// ** Write a footer (with updated durations) **
	//

	// Flush any previous index table before writing the footer for isolated index
	if(VeryIsolatedIndex && (IndexChunk->Size))
	{
		// Write the index in a partition of its own

		ThisPartition->ChangeType("ClosedCompleteBodyPartition");
		ThisPartition->SetUint("BodySID", 0);
		ThisPartition->SetUint("BodyOffset", 0);
		ThisPartition->SetUint("IndexSID",  IndexSID);

		Out->WritePartitionWithIndex(ThisPartition, IndexChunk, false);

		// Clear the index chunk
		IndexChunk->Resize(0);
	}

	// If we are writing OP-Atom this is the first place we can claim it
	if(OPAtom) 
	{
		MData->SetOP(OPAtomUL);

		// Set top-level file package correctly for OP-Atom
		// DRAGONS: This will nedd to be changed if we ever write more than one File Package for OP-Atom!
		MData->SetPrimaryPackage(FilePackage);
	}

	MData->SetTime();
	MPTimecodeComponent->SetDuration(Duration[0]);

	iTrack = 0;						//  Essence container index
	WrapCfgList_it = WrapCfgList.begin();
	while(WrapCfgList_it != WrapCfgList.end())
	{
		MPClip[iTrack]->SetDuration(Duration[0]);
		if((!OPAtom) || (iTrack == OutFileNum))
		{
			if((iTrack==0) || (!FrameGroup)) FPTimecodeComponent[iTrack]->SetDuration(Duration[0]);
			FPClip[iTrack]->SetDuration(Duration[0]);
			(*WrapCfgList_it)->EssenceDescriptor->SetInt64("ContainerDuration",Duration[0]);
		}
			
		WrapCfgList_it++;
		iTrack++;
	}

	// Update the generation UIDs in the metadata to reflect the changes
	MData->UpdateGenerations(Ident);

	// Turn the header or body partition into a footer
	ThisPartition->ChangeType("CompleteFooter");

	if(IndexChunk->Size)
	{
		ThisPartition->SetUint("IndexSID",  IndexSID);
	}

	// Make sure any new sets are linked in
	ThisPartition->UpdateMetadata(MData);

	// Actually write the footer
	// Note: No metadata in OP-Atom footer
	if(IndexChunk->Size)
	{
		if(OPAtom) Out->WritePartitionWithIndex(ThisPartition, IndexChunk, false);
		else Out->WritePartitionWithIndex(ThisPartition, IndexChunk);
	}
	else
	{
		if(OPAtom) Out->WritePartition(ThisPartition, false);
		else Out->WritePartition(ThisPartition);
	}

	// Add a RIP (note that we have to manually KAG align as a footer can end off the KAG)
	if(KAGSize > 1) Out->Align(KAGSize);
	Out->WriteRIP();

	//
	// ** Update the header ** 
	//
	// For generalized OPs update the value of "FooterPartition" in the header pack
	// For OP-Atom re-write the entire header
	//

	Uint64 FooterPos = ThisPartition->GetUint64("FooterPartition");
	Out->Seek(0);
	if(UpdateHeader)
	{
		ThisPartition->ChangeType("ClosedCompleteHeader");
		ThisPartition->SetUint64("FooterPartition", FooterPos);
		ThisPartition->SetUint("IndexSID", 0);				// DRAGONS: If we ever write index in the header this will need to change
		ThisPartition->SetUint64("IndexByteCount", 0);
		ThisPartition->SetUint64("BodySID", 1);
		Out->ReWritePartition(ThisPartition);
	}
	else
	{
		ThisPartition = Out->ReadPartition();
		ThisPartition->SetUint64("FooterPartition", FooterPos);
		Out->Seek(0);
		Out->WritePartitionPack(ThisPartition);
	}

	return Ret;
}






//! Write a set of essence containers
/*! IMPLEMENTATION NOTES:
 *		Wrapping more than one stream in a single container is achieved by using the same BodySID
 *		(but they must be contiguous)
 *		The current BodySID is read from ThisPartition
 *		Header metadata is currently not repeated
 *
 */
Int64 WriteBody(MXFFilePtr Out, BodyWrappingList BodyWrapList, PartitionPtr ThisPartition, Int64 Duration /*=0*/)
{
	Int64 Ret = 0;

	ThisPartition->ChangeType("ClosedCompleteBodyPartition");
	Uint32 CurrentBodySID = ThisPartition->GetUint("BodySID");

	//! Partition size to allow maximum body partition size to be set
	//  Start by calculating where the current partition starts
	Uint64 PartitionSize = 0;
	if(BodyMode == BodyWrapping::Body_Size) PartitionSize = Out->Tell() - ThisPartition->GetUint64("ThisPartition");

	// Assume done until we find out there is some data to write (in case the list is empty)
	bool Done = true;
	int ThisEditUnit = 0;
	for(;;)
	{
//@printf("\nOuter:");
		int i = 0;						//  BodySID index
		BodyWrappingList::iterator BodyWrapList_it = BodyWrapList.begin();
		while(BodyWrapList_it != BodyWrapList.end())
		{
//@printf("Inner:");
			Int64 Dur;
			if((*BodyWrapList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Clip) Dur = Duration; else Dur = 1;

			if((*BodyWrapList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Clip)
			{
				// Force a single pass...
				Done = true;

				EssenceSource *Source = (*BodyWrapList_it).Config->WrapOpt->Handler->GetEssenceSource((*BodyWrapList_it).InFile, (*BodyWrapList_it).Config->Stream, 0 /*, (*BodyWrapList_it).Writer->Index*/);

				// Ensure this clip is indexed in sparse mode
				if((i == 0) && UseIndex && SparseIndex)
				{
					// Force the first edit unit to be accepted, even if provisional, 
					// and add it's edit unit to the sparse list for this BodySID
					Position EditUnit = (*BodyWrapList_it).Config->WrapOpt->Handler->AcceptProvisional();
					if(EditUnit == -1) EditUnit = (*BodyWrapList_it).Config->WrapOpt->Handler->GetLastNewEditUnit();
					SparseList[(*BodyWrapList_it).BodySID-1].push_back(EditUnit);
//printf("a: Add %d %d\n", (*BodyWrapList_it).BodySID-1, (int)EditUnit);
				}

				(*BodyWrapList_it).Writer->AddEssenceData((*BodyWrapList_it).EssenceID, Source);
			}
			else
			{
				DataChunkPtr Dat = (*BodyWrapList_it).Config->WrapOpt->Handler->Read((*BodyWrapList_it).InFile, (*BodyWrapList_it).Config->Stream, Dur/*, (*BodyWrapList_it).Writer->Index*/);

				if(Dat->Size == 0)
				{
					Done = true;
					break;
				}
				else Done = false;

				if((i == 0) && SparseIndex && (ThisEditUnit == 0))
				{
					// Force the first edit unit to be accepted, even if provisional,
					// and add it's edit unit to the sparse list for this BodySID
					Position EditUnit = (*BodyWrapList_it).Config->WrapOpt->Handler->AcceptProvisional();
					if(EditUnit == -1) EditUnit = (*BodyWrapList_it).Config->WrapOpt->Handler->GetLastNewEditUnit();
					SparseList[(*BodyWrapList_it).BodySID-1].push_back(EditUnit);
//printf("b: Add %d %d\n", (*BodyWrapList_it).BodySID-1, (int)EditUnit);
				}

				(*BodyWrapList_it).Writer->AddEssenceData((*BodyWrapList_it).EssenceID, Dat);
			}

			// Only allow starting a new partition by size or duration on first essence of a set
			if(i == 0)
			{
				if((*BodyWrapList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Frame)
				{
					if((*BodyWrapList_it).BodyMode == BodyWrapping::Body_Size)
					{
						Int64 NewPartitionSize = PartitionSize + (*BodyWrapList_it).Writer->CalcWriteSize();
//@	printf("%d,",PartitionSize);
						if((!EditAlign) || ((*BodyWrapList_it).Config->WrapOpt->Handler->SetOption("EditPoint")))
						{
							if(NewPartitionSize > (*BodyWrapList_it).BodyRate)
							{
								// Force a new partition pack by clearing CurrentBodySID
								CurrentBodySID = 0;
								
								if(SparseIndex && (ThisEditUnit != 0))
								{
									// Force the first edit unit of the new partition to be accepted, even if provisional,
									// and add it's edit unit to the sparse list for this BodySID
									Position EditUnit = (*BodyWrapList_it).Config->WrapOpt->Handler->AcceptProvisional();
									if(EditUnit == -1) EditUnit = (*BodyWrapList_it).Config->WrapOpt->Handler->GetLastNewEditUnit();
									SparseList[(*BodyWrapList_it).BodySID-1].push_back(EditUnit);
//printf("c: Add %d %d\n", (*BodyWrapList_it).BodySID-1, (int)EditUnit);
								}
//@	printf("##\n");
							}
						}
					}

					if((*BodyWrapList_it).BodyMode == BodyWrapping::Body_Duration)
					{
						if(Dur) PartitionSize += Dur;
						else PartitionSize++;			// DRAGONS: What should we do here?

//@	printf("%d,",PartitionSize);
						if((!EditAlign) || ((*BodyWrapList_it).Config->WrapOpt->Handler->SetOption("EditPoint")))
						{
							if(PartitionSize >= (*BodyWrapList_it).BodyRate)
							{
								// Force a new partition pack by clearing CurrentBodySID
								CurrentBodySID = 0;

								if(SparseIndex && (ThisEditUnit != 0))
								{
									// Force the first edit unit of the new partition to be accepted, even if provisional,
									// and add it's edit unit to the sparse list for this BodySID
									Position EditUnit = (*BodyWrapList_it).Config->WrapOpt->Handler->AcceptProvisional();
									if(EditUnit == -1) EditUnit = (*BodyWrapList_it).Config->WrapOpt->Handler->GetLastNewEditUnit();
									SparseList[(*BodyWrapList_it).BodySID-1].push_back(EditUnit);
//printf("d: Add %d %d\n", (*BodyWrapList_it).BodySID-1, (int)EditUnit);
								}
//@	printf("##\n");
							}
						}
					}
				}
			}

			// Start a new partition if required
			if(CurrentBodySID != (*BodyWrapList_it).BodySID)
			{
				DataChunkPtr IndexChunk = new DataChunk();

				// Perform any index table building work for a sprinkled index
				if(SprinkledIndex)
				{
					IndexManagerPtr Manager = (*BodyWrapList_it).Config->WrapOpt->Handler->GetIndexManager();

					if(Manager)
					{
						IndexTablePtr Index = Manager->MakeIndex();

						if(Index)
						{
							Position EditUnit = Manager->GetLastNewEditUnit();

							Manager->AddEntriesToIndex(Index, LastEditUnit[(*BodyWrapList_it).BodySID], EditUnit-1);

							LastEditUnit[(*BodyWrapList_it).BodySID] = EditUnit;

							Index->WriteIndex(*IndexChunk);

							// Clear all the used entries if no longer required
							// DRAGONS: Not implemented!

							// Set the IndexSID from the index itself
							ThisPartition->SetUint("IndexSID",  Index->IndexSID);
						}
					}
				}

//@printf("Body Part at 0x%08x\n", (int)Out->Tell());
				CurrentBodySID = (*BodyWrapList_it).BodySID;
				PartitionSize = 0;

				Int64 Pos = Out->Tell();
				if(IndexChunk->Size)
				{
					if(IsolatedIndex)
					{
						ThisPartition->ChangeType("ClosedCompleteBodyPartition");
						ThisPartition->SetUint("BodySID", 0);
						ThisPartition->SetUint("BodyOffset", 0);
						Out->WritePartitionWithIndex(ThisPartition, IndexChunk, false);

						ThisPartition->SetUint("BodySID", CurrentBodySID);
						ThisPartition->SetUint64("BodyOffset",(*BodyWrapList_it).Writer->GetStreamOffset());
						ThisPartition->SetUint("IndexSID",  0);
						Out->WritePartition(ThisPartition, false);
					}
					else
					{
						ThisPartition->SetUint("BodySID", CurrentBodySID);
						ThisPartition->SetUint64("BodyOffset",(*BodyWrapList_it).Writer->GetStreamOffset());
						Out->WritePartitionWithIndex(ThisPartition, IndexChunk, false);
					}
				}
				else
				{
					ThisPartition->SetUint("BodySID", CurrentBodySID);
					ThisPartition->SetUint64("BodyOffset",(*BodyWrapList_it).Writer->GetStreamOffset());
					ThisPartition->SetUint("IndexSID",  0);
					Out->WritePartition(ThisPartition, false);
				}

				// If partitioning by size take into account the partition pack size
				if(BodyMode == BodyWrapping::Body_Size) PartitionSize = Out->Tell() - Pos;
			}

			// Fix index table stream offsets
			if((UseIndex || SparseIndex || SprinkledIndex) && ((*BodyWrapList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Frame))
			{
//@printf("Fixing index: %d ", ThisEditUnit);
				(*BodyWrapList_it).Config->WrapOpt->Handler->OfferStreamOffset(ThisEditUnit, (*BodyWrapList_it).Writer->GetStreamOffset());
			}

//@printf("Frame at 0x%08x - ", (int)Out->Tell());
			Int64 Pos = Out->Tell();
			(*BodyWrapList_it).Writer->StartNewCP();
			if((*BodyWrapList_it).BodyMode == BodyWrapping::Body_Size) PartitionSize += (Out->Tell() - Pos);

			// Determine the duration of this item if it was clip-wrapped and if we don't yet know the duration
			if(Ret == 0)
			{
				if((*BodyWrapList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Clip)
				{
					Ret = (*BodyWrapList_it).Config->WrapOpt->Handler->GetCurrentPosition();
				}
			}

//@printf("0x%08x\n", (int)Out->Tell());
			BodyWrapList_it++;
			i++;
		}

		ThisEditUnit++;

		if(Done) break;

		Ret++;
	}

	// DRAGONS - The indexing needs work!!
	//			 The entire index table is built before committing
	//			 It doesn't index clip wrapped essence
	return Ret;
}






