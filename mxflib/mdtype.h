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
 *
 *	\version $Id: mdtype.h,v 1.8 2006/02/11 18:08:43 matt-beard Exp $
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
#ifndef MXFLIB__MDTYPE_H
#define MXFLIB__MDTYPE_H

// STL Includes
#include <string>
#include <list>
#include <map>

namespace mxflib
{
	typedef std::list<std::string> StringList;
}

namespace mxflib
{
	enum MDContainerType				//!< Container types
	{ 
		NONE,							//!< Not a container - a simple metadata item
		SET,							//!< A SMPTE-336M Set
		PACK,							//!< A SMPTE-336M Pack
		BATCH,							//!< A Batch (ordered or unordered)
		ARRAY							//!< An array
	};
	enum MDTypeClass					//!< Class of this type
	{ 
		BASIC,							//!< A basic, indivisible, type
		INTERPRETATION,					//!< An interpretation of another class
		TYPEARRAY,						//!< An array of another class
		COMPOUND						//!< A compound type
	};
	enum MDArrayClass					//!< Sub-classes of arrays
	{
		ARRAYARRAY,						//!< Just a normal array
		ARRAYBATCH						//!< A batch with count and size
	};
}


namespace mxflib
{
	//! Number/String duality object for index item in objects
	/*! Number facet is used for arrays, String facet for compounds
	 */
	class MapIndex
	{
	public:
		bool IsNum;
		UInt32 Number;
		std::string String;

	public:
//		MapIndex() { ASSERT(0); };
		MapIndex(UInt32 Num) { IsNum = true; Number = Num; String = Int2String(Num); };
		MapIndex(std::string Str) { IsNum = false; String = Str; };
		MapIndex(const MapIndex& Map) 
		{ IsNum = Map.IsNum; 
			if(IsNum) Number = Map.Number; 
			String = Map.String; 
		};

		bool operator<(const MapIndex& Other) const
		{
			if((IsNum==true) && (Other.IsNum==true)) return (Number < Other.Number);
			if((IsNum==false) && (Other.IsNum==false)) return (String < Other.String);
			
			// Numbers come before strings
			return Other.IsNum;
		}

		//! Allocator which does not change underlying type
		MapIndex& operator=(UInt32 Num) 
		{ 
			if(IsNum) { Number = Num; String = Int2String(Num); } 
			return *this;
		};

		//! Allocator which does not change underlying type
		MapIndex& operator=(std::string Str) 
		{ 
			if(IsNum) 
			{
				Number = atoi(Str.c_str());
				String = Int2String(Number);		// Reformat the string
			}
			else
			{
				String = Str;
			}
			return *this;
		};

		//! Allocator which does not change underlying type
		MapIndex& operator=(const MapIndex& Map)
		{ 
			if(IsNum)
			{
				Number = atoi(Map.String.c_str());	// Will be zero in !Map.IsNum
				String = Int2String(Number);		// Reformat the string
			}
			else
			{
				String = Map.String;
			}
			return *this;
		};

		const char *c_str() const
		{
			return (const char *)String.c_str();
		}
	};
}


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDType;

	//! A smart pointer to an MDType object
	typedef SmartPtr<MDType> MDTypePtr;

	//! A parent pointer to an MDType object
	typedef ParentPtr<MDType> MDTypeParent;

	//! A list of smart pointers to MDType objects
	typedef std::list<MDTypePtr> MDTypeList;

	//! A list of smart pointers to MDType objects with names
//	typedef std::pair<MDTypePtr, std::string> MDNamedType;
//	typedef std::list<MDNamedType> MDNamedTypeList;

	typedef std::map<std::string, MDTypePtr> MDTypeMap;
}


namespace mxflib
{
	//! Holds the definition of a metadata type
	class MDType : public RefCount<MDType> , public MDTypeMap
	{
	protected:
		std::string TypeName;			//!< Name of this MDType
		MDTypeClass Class;				//!< Class of this MDType
		MDArrayClass ArrayClass;		//!< Sub-class of array
		MDTraitsPtr Traits;				//!< Traits for this MDType
		bool Endian;					//!< Flag set to 'true' if this basic type should ever be byte-swapped

