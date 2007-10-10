/*! \file	mdobject.cpp
 *	\brief	Implementation of classes that define metadata objects
 *
 *			Class MDObject holds info about a specific metadata object
 *<br><br>
 *			Class MDOType holds the definition of MDObjects derived from
 *			the XML dictionary.
 *
 *	\version $Id: mdobject.cpp,v 1.25 2007/10/10 15:45:35 matt-beard Exp $
 *
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

#include <mxflib/mxflib.h>

#include <stdarg.h>

using namespace mxflib;


//! Static flag to say if dark metadata sets that appear to be valid KLV 2x2 sets should be parsed
bool MDObject::ParseDark = false;

//! Static primer to use for index tables
PrimerPtr MDOType::StaticPrimer;

//! List of all existing symbol spaces to allow full searching
SymbolSpaceMap SymbolSpace::AllSymbolSpaces;

//! Global SymbolSpace for all MXFLib's normal symbols
SymbolSpacePtr mxflib::MXFLibSymbols = new SymbolSpace("http://www.freemxf.org/MXFLibSymbols");

//! Translator function to translate unknown ULs to object names
MDObject::ULTranslator MDObject::UL2NameFunc = NULL;


//! Build a Primer object for the current dictionary
/*! This primer has the mappings of tag to UL from the dictionary
 *  \param SetStatic - If true the StaticPrimer will be set to this new primer
 */
PrimerPtr MDOType::MakePrimer(bool SetStatic /*=false*/)
{
	PrimerPtr Ret = new Primer;

	// DRAGONS: Some debug!
	MDOTypeList::iterator it = AllTypes.begin();
	while(it != AllTypes.end())
	{
		if((*it)->Key.Size == 2)
		{
			Tag ThisTag = (*it)->Key.Data[1] + ((*it)->Key.Data[0] << 8);

			// Don't barf if the dictionary entry is invalid!
			if((*it)->GlobalKey.Size != 16)
			{
				error("Dictionary entry for \"%s\" has a 2-byte tag, but doesn't have a 16-byte UL\n", (*it)->FullName().c_str());
			}
			else
			{
				mxflib::UL ThisUL((*it)->GlobalKey.Data);
				Ret->insert(Primer::value_type(ThisTag, ThisUL));
			}
		}

		it++;
	}

	// Replace existing StaticPrimer if requested
	if(SetStatic) StaticPrimer = Ret;

	return Ret;
}


//! Builds an MDOType
/*! This constructor is private so the ONLY way to create
 *	new MDOTypes from outside this class is via member methods
*/
MDOType::MDOType(void)
{
	// Initialise dictionary data
	KeyFormat = DICT_KEY_NONE;
	LenFormat = DICT_LEN_BER;
	minLength = 0;
	maxLength = (unsigned int)-1;
	Use = DICT_USE_NONE;
	RefType = DICT_REF_NONE;

	// Initially assume not a container
	ContainerType = NONE;
}


//! Find the MDOType object that defines a named type
/*! \return Pointer to the object
 *  \return NULL if there is no type of that name
 *	\note If BaseType contains a qualified name of the format "symbolspace::name" then only 
 *        the specified symbolspace is searched
 */
MDOTypePtr MDOType::Find(std::string BaseType, SymbolSpacePtr &SymSpace, bool SearchAll /*=false*/)
{
	// Check for a symbol space given in the name
	size_type Pos = BaseType.find("::");

	if(Pos != std::string::npos)
	{
		SymbolSpacePtr Sym;

		// DRAGONS: A zero length namespace represents the default namespace
		if(Pos == 0) Sym = MXFLibSymbols;
		else Sym = SymbolSpace::FindSymbolSpace(BaseType.substr(0, Pos));

		if(Sym)
		{
			ULPtr ThisUL = Sym->Find(BaseType.substr(Pos+2), false);
			if(ThisUL) return MDOType::Find(ThisUL);
		}
	}
	else
	{
		ULPtr ThisUL = SymSpace->Find(BaseType, SearchAll);
		if(ThisUL) return MDOType::Find(ThisUL);
	}

	return NULL;
}


//! Find the MDOType object that defines a type with a specified UL
/*! \return Pointer to the object
 *  \return NULL if there is no type with that UL
 */
MDOTypePtr MDOType::Find(const UL& BaseUL)
{
	MDOTypePtr theType;

	std::map<UL, MDOTypePtr>::iterator it = ULLookup.find(BaseUL);

	if(it != ULLookup.end())
	{
		theType = (*it).second;
	}
	else
	{
		// If the exact match is not found try a version-less lookup by changing the version number to 1
		if((BaseUL.GetValue()[0] == 0x06) && (BaseUL.GetValue()[1] == 0x0e) && (BaseUL.GetValue()[2] == 0x2b) && (BaseUL.GetValue()[3] == 0x34))
		{
			UL Ver1UL = BaseUL;
			Ver1UL.Set(7, 1);

			std::map<UL, MDOTypePtr>::iterator it2 = ULLookupVer1.find(Ver1UL);
			if(it2 != ULLookupVer1.end())
			{
				theType = (*it2).second;
			}
		}
	}

	return theType;
}


//! Find the MDOType object that defines a type with a specified Tag
/*! The tag is looked up in the supplied primer
 *	\note if BasePrimer is NULL then the static primer is searched
 *  \return Pointer to the object
 *  \return NULL if there is no type with that tag
 */
MDOTypePtr MDOType::Find(Tag BaseTag, PrimerPtr BasePrimer)
{
	MDOTypePtr theType;

	// Search the static primer by default
	if(!BasePrimer) BasePrimer = GetStaticPrimer();

	// Search the primer
	Primer::iterator it = BasePrimer->find(BaseTag);

	// Return NULL if not in the primer
	if(it == BasePrimer->end()) return NULL;

	// Now search on the located UL
	return MDOType::Find((*it).second);
}



//! MDObject named constructor
/*! Builds a "blank" metadata object of a named type
 *	\note packs are built with defaut values
 */
