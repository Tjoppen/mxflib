/*! \file	deftypes.cpp
 *	\brief	Dictionary processing
 *
 *	\version $Id: deftypes.cpp,v 1.25 2007/07/17 07:55:47 matt-beard Exp $
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

#include "mxflib/mxflib.h"

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
	//! Set when built-in traits need to be loaded
	bool LoadBuiltInTraits = true;
}

namespace mxflib
{
	//! Disable automatic loading of built-in traits
	/*! \note This needs to be called early as they may have already been loaded!
	 *  \ret false if the traits have already been loaded (or already disabled)
	 */
	bool DisableBuiltInTraits(void)
	{
		// Is it too late?
		if(!LoadBuiltInTraits) return false;

		LoadBuiltInTraits = false;
		return true;
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

	//! Build the map of all known traits
	void DefineTraits(void)
	{
		// Ensure we should be here!
		if (!LoadBuiltInTraits) return;
		LoadBuiltInTraits = false;

		// Not a real type, but the default for basic types
		AddTraitsMapping("Default-Basic", new MDTraits_Raw);

		// Not a real type, but the default for array types
		AddTraitsMapping("Default-Array", new MDTraits_BasicArray);

		// Not a real type, but the default for compound types
		AddTraitsMapping("Default-Compound", new MDTraits_BasicCompound);

		// Not a real type, but the default for enumeration types
		AddTraitsMapping("Default-Enum", new MDTraits_BasicEnum);

		AddTraitsMapping("RAW", new MDTraits_Raw);
		AddTraitsMapping("UnknownType", new MDTraits_Raw);

		AddTraitsMapping("Int8", new MDTraits_Int8);
		AddTraitsMapping("UInt8", new MDTraits_UInt8);
		AddTraitsMapping("Uint8", new MDTraits_UInt8);
		AddTraitsMapping("Internal-UInt8", new MDTraits_UInt8);
		AddTraitsMapping("Int16", new MDTraits_Int16);
		AddTraitsMapping("UInt16", new MDTraits_UInt16);
		AddTraitsMapping("Uint16", new MDTraits_UInt16);
		AddTraitsMapping("Int32", new MDTraits_Int32);
		AddTraitsMapping("UInt32", new MDTraits_UInt32);
		AddTraitsMapping("Uint32", new MDTraits_UInt32);
		AddTraitsMapping("Int64", new MDTraits_Int64);
		AddTraitsMapping("UInt64", new MDTraits_UInt64);
		AddTraitsMapping("Uint64", new MDTraits_UInt64);

		AddTraitsMapping("ISO7", new MDTraits_ISO7);
		AddTraitsMapping("UTF16", new MDTraits_UTF16);

		AddTraitsMapping("ISO7String", new MDTraits_BasicStringArray);
		AddTraitsMapping("UTF16String", new MDTraits_UTF16String);

		// DRAGONS: At the moment we assume all unknown UTF is basically 7-bit text!
		AddTraitsMapping("UTF", new MDTraits_ISO7);
		AddTraitsMapping("UTFString", new MDTraits_BasicStringArray);

		AddTraitsMapping("UInt8Array", new MDTraits_RawArray);
		AddTraitsMapping("Uint8Array", new MDTraits_RawArray);

		AddTraitsMapping("UUID", new MDTraits_UUID);
		AddTraitsMapping("Label", new MDTraits_Label);

		AddTraitsMapping("UMID", new MDTraits_UMID);

		AddTraitsMapping("LabelCollection", new MDTraits_RawArrayArray);

		AddTraitsMapping("Rational", new MDTraits_Rational);
		AddTraitsMapping("Timestamp", new MDTraits_TimeStamp);
	}

	//! Set true once the basic required classes have been loaded
	static bool BasicClassesDefined = false;
}


