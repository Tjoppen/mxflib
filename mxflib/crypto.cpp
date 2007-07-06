/*! \file	crypto.cpp
 *	\brief	Implementation of classes that hanldle basic encryption and decryption
 *
 *	\version $Id: crypto.cpp,v 1.13 2007/07/06 12:01:31 matt-beard Exp $
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
bool KLVEObject::SetDecryptIV(size_t IVSize, const UInt8 *IV, bool Force /*=false*/)
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
	SequenceNumber = 0;
	HasSequenceNumber = false;

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
	if(Source.KLSize >= 0) return static_cast<Int32>(Source.KLSize + DataOffset);
	else if(Dest.KLSize >= 0) return static_cast<Int32>(Dest.KLSize + DataOffset);
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
	UInt8 *p = Data.Data;


	// ** Load the ContextID **

	// Read the BER length and move the pointer
	UInt8 *Prev_p = p;
	Length ItemLength = ReadBER(&p, (int)Bytes);
	Bytes -= (p - Prev_p);

	if((ItemLength != 16) || (Bytes < 16))
	{
		error("Invalid AS-DCP data (%s) in %s\n", "ContextID not 16 bytes", GetSourceLocation().c_str());
		return false;
	}

	// Build the ContextID
	ContextID = new UUID(p);

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
	// DRAGONS: The format used in the file is UInt64, but as KLVObject used "Length" (Int64) for its lengths
	//          we impose a limit of 2^63 bytes on Plaintext offset and keep everything signed internally
	PlaintextOffset = (Length)GetU64(p);

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
	//       A later version of the specification uses UInt64
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

	// If not 8-bytes it can't be UInt64 - otherwise it may be BER or UInt64 so see if it is valid BER
	// This will fail for unimaginably large UInt64 coded SourceLengths
	if((ItemLength != 8) || (*p == 0x87))
	{
		// Read the BER SourceLength
		Prev_p = p;
		ValueLength = ReadBER(&p, (int)Bytes);
		SourceLengthFormat = static_cast<int>(p - Prev_p);
		Bytes -= SourceLengthFormat;
	}
	else
	{
		// Read the UInt64 SourceLength
		ValueLength = GetU64(p);
		SourceLengthFormat = 8;
		p += 8;
		Bytes -= 8;
	}

	if(PlaintextOffset > ValueLength)
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
			GC.Item = 0;
			GC.Count = 0;
			GC.ElementType = 0;
			GC.Number = 0;
			return GC;
		}


	// Return the GCElementKind of the plaintext KLV
	return mxflib::GetGCElementKind(SourceKey);
}


//! Determine if this is a system item
bool KLVEObject::IsGCSystemItem(void)
{
	if(!DataLoaded) 
	{
		if(!LoadData()) return false;
	}

	return mxflib::IsGCSystemItem(TheUL);
}


//! Get the track number of this KLVObject (if it is a GC KLV, else 0)
UInt32 KLVEObject::GetGCTrackNumber(void)
{
	if(!DataLoaded) if(!LoadData()) return 0;

	// Return the TrackNumber of the plaintext KLV
	return mxflib::GetGCTrackNumber(SourceKey);
}


//! Read data from a specified position in the KLV value field into the DataChunk
/*! \param Offset Offset from the start of the KLV value from which to start reading
 *  \param Size Number of bytes to read, if -1 all available bytes will be read (which could be billions!)
 *  \return The number of bytes read
 */
