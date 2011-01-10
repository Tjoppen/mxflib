/*! \file	mdtraits.h
 *	\brief	Definition of traits for MDType definitions
 *
 *	\version $Id: mdtraits.h,v 1.9 2011/01/10 10:42:09 matt-beard Exp $
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

	// Forward declare for parameter use
	class MDObject;
	class MDObjectPtr;
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

	//! Options for converting labels to a string of text or hex
	enum LabelFormat 
	{
		LabelFormatText = 0,			//!< Use just the text, if known, otherwise use hex
		LabelFormatHex,				//!< Use only the hex
		LabelFormatTextHex,			//!< Use the text, if known, but append the hex
		LabelFormatTextHexMask			//!< Use the text, if known, but append the hex if a mask was used in label matching
	};

	//! The current options for converting labels to strings
	extern LabelFormat LabelFormatOption;

	//! Set the options for converting labels to strings
	inline void SetLabelFormat(LabelFormat Value) { LabelFormatOption = Value; }

	//! Get the options for converting labels to strings
	inline LabelFormat GetLabelFormat(void) { return LabelFormatOption; }


	//! Class for managing allocation of OutputFormatEnum enumerations at runtime
	class MDTraitsEnum
	{
	protected:
		//! The last value allocated an an enumerated value, -1 if none ever issued
		static OutputFormatEnum LastAllocatedEnum;

	public:
		//! Allocate a new and unique enumerated value	
		static OutputFormatEnum GetNewEnum(void)
		{
			return ++LastAllocatedEnum;
		}
	};

}


namespace mxflib
{
	class MDTraits : public RefCount<MDTraits>
	{
	protected:
		//! List of all traits that exist
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
		virtual void SetInt(MDObject *Object, Int32 Val);
		virtual void SetInt64(MDObject *Object, Int64 Val);
		virtual void SetUInt(MDObject *Object, UInt32 Val);
		virtual void SetUInt64(MDObject *Object, UInt64 Val);
		virtual void SetString(MDObject *Object, std::string Val);
		virtual Int32 GetInt(const MDObject *Object) const;
		virtual Int64 GetInt64(const MDObject *Object) const;
		virtual UInt32 GetUInt(const MDObject *Object) const;
		virtual UInt64 GetUInt64(const MDObject *Object) const;
		virtual std::string GetString(const MDObject *Object) const;

		//! Base version of GetString without with Format that passes control to version without
		/*! DRAGONS: This allows traits that don't support different formats to omit a definition of GetString with the Format parameter (for backwards compatibility) */
		virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }

		virtual size_t ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);

		//! Support old capitalization of SetUInt
		void SetUint(MDObject *Object, UInt32 Val);

		//! Support old capitalization of SetUInt64
		void SetUint64(MDObject *Object, UInt64 Val);

		//! Support old capitalization of GetUInt
		UInt32 GetUint(const MDObject *Object) const;

		//! Support old capitalization of GetUInt64
		UInt64 GetUint64(const MDObject *Object) const;


		/* MDObjectPtr versions */
		void SetInt(MDObjectPtr Object, Int32 Val);
		void SetInt64(MDObjectPtr Object, Int64 Val);
		void SetUInt(MDObjectPtr Object, UInt32 Val);
		void SetUInt64(MDObjectPtr Object, UInt64 Val);
		void SetString(MDObjectPtr Object, std::string Val);
		void SetUint(MDObjectPtr Object, UInt32 Val);
		void SetUint64(MDObjectPtr Object, UInt64 Val);
		Int32 GetInt(const MDObjectPtr Object) const;
		Int64 GetInt64(const MDObjectPtr Object) const;
		UInt32 GetUInt(const MDObjectPtr Object) const;
		UInt64 GetUInt64(const MDObjectPtr Object) const;
		UInt32 GetUint(const MDObjectPtr Object) const;
		UInt64 GetUint64(const MDObjectPtr Object) const;
		std::string GetString(const MDObjectPtr Object) const;
		size_t ReadValue(MDObjectPtr Object, const UInt8 *Buffer, size_t Size, int Count=0);

	public:
		//! Set the default output format from a string and return an OutputFormatEnum value to use in future
		static OutputFormatEnum SetOutputFormat(std::string Format) { return -1; }

		//! Set the default output format
		static void SetOutputFormat(OutputFormatEnum Format) { }

		// Give the MDObject class access to our internals to call Set/Get functions
		friend class MDObject;

