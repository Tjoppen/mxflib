/*! \file	klvobject.cpp
 *	\brief	Implementation of classes that define basic KLV objects
 *
 *			Class KLVObject holds info about a KLV object
 *
 *	\version $Id: klvobject.cpp,v 1.1.2.6 2004/06/26 18:08:20 matt-beard Exp $
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
	KLSize = 0;
	DestKLSize = -1;
	SourceFile = NULL;
	SourceOffset = -1;
	DestFile = NULL;
	DestOffset = -1;
	ValueLength = 0;

	DataBase = 0;

//	ObjectName = "";
}


//! Get text that describes where this item came from
std::string KLVObject::GetSource(void) 
{ 
	if(SourceFile) return SourceFile->Name; else return "memory buffer"; 
}


//! Base verion: Read the key and length fot this KLVObject from the current source
/*! \return The number of bytes read (i.e. KLSize)
 *
 *  DRAGONS: This base function may be called from derived class objects to get base behaviour.
 *           It is therefore vital that the function does not call any "virtual" KLVObject
 *           functions, directly or indirectly.
 */
Int32 KLVObject::Base_ReadKL(void)
{
	if(!SourceFile)
	{
		error("KLVObject::Base_ReadKL() called with no SourceFile defined\n");
		return 0;
	}

	// Read the key
	SourceFile->Seek(SourceOffset);
	TheUL = SourceFile->ReadKey();

	// Abort now if now valid key
	if(!TheUL) return 0;

	// Read the length
	ValueLength = SourceFile->ReadBER();

	// Work out the size of the key and length
	KLSize = SourceFile->Tell() - SourceOffset;

	return KLSize;
}



//! Base verion: Read data from a specified position in the KLV value field into the DataChunk
/*! \param Offset Offset from the start of the KLV value from which to start reading
 *  \param Size Number of bytes to read, if <=0 all available bytes will be read (which could be billions!)
 *  \return The number of bytes read
 *
 *  DRAGONS: This base function may be called from derived class objects to get base behaviour.
 *           It is therefore vital that the function does not call any "virtual" KLVObject
 *           functions, directly or indirectly.
 */
Length KLVObject::Base_ReadDataFrom(Position Offset, Length Size /*=-1*/)
{
	// Delagate to ReadHandler if defined
	if(ReadHandler) return ReadHandler->ReadData(this, Offset, Size);

	if(SourceOffset < 0)
	{
		error("Call to KLVObject::Base_ReadDataFrom() with no read handler defined and DataBase undefined\n");
		return 0;
	}

	if(!SourceFile)
	{
		error("Call to KLVObject::Base_ReadDataFrom() with no read handler defined and source file not set\n");
		return 0;
	}

	// Initially plan to read all the bytes available
	Length BytesToRead = ValueLength - Offset;

	// Limit to specified size if > 0 and if < available
	if( Size && (Size < BytesToRead)) BytesToRead = Size;

	// Don't do anything if nothing to read
	if(BytesToRead <= 0) return 0;

	// Seek to the start of the requested data
	SourceFile->Seek(SourceOffset + KLSize + Offset);

	// Resize the chunk (discarding old data)
	Data.Size = 0;
	Data.Resize(BytesToRead);

	// Read into the buffer (only as big as the buffer is!)
	Length Bytes = (Length)SourceFile->Read(Data.Data, Data.Size);

	// Resize the buffer if something odd happened (such as an early end-of-file)
	if(Bytes != BytesToRead) Data.Resize(Bytes);

	return Bytes;
}


//! Base verion: Write the key and length of the current DataChunk to the destination file
/*! The key and length will be written to the source file as set by SetSource.
 *  If LenSize is zero the length will be formatted to match KLSize (if possible!)
 *
 *  DRAGONS: This base function may be called from derived class objects to get base behaviour.
 *           It is therefore vital that the function does not call any "virtual" KLVObject
 *           functions, directly or indirectly.
 */
Int32 KLVObject::Base_WriteKL(Int32 LenSize /*=0*/)
{
	if(!DestFile)
	{
		error("Call to KLVObject::Base_WriteKL() with destination file not set\n");
		return 0;
	}

	if(DestOffset < 0)
	{
		error("Call to KLVObject::Base_WriteKL() with destination file location undefined\n");
		return 0;
	}

	// Seek to the start of the KLV space
	DestFile->Seek(DestOffset);

	// Write the key
	Int32 Bytes = (Int32)DestFile->Write(TheUL->GetValue(), TheUL->Size());

	if(LenSize == 0) 
	{
		Bytes -= KLSize;		// Work out how many bytes we should use for the length
		if(Bytes > 0) LenSize = Bytes;
	}

	// Write the length
	DestFile->WriteBER(ValueLength, LenSize);

	// Work out the new KLSize
	DestKLSize = DestFile->Tell() - DestOffset;

	// Return the number of bytes we wrote
	return DestKLSize;
}


//! Base verion: Write data from a given buffer to a given location in the destination file
/*! \param Buffer Pointer to data to be written
 *  \param Offset The offset within the KLV value field of the first byte to write
 *  \param Size The number of bytes to write
 *  \return The number of bytes written
 *
 *  DRAGONS: This base function may be called from derived class objects to get base behaviour.
 *           It is therefore vital that the function does not call any "virtual" KLVObject
 *           functions, directly or indirectly.
 */
Length KLVObject::Base_WriteDataTo(Uint8 *Buffer, Position Offset, Length Size)
{
	// Don't write zero bytes
	if(Size == 0) return 0;

	if(!DestFile)
	{
		error("Call to KLVObject::Base_WriteDataTo() with destination file not set\n");
		return 0;
	}

	if(DestOffset < 0)
	{
		error("Call to KLVObject::Base_WriteDataTo() with destination file location undefined\n");
		return 0;
	}

	if(DestKLSize < 0)
	{
		error("Call to KLVObject::Base_WriteDataTo() before call to KLVObject::Base_WriteKL()\n");
		return 0;
	}

	// Seek to the start of the requested data
	DestFile->Seek(DestOffset + DestKLSize + Offset);

printf("@0x%06x:Base_WriteDataTo(Buffer, 0x%06x, 0x%06x)", (int)DestFile->Tell(), (int)Offset, (int)Size);
	// Write from the specified buffer
	return (Length)DestFile->Write(Buffer, Size);
printf("->0x%06x ", (int)DestFile->Tell());
}

