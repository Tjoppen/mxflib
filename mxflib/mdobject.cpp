/*! \file	mdobject.cpp
 *	\brief	Implementation of classes that define metadata objects
 *
 *			Class MDObject holds info about a specific metadata object
 *<br><br>
 *			Class MDOType holds the definition of MDObjects derived from
 *			the XML dictionary.
 *
 *	\version $Id: mdobject.cpp,v 1.26 2011/01/10 10:42:09 matt-beard Exp $
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
 
#include "mxflib/mxflib.h"

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

//! Translator function to translate unknown ULs to object names
MDObject::ULTypeMaker MDObject::TypeMaker = NULL;

//! Build a Primer object from built-in knowledge
/*! This primer has the mappings of axiomatic tags to ULs from the dictionary
 *  /param SetStatic - If true the StaticPrimer will be set to this new primer
 */
PrimerPtr MDOType::MakeBuiltInPrimer(bool SetStatic /*=false*/)
{
	PrimerPtr Ret = new Primer;



	// Replace existing StaticPrimer if requested
	if(SetStatic) StaticPrimer = Ret;

	return Ret;
}
		
//! Build a primer object from the current dictionary
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
	//! Check if we have required internal items defined
	if(!MDOType::GetInternalsDefined()) MDOType::DefineInternals();

	ULPtr ThisUL = SymSpace->Find(BaseType);
			
	// If name not found, try by UL
	if(!ThisUL) ThisUL = StringToUL(BaseType);

	if(ThisUL) Type = MDOType::Find(ThisUL);
	if(Type)
	{
		ObjectName = Type->Name();
	}
	else
	{
		error("Metadata object type \"%s\" doesn't exist\n", BaseType.c_str());

		Type = MDOType::Find("Unknown");

		mxflib_assert(Type);

		ObjectName = "Unknown " + BaseType;
	}

	// Set up the value type
	if(Type->GetContainerType() == NONE)
	{
		ValueType = Type->GetValueType();
		if(!ValueType) ValueType = MDType::Find("UnknownType");

		// Set the legacy value link to us as we are a value
		Value = this;
		IsValue = true;
	}
	else
		IsValue = false;

	IsConstructed = true;
	IsSubItem = false;
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
	//! Check if we have required internal items defined
	if(!MDOType::GetInternalsDefined()) MDOType::DefineInternals();

	// Set up the value type
	if(Type->GetContainerType() == NONE)
	{
		ValueType = Type->GetValueType();
		if(!ValueType) ValueType = MDType::Find("UnknownType");

		// Set the legacy value link to us as we are a value
		Value = this;
		IsValue = true;
	}
	else
		IsValue = false;

	ObjectName = BaseType->Name();
	
	IsConstructed = true;
	IsSubItem = false;
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


//! Construct a generic MDObject with a given value type
MDObject::MDObject(const MDType *ValType)
{
	//! Check if we have required internal items defined
	if(!MDOType::GetInternalsDefined()) MDOType::DefineInternals();

	Type = MDOType::Find("Unknown");
	mxflib_assert(Type);

	// DRAGONS: We have to be able to build from a const pointer as many of the functions that
	//          produce a MDType pointer return const values. We deal with this by re-finding by UL

	ValueType = MDType::Find(ValType->GetTypeUL());
	mxflib_assert(ValueType);
	if(!ValueType) ValueType = MDType::Find("UnknownType");

	ObjectName = ValueType->Name();

	// Set the legacy value link to us as we are a value
	Value = this;
	IsValue = true;

	IsConstructed = true;
	IsSubItem = true;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;
	TheTag = 0;

	Outer = NULL;

	// Initialise the new object
	Init();
}


//! Construct a generic MDObject with given traits
MDObject::MDObject(MDTraitsPtr &Tr)
{
	//! Check if we have required internal items defined
	if(!MDOType::GetInternalsDefined()) MDOType::DefineInternals();

	Type = MDOType::Find("Unknown");
	mxflib_assert(Type);

	ValueType = MDType::Find("UnknownType");
	mxflib_assert(ValueType);

	ObjectName = Tr->Name();

	// Set the legacy value link to us as we are a value
	Value = this;
	IsValue = true;

	IsConstructed = true;
	IsSubItem = true;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;
	TheTag = 0;

	Outer = NULL;

	// Initialise the new object
	Init();

	// Set the traits after initializing (as this may have changed the traits)
	Traits = Tr;
}


//! MDObject UL based constructor body
/*! Builds a "blank" metadata object of a specified type
 *	\note packs are built with default values
 *
 *  \note TheUL must be set before calling
 */
void MDObject::ULCtor(void) 
{
	//! Check if we have required internal items defined
	if(!MDOType::GetInternalsDefined()) MDOType::DefineInternals();

	Type = MDOType::Find(TheUL);
	if(Type)
	{
		ObjectName = Type->Name();
		if(Type->GetContainerType() == NONE)
		{
			ValueType = Type->GetValueType();
			if(!ValueType) ValueType = MDType::Find("UnknownType");

			// Set the legacy value link to us for values
			Value = this;
			IsValue = true;
		}
		else
			IsValue = false;
	}
	else
	{
		ValueType = MDType::Find(TheUL);
		if(ValueType)
		{
			Type = MDOType::Find("Unknown");
			ObjectName = ValueType->Name();
			
			// Set the legacy value link to us as we are a value
			Value = this;
			IsValue = true;
		}
		else
		{
			// Build as an unknown object
			TheTag = 0;
			UnknownCtor();
			return;
		}
	}

	IsConstructed = true;
	IsSubItem = false;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;
	TheTag = 0;

	Outer = NULL;

	// Initialise the new object
	Init();
}


//! MDObject "unknown" object constructor body
/*! Builds a "blank" metadata object with an unknown type based on a given UL
 *	\note packs are built with default values
 *
 *  \note TheUL and TheTag must be set before calling
 */
