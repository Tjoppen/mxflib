/*! \file	mdobject.cpp
 *	\brief	Implementation of classes that define metadata objects
 *
 *			Class MDObject holds info about a specific metadata object
 *<br><br>
 *			Class MDOType holds the definition of MDObjects derived from
 *			the XML dictionary.
 *<br><br>
 *	\note
 *			These classes are currently wrappers around KLVLib structures
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

#include "mxflib.h"

using namespace mxflib;


//! MDDict constructor
/*! Loads the dictionary from the specified file
*/
void MDOType::DictManager::Load(const char *DictFile)
{
	// Build an entry for all unknown types
	// Note that we malloc it because KLVLib will "free" it later
	DictEntry *Unknown = (DictEntry*)malloc(sizeof(DictEntry));

	ASSERT(Unknown);
	if(!Unknown)
	{
		error("Out of memory\n");
		return;
	}
	
	InitialiseDictEntry(Unknown);

	Unknown->Name = (char *)malloc(8);
	strcpy(Unknown->Name, "Unknown");


	// Load the KLVLib dictionary
	MainDict = LoadXMLDictionary(DictFile);

	if(MainDict == NULL)
	{
		error("Couldn't open dictionary file \"%s\"\n", DictFile);

		// Note: We don't bug out here, we process the single "Unknown" type
		MainDict = Unknown;
	}
	else
	{
		// Add an "Unknown" entry at the end
		DictEntry *Dict = MainDict;
		for(;;)
		{
			if(Dict->Next == NULL)
			{
				Dict->Next = Unknown;
				break;
			}
			Dict = Dict->Next;
		}
	}

	// Build all MDOTypes from the KLVLib dictionary
	DictEntry *Dict = MainDict;
	while(Dict != NULL)
	{
		DictEntry *p = Dict->Parent;
		while(p)
		{
			debug("*");
			p = p->Parent;
		};

		debug("DictEntry: %s\n", Dict->Name);

		// Add any top level types (and their children)
		if(Dict->Parent == NULL)
		{
			MDOType::AddDict(Dict);
		}

		// Continue looping
		Dict = Dict->Next;
	}

	// DRAGONS: Clumbsy code to sort out base types
	{
		MDOTypeList::iterator it = AllTypes.begin();

		while(it != AllTypes.end())
		{
			DictEntry *Base = (*it)->GetDict()->Base;

			if(Base != NULL)
			{
				if( DictLookup.find(Base) == DictLookup.end() )
				{
					error("Missing base type for MDOType \"%s\"\n", (*it)->GetDict()->Name);
				}
				else
				{
					(*it)->Base = DictLookup[Base];
				}
			}
			it++;
		}
	}

	// DRAGONS: Some debug!
	MDOTypeList::iterator it = TopTypes.begin();
	while(it != TopTypes.end())
	{
		debug("MDOType: %s\n", (*it)->GetDict()->Name);

		MDOTypeList::iterator it2 = (*it)->Children.begin();
		StringList::iterator it2n = (*it)->ChildrenNames.begin();
		while(it2 != (*it)->Children.end())
		{
			debug("  Sub->: %s = %s\n", (*it2n).c_str() ,(*it2)->GetDict()->Name);

			MDOTypeList::iterator it3 = (*it2)->Children.begin();
			while(it3 != (*it2)->Children.end())
			{
				debug("    SubSub->: %s\n", (*it3)->GetDict()->Name);
				it3++;
			}

			it2++;
			it2n++;
		}

		it++;
	}
}


//! Build a Primer object for the current dictionary
/*! This primer has the mappings of tag to UL from the dictionary
 */
PrimerPtr MDOType::DictManager::MakePrimer(void)
{
	PrimerPtr Ret = new Primer;

	// DRAGONS: Some debug!
	MDOTypeList::iterator it = AllTypes.begin();
	while(it != AllTypes.end())
	{
		const DictEntry *Dict = (*it)->GetDict();

		if(Dict->KeyLen == 2)
		{
			Tag ThisTag = Dict->Key[1] + (Dict->Key[0] << 8);

			// Don't barf if the dictionary entry is invalid!
			if(Dict->GlobalKeyLen != 16)
			{
				error("Dictionary entry for \"%s\" has a 2-byte tag, but doesn't have a 16-byte UL\n", Dict->Name);
			}
			else
			{
				mxflib::UL ThisUL(Dict->GlobalKey);
				Ret->insert(Primer::value_type(ThisTag, ThisUL));
			}
		}

		it++;
	}

	return Ret;
}



