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



//! Static primer to use for index tables
PrimerPtr MDOType::DictManager::StaticPrimer;


// Map used to convert KLVLib "DictType" enum to text string of type name
typedef std::map<DictType, char *> KLVLib_XLateType;
KLVLib_XLateType KLVLib_XLate;


//! Initialise the table used to convert KLVLib "DictType" enum to text string of type name
void InitDictType2Text(void)
{
	if(KLVLib_XLate.empty())
	{
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_NONE,"Unknown"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_U8,"Uint8"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_I8,"Int8"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_U16,"Uint16"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_I16,"Int16"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_U32,"Uint32"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_I32,"Int32"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_U64,"Uint64"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_I64,"Int64"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_ISO7,"ISO7"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_UTF8,"UTF8"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_UTF16,"UTF16"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_UUID,"UUID"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_UMID,"UMID"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_LABEL,"Label"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_TIMESTAMP,"TimeStamp"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_VERTYPE,"VersionType"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_RATIONAL,"Rational"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_BOOLEAN,"Boolean"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_ISO7STRING,"ISO7String"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_UTF16STRING,"UTF16String"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_IEEEFLOAT64,"Float64"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_UINT8STRING,"Uint8Array")); // DRAGONS: Is this right?
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_PRODUCTVERSION,"ProductVersion"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_RAW,"Uint8Array"));
		KLVLib_XLate.insert(KLVLib_XLateType::value_type(DICT_TYPE_I32ARRAY,"Int32Array"));
	}
}


//! Convert KLVLib "DictType" enum to text string of type name
/*! \return Pointer to a string constant.
 *  \return "" if the DictType is not known or is a container (e.g. a pack)
 */
char *DictType2Text(DictType Type)
{
	KLVLib_XLateType::iterator it = KLVLib_XLate.find(Type);

	if(it == KLVLib_XLate.end()) return "";

	return (*it).second;
}


