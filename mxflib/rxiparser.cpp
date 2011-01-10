/*! \file	rxiparser.cpp
 *	\brief	RXI format dictionary parser
 *
 *	\version $Id: rxiparser.cpp,v 1.1 2011/01/10 10:42:09 matt-beard Exp $
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
 
#include "mxflib/mxflib.h"

#include <stdarg.h>

/* Allow us to use std::numeric_limits<>::max(), which includes removing any "max" macro! */
#ifdef max
#undef max
#endif
#include <limits>
/******************************************************************************************/

using namespace mxflib;


/* XML parsing functions */
namespace
{
	//! XML parsing functions for dictionary loading
	void DictLoad_startElement(void *user_data, const char *name, const char **attrs);
	void DictLoad_endElement(void *user_data, const char *name);

	//! Process an XML element that has been determined to be part of a class definition
	/*! \return true if all OK
	*/
	bool ProcessClassElement(void *user_data, const char *name, const char **attrs);


	//! XML callback - Handle warnings during XML parsing
	void XML_warning(void *user_data, const char *msg, ...)
	{
		char Buffer[10240];			// DRAGONS: Could burst!!
		va_list args;

		va_start(args, msg);
		vsprintf(Buffer, msg, args);
		warning("XML WARNING: %s\n",Buffer);
		va_end(args);
	}

	//! XML callback - Handle errors during XML parsing
	void XML_error(void *user_data, const char *msg, ...)
	{
		char Buffer[10240];			// DRAGONS: Could burst!!
		va_list args;

		va_start(args, msg);
		vsprintf(Buffer, msg, args);
		error("XML ERROR: %s\n",Buffer);
		va_end(args);
	}

	//! XML callback - Handle fatal errors during XML parsing
	void XML_fatalError(void *user_data, const char *msg, ...)
	{
		char Buffer[10240];			// DRAGONS: Could burst!!
		va_list args;

		va_start(args, msg);
		vsprintf(Buffer, msg, args);
		error("XML FATAL ERROR: %s\n",Buffer);
		va_end(args);
	}

	//! Our XML handler
	static XMLParserHandler DictLoad_XMLHandler = 
	{
		(startElementXMLFunc) DictLoad_startElement,		/* startElement */
		(endElementXMLFunc) DictLoad_endElement,			/* endElement */
		(warningXMLFunc) XML_warning,						/* warning */
		(errorXMLFunc) XML_error,							/* error */
		(fatalErrorXMLFunc) XML_fatalError,					/* fatalError */
	};

	//! State-machine state for XML parsing
	enum RegisterLeafType
	{
		RegLeafNone = 0,					//!< Not a known leaf type, or not a leaf
		RegLeafType,						//!< A type leaf
		RegLeafElement,						//!< An element leaf
		RegLeafGroup,						//!< A group leaf
		RegLeafLabel						//!< A label leaf
	};


	// Forward declare so we can have a typedef to allow recursive structures
	struct RegisterLeafData;

	//! A list of RegisterLeafData structures
	/*! /note This list contains the actual structures, not pointers to them
	 */
	typedef std::list<RegisterLeafData> RegisterLeafList;

	//! Information about the current leaf
	struct RegisterLeafData
	{
		RegisterLeafType LeafType;			//!< The type of this leaf
		bool Active;						//!< Is this leaf active (i.e. not disabled due to not matching the current application)?
		std::string LeafURN;				//!< The URN for this leaf
		std::string SubURN;					//!< The URN for the type of this element leaf or target for this type leaf
		std::string ParentURN;				//!< The URN for the parent (base class) of this group leaf
		std::string NamespaceURI;			//!< The namespace URI for this leaf, or "" if default
		std::string LeafSymbol;				//!< The symbol for the item being defined by this leaf
		std::string TypeKind;				//!< The "kind" for this type leaf
		std::string TypeQualif;				//!< The "qualif" for this type leaf
		std::string TypeValue;				//!< The "value" for this type leaf
		std::string MinOccurs;				//!< The "minOccurs" for this type leaf
		std::string MaxOccurs;				//!< The "maxOccurs" for this type leaf
		std::string Tag;					//!< The "tag" for this group leaf
		std::string Coding;					//!< The "coding" for this group leaf
		std::string IsAbstract;				//!< The "isAbstract" for this group leaf
		std::string TraitsName;				//!< The "name" from the last "traits" section
		std::string TraitsDetail;			//!< The "detail" from the last "traits" section
//		std::string TraitsLength;			//!< The "length" from the last "traits" section
		RegisterLeafList ShootList;			//!< A list of the shoots under the current leaf (for packs and records)
											/*!< DRAGONS: The old name of "shoot" is used rather than "link" as this
											 *            can be confused with "leaf" when reading the code
											 */
	};



	//! Information about the current wildcard section
	struct RegisterWildcardData
	{
		std::string Symbol;					//!< The "sym" from this "wildcard" section
		std::string URN;					//!< The "urn" from this "wildcard" section
	};

	//! A list of RegisterWildcardData structures
	/*! /note This list contains the actual structures, not pointers to them
	 */
	typedef std::list<RegisterWildcardData> RegisterWildcardList;

	struct ElementInfo
	{
		ClassRecordPtr Item;				//!< The item in question
		ClassRecordPtr Group;				//!< The group containing this item (if yet allocated, otherwise NULL)
	};

	//! A map of ElementInfo structures by UL
	/*! /note This map contains the actual ULs and structures, not pointers to them
	 */
	typedef std::map<UL, ElementInfo> ElementInfoMap;


	//! State-machine state for XML parsing
	enum DictCurrentState
	{
		DictStateIdle = 0,					//!< Processing not yet started
		DictStateRegister,					//!< Within the outer RXI tag
		DictStateLeaf,						//!< Processing a leaf
		DictStateShoot,						//!< Processing a shoot
											/*!< DRAGONS: The old name of "shoot" is used rather than "link" as this
											 *            can be confused with "leaf" when reading the code
											 */
		DictStateError						//!< A fatal error occurred
	};

	//! The type for RegisterParserState.SymbolMap
	typedef std::multimap<std::string, RegisterLeafType> SymbolMapType;

	//! Map of symbol space by coding type to allow groups to have the same name but different keys due to coding
	typedef std::map<UInt8, SymbolSpacePtr> SymSpaceMapType;

	//! A namespace URI with associated tag depth at which it was defined
	typedef std::pair<int, std::string> DepthNamespace;

	//! A list of namespace URIs, each with associated tag depth at which it was defined
	typedef std::list<DepthNamespace> DepthNamespaceList;

	//! State structure for XML parsing types file
	struct RegisterParserState
	{
		DictCurrentState State;				//!< Current state of the parser state-machine
		int Depth;							//!< The depth of tag nesting
		bool InTraits;						//!< Set true when we are inside the "traits" within a leaf
		SymbolSpacePtr DefaultSymbolSpace;	//!< Default symbol space to use for all classes (in current MXFClasses section)
		SymbolSpacePtr DictSymbolSpace;		//!< Default symbol space to use for all classes (in the whole dictionary)
		std::string BaseNS;					//!< The base RXI namespace for this document
		std::string NormativeNS;			//!< The 'normative' RXI namespace for this document
		RegisterLeafList LeafList;			//!< A list of the nested leaves being processed, last entry is deepest nesting
		RegisterWildcardList WildcardList;	//!< A list of the nested wildcards, last entry is deepest nesting
		XML_Parser Parser;					//!< A pointer to the current parser to allow us to turn character handling on and off
		std::string CharData;				//!< Character data found when parsing tags under "traits"
		std::string Application;			//!< The application for which to filter this data
		int AppVersion;						//!< The application version number (times 100) for which to filter this data. Zero = oldest version
		bool AppAlias;						//!< Enable alias names if set
		RXIDataPtr RXIData;					//!< Parsed RXI items
		DepthNamespaceList NamespaceList;	//!< Namespace URIs defined at this level, or above

		//! Map of symbols added to the generated while parsing, including sub-items of a wildcard - used to prevent duplication of symbols
		/*! This is a multimap as there can be duplicates of different types, but duplicates of the same type are forbidden
		 */
		SymbolMapType SymbolMap;

		//! Map of all elements together with a link to thier parent group
		/*! This is so it can be established is an element is added to multiple groups */
		ElementInfoMap ElementMap;

		//! Map of symbol space by coding type to allow groups to have the same name but different keys due to coding
		SymSpaceMapType SymSpaceMap;
	};
}


//! Read a version number, with up to 2 decimal places, from a string and return as an integer of that number times 100
int ParseAppVersion(std::string Text)
{
	// The version number * 100
	int Ret = 0;

	// Decimal places processed so far. -1 until '.' found, then 0 and incremented on each digit
	int DecimalPlaces = -1;

	std::string::const_iterator it = Text.begin();
	while(it != Text.end())
	{
		// Skip leading spaces
		if((*it) == ' ') 
		{ 
			// Stop parsing if this space is not leading
			if((Ret != 0) || (DecimalPlaces != -1)) break;

			it++;
			continue; 
		}

		// Process decimal point
		if((*it) == '.')
		{
			// If we find a second '.', stop processing
			if(DecimalPlaces != -1) break;

			DecimalPlaces = 0;
			it++;
			continue;
		}

		// If we find any other non-digits, stop processing
		if(!isdigit(*it)) break;

		Ret = (Ret * 10) + ((*it) - '0');
		if(DecimalPlaces >= 0) DecimalPlaces++;

		it++;
	}

	// Short-cuts for versions with no decimal point or only one decimal place
	if(DecimalPlaces <= 0) return Ret * 100;
	if(DecimalPlaces == 1) return Ret * 10;

	return Ret;
}


