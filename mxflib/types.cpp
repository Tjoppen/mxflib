//types.cpp
//This file adds support for parsing a lot of UL text formats into an Indentifier<16>


#include <stdlib.h>
#include <string>
#include <sstream>


#include "mxflib/mxflib.h"
using namespace mxflib;
#include "types.h"


//! Fast compare a UL based on testing most-likely to fail bytes first
/*! We use an unrolled loop with modified order for best efficiency
 *  There may be a slightly faster way that will prevent pipeline stalling, but this is fast enough!
 */
bool mxflib::UL::operator==(const UL &RHS) const
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


//! Fast compare of effective values of UL based on testing most-likely to fail bytes first
/*! DRAGONS: This comparison ignores the UL version number and group coding
 *	We use an unrolled loop with modified order for best efficiency.
 *	There may be a slightly faster way that will prevent pipeline stalling, but this is fast enough!
 */
bool mxflib::UL::Matches(const UL &RHS) const
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
	
	if(*--pLHS != *--pRHS)						// Test byte 5
	{
		// DRAGONS: If the UL is a group UL then the we ignore coding mismatches
		// The following code uses a pre-decrement to test byte 4, so we can see if this is a group key,
		// but if it is a group key we need to increment it again so that the following pre-decrement works right!
		if(*--pLHS != 0x02) return false;
		else ++pLHS;
	}

	if(*--pLHS != *--pRHS) return false;		// Test byte 4
	if(*--pLHS != *--pRHS) return false;		// Test byte 3
	if(*--pLHS != *--pRHS) return false;		// Test byte 2
	if(*--pLHS != *--pRHS) return false;		// Test byte 1
	if(*--pLHS != *--pRHS) return false;		// Test byte 0

	// So far the values match, but did we skip version and coding bytes when this is a UUID rather than a UL?
	// If the first byte of the value is not 0x06 then we must go back and check the skipped bytes
	if(*pLHS == 0x06) return true;

	// Do the remaining two comparisons
	// DRAGONS: We do the byute 5 comparison even if we have already done it because it would take as long to
	//          determine if we have already done it or not!
	if(Ident[8] != RHS.Ident[8]) return false;
	return (Ident[5] == RHS.Ident[5]);
}


static inline bool isHex(char ch)
{
	if( ch>='0' && ch<='9') return true;
	if( ch>='a' && ch<='f') return true;
	if( ch>='A' && ch<='F') return true;
	return false;
}


static inline unsigned int PairofCharToHex( const char *pch)  
{
	unsigned int tmp=0;

	if(*pch>='A' && *pch<='F')
		tmp=(unsigned char)((*pch-L'A')+10);
	else if(*pch>='a' && *pch<='f')
		tmp=(unsigned char)((*pch-L'a')+10);
	else if(*pch>='0' && *pch<='9')
		tmp=(unsigned char)(*pch-L'0');
	else
	{
		//assert(false,"Error in SMPTE label string: ");
	}
	tmp=tmp<<4;

	pch++;
	if(*pch>=L'A' && *pch<=L'F')
		tmp+=(unsigned char)((*pch-L'A')+10);
	else if(*pch>='a' && *pch<='f')
		tmp+=(unsigned char)((*pch-L'a')+10);
	else if(*pch>='0' && *pch<='9')
		tmp+=(unsigned char)(*pch-L'0');
	else   //chose to treat as a single digit number
	{
		tmp=tmp>>4; //so restore the LSB
	}
	return tmp;
}


bool is_urn_oid( const char * puuid) 
{

	const char header[]="urn:oid:";
	for(int i=0;i<8;i++)
		if( tolower(puuid[i]!=header[i] ) ) //check for correct header 
			return false;

	const char * pch=puuid+8;

	if(*pch=='\0')
		return false;

	while(*pch!='\0')
	{
		while(*pch==' ') pch++;
		if(!(*pch=='.' || (*pch>='0' && *pch<='9')))
			return false;
		pch++;
	}

	return true;

}



/**************************************************
' A label can be formatted as an URN (uuid, x-ul or oid), or as various lesser formats
' URNmode == 0  urn:uuid: or urn:x-ul:
' URNmode == 1  urn:uuid: or urn:oid: (BER OID encoding is undone for output)
' URNmode == 2  {uuid} or [x-ul]
' URNmode == 3  {uuid}
' URNmode == 4  0x
' URNmode == 5  dotted SMPTE
' URNmode == 6  AAF aafUID_t (possibly WRONG)
' URNmode == 7  urn:uuid: 

**********************************************************/
enum _label_formats
{
	urn_x_ul,
	urn_oid,
	x_ul,
	uuid,
	hex_ul,
	SMPTE_dots,
	AAF_aafUID_t,
	urn_uuid,
	mxflib_style,
	unknown
};

typedef enum _label_formats _LabelFormat;



