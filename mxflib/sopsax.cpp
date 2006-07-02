/*! \file	sopSAX.cpp
 *	\brief	'sopranino SAX' super-light SAX style XML Parsers
 *
 *	\version $Id: sopsax.cpp,v 1.8 2006/07/02 13:27:51 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) BBC R&D 2004
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



#include <string.h>		/* strcpy */

#include "sopsax.h"
#include <mxflib/mxflib.h>

using namespace mxflib;


/* Local Prototypes */
static int sopSkipToClose(FILE *xmlFile);
static int sopGetCharNonQ(FILE *xmlFile);
static int sopGetChar(FILE *xmlFile);
static void sopSkipSpace(FILE *xmlFile);
static void sopGetItem(FILE *xmlFile, char *Buffer, int Max);


#define MAXTAGDEPTH 32
#define MAXTAGLENGTH 256
#define MAXATTRIBS 128
#define ATTRIBBUFFERSIZE 20480

/*
** sopSAXParseFile() - Parse an XML file (not re-entrant due to static data)
*/

bool mxflib::sopSAXParseFile(sopSAXHandlerPtr sax, void *UserData, const char *filename)
{
	FILE *xmlFile;
	int ElementNesting;
	char ThisTag[MAXTAGLENGTH+1];

	/* NOTE: These are static because they are quite large */
	/*       For re-entrant use they can be made auto */
	static char TagName[MAXTAGDEPTH][MAXTAGLENGTH+1];
	static char AttribBuffer[ATTRIBBUFFERSIZE];
	static char *Attribs[(MAXATTRIBS+1)*2];

	/* Validate the handler */
	if(sax == NULL)
	{
		// Note that this is far from ideal - but we don't have a valid error handler now!
		fprintf(stderr, "Cannot parse file with no handler\n");
		return false;
	}

	if (filename == NULL)
		return false;

	xmlFile = fopen(filename, "rb");

	if(xmlFile == NULL)
	{
		if(sax->fatalError != NULL) sax->fatalError(UserData, 
			"Cannot open file %s", filename);

		return false;
	}

	ElementNesting = 0;
	while(!feof(xmlFile))
	{
		int c;
		int attrib;
		int closed;
		int AttribBufferFree;
		char *CurrentAttrib;

		/* Assume elements are open (have separate end tag) */
		closed = 0;

		/* Scan for start of tag */
		do
		{	
			c = sopGetChar(xmlFile);
			if(c == EOF) break;
		} while( c != '<' );
		if(c == EOF) break;

		/* Get name of element (tag) */
		sopGetItem(xmlFile, ThisTag, MAXTAGLENGTH);

		/* Make no attempt to parse "?xml" etc. */
		if(ThisTag[0] == '?')
		{
			sopSkipToClose(xmlFile);
			continue;
		}
		
		if(ThisTag[0] == '/')
		{
			if(ElementNesting == 0)
			{
				if(sax->error != NULL) sax->error(UserData, 
					"Unexpected end tag \"%s\"", ThisTag);

				sopSkipToClose(xmlFile);
				continue;
			}

			if(strcmp(&ThisTag[1], TagName[ElementNesting-1]) != 0)
			{
				if(sax->error != NULL) sax->error(UserData, 
					"Expecting end tag \"%s\", found \"%s\"", TagName[ElementNesting-1], ThisTag);
				
				sopSkipToClose(xmlFile);
				continue;
			}
			
			/* Pop up a level */
			ElementNesting--;

			/* Skip to the end of the tag */
			if(sopSkipToClose(xmlFile))
			{
				if(sax->warning != NULL) sax->warning(UserData, 
					"Unwanted characters in close tag for element \"%s\"", &ThisTag[1]);
			}

			/* Call the handler (if there is one) */
			if(sax->endElement != NULL)
			{
				sax->endElement(UserData, &ThisTag[1]);
			}

			/* Go find the next */
			continue;
		}

		/* Copy the name for validation of close tags */
		strcpy(TagName[ElementNesting], ThisTag);

		/* Move down a level */
		if(ElementNesting < (MAXTAGDEPTH-1))
		{
			ElementNesting++;
		}
		else
		{
			if(sax->error != NULL) sax->error(UserData, 
					"Error processing element \"%s\" too deeply nested", 
					ThisTag);
		}

		attrib = 0;
		AttribBufferFree = ATTRIBBUFFERSIZE - 1;
		CurrentAttrib = &AttribBuffer[0];
		while(attrib < MAXATTRIBS)
		{
			size_t i;

			/* Index current attribute's starting point in buffer */
			Attribs[attrib*2] = CurrentAttrib;

			/* Get attribute name */
			sopSkipSpace(xmlFile);
			sopGetItem(xmlFile, Attribs[attrib*2], AttribBufferFree);
			
			/* Work out how much buffer is now free */
			i = strlen(Attribs[attrib*2]) + 1;
			AttribBufferFree -= static_cast<int>(i);
			CurrentAttrib += i;

			if(AttribBufferFree < 3)
			{
				int last_c = 0;

				if(sax->error != NULL) sax->error(UserData, 
						"Error processing element \"%s\" out of attribute buffer", 
						ThisTag);

				/* Skip to the end of this element here, but continue parsing file */
				while(!feof(xmlFile))
				{
					c = sopGetCharNonQ(xmlFile);
					if(c=='>')
					{
						if(last_c == '/') closed = 1;
						break;
					}

					last_c = c;
				}
				
				break;
			}

			/* Open end of element */
			if(Attribs[attrib*2][0] == 0) break;

			/* Closed end of element */
			if(Attribs[attrib*2][0] == '/')
			{
				closed = 1;
				break;
			}

			/* Check for '=' after attribute name */
			sopSkipSpace(xmlFile);
			c = sopGetChar(xmlFile);

			if(c != '=')
			{
				int last_c = 0;

				if(sax->error != NULL) sax->error(UserData, 
						"Error processing attribute \"%s\" of element \"%s\" '=' not found where expected", 
						Attribs[attrib*2], ThisTag);
			
				/* Push back the rouge char (in case is is a quote) */
				ungetc(c, xmlFile);

				/* Skip to the end of this element here, but continue parsing file */
				while(!feof(xmlFile))
				{
					c = sopGetCharNonQ(xmlFile);
					if(c=='>')
					{
						if(last_c == '/') closed = 1;
						break;
					}

					last_c = c;
				}
				
				break;
			}

			/* Index current value's starting point in buffer */
			Attribs[attrib*2+1] = CurrentAttrib;

			/* Get attribute value */
			sopSkipSpace(xmlFile);
			sopGetItem(xmlFile, Attribs[attrib*2+1], AttribBufferFree);
			
			/* Work out how much buffer is now free */
			i = strlen(Attribs[attrib*2+1]) + 1;
			AttribBufferFree -= static_cast<int>(i);
			CurrentAttrib += i;

			attrib++;
		}

		/* Call the handler (if there is one) */
		if(sax->startElement != NULL)
		{
			Attribs[attrib*2] = NULL;
			Attribs[attrib*2+1] = NULL;
			sax->startElement(UserData, ThisTag, (const char**)Attribs);
		}

		/* Call the close handler (if required and there is one) */
		if(closed)
		{
			if(ElementNesting) ElementNesting--;

			if(sax->endElement != NULL)
			{
				sax->endElement(UserData, ThisTag);
			}
		}
	}

	fclose(xmlFile);

	return true;
}