//! Convert KLVLib "DictType" enum to text string of type name
/*! /ret Pointer to a string constant
 *  /ret "" if the DictType is not known or is a container (e.g. a pack)
 */
char *DictType2Text(DictType Type)
{
	typedef std::map<DictType, char *> XLateType;
	static XLateType XLate;

	if(XLate.empty())
	{
		XLate.insert(XLateType::value_type(DICT_TYPE_NONE,"Unknown"));
		XLate.insert(XLateType::value_type(DICT_TYPE_U8,"Uint8"));
		XLate.insert(XLateType::value_type(DICT_TYPE_I8,"Int8"));
		XLate.insert(XLateType::value_type(DICT_TYPE_U16,"Uint16"));
		XLate.insert(XLateType::value_type(DICT_TYPE_I16,"Int16"));
		XLate.insert(XLateType::value_type(DICT_TYPE_U32,"Uint32"));
		XLate.insert(XLateType::value_type(DICT_TYPE_I32,"Int32"));
		XLate.insert(XLateType::value_type(DICT_TYPE_U64,"Uint64"));
		XLate.insert(XLateType::value_type(DICT_TYPE_I64,"Int64"));
		XLate.insert(XLateType::value_type(DICT_TYPE_ISO7,"ISO7"));
		XLate.insert(XLateType::value_type(DICT_TYPE_UTF8,"UTF8"));
		XLate.insert(XLateType::value_type(DICT_TYPE_UTF16,"UTF16"));
		XLate.insert(XLateType::value_type(DICT_TYPE_UUID,"UUID"));
		XLate.insert(XLateType::value_type(DICT_TYPE_UMID,"UMID"));
		XLate.insert(XLateType::value_type(DICT_TYPE_LABEL,"Label"));
		XLate.insert(XLateType::value_type(DICT_TYPE_TIMESTAMP,"TimeStamp"));
		XLate.insert(XLateType::value_type(DICT_TYPE_VERTYPE,"VersionType"));
		XLate.insert(XLateType::value_type(DICT_TYPE_RATIONAL,"Rational"));
		XLate.insert(XLateType::value_type(DICT_TYPE_BOOLEAN,"Boolean"));
		XLate.insert(XLateType::value_type(DICT_TYPE_ISO7STRING,"ISO7String"));
		XLate.insert(XLateType::value_type(DICT_TYPE_UTF16STRING,"UTF16String"));
		XLate.insert(XLateType::value_type(DICT_TYPE_IEEEFLOAT64,"Float64"));
		XLate.insert(XLateType::value_type(DICT_TYPE_UINT8STRING,"Uint8Array")); // DRAGONS: Is this right?
		XLate.insert(XLateType::value_type(DICT_TYPE_PRODUCTVERSION,"ProductVersion"));
		XLate.insert(XLateType::value_type(DICT_TYPE_RAW,"Uint8Array"));
	}

	XLateType::iterator it = XLate.find(Type);
	
	if(it == XLate.end()) return "";

	return (*it).second;
}


