/*! \file	vbi.cpp
 *	\brief	Implementation of classes that handle Vertical Inerval Blanking data
 *
 *	\version $Id: vbi.cpp,v 1.5 2011/01/10 10:42:09 matt-beard Exp $
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

/*// Add a line of data
void ANCVBISource::AddLine(int LineNumber, ANCWrappingType Wrapping, ANCSampleCoding Coding, UInt16 SampleCount, DataChunkPtr &LineData)
{
	Lines.insert(ANCLineMap::value_type(LineNumber, new ANCLine(LineNumber, Wrapping, Coding, SampleCount, LineData, GetDID(), GetSDID())));
}*/

//! Get the offset to add to lines in field 2
int ANCVBISource::Field2Offset(void)
{
	if(F2Offset >= 0) return F2Offset;

	MDObjectPtr Descriptor = MasterSource->GetDescriptor();
	if(!Descriptor)
	{
		error("EssenceDescriptor not defined for master source of ANCVBISource before calling Field2Offset()\n");
		F2Offset = 0;
		return F2Offset;
	}

	// If this is a multpile descriptor, locate the video
	// DRAGONS: If we can't find a picture descriptor we will drop through with the MultipleDescriptor and give a "does not have a VideoLineMap" error
	if(Descriptor->IsA(MultipleDescriptor_UL))
	{
		MDObject::iterator it = Descriptor->begin();
		while(it != Descriptor->end())
		{
			if((*it).second->IsA(GenericPictureEssenceDescriptor_UL))
			{
				Descriptor = (*it).second;
				break;
			}
			it++;
		}
	}


	/* Check if this is interlaced essence */

	if(Descriptor->IsDValue(FrameLayout_UL))
	{
		warning("EssenceDescriptor for ANCVBISource does not have a valid FrameLayout\n");
		F2Offset = 0;
		return F2Offset;
	}

	if(Descriptor->GetInt(FrameLayout_UL) != 1)
	{
		F2Offset = 0;
		return F2Offset;
	}

	
	/* Calculate F1 to F2 distance from Video Line Map */

	MDObjectPtr VideoLineMap = Descriptor->Child(VideoLineMap_UL);
	if(!VideoLineMap)
	{
		error("EssenceDescriptor for ANCVBISource does not have a valid VideoLineMap\n");
		F2Offset = 0;
		return F2Offset;
	}

	MDObjectPtr F1Entry = VideoLineMap->Child(0);
	MDObjectPtr F2Entry = VideoLineMap->Child(1);
	if((!F1Entry) || (!F2Entry))
	{
		error("EssenceDescriptor for ANCVBISource does not have a valid VideoLineMap\n");
		F2Offset = 0;
		return F2Offset;
	}

	F2Offset = static_cast<int>(F2Entry->GetInt() - F1Entry->GetInt());
	return F2Offset;
}


