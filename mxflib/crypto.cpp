/*! \file	crypto.cpp
 *	\brief	Implementation of classes that hanldle basic encryption and decryption
 *
 *	\version $Id: crypto.cpp,v 1.1.2.6 2004/10/16 20:51:06 terabrit Exp $
 *
 */
/*
 *	Copyright (c) 2004, Matt Beard
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


//! Set a decryption Initialization Vector
/*! \return False if Initialization Vector is rejected
 */
bool KLVEObject::SetDecryptIV(Uint32 IVSize, const Uint8 *IV, bool Force /*=false*/)
{
	// Fail if we don't have a decryption wrapper
	if(!Decrypt) return false;

	return Decrypt->SetIV(IVSize, IV, Force);
}


//! Get the Initialization Vector that will be used for the next encryption
DataChunkPtr KLVEObject::GetEncryptIV(void)
{
	// Fail if we don't have an encryption wrapper
	if(!Encrypt) return NULL;

	// If we are waiting to set a new IV return that
	if(EncryptionIV) return EncryptionIV;

	// Otherwise ask the wrapper
	return Encrypt->GetIV();
}


//! Get the Initialization Vector that will be used for the next decryption
DataChunkPtr KLVEObject::GetDecryptIV(void)
{
	// Fail if we don't have a decryption wrapper
	if(!Decrypt) return NULL;

	return Decrypt->GetIV();
}



//! Construct a new KLVEObject
KLVEObject::KLVEObject(ULPtr ObjectUL)
	: KLVObject(ObjectUL)
{
	Init();
}


//! Construct a KLVEObject from a KLVObject which either contains plaintext or encrypted data
KLVEObject::KLVEObject(KLVObjectPtr &Object)
	: KLVObject(Object->TheUL)
{
	// Copy all properties from the original KLVObject
	// Note: TheUL is handled by KLVObject constructor call above

	Source = Object->Source;
	Dest = Object->Dest;

	ValueLength = Object->ValueLength;
	ReadHandler = Object->ReadHandler;

	// Initially ensume that the KLVObject contains plaintext
	// LoadData will change this later if we are wrong
	SourceKey = Object->GetUL();

	// Copy any data already loaded
	// DRAGONS: Is this a wise thing???
	Data.Set(Object->Data);

	// Initialize the KLVEObject specifics
	Init();
}


/*
//! Construct a KLVEObject linked to a KLVObject which either contains plaintext or encrypted data
KLVEObject::KLVEObject(KLVObject &Object)
	: KLVObject(Object.TheUL)
{
	// Copy all properties from the original KLVObject
	// Note: TheUL is handled by KLVObject constructor call above

	IsConstructed = Object.IsConstructed;
	SourceOffset = Object.SourceOffset;
	KLSize = Object.KLSize;
	SourceFile = Object.SourceFile;
	ValueLength = Object.ValueLength;
	ReadHandler = Object.ReadHandler;

	// Copy any data already loaded
	// DRAGONS: Is this a wise thing???
	Data.Set(Object.Data);

	// Initialize the KLVEObject specifics
	Init();
}
*/


//! Initialise the KLVEObject specifics after basic construction (and KLVObject::Init())
void KLVEObject::Init(void)
{
	DataLoaded = false;
	DataOffset = 0;
	PlaintextOffset = 0;
	SourceLengthFormat = 0;

	CurrentReadOffset = 0;
	CurrentWriteOffset = 0;

	PreDecrypted = 0;
	AwaitingEncryption = 0;
};



//** KLVObject interfaces **//

//! Get text that describes where this item came from
std::string KLVEObject::GetSource(void)
{
	return std::string("Encrypted KLV: ") + KLVObject::GetSource();
}

//! Get the size of the key and length (not of the value)
/*! \note For an KLVEObject this actually returns the sum of the size of all parts of the
 *        KLV other than the decrypted value - in other words total KLVE length - Source Length
 */
Int32 KLVEObject::GetKLSize(void) 
{ 
	if(!DataLoaded) if(!LoadData()) return 0;

	// Return the total overhead
	if(Source.KLSize >= 0) return Source.KLSize + DataOffset;
	else if(Dest.KLSize >= 0) return Dest.KLSize + DataOffset;
	else return -1;
}


//! Read the key and length for this KLVObject from the current source
/*! \return The number of bytes read (i.e. KLSize)
 */
Int32 KLVEObject::ReadKL(void)
{
	// Read the actual KL
	Base_ReadKL();

	// Force loading of AS-DCP header data if we are decrypting
	if(Decrypt)
	{
		DataLoaded = false;
		LoadData();
	}

	// Return the number of bytes that occur before the data (KL + AS-DCP-Header)
	return Source.KLSize;
}


//! Load the AS-DCP set data
/*! Sets DataLoaded on success
 * \return true if all loaded OK, false on error
 */
