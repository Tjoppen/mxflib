/*! \file	deftypes.cpp
 *	\brief	Defines known types
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

#include "mxflib.h"

extern "C"
{
#include "Klv.h"
#include "sopSAX.h"
}

#include "stdarg.h"


using namespace mxflib;

/* SAX functions */
extern "C" void DefTypes_startElement(void *user_data, const char *name, const char **attrs);
extern "C" void DefTypes_endElement(void *user_data, const char *name);
extern "C" void DefTypes_warning(void *user_data, const char *msg, ...);
extern "C" void DefTypes_error(void *user_data, const char *msg, ...);
extern "C" void DefTypes_fatalError(void *user_data, const char *msg, ...);

/*
** Our SAX handler
*/
sopSAXHandler DefTypes_SAXHandler = {
    (startElementSAXFunc) DefTypes_startElement,		/* startElement */
    (endElementSAXFunc) DefTypes_endElement,			/* endElement */
    (warningSAXFunc) DefTypes_warning,					/* warning */
    (errorSAXFunc) DefTypes_error,						/* error */
    (fatalErrorSAXFunc) DefTypes_fatalError,			/* fatalError */
};

typedef enum
{
	DEFTYPES_STATE_START = 0,
	DEFTYPES_STATE_STARTED,					/*!< Inside the 'MXFTypes' tag */
	DEFTYPES_STATE_BASIC,					/*!< Processing 'Basic' section */
	DEFTYPES_STATE_INTERPRETATION,			/*!< Processing 'Interpretation' section */
	DEFTYPES_STATE_MULTIPLE,				/*!< Processing 'Multiple' section */
	DEFTYPES_STATE_COMPOUND,				/*!< Processing 'Compound' section */
	DEFTYPES_STATE_COMPOUNDITEMS,			/*!< Addind items to a 'Compound' */
	DEFTYPES_STATE_END,						/*!< Finished the dictionary */
	DEFTYPES_STATE_ERROR					/*!< Skip all else - error condition */
} DefTypesStateState;

typedef struct
{
	DefTypesStateState	State;				/*!< State machine current state */
	MDTypePtr			CurrentCompound;	/*!< The current compound being build (or NULL) */
	char				CompoundName[65];	/*!< Name of the current compound (or "") */
} DefTypesState;


//! Type to map type names to their handling traits
typedef std::map<std::string,MDTraitsPtr> TraitsMapType;

//! Map of type names to thair handling traits
TraitsMapType TraitsMap;

//! Build the map of all known traits
void DefineTraits(void)
{
	// Not a real type, but the default for basic types
	TraitsMap.insert(TraitsMapType::value_type("Default-Basic", new MDTraits_Raw));

	// Not a real type, but the default for array types
	TraitsMap.insert(TraitsMapType::value_type("Default-Array", new MDTraits_BasicArray));

	// Not a real type, but the default for compound types
	TraitsMap.insert(TraitsMapType::value_type("Default-Compound", new MDTraits_BasicCompound));


	TraitsMap.insert(TraitsMapType::value_type("Int8", new MDTraits_Int8));
	TraitsMap.insert(TraitsMapType::value_type("Uint8", new MDTraits_Uint8));
	TraitsMap.insert(TraitsMapType::value_type("Int16", new MDTraits_Int16));
	TraitsMap.insert(TraitsMapType::value_type("Uint16", new MDTraits_Uint16));
	TraitsMap.insert(TraitsMapType::value_type("Int32", new MDTraits_Int32));
	TraitsMap.insert(TraitsMapType::value_type("Uint32", new MDTraits_Uint32));
	TraitsMap.insert(TraitsMapType::value_type("Int64", new MDTraits_Int64));
	TraitsMap.insert(TraitsMapType::value_type("Uint64", new MDTraits_Uint64));

	TraitsMap.insert(TraitsMapType::value_type("ISO7", new MDTraits_ISO7));
	TraitsMap.insert(TraitsMapType::value_type("UTF16", new MDTraits_UTF16));

	TraitsMap.insert(TraitsMapType::value_type("ISO7String", new MDTraits_BasicStringArray));
	TraitsMap.insert(TraitsMapType::value_type("UTF16String", new MDTraits_BasicStringArray));
	TraitsMap.insert(TraitsMapType::value_type("Uint8Array", new MDTraits_RawArray));

	TraitsMap.insert(TraitsMapType::value_type("LabelCollection", new MDTraits_RawArrayArray));

	TraitsMap.insert(TraitsMapType::value_type("Rational", new MDTraits_Rational));
	TraitsMap.insert(TraitsMapType::value_type("TimeStamp", new MDTraits_TimeStamp));
}


