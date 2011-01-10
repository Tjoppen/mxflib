/*! \file	types.h
 *	\brief	The main MXF data types
 *
 *	\version $Id: types.h,v 1.10 2011/01/10 10:42:09 matt-beard Exp $
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

#ifndef MXFLIB__TYPES_H
#define MXFLIB__TYPES_H

// Ensure NULL is available
#include <stdlib.h>

// Standard library includes
#include <list>

#ifdef _WIN32
#pragma warning(disable : 4995) //turn off warnings about sprintf deprecated
#endif

/*                        */
/* Basic type definitions */
/*                        */

namespace mxflib
{
	typedef Int64 Length;				//!< Lenth of an item in bytes
	typedef Int64 Position;				//!< Position within an MXF file

	typedef UInt16 Tag;					//!< 2-byte tag for local sets

	//! Pair of UInt32 values
	typedef std::pair<UInt32, UInt32> U32Pair;
}


// Some string conversion utilities
namespace mxflib
{
	//! String version of a tag
	inline std::string Tag2String(Tag value)
	{
		char Buffer[8];
		sprintf(Buffer, "%02x.%02x", value >> 8, value & 0xff);
		return std::string(Buffer);
	}
}


namespace mxflib
{
	//! Dynamic enumeration type to be used to select output formats for GetString()
	/*! DRAGONS: Value -1 is never allocated and can be safely used for "not known" or other special purposes */
	typedef int OutputFormatEnum;
}


namespace mxflib
{
	template <int SIZE> class Identifier /*: public RefCount<Identifier<SIZE> >*/
	{
	protected:
		UInt8 Ident[SIZE];
	public:
		Identifier(const UInt8 *ID = NULL) { if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID, SIZE); };
		
		Identifier(const SmartPtr<Identifier> ID) { mxflib_assert(SIZE == ID->Size()); if(!ID) memset(Ident,0,SIZE); else memcpy(Ident,ID->Ident, SIZE); };
		
		//! Set the value of the Identifier
		void Set(const UInt8 *ID = NULL) { if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID, SIZE); };

		//! Set an individual byte of the identifier
		void Set(int Index, UInt8 Value) { if(Index < SIZE) Ident[Index] = Value; };

		//! Get a read-only pointer to the identifier value
		const UInt8 *GetValue(void) const { return Ident; };

		//! Get the size of the identifier
		int Size(void) const { return SIZE; };

		bool operator!(void) const
		{
			int i;
			for(i=0; i<SIZE; i++) if(Ident[i]) return false;
			return true;
		}

		bool operator<(const Identifier& Other) const
		{
			if(Other.Size() < SIZE ) return (memcmp(Ident, Other.Ident, Other.Size()) < 0);
			                    else return (memcmp(Ident, Other.Ident, SIZE) < 0);
		}

		bool operator==(const Identifier& Other) const
		{
			if(Other.Size() != SIZE ) return false;
								 else return (memcmp(Ident, Other.Ident, SIZE) == 0);
		}

		bool operator!=(const Identifier& Other) const { return !operator==(Other); }

		std::string GetString(void) const
		{
			std::string Ret;
			char Buffer[8];

			for(int i=0; i<SIZE; i++)
			{
				sprintf(Buffer, "%02x", Ident[i]);
				if(i!=0) Ret += " ";

				Ret += Buffer;
			}

			return Ret;
		}

		bool IsSMPTEKey() const 
		{
			if( SIZE==16 &&
				(Ident[0]==0x06) 
				&& ((Ident[1]==0x0E || Ident[1]==0x0A))  )	
				return true; 
			else  return false;
		};

	};

}

namespace mxflib
{
	// Forward declare UUID
	class UUID;

	//! A smart pointer to a UUID object
	typedef SmartPtr<UUID> UUIDPtr;
	
	//! 16-byte identifier
	typedef Identifier<16> Identifier16;

	//! Universal Label class with optimized comparison and string formatting
	class UL : public RefCount<UL>, public Identifier16
	{
	private:
//		//! Prevent default construction
//		UL();

	protected:
		//! Helper for string constructors - sets Ident from a string
		void SetIdentifier(std::string const & strID);

		static OutputFormatEnum DefaultFormat;			//!< Current default output format

