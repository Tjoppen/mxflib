/*! \file	mxflib.h
 *	\brief	The main MXFLib header file
 *
 *	\version $Id: mxflib.h,v 1.7 2005/03/25 13:13:50 terabrit Exp $
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

#ifndef MXFLIB__MXFLIB_H
#define MXFLIB__MXFLIB_H

//! Namespace for all MXFLib items
namespace mxflib {}


//! Include system specifics first to allow any required compiler set-up
#include <mxflib/system.h>


// Required std::headers

#include <list>
#include <map>


namespace mxflib
{
#define MXFLIB_VERSION_MAJOR "0"
#define MXFLIB_VERSION_MINOR "5"
#define MXFLIB_VERSION_TWEAK "2"
#define MXFLIB_VERSION_BUILD "1"
#define MXFLIB_VERSION_REL   2
#define MXFLIB_VERSION_RELTEXT(REL) (REL==1?"-Release":(REL==2?"-Development":(REL==3?"-Patched":(REL==4?"-Beta":(REL==5?"-Private":"")))))
#define MXFLIB_VERSION_RELNUMBER(REL) (REL==1?"1":(REL==2?"2":(REL==3?"3":(REL==4?"4":(REL==5?"5":"0")))))

	//! Get a human readable version of the library name
	inline std::string LibraryName(void) { return std::string("MXFLib"); }

	//! Get a human readable version of the library version
	inline std::string LibraryVersion(void) 
	{ 
		return std::string("MXFLib " MXFLIB_VERSION_MAJOR "." MXFLIB_VERSION_MINOR "."
			                MXFLIB_VERSION_TWEAK "(" MXFLIB_VERSION_BUILD ")")
			 + std::string( MXFLIB_VERSION_RELTEXT(MXFLIB_VERSION_REL) ); 
	}

	//! Get a version of the library version suitable for setting ProductVersion
	inline std::string LibraryProductVersion(void) 
	{ 
		return std::string( "\""    MXFLIB_VERSION_MAJOR "\",\"" MXFLIB_VERSION_MINOR 
			                "\",\"" MXFLIB_VERSION_TWEAK "\",\"" MXFLIB_VERSION_BUILD "\",\"")
							+ std::string( MXFLIB_VERSION_RELNUMBER(MXFLIB_VERSION_REL)) + std::string("\""); 
	}
}

//! Touch an unused parameter in such a way that no compiler warnings are produced, but optimizer will remove anything we do
#define UNUSED_PARAMETER(x) ( x=x )

#include <mxflib/debug.h>

#include <mxflib/smartptr.h>

#include <mxflib/endian.h>

#include <mxflib/types.h>

#include <mxflib/datachunk.h>

#include <mxflib/forward.h>




#include <mxflib/helper.h>


#include <mxflib/mdtraits.h>
#include <mxflib/mdtype.h>
#include <mxflib/deftypes.h>

#include <mxflib/mdobject.h>

#include <mxflib/metadata.h>

#include <mxflib/rip.h>

#include <mxflib/mxffile.h>

#include <mxflib/index.h>

#include <mxflib/essence.h>

#include <mxflib/klvobject.h>

#include <mxflib/crypto.h>

#include <mxflib/esp_mpeg2ves.h>
#include <mxflib/esp_wavepcm.h>
#include <mxflib/esp_dvdif.h>
#include <mxflib/sopsax.h>
#include <mxflib/xmlparse.h>

#endif // MXFLIB__MXFLIB_H