//! Add a KLVLib DictEntry definition to the managed types
void MDOType::AddDict(DictEntry *Dict, MDOTypePtr ParentType /* = NULL */ )
{
	// Create a new MDOType to manage
	MDOTypePtr NewType = new MDOType(Dict);

	// Add to the list of all types
	AllTypes.push_back(NewType);

	// If it is a top level type then add it to TopTypes as well
	if(Dict->Parent == NULL) TopTypes.push_back(NewType);

	// Record the parent
	NewType->Parent = ParentType;

	// If it is a child of another type then add to the children lists
	if(ParentType)
	{
		ParentType->Children.push_back(NewType);
		ParentType->ChildrenNames.push_back(Dict->Name);
	}

	// Build base name for any children
	MDOTypePtr Scan = ParentType;
	std::string BaseName = "";
	while(Scan)
	{
		BaseName = std::string(Scan->Dict->Name) + "/" + BaseName;
		Scan = Scan->Parent;
	}

	// Copy any children from our base
	if(Dict->Base)
	{
		MDOTypePtr Base = DictLookup[Dict->Base];
		NewType->Children = Base->Children;
		NewType->ChildrenNames = Base->ChildrenNames;

		// Add child names to name lookup
		MDOTypeList::iterator it = NewType->Children.begin();
		StringList::iterator itn = NewType->ChildrenNames.begin();
		while(it != NewType->Children.end())
		{
			NameLookup[BaseName + (*itn)] = (*it);
			it++;
			itn++;
		}
	}

	// Get name of this value type
	// First we do a quick trick to make vectors work as MDValue array types
	std::string Type;
	if( (Dict->Type == DICT_TYPE_VECTOR) || (Dict->Type == DICT_TYPE_ARRAY) )
	{
		// First see if there is only one item in the vector
		if(Dict->Children && (Dict->Children->Next == NULL))
		{
//Type = "";
			// If it is a ref this is more important than the UUID type
			if(Dict->RefType == DICT_REF_STRONG)
				Type = "StrongRef";
			else if(Dict->RefType == DICT_REF_WEAK)
				Type = "WeakRef";
			else
				Type = DictType2Text(Dict->Children->Link->Type);

			if(Type != "")
			{
				if(Dict->Type == DICT_TYPE_VECTOR) Type += "Batch";
				else Type += "Array";
			}

			AddDict(Dict->Children->Link, NewType);
		}
		else
		{
//			// If more complex type use the vector properties name as the type
//			Type = Dict->Name;

			Type = "";

			DictEntryList *ChildList = Dict->Children;
			while(ChildList != NULL)
			{
				// Rinse and repeat!
				AddDict(ChildList->Link, NewType);

				// Iterate through the list
				ChildList = ChildList->Next;
			}
		}
	}
	else
	{
		// Not a vector or array, look up the type
		Type = DictType2Text(Dict->Type);
	}

	if(Type != "")
	{
		NewType->ValueType = MDType::Find(Type.c_str());
		if(!NewType->ValueType)
		{
			std::string Temp = BaseName;
			Temp += Dict->Name;
			warning("Object type \"%s\" is of unknown type \"%s\"\n", Temp.c_str(), Type.c_str());

			NewType->ValueType = MDType::Find("Unknown");

			ASSERT(NewType->ValueType);
		}
	}
	else
	{
		// Add any children of our own
		// Note that this is only done if the type is not a known
		// MDType because this allows vectors to be handled as
		// MDValue objects rather than containers
		DictEntryList *ChildList = Dict->Children;
		while(ChildList != NULL)
		{
			// Rinse and repeat!
			AddDict(ChildList->Link, NewType);

			// Iterate through the list
			ChildList = ChildList->Next;
		}
	}

	if(Dict->GlobalKeyLen == 16)
	{
		NewType->TypeUL = new UL(Dict->GlobalKey);
	}

	// Set the lookups
	if(NewType->TypeUL) ULLookup[UL(NewType->TypeUL)] = NewType;
	DictLookup[Dict] = NewType;
	NameLookup[BaseName + std::string(Dict->Name)] = NewType;
}


//! MDOType destructor
/*! Frees all items loaded and generated by this object
*/
MDOType::DictManager::~DictManager()
{
	// Free the main dictionary (as long as it has been loaded)
	if (MainDict) FreeDictionary(MainDict);
}



//! Builds an MDOType
/*! This constructor is private so the ONLY way to create
 *	new MDOTypes from outside this class is via AddDict()
*/
MDOType::MDOType(DictEntry *RootDict) : Dict(RootDict), Parent(NULL)
{
	// Can't build an MDOType based on nothing
	ASSERT( RootDict != NULL );

	// Assume we have the same type as KLVLib uses
	RefType = RootDict->RefType;

	//! Determine the container type
	if( (RootDict->Type == DICT_TYPE_UNIVERSAL_SET) 
	  ||(RootDict->Type == DICT_TYPE_LOCAL_SET) )
	{
		ContainerType = SET;
	}
	else if( (RootDict->Type == DICT_TYPE_FIXED_PACK) 
		   ||(RootDict->Type == DICT_TYPE_VARIABLE_PACK) )
	{
		ContainerType = PACK;
	}
	else if(RootDict->Type == DICT_TYPE_VECTOR)
	{
		ContainerType = BATCH;

		// Children will have the ref property (different to KLVLib)
		RefType = DICT_REF_NONE;
	}
	else if(RootDict->Type == DICT_TYPE_ARRAY)
	{
		ContainerType = ARRAY;

		// Children will have the ref property (different to KLVLib)
		RefType = DICT_REF_NONE;
	}
	else
	{
		ContainerType = NONE;

		if(RootDict->Parent)
		{
//printf("%s is a child of %s\n",RootDict->Name, RootDict->Parent->Name);
//if(RootDict->Parent->RefType != DICT_REF_NONE) printf("Parent Ref Type = %d\n", RootDict->Parent->RefType );
			// Inherit ref type from parent if it has one
			if(RootDict->Parent->RefType != DICT_REF_NONE) RefType = RootDict->Parent->RefType;
		}
	}
};