//! Look up the namespace for a given coding in the given map, add a new one if required
/*! TODO: Update this to build sub-symspaces of the parent if required */
SymbolSpacePtr &GetCodingSymbolSpace(UInt8 Coding, RegisterParserState *State)
{
	// Get a reference to the mapfor readability
	SymSpaceMapType &SymSpaceMap = State->SymSpaceMap;

	SymSpaceMapType::iterator it = SymSpaceMap.find(Coding);
	
	// Not found - add a new one
	if(it == SymSpaceMap.end())
	{
		// Build a name for this new symbol space
		char Name[32];
		sprintf(Name, "Coding%02x", (int)Coding);

		// Build the symbol space
		SymbolSpacePtr NewSS = new SymbolSpace(Name);

		// Add it to the map, updating the iterator to index it so the return works as if we had found it
		it = SymSpaceMap.insert(SymSpaceMapType::value_type(Coding, NewSS)).first;
	}

	// Return the found, or new symbol space
	return (*it).second;
}


namespace
{
	//! Parse an RXI file or string into an RXIData structure
	/*! If DictFile is empty the contents of strXML will be parsed instead
	 */
	RXIDataPtr ParseRXIInternal(std::string DictFile, std::string &strXML, SymbolSpacePtr DefaultSymbolSpace, std::string Application)
	{
		// Info bloack to return
		RXIDataPtr Ret = new RXIData;

		// State data block passed through XML parser
		RegisterParserState State;

		// Initialize the state
		State.State = DictStateIdle;
		State.Depth = 0;
		State.InTraits = false;
		State.DefaultSymbolSpace = DefaultSymbolSpace;
		State.DictSymbolSpace = DefaultSymbolSpace;
		State.RXIData = Ret;

		Ret->LegacyFormat = false;

		if(Application.empty())
		{
			State.AppVersion = 0;
		}
		else
		{
			// Check for alias enable flag
			size_t Pos = Application.find_last_of('~');
			if(Pos == std::string::npos)
			{
				State.AppAlias = false;
			}
			else
			{
				State.AppAlias = true;
				Application = Application.substr(0,Pos);
			}

			// Check for version number
			Pos = Application.find_first_of('[');
			if(Pos == std::string::npos)
			{
				// If no version number, assume the highest possible
				State.Application = Application;
				State.AppVersion = std::numeric_limits<int>::max();
			}
			else
			{
				State.Application = Application.substr(0,Pos);
				State.AppVersion = ParseAppVersion(Application.substr(Pos+1));
			}
		}

		// Parse the file
		bool result = false;

		if(!DictFile.empty())
		{
			std::string XMLFilePath = LookupDictionaryPath(DictFile);
#ifdef HAVE_EXPAT
			if(XMLFilePath.size()) result = XMLParserParseFile(&State.Parser, &DictLoad_XMLHandler, &State, XMLFilePath.c_str(), true);
#else
			if(XMLFilePath.size()) result = XMLParserParseFile(&DictLoad_XMLHandler, &State, XMLFilePath.c_str());
#endif
			if (!result)
			{
				XML_fatalError(NULL, "Failed to load dictionary \"%s\"\n", XMLFilePath.size() ? XMLFilePath.c_str() : DictFile.c_str());
				return NULL;
			}
		}
		else
		{
#ifdef HAVE_EXPAT
			if(strXML.size()) result = XMLParserParseString(&DictLoad_XMLHandler, &State, strXML);
#else
			if(strXML.size()) XML_error(NULL, "Cannot parse dictionary from XML unless compiled with Expat XML parser\n");
#endif
			if (!result)
			{
				XML_fatalError(NULL, "Failed to load dictionary from XML");
				return NULL;
			}
		}

		// Legacy dictionary detected
		if(State.RXIData->LegacyFormat) return State.RXIData;

		// Flag an error if it all went bad
		if(State.State == DictStateError) return NULL;

		// Work out orphaned elements
		ElementInfoMap::iterator it = State.ElementMap.begin();
		while(it != State.ElementMap.end())
		{
			if(!(*it).second.Group)
			{
				State.RXIData->ElementList.push_back((*it).second.Item);
			}
			it++;
		}

		return State.RXIData;
	}
}


//! Parse an RXI file into an RXIData structure
RXIDataPtr mxflib::ParseRXIFile(std::string DictFile, SymbolSpacePtr DefaultSymbolSpace, std::string Application /*=""*/)
{
	std::string Empty;
	return ParseRXIInternal(DictFile, Empty, DefaultSymbolSpace, Application);
}

//! Parse an RXI file into an RXIData structure
RXIDataPtr mxflib::ParseRXIData(std::string &strXML, SymbolSpacePtr DefaultSymbolSpace, std::string Application /*=""*/)
{
	return ParseRXIInternal("", strXML, DefaultSymbolSpace, Application);
}


namespace
{
	//! Parse the attributes of a wildcard
	void ParseWildcard(RegisterParserState *State, const char **attrs)
	{
		// Add a new data structure
		State->WildcardList.resize(State->WildcardList.size() + 1);
		
		// Set up a handy reference to this wildcard's details
		RegisterWildcardData &ThisWildcard = State->WildcardList.back();

		const char **attr = attrs;
		while(*attr)
		{
			// Get a std::string version of the attribute name for comparisons and index the value
			std::string AttrString = *(attr++);
			if(AttrString == State->NormativeNS + "|sym") ThisWildcard.Symbol = *attr;
			else if(AttrString == State->NormativeNS + "|urn") ThisWildcard.URN = *attr;

			// Index the next name
			attr++;
		}
	}


	//! Parse the attributes of a trunk, stem or node to see if it defines a namespace URI
	void ParseNamespace(RegisterParserState *State, const char **attrs)
	{
		const char **attr = attrs;
		while(*attr)
		{
			// Get a std::string version of the attribute name for comparisons and index the value
			std::string AttrString = *(attr++);
			if(AttrString == State->NormativeNS + "|ns_uri")
			{
				State->NamespaceList.push_back(DepthNamespace(State->Depth, *attr));
				break;
			}

			// Index the next name
			attr++;
		}
	}


	//! Build a new symbol based on this item's containing wildcard
	void BuildWildcardedSymbol(RegisterParserState *State)
	{
		// Set up some references for easy access to data
		std::string &LeafSymbol = State->LeafList.back().LeafSymbol;
		std::string &LeafURN = State->LeafList.back().LeafURN;

		// Build a base name if none found in the containing wildcard section
		if(State->WildcardList.empty() || State->WildcardList.back().Symbol.empty())
		{
			// We need something with a chance of being unique - so use the traits name fist, or the URN, or just use "Unknown" and let the de-duplicator sort it out
			if(!State->LeafList.back().TraitsName.empty())
			{
				LeafSymbol = State->LeafList.back().TraitsName;
				warning("No symbol supplied for leaf %s %s\n", LeafSymbol.c_str(), LeafURN.c_str());
			}
			else if(!LeafURN.empty())
			{
				LeafSymbol = LeafURN;
				warning("No symbol supplied for leaf %s\n", LeafSymbol.c_str());
			}
			else
			{
				LeafSymbol = "Unknown";
				warning("No symbol or URN supplied for leaf\n");
			}
		}
		else
		{
			LeafSymbol = State->WildcardList.back().Symbol;
		}

		// Now we need to swap spaces to underscores and make the name valid
		std::string NewSymbol;
		size_t Pos = 0;
		while(Pos < LeafSymbol.length())
		{
			char c = LeafSymbol[Pos];
			if(c == ' ') NewSymbol += '_';
			else if(isalnum(c)) NewSymbol += c;
			Pos++;
		}
		LeafSymbol = NewSymbol;

		// Try and prevent duplicates by building a simple URN based suffix.
		// This should simply tack the hex digit that differs to the end of each name
		if((!State->WildcardList.empty()) && (!State->WildcardList.back().URN.empty()) && (!LeafURN.empty()))
		{
			// Rather that trying to parse urns here, build them into ULs and do a binary compare
			ULPtr Wild = StringToUL(State->WildcardList.back().URN);
			ULPtr Leaf = StringToUL(LeafURN);

			// Check that both UL conversions worked (the strings could be broken)
			if(Wild && Leaf)
			{
				const UInt8 *pWild = Wild->GetValue();
				const UInt8 *pLeaf = Leaf->GetValue();

				// Perform a simple test to see if both are apparently ULs rather then UUIDs
				if(((*pWild) == 0x06) && ((*pLeaf) == 0x06))
				{
					// Scan for the first non-matching byte (max bytes to scan = 16)
					int Remaining=16;
					while(((Remaining--) > 0) && ((*pWild) == (*pLeaf)))
					{
						pWild++;
						pLeaf++;
					}

					// Now copy all remaining non-matching hex digit pairs to the name
					while((Remaining--) > 0)
					{
						if((*pWild) != (*pLeaf))
						{
							char Buffer[3];
							sprintf(Buffer, "%02x", *pLeaf);
							LeafSymbol += Buffer;
						}
						pWild++;
						pLeaf++;
					}
				}
			}
		}
	}

	
	//! De-duplicate a symbol by checking the SymbolMap
	std::string DeDuplicateSymbol(RegisterParserState *State, std::string LeafSymbol)
	{
		// Find the current leaf type
		RegisterLeafType LeafType = State->LeafList.back().LeafType;

		/* We de-duplicate the name in case there is a corner case where the normal algorythms do not give unique names.
		 * This may be cause by poorly used wildcards or where no symbol is specified for a non-wildcarded leaf and one is build from its name 
		 */

		// We start by trying the unmodified symbol name, if this already exists, we add _1, then _2 etc.
		std::string Candidate = LeafSymbol;
		int SuffixNumber = 0;
		SymbolMapType::iterator it;
		for(;;)
		{
			// Look up the symbol
			it = State->SymbolMap.find(Candidate);

			// If this symbol is not used - all is OL
			if(it == State->SymbolMap.end()) break;

			// We have found the symbol, but is it used for this leaf type?
			bool SymbolOK = false;
			while((!SymbolOK) && ((*it).second != LeafType))
			{
				// Try the next entry in the map
				it++;

				// If this entry is the map end, or it no longer for the same symbol, then this symbol is OK
				if((it == State->SymbolMap.end()) || ((*it).first != Candidate)) SymbolOK = true;
			}
			if(SymbolOK) break;

			// We found a match for the current candidate symbol - increment the suffix number and try again
			char Buffer[16];
			sprintf(Buffer, "_%d", ++SuffixNumber);
			Candidate = LeafSymbol + Buffer;
		}

		if(SuffixNumber > 0) debug("De-duplicated symbol to %s\n", Candidate.c_str());

		// Now we have de-duplicated this symbol - add it to the SymbolMap
		State->SymbolMap.insert(SymbolMapType::value_type(Candidate, LeafType));

		// Return the successful candidate
		return Candidate;
	}


