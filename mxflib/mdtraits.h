/*! \file	mdtraits.h
 *	\brief	Definition of traits for MDType definitions
 *
 *	\version $Id: mdtraits.h,v 1.6 2006/07/02 13:27:51 matt-beard Exp $
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

	//! A map of smart pointers to MDTraits objects, indexed by name
	typedef std::map<std::string, MDTraitsPtr> MDTraitsMap;
}


namespace mxflib
{
	//! Soft limit for strings returned by MDTraits
	/*! \note This is a soft limit in that it is not enforced strictly.
	 *        It is possible for string values to be returned that are longer than this value, but where
	 *		  the string is built by several passes around a loop that loop should exit once this value
	 *		  has been reached
	 *
     * TODO: Apply this limit to everywhere it is required!!
	 */
	extern UInt32 MDTraits_StringLimit;

	//! Set the string size soft limit
	inline void SetStringLimit(UInt32 StringLimit) { MDTraits_StringLimit = StringLimit; }

	//! Get the current string size soft limit
	inline UInt32 GetStringLimit(void) { return MDTraits_StringLimit; }
}


namespace mxflib
{
	//! Flag to modify string behaviour to terminate all strings written
	/*! \note This only works for UTF16 and ISO7 string SetString traits
	 */
	extern bool TerminateStrings;

	//! Set the string termination flag
	inline void SetStringTermination(bool Value) { TerminateStrings = Value; }

	//! Get the string termination flag
	inline bool GetStringTermination(void) { return TerminateStrings; }
}


namespace mxflib
{
	class MDTraits : public RefCount<MDTraits>
	{
	protected:
		static MDTraitsMap AllTraits;

		//! Protected constructor so all traits need to be created via Create()
		MDTraits() {};

	public:
		//! Allow virtual destruction
		virtual ~MDTraits() {}

		//! Does this trait take control of all sub-data and build values in the values own DataChunk?
		/*! Normally any contained sub-types (such as array items or compound members) hold their own data */
		virtual bool HandlesSubdata(void) const { return false; };

		//! A unique name for this trait
		virtual std::string Name() const = 0;

	/* Static Methods */
	public:
		//! Add a new trait to the list of known traits
		/*! \ret True is all went well, else false
		 */
		static bool Add(std::string Name, MDTraitsPtr Trait);

		//! Replace the named trait in the list of known traits
		/*! \ret True is all went well, else false
		 */
		static bool Replace(std::string Name, MDTraitsPtr Trait);

		//! Locate a named trait in the list of known traits
		/*! \ret A pointer to the named trait, or NULL if not found
		 */
		static MDTraitsPtr Find(std::string Name);

	/* Default implementations */
	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual void SetInt64(MDValuePtr Object, Int64 Val);
		virtual void SetUInt(MDValuePtr Object, UInt32 Val);
		virtual void SetUInt64(MDValuePtr Object, UInt64 Val);
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual UInt32 GetUInt(MDValuePtr Object);
		virtual UInt64 GetUInt64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);

