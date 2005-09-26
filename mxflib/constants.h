/*! \file	constants.h
 *	\brief	Definitions of constant values and "magic" numbers
 *
 *	\version $Id: constants.h,v 1.1 2005/09/26 08:35:59 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2005, Matt Beard
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

#ifndef MXFLIB__CONSTANTS_H
#define MXFLIB__CONSTANTS_H

namespace mxflib
{
	/*********************************************/
	/*											 */
	/*          RP224 Based Definitions          */
	/*											 */
	/*********************************************/

	/* Track identification data definitions */

	//! Track type data def for 12M timecode tracks (without use bits)
	const UInt8 TrackTypeDataDefTimecode12M[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00 };
	//! Number of bytes to compare to tell if this is a timecode 12M track
	const int TrackTypeDataDefTimecode12MCompare = 13;

	//! Track type data def for 12M timecode tracks (with use bits)
	const UInt8 TrackTypeDataDefTimecode12MUser[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x02, 0x00, 0x00, 0x00 };
	//! Number of bytes to compare to tell if this is a timecode 12M track (with user bits)
	const int TrackTypeDataDefTimecode12MUserCompare = 13;

	//! Track type data def for 309M timecode tracks
	const UInt8 TrackTypeDataDefTimecode309M[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x03, 0x00, 0x00, 0x00 };
	//! Number of bytes to compare to tell if this is a timecode 309M track
	const int TrackTypeDataDefTimecode309MCompare = 13;

	//! Track type data def for picture tracks
	const UInt8 TrackTypeDataDefPicture[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00 };
	//! Number of bytes to compare to tell if this is a picture track
	const int TrackTypeDataDefPictureCompare = 16;

	//! Track type data def for sound tracks
	const UInt8 TrackTypeDataDefSound[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00 };
	//! Number of bytes to compare to tell if this is a sound track
	const int TrackTypeDataDefSoundCompare = 16;

	//! Track type data def for data tracks
	const UInt8 TrackTypeDataDefData[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00 };
	//! Number of bytes to compare to tell if this is a data track
	const int TrackTypeDataDefDataCompare = 16;

	//! Track type data def for DM tracks
	const UInt8 TrackTypeDataDefDM[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00 };
	//! Number of bytes to compare to tell if this is a DM track
	const int TrackTypeDataDefDMCompare = 13;
}

#endif //MXFLIB__CONSTANTS_H
