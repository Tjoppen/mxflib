/*! \file	klvobject.cpp
 *	\brief	Implementation of classes that define basic KLV objects
 *
 *			Class KLVObject holds info about a KLV object
 *
 *	\version $Id: klvobject.cpp,v 1.1.2.2 2004/05/16 10:47:03 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2003, Matt Beard
 *	Portions Copyright (c) 2003, Metaglue Corporation
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

//! Build a new KLVObject
KLVObject::KLVObject(ULPtr ObjectUL)
{
	TheUL = ObjectUL;
	
	Init();
}


//! Initialise newly built KLVObject
void KLVObject::Init(void)
{
	IsConstructed = true;
	SourceOffset = -1;
	KLSize = 0;
	SourceFile = NULL;

//	ObjectName = "";
}


//! Get text that describes where this item came from
std::string KLVObject::GetSource(void) 
{ 
	if(SourceFile) return SourceFile->Name; else return "memory buffer"; 
}

//! Get a GCElementKind structure
GCElementKind KLVObject::GetGCElementKind(void)
{
	GCElementKind ret;

	//! Base of all standard GC keys
	/*! DRAGONS: version number is hard-coded as 1 */
	const Uint8 DegenerateGCLabel[12] = { 0x06, 0x0E, 0x2B, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01 };
	
	// Note that we first test the 11th byte as this where "Application = MXF Generic Container Keys"
	// is set and so is the same for all GC keys and different in the majority of non-CG keys
	if( ( TheUL->GetValue()[10] == DegenerateGCLabel[10] ) && (memcmp(TheUL->GetValue(), DegenerateGCLabel, 12) == 0) )
	{
		ret.IsValid =			true;
		ret.Item =				(TheUL->GetValue())[12];
		ret.Count =				(TheUL->GetValue())[13];
		ret.ElementType = (TheUL->GetValue())[14];
		ret.Number =			(TheUL->GetValue())[15];
	}
	else
		ret.IsValid =			false;

	return ret;
}


//! Get the track number of this KLVObject (if it is a GC KLV)
/*! \return 0 if not a valid GC KLV
 */
Uint32 KLVObject::GetGCTrackNumber(void)
{
	//! Base of all standard GC keys
	/*! DRAGONS: version number is hard-coded as 1 */
	const Uint8 DegenerateGCLabel[12] = { 0x06, 0x0E, 0x2B, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01 };
	
	// Note that we first test the 11th byte as this where "Application = MXF Generic Container Keys"
	// is set and so is the same for all GC keys and different in the majority of non-CG keys
	if( ( TheUL->GetValue()[10] == DegenerateGCLabel[10] ) && (memcmp(TheUL->GetValue(), DegenerateGCLabel, 12) == 0) )
	{
		return (Uint32(TheUL->GetValue()[12]) << 24) || (Uint32(TheUL->GetValue()[13]) << 16) 
			|| (Uint32(TheUL->GetValue()[14]) << 8) || Uint32(TheUL->GetValue()[15]);
	}
	else
		return 0;
}


//! Read data from the KLVObject source into the DataChunk
/*! If Size is zero an attempt will be made to read all available data (which may be billions of bytes!)
 *  \note Any previously read data in the current DataChunk will be discarded before reading the new data
 *	\return Number of bytes read - zero if none could be read
 */
Length KLVObject::ReadData(Position Start /*=0*/, Length Size /*=0*/)
{
	// Delagate to ReadHandler if defined
	if(ReadHandler) return ReadHandler->ReadData(this, Start, Size);

	if(SourceOffset < 0)
	{
		error("Call to KLVObject::ReadData() with no read handler defined and DataBase undefined\n");
		return 0;
	}

	if(!SourceFile)
	{
		error("Call to KLVObject::ReadData() with no read handler defined and source file not set\n");
		return 0;
	}

	// Initially plan to read all the bytes available
	Length BytesToRead = ValueLength - Start;

	// Limit to specified size if > 0 and if < available
	if( Size && (Size < BytesToRead)) BytesToRead = Size;

	// Don't do anything if nothing to read
	if(BytesToRead <= 0) return 0;

	// Seek to the start of the requested data
	SourceFile->Seek(SourceOffset + Start);

	// Resize the chunk (discarding old data)
	Data.Size = 0;
	Data.Resize(Size);

	// Read into the buffer (only as big as the buffer is!)
	Length Bytes = (Length)SourceFile->Read(Data.Data, Data.Size);

	// Resize the buffer if something odd happened (such as an early end-of-file)
	if(Bytes != Size) Data.Resize(Bytes);

	return Bytes;
}



//! Write data from the current DataChunk to the source file
/*! \note The data in the chunk will be written to the specified position 
 *  <B>regardless of the position from where it was origanally read</b>
 */
Length KLVObject::WriteData(Position Start /*=0*/, Length Size /*=0*/)
{
//[Future?]	// Delagate to WriteHandler if defined
//[Future?]	if(WriteHandler) return WriteHandler->WriteData(this, Start, Size);

	if(SourceOffset < 0)
	{
		error("Call to KLVObject::WriteData() with DataBase undefined\n");
		return 0;
	}

	if(!SourceFile)
	{
		error("Call to KLVObject::WriteData() with source file not set\n");
		return 0;
	}

	// Initially plan to write all the bytes available
	Length BytesToWrite = Data.Size;

	// Limit to specified size if > 0 and if < available
	if( Size && (Size < BytesToWrite)) BytesToWrite = Size;

	// Don't do anything if nothing to write
	if(BytesToWrite <= 0) return 0;

	// Seek to the start of the requested data
	SourceFile->Seek(SourceOffset + Start);

	// Write from the buffer
	Length Bytes = (Length)SourceFile->Write(Data.Data, Size);

	return Bytes;
}

