/*! \file	crypto.h
 *	\brief	Definition of classes that wrap encryption and decryption tools
 *
 *	\version $Id: crypto.h,v 1.1.2.2 2004/05/10 17:10:51 matt-beard Exp $
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
		Encrypt_Base();
		virtual ~Encrypt_Base();

		//! Set an encryption key
		/*! \return True if key is accepted
		 */
		virtual bool SetKey(Uint32 KeySize, Uint8 *Key) = 0;

		//! Set an encryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 *  \note Some crypto schemes, such as cypher block chaining, only require
		 *        the initialization vector to be set at the start - in these cases
		 *        Force will be set to true when the vector needs to be initialized,
		 *        and false for any other calls.  This allows different schemes to be
		 *        used with minimal changes in the calling code.
		 */
		virtual bool SetIV(Uint32 IVSize, Uint8 *IV, bool Force = false) = 0;

		//! Can this encryption system safely encrypt in place?
		virtual bool CanEncryptInPlace(void) = 0;

		//! Encrypt data bytes in place
		/*! \return true if the encryption is successful
		 */
		virtual bool EncryptInPlace(Uint32 Size, Uint8 *Data) = 0;

		//! Encrypt data bytes in place
		/*! \return true if the encryption is successful
		 */
		bool EncryptInPlace(DataChunkPtr &Data) { return EncryptInPlace(Data->Size(), Data->Data()); };

		//! Encrypt data and return in a new buffer
		/*! \return NULL pointer if the encryption is unsuccessful
		 */
		virtual DataChunkPtr Encrypt(Uint32 Size, const Uint8 *Data) = 0;

		//! Encrypt data and return in a new buffer
		/*! \return NULL pointer if the encryption is unsuccessful
		 */
		DataChunkPtr Encrypt(DataChunkPtr &Data) { return Encrypt(Data->Size(), Data->Data()); };
	};

	//! Base decryptor wrapper class
	/*! \note Classes derived from this class <b>must not</b> include their own RefCount<> derivation
	 */
	class Decrypt_Base : public RefCount<Decrypt_Base>
	{
	public:
		Decrypt_Base();
		virtual ~Decrypt_Base();

		//! Set a decryption key
		/*! \return True if key is accepted
		 */
		virtual bool SetKey(Uint32 KeySize, Uint8 *Key) = 0;

		//! Set a decryption Initialization Vector
		/*! \return False if Initialization Vector is rejected
		 *  \note Some crypto schemes, such as cypher block chaining, only require
		 *        the initialization vector to be set at the start - in these cases
		 *        Force will be set to true when the vector needs to be initialized,
		 *        and false for any other calls.  This allows different schemes to be
		 *        used with minimal changes in the calling code.
		 */
		virtual bool SetIV(Uint32 IVSize, Uint8 *IV, bool Force = false) = 0;

		//! Can this decryption system safely decrypt in place?
		virtual bool CanEncryptInPlace(void) = 0;

		//! Decrypt data bytes in place
		/*! \return true if the decryption <i>appears to be</i> successful
		 */
		virtual bool DecryptInPlace(Uint32 Size, Uint8 *Data) = 0;

		//! Decrypt data bytes in place
		/*! \return true if the decryption <i>appears to be</i> successful
		 */
		bool DecryptInPlace(DataChunkPtr &Data) { return DecryptInPlace(Data->Size(), Data->Data()); };

		//! Decrypt data and return in a new buffer
		/*! \return NULL pointer if the decryption is unsuccessful
		 */
		virtual DataChunkPtr Decrypt(Uint32 Size, const Uint8 *Data) = 0;

		//! Decrypt data and return in a new buffer
		/*! \return NULL pointer if the decryption is unsuccessful
		 */
		DataChunkPtr Decrypt(DataChunkPtr &Data) { return Decrypt(Data->Size(), Data->Data()); };
	};

}


#endif // MXFLIB__CRYPTO_H