	public:
		MDTypeParent Base;					//!< Base class if this is a derived class, else NULL
//		MDTypeList Children;			//!< Types contained in this if it is a compound
////		StringList ChildrenNames;		//!< Corresponding child names if it is a compound
		StringList ChildOrder;			//!< Child names in order for compound types
		int Size;						//!< The size of the item in multiples of base class items, or 0 if it is variable

		//! Access function for ContainerType
//		const MDContainerType &GetContainerType(void) { return (const MDContainerType &)ContainerType; };

	protected:
		//!	Construct a basic MDType
		/*! This constructor is private so the ONLY way to create
		 *	new MDTypes from outside this class is via AddBasic() etc.
		*/
		MDType(std::string TypeName, MDTypeClass TypeClass, MDTraitsPtr TypeTraits)
			: TypeName(TypeName) , Class(TypeClass) , ArrayClass(ARRAYARRAY) , Traits(TypeTraits) , Endian(false) 
		{
			// Ensure that the traits are initialized
//			TypeTraits->Init();
		};
 
		//! Prevent auto construction by NOT having an implementation to this constructor
		MDType();

		//! Prevent copy construction by NOT having an implementation to this copy constructor
		MDType(const MDType &rhs);

		//! Add a sub to a compound type
		void AddSub(std::string SubName, MDTypePtr SubType);

	public:
		//! Report the effective type of this type
		MDTypePtr EffectiveType(void);

		//! Report the effective class of this type
		MDTypeClass EffectiveClass(void) const;

		//! Report the effective base type of this type
		MDTypePtr EffectiveBase(void) const;

		//! Report the effective size of this type
		/*! \return The size in bytes of a single instance of this type, or 0 if variable size
		 */
		UInt32 EffectiveSize(void) const;

		//! Does this value's trait take control of all sub-data and build values in the our own DataChunk?
		/*! Normally any contained sub-types (such as array items or compound members) hold their own data */
		bool HandlesSubdata(void) const
		{
			if(Traits) return Traits->HandlesSubdata();
			return false;
		}

		//! Endian access function (set)
		void SetEndian(bool Val) { Endian = Val; };

		//! Endian access function (get)
		bool GetEndian(void) { return Endian; };

		//! ArrayClass access function (set)
		void SetArrayClass(MDArrayClass Val) { ArrayClass = Val; };

		//! ArrayClass access function (get)
		MDArrayClass GetArrayClass(void) { return ArrayClass; };

		//! Get the name of this type
		const std::string &Name(void) const { return TypeName; }

	//** Static Dictionary Handling data and functions **
	//***************************************************
	protected:
		static MDTypeList Types;		//!< All types managed by this object

		//! Map for reverse lookups based on type name
		static std::map<std::string, MDTypePtr> NameLookup;

	public:
		//! Add a new basic type
		static MDTypePtr AddBasic(std::string TypeName, int TypeSize);

		//! Add a new interpretation type
		static MDTypePtr AddInterpretation(std::string TypeName, MDTypePtr BaseType, int Size = 0);

		//! Add a new array type
		static MDTypePtr AddArray(std::string TypeName, MDTypePtr BaseType, int ArraySize = 0);

		//! Add a new compound type
		static MDTypePtr AddCompound(std::string TypeName);

		static MDTypePtr Find(const std::string& TypeName);
	

	/* Traits handling */
	/*******************/

	public:
		//! Set the traits for this type
		void SetTraits(MDTraitsPtr Tr) 
		{ 
			Traits = Tr; 
			
			// Ensure that the traits are initialized
//			Tr->Init();
		};

		//! Access the traits for this type
		MDTraitsPtr GetTraits(void) const { return Traits; };

