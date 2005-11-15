/*! \file	deftypes.cpp
 *	\brief	Dictionary processing
 *
 *	\version $Id: deftypes.cpp,v 1.13 2005/11/15 12:15:09 matt-beard Exp $
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

#include <stdarg.h>


using namespace mxflib;

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
	static XMLParserHandler DefTypes_XMLHandler = {
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
		StateDone							//!< Finished processing
	};

	//! State structure for XML parsing types file
	struct TypesParserState
	{
		TypesCurrentState State;			//!< Current state of the parser state-machine
		TypeRecordList Types;				//!< The types being built
		TypeRecordPtr Compound;				//!< The current compound being built (or NULL)
	};

	//! Type to map type names to their handling traits
	typedef std::map<std::string,MDTraitsPtr> TraitsMapType;

	//! Map of type names to thair handling traits
	static TraitsMapType TraitsMap;

	
	//! Build the map of all known traits
	static void DefineTraits(void)
	{
		// Not a real type, but the default for basic types
		TraitsMap.insert(TraitsMapType::value_type("Default-Basic", new MDTraits_Raw));

		// Not a real type, but the default for array types
		TraitsMap.insert(TraitsMapType::value_type("Default-Array", new MDTraits_BasicArray));

		// Not a real type, but the default for compound types
		TraitsMap.insert(TraitsMapType::value_type("Default-Compound", new MDTraits_BasicCompound));

		TraitsMap.insert(TraitsMapType::value_type("RAW", new MDTraits_Raw));

		TraitsMap.insert(TraitsMapType::value_type("Int8", new MDTraits_Int8));
		TraitsMap.insert(TraitsMapType::value_type("UInt8", new MDTraits_UInt8));
		TraitsMap.insert(TraitsMapType::value_type("Uint8", new MDTraits_UInt8));
		TraitsMap.insert(TraitsMapType::value_type("Internal-UInt8", new MDTraits_UInt8));
		TraitsMap.insert(TraitsMapType::value_type("Int16", new MDTraits_Int16));
		TraitsMap.insert(TraitsMapType::value_type("UInt16", new MDTraits_UInt16));
		TraitsMap.insert(TraitsMapType::value_type("Uint16", new MDTraits_UInt16));
		TraitsMap.insert(TraitsMapType::value_type("Int32", new MDTraits_Int32));
		TraitsMap.insert(TraitsMapType::value_type("UInt32", new MDTraits_UInt32));
		TraitsMap.insert(TraitsMapType::value_type("Uint32", new MDTraits_UInt32));
		TraitsMap.insert(TraitsMapType::value_type("Int64", new MDTraits_Int64));
		TraitsMap.insert(TraitsMapType::value_type("UInt64", new MDTraits_UInt64));
		TraitsMap.insert(TraitsMapType::value_type("Uint64", new MDTraits_UInt64));

		TraitsMap.insert(TraitsMapType::value_type("ISO7", new MDTraits_ISO7));
		TraitsMap.insert(TraitsMapType::value_type("UTF16", new MDTraits_UTF16));

		TraitsMap.insert(TraitsMapType::value_type("ISO7String", new MDTraits_BasicStringArray));
		TraitsMap.insert(TraitsMapType::value_type("UTF16String", new MDTraits_UTF16String));
		TraitsMap.insert(TraitsMapType::value_type("UInt8Array", new MDTraits_RawArray));
		TraitsMap.insert(TraitsMapType::value_type("Uint8Array", new MDTraits_RawArray));

		TraitsMap.insert(TraitsMapType::value_type("UUID", new MDTraits_UUID));
		TraitsMap.insert(TraitsMapType::value_type("Label", new MDTraits_Label));

		TraitsMap.insert(TraitsMapType::value_type("UMID", new MDTraits_UMID));

		TraitsMap.insert(TraitsMapType::value_type("LabelCollection", new MDTraits_RawArrayArray));

		TraitsMap.insert(TraitsMapType::value_type("Rational", new MDTraits_Rational));
		TraitsMap.insert(TraitsMapType::value_type("Timestamp", new MDTraits_TimeStamp));
	}

	//! Set true once the basic required classes have been loaded
	static bool BasicClassesDefined = false;
}

#ifdef TRAITS_HACK
//FIXME
//This is only needed for the moment until user extensions to the traits are implemented
//It is not recommended that you use the function as it will be unceremoniously removed when it can be.
//! Lookup a Trait by name
const MDTraits*  mxflib::LookupTraits(const char* TraitsName) { return TraitsMap[TraitsName]; };


#endif

//! Load types from the specified XML definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadTypes(char *TypesFile)
{
	// Define the known traits
	// Test before calling as two partial definition files could be loaded!
	if(TraitsMap.empty()) DefineTraits();

	// State data block passed through XML parser
	TypesParserState State;

	// Initialize the state
	State.State = StateIdle;

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


//! Load types from the specified in-memory definitions
/*! \note The last entry in the array must be a terminating entry with Class == TypeNULL
 *  \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadTypes(const ConstTypeRecord *TypesData)
{
	// Pointer to walk through the array
	const ConstTypeRecord *CurrentType = TypesData;

	// Run-time list of types
	TypeRecordList Types;

	while(CurrentType->Class != TypeNULL)
	{
		TypeRecordPtr ThisType = new TypeRecord;

		// Copy over the attributes
		ThisType->Class = CurrentType->Class;
		ThisType->Type = CurrentType->Type;
		ThisType->Detail = CurrentType->Detail;
		ThisType->Base = CurrentType->Base;
		ThisType->Size = CurrentType->Size;
		ThisType->Endian = CurrentType->Endian;
		ThisType->IsBatch = CurrentType->IsBatch;

		// Add all children to compounds
		if(CurrentType->Class == TypeCompound)
		{
			CurrentType++;
			while(CurrentType->Class == TypeSub)
			{
				TypeRecordPtr SubType = new TypeRecord;

				// Copy over the attributes
				SubType->Class = CurrentType->Class;
				SubType->Type = CurrentType->Type;
				SubType->Detail = CurrentType->Detail;
				SubType->Base = CurrentType->Base;
				SubType->Size = CurrentType->Size;
				SubType->Endian = CurrentType->Endian;
				SubType->IsBatch = CurrentType->IsBatch;

				// Add this child to the current compound
				ThisType->Children.push_back(SubType);

				CurrentType++;
			}
		}
		else
		{
			CurrentType++;
		}

		Types.push_back(ThisType);
	}

	// Load the types from the new in-memory list
	return LoadTypes(Types);
}


// Basic "internally required" types (enough to hold an "unknown")
namespace mxflib
{
	MXFLIB_TYPE_START(BasicInternalTypes)
		MXFLIB_TYPE_BASIC("Internal-UInt8", "Internally used 8 bit unsigned integer", 1, false)
		MXFLIB_TYPE_MULTIPLE("Unknown", "Array of bytes", "Internal-UInt8", false, 0)
	MXFLIB_TYPE_END
}


//! Load types from the specified in-memory definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadTypes(TypeRecordList &TypesData)
{
	// Define the basic "internally required" types (enough to hold an "unknown")
	static bool BasicDefined = false;
	if(!BasicDefined)
	{
		BasicDefined = true;
		LoadTypes(BasicInternalTypes);
	}

	// Define the known traits if required
	if(TraitsMap.empty()) DefineTraits();

	//! List to hold any entries that are not resolved during this pass (we will recurse to resolve them at the end of the pass)
	TypeRecordList Unresolved;

	// Iterate through the list
	TypeRecordList::iterator it = TypesData.begin();
	while(it != TypesData.end())
	{
		switch((*it)->Class)
		{
			// Basic type definition
			case TypeBasic:
			{
				MDTypePtr Ptr = MDType::AddBasic((*it)->Type, (*it)->Size);
				if((*it)->Endian) Ptr->SetEndian(true);

				MDTraitsPtr Traits = TraitsMap[(*it)->Type];
				if(!Traits) Traits = TraitsMap["Default-Basic"];

				if(Traits) Ptr->SetTraits(Traits);

				break;
			}

			// Interpretation type
			case TypeInterpretation:
			{
				MDTypePtr BaseType = MDType::Find((*it)->Base);
				if(!BaseType)
				{
					debug("Interpretation \"%s\" is based on (as yet) undefined base \"%s\"\n", (*it)->Type.c_str(), (*it)->Base.c_str());

					// Add to the "do later" pile
					Unresolved.push_back(*it);
				}
				else
				{
					MDTypePtr Ptr = MDType::AddInterpretation((*it)->Type, BaseType, (*it)->Size);

					MDTraitsPtr Traits = TraitsMap[(*it)->Type];

					// If we don't have specific traits for this type
					// it will inherit the base type's traits
					if(Traits) Ptr->SetTraits(Traits);
				}

				break;
			}

			// Multiple type
			case TypeMultiple:
			{
				MDTypePtr BaseType = MDType::Find((*it)->Base);
				if(!BaseType)
				{
					debug("Multiple \"%s\" is based on (as yet) undefined base \"%s\"\n", (*it)->Type.c_str(), (*it)->Base.c_str());

					// Add to the "do later" pile
					Unresolved.push_back(*it);
				}
				else
				{
					MDTypePtr Ptr = MDType::AddArray((*it)->Type, BaseType, (*it)->Size);
					if((*it)->IsBatch) Ptr->SetArrayClass(ARRAYBATCH);

					MDTraitsPtr Traits = TraitsMap[(*it)->Type];
					if(!Traits) Traits = TraitsMap["Default-Array"];
					if(Traits) Ptr->SetTraits(Traits);
				}

				break;
			}

			// Compound type
			case TypeCompound:
			{
				// First check that we currently have all types required
				TypeRecordList::iterator subit = (*it)->Children.begin();
				while(subit != (*it)->Children.end())
				{
					MDTypePtr SubType = MDType::Find((*subit)->Base);
					if(!SubType)
					{
						debug("Compound item \"%s\" in \"%s\" is based on (as yet) undefined base \"%s\"\n", (*subit)->Type.c_str(), (*it)->Type.c_str(), (*subit)->Base.c_str());

						// Add to the "do later" pile
						Unresolved.push_back(*it);

						break;
					}
					subit++;
				}
				
				// If we quit the loop due to an unresolved item skip this type
				if(subit != (*it)->Children.end()) break;

				MDTypePtr Ptr = MDType::AddCompound((*it)->Type);

				MDTraitsPtr Traits = TraitsMap[(*it)->Type];
				if(!Traits) Traits = TraitsMap["Default-Compound"];

				if(Traits) Ptr->SetTraits(Traits);

				/* Process sub-items */

				subit = (*it)->Children.begin();
				while(subit != (*it)->Children.end())
				{
					MDTypePtr SubType = MDType::Find((*subit)->Base);
					ASSERT(SubType);

					// Add this child item
					Ptr->insert(MDType::value_type((*subit)->Type, SubType));

					// Add the child to the order list
					Ptr->ChildOrder.push_back((*subit)->Type);
				
					subit++;
				}

				break;
			}

			// Should never be possible to get here
			default:
				ASSERT(0);
		}
	
		it++;
	}


	// Resolve any remaining entries
	int UnresolvedCount = (int)Unresolved.size();
	if(UnresolvedCount)
	{
		// Unless we were stuck this time (cannot resolve any more)
		if(UnresolvedCount == (int)TypesData.size())
		{
			error("Undefined base class or circular reference in types definitions\n");
			return -1;
		}

		// Recurse...
		LoadTypes(Unresolved);
	}

	// All done OK
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
				if(strcmp(name, "MXFTypes") != 0)
				{
					XML_fatalError(user_data, "Outer tag <MXFTypes> expected - <%s> found\n", name);
					return;
				}

				State->State = StateTypes;

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
					XML_error(user_data, "Tag <%s> found when types class expected\n", name);

				break;
			}

			case StateTypesBasic:
			{
				const char *Detail = "";
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
							Detail = val;
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
							// TODO: Implement this
						}
						else if(strcmp(attr, "ref") == 0)
						{
							// Ignore
						}
						else
						{
							XML_error(user_data, "Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
						}
					}
				}

				// Build a new type record
				TypeRecordPtr ThisType = new TypeRecord;

				ThisType->Class = TypeBasic;
				ThisType->Type = name;
				ThisType->Detail = Detail;
				ThisType->Base = "";
				ThisType->Size = Size;
				ThisType->Endian = Endian;
				ThisType->IsBatch = false;

				// Add this type record
				State->Types.push_back(ThisType);

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
				ThisType->Size = Size;
				ThisType->Endian = false;
				ThisType->IsBatch = false;

				// Add this type record
				State->Types.push_back(ThisType);

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
				ThisType->Size = Size;
				ThisType->Endian = false;
				ThisType->IsBatch = IsBatch;

				// Add this type record
				State->Types.push_back(ThisType);

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
				ThisType->Size = 0;
				ThisType->Endian = false;
				ThisType->IsBatch = false;

				// Add this type record
				State->Types.push_back(ThisType);

				State->State = StateTypesCompoundItem;
				State->Compound = ThisType;

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

				// Build a new type record
				TypeRecordPtr ThisType = new TypeRecord;

				ThisType->Class = TypeSub;
				ThisType->Type = name;
				ThisType->Detail = Detail;
				ThisType->Base = Type;
				ThisType->Size = Size;
				ThisType->Endian = false;
				ThisType->IsBatch = false;

				// Add as a child of the current compound
				State->Compound->Children.push_back(ThisType);

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
				if(strcmp(name,State->Compound->Type.c_str()) == 0)
				{
					State->State = StateTypesCompound;
					State->Compound = NULL;
				}
				break;
			}
		}
	}
}


