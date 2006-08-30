/*! \file	types.h
 *	\brief	The main MXF data types
 *
 *	\version $Id: types.h,v 1.8 2006/08/30 15:30:26 matt-beard Exp $
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
	template <int SIZE> class Identifier /*: public RefCount<Identifier<SIZE> >*/
	{
	protected:
		UInt8 Ident[SIZE];
	public:
		Identifier(const UInt8 *ID = NULL) { if(ID == NULL) memset(Ident,0,SIZE); else memcpy(Ident,ID, SIZE); };
		Identifier(const SmartPtr<Identifier> ID) { ASSERT(SIZE == ID->Size()); if(!ID) memset(Ident,0,SIZE); else memcpy(Ident,ID->Ident, SIZE); };
		
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
		//! Prevent default construction
		UL();

	public:
		//! Construct a UL from a sequence of bytes
		/*! \note The byte string must contain at least 16 bytes or errors will be produced when it is used
		 */
		UL(const UInt8 *ID) : Identifier16(ID) {};

		//! Construct a UL as a copy of another UL
		UL(const SmartPtr<UL> ID) { if(!ID) memset(Ident,0,16); else memcpy(Ident,ID->Ident, 16); };

		//! Copy constructor
		UL(const UL &RHS) { memcpy(Ident,RHS.Ident, 16); };

		//! Construct a UL from an end-swapped UUID
		UL(const UUID &RHS) { operator=(RHS); }

		//! Construct a UL from an end-swapped UUID
		UL(const UUIDPtr &RHS) { operator=(*RHS); }

		//! Construct a UL from an end-swapped UUID
		UL(const UUID *RHS) { operator=(*RHS); }

		//! Fast compare a UL based on testing most-likely to fail bytes first
		/*! We use an unrolled loop with modified order for best efficiency
		 *  DRAGONS: There may be a slightly faster way that will prevent pipeline stalling, but this is fast enough!
		 */
		bool operator==(const UL &RHS) const
		{
			// Most differences are in the second 8 bytes so we check those first
			UInt8 const *pLHS = &Ident[8];
			UInt8 const *pRHS = &RHS.Ident[8];
			
			if(*pLHS++ != *pRHS++) return false;		// Test byte 8
			if(*pLHS++ != *pRHS++) return false;		// Test byte 9
			if(*pLHS++ != *pRHS++) return false;		// Test byte 10
			if(*pLHS++ != *pRHS++) return false;		// Test byte 11
			if(*pLHS++ != *pRHS++) return false;		// Test byte 12
			if(*pLHS++ != *pRHS++) return false;		// Test byte 13
			if(*pLHS++ != *pRHS++) return false;		// Test byte 14
			if(*pLHS != *pRHS) return false;			// Test byte 15

			// Now we test the first 8 bytes, but in reverse as the first 4 are almost certainly "06 0e 2b 34"
			// We use predecrement from the original start values so that the compiler will optimize the address calculation if possible
			pLHS = &Ident[8];
			pRHS = &RHS.Ident[8];

			if(*--pLHS != *--pRHS) return false;		// Test byte 7
			if(*--pLHS != *--pRHS) return false;		// Test byte 6
			if(*--pLHS != *--pRHS) return false;		// Test byte 5
			if(*--pLHS != *--pRHS) return false;		// Test byte 4
			if(*--pLHS != *--pRHS) return false;		// Test byte 3
			if(*--pLHS != *--pRHS) return false;		// Test byte 2
			if(*--pLHS != *--pRHS) return false;		// Test byte 1
			
			return (*--pLHS == *--pRHS);				// Test byte 0
		}

		//! Fast compare a UL based on testing most-likely to fail bytes first *IGNORING THE VERSION NUMBER*
		/*! We use an unrolled loop with modified order for best efficiency
		 *  DRAGONS: There may be a slightly faster way that will prevent pipeline stalling, but this is fast enough!
		 */
		bool Matches(const UL &RHS) const
		{
			// Most differences are in the second 8 bytes so we check those first
			UInt8 const *pLHS = &Ident[8];
			UInt8 const *pRHS = &RHS.Ident[8];
			
			if(*pLHS++ != *pRHS++) return false;		// Test byte 8
			if(*pLHS++ != *pRHS++) return false;		// Test byte 9
			if(*pLHS++ != *pRHS++) return false;		// Test byte 10
			if(*pLHS++ != *pRHS++) return false;		// Test byte 11
			if(*pLHS++ != *pRHS++) return false;		// Test byte 12
			if(*pLHS++ != *pRHS++) return false;		// Test byte 13
			if(*pLHS++ != *pRHS++) return false;		// Test byte 14
			if(*pLHS != *pRHS) return false;			// Test byte 15

			// Now we test the first 8 bytes, but in reverse as the first 4 are almost certainly "06 0e 2b 34"
			// We use predecrement from the original start values so that the compiler will optimize the address calculation if possible
			pLHS = &Ident[8];
			pRHS = &RHS.Ident[8];

			// Skip the UL version number
			--pLHS;
			--pRHS;

			if(*--pLHS != *--pRHS) return false;		// Test byte 6
			if(*--pLHS != *--pRHS) return false;		// Test byte 5
			if(*--pLHS != *--pRHS) return false;		// Test byte 4
			if(*--pLHS != *--pRHS) return false;		// Test byte 3
			if(*--pLHS != *--pRHS) return false;		// Test byte 2
			if(*--pLHS != *--pRHS) return false;		// Test byte 1
			
			return (*--pLHS == *--pRHS);				// Test byte 0
		}

		//! Set a UL from a UUID, does end swapping
		UL &operator=(const UUID &RHS);

		//! Set a UL from a UUID, does end swapping
		UL &operator=(const UUIDPtr &RHS) { return operator=(*RHS); }

		//! Set a UL from a UUID, does end swapping
		UL &operator=(const UUID *RHS) { return operator=(*RHS); }

		//! Produce a human-readable string in one of the "standard" formats
		std::string GetString(void) const
		{
			char Buffer[100];

			// Check which format should be used
			if( !(0x80&Ident[0]) )
			{	
				// This is a UL rather than a UUID packed into a UL datatype
				// Print as compact SMPTE format [060e2b34.rrss.mmvv.ccs1s2s3.s4s5s6s7]
				// Stored in the following 0-based index order: 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
				// (i.e. network byte order)
				sprintf (Buffer, "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x]",
							       Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
							       Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
						);
			}
			else
			{	
				// Half-swapped UUID
				// Print as compact GUID format {8899aabb-ccdd-eeff-0011-223344556677}
				sprintf (Buffer, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
							       Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15],
							       Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7]
						);
			}

			return std::string(Buffer);
		}
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
	public:
		//! Construct a new UUID with a new unique value
		UUID() { MakeUUID(Ident); };

		//! Construct a UUID from a sequence of bytes
		/*! \note The byte string must contain at least 16 bytes or errors will be produced when it is used
		 */
		UUID(const UInt8 *ID) : Identifier16(ID) {};

		//! Construct a UUID as a copy of another UUID
		UUID(const SmartPtr<UUID> ID) { if(!ID) memset(Ident,0,16); else memcpy(Ident,ID->Ident, 16); };

		//! Copy constructor
		UUID(const UUID &RHS) { memcpy(Ident,RHS.Ident, 16); };

		//! Construct a UUID from an end-swapped UL
		UUID(const UL &RHS) { operator=(RHS); }

		//! Construct a UUID from an end-swapped UL
		UUID(const ULPtr &RHS) { operator=(*RHS); }

		//! Construct a UUID from an end-swapped UL
		UUID(const UL *RHS) { operator=(*RHS); }

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

		//! Produce a human-readable string in one of the "standard" formats
		std::string GetString(void) const
		{
			char Buffer[100];

			// Check which format should be used
			if( !(0x80&Ident[8]) )
			{	// Half-swapped UL packed into a UUID datatype
				// Print as compact SMPTE format [bbaa9988.ddcc.ffee.00010203.04050607]
				// Stored with upper/lower 8 bytes exchanged
				// Stored in the following 0-based index order: 88 99 aa bb cc dd ee ff 00 01 02 03 04 05 06 07
				sprintf (Buffer, "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x]",
							       Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15],
							       Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7]
						);
			}
			else
			{	// UUID
				// Stored in the following 0-based index order: 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
				// (i.e. network byte order)
				// Print as compact GUID format {00112233-4455-6677-8899-aabbccddeeff}
				sprintf (Buffer, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
							       Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
							       Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
						);
			}

			return std::string(Buffer);
		}
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
			Int32 GCD = GreatestCommonDivisor();
			Numerator /= GCD;
			Denominator /= GCD;
		}
		
		//! Check for exact equality (not just the same ratio)
		inline bool operator==(const Rational &RHS)
		{
			return (Numerator == RHS.Numerator) && (Denominator == RHS.Denominator);
		}
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
		if(Remainder >= (Multiplier.Denominator/2)) Ret++;

		return Ret;
	}
}


#endif // MXFLIB__TYPES_H

