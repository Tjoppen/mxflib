/*! \file	primer.cpp
 *	\brief	Implementation of Primer class
 *
 *			The Primer class holds data about the mapping between local
 *          tags in a partition and the UL that gives access to the full
 *			definition
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

#include "mxflib.h"

using namespace mxflib;


//! Read the primer from a buffer
/*!	/ret Number of bytes read
 */
Uint32 Primer::ReadValue(const Uint8 *Buffer, Uint32 Size)
{
	debug("Reading Primer\n");

	// Start off empty
	clear();

	if(Size < 8)
	{
		error("Primer too small, must be at least 8 bytes!\n");
		return 0;
	}

	// Each entry in the primer is 18 bytes
	Uint32 Items = (Size-8) / 18;

	// Validate the size and only read whole items
	if((Items * 18) != (Size-8))
	{
		error("Primer not an integer number of multiples of 18 bytes!\n");
		Size = (Items * 18) + 8;
	}

	// Read the vector header
	Uint32 ClaimedItems = GetU32(Buffer);
	Uint32 ClaimedItemSize = GetU32(&Buffer[4]);
	Buffer += 8;

	if(ClaimedItemSize != 18)
	{
		error("Malformed vector header in Primer - each entry is 18 bytes, size in vector header is %d\n", ClaimedItemSize);
	}
	else
	{
		if(Items != ClaimedItems)
		{
			error("Malformed vector header in Primer - number of entries is %d, vector header claims %d\n", Items, ClaimedItems);
		}
	}

	// Read each item
	while(Items--)
	{
		Tag ThisTag = GetU16(Buffer);
		Buffer += 2;

		UL ThisUL(Buffer);
		Buffer += 16;

		// Add this new entry to the primer
		insert(Primer::value_type(ThisTag, ThisUL));

		debug("  %s -> %s\n", Tag2String(ThisTag).c_str(), ThisUL.GetString().c_str());
	}

	// Return how many bytes we actually read
	return Size;
}