bool KLVEObject::LoadData(void)
{
	if(DataLoaded) return true;

	// Max length of AS-DCP header is:
	//   BER-Item-Length(9) + ContextID(16)
	//   BER-Item-Length(9) + PlaintextOffset(8)
	//   BER-Item-Length(9) + SourceULKey(16)
	//   BER-Item-Length(9) + SourceLength(9)
	//   BER-Item-Length(9) + IV(32) + Check(16)
	// So we load 116 bytes if we can

	Length Bytes = Base_ReadDataFrom(0, 116);

	// Min length of AS-DCP header is:
	//   BER-Item-Length(1) + ContextID(16)
	//   BER-Item-Length(1) + PlaintextOffset(8)
	//   BER-Item-Length(1) + SourceULKey(16)
	//   BER-Item-Length(1) + SourceLength(1)
	//   BER-Item-Length(1) + IV(16) + Check(16)
	// So we barf for less than 76 bytes

	if(Bytes < 76) return false;

	// Index the start of the data
	Uint8 *p = Data.Data;


	// ** Load the ContextID **

	// Read the BER length and move the pointer
	Uint8 *Prev_p = p;
	Length ItemLength = ReadBER(&p, (int)Bytes);
	Bytes -= (p - Prev_p);

	if((ItemLength != 16) || (Bytes < 16))
	{
		error("Invalid AS-DCP data (%s) in %s\n", "ContextID not 16 bytes", GetSourceLocation().c_str());
		return false;
	}

	// Build the ContextID
	ContextID = new UL(p);

	// Update pointer and count
	p += 16;
	Bytes -= 16;


	// ** Load the PlaintextOffset **

	// Read the BER length and move the pointer
	Prev_p = p;
	ItemLength = ReadBER(&p, (int)Bytes);
	Bytes -= (p - Prev_p);

	if((ItemLength != 8) || (Bytes < 8))
	{
		error("Invalid AS-DCP data (%s) in %s\n", "PlaintextOffset not 8 bytes", GetSourceLocation().c_str());
		return false;
	}

	// Read the PlaintextOffset
	PlaintextOffset = GetU64(p);

	// Update pointer and count
	p += 8;
	Bytes -= 8;


	// ** Load the SourceKey **

	// Read the BER length and move the pointer
	Prev_p = p;
	ItemLength = ReadBER(&p, (int)Bytes);
	Bytes -= (p - Prev_p);

	if((ItemLength != 16) || (Bytes < 16))
	{
		error("Invalid AS-DCP data (%s) in %s\n", "SourceKey not 16 bytes", GetSourceLocation().c_str());
		return false;
	}

	// Build the SourceKey
	SourceKey = new UL(p);

	// Set our published UL to be this key
	SetUL(SourceKey);

	// Update pointer and count
	p += 16;
	Bytes -= 16;


	// ** Load the SourceLength **
	//
	// Note: The source length in the original specification was BER coded
	//       A later version of the specification uses Uint64
	//       This code attempts to determine which is used

	// Read the BER length and move the pointer
	Prev_p = p;
	ItemLength = ReadBER(&p,(int) Bytes);
	Bytes -= (p - Prev_p);

	if((ItemLength <= 0) || (ItemLength >= 9) || (Bytes < ItemLength))
	{
		error("Invalid AS-DCP data (%s) in %s\n", "Invalid SourceLength size", GetSourceLocation().c_str());
		return false;
	}

	// If not 8-bytes it can't be Uint64 - otherwise it may be BER or Uint64 so see if it is valid BER
	// This will fail for unimaginably large Uint64 coded SourceLengths
	if((ItemLength != 8) || (*p == 0x87))
	{
		// Read the BER SourceLength
		Prev_p = p;
		ValueLength = ReadBER(&p, (int)Bytes);
		SourceLengthFormat = (p - Prev_p);
		Bytes -= SourceLengthFormat;
	}
	else
	{
		// Read the Uint64 SourceLength
		ValueLength = GetU64(p);
		SourceLengthFormat = 8;
		p += 8;
		Bytes -= 8;
	}

	if((Int64)PlaintextOffset > ValueLength)
	{
		warning("Invalid AS-DCP data: PlaintextOffset(%s) > SourceLength(%s) in %s\n", Int64toString(PlaintextOffset).c_str(), Int64toString(ValueLength).c_str(), GetSourceLocation().c_str());
		PlaintextOffset = ValueLength;
	}

	// Read and the BER length of the Encrypted Source Value
	Prev_p = p;
	Length ESVLength = ReadBER(&p, (int)Bytes);
	Bytes -= (p - Prev_p);

	// Check that we have enough left for the IV and Check Value
	if(Bytes < EncryptionOverhead)
	{
		error("Invalid AS-DCP data (%s) in %s\n", "Not enough bytes for IV and Check", GetSourceLocation().c_str());
		return false;
	}


	// DRAGONS: Do we need to load these two items?

	// Store the initialization value
	memcpy(IV, p, 16);

	// Store the check value
	memcpy(Check, &p[16], 16);

	// Record offset to the encrypted data
	DataOffset = &p[EncryptionOverhead] - Data.Data;

	// Check there is enough left for the specified data
	// There is padding from 1 to 16 bytes at the end of the data

	EncryptedLength = ((ValueLength-PlaintextOffset) + EncryptionGranularity) / EncryptionGranularity;
	EncryptedLength *= EncryptionGranularity;
	EncryptedLength += PlaintextOffset;

//@'@printf("}} ValueLength %d\n}} PlaintextOffset %d\n", (int)ValueLength, (int)PlaintextOffset);
//@'@printf("@@ Size of Encrypted Source Value is %s - should be %s\n", Int64toString(ESVLength).c_str(), Int64toString((EncryptedLength + EncryptionOverhead)).c_str());
	if(ESVLength != (EncryptedLength + EncryptionOverhead))
	{
		error("Size of Encrypted Source Value is %s - should be %s in %s\n", Int64toString(ESVLength).c_str(), Int64toString((EncryptedLength + EncryptionOverhead)).c_str(), GetSourceLocation().c_str());
		return false;
	}

	Length BytesLeft = Source.OuterLength - DataOffset;
	if(BytesLeft < EncryptedLength)
	{
		error("Invalid AS-DCP data (%s) in %s\n", "Not enough bytes for encrypted value", GetSourceLocation().c_str());
		return false;
	}

	// All seems OK
	DataLoaded = true;
	return true;
}


