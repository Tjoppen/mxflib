/*! \file	legacytypes.cpp
 *	\brief	Implementations of classes that load legacy format type and class dictionaries
 *
 *	\version $Id: legacytypes.cpp,v 1.1 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2005-2008, Matt Beard
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

#include <stdarg.h>


using namespace mxflib;

// Allow us simple access to deftypes specials
using namespace mxflib_deftypes;


/* XML parsing functions - with file scope */
namespace
{
	//! XML parsing functions for defining types
	void DefTypes_startElement(void *user_data, const char *name, const char **attrs);
	void DefTypes_endElement(void *user_data, const char *name);

	//! XML parsing functions for dictionary loading
	void DictLoad_startElement(void *user_data, const char *name, const char **attrs);
	void DictLoad_endElement(void *user_data, const char *name);

	//! Process an XML element that has been determined to be part of a class definition
	/*! \return true if all OK
	*/
	bool ProcessClassElement(void *user_data, const char *name, const char **attrs);


	//! XML callback - Handle warnings during XML parsing
	void XML_warning(void *user_data, const char *msg, ...)
	{
		char Buffer[10240];			// DRAGONS: Could burst!!
		va_list args;

		va_start(args, msg);
		vsprintf(Buffer, msg, args);
		warning("XML WARNING: %s\n",Buffer);
		va_end(args);
	}

	//! XML callback - Handle errors during XML parsing
	void XML_error(void *user_data, const char *msg, ...)
	{
		char Buffer[10240];			// DRAGONS: Could burst!!
		va_list args;

		va_start(args, msg);
		vsprintf(Buffer, msg, args);
		error("XML ERROR: %s\n",Buffer);
		va_end(args);
	}

	//! XML callback - Handle fatal errors during XML parsing
	void XML_fatalError(void *user_data, const char *msg, ...)
	{
		char Buffer[10240];			// DRAGONS: Could burst!!
		va_list args;

		va_start(args, msg);
		vsprintf(Buffer, msg, args);
		error("XML FATAL ERROR: %s\n",Buffer);
		va_end(args);
	}
}


namespace
{
	//! Our XML handler
	XMLParserHandler DefTypes_XMLHandler = {
		(startElementXMLFunc) DefTypes_startElement,		/* startElement */
		(endElementXMLFunc) DefTypes_endElement,			/* endElement */
		(warningXMLFunc) XML_warning,						/* warning */
		(errorXMLFunc) XML_error,							/* error */
		(fatalErrorXMLFunc) XML_fatalError,					/* fatalError */
	};

	//! State-machine state for XML parsing
	enum TypesCurrentState
	{
		StateIdle = 0,						//!< Processing not yet started
		StateTypes,							//!< Processing types - not yet processing a types section
		StateTypesBasic,					//!< Processing basic types section
		StateTypesInterpretation,			//!< Processing interpretation types section
		StateTypesMultiple,					//!< Processing multiple types section
		StateTypesCompound,					//!< Processing compound types section
		StateTypesCompoundItem,				//!< Processing sub-items within a compound
		StateTypesEnum,						//!< Processing enumerated types section
		StateTypesEnumValue,				//!< Processing valuess within an enumeration
		StateTypesLabel,					//!< Processing labels types section
		StateDone							//!< Finished processing
	};

	//! State structure for XML parsing types file
	struct TypesParserState
	{
		TypesCurrentState State;			//!< Current state of the parser state-machine
		TypeRecordList Types;				//!< The types being built
		TypeRecordPtr Parent;				//!< The current compound or enum being built (or NULL)
		SymbolSpacePtr DefaultSymbolSpace;	//!< Default symbol space to use for all types (in current MXFTypes section)
		bool LabelsOnly;					//!< True if this is a labels section rather than a full types section
											// ** DRAGONS: Labels are treated as types rather than defining a third kind (types, classes and labels)
	};
}


//! Load types from the specified legacy format XML definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadTypes(char *TypesFile, SymbolSpacePtr DefaultSymbolSpace /*=MXFLibSymbols*/)
{
	// Define the known traits
	// Test before calling as two partial definition files could be loaded!
	if(LoadBuiltInTraits) DefineTraits();

	// State data block passed through XML parser
	TypesParserState State;

	// Initialize the state
	State.State = StateIdle;
	State.DefaultSymbolSpace = DefaultSymbolSpace;
	State.LabelsOnly = false;

	// Get the qualified filename
	std::string XMLFilePath = LookupDictionaryPath(TypesFile);

	// Parse the file
	bool result = false;
	
	if(XMLFilePath.size()) result = XMLParserParseFile(&DefTypes_XMLHandler, &State, XMLFilePath.c_str());
	if (!result)
	{
		XML_fatalError(NULL, "Failed to load types dictionary \"%s\"\n", XMLFilePath.size() ? XMLFilePath.c_str() : TypesFile);
		return -1;
	}

	// Load the types that were found
	LoadTypes(State.Types);

	return 0;
}


