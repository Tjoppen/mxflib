/*! \file	mxfcrypt.cpp
 *	\brief	MXF e/decrypt utility for MXFLib
 *
 *	\version $Id: mxfcrypt.cpp,v 1.1.2.3 2004/07/05 17:53:12 matt-beard Exp $
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

#include <mxflib/mxflib.h>

using namespace mxflib;

#include <stdio.h>
#include <stdlib.h>

// Include AES encryption from OpenSSL
#include "aes.h"


using namespace std;


// base library version
const char* BaseVersion = "based on mxflib 0.4-Alpha";

// Product GUID and version text for this release
Uint8 ProductGUID_Data[16] = { 0x84, 0x63, 0x41, 0xf2, 0x37, 0xdd, 0xde, 0x40, 0x86, 0xdc, 0xe0, 0x99, 0xda, 0x7f, 0xd0, 0x53 };
string CompanyName = "freeMXF.org";
string ProductName = "mxfcrypt file de/encrypt utility";
string ProductVersion = BaseVersion;

// ============================================================================
//! Basic encryption class - currently does AES encryption
// ============================================================================
class BasicEncrypt : public Encrypt_Base
{
protected:
	AES_KEY CurrentKey;
	Uint8 CurrentIV[16];

public:
	//! Set an encryption key
	/*! \return True if key is accepted
	 */
	bool SetKey(Uint32 KeySize, const Uint8 *Key) 
	{
		int Ret = AES_set_encrypt_key(Key, 128, &CurrentKey);

		// Return true only if key setting was OK
		return Ret ? false : true; 
	};

	//! Set an encryption Initialization Vector
	/*! \return False if Initialization Vector is rejected
	 *  \note Some crypto schemes, such as cypher block chaining, only require
	 *        the initialization vector to be set at the start - in these cases
	 *        Force will be set to true when the vector needs to be initialized,
	 *        and false for any other calls.  This allows different schemes to be
	 *        used with minimal changes in the calling code.
	 */
	bool SetIV(Uint32 IVSize, const Uint8 *IV, bool Force = false) 
	{ 
		if(!Force) return false;

		if(IVSize != 16)
		{
			error("IV for AES encryption must by 16 bytes, tried to use IV of size %d\n", IVSize);
			return false;
		}

		memcpy(CurrentIV, IV, 16);

		return true; 
	};

	//! Get the Initialization Vector that will be used for the next encryption
	/*! If called immediately after SetIV() with Force=true or SetIV() for a crypto
	 *  scheme that accepts each offered vector (rather than creating its own ones)
	 *  the result will be the vector offered in that SetIV() call.
	 */
	DataChunkPtr GetIV(void) 
	{
		return new DataChunk(16, CurrentIV);
	}

	//! Can this encryption system safely encrypt in place?
	/*! If BlockSize is 0 this function will return true if encryption of all block sizes can be "in place".
	 *  Otherwise the result will indicate whether the given blocksize can be encrypted "in place".
	 */
	bool CanEncryptInPlace(Uint32 BlockSize = 0) { return false; }

	//! Encrypt data bytes in place
	/*! \return true if the encryption is successful
	 */
	bool EncryptInPlace(Uint32 Size, Uint8 *Data) { return false; }

	//! Encrypt data and return in a new buffer
	/*! \return NULL pointer if the encryption is unsuccessful
	 */
	DataChunkPtr Encrypt(Uint32 Size, const Uint8 *Data);
};


//! Encrypt data and return in a new buffer
/*! \return NULL pointer if the encryption is unsuccessful
 */
DataChunkPtr BasicEncrypt::Encrypt(Uint32 Size, const Uint8 *Data)
{
	// Calculate size of encrypted data (always a multiple of 16-bytes)
	Uint32 RetSize = (Size + 15) / 16;
	RetSize *= 16;

	DataChunkPtr Ret = new DataChunk(RetSize);

	AES_cbc_encrypt(Data, Ret->Data, Size, &CurrentKey, CurrentIV, AES_ENCRYPT);

	return Ret;
}


