/*! \file	dictconvert.cpp
 *	\brief	Convert an XML dictionary file to compile-time definitions
 *
 *	\version $Id: dictconvert.cpp,v 1.1 2005/03/25 13:38:25 terabrit Exp $
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

// Name of the structure to build
std::string UseName = "DictData";

// Name of the file to be converted
char *InputFile = "";

// Declare main process function
int main_process(int argc, char *argv[]);

//! Do the main processing and pause if required
int main(int argc, char *argv[]) 
{ 
	int Ret = main_process(argc, argv);

	if(PauseBeforeExit) PauseForInput();

	return Ret;
}


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
	StateClasses,								//!< Processing classes
	StateDone,									//!< Finished processing
	StateError									//!< Error encountered - process nothing else
};

//! State structure for XML parsing
struct ConvertState
{
	CurrentState State;							//!< Current state of the parser state-machine
	std::string Compound;						//!< The name of the current compound being built
	FILE *OutFile;								//!< The file being written
	int Depth;									//!< Nesting depth in class parsing
	std::list<std::string> EndTagText;			//!< Text to be output at the next class end tag
	std::map<std::string, std::string> TypeMap;	//!< Map of type for each class - to allow types to be inherited
};


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
		printf("Options: -n=name    Use \"name\" as the name of the structure built\n");
		printf("         -v         Verbose mode - shows lots of debug info\n");
		printf("         -z         Pause for input before final exit\n");
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

	// Parse the file
	bool result = false;
	result = XMLParserParseFile(&XMLHandler, &State, InputFile);

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
			if(strcmp(name, "MXFTypes") == 0)
			{
				State->State = StateTypes;

				fprintf(State->OutFile, "\t// Types definitions converted from file %s\n", InputFile);
				fprintf(State->OutFile, "\tMXFLIB_TYPE_START(%s)\n", UseName.c_str());
			}
			else if(strcmp(name, "MXFDictionary") == 0)
			{
				State->State = StateClasses;

				fprintf(State->OutFile, "\t// Class definitions converted from file %s\n", InputFile);
				fprintf(State->OutFile, "\tMXFLIB_CLASS_START(%s)\n", UseName.c_str());
			}
			else
			{
				Convert_fatalError(user_data, "Outer tag <MXFTypes> or <MXFDictionary> expected - <%s> found\n", name);
				return;
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
			else
				Convert_error(user_data, "Tag <%s> found when types class expected\n", name);

			break;
		}

		case StateTypesBasic:
		{
//			const char *Detail = "";
			std::string Detail;

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
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						Convert_error(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
					}
				}
			}

			fprintf(State->OutFile, "\t\tMXFLIB_TYPE_BASIC(\"%s\", \"%s\", %d, %s)\n", name, Detail.c_str(), Size, Endian ? "true" : "false");
			
			break;
		}

		case StateTypesInterpretation:
		{
			const char *Detail = "";
			const char *Base = "";
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
						Detail = val;
					}
					else if(strcmp(attr, "base") == 0)
					{
						Base = val;
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						Convert_error(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
					}
				}
			}

			fprintf(State->OutFile, "\t\tMXFLIB_TYPE_INTERPRETATION(\"%s\", \"%s\", \"%s\", %d)\n", name, Detail, Base, Size);

			break;
		}

		case StateTypesMultiple:
		{
			const char *Detail = "";
			const char *Base = "";
			bool IsBatch = false;
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
						Detail = val;
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
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						Convert_error(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
					}
				}
			}

			fprintf(State->OutFile, "\t\tMXFLIB_TYPE_MULTIPLE(\"%s\", \"%s\", \"%s\", %s, %d)\n", name, Detail, Base, IsBatch ? "true" : "false", Size);

			break;
		}

		case StateTypesCompound:
		{
			const char *Detail = "";

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
						Detail = val;
					}
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						Convert_error(user_data, "Unexpected attribute \"%s\" in compound type \"%s\"\n", attr, name);
					}
				}
			}

			fprintf(State->OutFile, "\t\tMXFLIB_TYPE_COMPOUND(\"%s\", \"%s\")\n", name, Detail);

			State->State = StateTypesCompoundItem;
			State->Compound = name;

			break;
		}

		case StateTypesCompoundItem:
		{
			const char *Detail = "";
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
						Detail = val;
					}
					else if(strcmp(attr, "type") == 0)
					{
						Type = val;
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						error("Unexpected attribute \"%s\" in compound item \"%s\"\n", attr, name);
					}
				}
			}

			fprintf(State->OutFile, "\t\t\tMXFLIB_TYPE_COMPOUND_ITEM(\"%s\", \"%s\", \"%s\", %d)\n", name, Detail, Type, Size);

			break;
		}

		case StateClasses:
		{
			bool HasGlobalKey = false;
			bool HasDValue = false;
			bool HasDefault = false;
			std::string Key;
			std::string GlobalKey;
			std::string Use = "ClassUsageOptional";
			std::string RefType = "ClassRefNone";
			std::string Detail;
			std::string Type;
			Uint32 minLength = 0;
			Uint32 maxLength = 0;
			std::string RefTargetName;
			std::string Base;
			std::string default_text;
			std::string dvalue_text;

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
					else
					{
						Convert_warning(State, "Unexpected attribute '%s' in <%s/>", attr, name);
					}
				}
			}

			// If only the Key is supplied this must be the UL
			if(!HasGlobalKey) GlobalKey = Key;

			// Calculate the local tag (if that is what the key is)
			Uint16 Tag = 0;
			Uint8 KeyBuff[16];
			int Count = ReadHexString(Key.c_str(), 16, KeyBuff, " \t.");
			if(Count == 2) Tag = GetU16(KeyBuff);

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

			// TODO: We should really work out if a set or pack is a simple rename (will work as it is but will produce an empty section)

			char TypeBuff[32];
			strncpy(TypeBuff, Type.c_str(), 32);
			TypeBuff[32] = '\0';

			if(   (strcasecmp(TypeBuff,"universalSet") == 0) 
			   || (strcasecmp(TypeBuff,"variablePack") == 0)
			   || (strcasecmp(TypeBuff,"subVariablePack") == 0) )
			   Convert_error(State, "Class %s is unsupported type %s\n", name, Type.c_str());
			else if(   (strcasecmp(TypeBuff,"localSet") == 0)
				    || (strcasecmp(TypeBuff,"subLocalSet") == 0) )
			{
				fprintf(State->OutFile, "%sMXFLIB_CLASS_SET(\"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str());
				State->EndTagText.push_back(Indent + "MXFLIB_CLASS_SET_END");
			}
			else if(   (strcasecmp(TypeBuff,"fixedPack") == 0)
				    || (strcasecmp(TypeBuff,"subFixedPack") == 0) )
			{
				fprintf(State->OutFile, "%sMXFLIB_CLASS_FIXEDPACK(\"%s\", \"%s\", \"%s\", \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Base.c_str(), GlobalKey.c_str());
				State->EndTagText.push_back(Indent + "MXFLIB_CLASS_FIXEDPACK_END");
			}
			else if(   (strcasecmp(TypeBuff,"vector") == 0)
				    || (strcasecmp(TypeBuff,"subVector") == 0) )
			{
				if(RefType == "ClassRefNone")
					fprintf(State->OutFile, "%sMXFLIB_CLASS_VECTOR(\"%s\", \"%s\", %s, 0x%04x, \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Use.c_str(), Tag, GlobalKey.c_str());
				else
					fprintf(State->OutFile, "%sMXFLIB_CLASS_VECTOR_REF(\"%s\", \"%s\", %s, 0x%04x, \"%s\", %s, \"%s\")\n", Indent.c_str(), name, Detail.c_str(), Use.c_str(), Tag, GlobalKey.c_str(), RefType.c_str(), RefTargetName.c_str());

				State->EndTagText.push_back(Indent + "MXFLIB_CLASS_VECTOR_END");
			}
			else if(   (strcasecmp(TypeBuff,"array") == 0)
				    || (strcasecmp(TypeBuff,"subArray") == 0) )
			{
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
					if(Base == "")
						error("Class %s does not have a type specified\n", name);
					else
						warning("Type %s is derived from type %s which is not known at this point - output file may need manual edit\n", name, Base.c_str());
				}

				std::string Ref;
				if(RefType != "ClassRefNone") Ref = "_REF";
				fprintf(State->OutFile, "%sMXFLIB_CLASS_ITEM%s(\"%s\", \"%s\", %s, \"%s\", %d, %d, 0x%04x, \"%s\", ", Indent.c_str(), Ref.c_str(), name, Detail.c_str(), Use.c_str(), Type.c_str(), minLength, maxLength, Tag, GlobalKey.c_str());

				if(RefType != "ClassRefNone")	fprintf(State->OutFile, "%s, \"%s\", ", RefType.c_str(), RefTargetName.c_str());

				if(HasDefault) fprintf(State->OutFile, "\"%s\", ", default_text.c_str()); else fprintf(State->OutFile, "NULL, ");
				if(HasDValue) fprintf(State->OutFile, "\"%s\")\n", dvalue_text.c_str()); else fprintf(State->OutFile, "NULL)\n");

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
			Convert_error(user_data, "Closing tag </%s> found when not unexpected\n", name);
			return;
		}

		case StateTypes:
		{
			fprintf(State->OutFile, "\tMXFLIB_TYPE_END\n");
			State->State = StateDone;
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
			if(strcmp(name,State->Compound.c_str()) == 0)
			{
				fprintf(State->OutFile, "\t\tMXFLIB_TYPE_COMPOUND_END\n");
				State->State = StateTypesCompound;
				State->Compound = "";
			}
			break;
		}

		case StateClasses:
		{
			if(State->Depth == 0)
			{
				fprintf(State->OutFile, "\tMXFLIB_CLASS_END\n");
				State->State = StateDone;
				break;
			}

			// Emit any end text
			std::list<std::string>::iterator it = State->EndTagText.end();
			it--;

			ASSERT(it != State->EndTagText.end());

			if((*it).size()) fprintf(State->OutFile, "%s\n", (*it).c_str());
			
			// Remove this text
			State->EndTagText.erase(it);

			State->Depth--;
		
			break;
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