//! Get a GCElementKind structure
GCElementKind KLVEObject::GetGCElementKind(void)
{
	if(!DataLoaded) 
		if(!LoadData())
		{
			// Failed to load the KLVE header
			GCElementKind GC;
			GC.IsValid = false;
			return GC;
		}


	// Return the GCElementKind of the plaintext KLV
	return mxflib::GetGCElementKind(SourceKey);
}


//! Get the track number of this KLVObject (if it is a GC KLV, else 0)
Uint32 KLVEObject::GetGCTrackNumber(void)
{
	if(!DataLoaded) if(!LoadData()) return 0;

	// Return the TrackNumber of the plaintext KLV
	return mxflib::GetGCTrackNumber(SourceKey);
}


//! Read data from a specified position in the KLV value field into the DataChunk
/*! \param Offset Offset from the start of the KLV value from which to start reading
 *  \param Size Number of bytes to read, if <=0 all available bytes will be read (which could be billions!)
 *  \return The number of bytes read
 */
Length KLVEObject::ReadDataFrom(Position Offset, Length Size /*=-1*/)
{
//@'@printf("KLVEObject::ReadDataFrom(0x%08x, %d)\n", (int)Offset, (int)Size);

	// Don't decrypt if we have no decryption wrapper
	if(!Decrypt) return Base_ReadDataFrom(Offset, Size);

	// Load the header if required (and if we can!)
	if(!DataLoaded) if(!LoadData()) return 0;

	// Don't bother reading zero bytes or of the end of the value
	if((Size == 0) || (Offset >= ValueLength))
	{
		Data.Resize(0);
		return 0;
	}

	// Load the IV and check the Check value if this is the first read
	if(	CurrentReadOffset == 0)
	{
//@'@printf("Loading IV:");
		if( Base_ReadDataFrom(DataOffset - EncryptionOverhead, EncryptionOverhead) < EncryptionOverhead)
		{
			error("Unable to read Initialization Vector and Check Value in KLVEObject::ReadDataFrom()\n");
			return 0;
		}

//@'@{
//@'@	int i;
//@'@	for(i=0; i<16; i++) printf(" %02x", Data.Data[i]);
//@'@	printf("\n");
//@'@}
		// Initialize the decryption engine with the specified Initialization Vector
		Decrypt->SetIV(16, Data.Data, true);

		// Decrypt the check value...
		DataChunkPtr PlainCheck = Decrypt->Decrypt(16, &Data.Data[16]);

		// Encrypt the check value... (Which is "CHUKCHUKCHUKCHUK" who ever said Chuck Harrison has no ego?)
		const Uint8 DefinitivePlainCheck[16] = { 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B };
		if((!PlainCheck) || (memcmp(PlainCheck->Data, DefinitivePlainCheck, PlainCheck->Size) != 0))
		{
			error("Check value did not correctly decrypt in KLVEObject::ReadDataFrom() - is the encryption key correct?\n");
			return 0;
		}
	}

	// If all the requested bytes are encrypted read-and-decrypt
	if(Offset >= (Int64)PlaintextOffset) 
	{
//@'@printf("Data starts in encrypted area\n");
		// Check if an attempt is being made to random access the encrypted data - and barf if this is so
		if(Offset != CurrentReadOffset)
		{
			error("Attempt to perform random-access reading of an encrypted KLV value field\n");
			return 0;
		}

		return ReadCryptoDataFrom(Offset, Size);
	}

	// If all the bytes requested are plaintext use the base read
	if( (Size > 0) && ((Offset + Size) < (Int64)PlaintextOffset) )
	{
//@'@printf("Data is all plaintext\n");
		Length Ret = Base_ReadDataFrom(DataOffset + Offset, Size);

		// Update the read pointer (it is possible to random access within the plaintext area)
		CurrentReadOffset = Offset + Ret;

		return Ret;
	}

	/* We will be mixing plaintext and encrypted */

//@'@printf("Data starts in plaintext area\n");
	// Check if an attempt is being made to random access the encrypted data - and barf if this is so
	// DRAGONS: It is possible to re-load the initial IV to allow a "rewind" but this is not implemented
	if(CurrentReadOffset > (Int64)PlaintextOffset)
	{
		error("Attempt to perform random-access reading of an encrypted KLV value field\n");
		return 0;
	}

	// Determine how many plaintext bytes could be available (maximum)
	Length PlainSize = PlaintextOffset - Offset;

	// Try and read them all
	Length PlainBytes = Base_ReadDataFrom(DataOffset + Offset, PlainSize);

	// If we couldn't read them all then the data ends before any encrypted bytes so we can exit now
	if(PlainBytes < PlainSize)
	{
		return PlainBytes;
	}

	/* We have all the plaintext bytes from Offset forwards, now we read all encrypted bytes too */

	// Take the buffer from the current DataChunk to preserve it
	DataChunkPtr PlainData = new DataChunk;
	PlainData->TakeBuffer(Data, true);

	// Work out how many encrypted bytes to read
	Length EncSize;
	if(Size == -1) EncSize = -1; else EncSize = Size - PlainSize;

	// Read the encrypted bytes
	Length EncBytes = ReadCryptoDataFrom(PlaintextOffset, EncSize);

	// Append the decrypted data to the plaintext data
	PlainData->Append(Data);

	// Transfer this data to the "current" DataChunk
	Data.TakeBuffer(PlainData);

	// Set the "next" position to just after the end of what we read
	CurrentReadOffset = Offset + Data.Size;

	// Return the total number of bytes
	return Data.Size;
}