//! Load types from the specified XML definitions
/*! \return 0 if all OK
 *! \return -1 on error
 */
int mxflib::LoadTypes(char *TypesFile)
{
	// State data block passed through SAX parser
	DefTypesState State;

	// Define the known traits
	// Test before calling as two partial definition files could be loaded!
	if(TraitsMap.empty()) DefineTraits();

	State.State = DEFTYPES_STATE_START;
	State.CurrentCompound = NULL;
	State.CompoundName[0] = '\0';

	// Parse the file
	sopSAXParseFile(&DefTypes_SAXHandler, &State, TypesFile);

	// DRAGONS - we should test for an error condition!


	// Finally ensure we have a valid "Unknown" type
	if(!MDType::Find("Unknown"))
	{
		MDType::AddInterpretation("Unknown", MDType::Find("Uint8Array"));
	}

	return 0;
}

//! SAX callback - Deal with start tag of an element
void DefTypes_startElement(void *user_data, const char *name, const char **attrs)
{
	DefTypesState *State = (DefTypesState*)user_data;
	int this_attr = 0;

/* DEBUG */
	{
		debug("Element : %s\n", name);

		if(attrs != NULL)
		{
			while(attrs[this_attr])
			{
				debug("  Attribute : %s = \"%s\"\n", attrs[this_attr], attrs[this_attr+1]);
				this_attr += 2;
			}
		}
	}
/* /DEBUG */

	switch(State->State)
	{
		/* Skip if all has gone 'belly-up' */
		case DEFTYPES_STATE_ERROR:
		default:
			return;

		case DEFTYPES_STATE_START:
		{
			if(strcmp(name,"MXFTypes") != 0)
			{
				error("Types definitions file does not start with tag <MXFTypes>\n");
				State->State = DEFTYPES_STATE_ERROR;
				return;
			}

			State->State = DEFTYPES_STATE_STARTED;
			break;
		}

		case DEFTYPES_STATE_STARTED:
		{
			if(strcmp(name, "Basic") == 0)
			{
				State->State = DEFTYPES_STATE_BASIC;
			}
			else if(strcmp(name, "Interpretation") == 0)
			{
				State->State = DEFTYPES_STATE_INTERPRETATION;
			}
			else if(strcmp(name, "Multiple") == 0)
			{
				State->State = DEFTYPES_STATE_MULTIPLE;
			}
			else if(strcmp(name, "Compound") == 0)
			{
				State->State = DEFTYPES_STATE_COMPOUND;
			}
			else
			{
				error("Unexpected types definitions section tag <%s>\n", name);
				State->State = DEFTYPES_STATE_ERROR;
			}
			break;
		}

		case DEFTYPES_STATE_END:
		{
			error("Unexpected types definition tag <%s> after final end tag\n", name);
			State->State = DEFTYPES_STATE_ERROR;
			break;
		}

		case DEFTYPES_STATE_BASIC:
		{
			const char *Detail = "";
			int Size = 1;
			bool Endian = false;
			
			/* Process attributes */
			if(attrs != NULL)
			{
				this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = val;
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "endian") == 0)
					{
						if(strcasecmp(val,"yes") == 0) Endian = true;
					}
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						error("Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
					}
				}
			}

			MDTypePtr Ptr = MDType::AddBasic(name, Size);
			if(Endian) Ptr->SetEndian(true);

			MDTraitsPtr Traits = TraitsMap[name];
			if(!Traits) Traits = TraitsMap["Default-Basic"];

			if(Traits) Ptr->SetTraits(Traits);

			break;
		}

		case DEFTYPES_STATE_INTERPRETATION:
		{
			const char *Detail = "";
			const char *Base = "";
			int Size = 0;
			
			/* Process attributes */
			if(attrs != NULL)
			{
				this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = val;
					}
					else if(strcmp(attr, "base") == 0)
					{
						Base = val;
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						error("Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
					}
				}
			}

			MDTypePtr BaseType = MDType::Find(Base);
			if(!BaseType)
			{
				error("Type \"%s\" specifies unknown base type \"%s\"\n", name, Base);
			}
			else
			{
				MDTypePtr Ptr = MDType::AddInterpretation(name, BaseType, Size);

				MDTraitsPtr Traits = TraitsMap[name];

				// If we don't have specific traits for this type
				// it will inherit the base type's traits
				if(Traits) Ptr->SetTraits(Traits);
			}

			break;
		}

		case DEFTYPES_STATE_MULTIPLE:
		{
			const char *Detail = "";
			const char *Base = "";
			MDArrayClass Class = ARRAYARRAY;
			int Size = 0;
			
			/* Process attributes */
			if(attrs != NULL)
			{
				this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = val;
					}
					else if(strcmp(attr, "base") == 0)
					{
						Base = val;
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "type") == 0)
					{
						if(strcasecmp(val, "Collection") == 0) Class = ARRAYCOLLECTION;
					}
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						error("Unexpected attribute \"%s\" in basic type \"%s\"\n", attr, name);
					}
				}
			}

			MDTypePtr BaseType = MDType::Find(Base);
			if(!BaseType)
			{
				error("Type \"%s\" specifies unknown base type \"%s\"\n", name, Base);
			}
			else
			{
				MDTypePtr Ptr = MDType::AddArray(name, BaseType, Size);
				if(Class == ARRAYCOLLECTION) Ptr->SetArrayClass(ARRAYCOLLECTION);

				MDTraitsPtr Traits = TraitsMap[name];
				if(!Traits) Traits = TraitsMap["Default-Array"];
				if(Traits) Ptr->SetTraits(Traits);
			}

			break;
		}

		case DEFTYPES_STATE_COMPOUND:
		{
			const char *Detail = "";
			
			/* Process attributes */
			if(attrs != NULL)
			{
				this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = val;
					}
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						error("Unexpected attribute \"%s\" in compound type \"%s\"\n", attr, name);
					}
				}
			}

			MDTypePtr Ptr = MDType::AddCompound(name);

			MDTraitsPtr Traits = TraitsMap[name];
			if(!Traits) Traits = TraitsMap["Default-Compound"];

			if(Traits) Ptr->SetTraits(Traits);

			State->State = DEFTYPES_STATE_COMPOUNDITEMS;

			State->CurrentCompound = Ptr;
			strncpy(State->CompoundName,name,64);

			break;
		}

		case DEFTYPES_STATE_COMPOUNDITEMS:
		{
			const char *Detail = "";
			const char *Type = "";
			int Size = 0;
			
			/* Process attributes */
			if(attrs != NULL)
			{
				this_attr = 0;
				while(attrs[this_attr])
				{
					char const *attr = attrs[this_attr++];
					char const *val = attrs[this_attr++];
					
					if(strcmp(attr, "detail") == 0)
					{
						Detail = val;
					}
					else if(strcmp(attr, "type") == 0)
					{
						Type = val;
					}
					else if(strcmp(attr, "size") == 0)
					{
						Size = atoi(val);
					}
					else if(strcmp(attr, "ref") == 0)
					{
						// Ignore
					}
					else
					{
						error("Unexpected attribute \"%s\" in compound item \"%s\"\n", attr, name);
					}
				}
			}

			MDTypePtr SubType = MDType::Find(Type);
			if(!SubType)
			{
				error("Compound Item \"%s\" specifies unknown type \"%s\"\n", name, Type);
			}
			else
			{
				// Add reference to sub-item type
				State->CurrentCompound->insert(MDType::value_type(std::string(name),SubType));
				State->CurrentCompound->ChildOrder.push_back(std::string(name));
			}

			break;
		}
	}

}


