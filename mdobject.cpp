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

		StringList::iterator it2 = (*it)->ChildOrder.begin();
		while(it2 != (*it)->ChildOrder.end())
		{
			MDOTypePtr Current = NameLookup[(*it)->FullName() + "/" + *it2];
			debug("  Sub->: %s = %s\n", (*it2).c_str() , Current->GetDict()->Name);

			StringList::iterator it3 = Current->ChildOrder.begin();
			while(it3 != Current->ChildOrder.end())
			{
				debug("    SubSub->: %s\n", (*it3).c_str());
				it3++;
			}
			it2++;
//			it2n++;
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
/*! \return Pointer to a string constant
 *  \return "" if the DictType is not known or is a container (e.g. a pack)
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
		ParentType->insert(NewType);
	}

	// Build base name for any children
	MDOTypePtr Scan = ParentType;
	NewType->RootName = "";
	if(Scan) NewType->RootName = ParentType->FullName() + "/";

	// Copy any children from our base
	if(Dict->Base)
	{
		MDOTypePtr Base = DictLookup[Dict->Base];
		
		// Add child names to name lookup
		StringList::iterator it = Base->ChildOrder.begin();
		while(it != Base->ChildOrder.end())
		{
			MDOTypePtr Current = NameLookup[Base->FullName() + "/" + (*it)];

			ASSERT(Current);

			// Add the base types children
			NewType->insert(Current);

			NameLookup[NewType->FullName() + "/" + (*it)] = Current;
			it++;
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
			Type = "";
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
			std::string Temp = NewType->RootName;
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
	NameLookup[NewType->RootName + std::string(Dict->Name)] = NewType;
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
/*! \return Pointer to the object
 *  \return NULL if there is no type of that name
 */
MDOTypePtr MDOType::Find(std::string BaseType)
{
	MDOTypePtr theType;

	std::map<std::string,MDOTypePtr>::iterator it = NameLookup.find(BaseType);
	if(it == NameLookup.end())
	{
		return NULL;
	}

	theType = (*it).second;

	return theType;
}


//! Find the MDOType object that defines a type with a specified UL
/*! \return Pointer to the object
 *  \return NULL if there is no type with that UL
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
 *  \return Pointer to the object
 *  \return NULL if there is no type with that UL
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
	if(Type)
	{
		ObjectName = Type->Name();
	}
	else
	{
		error("Metadata object type \"%s\" doesn't exist\n", BaseType);

		Type = MDOType::Find("Unknown");

		ASSERT(Type);

		ObjectName = "Unknown (" + std::string(BaseType) + ")";
	}

	IsConstructed = true;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;
	TheUL = Type->GetUL();
	TheTag = 0;

	// Initialise the new object
	Init();
}


//! MDObject typed constructor
/*! Builds a "blank" metadata object of a specified type
*/
MDObject::MDObject(MDOTypePtr BaseType) : Type(BaseType) 
{
	ObjectName = BaseType->Name();
	
	IsConstructed = true;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;
	TheUL = Type->GetUL();
	TheTag = 0;

	// Initialise the new object
	Init();
};


//! MDObject typed constructor
/*! Builds a "blank" metadata object of a specified type
*/
MDObject::MDObject(ULPtr UL) 
{
	Type = MDOType::Find(UL);
	if(Type)
	{
		ObjectName = Type->Name();
	}
	else
	{
		Type = MDOType::Find("Unknown");

		ObjectName = "Unknown (" + UL->GetString() + ")";

		ASSERT(Type);

		// Shall we tray and parse this?
		// DRAGONS: Somewhat clunky version for 2-byte tag, 2-byte len
#define ParseDark true
		if(ParseDark)
		{
			static MDOTypePtr Preface = MDOType::Find("Preface");

			ASSERT(Preface);

			if(memcmp(Preface->TypeUL->GetValue(), UL->GetValue(), 6) == 0)
			{
				Type = MDOType::Find("DefaultObject");
				ASSERT(Type);
			}
		}
	}

	IsConstructed = true;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;
	TheUL = UL;
	TheTag = 0;

	// Initialise the new object
	Init();
};


//! MDObject typed constructor
/*! Builds a "blank" metadata object of a specified type
*/
MDObject::MDObject(Tag BaseTag, PrimerPtr BasePrimer)
{
	Primer::iterator it = BasePrimer->find(BaseTag);

	if(it == BasePrimer->end())
	{
		error("Metadata object with Tag \"%s\" doesn't exist in specified Primer\n", Tag2String(BaseTag).c_str());

		Type = MDOType::Find("Unknown");

		ASSERT(Type);

		// DRAGONS: What do we do if the UL is unknown!!
		TheUL = new UL();

		ObjectName = "Unknown (" + Tag2String(BaseTag) + ")";
	}
	else
	{
		TheUL = new UL((*it).second);

		Type = MDOType::Find(TheUL);

		if(Type)
		{
			ObjectName = Type->Name();
		}
		else
		{
			Type = MDOType::Find("Unknown");

			ASSERT(Type);

			ObjectName = "Unknown (" + Tag2String(BaseTag) + " -> " + TheUL->GetString() + ")";
		}
	}

	IsConstructed = true;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;
	TheTag = BaseTag;

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

			StringList::iterator it = Type->ChildOrder.begin();
			while(it != Type->ChildOrder.end())
			{
				MDOTypePtr Current = MDOType::Find(Type->FullName() + "/" + (*it));

				ASSERT(Current);

				if(!Current)
				{
					error("Cannot find type %s in Init (Pack)\n", (*it).c_str());
				}
				else
				{
					insert(new MDObject(Current));
				}

				it++;
			}
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
 *  \return NULL if it is not a valid child to add to this type of container
 *	\note If you want to add an child that is non-standard (i.e. not listed
 *        as a child in the container's MDOType then you must build the child
 *        then add it with AddChild(MDObjectPtr)
 *  \note If you want to add a second child of the same name to an object
 *        you must build the child and then add it with AddChild(MDObjectPtr)
 */
MDObjectPtr MDObject::AddChild(std::string ChildName)
{
	// Try and find an existing child
	MDObjectPtr Ret = Child(ChildName);

	// Only add a new one if we didn't find it
	if(!Ret)
	{
		// Find the child definition
		MDOType::iterator it = Type->find(ChildName);

		// Return NULL if not found
		if(it == Type->end()) return Ret;

		// Insert a new item of the correct type
		MDObjectPtr Ret = new MDObject((*it).second);
		insert(Ret);
	}

	// Return smart pointer to the new object
	return Ret;
};


//! Add a given MDObject to an MDObject continer
/*! \return pointer to the object added (for compatibility with other ADDChild funcs)
 *  \return NULL if there was an error
 *  \note If there is already a child of this type it is removed
 */
MDObjectPtr MDObject::AddChild(MDObjectPtr ChildObject, bool Replace /* = false */)
{
	// If replacing, remove any existing children of this type
	if (Replace) RemoveChild(ChildObject->Type);

	// Insert the new item at the end
	insert(ChildObject);

	return ChildObject;
}


//! Remove any children with a specified name from an MDObject continer
void MDObject::RemoveChild(std::string ChildName)
{
	MDObjectNamedList::iterator it = begin();
	while(it != end())
	{
		if((*it).first == ChildName)
		{
			erase(it);

			// Restart the scan - DRAGONS: Not very efficient
			it = begin();
		}
		else
		{
			it++;
		}
	}
}


//! Remove any children of a specified type from an MDObject continer
void MDObject::RemoveChild(MDOTypePtr ChildType)
{
	// Note that we cannot rely on removing by name as names are changeable
	
	MDObjectNamedList::iterator it = begin();
	while(it != end())
	{
		if((*it).second->Type == ChildType)
		{
			erase(it);

			// Restart the scan - DRAGONS: Not very efficient
			it = begin();
		}
		else
		{
			it++;
		}
	}
}


//! Remove a specified object from an MDObject continers children lists
/*! If the object is not an child of the container nothing is done
 */
void MDObject::RemoveChild(MDObjectPtr ChildObject)
{
	MDObjectNamedList::iterator it = begin();
	while(it != end())
	{
		if((*it).second == ChildObject)
		{
			erase(it);
			return;
		}
		else
		{
			it++;
		}
	}
}


//! MDObject destructor
/*! Frees the data of the object if it exists
*/
MDObject::~MDObject()
{
	// Free any memory used
}



//! Access named sub-item within a compound MDObject
/*! If the child does not exist in this item then NULL is returned
 *  even if it is a valid child to have in this type of container
 *
 *  DRAGONS: This doesn't work well with SmartPtrs
 *           so member function Child() is also available
*/
MDObjectPtr MDObject::operator[](std::string ChildName)
{
	MDObjectNamedList::iterator it = begin();
	while(it != end())
	{
		if((*it).first == ChildName)
		{
			return (*it).second;
		}
		it++;
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
	MDObjectNamedList::iterator it = begin();
	while(it != end())
	{
		if((*it).second->Type == ChildType)
		{
			return (*it).second;
		}
		it++;
	}

	return NULL;
}


//! Find all sub-items within a compound MDObject of a named type
MDObjectListPtr MDObject::ChildList(std::string ChildName)
{
	MDObjectListPtr Ret = new MDObjectList;
	MDObjectNamedList::iterator it = begin();

	while(it != end())
	{
		if((*it).first == ChildName)
		{
			// Add this object to the list
			Ret->push_back((*it).second);
		}
		it++;
	}

	return Ret;
}


//! Find all sub-items within a compound MDObject of a named type
MDObjectListPtr MDObject::ChildList(MDOTypePtr ChildType)
{
	MDObjectListPtr Ret = new MDObjectList;
	MDObjectNamedList::iterator it = begin();

	while(it != end())
	{
		if((*it).second->Type == ChildType)
		{
			// Add this object to the list
			Ret->push_back((*it).second);
		}
		it++;
	}

	return Ret;
}


//! Read value from a buffer
/*! Note that collection headers are handled here rather than in the MDValue
 *  because MDValue objects don't differentiate. A primer must be supplied for reading sets
 *
 *  DRAGONS: This function is bloated and should be split up
 *
 *  \return Number of bytes read
 */
Uint32 MDObject::ReadValue(const Uint8 *Buffer, Uint32 Size, PrimerPtr UsePrimer /*=NULL*/)
{
	Uint32 Bytes = 0;
	Uint32 Count = 0;
	Uint32 ItemSize = 0;

	switch(Type->GetContainerType())
	{
	case NONE:
		return Value->ReadValue(Buffer, Size);

	case BATCH:
		{
			ASSERT(Size >= 8);

			Count = GetU32(Buffer);
			Buffer += 4;

			ItemSize = GetU32(Buffer);
			Buffer += 4;

			if(Size <= 8) Size = 0; else Size -= 8;
			if((ItemSize*Count) != Size)
			{
				warning("Malformed batch found, item size = %u, count = %u, but bytes = %u\n",
						ItemSize, Count, (Size));

				// Prevent us reading off the end of the buffer
				if(Size < (ItemSize*Count))	Count = Size / ItemSize;
			}

			Bytes = 8;
			Size = ItemSize;
		}
		// Fall through and process as an array

	case ARRAY:
		{
			if(Type->empty())
			{
				error("Object %s is a multiple, but has no contained types\n", Name().c_str());
				return Bytes;
			}

			// Start with no children
			clear();

			// Find the first (or only) child type
			StringList::iterator it = Type->ChildOrder.begin();
			MDOType::iterator it2 = Type->find(*it);
			ASSERT(it2 != Type->end());
			MDOTypePtr ChildType = (*it2).second;
			ASSERT(ChildType);

			int ChildCount = Type->size();
			while(Size || Count)
			{
				MDObjectPtr NewItem = new MDObject(ChildType);

				ASSERT(NewItem);

				NewItem->Parent = this;
				NewItem->ParentOffset = Bytes;
				NewItem->KLSize = 0;

				Uint32 ThisBytes = NewItem->ReadValue(Buffer, Size);

				Bytes += ThisBytes;
				Buffer += ThisBytes;
				if(ThisBytes > Size) Size = 0; else Size -= ThisBytes;
				insert(NewItem);


				bool ItemStart = true;

				// If this array has multiple children, get the next type
				if(ChildCount > 1)
				{
					it++;
					
					if(it == Type->ChildOrder.end()) it = Type->ChildOrder.begin(); 
					else ItemStart = false;
					
					it2 = Type->find(*it);
					ASSERT(it2 != Type->end());
					ChildType = (*it2).second;
					ASSERT(ChildType);
				}

				// If processing a batch, set up for the next item
				if(ItemStart && (Count != 0)) 
				{
					if(--Count) Size = ItemSize; else break;
				}
			}

			if((ChildCount > 1) && (it != Type->ChildOrder.begin()))
			{
				error("Multiple %s at 0x%s in %s does not contain an integer number of sub-items\n", 
					  FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str());
			}

			return Bytes;
		}

	case PACK:
		{
			debug("Reading pack at 0x%s\n", Int64toHexString(GetLocation(), 8).c_str());

			Uint32 Bytes = 0;
			MDObjectNamedList::iterator it = begin();
			if(Size) for(;;)
			{
				// If we are already at the end of the list, we have too many bytes!
				if(it == end()) 
				{
					warning("Extra bytes found parsing buffer in MDObject::ReadValue()\n");
					break;
				}

				(*it).second->Parent = this;
				(*it).second->ParentOffset = Bytes;
				(*it).second->KLSize = 0;

				// DRAGONS: Array length calculation fudge!
				// If an array exists in a pack there is no easy way to determine the size 
				// of the array unless it is the last item in the pack.  Unfortunately there
				// are some cases where MXF packs have arrays that are not the last entry
				// This section deals with each in turn (Nasty!!)

				Uint32 ValueSize = Size;
				if((*it).second->Type->GetContainerType() == ARRAY)
				{
					std::string FullName = (*it).second->FullName();
					if(FullName == "IndexTableSegment/IndexEntryArray/SliceOffsetArray")
					{
						// Number of entries in SliceOffsetArray is in IndexTableSegment/SliceCount
						// Each entry is 4 bytes long
						ValueSize = Parent->GetInt("SliceCount") * 4;
					}
					else if(FullName == "RandomIndexMetadata/PartitionArray")
					{
						// RandomIndexMetadata/PartitionArray is followed by a Uint32
						if(ValueSize >4) ValueSize = ValueSize - 4; else ValueSize = 0;
					}
				}

				Uint32 ThisBytes = (*it).second->ReadValue(Buffer, ValueSize);

				debug("  at 0x%s Pack item %s = %s\n", Int64toHexString((*it).second->GetLocation(), 8).c_str(), 
					  (*it).first.c_str(), (*it).second->GetString().c_str());

				Bytes += ThisBytes;

				it++;
				
				if(ThisBytes >= Size) break;

				Buffer += ThisBytes;
				Size -= ThisBytes;
			}

			if(it != end())
			{
				warning("Not enough bytes in buffer for %s at 0x%s in %s\n", 
					    FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str());
			}

			return Bytes;
		}

	case SET:
		{
			debug("Reading set at 0x%s\n", Int64toHexString(GetLocation(), 8).c_str());

			Uint32 Bytes = 0;

			// Start with an empty list
			clear();

			ASSERT(UsePrimer);
			if(!UsePrimer)
			{
				error("No Primer supplier when reading set %s in MDObject::ReadValue()\n", FullName().c_str());
				return 0;
			}

			// Scan until out of data
			while(Size)
			{
				Uint32 BytesAtItemStart = Bytes;

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
						error("Not enough bytes for value for %s at 0x%s in %s\n", 
							  FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str());

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
					
					NewItem->Parent = this;
					NewItem->ParentOffset = BytesAtItemStart;
					NewItem->KLSize = Bytes - BytesAtItemStart;

					ThisBytes = NewItem->ReadValue(Buffer, Length);

					debug("  at 0x%s Set item (%s) %s = %s\n", Int64toHexString(NewItem->GetLocation(), 8).c_str(), Key.GetString().c_str(), NewItem->Name().c_str(), NewItem->GetString().c_str());

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
						error("Failed to read complete %s value at 0x%s in %s - specified length=%d, read=%d\n", 
							  NewItem->FullName().c_str(), Int64toHexString(NewItem->GetLocation(), 8).c_str(), 
							  NewItem->GetSource().c_str(), Length, ThisBytes);
						
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


//! Get the location within the ultimate parent
Uint64 mxflib::MDObject::GetLocation(void)
{
	Uint64 Ret = ParentOffset;

	if(Parent) Ret += Parent->KLSize + Parent->GetLocation();

	return Ret;
}

//! Get text that describes where this item came from
std::string mxflib::MDObject::GetSource(void)
{
	if(Parent) return Parent->GetSource();
	if(ParentFile) return std::string("file \"") + ParentFile->Name + std::string("\"");

	return std::string("memory buffer");
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