//! Find the MDOType object that defines a named type
/*! /ret Pointer to the object
 *  /ret NULL if there is no type of that name
 */
MDOTypePtr MDOType::Find(const char *BaseType)
{
	MDOTypePtr theType;

	std::map<std::string,MDOTypePtr>::iterator it = NameLookup.find(std::string(BaseType));
	if(it == NameLookup.end())
	{
		return NULL;
	}

	theType = (*it).second;

	return theType;
}


//! Find the MDOType object that defines a type with a specified UL
/*! /ret Pointer to the object
 *  /ret NULL if there is no type with that UL
 */
MDOTypePtr MDOType::Find(ULPtr BaseUL)
{
	MDOTypePtr theType;

	std::map<UL, MDOTypePtr>::iterator it = ULLookup.find(UL(BaseUL));

	if(it != ULLookup.end())
	{
		theType = (*it).second;
	}

	return theType;
};


//! Find the MDOType object that defines a type with a specified Tag
/*! The tag is looked up in the supplied primer
 *  /ret Pointer to the object
 *  /ret NULL if there is no type with that UL
 */
MDOTypePtr MDOType::Find(Tag BaseTag, PrimerPtr BasePrimer)
{
	MDOTypePtr theType;

	Primer::iterator it = BasePrimer->find(BaseTag);

	if(it != BasePrimer->end())
	{
		UL BaseUL = (*it).second;
		std::map<UL, MDOTypePtr>::iterator it2 = ULLookup.find(UL(BaseUL));

		if(it2 != ULLookup.end())
		{
			theType = (*it2).second;
		}
	}

	return theType;
};



//! MDObject named constructor
/*! Builds a "blank" metadata object of a named type
*/
MDObject::MDObject(const char *BaseType)
{
	Type = MDOType::Find(BaseType);
	if(!Type)
	{
		error("Metadata object type \"%s\" doesn't exist\n", BaseType);

		Type = MDOType::Find("Unknown");

		ASSERT(Type);
	}

	// Initialise the new object
	Init();
}


//! MDObject typed constructor
/*! Builds a "blank" metadata object of a specified type
*/
MDObject::MDObject(MDOTypePtr BaseType) : Type(BaseType) 
{
	// Initialise the new object
	Init();
};


//! MDObject typed constructor
/*! Builds a "blank" metadata object of a specified type
*/
MDObject::MDObject(ULPtr UL) 
{
	Type = MDOType::Find(UL);
	if(!Type)
	{
		/// Note - this is not an error - it could be dark metadata!
		///		error("Metadata object with UL \"%s\" doesn't exist\n", UL->GetString().c_str());

		Type = MDOType::Find("Unknown");

		ASSERT(Type);
	}

	// Initialise the new object
	Init();
};


//! MDObject typed constructor
/*! Builds a "blank" metadata object of a specified type
*/
MDObject::MDObject(Tag BaseTag, PrimerPtr BasePrimer)
{
	Type = MDOType::Find(BaseTag, BasePrimer);
	if(!Type)
	{
		error("Metadata object with Tag \"%s\" doesn't exist in specified Primer\n", Tag2String(BaseTag).c_str());

		Type = MDOType::Find("Unknown");

		ASSERT(Type);
	}

	// Initialise the new object
	Init();
};


