/*! \file	test.cpp
 *	\brief	Test program for MXFLib
 */

#include "..\mxflib.h"

using namespace mxflib;

#include <stdio.h>
#include <iostream>

using namespace std;


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

	LoadTypes("types.xml");
	MDOType::LoadDict("XMLDict.xml");
//	PrimerPtr OurPrimer = MDOType::MakePrimer();

	MXFFile TestFile;
	TestFile.Open(argv[1], true);

	TestFile.ReadRIP();

	RIP::iterator it = TestFile.FileRIP.begin();
	while(it != TestFile.FileRIP.end())
	{
		printf("Partition at 0x%s is for BodySID 0x%04x\n", Int64toHexString((*it)->ByteOffset,8).c_str(), (*it)->BodySID);
		it++;
	}

	TestFile.Close();


	// Matt's notes for later work..
	// NOTE: Partition pack children are the header metadata...?
	// NOTE2: AddChild -> AddChild and ReplaceChild...?

	printf("\nPress RETURN key ");
	getchar();
	return 0;
}

Uint8 PartPackData[] =
{ 
	0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x22, 0x88, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
	0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x01, 0x09, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01,
	0x0d, 0x01, 0x03, 0x01, 0x02, 0x01, 0x01, 0x01
};

const int PartPackDataSize = sizeof(PartPackData) / sizeof(PartPackData[0]);


