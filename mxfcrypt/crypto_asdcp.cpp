/*! \file	crypto_asdcp.cpp
 *	\brief	AS-DCP compatible encryption and decryption
 *
 *	\version $Id: crypto_asdcp.cpp,v 1.8 2006/09/04 16:18:21 matt-beard Exp $
 *
 */
/*
 *  Copyright (c) 2004, Matt Beard
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

#include "crypto_asdcp.h"

using namespace mxflib;

using namespace std;

//! True if we are doing hashing calculations
bool Hashing = false;

//! Flag set if forcing a given key irrespective of the key details in the file
bool ForceKeyMode = false;


//! Build an AS-DCP hashing key from a given crypto key
/*  The hashing key is: 
 *  - trunc( HMAC-SHA-1( CipherKey, 0x00112233445566778899aabbccddeeff ) )
 *  Where trunc(x) is the first 128 bits of x
 */
DataChunkPtr BuildHashKey(size_t Size, const UInt8 *CryptoKey)
{
	//! Constant value to be hashed with cypher key to produce the hashing key
	const UInt8 KeyConst[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };

	HashPtr Hasher = new HashHMACSHA1();

	// Hash the constant data with the crypto key
	Hasher->SetKey(Size, CryptoKey);
	Hasher->HashData(16, KeyConst);
	DataChunkPtr Ret = Hasher->GetHash();

	// Truncate the hashed key to 128-bits (16-bytes)
	Ret->Resize(16);

	return Ret;
}


//! Set the key and start hashing
/*  \return True if key is accepted
 */
bool HashHMACSHA1::SetKey(size_t Size, const UInt8 *Key)
{
	if(Size > 64)
	{
		error("Key size > 64 bytes not supported by HashHMACSHA1\n");
		return false;
	}

	// Clear the key buffers
	memset(KeyBuffer_i, 0, 64);
	memset(KeyBuffer_o, 0, 64);

	// Copy the hash key to the key buffers
	memcpy(KeyBuffer_i, Key, Size);
	memcpy(KeyBuffer_o, Key, Size);

	// Exclusive-or the keys with the required constants
	int i;
	for(i=0; i<64; i++)
	{
		KeyBuffer_i[i] ^= 0x36;
		KeyBuffer_o[i] ^= 0x5c;
	}

	// Initialize the SHA-1 algorythm and inject the inner key
	SHA1_Init(&Context);
	SHA1_Update(&Context, KeyBuffer_i, 64);

	KeyInited = true;

/*
int n = 1;
char Name[128];
sprintf(Name, "Hash%03d.dat", n);
while(FileExists(Name)) 
{
n++;
sprintf(Name, "Hash%03d.dat", n);
}
OutFile = FileOpenNew(Name);
printf("Writing hash data to \"%s\"\n", Name);
*/
	return true;
};


//! Add the given data to the current hash being calculated
void HashHMACSHA1::HashData(size_t Size, const UInt8 *Data)
{
//FileWrite(OutFile, Data, Size);
	if(!KeyInited)
	{
		error("HashHMACSHA1::HashData() called without setting the key\n");
		return;
	}

	if((sizeof(size_t) > 4) &&(Size > 0xffffffff))
	{
		error("Maximum hashable chunk size in HashHMACSHA1::HashData() is 4GBytes\n");
		return;
	}

	SHA1_Update(&Context, Data, static_cast<unsigned long>(Size));
}


//! Get the finished hash value
DataChunkPtr HashHMACSHA1::GetHash(void)
{
//FileClose(OutFile);
	// Build a data chunk for the output
	DataChunkPtr Ret = new DataChunk;
	Ret->Resize(20);

	SHA1_Final(Ret->Data, &Context);

	// Hash the inner hash with the outer key
	SHA1_Init(&Context);
	SHA1_Update(&Context, KeyBuffer_o, 64);
	SHA1_Update(&Context, Ret->Data, static_cast<unsigned long>(Ret->Size));
	SHA1_Final(Ret->Data, &Context);

/*
printf("Hash is:");
for(int i=0; i<20; i++)
{
  printf(" %02x", Ret->Data[i]);
}
printf("\n");
*/
	return Ret;
}



//! Encrypt data and return in a new buffer
/*! \return NULL pointer if the encryption is unsuccessful
 */
