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
	class Partition : public MDObject
	{
	public:

		PrimerPtr PartitionPrimer;	//!< The Primer for this partition
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
		Partition(const char *BaseType) : MDObject(BaseType) {};
		Partition(MDOTypePtr BaseType) : MDObject(BaseType) {};
		Partition(ULPtr BaseUL) : MDObject(BaseUL) {};

		void AddMetadata(MDObjectPtr Object);

		//! Clear all header metadata for this partition (including the primer)
		void ClearMetadata(void)
		{
			PartitionPrimer = NULL;
			AllMetadata.clear();
			TopLevelMetadata.clear();
			RefTargets.clear();
			UnmatchedRefs.clear();
		}

		//! Read a full set of header metadata from a file (including primer)
		Uint64 ReadMetadata(MXFFilePtr File, Uint64 Size);

//		//! Read the partition from a buffer
//		Uint32 ReadValue(const Uint8 *Buffer, Uint32 Size);

		// Access functions for the reference resolving properties
		// DRAGONS: These should be const, but can't make it work!
		std::map<UUID, MDObjectPtr>& GetRefTargets(void) { return RefTargets; };
		std::multimap<UUID, MDObjectPtr>& GetUnmatchedRefs(void) { return UnmatchedRefs;	};

	private:
		void ProcessChildRefs(MDObjectPtr Object);
	};
}

// These simple inlines need to be defined after Partition
namespace mxflib
{
inline MDObjectPtr PartitionPtr::operator[](const char *ChildName) { return GetPtr()->operator[](ChildName); };
inline MDObjectPtr PartitionPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->operator[](ChildType); };
}

#endif MXFLIB__PARTITION_H


