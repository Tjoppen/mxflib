/*! \file	mpegwrap.cpp
 *	\brief	Basic MXF MPEG ES wrapper
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

#include "..\mxflib.h"

using namespace mxflib;

#include <stdio.h>
#include <iostream>

using namespace std;

//! Debug flag for KLVLib
extern "C" int Verbose = 0;

//! Debug flag for MXFLib
bool DebugMode = false;

Int64 ProcessEssence(FileHandle InFile, MXFFilePtr &Out, Int64 PictureLimit = 0);
void WriteMPEG(FileHandle InFile, MXFFilePtr &Out, Int64 PictureStart, int PictureStartBitOffset, Int64 PictureEnd, int PictureEndBitOffset);
MDObjectPtr BuildMPEGEssenceDescriptor(FileHandle InFile);

// Options
Uint32 KAGSize = 1;

//IT# // The index table
//IT# IndexTablePtr Table;

int main(int argc, char *argv[])
{
	printf("Simple MXF wrapping of MPEG\n\n");

//IT# 	// Clear the index table
//IT# 	Table = new IndexTable();

	// Parse command line options
	int i;
	for(i=1; i<argc;)
	{
		if((argv[i][0] == '/') || (argv[i][0] == '-'))
		{
			char *p = &argv[i][1];					// The option less the '-' or '/'
			char Opt = tolower(*p);					// The option itself (in lower case)
			char *Val = "";							// Any value attached to the option
			if(strlen(p) > 2) Val = &p[2];			// Only set a value if one found

			if(Opt == 'v') DebugMode = true;
			else if(Opt == 'k') KAGSize = atoi(Val);
			else 
			{
				error("Unknown command-line option %s\n", argv[i]);
			}

			// Remove this option
			int j;
			for(j=i; j<(argc-1); j++) argv[j] = argv[j+1];
			argc--;
		}
		else 
		{
			// Move on to next option
			i++;
		}
	}

	if(argc < 2)
	{
		printf("Usage:    mpegwrap <mpegfile> [ <mxffile> ]\n\n");
		printf("Options:  -v        = Verbose mode\n");
		printf("          -k=<size> = Set KAG size (default=1)\n\n");
		return -1;
	}

	char InFilename[128];
	char OutFilename[128];

	strncpy(InFilename, argv[1], 120);

	if(argc >= 3)
	{
		strncpy(OutFilename, argv[2], 120);
	}
	else
	{
		strncpy(OutFilename, argv[1], 120);
	}

	char *LastDot = NULL;
	char *ps = InFilename;

	// Find the position of last dot in the input filename
	while(*ps)
	{
		if(*ps == '.') LastDot = ps;
		ps++;
	};

	if(LastDot == NULL)
	{
		strcat(InFilename, ".mpg");
	}
	
	LastDot = NULL;
	ps = OutFilename;
	// Find the position of last dot in the output filename
	while(*ps)
	{
		if(*ps == '.') LastDot = ps;
		ps++;
	};

	if(argc >= 3)
	{
		// If both filenames specified no extension add ".mxf"
		if(LastDot == NULL) strcat(OutFilename, ".mxf");
	}
	else
	{
		// If only one filename specified force extension to ".mxf"
		if(LastDot != NULL) strcpy(LastDot, ".mxf"); else strcat(OutFilename, ".mxf");
	}


	// Detail the options if debug mode is enabled
	debug("** Verbose Mode **\n\n");
	debug("KAGSize     = %u\n\n", KAGSize);
	debug("Input file  = %s\n", InFilename);
	debug("Output file = %s\n\n", OutFilename);


	// Open the input file
	FileHandle InFile = FileOpenRead(InFilename);
	if(!FileValid(InFile))
	{
		error("Can't open input file \"%s\"\n", InFilename);
		return -2;
	}

	LoadTypes("types.xml");
	MDOType::LoadDict("XMLDict.xml");

	MetadataPtr MData = new Metadata();
	ASSERT(MData);
	ASSERT(MData->Object);

	// Frame wrapped MPEG-ES
	// stream_id range starts at 0xe0 so byte 15 is 0x60
	Uint8 MPEG_VES_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x02, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x04, 0x60, 0x01 };
	ULPtr MPEG_VES = new UL(MPEG_VES_Data);
	MData->AddEssenceType(MPEG_VES);

	// ## Qualifiers need work!
	Uint8 OP1a_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x01, 0x09, 0x00 };
	ULPtr OP1a = new UL(OP1a_Data);
	MData->SetOP(OP1a);

	// Build an essence descriptor
	MDObjectPtr EssenceDescriptor = BuildMPEGEssenceDescriptor(InFile);

	bool DropFrame = 0;
	std::string EditRate = EssenceDescriptor->GetString("SampleRate");
	Uint32 FrameRate = atoi(EditRate.c_str());

	// If the numerator is large the denominator will be "1001" so we are in drop-frame
	if(FrameRate > 1000)
	{
		FrameRate /= 1000;
		DropFrame = true;
	}

	// Build the Material Package
	PackagePtr MaterialPackage = MData->AddMatarialPackage("Material Package");
	MData->SetPrimaryPackage(MaterialPackage);
	TrackPtr MPTimecodeTrack = MaterialPackage->AddTimecodeTrack(EditRate);
	TimecodeComponentPtr MPTimecodeComponent = MPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, 0);
	TrackPtr MPPictureTrack = MaterialPackage->AddPictureTrack(EditRate);
	SourceClipPtr MPClip = MPPictureTrack->AddSourceClip();

	// Build the File Package
	PackagePtr FilePackage = MData->AddFilePackage(1, "File Package");
	TrackPtr FPTimecodeTrack = FilePackage->AddTimecodeTrack(EditRate);
	TimecodeComponentPtr FPTimecodeComponent = FPTimecodeTrack->AddTimecodeComponent(25, false, 0);
	TrackPtr FPPictureTrack = FilePackage->AddPictureTrack(0x15010500, EditRate);		// Link to MPEG-ES picture stream 0
	SourceClipPtr FPClip = FPPictureTrack->AddSourceClip();

	// Add the file descriptor
	EssenceDescriptor->SetUint("LinkedTrackID", FPPictureTrack->GetUint("TrackID"));
	EssenceDescriptor->AddChild("EssenceContainer")->ReadValue(MPEG_VES->GetValue(), 16);
	FilePackage->AddChild("Descriptor")->MakeLink(EssenceDescriptor);

	// Link the MP to the FP
	MPClip->MakeLink(FPPictureTrack, 0);

	// ==== Build a file ====

	PartitionPtr ThisPartition = new Partition("OpenHeader");
	ASSERT(ThisPartition);
	ThisPartition->SetKAG(KAGSize);			// Everything else can stay at default
	ThisPartition->SetUint("BodySID", 1);

	ThisPartition->AddMetadata(MData);

	// Set the initial generation UIDs in the metadata
	MDObjectPtr Ident = new MDObject("Identification");
	Ident->SetString("CompanyName", "freeMXF.org");
	Ident->SetString("ProductName", "mpegwrap");
	Ident->SetString("VersionString", "1.0");
	UUIDPtr ProductUID = new mxflib::UUID;
	Ident->SetValue("ProductUID", DataChunk(16,ProductUID->GetValue()));		// Luck-dip!!

	MData->UpdateGenerations(Ident);

	MXFFilePtr Out = new MXFFile;

	Out->OpenNew(OutFilename);
	Out->WritePartition(ThisPartition);

	// Process Essence
	Uint64 Duration = ProcessEssence(InFile, Out);
	printf("Found %d pictures\n", (int)Duration);

	// Write a footer (with updated durations)
	MData->SetTime();
	MPTimecodeComponent->SetDuration(Duration);
	MPClip->SetDuration(Duration);
	FPTimecodeComponent->SetDuration(Duration);
	FPClip->SetDuration(Duration);
	EssenceDescriptor->SetInt64("ContainerDuration",Duration);
	
	// Update the generation UIDs in the metadata
	MData->UpdateGenerations(Ident);

	ThisPartition->ChangeType("Footer");
	ThisPartition->SetUint("BodySID", 0);
	ThisPartition->UpdateMetadata(MData);

	Out->WritePartition(ThisPartition);
	Out->WriteRIP();

	// Update the FooterPartition in the header
	Uint64 FooterPos = ThisPartition->GetUint64("FooterPartition");
	Out->Seek(0);
	ThisPartition = Out->ReadPartition();
	ThisPartition->SetUint64("FooterPartition", FooterPos);
	Out->Seek(0);
	Out->WritePartitionPack(ThisPartition);

	Out->Close();

/*IT# 	//int i;
	for(i=0; i<8; i++)
	{
		int j;
		for(j=0; j<3; j++)
		{
			IndexPosPtr Pos = Table->Lookup(0 + i,j);
			printf("  EditUnit %d for stream %d is at 0x%s", (int)(0 + i), j, Int64toHexString(Pos->Location,8).c_str());
			printf(", Flags=%02x", Pos->Flags);
			if(Pos->Exact) printf("  *Exact*\n"); else printf("\n");
		}
	}
#IT#*/
	return true;
}