size_t KLVEObject::ReadDataFrom(Position Offset, size_t Size /*=-1*/)
{
	// Don't decrypt if we have no decryption wrapper
	if(!Decrypt) return Base_ReadDataFrom(Offset, Size);

	// Load the header if required (and if we can!)
	if(!DataLoaded) if(!LoadData()) return 0;

	// Don't bother reading zero bytes or off the end of the value
	if((Size == 0) || (Offset >= ValueLength))
	{
		Data.Resize(0);
		return 0;
	}

	// Load the IV and check the Check value if this is the first read
	if(	CurrentReadOffset == 0)
	{
		if( Base_ReadDataFrom(DataOffset - EncryptionOverhead, EncryptionOverhead) < EncryptionOverhead)
		{
			error("Unable to read Initialization Vector and Check Value in KLVEObject::ReadDataFrom()\n");
			return 0;
		}

		// Update the current hash if we are calculating one
		if(ReadHasher) ReadHasher->HashData(Data);

		// Initialize the decryption engine with the specified Initialization Vector
		Decrypt->SetIV(16, Data.Data, true);

		// Decrypt the check value...
		DataChunkPtr PlainCheck = Decrypt->Decrypt(16, &Data.Data[16]);

		// Encrypt the check value... (Which is "CHUKCHUKCHUKCHUK" who ever said Chuck Harrison has no ego?)
		const UInt8 DefinitivePlainCheck[16] = { 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B };
		if((!PlainCheck) || (memcmp(PlainCheck->Data, DefinitivePlainCheck, PlainCheck->Size) != 0))
		{
			error("Check value did not correctly decrypt in KLVEObject::ReadDataFrom() - is the encryption key correct?\n");
			return 0;
		}
	}

	// If all the requested bytes are encrypted read-and-decrypt
	if(Offset >= PlaintextOffset) 
	{
		// Check if an attempt is being made to random access the encrypted data - and barf if this is so
		if(Offset != CurrentReadOffset)
		{
			error("Attempt to perform random-access reading of an encrypted KLV value field\n");
			return 0;
		}

		size_t Ret = ReadCryptoDataFrom(Offset, Size);

		// Read the AS-DCP footer if we have read the last of the data
		if(CurrentReadOffset >= ValueLength) if(!ReadFooter()) { Ret = 0; Data.Resize(0); }

		return Ret;
	}

	// If all the bytes requested are plaintext use the base read
	if( (Size != static_cast<size_t>(-1)) && ((Offset + Size) < PlaintextOffset) )
	{
		// Check if an attempt is being made to random access the plaintext while hashing - and barf if this is so
		if(ReadHasher && (Offset != CurrentReadOffset))
		{
			error("Attempt to perform random-access reading of an encrypted KLV value field\n");
			return 0;
		}

		size_t Ret = Base_ReadDataFrom(DataOffset + Offset, Size);

		// Update the read pointer (it is possible to random access within the plaintext area)
		CurrentReadOffset = Offset + Ret;

		// Update the current hash if we are calculating one
		if(ReadHasher) ReadHasher->HashData(Data);

		// Read the AS-DCP footer if we have read the last of the data
		if(CurrentReadOffset >= ValueLength) if(!ReadFooter()) { Ret = 0; Data.Resize(0); }

		return Ret;
	}

	/* We will be mixing plaintext and encrypted */

	// Check if an attempt is being made to random access the encrypted data - and barf if this is so
	// DRAGONS: It is possible to re-load the initial IV to allow a "rewind" but this is not implemented
	if(CurrentReadOffset > PlaintextOffset)
	{
		error("Attempt to perform random-access reading of an encrypted KLV value field\n");
		return 0;
	}

	// Determine how many plaintext bytes could be available (maximum)
	Length PlainSize = PlaintextOffset - Offset;

	// Validate the size
	if((sizeof(size_t) < 8) && (PlainSize > 0xffffffff))
	{
		error("Encrypted KLV contains > 4GBytes of plaintext, but this platform can only handle <= 4GByte chunks\n");
		return 0;
	}

	// Try and read them all
	size_t PlainBytes = Base_ReadDataFrom(DataOffset + Offset, static_cast<size_t>(PlainSize));

	// Update the current hash if we are calculating one
	if(ReadHasher) ReadHasher->HashData(Data);

	// If we couldn't read them all then the data ends before any encrypted bytes so we can exit now
	if(PlainBytes < static_cast<size_t>(PlainSize))
	{
		// DRAGONS: We don't read the footer if we ran out of data early... should we issue an error or a warning?
		//# Read the AS-DCP footer if we have read the last of the data
		//# if(CurrentReadOffset >= ValueLength) ReadFooter();

		return PlainBytes;
	}

	/* We have all the plaintext bytes from Offset forwards, now we read all encrypted bytes too */

	// Take the buffer from the current DataChunk to preserve it
	DataChunkPtr PlainData = new DataChunk;
	PlainData->TakeBuffer(Data, true);

	// Work out how many encrypted bytes to read
	size_t EncSize;
	if(Size == static_cast<size_t>(-1)) EncSize = Size; else EncSize = Size - static_cast<size_t>(PlainSize);

	// Read the encrypted bytes
	ReadCryptoDataFrom(PlaintextOffset, EncSize);

	// Append the decrypted data to the plaintext data
	PlainData->Append(Data);

	// Transfer this data to the "current" DataChunk
	Data.TakeBuffer(PlainData);

	// Set the "next" position to just after the end of what we read
	CurrentReadOffset = Offset + Data.Size;

	// Read the AS-DCP footer if we have read the last of the data
	if(CurrentReadOffset >= ValueLength) if(!ReadFooter()) Data.Resize(0);

	// Return the total number of bytes
	return Data.Size;
}


