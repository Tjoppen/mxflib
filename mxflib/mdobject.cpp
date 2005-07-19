/*! \file	mdobject.cpp
 *	\brief	Implementation of classes that define metadata objects
 *
 *			Class MDObject holds info about a specific metadata object
 *<br><br>
 *			Class MDOType holds the definition of MDObjects derived from
 *			the XML dictionary.
 *
 *	\version $Id: mdobject.cpp,v 1.13 2005/07/19 11:55:48 matt-beard Exp $
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



//! Static primer to use for index tables
PrimerPtr MDOType::StaticPrimer;



//! Build a Primer object for the current dictionary
/*! This primer has the mappings of tag to UL from the dictionary
 *  /param SetStatic - If true the StaticPrimer will be set to this new primer
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
 *	new MDOTypes from outside this class is via AddDict()
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
MDOTypePtr MDOType::Find(const UL& BaseUL)
{
	MDOTypePtr theType;

	std::map<UL, MDOTypePtr>::iterator it = ULLookup.find(BaseUL);

	if(it != ULLookup.end())
	{
		theType = (*it).second;
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
}



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

		// TODO: Needs to have a more complete name
		ObjectName = "Unknown"; // add " g:type=\"" + BaseType + "\"";
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


//! MDObject typed constructor
/*! Builds a "blank" metadata object of a specified type
 *	\note packs are built with defaut values
 */