MDObject::MDObject(std::string BaseType, SymbolSpacePtr &SymSpace /*=MXFLibSymbols*/ )
{
	ULPtr ThisUL = SymSpace->Find(BaseType);
	if(ThisUL) Type = MDOType::Find(ThisUL);
	if(Type)
	{
		ObjectName = Type->Name();
	}
	else
	{
		error("Metadata object type \"%s\" doesn't exist\n", BaseType.c_str());

		Type = MDOType::Find("Unknown");

		ASSERT(Type);

		ObjectName = "Unknown " + BaseType;
	}

	IsConstructed = true;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;
	TheUL = Type->GetUL();
	TheTag = 0;

	Outer = NULL;

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

	Outer = NULL;

	// Initialise the new object
	Init();
}


//! MDObject UL based constructor body
/*! Builds a "blank" metadata object of a specified type
 *	\note packs are built with default values
 *
 *  \note TheUL must be set before calling
 */
void MDObject::ULCtor(void) 
{
	Type = MDOType::Find(TheUL);
	if(Type)
	{
		ObjectName = Type->Name();
	}
	else
	{
		Type = MDOType::Find("Unknown");

		if(UL2NameFunc)
		{
			ObjectName = UL2NameFunc(TheUL,NULL);
		}
		else
		{
			// TODO: Needs to have a more complete name
			ObjectName = "Unknown " + TheUL->GetString();
		}

		ASSERT(Type);

		// Shall we try and parse this?
		if(ParseDark)
		{
			const UInt8 Set2x2[6] = { 0x06, 0x0E, 0x2B, 0x34, 0x02, 0x53 };
			if(memcmp(Set2x2, TheUL->GetValue(), 6) == 0)
			{
				MDOTypePtr DefaultType = MDOType::Find("DefaultObject");
				if(DefaultType) Type = DefaultType;
			}
		}
	}

	IsConstructed = true;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;
	TheTag = 0;

	Outer = NULL;

	// Initialise the new object
	Init();
}


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
			if(Type) TheUL = Type->GetTypeUL();
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
		if(Type) TheUL = Type->GetTypeUL();
	}

	// If it was unknown build an "Unknown" and set a meaningful name
	if(!Type)
	{
		Type = MDOType::Find("Unknown");

		ASSERT(Type);

		if(UL2NameFunc)
		{
			ObjectName = UL2NameFunc(TheUL, &BaseTag);
		}
		else
		{
			if(TheUL)
			{
				// Tag found, but UL unknown
				// FIXME: Needs to have a more complete name
				ObjectName = "Unknown " + Tag2String(BaseTag) + std::string(" ") + TheUL->GetString();
			}
			else
			{
				// Tag not found, build a blank UL
				// FIXME: Needs to have a more complete name
				ObjectName = "Unknown " + Tag2String(BaseTag);

				// We will need a UL, so try just using the generic "Unknown" UL
				TheUL = Type->GetTypeUL();
			}
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

	Outer = NULL;

	// Initialise the new object
	Init();
}


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
			// If we are trying to build a type that is not defined, build it as an "Unknown"
			MDTypePtr ValType = Type->GetValueType();
			if(!ValType) ValType = MDType::Find("UnknownType");
			ASSERT(ValType);

			Value = new MDValue(ValType);

			if(ValType->EffectiveClass() == TYPEARRAY)
			{
				// Build the minimum size array
				Value->Resize(Type->GetMinLength());
			}
		}

		break;

	case PACK:
		// If it's a pack build all items
		{
			Value = NULL;

			MDOTypeList::const_iterator it = Type->GetChildList().begin();
			while(it != Type->GetChildList().end())
			{
				MDObjectPtr NewItem = new MDObject(*it);
				NewItem->SetDefault();
				insert(NewItem);

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
}


#if 0 // WORK IN PROGRESS...
//! Add an empty numbered child to an MDObject continer and return a pointer to it
/*!	\return NULL if this object does not take numbered children
 *	\note If a child with this index already exists it is returned rather than adding a new one
 */
MDObjectPtr MDObject::AddChild(UInt32 ChildIndex)
{
	MDObjectPtr Ret;

	SetModified(true); 

	// Try and find an existing child (if replacing)
	Ret = Child(ChildIndex);

	// Only add a new one if we didn't find it
	if(!Ret)
	{
		// Find the child definition
		MDOType::iterator it = Type->begin();

		// Return NULL if not found
		if(it == Type->end()) return Ret;

		// Build a new item of the correct type
		Ret = new MDObject((*it).second);

		// Add the new child
		AddChildInternal(Ret, false);
	}

	// Return smart pointer to the new object
	return Ret;
}
#endif // 0


//! Add an empty named child to an MDObject continer and return a pointer to it
/*! If Replace is true (or not supplied) and a child of this name already 
 *	exists a pointer to that child is returned but the value is not changed.
 *  ChildName is a symbol to be located in the given SymbolSpace - if no SymbolSpace is specifed the default MXFLib space is used 
 *  \return NULL if it is not a valid child to add to this type of container
 *	\note If you want to add an child that is non-standard (i.e. not listed
 *        as a child in the container's MDOType then you must build the child
 *        then add it with AddChild(MDObjectPtr)
 */
MDObjectPtr MDObject::AddChild(std::string ChildName, SymbolSpacePtr &SymSpace /*=MXFLibSymbols*/, bool Replace /*=true*/)
{
	MDObjectPtr Ret;

	SetModified(true); 

	// Try and find an existing child (if replacing)
	if(Replace) Ret = Child(ChildName); else Ret = NULL;

	// Only add a new one if we didn't find it
	if(!Ret)
	{
		// Find the child definition
		MDOTypePtr ChildType = Type->Child(ChildName);

		// If not a known child of this type try and add the named item anyway
		if(!ChildType)
		{
			ULPtr ChildUL = SymSpace->Find(ChildName, false);

			// If not a known name return NULL
			if(!ChildUL) return Ret;

			// Insert a new item of the correct type
			Ret = new MDObject(ChildUL);
		}
		else
		{
			// Insert a new item of the correct type
			Ret = new MDObject(ChildType);
		}

		if(Ret) insert(Ret);
	}

	// Return smart pointer to the new object
	return Ret;
}


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
		// Insert a new item of the correct type
		Ret = new MDObject(ChildType);
		insert(Ret);
	}

	// Return smart pointer to the new object
	return Ret;
}


