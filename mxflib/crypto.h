/*! \file	crypto.h
 *	\brief	Definition of classes that wrap encryption and decryption tools
 *
 *	\version $Id: crypto.h,v 1.1.2.5 2004/06/14 17:06:53 matt-beard Exp $
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
#ifndef MXFLIB__CRYPTO_H
#define MXFLIB__CRYPTO_H


#include <map>
#include <list>


// Forward refs
namespace mxflib
{
}


namespace mxflib
{
	//! Base encryptor wrapper class
	/*! \note Classes derived from this class <b>must not</b> include their own RefCount<> derivation
	 */
	class Encrypt_Base : public RefCount<Encrypt_Base>
	{
	public:
		Encrypt_Base() {};
		virtual ~Encrypt_Base() {};

		//! Set an encryption key
		/*! \return True if key is accepted
		 */
		virtual bool SetKey(Uint32 KeySize, const Uint8 *Key) = 0;

		//! Set an encryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 *  \note Some crypto schemes, such as cypher block chaining, only require
		 *        the initialization vector to be set at the start - in these cases
		 *        Force will be set to true when the vector needs to be initialized,
		 *        and false for any other calls.  This allows different schemes to be
		 *        used with minimal changes in the calling code.
		 */
		virtual bool SetIV(Uint32 IVSize, const Uint8 *IV, bool Force = false) = 0;

		//! Set an encryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 *  \note Some crypto schemes, such as cypher block chaining, only require
		 *        the initialization vector to be set at the start - in these cases
		 *        Force will be set to true when the vector needs to be initialized,
		 *        and false for any other calls.  This allows different schemes to be
		 *        used with minimal changes in the calling code.
		 */
		bool SetIV(DataChunk &IV, bool Force = false) { return SetIV(IV.Size, IV.Data, Force); }

		//! Set an encryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 *  \note Some crypto schemes, such as cypher block chaining, only require
		 *        the initialization vector to be set at the start - in these cases
		 *        Force will be set to true when the vector needs to be initialized,
		 *        and false for any other calls.  This allows different schemes to be
		 *        used with minimal changes in the calling code.
		 */
		bool SetIV(DataChunkPtr &IV, bool Force = false) { return SetIV(IV->Size, IV->Data, Force); }

		//! Get the Initialization Vector that will be used for the next encryption
		/*! If called immediately after SetIV() with Force=true or SetIV() for a crypto
		 *  scheme that accepts each offered vector (rather than creating its own ones)
		 *  the result will be the vector offered in that SetIV() call.
		 */
		virtual DataChunkPtr GetIV(void) = 0;

		//! Can this encryption system safely encrypt in place?
		/*! If BlockSize is 0 this function will return true if encryption of all block sizes can be "in place".
		 *  Otherwise the result will indicate whether the given blocksize can be encrypted "in place".
		 */
		virtual bool CanEncryptInPlace(Uint32 BlockSize = 0) = 0;

		//! Encrypt data bytes in place
		/*! \return true if the encryption is successful
		 */
		virtual bool EncryptInPlace(Uint32 Size, const Uint8 *Data) = 0;

		//! Encrypt data bytes in place
		/*! \return true if the encryption is successful
		 */
		bool EncryptInPlace(DataChunk &Data) { return EncryptInPlace(Data.Size, Data.Data); };

		//! Encrypt data bytes in place
		/*! \return true if the encryption is successful
		 */
		bool EncryptInPlace(DataChunkPtr &Data) { return EncryptInPlace(Data->Size, Data->Data); };

		//! Encrypt data and return in a new buffer
		/*! \return NULL pointer if the encryption is unsuccessful
		 */
		virtual DataChunkPtr Encrypt(Uint32 Size, const Uint8 *Data) = 0;

		//! Encrypt data and return in a new buffer
		/*! \return NULL pointer if the encryption is unsuccessful
		 */
		DataChunkPtr Encrypt(DataChunk &Data) { return Encrypt(Data.Size, Data.Data); };

		//! Encrypt data and return in a new buffer
		/*! \return NULL pointer if the encryption is unsuccessful
		 */
		DataChunkPtr Encrypt(DataChunkPtr &Data) { return Encrypt(Data->Size, Data->Data); };
	};

	// Smart pointer to an encryption wrapper object
	typedef SmartPtr<Encrypt_Base> EncryptPtr;


	//! Base decryptor wrapper class
	/*! \note Classes derived from this class <b>must not</b> include their own RefCount<> derivation
	 */
	class Decrypt_Base : public RefCount<Decrypt_Base>
	{
	public:
		Decrypt_Base() {};
		virtual ~Decrypt_Base() {};

		//! Set a decryption key
		/*! \return True if key is accepted
		 */
		virtual bool SetKey(Uint32 KeySize, const Uint8 *Key) = 0;

		//! Set a decryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 *  \note Some crypto schemes, such as cypher block chaining, only require
		 *        the initialization vector to be set at the start - in these cases
		 *        Force will be set to true when the vector needs to be initialized,
		 *        and false for any other calls.  This allows different schemes to be
		 *        used with minimal changes in the calling code.
		 */
		virtual bool SetIV(Uint32 IVSize, const Uint8 *IV, bool Force = false) = 0;

		//! Set a decryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 *  \note Some crypto schemes, such as cypher block chaining, only require
		 *        the initialization vector to be set at the start - in these cases
		 *        Force will be set to true when the vector needs to be initialized,
		 *        and false for any other calls.  This allows different schemes to be
		 *        used with minimal changes in the calling code.
		 */
		bool SetIV(DataChunk &IV, bool Force = false) { return SetIV(IV.Size, IV.Data, Force); }

		//! Set a decryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 *  \note Some crypto schemes, such as cypher block chaining, only require
		 *        the initialization vector to be set at the start - in these cases
		 *        Force will be set to true when the vector needs to be initialized,
		 *        and false for any other calls.  This allows different schemes to be
		 *        used with minimal changes in the calling code.
		 */
		bool SetIV(DataChunkPtr &IV, bool Force = false) { return SetIV(IV->Size, IV->Data, Force); }

		//! Get the Initialization Vector that will be used for the next encryption
		/*! If called immediately after SetIV() with Force=true or SetIV() for a crypto
		 *  scheme that accepts each offered vector (rather than creating its own ones)
		 *  the result will be the vector offered in that SetIV() call.
		 */
		virtual DataChunkPtr GetIV(void) = 0;

		//! Can this decryption system safely decrypt in place?
		/*! If BlockSize is 0 this function will return true if decryption of all block sizes can be "in place".
		 *  Otherwise the result will indicate whether the given blocksize can be decrypted "in place".
		 */
		virtual bool CanDecryptInPlace(Uint32 BlockSize = 0) = 0;

		//! Decrypt data bytes in place
		/*! \return true if the decryption <i>appears to be</i> successful
		 */
		virtual bool DecryptInPlace(Uint32 Size, Uint8 *Data) = 0;

		//! Decrypt data bytes in place
		/*! \return true if the decryption <i>appears to be</i> successful
		 */
		bool DecryptInPlace(DataChunk &Data) { return DecryptInPlace(Data.Size, Data.Data); };

		//! Decrypt data bytes in place
		/*! \return true if the decryption <i>appears to be</i> successful
		 */
		bool DecryptInPlace(DataChunkPtr &Data) { return DecryptInPlace(Data->Size, Data->Data); };

		//! Decrypt data and return in a new buffer
		/*! \return NULL pointer if the decryption is unsuccessful
		 */
		virtual DataChunkPtr Decrypt(Uint32 Size, const Uint8 *Data) = 0;

		//! Decrypt data and return in a new buffer
		/*! \return NULL pointer if the decryption is unsuccessful
		 */
		DataChunkPtr Decrypt(DataChunk &Data) { return Decrypt(Data.Size, Data.Data); };

		//! Decrypt data and return in a new buffer
		/*! \return NULL pointer if the decryption is unsuccessful
		 */
		DataChunkPtr Decrypt(DataChunkPtr &Data) { return Decrypt(Data->Size, Data->Data); };
	};

	// Smart pointer to a dencryption wrapper object
	typedef SmartPtr<Decrypt_Base> DecryptPtr;


	//! KLVEObject class
	/*! This class gives access to single AS-DCP encrypted KLV items within an MXF file with KLVObject interfacing
	 */
	class KLVEObject : public KLVObject
	{
	protected:
		EncryptPtr	Encrypt;						//!< Pointer to the encryption wrapper
		DecryptPtr	Decrypt;						//!< Pointer to the decryption wrapper

		bool DataLoaded;							//!< True once the AS-DCP header data has been read
		ULPtr ContextID;							//!< The context ID used to link to encryption metadata
		Uint64 PlaintextOffset;						//!< Number of unencrypted bytes at start of source data
		ULPtr SourceKey;							//!< Key for the plaintext KLV
		Length SourceLength;						//!< Length of the plaintext KLV Value
		int SourceLengthFormat;						//!< Number of bytes used to encode SourceLength in the KLVE (allows us to faithfully recreate if required)
		Uint8 IV[16];								//!< The Initialization Vector for this KLVE
		Uint8 Check[16];							//!< The check value for this KLVE
		int DataOffset;								//!< Offset of the start of the excrypted value from the start of the KLV value

		DataChunkPtr EncryptionIV;						//!< Encryption IV if one has been specified

		/* Some useful constants */
		//! The AS-DCP encryption system adds 32 bytes to the start of the encrypted data
		enum { EncryptionOverhead = 32 };

		//! The AS-DCP encryption system forces all encrypted data to be in multiples of 16 bytes
		enum { EncryptionGranularity = 16 };

	public:
		//** KLVEObject Specifics **//

		//! Set the encryption wrapper
		void SetEncrypt(EncryptPtr NewWrapper) { Encrypt = NewWrapper; };

		//! Set the decryption wrapper
		void SetDecrypt(DecryptPtr NewWrapper) { Decrypt = NewWrapper; };

		//! Set an encryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 */
		virtual bool SetEncryptIV(Uint32 IVSize, const Uint8 *IV, bool Force = false)
		{
			Force;		// Unused parameter

			EncryptionIV = new DataChunk(IVSize, IV);

			return true;
		}

		//! Set a decryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 */
		virtual bool SetDecryptIV(Uint32 IVSize, const Uint8 *IV, bool Force = false);

		//! Get the Initialization Vector that will be used for the next encryption
		virtual DataChunkPtr GetEncryptIV(void);

		//! Get the Initialization Vector that will be used for the next decryption
		virtual DataChunkPtr GetDecryptIV(void);


		//** Construction / desctruction **//
		KLVEObject(ULPtr ObjectUL);				//!< Construct a new KLVEObject
		KLVEObject(KLVObjectPtr Object);		//!< Construct a KLVEObject linked to an encrypted KLVObject
		KLVEObject(KLVObject &Object);			//!< Construct a KLVEObject linked to an encrypted KLVObject
		virtual void Init(void);
		virtual ~KLVEObject() {};


		//** KLVObject interfaces **//
		/* DRAGONS: We should prune these and fall back where possible? */

		//! Set the source details when an object has been read from a file
		virtual void SetSource(MXFFilePtr File, Position Location, Uint32 NewKLSize, Length ValueLen)
		{
			IsConstructed = false;
			SourceOffset = Location;
			KLSize = NewKLSize;
			SourceFile = File;
		}

		//! Set the source details when an object is build in memory
		virtual void SetSource(Position Location, Uint32 NewKLSize, Length ValueLen)
		{
			IsConstructed = false;
			SourceOffset = Location;
			KLSize = NewKLSize;
			SourceFile = NULL;
		}

		//! Get the object's UL
		virtual ULPtr GetUL(void) { return TheUL; }

		//! Set the object's UL
		virtual void SetUL(ULPtr NewUL) { TheUL = NewUL; }

		//! Get the location within the ultimate parent
		virtual Uint64 GetLocation(void) { return SourceOffset; }

		//! Get text that describes where this item came from
		virtual std::string GetSource(void);

		//! Get the size of the key and length (not of the value)
		/*! \note For an KLVEObject this actually returns the sum of the size of all parts of the
		 *        KLV other than the decrypted value - in other words total KLVE length - Source Length
		 */
		virtual Uint32 GetKLSize(void);

		//! Get a GCElementKind structure
		virtual GCElementKind GetGCElementKind(void);

		//! Get the track number of this KLVObject (if it is a GC KLV, else 0)
		virtual Uint32 GetGCTrackNumber(void);

		//! Get the position of the first byte in the DataChunk as an offset into the file
		/*! \return -1 allways to indicate that the data cannot be read directly from the file 
		 */
		virtual Position GetDataBase(void) { return -1; };

		//! Set the position of the first byte in the DataChunk as an offset into the file
		/*! \note This function must be used with great care as data may will be written to this location
		 */
		virtual void SetDataBase(Position NewBase) { SourceOffset = NewBase; };

		//! Read data from the KLVObject source into the DataChunk
		/*! If Size is zero an attempt will be made to read all available data (which may be billions of bytes!)
		 *  \note Any previously read data in the current DataChunk will be discarded before reading the new data
		 *	\return Number of bytes read - zero if none could be read
		 */
		virtual Length ReadData(Position Start = 0, Length Size = 0);

		//! Write the key and length of the current DataChunk to the source file
		/*! This function writes the entire header - not just the Key and Length
		 */
		virtual Uint32 WriteKL(Uint32 LenSize = 0);

		//! Write the key and length of the current DataChunk to the specified file
		/*! This function writes the entire header - not just the Key and Length
		 */
		virtual Uint32 WriteKL(MXFFilePtr &File, Uint32 LenSize = 0);

		//! Write data from a specified buffer to the source file
		virtual Length WriteData(const Uint8 *Buffer, Position Start = 0, Length Size = 0);

		//! Write data from the a buffer to a specified source file
		virtual Length WriteData(MXFFilePtr &File, const Uint8 *Buffer, Length Size);

		//! Set a handler to supply data when a read is performed
		/*! \note If not set it will be read from the source file (if available) or cause an error message
		 */
		virtual void SetReadHandler(KLVReadHandlerPtr Handler) { ReadHandler = Handler; }

		//! Get the length of the value field
		virtual Length GetLength(void) { return ValueLength; }

		//! Get a reference to the data chunk
		virtual DataChunk& GetData(void) { return Data; }

	protected:
		//! Load the AS-DCP set data
		/*! Sets DataLoaded on success
		 * \return true if all loaded OK, false on error
		 */
		bool LoadData(void);
	};

	// Smart pointer to a KLVEObject (callot point to KLVObjects)
	typedef SmartPtr<KLVEObject> KLVEObjectPtr;

}


#endif // MXFLIB__CRYPTO_H
