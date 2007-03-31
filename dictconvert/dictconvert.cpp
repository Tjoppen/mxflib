/*! \file	dictconvert.cpp
 *	\brief	Convert an XML dictionary file to compile-time definitions
 *
 *	\version $Id: dictconvert.cpp,v 1.10 2007/03/31 14:23:00 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2005, Matt Beard
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


//! MXFLib debug flag
static bool DebugMode = false;

/* XML functions */
void Convert_startElement(void *user_data, const char *name, const char **attrs);
void Convert_endElement(void *user_data, const char *name);
void Convert_warning(void *user_data, const char *msg, ...);
void Convert_error(void *user_data, const char *msg, ...);
void Convert_fatalError(void *user_data, const char *msg, ...);


//! Should we pause before exit?
bool PauseBeforeExit = false;

//! Name of the structure to build
std::string UseName = "DictData";

//! The number of (new style) types sections found
int TypesCount = 0;

//! The number of (new style) classes sections found
int ClassesCount = 0;

//! Name of the file to be converted
char *InputFile = "";

//! Should we output UL consts?
bool ULConsts = true;

//! Should we only output UL consts?
bool OnlyConsts = false;

//! Should UL consts always be long-form?
bool LongFormConsts = false;

//! Namespace for defining UL constants
std::string ULNamespace = "mxflib";


// Declare main process function
int main_process(int argc, char *argv[]);

//! Do the main processing and pause if required
int main(int argc, char *argv[]) 
{ 
	int Ret = main_process(argc, argv);

	if(PauseBeforeExit) PauseForInput();

	return Ret;
}

namespace
{
	class ULData;
	typedef SmartPtr<ULData> ULDataPtr;

	//! Class holding information about an item that is UL indexed (class, class member or type)
	class ULData : public RefCount<ULData>
	{
	public:
		std::string Name;
		std::string Detail;
		ULDataPtr Parent;
		bool IsSet;
		bool IsPack;
		bool IsMulti;
		bool IsType;
		ULPtr UL;
		Tag LocalTag;

	public:
		ULData() : IsSet(false), IsPack(false), IsMulti(false), IsType(false) {};
	};

	typedef std::map<std::string, ULDataPtr> ULDataMap;
	typedef std::list<ULDataPtr> ULDataList;

	ULDataMap ULMap;
	ULDataList ULFixupList;
};


//! State-machine state for XML parsing
enum CurrentState
{
	StateIdle = 0,								//!< Processing not yet started
	StateTypes,									//!< Processing types - not yet processing a types section
	StateTypesBasic,							//!< Processing basic types section
	StateTypesInterpretation,					//!< Processing interpretation types section
	StateTypesMultiple,							//!< Processing multiple types section
	StateTypesCompound,							//!< Processing compound types section
	StateTypesCompoundItem,						//!< Processing sub-items within a compound
	StateTypesEnum,								//!< Processing enumerated types section
	StateTypesEnumValue,						//!< Processing values within an enumeration
	StateTypesLabel,							//!< Processing lablels, possibly inside a types section
	StateClasses,								//!< Processing classes
	StateDone,									//!< Finished processing
	StateError									//!< Error encountered - process nothing else
};

//! State structure for XML parsing
struct ConvertState
{
	CurrentState State;							//!< Current state of the parser state-machine
	std::string Parent;							//!< The name of the current compound or set/pack being built
	std::string Multi;							//!< The name of the current multiple being built (possibly inside a set of pack)
	ULDataPtr ParentData;						//!< The ULData item of the parent set or pack
	ULDataPtr MultiData;						//!< The ULData item of the parent multiple
	FILE *OutFile;								//!< The file being written
	int Depth;									//!< Nesting depth in class parsing
	std::list<std::string> EndTagText;			//!< Text to be output at the next class end tag
	std::map<std::string, std::string> TypeMap;	//!< Map of type for each class - to allow types to be inherited
	std::list<bool> ExtendSubsList;				//!< List of extendSubs flags (explicit and inherited) for each level
	bool FoundType;								//!< Set true once we have determined the dictionary type (old or new)
	bool FoundMulti;							//!< Found new multi-style dictionary
	bool LabelsOnly;							//!< True if this is a labels section rather than a full types section
												// ** DRAGONS: Labels are treated as types rather than defining a third kind (types, classes and labels)

	std::string SymSpace;						//!< The symbol space attribute of the classes tag (stored if deferring the header line)
};


//! Add a ULData item for a type
void AddType(ConvertState *State, std::string Name, std::string Detail, std::string TypeUL);