//! Add a new child MDObject of the specified type
/*! DRAGONS: This code searches for child entries that match rather than simply doing a full UL-lookup as there
 *           may be cases where the same UL is used somewhere else but is not compatible (such as in a pack definition
 *           whereas are adding to a local set to need a valid local tag)
 */
MDObjectPtr MDObject::AddChild(const UL &ChildType, bool Replace /*=true*/)
{
	MDObjectPtr Ret;

	SetModified(true); 

	// Try and find an existing child (if replacing)
	if(Replace) Ret = Child(ChildType); else Ret = NULL;

	// Only add a new one if we didn't find it
	if(!Ret)
	{
		// Find the child definition
		MDOType::iterator it = Type->begin();
		while(it != Type->end())
		{
			if(((*it).second->GetUL()) && (*((*it).second->GetUL()) == ChildType)) break;
			it++;
		}

		// If not a known child of this type try and add the specified item anyway
		if(it == Type->end())
		{
			// Insert a new item of the correct type
			Ret = new MDObject(ChildType);
		}
		else
		{
			// Insert a new item of the correct type
			Ret = new MDObject((*it).second);
		}

		if(Ret) insert(Ret);
	}

	// Return smart pointer to the new object
	return Ret;
}
		
//! Add a new child MDObject to a vector
/*! \note The type of the object added is automatic. 
 *        If the vector is of multiple members the next type will be chosen by the number of members currently
 *        in the array, so if there are 3 sub member types the 7th entry will be type 1 [ 7 = (2*3) + 1 ]
 *
 *  \note This version of AddChild will <b>not</b> replace duplicates, it always appends
 */
MDObjectPtr MDObject::AddChild(void)
{
	// Number of sub items types declares
	int SubCount = (int)Type->size();

	if(!SubCount)
	{
		error("Attempted to AddChild() to %s which has no child types declared\n", FullName().c_str());
		return NULL;
	}

	// Number sub items currently existing
	int ItemCount = (int)size();

	// The sub-type number (zero based) for the next item to add
	int ItemType = ItemCount % SubCount;

	// Get an iterator that points at the required type
	MDOType::iterator it = Type->begin();
	while(ItemType--) it++;

	// Add a child of this type
	return AddChild((*it).second, false);
}

//! Add a given MDObject to an MDObject continer
/*! \return pointer to the object added (for compatibility with other ADDChild funcs)
 *  \return NULL if there was an error
 *  \note If there is already a child of this type it is replaces if Replace = true
 */