		static OutputFormatEnum OutputFormatBraced;		//!< Dynamic enum value allocated to {00112233-4455-6677-8899-aabbccddeeff} or [8899aabb.ccdd.eeff.00112233.44556677] format
		static OutputFormatEnum OutputFormatHex;		//!< Dynamic enum value allocated to 00112233-4455-6677-8899-aabbccddeeff format
		static OutputFormatEnum OutputFormat0xHex;		//!< Dynamic enum value allocated to 0x00112233445566778899aabbccddeeff format
		static OutputFormatEnum OutputFormatDottedHex;	//!< Dynamic enum value allocated to 00.11.22.33.44.55.66.77.88.99.aa.bb.cc.dd.ee.ff format
		static OutputFormatEnum OutputFormatURN;		//!< Dynamic enum value allocated to urn:uuid or urn:smpte:ul format
		static OutputFormatEnum OutputFormatx_ul;		//!< Dynamic enum value allocated to urn:uuid or urn:x-ul format
		static OutputFormatEnum OutputFormatOID;		//!< Dynamic enum value allocated to BER OID format
		static OutputFormatEnum OutputFormatAAF;		//!< Dynamic enum value allocated to AAF aafUID_t format

	public:
		//! Empty constructor - avoid using!
		/*! DRAGONS: For some reason it is not possible to build an MDObjectULList::value_type without this, even though both arguments are supplied to its constructor!
		 */
		UL() { memset(Ident, 0, 16); }

		//! Construct a UL from a sequence of bytes
		/*! \note The byte string must contain at least 16 bytes or errors will be produced when it is used
		 */
		UL(const UInt8 *ID) : Identifier16(ID) {};
		UL( std::string const & ID);

		//! Construct a UL as a copy of another UL
		UL(const SmartPtr<UL> ID) { if(!ID) memset(Ident,0,16); else memcpy(Ident,ID->Ident, 16); };

		//! Copy constructor
		UL(const UL &RHS) { memcpy(Ident,RHS.Ident, 16); };

		//! Construct a UL from an end-swapped UUID
		UL(const UUID &RHS) { operator=(RHS); }

		//! Construct a UL from an end-swapped UUID

		UL(const UUIDPtr &RHS) { operator=(*((UUID*)RHS)); }

		//! Construct a UL from an end-swapped UUID
		UL(const UUID *RHS) { operator=(*RHS); }

		//! Fast compare a UL based on testing most-likely to fail bytes first
		bool operator==(const UL &RHS) const;

		//! Simple != implementation
		bool operator!=(const UL& Other) const { return !operator==(Other); }

		//! Fast compare of effective values of UL based on testing most-likely to fail bytes first
		/*! DRAGONS: This comparison ignores the UL version number and group coding */
		bool Matches(const UL &RHS) const;

		//! Set a UL from a UUID, does end swapping
		UL &operator=(const UUID &RHS);

		//! Set a UL from a UUID, does end swapping
		UL &operator=(const UUIDPtr &RHS) { return operator=(*((UUID*)RHS)); }

		//! Set a UL from a UUID, does end swapping
		UL &operator=(const UUID *RHS) { return operator=(*RHS); }

		//! Set the default output format from a string and return an OutputFormatEnum value to use in future
		static OutputFormatEnum SetOutputFormat(std::string Format);

		//! Set the default output format
		static void SetOutputFormat(OutputFormatEnum Format) { DefaultFormat = Format; }

		//! Get the current default output format
		static OutputFormatEnum GetOutputFormat(void) { return DefaultFormat; }

		//! Produce a human-readable string in one of the "standard" formats
		std::string GetString(OutputFormatEnum Format = -1) const
		{
			return FormatString(Ident, Format);
		}

		//! Format using one of the "standard" formats
		static std::string FormatString(UInt8 const *Ident, OutputFormatEnum Format = -1);
	};

	//! A smart pointer to a UL object
	typedef SmartPtr<UL> ULPtr;

	//! A list of smart pointers to UL objects
	typedef std::list<ULPtr> ULList;
}


namespace mxflib
{
	//! Universally Unique Identifier class with string formatting
	class UUID : public Identifier16, public RefCount<UUID>
	{
	protected:
		static OutputFormatEnum DefaultFormat;			//!< Current default output format

