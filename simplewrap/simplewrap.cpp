/*! \file	simplewrap.cpp
 *	\brief	Simple MXF essence wrapping utility
 *
 *	\version $Id: simplewrap.cpp,v 1.3 2006/09/04 13:59:36 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2006, Matt Beard
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

#include <mxflib/mxflib.h>

using namespace mxflib;

#include <stdio.h>

using namespace std;

// Product GUID and version text for this release
UInt8 ProductGUID_Data[16] = { 0x84, 0x55, 0x23, 0xe2, 0x16, 0x8c, 0xc2, 0x30, 0x85, 0xcb, 0xef, 0x78, 0x9c, 0xde, 0xef, 0x42 };
string CompanyName = "freeMXF.org";
string ProductName = "simplewrap file wrapper";
string ProductVersion = "Based on " + LibraryVersion();
string PlatformName = "MXFLib (" + OSName() + ")";

//! Debug flag for MXFLib
static bool DebugMode = false;


//! Wrap the file
int main(int argc, char *argv[]) 
{ 
	/***************************************************/
	/**  INITIAL SETUP - Parse the command line etc.  **/
	/***************************************************/

	printf("MXFlib Simple Wrapper\n" );

	char *SourceFile = NULL;
	char *DestFile = NULL;
	
	// Parse the command line
	for(int i=1; i<argc; i++)
	{
		// Parse options
		if(argv[i][0] == '-')
		{
			char *p = &argv[i][1];					// The option less the '-' or '/'
			char Opt = tolower(*p);					// The option itself (in lower case)
			if(Opt == 'v') DebugMode = true;
			else 
			{
				fprintf(stderr, "Unknown option %s\n", argv[i]);
				return 1;
			}
		}
		else
		{
			// Parse filenames
			if(SourceFile == NULL)
			{
				SourceFile = argv[i];
			}
			else if(DestFile == NULL)
			{
				DestFile = argv[i];
			}
			else
			{
				fprintf(stderr, "Too many filenames\n");
				return 1;
			}
		}
	}

	// If two filenames were not supplied, give usage instructions
	if(DestFile == NULL)
	{
		fprintf(stderr, "\nUsage: %s [-v] <infile> <outfile>\n\n", argv[0]);

		fprintf(stderr, "Where: -v enables verbose mode (debug output)\n");
		fprintf(stderr, "       <infile> is the essence file to wrap\n");
		fprintf(stderr, "       <outfile> is the output file to produce\n");

		return 1;
	}

	// Load the dictionary
	LoadDictionary("dict.xml");


	/*********************************************/
	/**       IDENTIFY THE SOURCE ESSENCE       **/
	/*********************************************/

	// Open the file to be parsed
	FileHandle InFile = FileOpen(SourceFile);
	if(!FileValid(InFile))
	{
		error("Couldn't open source file %s\n", SourceFile);
		return 1;
	}

	// Identify the essence
	ParserDescriptorListPtr PDList = EssenceParser::IdentifyEssence(InFile);

	// If the descriptor list is empty, the parser couldn't identify the essence
	if(PDList->size() == 0)
	{
		error("Couldn't identify the essence in %s\n", SourceFile);
		return 1;
	}

	// Select appropriate wrapping options for this essence
	WrappingConfigPtr WrapConfig = EssenceParser::SelectWrappingOption(InFile, PDList);

	// If a no configuration is returned, the parser couldn't find a valid wrapping
	if(!WrapConfig)
	{
		error("Couldn't identify a suitable wrapping for essence in %s\n", SourceFile);
		return false;
	}

	// Get the EssenceSource for this wrapping
	EssenceSourcePtr Source = WrapConfig->Parser->GetEssenceSource(InFile, WrapConfig->Stream);


	/*********************************************/
	/**          SET UP ESSENCE WRITING         **/
	/*********************************************/

	// Enable FastClipWrap mode - don't do this if random access not available of the output medium
	SetFastClipWrap(true);

	// Open the destination MXF file
	MXFFilePtr OutFile = new MXFFile;
	if(!OutFile->OpenNew(DestFile))
	{
		error("Couldn't open source file %s\n", SourceFile);
		return 1;
	}

	// Build a new BodyStream with BodySID = 1
	BodyStreamPtr Stream = new BodyStream(1, Source);

	// Set the wrapping type
	Stream->SetWrapType(WrapConfig->WrapOpt->ThisWrapType);

	// Build a new BodyWriter attached to the destination file
	BodyWriterPtr Writer = new BodyWriter(OutFile);

	// Set the writer's general parameters, KAG=1 and set all essence BERs to 4-byte
	Writer->SetKAG(1);
	Writer->SetForceBER4(true);

	// Allow essence and metadata to share partitions (makes a slightly smaller file)
	Writer->SetMetadataSharing(true, true);

	// Add the essence stream to the writer
	Writer->AddStream(Stream);


	/*********************************************/
	/**       BUILD BASIC HEADER METADATA       **/
	/*********************************************/

	MetadataPtr MData = new Metadata();


	/*  Set as OP1a */

	UInt8 OP1a_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x00 };
	ULPtr OP1aUL = new UL(OP1a_Data);
	
	MData->SetOP(OP1aUL);

	
	/* Build UMIDs for the material and file packages */

	UMIDPtr MPUMID;
	UMIDPtr FPUMID;
	
	if(Source->IsPictureEssence())
	{
		MPUMID = MakeUMID( 0x01 ); // Picture essence
		FPUMID = MakeUMID( 0x01 ); // Picture essence
	}
	else if(Source->IsSoundEssence())
	{
		MPUMID = MakeUMID( 0x02 ); // Picture essence
		FPUMID = MakeUMID( 0x02 ); // Picture essence
	}
	else if(Source->IsDataEssence())
	{
		MPUMID = MakeUMID( 0x03 ); // Picture essence
		FPUMID = MakeUMID( 0x03 ); // Picture essence
	}
	else
	{
		MPUMID = MakeUMID( 0x0f ); // "Not identified" essence
		FPUMID = MakeUMID( 0x0f ); // "Not identified" essence
	}


	/* Add packages */

	PackagePtr MaterialPackage = MData->AddMaterialPackage(MPUMID);
	PackagePtr FilePackage = MData->AddFilePackage(1, FPUMID);

	// Add MP timecode track
	TrackPtr MPTimecodeTrack = MaterialPackage->AddTimecodeTrack(WrapConfig->EditRate);
	TimecodeComponentPtr MPTimecodeComponent = MPTimecodeTrack->AddTimecodeComponent();

	// Add FP timecode track
	TrackPtr FPTimecodeTrack = FilePackage->AddTimecodeTrack(WrapConfig->EditRate);
	TimecodeComponentPtr FPTimecodeComponent = FPTimecodeTrack->AddTimecodeComponent();


	/* Add essence tracks */

	TrackPtr MPEssenceTrack;
	TrackPtr FPEssenceTrack;
	if(Source->IsPictureEssence())
	{
		MPEssenceTrack = MaterialPackage->AddPictureTrack(WrapConfig->EditRate);
		FPEssenceTrack = FilePackage->AddPictureTrack(Stream->GetTrackNumber(), WrapConfig->EditRate);
	}
	else if(Source->IsSoundEssence())
	{
		MPEssenceTrack = MaterialPackage->AddSoundTrack(WrapConfig->EditRate);
		FPEssenceTrack = FilePackage->AddSoundTrack(Stream->GetTrackNumber(), WrapConfig->EditRate);
	}
	// We assume anything not picture or sound is data!
	else
	{
		MPEssenceTrack = MaterialPackage->AddDataTrack(WrapConfig->EditRate);
		FPEssenceTrack = FilePackage->AddDataTrack(Stream->GetTrackNumber(), WrapConfig->EditRate);
	}


	// Add source clips
	SourceClipPtr MPClip = MPEssenceTrack->AddSourceClip();
	SourceClipPtr FPClip = FPEssenceTrack->AddSourceClip();


	// Link the MP clip to the FP track
	MPClip->MakeLink(FPEssenceTrack, 0);


	// Add the descriptor to the file package
	WrapConfig->EssenceDescriptor->SetUInt(LinkedTrackID_UL, FPEssenceTrack->GetUInt(TrackID_UL));
	FilePackage->AddChild(Descriptor_UL)->MakeLink(WrapConfig->EssenceDescriptor);


	// Add the essecne type
	MData->AddEssenceType(WrapConfig->WrapOpt->WrappingUL);


	// Set the material package as the primary package
	MData->AddChild(PrimaryPackage_UL)->MakeLink(MaterialPackage->Object);


	/* Build an Ident set describing us and link into the metadata */

	MDObjectPtr Ident = new MDObject(Identification_UL);
	Ident->SetString(CompanyName_UL, CompanyName);
	Ident->SetString(ProductName_UL, ProductName);
	Ident->SetString(VersionString_UL, ProductVersion);
	Ident->SetString(ToolkitVersion_UL, LibraryProductVersion());
	Ident->SetString(Platform_UL, PlatformName);
	UUIDPtr ProductUID = new mxflib::UUID(ProductGUID_Data);

	MData->UpdateGenerations(Ident);


	/* Use this metadata to build a template partition pack */

	PartitionPtr ThisPartition = new Partition(OpenHeader_UL);
	ThisPartition->SetKAG(1);
	ThisPartition->SetUInt(BodySID_UL, 1);

	ThisPartition->AddMetadata(MData);

	// Add the template partition pack, with associated metadata
	Writer->SetPartition(ThisPartition);


	/*********************************************/
	/**        WRITE THE HEADER AND BODY        **/
	/*********************************************/
	
	// Write the header
	Writer->WriteHeader(false, false);

	// Write the body
	Writer->WriteBody();


	/*********************************************/
	/**      UPDATE THE METADATA DURATIONS      **/
	/*********************************************/
	
	// Get the duration from the essence source position
	Length EssenceDuration = static_cast<Length>(Source->GetCurrentPosition());

	// Set the material package timecode track duration
	MPTimecodeComponent->SetDuration(EssenceDuration);

	// Set the material package essence track source clip duration
	MPClip->SetDuration(EssenceDuration);

	// Set the file package timecode track duration
	FPTimecodeComponent->SetDuration(EssenceDuration);

	// Set the file package essence track source clip duration
	FPClip->SetDuration(EssenceDuration);

	// Set the essence descriptor duration
	WrapConfig->EssenceDescriptor->SetInt64(ContainerDuration_UL,EssenceDuration);

	// Update the modification time
	MData->SetTime();

	// Update the generation UIDs in the metadata to reflect the changes
	MData->UpdateGenerations(Ident);

	// Write the footer, with updated metadata (and a RIP)
	Writer->WriteFooter(true, true);


	/*********************************************/
	/**                CLEAN UP                 **/
	/*********************************************/

	// Close the destination file
	OutFile->Close();

	// Clsoe the source file
	FileClose(InFile);

	// Return success
	return 0;
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
#endif // MXFLIB_DEBUG

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


