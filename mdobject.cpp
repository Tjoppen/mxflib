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
	// Load the KLVLib dictionary
	MainDict = LoadXMLDictionary(DictFile);

	if(MainDict == NULL)
	{
		error("Couldn't open dictionary file \"%s\"\n", DictFile);
		return;
	}

	// DRAGONS: Debug code in here to dump the top level
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

	// DRAGONS: More debug!
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


//! Convert KLVLib "DictType" enum to text string of type name
/*! /ret Pointer to a string constant
 *  /ret "" if the DictType is unknown or is a container (e.g. a pack)
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
		XLate.insert(XLateType::value_type(DICT_TYPE_VERTYPE,"VerType"));
		XLate.insert(XLateType::value_type(DICT_TYPE_RATIONAL,"Rational"));
		XLate.insert(XLateType::value_type(DICT_TYPE_BOOLEAN,"Boolean"));
		XLate.insert(XLateType::value_type(DICT_TYPE_ISO7STRING,"ISO7String"));
		XLate.insert(XLateType::value_type(DICT_TYPE_UTF16STRING,"UTF16String"));
		XLate.insert(XLateType::value_type(DICT_TYPE_IEEEFLOAT64,"Float64"));
		XLate.insert(XLateType::value_type(DICT_TYPE_UINT8STRING,"Uint8Array")); // DRAGONS: Is this right?
		XLate.insert(XLateType::value_type(DICT_TYPE_PRODUCTVERSION,"ProductVersion"));
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
			// If it is a ref this is more important than the UUID type
			if(Dict->RefType == DICT_REF_STRONG)
				Type = "StrongRef";
			else if(Dict->RefType == DICT_REF_WEAK)
				Type = "WeakRef";
			else
				Type = DictType2Text(Dict->Children->Link->Type);

			if(Type != "")
			{
				if(Dict->Type == DICT_TYPE_VECTOR) Type += "Collection";
				else Type += "Array";
			}
		}
		else
		{
			// If more complex type use the vector properties name as the type
			Type = Dict->Name;
		}
	}
	else
	{
		// Not a vector or array, look up the type
		Type = DictType2Text(Dict->Type);
	}

printf("%s is a %s\n", Dict->Name, Type.c_str());

	if(Type != "")
	{
		NewType->ValueType = MDType::Find(Type.c_str());
		if(!NewType->ValueType)
		{
			std::string Temp = BaseName;
			Temp += Dict->Name;
			warning("Object type \"%s\" is of unknown type \"%s\"\n", Temp.c_str(), Type.c_str());

			// Add either a dummy basic, or a dummy raw array
			if((Dict->minLength > 0) && (Dict->minLength == Dict->maxLength))
			{
				NewType->ValueType = MDType::AddBasic(Type, Dict->minLength);
			}
			else
			{
				NewType->ValueType = MDType::AddArray(Type, MDType::Find("Uint8"));
			}
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

	// Set the lookups
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

	//! Determine the container type
	if( (RootDict->Type == DICT_TYPE_UNIVERSAL_SET) 
	  ||(RootDict->Type == DICT_TYPE_LOCAL_SET) )
	{
		ContainerType = SET;
	}
	else if( (RootDict->Type == DICT_TYPE_UNIVERSAL_SET) 
		   ||(RootDict->Type == DICT_TYPE_LOCAL_SET) )
	{
		ContainerType = PACK;
	}
	else if(RootDict->Type == DICT_TYPE_VECTOR)
	{
		ContainerType = VECTOR;
	}
	else if(RootDict->Type == DICT_TYPE_ARRAY)
	{
		ContainerType = ARRAY;
	}
	else
	{
		ContainerType = NONE;
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


//! MDObject named constructor
/*! Builds a "blank" metadata object of a named type
*/
MDObject::MDObject(const char *BaseType)
{
	Type = MDOType::Find(BaseType);
	if(!Type)
	{
		error("Metadata object type \"%s\" doesn't exist\n", BaseType);
		// DRAGONS: Must sort this!!
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


//! Second part of MDObject constructors
/*! Builds a "blank" metadata object
*/
void MDObject::Init(void)
{
	// If it isn't a container build the basic item
	switch(Type->GetContainerType())
	{
	case NONE:
	case VECTOR:
	case ARRAY:
		{
			Value = new MDValue(Type->ValueType);

			if(Type->ValueType->EffectiveClass() == ARRAY)
			{
				// Build the minimum size array
				Value->ResizeChildren(Type->GetDict()->minLength);
			}
		}
		break;

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
 */
MDObjectPtr MDObject::AddChild(const char *ChildName)
{
	StringList::iterator it = ChildrenNames.begin();
	MDObjectList::iterator it2 = Children.begin();

	// Try and find an existing child
	MDObjectPtr Ret = Child(ChildName);

	// Only add a new one if we didn't find it
	if(!Ret)
	{
		it = Type->ChildrenNames.begin();
		MDOTypeList::iterator it3 = Type->Children.begin();
	
		while(it != Type->ChildrenNames.end())
		{
			ASSERT(it3 != Type->Children.end());

			if(strcmp((*it).c_str(),ChildName) == 0)
			{
				// Insert a new item of the correct type at the end
				MDObjectPtr Ret = new MDObject(*it3);
				Children.push_back(Ret);
				ChildrenNames.push_back(*it);

				// Return smart pointer to the new object
				return Ret;
			}
			it++;
			it3++;
		}
	}

	return Ret;
};

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


//** Static Instantiations for MDOType class **
//*********************************************

MDOTypeList MDOType::AllTypes;	//!< All types managed by the MDOType class
MDOTypeList MDOType::TopTypes;	//!< The top-level types managed by the MDOType class

//! Map for reverse lookups based on DictEntry pointer
std::map<DictEntry*, MDOTypePtr> MDOType::DictLookup;

//! Map for reverse lookups based on type name
std::map<std::string, MDOTypePtr> MDOType::NameLookup;

MDOType::DictManager MDOType::DictMan;		//!< Dictionary manager

