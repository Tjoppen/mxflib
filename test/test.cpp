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

	MDOType::LoadDict("XMLDict.xml");
	
	RIP TestRIP;
	TestRIP.AddPartition(new Partition("Part1"),45,67);
	TestRIP.AddPartition(new Partition("Part2"),56,67);
	TestRIP.AddPartition(new Partition("Part3"),67,67);

	MDType::AddBasic("Int8", 1);
	MDType::AddBasic("Int16", 2);

	MDTypePtr Int8Type = MDType::Find("Int8");
	ASSERT(Int8Type);

	MDTraits Int8_Traits(Int8_SetInt, Int8_SetInt64, Int8_SetUint, Int8_SetUint64, Int8_SetString,
						 Int8_GetInt, Int8_GetInt64, Int8_GetUint, Int8_GetUint64, Int8_GetString);

	Int8Type->SetTraits(Int8_Traits);

	MDValuePtr Test;

	Test = new MDValue("Int16");
	
	std::string X;
	X = Test->GetString();
	printf("Int16 uses %s\n", X.c_str());
	
	Test = new MDValue("Int8");

	X = Test->GetString();
	printf("Int8 uses %s\n", X.c_str());

	Test->SetInt(42);
	X = Test->GetString();
	printf("The answer = \"%s\"\n", X.c_str());

	Test->SetInt(-1);
	X = Test->GetString();
	printf("i*i = \"%s\"\n", X.c_str());

	Test->SetInt(0x12345678);
	X = Test->GetString();
	printf("0x12345678 = \"%s\"\n", X.c_str());

	Test->SetInt(0x34567890);
	X = Test->GetString();
	printf("0x34567890 = \"%s\"\n", X.c_str());

	Test->SetInt(0x87654321);
	X = Test->GetString();
	printf("0x87654321 = \"%s\"\n", X.c_str());

	Int8 i8 = (Int8) (0x87654321);
	printf("Casting gives %d\n", (int)i8);

	Test->SetString("42");
	printf("Value = %d, %x\n",Test->GetInt(), Test->GetInt()); 

	Uint64 T = 0x12345678a1b2c3d4;
	printf("%I64x --> %I64x\n", T, Swap(T));

	Uint16 T2 = 0x1234;
	printf("%x -> %x\n", T2, Swap(T2));

	printf("Press RETURN key ");
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