// Basic "internally required" class (enough to hold an "Unknown")
namespace mxflib
{
	MXFLIB_CLASS_START(BasicInternalClasses)
		MXFLIB_CLASS_ITEM("Unknown", "Unknown Set", ClassUsageOptional, "Unknown", 0, 0, 0x0000, "", NULL, NULL)
	MXFLIB_CLASS_END
}

//! Load classes from the specified in-memory definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadClasses(ClassRecordList &ClassesData, SymbolSpacePtr DefaultSymbolSpace /*=MXFLibSymbols*/)
{
	// Define the basic "internally required" classes (enough to hold an "Unknown")
	if(!BasicClassesDefined)
	{
		BasicClassesDefined = true;
		LoadClasses(BasicInternalClasses, MXFLibSymbols);
	}

	//! List to hold any entries that are not resolved during this pass (we will recurse to resolve them at the end of the pass)
	ClassRecordList Unresolved;

	// Iterate through the list
	ClassRecordList::iterator it = ClassesData.begin();
	while(it != ClassesData.end())
	{
		ClassRecordPtr Dat = (*it);
		if((*it)->Class == ClassSymbolSpace)
		{
			// A symbol space has been specified - look it up
			DefaultSymbolSpace = SymbolSpace::FindSymbolSpace((*it)->SymSpace);

			// If it does not already exist, create it
			if(!DefaultSymbolSpace) DefaultSymbolSpace = new SymbolSpace((*it)->SymSpace);
		}
		else
		{
			// All other entries are used to build classes
			MDOTypePtr ThisType = MDOType::DefineClass(*it, DefaultSymbolSpace);

			// If anything went wrong with this definition stack it for later
			if(!ThisType) Unresolved.push_back(*it);
		}

		it++;
	}

	// Resolve any remaining entries
	int UnresolvedCount = (int)Unresolved.size();
	if(UnresolvedCount)
	{
		// Unless we were stuck this time (cannot resolve any more)
		if(UnresolvedCount == (int)ClassesData.size())
		{
			error("Undefined base class or circular reference in class definitions\n");
			return -1;
		}

		// Recurse...
		LoadClasses(Unresolved, DefaultSymbolSpace);
	}

	// Build a static primer (for use in index tables)
	MDOType::MakePrimer(true);

	return 0;
}