	//! Parse the attributes of a leaf_types
	// DRAGONS: Could do with some optimization as strings are built every time through the test loop
	void ParseTypeLeaf(RegisterParserState *State, const char **attrs)
	{
		// Set up a handy reference to this leaf's details
		RegisterLeafData &ThisLeaf = State->LeafList.back();

		const char **attr = attrs;
		while(*attr)
		{
			// Get a std::string version of the attribute name for comparisons and index the value
			std::string AttrString = *(attr++);
			if(AttrString == State->NormativeNS + "|sym") ThisLeaf.LeafSymbol = *attr;
			else if(AttrString == State->NormativeNS + "|urn") ThisLeaf.LeafURN = *attr;
			else if(AttrString == State->NormativeNS + "|kind") ThisLeaf.TypeKind = *attr;
			else if(AttrString == State->NormativeNS + "|qualif") ThisLeaf.TypeQualif = *attr;
			else if(AttrString == State->NormativeNS + "|value") ThisLeaf.TypeValue = *attr;
			else if(AttrString == State->NormativeNS + "|target_urn") ThisLeaf.SubURN = *attr;
			else if(AttrString == State->NormativeNS + "|minOccurs") ThisLeaf.MinOccurs = *attr;
			else if(AttrString == State->NormativeNS + "|maxOccurs") ThisLeaf.MaxOccurs = *attr;

			// Index the next name
			attr++;
		}
	}


	//! Parse the attributes of a shoot_types
	// DRAGONS: Could do with some optimization as strings are built every time through the test loop
	void ParseTypeShoot(RegisterParserState *State, const char **attrs)
	{
		// Set up a handy reference to this shoot's details
		RegisterLeafData &ThisShoot = State->LeafList.back().ShootList.back();

		const char **attr = attrs;
		while(*attr)
		{
			// Get a std::string version of the attribute name for comparisons and index the value
			std::string AttrString = *(attr++);
			if(AttrString == State->NormativeNS + "|sym") ThisShoot.LeafSymbol = *attr;
			else if(AttrString == State->NormativeNS + "|urn") ThisShoot.LeafURN = *attr;
			else if(AttrString == State->NormativeNS + "|type_urn") ThisShoot.SubURN = *attr;
			else if(AttrString == State->NormativeNS + "|value") ThisShoot.TypeValue = *attr;

			// Index the next name
			attr++;
		}
	}


