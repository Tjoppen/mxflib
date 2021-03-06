/*! \file	parsoptions.h
 *	\brief	Declarations for MXFWrap commandline options
 *
 *	\version $Id: parseoptions.h,v 1.1 2011/01/10 10:42:27 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2010, Metaglue Corporation
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
#ifndef _parseoptions_h_
#define _parseoptions_h_

#include "libprocesswrap/process.h"

//! Parse the command line options
/*!	\return -1 if an error or none supplied, 1 if pause before exit (-z), 0 otherwise */
int ParseOptions(int &argc, char **argv, ProcessOptions *pOpt);

#endif // _parseoptions_h_
