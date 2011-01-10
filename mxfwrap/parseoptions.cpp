/*! \file	parsoptions.cpp
 *	\brief	Parse MXFWrap commandline options
 *
 *	\version $Id: parseoptions.cpp,v 1.2 2011/01/10 11:00:50 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2010, Metaglue Corporation
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

#include "parseoptions.h"

#include "mxflib/mxflib.h"
using namespace mxflib;
using namespace std;

#include "mxflib/system.h" // for stricmp/strcasecmp


#include "libprocesswrap/process.h"


#include <stdio.h>
#include <iostream>

#ifdef COMPILED_DICT
const bool DefaultCompiledDict = true;
#else
const bool DefaultCompiledDict = false;
#endif // COMPILED_DICT


//Import a UUID from a character string
bool ParseUUID(const char * inputUUID, byte * buffer)
{
	int index=0;
	bool firstNibble=true;
	byte b=0;
	byte nibble=0;
	const char * pch=inputUUID;
	while(*pch!='\0')
	{
		if(*pch=='0' && (*(pch+1)=='x' || *(pch+1)=='X')) //check for special case 0x 
		{
			pch+=2;
			continue;
		}
		if(*pch=='i' && *(pch+1)=='d' ) //check for special case uuid: 
		{
			pch+=2;
			continue;
		}
		nibble=0xff; //set to impossible value
		if(*pch>='0' && *pch<='9' )
			nibble=*pch-'0';
		else if(*pch>='A' && *pch<='F' ) 
			nibble=*pch-'A'+10;
		else if(*pch>='a' && *pch<='f' ) 
			nibble=*pch-'a'+10;
		if(nibble!=0xff) //i.e. we have found a valid hex digit
		{
			if(firstNibble)
			{
				b=nibble<<4;
				firstNibble=false;
			}
			else
			{
				b+=nibble;
				firstNibble=true;
				buffer[index++]=b;
				b=0;
			}
		}

		if(index>17) //make sure we don't run over the buffer end
			index=17;
		pch++;
	}

	if(index==16) //i.e. there were 16 hex number found
		return true;

	memset(buffer,0,16); //not valid so put in a zero UUID
	return false;
}

//! Print usage help
void HelpText()
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
		printf("    -1         = Use a version 1 KLVFill item key\n");
		printf("    -a[2]      = Force OP-Atom (optionally with only 2 partitions if VBR)\n");
		printf("    -c=<file>  = Read commandline from file (incomplete - MUCH TODO)\n");
		printf("    -c=<num>   = Demultiplex multi-channel audio to <num> channels or less\n");
		printf("    -c=<n>:<m> = Demultiplex multi-channel audio to n channels, each m-bits\n");
		printf("    -e         = Only start body partitions at edit points\n");

		printf("    -dp=<path> = Path to metadata dictionaries\n" );
		printf("    -dd=<name> = Use DM dictionary\n" );

		if( DefaultCompiledDict )
			printf("    -dc        = Use compiled dictionary instead of xml dictionary\n" );
		else
			printf("    -dc        = Use xml dictionary instead of compiled dictionary\n" );

		printf("    -do=<name> = Use alternative orthodox dictionary\n" );


		printf("    -mm=<name> = Add Metadata Track to Material Package\n");

		// following is not implemented yet
		// printf("    -mp?        = Add a physical source package derived from input file(s) \n" );
		// following is not implemented yet
		// printf("    -mp?=<name> = Add a physical source package derived from <name> \n" );


		printf("    -mz        = Enable DM Custom\n");

		printf("    -n         = Use negative indexing during pre-charge (aligns 0 with start)\n");
		printf("    -f         = Frame-wrap and group in one container\n");
		printf("    -f0        = Frame-wrap and group in one container, padding streams that end early\n");
		printf("    -hp=<size> = Leave at least <size> bytes of expansion space in the header (-h deprecated)\n");
		printf("    -hs=<size> = Make the header at least <size> bytes\n");
		printf("    -i         = Write index tables (at the end of the file)\n");
		printf("    -ip        = Write sparse index tables with one entry per partition\n");
		printf("    -is        = Write index tables sprinkled one section per partition\n");
		printf("    -ii        = Isolated index tables (don't share partition with essence)\n");
		printf("    -ii2       = Isolated index tables (don't share with essence or metadata)\n");
		printf("    -ka=<size> = Set KAG size (default=1) (-k deprecated)\n");
		printf("    -pd=<dur>  = Body partition every <dur> frames\n");
		printf("    -ps=<size> = Body partition roughly every <size> bytes\n");
		printf("                 (early rather than late)\n");
		printf("    -fr=<n>/<d>= Force edit rate (if possible) (-r deprecated, but allowed for legacy\n");
		printf("    -s         = Interleave essence containers for streaming\n");
		printf("    -kxs       = Use 377-2 KLV Extension Syntax (KXS) including only extensions beyond the baseline\n");
		printf("    -u         = Update the header after writing footer\n");
		printf("    -v         = Verbose mode\n");
		printf("    -w         = List available wrapping options (does not build a file)\n");
		printf("    -w=<num>   = Use wrapping option <num>\n");
		printf("    -z         = Pause for input before final exit\n");
}

int EvaluateConfigurationfromFile( const char * filename , int &argc, char **argv, int curarg, ProcessOptions *pOpt);


//! Parse the command line options
/*!	\return true if all parsed ok, false if an error or none supplied */
int ParseOptions(int &argc, char **argv, ProcessOptions *pOpt)
{
	//! Should we pause before exit?
	int PauseBeforeExit = 0;

	// iterate over argv
	int i;
	for(i=1; i<argc;)
	{
		if (IsCommandLineSwitchPrefix(argv[i][0]))
		{
			// use command-line style of decoding options
			char *p = &argv[i][1];					// The option less the '-'
			char Opt = tolower(*p);					// The option itself (in lower case)

			char *Val = "";							// Any value attached to the option
			if(strlen(p) > 2) Val = &p[2];			// Only set a value if one found

			// deal with -c=filename to get config from file
			// DRAGONS: exclude filenames that start with digit, to allow legacy use of -c= for channel-splitting
			if( Opt == 'c' && strlen(Val) && !isdigit(Val[0]) )
			{
				return EvaluateConfigurationfromFile( Val, argc, argv, i, pOpt );
			}

			else if((Opt == 'r') || ( 0==strncmp("fr",p,strlen("fr")) ))
			{
				// -r is now deprecated; use -fr instead
				int N, D; // Use ints in case Int32 is not the same size as "int" on this platform
				if(sscanf(Val,"%d/%d", &N, &D) == 2)
				{
					pOpt->ForceEditRate.Numerator = N;
					pOpt->ForceEditRate.Denominator = D;
				}
				else
				{
					error("Invalid edit rate format \"%s\"\n", Val);
				}

			}
			else if( 0==strncmp("kxs",p,3) )
			{
				SetFeature(FeatureSaveMetadict);
				SetFeature(FeatureKXSMetadict);
			}
			else if(Opt == 'a') 
			{
				pOpt->OPAtom = true;

				// See if the user has requested for a 2-partition OP-Atom file
				if(p[1] == '2') pOpt->OPAtom2Part = true;
			}
			else if(Opt == '1') 
			{
				SetFeature(FeatureVersion1KLVFill);
			}
			else if(Opt == 'p')
			{
				// The value is further along as we are using a 2-byte option
				Val++;
				if(tolower(p[1]) == 'd')
				{
					char *temp;
					pOpt->BodyMode = Body_Duration;
					pOpt->BodyRate = strtoul(Val, &temp, 0);
				}
				else if(tolower(p[1]) == 's')
				{
					char *temp;
					pOpt->BodyMode = Body_Size;
					pOpt->BodyRate = strtoul(Val, &temp, 0);
				}
				else error("Unknown body partition mode '%c'\n", p[1]);
			}
			else if(Opt == 'e') pOpt->EditAlign = true;
			else if(Opt == 'f') 
			{
				pOpt->FrameGroup = true;
				if(p[1] == '0') pOpt->ZeroPad = true;
			}
			else if(Opt == 's') pOpt->StreamMode = true;
			else if(Opt == 'i')
			{
				if(tolower(p[1]) == 'i')
				{
					pOpt->IsolatedIndex = true;
					if(p[2] == '2') pOpt->VeryIsolatedIndex = true;
				}
				else if(tolower(p[1]) == 'p')
				{
					pOpt->SparseIndex = true;
				}
				else if(tolower(p[1]) == 's')
				{
					pOpt->SprinkledIndex = true;
				}
				else
				{
					pOpt->UseIndex = true;
				}
			}
			else if(Opt == 'n')		// Negative indexing
			{
				SetFeature(FeatureNegPrechargeIndex);
			}
			else if(Opt == 'c') 
			{
				unsigned int Limit = 0;
				unsigned int Bits = 0;

				// Read the parameters
				sscanf(Val, "%u:%u", &Limit, &Bits);

				pOpt->AudioLimit = Limit;
				pOpt->AudioBits = Bits;
			}
			else if(Opt == 'h') 
			{
				char *temp;
				if(tolower(p[1]) == 's')
				{
					// -hs for header size
					Val++;
					pOpt->HeaderSize = strtoul(Val, &temp, 0);
				}
				else if(tolower(p[1]) == 'p')
				{
					// -hp for header padding
					Val++;
					pOpt->HeaderPadding = strtoul(Val, &temp, 0);
				}
				else // -h legacy
					pOpt->HeaderPadding = strtoul(Val, &temp, 0);
			}
			else if(Opt == 'k') 
			{
				char *temp;
				if(tolower(p[1]) == 'a')
				{
					// -ka for KAG
					Val++;
					pOpt->KAGSize = strtoul(Val, &temp, 0);
				}
				else // -k legacy
					pOpt->KAGSize = strtoul(Val, &temp, 0);
			}
			else if(Opt == 'u') 
			{
				pOpt->UpdateHeader = true;
			}
			else if(Opt == 'w') 
			{
				char *temp;
				char *name=(p+1); // default name
				if( '='==*(p+1) || ':'==*(p+1))	name=p+2; // explicit name

				if(true) // For later expansion...
				{
					pOpt->SelectedWrappingOption = strtoul(name, &temp, 0);
					pOpt->SelectedWrappingOptionText = name;
				}
			}
			else if(Opt == 'd') // dictionary
			{
				// -dp=DictionaryPath
				if(tolower(*(p+1))=='p')
				{
					char *name=""; // default name
					if( '='==*(p+2) || ':'==*(p+2))	name=p+3; // explicit name
					else if( i+1<argc ) name=argv[++i]; // explicit name in next arg

					if( strlen(name) ) 
					{
						SetDictionaryPath( name );
					}
				}
				// -dc Use compiled dictionary
				else if(tolower(*(p+1))=='c')
				{
					pOpt->OverrideDictionary = true;
				}
				// -do=Orthodox Dictionary
				else if(tolower(*(p+1))=='o')
				{
					char *name=""; // default name
					if( '='==*(p+2) || ':'==*(p+2))	name=p+3; // explicit name
					else if( i+1<argc ) name=argv[++i]; // explicit name in next arg

					if( strlen(name) ) 
					{
						pOpt->OrthodoxDict = name;
					}
				}
				// -dd=Descriptive Dictionary
				else if(tolower(*(p+1))=='d')
				{
					char *name=""; // default name
					if( '='==*(p+2) || ':'==*(p+2))	name=p+3; // explicit name
					else if( i+1<argc ) name=argv[++i]; // explicit name in next arg

					if( strlen(name) ) 
					{
						pOpt->DMDicts.push_back( std::string(name) );

					}
				}
			}
			else if(Opt == 'v') pOpt->DebugMode = true;
			else if(Opt == 'z') 
			{
				PauseBeforeExit = 1;
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
		HelpText();
		return false;
	}



	char *ps;
	
	//now sort out Infilename
	pOpt->InFileGangCount = 1;
	pOpt->InFileGangSize = 1;

	strncpy(pOpt->InFilenameSet, argv[1], sizeof(pOpt->InFilenameSet)-1);

	int InCount = 0;
	ps = pOpt->InFilenameSet;
	for(;;)
	{
		char *LastDot = NULL;
		char *pd = pOpt->InFilename[InCount];
		bool InBracket = false;

		// Locate the end of the input filename (processing any special codes)
		// TODO: Add escape character to allow funny filenames
		int CntLen=0;
		while(*ps)
		{
			if(*ps == '.') LastDot = ps;
			if(*ps == '[') InBracket = true;
			if(*ps == ']') InBracket = false;
			if(*ps == ',') { pOpt->InFileGangCount++; break; }
			if(*ps == '+' && (!InBracket))
			{
				if(pOpt->InFileGangCount == 1) pOpt->InFileGangSize++;
				break;
			}

			*pd++ = *ps++;
			if(CntLen++ > pOpt->MaxFilenameLen-2)
			{	//filename too long - do something approximately sensible and abort
				*pd='\0';
				break;
			}
		};
		*pd = '\0';
		InCount++;

		// If all files processed end scan
		if(*ps == '\0') break;

		// Otherwise we found ',' or '+' so skip it
		ps++;
	}

	//now sort out Outfilename
	strncpy(pOpt->OutFilenameSet, argv[2], 510);
	pOpt->OutFileCount = 0;
	ps = pOpt->OutFilenameSet;
	for(;;)
	{
		char *LastDot = NULL;
		char *pd = pOpt->OutFilename[pOpt->OutFileCount];

		// Find the position of last dot in the input filename
		while(*ps)
		{
			if(*ps == '.') LastDot = ps;
			if(*ps == ',') { break; }
			if(*ps == '+') { break; }
			*pd++ = *ps++;
		};
		*pd = '\0';
		pOpt->OutFileCount++;

		// If input filename specified no extension add ".mxf"
		if(LastDot == NULL)	strcpy(pd, ".mxf");

		// If all files processed end scan
		if(*ps == '\0') break;

		// Otherwise we found ',' or '+' so skip it
		ps++;
	}


	// Detail the options

	debug("** Verbose Mode **\n\n");
	
	printf("KAGSize     = %u\n\n", pOpt->KAGSize);
	
	if(pOpt->InFileGangSize == 1)
	{
		if(pOpt->InFileGangCount == 1) printf("Input file  = %s\n", pOpt->InFilename[0]);
		else
		{
			printf("Input files = ");
			int i;
			for(i=0; i<pOpt->InFileGangCount; i++) 
			{ 
				if(i != 0) printf(" then ");
				printf("%s", pOpt->InFilename[i]);
			}
			printf("\n");
		}
	}
	else
	{
		if(pOpt->InFileGangSize != 0)
		{
			printf("Input files = ");
	
			int i;
			for(i=0; i<pOpt->InFileGangCount; i++) 
			{ 
				if(i != 0) printf(" followed by: ");
				int j;
				for(j=0; j<pOpt->InFileGangSize; j++) 
				{ 
					if(j != 0) printf(" with ");
					printf("%s", pOpt->InFilename[i*pOpt->InFileGangSize + j]);
				}
				printf("\n");
			}
			if(pOpt->InFileGangCount > 1) printf("\n");
		}
	}

	if(pOpt->OutFileCount == 1)
	{
		printf("Output file = %s\n\n", pOpt->OutFilename[0]);
	}
	else
	{
		printf("Output files = ");
		int i;
		for(i=0; i<pOpt->OutFileCount; i++) 
		{ 
			if(i != 0) printf(" with ");
			printf("%s", pOpt->OutFilename[i]);
		}
		printf("\n");
	}

	if((pOpt->SelectedWrappingOption >= 0) && (pOpt->InFileGangCount * pOpt->InFileGangSize) != 1)
	{
		error("Selection of wrapping options only currently available with single input files\n");
		//pOpt->SelectedWrappingOption = -1;
	}


	if(pOpt->OPAtom)
	{
		if(pOpt->OPAtom2Part)	printf("Output OP = OP-Atom (with only 2 partitions if VBR)\n");
		else printf("Output OP = OP-Atom\n");

		// We will need to update the header
		pOpt->UpdateHeader = true;

		pOpt->OPUL = OPAtomUL;

		if((pOpt->InFileGangCount * pOpt->InFileGangSize) != pOpt->OutFileCount) error("OP-Atom can only output a single essence container per file so requires as many output files as input files\n");
		
		if(pOpt->BodyMode != Body_None) 
		{
			warning("Splitting essence across body partitions is forbidden in OP-Atom\n");
			pOpt->BodyMode = Body_None;
		}

		// Force mandatory index table for OP-Atom
		pOpt->UseIndex = true;
		pOpt->IsolatedIndex = true;
	}
	else
	{
		if((pOpt->FrameGroup) || (pOpt->InFileGangSize == 1))
		{
			if(pOpt->InFileGangCount == 1) { printf("Output OP = OP1a\n"); pOpt->OPUL = OP1aUL; }
			else { printf("Output OP = OP2a\n"); pOpt->OPUL = OP2aUL; }
		}
		else
		{
			if(pOpt->InFileGangCount == 1) { printf("Output OP = OP1b\n"); pOpt->OPUL = OP1bUL; }
			else { printf("Output OP = OP2b\n"); pOpt->OPUL = OP2bUL; }
		}
	}

	if(pOpt->AudioLimit)
	{
		if(pOpt->AudioLimit == 1)
            printf("Audio streams will be demultiplexed to single channels\n");
		else
			printf("Audio streams will be demultiplexed to %u channels (or less)\n", pOpt->AudioLimit);

		if(pOpt->AudioBits != 0)
			printf("Audio streams will be forced to be %u-bit\n", pOpt->AudioBits);
	}

	if(pOpt->UpdateHeader)
	{
		printf("An updated header will be written after writing the footer\n");

		// We may need some extra space in the header
		if(pOpt->HeaderPadding<EmpiricalSmallestHeader && pOpt->HeaderSize<EmpiricalSmallestHeader)
		{
			pOpt->HeaderPadding = EmpiricalSmallestHeader;
			printf("Header padding has been increased to the empirical minimum: %d bytes\n", pOpt->HeaderPadding);
		}
	}

	if(Feature(FeatureSaveMetadict))
	{
		if(Feature(FeatureFullMetadict))
		{
			printf("A Metadictionary will be written containing all known sets, properties and types\n");
		}
		else if(Feature(FeatureUsedMetadict))
		{
			printf("A Metadictionary will be written containing all sets, properties and types used in the file\n");
		}
		else
		{
			printf("A Metadictionary will be written containing all extension sets, properties and types\n");
		}

		if(Feature(FeatureKXSMetadict))
		{
			printf("Note: KLV Encoded Extension Syntax 3c will be used\n");
		}
	}

	if(pOpt->HeaderPadding)
	{
		if(pOpt->UpdateHeader) printf("At least %d padding bytes will be left after the initial writing of the header\n", pOpt->HeaderPadding);
		else printf("At least %d padding bytes will be left after writing the header\n", pOpt->HeaderPadding);
	}

	if(pOpt->HeaderSize)
	{
		printf("The header will be at least %d bytes long\n", pOpt->HeaderSize);
	}

	if(pOpt->StreamMode && (pOpt->InFileGangSize == 1))
	{
		warning("Essence containers will not be interleaved for streaming as none are ganged\n");
		pOpt->StreamMode = false;
	}

	if(pOpt->StreamMode) 
	{
		printf("Essence containers will be interleaved for streaming\n");

		if(pOpt->BodyMode != Body_None) 
		{
			warning("Body partitions will be inserted for interleaving - this overrides other body partitioning options\n");
			pOpt->BodyMode = Body_None;
		}

		error("Stream mode not yet supported\n");
	}
	else
	{
		if(pOpt->BodyMode == Body_Duration)
		{
			if(pOpt->EditAlign)
				printf("A new body partition will be inserted at the first new GOP after each %d frame%s\n", pOpt->BodyRate, pOpt->BodyRate==1 ? "" : "s");
			else
				printf("A new body partition will be inserted every %d frame%s\n", pOpt->BodyRate, pOpt->BodyRate==1 ? "" : "s");
		}

		if(pOpt->BodyMode == Body_Size)
			printf("Partitions will be limited to %d byte%s (if possible)\n", pOpt->BodyRate, pOpt->BodyRate==1 ? "" : "s");
	}

	if(pOpt->UseIndex) printf("Index tables will be written for each frame wrapped essence container\n");
	if(pOpt->SprinkledIndex) 
	{
		if(pOpt->UseIndex) printf("Index tables will also be sprinkled across partitions for each frame wrapped container\n");
		else		 printf("Index tables will be sprinkled across partitions for each frame wrapped essence container\n");
	}
	if(pOpt->SparseIndex) 
	{
		if(pOpt->UseIndex || pOpt->SprinkledIndex) printf("Sparse index tables will also be written for each frame wrapped container\n");
		else						   printf("Sparse index tables will be written for each frame wrapped essence container\n");
	}

	if((pOpt->UseIndex || pOpt->SparseIndex || pOpt->SprinkledIndex) && (pOpt->IsolatedIndex))
	{
		if(pOpt->VeryIsolatedIndex) printf("Index table segments will not share a partition with essence or metadata\n");
		else printf("Index table segments will not share a partition with essence\n");
	}

	if( Feature(FeatureNegPrechargeIndex) )
	{
		printf("Negative indexing will be used in any pre-charge\n");
	}

	if(Feature(FeatureVersion1KLVFill))
	{
		printf("KLVFill items will be written with a version 1 key, for compatibility\n");
	}

	// Check for stray parameters as a space in the wrong place can otherise cause us to overwrite input files!
	if(argc > 3)
	{
		printf("\nThere appear to be too many filenames on the command line\n");
		return -1;
	}

	return PauseBeforeExit;
}

//! Get config from file
// DRAGONS mutually recursive with ParseOptions()
// - only the deepest nested ParseOptions() gets to do the real work
int EvaluateConfigurationfromFile( const char * filename , int &eargc, char **eargv, int curarg, ProcessOptions *pOpt)
{
	int ret=0;	
	int argc=1; //start from 1 cos when the args come from the command line the 0th arg is the filename
	char ** argv=NULL;
	char *pConfigFromFile;
	FILE * ip=NULL;

	try{
		ip=fopen(filename,"r");
		if(ip==NULL)
			throw( "Unable to open input command file");

		fseek(ip,0,SEEK_END);
		size_t flen=ftell(ip);
		fseek(ip,0,SEEK_SET);

		pConfigFromFile=new char[flen+2];
		if(pConfigFromFile==NULL)
			throw( "Out of memory");


		size_t nread=fread(pConfigFromFile+1,1,flen,ip);
		fclose(ip);
		ip=NULL;

		pConfigFromFile[0]=' ';//to ensure that the string starts with a space
		pConfigFromFile[nread+1]='\0';  //make sure it ends with terminating char

		char * pch=pConfigFromFile+1;

		int maxPossibleArgs=1;
		while (*pch!='\0')  //don't worry about quoted items for simplicity and speed at this point
		{
			if(isspace(*(pch-1)) && !isspace(*pch))
				maxPossibleArgs++;
			pch++;
		}

		argv=new char *[maxPossibleArgs];
		memset(argv,0, sizeof(char *)*maxPossibleArgs);


		argc=1;
		pch=pConfigFromFile+1;
		
		//note - to save many nested loops and tests - or many exit points of code, gotos are used to exit the outer loop
		//this is one of  the few (only?) cases where goto is OK
		while (*pch!='\0')
		{
			while (isspace(*pch))
			{
				pch++;
				if(*pch=='\0')
					goto foundendofstring;
				if(*pch=='#')
				{
					while(!(*pch=='\n' || *pch=='\0')) //traverse comment which must be space followed by #
						pch++;
				}
			}
			if(*pch=='#') //comment at start of line
			{
				while(!(*pch=='\n' || *pch=='\0')) //traverse comment which must be space followed by #
					pch++;
			}
			else if(*pch=='"') //in quotes
			{
				pch++;
				argv[argc++]=pch;
				while (*pch!='"') //look for endquote
				{
					pch++;
					if(*pch=='\0')
					{ //unmatched quotes
						throw( "Error matching quotes");
						
					}	
				}
				*pch='\0'; //terminate string;
				pch++; //skip endquote
				if(*pch=='\0')
					goto foundendofstring;
				if(!isspace(*pch))
					throw( "Error matching quotes");
			}
			else
			{
				argv[argc++]=pch;
				while (!isspace(*pch)) //traverse argument
				{
					pch++;
					if(*pch=='\0')
						goto foundendofstring;
				}
				*pch='\0'; //terminate string;
				pch++;
			}


		}
foundendofstring:
		;

		// TODO inherit argv[0]
		// TODO append inherited remaining arguments

		// echo 
		// TODO move this into ParseOptions
		// TODO give choice to continue or abandon
		for( int i=0; i<argc; i++ )
		{
			printf( "<arg id=\"%d\">%s</arg>\n", i, argv[i] );
		}
		printf( "\n" );

		// now actually do the parsing
		ret=ParseOptions( argc,  argv, pOpt );
	}
	catch (char * message) 
	{
		printf("Error parsing command file: %s\n",message);
		if(ip)
			fclose(ip);
		if(argv)
			delete[] argv; 
		return -3;


	}
	catch (...) 
	{
		puts("Unexpected error parsing command file");
		if(ip)
			fclose(ip);
		if(argv)
			delete[] argv; 
		return -3;

	}
	
	if(argv)
		delete[] argv; 

	if(pConfigFromFile!=NULL)
		delete[] pConfigFromFile;

	return ret;

}