	//! Handle the data for a single leaf_types entry and contained traits
	/*! Called during the processing of the end tag 
	 */
	void ProcessTypeData(RegisterParserState *State)
	{
		// Set up a handy reference to this leaf's details
		RegisterLeafData &ThisLeaf = State->LeafList.back();

		// Build the new type record
		TypeRecordPtr ThisType = new TypeRecord;

		// Check if we have to derive a symbol from the wildcard entry
		if(ThisLeaf.LeafSymbol.empty()) BuildWildcardedSymbol(State);

		/* Set common properties */
		ThisType->Type = ThisLeaf.LeafSymbol;
		ThisType->Detail = ThisLeaf.TraitsDetail;
		
		// Make the type UL if one is specified
		if(!ThisLeaf.LeafURN.empty()) ThisType->UL = StringToUL(ThisLeaf.LeafURN);
		
		// If no valid UL (including if building the UL failed) build a ranodm end-swapped UUID
		if(!ThisType->UL) ThisType->UL = RandomUL();

		// Add the namespace, if required
		if(!ThisLeaf.NamespaceURI.empty())
		{
			ThisType->SymSpace = SymbolSpace::FindSymbolSpace(ThisLeaf.NamespaceURI);
			if(!ThisType->SymSpace) ThisType->SymSpace = new SymbolSpace(ThisLeaf.NamespaceURI);
		}


		/* Set kind-specific properties */

		if((ThisLeaf.TypeKind == "integer") || (ThisLeaf.TypeKind == "character"))
		// FIXME: Add flagging of character types
		{
            ThisType->Class = TypeBasic;
			ThisType->Size = atoi(ThisLeaf.TypeQualif.c_str());
			if(ThisLeaf.TypeValue == "True") ThisType->Endian = true;
		}
		else if(ThisLeaf.TypeKind == "string")
		{
            ThisType->Class = TypeMultiple;
			ThisType->Base = ThisLeaf.SubURN;

			if(ThisLeaf.MinOccurs.length())
			{
				if((ThisLeaf.MaxOccurs.length() > 0 ) && (ThisLeaf.MaxOccurs != ThisLeaf.MinOccurs)) 
					error("leaf_types for %s has minOccurs=\"%s\" and maxOccurs=\"%s\", which is not currently supported\n", 
					ThisLeaf.LeafSymbol.c_str(), ThisLeaf.MinOccurs.c_str(), ThisLeaf.MaxOccurs.c_str());

				ThisType->Size = atoi(ThisLeaf.MinOccurs.c_str());
			}
		}
		else if(ThisLeaf.TypeKind == "rename")
		{
            ThisType->Class = TypeInterpretation;
			ThisType->Base = ThisLeaf.SubURN;
		}
		else if(ThisLeaf.TypeKind == "reference")
		{
			if(ThisLeaf.TypeQualif == "strong")
			{
	            ThisType->Class = TypeInterpretation;
				ThisType->Base = "Internal-UUID";
	
				ThisType->RefTarget = ThisLeaf.SubURN;
				ThisType->RefType = TypeRefStrong;
			}
			else if(ThisLeaf.TypeQualif == "weak")
			{
	            ThisType->Class = TypeInterpretation;
				ThisType->Base = "Internal-UUID";
	
				ThisType->RefTarget = ThisLeaf.SubURN;
				ThisType->RefType = TypeRefWeak;
			}
			else if(ThisLeaf.TypeQualif == "meta")
			{
	            ThisType->Class = TypeInterpretation;
				ThisType->Base = "Internal-UUID";
	
				ThisType->RefTarget = ThisLeaf.SubURN;
				ThisType->RefType = TypeRefMeta;
			}
			else if(ThisLeaf.TypeQualif == "dict")
			{
	            ThisType->Class = TypeInterpretation;
				ThisType->Base = "Internal-UUID";
	
				ThisType->RefTarget = ThisLeaf.SubURN;
				ThisType->RefType = TypeRefDict;
			}
			// DRAGONS: We treat any unknown ref-types as global
			else
			{
	            ThisType->Class = TypeInterpretation;
				ThisType->Base = "Internal-UUID";
	
				ThisType->RefTarget = ThisLeaf.SubURN;
				ThisType->RefType = TypeRefGlobal;
			}
		}
		else if(ThisLeaf.TypeKind == "set")
		{
			if(ThisLeaf.TypeQualif == "strong")
			{
				ThisType->RefType = TypeRefStrong;
			}
			else if(ThisLeaf.TypeQualif == "weak")
			{
				ThisType->RefType = TypeRefWeak;
			}
			else if(ThisLeaf.TypeQualif == "meta")
			{
				ThisType->RefType = TypeRefMeta;
			}
			else if(ThisLeaf.TypeQualif == "dict")
			{
				ThisType->RefType = TypeRefDict;
			}
			else if(ThisLeaf.TypeQualif == "global")
			{
				ThisType->RefType = TypeRefGlobal;
			}

			if(ThisType->RefType != TypeRefUndefined)
			{
				// Build the record for the actual reference
				TypeRecordPtr ThisSub = new TypeRecord;

				/* Set sub-item properties */
				ThisSub->Class = TypeInterpretation;
				ThisSub->Type = ThisType->Type + "_Item";
				ThisSub->Detail = "Item in " + ThisType->Detail;
				ThisSub->Endian = false;
				ThisSub->ArrayClass = ARRAYIMPLICIT;
				ThisSub->Base = "Internal-UUID";
				ThisSub->UL = RandomUL();
				ThisSub->RefType = ThisType->RefType;
				ThisSub->RefTarget = ThisLeaf.SubURN;

				// Add this type to the list to be built
				State->RXIData->TypesList.push_back(ThisSub);

				// Add this sub-item to the clid list
	            ThisType->Class = TypeMultiple;
				ThisType->ArrayClass = ARRAYEXPLICIT;
				ThisType->Base = ThisSub->UL->GetString();
			}
			else
			{
	            ThisType->Class = TypeMultiple;
				ThisType->ArrayClass = ARRAYEXPLICIT;
				ThisType->Base = ThisLeaf.SubURN;
			}
		}
		else if(ThisLeaf.TypeKind == "array")
		{
			if(ThisLeaf.TypeQualif == "strong")
			{
				ThisType->RefType = TypeRefStrong;
			}
			else if(ThisLeaf.TypeQualif == "weak")
			{
				ThisType->RefType = TypeRefWeak;
			}
			else if(ThisLeaf.TypeQualif == "meta")
			{
				ThisType->RefType = TypeRefMeta;
			}
			else if(ThisLeaf.TypeQualif == "dict")
			{
				ThisType->RefType = TypeRefDict;
			}
			else if(ThisLeaf.TypeQualif == "global")
			{
				ThisType->RefType = TypeRefGlobal;
			}

			if(ThisType->RefType != TypeRefUndefined)
			{
				// Build the record for the actual reference
				TypeRecordPtr ThisSub = new TypeRecord;

				/* Set sub-item properties */
				ThisSub->Class = TypeInterpretation;
				ThisSub->Type = ThisType->Type + "_Item";
				ThisSub->Detail = "Item in " + ThisType->Detail;
				ThisSub->Endian = false;
				ThisSub->ArrayClass = ARRAYIMPLICIT;
				ThisSub->Base = "Internal-UUID";
				ThisSub->UL = RandomUL();
				ThisSub->RefType = ThisType->RefType;
				ThisSub->RefTarget = ThisLeaf.SubURN;

				// Add this type to the list to be built
				State->RXIData->TypesList.push_back(ThisSub);

				// Add this sub-item to the clid list
	            ThisType->Class = TypeMultiple;
				ThisType->ArrayClass = ARRAYEXPLICIT;
				ThisType->Base = ThisSub->UL->GetString();
			}
			else
			{
	            ThisType->Class = TypeMultiple;
				ThisType->ArrayClass = ARRAYIMPLICIT;
				ThisType->Base = ThisLeaf.SubURN;

				if(ThisLeaf.MinOccurs.length())
				{
					if((ThisLeaf.MaxOccurs.length() > 0 ) && (ThisLeaf.MaxOccurs != ThisLeaf.MinOccurs)) 
						error("leaf_types for %s has minOccurs=\"%s\" and maxOccurs=\"%s\", which is not currently supported\n", 
						ThisLeaf.LeafSymbol.c_str(), ThisLeaf.MinOccurs.c_str(), ThisLeaf.MaxOccurs.c_str());

					ThisType->Size = atoi(ThisLeaf.MinOccurs.c_str());
				}
			}
		}
		else if(ThisLeaf.TypeKind == "record")
		{
            ThisType->Class = TypeCompound;

			if(ThisLeaf.ShootList.empty()) warning("Empty record definition found: %s\n", ThisType->Type.c_str());

			RegisterLeafList::iterator it = ThisLeaf.ShootList.begin();
			while(it != ThisLeaf.ShootList.end())
			{
				// Build the new child record
				TypeRecordPtr ThisSub = new TypeRecord;

				/* Set sub-item properties */
				ThisSub->Class = TypeSub;
				ThisSub->Type = (*it).LeafSymbol;
				ThisSub->Detail = (*it).TraitsDetail;
				ThisSub->Endian = false;
				ThisSub->ArrayClass = ARRAYIMPLICIT;
				ThisSub->Base = (*it).SubURN;

				// Make the type UL if one is specified
				if(!(*it).LeafURN.empty()) ThisSub->UL = StringToUL((*it).LeafURN);
				
				// If no valid UL (including if building the UL failed) build a ranodm end-swapped UUID
				if((!ThisSub->UL) || (*ThisSub->UL == Null_UL)) ThisSub->UL = RandomUL();

				// Add this sub-item to the child list
				ThisType->Children.push_back(ThisSub);

				it++;
			}
		}
		else if(ThisLeaf.TypeKind == "enumeration")
		{
            ThisType->Class = TypeEnum;
			ThisType->Base = ThisLeaf.SubURN;

			if(ThisLeaf.ShootList.empty()) warning("Empty enumeration definition found: %s\n", ThisType->Type.c_str());

			RegisterLeafList::iterator it = ThisLeaf.ShootList.begin();
			while(it != ThisLeaf.ShootList.end())
			{
				// Build the new child record
				TypeRecordPtr ThisSub = new TypeRecord;

				/* Set sub-item properties */
				ThisSub->Class = TypeSub;
				ThisSub->Type = (*it).LeafSymbol;
				ThisSub->Detail = (*it).TraitsDetail;
				ThisSub->Value = (*it).TypeValue;
				ThisSub->Endian = false;
				ThisSub->ArrayClass = ARRAYIMPLICIT;

				// Add this value to the enumeration
				ThisType->Children.push_back(ThisSub);

				it++;
			}
		}
		else if(ThisLeaf.TypeKind == "extendible")
		{
            ThisType->Class = TypeEnum;
			ThisType->Base = "Internal-UUID";

			if(ThisLeaf.ShootList.empty()) debug("Extendible definition found with no specified values: %s\n", ThisType->Type.c_str());

			RegisterLeafList::iterator it = ThisLeaf.ShootList.begin();
			while(it != ThisLeaf.ShootList.end())
			{
				// Build the new child record
				TypeRecordPtr ThisSub = new TypeRecord;

				/* Set sub-item properties */
				ThisSub->Class = TypeSub;
				ThisSub->Type = (*it).LeafSymbol;
				ThisSub->Detail = (*it).TraitsDetail;
				ThisSub->Value = (*it).LeafURN;
				ThisSub->Endian = false;
				ThisSub->ArrayClass = ARRAYIMPLICIT;

				// Add this value to the enumeration
				ThisType->Children.push_back(ThisSub);

				it++;
			}
		}
		else if(ThisLeaf.TypeKind == "stream")
		{
	        ThisType->Class = TypeInterpretation;
			ThisType->Base = "Internal-UInt8Array";
		}
		else if(ThisLeaf.TypeKind == "indirect")
		{
	        ThisType->Class = TypeInterpretation;
			ThisType->Base = "Internal-Indirect";
		}
		else if(ThisLeaf.TypeKind == "opaque")
		{
	        ThisType->Class = TypeInterpretation;
			ThisType->Base = "Internal-UInt8Array";
		}
else { warning("Found type kind %s - not yet supported\n", ThisLeaf.TypeKind.c_str()); return; }

		// Set the type name to be a de-duplicated symbol name
		ThisType->Type = DeDuplicateSymbol(State, ThisLeaf.LeafSymbol);

		// Add this type to the list to be built
		State->RXIData->TypesList.push_back(ThisType);
	}


	//! Parse the attributes of a leaf_labels
	// DRAGONS: Could do with some optimization as strings are built every time through the test loop
	void ParseLabelLeaf(RegisterParserState *State, const char **attrs)
	{
		// Set up a handy reference to this leaf's details
		RegisterLeafData &ThisLeaf = State->LeafList.back();

		const char **attr = attrs;
		while(*attr)
		{
			// Get a std::string version of the attribute name for comparisons and index the value
			std::string AttrString = *(attr++);
			if(AttrString == State->NormativeNS + "|sym") ThisLeaf.LeafSymbol = *attr;
			else if(AttrString == State->NormativeNS + "|urn") ThisLeaf.LeafURN = *attr;

			// Index the next name
			attr++;
		}
	}


	//! Handle the data for a single leaf_labels entry and contained traits
	/*! Called during the processing of the end tag 
	 */
	void ProcessLabelData(RegisterParserState *State)
	{
		// Set up a handy reference to this leaf's details
		RegisterLeafData &ThisLeaf = State->LeafList.back();

		// Build the new type record
		TypeRecordPtr ThisType = new TypeRecord;

		// Check if we have to derive a symbol from the wildcard entry
		if(ThisLeaf.LeafSymbol.empty()) BuildWildcardedSymbol(State);

		ThisType->Type = ThisLeaf.LeafSymbol;
		ThisType->Detail = ThisLeaf.TraitsDetail;
		
		// Add the namespace, if required
		if(!ThisLeaf.NamespaceURI.empty())
		{
			ThisType->SymSpace = SymbolSpace::FindSymbolSpace(ThisLeaf.NamespaceURI);
			if(!ThisType->SymSpace) ThisType->SymSpace = new SymbolSpace(ThisLeaf.NamespaceURI);
		}

		// Make the type UL if one is specified
		if(!ThisLeaf.LeafURN.empty()) ThisType->UL = StringToUL(ThisLeaf.LeafURN);
		
		// If no valid UL (including if building the UL failed) build a random end-swapped UUID
		if(!ThisType->UL) ThisType->UL = RandomUL();

		// Set the type name to be a de-duplicated symbol name
		ThisType->Type = DeDuplicateSymbol(State, ThisLeaf.LeafSymbol);

		// Add this type to the list to be built
		State->RXIData->LabelsList.push_back(ThisType);
	}