Int64 ProcessEssence(FileHandle InFile, MXFFilePtr &Out, Int64 PictureLimit /*=0*/)
{
//IT# 	Int64 ECStart = Out->Tell();
//IT# 	Int64 EditUnit = 0;
	Int64 PictureCount = 0;

	// Flag that we haven't yet found the first picture
	Int64 PictureStart = -1;
	int PictureStartBitOffset = 0;
	Int64 PictureEnd = 0;
	int PictureEndBitOffset = 0;
	bool FoundEnd = false;									// We haven't yet found the next picture start

	// 32K buffer for scanning
	int ScanBufferSize = 32768;
//ScanBufferSize = 32;
	Uint8 *ScanBuffer = new Uint8[ScanBufferSize];

	// Try and fill the buffer from the start
	Int64 BufferStart = 0;		// Offset in the file of the first byte in the buffer
	
	for(;;)
	{
		FileSeek(InFile,BufferStart);
		int BufferCount = FileRead(InFile, ScanBuffer, ScanBufferSize);

//printf("Read %d\n", BufferCount);

		// If less than 4 bytes in the input file then we are doomed!
		if(BufferCount <= 4) 
		{ 
			// Don't write anything until we have found the first picture
			if(PictureStart >= 0)
			{
				// Align with the KAG is we are using one
				if(KAGSize > 1) Out->Align(KAGSize);

//IT# 				Table->AddEntry(EditUnit, Out->Tell() - ECStart);
//IT# 				EditUnit++;

				WriteMPEG(InFile, Out, PictureStart, PictureStartBitOffset, FileTell(InFile)+1, 0);
				PictureCount++;
			}

			return PictureCount; 
		}

		// Scan all but the last four bytes
		int ScanPos = 0;
		int ScanLeft = BufferCount - 4;
		Uint8 *BuffPtr = ScanBuffer;
		while(ScanLeft)
		{
//printf("%02x ", *BuffPtr);

			if(*BuffPtr == 0)
			{
				if(BuffPtr[1] == 0)
				{
					// We know we have 16 zeros, but is it part of a start_code?
					// We check for this by testing the outer two bytes
					// Perform the test in this CPU's "favorite" integer type
					unsigned int Test;
					if(ScanPos == 0) Test = 0xff00 | BuffPtr[2];
					else Test = (BuffPtr[-1] << 8) | (BuffPtr[2]);

//printf("Found two zeros.. Test=0x%04x\n", Test);

					int Pos = 0;
					if((Test & 0x00ff) == 0x0001) Pos = 8;			// Test the most common first
					else if((Test & 0x7f80) == 0x0080) Pos = 1;
					else if((Test & 0x3fc0) == 0x0040) Pos = 2;
					else if((Test & 0x1fe0) == 0x0020) Pos = 3;
					else if((Test & 0x0ff0) == 0x0010) Pos = 4;
					else if((Test & 0x07f8) == 0x0008) Pos = 5;
					else if((Test & 0x03fc) == 0x0004) Pos = 6;
					else if((Test & 0x01fe) == 0x0002) Pos = 7;

					if(Pos)
					{
						Uint8 start_code;

						// We have a startcode... but which one?
						if(Pos == 8)
						{
							start_code = BuffPtr[3];
						}
						else
						{
							unsigned int Temp = (BuffPtr[2] << 8) | BuffPtr[3];
							start_code = (Temp >> (8-Pos));
						}

//printf("Found startcode 0x%02x\n", start_code);
						if(!FoundEnd)
						{
							// End of current picture is picture_start or sequence_header or group_start
							if((start_code == 0) || (start_code == 0xb3) || (start_code == 0xb8))
							{
								if(Pos == 8)
								{
									int Temp = int(&BuffPtr[0]) - int(ScanBuffer);
									PictureEnd = BufferStart + Temp;
									PictureEndBitOffset = 0;
								}
								else
								{
									int Temp = int(&BuffPtr[-1]) - int(ScanBuffer);
									PictureEnd = BufferStart + Temp;
									PictureEndBitOffset = Pos;
									error("Only byte aligned picture starts currently supported!\n");
								}
								FoundEnd = true;
							}
						}

						// If this is the picture start code write the previous picture
						if(start_code == 0)
						{
							// Don't write anything until we have found the first picture
							if(PictureStart >= 0)
							{
								// Align with the KAG is we are using one
								if(KAGSize > 1) Out->Align(KAGSize);

//IT# 								Table->AddEntry(EditUnit, Out->Tell() - ECStart);
//IT# 								EditUnit++;

								WriteMPEG(InFile, Out, PictureStart, PictureStartBitOffset, PictureEnd, PictureEndBitOffset);
								PictureCount++;
							}

							// Restart
							PictureStart = PictureEnd;
							PictureStartBitOffset = PictureEndBitOffset;
							FoundEnd = false;
						}
					}
				}
			}

			// Scan through the buffer
			BuffPtr++;
			ScanPos++;
			ScanLeft--;
		}

		// Get another chunk of buffer
		BufferStart += ScanPos;
	}
	
	return PictureCount;
}

