/*! \file	vbi.cpp
 *	\brief	Implementation of classes that handle Vertical Inerval Blanking data
 *
 *	\version $Id: vbi.cpp,v 1.4 2007/10/10 15:21:07 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2006, Matt Beard
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

// Add a line of data
void VBISource::AddLine(int LineNumber, VBIWrappingType Wrapping, VBISampleCoding Coding, UInt16 SampleCount, DataChunkPtr &LineData)
{
	Lines.insert(VBILineMap::value_type(LineNumber, new VBILine(LineNumber, Wrapping, Coding, SampleCount, LineData)));
}


//! Build the VBI data for this frame in SMPTE-436M format
DataChunkPtr VBISource::BuildChunk(void)
{
	/* First we handle the special case of no lines this frame (should be quite common) */
	if(Lines.empty())
	{
		// Simply return "Number of Lines = 0"
		DataChunkPtr Ret = new DataChunk(2);
		PutU16(0, Ret->Data);

		return Ret;
	}

	VBILineMap::iterator it = Lines.begin();

	// Guess the buffer size by assuming that all the lines are the same size, then add 2 bytes for the line count
	// DRAGONS: If thes line sizes do vary this is a bottle-neck
	// DRAGONS: We will use this as a remaining-bytes counter while writing the data
	size_t BufferSize = ((*it).second->GetFullDataSize() * Lines.size()) + 2;

	// Get a buffer of this size
	DataChunkPtr Ret = new DataChunk(BufferSize);

	// Index the start of the buffer
	UInt8 *pBuffer = Ret->Data;

	// Write in the number of lines
	PutU16(static_cast<UInt16>(Lines.size()), Ret->Data);
	pBuffer += 2;
	BufferSize -= 2;

	while(it != Lines.end())
	{
		// Get the number of bytes required to add this line to the buffer
		size_t RequiredBytes = (*it).second->GetFullDataSize();

		// If we don't have enough space we must increase the buffer size - can only happen if lines differ in size
		if(RequiredBytes > BufferSize)
		{
			// Work out how far through the buffer we currently are
			size_t CurrentPos = pBuffer - Ret->Data;

			// Make the buffer big enough for this line
			Ret->Resize(static_cast<UInt32>(CurrentPos + RequiredBytes));

			// Flag that we now have just enough bytes left
			BufferSize = RequiredBytes;

			// Set the buffer pointer in the (possibly re-allocated) buffer
			pBuffer = &Ret->Data[CurrentPos];
		}

		// Write the data into the buffer (formatted by the VBILine itself)
		(*it).second->WriteData(pBuffer);

		// Update the buffer pointer and bytes-remaining count
		pBuffer += RequiredBytes;
		BufferSize -= RequiredBytes;

		it++;
	}

	// Resize the buffer to the actual number of bytes that we wrote
	Ret->Resize(static_cast<UInt32>(pBuffer - Ret->Data));

	// Clear the list of pending lines
	Lines.clear();

	// Return the finished data
	return Ret;
}


//! Write the line of data into a buffer, including the line number, wrapping type, sample coding and sample count bytes
/*! \note It is the caller's responsibility to ensure that the buffer has enough space - the number of bytes written <b>will be</b> GetFullDataSize()
 */
void VBILine::WriteData(UInt8 *Buffer)
{
	// Write the line number (after removing any field indicator)
	PutU16(static_cast<UInt16>(LineNumber & 0x3fff), Buffer);

	// Add the wrapping type
	Buffer[2] = static_cast<UInt8>(WrappingType);

	// Add the sample coding
	Buffer[3] = static_cast<UInt8>(SampleCoding);

	// And the sample count
	PutU16(static_cast<UInt16>(SampleCount), &Buffer[4]);

	// Then copy in all the line data (assuming we have some) including the array header
	if(Data.Data)
	{
		PutU32(static_cast<UInt32>(Data.Size), &Buffer[6]);
		PutU32(1, &Buffer[10]);
		memcpy(&Buffer[14], Data.Data, Data.Size);
	}
	else
	{
		PutU32(0, &Buffer[6]);
		PutU32(1, &Buffer[10]);
	}
}


//! Get the size of the essence data in bytes
/*! \note There is intentionally no support for an "unknown" response */
size_t VBISource::GetEssenceDataSize(void)
{
	// If we don't yet have any data prepared, prepare some (even if this will be an "empty" chunk)
	if(BufferedData.empty())
	{
		if((!MasterSource) || (MasterSource->EndOfData())) return 0;

		BufferedData.push_back(BuildChunk());
	}

	// Return the size of the next available chunk
	return static_cast<size_t>(BufferedData.front()->Size);
}


//! Get the next "installment" of essence data
/*! This will attempt to return an entire wrapping unit (e.g. a full frame for frame-wrapping) but will return it in
 *  smaller chunks if this would break the MaxSize limit. If a Size is specified then the chunk returned will end at
 *  the first wrapping unit end encountered before Size. On no account will portions of two or more different wrapping
 *  units be returned together. The mechanism for selecting a type of wrapping (e.g. frame, line or clip) is not 
 *  (currently) part of the common EssenceSource interface.
 *  \return Pointer to a data chunk holding the next data or a NULL pointer when no more remains
 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
 *	\note If Size = 0 the object will decide the size of the chunk to return
 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
 */
DataChunkPtr VBISource::GetEssenceData(size_t Size /*=0*/, size_t MaxSize /*=0*/)
{
	// Once this read is done we will be in sync with the master stream position
	CurrentPosition = MasterSource->GetCurrentPosition();

	// If we don't yet have any data prepared, prepare some (even if this will be an "empty" chunk)
	if(BufferedData.empty())
	{
		if((!MasterSource) || (MasterSource->EndOfData())) return NULL;

		BufferedData.push_back(BuildChunk());
	}

	/* Handle the simple case first:
	 * - We are allowed to decide how much data to return (one frame)
	 * - We are not already part way through a buffer
	 * - We are permitted to return the whole buffer in one go
	 */
	if(/*(Size == 0) && */(BufferOffset == 0) && ((MaxSize == 0) || (BufferedData.front()->Size <= MaxSize)))
	{
		// We will return the head item
		DataChunkPtr Ret = BufferedData.front();

		// Remove this item from the list of buffers
		BufferedData.pop_front();

		// Return it
		return Ret;
	}

	// First see how many bytes remain
	size_t Bytes = BufferedData.front()->Size - BufferOffset;
	
	// If we can return all these now, do so
	if(Bytes <= MaxSize)
	{
		// Build a new buffer to hold the reduced data
		DataChunkPtr Ret = new DataChunk(Bytes);

		// Copy in the remaining bytes
		Ret->Set(static_cast<UInt32>(Bytes), BufferedData.front()->Data);

		// Remove this item from the list of buffers
		BufferedData.pop_front();

		// Clear the buffer offset as we will start at the beginning of the next chunk
		BufferOffset = 0;

		// Return the data
		return Ret;
	}

	// Build a new buffer to hold the reduced data
	DataChunkPtr Ret = new DataChunk(MaxSize);

	// Copy in as many bytes as permitted
	Ret->Set(static_cast<UInt32>(MaxSize), BufferedData.front()->Data);

	// Update the offset
	BufferOffset -= MaxSize;

	// Return the data
	return Ret;
}