/*
** sopGetChar() - Get character from XML file, skipping comments
*/
int sopGetChar(FILE *xmlFile)
{
	int c;
	int c2, c3;
	long pos;

	/* Get the next character */
	c = fgetc(xmlFile);
	/* return it if safe */
	if(c != '<') return c;

	pos = ftell(xmlFile);
	/* Else sniff the next chars for '!--' */
	c = fgetc(xmlFile);
	if(c != '!')
	{
		/* We're safe, move back and continue */
		fseek(xmlFile, pos, SEEK_SET);
		return '<';
	}
	
	c2 = fgetc(xmlFile);
	if(c2 != '-')
	{
		/* We're safe, move back and continue */
		fseek(xmlFile, pos, SEEK_SET);
		return '<';
	}

	c3 = fgetc(xmlFile);
	if(c3 != '-')
	{
		/* We're safe, move back and continue */
		fseek(xmlFile, pos, SEEK_SET);
		return '<';
	}

	/* Scan for end of comment */
	c3 = 0;
	c2 = 0;
	while(!feof(xmlFile))
	{
		c = fgetc(xmlFile);
		if((c=='>') && (c2=='-') && (c3=='-'))
		{
			return sopGetChar(xmlFile);
		}

		c3 = c2;
		c2 = c;
	}

	/* Fell out of scan! */
	return EOF;
}