MDObject::MDObject(ULPtr BaseUL) 
{
	Type = MDOType::Find(BaseUL);
	if(Type)
	{
		ObjectName = Type->Name();
	}
	else
	{
		Type = MDOType::Find("Unknown");

		// TODO: Needs to have a more complete name
		ObjectName = "Unknown"; // add " g:uid=\"" + BaseUL->GetString() + "\"";

		ASSERT(Type);

		// Shall we try and parse this?
		bool ParseDark = false;

#ifdef PARSEDARK
		ParseDark = true;
#endif

		if(ParseDark)
		{
			const Uint8 Set2x2[6] = { 0x06, 0x0E, 0x2B, 0x34, 0x02, 0x53 };
			if(memcmp(Set2x2, BaseUL->GetValue(), 6) == 0)
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
	TheUL = BaseUL;
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

		if(TheUL)
		{
			// Tag found, but UL unknown
			// FIXME: Needs to have a more complete name
			ObjectName = "Unknown"; // add " g:tag=\"" + Tag2String(BaseTag) + "\" g:uid=\"" + TheUL->GetString() + "\"";
		}
		else
		{
			// Tag not found, build a blank UL
			// FIXME: Needs to have a more complete name
			ObjectName = "Unknown"; // add " g:tag=\"" + Tag2String(BaseTag) + "\"";
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
			Value = new MDValue(Type->GetValueType());

			if(Type->GetValueType()->EffectiveClass() == TYPEARRAY)
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

			StringList::const_iterator it = Type->GetChildOrder().begin();
			while(it != Type->GetChildOrder().end())
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
}


#if 0 // WORK IN PROGRESS...
//! Add an empty numbered child to an MDObject continer and return a pointer to it
/*!	\return NULL if this object does not take numbered children
 *	\note If a child with this index already exists it is returned rather than adding a new one
 */
MDObjectPtr MDObject::AddChild(Uint32 ChildIndex)
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
}


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
			StringList::const_iterator it = Type->GetChildOrder().begin();
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
					
					if(it == Type->GetChildOrder().end()) it = Type->GetChildOrder().begin(); 
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

			if((ChildCount > 1) && (it != Type->GetChildOrder().begin()))
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
							// DRAGONS: Does this ever get called these days?
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

					// Read Length
					Uint32 Length;
					Uint32 ThisBytes = ReadLength(Type->GetLenFormat(), Size, Buffer, Length);

					// Advance counters and pointers past Length
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

						ThisBytes = (*it).second->ReadValue(Buffer, Length);

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

			Uint32 Bytes = 0;

			// Start with an empty list
			clear();

			// Scan until out of data
			while(Size)
			{
				Uint32 BytesAtItemStart = Bytes;

				DataChunk Key;
				Uint32 ThisBytes = ReadKey(Type->GetKeyFormat(), Size, Buffer, Key);

				// Abort if we can't read the key
				// this prevents us looping for ever if we
				// come across invalid data
				if(ThisBytes == 0) break;

				// Advance counters and pointers passed key
				Size -= ThisBytes;
				Buffer += ThisBytes;
				Bytes += ThisBytes;

				Uint32 Length;
				ThisBytes = ReadLength(Type->GetLenFormat(), Size, Buffer, Length);

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
					NewItem->KLSize = Bytes - BytesAtItemStart;

					ThisBytes = NewItem->ReadValue(Buffer, Length);

//					debug("  at 0x%s Set item (%s) %s = %s\n", Int64toHexString(NewItem->GetLocation(), 8).c_str(), Key.GetString().c_str(), NewItem->Name().c_str(), NewItem->GetString().c_str());

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

	ASSERT(GenUID);

	// Set the actual UID
	GenUID->Value->ReadValue(NewGen->GetValue(), NewGen->Size());

	return true;
}


//! Read a key from a memory buffer
Uint32 MDObject::ReadKey(DictKeyFormat Format, Uint32 Size, const Uint8 *Buffer, DataChunk& Key)
{
	Uint32 KeySize;

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
		ASSERT(0);							// Cause debug sessions to show this error
		Length = 0;
		return 0;

	case DICT_LEN_BER:
		{
			if( Size <1 ) break;
			Uint8 LenLen = GetU8(Buffer);

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

				Uint32 RetLen = LenLen + 1;
				if( RetLen > Size) break;

				// DRAGONS: ReadLength should return Uint64, BER length up to 7
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

					Uint64 Length64 = 0;
					const Uint8* LenBuff = (Buffer+1);
					while(LenLen--) Length64 = (Length64<<8) + *LenBuff++;

					if((Length64 >> 32) != 0)
					{
						error("Excessive BER length field in MDObject::ReadLength() - Metadata objects are limited to 4Gb each\n");
						Length = 0;
						return 0;
					}
					
					Length =(Uint32) Length64;
				}
				else
				{
					// Handle sane sized BER lengths
					Length = 0;
					const Uint8* LenBuff = (Buffer+1);
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


//! Build a data chunk with all this item's data (including child data)
const DataChunkPtr MDObject::PutData(PrimerPtr UsePrimer /* =NULL */) 
{ 
	if(Value) return Value->PutData(); 
	
	// DRAGONS: Pre-allocating a buffer could speed things up
	DataChunkPtr Ret = new DataChunk; 

	MDObjectNamedList::iterator it = begin();

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
Uint32 MDObject::WriteLinkedObjects(DataChunkPtr &Buffer, PrimerPtr UsePrimer /*=NULL*/)
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
#define DEBUG_WRITEOBJECT(x)
//#define DEBUG_WRITEOBJECT(x) x
Uint32 MDObject::WriteObject(DataChunkPtr &Buffer, MDObjectPtr ParentObject, PrimerPtr UsePrimer /*=NULL*/)
{
	Uint32 Bytes = 0;

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
			DEBUG_WRITEOBJECT( debug("Key = %s, ", Buffer->GetString().c_str()); )
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
		Uint32 Count = 0;
		Uint32 Size = 0;

		// DRAGONS: Pre-allocating a buffer could speed things up
		DataChunkPtr Val = new DataChunk();

		// Work out how many sub-items per child 
		Uint32 SubCount = Type->GetChildOrder().size();
		
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

			StringList::const_iterator it = Type->GetChildOrder().begin();
			while(it != Type->GetChildOrder().end())
			{
				MDOType::iterator it2 = Type->find(*it);
				ASSERT(it2 != Type->end());
				MDOTypePtr ChildType = (*it2).second;
				ASSERT(ChildType);

				MDObjectPtr Ptr = new MDObject(ChildType);
				Ptr->WriteObject(Temp, this, UsePrimer);
				it++;
			}
			Size = Temp->Size;
		}

		if(CType == BATCH)
		{
			// Write the length and batch header
			Bytes += WriteLength(Buffer, Val->Size+8, LenFormat);
			Uint8 Buff[4];
			PutU32(Count, Buff);
			Buffer->Append(4, Buff);
			PutU32(Size, Buff);
			Buffer->Append(4, Buff);
			Bytes += 8;
		}
		else
		{
			Bytes += WriteLength(Buffer, Val->Size, LenFormat);
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
				Bytes += Ptr->WriteObject(Val, this, UsePrimer);
			}
			it++;
		}

		// Write the length of the value
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

		MDObjectNamedList::iterator it = begin();

		while(it != end())
		{
			Bytes += (*it).second->WriteObject(Val, this, UsePrimer);
			it++;
		}

		// Write the length of the value
		Bytes += WriteLength(Buffer, Val->Size, LenFormat);

		// Append this data
		Buffer->Append(Val);
		Bytes += Val->Size;

		DEBUG_WRITEOBJECT( debug("  > %s\n", Val->GetString().c_str()); )
	}
	else if(Value)
	{
		DEBUG_WRITEOBJECT( debug("  *Value*\n"); )

		DataChunkPtr Val = Value->PutData();
		Bytes += WriteLength(Buffer, Val->Size, LenFormat);
		Buffer->Append(Val);
		Bytes += Val->Size;

		DEBUG_WRITEOBJECT( debug("  > %s\n", Val->GetString().c_str()); )
	}
	else
	{
		DEBUG_WRITEOBJECT( debug("  *Empty!*\n"); )

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
Uint32 MDObject::WriteLength(DataChunkPtr &Buffer, Uint64 Length, DictLenFormat Format, Uint32 Size /*=0*/)
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
			return BER->Size;
		}

	case DICT_LEN_1_BYTE:		
		{ 
			Uint8 Buff;
			PutU8((Uint8)Length, &Buff);

			Buffer->Append(1, &Buff);
			return 1;
		}

	case DICT_LEN_2_BYTE:
		{ 
			Uint8 Buff[2];
			PutU16((Uint16)Length, Buff);

			Buffer->Append(2, Buff);
			return 2;
		}

	case DICT_LEN_4_BYTE:
		{ 
			Uint8 Buff[4];
			PutU32((Uint32)Length, Buff);

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
Uint32 MDObject::WriteKey(DataChunkPtr &Buffer, DictKeyFormat Format, PrimerPtr UsePrimer /*=NULL*/)
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

			Uint8 Buff[2];
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
bool MDObject::MakeLink(MDObjectPtr &TargetSet, bool ForceLink /*=false*/)
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
		DataChunkPtr Data = InstanceUID->Value->PutData();
		ASSERT(Data->Size == 16);
		memcpy(TheUID, Data->Data, 16);
	}

	// Validate that we are a reference source
	// Note: The link will be attempted even if an error is produced
	//		 This is intentional as it may be valid in a later file spec
	DictRefType RType = Type->GetRefType();
	if((RType != DICT_REF_STRONG) && (RType != DICT_REF_WEAK))
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

	DataChunk DVal = PutData();
	if(DVal.Size != Type->GetDValue().Size) return false;
	
	if(memcmp(DVal.Data, Type->GetDValue().Data, DVal.Size) == 0) return true;

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

//! XML Dictionary parsing state-machine state
typedef enum
{
	DICT_STATE_START = 0,				//!< Not yet inside the 'MXFDictionary' tag
	DICT_STATE_DICT,					//!< Inside the 'MXFDictionary' tag
	DICT_STATE_END,						//!< Finished the dictionary
	DICT_STATE_ERROR					//!< Skip all else - error condition
} DictStateState;


//! XML parsing state structure for XML dictionary parser
typedef struct
{
	DictStateState	State;					//!< State-machine current state
	std::list<MDOTypePtr> Parents;			//!< List of pointers to parents, empty if currently at the top level
	std::list<std::string> ParentNames;		//!< List of parent names, empty if currently at the top level
} DictState;


//! Load the dictionary from the specified file
void MDOType::LoadDict(const char *DictFile)
{
	DictState State;
	// Set start state
	State.State = DICT_STATE_START;

	// Parse the file
	XMLParserHandler XMLHandler = {
		(startElementXMLFunc) XML_startElement,		/* startElement */
		(endElementXMLFunc) XML_endElement,			/* endElement */
		(warningXMLFunc) XML_warning,				/* warning */
		(errorXMLFunc) XML_error,					/* error */
		(fatalErrorXMLFunc) XML_fatalError,			/* fatalError */
	};

	std::string XMLFilePath = LookupDictionaryPath(DictFile);
	
	// Parse the file
	bool result = false;
	
	if(XMLFilePath.size()) result = XMLParserParseFile(&XMLHandler, &State, XMLFilePath.c_str());
	if(!result)
	{
		XML_fatalError(NULL, "Failed to load classes dictionary \"%s\"\n", XMLFilePath.size() ? XMLFilePath.c_str() : DictFile);
		return;
	}

	// If "Unknown" is not yet defined - define it
	MDOTypePtr Type = MDOType::Find("Unknown");
	if(!Type)
	{
		Type = new MDOType;
		Type->DictName = "Unknown";

		// The type of "Unknown" is also "Unknown"
		MDTypePtr ValType = MDType::Find("Unknown");
		ASSERT(ValType);
		Type->ValueType = ValType;

		// Add to the list of all types
		AllTypes.push_back(Type);

		// As it is a top level type then add it to TopTypes as well
		TopTypes.push_back(Type);

		// Set the name lookup - no UL for unknown
		NameLookup[Type->DictName] = Type;

	}

	// Locate reference target types
	MDOTypeList::iterator it = AllTypes.begin();
	while(it != AllTypes.end())
	{
		if((*it)->RefTargetName.size())
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

	// Build a static primer (for use in index tables)
	MakePrimer(true);
}


//! XML Dictionary parsing - Deal with start tag of an element
void MDOType::XML_startElement(void *user_data, const char *name, const char **attrs)
{
	DictState *State = (DictState*)user_data;
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

	switch(State->State)
	{
		case DICT_STATE_START:
			if(strcmp(name, "MXFDictionary") != 0)
			{
				XML_error(State, "Expected outer tag <MXFDictionary> got <%s/>", name);
				State->State = DICT_STATE_ERROR;
			}
			else
			{
				/* Start above outer layer of MXF dictionary */
				State->State = DICT_STATE_DICT;
			}

			break;

		case DICT_STATE_DICT:
		{
			bool HasGlobalKey = false;
			std::string default_text;
			std::string dvalue_text;

			/* Grab a new dictionary entry */
			MDOTypePtr Dict = new MDOType;

			// Record our name
			Dict->DictName = std::string(name);

			/* Check for inheritance */
			// We do this first as the entry needs to be BASED on the base entry
			if(attrs != NULL)
			{
				this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "base") == 0)
					{
						MDOTypePtr BaseType = Find(val);

						if(!BaseType)
						{
							XML_error(State, "Cannot find base type '%s' for <%s/>", val, name);
						}
						else
						{
							debug("Deriving %s from %s\n", name, BaseType->Name().c_str());
							Dict->Derive(BaseType);
						}

						break;
					}
				}
			}

			// Record our parent
			if(State->Parents.size())
			{
				// Our parent is the last entry in the parents list
				Dict->Parent = *State->Parents.rbegin();

				// Add us as a child of our parent
				Dict->Parent->insert(Dict);

				// Move reference details from parent (used for vectors)
				if(Dict->Parent->RefType != DICT_REF_NONE)
				{
					Dict->RefType = Dict->Parent->RefType;
					Dict->Parent->RefType = DICT_REF_NONE;
				}

//				/* Inherit vector details */
//				if((Parent->Type == DICT_TYPE_VECTOR) || (Parent->Type == DICT_TYPE_ARRAY))
//				{
//					Current->Type = Parent->VectorType;
//					Current->minLength = Parent->minLength;
//					Current->maxLength = Parent->maxLength;
//				}
			}

			// If we are not top level then record out "family tree"
			if(Dict->Parent) Dict->RootName = Dict->Parent->FullName() + "/";

			// Add to the list of all types
			AllTypes.push_back(Dict);

			// If it is a top level type then add it to TopTypes as well
			if(!Dict->Parent) TopTypes.push_back(Dict);

			// Set the name lookup - UL lookup set when key set
			NameLookup[Dict->RootName + Dict->DictName] = Dict;

			// Add us as parent to any following entries
			State->Parents.push_back(Dict);

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
						Uint8 Buffer[32];

						Size = ReadHexString(&p, 32, Buffer, " \t.");

						Dict->Key.Set(Size, Buffer);
					}
					else if(strcmp(attr, "globalKey") == 0)
					{
						int Size;
						const char *p = val;
						Uint8 Buffer[32];

						Size = ReadHexString(&p, 32, Buffer, " \t.");

						Dict->GlobalKey.Set(Size, Buffer);

						HasGlobalKey = true;
					}
					else if(strcmp(attr, "detail") == 0)
					{
						Dict->Detail = std::string(val);
					}
					else if(strcmp(attr, "use") == 0)
					{
						DictUse Use;

						if(strcasecmp(val,"required") == 0) Use = DICT_USE_REQUIRED;
						else if(strcasecmp(val,"encoder required") == 0) Use = DICT_USE_ENCODER_REQUIRED;
						else if(strcasecmp(val,"decoder required") == 0) Use = DICT_USE_DECODER_REQUIRED;
						else if(strcasecmp(val,"best effort") == 0) Use = DICT_USE_BEST_EFFORT;
						else if(strcasecmp(val,"optional") == 0) Use = DICT_USE_OPTIONAL;
						else if(strcasecmp(val,"dark") == 0) Use = DICT_USE_DARK;
						else if(strcasecmp(val,"toxic") == 0) Use = DICT_USE_TOXIC;
						else
						{
							XML_warning(State, "Unknown use value use=\"%s\" in <%s/>", val, name);
							Use = DICT_USE_OPTIONAL;
						}

						Dict->Use = Use;
					}
					else if(strcmp(attr, "ref") == 0)
					{
						DictRefType RefType;

						if(strcasecmp(val,"strong") == 0) RefType = DICT_REF_STRONG;
						else if(strcasecmp(val,"target") == 0) RefType = DICT_REF_TARGET;
						else if(strcasecmp(val,"weak") == 0) RefType = DICT_REF_WEAK;
						else
						{
							XML_warning(State, "Unknown ref value ref=\"%s\" in <%s/>\n", val, name);
							RefType = DICT_REF_NONE;
						}

						Dict->RefType = RefType;
					}
					else if(strcmp(attr, "type") == 0)
					{
						MDTypePtr Type = MDType::Find(val);

						if(Type)
						{
							Dict->ValueType = Type;
						}
						else
						{
							/*
							** Enumeration type for dictionary entry 'Type' field
							*/
							typedef enum 
							{
								DICT_TYPE_NONE = 0,
								DICT_TYPE_U8,
								DICT_TYPE_I8,
								DICT_TYPE_U16,
								DICT_TYPE_I16,
								DICT_TYPE_U32,
								DICT_TYPE_I32,
								DICT_TYPE_U64,
								DICT_TYPE_I64,
								DICT_TYPE_ISO7,
								DICT_TYPE_UTF8,
								DICT_TYPE_UTF16,
								DICT_TYPE_UUID,
								DICT_TYPE_UMID,
								DICT_TYPE_LABEL,
								DICT_TYPE_TIMESTAMP,
								DICT_TYPE_VERTYPE,
								DICT_TYPE_RATIONAL,
								DICT_TYPE_RAW,
								DICT_TYPE_I32ARRAY,

								/* Special to allow decoding of Sony IBC 2000 file */
								DICT_TYPE_UTFSONY,

								/* New types (soon to be added via types registry) */
								DICT_TYPE_BOOLEAN,
								DICT_TYPE_ISO7STRING,
								DICT_TYPE_UTF16STRING,
								DICT_TYPE_IEEEFLOAT64,
								DICT_TYPE_UINT8STRING,
								DICT_TYPE_PRODUCTVERSION,

								/* Container types */
								DICT_TYPE_UNIVERSAL_SET,
								DICT_TYPE_LOCAL_SET,
								DICT_TYPE_FIXED_PACK,
								DICT_TYPE_VARIABLE_PACK,
								DICT_TYPE_VECTOR,
								DICT_TYPE_ARRAY,

								/* Types for pixel layout */
								DICT_TYPE_RGBALAYOUT,

							} DictType;


							DictType DType = DICT_TYPE_NONE;
							if(strcasecmp(val,"universalSet") == 0) DType = DICT_TYPE_UNIVERSAL_SET;
							else if(strcasecmp(val,"localSet") == 0) DType = DICT_TYPE_LOCAL_SET;
							else if(strcasecmp(val,"subLocalSet") == 0) DType = DICT_TYPE_LOCAL_SET;
							else if(strcasecmp(val,"fixedPack") == 0) DType = DICT_TYPE_FIXED_PACK;
							else if(strcasecmp(val,"subFixedPack") == 0) DType = DICT_TYPE_FIXED_PACK;
							else if(strcasecmp(val,"variablePack") == 0) DType = DICT_TYPE_VARIABLE_PACK;
							else if(strcasecmp(val,"subVariablePack") == 0) DType = DICT_TYPE_VARIABLE_PACK;
							else if(strcasecmp(val,"vector") == 0) DType = DICT_TYPE_VECTOR;
							else if(strcasecmp(val,"subVector") == 0) DType = DICT_TYPE_VECTOR;
							else if(strcasecmp(val,"array") == 0) DType = DICT_TYPE_ARRAY;
							else if(strcasecmp(val,"subArray") == 0) DType = DICT_TYPE_ARRAY;
							else
							{
								XML_warning(State, "Unknown type \"%s\" in <%s/>", val, name);
							}

							// Set the container type
							if((DType == DICT_TYPE_UNIVERSAL_SET) || (DType == DICT_TYPE_LOCAL_SET))
							{
								Dict->ContainerType = SET;
							}
							else if((DType == DICT_TYPE_FIXED_PACK) || (DType == DICT_TYPE_VARIABLE_PACK))
							{
								Dict->ContainerType = PACK;
							}
							else if(DType == DICT_TYPE_VECTOR)
							{
								Dict->ContainerType = BATCH;
							}
							else if(DType == DICT_TYPE_ARRAY)
							{
								Dict->ContainerType = ARRAY;
							}

							// Modify 'omitted' items by type
							if(DType == DICT_TYPE_FIXED_PACK)
							{
								Dict->KeyFormat = DICT_KEY_NONE;
								Dict->LenFormat = DICT_LEN_NONE;
							}
							else if(DType == DICT_TYPE_VARIABLE_PACK)
							{
								Dict->KeyFormat = DICT_KEY_NONE;
							}
						}

						Dict->TypeName = std::string(val);

					}
//					else if(strcmp(attr, "length") == 0)
//					{
//						State->Tree[State->Sub]->minLength = atoi(val);
//						State->Tree[State->Sub]->maxLength = State->Tree[State->Sub]->minLength;
//					}
					else if(strcmp(attr, "minLength") == 0)
					{
						Dict->minLength = atoi(val);
					}
					else if(strcmp(attr, "maxLength") == 0)
					{
						Dict->maxLength = atoi(val);
					}
					else if(strcmp(attr, "keyFormat") == 0)
					{
						Dict->KeyFormat = (DictKeyFormat)atoi(val);
					}
					else if(strcmp(attr, "lengthFormat") == 0)
					{
						if(strcmp(val, "BER")==0)
						{
							Dict->LenFormat = DICT_LEN_BER;
						}
						else
						{
							Dict->LenFormat = (DictLenFormat)atoi(val);
						}
					}
					else if(strcmp(attr, "default") == 0)
					{
						/* Process later when all attributes read! */
						default_text = val;
					}
					else if(strcmp(attr, "dvalue") == 0)
					{
						/* Process later when all attributes read! */
						dvalue_text = val;
					}
					else if(strcmp(attr, "target") == 0)
					{
						Dict->RefTargetName = std::string(val);
					}
					else if(strcmp(attr, "base") == 0)
					{
						/* Do nothing here as base attribute is handled first! */
					}
					else
					{
						XML_warning(State, "Unexpected attribute '%s' in <%s/>", attr, name);
					}
				}
			}

			/* If only a 'key' is given index it with global key as well */
			if(!HasGlobalKey)
			{
				Dict->GlobalKey.Set(Dict->Key);
			}

			// Set the type UL
			if(Dict->GlobalKey.Size != 16)
			{
				// Zero byte keys are fine for abstract base classes
				if(Dict->GlobalKey.Size != 0) XML_error(State, "Global key is not 16 bytes long for <%s/>", name);
			}
			else
			{
				Dict->TypeUL = new UL(Dict->GlobalKey.Data);
				if(Dict->TypeUL) ULLookup[UL(Dict->TypeUL)] = Dict;
			}

			/* We process the default at the end so we are certain to know the type! */
			if(default_text.size())
			{
				if(Dict->ValueType)
				{
					MDValuePtr Val = new MDValue(Dict->ValueType);
					if(Val)
					{
						Val->SetString(std::string(default_text));
						DataChunkPtr Temp = Val->PutData();
						Dict->Default.Set(Temp);
					}
				}
				else
				{
					XML_warning(State, "Default value for <%s/> ignored as it is an unknown type", name);
				}
			}

			/* We also process the distinguished at the end so we are certain to know the type! */
			if(dvalue_text != "")
			{
				if(Dict->ValueType)
				{
					MDValuePtr Val = new MDValue(Dict->ValueType);
					if(Val)
					{
						Val->SetString(std::string(dvalue_text));
						DataChunkPtr Temp = Val->PutData();
						Dict->DValue.Set(Temp);
					}
				}
				else
				{
					XML_warning(State, "Distinguished value for <%s/> ignored as it is an unknown type", name);
				}
			}
			break;
		}

		case DICT_STATE_END:
			XML_error(State, "Unexpected outer tag after <MXFDictionary/> completed -> <%s/>", name);
			State->State = DICT_STATE_ERROR;
			break;

		default:			/* Unknown! */
		case DICT_STATE_ERROR:
			break;
	}
}


