/*! \file	mdtraits.h
 *	\brief	Definition of traits for MDType definitions
 */
/*
 *	Copyright (c) 2002, Matt Beard
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
#ifndef MXFLIB__MDTRAITS_H
#define MXFLIB__MDTRAITS_H


namespace mxflib
{
	// We need access to the MDValue class
	class MDValue;

	class MDTraits
	{
	public:
		MDTraits() {};

	// Default implementations
	protected:
		virtual void SetInt(MDValue *Object, Int32 Val);
		virtual void SetInt64(MDValue *Object, Int64 Val);
		virtual void SetUint(MDValue *Object, Uint32 Val);
		virtual void SetUint64(MDValue *Object, Uint64 Val);
		virtual void SetString(MDValue *Object, std::string Val);
		virtual Int32 GetInt(MDValue *Object);
		virtual Int64 GetInt64(MDValue *Object);
		virtual Uint32 GetUint(MDValue *Object);
		virtual Uint64 GetUint64(MDValue *Object);
		virtual std::string GetString(MDValue *Object);

		// Give the MDValue class access to our internals to call Set/Get functions
		friend class MDValue;
	};

	
	// Extended implementations

	class MDTraits_BasicInt : public MDTraits
	{
	protected:
		virtual void SetInt64(MDValue *Object, Int64 Val);
		virtual void SetUint(MDValue *Object, Uint32 Val);
		virtual void SetUint64(MDValue *Object, Uint64 Val);
		virtual void SetString(MDValue *Object, std::string Val);
		virtual Int64 GetInt64(MDValue *Object);
		virtual Uint64 GetUint64(MDValue *Object);
		virtual std::string GetString(MDValue *Object);
	};

	class MDTraits_Int8 : public MDTraits_BasicInt
	{
	protected:
		virtual void SetInt(MDValue *Object, Int32 Val);
		virtual Int32 GetInt(MDValue *Object);
		virtual Uint32 GetUint(MDValue *Object);
	};

	class MDTraits_Uint8 : public MDTraits_Int8
	{
	};

	class MDTraits_Int16 : public MDTraits_BasicInt
	{
	protected:
		virtual void SetInt(MDValue *Object, Int32 Val);
		virtual Int32 GetInt(MDValue *Object);
		virtual Uint32 GetUint(MDValue *Object);
	};

	class MDTraits_Uint16 : public MDTraits_Int16
	{
	};

	class MDTraits_Int32 : public MDTraits_BasicInt
	{
	protected:
		virtual void SetInt(MDValue *Object, Int32 Val);
		virtual Int32 GetInt(MDValue *Object);
		virtual Uint32 GetUint(MDValue *Object);
	};

	class MDTraits_Uint32 : public MDTraits_Int32
	{
		virtual std::string GetString(MDValue *Object);
	};

	// DRAGONS: Need 64-bit ints

	class MDTraits_ISO7 : public MDTraits_Uint8
	{
		virtual void SetString(MDValue *Object, std::string Val);
		virtual std::string GetString(MDValue *Object);
	};


/*
	void Uint8_SetInt(MDValue *Object, Int32 Val);
	void Uint8_SetInt64(MDValue *Object, Int64 Val);
	void Uint8_SetUint(MDValue *Object, Uint32 Val);
	void Uint8_SetUint64(MDValue *Object, Uint64 Val);
	void Uint8_SetString(MDValue *Object, std::string Val);
	Int32 Uint8_GetInt(MDValue *Object);
	Int64 Uint8_GetInt64(MDValue *Object);
	Uint32 Uint8_GetUint(MDValue *Object);
	Uint64 Uint8_GetUint64(MDValue *Object);
	std::string Uint8_GetString(MDValue *Object);

	void Int16_SetInt(MDValue *Object, Int32 Val);
	void Int16_SetInt64(MDValue *Object, Int64 Val);
	void Int16_SetUint(MDValue *Object, Uint32 Val);
	void Int16_SetUint64(MDValue *Object, Uint64 Val);
	void Int16_SetString(MDValue *Object, std::string Val);
	Int32 Int16_GetInt(MDValue *Object);
	Int64 Int16_GetInt64(MDValue *Object);
	Uint32 Int16_GetUint(MDValue *Object);
	Uint64 Int16_GetUint64(MDValue *Object);
	std::string Int16_GetString(MDValue *Object);

	void Uint16_SetInt(MDValue *Object, Int32 Val);
	void Uint16_SetInt64(MDValue *Object, Int64 Val);
	void Uint16_SetUint(MDValue *Object, Uint32 Val);
	void Uint16_SetUint64(MDValue *Object, Uint64 Val);
	void Uint16_SetString(MDValue *Object, std::string Val);
	Int32 Uint16_GetInt(MDValue *Object);
	Int64 Uint16_GetInt64(MDValue *Object);
	Uint32 Uint16_GetUint(MDValue *Object);
	Uint64 Uint16_GetUint64(MDValue *Object);
	std::string Uint16_GetString(MDValue *Object);
*/
}

#endif MXFLIB__MDTRAITS_H