namespace
{
	typedef ConstClassRecord const *ConstClassRecordPTR;

	//! File local function to build class list from the specified in-memory definitions
	/*! This function is called by LoadClasses() and is recursive
	 *  DRAGONS: ClassData is changed by LoadClassesSub - at return it points to the next peer entry
	 *  \note There must be enough terminating entries (with Class == TypeNULL) to end any children
	 *  \return The root class, or NULL on error
	 */
	ClassRecordPtr LoadClassesSub(ConstClassRecordPTR &ClassData)
	{
		ClassRecordPtr ThisClass = new ClassRecord;

		// Copy over the attributes
		ThisClass->Class = ClassData->Class;
		ThisClass->MinSize = ClassData->MinSize;
		ThisClass->MaxSize = ClassData->MaxSize;
		ThisClass->Name = ClassData->Name;
		ThisClass->Detail = ClassData->Detail;
		ThisClass->Usage = ClassData->Usage;
		ThisClass->Base = ClassData->Base;
		ThisClass->Tag = ClassData->Tag;
		
		UInt8 ULBuffer[16];
		int Count = mxflib::ReadHexString(ClassData->UL, 16, ULBuffer, " \t.");

		if(Count == 16) 
		{
			ThisClass->UL = new UL(ULBuffer);
			// printf("Class %s : UL = %02x %02x %02x ...\n", ThisClass->Name.c_str(), ULBuffer[0], ULBuffer[1], ULBuffer[2]);
		}
		// else printf("Class %s : * NO-UL * Count = %d\n", ThisClass->Name.c_str(), Count);

		if(ClassData->Default)
		{
			ThisClass->Default = ClassData->Default;
			ThisClass->HasDefault = true;
		}
		else
			ThisClass->HasDefault = false;

		if(ClassData->DValue)
		{
			ThisClass->DValue = ClassData->DValue;
			ThisClass->HasDValue = true;
		}
		else
			ThisClass->HasDValue = false;

		ThisClass->RefType = ClassData->RefType;
		ThisClass->RefTarget = ClassData->RefTarget;

		if(ClassData->SymSpace == NULL) ThisClass->SymSpace = "";
		else ThisClass->SymSpace = ClassData->SymSpace;

		ThisClass->ExtendSubs = ClassData->ExtendSubs;

		// Add any children
		if((ClassData->Class == ClassSet) || (ClassData->Class == ClassPack) 
			|| (ClassData->Class == ClassVector) || (ClassData->Class == ClassArray) )
		{
			// Move to the first child
			ClassData++;
			while(ClassData->Class != ClassNULL)
			{
				// DRAGONS: ClassData is changed by LoadClassesSub
				ClassRecordPtr Child = LoadClassesSub(ClassData);
				
				// Propergate error flag by returning the NULL
				if(!Child) return Child;

				ThisClass->Children.push_back(Child);
			}
		}

		// Move to the next peer
		ClassData++;

		return ThisClass;
	}
}