bool CheckFormat( const char * puuid, const char * pattern)
{

	const char * pch=puuid;
	int index=0;
	char checktype;

	while(pattern[index]!='Z')
	{
		checktype='\0';
		while(*pch==' ') pch++;;
		if(*pch>='0' && *pch<='9' ) checktype='h';
		else if(*pch>='A' && *pch<='F' ) checktype='h';
		else if(*pch>='a' && *pch<='f' ) checktype='h';
		else checktype=*pch;

		if(checktype==pattern[index])
		{
			pch++;
			index++;
		}
		else
			return false;

	}
	return true;
}


static bool is_uuid( const char * puuid) { return  CheckFormat(puuid,  "{hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh}Z");};
static bool is_dotted( const char * puuid) { return  CheckFormat(puuid,  "hh.hh.hh.hh.hh.hh.hh.hh.hh.hh.hh.hh.hh.hh.hh.hhZ");};
static bool is_urn_uuid( const char * puuid)  { return  CheckFormat(puuid,  "urn:uuih:hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhhZ");};
//note that there is confusion in the on the above as the algotrihm assumes that the d is a hex number and so fails 
//- thus we make th pattern contanin a hex code.
static bool is_urn_x_ul( const char * puuid) { return  CheckFormat(puuid,  "urn:x-ul:hhhhhhhh.hhhh.hhhh.hhhhhhhh.hhhhhhhhZ");};
static bool is_urn_x_ul_12byte( const char * puuid) { return  CheckFormat(puuid,  "urn:x-ul:hhhhhhhh.hhhh.hhhh.hhhhhhhhZ");};

static bool is_x_ul( const char * puuid) { return  CheckFormat(puuid,  "[hhhhhhhh.hhhh.hhhh.hhhhhhhh.hhhhhhhh]Z");};
static bool is_hex_ul( const char * puuid) { return  CheckFormat(puuid,  "hxhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhZ");};