		static OutputFormatEnum OutputFormatBraced;		//!< Dynamic enum value allocated to {00112233-4455-6677-8899-aabbccddeeff} or [8899aabb.ccdd.eeff.00112233.44556677] format
		static OutputFormatEnum OutputFormatHex;		//!< Dynamic enum value allocated to 00112233-4455-6677-8899-aabbccddeeff format
		static OutputFormatEnum OutputFormat0xHex;		//!< Dynamic enum value allocated to 0x00112233445566778899aabbccddeeff format
		static OutputFormatEnum OutputFormatDottedHex;	//!< Dynamic enum value allocated to 00.11.22.33.44.55.66.77.88.99.aa.bb.cc.dd.ee.ff format
		static OutputFormatEnum OutputFormatURN;		//!< Dynamic enum value allocated to urn:uuid or urn:smpte:ul format
		static OutputFormatEnum OutputFormatx_ul;		//!< Dynamic enum value allocated to urn:uuid or urn:x-ul format
		static OutputFormatEnum OutputFormatOID;		//!< Dynamic enum value allocated to BER OID format
		static OutputFormatEnum OutputFormatAAF;		//!< Dynamic enum value allocated to AAF aafUID_t format

	public:
		//! Construct a new UUID with a new unique value
		UUID() 
		{ 
			MakeUUID(Ident); 
		}

		//! Construct a UUID from a sequence of bytes
		/*! \note The byte string must contain at least 16 bytes or errors will be produced when it is used
		 */
		UUID(const UInt8 *ID) : Identifier16(ID) {}

		//! Construct a UUID as a copy of another UUID
		UUID(const SmartPtr<UUID> ID) 
		{ 
			if(!ID) memset(Ident,0,16); else memcpy(Ident,ID->Ident, 16); 
		}

		//! Copy constructor
		UUID(const UUID &RHS)
		{ 
			memcpy(Ident,RHS.Ident, 16);
		}

		//! Construct a UUID from an end-swapped UL
		UUID(const UL &RHS)
		{ 
			operator=(RHS);
		}

		//! Construct a UUID from an end-swapped UL
		UUID(const ULPtr &RHS) 
		{ 
			operator=(*RHS); 
		}

		//! Construct a UUID from an end-swapped UL
		UUID(const UL *RHS) 
		{ 
			operator=(*RHS); 
		}

		//! Set a UUID from a UL, does end swapping
		UUID &operator=(const UL &RHS)
		{
			memcpy(Ident, &RHS.GetValue()[8], 8);
			memcpy(&Ident[8], RHS.GetValue(), 8);
			return *this;
		}

		//! Set a UUID from a UL, does end swapping
		UUID &operator=(const ULPtr &RHS) { return operator=(*RHS); }

		//! Set a UUID from a UL, does end swapping
		UUID &operator=(const UL *RHS) { return operator=(*RHS); }

		//! Set the default output format from a string and return an OutputFormatEnum value to use in future
		static OutputFormatEnum SetOutputFormat(std::string Format);

		//! Set the default output format
		static void SetOutputFormat(OutputFormatEnum Format) { DefaultFormat = Format; }

		//! Get the current default output format
		static OutputFormatEnum GetOutputFormat(void) { return DefaultFormat; }

		//! Produce a human-readable string in one of the "standard" formats
		std::string GetString(OutputFormatEnum Format = -1) const
		{
			return FormatString(Ident, Format);
		}

		//! Format using one of the "standard" UUID formats
		static std::string FormatString(UInt8 const *Ident, OutputFormatEnum Format = -1);
	};
}


namespace mxflib
{
	//! Set a UL from a UUID, does end swapping
	/*  DRAGONS: Defined here as we need UUID to be fully defined */
	inline UL &UL::operator=(const UUID &RHS)
	{
		memcpy(Ident, &RHS.GetValue()[8], 8);
		memcpy(&Ident[8], RHS.GetValue(), 8);
		return *this;
	}
}

namespace mxflib
{
	//! Represent an AUID as an endian-sensitive struct as in AAFSDK API (i.e. half-swapped again)
	class IDAUstruct
	{
	private:
		UInt8 reordered[16];