MDObjectPtr MDObject::AddChild(MDObjectPtr &ChildObject, bool Replace /* = false */)
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

	MDObjectULList::iterator it = begin();
	while(it != end())
	{
		if((*it).second->Name() == ChildName)
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
void MDObject::RemoveChild(MDOTypePtr &ChildType)
{
	// Note that we cannot rely on removing by name as names are changeable
	
	SetModified(true); 

	MDObjectULList::iterator it = begin();
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

	MDObjectULList::iterator it = begin();
	while(it != end())
	{
		if((*it).second == ChildObject)
		{
			erase(it);
			return;
		}
		it++;
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
	MDObjectULList::iterator it = begin();
	while(it != end())
	{
		if((*it).second->Name() == ChildName)
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
	MDObjectULList::iterator it = begin();
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


//! Access sub-item of the specified type within a compound MDObject
/*! If the child does not exist in this item then NULL is returned
 *  even if it is a valid child to have in this type of container
 *
 *  DRAGONS: This doesn't work well with SmartPtrs
 *           so member function Child() is also available
*/
MDObjectPtr MDObject::operator[](const UL &ChildType)
{
	MDObjectULList::iterator it = begin();
	while(it != end())
	{
		if((*it).first == ChildType)
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
	MDObjectULList::iterator it = begin();

	while(it != end())
	{
		if((*it).second->Name() == ChildName)
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
	MDObjectULList::iterator it = begin();

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


//! Find all sub-items within a compound MDObject of a specified type
MDObjectListPtr MDObject::ChildList(const UL &ChildType)
{
	MDObjectListPtr Ret = new MDObjectList;
	MDObjectULList::iterator it = begin();

	while(it != end())
	{
		if((*it).first == ChildType)
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
size_t MDObject::ReadValue(const UInt8 *Buffer, size_t Size, PrimerPtr UsePrimer /*=NULL*/)
{
	size_t Bytes = 0;
	size_t Count = 0;
	UInt32 ItemSize = 0;

	SetModified(false); 

	switch(Type->GetContainerType())
	{
	case NONE:
		return Value->ReadValue(Buffer, Size);

	case BATCH:
		{
			if( Size >= 4 )
			{
				Count = GetU32(Buffer);
				Buffer += 4;
				Bytes = 4;
				Size -= 4;
			}
			else
			{
				error("Malformed batch found in %s at 0x%s in %s - total bytes = %u\n", 
					  FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str(),
					  Size);

				Count = 0;
				Bytes = Size;
				Size = 0;
			}

			if( Size >= 4 )
			{
				ItemSize = GetU32(Buffer);
				Buffer += 4;
				Bytes += 4;
				Size -= 4;
			}
			else
			{
				error("Malformed batch found in %s at 0x%s in %s - total bytes = %u\n", 
					  FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str(),
					  Bytes + Size);

				ItemSize = 0;
				Bytes += Size;
				Size = 0;
			}

			if((ItemSize*Count) != Size)
			{
				error("Malformed batch found in %s at 0x%s in %s - item size = %u, count = %u, but bytes = %u\n", 
					  FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str(),
					  ItemSize, Count, Size);

				// Prevent us reading off the end of the buffer
				if(Size < (ItemSize*Count))	Count = Size / ItemSize;
			}

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
			MDOTypeList::const_iterator it = Type->GetChildList().begin();

			size_t ChildCount = Type->size();
			while(Size || Count)
			{
				MDObjectPtr NewItem = new MDObject(*it);

				ASSERT(NewItem);

				NewItem->Parent = this;
				NewItem->ParentOffset = Bytes;
				NewItem->KLSize = 0;

				size_t ThisBytes = NewItem->ReadValue(Buffer, Size);

				Bytes += ThisBytes;
				Buffer += ThisBytes;
				if(ThisBytes > Size) Size = 0; else Size -= ThisBytes;
				insert(NewItem);


				bool ItemStart = true;

				// If this array has multiple children, get the next type
				if(ChildCount > 1)
				{
					it++;
					
					if(it == Type->GetChildList().end()) it = Type->GetChildList().begin(); 
					else ItemStart = false;
				}

				// If processing a batch, set up for the next item
				if(ItemStart && (Count != 0)) 
				{
					if(--Count) Size = ItemSize; else break;
				}
			}

			if((ChildCount > 1) && (it != Type->GetChildList().begin()))
			{
				error("Multiple %s at 0x%s in %s does not contain an integer number of sub-items\n", 
					  FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str());
			}

			return Bytes;
		}

	case PACK:
		{
			debug("Reading pack at 0x%s\n", Int64toHexString(GetLocation(), 8).c_str());

			if( Type->GetLenFormat() == DICT_LEN_NONE )
			{
				// Fixed pack - items know their own length
				MDObjectULList::iterator it = begin();
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

					size_t ValueSize = Size;
					if((*it).second->Type->GetContainerType() == ARRAY)
					{
						// FIXME: Shouldn't we do UL lookups here?
						std::string FullName = (*it).second->FullName();
						if(FullName == "IndexTableSegment/IndexEntryArray/SliceOffsetArray")
						{
							// DRAGONS: Does this ever get called these days?
							// Number of entries in SliceOffsetArray is in IndexTableSegment/SliceCount
							// Each entry is 4 bytes long
							ValueSize = Parent->GetInt(SliceCount_UL) * 4;
						}
						else if(FullName == "RandomIndexMetadata/PartitionArray")
						{
							// RandomIndexMetadata/PartitionArray is followed by a UInt32
							if(ValueSize >4) ValueSize = ValueSize - 4; else ValueSize = 0;
						}
					}

					size_t ThisBytes = (*it).second->ReadValue(Buffer, ValueSize);

//					debug("  at 0x%s Pack item %s = %s\n", Int64toHexString((*it).second->GetLocation(), 8).c_str(), 
//						  (*it).first.c_str(), (*it).second->GetString().c_str());

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
			else
			{
				// Variable pack - each item has a length
				MDObjectULList::iterator it = begin();
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

					// Read Length
					Length Length;
					size_t ThisBytes = ReadLength(Type->GetLenFormat(), Size, Buffer, Length);

					// Advance counters and pointers past Length
					Size -= ThisBytes;
					Buffer += ThisBytes;
					Bytes += ThisBytes;

					// Sanity check the length of this value
					if((sizeof(size_t) < 8) && (Length > 0xffffffff))
					{
						error("Tried to read %s at 0x%s in %s which is > 4GBytes, but this platform can only handle <= 4GByte chunks\n", FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str());
						return 0;
					}

					if(Length)
					{
						if(Size < static_cast<size_t>(Length))
						{
							error("Not enough bytes for value for %s at 0x%s in %s\n", 
								  FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str());

							// Read what we can!
							Length = Size;
						}

						ThisBytes = (*it).second->ReadValue(Buffer, static_cast<size_t>(Length));

//						debug("  at 0x%s Variable Pack item %s = %s\n", Int64toHexString((*it).second->GetLocation(), 8).c_str(), 
//							  (*it).first.c_str(), (*it).second->GetString().c_str());

						Bytes += ThisBytes;
					}
					else
					{
						// Length == 0, so skip this item
						//(*it).second->ReadValue(Buffer, 0);
						(*it).second->ClearModified();
						ThisBytes = 0;
					}

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
		}

	case SET:
		{
			debug("Reading set at 0x%s\n", Int64toHexString(GetLocation(), 8).c_str());

			// Start with an empty list
			clear();

			// Scan until out of data
			while(Size)
			{
				size_t BytesAtItemStart = Bytes;

				DataChunk Key;
				size_t ThisBytes = ReadKey(Type->GetKeyFormat(), Size, Buffer, Key);

				// Abort if we can't read the key
				// this prevents us looping for ever if we
				// come across invalid data
				if(ThisBytes == 0) break;

				// Advance counters and pointers passed key
				Size -= ThisBytes;
				Buffer += ThisBytes;
				Bytes += ThisBytes;

				Length Length;
				ThisBytes = ReadLength(Type->GetLenFormat(), Size, Buffer, Length);

				// Advance counters and pointers passed Length
				Size -= ThisBytes;
				Buffer += ThisBytes;
				Bytes += ThisBytes;

				// Sanity check the length of this value
				if((sizeof(size_t) < 8) && (Length > 0xffffffff))
				{
					error("Tried to read %s at 0x%s in %s which is > 4GBytes, but this platform can only handle <= 4GByte chunks\n", FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str());
					return 0;
				}

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
					if(Type->GetKeyFormat() == DICT_KEY_2_BYTE)
					{
						ASSERT(Key.Size == 2);
						Tag ThisKey = GetU16(Key.Data);

						NewItem = new MDObject(ThisKey, UsePrimer);
					}
					else if(Type->GetKeyFormat() == DICT_KEY_AUTO)
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
					NewItem->KLSize = static_cast<UInt32>(Bytes - BytesAtItemStart);

					// Handle cases where a batch has burst the 2-byte length (non-standard)
					if((Length == 0xffff) && (Type->GetLenFormat() == DICT_LEN_2_BYTE) && (NewItem->Type->GetContainerType() == BATCH))
					{
						ThisBytes = NewItem->ReadValue(Buffer, Size);
					}
					else
					{
						ThisBytes = NewItem->ReadValue(Buffer, static_cast<size_t>(Length));

//						debug("  at 0x%s Set item (%s) %s = %s\n", Int64toHexString(NewItem->GetLocation(), 8).c_str(), Key.GetString().c_str(), NewItem->Name().c_str(), NewItem->GetString().c_str());

						if(ThisBytes != Length)
						{
							error("Failed to read complete %s value at 0x%s in %s - specified length=%d, read=%d\n", 
								NewItem->FullName().c_str(), Int64toHexString(NewItem->GetLocation(), 8).c_str(), 
								NewItem->GetSource().c_str(), Length, ThisBytes);
							
							// Skip anything left over
							if(Length > ThisBytes) ThisBytes = static_cast<size_t>(Length);
						}
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
		MDObjectULList::iterator it = begin();

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
		MDObjectULList::iterator it = begin();

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

	// Find (or add) the GenerationUID property
	MDObjectPtr GenUID = Child("GenerationUID");
	if(!GenUID)
	{
		// If we don't currenly have a GenerationUID, forst check if this is because we shouldn't have one
		if(!IsA(GenerationInterchangeObject_UL)) return false;

		// Otherwise go ahead and add one
		GenUID = AddChild(GenerationUID_UL);
	}

	ASSERT(GenUID);

	// Set the actual UID
	GenUID->Value->ReadValue(NewGen->GetValue(), NewGen->Size());

	return true;
}


//! Read a key from a memory buffer
UInt32 MDObject::ReadKey(DictKeyFormat Format, size_t Size, const UInt8 *Buffer, DataChunk& Key)
{
	UInt32 KeySize;

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
UInt32 MDObject::ReadLength(DictLenFormat Format, size_t Size, const UInt8 *Buffer, Length& Length)
{
//	int LenSize;

	switch(Format)
	{
	default:
	// Unsupported key types!
	case DICT_LEN_NONE:
		ASSERT(0);							// Cause debug sessions to show this error
		Length = 0;
		return 0;

	case DICT_LEN_BER:
		{
			if( Size <1 ) break;
			UInt8 LenLen = GetU8(Buffer);

			if( LenLen < 0x80 )
			{
				// Short form
				Length = LenLen;
				return 1;
			}
			else
			{
				// Long form
				LenLen &= 0x7f;

				UInt32 RetLen = LenLen + 1;
				if( RetLen > Size) break;

				// DRAGONS: ReadLength should return UInt64, BER length up to 7
				if(LenLen > 8)
				{
					error("Excessive BER length field in MDObject::ReadLength()\n");
					Length = 0;
					return 0;
				}

				if(LenLen > 4)
				{
					// It is valid to have BER length > 4 bytes long however we don't
					// support metadata values > 4Gb in size so we ensure they are
					// not that big by reading the length and validating

					UInt64 Length64 = 0;
					const UInt8* LenBuff = (Buffer+1);
					while(LenLen--) Length64 = (Length64<<8) + *LenBuff++;

					if((Length64 >> 32) != 0)
					{
						error("Excessive BER length field in MDObject::ReadLength() - Metadata objects are limited to 4Gb each\n");
						Length = 0;
						return 0;
					}
					
					Length =(UInt32) Length64;
				}
				else
				{
					// Handle sane sized BER lengths
					Length = 0;
					const UInt8* LenBuff = (Buffer+1);
					while(LenLen--) Length = (Length<<8) + *LenBuff++;
				}

				return RetLen;
			}
		}

	case DICT_LEN_1_BYTE:		
		{ 
			if(Size >= 1) 
			{ 
				Length = GetU8(Buffer); 
				return 1;
			}

			// Else we drop through to error handler
			break;
		}

	case DICT_LEN_2_BYTE://		{ LenSize = 2;  Length = GetU16(Buffer); };
		{ 
			if(Size >= 2) 
			{ 
				Length = GetU16(Buffer); 
				return 2;
			}

			// Else we drop through to error handler
			break;
		}

	case DICT_LEN_4_BYTE://		{ LenSize = 4;  Length = GetU32(Buffer); };
		{ 
			if(Size >= 4) 
			{ 
				Length = GetU32(Buffer); 
				return 4;
			}

			// Else we drop through to error handler
			break;
		}
	}

	error("Not enough bytes for required length field in MDObject::ReadLength()\n");
	Length = 0;
	return 0;
}


//! Get the location within the ultimate parent
UInt64 mxflib::MDObject::GetLocation(void)
{
	UInt64 Ret = ParentOffset;

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


//! Build a data chunk with all this item's data (including child data)
const DataChunkPtr MDObject::PutData(PrimerPtr UsePrimer /* =NULL */) 
{ 
	if(Value) return Value->PutData(); 
	
	// DRAGONS: Pre-allocating a buffer could speed things up
	DataChunkPtr Ret = new DataChunk; 

	MDObjectULList::iterator it = begin();

	while(it != end())
	{
		(*it).second->WriteObject(Ret, this, UsePrimer);
		it++;
	}

	return Ret;
}


//! Write this object, and any strongly linked sub-objects, to a memory buffer
/*! The object must be at the outer or top KLV level. 
 *	The objects are appended to the buffer
 *	\return The number of bytes written
 */
size_t MDObject::WriteLinkedObjects(DataChunkPtr &Buffer, PrimerPtr UsePrimer /*=NULL*/)
{
	size_t Bytes = 0;

	Bytes = WriteObject(Buffer, NULL, UsePrimer);

	MDObjectULList::iterator it = begin();
	while(it != end())
	{
		if((*it).second->Link)
		{
			if((*it).second->GetRefType() == DICT_REF_STRONG) Bytes += (*it).second->Link->WriteLinkedObjects(Buffer, UsePrimer);
		}
		else if(!((*it).second->empty()))
		{
			MDObjectULList::iterator it2 = (*it).second->begin();
			MDObjectULList::iterator itend2 = (*it).second->end();
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
#define DEBUG_WRITEOBJECT(x)
//#define DEBUG_WRITEOBJECT(x) x
size_t MDObject::WriteObject(DataChunkPtr &Buffer, MDObjectPtr ParentObject, PrimerPtr UsePrimer /*=NULL*/, UInt32 BERSize /*=0*/)
{
	size_t Bytes = 0;

	DictLenFormat LenFormat;

	DEBUG_WRITEOBJECT( debug("WriteObject(%s) ", FullName().c_str()); )
	
	// Write the key (and determine the length format)
	if(!ParentObject)
	{
		DEBUG_WRITEOBJECT( debug("no parent\n"); )
		Bytes += WriteKey(Buffer, DICT_KEY_AUTO, UsePrimer);
		LenFormat = DICT_LEN_BER;
	}
	else
	{
		DEBUG_WRITEOBJECT( debug("Parent %s, ", ParentObject->FullName().c_str()); )
		
		// Only sets need keys
		if(ParentObject->Type->GetContainerType() == SET)
		{
			Bytes = WriteKey(Buffer, ParentObject->Type->GetKeyFormat(), UsePrimer);

			DEBUG_WRITEOBJECT( DataChunk Key; Key.Set(Buffer, Buffer->Size - Bytes); )
			DEBUG_WRITEOBJECT( debug("Key = %s, ", Key.GetString().c_str()); )
		}

		if((ParentObject->Type->GetContainerType() == BATCH) || (ParentObject->Type->GetContainerType() == ARRAY))
		{
			LenFormat = DICT_LEN_NONE;
		}
		else
		{
			LenFormat = ParentObject->Type->GetLenFormat();
		}

		DEBUG_WRITEOBJECT( debug("ParentObject->ContainerType = %d, ", ParentObject->Type->GetContainerType()); )
		DEBUG_WRITEOBJECT( if(LenFormat == DICT_LEN_BER) debug("Length = BER\n"); )
		DEBUG_WRITEOBJECT( else debug("Length = %d-byte\n", (int)LenFormat); )
	}

	// The rest depends on the container type
	MDContainerType CType = Type->GetContainerType();

	// Build value
	if(CType == BATCH || CType == ARRAY)
	{
		UInt32 Count = 0;
		UInt32 Size = 0;

		// DRAGONS: Pre-allocating a buffer could speed things up
		DataChunkPtr Val = new DataChunk();

		// Work out how many sub-items per child 
		// DRAGONS: We assume < 4 billion
		UInt32 SubCount = static_cast<UInt32>(Type->GetChildOrder().size());
		
		// Count of remaining subs for this item
		UInt32 Subs = 0;

		MDObjectULList::iterator it = begin();
		while(it != end())
		{
			// Start of an item
			if(Subs == 0)
			{
				Subs = SubCount;
				Size = 0;
				Count++;
			}
			// DRAGONS: do NOT force embedded objects to inherit BERSize
			UInt32 ThisBytes = static_cast<UInt32>((*it).second->WriteObject(Val, this, UsePrimer));
			//Bytes += ThisBytes;
			Size += ThisBytes;
			
			Subs--;
			it++;
		}

		// Determine item size if batch is empty
		// May not be strictly required, but 0 items of 0 size is a little dubious
		if(Count == 0)
		{
			DataChunkPtr Temp = new DataChunk();

			MDOTypeList::const_iterator it = Type->GetChildList().begin();
			while(it != Type->GetChildList().end())
			{
				MDObjectPtr Ptr = new MDObject(*it);
				Ptr->WriteObject(Temp, this, UsePrimer, BERSize);
				it++;
			}
			Size = static_cast<UInt32>(Temp->Size);
		}

		if(CType == BATCH)
		{
			// Write the length and batch header
			Bytes += WriteLength(Buffer, Val->Size+8, LenFormat, BERSize);
			UInt8 Buff[4];
			PutU32(Count, Buff);
			Buffer->Append(4, Buff);
			PutU32(Size, Buff);
			Buffer->Append(4, Buff);
			Bytes += 8;
		}
		else
		{
			Bytes += WriteLength(Buffer, Val->Size, LenFormat, BERSize);
		}

		// Append this data
		Buffer->Append(Val);
		Bytes += Val->Size;
		
		DEBUG_WRITEOBJECT( debug("  > %s\n", Val->GetString().c_str()); )
	}
	else if(CType == PACK)
	{
		DEBUG_WRITEOBJECT( debug("  *PACK*\n"); )

		// DRAGONS: Pre-allocating a buffer could speed things up
		DataChunkPtr Val = new DataChunk;

		// Ensure we write the pack out in order
		StringList::const_iterator it = Type->GetChildOrder().begin();

		while(it != Type->GetChildOrder().end())
		{
			MDObjectPtr Ptr = Child(*it);
			if(!Ptr)
			{
				error("Pack %s is missing sub-item %s\n", FullName().c_str(), (*it).c_str());
			}
			else
			{
				Bytes += Ptr->WriteObject(Val, this, UsePrimer, BERSize);
			}
			it++;
		}

		// Write the length of the value
		// DRAGONS: do NOT force embedded objects to inherit BERSize
		Bytes += WriteLength(Buffer, Val->Size, LenFormat);

		// Append this data
		Buffer->Append(Val);
		Bytes += Val->Size;
		
		DEBUG_WRITEOBJECT( debug("  > %s\n", Val->GetString().c_str()); )
	}
	else if(!empty())
	{
		DEBUG_WRITEOBJECT( debug("  *Not Empty*\n"); )

		// DRAGONS: Pre-allocating a buffer could speed things up
		DataChunkPtr Val = new DataChunk;

		MDObjectULList::iterator it = begin();
		while(it != end())
		{
			// DRAGONS: do NOT force embedded objects to inherit BERSize
			// don't double-count the bytes! 
			(*it).second->WriteObject(Val, this, UsePrimer);
			it++;
		}

		// Write the length of the value
		Bytes += WriteLength(Buffer, Val->Size, LenFormat, BERSize);

		// Append this data
		Buffer->Append(Val);
		Bytes += Val->Size;

		DEBUG_WRITEOBJECT( debug("  > %s\n", Val->GetString().c_str()); )
	}
	else if(Value)
	{
		DEBUG_WRITEOBJECT( debug("  *Value*\n"); )

		DataChunkPtr Val = Value->PutData();
		Bytes += WriteLength(Buffer, Val->Size, LenFormat, BERSize);
		Buffer->Append(Val);
		Bytes += Val->Size;

		DEBUG_WRITEOBJECT( debug("  > %s\n", Val->GetString().c_str()); )
	}
	else
	{
		DEBUG_WRITEOBJECT( debug("  *Empty!*\n"); )

		Bytes += WriteLength(Buffer, 0, LenFormat, BERSize);
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
UInt32 MDObject::WriteLength(DataChunkPtr &Buffer, Length Length, DictLenFormat Format, UInt32 Size /*=0*/)
{
	switch(Format)
	{
	default:
	case DICT_LEN_NONE:
		return 0;

	case DICT_LEN_BER:
		{
			DataChunkPtr BER = MakeBER(Length, Size);
			Buffer->Append(*BER);
			return static_cast<UInt32>(BER->Size);
		}

	case DICT_LEN_1_BYTE:
		{ 
			UInt8 Buff;
			UInt8 Len8 = Length <= 0xff ? (UInt8)Length : 0xff;
			PutU8(Len8, &Buff);

			Buffer->Append(1, &Buff);
			return 1;
		}

	case DICT_LEN_2_BYTE:
		{ 
			UInt8 Buff[2];
			UInt16 Len16 = Length <= 0xffff ? (UInt16)Length : 0xffff;
			PutU16(Len16, Buff);

			Buffer->Append(2, Buff);
			return 2;
		}

	case DICT_LEN_4_BYTE:
		{ 
			UInt8 Buff[4];
			UInt32 Len32 = Length <= 0xffffffff ? (UInt32)Length : 0xffffffff;
			PutU32(Len32, Buff);

			Buffer->Append(4, Buff);
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
UInt32 MDObject::WriteKey(DataChunkPtr &Buffer, DictKeyFormat Format, PrimerPtr UsePrimer /*=NULL*/)
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

			Buffer->Append(16, TheUL->GetValue());
			return 16;
		}

	case DICT_KEY_2_BYTE:
		{ 
			if(!TheUL)
			{
				error("Call to WriteKey() for %s, but the UL is not known\n", FullName().c_str());
				return 0;
			}


			Tag UseTag;
			if(UsePrimer) UseTag = UsePrimer->Lookup(TheUL, TheTag);
			else UseTag = MDOType::GetStaticPrimer()->Lookup(TheUL, TheTag);

			UInt8 Buff[2];
			PutU16(UseTag, Buff);

			Buffer->Append(2, Buff);
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
 *  \param TargetSet A pointer to the set to be the target of this reference
 *  \param ForceLink True if the link is to be made even if not a valid reference source/target pair
 *	\return true on success, else false
 *	\note The link will be made from the source <b>property</b> to the target <b>set</b>
 *		  so be aware that "this" must be a reference source property and "TargetSet"
 *		  must be a set (or pack) containing an InstanceUID property which is a
 *		  reference target
 */
bool MDObject::MakeRef(MDObjectPtr &TargetSet, bool ForceLink /*=false*/)
{
	UInt8 TheUID[16];

	// Does the target set already have an InstanceUID?
	MDObjectPtr InstanceUID = TargetSet[InstanceUID_UL];

	// If not add one
	if(!InstanceUID)
	{
		InstanceUID = TargetSet->AddChild(InstanceUID_UL);

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
		DataChunkPtr Data = InstanceUID->Value->PutData();
		ASSERT(Data->Size == 16);
		memcpy(TheUID, Data->Data, 16);
	}

	// Validate that we are a reference source
	// Note: The link will be attempted even if an error is produced
	//		 This is intentional as it may be valid in a later file spec
	ClassRef RType = Type->GetRefType();
	if((RType != ClassRefStrong) && (RType != ClassRefWeak) && (RType != ClassRefGlobal))
	{
		if(!ForceLink)
		{
			error("Attempting to reference %s from %s (which is not a reference source)\n",
				   TargetSet->FullName().c_str(), FullName().c_str() );
		}
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
	if(Type->GetDValue().Size == 0) return false;

	SetModified(true);
	ReadValue(Type->GetDValue());

	return true;
}


//! Is an object set to its distinguished value?
/*! \return true if distinguished value set, else false */
bool MDObject::IsDValue(void)
{
	if(Type->GetDValue().Size == 0) return false;

	DataChunkPtr DVal = PutData();
	if(DVal->Size != Type->GetDValue().Size) return false;
	
	if(memcmp(DVal->Data, Type->GetDValue().Data, DVal->Size) == 0) return true;

	return false;
}


//! Make a copy of this object
MDObjectPtr MDObject::MakeCopy(void)
{
	MDObjectPtr Ret = new MDObject(Type);

	MDObjectULList::iterator it = begin();
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


//! Derive this new entry from a base entry
/*! \note It is important that DictName is set before calling
 *	\note Don't attempt to call this function on objects that are not freshly created
 */
void MDOType::Derive(MDOTypePtr &BaseEntry)
{
	// Default to using the base entry keys
	Key.Set(BaseEntry->Key);
	GlobalKey.Set(BaseEntry->GlobalKey);

	// Copy the base root name, note that we can't copy the name as this must be unique
	RootName = BaseEntry->RootName;

	// Copy the container type
	ContainerType = BaseEntry->ContainerType;

	// Copy the base detail
	Detail = BaseEntry->Detail;

	// Copy the type info
	ValueType = BaseEntry->ValueType;
	TypeName = BaseEntry->TypeName;

	//VectorType = BaseEntry->VectorType;
	
	// Copy the data relating to content format
	KeyFormat = BaseEntry->KeyFormat;
	LenFormat = BaseEntry->LenFormat;
	minLength = BaseEntry->minLength;
	maxLength = BaseEntry->maxLength;

	// Copy the usage
	Use = BaseEntry->Use;

	// Copy the default and the destinguished value
	Default.Set(BaseEntry->Default);
	DValue.Set(BaseEntry->DValue);

	// Copy the reference type
	RefType = BaseEntry->RefType;
	
	// Copy the parentage
	Parent = BaseEntry->Parent;

	// Tie to the base class
	Base = BaseEntry;
	
	// Remove any of our existing children
	clear();

	// Add children from base class
	MDOTypeList::iterator it = BaseEntry->ChildList.begin();
	while(it != BaseEntry->ChildList.end())
	{
		// Add the base types children
		insert(*it);

		NameLookup[RootName + DictName + "/" + (*it)->Name()] = *it;
		it++;
	}

	// Copy the base type's ref target setting
	RefTarget = Base->RefTarget;
	RefTargetName = Base->RefTargetName;
}


//! Re-Derive sub-items from a base entry
/*! Used when the base entry is being extended.
 *  We scan the list of sub items until a new sub-item is found, then insert any remaining items at
 *  that point (before any items that were added to the derived type and were not part of the base type)
 */
void MDOType::ReDerive(MDOTypePtr &BaseEntry)
{
	/* Build iterators that show the insertion point for child lists */
	MDOTypeList::iterator ChildListIt = ChildList.begin();
	std::list<std::string>::iterator ChildOrderIt = ChildOrder.begin();

	// Set true once we find new items that we need to insert
	bool Inserting = false;

	MDOTypeList::iterator it = BaseEntry->ChildList.begin();
	while(it != BaseEntry->ChildList.end())
	{
		if(!Inserting)
		{
			// If we are scanning and have have found a mismatch, this is the insertion point.
			if((*it) != (*ChildListIt)) Inserting = true;
		}

		if(Inserting)
		{
			Inserting = true;

            std::string NewName = (*it)->Name();
			std::pair<iterator, bool> Ret = MDOTypeMap::insert(MDOTypeMap::value_type(NewName, (*it)));
			
			ChildList.insert(ChildListIt, *it);
			ChildOrder.insert(ChildOrderIt, NewName);
		}

		// Increment the insertion points (unless already at the end of the lists)
		if(ChildListIt != ChildList.end()) ChildListIt++;
		if(ChildOrderIt != ChildOrder.end()) ChildOrderIt++;

		if(!Inserting)
		{
			// If we hit the end of the list we insert at the end (both lists should end at the same point!)
			if((ChildListIt == ChildList.end()) || (ChildOrderIt == ChildOrder.end())) Inserting = true;
		}

		it++;
	}
}


//! Determine if this object is derived from a specified type (directly or indirectly)
bool MDObject::IsA(std::string BaseType)
{
	MDOTypePtr TestType = Type;

	while(TestType)
	{
		if(TestType->Name() == BaseType) return true;
		TestType = TestType->Base;
	}

	return false;
}


//! Determine if this object is derived from a specified type (directly or indirectly)
bool MDObject::IsA(MDOTypePtr &BaseType)
{
	MDOTypePtr TestType = Type;

	while(TestType)
	{
		if(TestType == BaseType) return true;
		TestType = TestType->Base;
	}

	return false;
}
		

//! Determine if this object is derived from a specified type (directly or indirectly)
bool MDObject::IsA(const UL &BaseType)
{
	MDOTypePtr TestType = Type;

	while(TestType)
	{
		const ULPtr &TestUL = TestType->GetTypeUL();
		if((*TestUL).Matches(BaseType)) return true;
		TestType = TestType->Base;
	}

	return false;
}
		

//! Determine if this type is derived from a specified type (directly or indirectly)
bool MDOType::IsA(std::string BaseType)
{
	MDOTypePtr TestType = this;

	while(TestType)
	{
		if(TestType->Name() == BaseType) return true;
		TestType = TestType->Base;
	}

	return false;
}


//! Determine if this type is derived from a specified type (directly or indirectly)
bool MDOType::IsA(MDOTypePtr &BaseType)
{
	MDOTypePtr TestType = this;

	while(TestType)
	{
		if(TestType == BaseType) return true;
		TestType = TestType->Base;
	}

	return false;
}
		

//! Determine if this type is derived from a specified type (directly or indirectly)
bool MDOType::IsA(const UL &BaseType)
{
	MDOTypePtr TestType = this;

	while(TestType)
	{
		const ULPtr &TestUL = TestType->GetTypeUL();
		if((*TestUL).Matches(BaseType)) return true;
		TestType = TestType->Base;
	}

	return false;
}
		




/*
** XML_warning() - Handle warnings during XML parsing
*/
void MDOType::XML_warning(void *user_data, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);

	// DRAGONS: How do we prevent bursting?
	char Buffer[10240];
	vsprintf(Buffer, msg, args);

// DRAGONS: This should end up on stderr or similar, not in any XML output file!
//          If there is any reason to send warnings to an XML file that should be done in
//          the implementation of wraning(), not each message.
//	warning("<!-- XML WARNING: %s -->\n", Buffer);
	
	warning("XML WARNING: %s\n", Buffer);
	
    va_end(args);
}

/*
** XML_error() - Handle errors during XML parsing
*/
void MDOType::XML_error(void *user_data, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);

	// DRAGONS: How do we prevent bursting?
	char Buffer[10240];
	vsprintf(Buffer, msg, args);
	error("XML ERROR: %s\n", Buffer);

    va_end(args);
}

/*
** XML_fatalError() - Handle fatal erros during XML parsing
*/
void MDOType::XML_fatalError(void *user_data, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);

	// DRAGONS: How do we prevent bursting?
	char Buffer[1024];
	vsprintf(Buffer, msg, args);
	error("XML FATAL ERROR: %s\n", Buffer);

    va_end(args);
}




//! Locate reference target types for any types not yet located
void mxflib::MDOType::LocateRefTypes(void)
{
	MDOTypeList::iterator it = MDOType::AllTypes.begin();
	while(it != MDOType::AllTypes.end())
	{
		// Locate the reference target if the name exists, but not the type
		if((*it)->RefTargetName.size() && !(*it)->RefTarget)
		{
			MDOTypeMap::iterator it2 = NameLookup.find((*it)->RefTargetName);

			if(it2 == NameLookup.end())
			{
				error("Type %s specifies an unknown reference target type of %s\n", (*it)->Name().c_str(), (*it)->RefTargetName.c_str());
			}
			else
			{
				(*it)->RefTarget = (*it2).second;
			}
		}

		it++;
	}
}


//** Static Instantiations for MDOType class **
//*********************************************

MDOTypeList MDOType::AllTypes;	//!< All types managed by the MDOType class
MDOTypeList MDOType::TopTypes;	//!< The top-level types managed by the MDOType class

//! Map for UL lookups
std::map<UL, MDOTypePtr> MDOType::ULLookup;
		
//! Map for UL version-less lookups
std::map<UL, MDOTypePtr> MDOType::ULLookupVer1;
		
//! Map for reverse lookups based on type name
std::map<std::string, MDOTypePtr> MDOType::NameLookup;


//! Redefine a sub-item in a container
void MDOType::ReDefine(std::string NewDetail, std::string NewBase, unsigned int NewMinSize, unsigned int NewMaxSize)
{
	if(NewDetail.length()) Detail = NewDetail;
	
	if(NewBase.length())
	{
		MDTypePtr Type = MDType::Find(NewBase);
		if(!Type)
			error("Attempt to redefine %s to be of type %s which is not known\n", FullName().c_str(), NewBase.c_str());
		else
			ValueType = Type;
	}

	if(NewMinSize != 0) minLength = NewMinSize;
	if(NewMaxSize != 0) maxLength = NewMaxSize;
}