UL::UL( std::string const & ID) : Identifier16( NULL )
{
	int i;
	const char * puuid=ID.c_str();

	if(puuid==NULL)
		return;

	_LabelFormat incoming_Format=unknown;

	//first detect the type of uuid we are dealing with.

	if(is_urn_x_ul( puuid ))
		incoming_Format=urn_x_ul;
	else if (is_urn_x_ul_12byte( puuid ))
		incoming_Format=urn_x_ul;
	else if (is_urn_oid( puuid ))
		incoming_Format=urn_oid;
	else if (is_uuid( puuid ))
		incoming_Format=uuid;
	else if (is_hex_ul( puuid ))
		incoming_Format=hex_ul;
	else if (is_dotted( puuid ))
		incoming_Format=SMPTE_dots;
	else if (is_urn_uuid( puuid ))
		incoming_Format=urn_uuid;
	else if (is_x_ul( puuid ))
		incoming_Format=x_ul;

	if ( incoming_Format==unknown)
		return ;

	//remove whitespace
	char buff[(16*3)+10];
	const char * pch=puuid;
	char * pch2=buff;

	while(isspace(*pch) && *pch!='\0')
	{
		pch++;
	}
	while(!isspace(*pch) && *pch!='\0')
	{
		*pch2++=*pch++;
	}
	*pch2='\0';
	//now buff hods the data with no whitespace.

	if(incoming_Format==urn_oid)
	{
		mxflib_assert(0); //this is not implemented
		/*
		//urn:oid:1.3.52.2.6.1.1.13.1.1.1.1.1
		pch=buff+8;
		//lint -e722 (Info -- Suspicious use of ;	
		Ident[0]=0x06;
		Ident[1]=0x0E;
		Ident[2]=(unsigned long long)(unsigned long)atol(pch)*40;
		while(*pch!='\0' && *pch++!='.' );  
		Ident[2]+=(unsigned long long)(unsigned long)atol(pch);
		while(*pch!='\0' && *pch++!='.');   
		Ident[3]=(unsigned long long)(unsigned long)atol(pch);
		while(*pch!='\0' && *pch++!='.');  
		Ident[4]= (unsigned long long)(unsigned long)atol(pch);
		while(*pch!='\0' && *pch++!='.');  
		Ident[5]=  (unsigned long long)(unsigned long)atol(pch);
		while(*pch!='\0' && *pch++!='.'); 
		Ident[6]= (unsigned long long)(unsigned long)atol(pch);
		while(*pch!='\0' && *pch++!='.');  
		Ident[7]=  (unsigned long long)(unsigned long)atol(pch);
		while(*pch!='\0' && *pch++!='.');  

		//lint +e722 (Info -- Suspicious use of ;	

		int index=8;

		while(*pch!='\0' )
		{
			Ident[index++]= (unsigned long)atol(pch);
			while(*pch!='\0' && *pch++!='.');  //lint !e722 (Info -- Suspicious use of ;
		}

		*/

	}


	if(incoming_Format==SMPTE_dots)
	{
		pch=buff;

		int bytesdone=0;

		for(i=0;i<8;i++)
		{
			Ident[i]= PairofCharToHex(pch);
			pch+=3;
		}

		if(IsSMPTEKey())
		{
			int index=8;
			Ident[index]=0;
			unsigned int tmp;

			int remaining_bytes;
			if(Ident[1]==0x0a)
				remaining_bytes=4;
			else
				remaining_bytes=8; 

			while(bytesdone<remaining_bytes)
			{
				tmp=PairofCharToHex(pch);

				if(tmp>=0x80)
					Ident[index]=(Ident[index]<<7)+(tmp & 0x7f);
				else
				{
					Ident[index]=(Ident[index]<<7)+tmp;
					index++;
					if(index<16)
						Ident[index]=0;
				}
				bytesdone++;
				pch+=3;
			}

			// Now if there are extra bytes on the end for internal use
			// Note that it is only valid to have the extra bytes if in SMPTE Hex format
			if( isHex(*pch))
			{
				tmp=PairofCharToHex(pch);
				if(tmp>=0x80)
					Ident[index]=(Ident[index]<<7)+(tmp & 0x7f);
				else
				{
					Ident[index]=(Ident[index]<<7)+tmp;
					index++;
				}
				pch+=3;

				if( isHex(*pch))
				{
					tmp=PairofCharToHex(pch);
					if(tmp>=0x80)
						Ident[index]=(Ident[index]<<7)+(tmp & 0x7f);
					else
					{
						Ident[index]=(Ident[index]<<7)+tmp;
						index++;
					}
					pch+=3;
				}
			}

			if(Ident[1]==0x0a)
				for(int j=12;j<16;j++)
					Ident[j]=0;
		}
		else
		{
			for(i=8;i<16;i++)
			{
				Ident[i]= PairofCharToHex(pch);
				pch+=3;
			}
		}
	}

	if(incoming_Format==uuid)
	{
		//{07020201-0103-0000-060E-2B3401010102}
		pch=buff+1;
		int bytesdone=0;
		unsigned int tmp;
		int index=8;
		Ident[index]=0;

		pch+=19;

		for(i=0;i<8;i++)
		{	Ident[i]=PairofCharToHex(pch);
		pch+=2;
		if(i==1)
			pch++;
		}

		pch=buff+1;

		if(IsSMPTEKey())
		{
			while(bytesdone<8)
			{
				tmp=PairofCharToHex(pch);

				if(tmp>=0x80)
					Ident[index]=(Ident[index]<<7)+(tmp & 0x7f);
				else
				{
					Ident[index]=(Ident[index]<<7)+tmp;
					index++;
					if(index<8)
						Ident[index]=0;
				}
				bytesdone++;
				pch+=2;
				if(bytesdone==4 || bytesdone==6)
					pch++;  // skip the - character
			}
			pch++;
			Ident[0]=PairofCharToHex(pch);
			pch+=2;
			Ident[1]=PairofCharToHex(pch);
			pch+=3;
			Ident[2]=PairofCharToHex(pch);
			pch+=2;
			Ident[3]=PairofCharToHex(pch);
			pch+=2;
			for(i=4;i<8;i++)
			{
				Ident[i]=  PairofCharToHex(pch);
				pch+=2;
			}
		}
		else
		{
			for(i=0;i<8;i++)
			{	Ident[i+8]=PairofCharToHex(pch);
			pch+=2;
			if(i==3 || i==5)
				pch++;
			}
		}

	}

	if(incoming_Format==urn_uuid)
	{
		//urn:uuid:96C46992-4F62-11D3-A022-006094EB75CB


		pch=buff+9;

		for( i=0;i<16;i++)
		{
			Ident[i]=PairofCharToHex(pch);
			if(*pch=='\0' || *(pch+1)=='\0')
				if(i<15)
					return ;

			pch+=2;
			if(i==3 || i==5 || i==7 || i==9)
				pch++;

		}

	}
	if(incoming_Format==hex_ul)
	{
		pch=buff;
		pch+=2;  // skip over 0x

		int bytesdone=0;

		for( i=0;i<8;i++)
		{
			Ident[i]= PairofCharToHex(pch);
			pch+=2;
		}

		if(IsSMPTEKey())
		{

			int index=8;
			Ident[index]=0;
			unsigned int tmp;

			int remaining_bytes;
			if(Ident[1]==0x0a)
				remaining_bytes=4;
			else
				remaining_bytes=8;

			while(bytesdone<remaining_bytes)
			{
				tmp=PairofCharToHex(pch);

				if(tmp>=0x80)
					Ident[index]=(Ident[index]<<7)+(tmp & 0x7f);
				else
				{
					Ident[index]=(Ident[index]<<7)+tmp;
					index++;
					if(index<16)
						Ident[index]=0;
				}
				bytesdone++;
				pch+=2;
			}

			if(Ident[1]==0x0a)
				for(int j=12;j<16;j++)
					Ident[j]=0;
		}
		else
		{
			for( i=8;i<16;i++)
			{
				Ident[i]= PairofCharToHex(pch);
				pch+=2;
			}
		}
	}


	if(incoming_Format==urn_x_ul || incoming_Format==x_ul)
	{
		//urn:x-ul:060E2B34.0101.0102.07020201.01030000

		//or x_ul is [060E2B34.0101.0102.07020201.01030000]
		if(incoming_Format==urn_x_ul)
			pch=buff+9;
		else
			pch=buff+1;

		int bytesdone=0;
		unsigned int tmp;
		int index=8;
		Ident[index]=0;

		if(*pch=='0' && *(pch+1)=='\0')  //shorthand for null UL
		{
			for( i=0;i<4;i++)
				Ident[i]=0;
			return ;
		}
		for( i=0;i<8;i++)
		{
			Ident[i]=PairofCharToHex(pch);
			pch+=2;
			if(i==3 || i==5 || i==7)
				pch++;
		}


		if(!IsSMPTEKey())
		{
			for( i=8;i<16;i++)
			{
				Ident[i]=PairofCharToHex(pch);
				pch+=2;
				if(i==11)
					pch++;
			}
		}
		else
		{

			int remaining_bytes;
			if(Ident[1]==0x0a)
				remaining_bytes=4;
			else
				remaining_bytes=8;


			while(bytesdone<remaining_bytes)
			{
				tmp=PairofCharToHex(pch);

				if(tmp>=0x80)
					Ident[index]=(Ident[index]<<7)+(tmp & 0x7f);
				else
				{
					Ident[index]=(Ident[index]<<7)+tmp;
					index++;
					if(index<16)
						Ident[index]=0;
				}
				bytesdone++;
				pch+=2;
				if(bytesdone==4 )
					pch++;  // skip the . character
			}
			if(Ident[1]==0x0a)
				for(int j=12;j<16;j++)
					Ident[j]=0;
		}

	}

}



