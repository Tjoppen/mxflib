/*! \file	klvobject.cpp
 *	\brief	Implementation of classes that define basic KLV objects
 *
 *			Class KLVObject holds info about a KLV object
 *
 *	\version $Id: klvobject.cpp,v 1.1.2.4 2004/05/28 14:45:21 matt-beard Exp $
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
	ValueLength = 0;

//	ObjectName = "";
}



//! Get text that describes where this item came from
std::string KLVObject::GetSource(void) 
{ 
	if(SourceFile) return SourceFile->Name; else return "memory buffer"; 
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
	SourceFile->Seek(SourceOffset + KLSize + Start);

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
Length KLVObject::WriteData(const Uint8 *Buffer, Position Start, Length Size)
{
//[Future?]	// Delagate to WriteHandler if defined
//[Future?]	if(WriteHandler) return WriteHandler->WriteData(this, Start, Size);

	// Don't write zero bytes
	if(Size == 0) return 0;

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

	// Seek to the start of the requested data
	SourceFile->Seek(SourceOffset + KLSize + Start);

	// Write from the specified buffer
	Length Bytes = (Length)SourceFile->Write(Buffer, Size);

	return Bytes;
}


//! Write the key and length of the current DataChunk to the source file
/*! The key and length will be written to the source file as set by SetSource.
 *  If LenSize is zero the length will be formatted to match KLSize (if possible!)
 *  \note KLSize will be updated as appropriate after the key and length are written
 */
Uint32 KLVObject::WriteKL(Uint32 LenSize /*=0*/)
{
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

	// Seek to the start of the KLV space
	SourceFile->Seek(SourceOffset);

	// Write the key
	int Bytes = (int)SourceFile->Write(TheUL->GetValue(), TheUL->Size());

	if(LenSize == 0) 
	{
		Bytes -= KLSize;		// Work out how many bytes we should use for the length
		if(Bytes > 0) LenSize = (Uint32)Bytes;
	}

	// Write the length
	SourceFile->WriteBER(ValueLength, LenSize);

	// Work out the new KLSize
	KLSize = SourceFile->Tell() - SourceOffset;

	// Return the number of bytes we wrote
	return KLSize;
}


