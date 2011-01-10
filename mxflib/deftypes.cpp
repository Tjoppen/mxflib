/*! \file	deftypes.cpp
 *	\brief	Dictionary processing
 *
 *	\version $Id: deftypes.cpp,v 1.26 2011/01/10 10:42:08 matt-beard Exp $
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


// DRAGONS: namespace mxflib_deftypes is semi-privat eto allow legacytypes.cpp to access items
namespace mxflib_deftypes
{
	//! Set when built-in traits need to be loaded
	bool LoadBuiltInTraits = true;
}

// Allow us to see our semi-private data
using namespace mxflib_deftypes;


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


// DRAGONS: namespace mxflib_deftypes is semi-privat eto allow legacytypes.cpp to access items
namespace mxflib_deftypes
{
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
		AddTraitsMapping(Int8_UL, new MDTraits_Int8);

		AddTraitsMapping("UInt8", new MDTraits_UInt8);
		AddTraitsMapping("Uint8", new MDTraits_UInt8);
		AddTraitsMapping(UInt8_UL, new MDTraits_UInt8);

		AddTraitsMapping("Internal-UInt8", new MDTraits_UInt8);
		AddTraitsMapping("Int16", new MDTraits_Int16);
		AddTraitsMapping(Int16_UL, new MDTraits_Int16);

		AddTraitsMapping("UInt16", new MDTraits_UInt16);
		AddTraitsMapping("Uint16", new MDTraits_UInt16);
		AddTraitsMapping(UInt16_UL, new MDTraits_UInt16);

		AddTraitsMapping("Int32", new MDTraits_Int32);
		AddTraitsMapping(Int32_UL, new MDTraits_Int32);

		AddTraitsMapping("UInt32", new MDTraits_UInt32);
		AddTraitsMapping("Uint32", new MDTraits_UInt32);
		AddTraitsMapping(UInt32_UL, new MDTraits_UInt32);

		AddTraitsMapping("Int64", new MDTraits_Int64);
		AddTraitsMapping(Int64_UL, new MDTraits_Int64);

		AddTraitsMapping("UInt64", new MDTraits_UInt64);
		AddTraitsMapping("Uint64", new MDTraits_UInt64);
		AddTraitsMapping(UInt64_UL, new MDTraits_UInt64);

		AddTraitsMapping("ISO7", new MDTraits_ISO7);
		AddTraitsMapping("UTF16", new MDTraits_UTF16);

		AddTraitsMapping("ISO7String", new MDTraits_BasicStringArray);
		AddTraitsMapping("UTF16String", new MDTraits_UTF16String);
		AddTraitsMapping(UTF16String_UL, new MDTraits_UTF16String);
		AddTraitsMapping("StringArray", new MDTraits_StringArray);

		// DRAGONS: At the moment we assume all unknown UTF is basically 7-bit text!
		AddTraitsMapping("UTF", new MDTraits_ISO7);
		AddTraitsMapping("UTFString", new MDTraits_BasicStringArray);

		AddTraitsMapping("UInt8Array", new MDTraits_RawArray);
		AddTraitsMapping("Uint8Array", new MDTraits_RawArray);

		AddTraitsMapping("Internal-UUID", new MDTraits_UUID);
		AddTraitsMapping("UUID", new MDTraits_UUID);

		AddTraitsMapping("UID", new MDTraits_Label);

		AddTraitsMapping("UL", new MDTraits_Label);

		AddTraitsMapping("UMID", new MDTraits_UMID);

		AddTraitsMapping("Rational", new MDTraits_Rational);
		AddTraitsMapping("Timestamp", new MDTraits_TimeStamp);
	}
}


//! Load types from the specified in-memory definitions
/*! \note The last entry in the array must be a terminating entry with Class == TypeNULL
 *  \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadTypes(const ConstTypeRecord *TypesData, SymbolSpacePtr DefaultSymbolSpace /*=MXFLibSymbols*/)
{
	//! Check if we have required internal items defined
	if(!MDOType::GetInternalsDefined()) MDOType::DefineInternals();

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
		ThisType->Endian = (CurrentType->Flags & TypeFlags_Endian) ? true : false;
		ThisType->IsBaseline = (CurrentType->Flags & TypeFlags_Baseline) ? true : false;
		ThisType->ArrayClass = CurrentType->ArrayClass;
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
				SubType->Endian = (CurrentType->Flags & TypeFlags_Endian) ? true : false;
				SubType->IsBaseline = (CurrentType->Flags & TypeFlags_Baseline) ? true : false;
				SubType->ArrayClass = CurrentType->ArrayClass;
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


// Basic "internally required" types (enough to hold an "unknown" and to make references work)
// DRAGONS UnknownType_UL is also defined in deftypes.h
namespace mxflib
{
	MXFLIB_TYPE_START(BasicInternalTypes)
		MXFLIB_TYPE_BASIC("Internal-UInt8", "Internally used 8 bit unsigned integer", "8f 64 35 9b fe 75 36 89 8c 4e 57 91 cd 68 6c e3", 1, false, false )
		MXFLIB_TYPE_MULTIPLE("Internal-UInt8Array", "Array of bytes", "Internal-UInt8", "8f 64 35 9b fe 75 36 89 8c 4e 57 91 cd 68 6c 31", ARRAYIMPLICIT, 0 )
		MXFLIB_TYPE_MULTIPLE("UnknownType", "Array of bytes", "Internal-UInt8", "8f 64 35 9b fe 75 36 89 8c 4e 57 91 cd 68 6c e4", ARRAYIMPLICIT, 0 )
		MXFLIB_TYPE_MULTIPLE("Internal-UUID", "Internally used 16-byte UUID", "Internal-UInt8", "8f 64 35 9b fe 75 36 89 8c 4e 57 91 cd 68 6c e5", ARRAYIMPLICIT, 16 )
		MXFLIB_TYPE_MULTIPLE("Internal-UUIDSet", "Internally used batch of UUIDs", "Internal-UUID", "8f 64 35 9b fe 75 36 89 8c 4e 57 91 cd 68 6c e6", ARRAYEXPLICIT, 0 )
		MXFLIB_TYPE_COMPOUND("Internal-Indirect", "Indirect Data", "8f 64 35 9b fe 75 36 89 8c 4e 57 91 cd 68 6c ee")
			MXFLIB_TYPE_COMPOUND_ITEM("Order", "Object Byte Order", "Internal-UInt8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Type", "Type of Data", "Internal-UUID", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Data", "Data", "Internal-UInt8Array", "", 0)
		MXFLIB_TYPE_COMPOUND_END
	MXFLIB_TYPE_END
}

// Basic "internally required" class (enough to hold an "Unknown")
// DRAGONS Unknown_UL is also defined in deftypes.h
// DRAGONS UnknownSet_UL is also defined in deftypes.h
namespace mxflib
{
	MXFLIB_CLASS_START(BasicInternalClasses)
		MXFLIB_CLASS_ITEM("Unknown", "Unknown Item", ClassUsageOptional, "UnknownType", 0, 0, 0x0000, "e0 72 d2 17 ef 1e fb 6e 9b 9e e1 a8 83 b6 4b 3c", NULL, NULL)
		MXFLIB_CLASS_SET("UnknownSet", "Unknown Set", "", "e0 72 d2 17 ef 1e fb 6e 9b 9e e1 a8 83 b6 4b 3d")
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END
}


//! Flag to show when we have loaded types and classes required for internal use
bool MDOType::InternalsDefined = false;

//! Load types and classes required for internal use 
void MDOType::DefineInternals(void)
{
	// DRAGONS: We set the "inited" flag first as otherwise the test in LoadTypes() will cause infinite recursion!
	InternalsDefined = true;

	LoadTypes(BasicInternalTypes);
	LoadClasses(BasicInternalClasses, MXFLibSymbols);

	// Define the known traits if required
	if(LoadBuiltInTraits) DefineTraits();
}


//! Load types from the specified in-memory definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadTypes(TypeRecordList &TypesData, SymbolSpacePtr DefaultSymbolSpace /*=MXFLibSymbols*/)
{
	//! Check if we have required internal items defined
	if(!MDOType::GetInternalsDefined()) MDOType::DefineInternals();

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

		// Start by locating any current type in case we are re-defining or extending
		MDTypePtr Ptr = MDType::Find((*it)->UL);
		bool Extending = Ptr ? true : false;

		if(Extending && (!(*it)->Type.empty()) && ((*it)->Type != Ptr->Name()))
		{
			warning("%s is assigned to type %s - the dictionary entry named %s has the same identifier so simply updates parts of the definition\n",
				    (*it)->UL->GetString().c_str(), Ptr->Name().c_str(), (*it)->Type.c_str());
		}

		switch((*it)->Class)
		{
			// Basic type definition
			case TypeBasic:
			{
				if(!Extending) Ptr = MDType::AddBasic((*it)->Type, (*it)->Detail, (*it)->UL, (*it)->Size);
				if((*it)->Endian) Ptr->SetEndian(true);
				Ptr->SetBaseline((*it)->IsBaseline);
				if((*it)->ArrayClass == ARRAYSTRING) Ptr->SetCharacter(true);

				if(!Extending)
				{
					MDTraitsPtr Traits;
					if(!GeneratedUUID) Traits = MDType::LookupTraitsMapping((*it)->UL);
					if(!Traits) Traits = MDType::LookupTraitsMapping((*it)->Type, "Default-Basic");
					if(Traits) Ptr->SetTraits(Traits);

					// Add the name and UL to the symbol space
					(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);
				}

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
					if(!Extending) Ptr = MDType::AddInterpretation((*it)->Type, (*it)->Detail, BaseType, (*it)->UL, (*it)->Size);
					if((*it)->ArrayClass == ARRAYSTRING) Ptr->SetCharacter(true);
					Ptr->SetBaseline((*it)->IsBaseline);

					if(!Extending)
					{
						MDTraitsPtr Traits;
						if(!GeneratedUUID) Traits = MDType::LookupTraitsMapping((*it)->UL);
						if(!Traits) Traits = MDType::LookupTraitsMapping((*it)->Type);

						// If we don't have specific traits for this type
						// it will inherit the base type's traits
						if(Traits) Ptr->SetTraits(Traits);

						// Add the name and UL to the symbol space
						(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);
					}

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
					if(!Extending) Ptr = MDType::AddArray((*it)->Type, (*it)->Detail, BaseType, (*it)->UL, (*it)->Size);
					if((*it)->ArrayClass != ARRAYIMPLICIT) Ptr->SetArrayClass((*it)->ArrayClass);
					Ptr->SetBaseline((*it)->IsBaseline);

					if(!Extending)
					{
						MDTraitsPtr Traits;
						if(!GeneratedUUID) Traits = MDType::LookupTraitsMapping((*it)->UL);
						if(!Traits) Traits = MDType::LookupTraitsMapping((*it)->Type, "Default-Array");
						if(Traits) Ptr->SetTraits(Traits);

						// Add the name and UL to the symbol space
						(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);
					}

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
					ULPtr NewUL = (*subit)->UL ? (*subit)->UL : RandomUL();
					MDTypePtr SubType = MDType::AddInterpretation((*subit)->Type, BaseType, NewUL, (*subit)->Size);
					ChildList.push_back(SubType);

					subit++;
				}

				// If we quit the loop due to an unresolved item skip this type
				if(subit != (*it)->Children.end()) break;

				if(!Extending) 
				{
					Ptr = MDType::AddCompound((*it)->Type, (*it)->Detail, (*it)->UL);

					MDTraitsPtr Traits;
					if(!GeneratedUUID) Traits = MDType::LookupTraitsMapping((*it)->UL);
					if(!Traits) Traits = MDType::LookupTraitsMapping((*it)->Type, "Default-Compound");
					if(Traits) Ptr->SetTraits(Traits);

					// Add the name and UL to the symbol space
					(*it)->SymSpace->AddSymbol((*it)->Type, (*it)->UL);
				}

				// Set the baseline flag whether extending or not
				Ptr->SetBaseline((*it)->IsBaseline);

				/* Process sub-items */

				MDTypeList::iterator childit = ChildList.begin();
				subit = (*it)->Children.begin();
				while(subit != (*it)->Children.end())
				{
					// Check if this is a duplicate
					bool Skip = false;
					MDType::iterator Child_it = Ptr->begin();
					while(Child_it != Ptr->end())
					{
						if(Extending)
						{
							if((*Child_it).first == (*subit)->Type)
							{
								debug("Skipping redefinition of sub-item %s of %s\n", (*subit)->Type.c_str(), (*it)->Type.c_str());
								Skip = true;
								break;
							}
							if(((*subit)->UL) && (*((*Child_it).second->GetTypeUL()) == *((*subit)->UL)))
							{
								debug("Skipping redefinition of sub-item %s of %s\n", (*subit)->UL->GetString().c_str(), (*it)->Type.c_str());
								Skip = true;
								break;
							}
						}
						else
						{
							if((*Child_it).first == (*subit)->Type)
							{
								error("Tried to define two sub-items called \"%s\" in type %s\n", (*subit)->Type.c_str(), (*it)->Type.c_str());
								Skip = true;
								break;
							}
							if(((*subit)->UL) && (*((*Child_it).second->GetTypeUL()) == *((*subit)->UL)))
							{
								error("Tried to define two sub-items with UL \"%s\" in type %s\n", (*subit)->UL->GetString().c_str(), (*it)->Type.c_str());
								Skip = true;
								break;
							}
						}
						Child_it++;
					}

					if(!Skip)
					{
						// Add this child item
						Ptr->insert((*subit)->Type, *childit);
					}

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
					if(!Extending) 
					{
						Ptr = MDType::AddEnum((*it)->Type, (*it)->Detail, BaseType, (*it)->UL);
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
					}

					// Set the baseline flag whether extending or not
					Ptr->SetBaseline((*it)->IsBaseline);
	
					/* Process values */

					TypeRecordList::iterator subit = (*it)->Children.begin();
					while(subit != (*it)->Children.end())
					{
						// Add this value
						Ptr->AddEnumValue((*subit)->Type, (*subit)->Value);
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
				mxflib_assert(0);
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
	//! Count all the class records in a list - including children
	size_t CountClassRecords(ClassRecordList &List)
	{
		// Count this level
		size_t Ret = List.size();

		// Add in any sub-levels by recursion
		ClassRecordList::iterator it = List.begin();
		while(it != List.end())
		{
			if((*it)->Children.size()) Ret += CountClassRecords((*it)->Children);
			it++;
		}

		return Ret;
	}
}


//! Load classes from the specified in-memory definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadClasses(ClassRecordList &ClassesData, SymbolSpacePtr DefaultSymbolSpace /*=MXFLibSymbols*/)
{
	//! List to hold any entries that are not resolved during this pass (we will recurse to resolve them at the end of the pass)
	ClassRecordList Unresolved;

	// Iterate through the list
	ClassRecordList::iterator it = ClassesData.begin();
	while(it != ClassesData.end())
	{
		// All other entries are used to build classes
		MDOTypePtr ThisType = MDOType::DefineClass(*it, DefaultSymbolSpace, &Unresolved);

		// If anything went wrong with this definition stack it for later
		if(!ThisType) Unresolved.push_back(*it);
		it++;
	}

	// Resolve any remaining entries
	size_t UnresolvedCount = Unresolved.size();
	if(UnresolvedCount)
	{
		// Unless we were stuck this time (cannot resolve any more)
		if(UnresolvedCount == ClassesData.size())
		{
			/* The list of unresolved classes may be different from the classes we were asked to build due to sets being built, 
			 * but with left-over children - so we need to check that by counting children */
			size_t FullCount = CountClassRecords(ClassesData);
			size_t UnresolvedFullCount = CountClassRecords(Unresolved);

			if(FullCount == UnresolvedFullCount)
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
	//! A const pointer to a ConstClassRecord
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

		// Decode flags
		ThisClass->ExtendSubs = (ClassData->Flags & ClassFlags_ExtendSubs) ? true : false;
		ThisClass->IsBaseline = (ClassData->Flags & ClassFlags_Baseline) ? true : false;

		// Add any children
		if((ClassData->Class == ClassSet) || (ClassData->Class == ClassPack) 
			|| (ClassData->Class == ClassVector) || (ClassData->Class == ClassArray) 
			|| (ClassData->Class == ClassExtend))
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



//! Load dictionary from the specified XML definitions
/*! \return 0 if all OK
 *  \return -1 on error
 */
int mxflib::LoadDictionary(const char *DictFile, SymbolSpacePtr DefaultSymbolSpace, std::string Application, bool FastFail /*=false*/)
{
	RXIDataPtr Dict = ParseRXIFile(DictFile, DefaultSymbolSpace, Application);
	if(!Dict) return -1;

	if(Dict->LegacyFormat) return LoadLegacyDictionary(DictFile, DefaultSymbolSpace, FastFail);

	// If any types were found they will be stored ready to build, so build them now
	if(!Dict->TypesList.empty()) LoadTypes(Dict->TypesList, DefaultSymbolSpace);

	// If any classes were found they will be stored ready to build, so build them now
	if(!Dict->GroupList.empty()) LoadClasses(Dict->GroupList, DefaultSymbolSpace);

	// If any orphaned elements were found they will be stored ready to build, so build them now
	if(!Dict->ElementList.empty()) LoadClasses(Dict->ElementList, DefaultSymbolSpace);

	// If we loaded any classes, build a static primer (for use in index tables)
	if((!Dict->GroupList.empty()) || (!Dict->ElementList.empty()))
	{
		MDOType::MakePrimer(true);
	}

	// Locate reference target types for any new types
	MDOType::LocateRefTypes();

	return 0;
}


//! Load dictionary from an XML string
int mxflib::LoadDictionaryFromXML(std::string &strXML, std::string Application, bool /*=false*/)
{
	RXIDataPtr Dict = ParseRXIData(strXML, MXFLibSymbols, Application);
	if(!Dict) return -1;

	if(Dict->LegacyFormat) return LoadLegacyDictionary(strXML, MXFLibSymbols);

	// If any types were found they will be stored ready to build, so build them now
	if(!Dict->TypesList.empty()) LoadTypes(Dict->TypesList, MXFLibSymbols);

	// If any classes were found they will be stored ready to build, so build them now
	if(!Dict->GroupList.empty()) LoadClasses(Dict->GroupList, MXFLibSymbols);

	// If any orphaned elements were found they will be stored ready to build, so build them now
	if(!Dict->ElementList.empty()) LoadClasses(Dict->ElementList, MXFLibSymbols);

	// If we loaded any classes, build a static primer (for use in index tables)
	if((!Dict->GroupList.empty()) || (!Dict->ElementList.empty()))
	{
		MDOType::MakePrimer(true);
	}

	// Locate reference target types for any new types
	MDOType::LocateRefTypes();

	return 0;
}