// ============================================================================
//! Encrypting GCReader handler
// ============================================================================
class Encrypt_GCReadHandler : public GCReadHandler_Base
{
protected:
	Uint32 OurSID;								//!< The BodySID of this essence
	GCWriterPtr Writer;							//!< GCWriter to receive encrypted data

private:
	Encrypt_GCReadHandler();						//!< Don't allow standard construction

public:
	//! Construct a test handler for a specified BodySID
	Encrypt_GCReadHandler(GCWriterPtr Writer, Uint32 BodySID) : Writer(Writer), OurSID(BodySID) {};

	//! Handle a "chunk" of data that has been read from the file
	/*! \return true if all OK, false on error 
	 */
	virtual bool HandleData(GCReaderPtr Caller, KLVObjectPtr Object);
};

bool Encrypt_GCReadHandler::HandleData(GCReaderPtr Caller, KLVObjectPtr Object)
{
//	printf("0x%08x -> %02x:0x%08x Data for Track 0x%08x, ", (int)Object->GetLocation(), OurSID, (int)Caller->GetStreamOffset(), Object->GetGCTrackNumber());
//	printf("Size = 0x%08x\n", (int)Object->GetLength());

	// Create an encrypted vertion of this KLVObject
	KLVEObjectPtr KLVE = new KLVEObject(Object);

	// Set an encryption wrapper
	BasicEncrypt *Enc = new BasicEncrypt;
	KLVE->SetEncrypt(Enc);

	// ##@@##@@
	// ##@@##@@ Set a dummy encryption key...
	// ##@@##@@

	char *Key = "This is a dummy encryption key to use with the AES encryption system";
	Enc->SetKey( strlen(Key), (Uint8*)Key);


	// Set a basic IV
	const Uint8 IV[16] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
	KLVE->SetEncryptIV(16, IV, true);

	// Write the encrypted data
	Writer->WriteRaw(SmartPtr_Cast(KLVE, KLVObject));

	return true;
}
// ============================================================================


// ============================================================================
//! Encrypting GCReader handler for filler
// ============================================================================
class Test_GCFillerHandler : public GCReadHandler_Base
{
protected:
	Uint32 OurSID;								//!< The BodySID of this essence
	GCWriterPtr Writer;							//!< GCWriter to receive encrypted data

private:
	Test_GCFillerHandler();						//!< Don't allow standard construction

public:
	//! Construct a filler handler for a specified BodySID
	Test_GCFillerHandler(GCWriterPtr Writer, Uint32 BodySID) : Writer(Writer), OurSID(BodySID) {};

	//! Handle a "chunk" of data that has been read from the file
	/*! \return true if all OK, false on error 
	 */
	virtual bool HandleData(GCReaderPtr Caller, KLVObjectPtr Object);
};

bool Test_GCFillerHandler::HandleData(GCReaderPtr Caller, KLVObjectPtr Object)
{
//	printf("0x%08x -> %02x:0x%08x Filler ", (int)Object->GetLocation(), OurSID, (int)Caller->GetStreamOffset());
//	printf("Size = 0x%08x\n", (int)Object->GetLength());

	return true;
}
// ============================================================================



// ============================================================================
//! Basic decryption class - currently does AES decryption
// ============================================================================
class BasicDecrypt : public Decrypt_Base
{
protected:
	AES_KEY CurrentKey;
	Uint8 CurrentIV[16];

public:
	//! Set an encryption key
	/*! \return True if key is accepted
	 */
	bool SetKey(Uint32 KeySize, const Uint8 *Key) 
	{
		int Ret = AES_set_decrypt_key(Key, 128, &CurrentKey);

		// Return true only if key setting was OK
		return Ret ? false : true; 
	};

