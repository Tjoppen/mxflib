/*! \file	crypto.cpp
 *	\brief	Implementation of classes that hanldle basic encryption and decryption
 *
 *	\version $Id: crypto.cpp,v 1.1.2.4 2004/07/05 14:46:48 matt-beard Exp $
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

	KLSize = Object->KLSize;
	SourceFile = Object->SourceFile;
	SourceOffset = Object->SourceOffset;
	DestFile = Object->DestFile;
	DestOffset = Object->DestOffset;
	ValueLength = Object->ValueLength;
	OuterLength = Object->OuterLength;
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
	return KLSize + DataOffset;
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
	return KLSize;
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
	Length ItemLength = ReadBER(&p, Bytes);
	Bytes -= (p - Prev_p);

	if((ItemLength != 16) || (Bytes < 16))
	{
		error("Invalid AS-DCP data in %s\n", GetSourceLocation().c_str());
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
	ItemLength = ReadBER(&p, Bytes);
	Bytes -= (p - Prev_p);

	if((ItemLength != 8) || (Bytes < 8))
	{
		error("Invalid AS-DCP data in %s\n", GetSourceLocation().c_str());
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
	ItemLength = ReadBER(&p, Bytes);
	Bytes -= (p - Prev_p);

	if((ItemLength != 16) || (Bytes < 16))
	{
		error("Invalid AS-DCP data in %s\n", GetSourceLocation().c_str());
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

	// Read the BER length and move the pointer
	Prev_p = p;
	ItemLength = ReadBER(&p, Bytes);
	Bytes -= (p - Prev_p);

	if((ItemLength <= 0) || (ItemLength >= 9) || (Bytes < ItemLength))
	{
		error("Invalid AS-DCP data in %s\n", GetSourceLocation().c_str());
		return false;
	}

	// Read the SourceLength
	Prev_p = p;
	ValueLength = ReadBER(&p, Bytes);
	SourceLengthFormat = (p - Prev_p);
	Bytes -= SourceLengthFormat;

	// Read and discard the BER length of the Encrypted Source Value
	Prev_p = p;
	ReadBER(&p, Bytes);
	Bytes -= (p - Prev_p);

	// Check that we have enough left for the IV and Check Value
	if(Bytes < EncryptionOverhead)
	{
		error("Invalid AS-DCP data in %s\n", GetSourceLocation().c_str());
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

	Length BytesLeft = OuterLength - DataOffset;
	EncryptedLength = (ValueLength + EncryptionGranularity) / EncryptionGranularity;
	EncryptedLength *= EncryptionGranularity;

	if(BytesLeft < EncryptedLength)
	{
		error("Invalid AS-DCP data in %s\n", GetSourceLocation().c_str());
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
	// Don't decrypt if we have no decryption wrapper
	if(!Decrypt) return Base_ReadDataFrom(Offset, Size);

	// Load the header if required (and if we can!)
	if(!DataLoaded) if(!LoadData()) return 0;

	// Don't try reading off the end
	if(Offset >= ValueLength) return 0;

	// FIXME: We need some way to update the IV and ensure we only read/decrypt on 16-byte boundaries!!

	// Load the IV and check the Check value if this is the first read
	if(Offset == 0)
	{
		if( Base_ReadDataFrom(DataOffset - EncryptionOverhead, EncryptionOverhead) < EncryptionOverhead)
		{
			error("Unable to read Initialization Vector and Check Value in KLVEObject::ReadDataFrom()\n");
			return 0;
		}

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

	// Read the encrypted data (and reset size to what we actually read)
	Length NewSize = Base_ReadDataFrom(DataOffset + Offset, Size);

	// Resize if less bytes than requested were actualy read
	if(NewSize != Size)
	{
		Size = NewSize;
		Data.Resize(Size);
		if(Size == 0) return 0;
	}

	// See if we can decrypt this in place...
	if(Decrypt->CanDecryptInPlace(Size))
	{
		if(!Decrypt->DecryptInPlace(Data))
		{
			Data.Resize(0);
			return 0;
		}

		// Remove any padding if required
		if(Offset + Size > ValueLength)
		{
			Size = ValueLength - Offset;
			Data.Resize(Size);
		}

		return Size;
	}

	// Decrypt by making a copy
	DataChunkPtr NewData = Decrypt->Decrypt(Data);
	if(!NewData)
	{
		Data.Resize(0);
		return 0;
	}

	// Take over the buffer from the decrypted data
	Data.TakeBuffer(NewData);

	// Remove any padding if required
	if(Offset + Size > ValueLength)
	{
		Size = ValueLength - Offset;
		Data.Resize(Size);
	}

	return Size;
}


//! Write data from a given buffer to a given location in the destination file
/*! \param Buffer Pointer to data to be written
 *  \param Offset The offset within the KLV value field of the first byte to write
 *  \param Size The number of bytes to write
 *  \return The number of bytes written
 *  \note As there may be a need for the implementation to know where within the value field
 *        this data lives, there is no WriteData(Buffer, Size) function.
 *
 *  TODO: Monitor write order and error if an attempt is made to write "random-access" rather
 *        than in a contiguous set of writes
 */