//! SAX callback - Deal with end tag of an element
extern void DefTypes_endElement(void *user_data, const char *name)
{
	DefTypesState *State = (DefTypesState*)user_data;

	/* Skip if all has gone 'belly-up' */
	if(State->State == DEFTYPES_STATE_ERROR) return;

	if(State->State == DEFTYPES_STATE_STARTED)
	{
		State->State = DEFTYPES_STATE_END;
		return;
	}

	if(State->State == DEFTYPES_STATE_BASIC)
	{
		if(strcmp(name,"Basic") == 0) State->State = DEFTYPES_STATE_STARTED;
	}
	else if(State->State == DEFTYPES_STATE_INTERPRETATION)
	{
		if(strcmp(name,"Interpretation") == 0) State->State = DEFTYPES_STATE_STARTED;
	}
	else if(State->State == DEFTYPES_STATE_MULTIPLE)
	{
		if(strcmp(name,"Multiple") == 0) State->State = DEFTYPES_STATE_STARTED;
	}
	else if(State->State == DEFTYPES_STATE_COMPOUND)
	{
		if(strcmp(name,"Compound") == 0) State->State = DEFTYPES_STATE_STARTED;
	}
	else if(State->State == DEFTYPES_STATE_COMPOUNDITEMS)
	{
		if(State->CompoundName && strcmp(name,State->CompoundName) == 0)
		{
			State->State = DEFTYPES_STATE_COMPOUND;
			State->CurrentCompound = NULL;
			State->CompoundName[0] = '\0';
		}
	}
}