namespace
{
	// Map of bit counts for each byte value
	static UInt8 BitCount[256] = 
	{
		0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
	};
}

//! Find a label with a given value, from a UL reference
/*! If more than one masked label matches, the value with the least mask bits is returned. 
 *  If more than one have the same number of mask bits, the last one found will be returned - which this is is undefined.
 */
LabelPtr Label::Find(const UL &LabelValue)
{
	// Search for an exact match
	LabelULMap::iterator it = LabelMap.find(LabelValue);

	// If we found an exact match, return it
	if(it != LabelMap.end()) return (*it).second;

	/* Now we have to do the long-hand search for masked values */

	// DRAGONS: This can be simplified by seeking to a point in the list before the first possible match - but this code is good enough
	
	int SmallestBitMask = 255;
	LabelPtr BestMatch;

	LabelULMultiMap::iterator mit = LabelMultiMap.begin();
	while(mit != LabelMultiMap.end())
	{
		// Only check those entries that have a mask (non-masked matches will have been found with the "find" call
		if((*mit).second->NonZeroMask)
		{
			// Set up pointers to this entry's mask and value
			const UInt8 *p1 = (*mit).first.GetValue();
			const UInt8 *pMask = (*mit).second->Mask;

			// Set up a pointer to the search value
			const UInt8 *p2 = LabelValue.GetValue();

			// Compare each byte
			int Count = 16;
			do
			{
				// Do a masked compare, first use XOR to select which bits don't match, then clear the masked ones
				// Any bytes that match the test will end up zero, otherwise the failing bit will be set
				if( ((*p1++) ^ (*p2++)) & ~(*pMask++) ) break;
			} while(--Count);

			// If we counted all the way to zero, we found a match - so count the mask bits
			if(!Count)
			{
				int BitMaskSize = 0;

				pMask = (*mit).second->Mask;
				Count = 16;
//while(Count--) { printf("%02x[%d].", *pMask, BitCount[*pMask]); BitMaskSize += BitCount[*pMask++]; };
				while(Count--) { BitMaskSize += BitCount[*pMask++]; };

//printf("Found a match with %d mask bits = %s\n", BitMaskSize, (*mit).second->GetName().c_str());
				if(BitMaskSize <= SmallestBitMask)
				{
//printf("Best so far\n");
					SmallestBitMask = BitMaskSize;
					BestMatch = (*mit).second;
				}
			}	
		}

		mit++;
	}

	// Return the best match - which may be NULL
	return BestMatch;
}


//! Map of all existing labels that don't use masking
Label::LabelULMap Label::LabelMap;

//! Map of all existing labels that use masking - this is a multimap to allow the same base with different masks
Label::LabelULMultiMap Label::LabelMultiMap;