//! MDDict constructor
/*! Loads the dictionary from the specified file
*/
void MDOType::DictManager::Load(const char *DictFile)
{
	// Initialise the map that converts KLVLib dictionary type enums to type names
	InitDictType2Text();

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

/*
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
*/

	// Build a static primer (for use in index tables)
	StaticPrimer = MakePrimer();
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
 *	\note if BasePrimer is NULL then a standard dictionary lookup of known static tags is performed
 *  \return Pointer to the object
 *  \return NULL if there is no type with that UL
 */
MDOTypePtr MDOType::Find(Tag BaseTag, PrimerPtr BasePrimer)
{
	MDOTypePtr theType;

	if(BasePrimer)
	{
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
	}
	else
	{
		// See if we know this static tag
		if(BaseTag < 0x8000)
		{
			Uint8 Key[2];
			Key[0] = BaseTag >> 8;
			Key[1] = BaseTag & 0xff;

			DictEntry *Dict = FindDictByLocalKey(DictMan.MainDict, 2, Key, NULL);

			if(Dict) return MDOType::Find(new UL(Dict->GlobalKey));
		}
	}

	return theType;
};



//! MDObject named constructor
/*! Builds a "blank" metadata object of a named type
 *	\note packs are built with defaut values
 */
MDObject::MDObject(std::string BaseType)
{
	Type = MDOType::Find(BaseType);
	if(Type)
	{
		ObjectName = Type->Name();
	}
	else
	{
		error("Metadata object type \"%s\" doesn't exist\n", BaseType.c_str());

		Type = MDOType::Find("Unknown");

		ASSERT(Type);

		ObjectName = "Unknown (" + BaseType + ")";
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
 *	\note packs are built with defaut values
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
 *	\note packs are built with defaut values
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

		// Shall we try and parse this?
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
 *	\note packs are built with defaut values
 */
MDObject::MDObject(Tag BaseTag, PrimerPtr BasePrimer)
{
	// Try and find the tag in the primer
	if(BasePrimer) 
	{
		Primer::iterator it = BasePrimer->find(BaseTag);

	// Didn't find it!!
		if(it == BasePrimer->end())
		{
			error("Metadata object with Tag \"%s\" doesn't exist in specified Primer\n", Tag2String(BaseTag).c_str());
			
			// See if we know this tag anyway
			Type = MDOType::Find(BaseTag, NULL);

			// If it is a "known" static then use it (but still give the error)
			if(Type) TheUL = Type->TypeUL;
		}
		else
		{
			// It was found in the primer, so lookup the type from the UL
			TheUL = new UL((*it).second);
			Type = MDOType::Find(TheUL);
		}
	}
	else
	{
		// No primer supplied - see if we know this tag anyway
		Type = MDOType::Find(BaseTag, NULL);
		if(Type) TheUL = Type->TypeUL;
	}

	// If it was unknown build an "Unknown" and set a meaningful name
	if(!Type)
	{
		Type = MDOType::Find("Unknown");

		ASSERT(Type);

		if(TheUL)
		{
			// Tag found, but UL unknown
			ObjectName = "Unknown (" + Tag2String(BaseTag) + " -> " + TheUL->GetString() + ")";
		}
		else
		{
			// Tag not found, build a blank UL
			TheUL = new UL();
			ObjectName = "Unknown (" + Tag2String(BaseTag) + ")";
		}
	}
	else
	{
		ObjectName = Type->Name();
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
 *	\note packs are built with defaut values
 */
void MDObject::Init(void)
{
	SetModified(true); 

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
					MDObjectPtr NewItem = new MDObject(Current);
					NewItem->SetDefault();
					insert(NewItem);
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
};


//! Add an empty named child to an MDObject continer and return a pointer to it
/*! If Replace is true (or not supplied) and a child of this name already 
 *	exists a pointer to that child is returned but the value is not changed.
 *  \return NULL if it is not a valid child to add to this type of container
 *	\note If you want to add an child that is non-standard (i.e. not listed
 *        as a child in the container's MDOType then you must build the child
 *        then add it with AddChild(MDObjectPtr)
 */
MDObjectPtr MDObject::AddChild(std::string ChildName, bool Replace /*=true*/)
{
	MDObjectPtr Ret;

	SetModified(true); 

	// Try and find an existing child (if replacing)
	if(Replace) Ret = Child(ChildName); else Ret = NULL;

	// Only add a new one if we didn't find it
	if(!Ret)
	{
		// Find the child definition
		MDOType::iterator it = Type->find(ChildName);

		// Return NULL if not found
		if(it == Type->end()) return Ret;

		// Insert a new item of the correct type
		Ret = new MDObject((*it).second);
		insert(Ret);
	}

	// Return smart pointer to the new object
	return Ret;
};


//! Add an empty child of a specified type to an MDObject continer and return a pointer to it
/*! If Replace is true (or not supplied) and a child of this type already 
 *	exists a pointer to that child is returned but the value is not changed.
 *  \return NULL if it is not a valid child to add to this type of container
 *	\note If you want to add an child that is non-standard (i.e. not listed
 *        as a child in the container's MDOType then you must build the child
 *        then add it with AddChild(MDObjectPtr)
 */
MDObjectPtr MDObject::AddChild(MDOTypePtr ChildType, bool Replace /*=true*/)
{
	MDObjectPtr Ret;

	SetModified(true); 

	// Try and find an existing child (if replacing)
	if(Replace) Ret = Child(ChildType); else Ret = NULL;

	// Only add a new one if we didn't find it
	if(!Ret)
	{
		// Find the child definition
		MDOType::iterator it = Type->find(ChildType->Name());

		// Return NULL if not found
		if(it == Type->end()) return Ret;

		// Insert a new item of the correct type
		Ret = new MDObject((*it).second);
		insert(Ret);
	}

	// Return smart pointer to the new object
	return Ret;
};


//! Add a given MDObject to an MDObject continer
/*! \return pointer to the object added (for compatibility with other ADDChild funcs)
 *  \return NULL if there was an error
 *  \note If there is already a child of this type it is replaces if Replace = true
 */
MDObjectPtr MDObject::AddChild(MDObjectPtr ChildObject, bool Replace /* = false */)
{
	SetModified(true); 

	return AddChildInternal(ChildObject, Replace);
}


//! Same as AddChild(), but does not set "Modified"
/*! \return pointer to the object added (for compatibility with other ADDChild funcs)
 *  \return NULL if there was an error
 *	This function is used when reading an objects children
 *  \note If there is already a child of this type it is replaces if Replace = true
 */
MDObjectPtr MDObject::AddChildInternal(MDObjectPtr ChildObject, bool Replace /* = false */)
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
	SetModified(true); 

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
	
	SetModified(true); 

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
	SetModified(true); 

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

	SetModified(false); 

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
				error("Malformed batch found in %s at 0x%s in %s - item size = %u, count = %u, but bytes = %u\n", 
					  FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str(),
					  ItemSize, Count, Size);

				// Prevent us reading off the end of the buffer
				if(Size < (ItemSize*Count))	Count = Size / ItemSize;
			}

			Bytes = 8;
			if(Count) Size = ItemSize; else Size = 0;

			// Don't try and read an empty batch
			if(Count == 0) return Bytes;
		}
		// Fall through and process as an array

	case ARRAY:
		{
			if(Type->empty())
			{
				error("Object %s at 0x%s in %s is a multiple, but has no contained types\n", FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str());
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

					AddChildInternal(NewItem);
				}
			}

			return Bytes;
		}

	default:
		ASSERT(0);
		return 0;
	}
}