DataChunkPtr AESEncrypt::Encrypt(size_t Size, const UInt8 *Data)
{
	// Calculate size of encrypted data (always a multiple of 16-bytes)
	size_t RetSize = (Size + 15) / 16;
	RetSize *= 16;

	DataChunkPtr Ret = new DataChunk(RetSize);

	AES_cbc_encrypt(Data, Ret->Data, static_cast<unsigned long>(Size), &CurrentKey, CurrentIV, AES_ENCRYPT);

	return Ret;
}


//! Construct a handler for a specified BodySID
Encrypt_GCReadHandler::Encrypt_GCReadHandler(GCWriterPtr Writer, UInt32 BodySID, UUIDPtr &ContextID, DataChunkPtr KeyID, std::string KeyFileName) 
  : OurSID(BodySID), Writer(Writer), ContextID(ContextID), PlaintextOffset(0)
{
	char Buffer[45];

	FileHandle KeyFile = FileOpenRead(KeyFileName.c_str());
	size_t Bytes = 0;
	if(FileValid(KeyFile))
	{
		Bytes = FileRead(KeyFile, (UInt8*)Buffer, 32);
		FileClose(KeyFile);

		if(Bytes != 32) error("Failed to read key from key-file \"%s\"\n", KeyFileName.c_str());
	}
	else if((!ForceKeyMode) && (KeyID) && (KeyID->Size == 16))
	{
		{
			char NameBuffer[45];

			unsigned char *p = KeyID->Data;
			sprintf(NameBuffer, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
							p[0], p[1], p[2], p[3],     p[4], p[5],      p[6], p[7],     p[8], p[9],
							p[10], p[11], p[12], p[13], p[14], p[15] );

			std::string UseName = SearchPath(KeyFileName.c_str(), NameBuffer);

			if(UseName.size())
			{
				FileHandle KeyFile = FileOpenRead(UseName.c_str());
				if(FileValid(KeyFile))
				{
					Bytes = (int)FileRead(KeyFile, (UInt8*)Buffer, 32);
					FileClose(KeyFile);
				}
			}

			if(Bytes != 32)
			{
				// Need to allow string to live long enough to use...
				char *KFN = new char[KeyFileName.size() + 1];
				strcpy(KFN, KeyFileName.c_str());
				error("Failed to read key-file \"%s\" or key-file \"%s\" in directory \"%s\"\n", 
					              KFN, NameBuffer, KFN );
				delete[] KFN;
			}
		}
	}

	if(Bytes == 32)
	{
		UInt8 KeyBuff[16];
		int Count = 0;
		while(Bytes)
		{
			int Val;
			if(sscanf(&Buffer[Count*2], "%02x", &Val) == 1)
			{
				KeyBuff[Count] = static_cast<UInt8>(Val);
				Count++;
				Bytes -= 2;
			}
			else break;
		}

		if(Count == 16)
		{
			EncKey.Set(16, KeyBuff);
			printf("Key Set OK\n");
		}
	}

	IndexPos = 0;
}


//! Handle a "chunk" of data that has been read from the file
/*! \return true if all OK, false on error 
 */
bool Encrypt_GCReadHandler::HandleData(GCReaderPtr Caller, KLVObjectPtr Object)
{
//	printf("0x%08x -> %02x:0x%08x Data for Track 0x%08x, ", (int)Object->GetLocation(), OurSID, (int)Caller->GetStreamOffset(), Object->GetGCTrackNumber());
//	printf("Size = 0x%08x\n", (int)Object->GetLength());

	// Create an encrypted vertion of this KLVObject
	KLVEObjectPtr KLVE = new KLVEObject(Object);

	// Set an encryption wrapper
	Encrypt_Base *Enc = new AESEncrypt;
	KLVE->SetEncrypt(Enc);
	KLVE->SetPlaintextOffset(PlaintextOffset);
	KLVE->SetContextID(ContextID);

	// Set the encryption key
	Enc->SetKey(EncKey);

	// If we are hashing build add a new hasher
	if(Hashing)
	{
		HashPtr Hasher = new HashHMACSHA1();
		KLVE->SetWriteHasher(Hasher);
		DataChunkPtr HashKey = BuildHashKey(EncKey);
		Hasher->SetKey(HashKey);
	}


	// Set an encryption IV
	// DRAGONS: The current draft AS-DCP specification requires this to be an encryption strength random number generator.
	//          However as the IV is always sent in plaintext there is no advantage doing this.
	//          In fact it is actually more secure to use sequential IVs starting at some moderately random value
	// TODO: Make IVs sequential
	UInt8 IV[16];
	int i; for(i=0; i<16; i++) IV[i] = (UInt8) rand();
	KLVE->SetEncryptIV(16, IV, true);

	// Update the index table to the new position
	if(Index)
	{
		Index->Update(IndexPos, (UInt64)Writer->GetStreamOffset());
	}

	// Write the encrypted data
	Writer->WriteRaw(SmartPtr_Cast(KLVE, KLVObject));

	// Update the index position count (even if not yet indexing)
	IndexPos++;

	return true;
}