//! Find a label with a given value, from a text Name
LabelPtr Label::Find(const std::string Name)
{
	// linear search - don't use this if performance matters

	LabelPtr Ret;

	LabelULMap::iterator it = LabelMap.begin();

	while( it != LabelMap.end() )
	{
		if( (*it).second->Name == Name ) return (*it).second;
		it++;
	}

	return Ret;
}


//! Construct and add a label from a byte array
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const UInt8 *LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, LabelValue, LabelMask);

	if(LabelMask)
	{
//#printf("Adding %s to the masked map\n", Name.c_str());
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
//#printf("Adding %s to the unmasked map\n", Name.c_str());
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}


//! Construct and add a label from a UL smart pointer
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const ULPtr &LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, LabelValue->GetValue(), LabelMask);

	if(LabelMask)
	{
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}


//! Construct and add a label from a UL reference
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const UL &LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, LabelValue.GetValue(), LabelMask);

	if(LabelMask)
	{
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}


//! Construct and add a label from a UUID smart pointer
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const UUIDPtr &LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, *LabelValue, LabelMask);

	if(LabelMask)
	{
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}


//! Construct and add a label from a UUID reference
/*! \return true if succeeded, else false
	*/
bool Label::Insert(std::string Name, std::string Detail, const mxflib::UUID &LabelValue, const UInt8 *LabelMask /*=NULL*/)
{
	// Create the new Label
	// If the insert succeeds the label map will take ownership, 
	// if not it will only be owned by this pointer and will be deleted at the end of this function
	LabelPtr NewLabel = new Label(Name, Detail, LabelValue, LabelMask);

	if(LabelMask)
	{
		// Masked labels go in the multi-map and this will always succeed
		LabelMultiMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel));
		return true;
	}
	else
	{
		// Try and insert this new label - if that succeeded, return true
		return LabelMap.insert(LabelULMap::value_type(NewLabel->Value, NewLabel)).second;
	}
}


//! Get the value of this rational as a string
std::string Rational::GetString(void) const
{
	std::stringstream Str;

	Str << Numerator << ":" << Denominator;

	return Str.str();
}

//! Set the value of the rational from a string
void Rational::SetString(std::string Value)
{
	size_t Pos = Value.find_first_of("/:,");

	if(Pos == std::string::npos)
	{
		Denominator = 1;
	}
	else
	{
		Denominator = atoi(Value.substr(Pos + 1).c_str());
	}

	Numerator = atoi(Value.c_str());
}


//! Current default output format for ULs
OutputFormatEnum mxflib::UL::DefaultFormat = -1;

/* Static members holding allocated dynamic enum values for UL output formats */
OutputFormatEnum UL::OutputFormatBraced = -1;
OutputFormatEnum UL::OutputFormatHex = -1;
OutputFormatEnum UL::OutputFormat0xHex = -1;
OutputFormatEnum UL::OutputFormatDottedHex = -1;
OutputFormatEnum UL::OutputFormatURN = -1;
OutputFormatEnum UL::OutputFormatx_ul = -1;
OutputFormatEnum UL::OutputFormatOID = -1;
OutputFormatEnum UL::OutputFormatAAF = -1;

//! Set the default output format from a string and return an OutputFormatEnum value to use in future
OutputFormatEnum UL::SetOutputFormat(std::string Format)
{
	/* Valid formats:
	 * "Braced", "Bracketed" or "[]" or "{}"			[00112233.4455.6677.8899aabb.ccddeeff] or {8899aabb-ccdd-eeff-0011-223344556677}
	 * "Hex"											00112233.4455.6677.8899aabb.ccddeeff
	 * "HexNumber" or "0x"								0x00112233445566778899aabbccddeeff
	 * "DottedHex" or "Dotted"							00.11.22.33.44.55.66.77.88.99.aa.bb.cc.dd.ee.ff
	 * "URN"											urn:uuid or urn:smpte:ul
	 * "urn:x-ul" or "x-ul"								urn:uuid or urn:x-ul
	 * "OID"											BER OID
	 * "AAF" or "aafUID_t"								aafUID_t format
	 */

	// Do the actual comparisons all in upper case to make case-insensitive
	std::transform(Format.begin(), Format.end(), Format.begin(), toupper);

	// We will set and/or return one of our static members, which depends on the format.
	// This is done by using a pointer to the one to apply - we default to the braced format
	OutputFormatEnum *FormatRef = &UL::OutputFormatBraced;

	if((Format == "BRACED") || (Format == "BRACKETED") || (Format == "[]") || (Format == "{}"))
	{
		FormatRef = &UL::OutputFormatBraced;
	}
	else if(Format == "HEX")
	{
		FormatRef = &UL::OutputFormatHex;
	}
	else if((Format == "HEXNUMBER") || (Format == "0X"))
	{
		FormatRef = &UL::OutputFormat0xHex;
	}
	else if((Format == "DOTTEDHEX") || (Format == "DOTTED"))
	{
		FormatRef = &UL::OutputFormatDottedHex;
	}
	else if(Format == "URN")
	{
		FormatRef = &UL::OutputFormatURN;
	}
	else if((Format == "URN:X-UL") || (Format == "X-UL"))
	{
		FormatRef = &UL::OutputFormatx_ul;
	}
	else if(Format == "OID")
	{
		FormatRef = &UL::OutputFormatOID;
	}
	else if((Format == "AAF") || (Format == "AAFUID_T"))
	{
		FormatRef = &UL::OutputFormatAAF;
	}
	else
	{
		error("Unknown UL format \"%s\" specified in call to UL::SetFormat()\n", Format.c_str());
		return -1;
	}

	// If we don't yet have an enum value for this format, ask for one
	if((*FormatRef) == -1) *FormatRef = MDTraitsEnum::GetNewEnum();

	// Set the format
	DefaultFormat = *FormatRef;

	// Return the numeric value for future SetFormat() or GetString() calls
	return DefaultFormat;
}