namespace
{
	//! Read hex values separated by any of 'Sep', if it is a urn read any of the supported formats via StringToUL
	/*! \return number of values read 
	 */
	int ReadHexStringOrUL(const char *Source, int Max, UInt8 *Dest, const char *Sep)
	{
		// Don't even attempt the UL version unless enough bytes
		if(Max >= 16)
		{
			// Anything starting "urn:" goes via StringToUL
			if((tolower(*Source) == 'u') && (tolower(Source[1]) == 'r') && (tolower(Source[2]) == 'n') && (tolower(Source[3]) == ':'))
			{
				// If StringToUL fails we drop through and read the raw hex bytes
				if(StringToUL(Dest, std::string(Source))) return 16;
			}
		}

		return ReadHexString(Source, Max, Dest, Sep);
	}
}


//! Load types from the specified XML definitions
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


//! Load types from the specified in-memory definitions
/*! \note The last entry in the array must be a terminating entry with Class == TypeNULL
 *  \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadTypes(const ConstTypeRecord *TypesData, SymbolSpacePtr DefaultSymbolSpace /*=MXFLibSymbols*/)
{
	// Pointer to walk through the array
	const ConstTypeRecord *CurrentType = TypesData;

	// Run-time list of types
	TypeRecordList Types;

	while(CurrentType->Class != TypeNULL)
	{
		if(CurrentType->Class == TypeSymbolSpace)
		{
			// A default symbol space has been specified - look it up
			DefaultSymbolSpace = SymbolSpace::FindSymbolSpace(CurrentType->SymSpace);

			// If it does not already exist, create it
			if(!DefaultSymbolSpace) DefaultSymbolSpace = new SymbolSpace(CurrentType->SymSpace);

			// Don't create a record for this entry
			CurrentType++;
			continue;
		}

		TypeRecordPtr ThisType = new TypeRecord;

		// Copy over the attributes
		ThisType->Class = CurrentType->Class;
		ThisType->Type = CurrentType->Type;
		ThisType->Detail = CurrentType->Detail;
		ThisType->Base = CurrentType->Base;
		if(CurrentType->Value) ThisType->Value = CurrentType->Value;
		ThisType->Size = CurrentType->Size;
		ThisType->Endian = CurrentType->Endian;
		ThisType->IsBatch = CurrentType->IsBatch;
		ThisType->RefType = CurrentType->RefType;
		if(CurrentType->RefTarget) ThisType->RefTarget = CurrentType->RefTarget;
		if(CurrentType->UL && *CurrentType->UL) ThisType->UL = StringToUL(CurrentType->UL);
		if(CurrentType->SymSpace) 
		{
			// A symbol space has been specified - look it up
			ThisType->SymSpace = SymbolSpace::FindSymbolSpace(CurrentType->SymSpace);

			// If it does not already exist, create it
			if(!ThisType->SymSpace) ThisType->SymSpace = new SymbolSpace(CurrentType->SymSpace);
		}

		// Add all children to compounds
		if((CurrentType->Class == TypeCompound) || (CurrentType->Class == TypeEnum))
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
				if(CurrentType->Value) SubType->Value = CurrentType->Value;
				if(CurrentType->UL && *CurrentType->UL) SubType->UL = StringToUL(CurrentType->UL);
				if(CurrentType->SymSpace) 
				{
					// A symbol space has been specified - look it up
					SubType->SymSpace = SymbolSpace::FindSymbolSpace(CurrentType->SymSpace);

					// If it does not already exist, create it
					if(!SubType->SymSpace) SubType->SymSpace = new SymbolSpace(CurrentType->SymSpace);
				}

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
		MXFLIB_TYPE_BASIC("Internal-UInt8", "Internally used 8 bit unsigned integer", "8f 64 35 9b fe 75 36 89 8c 4e 57 91 cd 68 6c e3", 1, false )
		MXFLIB_TYPE_MULTIPLE("UnknownType", "Array of bytes", "Internal-UInt8", "8f 64 35 9b fe 75 36 89 8c 4e 57 91 cd 68 6c e4", false, 0 )
	MXFLIB_TYPE_END
}


//! Load types from the specified in-memory definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadTypes(TypeRecordList &TypesData, SymbolSpacePtr DefaultSymbolSpace /*=MXFLibSymbols*/)
{
	// Define the basic "internally required" types (enough to hold an "unknown")
	static bool BasicDefined = false;
	if(!BasicDefined)
	{
		BasicDefined = true;
		LoadTypes(BasicInternalTypes);
	}

	// Define the known traits if required
	if(LoadBuiltInTraits) DefineTraits();

	//! List to hold any entries that are not resolved during this pass (we will recurse to resolve them at the end of the pass)
	TypeRecordList Unresolved;

	// Iterate through the list
	TypeRecordList::iterator it = TypesData.begin();
	while(it != TypesData.end())
	{
		// Set the symbol space
		if(!(*it)->SymSpace) (*it)->SymSpace = DefaultSymbolSpace;

		//! Set true if we generate a UUID for this type (rather than using a supplied one)
		bool GeneratedUUID = false;

		// Any types without a UL will be given a UUID
		if(!(*it)->UL)
		{
			UUIDPtr ID = new UUID;
			(*it)->UL = new UL(ID);
			GeneratedUUID = true;
		}

		switch((*it)->Class)
		{
			// Basic type definition
			case TypeBasic:
			{
				MDTypePtr Ptr = MDType::AddBasic((*it)->Type, (*it)->UL, (*it)->Size);
				if((*it)->Endian) Ptr->SetEndian(true);

				MDTraitsPtr Traits;
				if(!GeneratedUUID) Traits = MDType::LookupTraitsMapping((*it)->UL);
				if(!Traits) Traits = MDType::LookupTraitsMapping((*it)->Type, "Default-Basic");
				if(Traits) Ptr->SetTraits(Traits);

				// Add the name and UL to the symbol space
				(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);

				if((*it)->RefType != TypeRefUndefined) Ptr->SetRefType((*it)->RefType);
				if((*it)->RefTarget.length() != 0) Ptr->SetRefTarget((*it)->RefTarget);

				break;
			}

			// Interpretation type
			case TypeInterpretation:
			{
				// Search for the name first
				MDTypePtr BaseType = MDType::Find((*it)->Base);
				if(!BaseType)
				{
					// If the name is not found, we try looking for a UL
					ULPtr TypeUL = StringToUL((*it)->Base);

					if(TypeUL) BaseType = MDType::Find(TypeUL);

					if(!BaseType)
					{
						debug("Interpretation \"%s\" is based on (as yet) undefined base \"%s\"\n", (*it)->Type.c_str(), (*it)->Base.c_str());

						// Add to the "do later" pile
						Unresolved.push_back(*it);
					}
				}

				if(BaseType)
				{
					MDTypePtr Ptr = MDType::AddInterpretation((*it)->Type, BaseType, (*it)->UL, (*it)->Size);

					MDTraitsPtr Traits;
					if(!GeneratedUUID) Traits = MDType::LookupTraitsMapping((*it)->UL);
					if(!Traits) Traits = MDType::LookupTraitsMapping((*it)->Type);

					// If we don't have specific traits for this type
					// it will inherit the base type's traits
					if(Traits) Ptr->SetTraits(Traits);

					// Add the name and UL to the symbol space
					(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);

					if((*it)->RefType != TypeRefUndefined) Ptr->SetRefType((*it)->RefType);
					if((*it)->RefTarget.length() != 0) Ptr->SetRefTarget((*it)->RefTarget);
				}

				break;
			}

			// Multiple type
			case TypeMultiple:
			{
				// Search for the name first
				MDTypePtr BaseType = MDType::Find((*it)->Base);
				if(!BaseType)
				{
					// If the name is not found, we try looking for a UL
					ULPtr TypeUL = StringToUL((*it)->Base);

					if(TypeUL) BaseType = MDType::Find(TypeUL);

					if(!BaseType)
					{
						debug("Multiple \"%s\" is based on (as yet) undefined base \"%s\"\n", (*it)->Type.c_str(), (*it)->Base.c_str());

						// Add to the "do later" pile
						Unresolved.push_back(*it);
					}
				}

				if(BaseType)
				{
					MDTypePtr Ptr = MDType::AddArray((*it)->Type, BaseType, (*it)->UL, (*it)->Size);
					if((*it)->IsBatch) Ptr->SetArrayClass(ARRAYBATCH);

					MDTraitsPtr Traits;
					if(!GeneratedUUID) Traits = MDType::LookupTraitsMapping((*it)->UL);
					if(!Traits) Traits = MDType::LookupTraitsMapping((*it)->Type, "Default-Array");
					if(Traits) Ptr->SetTraits(Traits);

					// Add the name and UL to the symbol space
					(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);

					if((*it)->RefType != TypeRefUndefined) Ptr->SetRefType((*it)->RefType);
					if((*it)->RefTarget.length() != 0) Ptr->SetRefTarget((*it)->RefTarget);
				}

				break;
			}

			// Compound type
			case TypeCompound:
			{
				/* First check that we currently have all types required (and store them in a list for quick access later) */

				MDTypeList ChildList;

				TypeRecordList::iterator subit = (*it)->Children.begin();
				while(subit != (*it)->Children.end())
				{
					// Search for the name first
					MDTypePtr BaseType = MDType::Find((*subit)->Base);
					if(!BaseType)
					{
						// If the name is not found, we try looking for a UL
						ULPtr TypeUL = StringToUL((*subit)->Base);

						if(TypeUL) BaseType = MDType::Find(TypeUL);

						if(!BaseType)
						{
							debug("Compound item \"%s\" in \"%s\" is based on (as yet) undefined base \"%s\"\n", (*subit)->Type.c_str(), (*it)->Type.c_str(), (*subit)->Base.c_str());

							// Add to the "do later" pile
							Unresolved.push_back(*it);

							break;
						}
					}

					// Add this child type to the list
					ChildList.push_back(BaseType);

					subit++;
				}

				// If we quit the loop due to an unresolved item skip this type
				if(subit != (*it)->Children.end()) break;

				MDTypePtr Ptr = MDType::AddCompound((*it)->Type, (*it)->UL);

				MDTraitsPtr Traits;
				if(!GeneratedUUID) Traits = MDType::LookupTraitsMapping((*it)->UL);
				if(!Traits) Traits = MDType::LookupTraitsMapping((*it)->Type, "Default-Compound");
				if(Traits) Ptr->SetTraits(Traits);

				// Add the name and UL to the symbol space
				(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);


				/* Process sub-items */

				MDTypeList::iterator childit = ChildList.begin();
				subit = (*it)->Children.begin();
				while(subit != (*it)->Children.end())
				{
					// Add this child item
					Ptr->insert(MDType::value_type((*subit)->Type, (*childit)));

					// Add the child to the order list
					Ptr->ChildOrder.push_back((*subit)->Type);

					// Add the name and UL to the symbol space
					(*it)->SymSpace->AddSymbol((*it)->Type + "/" + (*subit)->Type, (*subit)->UL);

					childit++;
					subit++;
				}

				break;
			}

			// Enumeration type
			case TypeEnum:
			{
				// Search for the name first
				MDTypePtr BaseType = MDType::Find((*it)->Base);
				if(!BaseType)
				{
					// If the name is not found, we try looking for a UL
					ULPtr TypeUL = StringToUL((*it)->Base);

					if(TypeUL) BaseType = MDType::Find(TypeUL);

					if(!BaseType)
					{
						debug("Enumeration \"%s\" is based on (as yet) undefined base \"%s\"\n", (*it)->Type.c_str(), (*it)->Base.c_str());

						// Add to the "do later" pile
						Unresolved.push_back(*it);
					}
				}

				if(BaseType)
				{
					MDTypePtr Ptr = MDType::AddEnum((*it)->Type, BaseType, (*it)->UL);
					if(!Ptr)
					{
						error("Failed to build enumerated type %s\n", (*it)->Type.c_str());
						break;
					}

					MDTraitsPtr Traits;
					if(!GeneratedUUID) Traits = MDType::LookupTraitsMapping((*it)->UL);
					if(!Traits) Traits = MDType::LookupTraitsMapping((*it)->Type, "Default-Enum");
					if(Traits) Ptr->SetTraits(Traits);

					// Add the name and UL to the symbol space
					(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);


					/* Process values */

					TypeRecordList::iterator subit = (*it)->Children.begin();
					while(subit != (*it)->Children.end())
					{
						// Add this value
						Ptr->AddEnumValue((*subit)->Type, (*subit)->Value);

//						// Add the name and UL to the symbol space
//						(*it)->SymSpace->AddSymbol((*it)->Type + "/" + (*subit)->Type, (*subit)->UL);

						subit++;
					}
				}

				break;
			}

			// Label definition
			case TypeLabel:
			{
				UInt8 *Mask = NULL;
				UInt8 MaskData[16];

				// Check for a mask - kept in the value field an a hex string
				if((*it)->Value.length())
				{
					// We need to make a safe copy of the mask to parse
					char *MaskString = new char[(*it)->Value.length() + 1];
					strcpy(MaskString, (*it)->Value.c_str());

					// We read the hex mask, but if this fails we leave "Mask" as NULL so we don't attempt to use it
					if(ReadHexString(MaskString, 16, MaskData, " \t.") > 0) Mask = MaskData;

					delete[] MaskString;
				}

				if(!Label::Insert((*it)->Type, (*it)->Detail, (*it)->UL, Mask))
				{
					warning("Failed to add label %s - this probably means that it matches an existing label definition", (*it)->Type.c_str());
				}
				else
				{
					// Add the name and UL to the symbol space
					(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);
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
			error("Undefined base type or circular reference in types definitions (first unresolvable = %s)\n", Unresolved.front()->Type.c_str());

			it = Unresolved.begin();
			while(it != Unresolved.end())
			{
				debug("Unresolved Type: %s\n", (*it)->Type.c_str());
				it++;
			}

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
				ThisType->IsBatch = false;
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
						else if(strcmp(attr, "symSpace") == 0)
						{
							SymSpace = val;
						}
						else if(strcmp(attr, "ref") == 0)
						{
							if(strcasecmp(val,"strong") == 0) RefType = TypeRefStrong;
							else if(strcasecmp(val,"target") == 0) RefType = TypeRefTarget;
							else if(strcasecmp(val,"weak") == 0) RefType = TypeRefWeak;
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
				ThisType->IsBatch = false;
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
				ThisType->IsBatch = IsBatch;
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
				ThisType->IsBatch = false;

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
				ThisType->IsBatch = false;
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
				ThisType->IsBatch = false;

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
					ThisType->IsBatch = false;

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
				ThisType->IsBatch = false;

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


// Basic "internally required" class (enough to hold an "Unknown")
namespace mxflib
{
	MXFLIB_CLASS_START(BasicInternalClasses)
		MXFLIB_CLASS_ITEM("Unknown", "Unknown Set", ClassUsageOptional, "UnknownType", 0, 0, 0x0000, "e0 72 d2 17 ef 1e fb 6e 9b 9e e1 a8 83 b6 4b 3c", NULL, NULL)
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
		// All other entries are used to build classes
		MDOTypePtr ThisType = MDOType::DefineClass(*it, DefaultSymbolSpace);

		// If anything went wrong with this definition stack it for later
		if(!ThisType) Unresolved.push_back(*it);

		it++;
	}

	// Resolve any remaining entries
	int UnresolvedCount = (int)Unresolved.size();
	if(UnresolvedCount)
	{
		// Unless we were stuck this time (cannot resolve any more)
		if(UnresolvedCount == (int)ClassesData.size())
		{
			error("Undefined base class or circular reference in class definitions (first unresolvable = %s)\n", Unresolved.front()->Name.c_str());

			it = Unresolved.begin();
			while(it != Unresolved.end())
			{
				debug("Unresolved Class: %s\n", (*it)->Name.c_str());
				it++;
			}

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
	ClassRecordPtr LoadClassesSub(ConstClassRecordPTR &ClassData, SymbolSpacePtr DefaultSymbolSpace)
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
		int Count = ReadHexStringOrUL(ClassData->UL, 16, ULBuffer, " \t.");

		if(Count == 16) 
		{
			ThisClass->UL = new UL(ULBuffer);
		}

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

		if(ClassData->SymSpace)
		{
			// A symbol space has been specified - look it up
			ThisClass->SymSpace = SymbolSpace::FindSymbolSpace(ClassData->SymSpace);

			// If it does not already exist, create it
			if(!ThisClass->SymSpace) ThisClass->SymSpace = new SymbolSpace(ClassData->SymSpace);
		}
		else
		{
			ThisClass->SymSpace = DefaultSymbolSpace;
		}

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
				ClassRecordPtr Child = LoadClassesSub(ClassData, DefaultSymbolSpace);
				
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
		if(ClassData->Class == ClassSymbolSpace)
		{
			// A default symbol space has been specified - look it up
			DefaultSymbolSpace = SymbolSpace::FindSymbolSpace(ClassData->SymSpace);

			// If it does not already exist, create it
			if(!DefaultSymbolSpace) DefaultSymbolSpace = new SymbolSpace(ClassData->SymSpace);

			// Don't create a record for this entry
			ClassData++;
			continue;
		}

		// DRAGONS: ClassData is changed by LoadClassesSub
		ClassRecordPtr ThisClass = LoadClassesSub(ClassData, DefaultSymbolSpace);
		
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

	// The symbol space to use for this class
	SymbolSpacePtr ThisSymbolSpace;
	if(ThisClass->SymSpace) ThisSymbolSpace = ThisClass->SymSpace; else ThisSymbolSpace = DefaultSymbolSpace;

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

		// Build a UL from a new UUID (which will get a new value generated)
		UUID Temp;
		TypeUL = new UL(Temp);
	}

	// Work out the root name of this class (showing the list of parents)
	std::string RootName ;
	if(Parent) RootName = Parent->FullName() + "/";

	// Locate this type if it already exists (by UL if possible, else by name)
	MDOTypePtr Ret; 
	if(ValidUL && Parent) 
		Ret = Parent->Child(TypeUL);
	else
		Ret = MDOType::Find(RootName + ThisClass->Name, ThisSymbolSpace);

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
			MDTypePtr Type = MDType::Find(ThisClass->Base, ThisSymbolSpace);
			if(!Type)
			{
				error( "Item %s is of type %s which is not known\n", ThisClass->Name.c_str(), ThisClass->Base.c_str());
				return Ret;
			}

			if(Type->EffectiveClass() == TYPEARRAY)
			{
//#printf("** ARRAY = %s", ThisClass->Name.c_str());
				MDTypePtr SubType = Type->EffectiveBase();

				TypeRef RefType = Type->GetRefType();
//#printf(", RefType=%d", (int)RefType);
				if(RefType == TypeRefUndefined)
				{
					if(SubType) RefType = SubType->EffectiveRefType();
//#printf(", EffRefType=%d", (int)RefType);
				}
//#printf("\n");

				if((RefType != TypeRefNone) && (RefType != TypeRefUndefined) && SubType)
				{
//#printf("** REF ARRAY :");
					
					// Make a class description that is a copy of this item, but is an array of sub items
//					ClassRecordPtr ArrayClass = new ClassRecord;
//					*ArrayClass = *ThisClass;
//					ArrayClass->Class = Array or vector?
					Ret = new MDOType(BATCH, RootName, ThisClass->Name, ThisClass->Detail, NULL, DICT_KEY_NONE, DICT_LEN_NONE, 0, 0, ThisClass->Usage);
//#printf(" Added %s", (RootName + ThisClass->Name).c_str());

					// Make a sub that is a copy of this item, but is an array member
					ClassRecordPtr SubClass = new ClassRecord;
					*SubClass = *ThisClass;
					SubClass->Name += "_Item";
					SubClass->Base = SubType->Name();

					DefineClass(SubClass, ThisClass->SymSpace ? ThisClass->SymSpace : DefaultSymbolSpace, Ret);
//#printf(" Added %s of type %s\n", SubClass->Name.c_str(), SubClass->Base.c_str());

//					MDOTypePtr SubItem = new MDOType(NONE, RootName + ThisClass->Name + "/", ThisClass->Name + "_Item", ThisClass->Detail + "(sub-item)", SubType, DICT_KEY_NONE, DICT_LEN_NONE, -1, -1, ClassUsageRequired);
				}
			}

			// Only define if we have not just defined as an array
			if(!Ret) Ret = new MDOType(NONE, RootName, ThisClass->Name, ThisClass->Detail, Type, DICT_KEY_NONE, DICT_LEN_NONE, ThisClass->MinSize, ThisClass->MaxSize, ThisClass->Usage);
		}
		// Are we defining a derived class?
		else if(ThisClass->Base.size())
		{
			MDOTypePtr BaseType = MDOType::Find(ThisClass->Base, ThisSymbolSpace);

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

	// Work out which reference details to use - default to anything specified in this definition
	ClassRef RefType = ThisClass->RefType;
	std::string RefTarget = ThisClass->RefTarget;
//#printf("Processing class %s, RefType = %d, RefTarget=%s\n", Ret->Name().c_str(), (int)RefType, RefTarget.c_str());

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
			if(Parent->RefType != ClassRefNone)
			{
//#printf("Inheriting --> %s, Parent->RefType = %d, RefTarget=%s\n", Ret->Name().c_str(), (int)Parent->RefType, Parent->RefTargetName.c_str());

				RefType = Parent->RefType;
				RefTarget = Parent->RefTargetName;
				
				// DRAGONS: What if we have multiple subs?
				Parent->RefType = ClassRefNone;
			}

			// If we are not top level then record out "family tree"
			Ret->RootName = Parent->FullName() + "/";
		}
	}

	// If nothing specified this time then we use the details from the type
	if((!Extending) && Ret->ValueType)
	{
//#printf("Override...?\n");
		if(RefType == ClassRefUndefined) RefType = static_cast<ClassRef>(Ret->ValueType->EffectiveRefType());
		if(RefTarget.length() == 0) RefTarget = Ret->ValueType->EffectiveRefTarget();
//#printf("  RefType = %d, RefTarget=%s\n", (int)RefType, RefTarget.c_str());
	}

	// Sort referencing (overrides anything inherited)
	if(RefType != ClassRefUndefined)
//	if(RefType != ClassRefNone)
	{
		if(!Ret->empty())
		{
			MDOType::iterator it = Ret->begin();
			while(it != Ret->end())
			{
				if(((*it).second->minLength == 16) && ((*it).second->maxLength == 16))
				{
					(*it).second->RefType = RefType;
					(*it).second->RefTargetName = RefTarget;
//#printf("**SET** ");
				}
				it++;
			}
//#printf("\n");
		}
		else
		{
//#printf("- %s changed from %d to %d\n", Ret->Name().c_str(), (int)Ret->RefType, (int)RefType);
			Ret->RefType = RefType;
			Ret->RefTargetName = RefTarget;
//#printf("Set\n");		
		}
	}
//#	else
//#	{
//#printf("= Diff is - %s NOT changed from %d to %d\n", Ret->Name().c_str(), (int)Ret->RefType, (int)RefType);
//#	}

	// Set the local tag (if one exists)
	if(ThisClass->Tag)
	{
		Ret->Key.Resize(2);
		PutU16(ThisClass->Tag, Ret->Key.Data);
	}

//	// Determine the symbol space to use for this and any children - this is done irrespective of
//	// whether a UL exists for this item as there may be children that have a UL defined
//	SymbolSpacePtr ThisSymbolSpace;
//	if(ThisClass->SymSpace.size())
//	{
//		/* A symbol space has been specified - look it up */
//		ThisSymbolSpace = SymbolSpace::FindSymbolSpace(ThisClass->SymSpace);
//
//		// If it does not already exist, create it
//		if(!ThisSymbolSpace) ThisSymbolSpace = new SymbolSpace(ThisClass->SymSpace);
//	}
//	else 
//	{
//		ThisSymbolSpace = DefaultSymbolSpace;
//	}

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

		/* Add a version 1 UL for versionless compares */

		// Only add the version 1 lookup for SMPTE ULs (other labels may have other version rules)
		if((TypeUL->GetValue()[0] == 0x06) && (TypeUL->GetValue()[1] == 0x0e) && (TypeUL->GetValue()[2] == 0x2b) && (TypeUL->GetValue()[3] == 0x34))
		{
			// Make a version 1 copy of this UL
			ULPtr Ver1 = new UL(TypeUL);
			Ver1->Set(7,1);

			// Insert it into the version 1 lookup
			ULLookupVer1[*Ver1] = Ret;
		}
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
							|| (strcasecmp(val,"subFixedPack") == 0)
							|| (strcasecmp(val,"variablePack") == 0)
							|| (strcasecmp(val,"subVariablePack") == 0) )
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


namespace
{
	// Map of bit counts for each byte value
	static UInt8 BitCount[256] = 
	{
		0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
	};
}

//! Find a label with a given value, from a UL reference
/*! If more than one masked label matches, the value with the least mask bits is returned. 
 *  If more than one have the same number of mask bits, the last one found will be returned - which this is is undefined.
 */
LabelPtr Label::Find(const UL &LabelValue)
{
	// Search for an exact match
	LabelULMap::iterator it = LabelMap.find(LabelValue);

	// If we found an exact match, return it
	if(it != LabelMap.end()) return (*it).second;

	/* Now we have to do the long-hand search for masked values */

	// DRAGONS: This can be simplified by seeking to a point in the list before the first possible match - but this code is good enough
	
	int SmallestBitMask = 255;
	LabelPtr BestMatch;

	LabelULMultiMap::iterator mit = LabelMultiMap.begin();
	while(mit != LabelMultiMap.end())
	{
		// Only check those entries that have a mask (non-masked matches will have been found with the "find" call
		if((*mit).second->NonZeroMask)
		{
			// Set up pointers to this entry's mask and value
			const UInt8 *p1 = (*mit).first.GetValue();
			const UInt8 *pMask = (*mit).second->Mask;

			// Set up a pointer to the search value
			const UInt8 *p2 = LabelValue.GetValue();

			// Compare each byte
			int Count = 16;
			do
			{
				// Do a masked compare, first use XOR to select which bits don't match, then clear the masked ones
				// Any bytes that match the test will end up zero, otherwise the failing bit will be set
				if( ((*p1++) ^ (*p2++)) & ~(*pMask++) ) break;
			} while(--Count);

			// If we counted all the way to zero, we found a match - so count the mask bits
			if(!Count)
			{
				int BitMaskSize = 0;

				pMask = (*mit).second->Mask;
				Count = 16;
//while(Count--) { printf("%02x[%d].", *pMask, BitCount[*pMask]); BitMaskSize += BitCount[*pMask++]; };
				while(Count--) { BitMaskSize += BitCount[*pMask++]; };

//printf("Found a match with %d mask bits = %s\n", BitMaskSize, (*mit).second->GetName().c_str());
				if(BitMaskSize <= SmallestBitMask)
				{
//printf("Best so far\n");
					SmallestBitMask = BitMaskSize;
					BestMatch = (*mit).second;
				}
			}	
		}

		mit++;
	}

	// Return the best match - which may be NULL
	return BestMatch;
}



//! Map of all existing labels that don't use masking
Label::LabelULMap Label::LabelMap;

//! Map of all existing labels that use masking - this is a multimap to allow the same base with different masks
Label::LabelULMultiMap Label::LabelMultiMap;


//! Construct and add a label from a byte array
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const UInt8 *LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, LabelValue, LabelMask);

	if(LabelMask)
	{
//#printf("Adding %s to the masked map\n", Name.c_str());
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
//#printf("Adding %s to the unmasked map\n", Name.c_str());
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}


//! Construct and add a label from a UL smart pointer
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const ULPtr &LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, LabelValue->GetValue(), LabelMask);

	if(LabelMask)
	{
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}


//! Construct and add a label from a UL reference
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const UL &LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, LabelValue.GetValue(), LabelMask);

	if(LabelMask)
	{
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}


//! Construct and add a label from a UUID smart pointer
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const UUIDPtr &LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, *LabelValue, LabelMask);

	if(LabelMask)
	{
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}


//! Construct and add a label from a UUID reference
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const mxflib::UUID &LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, LabelValue, LabelMask);

	if(LabelMask)
	{
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}