//! Load classeses from the specified in-memory definitions
/*! \note There must be enough terminating entries (with Class == TypeNULL) to end the list
 *  \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadClasses(const ConstClassRecord *ClassData, SymbolSpacePtr DefaultSymbolSpace /*=MXFLibSymbols*/)
{
	// Run-time list of classes
	ClassRecordList Classes;

	// Add top-level classes (lower levels will be added for each top-level class)
	while(ClassData->Class != ClassNULL)
	{
		// DRAGONS: ClassData is changed by LoadClassesSub
		ClassRecordPtr ThisClass = LoadClassesSub(ClassData);
		
		// Propergate error flag
		if(!ThisClass) return -1;

		Classes.push_back(ThisClass);
	}

	// Load the classes from the new in-memory list
	return LoadClasses(Classes, DefaultSymbolSpace);
}


//! Define a class from an in-memory dictionary definition
MDOTypePtr MDOType::DefineClass(ClassRecordPtr &ThisClass, SymbolSpacePtr DefaultSymbolSpace, MDOTypePtr Parent /*=NULL*/)
{
	//! Lookup-table to convert key size to key format enum
	static const DictKeyFormat KeyConvert[] =
	{
		DICT_KEY_NONE,
		DICT_KEY_1_BYTE,
		DICT_KEY_2_BYTE,
		DICT_KEY_AUTO,
		DICT_KEY_4_BYTE
	};

	//! Lookup-table to convert lenght size to len format enum
	static const DictLenFormat LenConvert[] =
	{
		DICT_LEN_NONE,
		DICT_LEN_1_BYTE,
		DICT_LEN_2_BYTE,
		DICT_LEN_BER,
		DICT_LEN_4_BYTE
	};

	// Does this entry have a valid UL (rather than a UUID)
	bool ValidUL = false;

	// The UL for this type
	ULPtr TypeUL;

	// Decide the type UL
	if(ThisClass->UL)
	{
		TypeUL = ThisClass->UL;
		ValidUL = true;
	}
	else
	{
		/* If no valid UL given we use an end-swapped UUID to allow lookups */

		// Build a UL from a new UUID
		TypeUL = new UL(new UUID);
	}

	// Work out the root name of this class (showing the list of parents)
	std::string RootName ;
	if(Parent) RootName = Parent->FullName() + "/";

	// Locate this type if it already exists (by UL if possible, else by name)
	MDOTypePtr Ret; 
	if(ValidUL && Parent) 
		Ret = Parent->Child(TypeUL);
	else
		Ret = MDOType::Find(RootName + ThisClass->Name);

	// Initially assume that we aren't extending
	bool Extending = false;

	// Are we extending an existing definition?
	if(Ret)
	{
		Extending = true;

		// If we extend an "item" then we will replace it
		if(ThisClass->Class == ClassItem)
		{
			Ret->ReDefine(ThisClass->Detail, ThisClass->Base, ThisClass->MinSize, ThisClass->MaxSize);
		}
		else
		{
			// Redefining a container can only change the detailed description
			Ret->ReDefine(ThisClass->Detail);
		}
	}
	else	// This class does not already exist so add it
	{
		if(ThisClass->Class == ClassItem)
		{
			// Find the type of this item
			MDTypePtr Type = MDType::Find(ThisClass->Base);
			if(!Type)
			{
				XML_error(NULL, "Item %s is of type %s which is not known\n", ThisClass->Name.c_str(), ThisClass->Base.c_str());
				return Ret;
			}

			Ret = new MDOType(NONE, RootName, ThisClass->Name, ThisClass->Detail, Type, DICT_KEY_NONE, DICT_LEN_NONE, ThisClass->MinSize, ThisClass->MaxSize, ThisClass->Usage);
		}
		// Are we defining a derived class?
		else if(ThisClass->Base.size())
		{
			MDOTypePtr BaseType = MDOType::Find(ThisClass->Base);

			// If the base type not found quit this attempt (deliberately returning the NULL)
			if(!BaseType) return BaseType;

			debug("Deriving %s from %s\n", ThisClass->Name.c_str(), BaseType->Name().c_str());

			// Derive the type
			Ret = new MDOType;
			if(Ret)
			{
				Ret->RootName = RootName;
				Ret->DictName = ThisClass->Name;

				Ret->Derive(BaseType);
			
				Ret->Detail = ThisClass->Detail;
				Ret->Use = ThisClass->Usage;

				// Set the name lookup - UL lookup set when key set
				NameLookup[RootName + ThisClass->Name] = Ret;
			}
		}
		else if(ThisClass->Class == ClassArray)
			Ret = new MDOType(ARRAY, RootName, ThisClass->Name, ThisClass->Detail, NULL, DICT_KEY_NONE, DICT_LEN_NONE, 0, 0, ThisClass->Usage);
		else if(ThisClass->Class == ClassVector)
			Ret = new MDOType(BATCH, RootName, ThisClass->Name, ThisClass->Detail, NULL, DICT_KEY_NONE, DICT_LEN_NONE, 0, 0, ThisClass->Usage);
		else if(ThisClass->Class == ClassPack)
		{
			if(ThisClass->MaxSize > 4)
			{
				error("Item %s has an invalid length size of %u\n", ThisClass->Name.c_str(), ThisClass->MaxSize);
				return Ret;
			}
			DictLenFormat LenFormat = LenConvert[ThisClass->MaxSize];

			Ret = new MDOType(PACK, RootName, ThisClass->Name, ThisClass->Detail, NULL, DICT_KEY_NONE, LenFormat, 0, 0, ThisClass->Usage);
		}
		else if(ThisClass->Class == ClassSet)
		{
			if(ThisClass->MinSize > 4)
			{
				error("Item %s has an invalid tag size of %u\n", ThisClass->Name.c_str(), ThisClass->MinSize);
				return Ret;
			}
			DictKeyFormat KeyFormat = KeyConvert[ThisClass->MinSize];

			if(ThisClass->MaxSize > 4)
			{
				error("Item %s has an invalid length size of %u\n", ThisClass->Name.c_str(), ThisClass->MaxSize);
				return Ret;
			}
			DictLenFormat LenFormat = LenConvert[ThisClass->MaxSize];

			Ret = new MDOType(SET, RootName, ThisClass->Name, ThisClass->Detail, NULL, KeyFormat, LenFormat, 0, 0, ThisClass->Usage);
		}
		else 
			ASSERT(0);							 // Not a valid class type

		// Quit now if the create failed
		if(!Ret) return Ret;
	}

	// Add us to the class lists
	if(!Extending)
	{
		if(Parent)
		{
			// Set our parent
			Ret->Parent = Parent;

			// Add us as a child of our parent
			Parent->insert(Ret);

			// Move reference details from parent (used for vectors)
			if(Parent->RefType != DICT_REF_NONE)
			{
				Ret->RefType = Parent->RefType;
				Parent->RefType = DICT_REF_NONE;
			}

			// If we are not top level then record out "family tree"
			Ret->RootName = Parent->FullName() + "/";
		}
	}

	// Sort referencing (overrides anything inherited)
	if(ThisClass->RefType != ClassRefNone)
	{
		Ret->RefType = ThisClass->RefType;
		Ret->RefTargetName = ThisClass->RefTarget;
	}

	// Set the local tag (if one exists)
	if(ThisClass->Tag)
	{
		Ret->Key.Resize(2);
		PutU16(ThisClass->Tag, Ret->Key.Data);
	}

	// Determine the symbol space to use for this and any children - this is done irrespective of
	// whether a UL exists for this item as thier may be children that have a UL defined
	SymbolSpacePtr ThisSymbolSpace;
	if(ThisClass->SymSpace.size())
	{
		/* A symbol space has been specified - look it up */
		ThisSymbolSpace = SymbolSpace::FindSymbolSpace(ThisClass->SymSpace);

		// If it does not already exist, create it
		if(!ThisSymbolSpace) ThisSymbolSpace = new SymbolSpace(ThisClass->SymSpace);
	}
	else 
	{
		ThisSymbolSpace = DefaultSymbolSpace;
	}

	// Set the global key (even if we have to use the UUID generated above)
	if(!Extending)
	{
		Ret->GlobalKey.Set(16, TypeUL->GetValue());

		// If we don't have a tag set this global key as the key
		if(ThisClass->Tag == 0) Ret->Key.Set(16, TypeUL->GetValue());

		Ret->TypeUL = TypeUL;
	}

	// Set the default value (if one exists)
	if(ThisClass->HasDefault)
	{
		if(Ret->ValueType)
		{
			MDValuePtr Val = new MDValue(Ret->ValueType);
			if(Val)
			{
				Val->SetString(ThisClass->Default);
				DataChunkPtr Temp = Val->PutData();
				Ret->Default.Set(Temp);
			}
		}
	}

	// Set the distinguished value (if one exists)
	if(ThisClass->HasDValue)
	{
		if(Ret->ValueType)
		{
			MDValuePtr Val = new MDValue(Ret->ValueType);
			if(Val)
			{
				Val->SetString(ThisClass->DValue);
				DataChunkPtr Temp = Val->PutData();
				Ret->DValue.Set(Temp);
			}
		}
	}

	// Build all children
	ClassRecordList::iterator it = ThisClass->Children.begin();
	while(it != ThisClass->Children.end())
	{
		// Propogate the extension flag to our children
		(*it)->ExtendSubs = (*it)->ExtendSubs && ThisClass->ExtendSubs;

		MDOTypePtr Child = DefineClass(*it, Ret, ThisSymbolSpace);

		// If the child was not added quit this attempt (deliberately returning the NULL)
		if(!Child) return Child;

		it++;
	}

	/* Add this new class to the lookups - this is done after building children so we can fail safely if children not built */
	if(!Extending)
	{
		ULLookup[*TypeUL] = Ret;

		// Add the name and UL to the symbol space
		ThisSymbolSpace->AddSymbol(Ret->FullName(), TypeUL);
	}

	if(!Parent)
	{
		// If it is a top level type then add it to TopTypes as well
		TopTypes.push_back(Ret);
	}

	// Add to the list of all types
	AllTypes.push_back(Ret);

	/* We need to ensure that any extension to a set or pack is also performed for all derived items,
	 * unless ThisClass->ExtendSubs = false
	 */
	if(Extending && ThisClass->ExtendSubs && (Ret->size() != 0))
	{
		MDOTypeList::iterator it = AllTypes.begin();
		while(it != AllTypes.end())
		{
			// Extend any types that are derived from our use (carefully not adding again to our us)
			if(((*it) != Ret) && ((*it)->IsA(Ret))) (*it)->ReDerive(Ret);
			it++;
		}
	}

	return Ret;
}


