/*! \file	test.cpp
 *	\brief	Test program for MXFLib
 */

#include "..\mxflib.h"

using namespace mxflib;

#include <stdio.h>

//! Debug flag for KLVLib
extern "C" int Verbose = 0;

//! Debug flag for MXFLib
bool DebugMode = false;

int main(int argc, char *argv[])
{
	printf("Test Program for MXFLib\n");

	int i;
	for(i=1; i<argc; i++)
	{
		if((argv[i][0] == '/') || (argv[i][0] == '-')) 
			if((argv[i][1] == 'v') || (argv[i][1] == 'V'))
				DebugMode = true;
	}

	printf("About to load dictionary \"XMLDict.xml\"\n");
	LoadTypes("types.xml");
	MDOType::LoadDict("XMLDict.xml");

	MDObjectPtr Obj = new MDObject("Identification");

	MDObjectPtr SubObj = Obj->Child("ModificationDate");

	printf("Address of \"ModificationDate\" = 0x%08x\n", SubObj.GetPtr());

	Obj->AddChild("ModificationDate");
	SubObj = Obj->Child("ModificationDate");
	printf("Address of \"ModificationDate\" = 0x%08x\n", SubObj.GetPtr());

	Obj["ModificationDate"]->SetString("\"1966\", \"11\", \"1\", \"12\", \"15\", \"00\", \"00\"" );
	std::string XO = SubObj->Value->GetString();
	printf("ModificationDate = \"%s\"\n", XO.c_str());
	
	Obj["ModificationDate"]->Value["Hours"]->SetInt(16);
	XO = Obj->Child("ModificationDate")->Value->GetString();
	printf("ModificationDate = \"%s\"\n", XO.c_str());

	Obj = new MDObject("Preface");
	ASSERT(Obj->AddChild("DMSchemes"));

	Obj["DMSchemes"]->SetString("{06 0e 2b 34 01 2 3 4}, {06 e 2b 24 5 6 7}");
	XO = Obj["DMSchemes"]->GetString();
	printf("DMSchemes = \"%s\"\n", XO.c_str());
	XO = Obj["DMSchemes"]->Value[0]->GetString();
	printf("DMSchemes[0] = \"%s\"\n", XO.c_str());
	XO = Obj["DMSchemes"]->Value[1]->GetString();
	printf("DMSchemes[1] = \"%s\"\n", XO.c_str());


//	RIP TestRIP;
//	TestRIP.AddPartition(new Partition("Part1"),45,67);
//	TestRIP.AddPartition(new Partition("Part2"),56,67);
//	TestRIP.AddPartition(new Partition("Part3"),67,67);


/*	MDType::AddBasic("Int8", 1);
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

	
*/	
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

	MDValuePtr Test64 = new MDValue("Int64");
	printf("Int64:\n");

	Test64->SetInt(-1);
	X = Test64->GetString();
	printf("i*i = \"%s\"\n", X.c_str());

	Test64->SetInt(0x12345678);
	X = Test64->GetString();
	printf("0x12345678 = \"%s\"\n", X.c_str());

	Test64->SetInt64(0x34567890);
	X = Test64->GetString();
	printf("0x34567890 = \"%s\"\n", X.c_str());

	printf("Using SetInt(0x87654321):\n");
	Test64->SetInt(0x87654321);
	X = Test64->GetString();
	printf("0x87654321 = \"%s\"\n", X.c_str());

	Int64 I64i = Test64->GetInt64();
	printf("0x87654321 = 0x%I64x = %I64d\n", I64i, I64i);

	Uint64 U64i = Test64->GetUint64();
	printf("0x87654321 (unsigned) = 0x%I64x = %I64u\n", U64i, U64i);
	
	printf("Using SetInt64(0x87654321):\n");
	Test64->SetInt64(0x87654321);
	X = Test64->GetString();
	printf("0x87654321 = \"%s\"\n", X.c_str());

	I64i = Test64->GetInt64();
	printf("0x87654321 = 0x%I64x = %I64d\n", I64i, I64i);

	U64i = Test64->GetUint64();
	printf("0x87654321 (unsigned) = 0x%I64x = %I64u\n", U64i, U64i);

	Test64->SetInt64(0x8765432123456789);
	X = Test64->GetString();
	printf("0x8765432123456789 = \"%s\"\n", X.c_str());

	I64i = Test64->GetInt64();
	printf("0x8765432123456789 = 0x%I64x = %I64d\n", I64i, I64i);

	U64i = Test64->GetUint64();
	printf("0x8765432123456789 (unsigned) = 0x%I64x = %I64u\n", U64i, U64i);

	Test64 = new MDValue("Uint64");

	printf("Uint64:\n");

	Test64->SetInt(-1);
	X = Test64->GetString();
	printf("i*i = \"%s\"\n", X.c_str());

	Test64->SetInt(0x12345678);
	X = Test64->GetString();
	printf("0x12345678 = \"%s\"\n", X.c_str());

	Test64->SetInt64(0x34567890);
	X = Test64->GetString();
	printf("0x34567890 = \"%s\"\n", X.c_str());

	printf("Using SetInt(0x87654321):\n");
	Test64->SetInt(0x87654321);
	X = Test64->GetString();
	printf("0x87654321 = \"%s\"\n", X.c_str());

	I64i = Test64->GetInt();
	printf("0x87654321 = 0x%I64x = %I64d\n", I64i, I64i);

	U64i = Test64->GetUint();
	printf("0x87654321 (unsigned) = 0x%I64x = %I64u\n", U64i, U64i);

	printf("Using SetInt64(0x87654321):\n");
	Test64->SetInt64(0x87654321);
	X = Test64->GetString();
	printf("0x87654321 = \"%s\"\n", X.c_str());

	I64i = Test64->GetInt64();
	printf("0x87654321 = 0x%I64x = %I64d\n", I64i, I64i);

	U64i = Test64->GetUint64();
	printf("0x87654321 (unsigned) = 0x%I64x = %I64u\n", U64i, U64i);

	Test64->SetInt64(0x8765432123456789);
	X = Test64->GetString();
	printf("0x8765432123456789 = \"%s\"\n", X.c_str());

	I64i = Test64->GetInt64();
	printf("0x8765432123456789 = 0x%I64x = %I64d\n", I64i, I64i);

	U64i = Test64->GetUint64();
	printf("0x8765432123456789 (unsigned) = 0x%I64x = %I64u\n", U64i, U64i);



	MDValuePtr Test = new MDValue("TimeStamp");
	Test->SetString("\"1966\", \"11\", \"1\", \"12\", \"15\", \"00\", \"00\"" );
	X = Test->GetString();
	printf("TimeStamp = \"%s\"\n", X.c_str());

	Test = new MDValue("Boolean");
	Test->SetUint(0xff);
	X = Test->GetString();
	printf("True = \"%s\"\n", X.c_str());

	MDValuePtr TestArray = new MDValue("Label");
	ASSERT(TestArray);

	TestArray->SetString("06 0e 2b 34 0 0 1 d 3 10");

	MDValuePtr TestItem = (*TestArray)[7];
	printf("Sub-item 7 = %d\n", TestItem->GetInt());

	X = TestArray->GetString();
	printf("TestArray = \"%s\"\n", X.c_str());

	
/*
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
*/

	printf("\nPress RETURN key ");
	getchar();
	return 0;
}


// Debug and error messages
#include <stdarg.h>

//! Display a general debug message
int debug(const char *Fmt, ...)
{
	if(!DebugMode) return 0;

	int ret;

	va_list args;

	va_start(args, Fmt);
	ret = vprintf(Fmt, args);
	va_end(args);

	return ret;
}

//! Display a warning message
int warning(const char *Fmt, ...)
{
	int ret;

	va_list args;

	va_start(args, Fmt);
	printf("Warning: ");
	ret = vprintf(Fmt, args);
	va_end(args);

	return ret;
}

//! Display an error message
int error(const char *Fmt, ...)
{
	int ret;

	va_list args;

	va_start(args, Fmt);
	printf("ERROR: ");
	ret = vprintf(Fmt, args);
	va_end(args);

	return ret;
}