//! Decrypt data and return in a new buffer
/*! \return NULL pointer if the encryption is unsuccessful
 */
DataChunkPtr AESDecrypt::Decrypt(size_t Size, const UInt8 *Data)
{
	if((sizeof(size_t) > 4) &&(Size > 0xffffffff))
	{
		error("Maximum chunk size in AESDecrypt::Decrypt() is 4GBytes\n");
		return NULL;
	}

	DataChunkPtr Ret = new DataChunk(Size);

	AES_cbc_encrypt(Data, Ret->Data, static_cast<unsigned long>(Size), &CurrentKey, CurrentIV, AES_DECRYPT);

	return Ret;
}


//! Construct a handler for a specified BodySID
Decrypt_GCEncryptionHandler::Decrypt_GCEncryptionHandler(UInt32 BodySID, DataChunkPtr KeyID, std::string KeyFileName) : OurSID(BodySID) 
{
	char Buffer[45];

	FileHandle KeyFile = FileOpenRead(KeyFileName.c_str());
	size_t Bytes = 0;
	if(FileValid(KeyFile))
	{
		Bytes = FileRead(KeyFile, (UInt8*)Buffer, 32);
		FileClose(KeyFile);

		if(Bytes != 32) error("Failed to read key from key-file \"%s\"\n", KeyFileName.c_str());
	}
	else if((!ForceKeyMode) && (KeyID) && (KeyID->Size == 16))
	{
		{
			char NameBuffer[45];

			unsigned char *p = KeyID->Data;
			sprintf(NameBuffer, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
							p[0], p[1], p[2], p[3],     p[4], p[5],      p[6], p[7],     p[8], p[9],
							p[10], p[11], p[12], p[13], p[14], p[15] );

			std::string UseName = SearchPath(KeyFileName.c_str(), NameBuffer);

			if(UseName.size())
			{
				FileHandle KeyFile = FileOpenRead(UseName.c_str());
				if(FileValid(KeyFile))
				{
					Bytes = (int)FileRead(KeyFile, (UInt8*)Buffer, 32);
					FileClose(KeyFile);
				}
			}

			if(Bytes != 32)
			{
				// Need to allow string to live long enough to use...
				char *KFN = new char[KeyFileName.size() + 1];
				strcpy(KFN, KeyFileName.c_str());
				error("Failed to read key-file \"%s\" or key-file \"%s\" in directory \"%s\"\n", 
					              KFN, NameBuffer, KFN );
				delete[] KFN;
			}
		}
	}

	if(Bytes == 32)
	{
		UInt8 KeyBuff[16];
		int Count = 0;
		while(Bytes)
		{
			int Val;
			if(sscanf(&Buffer[Count*2], "%02x", &Val) == 1)
			{
				KeyBuff[Count] = static_cast<UInt8>(Val);
				Count++;
				Bytes -= 2;
			}
			else break;
		}

		if(Count == 16)
		{
			DecKey.Set(16, KeyBuff);
			printf("Key Set OK\n");
		}
	}
}


//! Handle a "chunk" of data that has been read from the file
/*! \return true if all OK, false on error 
 */
bool Decrypt_GCEncryptionHandler::HandleData(GCReaderPtr Caller, KLVObjectPtr Object)
{
//	printf("0x%08x -> %02x:0x%08x Encrypted data, ", (int)Object->GetLocation(), OurSID, (int)Caller->GetStreamOffset());
//	printf("Size = 0x%08x\n", (int)Object->GetLength());

	KLVEObjectPtr KLVE = new KLVEObject(Object);

	// Set a decryption wrapper
	Decrypt_Base *Dec = new AESDecrypt;
	KLVE->SetDecrypt(Dec);

	// Set the decryption key
	Dec->SetKey(DecKey);

	// If we are hashing build add a new hasher
	if(Hashing)
	{
		HashPtr Hasher = new HashHMACSHA1();
		KLVE->SetReadHasher(Hasher);
		DataChunkPtr HashKey = BuildHashKey(DecKey);
		Hasher->SetKey(HashKey);
	}

	// Pass decryption wrapped data back for handling
	Caller->HandleData(SmartPtr_Cast(KLVE, KLVObject));

	return true;
}