	public:
		IDAUstruct( const UInt8* UL_Data, bool AsLittle = true )
		{
			memcpy( reordered+8, UL_Data, 8 );

			if( AsLittle )
			{
				reordered[3] = UL_Data[8];
				reordered[2] = UL_Data[9];
				reordered[1] = UL_Data[10];
				reordered[0] = UL_Data[11];

				reordered[5] = UL_Data[12];
				reordered[4] = UL_Data[13];

				reordered[7] = UL_Data[14];
				reordered[6] = UL_Data[15];
			}
			else
			{
				memcpy( reordered, UL_Data+8, 4 );
				memcpy( reordered+4, UL_Data+12, 2 );
				memcpy( reordered+6, UL_Data+14, 2 );
			}
		};

		const UInt8* Data(){ return reordered; }
	};
}

namespace mxflib
{
	typedef Identifier<32> Identifier32;
	class UMID : public Identifier32, public RefCount<UMID>
	{
	public:
		//! Construct a new UMID either from a sequence of bytes, or as a NULL UMID (32 zero bytes)
		/*! \note The byte string must contain at least 32 bytes or errors will be produced when it is used
		 */
		UMID(const UInt8 *ID = NULL) : Identifier32(ID) {};

		//! Construct a UMID from a sequence of bytes
		/*! \note The byte string must contain at least 16 bytes or errors will be produced when it is used
		 */
		UMID(const SmartPtr<UMID> ID) { if(!ID) memset(Ident,0,32); else memcpy(Ident,ID->Ident, 32); };

		//! Copy constructor
		UMID(const UMID &RHS) { memcpy(Ident,RHS.Ident, 16); };

		//! Get the UMID's instance number
		/*! \note The number returned interprets the instance number as big-endian */
		UInt32 GetInstance(void) const
		{
			return (Ident[13] << 16) | (Ident[14] << 8) | Ident[15];
		}

		//! Set the UMID's instance number
		/*! \note The number is set as big-endian */
		//	TODO: Should add an option to generate a random instance number
		void SetInstance(int Instance, int Method = -1)
		{
			UInt8 Buffer[4];
			PutU32(Instance, Buffer);

			// Set the instance number
			memcpy(&Ident[13], &Buffer[1], 3);

			// Set the method if a new one is specified
			if(Method >= 0)
			{
				Ident[11] = (Ident[11] & 0xf0) | Method;
			}
		}

		//! Set the UMID's material number
		void SetMaterial( ULPtr aUL )
		{
			// Set the instance number
			memcpy(&Ident[16], aUL->GetValue(), 16);

			// DRAGONS: Is this the right method for a UL?
			Ident[11] = (Ident[11] & 0x0f) | 2<<4;
		}
	};

	//! A smart pointer to a UMID object
	typedef SmartPtr<UMID> UMIDPtr;
}


namespace mxflib
{
	//! Structure for holding fractions
	class Rational
	{
	public:
		Int32 Numerator;				//!< Numerator of the fraction (top number)
		Int32 Denominator;				//!< Denominator of the fraction (bottom number)
	
		//! Build an empty Rational
		Rational() : Numerator(0), Denominator(0) {};

		//! Initialise a Rational with a value
		Rational(Int32 Num, Int32 Den) : Numerator(Num), Denominator(Den) {};

		//! Determine the greatest common divisor using the Euclidean algorithm
		Int32 GreatestCommonDivisor(void) 
		{
			Int32 a;			//! The larger value for Euclidean algorithm
			Int32 b;			//! The smaller value for Euclidean algorithm
			
			// Set the larger and smaller values
			if(Numerator>Denominator)
			{
				a = Numerator;
				b = Denominator;
			}
			else
			{
				a = Denominator;
				b = Numerator;
			}
			
			// Euclid's Algorithm
			while (b != 0) 
			{
				Int32 Temp = b;
				b = a % b;
				a = Temp;
			}

			// Return the GCD
			return a;
		}

		//! Reduce a rational to its lowest integer form
		void Reduce(void)
		{
			if((Numerator != 0) && (Denominator != 0))
			{
				Int32 GCD = GreatestCommonDivisor();
				Numerator /= GCD;
				Denominator /= GCD;
			}
		}
		
		//! Check for exact equality (not just the same ratio)
		inline bool operator==(const Rational &RHS)
		{
			return (Numerator == RHS.Numerator) && (Denominator == RHS.Denominator);
		}
		inline bool operator!=(const Rational &RHS)
		{
			return !operator==(RHS);
		}

		//! Set the value of the rational from a string
		void SetString(std::string Value);