/*
** sopSkipSpace() - Skip any whitespace or newline characters in XML file
*/
void sopSkipSpace(FILE *xmlFile)
{
	int c;

	for(;;)
	{
		c = sopGetChar(xmlFile);
		if((c!=' ') && (c!='\t') && (c!='\r') && (c!='\n'))
		{
			ungetc(c, xmlFile);
			return;
		}
	}
}


/*
** sopGetCharNonQ() - Get character from XML file, skipping quoted strings
*/
int sopGetCharNonQ(FILE *xmlFile)
{
	int c;

	for(;;)
	{
		c = sopGetChar(xmlFile);
		if(c != '"') return c;

		/* In quotes - skip to end */
		do
		{
			c = sopGetChar(xmlFile);
			if(c == EOF) return c;
		} while(c != '"');
	}
}


/*
** sopSkipToClose() - Skips to the next '>' in an XML file
**
** Returns: 0 if '>' found after nothing but whitespace and '\n'
**			1 if anything else found
**
** Note: '?' is permitted before the '>' and is discarded
**
*/
int sopSkipToClose(FILE *xmlFile)
{
	int c;

	do
	{
		sopSkipSpace(xmlFile);
		c = sopGetChar(xmlFile);
	} while(c == '?');

	/* Found '>' straight away */
	if(c == '>') return 0;

	/* Other characters found - skip them */

	/* Push back the rouge char (in case is is a quote) */
	ungetc(c, xmlFile);

	while(!feof(xmlFile))
	{
		c = sopGetCharNonQ(xmlFile);
		if(c=='>') break;
	}

	return 1;
}


/*
** sopGetItem() - Read next 'item' in an XML file
**
** If the first character is a quote, the whole quoted
** string is returned, otherwise the first chunk that
** ends in a whitespace, or a return, or '>'
**
*/
void sopGetItem(FILE *xmlFile, char *Buffer, int Max)
{
	int c;

	c = sopGetChar(xmlFile);

	/* Quoted or other token? */
	if(c=='"')
	{
		/* Copy quoted string */
		for(;;)
		{
			c = sopGetChar(xmlFile);
			if((c=='"') || c==EOF) break;

			if(c=='&')
			{
				char Tag[32];
				sopGetItem(xmlFile, Tag, 31);
				
				c = sopGetChar(xmlFile);
				// Should validate for ';' here!!

				if(strcasecmp(Tag, "amp")==0)			c='&';
				else if(strcasecmp(Tag, "apos")==0)		c='\'';
				else if(strcasecmp(Tag, "quot")==0)		c='"';
				else if(strcasecmp(Tag, "lt")==0)		c='<';
				else if(strcasecmp(Tag, "gt")==0)		c='>';
				else
				{
					// Should error here!!
					c = '?';
				}
			}

			if(Max)
			{
				Max--;
				*Buffer++ = c;
			}
		}
	}
	else if(c != EOF)
	{
		/* Copy chunk up to next separator */
		while((c!=' ') && (c!='\t') && (c!='=') && (c!='>') && (c!='\n') && (c!='\r') && (c!=';'))
		{
			if(Max)
			{
				Max--;
				*Buffer++ = c;
			}
			c = sopGetChar(xmlFile);
			if(c==EOF) break;
		}		

		/* Push back the separator */
		ungetc(c, xmlFile);
	}

	/* Terminate the string */
	*Buffer = '\0';
}