	//! Set a decryption Initialization Vector
	/*! \return False if Initialization Vector is rejected
	 *  \note Some crypto schemes, such as cypher block chaining, only require
	 *        the initialization vector to be set at the start - in these cases
	 *        Force will be set to true when the vector needs to be initialized,
	 *        and false for any other calls.  This allows different schemes to be
	 *        used with minimal changes in the calling code.
	 */
	bool SetIV(Uint32 IVSize, const Uint8 *IV, bool Force = false)
	{ 
		if(!Force) return false;

		if(IVSize != 16)
		{
			error("IV for AES encryption must by 16 bytes, tried to use IV of size %d\n", IVSize);
			return false;
		}

		memcpy(CurrentIV, IV, 16);

		return true; 
	};

	//! Get the Initialization Vector that will be used for the next decryption
	/*! If called immediately after SetIV() with Force=true or SetIV() for a crypto
	 *  scheme that accepts each offered vector (rather than creating its own ones)
	 *  the result will be the vector offered in that SetIV() call.
	 */
	DataChunkPtr GetIV(void)
	{
		return new DataChunk(16, CurrentIV);
	}

	//! Can this decryption system safely decrypt in place?
	/*! If BlockSize is 0 this function will return true if decryption of all block sizes can be "in place".
	 *  Otherwise the result will indicate whether the given blocksize can be decrypted "in place".
	 */
	bool CanDecryptInPlace(Uint32 BlockSize = 0) { return false; }

	//! Decrypt data bytes in place
	/*! \return true if the decryption <i>appears to be</i> successful
	 */
	bool DecryptInPlace(Uint32 Size, Uint8 *Data) { return false; }

	//! Decrypt data and return in a new buffer
	/*! \return true if the decryption <i>appears to be</i> successful
	 */
	DataChunkPtr Decrypt(Uint32 Size, const Uint8 *Data);
};



//! Decrypt data and return in a new buffer
/*! \return NULL pointer if the encryption is unsuccessful
 */
DataChunkPtr BasicDecrypt::Decrypt(Uint32 Size, const Uint8 *Data)
{
	DataChunkPtr Ret = new DataChunk(Size);

	AES_cbc_encrypt(Data, Ret->Data, Size, &CurrentKey, CurrentIV, AES_DECRYPT);

	return Ret;
}


// ============================================================================
//! Decrypting GCReader encryption handler
// ============================================================================
class Decrypt_GCEncryptionHandler : public GCReadHandler_Base
{
protected:
	Uint32 OurSID;										//!< The BodySID of this essence

private:
	Decrypt_GCEncryptionHandler();						//!< Don't allow standard construction

public:
	//! Construct a test handler for a specified BodySID
	Decrypt_GCEncryptionHandler(Uint32 BodySID) : OurSID(BodySID) {};

	//! Handle a "chunk" of data that has been read from the file
	/*! \return true if all OK, false on error 
	 */
	virtual bool HandleData(GCReaderPtr Caller, KLVObjectPtr Object);
};

bool Decrypt_GCEncryptionHandler::HandleData(GCReaderPtr Caller, KLVObjectPtr Object)
{
//	printf("0x%08x -> %02x:0x%08x Encrypted data, ", (int)Object->GetLocation(), OurSID, (int)Caller->GetStreamOffset());
//	printf("Size = 0x%08x\n", (int)Object->GetLength());

	KLVEObjectPtr KLVE = new KLVEObject(Object);

	// Set a decryption wrapper
	BasicDecrypt *Dec = new BasicDecrypt;
	KLVE->SetDecrypt(Dec);

	// ##@@##@@
	// ##@@##@@ Set a dummy encryption key...
	// ##@@##@@

	char *Key = "This is a dummy encryption key to use with the AES encryption system";
	Dec->SetKey( strlen(Key), (Uint8*)Key);

//	// Set a basic IV
//	const Uint8 IV[16] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
//	KLVE->SetEncryptIV(16, IV, true);

	// Pass decryption wrapped data back for handling
	Caller->HandleData(SmartPtr_Cast(KLVE, KLVObject));

	return true;
}