	//! Handle the data for a single leaf_elements entry and contained traits
	/*! Called during the processing of the end tag 
	 */
	void ProcessElementData(RegisterParserState *State)
	{
		// Set up a handy reference to this leaf's details
		RegisterLeafData &ThisLeaf = State->LeafList.back();

		// Build the new item record
		ClassRecordPtr ThisItem = new ClassRecord;

		// Check if we have to derive a symbol from the wildcard entry
		if(ThisLeaf.LeafSymbol.empty()) BuildWildcardedSymbol(State);

		/* Set item properties */
		ThisItem->Class = ClassItem;
		ThisItem->Name = DeDuplicateSymbol(State, ThisLeaf.LeafSymbol);
		ThisItem->Detail = ThisLeaf.TraitsDetail;
		ThisItem->Base = ThisLeaf.SubURN;

		// Add the namespace, if required
		if(!ThisLeaf.NamespaceURI.empty())
		{
			ThisItem->SymSpace = SymbolSpace::FindSymbolSpace(ThisLeaf.NamespaceURI);
			if(!ThisItem->SymSpace) ThisItem->SymSpace = new SymbolSpace(ThisLeaf.NamespaceURI);
		}

		// DRAGONS: The usage and tag get fixed up later when the groups are built

		// Make the type UL if one is specified
		if(!ThisLeaf.LeafURN.empty()) ThisItem->UL = StringToUL(ThisLeaf.LeafURN);
		
		// DRAGONS: Here we check if this is an InstanceUID as RXI does not flag reference targets
		if(ThisItem->UL && (ThisItem->UL->Matches(InstanceUID_UL)))
		{
			ThisItem->RefType = TypeRefTarget;
		}

		// If no valid UL (including if building the UL failed) build a ranodm end-swapped UUID
		if(!ThisItem->UL) ThisItem->UL = RandomUL();

		// Build an element info block
		ElementInfo Info;
		Info.Item = ThisItem;

		// Insert this as an un-used element - it will get a pointer to its group when used
		State->ElementMap.insert(ElementInfoMap::value_type(*(ThisItem->UL), Info));
	}


	//! Parse the attributes of a shoot_groups
	// DRAGONS: Could do with some optimization as strings are built every time through the test loop
	void ParseGroupShoot(RegisterParserState *State, const char **attrs)
	{
		// Set up a handy reference to this shoot's details
		RegisterLeafData &ThisShoot = State->LeafList.back().ShootList.back();

		const char **attr = attrs;
		while(*attr)
		{
			// Get a std::string version of the attribute name for comparisons and index the value
			std::string AttrString = *(attr++);
			if(AttrString == State->NormativeNS + "|sym") ThisShoot.LeafSymbol = *attr;
			else if(AttrString == State->NormativeNS + "|urn") ThisShoot.LeafURN = *attr;
			else if(AttrString == State->NormativeNS + "|type_urn") ThisShoot.SubURN = *attr;
			else if(AttrString == State->NormativeNS + "|minOccurs") ThisShoot.MinOccurs = *attr;
			else if(AttrString == State->NormativeNS + "|tag") ThisShoot.Tag = *attr;

			// Index the next name
			attr++;
		}
	}


	//! Update the set and pack properties of a given class definition based of byte 6 of its UL
	void UpdateClassType(ClassRecordPtr &ThisClass)
	{
		int Type;
		
		// Set the type from the UL if known and if it is a UL rather than a UUID
		if((ThisClass->UL) && (ThisClass->UL->GetValue()[0] == 0x06)) Type = static_cast<int>(ThisClass->UL->GetValue()[5]);
		// Set as unspecified
		else Type = 0x7f;

		// Handle unspecified types first (which may have jsut been set above)
		if(Type == 0x7f)
		{
			ThisClass->Class = ClassSet;
			ThisClass->MinSize = DICT_KEY_2_BYTE;
			ThisClass->MaxSize = DICT_LEN_BER;
			return;
		}

		// Universal Set
		switch(Type & 0x07)
		{
			// Treat unknown and AAF items as MXF BER:2 sets
			default:
				warning("Unknown coding type 0x%02x for %s\n", Type, ThisClass->Name.c_str());
			case 0x06:
				ThisClass->Class = ClassSet;
				ThisClass->MinSize = DICT_KEY_2_BYTE;
				ThisClass->MaxSize = DICT_LEN_BER;
				break;

			case 0x01:
				ThisClass->Class = ClassSet;
				// DRAGONS: key format is carried in MinSize when defining a set
				ThisClass->MinSize = DICT_KEY_AUTO;
				ThisClass->MaxSize = DICT_LEN_BER;
				break;

			// Global Set
			case 0x02:
				ThisClass->Class = ClassSet;
				// DRAGONS: key format is carried in MinSize when defining a set
				ThisClass->MinSize = DICT_KEY_GLOBAL;

				// DRAGONS: length format is carried in MaxSize when defining a set
				if((Type & 0xe0) == 0x20) ThisClass->MaxSize = DICT_LEN_1_BYTE;
				else if((Type & 0xe0) == 0x40) ThisClass->MaxSize = DICT_LEN_2_BYTE;
				else if((Type & 0xe0) == 0x60) ThisClass->MaxSize = DICT_LEN_4_BYTE;
				else ThisClass->MaxSize = DICT_LEN_BER;
				break;

			// Local Set
			case 0x03:
				ThisClass->Class = ClassSet;
				// DRAGONS: key format is carried in MinSize when defining a set
				if((Type & 0x18) == 0x00) ThisClass->MinSize = DICT_KEY_1_BYTE;
				else if((Type & 0x18) == 0x18) ThisClass->MinSize = DICT_KEY_4_BYTE;
				else ThisClass->MinSize = DICT_KEY_2_BYTE;

				// DRAGONS: length format is carried in MaxSize when defining a set
				if((Type & 0xe0) == 0x20) ThisClass->MaxSize = DICT_LEN_1_BYTE;
				else if((Type & 0xe0) == 0x40) ThisClass->MaxSize = DICT_LEN_2_BYTE;
				else if((Type & 0xe0) == 0x60) ThisClass->MaxSize = DICT_LEN_4_BYTE;
				else ThisClass->MaxSize = DICT_LEN_BER;
				break;

			// Variable Length Pack
			case 0x04:
				ThisClass->Class = ClassPack;
				// DRAGONS: key format is carried in MinSize when defining a set
				ThisClass->MinSize = DICT_KEY_NONE;

				// DRAGONS: length format is carried in MaxSize when defining a set
				if((Type & 0xe0) == 0x20) ThisClass->MaxSize = DICT_LEN_1_BYTE;
				else if((Type & 0xe0) == 0x40) ThisClass->MaxSize = DICT_LEN_2_BYTE;
				else if((Type & 0xe0) == 0x60) ThisClass->MaxSize = DICT_LEN_4_BYTE;
				else ThisClass->MaxSize = DICT_LEN_BER;
				break;

			// Defined Length Pack
			case 0x05:
				ThisClass->Class = ClassPack;
				// DRAGONS: key format is carried in MinSize when defining a set
				ThisClass->MinSize = DICT_KEY_NONE;

				// DRAGONS: length format is carried in MaxSize when defining a set
				ThisClass->MaxSize = DICT_LEN_NONE;
				break;
		}
	}


