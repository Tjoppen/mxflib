/*! \file	sopSAX.h
 *	\brief	'sopranino SAX' super-light SAX style XML Parsers
 *
 *	\version $Id: sopsax.h,v 1.5 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) BBC R&D 2001
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

#ifndef _SOPSAX_H
#define _SOPSAX_H

namespace mxflib
{
	typedef struct sopSAXHandlerStruct sopSAXHandler;
	typedef sopSAXHandler *sopSAXHandlerPtr;


	/* Function pointer definitions */
	typedef void (*startElementSAXFunc) (void *, const char *, const char **);
	typedef void (*endElementSAXFunc) (void *, const char *);
	typedef void (*warningSAXFunc) (void *, const char *msg, ...);
	typedef void (*errorSAXFunc) (void *, const char *msg, ...);
	typedef void (*fatalErrorSAXFunc) (void *, const char *msg, ...);


	/* Handler structure */
	struct sopSAXHandlerStruct
	{
		startElementSAXFunc startElement;		/* startElement */
		endElementSAXFunc endElement;			/* endElement */
		warningSAXFunc warning;					/* warning */
		errorSAXFunc error;						/* error */
		fatalErrorSAXFunc fatalError;			/* fatalError */
	};


	/* Function Prototypes */
	bool sopSAXParseFile(sopSAXHandlerPtr sax, void *UserData, const char *filename);
}

#endif /* _SOPSAX_H */