//! Format using one of the "standard" UUID formats
std::string mxflib::UL::FormatString(UInt8 const *Ident, OutputFormatEnum Format /*=-1*/)
{
	std::string Ret;

	// If we are not simply returning the hex, lookup the string
	if(GetLabelFormat() != LabelFormatHex)
	{
		LabelPtr Label =  Label::Find(Ident);
		if(Label) 
		{
			Ret = Label->GetDetail();

			// If we are just getting the text - return it
			if(    (GetLabelFormat() == LabelFormatText) 
				|| ((GetLabelFormat() == LabelFormatTextHexMask) && !Label->HasMask())) return Ret;
		}
	}

	// ...else emit underlying identifier

	char Buffer[100];

	// If no format specified, use current default
	if(Format == -1)
	{
		if(DefaultFormat == -1)
		{
			if(OutputFormatBraced == -1) OutputFormatBraced = MDTraitsEnum::GetNewEnum();
			DefaultFormat = OutputFormatBraced;
		}
		Format = DefaultFormat;
	}

	if(Format == OutputFormatBraced)
	{
		// Check which format should be used
		if( (Ident[0] & 0x80) == 0)
		{	
			// This is a UL rather than a half-swapped UUID
			// Return as compact SMPTE format [060e2b34.rrss.mmvv.ccs1s2s3.s4s5s6s7]
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
			// Return as compact GUID format {8899aabb-ccdd-eeff-0011-223344556677}
			sprintf (Buffer, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
							   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15],
							   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7]
					);
		}
	}
	else if(Format == OutputFormatHex)
	{
		// Output un-swapped in raw mode
		// Return as hex 00112233.4455.6677.8899aabb.ccddeeff
		// Stored in the following 0-based index order: 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
		// (i.e. network byte order)
		sprintf (Buffer, "%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x",
						   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
						   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
				);
	}
	else if((Format == OutputFormatURN) || (Format == OutputFormatx_ul))
	{
		// Check which format should be used
		if( (Ident[8] & 0x80) == 0 )
		{	// UL
			// Print as per SMPTE 2029-2009 "urn:smpte:ul:00112233.44556677.8899aabb.ccddeeff", or old x-ul version
			// Stored with upper/lower 8 bytes exchanged
			sprintf (Buffer, "urn:%s:%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x",
							   (Format == OutputFormatURN) ? "smpte:ul" : "x-ul",
							   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
							   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
					);
		}
		else
		{	// Half-swapped UUID packed into a UL datatype
			// Print as RFC 4122 format "urn:uuid:8899aabb-ccdd-eeff-0011-223344556677"
			sprintf (Buffer, "urn:uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
							   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15],
							   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7]
					);
		}
	}
	else if(Format == OutputFormat0xHex)
	{
		warning("Unsupported OutputFormat \"HexNumber\" in UL::FormatString()\n");
		return FormatString(Ident, OutputFormatBraced);
	}
	else if(Format == OutputFormatDottedHex)
	{
		// Output as hex 00.11.22.33.44.55.66.77.88.99.aa.bb.cc.dd.ee.ff
		sprintf (Buffer, "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
						   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
						   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
				);
	}
	else if(Format == OutputFormatOID)
	{
		warning("Unsupported OutputFormat \"OID\" in UL::FormatString()\n");
		return FormatString(Ident, OutputFormatBraced);
	}
	else if(Format == OutputFormatAAF)
	{
		warning("Unsupported OutputFormat \"AAF\" in UL::FormatString()\n");
		return FormatString(Ident, OutputFormatBraced);
	}
	else
	{
		error("Unknown OutputFormat %d in UL::FormatString()\n", (int)Format);
		return FormatString(Ident, OutputFormatBraced);
	}

	if(Ret.length() == 0)
	{
		Ret = Buffer;
	}
	else
	{
		Ret += " ";
		Ret += Buffer;
	}

	return Ret;
}


