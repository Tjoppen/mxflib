/*! \file	deftypes.cpp
 *	\brief	Dictionary processing
 *
 *	\version $Id: deftypes.cpp,v 1.5 2005/03/25 13:18:51 terabrit Exp $
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

//! Type to map type names to their handling traits
typedef std::map<std::string,MDTraitsPtr> TraitsMapType;

//! Map of type names to thair handling traits
static TraitsMapType TraitsMap;

//! Lookup a Trait by name
const MDTraits*  mxflib::LookupTraits(const char* TraitsName) { return TraitsMap[TraitsName]; }


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
	TraitsMap.insert(TraitsMapType::value_type("Uint8", new MDTraits_Uint8));
	TraitsMap.insert(TraitsMapType::value_type("UInt8", new MDTraits_Uint8));
	TraitsMap.insert(TraitsMapType::value_type("Internal-UInt8", new MDTraits_Uint8));
	TraitsMap.insert(TraitsMapType::value_type("Int16", new MDTraits_Int16));
	TraitsMap.insert(TraitsMapType::value_type("Uint16", new MDTraits_Uint16));
	TraitsMap.insert(TraitsMapType::value_type("UInt16", new MDTraits_Uint16));
	TraitsMap.insert(TraitsMapType::value_type("Int32", new MDTraits_Int32));
	TraitsMap.insert(TraitsMapType::value_type("Uint32", new MDTraits_Uint32));
	TraitsMap.insert(TraitsMapType::value_type("UInt32", new MDTraits_Uint32));
	TraitsMap.insert(TraitsMapType::value_type("Int64", new MDTraits_Int64));
	TraitsMap.insert(TraitsMapType::value_type("Uint64", new MDTraits_Uint64));
	TraitsMap.insert(TraitsMapType::value_type("UInt64", new MDTraits_Uint64));

	TraitsMap.insert(TraitsMapType::value_type("ISO7", new MDTraits_ISO7));
	TraitsMap.insert(TraitsMapType::value_type("UTF16", new MDTraits_UTF16));

	TraitsMap.insert(TraitsMapType::value_type("ISO7String", new MDTraits_BasicStringArray));
	TraitsMap.insert(TraitsMapType::value_type("UTF16String", new MDTraits_UTF16String));
	TraitsMap.insert(TraitsMapType::value_type("Uint8Array", new MDTraits_RawArray));
	TraitsMap.insert(TraitsMapType::value_type("UInt8Array", new MDTraits_RawArray));

	TraitsMap.insert(TraitsMapType::value_type("UUID", new MDTraits_UUID));
	TraitsMap.insert(TraitsMapType::value_type("Label", new MDTraits_Label));

	TraitsMap.insert(TraitsMapType::value_type("UMID", new MDTraits_UMID));

	TraitsMap.insert(TraitsMapType::value_type("LabelCollection", new MDTraits_RawArrayArray));

	TraitsMap.insert(TraitsMapType::value_type("Rational", new MDTraits_Rational));
	TraitsMap.insert(TraitsMapType::value_type("Timestamp", new MDTraits_TimeStamp));
}

//! XML parsing functions for defining types
void DefTypes_startElement(void *user_data, const char *name, const char **attrs);
void DefTypes_endElement(void *user_data, const char *name);

/* XML Error handling functions - with file scope */
namespace
{
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
	enum CurrentState
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