	protected:
		//! Type to map type names to their handling traits
		typedef std::map<std::string,MDTraitsPtr> TraitsMapType;

		//! Map of type names to thair handling traits
		static TraitsMapType TraitsMap;

	public:
		//! Add a mapping to be applied to all types of a given type name
		/*! \note This will act retrospectively
		 */
		static bool AddTraitsMapping(std::string TypeName, std::string TraitsName);

		//! Update an existing mapping and apply to any existing type of the given name
		static bool UpdateTraitsMapping(std::string TypeName, std::string TraitsName);

		//! Lookup the traits for a specified type name
		/*! If no traits have been defined for the specified type the traits with the name given in DefaultTraitsName is used (if specified)
		 */
		static MDTraitsPtr LookupTraitsMapping(std::string TypeName, std::string DefaultTraitsName = "");

		/* Allow MDValue class to view internals of this class */
		friend class MDValue;
	};
}

namespace mxflib
{
	//! Add a mapping to apply a given set of traits to a certain type
	/*! \ret The name of the traits
		*/
	template<class C> std::string AddTraitsMapping(std::string TypeName)
	{
		MDTraitsPtr Tr = new C;
		MDTraitsPtr TrLookup = MDTraits::Find(Tr->Name());

		if(!TrLookup) MDTraits::Add(Tr->Name(), Tr);

		if(MDType::AddTraitsMapping(TypeName, Tr->Name()))
			return Tr->Name();
		else
			return "";
	}


	//! Update an existing mapping and apply to any existing type of the given name
	/*! \ret The name of the traits
		*/
	template<class C> std::string UpdateTraitsMapping(std::string TypeName)
	{
		MDTraitsPtr Tr = new C;
		MDTraitsPtr TrLookup = MDTraits::Find(Tr->Name());

		if(!TrLookup) MDTraits::Add(Tr->Name(), Tr);

		if(MDType::UpdateTraitsMapping(TypeName, Tr->Name()))
			return Tr->Name();
		else
			return "";
	}
}


namespace mxflib
{
	//! A map of smart pointers to MDValue objects indexed by MapIndex
	typedef std::map<MapIndex, MDValuePtr> MDValueMap;
}


namespace mxflib
{
	//! Metadata Object class
	class MDValue : public RefCount<MDValue>, public MDValueMap
	{
	private:
		MDTypePtr Type;
		DataChunk Data;
//		int Size;
//		UInt8 *Data;				// DRAGONS: This should be a DataChunk

	public:
//		MDValueList Children;

	public:
		MDValue(const std::string &BaseType);
		MDValue(MDTypePtr BaseType);
		void Init(void);
//		~MDValue();
~MDValue() {}; // ## DRAGONS: For debug ONLY!!

		void AddChild(MDValuePtr Child, int Index = -1);
		void Resize(UInt32 Index);

		MDValuePtr operator[](int Index);
		MDValuePtr Child(int Index) { return operator[](Index); };

		//! Access function for child values of compound items
		MDValuePtr operator[](const std::string ChildName);
		MDValuePtr Child(const std::string ChildName) { return operator[](ChildName); };

//		std::string ChildName(int Child);

		void SetInt(Int32 Val) { Type->Traits->SetInt(this, Val); };
		void SetInt64(Int64 Val) { Type->Traits->SetInt64(this, Val); };
		void SetUInt(UInt32 Val) { Type->Traits->SetUInt(this, Val); };
		void SetUInt64(UInt64 Val) { Type->Traits->SetUInt64(this, Val); };
		void SetUint(UInt32 Val) { Type->Traits->SetUInt(this, Val); };
		void SetUint64(UInt64 Val) { Type->Traits->SetUInt64(this, Val); };
		void SetString(std::string Val)	{ Type->Traits->SetString(this, Val); };
		Int32 GetInt(void) { return Type->Traits->GetInt(this); };
		Int64 GetInt64(void) { return Type->Traits->GetInt64(this); };
		UInt32 GetUInt(void) { return Type->Traits->GetUInt(this); };
		UInt64 GetUInt64(void) { return Type->Traits->GetUInt64(this); };
		UInt32 GetUint(void) { return Type->Traits->GetUInt(this); };
		UInt64 GetUint64(void) { return Type->Traits->GetUInt64(this); };
		std::string GetString(void)	{ return Type->Traits->GetString(this); };