namespace
{
	//! XML callback - Deal with start tag of an element
	void DefTypes_startElement(void *user_data, const char *name, const char **attrs)
	{
		TypesParserState *State = (TypesParserState*)user_data;

		switch(State->State)
		{
			case StateIdle:
			{
				State->State = StateTypes;

				if((strcmp(name, "MXFLabels") == 0) || (strcmp(name, "Labels") == 0))
				{
					State->LabelsOnly = true;
					State->State = StateTypesLabel;
				}
				else if(strcmp(name, "MXFTypes") != 0)
				{
					XML_fatalError(user_data, "Outer tag <MXFTypes> expected - <%s> found\n", name);
					return;
				}

				/* Check for symSpace */
				if(attrs != NULL)
				{
					int this_attr = 0;
					while(attrs[this_attr])
					{
						char const *attr = attrs[this_attr++];
						char const *val = attrs[this_attr++];
						
						if(strcmp(attr, "symSpace") == 0)
						{
							// See if this symbol space already exists
							SymbolSpacePtr DefaultSymbolSpace = SymbolSpace::FindSymbolSpace(val);

							// If it doesn't exist we must create it
							if(!DefaultSymbolSpace)
							{
								DefaultSymbolSpace = new SymbolSpace(val);
							}

							State->DefaultSymbolSpace = DefaultSymbolSpace;
						}
					}
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
				else if((strcmp(name, "Labels") == 0) || (strcmp(name, "MXFLabels") == 0))
				{
					State->LabelsOnly = false;
					State->State = StateTypesLabel;
				}
				else
					XML_error(user_data, "Tag <%s> found when types class expected\n", name);

				break;
			}

			case StateTypesBasic:
			{
				const char *Detail = "";
				const char *TypeUL = NULL;
				const char *SymSpace = NULL;
				TypeRef RefType = TypeRefUndefined;
				const char *RefTarget = NULL;
				int Size = 1;
				bool Endian = false;
				bool IsCharacter = false;
				bool IsBaseline = false;
				
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
						else if(strcmp(attr, "size") == 0)
						{
							Size = atoi(val);
						}
						else if(strcmp(attr, "endian") == 0)
						{
							if(strcasecmp(val,"yes") == 0) Endian = true;
							else if(strcasecmp(val,"true") == 0) Endian = true;
						}
						else if(strcmp(attr, "baseline") == 0)
						{
							if((strcasecmp(val, "true") == 0) || (strcasecmp(val, "yes") == 0)) IsBaseline = true;
							else IsBaseline = false;
						}
						else if(strcmp(attr, "character") == 0)
						{
							if(strcasecmp(val,"yes") == 0) IsCharacter = true;
							else if(strcasecmp(val,"true") == 0) IsCharacter = true;
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
							if(strcasecmp(val,"strong") == 0) RefType = TypeRefStrong;
							else if(strcasecmp(val,"target") == 0) RefType = TypeRefTarget;
							else if(strcasecmp(val,"weak") == 0) RefType = TypeRefWeak;
							else if(strcasecmp(val,"meta") == 0) RefType = TypeRefMeta;
							else if(strcasecmp(val,"dict") == 0) RefType = TypeRefDict;
							else if(strcasecmp(val,"none") == 0) RefType = TypeRefNone;
							else if(strcasecmp(val,"global") == 0) RefType = TypeRefGlobal;
							else
							{
								XML_warning(user_data, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
							}
						}
						else if(strcmp(attr, "target") == 0)
						{
							RefTarget = val;
						}
						else if(strcmp(attr, "doc") == 0)
						{
							// Ignore any documentation attributes
						}
						else
						{
							XML_warning(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
						}
					}
				}

				// Build a new type record
				TypeRecordPtr ThisType = new TypeRecord;

				ThisType->Class = TypeBasic;
				ThisType->Type = name;
				ThisType->Detail = Detail;
				ThisType->Base = "";
				if(TypeUL) ThisType->UL = StringToUL(TypeUL);
				if(SymSpace)
				{
					// A symbol space has been specified - look it up
					ThisType->SymSpace = SymbolSpace::FindSymbolSpace(SymSpace);

					// If it does not already exist, create it
					if(!ThisType->SymSpace) ThisType->SymSpace = new SymbolSpace(SymSpace);
				}
				else
				{
					ThisType->SymSpace = State->DefaultSymbolSpace;
				}
				ThisType->Size = Size;
				ThisType->Endian = Endian;
				ThisType->IsBaseline = IsBaseline;
				if(IsCharacter) ThisType->ArrayClass = ARRAYSTRING; else ThisType->ArrayClass = ARRAYIMPLICIT;
				ThisType->RefType = RefType;
				if(RefTarget) ThisType->RefTarget = RefTarget;

				// Add this type record
				State->Types.push_back(ThisType);

				break;
			}

			case StateTypesInterpretation:
			{
				const char *Detail = "";
				const char *Base = "";
				const char *TypeUL = NULL;
				const char *SymSpace = NULL;
				TypeRef RefType = TypeRefUndefined;
				const char *RefTarget = NULL;
				int Size = 0;
				bool IsCharacter = false;
				bool IsBaseline = false;
				
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
						else if(strcmp(attr, "ul") == 0)
						{
							TypeUL = val;
						}
						else if(strcmp(attr, "baseline") == 0)
						{
							if((strcasecmp(val, "true") == 0) || (strcasecmp(val, "yes") == 0)) IsBaseline = true;
							else IsBaseline = false;
						}
						else if(strcmp(attr, "character") == 0)
						{
							if(strcasecmp(val,"yes") == 0) IsCharacter = true;
							else if(strcasecmp(val,"true") == 0) IsCharacter = true;
						}
						else if(strcmp(attr, "symSpace") == 0)
						{
							SymSpace = val;
						}
						else if(strcmp(attr, "ref") == 0)
						{
							if(strcasecmp(val,"strong") == 0) RefType = TypeRefStrong;
							else if(strcasecmp(val,"target") == 0) RefType = TypeRefTarget;
							else if(strcasecmp(val,"weak") == 0) RefType = TypeRefWeak;
							else if(strcasecmp(val,"meta") == 0) RefType = TypeRefMeta;
							else if(strcasecmp(val,"dict") == 0) RefType = TypeRefDict;
							else if(strcasecmp(val,"none") == 0) RefType = TypeRefNone;
							else if(strcasecmp(val,"global") == 0) RefType = TypeRefGlobal;
							else
							{
								XML_warning(user_data, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
							}
						}
						else if(strcmp(attr, "target") == 0)
						{
							RefTarget = val;
						}
						else if(strcmp(attr, "doc") == 0)
						{
							// Ignore any documentation attributes
						}
						else
						{
							XML_error(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
						}
					}
				}

				// Build a new type record
				TypeRecordPtr ThisType = new TypeRecord;

				ThisType->Class = TypeInterpretation;
				ThisType->Type = name;
				ThisType->Detail = Detail;
				ThisType->Base = Base;
				if(TypeUL) ThisType->UL = StringToUL(TypeUL);
				if(SymSpace)
				{
					// A symbol space has been specified - look it up
					ThisType->SymSpace = SymbolSpace::FindSymbolSpace(SymSpace);

					// If it does not already exist, create it
					if(!ThisType->SymSpace) ThisType->SymSpace = new SymbolSpace(SymSpace);
				}
				else
				{
					ThisType->SymSpace = State->DefaultSymbolSpace;
				}
				ThisType->Size = Size;
				ThisType->Endian = false;
				ThisType->IsBaseline = IsBaseline;
				if(IsCharacter) ThisType->ArrayClass = ARRAYSTRING; else ThisType->ArrayClass = ARRAYIMPLICIT;
				ThisType->RefType = RefType;
				if(RefTarget) ThisType->RefTarget = RefTarget;

				// Add this type record
				State->Types.push_back(ThisType);

				break;
			}

			case StateTypesMultiple:
			{
				const char *Detail = "";
				const char *Base = "";
				const char *TypeUL = NULL;
				const char *SymSpace = NULL;
				TypeRef RefType = TypeRefUndefined;
				const char *RefTarget = NULL;
				MDArrayClass ArrayClass = ARRAYIMPLICIT;
				int Size = 0;
				bool IsBaseline = false;

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
							if((strcasecmp(val, "Batch") == 0) || (strcasecmp(val, "Explicit") == 0)) ArrayClass = ARRAYEXPLICIT;
							if(strcasecmp(val, "String") == 0) ArrayClass = ARRAYSTRING;
						}
						else if(strcmp(attr, "ul") == 0)
						{
							TypeUL = val;
						}
						else if(strcmp(attr, "baseline") == 0)
						{
							if((strcasecmp(val, "true") == 0) || (strcasecmp(val, "yes") == 0)) IsBaseline = true;
							else IsBaseline = false;
						}
						else if(strcmp(attr, "symSpace") == 0)
						{
							SymSpace = val;
						}
						else if(strcmp(attr, "ref") == 0)
						{
							if(strcasecmp(val,"strong") == 0) RefType = TypeRefStrong;
							else if(strcasecmp(val,"target") == 0) RefType = TypeRefTarget;
							else if(strcasecmp(val,"weak") == 0) RefType = TypeRefWeak;
							else if(strcasecmp(val,"meta") == 0) RefType = TypeRefMeta;
							else if(strcasecmp(val,"dict") == 0) RefType = TypeRefDict;
							else if(strcasecmp(val,"none") == 0) RefType = TypeRefNone;
							else if(strcasecmp(val,"global") == 0) RefType = TypeRefGlobal;
							else
							{
								XML_warning(user_data, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
							}
						}
						else if(strcmp(attr, "target") == 0)
						{
							RefTarget = val;
						}
						else if(strcmp(attr, "doc") == 0)
						{
							// Ignore any documentation attributes
						}
						else
						{
							XML_error(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
						}
					}
				}

				// Build a new type record
				TypeRecordPtr ThisType = new TypeRecord;

				ThisType->Class = TypeMultiple;
				ThisType->Type = name;
				ThisType->Detail = Detail;
				ThisType->Base = Base;
				if(TypeUL) ThisType->UL = StringToUL(TypeUL);
				if(SymSpace)
				{
					// A symbol space has been specified - look it up
					ThisType->SymSpace = SymbolSpace::FindSymbolSpace(SymSpace);

					// If it does not already exist, create it
					if(!ThisType->SymSpace) ThisType->SymSpace = new SymbolSpace(SymSpace);
				}
				else
				{
					ThisType->SymSpace = State->DefaultSymbolSpace;
				}
				ThisType->Size = Size;
				ThisType->Endian = false;
				ThisType->IsBaseline = IsBaseline;
				ThisType->ArrayClass = ArrayClass;
				ThisType->RefType = RefType;
				if(RefTarget) ThisType->RefTarget = RefTarget;

				// Add this type record
				State->Types.push_back(ThisType);

				break;
			}

			case StateTypesCompound:
			{
				const char *Detail = "";
				const char *TypeUL = NULL;
				const char *SymSpace = NULL;
				bool IsBaseline = false;

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
						else if(strcmp(attr, "ul") == 0)
						{
							TypeUL = val;
						}
						else if(strcmp(attr, "baseline") == 0)
						{
							if((strcasecmp(val, "true") == 0) || (strcasecmp(val, "yes") == 0)) IsBaseline = true;
							else IsBaseline = false;
						}
						else if(strcmp(attr, "symSpace") == 0)
						{
							SymSpace = val;
						}
						else if(strcmp(attr, "doc") == 0)
						{
							// Ignore any documentation attributes
						}
						else
						{
							XML_error(user_data, "Unexpected attribute \"%s\" in compound type \"%s\"\n", attr, name);
						}
					}
				}

				// Build a new type record
				TypeRecordPtr ThisType = new TypeRecord;

				ThisType->Class = TypeCompound;
				ThisType->Type = name;
				ThisType->Detail = Detail;
				ThisType->Base = "";
				if(TypeUL) ThisType->UL = StringToUL(TypeUL);
				if(SymSpace)
				{
					// A symbol space has been specified - look it up
					ThisType->SymSpace = SymbolSpace::FindSymbolSpace(SymSpace);

					// If it does not already exist, create it
					if(!ThisType->SymSpace) ThisType->SymSpace = new SymbolSpace(SymSpace);
				}
				else
				{
					ThisType->SymSpace = State->DefaultSymbolSpace;
				}
				ThisType->Size = 0;
				ThisType->Endian = false;
				ThisType->ArrayClass = ARRAYIMPLICIT;

				// Add this type record
				State->Types.push_back(ThisType);

				State->State = StateTypesCompoundItem;
				State->Parent = ThisType;

				break;
			}