/*
** XML_endElement() - Deal with end tag of an element
*/
void MDOType::XML_endElement(void *user_data, const char *name)
{
	DictState *State = (DictState *)user_data;

	/* Skip if all has gone 'belly-up' */
	if(State->State == DICT_STATE_ERROR) return;

	// Remove the last parent from the stack
	std::list<MDOTypePtr>::iterator it = State->Parents.end();
	if(--it != State->Parents.end()) State->Parents.erase(it);
}



//! Derive this new entry from a base entry
/*! /note It is important that DictName is set before calling
 *	/note Don't attempt to call this function on objects that are not freshly created
 */
void MDOType::Derive(MDOTypePtr BaseEntry)
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
	StringList::iterator it = BaseEntry->ChildOrder.begin();
	while(it != Base->ChildOrder.end())
	{
		MDOTypePtr Current = NameLookup[Base->FullName() + "/" + (*it)];

		if(!Current)
		{
			error("Cannot locate child %s while deriving %s from %s\n", 
				  (*it).c_str(), DictName.c_str(), BaseEntry->DictName.c_str() );
			it++;
			continue;
		}

		// Add the base types children
		insert(Current);

		NameLookup[RootName + DictName + "/" + (*it)] = Current;
		it++;
	}

	// Copy the base type's ref target setting
	RefTarget = Base->RefTarget;
	RefTargetName = Base->RefTargetName;
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
bool MDObject::IsA(MDOTypePtr BaseType)
{
	MDOTypePtr TestType = Type;

	while(TestType)
	{
		if(TestType == BaseType) return true;
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




//** Static Instantiations for MDOType class **
//*********************************************

MDOTypeList MDOType::AllTypes;	//!< All types managed by the MDOType class
MDOTypeList MDOType::TopTypes;	//!< The top-level types managed by the MDOType class

//! Map for UL lookups
std::map<UL, MDOTypePtr> MDOType::ULLookup;
		
//! Map for reverse lookups based on type name
std::map<std::string, MDOTypePtr> MDOType::NameLookup;







//! Build an MDOType from dictionary data
/*! \return a smart pointer to the new type, or NULL if the call failed
 *  \note This function should only be called from a dictionary parser
 */
MDOTypePtr MDOType::BuildTypeFromDict(std::string Name, std::string Base, MDOTypePtr Parent,
									  DataChunkPtr Key, DataChunkPtr GlobalKey, std::string Detail,
									  DictUse Use, DictRefType RefType, MDTypePtr ValueType, 
									  std::string TypeName, MDContainerType ContainerType, 
									  unsigned int minLength, unsigned int maxLength, DictKeyFormat KeyFormat, 
									  DictLenFormat LenFormat, std::string RefTargetName,
									  std::string Default, std::string DValue, UInt32 Items)
{
	/* Grab a new dictionary entry */
	MDOTypePtr Dict = new MDOType;
	if(!Dict) return Dict;

	// Record our name
	Dict->DictName = Name;

	/* Check for inheritance */
	// We do this first as the entry needs to be BASED on the base entry
	if(Base.size())
	{
		MDOTypePtr BaseType = Find(Base);

		if(!BaseType)
		{
			error("Cannot find base type \"%s\" for type \"%s\"\n", Base.c_str(), Name.c_str());
		}
		else
		{
			debug("Deriving %s from %s\n", Name.c_str(), BaseType->Name().c_str());
			Dict->Derive(BaseType);
		}
	}

	// Record our parent
	if(Parent)
	{
		Dict->Parent = Parent;

		// Add us as a child of our parent
		Parent->insert(Dict);

		// Move reference details from parent (used for vectors)
		if(Parent->RefType != DICT_REF_NONE)
		{
			Dict->RefType = Dict->Parent->RefType;
			Parent->RefType = DICT_REF_NONE;
		}
	}

	// If we are not top level then record out "family tree"
	if(Parent) Dict->RootName = Dict->Parent->FullName() + "/";

	// Add to the list of all types
	AllTypes.push_back(Dict);

	// If it is a top level type then add it to TopTypes as well
	if(!Parent) TopTypes.push_back(Dict);

	// Set the name lookup - UL lookup set when key set
	NameLookup[Dict->RootName + Dict->DictName] = Dict;

	// Set the key (if known)
	if(Key) Dict->Key.Set(Key);

	// Set the global key (if known)
	// Note: The caller must copy one key to the other if only one is given
	if(GlobalKey) Dict->GlobalKey.Set(GlobalKey);

	// Set other basic fields - conditionally to allow safe derivation
	if(Detail.size()) Dict->Detail = Detail;
	if(Items & DICT_ITEM_USE) Dict->Use = Use;
	if(Items & DICT_ITEM_REFTYPE) Dict->RefType = RefType;
	if(Items & DICT_ITEM_CONTAINERTYPE) Dict->ContainerType = ContainerType;
	if(Items & DICT_ITEM_MINLENGTH) Dict->minLength = minLength;
	if(Items & DICT_ITEM_MAXLENGTH) 
	{
		if((maxLength < minLength) ||(maxLength == 0)) Dict->maxLength = (unsigned int)-1;
		else Dict->maxLength = maxLength;
	}
    if(Items & DICT_ITEM_KEYFORMAT) Dict->KeyFormat = KeyFormat;
	if(Items & DICT_ITEM_LENFORMAT) Dict->LenFormat = LenFormat;
	if(ValueType) Dict->ValueType = ValueType;
	if(TypeName.size()) Dict->TypeName = TypeName;
	if(RefTargetName.size()) Dict->RefTargetName = RefTargetName;


	// Set the type UL
	if(Dict->GlobalKey.Size != 16)
	{
		// Zero byte keys are fine for abstract base classes
		if(Dict->GlobalKey.Size != 0) 
		{
			error("Global key is not 16 bytes long for \"%s\"", Name.c_str());
		}
	}
	else
	{
		Dict->TypeUL = new UL(Dict->GlobalKey.Data);
		if(Dict->TypeUL) ULLookup[UL(Dict->TypeUL)] = Dict;
	}

	// Process the default now that we know the type
	if(Default.size())
	{
		if(Dict->ValueType)
		{
			MDValuePtr Val = new MDValue(Dict->ValueType);
			if(Val)
			{
				Val->SetString(std::string(Default));
				DataChunkPtr Temp = Val->PutData();
				Dict->Default.Set(Temp);
			}
		}
		else
		{
			warning("Default value for \"%s\" ignored as it is an unknown type", Name.c_str());
		}
	}

	// We also process the distinguished value at the end so we know the type
	if(Items & DICT_ITEM_DVALUE)
	{
		if(Dict->ValueType)
		{
			MDValuePtr Val = new MDValue(Dict->ValueType);
			if(Val)
			{
				Val->SetString(DValue);
				DataChunkPtr Temp = Val->PutData();
				Dict->DValue.Set(Temp);
			}
		}
		else
		{
			warning("Distinguished value for \"%s\" ignored as it is an unknown type", Name.c_str());
		}
	}

	// Return the new type
	return Dict;
}
