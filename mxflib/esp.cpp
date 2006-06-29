/*! \file	esp.cpp
 *	\brief	Initialisation of the essence sub-parser list
 *
 *	\version $Id: esp.cpp,v 1.2 2006/06/29 14:40:41 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2006, Matt Beard
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


#include <mxflib/mxflib.h>

using namespace mxflib;


/* List of all essence sub-parser header files */
/***********************************************/

#include <mxflib/esp_mpeg2ves.h>
#include <mxflib/esp_wavepcm.h>
#include <mxflib/esp_dvdif.h>
#include <mxflib/esp_jp2k.h>



//! List of pointers to known parsers
/*! Used only for building parsers to parse essence - the parses 
 *  in this list must not themselves be used for essence parsing 
 */
EssenceParser::EssenceSubParserFactoryList EssenceParser::EPList;


//! Initialization flag for EPList
bool EssenceParser::Inited = false;


// Build an essence parser with all known sub-parsers
void EssenceParser::Init()
{
	if(!Inited)
	{
		//! Add one factory for of all known essence parsers
		AddNewSubParserType(new MPEG2_VES_EssenceSubParser);
		AddNewSubParserType(new WAVE_PCM_EssenceSubParser);
		AddNewSubParserType(new DV_DIF_EssenceSubParserFactory);
		AddNewSubParserType(new JP2K_EssenceSubParser);

		Inited = true;
	}
}