Length KLVEObject::WriteDataTo(Uint8 *Buffer, Position Offset, Length Size)
{
	// Don't encrypt if we have no encryption wrapper
	if(!Encrypt) return Base_WriteDataTo(Buffer, Offset, Size);

	// Don't write too many bytes
	if(Size > (EncryptedLength - Offset)) Size = (EncryptedLength - Offset);

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

	if(DataOffset == 0) 
	{
		error("KLVEObject::WriteData called before KLVEObject::WriteKL()\n");
		return 0;
	}


	// Write IV and check value if first data in value
	if(Offset == 0)
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

		Base_WriteDataTo(IV, DataOffset - EncryptionOverhead, 16);


		// ** Write the check value

		// Encrypt the check value... (Which is "CHUKCHUKCHUKCHUK" who ever said Chuck Harrison has no ego?)
		const Uint8 PlainCheck[16] = { 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B };
		DataChunkPtr CheckData = Encrypt->Encrypt(16, PlainCheck);

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


	// Pad the data if required (i.e. if this is the last chunk of data)
	if(Offset + Size == ValueLength)
	{
		// Infinite recursion should not happen (if all is sensible) - but prevent it to be safe!!
		// In fact the current method of padding could allow infinite recursion with padding bytes = 16
		static bool LoopPrevent = false;
		if(!LoopPrevent)
		{
			LoopPrevent = true;

			// Start by encrypting all but the last 16 bytes (including padding)
			Length StartSize = (EncryptedLength - EncryptionGranularity) - Offset;

			Length Bytes = WriteDataTo(Buffer, Offset, StartSize);

			// Buffer for last data to be encrypted
			Uint8 TempBuffer[EncryptionGranularity];

			// FIXME: The padding bytes are not currently initialized in the correct way...
			memset(TempBuffer, '%', EncryptionGranularity);

			// Copy in the remaining bytes from the end of the given buffer
			memcpy(TempBuffer, &Buffer[StartSize], Size - StartSize);

			// Write out these last 16 bytes (including padding)
			Bytes += WriteDataTo(Buffer, Offset + StartSize, EncryptionGranularity);

			LoopPrevent = false;

			// DRAGONS: Is this the right thing to do?
			//          It will be MORE than Size!!
			return Bytes;
		}
	}

	// FIXME: - we need some way to add end and an AS-DCP footer if required

	// FIXME: We need some way to update the IV for non 0 values of start and ensure we only write/encrypt on 16-byte boundaries!!

	// Encrypt by making a copy
	DataChunkPtr NewData = Encrypt->Encrypt(Size, Buffer);
	if(!NewData) return 0;

	// Write the encrypted data
	Size = Base_WriteDataTo(NewData->Data, DataOffset + Offset, Size);

	// Chain the IV for next time...
	EncryptionIV = Encrypt->GetIV();

	return Size;
}


//! Write the key and length of the current DataChunk to the destination file
/*! The key and length will be written to the source file as set by SetSource.
 *  If LenSize is zero the length will be formatted to match KLSize (if possible!)
 */
Int32 KLVEObject::WriteKL(Int32 LenSize /*=0*/)
{
	// If we don't have an encryption wrapper we are not writing encrypted data, so just write the KL
	if(!Encrypt) return Base_WriteKL(LenSize);

	if(!DestFile)
	{
		error("Call to KLVEObject::WriteKL() with destination file not set\n");
		return 0;
	}

	if(DestOffset < 0)
	{
		error("Call to KLVEObject::WriteKL() with destination file location undefined\n");
		return 0;
	}

	// Seek to the start of the KLV space
	DestFile->Seek(DestOffset);

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

	DataChunkPtr Len = MakeBER(ValueLength, SourceLengthFormat);
	pBuffer += MakeBER(pBuffer, 4, Len->Size, 4);
	memcpy(pBuffer, Len->Data, Len->Size);
	pBuffer += Len->Size;


	// ** Calculate EncryptedLength **
	// There is padding from 1 to 16 bytes at the end of the data

	EncryptedLength = (ValueLength + EncryptionGranularity) / EncryptionGranularity;
	EncryptedLength *= EncryptionGranularity;

	// ** Write the length of the encrypted source value
	// ** Including IV and Check as well as any padding

	pBuffer += MakeBER(pBuffer, 9, EncryptedLength + EncryptionOverhead, 0);

	// Set up the data offset
	// Note that we haven't yet written the IV and check value, but we count those as "header" bytes
	DataOffset = (Uint32)(pBuffer - Buffer) + EncryptionOverhead;


	/** Write Out the Header **/

	// Set the length to be the size of the header plus the size of the encrypted data
	OuterLength = DataOffset + EncryptedLength;

	// Start off with the actual KL - using the OuterLength to include the header
	Uint32 KLBytes = Base_WriteKL(LenSize, OuterLength);

	// Then write the header
	DestFile->Write(Buffer, DataOffset);

//##
//## DRAGONS: We have not done the AS-DCP footer...
//##

	return KLBytes + DataOffset;
}

