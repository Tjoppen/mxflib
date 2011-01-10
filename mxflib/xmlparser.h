/*! \file	xmlparser.h
 *	\brief	Interface to available SAX style XML parser
 *
 *	\version $Id: xmlparser.h,v 1.2 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) Matt Beard 2004
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

#ifndef MXFLIB__XMLPARSE_H
#define MXFLIB__XMLPARSE_H

#ifdef HAVE_EXPAT
#include <expat.h>
#include <string>

namespace mxflib
{
	// For the moment re-use the sopSAX handler types
	/* Function pointer definitions */
	typedef startElementSAXFunc startElementXMLFunc;
	typedef endElementSAXFunc endElementXMLFunc;
	typedef warningSAXFunc warningXMLFunc;
	typedef errorSAXFunc errorXMLFunc;
	typedef fatalErrorSAXFunc fatalErrorXMLFunc;

	typedef sopSAXHandler XMLParserHandler;
	typedef XMLParserHandler *XMLParserHandlerPtr;

	bool XMLParserParseFile(XML_Parser *pParser, XMLParserHandlerPtr Hand, void *UserData, const char *filename, bool ParseNamespaces = false);
	bool XMLParserParseString(XML_Parser *pParser, XMLParserHandlerPtr Hand, void *UserData, std::string & strXML, bool ParseNamespaces = false);

	inline bool XMLParserParseFile(XMLParserHandlerPtr Hand, void *UserData, const char *filename, bool ParseNamespaces = false)
	{
		return XMLParserParseFile(NULL, Hand, UserData, filename, ParseNamespaces);
	}

	inline bool XMLParserParseString(XMLParserHandlerPtr Hand, void *UserData, std::string & strXML, bool ParseNamespaces = false)
	{
		return XMLParserParseString(NULL, Hand, UserData, strXML, ParseNamespaces);
	}
}

#else // HAVE_EXPAT

namespace mxflib
{
	// Use the sopSAX handler for defining parsing behaviour
	typedef startElementSAXFunc startElementXMLFunc;
	typedef endElementSAXFunc endElementXMLFunc;
	typedef warningSAXFunc warningXMLFunc;
	typedef errorSAXFunc errorXMLFunc;
	typedef fatalErrorSAXFunc fatalErrorXMLFunc;

	typedef sopSAXHandler XMLParserHandler;
	typedef XMLParserHandler *XMLParserHandlerPtr;

	// Don't currently support parser pointers
	typedef void* XML_Parser;

	//! Use the sopSAX parser to parse the file
	inline bool XMLParserParseFile(XML_Parser *pParser, XMLParserHandlerPtr Hand, void *UserData, const char *filename, bool ParseNamespaces = false)
	{
		if(pParser)
		{
			error("XMLParserParseFile() must have NULL as first parameter if Expat XML parser not used\n");
			return false;
		}
		if(ParseNamespaces)
		{
			error("Unable to parse namespaces in XML file %s without Expat parser\n", filename);
			return false;
		}
		return sopSAXParseFile(Hand, UserData, filename);
	}

	inline bool XMLParserParseFile(XMLParserHandlerPtr Hand, void *UserData, const char *filename, bool ParseNamespaces = false)
	{
		return XMLParserParseFile(NULL, Hand, UserData, filename, ParseNamespaces);
	}

}

#endif // HAVE_EXPAT

#endif /* MXFLIB__XMLPARSE_H */