//! Second part of MDObject constructors
/*! Builds a "blank" metadata object
*/
void MDObject::Init(void)
{
	switch(Type->GetContainerType())
	{
	case NONE:
		// If it isn't a container build the basic item
		{
			Value = new MDValue(Type->ValueType);

			if(Type->ValueType->EffectiveClass() == ARRAY)
			{
				// Build the minimum size array
				Value->Resize(Type->GetDict()->minLength);
			}
		}
		break;

	case PACK:
		// If it's a pack build all items
		{
			Value = NULL;

			MDOTypeList::iterator it = Type->Children.begin();
			while(it != Type->Children.end())
			{
				Children.push_back(new MDObject(*it));
				it++;
			}

			// Copy all the names
			ChildrenNames = Type->ChildrenNames;

		}
		break;

	case BATCH:
	case ARRAY:
	default:
		{
			Value = NULL;
		}
	}

	// If it's a fixed size array build all items
//	else if(Type->GetContainerType() == ARRAY)
//	{
// DRAGONS: Arg! Problems!!!
//		if(Type->Size > 0)
//		{
//			// Add a blank last item - forces all to be created
//			AddChild(new MDObject(Type), Type->Size - 1);
//		}
//	}

};


//! Add an empty named child to an MDObject continer and return a pointer to it
/*! If a child of this name already exists a pointer to that child is returned
 *  but the value is not changed.
 *  /ret NULL if it is not a valid child to add to this type of container
 *	/note If you want to add an child that is non-standard (i.e. not listed
 *        as a child in the container's MDOType then you must build the child
 *        then add it with AddChild(MDObjectPtr)
 */
MDObjectPtr MDObject::AddChild(const char *ChildName)
{
	// Try and find an existing child
	MDObjectPtr Ret = Child(ChildName);

	// Only add a new one if we didn't find it
	if(!Ret)
	{
		StringList::iterator it = Type->ChildrenNames.begin();
		MDOTypeList::iterator it2 = Type->Children.begin();
	
		while(it != Type->ChildrenNames.end())
		{
			ASSERT(it2 != Type->Children.end());

			if(strcmp((*it).c_str(),ChildName) == 0)
			{
				// Insert a new item of the correct type at the end
				MDObjectPtr Ret = new MDObject(*it2);
				Children.push_back(Ret);
				ChildrenNames.push_back(*it);

				// Return smart pointer to the new object
				return Ret;
			}
			it++;
			it2++;
		}
	}

	return Ret;
};


//! Add a given MDObject to an MDObject continer
/*! /ret pointer to the object added (for compatibility with other ADDChild funcs)
 *  /ret NULL if there was an error
 *  /note If there is already a child of this type it is removed
 */
MDObjectPtr MDObject::AddChild(MDObjectPtr ChildObject)
{
	// Remove any existing child
	RemoveChild(ChildObject->Type);

	// Insert the new item at the end
	Children.push_back(ChildObject);
	ChildrenNames.push_back(ChildObject->Type->GetDict()->Name);

	return ChildObject;
}


//! Remove any children with a specified name from an MDObject continer
void MDObject::RemoveChild(const char *ChildName)
{
	StringList::iterator it = ChildrenNames.begin();
	MDObjectList::iterator it2 = Children.begin();

	while(it != ChildrenNames.end())
	{
		ASSERT(it2 != Children.end());

		if(strcmp((*it).c_str(),ChildName) == 0)
		{
			ChildrenNames.erase(it);
			Children.erase(it2);

			// Restart the scan - DRAGONS: Not very efficient
			it = ChildrenNames.begin();
			it2 = Children.begin();
		}
		else
		{
			it++;
			it2++;
		}
	}
}


//! Remove any children of a specified type from an MDObject continer
void MDObject::RemoveChild(MDOTypePtr ChildType)
{
	StringList::iterator it = ChildrenNames.begin();
	MDObjectList::iterator it2 = Children.begin();

	while(it2 != Children.end())
	{
		ASSERT(it != ChildrenNames.end());

		if((*it2)->Type == ChildType)
		{
			ChildrenNames.erase(it);
			Children.erase(it2);

			// Restart the scan - DRAGONS: Not very efficient
			it = ChildrenNames.begin();
			it2 = Children.begin();
		}
		else
		{
			it++;
			it2++;
		}
	}
}


//! Remove a specified object from an MDObject continers children lists
/*! If the object is not an child of the container nothing is done
 */
void MDObject::RemoveChild(MDObjectPtr ChildObject)
{
	StringList::iterator it = ChildrenNames.begin();
	MDObjectList::iterator it2 = Children.begin();

	while(it2 != Children.end())
	{
		ASSERT(it != ChildrenNames.end());

		if((*it2) == ChildObject)
		{
			ChildrenNames.erase(it);
			Children.erase(it2);
			return;
		}
		else
		{
			it++;
			it2++;
		}
	}
}


