/*! \file	test.cpp
 *	\brief	Test program for MXFLib
 */

#include "..\mxflib.h"

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

	mxflib::MDDict("XMLDict.xml");
	
	mxflib::RIP TestRIP;
	TestRIP.AddPartition(new mxflib::Partition("Part1"),45,67);
	TestRIP.AddPartition(new mxflib::Partition("Part2"),56,67);
	TestRIP.AddPartition(new mxflib::Partition("Part3"),67,67);

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