//! Read data from a specified position in the encrypted portion of the KLV value field into the DataChunk
/*! \param Offset Offset from the start of the KLV value from which to start reading
 *  \param Size Number of bytes to read, if <=0 all available bytes will be read (which could be billions!)
 *  \return The number of bytes read
 *	The IV must have already been set.
 *  Only encrypted parts of the value may be read using this function (i.e. Offset >= PlaintextOffset)
 */
Length KLVEObject::ReadCryptoDataFrom(Position Offset, Length Size /*=-1*/)
{
//@'@printf("KLVEObject::ReadCryptoDataFrom(0x%08x, %d)\n", (int)Offset, (int)Size);

	// Initially plan to read all the bytes available
	Length BytesToRead = EncryptedLength - Offset;

	// Limit to specified size if >= 0 and if < available
	if( (Size >= 0) && (Size < BytesToRead)) BytesToRead = Size;

	// Assume that the read will succeed and move the "next" pointer accordingly
	CurrentReadOffset += BytesToRead;

	// Work out the offset into the encrypted portion of the value
	Position EncOffset = Offset - PlaintextOffset;

//@'@printf(">> Requested bytes = %d\n", (int)Size);
//@'@printf(">> Corrected bytes = %d\n", (int)BytesToRead);
//@'@printf(">> EncOffset = %d\n", (int)EncOffset);
//@'@printf(">> PreDecrypted = %d:", (int)PreDecrypted);
//@'@{
//@'@	int i;
//@'@	for(i=0; i<PreDecrypted; i++) printf(" %02x", PreDecryptBuffer[i]);
//@'@	printf("\n");
//@'@}

	// Check if all the requested bytes have already been decrypted
	if(BytesToRead <= PreDecrypted)
	{
		// Set the data	into the DataChunk
		Data.Set(BytesToRead, (Uint32)PreDecryptBuffer);

		// Remove any padding if required
		if(Offset + Data.Size > ValueLength)
		{
			BytesToRead = ValueLength - Offset;
			Data.Resize((Uint32)BytesToRead);
		}

		// Shuffle any remaining bytes
		memmove(PreDecryptBuffer, &PreDecryptBuffer[BytesToRead], (size_t)(EncryptionGranularity - BytesToRead));

//@'@printf(">>>> All %d bytes read\n", BytesToRead);

		// All done
		return BytesToRead;
	}

	// Work out how many bytes have to be decrypted this time
	// First we work out the number of EncryptionGranularity sized chunks to decrypt...
	Length BytesToDecrypt = ((BytesToRead - PreDecrypted) + (EncryptionGranularity - 1)) / EncryptionGranularity;

	// ... then scale this back into bytes
	BytesToDecrypt *= EncryptionGranularity;

//@'@printf(">> BytesToDecrypt = %d\n", (int)BytesToDecrypt);

	// Read the encrypted data
	if(!ReadChunkedCryptoDataFrom(PreDecrypted + Offset, BytesToDecrypt))
	{
		// Abort if the decrypt failed
		return 0;
	}

	// Add back in any pre-decrypted data
	if(PreDecrypted)
	{
		// Steal the newly decrypted bytes
		Uint8 *Buffer = Data.StealBuffer(true);

		// Make a buffer large enough for the complete data
		Data.ResizeBuffer((Uint32)(PreDecrypted + BytesToDecrypt));

		// Put the pre-decrypted bytes at the start
		Data.Set(PreDecrypted, PreDecryptBuffer);

		// Follow it with the newly decrypted data
		Data.Set((Uint32)BytesToDecrypt, Buffer, PreDecrypted);
	}

	// If we have decrypted more than requested store them as pre-decrypted for next time
	if(Data.Size > Size)
	{
		PreDecrypted = (int)(Data.Size - Size);
//@'@printf(">> Storing %d bytes for next time", (int)PreDecrypted);
//@'@{
//@'@	int i;
//@'@	for(i=0; i<PreDecrypted; i++) printf(" %02x", Data.Data[Size + i]);
//@'@	printf("\n");
//@'@}
		memcpy(PreDecryptBuffer, &Data.Data[Size], PreDecrypted);
		Data.Resize((Uint32)Size);
	}
	else 
		PreDecrypted = 0;

	// Remove any padding if required
	if(Offset + Data.Size > ValueLength)
	{
		Size = ValueLength - Offset;
		Data.Resize((Uint32)Size);
	}

	return Size;
}