//#//! Remove children from an MDObject continer
//#/*! Remove all but the first "Index" children. 
//# *! Probably only useful for resizing arrays.
//# */
//#void MDObject::TrimChildren(int Index)
//#{
//#	ASSERT( Type->GetContainerType() != NONE );
//#	
//#	MDObjectList::iterator it = Children.begin();
//#
//#	// Move to the index point
//#	while(Index--) it++;
//#
//#	// Remove the old entries, automatically deleting the objects if required
//#	Children.erase(it, Children.end());
//#}


//! MDObject destructor
/*! Frees the data of the object if it exists
*/
MDObject::~MDObject()
{
	// Free any memory used
}


//#//!	Set the value of a metadata object from a "variable sized" chunk
//#/*! "variable sized" simply means that it has a size and a pointer
//# *	as opposed to the fixed size integer data types
//# *
//# *	DRAGONS: If a data item is "shrunk" then grows again it may be
//# *           re-allocated when this is not required...
//# */
//#void MDObject::SetData(int ValSize, Uint8 *Val)
//#{
//#	// Can only set the value of an individual item
//#	ASSERT(Type->GetContainerType() == NONE);
//#
//#	// Ignore containers in release mode
//#	if(Type->GetContainerType() != NONE) return;
//#
//#	// Make sure we don't make the item bigger than allowed
//#	int MaxSize = Type->GetDict()->maxLength;
//#	
//#	// Enforce the size limit
//#	if(ValSize > MaxSize) ValSize = MaxSize;
//#
//#	// Reallocate the data if it won't fit
//#	if(Size < ValSize)
//#	{
//#		if(Size) delete[] Data;
//#		Data = new Uint8[ValSize];
//#	}
//#
//#	// Set the new data
//#	Size = ValSize;
//#	memcpy(Data, Val, ValSize);
//#}


//#//! Access array member within an MDObject array
//#/*! DRAGONS: This doesn't work well with SmartPtrs
//# *           so member function Child() is also available
//#*/
//#MDObjectPtr MDObject::operator[](int Index)
//#{
//#	MDObjectList::iterator it = Children.begin();
//#
//#	while(Index--)
//#	{
//#		// End of list!
//#		if(it == Children.end()) return NULL;
//#	}
//#
//#	// Return a smart pointer to the object
//#	return *(it);
//#}

//! Access named sub-item within a compound MDObject
/*! If the child does not exist in this item then NULL is returned
 *  even if it is a valid child to have in this type of container
 *
 *  DRAGONS: This doesn't work well with SmartPtrs
 *           so member function Child() is also available
*/
MDObjectPtr MDObject::operator[](const char *ChildName)
{
	StringList::iterator it = ChildrenNames.begin();
	MDObjectList::iterator it2 = Children.begin();

	while(it != ChildrenNames.end())
	{
		ASSERT(it2 != Children.end());

		if(strcmp((*it).c_str(),ChildName) == 0)
		{
			// Return a smart pointer to the object
			MDObjectPtr Ret = *(it2);
			return Ret;
		}
		it++;
		it2++;
	}

	return NULL;
}


//! Access sub-item of the specified type within a compound MDObject
/*! If the child does not exist in this item then NULL is returned
 *  even if it is a valid child to have in this type of container
 *
 *  DRAGONS: This doesn't work well with SmartPtrs
 *           so member function Child() is also available
*/
MDObjectPtr MDObject::operator[](MDOTypePtr ChildType)
{
	MDObjectList::iterator it = Children.begin();

	while(it != Children.end())
	{
		if((*it)->Type == ChildType)
		{
			// Return a smart pointer to the object
			MDObjectPtr Ret = *(it);
			return Ret;
		}
		it++;
	}

	return NULL;
}


//! Read value from a buffer
/*! Note that collection headers are handled here rather than in the MDValue
 *  because MDValue objects don't differentiate. A primer must be supplied for reading sets
 *
 *  DRAGONS: This function is bloated and should be split up
 *
 *  /ret Number of bytes read
 */