//! Do the main processing (less any pause before exit)
int main_process(int argc, char *argv[])
{
	// Location of each filename in argv array
	int FileCount = 0;
	int FileArg[32];

	printf( "MXFlib Dictionary Convert\n\n" );

	int num_options = 0;
	for(int i=1; i<argc; i++)
	{
		if(argv[i][0] == '-')
		{
			num_options++;
			if((argv[i][1] == 'v') || (argv[i][1] == 'V'))
				DebugMode = true;
			if((argv[i][1] == 'c') || (argv[i][1] == 'C'))
				OnlyConsts = true;
			if((argv[i][1] == 'l') || (argv[i][1] == 'L'))
				LongFormConsts = true;
			else if((argv[i][1] == 'n') || (argv[i][1] == 'N'))
			{
				if((argv[i][2] == ':') || (argv[i][2] == '=')) UseName = &argv[i][3];
				else if(argv[i][2]) UseName = &argv[i][2];
				else if(argc > (i+1))
				{
					UseName = argv[++i];
					num_options++;
				}
			}
			else if((argv[i][1] == 's') || (argv[i][1] == 'S'))
			{
				if((argv[i][2] == ':') || (argv[i][2] == '=')) UseName = &argv[i][3];
				else if(argv[i][2]) ULNamespace = &argv[i][2];
				else if(argc > (i+1))
				{
					ULNamespace = argv[++i];
					num_options++;
				}
			}
			if((argv[i][1] == 'x') || (argv[i][1] == 'X'))
				ULConsts = false;
			else if((argv[i][1] == 'z') || (argv[i][1] == 'Z'))
				PauseBeforeExit = true;
		}
		else FileArg[FileCount++] = i;
	}

	if (FileCount != 2)
	{
		printf("\nUsage:   %s [options] <inputfile> <outputfile>\n\n", argv[0]);
		printf("Converts input XML dictionary file to a C++ source file containing the same\n");
		printf("items as a compile-time structure for passeing to function LoadDictionary\n\n");
		printf("Options: -c         Only output UL consts\n");
		printf("         -n=name    Use \"name\" as the name of the structure built\n");
		printf("         -l         Always use long-form names for UL consts\n");
		printf("         -s=name    Use \"name\" as the namespace for UL consts\n");
		printf("         -v         Verbose mode - shows lots of debug info\n");
		printf("         -x         Don't output UL consts\n");
		printf("         -z         Pause for input before final exit\n");
		printf("\nNote: It is recommended that supplementary dictionaries either use long-form\n");
		printf("      const names, or define them in a different namespace than \"mxflib\"\n");

		return 1;
	}

	InputFile = argv[FileArg[0]];

	FILE *outfile = fopen(argv[FileArg[1]],"w");
	if(!outfile)
	{
		error("Can't open output file\n");
		return 1;
	}

	// Set up the XML handler
	XMLParserHandler XMLHandler = 
	{
		(startElementXMLFunc) Convert_startElement,			/* startElement */
		(endElementXMLFunc) Convert_endElement,				/* endElement */
		(warningXMLFunc) Convert_warning,					/* warning */
		(errorXMLFunc) Convert_error,						/* error */
		(fatalErrorXMLFunc) Convert_fatalError,				/* fatalError */
	};

	// Initialize the state
	ConvertState State;
	State.State = StateIdle;
	State.OutFile = outfile;
	State.Depth = 0;
	State.FoundType = false;
	State.FoundMulti = false;

	// Parse the file
	bool result = false;
	result = XMLParserParseFile(&XMLHandler, &State, InputFile);

	if((ClassesCount > 0) || (TypesCount > 0))
	{
		fprintf(outfile, "\n");
		fprintf(outfile, "\t// Build a complete dictionary from above types and classes\n");
		fprintf(outfile, "\tMXFLIB_DICTIONARY_START(%s)\n", UseName.c_str());
		int i;
		for(i=1; i<=TypesCount; i++)
		{
			if(i==1)
				fprintf(outfile, "\t\tMXFLIB_DICTIONARY_TYPES(%s_Types)\n", UseName.c_str());
			else
				fprintf(outfile, "\t\tMXFLIB_DICTIONARY_TYPES(%s_Types_%d)\n", UseName.c_str(), i);
		}
		for(i=1; i<=ClassesCount; i++)
		{
			if(i==1)
				fprintf(outfile, "\t\tMXFLIB_DICTIONARY_CLASSES(%s_Classes)\n", UseName.c_str());
			else
				fprintf(outfile, "\t\tMXFLIB_DICTIONARY_CLASSES(%s_Classes_%d)\n", UseName.c_str(), i);
		}
		fprintf(outfile, "\tMXFLIB_DICTIONARY_END\n", UseName.c_str());
	}

	// DRAGONS: We currently cheat with "OnlyConsts" and output everything, but close and reopen the file
	//          before writing the UL consts, which causes the dictionary definitions to be lost!
	if(OnlyConsts)
	{
		fclose(outfile);
		outfile = fopen(argv[FileArg[1]],"w");
		if(!outfile)
		{
			error("Can't open re-output file\n");
			return 1;
		}
	}

	/* Resolve any duplicate names in the UL list */
	if(ULConsts && (ULMap.size() > 0))
	{
		ULDataList::iterator ListIt = ULFixupList.begin();
		while(ListIt != ULFixupList.end())
		{
			printf("\n* Resolving Duplicate %s\n", (*ListIt)->Name.c_str());

			// Must check that any parent is resolved first
			// DRAGONS: Can this ever be required?
			if((*ListIt)->Parent)
			{
				bool ParentUnresolved = false;
                ULDataList::iterator ParentIt = ULFixupList.begin();
				while(ParentIt != ULFixupList.end())
				{
					if((*ParentIt)->Name == (*ListIt)->Parent->Name)
					{
						ParentUnresolved = true;
						break;
					}
					ParentIt++;
				}

				if(ParentUnresolved)
				{
					ULDataPtr ThisData = (*ListIt);
					ULFixupList.erase(ListIt);
					ULFixupList.push_back(ThisData);
					printf("Defering %s as parent, %s, needs resolving first\n", ThisData->Name.c_str(), ThisData->Parent->Name.c_str());
					ListIt = ULFixupList.begin();
					continue;
				}
			}

			// A list of the items we are de-duplicating
			ULDataList WorkingList;
			
			// First extract the copy of the data already in ULMap using this name
			ULDataMap::iterator MapIt = ULMap.find((*ListIt)->Name);
			if(MapIt != ULMap.end())
			{
				WorkingList.push_back((*MapIt).second);
				ULMap.erase(MapIt);
			}

			// Extract all matching names from the fixup list into the working list
			std::string ThisName = (*ListIt)->Name;
			ULDataList::iterator ListIt2 = ULFixupList.begin();
			while(ListIt2 != ULFixupList.end())
			{
				if((*ListIt2)->Name == ThisName)
				{
					WorkingList.push_back(*ListIt2);
					ULDataList::iterator ToErase = ListIt2;
					ListIt2++;
					ULFixupList.erase(ToErase);
				}
				else
				{
					ListIt2++;
				}
			}

			/* First try to de-dulicate by appending Set, Pack, Array etc. */
			int SetCount = 0;
			int PackCount = 0;
			int ArrayCount = 0;
			int BatchCount = 0;
			int ItemCount = 0;
			int TypeCount = 0;
			ListIt2 = WorkingList.begin();
			while(ListIt2 != WorkingList.end())
			{
				if((*ListIt2)->IsSet) SetCount++;
				else if((*ListIt2)->IsPack) PackCount++;
				else if((*ListIt2)->IsMulti)
				{
					if(   ((*ListIt2)->Detail.find("Batch") != std::string::npos)
					   || ((*ListIt2)->Detail.find("batch") != std::string::npos)
					   || ((*ListIt2)->Detail.find("Unordered") != std::string::npos)
					   || ((*ListIt2)->Detail.find("unordered") != std::string::npos) )
					{
						BatchCount++;
					}
					else if(   ((*ListIt2)->Detail.find("Array") != std::string::npos)
							|| ((*ListIt2)->Detail.find("array") != std::string::npos)
							|| ((*ListIt2)->Detail.find("Ordered") != std::string::npos)
							|| ((*ListIt2)->Detail.find("ordered") != std::string::npos) )
					{
						ArrayCount++;
					}
				}
				else if((*ListIt2)->IsType) TypeCount++;
				else if((*ListIt2)->Parent)
				{
					if((*ListIt2)->Parent->IsMulti) ItemCount++;
				}

				ListIt2++;
			}

			// We can use appending if there are no duplicates
			if(   (SetCount <= 1) && (PackCount <= 1) && (ArrayCount <= 1) && (BatchCount <= 1) && (TypeCount <= 1)
			   && (WorkingList.size() - (SetCount + PackCount + ArrayCount + BatchCount + ItemCount + TypeCount) <= 1) )
			{
				ULDataList::iterator ListIt2 = WorkingList.begin();
				while(ListIt2 != WorkingList.end())
				{
					std::string NewName = (*ListIt2)->Name;
					if((*ListIt2)->IsSet) NewName += "Set";
					else if((*ListIt2)->IsPack) NewName += "Pack";
					else if((*ListIt2)->IsMulti)
					{
						if(   ((*ListIt2)->Detail.find("Batch") != std::string::npos)
						   || ((*ListIt2)->Detail.find("batch") != std::string::npos)
						   || ((*ListIt2)->Detail.find("Unordered") != std::string::npos)
						   || ((*ListIt2)->Detail.find("unordered") != std::string::npos) )
						{
							NewName += "Batch";
						}
						else if(   ((*ListIt2)->Detail.find("Array") != std::string::npos)
								|| ((*ListIt2)->Detail.find("array") != std::string::npos)
								|| ((*ListIt2)->Detail.find("Ordered") != std::string::npos)
								|| ((*ListIt2)->Detail.find("ordered") != std::string::npos) )
						{
							NewName += "Array";
						}
					}
					else if((*ListIt2)->IsType) NewName += "Type";
					else if((*ListIt2)->Parent)
					{
						if((*ListIt2)->Parent->IsMulti)
						{
							NewName = (*ListIt2)->Parent->Name + "Item";
						}
					}

					printf("%s -> %s\n", (*ListIt2)->Name.c_str(), NewName.c_str());

					(*ListIt2)->Name = NewName;
					ULMap.insert(ULDataMap::value_type(NewName, (*ListIt2)));

					ListIt2++;
				}
				WorkingList.clear();
			}
			else
			{
				/* Must use fully qualified names */
				ULDataList::iterator ListIt2 = WorkingList.begin();
				while(ListIt2 != WorkingList.end())
				{
					std::string NewName = (*ListIt2)->Name;
					if((*ListIt2)->Parent) NewName = (*ListIt2)->Parent->Name + "_" + NewName;

					ULMap.insert(ULDataMap::value_type(NewName, (*ListIt2)));

					printf("%s -> %s\n", (*ListIt2)->Name.c_str(), NewName.c_str());

					ListIt2++;
				}
				WorkingList.clear();
			}

			// DRAGONS: There is no need to increment ListIt as the first entry will have been removed
			//          We simply restart at the top of the shortened list
			ListIt = ULFixupList.begin();
		}

		/* Issue the list of ULs */

		if(OnlyConsts)
		{
			fprintf(outfile, "\t// Define ULs for the global keys in %s\n", InputFile);
			fprintf(outfile, "\tnamespace %s\n\t{\n", ULNamespace.c_str());
		}
		else
		{
			fprintf(outfile, "\n\n\t// Define ULs for the global keys in this dictionary\n");
			fprintf(outfile, "\tnamespace %s\n\t{\n", ULNamespace.c_str());
		}

		ULDataMap::iterator MapIt = ULMap.begin();
		while(MapIt != ULMap.end())
		{
			fprintf(outfile, "\t\tconst UInt8 %s_UL_Data[16] = { ", (*MapIt).first.c_str());
			int i;
			for(i=0; i<16; i++)
			{
				if(i == 0) fprintf(outfile, "0x%02x", (*MapIt).second->UL->GetValue()[0]);
				else fprintf(outfile, ", 0x%02x", (*MapIt).second->UL->GetValue()[i]);
			}
			fprintf(outfile, " };\n");
			fprintf(outfile, "\t\tconst UL %s_UL(%s_UL_Data);\n\n", (*MapIt).first.c_str(), (*MapIt).first.c_str());

			MapIt++;
		}

		fprintf(outfile, "\t} // namespace %s\n", ULNamespace.c_str());
	}

	return result ? 0 : 1;
}