//! Current default output format for UUIDs
OutputFormatEnum mxflib::UUID::DefaultFormat = -1;

/* Static members holding allocated dynamic enum values for UUID output formats */
OutputFormatEnum mxflib::UUID::OutputFormatBraced = -1;
OutputFormatEnum mxflib::UUID::OutputFormatHex = -1;
OutputFormatEnum mxflib::UUID::OutputFormat0xHex = -1;
OutputFormatEnum mxflib::UUID::OutputFormatDottedHex = -1;
OutputFormatEnum mxflib::UUID::OutputFormatURN = -1;
OutputFormatEnum mxflib::UUID::OutputFormatx_ul = -1;
OutputFormatEnum mxflib::UUID::OutputFormatOID = -1;
OutputFormatEnum mxflib::UUID::OutputFormatAAF = -1;


//! Set the default output format from a string and return an OutputFormatEnum value to use in future
OutputFormatEnum mxflib::UUID::SetOutputFormat(std::string Format)
{
	/* Valid formats:
	 * "Braced", "Bracketed" or "[]" or "{}"			{00112233-4455-6677-8899-aabbccddeeff} or [8899aabb.ccdd.eeff.00112233.44556677]
	 * "Hex"											00112233-4455-6677-8899-aabbccddeeff
	 * "HexNumber" or "0x"								0x00112233445566778899aabbccddeeff
	 * "DottedHex" or "Dotted"							00.11.22.33.44.55.66.77.88.99.aa.bb.cc.dd.ee.ff
	 * "URN"											urn:uuid or urn:smpte:ul
	 * "urn:x-ul" or "x-ul"								urn:uuid or urn:x-ul
	 * "OID"											BER OID
	 * "AAF" or "aafUID_t"								aafUID_t format
	 */

	// Do the actual comparisons all in upper case to make case-insensitive
	std::transform(Format.begin(), Format.end(), Format.begin(), toupper);

	// We will set and/or return one of our static members, which depends on the format.
	// This is done by using a pointer to the one to apply - we default to the braced format
	OutputFormatEnum *FormatRef = &mxflib::UUID::OutputFormatBraced;

	if((Format == "BRACED") || (Format == "BRACKETED") || (Format == "[]") || (Format == "{}"))
	{
		FormatRef = &mxflib::UUID::OutputFormatBraced;
	}
	else if(Format == "HEX")
	{
		FormatRef = &mxflib::UUID::OutputFormatHex;
	}
	else if((Format == "HEXNUMBER") || (Format == "0X"))
	{
		FormatRef = &mxflib::UUID::OutputFormat0xHex;
	}
	else if((Format == "DOTTEDHEX") || (Format == "DOTTED"))
	{
		FormatRef = &mxflib::UUID::OutputFormatDottedHex;
	}
	else if(Format == "URN")
	{
		FormatRef = &mxflib::UUID::OutputFormatURN;
	}
	else if((Format == "URN:X-UL") || (Format == "X-UL"))
	{
		FormatRef = &mxflib::UUID::OutputFormatx_ul;
	}
	else if(Format == "OID")
	{
		FormatRef = &mxflib::UUID::OutputFormatOID;
	}
	else if((Format == "AAF") || (Format == "AAFUID_T"))
	{
		FormatRef = &mxflib::UUID::OutputFormatAAF;
	}
	else
	{
		error("Unknown UL format \"%s\" specified in call to UUID::SetFormat()\n", Format.c_str());
		return -1;
	}

	// If we don't yet have an enum value for this format, ask for one
	if((*FormatRef) == -1) *FormatRef = MDTraitsEnum::GetNewEnum();

	// Set the format
	DefaultFormat = *FormatRef;

	// Return the numeric value for future SetFormat() or GetString() calls
	return DefaultFormat;
}


