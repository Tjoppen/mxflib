/*! \file	test.cpp
 *	\brief	Test program for MXFLib
 */

#include "..\mxflib.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	printf("Test Program for MXFLib\n");

	mxflib::RIP TestRIP;
	TestRIP.AddPartition(new mxflib::Partition("Part1"),45,67);
	TestRIP.AddPartition(new mxflib::Partition("Part2"),56,67);
	TestRIP.AddPartition(new mxflib::Partition("Part3"),67,67);

	return 0;
}