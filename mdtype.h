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
		ARRAYCOLLECTION					//!< A collection with count and size
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
		Uint32 Number;
		std::string String;

	public:
//		MapIndex() { ASSERT(0); };
		MapIndex(Uint32 Num) { IsNum = true; Number = Num; String = Int2String(Num); };
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
		MapIndex& operator=(Uint32 Num) 
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
	private:
		std::string Name;				//!< Name of this MDType
		MDTypeClass Class;				//!< Class of this MDType
		bool Endian;					//!< Flag set to 'true' if this basic type should ever be byte-swapped
		MDArrayClass ArrayClass;		//!< Sub-class of array
		MDTraits *Traits;

	public:
		MDTypePtr Base;					//!< Base class if this is a derived class, else NULL
//		MDTypeList Children;			//!< Types contained in this if it is a compound
////		StringList ChildrenNames;		//!< Corresponding child names if it is a compound
		StringList ChildOrder;			//!< Child names in order for compound types
		int Size;						//!< The size of the item in multiples of base class items, or 0 if it is variable

		//! Access function for ContainerType
//		const MDContainerType &GetContainerType(void) { return (const MDContainerType &)ContainerType; };

	private:
		//!	Construct a basic MDType
		/*! This constructor is private so the ONLY way to create
		 *	new MDTypes from outside this class is via AddBasic() etc.
		*/
		MDType(std::string TypeName, MDTypeClass TypeClass, MDTraits *TypeTraits)
			: Name(TypeName) , Class(TypeClass) , Traits(TypeTraits)
			, Endian(false) , ArrayClass(ARRAYARRAY) {};

		//! Add a sub to a compound type
		void AddSub(std::string SubName, MDTypePtr SubType);

	public:
		//! Report the effective type of this type
		MDTypePtr EffectiveType(void);

		//! Report the effective class of this type
		MDTypeClass EffectiveClass(void);

		//! Report the effective base type of this type
		MDTypePtr EffectiveBase(void);

		//! Endian access function (set)
		void SetEndian(bool Val) { Endian = Val; };

		//! Endian access function (get)
		bool GetEndian(void) { return Endian; };

		//! ArrayClass access function (set)
		void SetArrayClass(MDArrayClass Val) { ArrayClass = Val; };

		//! ArrayClass access function (get)
		MDArrayClass GetArrayClass(void) { return ArrayClass; };

	//** Static Dictionary Handling data and functions **
	//***************************************************
	private:
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
	
		//! Set the traits for this type
		void SetTraits(MDTraits *Tr) { Traits = Tr; };

		/* Allow MDValue class to view internals of this class */
		friend class MDValue;
	};
}


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class MDValue;
	class MDValuePtr;

	//! A smart pointer to an MDValue object (with operator[] overloads)
	class MDValuePtr : public SmartPtr<MDValue>
	{
	public:
		MDValuePtr() : SmartPtr<MDValue>() {};
		MDValuePtr(MDValue * ptr) : SmartPtr<MDValue>(ptr) {};
		
		//! Child access operator that overcomes dereferencing problems with SmartPtrs
		MDValuePtr operator[](int Index);

		//! Child access operator that overcomes dereferencing problems with SmartPtrs
		MDValuePtr operator[](const std::string ChildName);
	};

	//! A list of smart pointers to MDValue objects
	typedef std::list<MDValuePtr> MDValueList;

	//! A list of smart pointers to MDType objects with names
//	typedef std::pair<MDValuePtr, std::string> MDNamedValue;
//	typedef std::list<MDNamedValue> MDNamedValueList;

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
//		Uint8 *Data;				// DRAGONS: This should be a DataChunk

	public:
//		MDValueList Children;

	public:
		MDValue(const std::string &BaseType);
		MDValue(MDTypePtr BaseType);
		void Init(void);
//		~MDValue();
~MDValue() {}; // ## DRAGONS: For debug ONLY!!

		void AddChild(MDValuePtr Child, int Index = -1);
		void Resize(Uint32 Index);

		MDValuePtr operator[](int Index);
		MDValuePtr Child(int Index) { return operator[](Index); };

		//! Access function for child values of compound items
		MDValuePtr operator[](const std::string ChildName);
		MDValuePtr Child(const std::string ChildName) { return operator[](ChildName); };

