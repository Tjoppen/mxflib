/*! \file	mdtraits.h
 *	\brief	Definition of traits for MDType definitions
 *
 *	\version $Id: mdtraits.h,v 1.10 2003/12/18 17:51:55 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2003, Matt Beard
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
	// Forward declare so the class can include pointers to itself (if required)
	class MDTraits;

	//! A smart pointer to an MDTraits object
	typedef SmartPtr<MDTraits> MDTraitsPtr;

	//! A list of smart pointers to MDTraits objects
	typedef std::list<MDTraitsPtr> MDTraitsList;
}


namespace mxflib
{
	// We need access to the MDValue class
	class MDValue;
	//! A smart pointer to an MDValue object
	class MDValuePtr;

	class MDTraits
	{
	public:
		MDTraits() {};

	// Default implementations
	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual void SetInt64(MDValuePtr Object, Int64 Val);
		virtual void SetUint(MDValuePtr Object, Uint32 Val);
		virtual void SetUint64(MDValuePtr Object, Uint64 Val);
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual Uint32 GetUint(MDValuePtr Object);
		virtual Uint64 GetUint64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);

		virtual Uint32 ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count=0);

		// Give the MDValue class access to our internals to call Set/Get functions
		friend class MDValue;
	};

	// Extended implementations

	class MDTraits_BasicInt : public MDTraits
	{
	protected:
		virtual void SetInt64(MDValuePtr Object, Int64 Val);
		virtual void SetUint(MDValuePtr Object, Uint32 Val);
		virtual void SetUint64(MDValuePtr Object, Uint64 Val);
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual Uint64 GetUint64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);

		virtual Uint32 ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count=0);
	};

	//! Special unsigned integer ReadValue
	Uint32 ReadValueUint(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count=0);

	class MDTraits_Int8 : public MDTraits_BasicInt
	{
	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Uint32 GetUint(MDValuePtr Object);
	};

	class MDTraits_Uint8 : public MDTraits_Int8
	{
		virtual std::string GetString(MDValuePtr Object);
		virtual Uint32 ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count=0);
	};

	class MDTraits_Int16 : public MDTraits_BasicInt
	{
	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Uint32 GetUint(MDValuePtr Object);
	};

	class MDTraits_Uint16 : public MDTraits_Int16
	{
		virtual std::string GetString(MDValuePtr Object);
		virtual Uint32 ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count=0);
	};

	class MDTraits_Int32 : public MDTraits_BasicInt
	{
	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Uint32 GetUint(MDValuePtr Object);
	};

	class MDTraits_Uint32 : public MDTraits_Int32
	{
		virtual std::string GetString(MDValuePtr Object);
		virtual Uint32 ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count=0);
	};

	class MDTraits_Int64 : public MDTraits
	{
	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual void SetInt64(MDValuePtr Object, Int64 Val);
		virtual void SetUint(MDValuePtr Object, Uint32 Val);
		virtual void SetUint64(MDValuePtr Object, Uint64 Val);
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Uint32 GetUint(MDValuePtr Object);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual Uint64 GetUint64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_Uint64 : public MDTraits_Int64
	{
		virtual std::string GetString(MDValuePtr Object);
		virtual Uint32 ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count=0);
	};

	class MDTraits_ISO7 : public MDTraits_Uint8
	{
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_UTF16 : public MDTraits_Uint16
	{
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_Raw : public MDTraits
	{
		//DRAGONS: Should probably have set and get integer functions as well
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};


	class MDTraits_BasicArray : public MDTraits
	{
	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual void SetInt64(MDValuePtr Object, Int64 Val);
		virtual void SetUint(MDValuePtr Object, Uint32 Val);
		virtual void SetUint64(MDValuePtr Object, Uint64 Val);
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual Uint32 GetUint(MDValuePtr Object);
		virtual Uint64 GetUint64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);

		virtual Uint32 ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count=0);
	};
	
	class MDTraits_BasicStringArray : public MDTraits_BasicArray
	{
	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};
	
	class MDTraits_RawArray : public MDTraits_BasicArray
	{
	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_RawArrayArray : public MDTraits_BasicArray
	{
	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_BasicCompound : public MDTraits
	{
		// DRAGONS: What about all the other set and get functions?
	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);

		virtual Uint32 ReadValue(MDValuePtr Object, const Uint8 *Buffer, Uint32 Size, int Count=0);
	};

	class MDTraits_Rational : public MDTraits_BasicCompound
	{
	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_TimeStamp : public MDTraits_BasicCompound
	{
	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};
}

#endif // MXFLIB__MDTRAITS_H