		//! Get the value of this rational as a string
		std::string GetString(void) const;
	};

	//! Determine the greatest common divisor of a 64-bit / 64-bit pair using the Euclidean algorithm
	inline Int64 GreatestCommonDivisor( Int64 Numerator, Int64 Denominator )
	{
		Int64 a;			//! The larger value for Euclidean algorithm
		Int64 b;			//! The smaller value for Euclidean algorithm
		
		// Set the larger and smaller values
		if(Numerator>Denominator)
		{
			a = Numerator;
			b = Denominator;
		}
		else
		{
			a = Denominator;
			b = Numerator;
		}
		
		// Euclid's Algorithm
		while (b != 0) 
		{
			Int64 Temp = b;
			b = a % b;
			a = Temp;
		}

		// Return the GCD
		return a;
	}

	//! Divide one rational by another
	inline Rational operator/(const Rational Dividend, const Rational Divisor)
	{
		// Multiply the numerator of the dividend by the denominator of the divisor (getting a 64-bit result)
		Int64 Numerator = Dividend.Numerator;
		Numerator *= Divisor.Denominator;

		// Multiply the denominator of the dividend by the numerator of the divisor (getting a 64-bit result)
		Int64 Denominator = Dividend.Denominator;
		Denominator *= Divisor.Numerator;

		// Calculate the greated common divisor
		Int64 GCD = GreatestCommonDivisor(Numerator, Denominator);
		
		Numerator /= GCD;
		Denominator /= GCD;

		// Lossy-reduction of any fractions that won't fit in a 32-bit/ 32-bit rational
		// TDOD: Check if this is ever required
		while( (Numerator & INT64_C(0xffffffff00000000)) ||  (Denominator & INT64_C(0xffffffff00000000)))
		{
			Numerator >>= 1;
			Denominator >>= 1;
		}

		return Rational(static_cast<Int32>(Numerator), static_cast<Int32>(Denominator));
	}

	//! Multiply one rational by another
	inline Rational operator*(const Rational Multiplicand, const Rational Multiplier)
	{
		// Multiply the numerator of the multiplicand by the numerator of the multiplier (getting a 64-bit result)
		Int64 Numerator = Multiplicand.Numerator;
		Numerator *= Multiplier.Numerator;

		// Multiply the denominator of the multiplicand by the denominator of the multiplier (getting a 64-bit result)
		Int64 Denominator = Multiplicand.Denominator;
		Denominator *= Multiplier.Denominator;

		// Calculate the greated common divisor
		Int64 GCD = GreatestCommonDivisor(Numerator, Denominator);
		
		Numerator /= GCD;
		Denominator /= GCD;

		// Lossy-reduction of any fractions that won't fit in a 32-bit/ 32-bit rational
		// TDOD: Check if this is ever required
		while( (Numerator & INT64_C(0xffffffff00000000)) ||  (Denominator & INT64_C(0xffffffff00000000)))
		{
			Numerator >>= 1;
			Denominator >>= 1;
		}

		return Rational(static_cast<Int32>(Numerator), static_cast<Int32>(Denominator));
	}

	//! Multiply a position by a rational
	inline Position operator*(const Position Multiplicand, const Rational Multiplier)
	{
		Position Ret = Multiplicand * Multiplier.Numerator;

		Int64 Remainder = Ret % Multiplier.Denominator;

        Ret = Ret / Multiplier.Denominator;

		// Round up any result that is nearer to the next position
		if( Remainder >= ((Multiplier.Denominator+1)/2)) Ret++;

		return Ret;
	}
}


namespace mxflib
{
	class Label;

	// A Smart pointer to a Label
	typedef SmartPtr<Label> LabelPtr;

	//! A UL or end-swapped UUID label 
	class Label : public RefCount<Label>
	{
	protected:
		UL Value;										//!< The value of this label
		UInt8 Mask[16];									//!< Mask of ignore bits, each set bit flags a bit to be ignored when comparing
		bool NonZeroMask;								//!< True if there is a non-zero mask
		std::string Name;								//!< The XML-Tag-valid name for this label
		std::string Detail; 							//!< The human-readable description for this label

	protected:
		//! Type of the Label map (map of UL to LabelPtr)
		typedef std::map<UL, LabelPtr> LabelULMap;