			case StateTypesCompoundItem:
			{
				const char *Detail = "";
				const char *Type = "";
				const char *TypeUL = NULL;
				// DRAGONS: Not supporting separate symbol space for sub-items in a compound
				TypeRef RefType = TypeRefUndefined;
				const char *RefTarget = NULL;
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
						else if(strcmp(attr, "ul") == 0)
						{
							TypeUL = val;
						}
						else if(strcmp(attr, "target") == 0)
						{
							RefTarget = val;
						}
						else if(strcmp(attr, "doc") == 0)
						{
							// Ignore any documentation attributes
						}
						else
						{
							error("Unexpected attribute \"%s\" in compound item \"%s\"\n", attr, name);
						}
					}
				}

				// Build a new type record
				TypeRecordPtr ThisType = new TypeRecord;

				ThisType->Class = TypeSub;
				ThisType->Type = name;
				ThisType->Detail = Detail;
				ThisType->Base = Type;
				if(TypeUL) ThisType->UL = StringToUL(TypeUL);
				ThisType->Size = Size;
				ThisType->Endian = false;
				ThisType->ArrayClass = ARRAYIMPLICIT;
				ThisType->RefType = RefType;
				if(RefTarget) ThisType->RefTarget = RefTarget;

				// Add as a child of the current compound
				State->Parent->Children.push_back(ThisType);

				break;
			}

			case StateTypesEnum:
			{
				const char *Detail = "";
				const char *Base = NULL;
				const char *TypeUL = NULL;
				const char *SymSpace = NULL;
				bool IsBaseline = false;

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
							Base = val;
						}
						else if(strcmp(attr, "ul") == 0)
						{
							TypeUL = val;
						}
						else if(strcmp(attr, "baseline") == 0)
						{
							if((strcasecmp(val, "true") == 0) || (strcasecmp(val, "yes") == 0)) IsBaseline = true;
							else IsBaseline = false;
						}
						else if(strcmp(attr, "symSpace") == 0)
						{
							SymSpace = val;
						}
						else if(strcmp(attr, "doc") == 0)
						{
							// Ignore any documentation attributes
						}
						else
						{
							XML_error(user_data, "Unexpected attribute \"%s\" in enumeration type \"%s\"\n", attr, name);
						}
					}
				}

				if(!Base)
				{
					error("No value type specified for enumerated type %s\n", name);
				}

				// Build a new type record
				TypeRecordPtr ThisType = new TypeRecord;

				ThisType->Class = TypeEnum;
				ThisType->Type = name;
				ThisType->Detail = Detail;
				if(Base) ThisType->Base = Base;
				if(TypeUL) ThisType->UL = StringToUL(TypeUL);
				if(SymSpace)
				{
					// A symbol space has been specified - look it up
					ThisType->SymSpace = SymbolSpace::FindSymbolSpace(SymSpace);

					// If it does not already exist, create it
					if(!ThisType->SymSpace) ThisType->SymSpace = new SymbolSpace(SymSpace);
				}
				else
				{
					ThisType->SymSpace = State->DefaultSymbolSpace;
				}
				ThisType->Size = 0;
				ThisType->Endian = false;
				ThisType->IsBaseline = IsBaseline;
				ThisType->ArrayClass = ARRAYIMPLICIT;

				// Add this type record
				State->Types.push_back(ThisType);

				State->State = StateTypesEnumValue;
				State->Parent = ThisType;

				break;
			}

			case StateTypesEnumValue:
			{
				const char *ValueName = name;					// Allow the xml-name to be over-ridden
				const char *Detail = "";
				const char *Value = "";
				// DRAGONS: Not supporting separate symbol space for enum values

				/* Process attributes */
				if(attrs != NULL)
				{
					int this_attr = 0;
					while(attrs[this_attr])
					{
						char const *attr = attrs[this_attr++];
						char const *val = attrs[this_attr++];
						
						if(strcmp(attr, "name") == 0)
						{
							ValueName = val;
						}
						else if(strcmp(attr, "detail") == 0)
						{
							Detail = val;
						}
						else if(strcmp(attr, "value") == 0)
						{
							Value = val;
						}
						else if(strcmp(attr, "doc") == 0)
						{
							// Ignore any documentation attributes
						}
						else
						{
							error("Unexpected attribute \"%s\" in enumerated value \"%s\"\n", attr, ValueName);
						}
					}
				}

				if(!Value)
				{
					error("No value for enumerated value %s\n", ValueName);
				}
				else
				{
					// Build a new type record
					TypeRecordPtr ThisType = new TypeRecord;

					ThisType->Class = TypeSub;
					ThisType->Type = ValueName;
					ThisType->Detail = Detail;
					ThisType->Value = Value;
					ThisType->Endian = false;
					ThisType->ArrayClass = ARRAYIMPLICIT;

					// Add as a child of the current compound
					State->Parent->Children.push_back(ThisType);
				}

				break;
			}

			case StateTypesLabel:
			{
				const char *Detail = "";
				const char *TypeUL = NULL;
				const char *Mask = NULL;
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
							Detail = val;
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
						else if(strcmp(attr, "doc") == 0)
						{
							// Ignore any documentation attributes
						}
						else
						{
							XML_error(user_data, "Unexpected attribute \"%s\" in label \"%s\"\n", attr, name);
						}
					}
				}

				// Build a new type record
				TypeRecordPtr ThisType = new TypeRecord;

				ThisType->Class = TypeLabel;
				ThisType->Type = name;
				ThisType->Detail = Detail;
				if(TypeUL) ThisType->UL = StringToUL(TypeUL);
				if(Mask) ThisType->Value = Mask;

				if(SymSpace)
				{
					// A symbol space has been specified - look it up
					ThisType->SymSpace = SymbolSpace::FindSymbolSpace(SymSpace);

					// If it does not already exist, create it
					if(!ThisType->SymSpace) ThisType->SymSpace = new SymbolSpace(SymSpace);
				}
				else
				{
					ThisType->SymSpace = State->DefaultSymbolSpace;
				}
				ThisType->Size = 0;
				ThisType->Endian = false;
				ThisType->ArrayClass = ARRAYIMPLICIT;

				// Add this type record
				State->Types.push_back(ThisType);

				break;
			}

			default:		// Should not be possible
			case StateDone:
			{
				XML_error(user_data, "Tag <%s> found beyond end of dictionary data\n", name);
				return;
			}
		}
	}
}