//! Read data from a specified position in the encrypted portion of the KLV value field into the DataChunk
/*! \param Offset Offset from the start of the KLV value from which to start reading
 *  \param Size Number of bytes to read, if = -1 all available bytes will be read (which could be billions!)
 *  \return The number of bytes read
 *	The IV must have already been set.
 *  Only encrypted parts of the value may be read using this function (i.e. Offset >= PlaintextOffset)
 */
size_t KLVEObject::ReadCryptoDataFrom(Position Offset, size_t Size /*=-1*/)
{
	// Initially plan to read all the bytes available
	Length BytesToRead = EncryptedLength - Offset;

	// Limit to specified size if specified and if < available
	if( (Size != static_cast<size_t>(-1)) && (Size < BytesToRead)) BytesToRead = Size;

	// Assume that the read will succeed and move the "next" pointer accordingly
	CurrentReadOffset += BytesToRead;

	// Check if all the requested bytes have already been decrypted
	if(BytesToRead <= PreDecrypted)
	{
		// Set the data	into the DataChunk
		Data.Set(static_cast<size_t>(BytesToRead), PreDecryptBuffer);

		// Remove any padding if required
		if(Offset + Data.Size > ValueLength)
		{
			BytesToRead = ValueLength - Offset;
			Data.Resize(static_cast<size_t>(BytesToRead));
		}

		// Shuffle any remaining bytes
		memmove(PreDecryptBuffer, &PreDecryptBuffer[BytesToRead], static_cast<size_t>(EncryptionGranularity - BytesToRead));

		// All done
		return static_cast<size_t>(BytesToRead);
	}

	// Work out how many bytes have to be decrypted this time
	// First we work out the number of EncryptionGranularity sized chunks to decrypt...
	Length BytesToDecrypt = ((BytesToRead - PreDecrypted) + (EncryptionGranularity - 1)) / EncryptionGranularity;

	// ... then scale this back into bytes
	BytesToDecrypt *= EncryptionGranularity;

	// Sanity check the size of this chunk
	if((sizeof(size_t) < 8) && (BytesToDecrypt > 0xffffffff))
	{
		error("Tried to decrypt > 4GBytes, but this platform can only handle <= 4GByte chunks\n");
		return 0;
	}

	// Read the encrypted data
	if(!ReadChunkedCryptoDataFrom(PreDecrypted + Offset, static_cast<size_t>(BytesToDecrypt)))
	{
		// Abort if the decrypt failed
		return 0;
	}

	// Add back in any pre-decrypted data
	if(PreDecrypted)
	{
		// Steal the newly decrypted bytes
		UInt8 *Buffer = Data.StealBuffer(true);

		// Make a buffer large enough for the complete data
		Data.ResizeBuffer(PreDecrypted + static_cast<size_t>(BytesToDecrypt));

		// Put the pre-decrypted bytes at the start
		Data.Set(PreDecrypted, PreDecryptBuffer);

		// Follow it with the newly decrypted data
		Data.Set(static_cast<size_t>(BytesToDecrypt), Buffer, PreDecrypted);
	}

	// If we have decrypted more than requested store them as pre-decrypted for next time
	if(Data.Size > Size)
	{
		PreDecrypted = static_cast<int>(Data.Size - Size);
		memcpy(PreDecryptBuffer, &Data.Data[Size], PreDecrypted);
		Data.Resize(Size);
	}
	else 
		PreDecrypted = 0;

	// Remove any padding if required
	if(Offset + Data.Size > ValueLength)
	{
		Size = static_cast<size_t>(ValueLength - Offset);
		Data.Resize(Size);
	}

	return Size;
}