//! Build the data for this frame in SMPTE-436M format
DataChunkPtr ANCVBISource::BuildChunk(void)
{
	/* Fill lines from line sources */

	ANCVBILineSourceList::iterator LS_it = Sources.begin();
	while(LS_it != Sources.end())
	{
		int LineNumber = (*LS_it)->GetLineNumber();
		if((*LS_it)->GetField() == 2) LineNumber += Field2Offset();

		Lines.insert(ANCLineMap::value_type(
				LineNumber, 
				new ANCLine(LineNumber, 
				(*LS_it)->GetWrappingType(), 
				(*LS_it)->GetSampleCoding(), 
				(*LS_it)->GetLineData(), 
				(*LS_it)->GetDID(), 
				(*LS_it)->GetSDID())));

		LS_it++;
	}


	/* Now build the chunk from line data */

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
	// DRAGONS: If the line sizes do vary this is a bottle-neck
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
void ANCVBILine::WriteData(UInt8 *Buffer)
{
	// Write the line number
	PutU16( LineNumber, Buffer);

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
size_t ANCVBISource::GetEssenceDataSize(void)
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
DataChunkPtr ANCVBISource::GetEssenceData(size_t Size /*=0*/, size_t MaxSize /*=0*/)
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


//! Determine if this sub-source can slave from a source with the given wrapping configuration, if so build the sub-config
/*! \return A smart pointer to the new WrappingConfig for this source as a sub-stream of the specified master, or NULL if not a valid combination
 */
EssenceParser::WrappingConfigPtr ANCVBISource::MakeWrappingConfig(WrappingConfigPtr MasterCfg)
{
	EssenceParser::WrappingConfigPtr SubCfg;

	/* First we validate our requirements */

	// Only valid for frame wrapping
	if(MasterCfg->WrapOpt->ThisWrapType != WrappingOption::Frame) return SubCfg;

	// Not valid if we have no line sources
	if(Sources.empty()) return SubCfg;

	/* Now check each line source is happy */
	// TODO: Can we build a sub-set version where we only include those line sources that are happy?

	std::string Description;
	ANCVBILineSourceList::iterator it = Sources.begin();
	while(it != Sources.end())
	{
		std::string ThisDesc = (*it)->ValidateConfig(MasterCfg);
		if(ThisDesc.empty()) return SubCfg;

		if(!Description.empty()) Description += ", plus ";
		Description += ThisDesc;

		it++;
	}

	/* Requested wrapping is valid, build the new config */

	SubCfg = new EssenceParser::WrappingConfig();

	SubCfg->Parser = MasterCfg->Parser;

	SubCfg->WrapOpt = new WrappingOption();
	SubCfg->WrapOpt->Handler = SubCfg->Parser;
	SubCfg->WrapOpt->Name = "";
	SubCfg->WrapOpt->Description = Description;
	SubCfg->WrapOpt->GCEssenceType = GetGCEssenceType();
	SubCfg->WrapOpt->GCElementType = GetGCElementType();
	SubCfg->WrapOpt->ThisWrapType = MasterCfg->WrapOpt->ThisWrapType;
	SubCfg->WrapOpt->CanSlave = false;
	SubCfg->WrapOpt->CanIndex = false;
	SubCfg->WrapOpt->CBRIndex = false;
	SubCfg->WrapOpt->BERSize = 4;
	SubCfg->WrapOpt->BytesPerEditUnit = 0;

	SubCfg->WrapOpt->WrappingUL = GetWrappingUL();

	SubCfg->EssenceDescriptor = new MDObject(ANCDataDescriptor_UL);
	MDObjectPtr SampleRate = SubCfg->EssenceDescriptor->AddChild(SampleRate_UL);
	SampleRate->SetInt("Numerator", MasterCfg->EditRate.Numerator);
	SampleRate->SetInt("Denominator", MasterCfg->EditRate.Denominator);
	SubCfg->EssenceDescriptor->SetValue(EssenceContainer_UL, DataChunk(16,SubCfg->WrapOpt->WrappingUL->GetValue()));

	SubCfg->Stream = 0;
	SubCfg->EditRate = MasterCfg->EditRate;
	SubCfg->StartTimecode = 0;

	return SubCfg;
}


//! Determine if this line-source is able to be used when slaved from a master with the given wrapping configuration
/*! \return Simple and short text description of the line being wrapped if OK (e.g. "Fixed AFD of 0x54") or empty string if not valid
 */
std::string SimpleAFDSource::ValidateConfig(WrappingConfigPtr MasterCfg)
{
	if(FieldNum == 1) return "Fixed F1 AFD of 0x" + Int64toHexString(CurrentAFD);
	return "Fixed F2 AFD of 0x" + Int64toHexString(CurrentAFD);
}


//! Convert a binary AFD value string, with optional 'w' suffix for 16:9 image, to an AFD value as per SMPTE 2016-1-2007
UInt8 SimpleAFDSource::TextToAFD(std::string Text)
{
	bool Wide = false;
	UInt8 Ret = 0;

	std::string::iterator it = Text.begin();
	while(it != Text.end())
	{
		if(*it == '1') Ret = (Ret << 1) | 1;
		else if(*it == '0') Ret = (Ret << 1);
		else if(tolower(*it) == 'w') Wide = true;
		it++;
	}

	// Format as per SMPTE 2016-1-2007, table 4
	return Wide ? (Ret << 3) | 4 : (Ret << 3);
}