void MDObject::UnknownCtor(void)
{
	if(UL2NameFunc)
	{
		ObjectName = UL2NameFunc(TheUL,NULL);

		if(ObjectName.empty())
		{
			// If we were unable to translate at all, use the normal pattern
			ObjectName = "Unknown";
			if(TheTag) ObjectName += " " + Tag2String(TheTag);
			if(!(TheUL->Matches(AbstractObject_UL))) ObjectName += " " + TheUL->GetString();
		}
		else //if(Feature(FeatureUnknownsByUL2Name))
		{
			// FIXME: This doen not seem right here - to be checked!
			if(TypeMaker)
				Type = TypeMaker(TheUL,NULL);
			else
			// If this feature is enabled, try to now find the type by name
			Type = MDOType::Find(ObjectName);
		}
	}
	else
	{
		ObjectName = "Unknown";
		if(TheTag) ObjectName += " " + Tag2String(TheTag);
		if(!(TheUL->Matches(AbstractObject_UL))) ObjectName += " " + TheUL->GetString();
	}

	// Shall we try and parse this?
	if(ParseDark)
	{
		const UInt8 Set2x2[6] = { 0x06, 0x0E, 0x2B, 0x34, 0x02, 0x53 };
		if(memcmp(Set2x2, TheUL->GetValue(), 6) == 0)
		{
			Type = MDOType::Find(AbstractObject_UL);
		}
	}

	// If all else fails - just set this as unknown
	if(!Type) Type = MDOType::Find("Unknown");
	mxflib_assert(Type);

	// Determine the value type and container status
	if(Type)
	{
		if(Type->GetContainerType() == NONE)
		{
			ValueType = Type->GetValueType();
			if(!ValueType) ValueType = MDType::Find("UnknownType");

			// Set the legacy value link to us for values
			Value = this;
			IsValue = true;
		}
		else
			IsValue = false;
	}
	else IsValue = false;

	IsConstructed = true;
	IsSubItem = false;
	ParentOffset = 0;
	KLSize = 0;
	Parent = NULL;
	ParentFile = NULL;

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
			/* MJB 8-June-2007: DRAGONS: Don't complain about AAF built-in tags */
			if(BaseTag >= 0x0100)
			{
				//# //IDB chagned to warning for demo at NAB 06
				//# warning("Metadata object with Tag \"%s\" doesn't exist in specified Primer\n", Tag2String(BaseTag).c_str());
				error("Metadata object with Tag \"%s\" doesn't exist in specified Primer\n", Tag2String(BaseTag).c_str());
			}

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
		if(!TheUL) TheUL = new UL("Unknown");
		TheTag = BaseTag;
		UnknownCtor();
		return;
	}

	ObjectName = Type->Name();

	// Set up the value type
	if(Type->GetContainerType() == NONE)
	{
		ValueType = Type->GetValueType();
		if(!ValueType) ValueType = MDType::Find("UnknownType");

		// Set the legacy value link to us for values
		Value = this;
		IsValue = true;
	}
	else
		IsValue = false;

	IsConstructed = true;
	IsSubItem = false;
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

	// Set the traits from the value or type
	if(ValueType) Traits = ValueType->GetTraits();
	else if (Type && Type->GetValueType()) Traits = Type->GetValueType()->GetTraits();

	// Ensure that items that are handled by traits that manage the data as a whole don't build sub-items
	MDContainerType CType = GetContainerType();
	if(CType == ARRAY)
	{
		if(Traits && Traits->HandlesSubdata())
		{
			// Ensure that we set the value type for objects that handle thier own data
			if(!ValueType) ValueType = Type->GetValueType();

			if(ValueType && ValueType->Size > 0)
			{
				// Build buffer if the data is fixed size
				Data.Resize(ValueType->Size);
			}

			// Set the legacy value link to us as we are now a value
			Value = this;
			IsValue = true;

		}
		if(ValueType && ValueType->HandlesSubdata()) CType = NONE;
	}

	switch(CType)
	{
	case NONE:
		// If it isn't a container build the basic item
		{
			mxflib_assert(ValueType);

			// If it's a basic type (or handles its sub data) build an empty item
			if(ValueType->HandlesSubdata() || (ValueType->EffectiveClass() == BASIC))
			{
				size_t ValueSize = ValueType->EffectiveSize();
				if(ValueSize)
				{
					MakeSize(ValueSize);
					memset(Data.Data,0,Data.Size);
				}
			}

			// If it's a fixed size array build all items
			else if(ValueType->EffectiveClass() == TYPEARRAY)
			{
				// If this is an array of references, don't treat the array as a value, so it's subs get handled individually
///				ClassRef RefType = ValueType->EffectiveRefType();
///				if(RefType != ClassRefNone) IsValue = false;

				if(ValueType->Size > 0)
				{
					// Build blank array
					Resize(ValueType->Size);
				}
				else if(Type && Type->GetMinLength())
				{
					// Build the minimum size array
					Resize(Type->GetMinLength());
				}
			}

			// If it's a compound build all sub-items
			else if(ValueType->EffectiveClass() == COMPOUND)
			{
				const MDType *EType = ValueType->EffectiveType();

				MDTypeList::const_iterator it;
				it = EType->GetChildList().begin();

				while(it != EType->GetChildList().end())
				{
					// Insert a new item of the appropriate type
					insert(new MDObject(*it));
					it++;
				}
			}
		}

		break;

	case PACK:
		// If it's a pack build all items
		{
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
		{
			// Allow legacy access, even though we ae not a value
			Value = this;

			// Treat multiples of references as multiples, not values
			if(IsRefSource(GetRefType())) IsValue = false;
			else IsValue = true;

			// Ensure that it maintains minimum length
			Resize(Type->GetMinLength());
			break;
		}

	default:
		{
		}
	}
}


//! Set a variable to be a certain size in bytes
/*!	This function assumes that this is a viable thing to do!
 *  \return The size of the resized item
 */
size_t MDObject::MakeSize(size_t NewSize)
{
//	if((!IsValue) || (!ValueType)) return 0;

	// Enforce fixed size if one exists for this type
	if(ValueType->Size) NewSize = ValueType->Size;

	Data.Resize(NewSize);
	return NewSize;
}



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
			ULPtr ChildUL = SymSpace->Find(ChildName, true);

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
		/* The user has attempted to add a default type MDObject child when this type has no children.
		 * Try and interpret this as a request to add a child to the value (for compatibility with old subVector and subArray types)
		 */
		
		// See if this is a logical thing to do (i.e. is the value an array or batch)
		const MDType *ValueType = Type->GetValueType()->EffectiveType();
		if(ValueType->EffectiveClass() != TYPEARRAY)
		{
			error("Attempted to AddChild() to %s which has no child types declared\n", FullName().c_str());
			return NULL;
		}

		// Add a new child to the value
		MDTypePtr ChildType = ValueType->EffectiveBase();
		MDObjectPtr Ret = new MDObject(ChildType);
		Value->AddChild(Ret);

		return Ret;

		//error("Attempted to AddChild() to %s which has no child types declared\n", FullName().c_str());
		//return NULL;
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
/*! \return false if unable to add this child
 *  \note If there is already a child of this type it is replaced if Replace = true
 */
bool MDObject::AddChild(MDObjectPtr &ChildObject, bool Replace /* = false */)
{
	SetModified(true); 

	return AddChildInternal(ChildObject, Replace) == ChildObject;
}