//! Read an integer set of chunks from a specified position in the encrypted portion of the KLV value field into the DataChunk
/*! \param Offset Offset from the start of the KLV value from which to start reading
 *  \param Size Number of bytes to read, if <=0 all available bytes will be read (which could be billions!)
 *  \return The number of bytes read
 *	The IV must have already been set, Size must be a multiple of 16 as must (Offset - PlaintextOffset). 
 *  Only encrypted parts of the value may be read using this function (i.e. Offset >= PlaintextOffset)
 */
Length KLVEObject::ReadChunkedCryptoDataFrom(Position Offset, Length Size)
{
//@'@printf("KLVEObject::ReadChunkedCryptoDataFrom(0x%08x, %d)\n", (int)Offset, (int)Size);

	// Read the encrypted data
	Length NewSize = Base_ReadDataFrom(DataOffset + Offset, Size);

	// Resize if less bytes than requested were actualy read
	if(NewSize != Size)
	{
//@'@printf("Requested %d bytes, got %d\n", (int)Size, (int)NewSize);
		Size = NewSize;
		Data.Resize((Uint32)Size);
		if(Size == 0) return 0;

		// We can cope with less bytes, as long as it is an integer number of blocks
		if((Size % EncryptionGranularity) != 0)
		{
			error("Base_ReadDataFrom() failed to read an integer number of EncryptionGranularity bytes when requested\n");
			
			// Invalidate the "next" position to prevent further read attempts
			CurrentReadOffset = Source.OuterLength;
			return 0;
		}
	}

	// See if we can decrypt this in place...
	if(Decrypt->CanDecryptInPlace((Uint32)Size))
	{
		if(!Decrypt->DecryptInPlace(Data))
		{
			// Invalidate the "next" position to prevent further read attempts
			CurrentReadOffset = Source.OuterLength;
			Data.Resize(0);
			return 0;
		}

		return Size;
	}

//@'@printf("[] Need to decrypt %d bytes\n", (int)Data.Size);
	// Decrypt by making a copy
	DataChunkPtr NewData = Decrypt->Decrypt(Data);
	if(!NewData)
	{
		// Invalidate the "next" position to prevent further read attempts
		CurrentReadOffset = Source.OuterLength;
		Data.Resize(0);
		return 0;
	}

	// Take over the buffer from the decrypted data
//@'@printf("[] Take buffer size = 0x%08x or 0x%08x\n", (int)Data.Size, *(int*)(Data.Data - 16));
	Data.TakeBuffer(NewData);

	return Size;
}


//! Write data from a given buffer to a given location in the destination file
/*! \param Buffer Pointer to data to be written
 *  \param Offset The offset within the KLV value field of the first byte to write
 *  \param Size The number of bytes to write
 *  \return The number of bytes written
 *  \note As there may be a need for the implementation to know where within the value field
 *        this data lives, there is no WriteData(Buffer, Size) function.
 */