	//! Handle the data for a single leaf_groups entry and contained traits
	/*! Called during the processing of the end tag 
	 */
	void ProcessGroupData(RegisterParserState *State)
	{
		// Set up a handy reference to this leaf's details
		RegisterLeafData &ThisLeaf = State->LeafList.back();

		// Build the new group record
		ClassRecordPtr ThisClass = new ClassRecord;

		// Check if we have to derive a symbol from the wildcard entry
		if(ThisLeaf.LeafSymbol.empty()) BuildWildcardedSymbol(State);

		/* Set group properties */
		ThisClass->Class = ClassItem;
		ThisClass->Name = DeDuplicateSymbol(State, ThisLeaf.LeafSymbol);
		ThisClass->Detail = ThisLeaf.TraitsDetail;
		ThisClass->Base = ThisLeaf.SubURN;

		// Make the type UL if one is specified
		if(!ThisLeaf.LeafURN.empty()) ThisClass->UL = StringToUL(ThisLeaf.LeafURN);

		// Examine allowed coding types
		DataChunkPtr Coding;
		if(!ThisLeaf.Coding.empty())
		{
			if(ThisClass->UL->GetValue()[0] != 0x06)
			{
				error("Group %s has a coding of %s specified, but has an identifier of %s which is not a UL\n", 
					  ThisClass->Name.c_str(), ThisLeaf.Coding.c_str(), ThisClass->UL->GetString().c_str());
			}
			else
			{
				Coding = Hex2DataChunk(ThisLeaf.Coding);
				
				// Ignore zero-length coding strings
				if(Coding->Size == 0) Coding = NULL;
			}
		}

		// Work out the group type
		UpdateClassType(ThisClass);

		// If no valid UL (including if building the UL failed) build a random end-swapped UUID
		if(!ThisClass->UL) ThisClass->UL = RandomUL();

		// Add the namespace, if required
		if(!ThisLeaf.NamespaceURI.empty())
		{
			ThisClass->SymSpace = SymbolSpace::FindSymbolSpace(ThisLeaf.NamespaceURI);
			if(!ThisClass->SymSpace) ThisClass->SymSpace = new SymbolSpace(ThisLeaf.NamespaceURI);
		}

		/* Process contained items */
		RegisterLeafList::iterator it = ThisLeaf.ShootList.begin();
		while(it != ThisLeaf.ShootList.end())
		{
			// Skip inactive shoots
			if(!(*it).Active)
			{
				it++;
				continue;
			}

			// Make the type UL if one is specified
			ULPtr ShootUL;
			if(!(*it).LeafURN.empty()) ShootUL = StringToUL((*it).LeafURN);

			// Find the definition of this element
			ElementInfoMap::iterator Shoot_it = State->ElementMap.end();
			if(ShootUL) Shoot_it = State->ElementMap.find(*ShootUL);

			if(Shoot_it == State->ElementMap.end())
			{
				error("Group %s contains unknown member %s\n", ThisClass->Name.c_str(), (*it).LeafSymbol.c_str());
			}
			else
			{
				// If this element has already been used in a group we have to build a copy and use that instead
				if((*Shoot_it).second.Group)
				{
					// Make a new class record for this rename
					ClassRecordPtr NewItem = new ClassRecord;

					// Copy the basic info
					NewItem->Name = (*Shoot_it).second.Item->Name;
					NewItem->Detail = (*Shoot_it).second.Item->Detail;
					NewItem->MinSize = (*Shoot_it).second.Item->MinSize;
					NewItem->MaxSize = (*Shoot_it).second.Item->MaxSize;
					NewItem->Tag = (*Shoot_it).second.Item->Tag;
					NewItem->RefType = ClassRefUndefined;

					// Set as a rename of the original
					NewItem->Class = ClassRename;
					NewItem->Base = (*Shoot_it).second.Item->UL->GetString();

					// Allocate it a random UL
					NewItem->UL = RandomUL();

					// Build an element info block
					ElementInfo Info;
					Info.Item = NewItem;

					// Insert this as an un-used element - it will get a pointer to its group when used
					// DRAGONS: We also update the iterator to indicate that this is the item to use
					Shoot_it = State->ElementMap.insert(ElementInfoMap::value_type(*(NewItem->UL), Info)).first;
				}

				// Claim ownership of this element
				(*Shoot_it).second.Group = ThisClass;

				// Update the symbol per specified name which may be an alias
				if(!(*it).LeafSymbol.empty())
				{
					debug("Applying alias or member name %s in place of %s in %s\n", (*it).LeafSymbol.c_str(), (*Shoot_it).second.Item->Name.c_str(), ThisLeaf.LeafSymbol.c_str());
					(*Shoot_it).second.Item->Name = (*it).LeafSymbol;
				}

				// Add this as a child item
				ThisClass->Children.push_back((*Shoot_it).second.Item);

				// TODO: Finish
			}

			it++;
		}

		// Add this new group to the list of classes to build
		State->RXIData->GroupList.push_back(ThisClass);

		/* Iterate through permitted codings (if this is a multiply coded group) */
		// TODO:
		if(Coding)
		{
			// Set the symbol space for the generic version
			ThisClass->SymSpace = GetCodingSymbolSpace(ThisClass->UL->GetValue()[5], State);

			int i;
			for(i=0; i<static_cast<int>(Coding->Size); i++)
			{
				// Make a new class record for this coding
				ClassRecordPtr NewCoding = new ClassRecord;

				// Copy the basic info
				NewCoding->Name = ThisClass->Name;
				NewCoding->Detail = ThisClass->Detail;
				NewCoding->Tag = ThisClass->Tag;
				NewCoding->RefType = ClassRefUndefined;

				// Make the new item's UL be based on te 
				NewCoding->UL = new UL(ThisClass->UL);
				NewCoding->UL->Set(5, Coding->Data[i]);
				NewCoding->SymSpace = GetCodingSymbolSpace(Coding->Data[i], State);

				// Now set the correct set of pack properties for this coding
				UpdateClassType(NewCoding);

				// Set as a rename of the original - after calling UpdateClassType() as this changes Class
				NewCoding->Class = ClassRename;
				NewCoding->Base = ThisClass->UL->GetString();

				// Add this new coding to the list of classes to build
				State->RXIData->GroupList.push_back(NewCoding);
			}
		}
	}


	//! Parse the attributes of a leaf_elements
	void ParseElementLeaf(RegisterParserState *State, const char **attrs)
	{
		// Set up a handy reference to this leaf's details
		RegisterLeafData &ThisLeaf = State->LeafList.back();

		const char **attr = attrs;
		while(*attr)
		{
			// Get a std::string version of the attribute name for comparisons and index the value
			std::string AttrString = *(attr++);
			if(AttrString == State->NormativeNS + "|sym") ThisLeaf.LeafSymbol = *attr;
			else if(AttrString == State->NormativeNS + "|urn") ThisLeaf.LeafURN = *attr;
			else if(AttrString == State->NormativeNS + "|type_urn") ThisLeaf.SubURN = *attr;
			else if(AttrString == State->NormativeNS + "|target_urn") 
				ThisLeaf.SubURN = *attr;

			// Index the next name
			attr++;
		}
	}


	//! Parse the attributes of a leaf_groups
	void ParseGroupLeaf(RegisterParserState *State, const char **attrs)
	{
		// Set up a handy reference to this leaf's details
		RegisterLeafData &ThisLeaf = State->LeafList.back();

		const char **attr = attrs;
		while(*attr)
		{
			// Get a std::string version of the attribute name for comparisons and index the value
			std::string AttrString = *(attr++);
			if(AttrString == State->NormativeNS + "|sym") ThisLeaf.LeafSymbol = *attr;
			else if(AttrString == State->NormativeNS + "|urn") ThisLeaf.LeafURN = *attr;
			else if(AttrString == State->NormativeNS + "|coding") ThisLeaf.Coding = *attr;
			else if(AttrString == State->NormativeNS + "|parent_urn") ThisLeaf.SubURN = *attr;
			else if(AttrString == State->NormativeNS + "|isAbstract") ThisLeaf.IsAbstract = *attr;

			// Index the next name
			attr++;
		}
	}


	//! Parse the i:app element inside traits for the current leaf or shoot
	/*! Example i:app format: 
	 *     The following i:app element within "PrimaryPackage" means that in ASPA v0.8 and earlier
	 *     this was called PrimaryMob, and that it is also valid in AAF v1.2 and later, also with name PrimaryMob
	 *
	 *     <i:app>AAF[1.2:]~PrimaryMob ASPA[:0.8]~PrimaryMob</i:app>
	 */
	void ParseApplication(RegisterParserState *State, RegisterLeafData &ThisLeaf, std::string Application)
	{
		debug("Comparing this application \"%s\" (version=%.2f) against \"%s\"\n", 
			  State->Application.c_str(), static_cast<float>(State->AppVersion)/100.0, Application.c_str());

		// We start off by assuming that we have not matched
		ThisLeaf.Active = false;

		// Iterate through space separated applications
		std::string Item;
		size_t Pos = 0;
		while(Pos != std::string::npos)
		{
			// Search for the end of this item
			size_t EndPos = Application.find_first_of(' ', Pos+1);
			
			// Last item in the list
			if(EndPos == std::string::npos)
			{
				// Take the rest of the string
				Item = Application.substr(Pos);

				// Flag end-of-list by passing this npos to Pos
				Pos = EndPos;
			}
			else
			{
				// Take this item from the string
				Item = Application.substr(Pos, EndPos-Pos);

				// Next item starts after the space
				Pos = EndPos + 1;
			}

			debug(" Testing \"%s\"\n", Item.c_str());

			// Locate the end of the application name
			EndPos = Item.find_first_of("[~");

			/* Now we can quickly stop checking this item if it is not our application */

			// No version range or alias
			if(EndPos == std::string::npos)
			{
				if(Item != State->Application) 
				{
					debug(" ->Not a match by name\n");
					continue;
				}
			}
			// Either a version number, or an alias (or both)
			else
			{
				if(Item.substr(0, EndPos) != State->Application)
				{
					debug(" ->Not a match by name \"%s\"\n", Item.substr(0, EndPos).c_str());
					continue;
				}
			}

			/* If we have got here, the application name matches so we need to check the version number range and the look for an alias */

			/* Process version number range */
			// Was the first thing after the application name a version number range?
			if(Item[EndPos] == '[')
			{
				int VerMin = 0;
				int VerMax = std::numeric_limits<int>::max();

				// Locate the seperator, if there is one
				size_t VerSep = Item.find_first_of(':', EndPos);

				// Read the minimum version (which will be zero if omitted) either reading to the end of the string, or the seperator
				VerMin = ParseAppVersion(Item.substr(EndPos + 1, (VerSep == std::string::npos) ? std::string::npos : (VerSep - EndPos) - 1));

				// If there is no seperator the max is the same as the min (single version)
				if(VerSep == std::string::npos) VerMax = VerMin;
				else
				{
					// Read the max version number from after the seperator
					VerMax = ParseAppVersion(Item.substr(VerSep + 1));

					// If there was no max after the seperator, treat this as unbounded maximum
					if(VerMax == 0) VerMax = std::numeric_limits<int>::max();
				}

				debug("  Version range = %.2f to %.2f\n", static_cast<float>(VerMin)/100.0, static_cast<float>(VerMax)/100.0);

				// Do the version number check and skip this item if we don't match
				if((State->AppVersion < VerMin) || (State->AppVersion > VerMax))
				{
					debug("  ->Outside range\n");
					continue;
				}

				// We have matched application name and version number - this leaf or shoot is active
				debug("  ->Match\n");
				ThisLeaf.Active = true;

				// Scan for an alias start character and pot that in EndPos so the alias code below works the same as if there were no version range
				EndPos = Item.find_first_of('~', EndPos + 1);

				// If there is no alias then no point processing it, however we still need to check following items as there could be
				// an alias that also matches this application e.g. "AAF[1.0:] AAF[1.0:1.2]~OldName" means that this leaf is valid in all
				// versions of AAF from 1.0 onwards, but in versions 1.0 to 1.2 it was called "OldName"
				if(EndPos == std::string::npos) continue;
			}
			else
			{
				// We have matched application name and there is no version number to check - this leaf or shoot is active
				ThisLeaf.Active = true;
				debug(" ->Match (all versions)\n");
			}

			/* If we have got here, the application name and version match so we just check for an alias */

			if(State->AppAlias && (Item[EndPos] == '~'))
			{
				debug("  =>Alias %s to %s\n", ThisLeaf.LeafSymbol.c_str(), Item.substr(EndPos+1).c_str());

				// Rename using the alias
				ThisLeaf.LeafSymbol = Item.substr(EndPos+1);
			}
		}
	}

	
	//! Determine the namespace URI to use for the next new leaf under the current level
	std::string DetermineNamespace(RegisterParserState *State)
	{
		if(!State->LeafList.empty()) return State->LeafList.back().NamespaceURI;
		if(!State->NamespaceList.empty()) return State->NamespaceList.back().second;
		return "";
	}
}