		virtual size_t ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);

		//! Support old capitalization of SetUInt
		inline void SetUint(MDValuePtr Object, UInt32 Val) { SetUInt(Object, Val); }

		//! Support old capitalization of SetUInt64
		inline void SetUint64(MDValuePtr Object, UInt64 Val) { SetUInt64(Object, Val); }

		//! Support old capitalization of GetUInt
		inline UInt32 GetUint(MDValuePtr Object) { return GetUInt(Object); }

		//! Support old capitalization of GetUInt64
		inline UInt64 GetUint64(MDValuePtr Object) { return GetUInt64(Object); }

		// Give the MDValue class access to our internals to call Set/Get functions
		friend class MDValue;
	};

	//! Create a new trait of this type and add it to the known traits list
	/*! \ret The name of the trait as added to the list
		*/
	template<class C> std::string CreateMDTraits(void)
	{
		MDTraitsPtr Tr = new C;
		MDTraits::Add(Tr->Name(), Tr );
		return Tr->Name();
	}

	class MDTraits_DefaultTraits : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_DefaultTraits"; };
	};

	// Extended implementations
	class MDTraits_BasicInt : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_BasicInt"; };

	protected:
		virtual void SetInt64(MDValuePtr Object, Int64 Val);
		virtual void SetUInt(MDValuePtr Object, UInt32 Val);
		virtual void SetUInt64(MDValuePtr Object, UInt64 Val);
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual UInt64 GetUInt64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);

		virtual size_t ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	//! Special unsigned integer ReadValue
	size_t ReadValueUInt(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);

	class MDTraits_Int8 : public MDTraits_BasicInt
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Int8"; };

	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual UInt32 GetUInt(MDValuePtr Object);
	};

	class MDTraits_UInt8 : public MDTraits_Int8
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UInt8"; };

	protected:
		virtual std::string GetString(MDValuePtr Object);
		virtual size_t ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_Int16 : public MDTraits_BasicInt
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Int16"; };

	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual UInt32 GetUInt(MDValuePtr Object);
	};

	class MDTraits_UInt16 : public MDTraits_Int16
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UInt16"; };

	protected:
		virtual std::string GetString(MDValuePtr Object);
		virtual size_t ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_Int32 : public MDTraits_BasicInt
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Int32"; };

	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual UInt32 GetUInt(MDValuePtr Object);
	};

	class MDTraits_UInt32 : public MDTraits_Int32
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UInt32"; };

	protected:
		virtual std::string GetString(MDValuePtr Object);
		virtual size_t ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_Int64 : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Int64"; };

	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual void SetInt64(MDValuePtr Object, Int64 Val);
		virtual void SetUInt(MDValuePtr Object, UInt32 Val);
		virtual void SetUInt64(MDValuePtr Object, UInt64 Val);
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual UInt32 GetUInt(MDValuePtr Object);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual UInt64 GetUInt64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_UInt64 : public MDTraits_Int64
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UInt64"; };

	protected:
		virtual std::string GetString(MDValuePtr Object);
		virtual size_t ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_ISO7 : public MDTraits_UInt8
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_ISO7"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_UTF16 : public MDTraits_UInt16
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UTF16"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_Raw : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Raw"; };

		//! Does this trait take control of all sub-data and build values in the values own DataChunk?
		/*! Normally any contained sub-types (such as array items or compound members) hold their own data */
		virtual bool HandlesSubdata(void) const { return false; };

	protected:
		//DRAGONS: Should probably have set integer functions as well
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual UInt32 GetUInt(MDValuePtr Object);
		virtual UInt64 GetUInt64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);

		virtual size_t ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};


	class MDTraits_BasicArray : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_BasicArray"; };

	protected:
		virtual void SetInt(MDValuePtr Object, Int32 Val);
		virtual void SetInt64(MDValuePtr Object, Int64 Val);
		virtual void SetUInt(MDValuePtr Object, UInt32 Val);
		virtual void SetUInt64(MDValuePtr Object, UInt64 Val);
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual Int32 GetInt(MDValuePtr Object);
		virtual Int64 GetInt64(MDValuePtr Object);
		virtual UInt32 GetUInt(MDValuePtr Object);
		virtual UInt64 GetUInt64(MDValuePtr Object);
		virtual std::string GetString(MDValuePtr Object);

		virtual size_t ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_BasicStringArray : public MDTraits_BasicArray
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_BasicStringArray"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};
	
	class MDTraits_UTF16String : public MDTraits_BasicStringArray
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UTF16String"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_RawArray : public MDTraits_BasicArray
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_RawArray"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_UUID : public MDTraits_Raw
	{
	public:
		//! Does this trait take control of all sub-data and build values in the values own DataChunk?
		/*! The entire UUID is held locally */
		virtual bool HandlesSubdata(void) const { return true; };

		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UUID"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_Label : public MDTraits_Raw
	{
	public:
		//! Does this trait take control of all sub-data and build values in the values own DataChunk?
		/*! The entire Label is held locally */
		virtual bool HandlesSubdata(void) const { return true; };

		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Label"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_UMID : public MDTraits_Raw
	{
	public:
		//! Does this trait take control of all sub-data and build values in the values own DataChunk?
		/*! The entire UMID is held locally */
		virtual bool HandlesSubdata(void) const { return true; };

		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UMID"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_RawArrayArray : public MDTraits_BasicArray
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_RawArrayArray"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_BasicCompound : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_BasicCompound"; };

	protected:
		// DRAGONS: What about all the other set and get functions?
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);

		virtual size_t ReadValue(MDValuePtr Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_Rational : public MDTraits_BasicCompound
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Rational"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};

	class MDTraits_TimeStamp : public MDTraits_BasicCompound
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_TimeStamp"; };

	protected:
		virtual void SetString(MDValuePtr Object, std::string Val);
		virtual std::string GetString(MDValuePtr Object);
	};
}

#endif // MXFLIB__MDTRAITS_H