Length KLVEObject::WriteDataTo(const Uint8 *Buffer, Position Offset, Length Size)
{
//@'@printf("WriteDataTo(Buffer, 0x%x, 0x%x) - ", (int)Offset, (int)Size);
//@'@printf("OuterLength=0x%x, EncryptedLength=0x%x, DataOffset=0x%x, CurrentWriteOffset=0x%x\n", (int)Dest.OuterLength, (int)EncryptedLength, (int)DataOffset, (int)CurrentWriteOffset);

	// Don't encrypt if we have no encryption wrapper
	if(!Encrypt) return Base_WriteDataTo(Buffer, Offset, Size);

	// Don't write too many bytes
	if(Size > (EncryptedLength - Offset)) Size = (EncryptedLength - Offset);

	// Don't write zero bytes
	if(Size == 0) return 0;

	// Validate the output file
	if(!Dest.File)
	{
		error("Call to KLVObject::Base_WriteDataTo() with destination file not set\n");
		return 0;
	}

	if(Dest.Offset < 0)
	{
		error("Call to KLVObject::Base_WriteDataTo() with destination file location undefined\n");
		return 0;
	}

	if(DataOffset == 0) 
	{
		error("KLVEObject::WriteData called before KLVEObject::WriteKL()\n");
		return 0;
	}


	// Write IV and check value if first data to be written
	if(CurrentWriteOffset == 0)
	{
		// ** Write the IV

		// Initialize the encryption engine with a new Initialization Vector
		// If one has been given to use we use that - otherwise we create a new one
		
		// First validate any given IV and reject bad ones
		if(EncryptionIV && (EncryptionIV->Size != 16)) EncryptionIV = NULL;

		if(EncryptionIV)
		{
			memcpy(IV, EncryptionIV->Data, 16);
			Encrypt->SetIV(EncryptionIV, true);
			EncryptionIV = NULL;						// Only use a give IV once
		}
		else
		{
			// TODO: Add decent random number generator here... this one is from the system.h UUID gen
			srand((time(NULL)) ^ ((int)(Data.Data)) ^ (clock() << 2) ^ rand());

			int i;
			for(i=0; i< 16; i++) { IV[i] = (Uint8)rand(); };

			Encrypt->SetIV(16, IV, true);
		}

//@'@printf("Writing initial data -> IV =");
//@'@{
//@'@	int i;
//@'@	for(i=0; i<16; i++) printf(" %02x", IV[i]);
//@'@	printf(",");
//@'@}

		Base_WriteDataTo(IV, DataOffset - EncryptionOverhead, 16);


		// ** Write the check value

		// Encrypt the check value... (Which is "CHUKCHUKCHUKCHUK" who ever said Chuck Harrison has no ego?)
		const Uint8 PlainCheck[16] = { 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B };
		DataChunkPtr CheckData = Encrypt->Encrypt(16, PlainCheck);

//@'@printf("Check =");
//@'@{
//@'@	int i;
//@'@	for(i=0; i<16; i++) printf(" %02x", CheckData->Data[i]);
//@'@	printf("\n");
//@'@}
		// Write the encrypted check value
		if(CheckData && (CheckData->Size == 16))
		{
			Base_WriteDataTo(CheckData->Data, DataOffset - EncryptionOverhead + 16, 16);
		}
		else
		{
			error("Could not encrypt check value - encryption system is not working correctly\n");
		}
	}

	// If all the requested bytes are to be encrypted encrypt-and-write
	if(Offset >= (Int64)PlaintextOffset) 
	{
//@'@printf("All data is to be encrypted\n");
		// Check if an attempt is being made to random access the encrypted data - and barf if this is so
		if(Offset != CurrentWriteOffset)
		{
			error("Attempt to perform random-access writing of an encrypted KLV value field\n");
			return 0;
		}

		return WriteCryptoDataTo(Buffer, Offset, Size);
	}


	// If all the bytes requested are plaintext use the base write
	if( (Size > 0) && ((Offset + Size) < (Int64)PlaintextOffset) )
	{
//@'@printf("All data is plaintext\n");
		Length Ret = Base_WriteDataTo(Buffer, DataOffset + Offset, Size);
		
		// Update the write pointer (note that random access is possible within the plaintext area)
		CurrentWriteOffset = Offset + Ret;

		// Check if we need to tag some "dummy padding" onto a plaintext-only KLVE
		// This is because there must be 1-16 padding bytes whatever the number of encrypted
		// bytes in the object. If the object is wholly plaintext it gets 16 bytes of padding!
		if(PlaintextOffset == ValueLength)
		{
			if(CurrentWriteOffset == ValueLength)
			{
				// Write zero bytes (will write only padding)
				WriteCryptoDataTo(Buffer, CurrentWriteOffset, 0);
			}
		}

		return Ret;
	}

//@'@printf("Data is mixed plaintext and encrypted\n");
	/* We will be mixing plaintext and encrypted */

	// Check if an attempt is being made to random access the encrypted data - and barf if this is so
	// DRAGONS: It is possible to re-set the initial IV to allow a "rewind" but this is not implemented
	if(CurrentWriteOffset >(Int64) PlaintextOffset)
	{
		error("Attempt to perform random-access writing of an encrypted KLV value field\n");
		return 0;
	}

	// Determine how many plaintext bytes will be written
	Length PlainSize = PlaintextOffset - Offset;

	// Write the plaintext bytes
	Length PlainBytes = Base_WriteDataTo(Buffer, DataOffset + Offset, PlainSize);

	if(PlainBytes != PlainSize)
	{
		error("Writing of Plaintext bytes failed\n");
		return PlainBytes;
	}

	// Update the write pointer after the plaintext write
	CurrentWriteOffset = Offset + PlainBytes;


	/* We have written all the plaintext bytes from Offset forwards, now we write to encrypted bytes too */

	// Write the encrypted bytes
	Length EncBytes = WriteCryptoDataTo(&Buffer[PlainBytes], PlaintextOffset, Size - PlainSize);

	// Return the number of bytes written
	return PlainBytes + EncBytes;
}


//! Write encrypted data from a given buffer to a given location in the destination file
/*! \param Buffer Pointer to data to be written
 *  \param Offset The offset within the KLV value field of the first byte to write
 *  \param Size The number of bytes to write
 *  \return Size if all OK, else != Size.  This may not equal the actual number of bytes written.
 *	The IV must have already been set.
 *  Only encrypted parts of the value may be written using this function (i.e. Offset >= PlaintextOffset)
 */