//! SAX callback - Handle warnings during SAX parsing
extern void DefTypes_warning(void *user_data, const char *msg, ...)
{
    char Buffer[1024];			// DRAGONS: Could burst!!
	va_list args;

    va_start(args, msg);
	vsprintf(Buffer, msg, args);
	warning("XML WARNING: %s\n",Buffer);
    va_end(args);
}

//! SAX callback - Handle errors during SAX parsing
extern void DefTypes_error(void *user_data, const char *msg, ...)
{
    char Buffer[1024];			// DRAGONS: Could burst!!
	va_list args;

    va_start(args, msg);
	vsprintf(Buffer, msg, args);
	error("XML ERROR: %s\n",Buffer);
    va_end(args);
}

//! SAX callback - Handle fatal errors during SAX parsing
extern void DefTypes_fatalError(void *user_data, const char *msg, ...)
{
    char Buffer[1024];			// DRAGONS: Could burst!!
	va_list args;

    va_start(args, msg);
	vsprintf(Buffer, msg, args);
	error("XML FATAL ERROR: %s\n",Buffer);
    va_end(args);
}


#if 0


	printf("About to load dictionary \"XMLDict.xml\"\n");

//	MDOType::LoadDict("XMLDict.xml");
	
	RIP TestRIP;
	TestRIP.AddPartition(new Partition("Part1"),45,67);
	TestRIP.AddPartition(new Partition("Part2"),56,67);
	TestRIP.AddPartition(new Partition("Part3"),67,67);

	MDType::AddBasic("Int8", 1);
	MDType::AddBasic("Uint8", 1);
	MDType::AddBasic("Int16", 2);
	MDType::AddBasic("Uint16", 2);
	MDType::AddBasic("Int32", 4);
	MDType::AddBasic("Uint32", 4);
	
	MDTypePtr Int8Type = MDType::Find("Int8");
	ASSERT(Int8Type);

	MDTypePtr Uint8Type = MDType::Find("Uint8");
	ASSERT(Uint8Type);

	MDTypePtr Int16Type = MDType::Find("Int16");
	ASSERT(Int16Type);

	MDTypePtr Uint16Type = MDType::Find("Uint16");
	ASSERT(Int16Type);

	MDTypePtr Int32Type = MDType::Find("Int32");
	ASSERT(Int32Type);

	MDTypePtr Uint32Type = MDType::Find("Uint32");
	ASSERT(Uint32Type);