int xmain(int argc, char *argv[])
{
	printf("Test Program for MXFLib\n");

	int i;
	for(i=1; i<argc; i++)
	{
		if((argv[i][0] == '/') || (argv[i][0] == '-')) 
			if((argv[i][1] == 'v') || (argv[i][1] == 'V'))
				DebugMode = true;
	}

	LoadTypes("types.xml");
	MDOType::LoadDict("XMLDict.xml");
	PrimerPtr OurPrimer = MDOType::MakePrimer();

	MDObjectPtr PartitionPack = new MDObject("ClosedHeader");

	PartitionPack->ReadValue(PartPackData, PartPackDataSize);

	cout << "MajorVersion       = " << PartitionPack["MajorVersion"]->GetString() << endl;
	cout << "MinorVersion       = " << PartitionPack["MinorVersion"]->GetString() << endl;
	cout << "KAGSize            = " << PartitionPack["KAGSize"]->GetString() << endl;
	cout << "ThisPartition      = " << PartitionPack["ThisPartition"]->GetString() << endl;
	cout << "PreviousPartition  = " << PartitionPack["PreviousPartition"]->GetString() << endl;
	cout << "FooterPartition    = 0x" << hex << PartitionPack["FooterPartition"]->GetInt() << endl;
	cout << "HeaderByteCount    = " << PartitionPack["HeaderByteCount"]->GetString() << endl;
	cout << "IndexByteCount     = " << PartitionPack["IndexByteCount"]->GetString() << endl;
	cout << "IndexSID           = " << PartitionPack["IndexSID"]->GetString() << endl;
	cout << "BodyOffset         = " << PartitionPack["BodyOffset"]->GetString() << endl;
	cout << "BodySID            = " << PartitionPack["BodySID"]->GetString() << endl;
	cout << "OperationalPattern = " << PartitionPack["OperationalPattern"]->GetString() << endl;
	cout << "EssenceContainers  = " << PartitionPack["EssenceContainers"]->GetString() << endl;

	int Test = PartitionPack["OperationalPattern"]->Value[3]->GetInt();

	cout << endl;
	cout << "Forth byte of OP label is " << Test << endl;

	MDValuePtr DV_525_25 = new MDValue("Label");
	DV_525_25->SetString("06 0e 2b 34 4 1 1 1 d 1 3 1 2 2 40 1");
	
	PartitionPack["EssenceContainers"]->Value->AddChild(DV_525_25);

	cout << endl;
	cout << "New EssenceContainers = " << PartitionPack["EssenceContainers"]->GetString() << endl;
	cout << "First Container = " << PartitionPack["EssenceContainers"]->Value[0]->GetString() << endl;
	cout << "Second Container = " << PartitionPack["EssenceContainers"]->Value[1]->GetString() << endl;


	MDObjectPtr Obj = new MDObject("Identification");

	MDObjectPtr SubObj = Obj->Child("ModificationDate");

	printf("Address of \"ModificationDate\" = 0x%08x\n", SubObj.GetPtr());

//	ULPtr MyUL = new UL((Uint8*)NULL);
//	MDObjectPtr PartitionPack = new MDObject(MyUL);

//	Obj->AddChild("ModificationDate");

	PrimerPtr NewPrimer = new Primer;
	Uint8 PriBuff[] = { 0x3c, 0x06, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x02, 0x07, 0x02, 0x01, 0x10, 0x02, 0x03, 0x00, 0x00};
	NewPrimer->ReadValue(PriBuff, 18);

	Uint8 TestBuff[] = { 0x3c, 0x06, 0x00, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x00, 0x02, 0x7f, 0x40 };
	Obj->ReadValue(TestBuff, 18, NewPrimer);

/*	SubObj = new MDObject(Tag(0x3c06), NewPrimer);
	Obj->AddChild(SubObj);

	SubObj = Obj->Child("ModificationDate");
	printf("Address of \"ModificationDate\" = 0x%08x\n", SubObj.GetPtr());

	Obj["ModificationDate"]->SetString("\"1966\", \"11\", \"1\", \"12\", \"15\", \"00\", \"00\"" );
	std::string XO = SubObj->Value->GetString();
	printf("ModificationDate = \"%s\"\n", XO.c_str());

	Obj["ModificationDate"]->Value["Hours"]->SetInt(16);
*/	std::string XO = Obj->Child("ModificationDate")->Value->GetString();
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

	Obj->AddChild("ObjectModelVersion");
	Obj["ObjectModelVersion"]->Value->ReadValue((Uint8*)"*",1);
	XO = Obj["ObjectModelVersion"]->Value->GetString();
	printf("ObjectModelVersion = \"%s\"\n", XO.c_str());

	Obj->AddChild("PrimaryPackage");
	Obj["PrimaryPackage"]->Value->ReadValue((Uint8*)"*#A",3);
	XO = Obj["PrimaryPackage"]->Value->GetString();
	printf("PrimaryPackage = \"%s\"\n", XO.c_str());

	Obj->AddChild("Identifications");
	Obj["Identifications"]->Value->ReadValue((Uint8*)"This is a nice long string to make the IDs from",47);
	XO = Obj["Identifications"]->Value->GetString();
	printf("Identifications = \"%s\"\n", XO.c_str());
	Obj["Identifications"]->SetString(XO);
	XO = Obj["Identifications"]->Value->GetString();
	printf("Identifications = \"%s\"\n", XO.c_str());

	Obj->AddChild("LastModificationDate");
	Obj["LastModificationDate"]->Value->ReadValue((Uint8*)"\x007\x0b5\x005\x00b\x00a\x01e\x019\x07d",8);
	XO = Obj["LastModificationDate"]->Value->GetString();
	printf("LastModificationDate = \"%s\"\n", XO.c_str());

//	RIP TestRIP;
//	TestRIP.AddPartition(new Partition("Part1"),45,67);
//	TestRIP.AddPartition(new Partition("Part2"),56,67);
//	TestRIP.AddPartition(new Partition("Part3"),67,67);

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


	MDValuePtr TestArray = new MDValue("Label");
	ASSERT(TestArray);
	TestArray->SetString("06 0e 2b 34 0 0 1 d 3 10");
	X = TestArray->GetString();
	printf("Label = \"%s\"\n", X.c_str());

	TestArray = new MDValue("DoesNotExist");
	ASSERT(TestArray);
	TestArray->SetString("06 0e 2b, 34 0 0, 1 d 3, 10");
	X = TestArray->GetString();
	printf("DoesNotExist = \"%s\"\n", X.c_str());

#if 0
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
#endif 0
	printf("\nPress RETURN key ");
	getchar();
	return 0;
}


// Debug and error messages
#include <stdarg.h>

//! Display a general debug message
int mxflib::debug(const char *Fmt, ...)
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
int mxflib::warning(const char *Fmt, ...)
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
int mxflib::error(const char *Fmt, ...)
{
	int ret;

	va_list args;

	va_start(args, Fmt);
	printf("ERROR: ");
	ret = vprintf(Fmt, args);
	va_end(args);

	return ret;
}
