/*! \file	forward.h
 *	\brief	Various forward declarations
 *
 *	\version $Id: forward.h,v 1.7 2011/01/10 10:42:09 matt-beard Exp $
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

#ifndef MXFLIB__FORWARD_H
#define MXFLIB__FORWARD_H

// Many of the declarations are of smart pointers
#include "mxflib/smartptr.h"

// STL includes
#include <list>


namespace mxflib 
{
	class MXFFile;
	typedef SmartPtr<MXFFile> MXFFilePtr;					//!< A smart pointer to an MXFFile object
	typedef ParentPtr<MXFFile> MXFFileParent;				//!< A parent pointer to an MXFFile object

	// Forward declare so the class can include pointers to itself
	class KLVObject;

	//! A smart pointer to a KLVObject object
	typedef SmartPtr<KLVObject> KLVObjectPtr;


	/* Forward refs for index tables */
	
	class IndexTable;

	//! Smart pointer to an index table
	typedef SmartPtr<IndexTable> IndexTablePtr;
	
	//! Parent pointer to an index table
	typedef ParentPtr<IndexTable> IndexTableParent;

	class IndexSegment;

	//! Smart pointer to an index table segment
	typedef SmartPtr<IndexSegment> IndexSegmentPtr;

	//! List of smart pointers to index table segments
	typedef std::list<IndexSegmentPtr> IndexSegmentList;


	/* SymbolSpace pointer types */

	class SymbolSpace;

	//! A smart pointer to an SymbolSpace object
	typedef SmartPtr<SymbolSpace> SymbolSpacePtr;

	//! A parent pointer to an SymbolSpace object
	typedef ParentPtr<SymbolSpace> SymbolSpaceParent;

	//! A list of smart pointers to SymbolSpace objects
	typedef std::list<SymbolSpacePtr> SymbolSpaceList;

	//! A list of parent pointers to SymbolSpace objects
	typedef std::list<SymbolSpaceParent> SymbolSpaceParentList;

	//! A map of names to symbol space pointers
	typedef std::map<std::string, SymbolSpacePtr> SymbolSpaceMap;

	//! Global SymbolSpace for all MXFLib's normal symbols
	extern SymbolSpacePtr MXFLibSymbols;


	//! FIXME: Horrible fudge to fix unknown array size problem
	extern int IndexFudge_NSL;

	
	/* Enumerations used in MDType and MDValue */

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
		COMPOUND,						//!< A compound type
		ENUM							//!< An enumerated value
	};
	
	enum MDArrayClass					//!< Sub-classes of arrays
	{
		ARRAYIMPLICIT,					//!< An array that does not have an explicit count
		ARRAYEXPLICIT,					//!< An array with count and size in an 8-byte header
		ARRAYSTRING,					//!< A string, very similar to an implicit array, this information is only required for the metadictionary
		ARRAYARRAY = ARRAYIMPLICIT,		//!< DEPRECATED: For backwards compatibility only
		ARRAYBATCH = ARRAYEXPLICIT		//!< DEPRECATED: For backwards compatibility only
	};


	// Forward declare target
	class MDType;

	//! Smart Pointer to an MDType
	typedef SmartPtr<MDType> MDTypePtr;

	// Forward declare target
	class MDOType;

	//! A smart pointer to an MDOType object
	typedef SmartPtr<MDOType> MDOTypePtr;

	// Forward declare target
	class MDObject;

	// Required for MDObjectPtr constructor
	class MDObjectParent;

	//! A smart pointer to an MDObject object (with operator[] overloads)
	class MDObjectPtr : public SmartPtr<MDObject>
	{
	public:
		MDObjectPtr() : SmartPtr<MDObject>() {};
		MDObjectPtr(IRefCount<MDObject> * ptr) : SmartPtr<MDObject>(ptr) {};
		MDObjectPtr(MDObjectParent ptr);

		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName) const;
		MDObjectPtr operator[](const MDOTypePtr &ChildType) const;
		MDObjectPtr operator[](const MDTypePtr &ChildType) const;
		MDObjectPtr operator[](int Index) const;
		MDObjectPtr operator[](const UL &ChildType) const;
		MDObjectPtr operator[](const ULPtr &ChildType) const;
	};

	// Forward declare target
	class Partition;

	//! A smart pointer to an Partition object (with operator[] overload)
	class PartitionPtr : public SmartPtr<Partition>
	{
	public:
		PartitionPtr() : SmartPtr<Partition>() {};
		PartitionPtr(IRefCount<Partition> * ptr) : SmartPtr<Partition>(ptr) {};

		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
	};

	//! A parent pointer to an Partition object (with operator[] overload)
	class PartitionParent : public ParentPtr<Partition>
	{
	public:
		PartitionParent() : ParentPtr<Partition>() {};
		PartitionParent(IRefCount<Partition> * ptr) : ParentPtr<Partition>(ptr) {};
		
		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
	};

	//! A list of smart pointers to Partition objects
	typedef std::list<PartitionPtr> PartitionList;

	//! Wrapping type
	enum WrapType { UnknownWrap, FrameWrap, ClipWrap } ;
}


#endif // MXFLIB__FORWARD_H