Uint32 MDObject::ReadValue(const Uint8 *Buffer, Uint32 Size, PrimerPtr UsePrimer /*=NULL*/)
{
	Uint32 Bytes = 0;

	switch(Type->GetContainerType())
	{
	case NONE:
		return Value->ReadValue(Buffer, Size);

	case BATCH:
		{
			ASSERT(Size >= 8);

			Uint32 Count = GetU32(Buffer);
			Buffer += 4;

			Uint32 ItemSize = GetU32(Buffer);
			Buffer += 4;

			if(Size <= 8) Size = 0; else Size -= 8;
			if((ItemSize*Count) != Size)
			{
				warning("Malformed batch found, item size = %u, count = %u, but bytes = %u\n",
						ItemSize, Count, (Size));

				// Prevent us reading off the end of the buffer
				if(Size < (ItemSize*Count))	Count = Size / ItemSize;
			}

//			return 8 + Value->ReadValue(Buffer, ItemSize, Count);

			Bytes = 8;
			Size = ItemSize * Count;
		}
		// Fall through and process as an array

	case ARRAY:
		{
			if(Type->Children.empty())
			{
				error("Object %s is a multiple, but has no contained types\n", Name().c_str());
				return Bytes;
			}

			// Start with no children
			Children.clear();

			MDOTypeList::iterator it = Type->Children.begin();
			while(Size)
			{
				MDObjectPtr NewItem = new MDObject(*(it));
				Uint32 ThisBytes = NewItem->ReadValue(Buffer, Size);
				Bytes += ThisBytes;
				Buffer += ThisBytes;
				if(ThisBytes > Size) Size = 0; else Size -= ThisBytes;
				Children.push_back(NewItem);

				it++;
				if(it == Type->Children.end()) it = Type->Children.begin();
//printf("Child of %s is %s\n", Name().c_str(), NewItem->Name().c_str());
			}

			if(it != Type->Children.begin())
			{
				error("Multiple %s does not contain an integer number of sub-items\n", Name().c_str());
			}

			return Bytes;
		}

	case PACK:
		{
			Uint32 Bytes = 0;
			MDObjectList::iterator it = Children.begin();
			if(Size) for(;;)
			{
				// If we are already at the end of the list, we have too many bytes!
				if(it == Children.end()) 
				{
					warning("Extra bytes found parsing buffer in MDObject::ReadValue()\n");
					break;
				}

				Uint32 ThisBytes = (*it)->ReadValue(Buffer, Size);
				Bytes += ThisBytes;

				it++;
				
				if(ThisBytes >= Size) break;

				Buffer += ThisBytes;
				Size -= ThisBytes;
			}

			if(it != Children.end())
			{
				warning("Not enough bytes in buffer in MDObject::ReadValue() for %s\n", Name().c_str());
			}

			return Bytes;
		}

	case SET:
		{
			Uint32 Bytes = 0;

			// Start with an empty list
			Children.clear();
			ChildrenNames.clear();

			ASSERT(UsePrimer);
			if(!UsePrimer)
			{
				error("No Primer supplier when reading set in MDObject::ReadValue()\n");
				return 0;
			}

			// Scan until out of data
			while(Size)
			{
				int KeyLen = 0;

				DataChunk Key;
				Uint32 ThisBytes = ReadKey(Type->GetDict()->KeyFormat, Size, Buffer, Key);

				// Abort if we can't read the key
				// this prevents us looping for ever if we
				// come across invalid data
				if(ThisBytes == 0) break;

				// Advance counters and pointers passed key
				Size -= ThisBytes;
				Buffer += ThisBytes;
				Bytes += ThisBytes;

				Uint32 Length;
				ThisBytes = ReadLength(Type->GetDict()->LenFormat, Size, Buffer, Length);

				// Advance counters and pointers passed Length
				Size -= ThisBytes;
				Buffer += ThisBytes;
				Bytes += ThisBytes;

				if(Length)
				{
					if(Size < Length)
					{
						error("Not enough bytes for value in MDObject::ReadValue() for %s\n", Name().c_str());
						
						// Read what we can!
						Length = Size;
					}

					MDObjectPtr NewItem;
					if(Type->GetDict()->KeyFormat == DICT_KEY_2_BYTE)
					{
						ASSERT(Key.Size == 2);
						Tag ThisKey = GetU16(Key.Data);

						NewItem = new MDObject(ThisKey, UsePrimer);
					}
					else if(Type->GetDict()->KeyFormat == DICT_KEY_AUTO)
					{
						ASSERT(Key.Size == 16);
						ULPtr ThisUL = new UL(Key.Data);

						NewItem = new MDObject(ThisUL);
					}
					else
					{
						// Only 2-byte and 16-byte keys are supported at present
						ASSERT(0);
						return 0;
					}
					
					ThisBytes = NewItem->ReadValue(Buffer, Length);
/*if(NewItem->Value)
{
 printf(">Read %s, size = %d, object size = %d (%d)\n>", NewItem->Name().c_str(), ThisBytes, NewItem->Value->GetData().Size, NewItem->Value->size());
 DataChunk Dat = NewItem->Value->PutData();
 int i;
 for(i=0; i<Dat.Size; i++)
 {
   printf("%02x ", Dat.Data[i]);
 }
 printf("\n>%s\n", NewItem->Value->GetString().c_str());
}
else
 printf("Read %s, size = %d, Subs = %d)\n", NewItem->Name().c_str(), ThisBytes, NewItem->Children.size());

printf("Length = %d\n", Length);
*/
					if(ThisBytes != Length)
					{
						error("Failed to read complete %s/%s value - specified length=%d, read=%d\n", 
							  Name().c_str(), NewItem->Name().c_str(), Length, ThisBytes);
						
						// Skip anything left over
						if(Length > ThisBytes) ThisBytes = Length;
					}
					Size -= ThisBytes;
					Buffer += ThisBytes;
					Bytes += ThisBytes;

					AddChild(NewItem);
				}
			}

			return Bytes;
		}

	default:
		ASSERT(0);
		return 0;
	}
}