//! Format using one of the "standard" UUID formats
std::string mxflib::UUID::FormatString(UInt8 const *Ident, OutputFormatEnum Format /*=-1*/)
{
	std::string Ret;
	char Buffer[100];

	// Add label text for un-swapped ULs
	// TODO: Check that it is appropriate to a) do this when "BracedHex" was requested and b) only for un-swapped ULs
	if(((Ident[8] & 0x80) == 0) && ( (Ident[0]==0x06) && (Ident[1]==0x0e) && (Ident[2]==0x2b) && (Ident[3]==0x34) ))
	{	// DRAGONS: UNSWAPPED UL as used by AULref

		// If we are not simply returning the hex, return the string
		if(GetLabelFormat() != LabelFormatHex)
		{
			LabelPtr Label =  Label::Find(Ident);
			if(Label) 
			{
				Ret = Label->GetDetail();

				// If we are just getting the text - return it
				if(    (GetLabelFormat() == LabelFormatText) 
					|| ((GetLabelFormat() == LabelFormatTextHexMask) && !Label->HasMask())) return Ret;
			}
		}
	}

	// If no format specified, use current default
	if(Format == -1)
	{
		if(DefaultFormat == -1)
		{
			if(OutputFormatBraced == -1) OutputFormatBraced = MDTraitsEnum::GetNewEnum();
			DefaultFormat = OutputFormatBraced;
		}
		Format = DefaultFormat;
	}

	if(Format == OutputFormatBraced)
	{
		// Check which format should be used
		if(Ident[8] & 0x80)
		{	// UUID
			// Stored in the following 0-based index order: 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
			// (i.e. network byte order)
			// Print as compact GUID format {00112233-4455-6677-8899-aabbccddeeff}
			sprintf (Buffer, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
							   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
							   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
					);
		}
		else if( (Ident[0]==0x06) && (Ident[1]==0x0e) && (Ident[2]==0x2b) && (Ident[3]==0x34) )
		{	// DRAGONS: UNSWAPPED UL as used by AULref
			// Return as compact GUID format {060e2b34-4455-6677-8899-aabbccddeeff}
			sprintf (Buffer, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
							   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
							   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
					);
		}
		else
		{	// Half-swapped UL packed into a UUID datatype
			// Print as compact SMPTE format [8899aabb.ccdd.eeff.00112233.44556677]
			// Stored with upper/lower 8 bytes exchanged
			// Stored in the following 0-based index order: 88 99 aa bb cc dd ee ff 00 11 22 33 44 55 66 77
			sprintf (Buffer, "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x]",
							   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15],
							   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7]
					);
		}
	}
	else if(Format == OutputFormatHex)
	{
		// UUID
		// Stored in the following 0-based index order: 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
		// (i.e. network byte order)
		// Print as compact GUID format {00112233-4455-6677-8899-aabbccddeeff}
		sprintf (Buffer, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
						  Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
						  Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
				);
	}			
	else if((Format == OutputFormatURN) || (Format == OutputFormatx_ul))
	{
		// Check which format should be used
		if( (Ident[8] & 0x80) == 0)
		{
			if( (Ident[0]==0x06) && (Ident[1]==0x0e) && (Ident[2]==0x2b) && (Ident[3]==0x34) )
			{
				// DRAGONS: UNSWAPPED UL as used by AULref
				// Print as per SMPTE 2029-2009 "urn:smpte:ul:00112233.44556677.8899aabb.ccddeeff", or old x-ul version
				sprintf (Buffer, "urn:%s:%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x",
								   (Format == OutputFormatURN) ? "smpte:ul" : "x-ul",
								   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
								   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
						);
			}
			else
			{
				// Half-swapped UL packed into a UUID datatype
				// Print as per SMPTE 2029-2009 "urn:smpte:ul:8899aabb.ccddeeff.00112233.44556677", or old x-ul version
				// Stored with upper/lower 8 bytes exchanged
				// Stored in the following 0-based index order: 88 99 aa bb cc dd ee ff 00 11 22 33 44 55 66 77
				sprintf (Buffer, "urn:%s:%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x",
								   (Format == OutputFormatURN) ? "smpte:ul" : "x-ul",
								   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15],
								   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7]
						);
			}
		}
		else
		{	// UUID
			// Stored in the following 0-based index order: 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
			// (i.e. network byte order)
			// Print as RFC 4122 format "urn:uuid:00112233-4455-6677-8899-aabbccddeeff"
			sprintf (Buffer, "urn:uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
							   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
							   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
					);
		}
	}
	else if(Format == OutputFormat0xHex)
	{
		warning("Unsupported OutputFormat \"HexNumber\" in UUID::FormatString()\n");
		return FormatString(Ident, OutputFormatBraced);
	}
	else if(Format == OutputFormatDottedHex)
	{
		// Output as hex 00.11.22.33.44.55.66.77.88.99.aa.bb.cc.dd.ee.ff
		sprintf (Buffer, "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
						   Ident[0], Ident[1], Ident[2], Ident[3], Ident[4], Ident[5], Ident[6], Ident[7],
						   Ident[8], Ident[9], Ident[10], Ident[11], Ident[12], Ident[13], Ident[14], Ident[15]
				);
	}
	else if(Format == OutputFormatOID)
	{
		warning("Unsupported OutputFormat \"OID\" in UUID::FormatString()\n");
		return FormatString(Ident, OutputFormatBraced);
	}
	else if(Format == OutputFormatAAF)
	{
		warning("Unsupported OutputFormat \"AAF\" in UUID::FormatString()\n");
		return FormatString(Ident, OutputFormatBraced);
	}
	else
	{
		error("Unknown OutputFormat %d in UUID::FormatString()\n", (int)Format);
		return FormatString(Ident, OutputFormatBraced);
	}

	// Allow for 2-part return value
	if(Ret.length() == 0)
	{
		Ret = Buffer;
	}
	else
	{
		Ret += " ";
		Ret += Buffer;
	}

	return Ret;
}