		//! Type of the Label multi-map (map of UL to LabelPtr)
		typedef std::multimap<UL, LabelPtr> LabelULMultiMap;

		//! Map of all existing labels that don't use masking
		static LabelULMap LabelMap;

		//! Map of all existing labels that use masking - this is a multimap to allow the same base with different masks
		static LabelULMultiMap LabelMultiMap;

	protected:
		// Private constructor - to build a new label one of the Insert() functions must be called
		Label(std::string LabelName, std::string Detail, const UInt8 *LabelUL, const UInt8 *LabelMask) 
			: Value(LabelUL), Detail(Detail)
		{
			Init(LabelName, LabelMask);
		}

		// Private constructor - to build a new label one of the Insert() functions must be called
		Label(std::string LabelName, std::string Detail, const UUID &LabelULasUUID, const UInt8 *LabelMask) 
			: Value(LabelULasUUID), Detail(Detail)
		{
			Init(LabelName, LabelMask);
		}

		// Constructor common part - called by constructors
		void Init(std::string LabelName, const UInt8 *LabelMask)
		{
			Name = LabelName;

			if(LabelMask)
			{
				memcpy(Mask, LabelMask, 16);
				NonZeroMask = true;
			}
			else
			{
				memset(Mask, 0, 16);
				NonZeroMask = false;
			}
		}

	public:
		//! Get the value of this Label as a UL
		const ULPtr GetValue(void) { ULPtr Ret = new UL( Value ); return Ret; }

		//! Get the name of this label
		std::string GetName(void) { return Name; };

		//! Get the detail for this label - if no detail, get the name
		std::string GetDetail(void) { return Name + " - " + Detail; };

		//! Return true if this label uses a (non-zero) mask
		bool HasMask(void) { return NonZeroMask; }

	public:
		//! Construct and add a label from a byte array
		/*! \return true if succeeded, else false
		 */
		static bool Insert(std::string Name, std::string Detail, const UInt8 *LabelValue, const UInt8 *LabelMask = NULL);

		//! Construct and add a label from a UL smart pointer
		/*! \return true if succeeded, else false
		 */
		static bool Insert(std::string Name, std::string Detail, const ULPtr &LabelValue, const UInt8 *LabelMask = NULL);

		//! Construct and add a label from a UL reference
		/*! \return true if succeeded, else false
		 */
		static bool Insert(std::string Name, std::string Detail, const UL &LabelValue, const UInt8 *LabelMask = NULL);

		//! Construct and add a label from a UUID smart pointer
		/*! \return true if succeeded, else false
		 */
		static bool Insert(std::string Name, std::string Detail, const UUIDPtr &LabelValue, const UInt8 *LabelMask = NULL);

		//! Construct and add a label from a UUID reference
		/*! \return true if succeeded, else false
		 */
		static bool Insert(std::string Name, std::string Detail, const mxflib::UUID &LabelValue, const UInt8 *LabelMask = NULL);

	public:
		//! Find a label with a given value, from a UL reference
		static LabelPtr Find(const UL &LabelValue);

		//! Find a label with a given value, from a ULPtr
		static LabelPtr Find(const ULPtr &LabelValue)
		{
			return Find(*LabelValue);
		}

		//! Find a label with a given value, from the label bytes
		static LabelPtr Find(const UInt8 *LabelValue)
		{
			/* Make a value UL and use that in the main search */
			UL ValueUL(LabelValue);
			return Find(ValueUL);
		}

		//! Find a label with a given value, from a text Name
		/*! \param Name the name to find - linear search - don't use this if performance matters
		 */
		static LabelPtr Find(const std::string Name);

	};


	//types for Edgecode components

	enum _EdgeType_t
	{	
		kEtInvalid		= -1,
		kEtNull			= 0,
		kEtKeycode		= 1,
		kEtEdgenum4		= 2,
		kEtEdgenum5		= 3,
		kEtHeaderSize	= 8
	} 	;

	typedef enum _EdgeType_t EdgeType_t;


	enum _FilmType_t
	{
		kFtInvalid	= -1,
		kFtNull		= 0,
		ktFt35MM	= 1,
		ktFt16MM	= 2,
		kFt8MM		= 3,
		kFt65MM		= 4
	} ;

	typedef enum _FilmType_t FilmType_t;
}



#endif // MXFLIB__TYPES_H