	//! State structure for XML parsing
	struct ParserState
	{
		CurrentState State;					//!< Current state of the parser state-machine
		TypeRecordList Types;				//!< The types being built
		TypeRecordPtr Compound;				//!< The current compound being built (or NULL)
	};
}


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
	ParserState State;

	// Initialize the state
	State.State = StateIdle;

	std::string XMLFilePath = LookupDictionaryPath(TypesFile);

	// Parse the file
	bool result = false;
	
	if(XMLFilePath.size()) result = XMLParserParseFile(&DefTypes_XMLHandler, &State, XMLFilePath.c_str());
	if (!result)
	{
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
		}
	
		it++;
	}


	// Resolve any remaining entries
	int UnresolvedCount = Unresolved.size();
	if(UnresolvedCount)
	{
		// Unless we were stuck this time (cannot resolve any more)
		if(UnresolvedCount == TypesData.size())
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



//! XML callback - Deal with start tag of an element
void DefTypes_startElement(void *user_data, const char *name, const char **attrs)
{
	ParserState *State = (ParserState*)user_data;

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


//! XML callback - Deal with end tag of an element
void DefTypes_endElement(void *user_data, const char *name)
{
	ParserState *State = (ParserState*)user_data;

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


//! Load classes from the specified in-memory definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadClasses(ClassRecordList &ClassesData)
{
	//! List to hold any entries that are not resolved during this pass (we will recurse to resolve them at the end of the pass)
	ClassRecordList Unresolved;

	// Iterate through the list
	ClassRecordList::iterator it = ClassesData.begin();
	while(it != ClassesData.end())
	{
		MDOTypePtr ThisType = MDOType::DefineClass(*it);

		// If anything went wrong with this definition stack it for later
		if(!ThisType) Unresolved.push_back(*it);

		it++;
	}

	// Resolve any remaining entries
	int UnresolvedCount = Unresolved.size();
	if(UnresolvedCount)
	{
		// Unless we were stuck this time (cannot resolve any more)
		if(UnresolvedCount == ClassesData.size())
		{
			error("Undefined base class or circular reference in class definitions\n");
			return -1;
		}

		// Recurse...
		LoadClasses(Unresolved);
	}

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
		
		Uint8 ULBuffer[16];
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
int mxflib::LoadClasses(const ConstClassRecord *ClassData)
{
	// Pointer to walk through the array
	const ConstClassRecord *CurrentClass = ClassData;

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
	return LoadClasses(Classes);
}


//! Define a class from an in-memory dictionary definition
MDOTypePtr MDOType::DefineClass(ClassRecordPtr &ThisClass, MDOTypePtr Parent /*=NULL*/)
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

//	// Are we redefining an existing class (perhaps because the first attempt was postponed)?
//	bool RedefineClass = true;

	// Work out the root name of this class (showing the list of parents)
	std::string RootName ;
	if(Parent) RootName = Parent->FullName() + "/";

	// Locate this type if it already exists
	MDOTypePtr Ret = MDOType::Find(RootName + ThisClass->Name);

	// This class does not already exist so add it
	if(!Ret)
	{
//		RedefineClass = false;					// We are adding a new class rather than redefining one

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
		// Are we redefining a base class?
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
	else
	{
		// If it is a top level type then add it to TopTypes as well
		TopTypes.push_back(Ret);
	}

	// Add to the list of all types
	AllTypes.push_back(Ret);

	// Sort referencing (overrides anything inherited)
	if(ThisClass->RefType != ClassRefNone)
	{
		Ret->RefType = ThisClass->RefType;
		Ret->RefTargetName = ThisClass->RefTarget;
	}
	
//	// Set the name lookup - UL lookup set when key set
//	NameLookup[RootName + Ret->DictName] = Ret;

	// Set the local tag (if one exists)
	if(ThisClass->Tag)
	{
		Ret->Key.Resize(2);
		PutU16(ThisClass->Tag, Ret->Key.Data);
	}

	// Set the global key (if one exists)
	if(ThisClass->UL)
	{
		Ret->GlobalKey.Set(16, ThisClass->UL->GetValue());

		// If we don't have a "key" set this global key as the key
		if(Ret->Key.Size == 0) Ret->Key.Set(16, ThisClass->UL->GetValue());

		Ret->TypeUL = ThisClass->UL;
		ULLookup[UL(Ret->TypeUL)] = Ret;
		// printf("CLASS %s UL set\n", Ret->DictName.c_str());
	}
	// else printf("CLASS %s has no UL\n", Ret->DictName.c_str());

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
		MDOTypePtr Child = DefineClass(*it, Ret);

		// If the child was not added quit this attempt (deliberately returning the NULL)
		if(!Child) return Child;

		it++;
	}

	return Ret;
}