//	MDTraits Int8_Traits(Int8_SetInt, Int8_SetInt64, Int8_SetUint, Int8_SetUint64, Int8_SetString,
//						 Int8_GetInt, Int8_GetInt64, Int8_GetUint, Int8_GetUint64, Int8_GetString);

//	MDTraits Uint8_Traits(Uint8_SetInt, Uint8_SetInt64, Uint8_SetUint, Uint8_SetUint64, Uint8_SetString,
//						  Uint8_GetInt, Uint8_GetInt64, Uint8_GetUint, Uint8_GetUint64, Uint8_GetString);

//	MDTraits Int16_Traits(Int16_SetInt, Int16_SetInt64, Int16_SetUint, Int16_SetUint64, Int16_SetString,
//						  Int16_GetInt, Int16_GetInt64, Int16_GetUint, Int16_GetUint64, Int16_GetString);

	MDTraits_Int8 Int8_Traits;
	MDTraits_Uint8 Uint8_Traits;
	MDTraits_Int16 Int16_Traits;
	MDTraits_Uint16 Uint16_Traits;
	MDTraits_Int32 Int32_Traits;
	MDTraits_Uint32 Uint32_Traits;

	MDTraits_ISO7 ISO7_Traits;

	Int8Type->SetTraits(&Int8_Traits);
	Uint8Type->SetTraits(&Uint8_Traits);
	Int16Type->SetTraits(&Int16_Traits);
	Uint16Type->SetTraits(&Uint16_Traits);
	Int32Type->SetTraits(&Int32_Traits);
	Uint32Type->SetTraits(&Uint32_Traits);

	MDType::AddInterpretation("ISO7", Uint8Type);
	MDTypePtr ISO7Type = MDType::Find("ISO7");
	ASSERT(ISO7Type);
	ISO7Type->SetTraits(&ISO7_Traits);

	MDType::AddInterpretation("UTF16", Uint16Type);
	MDTypePtr UTF16Type = MDType::Find("UTF16");
	ASSERT(UTF16Type);
	MDTraits_UTF16 UTF16_Traits;
	UTF16Type->SetTraits(&UTF16_Traits);

	MDType::AddArray("Uint8Array", Uint8Type);
	MDTypePtr Uint8ArrayType = MDType::Find("Uint8Array");
	ASSERT(Uint8ArrayType);
	MDTraits_BasicArray BasicArray_Traits;
	Uint8ArrayType->SetTraits(&BasicArray_Traits);

	MDType::AddArray("ISO7Array", ISO7Type);
	MDTypePtr ISO7ArrayType = MDType::Find("ISO7Array");
	ASSERT(ISO7ArrayType);
	MDTraits_BasicStringArray BasicStringArray_Traits;
	ISO7ArrayType->SetTraits(&BasicStringArray_Traits);

	MDType::AddArray("UTF16Array", UTF16Type);
	MDTypePtr UTF16ArrayType = MDType::Find("UTF16Array");
	ASSERT(UTF16ArrayType);
	UTF16ArrayType->SetTraits(&BasicStringArray_Traits);

	MDType::AddArray("RawArray", Uint8Type);
	MDTypePtr RawArrayType = MDType::Find("RawArray");
	ASSERT(RawArrayType);
	MDTraits_RawArray RawArray_Traits;
	RawArrayType->SetTraits(&RawArray_Traits);

	MDType::AddInterpretation("UL", RawArrayType, 16);
	MDTypePtr ULType = MDType::Find("UL");
	ASSERT(ULType);
	RawArrayType->SetTraits(&RawArray_Traits);

	MDType::AddArray("ULArray", ULType);
	MDTypePtr ULArrayType = MDType::Find("ULArray");
	ASSERT(ULArrayType);
	MDTraits_RawArrayArray RawArrayArray_Traits;
	ULArrayType->SetTraits(&RawArrayArray_Traits);

	
	
	MDValuePtr Test16;
	MDValuePtr Test32;

	Test16 = new MDValue("Int16");
	Test32 = new MDValue("Int32");
	
	std::string X;
	X = Test16->GetString();
	printf("Int16 uses %s\n", X.c_str());
	
	MDValuePtr Test8 = new MDValue("Uint8");

	X = Test8->GetString();
	printf("Int8 uses %s\n", X.c_str());

	Test8->SetInt(42);
	X = Test8->GetString();
	printf("The answer = \"%s\"\n", X.c_str());

	Test16->SetInt(-1);
	X = Test16->GetString();
	printf("i*i = \"%s\"\n", X.c_str());

	Test8->SetInt(0x12345678);
	X = Test8->GetString();
	printf("0x12345678 = \"%s\"\n", X.c_str());

	Test16->SetInt(0x34567890);
	X = Test16->GetString();
	printf("0x34567890 = \"%s\"\n", X.c_str());

	Test8->SetInt(0x87654321);
	X = Test8->GetString();
	printf("0x87654321 = \"%s\"\n", X.c_str());

	Int8 i8 = (Int8) (0x87654321);
	printf("Casting gives %d\n", (int)i8);

	Test16->SetString("42");
	printf("Value = %d, %x\n",Test16->GetInt(), Test16->GetInt()); 

	Uint64 T = 0x12345678a1b2c3d4;
	printf("%I64x --> %I64x\n", T, Swap(T));

	Uint16 T2 = 0x1234;
	printf("%x -> %x\n", T2, Swap(T2));
	
	printf("Int32:\n");

	Test32->SetInt(-1);
	X = Test32->GetString();
	printf("i*i = \"%s\"\n", X.c_str());

	Test32->SetInt(0x12345678);
	X = Test32->GetString();
	printf("0x12345678 = \"%s\"\n", X.c_str());

	Test32->SetInt(0x34567890);
	X = Test32->GetString();
	printf("0x34567890 = \"%s\"\n", X.c_str());

	Test32->SetInt(0x87654321);
	X = Test32->GetString();
	printf("0x87654321 = \"%s\"\n", X.c_str());

	Int32 I32i = Test32->GetInt();
	printf("0x87654321 = 0x%x = %d\n", I32i, I32i);

	Uint32 U32i = Test32->GetUint();
	printf("0x87654321 (unsigned) = 0x%x = %u\n", U32i, U32i);

	Test32 = new MDValue("Uint32");

	printf("Uint32:\n");

	Test32->SetInt(-1);
	X = Test32->GetString();
	printf("i*i = \"%s\"\n", X.c_str());

	Test32->SetInt(0x12345678);
	X = Test32->GetString();
	printf("0x12345678 = \"%s\"\n", X.c_str());

	Test32->SetInt(0x34567890);
	X = Test32->GetString();
	printf("0x34567890 = \"%s\"\n", X.c_str());

	Test32->SetInt(0x87654321);
	X = Test32->GetString();
	printf("0x87654321 = \"%s\"\n", X.c_str());

	I32i = Test32->GetInt();
	printf("0x87654321 = 0x%x = %d\n", I32i, I32i);

	U32i = Test32->GetUint();
	printf("0x87654321 (unsigned) = 0x%x = %u\n", U32i, U32i);


	MDValuePtr TestISO;
	TestISO = new MDValue("ISO7");

	TestISO->SetString("A");
	X = TestISO->GetString();
	printf("\nFirst letter = \"%s\"\n", X.c_str());
	printf("Value of first letter = %d\n", TestISO->GetInt());

	TestISO->SetString("7");
	X = TestISO->GetString();
	printf("\nNumber seven = \"%s\"\n", X.c_str());
	printf("Value of number seven = %d\n", TestISO->GetInt());


	MDValuePtr TestArray;
	TestArray = new MDValue("Uint8Array");
	ASSERT(TestArray);
	

	MDValuePtr TestItem = new MDValue("Uint8");
	TestItem->SetInt(42);
	TestArray->AddChild(TestItem,3);

	{
		X = TestArray->GetString();
		printf("TestArray String Value = \"%s\"\n", X.c_str());

		printf("TestArray Value = ");
		int i=0;
		for(;;)
		{
			MDValuePtr Test = (*TestArray)[i];
			if(!Test) break;

			X = Test->GetString();
			printf("%s, ", X.c_str());

			i++;
		}
		printf("\n");
	}


	TestArray->SetInt(24);

	{
		printf("TestArray Value = ");
		int i=0;
		for(;;)
		{
			MDValuePtr Test = (*TestArray)[i];
			if(!Test) break;

			X = Test->GetString();
			printf("%s, ", X.c_str());

			i++;
		}
		printf("\n");
	}

	TestArray = new MDValue("ISO7Array");
	ASSERT(TestArray);
	
	TestArray->SetString("This is some Text");
	X = TestArray->GetString();
	printf("TestArray now = \"%s\"\n", X.c_str());

	TestItem = (*TestArray)[6];
	TestItem->SetInt(42);

	printf("Sub-item = \"%c\"\n", TestItem->GetInt());
	X = TestArray->GetString();
	printf("TestArray now = \"%s\"\n", X.c_str());

	TestItem->SetInt(0);

	printf("Sub-item = \"%c\"\n", TestItem->GetInt());
	X = TestArray->GetString();
	printf("TestArray now = \"%s\"\n", X.c_str());

 
	TestArray = new MDValue("UTF16Array");
	ASSERT(TestArray);

	TestArray->SetString("This is some Text");

	TestItem = (*TestArray)[6];
	TestItem->SetInt(12345);
	printf("Sub-item 6 = \"%c\" (%d)\n", TestItem->GetInt(),TestItem->GetInt());

	X = TestArray->GetString();
	printf("TestArray = \"%s\"\n", X.c_str());


	TestArray = new MDValue("RawArray");
	ASSERT(TestArray);

	TestArray->SetString("06 0e 2b 34 0 0 1 d 3 10");

	TestItem = (*TestArray)[7];
	printf("Sub-item 7 = %d\n", TestItem->GetInt());

	X = TestArray->GetString();
	printf("TestArray = \"%s\"\n", X.c_str());


	TestArray = new MDValue("UL");
	ASSERT(TestArray);

	TestArray->SetString("06 0e 2b 34 0 0 1 d 3 10");

	TestItem = (*TestArray)[7];
	printf("Sub-item 7 = %d\n", TestItem->GetInt());

	X = TestArray->GetString();
	printf("TestArray = \"%s\"\n", X.c_str());

	
	TestArray = new MDValue("ULArray");
	ASSERT(TestArray);

	TestArray->SetString("{06 0e 2b 34 0 0 1 d 3 10}, {06 0e 2b 34 1 2 3}");

	X = TestArray->GetString();
	printf("\nTestArray of ULs= \"%s\"\n", X.c_str());


	printf("\nPress RETURN key ");
	getchar();
	return 0;
}

#endif