// ============================================================================
//! Decrypting GCReader handler
/*! Passes data straight through to the output file - either decrypted by 
 *  Decrypt_GCEcryptionHandler or not encrypted in the source file
 */
// ============================================================================
class Decrypt_GCReadHandler : public GCReadHandler_Base
{
protected:
	Uint32 OurSID;								//!< The BodySID of this essence
	GCWriterPtr Writer;							//!< GCWriter to receive dencrypted data

private:
	Decrypt_GCReadHandler();						//!< Don't allow standard construction

public:
	//! Construct a test handler for a specified BodySID
	Decrypt_GCReadHandler(GCWriterPtr Writer, Uint32 BodySID) : Writer(Writer), OurSID(BodySID) {};

	//! Handle a "chunk" of data that has been read from the file
	/*! \return true if all OK, false on error 
	 */
	virtual bool HandleData(GCReaderPtr Caller, KLVObjectPtr Object);
};

bool Decrypt_GCReadHandler::HandleData(GCReaderPtr Caller, KLVObjectPtr Object)
{
//	printf("0x%08x -> %02x:0x%08x Data for Track 0x%08x, ", (int)Object->GetLocation(), OurSID, (int)Caller->GetStreamOffset(), Object->GetGCTrackNumber());
//	printf("Size = 0x%08x\n", (int)Object->GetLength());

	// Write the data without further processing
	Writer->WriteRaw(Object);

	return true;
}







//! MXFLib debug flag
bool DebugMode = false;

//! Flag for decrypt rather than encrypt
bool DecryptMode = false;