//! Convert a C-string to a C-source-code string (escape the quotes)
std::string CConvert(const char *str)
{
	std::string Ret;

	const char *p = str;
	while(*p)
	{
		if(*p == '\"') Ret += "\\\"";
		else Ret += *p;
		p++;
	}

	return Ret;
}



//! XML callback - Deal with start tag of an element
void Convert_startElement(void *user_data, const char *name, const char **attrs)
{
	ConvertState *State = (ConvertState*)user_data;

	switch(State->State)
	{
		case StateIdle:
		{
			bool FoundMXFTypes = false;

			if(strcmp(name, "MXFTypes") == 0) 
			{
				FoundMXFTypes = true;
				State->LabelsOnly = false;
			}
			// DRAGONS: We treat MXFLabels as a special case of the types section
			else if((strcmp(name, "MXFLabels") == 0) || (strcmp(name, "Labels") == 0))
			{
				FoundMXFTypes = true;
				State->LabelsOnly = true;
			}

			if(FoundMXFTypes)
			{
				// Types at the outer level is an old style dictionary
				if(!State->FoundType)
				{
					State->FoundType = true;
					State->FoundMulti = false;
				}

				if(State->FoundMulti) TypesCount++;

				// Set state to types - unless we are straight into the labels
				if(State->LabelsOnly) State->State = StateTypesLabel;
				else State->State = StateTypes;

				/* Check for symSpace */
				const char *SymSpace = NULL;
				if(attrs != NULL)
				{
					int this_attr = 0;
					while(attrs[this_attr])
					{
						char const *attr = attrs[this_attr++];
						char const *val = attrs[this_attr++];
						
						if(strcmp(attr, "symSpace") == 0)
						{
							SymSpace = val;
						}
					}
				}

				if((ClassesCount + TypesCount) > 0)	fprintf(State->OutFile, "\n");

				if(State->LabelsOnly)
					fprintf(State->OutFile, "\t// Label definitions converted from file %s\n", InputFile);
				else
					fprintf(State->OutFile, "\t// Types definitions converted from file %s\n", InputFile);

				if(!State->FoundMulti)
				{
					if(!SymSpace)
						fprintf(State->OutFile, "\tMXFLIB_TYPE_START(%s)\n", UseName.c_str());
					else
						fprintf(State->OutFile, "\tMXFLIB_TYPE_START_SYM(%s, \"%s\")\n", UseName.c_str(), SymSpace);
				}
				else if(TypesCount <= 1)
				{
					if(!SymSpace)
						fprintf(State->OutFile, "\tMXFLIB_TYPE_START(%s_Types)\n", UseName.c_str());
					else
						fprintf(State->OutFile, "\tMXFLIB_TYPE_START_SYM(%s_Types, \"%s\")\n", UseName.c_str(), SymSpace);
				}
				else
				{
					if(!SymSpace)
						fprintf(State->OutFile, "\tMXFLIB_TYPE_START(%s_Types_%d)\n", UseName.c_str(), TypesCount);
					else
						fprintf(State->OutFile, "\tMXFLIB_TYPE_START_SYM(%s_Types_%d, \"%s\")\n", UseName.c_str(), TypesCount, SymSpace);
				}
			}
			else if((strcmp(name, "MXFDictionary") == 0) || (strcmp(name, "MXFClasses") == 0))
			{
				State->State = StateClasses;

				/* Check for symSpace */
				const char *SymSpace = NULL;
				if(attrs != NULL)
				{
					int this_attr = 0;
					while(attrs[this_attr])
					{
						char const *attr = attrs[this_attr++];
						char const *val = attrs[this_attr++];
						
						if(strcmp(attr, "symSpace") == 0)
						{
							SymSpace = val;
						}
					}
				}

				// If the tag is MXFClasses we are in a new type dictionary
				if((!State->FoundType) && (strcmp(name, "MXFClasses") == 0))
				{
					State->FoundType = true;
					State->FoundMulti = true;
				}

				// If we already know what type of dictionary this is we can send the header
				if(State->FoundType)
				{
					if(State->FoundMulti) ClassesCount++;

					if((ClassesCount + TypesCount) > 0)	fprintf(State->OutFile, "\n");

					fprintf(State->OutFile, "\t// Class definitions converted from file %s\n", InputFile);
					
					if(!State->FoundMulti)
					{
						if(!SymSpace)
							fprintf(State->OutFile, "\tMXFLIB_CLASS_START(%s)\n", UseName.c_str());
						else
							fprintf(State->OutFile, "\tMXFLIB_CLASS_START_SYM(%s, \"%s\")\n", UseName.c_str(), SymSpace);
					}
					else if(ClassesCount <= 1)
					{
						if(!SymSpace)
							fprintf(State->OutFile, "\tMXFLIB_CLASS_START(%s_Classes)\n", UseName.c_str());
						else
							fprintf(State->OutFile, "\tMXFLIB_CLASS_START_SYM(%s_Classes, \"%s\")\n", UseName.c_str(), SymSpace);
					}
					else
					{
						if(!SymSpace)
							fprintf(State->OutFile, "\tMXFLIB_CLASS_START(%s_Classes_%d)\n", UseName.c_str(), ClassesCount);
						else
							fprintf(State->OutFile, "\tMXFLIB_CLASS_START_SYM(%s_Classes_%d, \"%s\")\n", UseName.c_str(), ClassesCount, SymSpace);
					}
				}
				// Otherwise we store the symspace (if any) for later when we do send the header
				else
				{
					if(SymSpace) State->SymSpace = SymSpace;
				}
			}
			else
			{
				// Allow MXF dictionaries to be wrapped inside other XML files
				debug("Stepping into outer level <%s>\n", name);

//				Convert_fatalError(user_data, "Outer tag <MXFTypes> or <MXFDictionary> expected - <%s> found\n", name);
//				return;
			}

			break;
		}

		case StateTypes:
		{
			if(strcmp(name, "Basic") == 0)
				State->State = StateTypesBasic;
			else if(strcmp(name, "Interpretation") == 0)
				State->State = StateTypesInterpretation;
			else if(strcmp(name, "Multiple") == 0)
				State->State = StateTypesMultiple;
			else if(strcmp(name, "Compound") == 0)
				State->State = StateTypesCompound;
			else if(strcmp(name, "Enumeration") == 0)
				State->State = StateTypesEnum;
			else
				Convert_error(user_data, "Tag <%s> found when types class expected\n", name);

			break;
		}

		case StateTypesBasic:
		{
			std::string Detail;
			std::string TypeUL;
			char *RefType = NULL;
			std::string RefTargetName;
			const char *SymSpace = NULL;

			int Size = 1;
			bool Endian = false;

			/* Process attributes */
			if(attrs != NULL)
			{
				int this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = CConvert(val);
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "endian") == 0)
					{
						if(strcasecmp(val,"yes") == 0) Endian = true;
					}
					else if(strcmp(attr, "ul") == 0)
					{
						TypeUL = CConvert(val);
					}
					else if(strcmp(attr, "symSpace") == 0)
					{
						SymSpace = val;
					}
					else if(strcmp(attr, "ref") == 0)
					{
						if(strcasecmp(val,"strong") == 0) RefType = "ClassRefStrong";
						else if(strcasecmp(val,"target") == 0) RefType = "ClassRefTarget";
						else if(strcasecmp(val,"weak") == 0) RefType = "ClassRefWeak";
						else if(strcasecmp(val,"global") == 0) RefType = "ClassRefGlobal";
						else
						{
							Convert_warning(State, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
							RefType = NULL;
						}
					}
					else if(strcmp(attr, "target") == 0)
					{
						RefTargetName = val;
					}
					else if(strcmp(attr, "doc") == 0)
					{
						// Ignore
					}
					else
					{
						Convert_error(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
					}
				}
			}

			// Add this type to the ULData map (if it has a UL)
			if(TypeUL != "") AddType(State, name, Detail, TypeUL);

			// Allow only the target type to be set
			if((!RefType) && (RefTargetName.length() > 0)) RefType = "ClassRefUndefined";

			if(!SymSpace)
			{
				if(!RefType)
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_BASIC(\"%s\", \"%s\", \"%s\", %d, %s)\n", name, Detail.c_str(), TypeUL.c_str(), Size, Endian ? "true" : "false");
				else
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_BASIC_REF(\"%s\", \"%s\", \"%s\", %d, %s, %s, \"%s\")\n", name, Detail.c_str(), TypeUL.c_str(), Size, Endian ? "true" : "false", RefType, RefTargetName.c_str());
			}
			else
			{
				if(!RefType)
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_BASIC_SYM(\"%s\", \"%s\", \"%s\", %d, %s, \"%s\")\n", name, Detail.c_str(), TypeUL.c_str(), Size, Endian ? "true" : "false", SymSpace);
				else
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_BASIC_REF_SYM(\"%s\", \"%s\", \"%s\", %d, %s, %s, \"%s\", \"%s\")\n", name, Detail.c_str(), TypeUL.c_str(), Size, Endian ? "true" : "false", RefType, RefTargetName.c_str(), SymSpace);
			}
			break;
		}

		case StateTypesInterpretation:
		{
			std::string Detail;
			std::string TypeUL;
			const char *SymSpace = NULL;
			const char *Base = "";
			int Size = 0;
			char *RefType = NULL;
			std::string RefTargetName;

			/* Process attributes */
			if(attrs != NULL)
			{
				int this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = CConvert(val);
					}
					else if(strcmp(attr, "base") == 0)
					{
						Base = val;
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "ul") == 0)
					{
						TypeUL = CConvert(val);
					}
					else if(strcmp(attr, "symSpace") == 0)
					{
						SymSpace = val;
					}
					else if(strcmp(attr, "ref") == 0)
					{
						if(strcasecmp(val,"strong") == 0) RefType = "ClassRefStrong";
						else if(strcasecmp(val,"target") == 0) RefType = "ClassRefTarget";
						else if(strcasecmp(val,"weak") == 0) RefType = "ClassRefWeak";
						else if(strcasecmp(val,"global") == 0) RefType = "ClassRefGlobal";
						else
						{
							Convert_warning(State, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
							RefType = NULL;
						}
					}
					else if(strcmp(attr, "target") == 0)
					{
						RefTargetName = val;
					}
					else if(strcmp(attr, "doc") == 0)
					{
						// Ignore
					}
					else
					{
						Convert_error(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
					}
				}
			}

			// Add this type to the ULData map (if it has a UL)
			if(TypeUL != "") AddType(State, name, Detail, TypeUL);

			// Allow only the target type to be set
			if((!RefType) && (RefTargetName.length() > 0)) RefType = "ClassRefUndefined";

			if(!SymSpace)
			{
				if(!RefType)
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_INTERPRETATION(\"%s\", \"%s\", \"%s\", \"%s\", %d)\n", name, Detail.c_str(), Base, TypeUL.c_str(), Size);
				else
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_INTERPRETATION_REF(\"%s\", \"%s\", \"%s\", \"%s\", %d, %s, \"%s\")\n", name, Detail.c_str(), Base, TypeUL.c_str(), Size, RefType, RefTargetName.c_str());
			}
			else
			{
				if(!RefType)
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_INTERPRETATION_SYM(\"%s\", \"%s\", \"%s\", \"%s\", %d, \"%s\")\n", name, Detail.c_str(), Base, TypeUL.c_str(), Size, SymSpace);
				else
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_INTERPRETATION_REF_SYM(\"%s\", \"%s\", \"%s\", \"%s\", %d, %s, \"%s\", \"%s\")\n", name, Detail.c_str(), Base, TypeUL.c_str(), Size, RefType, RefTargetName.c_str(), SymSpace);
			}

			break;
		}

		case StateTypesMultiple:
		{
			std::string Detail;
			std::string TypeUL;
			const char *Base = "";
			const char *SymSpace = NULL;
			bool IsBatch = false;
			int Size = 0;
			char *RefType = NULL;
			std::string RefTargetName;

			/* Process attributes */
			if(attrs != NULL)
			{
				int this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = CConvert(val);
					}
					else if(strcmp(attr, "base") == 0)
					{
						Base = val;
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "type") == 0)
					{
						if(strcasecmp(val, "Batch") == 0) IsBatch = true;
					}
					else if(strcmp(attr, "ul") == 0)
					{
						TypeUL = val;
					}
					else if(strcmp(attr, "symSpace") == 0)
					{
						SymSpace = val;
					}
					else if(strcmp(attr, "ref") == 0)
					{
						if(strcasecmp(val,"strong") == 0) RefType = "ClassRefStrong";
						else if(strcasecmp(val,"target") == 0) RefType = "ClassRefTarget";
						else if(strcasecmp(val,"weak") == 0) RefType = "ClassRefWeak";
						else if(strcasecmp(val,"global") == 0) RefType = "ClassRefGlobal";
						else
						{
							Convert_warning(State, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
							RefType = NULL;
						}
					}
					else if(strcmp(attr, "target") == 0)
					{
						RefTargetName = val;
					}
					else if(strcmp(attr, "doc") == 0)
					{
						// Ignore
					}
					else
					{
						Convert_error(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
					}
				}
			}

			// Add this type to the ULData map (if it has a UL)
			if(TypeUL != "") AddType(State, name, Detail, TypeUL);

			// Allow only the target type to be set
			if((!RefType) && (RefTargetName.length() > 0)) RefType = "ClassRefUndefined";

			if(!SymSpace)
 			{
				if(!RefType)
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_MULTIPLE(\"%s\", \"%s\", \"%s\", \"%s\", %s, %d)\n", name, Detail.c_str(), Base, TypeUL.c_str(), IsBatch ? "true" : "false", Size);
				else
					fprintf(State->OutFile, "\t\tMXFLIB_TYPE_MULTIPLE_REF(\"%s\", \"%s\", \"%s\", \"%s\", %s, %d, %s, \"%s\")\n", name, Detail.c_str(), Base, TypeUL.c_str(), IsBatch ? "true" : "false", Size, RefType, RefTargetName.c_str());
			}
			else
 			{
				if(!RefType)
                    fprintf(State->OutFile, "\t\tMXFLIB_TYPE_MULTIPLE_SYM(\"%s\", \"%s\", \"%s\", \"%s\", %s, %d, \"%s\")\n", name, Detail.c_str(), Base, TypeUL.c_str(), IsBatch ? "true" : "false", Size, SymSpace);
				else
                    fprintf(State->OutFile, "\t\tMXFLIB_TYPE_MULTIPLE_REF_SYM(\"%s\", \"%s\", \"%s\", \"%s\", %s, %d, %s, \"%s\", \"%s\")\n", name, Detail.c_str(), Base, TypeUL.c_str(), IsBatch ? "true" : "false", Size, RefType, RefTargetName.c_str(), SymSpace);
			}
			break;
		}

		case StateTypesCompound:
		{
			std::string Detail;
			std::string TypeUL;
			const char *SymSpace = NULL;

			/* Process attributes */
			if(attrs != NULL)
			{
				int this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = CConvert(val);
					}
					else if(strcmp(attr, "ul") == 0)
					{
						TypeUL = CConvert(val);
					}
					else if(strcmp(attr, "symSpace") == 0)
					{
						SymSpace = val;
					}
					else if(strcmp(attr, "doc") == 0)
					{
						// Ignore
					}
					else
					{
						Convert_error(user_data, "Unexpected attribute \"%s\" in compound type \"%s\"\n", attr, name);
					}
				}
			}

			// Add this type to the ULData map (if it has a UL)
			if(TypeUL != "") AddType(State, name, Detail, TypeUL);

			if(!SymSpace)
				fprintf(State->OutFile, "\t\tMXFLIB_TYPE_COMPOUND(\"%s\", \"%s\", \"%s\")\n", name, Detail.c_str(), TypeUL.c_str());
			else
				fprintf(State->OutFile, "\t\tMXFLIB_TYPE_COMPOUND_SYM(\"%s\", \"%s\", \"%s\", \"%s\")\n", name, Detail.c_str(), TypeUL.c_str(), SymSpace);

			State->State = StateTypesCompoundItem;
			State->Parent = name;

			break;
		}

		case StateTypesCompoundItem:
		{
			std::string Detail;
			std::string TypeUL;
			const char *Type = "";
			int Size = 0;
			
			/* Process attributes */
			if(attrs != NULL)
			{
				int this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = CConvert(val);
					}
					else if(strcmp(attr, "type") == 0)
					{
						Type = val;
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "ul") == 0)
					{
						TypeUL = CConvert(val);
					}
					else if(strcmp(attr, "doc") == 0)
					{
						// Ignore
					}
					else
					{
						error("Unexpected attribute \"%s\" in compound item \"%s\"\n", attr, name);
					}
				}
			}

			// Add this type to the ULData map (if it has a UL)
			if(TypeUL != "") AddType(State, name, Detail, TypeUL);

			fprintf(State->OutFile, "\t\t\tMXFLIB_TYPE_COMPOUND_ITEM(\"%s\", \"%s\", \"%s\", \"%s\", %d)\n", name, Detail.c_str(), Type, TypeUL.c_str(), Size);

			break;
		}

		case StateTypesEnum:
		{
			std::string ValueName = name;
			std::string Detail;
			std::string Base;
			std::string TypeUL;
			const char *SymSpace = NULL;

			/* Process attributes */
			if(attrs != NULL)
			{
				int this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = CConvert(val);
					}
					else if(strcmp(attr, "type") == 0)
					{
						Base = CConvert(val);
					}
					else if(strcmp(attr, "name") == 0)
					{
						ValueName = CConvert(val);
					}
					else if(strcmp(attr, "ul") == 0)
					{
						TypeUL = CConvert(val);
					}
					else if(strcmp(attr, "symSpace") == 0)
					{
						SymSpace = val;
					}
					else if(strcmp(attr, "doc") == 0)
					{
						// Ignore
					}
					else
					{
						Convert_error(user_data, "Unexpected attribute \"%s\" in compound type \"%s\"\n", attr, ValueName.c_str());
					}
				}
			}

			// Add this type to the ULData map (if it has a UL)
			if(TypeUL != "") AddType(State, ValueName, Detail, TypeUL);

			if(!SymSpace)
				fprintf(State->OutFile, "\t\tMXFLIB_TYPE_ENUM(\"%s\", \"%s\", \"%s\", \"%s\")\n", ValueName.c_str(), Detail.c_str(), Base.c_str(), TypeUL.c_str());
			else
				fprintf(State->OutFile, "\t\tMXFLIB_TYPE_ENUM_SYM(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")\n", ValueName.c_str(), Detail.c_str(), Base.c_str(), TypeUL.c_str(), SymSpace);

			State->State = StateTypesEnumValue;
			State->Parent = ValueName;

			break;
		}

		case StateTypesEnumValue:
		{
			std::string ValueName = name;
			std::string Detail;
			std::string Value;
			std::string TypeUL;
			
			/* Process attributes */
			if(attrs != NULL)
			{
				int this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = CConvert(val);
					}
					else if(strcmp(attr, "name") == 0)
					{
						ValueName = CConvert(val);
					}
					else if(strcmp(attr, "value") == 0)
					{
						Value = CConvert(val);
					}
					else if(strcmp(attr, "ul") == 0)
					{
						TypeUL = CConvert(val);
					}
					else if(strcmp(attr, "doc") == 0)
					{
						// Ignore
					}
					else
					{
						error("Unexpected attribute \"%s\" in compound item \"%s\"\n", attr, ValueName.c_str());
					}
				}
			}

			fprintf(State->OutFile, "\t\t\tMXFLIB_TYPE_ENUM_VALUE(\"%s\", \"%s\", \"%s\")\n", ValueName.c_str(), Detail.c_str(), Value.c_str());

			break;
		}

		case StateTypesLabel:
		{
			std::string Detail;
			const char *TypeUL = NULL;
			const char *Mask = NULL;
			const char *SymSpace = NULL;
			std::string ValueName = name;

			/* Process attributes */
			if(attrs != NULL)
			{
				int this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = CConvert(val);
					}
					else if(strcmp(attr, "ul") == 0)
					{
						TypeUL = val;
					}
					else if(strcmp(attr, "mask") == 0)
					{
						Mask = val;
					}
					else if(strcmp(attr, "symSpace") == 0)
					{
						SymSpace = val;
					}
					else if(strcmp(attr, "name") == 0)
					{
						ValueName = CConvert(val);
					}
					else if(strcmp(attr, "doc") == 0)
					{
						// Ignore any documentation attributes
					}
					else
					{
						error("Unexpected attribute \"%s\" in label \"%s\"\n", attr, name);
					}
				}
			}

			// DRAGONS: We don't add labels to the UL map

			// Work out the correct indentation level
			char *Indent = "\t\t\t";
			if(State->LabelsOnly) Indent = "\t\t";

			if(!SymSpace)
			{
				if(!Mask)
                    fprintf(State->OutFile, "%sMXFLIB_LABEL(\"%s\", \"%s\", \"%s\")\n", Indent, ValueName.c_str(), Detail.c_str(), TypeUL);
				else
					fprintf(State->OutFile, "%sMXFLIB_MASKED_LABEL(\"%s\", \"%s\", \"%s\", \"%s\")\n", Indent, ValueName.c_str(), Detail.c_str(), TypeUL, Mask);
			}
			else
			{
				if(!Mask)
                    fprintf(State->OutFile, "%sMXFLIB_LABEL_SYM(\"%s\", \"%s\", \"%s\", \"%s\")\n", Indent, ValueName.c_str(), Detail.c_str(), TypeUL, SymSpace);
				else
					fprintf(State->OutFile, "%sMXFLIB_MASKED_LABEL_SYM(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")\n", Indent, ValueName.c_str(), Detail.c_str(), TypeUL, Mask, SymSpace);
			}

			break;
		}

		case StateClasses:
		{
			// If we find a new style "MXFTypes" or "MXFClasses" section then restart in idle to enter mode properly
			if((strcmp(name, "MXFTypes") == 0) || (strcmp(name, "MXFClasses") == 0))
			{
				// Types or classes inside MXFDictionary is a new style dictionary
				if(!State->FoundType)
				{
					State->FoundType = true;
					State->FoundMulti = true;
				}

				State->State = StateIdle;
				Convert_startElement(user_data, name, attrs);
				return;
			}

			// Anything else at this point is an old style dictionary
			if(!State->FoundType)
			{
				State->FoundType = true;
				State->FoundMulti = false;

				// At this point we will have a deferred header - so issue it
				fprintf(State->OutFile, "\t// Class definitions converted from file %s\n", InputFile);
				
				if(State->SymSpace.size() == 0)
					fprintf(State->OutFile, "\tMXFLIB_CLASS_START(%s)\n", UseName.c_str());
				else
					fprintf(State->OutFile, "\tMXFLIB_CLASS_START_SYM(%s, \"%s\")\n", UseName.c_str(), State->SymSpace.c_str());
			}

			bool HasGlobalKey = false;
			bool HasDValue = false;
			bool HasDefault = false;
			std::string Key;
			std::string GlobalKey;
			std::string Use = "ClassUsageOptional";
			std::string RefType = "ClassRefNone";
			std::string Detail;
			std::string Type;
			UInt32 minLength = 0;
			UInt32 maxLength = 0;
			std::string RefTargetName;
			std::string Base;
			std::string default_text;
			std::string dvalue_text;
			std::string SymSpace;
			bool HasExtendSubs = false;
			bool ExtendSubs = true;

			/* Scan attributes */
			int this_attr = 0;
			if(attrs != NULL)
			{
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "key") == 0)
					{
						Key = val;
					}
					else if(strcmp(attr, "globalKey") == 0)
					{
						GlobalKey = val;
						HasGlobalKey = true;
					}
					else if(strcmp(attr, "detail") == 0)
					{
						Detail = CConvert(val);
					}
					else if(strcmp(attr, "use") == 0)
					{
						if(strcasecmp(val,"required") == 0) Use = "ClassUsageRequired";
						else if(strcasecmp(val,"encoder required") == 0) Use = "ClassUsageEncoderRequired";
						else if(strcasecmp(val,"decoder required") == 0) Use = "ClassUsageDecoderRequired";
						else if(strcasecmp(val,"best effort") == 0) Use = "ClassUsageBestEffort";
						else if(strcasecmp(val,"optional") == 0) Use = "ClassUsageOptional";
						else if(strcasecmp(val,"dark") == 0) Use = "ClassUsageDark";
						else if(strcasecmp(val,"toxic") == 0) Use = "ClassUsageToxic";
						else
						{
							Convert_warning(State, "Unknown use value use=\"%s\" in <%s/>", val, name);
							Use = "ClassUsageOptional";
						}
					}
					else if(strcmp(attr, "ref") == 0)
					{
						if(strcasecmp(val,"strong") == 0) RefType = "ClassRefStrong";
						else if(strcasecmp(val,"target") == 0) RefType = "ClassRefTarget";
						else if(strcasecmp(val,"weak") == 0) RefType = "ClassRefWeak";
						else if(strcasecmp(val,"global") == 0) RefType = "ClassRefGlobal";
						else
						{
							Convert_warning(State, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
							RefType = "ClassRefNone";
						}
					}
					else if(strcmp(attr, "type") == 0)
					{
						Type = val;
					}
//					else if(strcmp(attr, "length") == 0)
//					{
//						State->Tree[State->Sub]->minLength = atoi(val);
//						State->Tree[State->Sub]->maxLength = State->Tree[State->Sub]->minLength;
//					}
					else if(strcmp(attr, "minLength") == 0)
					{
						minLength = atoi(val);
					}
					else if(strcmp(attr, "maxLength") == 0)
					{
						maxLength = atoi(val);
					}
					else if(strcmp(attr, "keyFormat") == 0)
					{
						if(atoi(val) != 2)
						{
							Convert_error(State, "Class %s uses key format %s which is not supported\n", name, val);
						}
					}
					else if(strcmp(attr, "lengthFormat") == 0)
					{
						if(atoi(val) != 2)
						{
							Convert_error(State, "Class %s uses length format %s which is not supported\n", name, val);
						}
					}
					else if(strcmp(attr, "default") == 0)
					{
						HasDefault = true;
						default_text = val;
					}
					else if(strcmp(attr, "dvalue") == 0)
					{
						HasDValue = true;
						dvalue_text = val;
					}
					else if(strcmp(attr, "target") == 0)
					{
						RefTargetName = val;
					}
					else if(strcmp(attr, "base") == 0)
					{
						Base = val;
					}
					else if(strcmp(attr, "symSpace") == 0)
					{
						SymSpace = CConvert(val);
					}
					else if(strcmp(attr, "extendSubs") == 0)
					{
						HasExtendSubs = true;
						if((strcasecmp(val, "true") == 0) || (strcasecmp(val, "yes") == 0)) ExtendSubs = true;
						else ExtendSubs = false;
					}
					else
					{
						Convert_warning(State, "Unexpected attribute '%s' in <%s/>", attr, name);
					}
				}
			}

			// If only the Key is supplied this must be the UL
			if(!HasGlobalKey) GlobalKey = Key;

			// Calculate the local tag (if that is what the key is)
			UInt16 Tag = 0;
			UInt8 KeyBuff[16];
			int Count = ReadHexString(Key.c_str(), 16, KeyBuff, " \t.");
			if(Count == 2) Tag = GetU16(KeyBuff);

			ULDataPtr ThisItem;
			if(ULConsts && (GlobalKey.length() > 0))
			{
				UInt8 KeyBuff[16];

				int Count = ReadHexString(GlobalKey.c_str(), 16, KeyBuff, " \t.");
				if(Count == 16)
				{
					// Build a ULData item for this entry
					ThisItem = new ULData;
					ThisItem->Name = name;
					ThisItem->Detail = Detail;
					if(State->MultiData) ThisItem->Parent = State->MultiData;
					else ThisItem->Parent = State->ParentData;

					char TypeBuff[32];
					strncpy(TypeBuff, Type.c_str(), 32);
					TypeBuff[31] = '\0';

					if(   (strcasecmp(TypeBuff,"universalSet") == 0) 
					   || (strcasecmp(TypeBuff,"localSet") == 0)
				       || (strcasecmp(TypeBuff,"subLocalSet") == 0) )
					{
						ThisItem->IsSet = true;
						ThisItem->IsPack = false;
						ThisItem->IsMulti = false;
					}
					else if(   (strcasecmp(TypeBuff,"variablePack") == 0)
			                || (strcasecmp(TypeBuff,"subVariablePack") == 0)
							|| (strcasecmp(TypeBuff,"fixedPack") == 0)
							|| (strcasecmp(TypeBuff,"subFixedPack") == 0) )
					{
						ThisItem->IsSet = false;
						ThisItem->IsPack = true;
						ThisItem->IsMulti = false;
					}
					else
					{
						ThisItem->IsSet = false;
						ThisItem->IsPack = false;

						if(   (strcasecmp(TypeBuff,"vector") == 0)
						   || (strcasecmp(TypeBuff,"subVector") == 0)
						   ||   (strcasecmp(TypeBuff,"array") == 0)
						   || (strcasecmp(TypeBuff,"subArray") == 0) )
						{
							ThisItem->IsMulti = true;
						}
						else
						{
							ThisItem->IsMulti = false;
						}
					}

					ThisItem->UL = new UL(KeyBuff);
					ThisItem->LocalTag = (mxflib::Tag)Tag;

					// Build the name to use for the const
					std::string ItemName = ThisItem->Name;
					if(LongFormConsts && (ThisItem->Parent)) ItemName = ThisItem->Parent->Name + "_" + ItemName;

					// See if this is a duplicate entry
					ULDataMap::iterator it = ULMap.find(ItemName);
					if(it != ULMap.end())
					{
						if(*(ThisItem->UL) == *((*it).second->UL))
						{
							if((((*it).second->LocalTag != 0) && (Tag != 0)) && ((*it).second->LocalTag != ThisItem->LocalTag))
							{
								error("Multiple entries for %s with UL %s with different local tags (%s and %s)\n", 
									   ItemName.c_str(), ThisItem->UL->GetString().c_str(), 
									   Tag2String((*it).second->LocalTag).c_str(), Tag2String(ThisItem->LocalTag).c_str());
							}
							else
							{
								printf("Multiple entries for %s with UL %s - this is probably not an error\n", ItemName.c_str(), ThisItem->UL->GetString().c_str());
							}
						}
						else
						{
							printf("Duplicate name %s - will attempt to resolve later\n", ItemName.c_str());

							ULFixupList.push_back(ThisItem);
						}
					}
					else
					{
						ULMap.insert(ULDataMap::value_type(ItemName, ThisItem));
					}
				}
			}

			// Calculate the indent depth
			int i;
			std::string Indent = "\t\t";
			for(i=0; i<State->Depth; i++)
			{
				Indent += "\t";
			}

			// Inherit type from parent if not specified in this entry
			if((Type == "") && (Base != ""))
			{
				std::map<std::string, std::string>::iterator it = State->TypeMap.find(Base);
				if(it != State->TypeMap.end()) Type = (*it).second;
			}

			// Work out what the extend subs state (if not specified). 
			// Defaults to true if at top level (the initial state of the variable)
			if((!HasExtendSubs) && (!State->ExtendSubsList.empty()))
			{
				std::list<bool>::iterator it = State->ExtendSubsList.end();
				ExtendSubs = *(--it);
			}

			// Store the flag for this level
			State->ExtendSubsList.push_back(ExtendSubs);

			// TODO: We should really work out if a set or pack is a simple rename (will work as it is but will produce an empty section)

			char TypeBuff[32];
			strncpy(TypeBuff, Type.c_str(), 32);
			TypeBuff[31] = '\0';

			if(   (strcasecmp(TypeBuff,"universalSet") == 0) 
			   || (strcasecmp(TypeBuff,"variablePack") == 0)
			   || (strcasecmp(TypeBuff,"subVariablePack") == 0) )
			{
				if(!OnlyConsts)
				{
					Convert_error(State, "Class %s is unsupported type %s\n", name, Type.c_str());
					fprintf(State->OutFile, "%sERROR: Class %s is unsupported type %s\n", Indent.c_str(), name, Type.c_str());
				}
				State->Parent = name;
				State->EndTagText.push_back(Indent + "/* END UNSUPPORTED TYPE */");
			}
			else if(   (strcasecmp(TypeBuff,"localSet") == 0)
				    || (strcasecmp(TypeBuff,"subLocalSet") == 0) )
			{
				State->Parent = name;
				State->ParentData = ThisItem;

				if(SymSpace.size() == 0)
				{
					if(ExtendSubs)
					{
						fprintf(State->OutFile, "%sMXFLIB_CLASS_SET(\"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str());
					}
					else
					{
						fprintf(State->OutFile, "%sMXFLIB_CLASS_SET_NOSUB(\"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str());
					}
				}
				else
				{
					if(ExtendSubs)
					{
						fprintf(State->OutFile, "%sMXFLIB_CLASS_SET_SYM(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str(), SymSpace.c_str());
					}
					else
					{
						fprintf(State->OutFile, "%sMXFLIB_CLASS_SET_NOSUB_SYM(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str(), SymSpace.c_str());
					}
				}

				State->EndTagText.push_back(Indent + "MXFLIB_CLASS_SET_END");
			}
			else if(   (strcasecmp(TypeBuff,"fixedPack") == 0)
				    || (strcasecmp(TypeBuff,"subFixedPack") == 0) )
			{
				State->Parent = name;
				State->ParentData = ThisItem;


				if(SymSpace.size() == 0)
				{
					if(ExtendSubs)
					{
						fprintf(State->OutFile, "%sMXFLIB_CLASS_FIXEDPACK(\"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str());
					}
					else
					{
						fprintf(State->OutFile, "%sMXFLIB_CLASS_FIXEDPACK_NOSUB(\"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str());
					}
				}
				else
				{
					if(ExtendSubs)
					{
						fprintf(State->OutFile, "%sMXFLIB_CLASS_FIXEDPACK_SYM(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str(), SymSpace.c_str());
					}
					else
					{
						fprintf(State->OutFile, "%sMXFLIB_CLASS_FIXEDPACK_NOSUB_SYM(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str(), SymSpace.c_str());
					}
				}

				State->EndTagText.push_back(Indent + "MXFLIB_CLASS_FIXEDPACK_END");
			}
			else if(   (strcasecmp(TypeBuff,"vector") == 0)
				    || (strcasecmp(TypeBuff,"subVector") == 0) )
			{
				State->Multi = name;
				State->MultiData = ThisItem;

				if(SymSpace.size() != 0)
				{
					Convert_error(State, "Symbol space not currently supported for vector types such as <%s>\n", name);
				}

				if(RefType == "ClassRefNone")
					fprintf(State->OutFile, "%sMXFLIB_CLASS_VECTOR(\"%s\", \"%s\", %s, 0x%04x, \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Use.c_str(), Tag, GlobalKey.c_str());
				else
					fprintf(State->OutFile, "%sMXFLIB_CLASS_VECTOR_REF(\"%s\", \"%s\", %s, 0x%04x, \"%s\", %s, \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Use.c_str(), Tag, GlobalKey.c_str(), RefType.c_str(), RefTargetName.c_str());

				State->EndTagText.push_back(Indent + "MXFLIB_CLASS_VECTOR_END");
			}
			else if(   (strcasecmp(TypeBuff,"array") == 0)
				    || (strcasecmp(TypeBuff,"subArray") == 0) )
			{
				State->Multi = name;
				State->MultiData = ThisItem;

				if(SymSpace.size() != 0)
				{
					Convert_error(State, "Symbol space not currently supported for arrays types such as <%s>\n", name);
				}
				
				if(RefType == "ClassRefNone")
					fprintf(State->OutFile, "%sMXFLIB_CLASS_ARRAY(\"%s\", \"%s\", %s, 0x%04x, \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Use.c_str(), Tag, GlobalKey.c_str());
				else
					fprintf(State->OutFile, "%sMXFLIB_CLASS_ARRAY_REF(\"%s\", \"%s\", %s, 0x%04x, \"%s\", %s, \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Use.c_str(), Tag, GlobalKey.c_str(), RefType.c_str(), RefTargetName.c_str());

				State->EndTagText.push_back(Indent + "MXFLIB_CLASS_ARRAY_END");
			}
			// Must be an item
			else
			{
				if(Type == "") 
				{
					// Check if this is a redefinition (which is safe)
					std::map<std::string, std::string>::iterator it = State->TypeMap.find(name);
					if(it == State->TypeMap.end())
					{
					if(Base == "")
						{
						error("Class %s does not have a type specified\n", name);
						}
					else
						warning("Type %s is derived from type %s which is not known at this point - output file may need manual edit\n", name, Base.c_str());
					}
				}

				std::string Ref;
				if(RefType != "ClassRefNone") Ref = "_REF";

				if(SymSpace.size() == 0)
					fprintf(State->OutFile, "%sMXFLIB_CLASS_ITEM%s(\"%s\", \"%s\", %s, \"%s\", %d, %d, 0x%04x, \"%s\", ", Indent.c_str(), Ref.c_str(), name, Detail.c_str(), Use.c_str(), Type.c_str(), minLength, maxLength, Tag, GlobalKey.c_str());
				else
					fprintf(State->OutFile, "%sMXFLIB_CLASS_ITEM%s_SYM(\"%s\", \"%s\", %s, \"%s\", %d, %d, 0x%04x, \"%s\", ", Indent.c_str(), Ref.c_str(), name, Detail.c_str(), Use.c_str(), Type.c_str(), minLength, maxLength, Tag, GlobalKey.c_str());

				if(RefType != "ClassRefNone")	fprintf(State->OutFile, "%s, \"%s\", ", RefType.c_str(), RefTargetName.c_str());

				if(HasDefault) fprintf(State->OutFile, "\"%s\", ", default_text.c_str()); else fprintf(State->OutFile, "NULL, ");
				if(HasDValue) fprintf(State->OutFile, "\"%s\"", dvalue_text.c_str()); else fprintf(State->OutFile, "NULL");

				if(SymSpace.size() != 0) fprintf(State->OutFile, ", \"%s\"", SymSpace.c_str());
				fprintf(State->OutFile, ")\n");

				State->EndTagText.push_back("");
			}

			State->Depth++;

			// Record the type of this class so derived classes can inherit the type
			State->TypeMap[std::string(name)] = Type;

			break;
		}

		case StateDone:
		{
			Convert_error(user_data, "Tag <%s> found beyond end of dictionary data\n", name);
			return;
		}

		default:		// Should not be possible
		case StateError:
			return;
	}
}


//! XML callback - Deal with end tag of an element
void Convert_endElement(void *user_data, const char *name)
{
	ConvertState *State = (ConvertState*)user_data;

	switch(State->State)
	{
		default:
		case StateError:
			return;

		case StateIdle:
		{
			// Allow MXF dictionaries to be wrapped inside other XML files
			debug("Stepping out of outer level <%s>\n", name);
			break;

//			Convert_error(user_data, "Closing tag </%s> found when not unexpected\n", name);
//			return;
		}

		case StateTypes:
		{
			fprintf(State->OutFile, "\tMXFLIB_TYPE_END\n");
			State->State = StateIdle;
			break;
		}

		case StateTypesBasic: 
		{
			if(strcmp(name,"Basic") == 0) State->State = StateTypes;
			break;
		}
		case StateTypesInterpretation: 
		{
			if(strcmp(name,"Interpretation") == 0) State->State = StateTypes;
			break;
		}
		case StateTypesMultiple: 
		{
			if(strcmp(name,"Multiple") == 0) State->State = StateTypes;
			break;
		}
		case StateTypesCompound: 
		{
			if(strcmp(name,"Compound") == 0) State->State = StateTypes;
			break;
		}
		
		case StateTypesCompoundItem:
		{
			if(strcmp(name,State->Parent.c_str()) == 0)
			{
				fprintf(State->OutFile, "\t\tMXFLIB_TYPE_COMPOUND_END\n");
				State->State = StateTypesCompound;
				State->Parent = "";
			}
			break;
		}

		case StateTypesEnum: 
		{
			if(strcmp(name,"Enumeration") == 0) State->State = StateTypes;
			break;
		}
		
		case StateTypesEnumValue:
		{
			if(strcmp(name,State->Parent.c_str()) == 0)
			{
				fprintf(State->OutFile, "\t\tMXFLIB_TYPE_ENUM_END\n");
				State->State = StateTypesEnum;
				State->Parent = "";
			}
			break;
		}

		case StateTypesLabel: 
		{
			if((strcmp(name,"MXFLabels") == 0) || (strcmp(name,"Labels") == 0)) 
			{
				if(State->LabelsOnly) 
				{
					fprintf(State->OutFile, "\tMXFLIB_TYPE_END\n");
					State->State = StateIdle;
				}
				else 
					State->State = StateTypes;
			}
			break;
		}

		case StateClasses:
		{
			if(State->Depth == 0)
			{
				fprintf(State->OutFile, "\tMXFLIB_CLASS_END\n");
				State->State = StateIdle;
				break;
			}

			// Remove the parent name when we step out of a set or pack (and clear its data)
			if(strcmp(name,State->Parent.c_str()) == 0)
			{
				State->Parent = "";
				State->ParentData = NULL;
			}

			// Remove the multi name when we step out of a batch or array (and clear its data)
			if(strcmp(name,State->Multi.c_str()) == 0)
			{
				State->Multi = "";
				State->MultiData = NULL;
			}

			// Emit any end text
			std::list<std::string>::iterator it = State->EndTagText.end();
			it--;

			ASSERT(it != State->EndTagText.end());

			if((*it).size()) fprintf(State->OutFile, "%s\n", (*it).c_str());
			
			// Remove this text
			State->EndTagText.erase(it);

			// Remove the extend subs flag for this level
			State->ExtendSubsList.pop_back();

			State->Depth--;
		
			break;
		}
	}
}


//! Add a ULData item for a type
void AddType(ConvertState *State, std::string Name, std::string Detail, std::string TypeUL)
{
	ULDataPtr ThisItem;
	if(ULConsts && (TypeUL.length() > 0))
	{
		ULPtr ThisTypeUL = StringToUL(TypeUL);
		if(ThisTypeUL)
		{
			// Build a ULData item for this entry
			ThisItem = new ULData;
			ThisItem->Name = Name;
			ThisItem->Detail = Detail;
			ThisItem->Parent = State->ParentData;
			ThisItem->IsType = true;
			ThisItem->UL = ThisTypeUL;
			ThisItem->LocalTag = 0;

			// Build the name to use for the const
			std::string ItemName = ThisItem->Name;
			if(LongFormConsts && (ThisItem->Parent)) ItemName = ThisItem->Parent->Name + "_" + ItemName;

			// See if this is a duplicate entry
			ULDataMap::iterator it = ULMap.find(ItemName);
			if(it != ULMap.end())
			{
				if(*(ThisItem->UL) == *((*it).second->UL))
				{
					{
						printf("Multiple entries for type %s with UL %s - this is probably not an error\n", ItemName.c_str(), ThisItem->UL->GetString().c_str());
					}
				}
				else
				{
					printf("Duplicate name %s - will attempt to resolve later\n", ItemName.c_str());

					ULFixupList.push_back(ThisItem);
				}
			}
			else
			{
				ULMap.insert(ULDataMap::value_type(ItemName, ThisItem));
			}
		}
	}
}


//! XML callback - Handle warnings during XML parsing
extern void Convert_warning(void *user_data, const char *msg, ...)
{
    char Buffer[10240];			// DRAGONS: Could burst!!
	va_list args;

    va_start(args, msg);
	vsprintf(Buffer, msg, args);
	warning("XML WARNING: %s\n",Buffer);
    va_end(args);
}

//! XML callback - Handle errors during XML parsing
extern void Convert_error(void *user_data, const char *msg, ...)
{
    char Buffer[10240];			// DRAGONS: Could burst!!
	va_list args;

    va_start(args, msg);
	vsprintf(Buffer, msg, args);
	error("XML ERROR: %s\n",Buffer);
    va_end(args);
}

//! XML callback - Handle fatal errors during XML parsing
extern void Convert_fatalError(void *user_data, const char *msg, ...)
{
    char Buffer[10240];			// DRAGONS: Could burst!!
	va_list args;

    va_start(args, msg);
	vsprintf(Buffer, msg, args);
	error("XML FATAL ERROR: %s\n",Buffer);
    va_end(args);
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
