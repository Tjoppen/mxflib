/*! \file	rxiparser.h
 *	\brief	RXI format dictionary parser
 *	\version $Id: rxiparser.h,v 1.1 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2008, Metaglue
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
#ifndef MXFLIB__RXIPARSER_H
#define MXFLIB__RXIPARSER_H

// STL Includes
#include <string>
#include <list>


namespace mxflib
{
	//! Class holding information about dictionary data loaded from an RXI file
	class RXIData : public RefCount<RXIData>
	{
	public:
		bool LegacyFormat;					//!< Set true if the parser has detected that this is a legacy format dictionary rather than RXI
		ClassRecordList GroupList;			//!< Class definitions for all groups
		ClassRecordList ElementList;		//!< Class definitions for all elements not in groups
		TypeRecordList TypesList;			//!< Type definitions for all types
		TypeRecordList LabelsList;			//!< Type definitions for all labels
	};

	//! Smart pointer to an RXIData object
	typedef SmartPtr<RXIData> RXIDataPtr;

	//! Parse an RXI file into an RXIData structure
	RXIDataPtr ParseRXIFile(std::string DictFile, SymbolSpacePtr DefaultSymbolSpace, std::string Application = "");

	//! Parse an RXI file into an RXIData structure
	inline RXIDataPtr ParseRXIFile(std::string DictFile, std::string Application = "")
	{
		return ParseRXIFile(DictFile, MXFLibSymbols, Application);
	}

	//! Parse an RXI file into an RXIData structure
	RXIDataPtr ParseRXIData(std::string &strXML, SymbolSpacePtr DefaultSymbolSpace, std::string Application = "");

	//! Parse an RXI file into an RXIData structure
	inline RXIDataPtr ParseRXIData(std::string &strXML, std::string Application = "")
	{
		return ParseRXIData(strXML, MXFLibSymbols, Application);
	}
}

#endif // MXFLIB__RXIPARSER_H