//! Read an integer set of chunks from a specified position in the encrypted portion of the KLV value field into the DataChunk
/*! \param Offset Offset from the start of the KLV value from which to start reading
 *  \param Size Number of bytes to read, if = -1 all available bytes will be read (which could be billions!)
 *  \return The number of bytes read
 *	The IV must have already been set, Size must be a multiple of 16 as must (Offset - PlaintextOffset). 
 *  Only encrypted parts of the value may be read using this function (i.e. Offset >= PlaintextOffset)
 */
size_t KLVEObject::ReadChunkedCryptoDataFrom(Position Offset, size_t Size)
{
	// Read the encrypted data
	size_t NewSize = Base_ReadDataFrom(DataOffset + Offset, Size);

	// Update the current hash if we are calculating one
	if(ReadHasher) ReadHasher->HashData(Data);

	// Resize if less bytes than requested were actualy read
	if(NewSize != Size)
	{
		Size = NewSize;
		Data.Resize(Size);
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
	if(Decrypt->CanDecryptInPlace(Size))
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
	Data.TakeBuffer(NewData);

	return Size;
}


//! Write data from a given buffer to a given location in the destination file
/*! \param Buffer Pointer to data to be written
 *  \param Offset The offset within the KLV value field of the first byte to write
 *  \param Size The number of bytes to write
 *  \return The number of bytes written (not including AS-DCP header and footer)
 *  \note As there may be a need for the implementation to know where within the value field
 *        this data lives, there is no WriteData(Buffer, Size) function.
 */
size_t KLVEObject::WriteDataTo(const UInt8 *Buffer, Position Offset, size_t Size)
{
	// Don't encrypt if we have no encryption wrapper
	if(!Encrypt) return Base_WriteDataTo(Buffer, Offset, Size);

	// Don't write too many bytes
	if(static_cast<Length>(Size) > (EncryptedLength - Offset)) Size = static_cast<size_t>(EncryptedLength - Offset);

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
			// DRAGONS: Strange double-casting is to remove pointer conversion warning in 64-bit systems
			srand(static_cast<unsigned int>(time(NULL)) ^ static_cast<unsigned int>(reinterpret_cast<UInt64>(Data.Data)) ^ (clock() << 2) ^ rand());

			int i;
			for(i=0; i< 16; i++) { IV[i] = (UInt8)rand(); };

			Encrypt->SetIV(16, IV, true);
		}

		Base_WriteDataTo(IV, DataOffset - EncryptionOverhead, 16);

		// Update the current hash if we are calculating one
		if(WriteHasher) WriteHasher->HashData(16, IV);

		// ** Write the check value

		// Encrypt the check value... (Which is "CHUKCHUKCHUKCHUK" who ever said Chuck Harrison has no ego?)
		const UInt8 PlainCheck[16] = { 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B };
		DataChunkPtr CheckData = Encrypt->Encrypt(16, PlainCheck);

		// Write the encrypted check value
		if(CheckData && (CheckData->Size == 16))
		{
			Base_WriteDataTo(CheckData->Data, DataOffset - EncryptionOverhead + 16, 16);

			// Update the current hash if we are calculating one
			if(WriteHasher) WriteHasher->HashData(CheckData);
		}
		else
		{
			error("Could not encrypt check value - encryption system is not working correctly\n");
		}
	}

	// If all the requested bytes are to be encrypted encrypt-and-write
	if(Offset >= PlaintextOffset) 
	{
		// Check if an attempt is being made to random access the encrypted data - and barf if this is so
		if(Offset != CurrentWriteOffset)
		{
			error("Attempt to perform random-access writing of an encrypted KLV value field\n");
			return 0;
		}

		size_t Ret = WriteCryptoDataTo(Buffer, Offset, Size);

		// Read the AS-DCP footer if we have read the last of the data
		if(CurrentWriteOffset >= ValueLength) WriteFooter();

		return Ret;
	}


	// If all the bytes requested are plaintext use the base write
	if( (Offset + Size) < PlaintextOffset)
	{
		size_t Ret = Base_WriteDataTo(Buffer, DataOffset + Offset, Size);
		
		// Update the current hash if we are calculating one
		// TODO: Sort the possible overflow here
		if(WriteHasher) WriteHasher->HashData(Size, Buffer);

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

		// Read the AS-DCP footer if we have read the last of the data
		if(CurrentWriteOffset >= ValueLength) WriteFooter();

		return Ret;
	}

	/* We will be mixing plaintext and encrypted */

	// Check if an attempt is being made to random access the encrypted data - and barf if this is so
	// DRAGONS: It is possible to re-set the initial IV to allow a "rewind" but this is not implemented
	if(CurrentWriteOffset > PlaintextOffset)
	{
		error("Attempt to perform random-access writing of an encrypted KLV value field\n");
		return 0;
	}

	// Determine how many plaintext bytes will be written
	size_t PlainSize = static_cast<size_t>(PlaintextOffset - Offset);

	// Write the plaintext bytes
	size_t PlainBytes = Base_WriteDataTo(Buffer, DataOffset + Offset, PlainSize);

	if(PlainBytes != PlainSize)
	{
		error("Writing of Plaintext bytes failed\n");
		return PlainBytes;
	}

	// Update the current hash if we are calculating one
	// TODO: Sort the possible overflow here
	if(WriteHasher) WriteHasher->HashData(Size, Buffer);

	// Update the write pointer after the plaintext write
	CurrentWriteOffset = Offset + PlainBytes;


	/* We have written all the plaintext bytes from Offset forwards, now we write to encrypted bytes too */

	// Write the encrypted bytes
	size_t EncBytes = WriteCryptoDataTo(&Buffer[PlainBytes], PlaintextOffset, Size - PlainSize);

	// Read the AS-DCP footer if we have read the last of the data
	if(CurrentWriteOffset >= ValueLength) WriteFooter();

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
size_t KLVEObject::WriteCryptoDataTo(const UInt8 *Buffer, Position Offset, size_t Size)
{
	// Self-deleting store to allow us to extend working buffer if required
	DataChunk TempData;

	// Are we going to need to write padding bytes?
	bool AddPadding;
	if(static_cast<Length>(Offset + Size) >= ValueLength) AddPadding = true; else AddPadding = false;

	// Check if this is a "pointless" zero-byte write (rather than a request to write padding only)
	if((!AddPadding) && (Size == 0)) return 0;

	// Assume that the write will succeed and move the "next" pointer accordingly
	CurrentWriteOffset += Size;

	// Check if all the bytes will fit in the AwaitingEncryptionBuffer
	if((!AddPadding) && (Size < static_cast<size_t>(EncryptionGranularity - AwaitingEncryption)))
	{
		// Add to the end of the waiting buffer
		memcpy(&AwaitingEncryptionBuffer[AwaitingEncryption], Data.Data, Size);

		// All done
		return Size;
	}

	// Work out how many bytes we need to encrypt (estimate one - as many as offered)
	Length BytesToEncrypt = Size;

	// If there are any bytes waiting they need to be added to this write
	if(AwaitingEncryption)
	{
		// Build the full data in the temporary buffer
		TempData.ResizeBuffer(AwaitingEncryption + Size);

		// Start with "waiting" data
		TempData.Set(AwaitingEncryption, AwaitingEncryptionBuffer);

		// Copy in the new data
		TempData.Append(Size, Buffer);

		// Replace the working buffer pointer with a pointer to the temporary buffer
		Buffer = TempData.Data;

		// Update the offset (move it back to the first waiting byte)
		Offset -= AwaitingEncryption;

		// Record the revised size
		BytesToEncrypt = AwaitingEncryption + Size;
	}

	// Pad the data if required (i.e. if this is the last chunk of data)
	if(AddPadding)
	{
		if(Offset + BytesToEncrypt > ValueLength)
		{
			error("Attempted to write beyond the end of the encrypted value in KLVEObject::WriteCryptoDataTo()\n");
			return 0;
		}

		// Start by encrypting all but the last 16 bytes (including padding)
		Length StartSize = EncryptedLength - (EncryptionGranularity + Offset);

		// Sanity check the size of this chunk
		if((sizeof(size_t) < 8) && (StartSize > 0xffffffff))
		{
			error("Tried to encrypt > 4GBytes, but this platform can only handle <= 4GByte chunks\n");
			return 0;
		}

		// Don't write zero bytes
		if(StartSize)
		{
			// Encrypt by making a copy
			DataChunkPtr NewData = Encrypt->Encrypt(static_cast<size_t>(StartSize), Buffer);
			if(!NewData) return 0;

			// Write the encrypted data
			Base_WriteDataTo(NewData->Data, DataOffset + Offset, static_cast<size_t>(StartSize));

			// Update the current hash if we are calculating one
			// TODO: Sort the possible overflow here
			if(WriteHasher) WriteHasher->HashData(static_cast<size_t>(StartSize), NewData->Data);
		}

		// Buffer for last data to be encrypted
		UInt8 TempBuffer[EncryptionGranularity];

		// Copy in the remaining bytes from the end of the given buffer
		// Add padding bytes in a 16-byte version of the scheme defined in RFC 2898
		const UInt8 *pSrc = &Buffer[StartSize];
		UInt8 *pDst = TempBuffer;
		int i;
		int EncData = (int)(BytesToEncrypt - StartSize);
		int Pad = EncryptionGranularity - EncData;
		for(i=0; i<16; i++)
		{
			if(i < EncData) *(pDst++) = *(pSrc++);
			else *(pDst++) = (UInt8)Pad;
		}

		// Encrypt by making a copy
		DataChunkPtr NewData = Encrypt->Encrypt(EncryptionGranularity, TempBuffer);
		if(!NewData) return 0;

		// Write the encrypted data
		Base_WriteDataTo(NewData->Data, DataOffset + Offset + StartSize, EncryptionGranularity);

		// Update the current hash if we are calculating one
		if(WriteHasher) WriteHasher->HashData(EncryptionGranularity, NewData->Data);

		// There are no more bytes to encrypt
		AwaitingEncryption = 0;

		// Lie and say we wrote the requested number of bytes (tells the caller all was fine)
		return Size;
	}

	// Work out how many bytes have to be encrypted this time (an integer number of chunks)
	Length BytesRequiringEncryption = BytesToEncrypt;
	BytesToEncrypt = BytesToEncrypt / EncryptionGranularity;
	BytesToEncrypt *= EncryptionGranularity;

	// Sanity check the size of this chunk
	if((sizeof(size_t) < 8) && (BytesToEncrypt > 0xffffffff))
	{
		error("Tried to encrypt > 4GBytes, but this platform can only handle <= 4GByte chunks\n");
		return 0;
	}

	// Any differnece will be "left-over"
	AwaitingEncryption = static_cast<int>(BytesRequiringEncryption - BytesToEncrypt);

	// Encrypt by making a copy
	DataChunkPtr NewData = Encrypt->Encrypt(static_cast<size_t>(BytesToEncrypt), Buffer);
	if(!NewData) return 0;

	// If there will be any "left-over" bytes they will be awaiting next time
	if(AwaitingEncryption)
	{
		memcpy(AwaitingEncryptionBuffer, &Buffer[BytesToEncrypt], AwaitingEncryption);
	}

	// Write the encrypted data
	Size = Base_WriteDataTo(NewData->Data, DataOffset + Offset, static_cast<size_t>(BytesToEncrypt));

	// Update the current hash if we are calculating one
	// TODO: Sort the possible overflow here
	if(WriteHasher) WriteHasher->HashData(Size, NewData->Data);

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
	TheUL = new UL(EncryptedTriplet_UL);
	
	// Small buffer for header (note: max valid size of header should be 116 bytes)
	UInt8 Buffer[128];

	// Walking pointer for buffer writing
	UInt8 *pBuffer;

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
	if(PlaintextOffset > ValueLength) PlaintextOffset = ValueLength;

	pBuffer += MakeBER(pBuffer, 4, 8, 4);
	PutU64((UInt64)PlaintextOffset, pBuffer); pBuffer += 8;


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

	// ** Write the length of the encrypted source value
	// ** Including IV and Check as well as any padding

	pBuffer += MakeBER(pBuffer, 9, EncryptedLength + EncryptionOverhead, 0);

	// Set up the data offset
	// Note that we haven't yet written the IV and check value, but we count those as "header" bytes
	DataOffset = (pBuffer - Buffer) + EncryptionOverhead;


	/** Write Out the Header **/

	// Work out the length of the footer
	CalcFooterLength();

	// Set the length to be the size of the header plus the size of the encrypted data
	Dest.OuterLength = DataOffset + EncryptedLength + FooterLength;

	// Start off with the actual KL - using the OuterLength to include the header
	UInt32 KLBytes = Base_WriteKL(LenSize, Dest.OuterLength);

	// Then write the header
	Dest.File->Write(Buffer, DataOffset);

//##
//## DRAGONS: We have not done the AS-DCP footer...
//##

	return KLBytes + DataOffset;
}