//		std::string ChildName(int Child);

		void SetInt(Int32 Val) { Type->Traits->SetInt(this, Val); };
		void SetInt64(Int64 Val) { Type->Traits->SetInt64(this, Val); };
		void SetUint(Uint32 Val) { Type->Traits->SetUint(this, Val); };
		void SetUint64(Uint64 Val) { Type->Traits->SetUint64(this, Val); };
		void SetString(std::string Val)	{ Type->Traits->SetString(this, Val); };
		Int32 GetInt(void) { return Type->Traits->GetInt(this); };
		Int64 GetInt64(void) { return Type->Traits->GetInt64(this); };
		Uint32 GetUint(void) { return Type->Traits->GetUint(this); };
		Uint64 GetUint64(void) { return Type->Traits->GetUint64(this); };
		std::string GetString(void)	{ return Type->Traits->GetString(this); };

		// Child value access
		// DRAGONS: May need to add code to check inside "optimised" compounds
		Int32 GetInt(const char *ChildName, Int32 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetInt(); else return Default; };
		Int64 GetInt64(const char *ChildName, Int64 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetInt64(); else return Default; };
		Uint32 GetUint(const char *ChildName, Uint32 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetUint(); else return Default; };
		Uint64 GetUint64(const char *ChildName, Uint64 Default = 0) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetUint64(); else return Default; };
		std::string GetString(const char *ChildName, std::string Default = "") { MDValuePtr Ptr = operator[](ChildName); if (Ptr) return Ptr->GetString(); else return Default; };
		void SetInt(const char *ChildName, Int32 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetInt(Val); };
		void SetInt64(const char *ChildName, Int64 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetInt64(Val); };
		void SetUint(const char *ChildName, Uint32 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetUint(Val); };
		void SetUint64(const char *ChildName, Uint64 Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetUint64(Val); };
		void SetString(const char *ChildName, std::string Val) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->SetString(Val); };
		
		void ReadValue(const char *ChildName, const DataChunk &Source) { MDValuePtr Ptr = operator[](ChildName); if (Ptr) Ptr->ReadValue(Source); };

		// DRAGONS: These should probably be private and give access via MDTraits
		// to prevent users tinkering!
		Uint32 MakeSize(Uint32 NewSize);

		Uint32 ReadValue(const DataChunk &Chunk) { return ReadValue(Chunk.Data, Chunk.Size); };
		Uint32 ReadValue(const Uint8 *Buffer, Uint32 Size, int Count=0);

		//! Get a reference to the data chunk (const to prevent setting!!)
		const DataChunk& GetData(void) { return (const DataChunk&) Data; };

		//! Build a data chunk with all this items data (including child data)
		const DataChunk PutData(void) 
		{
			DataChunk Ret;
			if(size() == 0) 
			{
				Ret = GetData();
			}
			else
			{
				// Compounds must be written in the correct order
				if(Type->EffectiveClass() == COMPOUND)
				{
					StringList::iterator it = Type->ChildOrder.begin();
					while(it != Type->ChildOrder.end())
					{
						DataChunk SubItem = Child(*it)->PutData();
						Ret.Set(SubItem.Size, SubItem.Data, Ret.Size);
						it++;
					}
				}
				else
				{
					MDValue::iterator it = begin();
					while(it != end())
					{
						DataChunk SubItem = (*it).second->PutData();
						Ret.Set(SubItem.Size, SubItem.Data, Ret.Size);
						it++;
					}
				}
			}

			return Ret;
		};

		//! Set data into the datachunk
		// DRAGONS: This is dangerous!!
		void SetData(Uint32 MemSize, const Uint8 *Buffer) 
		{ 
			Data.Resize(MemSize); 
			Data.Set(MemSize, Buffer); 
		};

		// Report the name of this item (the name of its type)
		std::string Name(void) { ASSERT(Type); return Type->Name; };

		// Type access function
		MDTypePtr GetType(void) { return Type; };
		MDTypePtr EffectiveType(void) { return Type->EffectiveType(); };
		MDTypePtr EffectiveBase(void) { return Type->EffectiveBase(); };
	};
}


// These simple inlines need to be defined after MDValue
namespace mxflib
{
inline MDValuePtr MDValuePtr::operator[](int Index) { return operator->()->operator[](Index); };
inline MDValuePtr MDValuePtr::operator[](const std::string ChildName) { return operator->()->operator[](ChildName); };
}


#endif MXFLIB__MDTYPE_H