		// Child value access
		// DRAGONS: May need to add code to check inside "optimised" compounds
		Int32 GetInt(const char *ChildName, Int32 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetInt(); else return Default; };
		Int64 GetInt64(const char *ChildName, Int64 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetInt64(); else return Default; };
		UInt32 GetUInt(const char *ChildName, UInt32 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetUInt(); else return Default; };
		UInt64 GetUInt64(const char *ChildName, UInt64 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetUInt64(); else return Default; };
		UInt32 GetUint(const char *ChildName, UInt32 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetUInt(); else return Default; };
		UInt64 GetUint64(const char *ChildName, UInt64 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetUInt64(); else return Default; };
		std::string GetString(const char *ChildName, std::string Default = "") { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetString(); else return Default; };
		void SetInt(const char *ChildName, Int32 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetInt(Val); };
		void SetInt64(const char *ChildName, Int64 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetInt64(Val); };
		void SetUInt(const char *ChildName, UInt32 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetUInt(Val); };
		void SetUInt64(const char *ChildName, UInt64 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetUInt64(Val); };
		void SetUint(const char *ChildName, UInt32 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetUInt(Val); };
		void SetUint64(const char *ChildName, UInt64 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetUInt64(Val); };
		void SetString(const char *ChildName, std::string Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetString(Val); };
		
		void ReadValue(const char *ChildName, const DataChunk &Source) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->ReadValue(Source); };

		void ReadValue(const char *ChildName, DataChunkPtr &Source) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->ReadValue(Source); };

		// DRAGONS: These should probably be private and give access via MDTraits
		// to prevent users tinkering!
		UInt32 MakeSize(UInt32 NewSize);

		UInt32 ReadValue(const DataChunk &Chunk) { return ReadValue(Chunk.Data, Chunk.Size); };
		UInt32 ReadValue(DataChunkPtr &Chunk) { return ReadValue(Chunk->Data, Chunk->Size); };
		UInt32 ReadValue(const UInt8 *Buffer, UInt32 Size, int Count=0);

		//! Get a reference to the data chunk (const to prevent setting!!)
		const DataChunk& GetData(void) { return (const DataChunk&) Data; };

		//! Build a data chunk with all this items data (including child data)
		DataChunkPtr PutData(void);

		//! Set data into the datachunk
		// DRAGONS: This is dangerous!!
		void SetData(UInt32 MemSize, const UInt8 *Buffer) 
		{ 
			Data.Resize(MemSize); 
			Data.Set(MemSize, Buffer); 
		};

		// Report the name of this item (the name of its type)
		const std::string &Name(void) const { ASSERT(Type); return Type->TypeName; };

		// Type access function
		MDTypePtr GetType(void) { return Type; };
		MDTypePtr EffectiveType(void) { return Type->EffectiveType(); };
		MDTypePtr EffectiveBase(void) { return Type->EffectiveBase(); };
	};
}


// These simple inlines need to be defined after MDValue
namespace mxflib
{
	inline MDValuePtr MDValuePtr::operator[](int Index) 
	{ 
		// TODO: We need to find a solution to this!
		ASSERT(!(operator->()->GetType()->HandlesSubdata()));
		return operator->()->operator[](Index); 
	};
	inline MDValuePtr MDValuePtr::operator[](const std::string ChildName) 
	{ 
		// TODO: We need to find a solution to this!
		ASSERT(!(operator->()->GetType()->HandlesSubdata()));
		return operator->()->operator[](ChildName); 
	};
}


#endif // MXFLIB__MDTYPE_H