Length KLVEObject::WriteCryptoDataTo(const Uint8 *Buffer, Position Offset, Length Size)
{
//@'@printf("WriteCryptoDataTo(Buffer, 0x%x, 0x%x)\n", (int)Offset, (int)Size);

	// Self-deleting store to allow us to extend working buffer if required
	DataChunk TempData;

	// Are we going to need to write padding bytes?
	bool AddPadding;
	if(Offset + Size >= ValueLength) AddPadding = true; else AddPadding = false;

	// Check if this is a "pointless" zero-byte write (rather than a request to write padding only)
	if((!AddPadding) && (Size == 0)) return 0;

	// Assume that the write will succeed and move the "next" pointer accordingly
	CurrentWriteOffset += Size;

	// Work out the offset into the encrypted portion of the value
	Position EncOffset = Offset - PlaintextOffset;

//@'@printf(">> EncOffset = %d, AwaitingEncryption = %d\n", (int)EncOffset, (int)AwaitingEncryption);

	// Check if all the bytes will fit in the AwaitingEncryptionBuffer
	if((!AddPadding) && (Size < (EncryptionGranularity - AwaitingEncryption)))
	{
		// Add to the end of the waiting buffer
		memcpy(&AwaitingEncryptionBuffer[AwaitingEncryption], Data.Data, (size_t)Size);

		// All done
		return Size;
	}

	// Work out how many bytes we need to encrypt (estimate one - as many as offered)
	Length BytesToEncrypt = Size;

//@'@printf(">> BytesToEncrypt = %d\n", (int)BytesToEncrypt);

	// If there are any bytes waiting they need to be added to this write
	if(AwaitingEncryption)
	{
		// Build the full data in the temporary buffer
		TempData.ResizeBuffer((Uint32)(AwaitingEncryption + Size));

		// Start with "waiting" data
		TempData.Set(AwaitingEncryption, AwaitingEncryptionBuffer);

		// Copy in the new data
		TempData.Append((Uint32)Size, Buffer);

		// Replace the working buffer pointer with a pointer to the temporary buffer
		Buffer = TempData.Data;

		// Update the offset (move it back to the first waiting byte)
		Offset -= AwaitingEncryption;

		// Record the revised size
		BytesToEncrypt = AwaitingEncryption + Size;

//@'@printf(">> BytesToEncrypt updated to = %d\n", (int)BytesToEncrypt);
//@'@printf(">> Offset updated to = %d\n", (int)Offset);
	}

	// Pad the data if required (i.e. if this is the last chunk of data)
	if(AddPadding)
	{
		if(Offset + BytesToEncrypt > ValueLength)
		{
			error("Attempted to write beyond the end of the encrypted value in KLVEObject::WriteCryptoDataTo()\n");
			return 0;
		}

//@'@printf(">> Data requires padding\n");
		// Start by encrypting all but the last 16 bytes (including padding)
		Length StartSize = EncryptedLength - (EncryptionGranularity + Offset);

		// Don't write zero bytes
		if(StartSize)
		{
			// Encrypt by making a copy
			DataChunkPtr NewData = Encrypt->Encrypt((Uint32)StartSize, Buffer);
			if(!NewData) return 0;

//@'@printf(">> Writing first encrypted chunk (%d bytes)\n", (int)StartSize);
			// Write the encrypted data
			Base_WriteDataTo(NewData->Data, DataOffset + Offset, StartSize);
		}

		// Buffer for last data to be encrypted
		Uint8 TempBuffer[EncryptionGranularity];

		// Copy in the remaining bytes from the end of the given buffer
		// Add padding bytes in a 16-byte version of the scheme defined in RFC 2898
		const Uint8 *pSrc = &Buffer[StartSize];
		Uint8 *pDst = TempBuffer;
		int i;
		int EncData = (int)(BytesToEncrypt - StartSize);
		int Pad = EncryptionGranularity - EncData;
		for(i=0; i<16; i++)
		{
			if(i < EncData) *(pDst++) = *(pSrc++);
			else *(pDst++) = (Uint8)Pad;
		}

		// Encrypt by making a copy
		DataChunkPtr NewData = Encrypt->Encrypt(EncryptionGranularity, TempBuffer);
		if(!NewData) return 0;

//@'@printf(">> Writing remaining padded encrypted data (%d bytes)\n", (int)EncryptionGranularity);
		// Write the encrypted data
		Base_WriteDataTo(NewData->Data, DataOffset + Offset + StartSize, EncryptionGranularity);

		// There are no more bytes to encrypt
		AwaitingEncryption = 0;

		// FIXME: - we need some way to add end and an AS-DCP footer if required

		// Lie and say we wrote the requested number of bytes (tells the caller all was fine)
		return Size;
	}

	// Work out how many bytes have to be encrypted this time (an integer number of chunks)
	Length BytesRequiringEncryption = BytesToEncrypt;
	BytesToEncrypt = BytesToEncrypt / EncryptionGranularity;
	BytesToEncrypt *= EncryptionGranularity;

	// Any differnece will be "left-over"
	AwaitingEncryption = (Uint32)(BytesRequiringEncryption - BytesToEncrypt);

	// Encrypt by making a copy
	DataChunkPtr NewData = Encrypt->Encrypt((Uint32)BytesToEncrypt, Buffer);
	if(!NewData) return 0;

	// If there will be any "left-over" bytes they will be awaiting next time
	if(AwaitingEncryption)
	{
//@'@printf(">> Storing %d bytes for next time\n", (int)AwaitingEncryption);
//@'@{
//@'@	int i;
//@'@	for(i=0; i<AwaitingEncryption; i++) printf(" %02x", Buffer[BytesToEncrypt + i]);
//@'@	printf("\n");
//@'@}
		memcpy(AwaitingEncryptionBuffer, &Buffer[BytesToEncrypt], AwaitingEncryption);
	}

//@'@printf(">> Writing the encrypted data\n");
	// Write the encrypted data
	Size = Base_WriteDataTo(NewData->Data, DataOffset + Offset, BytesToEncrypt);

	// Chain the IV for next time...
	EncryptionIV = Encrypt->GetIV();

	// Lie and say we wrote the requested number of bytes (tells the caller all was fine)
	return Size;
}



