/*! \file	partition.h
 *	\brief	Definition of Partition class
 *
 *			The Partition class holds data about a partition, either loaded 
 *          from a partition in the file or built in memory
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
#ifndef MXFLIB__PARTITION_H
#define MXFLIB__PARTITION_H

#include "primer.h"

#include <list>

namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class Partition;
	class PartitionPtr;

	//! A smart pointer to an Partition object (with operator[] overload)
	class PartitionPtr : public SmartPtr<Partition>
	{
	public:
		PartitionPtr() : SmartPtr<Partition>() {};
		PartitionPtr(Partition * ptr) : SmartPtr<Partition>(ptr) {};
//		PartitionPtr(MDObjectPtr ptr) : SmartPtr<Partition>((Partition *)ptr.GetPtr()) {};
		
		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	//! A list of smart pointers to Partition objects
	typedef std::list<PartitionPtr> PartitionList;
}

namespace mxflib
{
	//! Holds data relating to a single partition
	class Partition : public RefCount<Partition>
	{
	public:
		MDObjectPtr Object;				//!< The MDObject for this partition pack

		PrimerPtr PartitionPrimer;		//!< The Primer for this partition
										/*!< Or NULL if no primer pack active (only valid
										 *   if there is no header metadata in this partition
										 *   OR it has not yet been written)
										 */

		MDObjectList AllMetadata;		//!< List of all header metadata sets in the partition
		MDObjectList TopLevelMetadata;	//!< List of all metadata items int the partition not linked from another

	private:
		std::map<UUID, MDObjectPtr> RefTargets;				//!< Map of UUID of all reference targets to objects
		std::multimap<UUID, MDObjectPtr> UnmatchedRefs;		//!< Map of UUID of all strong or weak refs not yet linked

	public:
		Partition(const char *BaseType) { Object = new MDObject(BaseType); };
		Partition(MDOTypePtr BaseType) { Object = new MDObject(BaseType); };
		Partition(ULPtr BaseUL) { Object = new MDObject(BaseUL); };

		void AddMetadata(MDObjectPtr NewObject);

		//! Clear all header metadata for this partition (including the primer)
		void ClearMetadata(void)
		{
			PartitionPrimer = NULL;
			AllMetadata.clear();
			TopLevelMetadata.clear();
			RefTargets.clear();
			UnmatchedRefs.clear();
		}

		//! Read a full set of header metadata from this partition's source file (including primer)
		Uint64 ReadMetadata(void);

		//! Read a full set of header metadata from a file (including primer)
		Uint64 ReadMetadata(MXFFilePtr File, Uint64 Size);

		//! Read any index table segments from this partition's source file
		MDObjectListPtr ReadIndex(void);

		//! Read any index table segments from a file
		MDObjectListPtr ReadIndex(MXFFilePtr File, Uint64 Size);

//		//! Read the partition from a buffer
//		Uint32 ReadValue(const Uint8 *Buffer, Uint32 Size);

		// Access functions for the reference resolving properties
		// DRAGONS: These should be const, but can't make it work!
		std::map<UUID, MDObjectPtr>& GetRefTargets(void) { return RefTargets; };
		std::multimap<UUID, MDObjectPtr>& GetUnmatchedRefs(void) { return UnmatchedRefs;	};

	private:
		void ProcessChildRefs(MDObjectPtr ThisObject);

	public:
		// ** MDObject Interface **
		std::string Name(void) { return Object->Name(); };
		std::string FullName(void) { return Object->FullName(); };

		void SetInt(const char *ChildName, Int32 Val) { Object->SetInt(ChildName, Val); };
		void SetInt64(const char *ChildName, Int64 Val) { Object->SetInt64(ChildName, Val); };
		void SetUint(const char *ChildName, Uint32 Val) { Object->SetUint(ChildName, Val); };
		void SetUint64(const char *ChildName, Uint64 Val) { Object->SetUint64(ChildName, Val); };
		void SetString(const char *ChildName, std::string Val) { Object->SetString(ChildName, Val); };
		Int32 GetInt(const char *ChildName, Int32 Default = 0) { return Object->GetInt(ChildName, Default); };
		Int64 GetInt64(const char *ChildName, Int64 Default = 0) { return Object->GetInt64(ChildName, Default); };
		Uint32 GetUint(const char *ChildName, Uint32 Default = 0) { return Object->GetUint(ChildName, Default); };
		Uint64 GetUint64(const char *ChildName, Uint64 Default = 0) { return Object->GetUint64(ChildName, Default); };
		std::string GetString(const char *ChildName, std::string Default = "") { return Object->GetString(ChildName, Default); };
		void SetInt(MDOTypePtr ChildType, Int32 Val) { Object->SetInt(ChildType, Val); };
		void SetInt64(MDOTypePtr ChildType, Int64 Val) { Object->SetInt64(ChildType, Val); };
		void SetUint(MDOTypePtr ChildType, Uint32 Val) { Object->SetUint(ChildType, Val); };
		void SetUint64(MDOTypePtr ChildType, Uint64 Val) { Object->SetUint64(ChildType, Val); };
		void SetString(MDOTypePtr ChildType, std::string Val) { Object->SetString(ChildType, Val); };
		Int32 GetInt(MDOTypePtr ChildType, Int32 Default = 0) { return Object->GetInt(ChildType, Default); };
		Int64 GetInt64(MDOTypePtr ChildType, Int64 Default = 0) { return Object->GetInt64(ChildType, Default); };
		Uint32 GetUint(MDOTypePtr ChildType, Uint32 Default = 0) { return Object->GetUint(ChildType, Default); };
		Uint64 GetUint64(MDOTypePtr ChildType, Uint64 Default = 0) { return Object->GetUint64(ChildType, Default); };
		std::string GetString(MDOTypePtr ChildType, std::string Default = "") { return Object->GetString(ChildType, Default); };

		//! Read the object's value from a memory buffer
		Uint32 ReadValue(const Uint8 *Buffer, Uint32 Size, PrimerPtr UsePrimer = NULL) { return Object->ReadValue(Buffer, Size, UsePrimer); };
		
		MDOTypePtr GetType(void) const { return Object->GetType(); };
		MDObjectPtr GetLink(void) const { return Object->GetLink(); };
		void SetLink(MDObjectPtr NewLink) { Object->SetLink(NewLink); };
		DictRefType GetRefType(void) const { return Object->GetRefType(); };

		//! Set the parent details when an object has been read from a file
		void SetParent(MXFFilePtr File, Uint64 Location, Uint32 NewKLSize) { Object->SetParent(File, Location, NewKLSize); };

		//! Set the parent details when an object has been read from memory
		void SetParent(MDObjectPtr ParentObject, Uint64 Location, Uint32 NewKLSize) { Object->SetParent(ParentObject, Location, NewKLSize); };

		bool IsModified(void) { return Object->IsModified(); }

		Uint64 GetLocation(void) { return Object->GetLocation(); }
		std::string GetSource(void) { return Object->GetSource(); }
		std::string GetSourceLocation(void) { return Object->GetSourceLocation(); }
	};
}

// These simple inlines need to be defined after Partition
namespace mxflib
{
inline MDObjectPtr PartitionPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; };
inline MDObjectPtr PartitionPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; };
}

#endif MXFLIB__PARTITION_H


