/*! \file	mxflib.h
 *	\brief	The main MXFLib header file
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

#include "system.h"

#include "debug.h"

#include "forward.h"

#include "endian.h"

#include "smartptr.h"

#include "types.h"

#include "helper.h"

#include "datachunk.h"

#include "mdtraits.h"
#include "mdtype.h"
#include "deftypes.h"

#include "klvobject.h"

#include "mdobject.h"

#include "metadata.h"

#include "rip.h"

#include "mxffile.h"

#include "index.h"

#include "essence.h"

#include "esp_mpeg2ves.h"
#include "esp_wavepcm.h"
#include "esp_dvdif.h"

#endif MXFLIB__MXFLIB_H