//! Write the key and length of the current DataChunk to the destination file
/*! The key and length will be written to the source file as set by SetSource.
 *  If LenSize is zero the length will be formatted to match KLSize (if possible!)
 */
Int32 KLVEObject::WriteKL(Int32 LenSize /*=0*/)
{
	// If we don't have an encryption wrapper we are not writing encrypted data, so just write the KL
	if(!Encrypt) return Base_WriteKL(LenSize, ValueLength);

	if(!Dest.File)
	{
		error("Call to KLVEObject::WriteKL() with destination file not set\n");
		return 0;
	}

	if(Dest.Offset < 0)
	{
		error("Call to KLVEObject::WriteKL() with destination file location undefined\n");
		return 0;
	}

	// Seek to the start of the KLV space
	Dest.File->Seek(Dest.Offset);

	// As we are writing an encrypted KLV we know that the key should be the KLVE key
	static bool InitedKey = false;
	static ULPtr TripletKey;

	if(InitedKey)
	{
		TheUL = TripletKey;
	}
	else
	{
		MDOTypePtr TripletType = MDOType::Find("EncryptedTriplet");
		if(!TripletType)
		{
			warning("EncryptedTriplet type not known\n");
			const Uint8 TripletData[] =  { 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x04, 0x01, 0x07, 0x0f, 0x01, 0x03, 0x7f, 0x01, 0x00, 0x00, 0x00 };
			TripletKey = new UL(TripletData);
			TheUL = TripletKey;
			InitedKey = true;
		}
		else
		{
			TripletKey = TripletType->GetUL();
			TheUL = TripletKey;
			InitedKey = true;
		}
	}
	
	// Small buffer for header (note: max valid size of header should be 116 bytes)
	Uint8 Buffer[128];

	// Walking pointer for buffer writing
	Uint8 *pBuffer;

	// ** Write ContextID **

	pBuffer = Buffer + MakeBER(Buffer, 4, 16, 4);		// Write BER length of ContextID (and index end of the length)

	if(!ContextID)
	{
		error("KLVEObject::WriteKL() called without a valid ContextID\n");

		// Write a dummy value rather than just discarding all the data
		memset(pBuffer, 0, 16);
	}
	else
	{
		memcpy(pBuffer, ContextID->GetValue(), 16);
	}
	pBuffer += 16;


	// ** Write PlaintextOffset **

	// First adjust if required
	if((Int64)PlaintextOffset > ValueLength) PlaintextOffset = ValueLength;

	pBuffer += MakeBER(pBuffer, 4, 8, 4);
	PutU64(PlaintextOffset, pBuffer); pBuffer += 8;


	// ** Write SourceKey **

	pBuffer += MakeBER(pBuffer, 4, 16, 4);
	if(!SourceKey)
	{
		error("KLVEObject::WriteKL() called without a valid SourceKey\n");

		// Write a dummy value rather than just discarding all the data
		memset(pBuffer, 0, 16);
	}
	else
	{
		memcpy(pBuffer, SourceKey->GetValue(), 16);
	}
	pBuffer += 16;


	// ** Write SourceLength **

#ifdef SOURCELENGTH_BER
	DataChunkPtr Len = MakeBER(ValueLength, SourceLengthFormat);
	pBuffer += MakeBER(pBuffer, 4, Len->Size, 4);
	memcpy(pBuffer, Len->Data, Len->Size);
	pBuffer += Len->Size;
#else // SOURCELENGTH_BER
	pBuffer += MakeBER(pBuffer, 4, 8, 4);
	PutU64(ValueLength, pBuffer);
	pBuffer += 8;
#endif // SOURCELENGTH_BER

	// ** Calculate EncryptedLength **
	// There is padding from 1 to 16 bytes at the end of the data

	EncryptedLength = ((ValueLength-PlaintextOffset) + EncryptionGranularity) / EncryptionGranularity;
	EncryptedLength *= EncryptionGranularity;
	EncryptedLength += PlaintextOffset;
//@'@printf("}} ValueLength %d\n}} PlaintextOffset %d\n", (int)ValueLength, (int)PlaintextOffset);
//@'@printf("@@ Size of Encrypted Source Value is %s\n", Int64toString(EncryptedLength).c_str());

	// ** Write the length of the encrypted source value
	// ** Including IV and Check as well as any padding

	pBuffer += MakeBER(pBuffer, 9, EncryptedLength + EncryptionOverhead, 0);

	// Set up the data offset
	// Note that we haven't yet written the IV and check value, but we count those as "header" bytes
	DataOffset = (Uint32)(pBuffer - Buffer) + EncryptionOverhead;


	/** Write Out the Header **/

	// Set the length to be the size of the header plus the size of the encrypted data
	Dest.OuterLength = DataOffset + EncryptedLength;

	// Start off with the actual KL - using the OuterLength to include the header
	Uint32 KLBytes = Base_WriteKL(LenSize, Dest.OuterLength);

	// Then write the header
	Dest.File->Write(Buffer, DataOffset);

//##
//## DRAGONS: We have not done the AS-DCP footer...
//##

	return KLBytes + DataOffset;
}