//! Read the AS-DCP footer (if any)
/*! \return false on error, else true
 */
bool KLVEObject::ReadFooter(void)
{
	// Clear the optional values
	TrackFileID = NULL;
	SequenceNumber = 0;
	HasSequenceNumber = false;
	MIC = NULL;

	DataChunkPtr Buffer = new DataChunk;

	// Read all data following the encrypted data
	size_t Bytes = Base_ReadDataFrom(*Buffer, DataOffset + EncryptedLength);

	// Leave now if no footer
	if(Bytes == 0) 
	{
		// If we are hashing we should record the MIC here
		if(ReadHasher) MIC = ReadHasher->GetHash();
		
		return true;
	}

	// Index the start of the data
	UInt8 *p = Buffer->Data;

	// ** Load the (optional) TrackFileID **

	// Read the BER length and move the pointer
	UInt8 *Prev_p = p;
	Length ItemLength = ReadBER(&p, (int)Bytes);
	Bytes -= (p - Prev_p);

	if(ItemLength > 0)
	{
		if((ItemLength != 16) || (Bytes < 16))
		{
			error("Invalid AS-DCP data (%s) in %s\n", "TrackFileID not 16 bytes", GetSourceLocation().c_str());
			return false;
		}

		// Build the ContextID
		TrackFileID = new UUID(p);

		// Update pointer and count
		p += 16;
		Bytes -= 16;
	}


	// ** Load the (optional) SequenceNumber **

	// Read the BER length and move the pointer
	Prev_p = p;
	ItemLength = ReadBER(&p, (int)Bytes);
	Bytes -= (p - Prev_p);

	if(ItemLength > 0)
	{
		if((ItemLength != 8) || (Bytes < 8))
		{
			error("Invalid AS-DCP data (%s) in %s\n", "SequenceNumber not 8 bytes", GetSourceLocation().c_str());
			return false;
		}

		// Read the sequence number
		SequenceNumber = GetU64(p);
		HasSequenceNumber = true;

		// Update pointer and count
		p += 8;
		Bytes -= 8;
	}


	// ** Load the (optional) MIC **

	// Read the BER length and move the pointer
	Prev_p = p;
	ItemLength = ReadBER(&p, (int)Bytes);
	Bytes -= (p - Prev_p);

	// Finish the hash if we are hashing
	// General concensus seems to be that the BER length of the hash is included IN the hash!
	if(ReadHasher)
	{
		// Include the section of the footer preceeding the MIC in the hash
		ReadHasher->HashData((int)(p - Buffer->Data), Buffer->Data);
	}

	if(ItemLength > 0)
	{
		if((ItemLength != 20) || (Bytes < 20))
		{
			error("Invalid AS-DCP data (%s) in %s\n", "MIC not 20 bytes", GetSourceLocation().c_str());
			return false;
		}

		// Read the sequence number
		MIC = new DataChunk(20, p);

		// If we are hashing as we read check that the MIC matches
		if(ReadHasher)
		{
			DataChunkPtr CalcMIC = ReadHasher->GetHash();

			if(*MIC != *CalcMIC)
			{
				error("Message Integrity Code check failed\n");
				return false;
			}
		}
	}
	else
	{
		// No MIC in file to check so just record what we calculated
		if(ReadHasher) MIC = ReadHasher->GetHash();
	}

	return true;
}


