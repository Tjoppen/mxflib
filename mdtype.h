/*! \file	mdtype.h
 *	\brief	Definition of classes that define metadata type info
 *
 *			Class MDDict holds the overall dictionary definitions and
 *          manages loading them from a dictionary file and adding new
 *			metadata types.
 *<br><br>
 *			Class MDType holds info about a specific metadata type
 *<br><br>
 *			These classes are currently wrappers around KLVLib structures
 */
/*
 *	Copyright (c) 2002, Matt Beard
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
#ifndef MXFLIB__MDTYPE_H
#define MXFLIB__MDTYPE_H

// STL Includes
#include <string>
#include <list>
#include <map>

namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDType;

	//! A smart pointer to an MDType object
	typedef SmartPtr<MDType> MDTypePtr;

	//! A list of smart pointers to MDType objects
	typedef std::list<MDTypePtr> MDTypeList;
}

namespace mxflib
{
	enum MDContainerType				//!< Container types
	{ 
		NONE,							//!< Not a container - a simple metadata item
		SET,							//!< A SMPTE-336M Set
		PACK,							//!< A SMPTE-336M Pack
		VECTOR,							//!< A vector (ordered or unordered)
		ARRAY							//!< An array
	};
	enum MDTypeClass					//!< Class of this type
	{ 
		BASIC,							//!< A basic, indivisible, type
		INTERPRETATION,					//!< An interpretation of another class
		TYPEARRAY,						//!< An array of another class
		COMPOUND,						//!< A compound type
		SUB								//!< A sub member of a compound type
	};
}

namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDType;

	//! A smart pointer to an MDType object
	typedef SmartPtr<MDType> MDTypePtr;

	//! A list of smart pointers to MDType objects
	typedef std::list<MDTypePtr> MDTypeList;
}


namespace mxflib
{
	//! Holds the definition of a metadata type
	class MDType
	{
	private:
		std::string Name;				//!< Name of this MDType
		MDTypeClass Class;				//!< Class of this MDType

	public:
		MDTypePtr Base;					//!< Base class if this is a derived class, else NULL
		MDTypeList Children;			//!< Types contained in this if it is a compound
		int Size;						//!< The size of the item multiples of base class items, or 0 if it is variable

		//! Access function for ContainerType
//		const MDContainerType &GetContainerType(void) { return (const MDContainerType &)ContainerType; };

	private:
		//!	Construct a basic MDType
		/*! This constructor is private so the ONLY way to create
		 *	new MDOTypes from outside this class is via AddBasic() etc.
		*/
		MDType(std::string TypeName, MDTypeClass TypeClass, MDTraits *TypeTraits)
			: Name(TypeName) , Class(TypeClass) , Traits(TypeTraits) {};

		//! Add a sub to a compound type
		void AddSub(std::string SubName, MDTypePtr SubType);


	//** Static Dictionary Handling data and functions **
	//***************************************************
	private:
		static MDTypeList Types;		//!< All types managed by this object

		//! Map for reverse lookups based on type name
		static std::map<std::string, MDTypePtr> NameLookup;

	public:
		//! Add a new basic type
		static void AddBasic(std::string TypeName, int TypeSize);

		//! Add a new interpretation type
		static void AddInterpretation(std::string TypeName, MDTypePtr BaseType);

		//! Add a new array type
		static void AddArray(std::string TypeName, MDTypePtr BaseType, int ArraySize = 0);

		//! Add a new compound type
		static void AddCompound(std::string TypeName);

		static MDTypePtr Find(const char *TypeName);
	
//! DRAGONS: Experimental
MDTraits *Traits;
void SetTraits(MDTraits *Tr) { Traits = Tr; };

		/* Allow MDValue class to view internals of this class */
		friend class MDValue;
	};
}


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDValue;

	//! A smart pointer to an MDValue object
	typedef SmartPtr<MDValue> MDValuePtr;

	//! A list of smart pointers to MDValue objects
	typedef std::list<MDValuePtr> MDValueList;
}


namespace mxflib
{
	//! Metadata Object class
	class MDValue
	{
	private:
		MDType *Type;
		int Size;
		Uint8 *Data;

		MDValueList Children;

	public:
		MDValue(const char *BaseType);
		MDValue(MDType *BaseType);
		void Init(void);
//		~MDValue();
~MDValue() {}; // ## DRAGONS: For debug ONLY!!

		//! Access function for child values of compound items
		MDValue &Child(const char *ChildName);

		void SetInt(Int32 Val) { Type->Traits->SetInt(this, Val); };
		void SetInt64(Int64 Val) { Type->Traits->SetInt64(this, Val); };
		void SetUint(Uint32 Val) { Type->Traits->SetUint(this, Val); };
		void SetUint64(Uint64 Val) { Type->Traits->SetUint64(this, Val); };
		void SetString(std::string Val)	{ Type->Traits->SetString(this, Val); };
		Int32 GetInt(void) { return Type->Traits->GetInt(this); };
		Int64 GetInt64(void) { return Type->Traits->GetInt64(this); };
		Uint32 GetUint(void) { return Type->Traits->GetUint(this); };
		Uint64 Gint64(void) { return Type->Traits->GetUint64(this); };
		std::string GetString(void)	{ return Type->Traits->GetString(this); };

		int GetSize(void) { return Size; };
		
		// DRAGONS: These should probably be private and give access via MDTraits
		// to prevent users tinkering!
		void MakeSize(int NewSize);

		void SetValue(int ValSize, Uint8 *Val);

		// Get a pointer to the data buffer (const to prevent setting!!)
		const Uint8* GetData(void) { return (const Uint8*) Data; };
	};
}

#endif MXFLIB__MDTYPE_H