//! Load dictionary from the specified in-memory definitions
/*! \return 0 if all OK
 *  \return -1 on error
 *  \note If any part of the dictionary loading fails the loading will continue unless FastFail is set to true
 */
int mxflib::LoadDictionary(DictionaryPtr &DictionaryData, SymbolSpacePtr DefaultSymbolSpace, bool FastFail /*=false*/)
{
	int Ret = 0;

	// Load all the types first
	TypeRecordListList::iterator Types_it = DictionaryData->Types.begin();
	while(Types_it != DictionaryData->Types.end())
	{
		if(LoadTypes(*Types_it) != 0) Ret = -1;
		if(FastFail && (Ret != 0)) return Ret;
		Types_it++;
	}

	// Load all the classes
	ClassRecordListList::iterator Classes_it = DictionaryData->Classes.begin();
	while(Classes_it != DictionaryData->Classes.end())
	{
		if(LoadClasses((*Classes_it), DefaultSymbolSpace) != 0) Ret = -1;
		if(FastFail && (Ret != 0)) return Ret;
		Classes_it++;
	}

	// Locate reference target types for any new types
	MDOType::LocateRefTypes();

	return Ret;
}


//! Load dictionary from the specified in-memory definitions
/*! \note There must be a terminating entry (with Type == DictionaryNULL) to end the list
 *  \return 0 if all OK
 *  \return -1 on error
 *  \note If any part of the dictionary loading fails the loading will continue unless FastFail is set to true
 */