namespace
{
	//! XML callback - Deal with end tag of an element
	void DefTypes_endElement(void *user_data, const char *name)
	{
		TypesParserState *State = (TypesParserState*)user_data;

		switch(State->State)
		{
			default:
			case StateIdle:
			{
				XML_error(user_data, "Closing tag </%s> found when not unexpected\n", name);
				return;
			}

			case StateTypes:
			{
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
				if(strcmp(name,State->Parent->Type.c_str()) == 0)
				{
					State->State = StateTypesCompound;
					State->Parent = NULL;
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
				if(strcmp(name,State->Parent->Type.c_str()) == 0)
				{
					State->State = StateTypesEnum;
					State->Parent = NULL;
				}
				break;
			}
			case StateTypesLabel:
			{
				if((strcmp(name,"MXFLabels") == 0) || (strcmp(name,"Labels") == 0))
				{
					if(State->LabelsOnly) State->State = StateDone;
					else State->State = StateTypes;
				}
				break;
			}
		}
	}
}


namespace
{
	//! Our XML handler
	static XMLParserHandler DictLoad_XMLHandler = 
	{
		(startElementXMLFunc) DictLoad_startElement,		/* startElement */
		(endElementXMLFunc) DictLoad_endElement,			/* endElement */
		(warningXMLFunc) XML_warning,						/* warning */
		(errorXMLFunc) XML_error,							/* error */
		(fatalErrorXMLFunc) XML_fatalError,					/* fatalError */
	};