namespace
{
	//! XML callback - Handle character data
	void CharacterHandler(void *user_data, const XML_Char *s, int len)
	{
		// Allocate a buffer so we can convert the data and terminate it
		char *pBuffer = new char[len + 1];
		char *p = pBuffer;

		// Copy into the buffer while converting to char (will only work for 8-bit data!!)
		while(len--)
		{
			*(p++) = (char)*(s++);
		};

		// Terminate this new string
		*p = '\0';

		// Add to the character data buffer
		RegisterParserState *State = (RegisterParserState*)user_data;
		State->CharData += pBuffer;

		// Free our working buffer
		delete[] pBuffer;
	}


	//! XML callback - Deal with start tag of an element
	void DictLoad_startElement(void *user_data, const char *name, const char **attrs)
	{
		RegisterParserState *State = (RegisterParserState*)user_data;

		// Increment our tag-depth
		// DRAGONS: This means that any tests done now will need to be aware this has already been incremented
		State->Depth++;

		switch(State->State)
		{
			// Find outer tag and verify that it is 
			case DictStateIdle:
			{
#ifndef HAVE_EXPAT
				// Check for RXI dictionary format
				if(strcmp(name, "register") == 0)
				{
					error("Unable to parse RXI dictionary format unless compiled with Expat XML parser\n");
					State->State = DictStateError;
					break;
				}
#else
				std::string StringName = name;
				size_t SepPos = StringName.find_last_of('|');
				std::string Namespace;
				if(SepPos != std::string::npos) Namespace = StringName.substr(0, SepPos);
				std::string BareName = (SepPos == std::string::npos) ? StringName : StringName.substr(SepPos + 1);

				if(BareName == "register")
				{
					if((Namespace.length() < 3) || Namespace.substr(Namespace.length()-3, 3) != "RXI")
					{
						warning("Dictionary file resembles RXI in that the outer tag is \"register\" but namespace is %s\n", Namespace.c_str());
					}
					State->State = DictStateRegister;
					State->BaseNS = Namespace;
					State->NormativeNS = Namespace + "/normative";
					break;
				}
#endif
				// Normal start of unified dictionary, or start of old-style classes dictionary
				else if((strcmp(name, "MXFDictionary") == 0) || (strcmp(name, "MXFTypes") == 0))
				{
					State->RXIData->LegacyFormat = true;
					State->State = DictStateError;
					break;					
				}
				else
				{
					// Allow MXF dictionaries to be wrapped inside other XML files
					debug("Stepping into outer level <%s>\n", name);
					break;
				}
			}

			// Scan for leaves
			case DictStateRegister:
			{
				if(name == State->BaseNS + "|leaf_types")
				{
					// Set up for parsing this leaf
					State->State = DictStateLeaf;
					std::string Namespace = DetermineNamespace(State);
					State->LeafList.resize(State->LeafList.size() + 1);
					State->LeafList.back().LeafType = RegLeafType;
					State->LeafList.back().Active = true;
					State->LeafList.back().NamespaceURI = Namespace;
					ParseTypeLeaf(State, attrs);
					break;
				}

				if(name == State->BaseNS + "|leaf_elements")
				{
					// Set up for parsing this leaf
					State->State = DictStateLeaf;
					std::string Namespace = DetermineNamespace(State);
					State->LeafList.resize(State->LeafList.size() + 1);
					State->LeafList.back().LeafType = RegLeafElement;
					State->LeafList.back().Active = true;
					State->LeafList.back().NamespaceURI = Namespace;
					ParseElementLeaf(State, attrs);
					break;
				}

				// Stems in groups still need to be built as they are often part of the derivation chain
				// DRAGONS: "stem" was the old name for "wildcard"
				if((name == State->BaseNS + "|wildcard") || (name == State->BaseNS + "|stem"))
				{
					bool GroupStem = false;
					const char **attr = attrs;
					while(*attr)
					{
						if(*(attr++) == State->NormativeNS + "|reg")
						{
							if(strcmp(*attr, "groups") == 0) GroupStem = true;
							break;
						}
						attr++;
					}

					// Parse this wildcard
					ParseWildcard(State, attrs);

					// Check if a namespace is being defined
					ParseNamespace(State, attrs);

					if(GroupStem)
					{
						// Set up for parsing this leaf
						State->State = DictStateLeaf;
						std::string Namespace = DetermineNamespace(State);
						State->LeafList.resize(State->LeafList.size() + 1);
						State->LeafList.back().LeafType = RegLeafGroup;
						State->LeafList.back().Active = true;
						State->LeafList.back().NamespaceURI = Namespace;
						ParseGroupLeaf(State, attrs);
						break;
					}
				}

				if(name == State->BaseNS + "|leaf_groups")
				{
					// Set up for parsing this leaf
					State->State = DictStateLeaf;
					std::string Namespace = DetermineNamespace(State);
					State->LeafList.resize(State->LeafList.size() + 1);
					State->LeafList.back().LeafType = RegLeafGroup;
					State->LeafList.back().Active = true;
					State->LeafList.back().NamespaceURI = Namespace;
					ParseGroupLeaf(State, attrs);
					break;
				}

				if(name == State->BaseNS + "|leaf_labels")
				{
					// Set up for parsing this leaf
					State->State = DictStateLeaf;
					std::string Namespace = DetermineNamespace(State);
					State->LeafList.resize(State->LeafList.size() + 1);
					State->LeafList.back().LeafType = RegLeafLabel;
					State->LeafList.back().Active = true;
					State->LeafList.back().NamespaceURI = Namespace;
					ParseLabelLeaf(State, attrs);
					break;
				}

				// Buds in groups still need to be built as they are often part of the derivation chain
				if(name == State->BaseNS + "|bud_groups")
				{
					// Set up for parsing this stem-under-(leaf or stem)
					std::string Namespace = DetermineNamespace(State);
					State->LeafList.resize(State->LeafList.size() + 1);
					State->LeafList.back().LeafType = RegLeafGroup;
					State->LeafList.back().Active = true;
					State->LeafList.back().NamespaceURI = Namespace;
					ParseGroupLeaf(State, attrs);
					break;
				}

				if((name == State->BaseNS + "|trunk") || (name == State->BaseNS + "|node"))
				{
					// Check if a namespace is being defined
					ParseNamespace(State, attrs);
					break;
				}

				break;
			}
			
			// Check for traits or links/shoots within leaves
			case DictStateLeaf:
			{
				if(name == State->BaseNS + "|traits") 
				{
					State->InTraits = true;
					break;
				}
				
				// DRAGONS: "shoot" was the old name for "link"
				if((name == State->BaseNS + "|link_types") || (name == State->BaseNS + "|shoot_types"))
				{
					// Set up for parsing this shoot
					State->State = DictStateShoot;
					State->LeafList.back().ShootList.resize(State->LeafList.back().ShootList.size() + 1);
					State->LeafList.back().ShootList.back().LeafType = RegLeafType;
					State->LeafList.back().ShootList.back().Active = true;
					ParseTypeShoot(State, attrs);
					break;
				}
				
				// DRAGONS: "shoot" was the old name for "link"
				if((name == State->BaseNS + "|link_groups") || (name == State->BaseNS + "|shoot_groups"))
				{
					// Set up for parsing this shoot
					State->State = DictStateShoot;
					State->LeafList.back().ShootList.resize(State->LeafList.back().ShootList.size() + 1);
					State->LeafList.back().ShootList.back().LeafType = RegLeafGroup;
					State->LeafList.back().ShootList.back().Active = true;
					ParseGroupShoot(State, attrs);
					break;
				}
				
				RegisterLeafType LeafType = State->LeafList.back().LeafType;

				if((LeafType == RegLeafElement) && (name == State->BaseNS + "|leaf_elements"))
				{
					// Set up for parsing this leaf-under-leaf
					std::string Namespace = DetermineNamespace(State);
					State->LeafList.resize(State->LeafList.size() + 1);
					State->LeafList.back().LeafType = RegLeafElement;
					State->LeafList.back().Active = true;
					State->LeafList.back().NamespaceURI = Namespace;
					ParseElementLeaf(State, attrs);
					break;
				}
				
				if((LeafType == RegLeafGroup) && (name == State->BaseNS + "|leaf_groups"))
				{
					// Set up for parsing this leaf-under-leaf
					std::string Namespace = DetermineNamespace(State);
					State->LeafList.resize(State->LeafList.size() + 1);
					State->LeafList.back().LeafType = RegLeafGroup;
					State->LeafList.back().Active = true;
					State->LeafList.back().NamespaceURI = Namespace;
					ParseGroupLeaf(State, attrs);
					break;
				}

				if((LeafType == RegLeafLabel) && (name == State->BaseNS + "|leaf_labels"))
				{
					warning("Unexpected leaf_labels under leaf_labels\n");
					break;
				}

				// Stems in groups still need to be built as they are often part of the derivation chain
				// DRAGONS: "stem" was the old name for "wildcard"
				if((name == State->BaseNS + "|wildcard") || (name == State->BaseNS + "|stem"))
				{
					bool GroupStem = (LeafType == RegLeafGroup);
					const char **attr = attrs;
					while(*attr)
					{
						if(*(attr++) == State->NormativeNS + "|reg")
						{
							if(strcmp(*attr, "groups") == 0) GroupStem = true; else GroupStem = false;
							break;
						}
						attr++;
					}

					// Parse this wildcard
					ParseWildcard(State, attrs);

					// Check if a namespace is being defined
					ParseNamespace(State, attrs);

					if(GroupStem)
					{
						// Set up for parsing this stem-under-(leaf or stem)
						std::string Namespace = DetermineNamespace(State);
						State->LeafList.resize(State->LeafList.size() + 1);
						State->LeafList.back().LeafType = RegLeafGroup;
						State->LeafList.back().Active = true;
						State->LeafList.back().NamespaceURI = Namespace;
						ParseGroupLeaf(State, attrs);
					}
					break;
				}

				// Buds in groups still need to be built as they are often part of the derivation chain
				// FIXME: Can a bud appear at this level?
				if(name == State->BaseNS + "|bud_groups")
				{
					// Set up for parsing this stem-under-(leaf or stem)
					std::string Namespace = DetermineNamespace(State);
					State->LeafList.resize(State->LeafList.size() + 1);
					State->LeafList.back().LeafType = RegLeafGroup;
					State->LeafList.back().Active = true;
					State->LeafList.back().NamespaceURI = Namespace;
					ParseGroupLeaf(State, attrs);
					break;
				}

				if(State->InTraits)
				{
					// Start capturing characters
#ifdef HAVE_EXPAT
					State->CharData = "";
					XML_SetCharacterDataHandler(State->Parser, CharacterHandler);
#endif
				}
				break;
			}

			// Check for traits within links/shoots
			case DictStateShoot:
			{
				if(name == State->BaseNS + "|traits") 
				{
					State->InTraits = true;
				}
				else if(State->InTraits)
				{
					// Start capturing characters
#ifdef HAVE_EXPAT
					State->CharData = "";
					XML_SetCharacterDataHandler(State->Parser, CharacterHandler);
#endif
				}
				break;
			}

			default:		// All other cases
				break;
		}

		return;
	}
}



