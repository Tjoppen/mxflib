/*! \file	legacytypes.h
 *	\brief	Definition of classes that load legacy format type and class dictionaries
 *	\version $Id: legacytypes.h,v 1.1 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2005-2008, Matt Beard
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
#ifndef MXFLIB__LEGACYTYPES_H
#define MXFLIB__LEGACYTYPES_H

// STL Includes
#include <string>
#include <list>



namespace mxflib
{
	//! Load types from the specified legacy format XML definitions
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	int LoadTypes(char *TypesFile, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols);
}


namespace mxflib
{
	//! Load dictionary from the specified legacy format XML definitions with a default symbol space
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	int LoadLegacyDictionary(const char *DictFile, SymbolSpacePtr DefaultSymbolSpace, bool FastFail = false);

	//! Load dictionary from the specified legacy format XML definitions with a default symbol space
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	inline int LoadLegacyDictionary(std::string DictFile, SymbolSpacePtr DefaultSymbolSpace, bool FastFail = false)
	{
		return LoadLegacyDictionary(DictFile.c_str(), DefaultSymbolSpace, FastFail );
	}

	//! Load dictionary from the specified legacy format XML definitions
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	inline int LoadLegacyDictionary(const char *DictFile, bool FastFail = false, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols)
	{
		return LoadLegacyDictionary(DictFile, DefaultSymbolSpace, FastFail );
	}

	//! Load dictionary from the specified legacy format XML definitions
	/*! \return 0 if all OK
	 *  \return -1 on error
	 */
	inline int LoadLegacyDictionary(std::string DictFile, bool FastFail = false, SymbolSpacePtr DefaultSymbolSpace = MXFLibSymbols)
	{
		return LoadLegacyDictionary(DictFile.c_str(), DefaultSymbolSpace, FastFail );
	}

	//! Load dictionary from the specified legacy format XML definitions with a default symbol space
	/*! \return 0 if all OK
	*  \return -1 on error
	*/
	int LoadLegacyDictionaryFromXML(std::string &strXML, bool FastFail = false);
}

#endif // MXFLIB__LEGACYTYPES_H