	//! State-machine state for XML parsing
	enum DictCurrentState
	{
		DictStateIdle = 0,					//!< Processing not yet started
		DictStateDictionary,				//!< Within the outer tags
		DictStateTypes,						//!< Found a types section
		DictStateClasses,					//!< Found a classes section
		DictStateError						//!< A fatal error occurred
	};

	//! State structure for XML parsing types file
	struct DictParserState
	{
		DictCurrentState State;				//!< Current state of the parser state-machine
		TypesParserState ClassState;		//!< Parser state for types sub-parser
		SymbolSpacePtr DefaultSymbolSpace;	//!< Default symbol space to use for all classes (in current MXFClasses section)
		SymbolSpacePtr DictSymbolSpace;		//!< Default symbol space to use for all classes (in the whole dictionary)
		ClassRecordList ClassList;			//!< Class being built at this level (one for each level in the hierarchy)
		ClassRecordList ClassesToBuild;		//!< Top level classes that need to be built at the end of the parsing
	};
}

//! Load dictionary from the specified XML definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadLegacyDictionary(const char *DictFile, SymbolSpacePtr DefaultSymbolSpace, bool FastFail /*=false*/)
{
	// State data block passed through XML parser
	DictParserState State;

	// Initialize the state
	State.State = DictStateIdle;
	State.DefaultSymbolSpace = DefaultSymbolSpace;
	State.DictSymbolSpace = DefaultSymbolSpace;

	std::string XMLFilePath = LookupDictionaryPath(DictFile);

	// Parse the file
	bool result = false;

	if(XMLFilePath.size()) result = XMLParserParseFile(&DictLoad_XMLHandler, &State, XMLFilePath.c_str());
	if (!result)
	{
		XML_fatalError(NULL, "Failed to load dictionary \"%s\"\n", XMLFilePath.size() ? XMLFilePath.c_str() : DictFile);
		return -1;
	}

	// If any classes were found they will be stored ready to build, so build them now
	if(!State.ClassesToBuild.empty())
	{
		LoadClasses(State.ClassesToBuild, DefaultSymbolSpace);

		// Build a static primer (for use in index tables)
		MDOType::MakePrimer(true);
	}

	// Locate reference target types for any new types
	MDOType::LocateRefTypes();

	return 0;
}


//load dictionary from a string
int mxflib::LoadLegacyDictionaryFromXML(std::string & strXML, bool FastFail)
{
#ifndef HAVE_EXPAT
	XML_fatalError(NULL, "Unable to load dictionary from string unless compiled with Expat XML parser\n");
	return -1;
#else // HAVE_EXPAT

	// State data block passed through XML parser
	DictParserState State;

	// Initialize the state
	State.State = DictStateIdle;
	State.DefaultSymbolSpace = MXFLibSymbols;
	State.DictSymbolSpace = MXFLibSymbols;

	// Parse the file
	bool result = false;

	if(strXML.size()) 
		result = XMLParserParseString(&DictLoad_XMLHandler, &State, strXML);
	if (!result)
	{
		XML_fatalError(NULL, "Failed to load dictionary from string\n");
		return -1;
	}

	// If any classes were found they will be stored ready to build, so build them now
	if(!State.ClassesToBuild.empty())
	{
		LoadClasses(State.ClassesToBuild, MXFLibSymbols);

		// Build a static primer (for use in index tables)
		MDOType::MakePrimer(true);
	}

	// Locate reference target types for any new types
	MDOType::LocateRefTypes();

	return 0;
#endif // HAVE_EXPAT
}



namespace
{
	//! XML callback - Deal with start tag of an element
	void DictLoad_startElement(void *user_data, const char *name, const char **attrs)
	{
		DictParserState *State = (DictParserState*)user_data;

		switch(State->State)
		{
			// Identify the outer type
			case DictStateIdle:
			{
				// Normal start of unified dictionary, or start of old-style classes dictionary
				if(strcmp(name, "MXFDictionary") == 0)
				{
					State->State = DictStateDictionary;

					/* Check for symSpace */
					if(attrs != NULL)
					{
						int this_attr = 0;
						while(attrs[this_attr])
						{
							char const *attr = attrs[this_attr++];
							char const *val = attrs[this_attr++];
							
							if(strcmp(attr, "symSpace") == 0)
							{
								// See if this symbol space already exists
								SymbolSpacePtr DefaultSymbolSpace = SymbolSpace::FindSymbolSpace(val);

								// If it doesn't exist we must create it
								if(!DefaultSymbolSpace)
								{
									DefaultSymbolSpace = new SymbolSpace(val);
								}

								State->DefaultSymbolSpace = DefaultSymbolSpace;
							}
						}
					}

					break;
				}
				// Start of old-style types dictionary
				else if(strcmp(name, "MXFTypes") == 0)
				{
					State->State = DictStateDictionary;
					
					// ... fall through to the DictStateDictionary code where we will process this tag again
				}
				else
				{
					// Allow MXF dictionaries to be wrapped inside other XML files
					debug("Stepping into outer level <%s>\n", name);
					break;
				}
			}

			case DictStateDictionary:
			{
				if((strcmp(name, "MXFTypes") == 0) || (strcmp(name, "MXFLabels") == 0) || (strcmp(name, "Labels") == 0))
				{
					/* Start types parsing */

					// Define the known traits
					// Test before calling as two partial definition files could be loaded!
					if(LoadBuiltInTraits) DefineTraits();

					// Initialize the types parser state
					State->ClassState.State = StateIdle;

					// Switch to types parsing
					State->State = DictStateTypes;

					// Call the old parser to process the MXFTypes tag
					DefTypes_startElement(&State->ClassState, name, attrs);

					break;
				}

				// Start classes parsing
				State->State = DictStateClasses;
				State->ClassList.clear();

				if(strcmp(name, "MXFClasses") == 0)
				{
					// Found an indicator that we are starting new-style unified dictionary classes

					SymbolSpacePtr DefaultSymbolSpace = State->DefaultSymbolSpace;
					
					/* Check for symSpace */
					if(attrs != NULL)
					{
						int this_attr = 0;
						while(attrs[this_attr])
						{
							char const *attr = attrs[this_attr++];
							char const *val = attrs[this_attr++];
							
							if(strcmp(attr, "symSpace") == 0)
							{
								// See if this symbol space already exists
								DefaultSymbolSpace = SymbolSpace::FindSymbolSpace(val);

								// If it doesn't exist we must create it
								if(!DefaultSymbolSpace)
								{
									DefaultSymbolSpace = new SymbolSpace(val);
								}

								State->DefaultSymbolSpace = DefaultSymbolSpace;

								break;
							}
						}
					}

					break;
				}

				// Otherwise it seems that this is an old-style classes dictionary and are now in a classes section
				// ... fall through to DictStateClasses and parse this first class
			}

			// Parse classes
			case DictStateClasses:
			{
				ProcessClassElement(user_data, name, attrs);

				break;
			}

			// Parse types
			case DictStateTypes:
			{
				// Call the old parser
				DefTypes_startElement(&State->ClassState, name, attrs);

				break;
			}

			default:		// All other cases
				return;

		}
	}
}



namespace
{
	//! Process an XML element that has been determined to be part of a class definition
	/*! \return true if all OK
	 */
	bool ProcessClassElement(void *user_data, const char *name, const char **attrs)
	{
		DictParserState *State = (DictParserState*)user_data;

		bool Ret = true;
		int this_attr = 0;

		debug("Element : %s\n", name);

		if(attrs != NULL)
		{
			while(attrs[this_attr])
			{
				debug("  Attribute : %s = \"%s\"\n", attrs[this_attr], attrs[this_attr+1]);
				this_attr += 2;
			}
		}

		// Build a record for this class
		ClassRecordPtr ThisClass = new ClassRecord;

		// Set our name
		ThisClass->Name = name;

		// The two keys
		DataChunkPtr Key;
		DataChunkPtr GlobalKey;

		// Used to determine if we need to copy the main key to the global key
		bool HasGlobalKey = false;

		// Index our level info
		ClassRecordPtr Parent;

		if(!State->ClassList.empty())
		{
			Parent = State->ClassList.back();
		}

		if(!Parent)
		{
			ThisClass->ExtendSubs = true;
			ThisClass->SymSpace = State->DefaultSymbolSpace;
		}
		else
		{
			ThisClass->ExtendSubs = Parent->ExtendSubs;		// Carry on extending subs if we were, not if we weren't
			ThisClass->SymSpace = Parent->SymSpace;			// Copy over our parents symbol space (we may override this)
		}

		// Set up temporary length and key format values to transfer to MnSize and MaxSize if we are defining a set or a pack
		unsigned int ThisKeyFormat = (unsigned int)DICT_KEY_UNDEFINED;
		unsigned int ThisLenFormat = (unsigned int)DICT_LEN_UNDEFINED;

		/* Scan attributes */
		this_attr = 0;
		if(attrs != NULL)
		{
			while(attrs[this_attr])
			{
				char const *attr = attrs[this_attr++];
				char const *val = attrs[this_attr++];
				
				if(strcmp(attr, "key") == 0)
				{
					int Size;
					const char *p = val;
					UInt8 Buffer[32];

					Size = ReadHexStringOrUL(p, 32, Buffer, " \t.");

					Key = new DataChunk(Size, Buffer);
				}
				else if(strcmp(attr, "globalKey") == 0)
				{
					int Size;
					const char *p = val;
					UInt8 Buffer[32];

					Size = ReadHexStringOrUL(p, 32, Buffer, " \t.");

					GlobalKey = new DataChunk(Size, Buffer);

					HasGlobalKey = true;
				}
				else if(strcmp(attr, "detail") == 0)
				{
					ThisClass->Detail = std::string(val);
				}
				else if(strcmp(attr, "use") == 0)
				{
					if(strcasecmp(val,"required") == 0) ThisClass->Usage = DICT_USE_REQUIRED;
					else if(strcasecmp(val,"encoder required") == 0) ThisClass->Usage = DICT_USE_ENCODER_REQUIRED;
					else if(strcasecmp(val,"decoder required") == 0) ThisClass->Usage = DICT_USE_DECODER_REQUIRED;
					else if(strcasecmp(val,"best effort") == 0) ThisClass->Usage = DICT_USE_BEST_EFFORT;
					else if(strcasecmp(val,"optional") == 0) ThisClass->Usage = DICT_USE_OPTIONAL;
					else if(strcasecmp(val,"dark") == 0) ThisClass->Usage = DICT_USE_DARK;
					else if(strcasecmp(val,"toxic") == 0) ThisClass->Usage = DICT_USE_TOXIC;
					else
					{
						XML_warning(user_data, "Unknown use value use=\"%s\" in <%s/>", val, name);
					}
				}
				else if(strcmp(attr, "ref") == 0)
				{
					if(strcasecmp(val,"strong") == 0) ThisClass->RefType = ClassRefStrong;
					else if(strcasecmp(val,"target") == 0) ThisClass->RefType = ClassRefTarget;
					else if(strcasecmp(val,"weak") == 0) ThisClass->RefType = ClassRefWeak;
					else if(strcasecmp(val,"meta") == 0) ThisClass->RefType = ClassRefMeta;
					else if(strcasecmp(val,"dict") == 0) ThisClass->RefType = ClassRefDict;
					else if(strcasecmp(val,"none") == 0) ThisClass->RefType = ClassRefNone;
					else if(strcasecmp(val,"global") == 0) ThisClass->RefType = ClassRefGlobal;
					else
					{
						XML_warning(user_data, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
					}
				}
				else if(strcmp(attr, "type") == 0)
				{
					if(strcasecmp(val,"universalSet") == 0) 
					{
						XML_error(user_data, "Class %s is unsupported type %s\n", name, val);
					}
					else if(   (strcasecmp(val,"localSet") == 0)
							|| (strcasecmp(val,"subLocalSet") == 0) )
					{
						ThisClass->Class = ClassSet;
					}
					else if(   (strcasecmp(val,"fixedPack") == 0)
							|| (strcasecmp(val,"subFixedPack") == 0) )
					{
						ThisClass->Class = ClassPack;
						ThisKeyFormat = DICT_KEY_NONE;
						ThisLenFormat = DICT_LEN_NONE;
					}
					else if(   (strcasecmp(val,"variablePack") == 0)
							|| (strcasecmp(val,"subVariablePack") == 0) )
					{
						ThisClass->Class = ClassPack;
						ThisKeyFormat = DICT_KEY_NONE;
					}
					else if(   (strcasecmp(val,"vector") == 0)
						    || (strcasecmp(val,"subVector") == 0) )
					{
						ThisClass->Class = ClassVector;
					}
					else if(   (strcasecmp(val,"array") == 0)
							|| (strcasecmp(val,"subArray") == 0) )
					{
						ThisClass->Class = ClassArray;
					}
					else
					{
						ThisClass->Class = ClassItem;
						ThisClass->Base = val;
					}
				}
				else if(strcmp(attr, "minLength") == 0)
				{
					ThisClass->MinSize = atoi(val);
				}
				else if(strcmp(attr, "maxLength") == 0)
				{
					ThisClass->MaxSize = atoi(val);
				}
				else if(strcmp(attr, "keyFormat") == 0)
				{
					ThisKeyFormat = (unsigned int)atoi(val);
				}
				else if(strcmp(attr, "lengthFormat") == 0)
				{
					if(strcasecmp(val, "BER")==0)
					{
						ThisLenFormat = (unsigned int)DICT_LEN_BER;
					}
					else
					{
						ThisLenFormat = (unsigned int)atoi(val);
					}
				}
				else if(strcmp(attr, "default") == 0)
				{
					ThisClass->HasDefault = true;
					ThisClass->Default = std::string(val);
				}
				else if(strcmp(attr, "dvalue") == 0)
				{
					ThisClass->HasDValue = true;
					ThisClass->DValue = std::string(val);
				}
				else if(strcmp(attr, "target") == 0)
				{
					ThisClass->RefTarget = std::string(val);
				}
				else if(strcmp(attr, "base") == 0)
				{
					ThisClass->Base = std::string(val);
				}
				else if(strcmp(attr, "symSpace") == 0)
				{
					// A symbol space has been specified - look it up
					ThisClass->SymSpace = SymbolSpace::FindSymbolSpace(val);

					// If it does not already exist, create it
					if(!ThisClass->SymSpace) ThisClass->SymSpace = new SymbolSpace(val);
				}
				else if(strcmp(attr, "extendSubs") == 0)
				{
					if((strcasecmp(val, "true") == 0) || (strcasecmp(val, "yes") == 0))ThisClass->ExtendSubs = true;
					else ThisClass->ExtendSubs = false;
				}
				else if(strcmp(attr, "baseline") == 0)
				{
					if((strcasecmp(val, "true") == 0) || (strcasecmp(val, "yes") == 0)) ThisClass->IsBaseline = true;
					else ThisClass->IsBaseline = false;
				}
				else if(strcmp(attr, "doc") == 0)
				{
					// Ignore any documentation attributes
				}
				else
				{
					XML_warning(user_data, "Unexpected attribute '%s' in <%s/>", attr, name);
				}
			}
		}

		if((ThisClass->Class == ClassSet) || (ThisClass->Class == ClassPack) || (ThisClass->Class == ClassNULL))
		{
			// DRAGONS: key format is carried in MinSize when defining a set and length format is carried in MaxSize when defining a set or pack
			ThisClass->MinSize = ThisKeyFormat;
			ThisClass->MaxSize = ThisLenFormat;
		}

		/* If only a 'key' is given index it with global key as well */
		if(!HasGlobalKey)
		{
			if(Key) GlobalKey = new DataChunk(Key);
		}

		// Build UL from global key
		if(GlobalKey)
		{
			if(GlobalKey->Size != 16)
			{
				error("Global key for %s is not 16 bytes\n", ThisClass->Name.c_str());
			}
			else
			{
				ThisClass->UL = new UL(GlobalKey->Data);
			}
		}

		// Build local tag from key (if local)
		if(Key && (Key->Size != 16))
		{
			if(Key->Size != 2)
			{
				error("Only 2-byte local tags currently supported, tag size for %s is %d\n", ThisClass->Name.c_str(), (int)Key->Size);
			}
			else
			{
				ThisClass->Tag = Key->Data[1] + (Key->Data[0] << 8);
			}
		}

		// If there is a parent class (in the level above) add us as a child of it
		if(Parent)
		{
			Parent->Children.push_back(ThisClass);
		}

		// Add this class to the list of classes (one class per level)
		State->ClassList.push_back(ThisClass);

		return Ret;
	}
}


namespace
{
	//! XML callback - Deal with end tag of an element
	void DictLoad_endElement(void *user_data, const char *name)
	{
		DictParserState *State = (DictParserState*)user_data;

		// If we have finished the classes dictionary then we are idle again
		if(State->State == DictStateClasses)
		{
			if(strcmp(name, "MXFDictionary") == 0) State->State = DictStateIdle;
			else if(strcmp(name, "MXFClasses") == 0) 
			{
				State->State = DictStateDictionary;
				State->DefaultSymbolSpace = State->DictSymbolSpace;
				State->ClassList.clear();
			}
			else
			{
				// If we have arrived back at the top level we make the current item (with its children)
				if(State->ClassList.size() == 1)
				{
					ClassRecordList::iterator it = State->ClassList.end();
					State->ClassesToBuild.push_back(*(--it));
				}

				// Remove the most recent level from the class list
				State->ClassList.pop_back();
			}

			return;
		}

		if(State->State == DictStateTypes)
		{
			// Call the old parser
			DefTypes_endElement(&State->ClassState, name);

			// Do a load if we have hit the end of the types
			if(State->ClassState.State == StateDone)
			{
				// Load the types that were found
				LoadTypes(State->ClassState.Types);

				// Clear these types now they have been loaded
				State->ClassState.Types.clear();

				// Back to the outer level of the dictionary
				State->State = DictStateDictionary;
			}

			return;
		}

		if(State->State == DictStateDictionary)
		{
			if(strcmp(name, "MXFDictionary") == 0) State->State = DictStateIdle;
			return;
		}

		if(State->State != DictStateDictionary)
		{
			// Allow MXF dictionaries to be wrapped inside other XML files
			debug("Stepping out of outer level <%s>\n", name);
		}
	}
}