namespace
{
	//! XML callback - Deal with end tag of an element
	void DictLoad_endElement(void *user_data, const char *name)
	{
		RegisterParserState *State = (RegisterParserState*)user_data;

		// Decrement our tag-depth
		// DRAGONS: This means that any tests done now will need to be aware this has already been decremented
		State->Depth--;

		// Remove the most recent namespace URI if we have just stepped out of the tag level in which it was defined
		if(!State->NamespaceList.empty())
		{
			if(State->NamespaceList.back().first > State->Depth) State->NamespaceList.pop_back();
		}

		switch(State->State)
		{
			case DictStateLeaf:
			{
				if(State->InTraits)
				{
					if(name == State->BaseNS + "|traits") State->InTraits = false;
					else if(name == State->NormativeNS + "|name") State->LeafList.back().TraitsName = State->CharData;
					else if(name == State->NormativeNS + "|detail") State->LeafList.back().TraitsDetail = State->CharData;
		//			else if(name == State->BaseNS + "/stated|length") State->LeafList.back().TraitsLength = State->CharData;

					// Parse i:app only if we have an application mask set
					if(!State->Application.empty())
					{
						if(name == State->BaseNS + "/informative|app") ParseApplication(State, State->LeafList.back(), State->CharData);
					}
#ifdef HAVE_EXPAT
					// Stop capturing characters
					XML_SetCharacterDataHandler(State->Parser, NULL);
#endif
				}
				else
				{
					RegisterLeafType LeafType = State->LeafList.back().LeafType;

					if(LeafType == RegLeafType && (name == State->BaseNS + "|leaf_types"))
					{
						// Process the leaf data if it is not inactive
						if(State->LeafList.back().Active) ProcessTypeData(State);

						// Remove this leaf from the stack and clear the state if back at the trunk
						State->LeafList.pop_back();
						if(State->LeafList.empty()) State->State = DictStateRegister;
					}
					else if(LeafType == RegLeafElement && (name == State->BaseNS + "|leaf_elements"))
					{
						ProcessElementData(State);

						// Remove this leaf from the stack and clear the state if back at the trunk
						State->LeafList.pop_back();
						if(State->LeafList.empty()) State->State = DictStateRegister;
					}
					else if(LeafType == RegLeafGroup && ((name == State->BaseNS + "|leaf_groups") || (name == State->BaseNS + "|stem") || (name == State->BaseNS + "|bud_groups")))
					{
						// Process the leaf data if it is not inactive
						if(State->LeafList.back().Active) ProcessGroupData(State);

						// Remove this leaf from the stack and clear the state if back at the trunk
						State->LeafList.pop_back();
						if(State->LeafList.empty()) State->State = DictStateRegister;
					}
					else if(LeafType == RegLeafLabel && (name == State->BaseNS + "|leaf_labels"))
					{
						// Process the leaf data if it is not inactive
						if(State->LeafList.back().Active) ProcessLabelData(State);

						// Remove this leaf from the stack and clear the state if back at the trunk
						State->LeafList.pop_back();
						if(State->LeafList.empty()) State->State = DictStateRegister;
					}
					// DRAGONS: "stem" was the old name for "wildcard"
					else if((!State->WildcardList.empty()) && (name == State->BaseNS + "|wildcard") || (name == State->BaseNS + "|stem"))
					{
						// Remove this wildcard
						State->WildcardList.pop_back();

						// If this is a groups wildcard, remove the leaf that we added to force it to be built
						if(LeafType == RegLeafGroup)
						{
							if(!State->LeafList.empty())
							{
								// Remove this leaf from the stack and clear the state if back at the trunk
								State->LeafList.pop_back();
							}
							if(State->LeafList.empty()) State->State = DictStateRegister;
						}
					}
					break;
				}
				break;
			}

			case DictStateShoot:
			{
				if(State->InTraits)
				{
					if(name == State->BaseNS + "|traits") State->InTraits = false;
					else if(name == State->NormativeNS + "|name") State->LeafList.back().ShootList.back().TraitsName = State->CharData;
					else if(name == State->NormativeNS + "|detail") State->LeafList.back().ShootList.back().TraitsDetail = State->CharData;
		//			else if(name == State->BaseNS + "/stated|length") State->LeafList.back().ShootList.back().TraitsLength = State->CharData;

					// Parse i:app only if we have an application mask set
					if(!State->Application.empty())
					{
						if(name == State->BaseNS + "/informative|app") ParseApplication(State, State->LeafList.back().ShootList.back(), State->CharData);
					}
#ifdef HAVE_EXPAT
					// Stop capturing characters
					XML_SetCharacterDataHandler(State->Parser, NULL);
#endif
				}
				else
				{
					RegisterLeafType LeafType = State->LeafList.back().ShootList.back().LeafType;

					if(LeafType == RegLeafType && ((name == State->BaseNS + "|link_types") || (name == State->BaseNS + "|shoot_types")))
					{
						State->State = DictStateLeaf;
					}
					else if(LeafType == RegLeafElement && ((name == State->BaseNS + "|link_elements") || (name == State->BaseNS + "|shoot_elements")))
					{
						State->State = DictStateLeaf;
					}
					else if(LeafType == RegLeafGroup && ((name == State->BaseNS + "|link_groups") || (name == State->BaseNS + "|shoot_groups")))
					{
						State->State = DictStateLeaf;
					}
					break;
				}
				break;
			}

			case DictStateRegister:
				// DRAGONS: "stem" was the old name for "wildcard"
				if((!State->WildcardList.empty()) && (name == State->BaseNS + "|wildcard") || (name == State->BaseNS + "|stem"))
				{
					State->WildcardList.pop_back();
				}
				break;

			case DictStateIdle:
			case DictStateError:
			default:
				break;
		}

		return;
	}
}

