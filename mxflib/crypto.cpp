/*! \file	crypto.cpp
 *	\brief	Implementation of classes that hanldle basic encryption and decryption
 *
 *	\version $Id: crypto.cpp,v 1.1.2.1 2004/05/26 18:01:37 matt-beard Exp $
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


//! Construct a KLVEObject linked to an encrypted KLVObject
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


void KLVEObject::Init(void)
{
	DataLoaded = false;
	DataOffset = 0;
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
Uint32 KLVEObject::GetKLSize(void) 
{ 
	if(!DataLoaded) if(!LoadData()) return 0;

	// Return the total overhead
	return KLSize + (ValueLength - SourceLength);
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

	Length Bytes = ReadData(0, 116);

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
		error("Invalid AS-DCP data in %s\n", GetSource().c_str());
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
		error("Invalid AS-DCP data in %s\n", GetSource().c_str());
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
		error("Invalid AS-DCP data in %s\n", GetSource().c_str());
		return false;
	}

	// Build the SourceKey
	SourceKey = new UL(p);

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
		error("Invalid AS-DCP data in %s\n", GetSource().c_str());
		return false;
	}

	// Read the SourceLength
	Prev_p = p;
	SourceLength = ReadBER(&p, Bytes);
	SourceLengthFormat = (p - Prev_p);
	Bytes -= SourceLengthFormat;

	// Update pointer and count
	p += 16;
	Bytes -= 16;

	// Check that we have enough left for the IV and Check Value
	if(Bytes < 16)
	{
		error("Invalid AS-DCP data in %s\n", GetSource().c_str());
		return false;
	}

	// Store the initialization value
	memcpy(IV, p, 16);

	// Store the check value
	memcpy(Check, &p[16], 16);

	// Record offset to the encrypted data
	DataOffset = &p[32] - Data.Data;

	// Check there is enough left for the specified data
	Length BytesLeft = ValueLength - DataOffset;
	Length BytesRequired = (SourceLength + EncryptionGranularity - 1) / EncryptionGranularity;
	BytesRequired *= EncryptionGranularity;

	if(BytesLeft < BytesRequired)
	{
		error("Invalid AS-DCP data in %s\n", GetSource().c_str());
		return false;
	}

	// All seems OK!
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


//! Read data from the KLVObject source into the DataChunk
/*! If Size is zero an attempt will be made to read all available data (which may be billions of bytes!)
 *  This function will attempt to decrypt the data. If the decryption fails no data will be returned.
 *	\return Number of bytes read - zero if none could be read
 *  \note Any previously read data in the current DataChunk will be discarded before reading the new data
 */
Length KLVEObject::ReadData(Position Start /*=0*/, Length Size /*=0*/)
{
	// Don't even bother if we have no decryption wrapper
	if(!Decrypt) return 0;

	// Load the header if required (and if we can!)
	if(!DataLoaded) if(!LoadData()) return 0;

	// Initialize the decryption engine with the specified Initialization Vector
	Decrypt->SetIV(16, IV);

	// Decrypt the check value...
	DataChunkPtr PlainCheck = Decrypt->Decrypt(16, Check);

	// FIXME: Check the check value and barf if wrong...

	// FIXME: We need some way to update the IV and ensure we only read/decrypt on 16-byte boundaries!!

	// Read the encrypted data (and reset size to what we actually read)
	Size = KLVObject::ReadData(DataOffset + Start, Size);

	// See if we can decrypt this in place...
	if(Decrypt->CanDecryptInPlace(Size))
	{
		if(!Decrypt->DecryptInPlace(Data))
		{
			Data.Resize(0);
			return 0;
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

	Data.TakeBuffer(NewData);

	return Size;
}



//! Write data from the current DataChunk to the source file
/*! \note The data in the chunk will be written to the specified position 
 *  <B>regardless of the position from where it was origanally read</b>
 */
Length KLVEObject::WriteData(Uint8 *Buffer, Position Start /*=0*/, Length Size /*=0*/)
{
	// Don't even bother if we have no encryption wrapper
	if(!Encrypt) return 0;

	if(DataOffset == 0) 
	{
		error("KLVEObject::WriteData called before KLVEObject::WriteKL()\n");
		return 0;
	}

	// Initialize the encryption engine with a new Initialization Vector
	// If one has been given to use we use that - otherwise we create a new one
	if(EncryptionIV)
	{
		// Write the IV
		KLVObject::WriteData(EncryptionIV, DataOffset - 32);

		Encrypt->SetIV(EncryptionIV, true);
		EncryptionIV = NULL;						// Only use a give IV once
	}
	else
	{
		// TODO: Add decent random number generator here... this one is from the system.h UUID gen
		srand((time(NULL)) ^ ((int)(Data.Data)) ^ (clock() << 2) ^ rand());

		Uint8 EncIV[16];

		int i;
		for(i=0; i< 16; i++) { EncIV[i] = (Uint8)rand(); };

		Encrypt->SetIV(16, EncIV, true);

		// Write the IV
		KLVObject::WriteData(EncIV, DataOffset - 32, 16);
	}


	// Encrypt the check value... (Which is "CHUKCHUKCHUKCHUK" who ever said Chuck Harrison has no ego?)
	const Uint8 PlainCheck[16] = { 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B, 0x43, 0x48, 0x55, 0x4B };
	DataChunkPtr CheckData = Encrypt->Encrypt(16, Check);

	// Write the encrypted check value
	KLVObject::WriteData(CheckData, DataOffset - 16);

	// FIXME: We need some way to update the IV for non 0 values of start and ensure we only write/encrypt on 16-byte boundaries!!

	// Encrypt by making a copy
	DataChunkPtr NewData = Encrypt->Encrypt(Size, Buffer);
	if(!NewData) return 0;

	// Write the encrypted data
	Size = KLVObject::WriteData(NewData->Data, DataOffset + Start, Size);

	// Chain the IV for next time...
	EncryptionIV = Encrypt->GetIV();

	return Size;
}