void WriteMPEG(FileHandle InFile, MXFFilePtr &Out, Int64 PictureStart, int PictureStartBitOffset, Int64 PictureEnd, int PictureEndBitOffset)
{
	printf("Write: 0x%08x:%d -> 0x%08x:%d\n", (int)PictureStart, PictureStartBitOffset, (int)PictureEnd, PictureEndBitOffset);

	if((PictureStartBitOffset != 0) || (PictureEndBitOffset != 0))
	{
		error("Only byte aligned picture writing currently supported!\n");
		return;
	}

	Int64 Size = PictureEnd - PictureStart;
	Uint8 *Buffer = new Uint8[Size];
	FileSeek(InFile, PictureStart);
	FileRead(InFile, Buffer, Size);

	// Note that the version number of 1 as the registry is not yet available!
	static Uint8 MPEG_ES_Key[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x15, 0x01, 0x05, 0x00 };

	Out->Write(MPEG_ES_Key, 16);
	Out->WriteBER(Size);
	Out->Write(Buffer, Size);
}


MDObjectPtr BuildMPEGEssenceDescriptor(FileHandle InFile)
{
	MDObjectPtr Ret;
	Uint8 Buffer[128];

	// Read the start of the file
	FileRead(InFile, Buffer, 128);

	if((Buffer[0] != 0) || (Buffer[1] != 0) || (Buffer[2] != 1) || (Buffer[3] != 0xb3))
	{
		error("Current implementation only supports essence starting with a sequence header\n");
		return Ret;
	}

	Uint32 HSize = (Buffer[4] << 4) | (Buffer[5] >> 4);
	Uint32 VSize = ((Buffer[5] & 0x0f) << 8) | (Buffer[6]);

	char *Aspect;
	switch(Buffer[7] & 0xf0)
	{
	default: Aspect = NULL; break;
	case 0x10: Aspect = "1/1"; break;
	case 0x20: Aspect = "4/3"; break;
	case 0x30: Aspect = "16/9"; break;
	case 0x40: Aspect = "221/100"; break;
	}

	int FrameRate = 0;
	bool DropFrame = false;
	switch(Buffer[7] & 0x0f)
	{
	case 0x01: FrameRate = 24; DropFrame = true; break;
	case 0x02: FrameRate = 24; break;
	case 0x03: FrameRate = 25; break;
	case 0x04: FrameRate = 30; DropFrame = true; break;
	case 0x05: FrameRate = 30; break;
	case 0x06: FrameRate = 50; break;
	case 0x07: FrameRate = 60; DropFrame = true; break;
	case 0x08: FrameRate = 60; break;
	}

	if(FrameRate == 0) error("Unknown frame rate!\n");

	Uint32 BitRate = (Buffer[8] << 10) | (Buffer[9] << 2) | (Buffer[10] >> 6);

	if(BitRate = 0x3ffff) warning("bit_rate = -1\n");

	// Assume some values if no extension found
	Uint8 PandL = 0;
	bool Progressive = true;
	int HChromaSub = 2;
	int VChromaSub = 2;
	bool LowDelay = false;

	Uint8 LoadIntra = Buffer[11] & 0x04;
	Uint8 LoadNonIntra;
	if(LoadIntra == 0)
	{
		LoadNonIntra = Buffer[11] & 0x02;
	}
	else
	{
		LoadNonIntra = Buffer[11 + 64] & 0x02;
	}

	int ExtPos = 12;
	if(LoadIntra) ExtPos += 64;
	if(LoadNonIntra) ExtPos += 64;

	FileSeek(InFile, ExtPos);
	// Read the sequence extention
	FileRead(InFile, Buffer, 128);

	if((Buffer[0] != 0) || (Buffer[1] != 0) || (Buffer[2] != 1) || (Buffer[3] != 0xb5))
	{
		error("Sequence extension does not follow sequence header - some assumptions made\n");
	}
	else
	{
		PandL = (Buffer[4] << 4) | (Buffer[5] >> 4);
		
		if(Buffer[5] & 0x08) Progressive = true; else Progressive = false;

		int Sub = ((Buffer[5] & 0x01) << 1) | (Buffer[6] >> 7);
		if(Sub >= 2) VChromaSub = 1;
		if(Sub == 3) HChromaSub = 1;

		HSize |= ((Buffer[5] & 0x01) << 13) | ((Buffer[6] & 0x80) << 5);
		VSize |= ((Buffer[6] & 0x60) << 7);
		BitRate |= ((Buffer[6] & 0x1f) << 25) | ((Buffer[7] & 0xfe) << 17);

		if(Buffer[9] & 0x80) LowDelay = true;

		int FR_n = ((Buffer[9] & 0x60) >> 5) + 1;
		int FR_d = (Buffer[9] & 0x1f) + 1;

		FrameRate *= FR_n;
		FrameRate /= FR_d;
	}

/*
if(PandL) printf("Profile-and-Level = 0x%02x\n", PandL); else printf("Profile-and-Level unknown!!\n");
printf("HSize = %d\n", HSize);
printf("VSize = %d\n", VSize);
if(Aspect) printf("Aspect Ratio = %s\n", Aspect); else printf("Aspect Ratio unknown!!\n");
printf("BitRate = %u\n", BitRate * 400);
printf("Low Delay mode %s\n", LowDelay ? "is used" : "is not used");
printf("FrameRate = %d%s\n", FrameRate, DropFrame ? " * 1000/1001" : "");
printf("Frame Layout = %s\n", Progressive ? "Progressive" : "Interlaced");
printf("Chroma horizontal sub-sampling = %d\n", HChromaSub);
printf("Chroma vertical sub-sampling = %d\n", VChromaSub);
*/

	// Build the essence descriptor, filling in all known values

	Ret = new MDObject("MPEG2VideoDescriptor");
	if(!Ret) return Ret;

	char Buff[32];
	if(DropFrame)
	{
		sprintf(Buff, "%d000/1001", FrameRate);
		Ret->SetString("SampleRate", Buff);
	}
	else
	{
		sprintf(Buff, "%d/1", FrameRate);
		Ret->SetString("SampleRate", Buff);
	}

	if(Progressive) Ret->SetInt("FrameLayout", 0); else Ret->SetInt("FrameLayout", 1);

	Ret->SetUint("StoredWidth", HSize);
	Ret->SetUint("StoredHeight", VSize);

	if(Aspect) Ret->SetString("AspectRatio", Aspect); else Ret->SetDValue("AspectRatio");

	if(VSize == 576)
		Ret->SetString("VideoLineMap", "1, 313");
	else if(VSize == 480)
		Ret->SetString("VideoLineMap", "4, 266");
//	else if(VSize == 720)
//		Ret->SetString("VideoLineMap", "1, 0");
//	else if((VSize == 1080) && Progressive)
//		Ret->SetString("VideoLineMap", "1, 0");
//	else if(VSize == 1080)
//		Ret->SetString("VideoLineMap", "1, 564");
	else Ret->SetDValue("VideoLineMap");

	Ret->SetUint("ComponentDepth", 8);

	Ret->SetUint("HorizontalSubSampling", HChromaSub);
	Ret->SetUint("VerticalSubSampling", VChromaSub);

	if((HChromaSub == 2) && (VChromaSub == 2))
		Ret->SetUint("ColorSiting", 3);				// Quincunx 4:2:0
	else if((HChromaSub == 2) && (VChromaSub == 1))
		Ret->SetUint("ColorSiting", 4);				// Rec 601 style 4:2:2
	if((HChromaSub == 1) && (VChromaSub == 1))
		Ret->SetUint("ColorSiting", 0);				// 4:4:4

	if(Progressive)	Ret->SetUint("CodedContentType", 1); else 	Ret->SetUint("CodedContentType", 2);
	if(LowDelay)	Ret->SetUint("LowDelay", 1); else	Ret->SetUint("LowDelay", 0);

	if(BitRate != 0x3ffff) Ret->SetUint("BitRate", BitRate * 400);

	Ret->SetUint("ProfileAndLevel", PandL);

	return Ret;
}



// Debug and error messages
#include <stdarg.h>

#ifdef MXFLIB_DEBUG
//! Display a general debug message
void mxflib::debug(const char *Fmt, ...)
{
	if(!DebugMode) return;

	va_list args;

	va_start(args, Fmt);
	vprintf(Fmt, args);
	va_end(args);
}
#endif MXFLIB_DEBUG

//! Display a warning message
void mxflib::warning(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("Warning: ");
	vprintf(Fmt, args);
	va_end(args);
}

//! Display an error message
void mxflib::error(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("ERROR: ");
	vprintf(Fmt, args);
	va_end(args);
}