//! Calculate the size of the AS-DCP footer for this KLVEObject
/*! The size it returned and also written to property FooterLength */
UInt32 KLVEObject::CalcFooterLength(void)
{
	// Start with a zero-byte footer
	FooterLength = 0;

	if((!TrackFileID) && (!HasSequenceNumber) && (!WriteHasher)) return FooterLength;

	// 4-byte BER plus an optional 16-byte TrackFileID
	if(!TrackFileID) FooterLength += 4; else FooterLength += 20;

	// 4-byte BER plus an optional 8-byte HasSequenceNumber
	if(!HasSequenceNumber) FooterLength += 4; else FooterLength += 12;

	// DRAGONS: We don't bother to write a "missing" MIC as this is the last item so can just be omitted
	if(WriteHasher) FooterLength += 24;

	return FooterLength;
}

//! Write the AS-DCP footer (if fequired)
/*! \return false on error, else true
 *
 *  DRAGONS: Ensure that the data written matches the size given by CalcFooterLength()
 */
bool KLVEObject::WriteFooter(void)
{
	// Don't write anything if we don't need to
	if((!TrackFileID) && (!HasSequenceNumber) && (!WriteHasher)) return true;

	// Make a reasonable sized buffer
	DataChunkPtr Buffer = new DataChunk(64);

	// Make a pointer to walk through the write buffer
	unsigned char *p = Buffer->Data;

	if(!TrackFileID)
	{
		// Write a "missing" TrackFile ID
		p += MakeBER(p, 4, 0);
	}
	else
	{
		// Write the TrackFile ID
		p += MakeBER(p, 4, 16);
		memcpy(p, TrackFileID->GetValue(), 16);
		p += 16;
	}

	if(!HasSequenceNumber)
	{
		// Write a "missing" sequence number
		p += MakeBER(p, 4, 0);
	}
	else
	{
		// Write the sequence number
		p += MakeBER(p, 4, 8);
		PutU64(SequenceNumber, p);
		p += 8;
	}

	// DRAGONS: We don't bother to write a "missing" MIC as this is the last item so can just be omitted
	if(WriteHasher)
	{
		// Write the BER length for the hash - general concensus seems to be that this is included IN the hash!
		p += MakeBER(p, 4, 20);

		// Finish calculating the MIC
		WriteHasher->HashData((int)(p - Buffer->Data), Buffer->Data);

		// Get the hash
		DataChunkPtr Hash = WriteHasher->GetHash();

		// Write the hash
		if(Hash->Size == 20) memcpy(p, Hash->Data, 20);
		else error("Hash for this KLVEObject is not 20 bytes\n");
		p += 20;
	}

	// Resize the buffer to exactly the amount of data we built
	ASSERT((UInt32)(p - Buffer->Data) == FooterLength);
	Buffer->Resize((int)(p - Buffer->Data));

	// Write the footer
	Dest.File->Write(Buffer);

	// Return "All OK"
	return true;
}
