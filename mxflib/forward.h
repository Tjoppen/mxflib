/*! \file	forward.h
 *	\brief	Various forward declarations
 *
 *	\version $Id: forward.h,v 1.3 2005/03/25 13:18:12 terabrit Exp $
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
#include <mxflib/smartptr.h>

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

	// We need access to the MDValue class
	class MDValue;
//
//	//! A smart pointer to an MDValue object
//	typedef SmartPtr<MDValue> MDValuePtr;

	//! A smart pointer to an MDValue object (with operator[] overloads)
	class MDValuePtr : public SmartPtr<MDValue>
	{
	public:
		MDValuePtr() : SmartPtr<MDValue>() {};
		MDValuePtr(IRefCount<MDValue> * ptr) : SmartPtr<MDValue>(ptr) {};

		//! Child access operator that overcomes dereferencing problems with SmartPtrs
		MDValuePtr operator[](int Index);

		//! Child access operator that overcomes dereferencing problems with SmartPtrs
		MDValuePtr operator[](const std::string ChildName);
	};

	//! A list of smart pointers to MDValue objects
	typedef std::list<MDValuePtr> MDValueList;
}


#endif // MXFLIB__FORWARD_H
