/*! \file	forward.h
 *	\brief	Various forward declarations
 *
 *	\version $Id: forward.h,v 1.1.2.2 2004/06/26 17:45:49 matt-beard Exp $
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

	// Forward declare so the class can include pointers to itself
	class KLVObject;

	//! A smart pointer to a KLVObject object
	typedef SmartPtr<KLVObject> KLVObjectPtr;
}


#endif // MXFLIB__FORWARD_H