bool remove_this_stub;
//! Add the given child object at a specific numerical index
/*! \ret false if unable to add this child at the specified location (for example if not numerically indexed) */
/*bool MDObject::AddChild(MDObjectPtr &Child, int Index)
{
	MDObjectPtr Ptr;

	SetModified(true); 

	// If we are adding to an array, simply resize the array
	MDTypeClass Class = ValueType->EffectiveClass();
	if(Class == TYPEARRAY)
	{
		if(Index < static_cast<int>(size())) Resize(Index);
		return true;
	}

	// Try and find an existing child at that position
	Ptr = operator[](Index);

	// Only add a new one if we didn't find it
	if(!Ptr)
	{
		// Find the child definition
		MDOTypePtr ChildType = Type->Child(Index);

		// Insert a new item of the correct type
		if(ChildType) Ptr = new MDObject(ChildType);

		if(Ptr) insert(Ptr);
	}

	// Return true if we succeeded
	if(Ptr) return true; else return false;
}*/


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
		if((*it).first.Matches(ChildType->GetTypeUL()))
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
MDObjectPtr MDObject::operator[](const std::string ChildName) const
{
	MDObjectULList::const_iterator it = begin();
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
MDObjectPtr MDObject::operator[](const MDOTypePtr &ChildType) const
{
	MDObjectULList::const_iterator it = begin();
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
MDObjectPtr MDObject::operator[](const MDTypePtr &ChildType) const
{
	MDObjectULList::const_iterator it = begin();
	while(it != end())
	{
		if((*it).second->ValueType == ChildType)
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
MDObjectPtr MDObject::operator[](const UL &ChildType) const
{
	MDObjectULList::const_iterator it = begin();
	while(it != end())
	{
		if((*it).first.Matches(ChildType))
		{
			return (*it).second;
		}
		it++;
	}

	return NULL;
}


//! Locate a numerically indexed child
/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
MDObjectPtr MDObject::operator[](int Index) const
{
	// Locate by child type (for sets and packs)
	if((Type->GetContainerType() == SET) || (Type->GetContainerType() == PACK))
	{
		if((Index < 0) || (static_cast<size_t>(Index) >= Type->GetChildList().size())) return NULL;
		
		MDOTypeList::const_iterator it = Type->GetChildList().begin();
		while(Index--) it++;
		return Child(*it);
	}

	// Locate by position (for arrays and batches)
	if((Index < 0) || (static_cast<size_t>(Index) >= size())) return NULL;

	const_iterator it = begin();
	while(Index--) it++;
	return (*it).second;
}


//! Find all sub-items within a compound MDObject of a named type
MDObjectListPtr MDObject::ChildList(const std::string ChildName) const
{
	MDObjectListPtr Ret = new MDObjectList;
	MDObjectULList::const_iterator it = begin();

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


//! Find all sub-items within a compound MDObject of a given type
MDObjectListPtr MDObject::ChildList(const MDOTypePtr &ChildType) const
{
	MDObjectListPtr Ret = new MDObjectList;
	MDObjectULList::const_iterator it = begin();

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


//! Find all sub-items within a compound MDObject of a given value type
MDObjectListPtr MDObject::ChildList(const MDTypePtr &ChildType) const
{
	MDObjectListPtr Ret = new MDObjectList;
	MDObjectULList::const_iterator it = begin();

	while(it != end())
	{
		if((*it).second->ValueType == ChildType)
		{
			// Add this object to the list
			Ret->push_back((*it).second);
		}
		it++;
	}

	return Ret;
}


//! Find all sub-items within a compound MDObject of a specified type
MDObjectListPtr MDObject::ChildList(const UL &ChildType) const
{
	MDObjectListPtr Ret = new MDObjectList;
	MDObjectULList::const_iterator it = begin();

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

	// Allow arrays and batches to be handled by thier traits if known
	MDContainerType CType = GetContainerType();
	if(((CType == ARRAY) || (CType == BATCH)) && Traits) CType = NONE;

	switch(CType)
	{
	case NONE:
		{
			Bytes = Traits->ReadValue(this, Buffer, Size);
			SetModified(false);
			return Bytes;
		}

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
			if(Count == 0)
			{
				SetModified(false);
				return Bytes;
			}
		}
		// Fall through and process as an array

	case ARRAY:
		{
			if(Type->empty())
			{
				// Start with no children
				clear();

				while(Size || Count)
				{
					MDObjectPtr NewItem = AddChild();

					mxflib_assert(NewItem);

					NewItem->Parent = this;
					NewItem->ParentOffset = Bytes;
					NewItem->KLSize = 0;

					size_t ThisBytes = NewItem->ReadValue(Buffer, Size);

					Bytes += ThisBytes;
					Buffer += ThisBytes;
					if(ThisBytes > Size) Size = 0; else Size -= ThisBytes;

					// If processing a batch, set up for the next item
					if(Count != 0) 
					{
						if(--Count) Size = ItemSize; else break;
					}
				}

				SetModified(false);
				return Bytes;
//##				error("Object %s at 0x%s in %s is a multiple, but has no contained types\n", FullName().c_str(), Int64toHexString(GetLocation(), 8).c_str(), GetSource().c_str());
//##				SetModified(false);
//##				return Bytes;
			}

warning("Entered dead code for reading old-style batches/arrays for %s\n", FullName().c_str());
			// Start with no children
			clear();

			// Find the first (or only) child type
			MDOTypeList::const_iterator it = Type->GetChildList().begin();

			size_t ChildCount = Type->size();
			while(Size || Count)
			{
				MDObjectPtr NewItem = new MDObject(*it);

				mxflib_assert(NewItem);

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

			SetModified(false);
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
						warning("Extra bytes found parsing buffer for %s in MDObject::ReadValue()\n", FullName().c_str());
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
					if((*it).second->Type->GetValueType()->EffectiveType()->GetArrayClass() == ARRAYIMPLICIT)
					{
						if((*it).second->IsA(PartitionArray_UL))
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

				SetModified(false);
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
						warning("Extra bytes found parsing buffer for %s in MDObject::ReadValue()\n", FullName().c_str());
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
						SetModified(false);
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

				SetModified(false);
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
					SetModified(false);
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
						mxflib_assert(Key.Size == 2);
						Tag ThisKey = GetU16(Key.Data);

						NewItem = new MDObject(ThisKey, UsePrimer);
					}
					else if(Type->GetKeyFormat() == DICT_KEY_AUTO)
					{
						mxflib_assert(Key.Size == 16);
						ULPtr ThisUL = new UL(Key.Data);

						NewItem = new MDObject(ThisUL);
					}
					else
					{
						// Only 2-byte and 16-byte keys are supported at present
						mxflib_assert(0);
						SetModified(false);
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
							error("Failed to read complete %s value at 0x%s in %s - specified length=%s, read=%s\n", 
								NewItem->FullName().c_str(), Int64toHexString(NewItem->GetLocation(), 8).c_str(), 
								NewItem->GetSource().c_str(), Int64toString(Length).c_str(), Int64toString(ThisBytes).c_str());
							
							// Skip anything left over
							if(Length > ThisBytes) ThisBytes = static_cast<size_t>(Length);
						}
					}

					Size -= static_cast<size_t>(Length);
					Buffer += static_cast<size_t>(Length);
					Bytes += static_cast<size_t>(Length);

#ifdef OPTION3ENABLED
					// Have we just read an ObjectClass for this object?
					if(NewItem->TheUL && NewItem->TheUL->Matches(ObjectClass_UL))
					{
						DataChunkPtr NewData = NewItem->PutData();
						
						if(NewData && (NewData->Size == 16))
						{
							// Record the baseline UL that was used as the set key
							BaselineUL = TheUL;

							// Build a new UL
							UL NewUL(NewData->Data);

							// Change us to that type
							ChangeType(NewUL);
						}
						else
						{
							error("%s at %s has an invalid value of %s\n", FullName().c_str(), GetSourceLocation().c_str(), NewItem->Name().c_str());
						}

						// DRAGONS: We now DON'T add this item so the ObjectClass property is invisible to us!!
					}
					else
#endif // OPTION3ENABLED
					{
						AddChildInternal(NewItem);
					}
				}
			}

			SetModified(false);
			return Bytes;
		}

	default:
		mxflib_assert(0);
		SetModified(false);
		return 0;
	}
}


//! Compare the exact value of two MDObjects
bool MDObject::operator==(const MDObject &RVal) const
{
	/* To be identical, they must be the same type or the same valuetype*/
	if((!ValueType) && (!RVal.ValueType))
	{
		// Neither is a value - check they are the same effective type
        if(Type != RVal.Type) return false;
	}
	else
	{
		// If one is a value and not the other - no match
		if((!ValueType) || (!RVal.ValueType)) return false;
	}

	// Get the two values as data
	DataChunkPtr Left = PutData();
	DataChunkPtr Right = RVal.PutData();

	if(Left->Size != Right->Size) return false;

	// The values match if the comparision says they are identical
	return (memcmp(Left->Data, Right->Data, Left->Size) == 0);
}


//! Add or Remove children from an MDObject continer to make a fixed size
/*! Probably only useful for resizing arrays.
 */
void MDObject::Resize(UInt32 Count)
{
	if(!ValueType) ValueType = Type->GetValueType();
	mxflib_assert(ValueType);
	if(!ValueType) return;

	MDTypeClass Class = ValueType->EffectiveClass();

	mxflib_assert( Class == TYPEARRAY /*|| Class == COMPOUND*/ );
	
	// If this function is called for a fixed size array
	// simply validate the size
	if(ValueType->GetSize()) Count = ValueType->GetSize();

	if(Count == 0)
	{
		clear();
		return;
	}

	unsigned int Current = static_cast<unsigned int>(size());

	// Extra padding items required
	if(Current < Count)
	{
		MDTypePtr Base = ValueType->EffectiveBase();
		while(Current < Count)
		{
			// Insert a new item of the appropriate type
//			insert(MDObject::value_type(Current, new MDObject(Type)));
			insert(new MDObject(Base));
			Current++;
		}
	}
	else if (Current > Count)
	{
		resize(Count);
	}
}


//! Has this object (including any child objects) been modified?
bool MDObject::IsModified(void) const
{
	if(IsSubItem) return false;

	if(Modified) return true;

	if(!empty())
	{
		MDObjectULList::const_iterator it = begin();

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
		// If we don't currently have a GenerationUID, first check if this is because we shouldn't have one
		if(!IsA(InterchangeObject_UL)) return false;

		// Otherwise go ahead and add one
		GenUID = AddChild(GenerationUID_UL);
	}

	mxflib_assert(GenUID);

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
		mxflib_assert(0);
		Key.Resize(0);
		return 0;

	case DICT_KEY_1_BYTE:		KeySize = 1; break;
	case DICT_KEY_2_BYTE:		KeySize = 2; break;
	case DICT_KEY_4_BYTE:		KeySize = 4; break;
	}

	if(Size < KeySize)
	{
		error("Not enough bytes for required key type in MDObject::ReadKey(), got %s, require %d\n", Int64toString(Size).c_str(), KeySize);
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
		mxflib_assert(0);							// Cause debug sessions to show this error
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
Position mxflib::MDObject::GetLocation(void) const
{
	Position Ret = ParentOffset;

	if(Parent) Ret += Parent->KLSize + Parent->GetLocation();

	return Ret;
}

//! Get text that describes where this item came from
std::string mxflib::MDObject::GetSource(void) const
{
	if(Parent) return Parent->GetSource();
	if(ParentFile) return std::string("file \"") + ParentFile->Name + std::string("\"");

	return std::string("memory buffer");
}


//! Build a data chunk with all this item's data (including child data)
DataChunkPtr MDObject::PutData(PrimerPtr UsePrimer /* =NULL */) const
{ 
	// DRAGONS: Pre-allocating a buffer could speed things up
	DataChunkPtr Ret = new DataChunk; 

//	if(!IsValue)
	if(!ValueType)
	{
		MDObject::const_iterator it = begin();

		while(it != end())
		{
			(*it).second->WriteObject(Ret, this, UsePrimer);
			it++;
		}

		return Ret;
	}

	// If this is a batch we write a dummy header and update it later
	MDArrayClass EffClass = ValueType->EffectiveType()->GetArrayClass();
	if(EffClass == ARRAYEXPLICIT)
	{
		static UInt8 DummyBuffer[8] = { 0,0,0,0, 0,0,0,0 };
		Ret->Set(8, DummyBuffer);
	}

	// If the size is zero we don't have any sub items
	// Otherwise we may not need to use them because the traits may build in our data
	if(size() == 0 || (ValueType->HandlesSubdata())) 
	{
		// If we are part of a batch this appends the data, otherwise it simply sets it to be the same
		Ret->Append(GetData());
	}
	else
	{
		// Compounds must be written in the correct order
		if(ValueType->EffectiveClass() == COMPOUND)
		{
			MDOTypeList::const_iterator it = Type->GetChildList().begin();
			while(it != Type->GetChildList().end())
			{
				DataChunkPtr SubItem = Child(*it)->PutData();
				Ret->Append(SubItem->Size, SubItem->Data);
				it++;
			}
		}
		else
		{
			MDObject::const_iterator it = begin();
			while(it != end())
			{
				DataChunkPtr SubItem = (*it).second->PutData();
				Ret->Append(SubItem->Size, SubItem->Data);
				it++;
			}
		}
	}

	// Now is the time to update the batch header as we can work out item sizes for variable length items
	if(EffClass == ARRAYEXPLICIT)
	{
		UInt8 Buffer[8];
		PutU32(static_cast<UInt32>(size()), Buffer);

		UInt32 ItemSize = ValueType->EffectiveBase()->EffectiveSize();
		
		// Calculate item size if variable (and not empty)
		if((ItemSize == 0) && (size() > 0)) ItemSize = static_cast<UInt32>((Ret->Size - 8) / size());

		PutU32(ItemSize, &Buffer[4]);

		// Set the header
		Ret->Set(8, Buffer);
	}

	return Ret;
};


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
			Bytes += (*it).second->WriteLinkedSubObjects(Buffer, UsePrimer);
		}
		it++;
	}

	return Bytes;
}


//! Write any items strongly linked from sub-items
size_t MDObject::WriteLinkedSubObjects(DataChunkPtr &Buffer, PrimerPtr UsePrimer)
{
	size_t Bytes = 0;

	MDObjectULList::iterator it = begin();
	MDObjectULList::iterator itend = end();
	while(it != itend)
	{
		if((*it).second->Link)
		{
			if((*it).second->GetRefType() == DICT_REF_STRONG) 
			{
				Bytes += (*it).second->Link->WriteLinkedObjects(Buffer, UsePrimer);
			}
		}
		else if(!((*it).second->empty()))
		{
			Bytes += (*it).second->WriteLinkedSubObjects(Buffer, UsePrimer);
		}
		it++;
	}

	return Bytes;
}


#ifdef OPTION3ENABLED
//! Determine the nearest baseline UL for this type
/*! The nearest baseline UL is the key of the closest type in the derevation chain to be a baseline class
 *  \ret NULL if no baseline UL found
 */
ULPtr MDOType::GetBaselineUL(void)
{
	MDOTypePtr ScanType = this;
	while(ScanType)
	{
		if(ScanType->IsBaseline()) break;
		else ScanType = ScanType->Base;
	}

	if((!ScanType) || (!ScanType->IsBaseline())) return NULL;

	return ScanType->TypeUL;
}
#endif // OPTION3ENABLED


//! Write this object to a memory buffer
/*! The object is appended to the buffer
 *	\return The number of bytes written
 */
#define DEBUG_WRITEOBJECT(x)
//#define DEBUG_WRITEOBJECT(x) x
size_t MDObject::WriteObject(DataChunkPtr &Buffer, const MDObject *ParentObject, PrimerPtr UsePrimer /*=NULL*/, UInt32 BERSize /*=0*/) const
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

			DEBUG_WRITEOBJECT( DataChunk Key; Key.Set(Buffer, Bytes); )
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
	
	// Treat value containers the same way as type containers
	if((CType == NONE) && (ValueType && (ValueType->EffectiveClass() == TYPEARRAY)))
	{
		if(ValueType->EffectiveType()->GetArrayClass() == ARRAYEXPLICIT) CType = BATCH; else CType = ARRAY;
	}

	// Build value
	if(CType == BATCH /*|| CType == ARRAY*/)
	{
		UInt32 Count = 0;
		UInt32 Size = 0;

		// Work out how many sub-items per child
		// DRAGONS: We assume < 4 billion
		UInt32 SubCount = static_cast<UInt32>(Type->GetChildList().size());
		
		// If this is a simple array then we have a single sub-type
		if(!SubCount) SubCount = 1;

		// Allocate a new data chunk with a granularity of 1k and reserve space at the start for the batch header (filled in later)
		DataChunkPtr Val = new DataChunk();
		Val->SetGranularity(1024);
		Val->Resize(8);

		// Count of remaining subs for this item
		UInt32 Subs = 0;

		MDObjectULList::const_iterator it = begin();
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

		// Write the length and batch header
		Bytes += WriteLength(Buffer, Val->Size, LenFormat, BERSize);

		// Add the batch header to the value buffer
		PutU32(Count, Val->Data);
		PutU32(Size, &Val->Data[4]);

		// Append the completed value buffer
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
		MDOTypeList::const_iterator it = Type->GetChildList().begin();

		while(it != Type->GetChildList().end())
		{
			MDObjectPtr Ptr = Child(*it);
			if(!Ptr)
			{
				error("Pack %s is missing sub-item %s\n", FullName().c_str(), (*it)->FullName().c_str());
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
 
		MDObjectULList::const_iterator it = begin();
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
	else if(IsValue && Value)
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
UInt32 MDObject::WriteKey(DataChunkPtr &Buffer, DictKeyFormat Format, PrimerPtr UsePrimer /*=NULL*/) const
{
	switch(Format)
	{
	default:
		error("Unsupported Key Format %d for %s\n", (int)Format, FullName().c_str());

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
			mxflib_assert(0);
			error("Call to WriteKey() for %s, but 1 and 4 byte tags not currently supported\n", FullName().c_str());
			return 0;
		}
	}
}


//! Make a link from this reference source to the specified target set
/*! If the target set already has a property of the target type it will be used, 
 *  otherwise one will be added.
 *  \param TargetSet A pointer to the set to be the target of this reference
 *  \param Target The UL of the target property in the target set
 *  \param ForceLink True if the link is to be made even if not a valid reference source/target pair
 *	\return true on success, else false
 *	\note The link will be made from the source <b>property</b> to the target <b>set</b>
 *		  so be aware that "this" must be a reference source property and "TargetSet"
 *		  must be a set (or pack) containing a property of Target type which is a
 *		  reference target
 */
bool MDObject::MakeRef(MDObjectPtr &TargetSet, const UL &Target, bool ForceLink /*=false*/)
{
	DataChunk TheUID_Data;

	// Does the target set already have an InstanceUID?
	MDObjectPtr TargetUID = TargetSet[Target];

	// If not add one
	if(!TargetUID)
	{
		TargetUID = TargetSet->AddChild(Target);

		// If this failed then chances are the set is not a reference target
		if(!TargetUID)
		{
			error("Attempt to reference %s from %s failed\n", FullName().c_str(), TargetSet->FullName().c_str());
			return false;
		}

		UUIDPtr NewID = new mxflib::UUID;
		TargetUID->SetData(NewID->Size(),NewID->GetValue());
		TheUID_Data = TargetUID->GetData();
	}
	else
	{
		TheUID_Data = TargetUID->GetData();
	}

	// Validate that we are a reference source
	// Note: The link will be attempted even if an error is produced
	//		 This is intentional as it may be valid in a later file spec
	TypeRef RType = GetRefType();
	if(!IsRefSource(RType))
	{
		if(!ForceLink)
		{
			warning("Attempting to reference %s from %s (which is not a reference source)\n",
				   TargetSet->FullName().c_str(), FullName().c_str() );
		}
	}

	// Make the link
	// without any flipping
	SetData( TheUID_Data.Size, TheUID_Data.Data ); 
	Link = TargetSet;
	if(RType == ClassRefStrong) OwningLink = TargetSet;

	return true;
}


//! Set an object to its distinguished value
/*! \return true if distinguished value set, else false */
bool MDObject::SetDValue(void)
{
	if(Type->GetDValue().Size == 0) return false;

	ReadValue(Type->GetDValue());
	SetModified(true);

	return true;
}


//! Is an object set to its distinguished value?
/*! \return true if distinguished value set, else false */
bool MDObject::IsDValue(void) const
{
	if(Type->GetDValue().Size == 0) return false;

	DataChunkPtr DVal = PutData();
	if(DVal->Size != Type->GetDValue().Size) return false;
	
	if(memcmp(DVal->Data, Type->GetDValue().Data, DVal->Size) == 0) return true;

	return false;
}


//! Make a copy of this object
MDObjectPtr MDObject::MakeCopy(void) const
{
	// If the object is already an "Unknown", build by value type, not type
	MDObjectPtr Ret = (ValueType && (Type->GetTypeUL()->Matches(Unknown_UL))) ? new MDObject(ValueType) : new MDObject(Type);

	// Copy any contents
	if(!empty())
	{
		MDObjectULList::const_iterator it = begin();
		if(ValueType)
		{
			// Update compound values
			if(ValueType->EffectiveClass() == COMPOUND)
			{
				while(it != end())
				{
					MDObjectPtr Sub = (*it).second->GetUL() ? Ret->Child((*it).second->GetUL()) : Ret->Child((*it).second->Name());
					if(Sub) (*Sub) = (*(*it).second); 
					else 
					{
						// DRAGONS: Some compilers don't like refs to temporary values
						MDObjectPtr NewChild = (*it).second->MakeCopy();
						Ret->AddChild(NewChild);
					}
					it++;
				}
			}
			else
			{
				while(it != end())
				{
					// DRAGONS: Some compilers don't like refs to temporary values
					MDObjectPtr NewChild = (*it).second->MakeCopy();
					Ret->AddChild(NewChild);
					it++;
				}
			}
		}
		else
		{
			// Copy set or pack children
			while(it != end())
			{
				// We replace any existing items if this is a pack
				// DRAGONS: Some compilers don't like refs to temporary values
				MDObjectPtr NewChild = (*it).second->MakeCopy();
				Ret->AddChild(NewChild, (Type->GetContainerType() == PACK));
				it++;
			}
		}
	}

	// Copy any value
	if(IsValue)
	{
		(*Ret) = (*this);
	}

	// Somewhat dangerous!!
	if(Link) 
	{
		Ret->Link = Link;
		Ret->OwningLink = OwningLink;
		if(GetRefType() == DICT_REF_STRONG)
		{
			warning("Copy made of %s which is a StrongRef!\n", FullName().c_str()); 
		}
	}

	// Copy any properties that are safe to copy
	Ret->TheUL = TheUL;
	Ret->TheTag = TheTag;
	Ret->Traits = Traits;
	Ret->ObjectName = ObjectName;

	Ret->SetModified(true);

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
	
	// Copy the data relating to content format (unless already defined)
	if(KeyFormat == DICT_KEY_UNDEFINED) KeyFormat = BaseEntry->KeyFormat;
	if(LenFormat == DICT_LEN_UNDEFINED) LenFormat = BaseEntry->LenFormat;
	minLength = BaseEntry->minLength;
	maxLength = BaseEntry->maxLength;

	// See if this derivation has swapped up from a set to a pack or vice-versa
	if((ContainerType == SET) && (KeyFormat == DICT_KEY_NONE)) ContainerType = PACK;
	else if((ContainerType == PACK) && (KeyFormat != DICT_KEY_NONE)) ContainerType = SET;

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
bool MDObject::IsA(std::string BaseType) const
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
bool MDObject::IsA(MDOTypePtr &BaseType) const
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
bool MDObject::IsA(const UL &BaseType) const
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
bool MDOType::IsA(std::string BaseType) const
{
	MDOType const *TestType = this;

	while(TestType)
	{
		if(TestType->Name() == BaseType) return true;
		TestType = TestType->Base;
	}

	return false;
}


//! Determine if this type is derived from a specified type (directly or indirectly)
bool MDOType::IsA(MDOTypePtr &BaseType) const
{
	MDOType const *TestType = this;

	while(TestType)
	{
		if(TestType == BaseType) return true;
		TestType = TestType->Base;
	}

	return false;
}
		

//! Determine if this type is derived from a specified type (directly or indirectly)
bool MDOType::IsA(const UL &BaseType) const
{
	MDOType const *TestType = this;

	while(TestType)
	{
		const ULPtr &TestUL = TestType->GetTypeUL();
		if((TestUL) && (*TestUL).Matches(BaseType)) return true;
		TestType = TestType->Base;
	}

	return false;
}
		

//! Determine if this type is known to have a child with a given UL
/*! This determines if the specified UL has been included as a child of this type in any loaded disctionary.
	*  It may be valid for children of this UL to be included, even if this function returns false 
	*/
bool MDOType::HasA(const ULPtr &ChildType) const
{
	MDOType::const_iterator it = begin();
	while(it != end())
	{
		if((*it).second->TypeUL && (*((*it).second->TypeUL) == *ChildType))
		{
			return true;
		}

		it++;
	}

	return false;
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
			MDOTypePtr Target = MDOType::Find((*it)->RefTargetName);

			if(!Target)
			{
				// Maybe the target is a UL
				ULPtr TargetUL = StringToUL((*it)->RefTargetName);
				if(TargetUL) Target = MDOType::Find(TargetUL);
			}

			// Still not found?
			if(!Target)
			{
				error("Type %s specifies an unknown reference target type of %s\n", (*it)->Name().c_str(), (*it)->RefTargetName.c_str());
			}
			else
			{
				(*it)->RefTarget = Target;
			}
		}

		it++;
	}
}




//! Change the type of an MDObject
/*! \note This may result in very wrong data - exercise great care! */
bool MDObject::ChangeType(const UL &NewType)
{
	// Set the new UL, we copy the UL here in case the original was a const
	TheUL = new UL(NewType);

	MDOTypePtr Ptr = MDOType::Find(NewType);

	if(Ptr)
	{
		Type = Ptr;
	}
	else
	{
		// As this is not a known type we have to assume that it has the same set or pack structure as the old one
		DictKeyFormat KeyFormat = Type->GetKeyFormat();
		DictLenFormat LenFormat = Type->GetLenFormat();

		// Add a new type that is based on Unknown
		Type = MDOType::Find("Unknown");

		std::string TypeName = TheUL->GetString();
		if(UL2NameFunc)	TypeName = UL2NameFunc(TheUL,NULL);
		if(Type) Type = MDOType::DeriveCopy(Type, TypeName, TypeName);

		// Ensure that the new type has the same structure as the old type
		if(Type)
		{
			Type->SetKeyFormat(KeyFormat);
			Type->SetLenFormat(LenFormat);
			
			Type->SetUL(TheUL);
		}
	}

	ObjectName = Type->Name();
	TheTag = MDOType::GetStaticPrimer()->Lookup(TheUL);

	return true;
}


//! Generate a new type based on a base type
/*! DRAGONS: Use with great care - this is intended for library code that generates inferred classes */
MDOTypePtr MDOType::DeriveCopy(MDOTypePtr &Base, std::string Name, std::string Detail)
{
	MDOTypePtr Ret = new MDOType(Base->ContainerType, Base->RootName, Name, Detail, Base->ValueType, Base->KeyFormat, Base->LenFormat, Base->minLength, Base->maxLength, Base->Use);

	if(Ret)
	{
		// Do the proper derivation
		Ret->Derive(Base);
	}

	return Ret;
}


//! Insert a new child type
std::pair<MDOType::iterator, bool> MDOType::insert(MDOTypePtr NewType) 
{ 
	std::string NewName = NewType->Name();
	std::pair<iterator, bool> Ret = MDOTypeMap::insert(MDOTypeMap::value_type(NewName, NewType));
	ChildList.push_back(NewType);
	ChildOrder.push_back(NewName);
	return Ret;
}


//! Locate a numerically indexed child
/*! DRAGONS: If the type is not numerically indexed then the index will be treated as a 0-based ChildList index */
MDOTypePtr MDOType::Child(int Index) const
{
	if((Index < 0) || (static_cast<size_t>(Index) >= ChildList.size())) return NULL;

	MDOTypeList::const_iterator it = ChildList.begin();
	while(Index--) it++;
	return *it;
}


//! Define a class from an in-memory dictionary definition
MDOTypePtr MDOType::DefineClass(ClassRecordPtr &ThisClass, SymbolSpacePtr DefaultSymbolSpace, ClassRecordList *Unresolved /*=NULL*/, MDOTypePtr Parent /*=NULL*/)
{
	// Ensure that warnings for old-style vector definitions are only given once
	static bool OldVectorWarningIssued = false;

	//! Lookup-table to convert key size to key format enum
	static const DictKeyFormat KeyConvert[] =
	{
		DICT_KEY_NONE,
		DICT_KEY_1_BYTE,
		DICT_KEY_2_BYTE,
		DICT_KEY_AUTO,
		DICT_KEY_4_BYTE,
		DICT_KEY_GLOBAL,
		DICT_KEY_UNDEFINED
	};

	//! Lookup-table to convert lenght size to len format enum
	static const DictLenFormat LenConvert[] =
	{
		DICT_LEN_NONE,
		DICT_LEN_1_BYTE,
		DICT_LEN_2_BYTE,
		DICT_LEN_BER,
		DICT_LEN_4_BYTE,
		DICT_LEN_UNDEFINED
	};

	// The symbol space to use for this class
	SymbolSpacePtr ThisSymbolSpace;
	if(ThisClass->SymSpace) ThisSymbolSpace = ThisClass->SymSpace; else ThisSymbolSpace = DefaultSymbolSpace;

	// If no parent was specified, inherit from the definition
	if(!Parent) Parent = MDOType::Find(ThisClass->Parent);

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
	if(ValidUL)
	{
		// If we are redefining a sub-item, first try to locate exactly the same sub-item
		if(Parent) Ret = Parent->Child(TypeUL);
		if(!Ret) Ret = MDOType::Find(TypeUL); 
	}
	else
	{
		Ret = MDOType::Find(RootName + ThisClass->Name, ThisSymbolSpace);
	}

	// Initially assume that we aren't extending
	bool Extending = false;

	// Work out which reference details to use - default to anything specified in this definition
	// DRAGONS: These values are processed later, but may be modified by the following code before processing
	ClassRef RefType = ThisClass->RefType;
	std::string RefTarget = ThisClass->RefTarget;

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
			MDTypePtr Type = MDType::Find(ThisClass->Base, ThisSymbolSpace, true);

			// Try UL lookup if name lookup failed
			if(!Type)
			{
				// If the name is not found, we try looking for a UL
				ULPtr TypeUL = StringToUL(ThisClass->Base);
				if(TypeUL) Type = MDType::Find(TypeUL);
			}

			if(!Type)
			{
				error("Item %s is of type %s which is not known\n", ThisClass->Name.c_str(), ThisClass->Base.c_str());
				return Ret;
			}

			MDContainerType CType = NONE;
			if(Type->GetClass() == TYPEARRAY /*&& ThisClass->Name != "Unknown"*/)
			{ 
				CType = Type->GetArrayClass() == ARRAYEXPLICIT ? BATCH : ARRAY;
			}

			// Define this type
			Ret = new MDOType(CType, RootName, ThisClass->Name, ThisClass->Detail, Type, DICT_KEY_NONE, DICT_LEN_NONE, ThisClass->MinSize, ThisClass->MaxSize, ThisClass->Usage);
		}
		// Are we defining a derived class?
		else if(ThisClass->Base.size())
		{
			MDOTypePtr BaseType = MDOType::Find(ThisClass->Base, ThisSymbolSpace, true);

			// Try UL lookup if name lookup failed
			if(!BaseType)
			{
				// If the name is not found, we try looking for a UL
				ULPtr TypeUL = StringToUL(ThisClass->Base);
				if(TypeUL) BaseType = MDOType::Find(TypeUL);
			}

			// If the base type not found quit this attempt (deliberately returning the NULL)
			if(!BaseType)
			{
				debug("Base type %s not found when trying to define %s\n", ThisClass->Base.c_str(), ThisClass->Name.c_str());
				return BaseType;
			}

			debug("Deriving %s from %s\n", ThisClass->Name.c_str(), BaseType->Name().c_str());

			// Derive the type
			Ret = new MDOType;
			if(Ret)
			{
				Ret->RootName = RootName;
				Ret->DictName = ThisClass->Name;
				
				Ret->KeyFormat = DICT_KEY_UNDEFINED;
				Ret->LenFormat = DICT_LEN_UNDEFINED;
				if((BaseType->GetContainerType() == SET) || (BaseType->GetContainerType() == PACK))
				{
					if(ThisClass->MinSize < (sizeof(KeyConvert)/sizeof(KeyConvert[0]))) Ret->KeyFormat = KeyConvert[ThisClass->MinSize];
					if(ThisClass->MaxSize < (sizeof(LenConvert)/sizeof(LenConvert[0]))) Ret->LenFormat = LenConvert[ThisClass->MaxSize];
				}
				Ret->Derive(BaseType);

				Ret->Detail = ThisClass->Detail;
				Ret->Use = ThisClass->Usage;

				// Set the name lookup - UL lookup set when key set
				NameLookup[RootName + ThisClass->Name] = Ret;
			}
		}
		else if(ThisClass->Class == ClassArray)
		{
			if(!OldVectorWarningIssued)
			{
                warning("Old style vector definitions used (e.g. for array %s)\n", ThisClass->Name.c_str());
				OldVectorWarningIssued = true;
			}

			if(ThisClass->Children.size() != 1) error("Unable to handle complex old-style array %s\n", ThisClass->Name.c_str());
			else
			{
				ClassRecordPtr &Sub = *ThisClass->Children.begin();
				MDTypePtr BaseType = MDType::Find(Sub->Base);
				if(!BaseType) error("Unable to build array of unknown item %s in %s\n", Sub->Base.c_str(), ThisClass->Name.c_str());
				else
				{
					if(Sub->RefType != TypeRefNone)
					{
						ULPtr TypeUL = RandomUL();
						MDTypePtr TargetType = MDType::AddInterpretation(Sub->Name, BaseType, TypeUL);
						TargetType->SetRefType(Sub->RefType);
						TargetType->SetRefTarget(Sub->RefTarget);
						BaseType = TargetType;
					}
					ULPtr TypeUL = RandomUL();
					MDTypePtr Type = MDType::AddArray(Sub->Name, BaseType, TypeUL);
					MDTraitsPtr Traits = MDType::LookupTraitsMapping("Default-Array");
					if(Traits) Type->SetTraits(Traits);

					Ret = new MDOType(ARRAY, RootName, ThisClass->Name, ThisClass->Detail, Type, DICT_KEY_NONE, DICT_LEN_NONE, 0, 0, ThisClass->Usage);

					// Now prevent this item being treated as a reference itself
					RefType = TypeRefNone;
					RefTarget = "";
				}
			}
		}
		else if(ThisClass->Class == ClassVector)
		{
			if(!OldVectorWarningIssued)
			{
                warning("Old style vector definitions used (e.g. for batch %s)\n", ThisClass->Name.c_str());
				OldVectorWarningIssued = true;
			}

			if(ThisClass->Children.size() != 1) error("Unable to handle complex old-style batch %s\n", ThisClass->Name.c_str());
			else
			{
				ClassRecordPtr &Sub = *ThisClass->Children.begin();
				MDTypePtr BaseType = MDType::Find(Sub->Base);
				if(!BaseType) error("Unable to build batch of unknown item %s in %s\n", Sub->Base.c_str(), ThisClass->Name.c_str());
				else
				{
					if(Sub->RefType != TypeRefNone)
					{
						ULPtr TypeUL = RandomUL();
						MDTypePtr TargetType = MDType::AddInterpretation(Sub->Name, BaseType, TypeUL);
						TargetType->SetRefType(Sub->RefType);
						TargetType->SetRefTarget(Sub->RefTarget);
						BaseType = TargetType;
					}
					ULPtr TypeUL = RandomUL();
					MDTypePtr Type = MDType::AddArray(Sub->Name, BaseType, TypeUL);
					MDTraitsPtr Traits = MDType::LookupTraitsMapping("Default-Array");
					if(Traits) Type->SetTraits(Traits);

					Ret = new MDOType(BATCH, RootName, ThisClass->Name, ThisClass->Detail, Type, DICT_KEY_NONE, DICT_LEN_NONE, 0, 0, ThisClass->Usage);

					// Now update the referencing for the container
					RefType = Sub->RefType;
					RefTarget = Sub->RefTarget;
				}
			}
		}
		else if(ThisClass->Class == ClassPack)
		{
			if(ThisClass->MaxSize > (sizeof(LenConvert)/sizeof(LenConvert[0])))
			{
				error("Item %s has an invalid length size of %u\n", ThisClass->Name.c_str(), ThisClass->MaxSize);
				return Ret;
			}
			DictLenFormat LenFormat = LenConvert[ThisClass->MaxSize];
			
			Ret = new MDOType(PACK, RootName, ThisClass->Name, ThisClass->Detail, NULL, DICT_KEY_NONE, LenFormat, 0, 0, ThisClass->Usage);
		}
		else if(ThisClass->Class == ClassSet)
		{
			if(ThisClass->MinSize > (sizeof(KeyConvert)/sizeof(KeyConvert[0])))
			{
				error("Item %s has an invalid tag size of %u\n", ThisClass->Name.c_str(), ThisClass->MinSize);
				return Ret;
			}
			DictKeyFormat KeyFormat = KeyConvert[ThisClass->MinSize];

			if(ThisClass->MaxSize > (sizeof(LenConvert)/sizeof(LenConvert[0])))
			{
				error("Item %s has an invalid length size of %u\n", ThisClass->Name.c_str(), ThisClass->MaxSize);
				return Ret;
			}
			DictLenFormat LenFormat = LenConvert[ThisClass->MaxSize];

			Ret = new MDOType(SET, RootName, ThisClass->Name, ThisClass->Detail, NULL, KeyFormat, LenFormat, 0, 0, ThisClass->Usage);
		}
		else if(ThisClass->Class == ClassExtend)
		{
			error("Item %s is a container of an unspecified type - is this an attempt to extend a set or pack that has not yet been defined?\n",
				  ThisClass->Name.c_str());
		}
		else 
		{
			error("Invalid definition for class %s\n", ThisClass->Name.c_str());
		}

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
			if(Parent->RefType != ClassRefNone)
			{
				RefType = Parent->RefType;
				RefTarget = Parent->RefTargetName;
				
				// DRAGONS: What if we have multiple subs?
				Parent->RefType = ClassRefNone;
			}

			// If we are not top level then record out "family tree"
			Ret->RootName = Parent->FullName() + "/";
		}
	}
	else
	{
		// If the parent does not match, it is possible that this is the same property being added to a different set - breaks single inheritance, but can be used with care!
		if(Parent && (Parent != Ret->Parent))
		{
			if(Ret->TypeUL)
			{
				// See if we are a child of the 'parent'
				MDOType::iterator it = Parent->begin();
				while(it != Parent->end())
				{
					if((*it).second->GetTypeUL() && (*it).second->GetTypeUL()->Matches(Ret->TypeUL)) break;
					it++;
				}

				// Not a child yet
				if(it == Parent->end())
				{
					// Add us as a child of this secondary parent
					Parent->insert(Ret);

					debug("Adding %s to secondary parent %s\n", Ret->FullName().c_str(), Parent->FullName().c_str());
				}
			}
		}
	}

	// If nothing specified this time then we use the details from the type
	if((!Extending) && Ret->ValueType)
	{
		if(RefType == ClassRefUndefined) RefType = static_cast<ClassRef>(Ret->ValueType->EffectiveRefType());
		if(RefTarget.length() == 0) RefTarget = Ret->ValueType->EffectiveRefTargetName();
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
				}
				it++;
			}
		}
		else
		{
			Ret->RefType = RefType;
			Ret->RefTargetName = RefTarget;
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

		// When the class is first defined we set whether it is baseline or not
		Ret->BaselineClass = ThisClass->IsBaseline;
	}

	// Set the default value (if one exists)
	if(ThisClass->HasDefault)
	{
		if(Ret->ValueType)
		{
			MDObjectPtr Val = new MDObject(Ret->ValueType);
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
			MDObjectPtr Val = new MDObject(Ret->ValueType);
			if(Val)
			{
				Val->SetString(ThisClass->DValue);
				DataChunkPtr Temp = Val->PutData();
				Ret->DValue.Set(Temp);
			}
		}
	}

	// Build all children - other than for arrays or vectors which are now converted to array types
	if((ThisClass->Class != ClassArray) && (ThisClass->Class != ClassVector))
	{
		ClassRecordList::iterator it = ThisClass->Children.begin();
		while(it != ThisClass->Children.end())
		{
			// Propogate the extension flag to our children
			(*it)->ExtendSubs = (*it)->ExtendSubs && ThisClass->ExtendSubs;

			MDOTypePtr Child = DefineClass(*it, Ret, ThisSymbolSpace);

			// The child was not added - possibly because it is not yet resolvable (e.g. references an as yet undefined type)
			if(!Child)
			{
				// If we don't have an unresolved list, we must abort the whole item
				if(!Unresolved) return Child;

				// Save this child item to be added later
				(*it)->Parent = TypeUL;
				Unresolved->push_back(*it);
			}
	//		if(!Child) return Child;
	//if(!Child) error("Failed to complete building this type - what to do now?\n");

			it++;
		}
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

		if(!Parent)
		{
			// If it is a top level type then add it to TopTypes as well
			TopTypes.push_back(Ret);
		}

		// Add to the list of all types
		AllTypes.push_back(Ret);
	}

	/* We need to ensure that any extension to a set or pack is also performed for all derived items,
	 * unless ThisClass->ExtendSubs = false
	 */
	if(Extending && ThisClass->ExtendSubs && (Ret->size() != 0))
	{
		MDOTypeList::iterator it = AllTypes.begin();
		while(it != AllTypes.end())
		{
			// Extend any types that are derived from our use (carefully not adding again to our use)
			if(((*it) != Ret) && ((*it)->IsA(Ret))) (*it)->ReDerive(Ret);
			it++;
		}
	}

	return Ret;
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
		
		// If name not found, try by UL
		if(!Type)
		{
			ULPtr TypeUL = StringToUL(NewBase);
			if(TypeUL) Type = MDType::Find(TypeUL);
		}

		if(!Type)
			error("Attempt to redefine %s to be of type %s which is not known\n", FullName().c_str(), NewBase.c_str());
		else
			ValueType = Type;
	}

	if(NewMinSize != 0) minLength = NewMinSize;
	if(NewMaxSize != 0) maxLength = NewMaxSize;
}


