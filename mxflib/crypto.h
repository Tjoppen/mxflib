/*! \file	crypto.h
 *	\brief	Definition of classes that wrap encryption and decryption tools
 *
 *	\version $Id: crypto.h,v 1.1.2.6 2004/06/26 18:05:29 matt-beard Exp $
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
		virtual bool EncryptInPlace(Uint32 Size, Uint8 *Data) = 0;

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

		//! Get the Initialization Vector that will be used for the next decryption
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
		Length EncryptedLength;						//!< Length of the encrypted KLV Value
		int SourceLengthFormat;						//!< Number of bytes used to encode SourceLength in the KLVE (allows us to faithfully recreate if required)
		Length OuterLength;							//!< The length field of the containing KLV (the KLVE length not the length of the plaintext data)
		Uint8 IV[16];								//!< The Initialization Vector for this KLVE
		Uint8 Check[16];							//!< The check value for this KLVE
		Int32 DataOffset;							//!< Offset of the start of the excrypted value from the start of the KLV value

		DataChunkPtr EncryptionIV;					//!< Encryption IV if one has been specified

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
		KLVEObject(KLVObjectPtr &Object);		//!< Construct a KLVEObject linked to an encrypted KLVObject
		virtual void Init(void);
		virtual ~KLVEObject() {};


		//** KLVObject interfaces **//
		/* DRAGONS: We should prune these and fall back where possible? */

		//! Get the object's UL
		virtual ULPtr GetUL(void) 
		{ 
			// If we are decrypting, ensure we know the proper plaintext UL
			if(Decrypt && (!DataLoaded)) LoadData();

			return TheUL; 
		}

		//! Set the object's UL
		virtual void SetUL(ULPtr NewUL) { TheUL = NewUL; }

		//! Get the size of the key and length (not of the value)
		/*! \note For an KLVEObject this actually returns the sum of the size of all parts of the
		 *        KLV other than the decrypted value - in other words total KLVE length - Source Length
		 */
		virtual Int32 GetKLSize(void);

		//! Get a GCElementKind structure
		virtual GCElementKind GetGCElementKind(void);

		//! Get the track number of this KLVObject (if it is a GC KLV, else 0)
		virtual Uint32 GetGCTrackNumber(void);

		//! Get text that describes where this item came from
		virtual std::string GetSource(void);

		//! Read the key and length for this KLVObject from the current source
		/*! \return The number of bytes read (i.e. KLSize)
		 */
		virtual Int32 ReadKL(void);

		//! Read data from the start of the KLV value into the current DataChunk
		/*! \param Size Number of bytes to read, if zero all available bytes will be read (which could be billions!)
		 *  \return The number of bytes read
		 */
		virtual Length ReadData(Length Size = 0) { return Base_ReadDataFrom(0, Size); }

		//! Read data from a specified position in the KLV value field into the DataChunk
		/*! \param Offset Offset from the start of the KLV value from which to start reading
		 *  \param Size Number of bytes to read, if <=0 all available bytes will be read (which could be billions!)
		 *  \return The number of bytes read
		 */
		virtual Length ReadDataFrom(Position Offset, Length Size = -1);

		//! Write the key and length of the current DataChunk to the destination file
		/*! The key and length will be written to the source file as set by SetSource.
		 *  If LenSize is zero the length will be formatted to match KLSize (if possible!)
		 *  \note If the length will not fit in KLSize then KLSize will be updated
		 */
		virtual Int32 WriteKL(Int32 LenSize = 0);

		//! Write (some of) the current data to the same location in the destination file
		/*! \param Size The number of bytes to write, if <= 0 all available bytes will be written
		 *  \return The number of bytes written
		 */
		virtual Length WriteData(Length Size = -1) { return WriteDataFromTo(0, 0, Size); }

		//! Write (some of) the current data to the same location in the destination file
		/*! \param Start The offset within the current DataChunk of the first byte to write
		 *  \param Size The number of bytes to write, if <= 0 all available bytes will be written
		 *  \return The number of bytes written
		 */
		virtual Length WriteDataFrom(Position Start, Length Size = -1) { return WriteDataFromTo(0, Start, Size); }

		//! Write (some of) the current data to a different location in the destination file
		/*! \param Offset The offset within the KLV value field of the first byte to write
		 *  \param Size The number of bytes to write, if <= 0 all available bytes will be written
		 *  \return The number of bytes written
		 */
		virtual Length WriteDataTo(Position Offset, Length Size = -1) { return WriteDataFromTo(Offset, 0, Size); }

		//! Write (some of) the current data to the same location in the destination file
		/*! \param Offset The offset within the KLV value field of the first byte to write
		 *  \param Start The offset within the current DataChunk of the first byte to write
		 *  \param Size The number of bytes to write, if <= 0 all available bytes will be written
		 *  \return The number of bytes written
		 */
		virtual Length WriteDataFromTo(Position Offset, Position Start, Length Size = -1)
		{
			// Calculate default number of bytes to write
			Length BytesToWrite = Data.Size - Start;

			// Write the requested size (if valid)
			if((Size > 0) && (Size < BytesToWrite)) BytesToWrite = Size;

			return WriteDataTo(&Data.Data[Start], Offset, BytesToWrite);
		}

		//! Write data from a given buffer to a given location in the destination file
		/*! \param Buffer Pointer to data to be written
		 *  \param Offset The offset within the KLV value field of the first byte to write
		 *  \param Size The number of bytes to write
		 *  \return The number of bytes written
		 *  \note As there may be a need for the implementation to know where within the value field
		 *        this data lives, there is no WriteData(Buffer, Size) function.
		 */
		virtual Length WriteDataTo(Uint8 *Buffer, Position Offset, Length Size);

		//! Get the length of the value field
		virtual Length GetLength(void) 
		{ 
			return ValueLength;
		}
/*			// If we are not decrypting then return entire value size
			if(!Decrypt) return ValueLength;

			// Try and load the header
			if(!DataLoaded) if(!LoadData()) return 0;

			// Return the plaintext size
			return SourceLength; 
		}
*/

		//! Set the length of the value field
		virtual void SetLength(Length NewLength) 
		{ 
			ValueLength = NewLength; 

			// Update encrypted length to take account of padding
			EncryptedLength = (ValueLength + EncryptionGranularity) / EncryptionGranularity;
			EncryptedLength *= EncryptionGranularity;
		}


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