//! Has this object (including any child objects) been modified?
bool MDObject::IsModified(void)
{ 
	if(Modified) return true;

	if(!empty())
	{
		MDObjectNamedList::iterator it = begin();

		while(it != end())
		{
			if((*it).second->IsModified()) return true;
			it++;
		}
	}

	return false; 
}


//! Clear the modified flag on this object and any contained objects
void MDObject::ClearModified(void)
{ 
	Modified = false;

	if(!empty())
	{
		MDObjectNamedList::iterator it = begin();

		while(it != end())
		{
			(*it).second->ClearModified();
			it++;
		}
	}
}


//! Set the GenerationUID of an object iff it has been modified
/*! \return true if the GenerationUID has been set, otherwise false
 *  \note If the object does not have a GenerationUID property false is returned!
 */
bool MDObject::SetGenerationUID(UUIDPtr NewGen)
{
	if(!IsModified()) return false;

	// Can't have a GenerationUID if not a set or pack
	MDContainerType CType = Type->GetContainerType();
	if((CType != SET) && (CType != PACK)) return false;

	// Quit if this object type doesn't have a GenerationUID
	if(Type->find("GenerationUID") == Type->end()) return false;

	// Find (or add) the GenerationUID property
	MDObjectPtr GenUID = Child("GenerationUID");
	if(!GenUID) GenUID = AddChild("GenerationUID");

//printf("Setting GenerationUID id %s to %s\n", FullName().c_str(), NewGen->GetString().c_str());
	ASSERT(GenUID);

	// Set the actual UID
	GenUID->Value->ReadValue(NewGen->GetValue(), NewGen->Size());

	return true;
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
//	int LenSize;

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

	case DICT_LEN_2_BYTE://		{ LenSize = 2;  Length = GetU16(Buffer); };
		{ 
			if(Size >= 2) 
			{ 
				Length = GetU16(Buffer); 
				return 2;
			};

			// Else we drop through to error handler
			break;
		}

	case DICT_LEN_4_BYTE://		{ LenSize = 4;  Length = GetU32(Buffer); };
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

//! Write this object, and any strongly linked sub-objects, to a memory buffer
/*! The object must be at the outer or top KLV level. 
 *	The objects are appended to the buffer
 *	\return The number of bytes written
 */
Uint32 MDObject::WriteLinkedObjects(DataChunk &Buffer, PrimerPtr UsePrimer /*=NULL*/)
{
	Uint32 Bytes = 0;

	Bytes = WriteObject(Buffer, NULL, UsePrimer);

	MDObjectNamedList::iterator it = begin();
	while(it != end())
	{
		if((*it).second->Link)
		{
			if((*it).second->GetRefType() == DICT_REF_STRONG) Bytes += (*it).second->Link->WriteLinkedObjects(Buffer, UsePrimer);
		}
		else if(!((*it).second->empty()))
		{
			MDObjectNamedList::iterator it2 = (*it).second->begin();
			MDObjectNamedList::iterator itend2 = (*it).second->end();
			while(it2 != itend2)
			{
				if((*it2).second->Link)
				{
					if((*it2).second->GetRefType() == DICT_REF_STRONG) 
					{
						Bytes += (*it2).second->Link->WriteLinkedObjects(Buffer, UsePrimer);
					}
				}
				else if(!((*it2).second->empty()))
				{
					error("Internal error for object %s - Cannot process nesting > 2 in WriteLinkedObjects()\n",
						   (*it2).second->FullName().c_str());
				}
				it2++;
			}
		}
		it++;
	}

	return Bytes;
}


//! Write this object to a memory buffer
/*! The object is appended to the buffer
 *	\return The number of bytes written
 */
Uint32 MDObject::WriteObject(DataChunk &Buffer, MDObjectPtr ParentObject, PrimerPtr UsePrimer /*=NULL*/)
{
	Uint32 Bytes = 0;

	DictLenFormat LenFormat;

//printf("WriteObject(%s) ", FullName().c_str());
	// DRAGONS: Should we update GenerationUID here ?

	// Write the key (and determine the length format)
	if(!ParentObject)
	{
//printf("no parent\n");
		Bytes += WriteKey(Buffer, DICT_KEY_AUTO, UsePrimer);
		LenFormat = DICT_LEN_BER;
	}
	else
	{
//printf("Parent %s, ", ParentObject->FullName().c_str());
		const DictEntry *Dict = ParentObject->Type->GetDict();
		ASSERT(Dict);

		// Only sets need keys
		if((Dict->Type == DICT_TYPE_UNIVERSAL_SET) || (Dict->Type == DICT_TYPE_LOCAL_SET))
		{
			Bytes = WriteKey(Buffer, Dict->KeyFormat, UsePrimer);
//printf("Key = %s, ", Buffer.GetString().c_str());
		}

		if((Dict->Type == DICT_TYPE_VECTOR) || (Dict->Type == DICT_TYPE_ARRAY))
		{
			LenFormat = DICT_LEN_NONE;
		}
		else
		{
			LenFormat = Dict->LenFormat;
		}
//printf("Dict->Type = %d, ", Dict->Type);
//if(LenFormat == DICT_LEN_BER) printf("Length = BER\n");
//else printf("Length = %d-byte\n", (int)LenFormat);
	}

	// The rest depends on the container type
	MDContainerType CType = Type->GetContainerType();

	// Build value
	if(CType == BATCH || CType == ARRAY)
	{
		Uint32 Count = 0;
		Uint32 Size;

		// DRAGONS: Pre-allocating a buffer could speed things up
		DataChunk Val;

		// Work out how many sub-items per child 
		Uint32 SubCount = Type->ChildOrder.size();
		
		// Count of remaining subs for this item
		Uint32 Subs = 0;

		MDObjectNamedList::iterator it = begin();

		while(it != end())
		{
			// Start of an item
			if(Subs == 0)
			{
				Subs = SubCount;
				Size = 0;
				Count++;
			}
			Uint32 ThisBytes = (*it).second->WriteObject(Val, this, UsePrimer);
			Bytes += ThisBytes;
			Size += ThisBytes;
			
			Subs--;
			it++;
		}

		// Determine item size if batch is empty
		// May not be strictly required, but 0 items of 0 size is a little dubious
		if(Count == 0)
		{
			DataChunk Temp;

			StringList::iterator it = Type->ChildOrder.begin();
			while(it != Type->ChildOrder.end())
			{
				MDOType::iterator it2 = Type->find(*it);
				ASSERT(it2 != Type->end());
				MDOTypePtr ChildType = (*it2).second;
				ASSERT(ChildType);

				MDObjectPtr Ptr = new MDObject(ChildType);
				Ptr->WriteObject(Temp, this, UsePrimer);
				it++;
			}
			Size = Temp.Size;
		}

		if(CType == BATCH)
		{
			// Write the length and batch header
			Bytes += WriteLength(Buffer, Val.Size+8, LenFormat);
			Uint8 Buff[4];
			PutU32(Count, Buff);
			Buffer.Append(4, Buff);
			PutU32(Size, Buff);
			Buffer.Append(4, Buff);
			Bytes += 8;
		}
		else
		{
			Bytes += WriteLength(Buffer, Val.Size, LenFormat);
		}

		// Append this data
		Buffer.Append(Val);
		Bytes += Val.Size;
//printf("  > %s\n", Val.GetString().c_str());
	}
	else if(CType == PACK)
	{
//printf("  *PACK*\n");
		// DRAGONS: Pre-allocating a buffer could speed things up
		DataChunk Val;

		// Ensure we write the pack out in order
		StringList::iterator it = Type->ChildOrder.begin();

		while(it != Type->ChildOrder.end())
		{
			MDObjectPtr Ptr = Child(*it);
			if(!Ptr)
			{
				error("Pack %s is missing sub-item %s\n", FullName().c_str(), (*it).c_str());
			}
			else
			{
				Bytes += Ptr->WriteObject(Val, this, UsePrimer);
			}
			it++;
		}

		// Write the length of the value
		Bytes += WriteLength(Buffer, Val.Size, LenFormat);

		// Append this data
		Buffer.Append(Val);
		Bytes += Val.Size;
//printf("  > %s\n", Val.GetString().c_str());
	}
	else if(!empty())
	{
//printf("  *Not Empty*\n");
		// DRAGONS: Pre-allocating a buffer could speed things up
		DataChunk Val;

		MDObjectNamedList::iterator it = begin();

		while(it != end())
		{
			Bytes += (*it).second->WriteObject(Val, this, UsePrimer);
			it++;
		}

		// Write the length of the value
		Bytes += WriteLength(Buffer, Val.Size, LenFormat);

		// Append this data
		Buffer.Append(Val);
		Bytes += Val.Size;
//printf("  > %s\n", Val.GetString().c_str());
	}
	else if(Value)
	{
//printf("  *Value*\n");
		DataChunk Val = Value->PutData();
		Bytes += WriteLength(Buffer, Val.Size, LenFormat);
		Buffer.Append(Val);
		Bytes += Val.Size;
//printf("  > %s\n", Val.GetString().c_str());
	}
	else
	{
//printf("  *Empty!*\n");
		Bytes += WriteLength(Buffer, 0, LenFormat);
	}

	return Bytes;
}


//! Write a length field to a memory buffer
/*!	The length is <b>appended</b> to the specified buffer
 *	\param Buffer	The buffer to receive the length
 *	\param Length	The length to be written
 *	\param Format	The format to use for the length
 *	\param Size		The total number of bytes to write for a BER length (or 0 for auto)
 *	\return Number of bytes written
 *	\note If the format is BER and a size is specified it will be overridden for
 *		  lengths that will not fit. However an error message will be produced.
 */
Uint32 MDObject::WriteLength(DataChunk &Buffer, Uint64 Length, DictLenFormat Format, Uint32 Size /*=0*/)
{
	switch(Format)
	{
	default:
	case DICT_LEN_NONE:
		return 0;

	case DICT_LEN_BER:
		{
			DataChunkPtr BER = MakeBER(Length, Size);
			Buffer.Append(*BER);
			return BER->Size;
		}

	case DICT_LEN_1_BYTE:		
		{ 
			Uint8 Buff;
			PutU8(Length, &Buff);

			Buffer.Append(1, &Buff);
			return 1;
		}

	case DICT_LEN_2_BYTE:
		{ 
			Uint8 Buff[2];
			PutU16(Length, Buff);

			Buffer.Append(2, Buff);
			return 2;
		}

	case DICT_LEN_4_BYTE:
		{ 
			Uint8 Buff[4];
			PutU32(Length, Buff);

			Buffer.Append(4, Buff);
			return 4;
		}
	}
}


//! Write an objects key
/*!	The key is <b>appended</b> to the specified buffer
 *	\return Number of bytes written
 *	\note If the object has no parent the full UL will be written, otherwise
 *		  the parent will be examined to determine the type of key to write.
 *	\note If a 2-byte local tag is used the primer UsePrimer is used to determine
 *		  the correct tag. UsePrimer will be updated if it doesn't yet incude the tag
 */
Uint32 MDObject::WriteKey(DataChunk &Buffer, DictKeyFormat Format, PrimerPtr UsePrimer /*=NULL*/)
{
	switch(Format)
	{
	default:
	case DICT_KEY_NONE:
		return 0;

	case DICT_KEY_AUTO:
		{
			if(!TheUL)
			{
				error("Call to WriteKey() for %s, but the UL is not known\n", FullName().c_str());
				return 0;
			}

			Buffer.Append(16, TheUL->GetValue());
			return 16;
		}

	case DICT_KEY_2_BYTE:
		{ 
			ASSERT(UsePrimer);

			if(!TheUL)
			{
				error("Call to WriteKey() for %s, but the UL is not known\n", FullName().c_str());
				return 0;
			}


			Tag UseTag;
			if(UsePrimer) UseTag = UsePrimer->Lookup(TheUL, TheTag);
			else UseTag = Primer::StaticLookup(TheUL, TheTag);

			Uint8 Buff[2];
			PutU16(UseTag, Buff);

			Buffer.Append(2, Buff);
			return 2;
		}

	case DICT_KEY_1_BYTE:		
	case DICT_KEY_4_BYTE:
		{ 
			ASSERT(0);
			error("Call to WriteKey() for %s, but 1 and 4 byte tags not currently supported\n", FullName().c_str());
			return 0;
		}
	}
}


//! Make a link from this reference source to the specified target set
/*! If the target set already has an instanceUID it will be used, otherwise
 *	one will be added.
 *	\return true on success, else false
 *	\note The link will be made from the source <b>property</b> to the target <b>set</b>
 *		  so be aware that "this" must be a reference source property and "TargetSet"
 *		  must be a set (or pack) containing an InstanceUID property which is a
 *		  reference target
 */
bool MDObject::MakeLink(MDObjectPtr TargetSet)
{
	Uint8 TheUID[16];
	
	// Does the target set already have an InstanceUID?
	MDObjectPtr InstanceUID = TargetSet["InstanceUID"];

	// If not add one
	if(!InstanceUID)
	{
		InstanceUID = TargetSet->AddChild("InstanceUID");

		// If this failed then chances are the set is not a reference target
		if(!InstanceUID)
		{
			error("Attempt to reference %s from %s failed\n", FullName().c_str(), TargetSet->FullName().c_str());
			return false;
		}

		MakeUUID(TheUID);
		InstanceUID->ReadValue(TheUID, 16);
	}
	else
	{
		DataChunk Data = InstanceUID->Value->PutData();
		ASSERT(Data.Size == 16);
		memcpy(TheUID, Data.Data, 16);
	}

	// Validate that we are a reference source
	// Note: The link will be attempted even if an error is produced
	//		 This is intentional as it may be valid in a later file spec
	DictRefType RType = Type->GetRefType();
	if((RType != DICT_REF_STRONG) && (RType != DICT_REF_WEAK))
	{
		error("Attempting to reference %s from %s (which is not a reference source)\n",
			   FullName().c_str(), TargetSet->FullName().c_str());
	}

	// Make the link
	ReadValue(TheUID, 16);
	Link = TargetSet;

	return true;
}

//! Set an object to its distinguished value
/*! \return true if distinguished value set, else false */
bool MDObject::SetDValue(void)
{
	DictEntry const *Dict = Type->GetDict();

	if(!Dict) return false;

	if(!Dict->HasDValue) return false;

	SetModified(true);
	ReadValue(Dict->DValue, Dict->DValueLen);

	return true;
}


//! Is an object set to its distinguished value?
/*! \return true if distinguished value set, else false */
bool MDObject::IsDValue(void)
{
	DictEntry const *Dict = Type->GetDict();

	if(!Dict) return false;
	if(!Dict->HasDValue) return false;

	DataChunk DVal = PutData();
	if(DVal.Size != Dict->DValueLen) return false;
	
	if(memcmp(DVal.Data, Dict->DValue, DVal.Size) == 0) return true;

	return false;
}


//! Make a copy of this object
MDObjectPtr MDObject::MakeCopy(void)
{
	MDObjectPtr Ret = new MDObject(Type);

	MDObjectNamedList::iterator it = begin();
	while(it != end())
	{
		Ret->insert((*it).second->MakeCopy());
		it++;
	}

	if(Value)
	{
		Ret->Value = new MDValue(Value->GetType());
		Ret->Value->ReadValue(Value->PutData());
	}

	// Somewhat dangerous!!
	if(Link) 
	{
		Ret->Link = Link;
		if(GetRefType() == DICT_REF_STRONG)
		{
			warning("Copy made of %s which is a StrongRef!\n", FullName().c_str()); 
		}
	}

	// Copy any properties that are safe to copy
	Ret->TheUL = TheUL;
	Ret->TheTag = TheTag;

	SetModified(true);

	return Ret;
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