int mxflib::LoadDictionary(const ConstDictionaryRecord *DictionaryData, SymbolSpacePtr DefaultSymbolSpace, bool FastFail /*=false*/)
{
	int Ret = 0;

	while(DictionaryData->Type != DictionaryNULL)
	{
		if(DictionaryData->Type == DictionaryTypes)
		{
			if(LoadTypes((const ConstTypeRecord *)DictionaryData->Dict) != 0) Ret = -1;
			if(FastFail && (Ret != 0)) return Ret;
		}
		else
		{
			if(LoadClasses((const ConstClassRecord *)DictionaryData->Dict, DefaultSymbolSpace) != 0) Ret = -1;
			if(FastFail && (Ret != 0)) return Ret;
		}

		DictionaryData++;
	}

	// Locate reference target types for any new types
	MDOType::LocateRefTypes();

	return Ret;
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
int mxflib::LoadDictionary(const char *DictFile, SymbolSpacePtr DefaultSymbolSpace, bool FastFail /*=false*/)
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

	return 0;
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
				if(strcmp(name, "MXFTypes") == 0)
				{
					/* Start types parsing */

					// Define the known traits
					// Test before calling as two partial definition files could be loaded!
					if(TraitsMap.empty()) DefineTraits();

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
			ClassRecordList::iterator it = State->ClassList.end();
			Parent = *(--it);
		}

		if(!Parent)
		{
			ThisClass->ExtendSubs = true;
		}
		else
		{
			ThisClass->ExtendSubs = Parent->ExtendSubs;		// Carry on extending subs if we were, not if we weren't
		}

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

					Size = ReadHexString(&p, 32, Buffer, " \t.");

					Key = new DataChunk(Size, Buffer);
				}
				else if(strcmp(attr, "globalKey") == 0)
				{
					int Size;
					const char *p = val;
					UInt8 Buffer[32];

					Size = ReadHexString(&p, 32, Buffer, " \t.");

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
					if(strcasecmp(val,"strong") == 0) ThisClass->RefType = DICT_REF_STRONG;
					else if(strcasecmp(val,"target") == 0) ThisClass->RefType = DICT_REF_TARGET;
					else if(strcasecmp(val,"weak") == 0) ThisClass->RefType = DICT_REF_WEAK;
					else
					{
						XML_warning(user_data, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
					}
				}
				else if(strcmp(attr, "type") == 0)
				{
					if(   (strcasecmp(val,"universalSet") == 0) 
					|| (strcasecmp(val,"variablePack") == 0)
					|| (strcasecmp(val,"subVariablePack") == 0) )
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
					// DRAGONS: key format is carried in MinSize when defining a set
					ThisClass->MinSize = (unsigned int)atoi(val);
				}
				else if(strcmp(attr, "lengthFormat") == 0)
				{
					// DRAGONS: length format is carried in MaxSize when defining a set
					if(strcasecmp(val, "BER")==0)
					{
						ThisClass->MaxSize = (unsigned int)DICT_LEN_BER;
					}
					else
					{
						ThisClass->MaxSize = (unsigned int)atoi(val);
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
					ThisClass->SymSpace = std::string(val);
				}
				else if(strcmp(attr, "extendSubs") == 0)
				{
					if((strcasecmp(val, "true") == 0) || (strcasecmp(val, "yes") == 0))ThisClass->ExtendSubs = true;
					else ThisClass->ExtendSubs = false;
				}
				else
				{
					XML_warning(user_data, "Unexpected attribute '%s' in <%s/>", attr, name);
				}
			}
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

			if(strcmp(name, "MXFTypes") == 0)
			{
				// Load the types that were found
				LoadTypes(State->ClassState.Types);

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