//! Read a key from a memory buffer
Uint32 MDObject::ReadKey(DictKeyFormat Format, Uint32 Size, const Uint8 *Buffer, DataChunk& Key)
{
	int KeySize;

	switch(Format)
	{
	default:
	// Unsupported key types!
	case DICT_KEY_NONE:
	case DICT_KEY_AUTO:		// DRAGONS: Should probably make this work at some point!
		ASSERT(0);
		Key.Resize(0);
		return 0;

	case DICT_KEY_1_BYTE:		KeySize = 1; break;
	case DICT_KEY_2_BYTE:		KeySize = 2; break;
	case DICT_KEY_4_BYTE:		KeySize = 4; break;
	}

	if(Size < KeySize)
	{
		error("Not enough bytes for required key type in MDObject::ReadKey()\n");
		Key.Resize(0);
		return 0;
	}

	Key.Resize(KeySize);
	Key.Set(KeySize, Buffer);

	return KeySize;
}


//! Read a length field from a memory buffer
Uint32 MDObject::ReadLength(DictLenFormat Format, Uint32 Size, const Uint8 *Buffer, Uint32& Length)
{
	int LenSize;

	switch(Format)
	{
	default:
	// Unsupported key types!
	case DICT_LEN_NONE:
	case DICT_LEN_BER:		// DRAGONS: Should probably make this work at some point!
		ASSERT(0);
		Length = 0;
		return 0;

	case DICT_LEN_1_BYTE:		
		{ 
			if(Size >= 1) 
			{ 
				Length = GetU8(Buffer); 
				return 1;
			};

			// Else we drop through to error handler
			break;
		}

	case DICT_LEN_2_BYTE:		{ LenSize = 2;  Length = GetU16(Buffer); };
		{ 
			if(Size >= 2) 
			{ 
				Length = GetU16(Buffer); 
				return 2;
			};

			// Else we drop through to error handler
			break;
		}

	case DICT_LEN_4_BYTE:		{ LenSize = 4;  Length = GetU32(Buffer); };
		{ 
			if(Size >= 4) 
			{ 
				Length = GetU32(Buffer); 
				return 4;
			};

			// Else we drop through to error handler
			break;
		}
	}

	error("Not enough bytes for required length field in MDObject::ReadLength()\n");
	Length = 0;
	return 0;
}


//** Static Instantiations for MDOType class **
//*********************************************

MDOTypeList MDOType::AllTypes;	//!< All types managed by the MDOType class
MDOTypeList MDOType::TopTypes;	//!< The top-level types managed by the MDOType class

//! Map for UL lookups
std::map<UL, MDOTypePtr> MDOType::ULLookup;
		
//! Map for reverse lookups based on DictEntry pointer
std::map<DictEntry*, MDOTypePtr> MDOType::DictLookup;

//! Map for reverse lookups based on type name
std::map<std::string, MDOTypePtr> MDOType::NameLookup;

MDOType::DictManager MDOType::DictMan;		//!< Dictionary manager