// FIXME: Horrible kludge as my brian hurts!
friend class MDTraits_BasicEnum;

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
		virtual void SetInt64(MDObject *Object, Int64 Val);
		virtual void SetUInt(MDObject *Object, UInt32 Val);
		virtual void SetUInt64(MDObject *Object, UInt64 Val);
		virtual void SetString(MDObject *Object, std::string Val);
		virtual Int64 GetInt64(const MDObject *Object) const;
		virtual UInt64 GetUInt64(const MDObject *Object) const;
		virtual std::string GetString(const MDObject *Object) const;

                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }

		virtual size_t ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	//! Special unsigned integer ReadValue
	size_t ReadValueUInt(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);

	class MDTraits_Int8 : public MDTraits_BasicInt
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Int8"; };

	protected:
		virtual void SetInt(MDObject *Object, Int32 Val);
		virtual Int32 GetInt(const MDObject *Object) const;
		virtual UInt32 GetUInt(const MDObject *Object) const;
	};

	class MDTraits_UInt8 : public MDTraits_Int8
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UInt8"; };

	protected:
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }

		virtual size_t ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_Int16 : public MDTraits_BasicInt
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Int16"; };

	protected:
		virtual void SetInt(MDObject *Object, Int32 Val);
		virtual Int32 GetInt(const MDObject *Object) const;
		virtual UInt32 GetUInt(const MDObject *Object) const;
	};

	class MDTraits_UInt16 : public MDTraits_Int16
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UInt16"; };

	protected:
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }

		virtual size_t ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_Int32 : public MDTraits_BasicInt
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Int32"; };

	protected:
		virtual void SetInt(MDObject *Object, Int32 Val);
		virtual Int32 GetInt(const MDObject *Object) const;
		virtual UInt32 GetUInt(const MDObject *Object) const;
	};

	class MDTraits_UInt32 : public MDTraits_Int32
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UInt32"; };

	protected:
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }

		virtual size_t ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_Int64 : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Int64"; };

	protected:
		virtual void SetInt(MDObject *Object, Int32 Val);
		virtual void SetInt64(MDObject *Object, Int64 Val);
		virtual void SetUInt(MDObject *Object, UInt32 Val);
		virtual void SetUInt64(MDObject *Object, UInt64 Val);
		virtual void SetString(MDObject *Object, std::string Val);
		virtual Int32 GetInt(const MDObject *Object) const;
		virtual UInt32 GetUInt(const MDObject *Object) const;
		virtual Int64 GetInt64(const MDObject *Object) const;
		virtual UInt64 GetUInt64(const MDObject *Object) const;
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};

	class MDTraits_UInt64 : public MDTraits_Int64
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UInt64"; };

	protected:
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
		virtual size_t ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_ISO7 : public MDTraits_UInt8
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_ISO7"; };

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};

	class MDTraits_UTF16 : public MDTraits_UInt16
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UTF16"; };

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
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
		virtual void SetString(MDObject *Object, std::string Val);
		virtual Int32 GetInt(const MDObject *Object) const;
		virtual Int64 GetInt64(const MDObject *Object) const;
		virtual UInt32 GetUInt(const MDObject *Object) const;
		virtual UInt64 GetUInt64(const MDObject *Object) const;
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }

		virtual size_t ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};


	class MDTraits_BasicArray : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_BasicArray"; };

	protected:
		virtual void SetInt(MDObject *Object, Int32 Val);
		virtual void SetInt64(MDObject *Object, Int64 Val);
		virtual void SetUInt(MDObject *Object, UInt32 Val);
		virtual void SetUInt64(MDObject *Object, UInt64 Val);
		virtual void SetString(MDObject *Object, std::string Val);
		virtual Int32 GetInt(const MDObject *Object) const;
		virtual Int64 GetInt64(const MDObject *Object) const;
		virtual UInt32 GetUInt(const MDObject *Object) const;
		virtual UInt64 GetUInt64(const MDObject *Object) const;
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }

		virtual size_t ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_BasicStringArray : public MDTraits_BasicArray
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_BasicStringArray"; };

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};
	
	class MDTraits_UTF16String : public MDTraits_BasicStringArray
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UTF16String"; };

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};

	class MDTraits_StringArray : public MDTraits_UTF16String
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_StringArray"; };

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};

	class MDTraits_RawArray : public MDTraits_BasicArray
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_RawArray"; };

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};

	class MDTraits_UUID : public MDTraits_Raw
	{
	protected:
		static OutputFormatEnum DefaultFormat;			//!< Current default output format

	public:
		//! Does this trait take control of all sub-data and build values in the values own DataChunk?
		/*! The entire UUID is held locally */
		virtual bool HandlesSubdata(void) const { return true; };

		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_UUID"; };

		//! Set the default output format from a string and return an OutputFormatEnum value to use in future
		static OutputFormatEnum SetOutputFormat(std::string Format)
		{
			// We use the UUID class to do the formatting, so we need to get enum values from it
			// To do this we temorarily switch the UUID format to the requested format (to get the value), then change back to what it was before
			OutputFormatEnum OldFormat = mxflib::UUID::GetOutputFormat();
			DefaultFormat = mxflib::UUID::SetOutputFormat(Format);
			mxflib::UUID::SetOutputFormat(OldFormat);
			return DefaultFormat;
		}

		//! Set the default output format
		static void SetOutputFormat(OutputFormatEnum Format) { DefaultFormat = Format; }

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const { return GetString(Object, -1); }
		virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const;
	};

	class MDTraits_Label : public MDTraits_Raw
	{
	protected:
		static OutputFormatEnum DefaultFormat;			//!< Current default output format

	public:
		//! Does this trait take control of all sub-data and build values in the values own DataChunk?
		/*! The entire Label is held locally */
		virtual bool HandlesSubdata(void) const { return true; };

		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Label"; };

		//! Set the default output format from a string and return an OutputFormatEnum value to use in future
		static OutputFormatEnum SetOutputFormat(std::string Format)
		{
			// We use the UL class to do the formatting, so we need to get enum values from it
			// To do this we temorarily switch the UL format to the requested format (to get the value), then change back to what it was before
			OutputFormatEnum OldFormat = UL::GetOutputFormat();
			DefaultFormat = UL::SetOutputFormat(Format);
			UL::SetOutputFormat(OldFormat);
			return DefaultFormat;
		}

		//! Set the default output format
		static void SetOutputFormat(OutputFormatEnum Format) { DefaultFormat = Format; }

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const { return GetString(Object, -1); }
		virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const;
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
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};

	class MDTraits_RawArrayArray : public MDTraits_BasicArray
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_RawArrayArray"; };

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};

	class MDTraits_BasicCompound : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_BasicCompound"; };

	protected:
		// DRAGONS: What about all the other set and get functions?
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }

		virtual size_t ReadValue(MDObject *Object, const UInt8 *Buffer, size_t Size, int Count=0);
	};

	class MDTraits_BasicEnum : public MDTraits
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_BasicEnum"; };

	protected:
		virtual void SetInt(MDObject *Object, Int32 Val);
		virtual void SetInt64(MDObject *Object, Int64 Val);
		virtual void SetUInt(MDObject *Object, UInt32 Val);
		virtual void SetUInt64(MDObject *Object, UInt64 Val);
		virtual void SetString(MDObject *Object, std::string Val);
		virtual Int32 GetInt(const MDObject *Object) const;
		virtual Int64 GetInt64(const MDObject *Object) const;
		virtual UInt32 GetUInt(const MDObject *Object) const;
		virtual UInt64 GetUInt64(const MDObject *Object) const;
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};

	class MDTraits_Rational : public MDTraits_BasicCompound
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_Rational"; };

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};

	class MDTraits_TimeStamp : public MDTraits_BasicCompound
	{
	public:
		//! A unique name for this trait
		virtual std::string Name() const { return "mxflib::MDTraits_TimeStamp"; };

	protected:
		virtual void SetString(MDObject *Object, std::string Val);
		virtual std::string GetString(const MDObject *Object) const;
                //Overridden to avoid partial overriden virtual function issues with ICC
                virtual std::string GetString(const MDObject *Object, OutputFormatEnum Format) const { return GetString(Object); }
	};


}

#endif // MXFLIB__MDTRAITS_H