int main(int argc, char *argv[])
{
	printf("MXF en/decrypt utility\n");

	int num_options = 0;
	for(int i=1; i<argc; i++)
	{
		if(argv[i][0] == '-')
		{
			num_options++;
			if((argv[i][1] == 'v') || (argv[i][1] == 'V'))
				DebugMode = true;
			if((argv[i][1] == 'd') || (argv[i][1] == 'D'))
				DecryptMode = true;
		}
	}

	LoadTypes("types.xml");
	MDOType::LoadDict("xmldict.xml");

	if (argc - num_options < 3)
	{
		printf( "\nUsage:  %s [-d] <in-filename> <out-filename>\n\n", argv[0] );

		return 1;
	}


	MXFFilePtr InFile = new MXFFile;
	if(!InFile->Open(argv[num_options+1], true))
	{
		error("Can't open input file\n");
		return 1;
	}

	// Read the header partition pack
	PartitionPtr HeaderPartition = InFile->ReadPartition();
	if(!HeaderPartition)
	{
		error("Could not read the Header!\n");
		InFile->Close();
		return 1;
	}

	// Warn if the header is not closed
	// TODO: Should really nip to the footer
	if(HeaderPartition->Name().find("Closed") != std::string::npos)
	{
		warning("Header is Open - Some essence may not be processed\n");
	}

	// Read the metadata from the header
	Length Bytes = HeaderPartition->ReadMetadata();

/**
	if(!HMeta)
	{
		error("Could not load the Header Metadata!\n");
		InFile->Close();
		return 1;
	}

	MetadataPtr HMeta = ...

	// Identify the primary package
	PackagePtr PrimaryPackage = Metadata->GetPrimaryPackage();

	if(!PrimaryPackage)
	{
		error("Could not locate the primary package\n");
		InFile->Close();
		return 1;
	}

	// Determine if the primary package is a file package
	bool PrimaryIsFile = false;
	if(PrimayPackage->GetName() != "MaterialPackage") PrimaryIsFile = true;

	// Iterate through tracks on the primary package
	MDObject::iterator it = PrimaryPackage->start();

***/
	
	MXFFilePtr OutFile = new MXFFile;
	if(!OutFile->OpenNew(argv[num_options+2]))
	{
		error("Can't open output file\n");
		InFile->Close();
		return 1;
	}


/*
	// ### This is where we should update the metadata...
	// ### At the moment we just identify ourselves

	// Build an Ident set describing us and link into the metadata
	MDObjectPtr Ident = new MDObject("Identification");
	Ident->SetString("CompanyName", CompanyName);
	Ident->SetString("ProductName", ProductName);
	Ident->SetString("VersionString", ProductVersion);
	UUIDPtr ProductUID = new mxflib::UUID(ProductGUID_Data);

	// DRAGONS: -- Need to set a proper GUID per released version
	//             Non-released versions currently use a random GUID
	//			   as they are not a stable version...
	Ident->SetValue("ProductUID", DataChunk(16,ProductUID->GetValue()));

	// Link the new Ident set with all new metadata
	// Note that this is done even for OP-Atom as the 'dummy' header written first
	// could have been read by another device. This flags that items have changed.
	MData->UpdateGenerations(Ident);
*/

	// Write the header partition
	OutFile->WritePartition(HeaderPartition);


	// ###
	// ### Not the right way to scan a file for BodySIDs (should check metadata)
	// ###

	// Start at the beginning of the file
	InFile->Seek(0);

	BodyReaderPtr BodyParser = new BodyReader(InFile);

	GCWriterPtr Writer = new GCWriter(OutFile);

	// Loop until all is done...
	for(;;)
	{
		if(!BodyParser->IsAtPartition())
		{
			BodyParser->ReSync();
		}

		// Move the main file pointer to the current body partition pack
		InFile->Seek(BodyParser->Tell());

		// Read the partition pack
		PartitionPtr CurrentPartition = InFile->ReadPartition();
		if(!CurrentPartition) break;

		// Find out what BodySID
		Uint32 BodySID = CurrentPartition->GetUint("BodySID");

		printf("%s at %s BodySID 0x%04x:\n", CurrentPartition->Name().c_str(), CurrentPartition->GetSourceLocation().c_str(), BodySID); 

		// Partition contains essence - see if we need to set up a new set of handlers
		if(BodySID != 0)
		{
			if(!BodyParser->GetGCReader(BodySID))
			{
				if(DecryptMode)
				{
					GCReadHandlerPtr Handler = new Decrypt_GCReadHandler(Writer, BodySID);
					GCReadHandlerPtr FillerHandler = new Test_GCFillerHandler(Writer, BodySID);
					GCReadHandlerPtr EncHandler = new Decrypt_GCEncryptionHandler(BodySID);
					BodyParser->MakeGCReader(BodySID, Handler, FillerHandler);
					GCReaderPtr Reader = BodyParser->GetGCReader(BodySID);
					if(Reader) Reader->SetEncryptionHandler(EncHandler);
				}
				else
				{
					GCReadHandlerPtr Handler = new Encrypt_GCReadHandler(Writer, BodySID);
					GCReadHandlerPtr FillerHandler = new Test_GCFillerHandler(Writer, BodySID);
					BodyParser->MakeGCReader(BodySID, Handler, FillerHandler);
				}
			}
		}

		// Parse the file until next partition or an error
		if (!BodyParser->ReadFromFile()) break;
	}

	InFile->Close();

//printf("Outfile at 0x%08x\n", (int)OutFile->Tell());
//char *x = "<<END OF FILE>>";
//OutFile->Write((Uint8*)x, 16);
	OutFile->Close();
	
	return 0;
}



// Debug and error messages
#include <stdarg.h>

#ifdef MXFLIB_DEBUG
//! Display a general debug message
void mxflib::debug(const char *Fmt, ...)
{
	if(!DebugMode) return;

	va_list args;

	va_start(args, Fmt);
	vprintf(Fmt, args);
	va_end(args);
}
#endif // MXFLIB_DEBUG

//! Display a warning message
void mxflib::warning(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("Warning: ");
	vprintf(Fmt, args);
	va_end(args);
}

//! Display an error message
void mxflib::error(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("ERROR: ");
	vprintf(Fmt, args);
	va_end(args);
}

