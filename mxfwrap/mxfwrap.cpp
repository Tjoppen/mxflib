/*! \file	mxfwrap.cpp
 *	\brief	Basic MXF essence wrapping utility
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


// Product GUID and version text for this release
Uint8 ProductGUID_Data[16] = { 0x84, 0x66, 0x14, 0xf3, 0x27, 0xdd, 0xde, 0x40, 0x86, 0xdc, 0xe0, 0x99, 0xda, 0x7f, 0xd0, 0x52 };
std::string ProductVersion = " Unreleased 0.2";

bool ParseCommandLine(int &argc, char **argv);


struct BodyWrapping
{
	GCWriterPtr Writer;
	GCStreamID EssenceID;
	EssenceParser::WrappingConfigPtr Config;
	FileHandle InFile;
	Uint32 BodySID;

	//! The mode of body partition insertion
	enum PartitionMode { Body_None, Body_Duration, Body_Size } BodyMode;
	Uint32 BodyRate;				//!< The rate of body partition insertion
};
typedef std::list<BodyWrapping> BodyWrappingList;
Int64 WriteBody(MXFFilePtr Out, BodyWrappingList WrappingList, PartitionPtr ThisPartition, Int64 Duration = 0);


// Options
Uint32 KAGSize = 1;						//!< The KAG Size for this file
char InFilenameSet[512];				//!< The set of input filenames
int InFileGangSize;						//!< The number of ganged files to process at a time
int InFileGangCount;					//!< The number of sets of ganged files to process
char InFilename[16][128];				//!< The list of input filenames
char OutFilenameSet[512];				//!< The set of output filenames
char OutFilename[16][128];				//!< The output filename
int OutFileCount;						//!< The number of files to output

bool OPAtom = false;					//!< Is OP-Atom mode being forced?
bool StreamMode = false;				//!< Wrap in stream-mode
bool EditAlign = false;					//!< Start new body partitions only at the start of a GOP
bool UseIndex = false;					//!< Write index tables
bool SparseIndex = false;				//!< Write sparse index tables (one entry per partition)

// DRAGONS: Temporary option!
bool FrameGroup = false;				//!< Group all as a frame-wrapped group (in one essence container)

Rational ForceEditRate;					//!< Edit rate to try and force

//! The mode of body partition insertion
//enum { Body_None, Body_Duration, Body_Size } 
BodyWrapping::PartitionMode BodyMode = BodyWrapping::Body_None;
Uint32 BodyRate = 0;					//!< The rate of body partition insertion

Uint32 HeaderPadding = 0;				//!< The (minimum) number of bytes of padding to leave in the header


// Derived options
ULPtr OPUL;								//!< The UL of the OP for this file



// Operational Pattern Labels
// ==========================

// OP-Atom - #### DRAGONS: Qualifiers need work later!
Uint8 OPAtom_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00 };
ULPtr OPAtomUL = new UL(OPAtom_Data);

// OP1a - #### DRAGONS: Qualifiers may need work!
Uint8 OP1a_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x00 };
ULPtr OP1aUL = new UL(OP1a_Data);

// OP1b - #### DRAGONS: Qualifiers need work!
Uint8 OP1b_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x02, 0x05, 0x00 };
ULPtr OP1bUL = new UL(OP1b_Data);

// OP2a - #### DRAGONS: Qualifiers need work!
Uint8 OP2a_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x02, 0x01, 0x01, 0x00 };
ULPtr OP2aUL = new UL(OP2a_Data);

// OP2b - #### DRAGONS: Qualifiers need work!
Uint8 OP2b_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x02, 0x02, 0x05, 0x00 };
ULPtr OP2bUL = new UL(OP2b_Data);



//IT# // The index table
//IT# IndexTablePtr Table;

class ES2 : public EssenceSource
{
public:
	virtual Uint64 GetEssenceDataSize(void) { return 0; };

		//! Get the next "installment" of essence data
		/*! /ret Pointer to a data chunk holding the next data or a NULL pointer when no more remains
		 *	/note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
		 *	/note If Size = 0 the object will decide the size of the chunk to return
		 *	/note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
		 */
	virtual DataChunkPtr GetEssenceData(Uint64 Size = 0, Uint64 MaxSize = 0) { return NULL; };

	virtual ~ES2() { printf("Sub destructor called\n"); };
};


int main(int argc, char *argv[])
{
	printf("Simple MXF wrapping application\n\n");

/*{
	DV_DIF_EssenceSubParser SPB;

	DV_DIF_EssenceSubParser::ESP_EssenceSource *X = new DV_DIF_EssenceSubParser::ESP_EssenceSource(&SPB, 0, 0, 1);

	printf("Size = %d\n", X->GetEssenceDataSize());

	DataChunkPtr Data = X->GetEssenceData();

	printf("Read %d bytes\n", (int)Data->Size);

	Uint8 *Z = (Uint8*)X;

	EssenceSource *Y = (EssenceSource*)Z;

	printf("b");

	delete Y;

	printf("c\n");
}*/

	// Build an essence parser
	EssenceParser EssParse;

	// Load the dictionaries
	LoadTypes("types.xml");
	MDOType::LoadDict("XMLDict.xml");

//IT# 	// Clear the index table
//IT# 	Table = new IndexTable();

	// Parse command line options and exit on error
	ForceEditRate.Numerator = 0;
	if(!ParseCommandLine(argc, argv)) return -1;

	EssenceParser::WrappingConfigList WrappingList;
	EssenceParser::WrappingConfigList::iterator WrappingList_it;

	// File handles
	FileHandle InFile[16];

	// The edit rate for all tracks in this file
	Rational EditRate;

	// Identify the wrapping options
	// DRAGONS: Not flexible yet
	int i;
	for(i=0; i<InFileGangSize; i++)
	{
		// Open the input file
		InFile[i] = FileOpenRead(InFilename[i]);
		if(!FileValid(InFile[i]))
		{
			error("Can't open input file \"%s\"\n", InFilename[i]);
			return -2;
		}

		// Build a list of parsers with their descriptors for this essence
		ParserDescriptorListPtr PDList = EssParse.IdentifyEssence(InFile[i]);

		if(PDList->empty())
		{
			error("Could not identify the essence in file \"%s\"\n", InFilename[i]);
			return -3;
		}

		EssenceParser::WrappingConfigPtr WCP;
		if(FrameGroup) WCP = EssParse.SelectWrappingOption(InFile[i], PDList, ForceEditRate, WrappingOption::WrapType::Frame);
		else WCP = EssParse.SelectWrappingOption(InFile[i], PDList, ForceEditRate);

// Fixed now ? ## When PDList is deleted so is the essence parser...

		if(!WCP)
		{
			error("Could not identify a wrapping mode for the essence in file \"%s\"\n", InFilename[i]);
			return -4;
		}

		// Ensure the essence descriptor reflects the new wrapping
		WCP->EssenceDescriptor->SetValue("EssenceContainer", DataChunk(16,WCP->WrapOpt->WrappingUL->GetValue()));

		// Add this wrapping option
		WrappingList.push_back(WCP);

		// Edit rate for this file
		EditRate = WCP->EditRate;

		// DRAGONS: Once we have set the edit rate for the first file we force it on the rest
		ForceEditRate = EditRate;

		printf("\nSelected wrapping for file \"%s\" : %s\n", InFilename[i], WCP->WrapOpt->Description.c_str());
	}

	// Generate UMIDs for each file package
	UMIDPtr FPUMID[16];								//! UMIDs for each file package (internal or external)
	i = 0;				//  Essence container and track index
	WrappingList_it = WrappingList.begin();
	while(WrappingList_it != WrappingList.end())
	{
		switch((*WrappingList_it)->WrapOpt->GCEssenceType)
		{
		case 0x05: case 0x15:
			FPUMID[i] = MakeUMID(1);
			break;
		case 0x06: case 0x16:
			FPUMID[i] = MakeUMID(2);
			break;
		case 0x07: case 0x17:
			FPUMID[i] = MakeUMID(3);
			break;
		case 0x18: default:
			FPUMID[i] = MakeUMID(4);
			break;
		}

		WrappingList_it++;
		i++;

		// DRAGONS: not the right way to do file package reduction!
		if(FrameGroup) break;
	}

	Int64 Duration = 0;
	int OutFileNum;
	for(OutFileNum=0; OutFileNum < OutFileCount ; OutFileNum++)
	{
		// Open the output file
		MXFFilePtr Out = new MXFFile;
		if(!Out->OpenNew(OutFilename[OutFileNum]))
		{
			error("Can't open output file \"%s\"\n", OutFilename[OutFileNum]);
			return -5;
		}

		// Create a set of header metadata
		MetadataPtr MData = new Metadata();
		ASSERT(MData);
		ASSERT(MData->Object);

		// Set the OP label
		// If we are writing OP-Atom we write the header as OP1a initially as another process
		// may try to read the file before it is complete and then it will NOT be a valid OP-Atom file
		if(OPAtom) MData->SetOP(OP1aUL); else MData->SetOP(OPUL);

		// Work out the edit rate from the descriptor
		bool DropFrame = 0;
		Uint32 FrameRate = EditRate.Numerator;

		// Use drop-frame for any non-integer frame rate
		if(EditRate.Denominator > 1)
		{
			FrameRate /= EditRate.Denominator;
			DropFrame = true;
		}

		// Build the Material Package
		PackagePtr MaterialPackage = MData->AddMaterialPackage("Material Package");

		MData->SetPrimaryPackage(MaterialPackage);		// This will be overwritten for OP-Atom

		TrackPtr MPTimecodeTrack = MaterialPackage->AddTimecodeTrack(EditRate);
		TimecodeComponentPtr MPTimecodeComponent = MPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, 0);

		// Build the File Packages and all essence tracks
		GCWriterPtr Writer[16];							//! Writers for each essence container
		GCStreamID EssenceID[16];						//! Essence stream ID for each essence stream
		TimecodeComponentPtr FPTimecodeComponent[16];	//! Timecode component for each file package
		TrackPtr MPTrack[16];							//! Material Package track for each essence stream
		TrackPtr FPTrack[16];							//! File Package track for each essence stream
		SourceClipPtr MPClip[16];						//! Material Package SourceClip for each essence stream 
		SourceClipPtr FPClip[16];						//! File Package SourceClip for each essence stream 

		i = 0;				//  Essence container and track index
		WrappingList_it = WrappingList.begin();
		PackagePtr FilePackage;
		while(WrappingList_it != WrappingList.end())
		{
			TrackPtr FPTimecodeTrack;

			// Don't write file packages for externally reffed essence in OP-Atom
			bool WriteFP = (!OPAtom) || (i == OutFileNum);

			if(WriteFP)
			{
				// Set up a writer for body SID (i + 1)
				if(FrameGroup) Writer[i] = new GCWriter(Out, 1);
				else Writer[i] = new GCWriter(Out, i + 1);

				// Set the KAG size and force 4-byte BER lengths (for maximum compatibility)
				Writer[i]->SetKAG(KAGSize, true);

				// Add an essence element
				EssenceID[i] = Writer[i]->AddEssenceElement((*WrappingList_it)->WrapOpt->GCEssenceType, (*WrappingList_it)->WrapOpt->GCElementType);

				// DRAGONS: not the right way to do file package reduction!
				if((!FrameGroup) || (i == 0))
				{
					FilePackage = MData->AddFilePackage(i+1, std::string("File Package: ") + (*WrappingList_it)->WrapOpt->Description, FPUMID[i]);

					FPTimecodeTrack = FilePackage->AddTimecodeTrack(EditRate);
					FPTimecodeComponent[i] = FPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, 0);
				}
			}

			switch((*WrappingList_it)->WrapOpt->GCEssenceType)
			{
			case 0x05: case 0x15:
				MPTrack[i] = MaterialPackage->AddPictureTrack(EditRate);
				if(WriteFP) FPTrack[i] = FilePackage->AddPictureTrack(Writer[i]->GetTrackNumber(EssenceID[i]), EditRate);
				break;
			case 0x06: case 0x16:
				MPTrack[i] = MaterialPackage->AddSoundTrack(EditRate);
				if(WriteFP) FPTrack[i] = FilePackage->AddSoundTrack(Writer[i]->GetTrackNumber(EssenceID[i]), EditRate);
				break;
			case 0x07: case 0x17: default:
				MPTrack[i] = MaterialPackage->AddDataTrack(EditRate);
				if(WriteFP) FPTrack[i] = FilePackage->AddDataTrack(Writer[i]->GetTrackNumber(EssenceID[i]), EditRate);
				break;
	//		case 0x18:
	//			MPTrack[i] = MaterialPackage->AddCompoundTrack(EditRate);
	//			if(WriteFP) FPTrack[i] = FilePackage->AddCompoundTrack(Writer[i]->GetTrackNumber(EssenceID[i]), EditRate);
	//			break;
			}

			MPClip[i] = MPTrack[i]->AddSourceClip();
			if(WriteFP) FPClip[i] = FPTrack[i]->AddSourceClip();

			// Add the file descriptor to the file package
			// Don't add for external refs in OP-Atom
			if(WriteFP)
			{
				if(FrameGroup)
				{
					if(i == 0)
					{
						MDObjectPtr MuxDescriptor = new MDObject("MultipleDescriptor");
						MuxDescriptor->AddChild("SampleRate")->SetInt("Numerator",(*WrappingList_it)->EssenceDescriptor["SampleRate"]->GetInt("Numerator"));
						MuxDescriptor->AddChild("SampleRate")->SetInt("Denominator",(*WrappingList_it)->EssenceDescriptor["SampleRate"]->GetInt("Denominator"));
						// DRAGONS: What to do about multiple descriptor container UL?
						MuxDescriptor->AddChild("SubDescriptorUIDs");
						FilePackage->AddChild("Descriptor")->MakeLink(MuxDescriptor);
					}

					(*WrappingList_it)->EssenceDescriptor->SetUint("LinkedTrackID", FPTrack[i]->GetUint("TrackID"));
					
					MDObjectPtr MuxDescriptor = FilePackage["Descriptor"]->GetLink();

					MuxDescriptor["SubDescriptorUIDs"]->AddChild("SubDescriptorUID", false)->MakeLink((*WrappingList_it)->EssenceDescriptor);

					MData->AddEssenceType((*WrappingList_it)->WrapOpt->WrappingUL);

					// Link the MP to the FP
					MPClip[i]->MakeLink(FPTrack[i], 0);
				}
				else
				{
					(*WrappingList_it)->EssenceDescriptor->SetUint("LinkedTrackID", FPTrack[i]->GetUint("TrackID"));
					FilePackage->AddChild("Descriptor")->MakeLink((*WrappingList_it)->EssenceDescriptor);

					MData->AddEssenceType((*WrappingList_it)->WrapOpt->WrappingUL);

					// Link the MP to the FP
					MPClip[i]->MakeLink(FPTrack[i], 0);
				}
			}
			else
			{
				// Link the MP to the FP
				// DRAGONS: This assumes that the linked track will be track 2
				MPClip[i]->MakeLink(FPUMID[i], 2, 0);
			}

			WrappingList_it++;
			i++;
		}


		//
		// ** Write out the header **
		//

		PartitionPtr ThisPartition = new Partition("OpenHeader");
		ASSERT(ThisPartition);
		ThisPartition->SetKAG(KAGSize);			// Everything else can stay at default
		ThisPartition->SetUint("BodySID", 1);

		ThisPartition->AddMetadata(MData);

		// Build an Ident set describing us and link into the metadata
		MDObjectPtr Ident = new MDObject("Identification");
		Ident->SetString("CompanyName", "freeMXF.org");
		Ident->SetString("ProductName", "mxfwrap");
		Ident->SetString("VersionString", ProductVersion);
		UUIDPtr ProductUID = new mxflib::UUID(ProductGUID_Data);

		// DRAGONS: -- Need to set a proper GUID per released version
		//             Non-released versions currently use a random GUID
		//			   as they are not a stable version...
		Ident->SetValue("ProductUID", DataChunk(16,ProductUID->GetValue()));

		// Link the new Ident set with all new metadata
		MData->UpdateGenerations(Ident);

		// Write the header partition
		Out->WritePartition(ThisPartition, HeaderPadding);


		//
		// ** Process Essence **
		//

		// Do all frame-wrappings first
		BodyWrappingList WrapConfig;
		WrappingList_it = WrappingList.begin();
		i=0;
		while(WrappingList_it != WrappingList.end())
		{
			if((*WrappingList_it)->WrapOpt->ThisWrapType == WrappingOption::Frame)
			{
				if((!OPAtom) || (i == OutFileNum))
				{
					BodyWrapping BW;

					BW.Writer = Writer[i];
					BW.EssenceID = EssenceID[i];
					BW.Config = (*WrappingList_it);
					BW.InFile = InFile[i];

					if(FrameGroup) BW.BodySID = 1;
					else BW.BodySID = i+1;

					BW.BodyMode = BodyMode;
					BW.BodyRate = BodyRate;

					WrapConfig.push_back(BW);
				}
			}

			WrappingList_it++;
			i++;
		}

		// Write all frame non-wrapped/clip items
		if(!WrapConfig.empty()) Duration = WriteBody(Out, WrapConfig, ThisPartition);


		// Non-clip-wrapped items
		WrapConfig.clear();
		WrappingList_it = WrappingList.begin();
		i=0;
		while(WrappingList_it != WrappingList.end())
		{
			if(    ((*WrappingList_it)->WrapOpt->ThisWrapType != WrappingOption::Frame)
				&& ((*WrappingList_it)->WrapOpt->ThisWrapType != WrappingOption::Clip))
			{
				if((!OPAtom) || (i == OutFileNum))
				{
					BodyWrapping BW;

					BW.Writer = Writer[i];
					BW.EssenceID = EssenceID[i];
					BW.Config = (*WrappingList_it);
					BW.InFile = InFile[i];
					BW.BodySID = i+1;

					WrapConfig.push_back(BW);
				}
			}

			WrappingList_it++;
			i++;
		}

		// Write all non-wrapped/clip items
		if(!WrapConfig.empty()) WriteBody(Out, WrapConfig, ThisPartition, Duration);


		// Clip wrappings
		WrapConfig.clear();
		WrappingList_it = WrappingList.begin();
		i=0;
		while(WrappingList_it != WrappingList.end())
		{
			if(((*WrappingList_it)->WrapOpt->ThisWrapType == WrappingOption::Clip))
			{
				if((!OPAtom) || (i == OutFileNum))
				{
					BodyWrapping BW;

					BW.Writer = Writer[i];
					BW.EssenceID = EssenceID[i];
					BW.Config = (*WrappingList_it);
					BW.InFile = InFile[i];
					BW.BodySID = i+1;

					WrapConfig.push_back(BW);
				}
			}

			WrappingList_it++;
			i++;
		}

		// Write all clip wrapped items
		if(!WrapConfig.empty()) WriteBody(Out, WrapConfig, ThisPartition, Duration);


		//
		// Write out a set of index tables
		//

		DataChunk IndexChunk;
		Uint32 IndexSID;

		if(UseIndex)
		{
			// Find all essence container data sets so we can update "IndexSID"
			MDObjectListPtr ECDataSets;
			MDObjectPtr Ptr = MData["ContentStorage"];
			if(Ptr) Ptr = Ptr->GetLink();
			if(Ptr) Ptr = Ptr["EssenceContainerData"];
			if(Ptr) ECDataSets = Ptr->ChildList("EssenceContainer");

			WrappingList_it = WrappingList.begin();
			IndexSID = 129;
			i = 0;
			while(WrappingList_it != WrappingList.end())
			{
				if((!OPAtom) || (i == OutFileNum))
				{
					if(Writer[i]->Index)
					{
						if((Writer[i]->Index->EditUnitByteCount != 0) || (!Writer[i]->Index->SegmentMap.empty()))
						{
							if(IndexChunk.Size)
							{
								// Write the index in a partition of its own

								// Work out how big the index (and associated filler) will be
								Uint64 IndexSize = IndexChunk.Size + Out->FillerSize(IndexChunk.Size, KAGSize);

								ThisPartition->ChangeType("ClosedCompleteBodyPartition");
								ThisPartition->SetUint("BodySID", 0);
								ThisPartition->SetUint("IndexSID",  IndexSID);
								ThisPartition->SetUint("IndexByteCount",  IndexSize);

								Out->WritePartition(ThisPartition, false);

								if(KAGSize > 1) Out->Align(KAGSize);

								Out->Write(IndexChunk);

								if(KAGSize > 1) Out->Align(KAGSize);
							}

							Writer[i]->Index->WriteIndex(IndexChunk);
	//@printf("Index is %d\n", IndexChunk.Size);

							// Update IndexSID in essence container data set
							if(ECDataSets)
							{
								MDObjectList::iterator ECD_it = ECDataSets->begin();
								while(ECD_it != ECDataSets->end())
								{
									if((*ECD_it)->GetLink())
									{
										int BodySID;
										if(FrameGroup) BodySID = 1; else BodySID = i + 1;

										if((*ECD_it)->GetLink()->GetInt("BodySID") == BodySID)
										{
											(*ECD_it)->GetLink()->SetInt("IndexSID", IndexSID);
											break;
										}
									}
									ECD_it++;
								}
							}
						}
					}
				}
				WrappingList_it++;

				// Don't increment the IndexSID for the ast partition
				if(WrappingList_it == WrappingList.end()) break;

				if(!FrameGroup) IndexSID++;
				i++;
			}
		}


		//
		// ** Write a footer (with updated durations) **
		//

		// If we are writing OP-Atom this is the first place we can claim it
		if(OPAtom) MData->SetOP(OPAtomUL);

		MData->SetTime();
		MPTimecodeComponent->SetDuration(Duration);

		i = 0;						//  Essence container index
		WrappingList_it = WrappingList.begin();
		while(WrappingList_it != WrappingList.end())
		{
			MPClip[i]->SetDuration(Duration);
			if((!OPAtom) || (i == OutFileNum))
			{
				if((i==0) || (!FrameGroup)) FPTimecodeComponent[i]->SetDuration(Duration);
				FPClip[i]->SetDuration(Duration);
				(*WrappingList_it)->EssenceDescriptor->SetInt64("ContainerDuration",Duration);
			}
				
			WrappingList_it++;
			i++;
		}

		// Update the generation UIDs in the metadata to reflect the changes
		MData->UpdateGenerations(Ident);

		// Turn the header or body partition into a footer
		ThisPartition->ChangeType("CompleteFooter");

		if(IndexChunk.Size)
		{
			// Work out how big the index (and associated filler) will be
			Uint64 IndexSize = IndexChunk.Size + Out->FillerSize(IndexChunk.Size, KAGSize);
			ThisPartition->SetUint("IndexSID",  IndexSID);
			ThisPartition->SetUint("IndexByteCount",  IndexSize);
		}
		else
		{
			ThisPartition->SetUint("IndexSID", 0);
			ThisPartition->SetUint("IndexByteCount", 0);
		}

		// Make sure any new sets are linked in
		ThisPartition->UpdateMetadata(MData);

		// Actually write the footer
		// Note: No metadata in OP-Atom footer
		if(OPAtom) Out->WritePartition(ThisPartition, false);
		else Out->WritePartition(ThisPartition);

		if(IndexChunk.Size)
		{
			if(KAGSize > 1) Out->Align(KAGSize);
			Out->Write(IndexChunk);
		}

		// Add a RIP
		Out->WriteRIP();

		//
		// ** Update the header ** 
		//
		// For generalized OPs update the value of "FooterPartition" in the header pack
		// For OP-Atom re-write the entire header
		//

		Uint64 FooterPos = ThisPartition->GetUint64("FooterPartition");
		Out->Seek(0);
		if(OPAtom)
		{
			ThisPartition->ChangeType("ClosedCompleteHeader");
			ThisPartition->SetUint64("FooterPartition", FooterPos);
			ThisPartition->SetUint("IndexSID", 0);			
			ThisPartition->SetUint64("IndexByteCount", 0);
			Out->ReWritePartition(ThisPartition);
		}
		else
		{
			ThisPartition = Out->ReadPartition();
			ThisPartition->SetUint64("FooterPartition", FooterPos);
			Out->Seek(0);
			Out->WritePartitionPack(ThisPartition);
		}

		// Close the file - all done!
		Out->Close();
	}

	return 0;
}


//! Parse the command line options
/*!	\return true if all parsed ok, false if an error or none supplied */
bool ParseCommandLine(int &argc, char **argv)
{
	int i;
	for(i=1; i<argc;)
	{
		if((argv[i][0] == '/') || (argv[i][0] == '-'))
		{
			char *p = &argv[i][1];					// The option less the '-' or '/'
			char Opt = tolower(*p);					// The option itself (in lower case)
			char *Val = "";							// Any value attached to the option
			if(strlen(p) > 2) Val = &p[2];			// Only set a value if one found

			if(Opt == 'a') OPAtom = true;
			else if(Opt == 'p')
			{
				// The value is further along as we are using a 2-byte option
				Val++;
				if(tolower(p[1]) == 'd')
				{
					char *temp;
					BodyMode = BodyWrapping::Body_Duration;
					BodyRate = strtoul(Val, &temp, 0);
				}
				else if(tolower(p[1]) == 's')
				{
					char *temp;
					BodyMode = BodyWrapping::Body_Size;
					BodyRate = strtoul(Val, &temp, 0);
				}
				else error("Unknown body partition mode '%c'\n", p[1]);
			}
			else if(Opt == 'e') EditAlign = true;
			else if(Opt == 'f') FrameGroup = true;
			else if(Opt == 's') StreamMode = true;
			else if(Opt == 'v') DebugMode = true;
			else if(Opt == 'i')
			{
				UseIndex = true;
				if(tolower(p[1]) == 'p')
				{
					SparseIndex = true;
				}
			}
			else if(Opt == 'h') 
			{
				char *temp;
				HeaderPadding = strtoul(Val, &temp, 0);
			}
			else if(Opt == 'k') 
			{
				char *temp;
				KAGSize = strtoul(Val, &temp, 0);
			}
			else if(Opt == 'r')
			{
				int N, D; // Use ints in case Int32 is not the same size as "int" on this platform
				if(sscanf(Val,"%d/%d", &N, &D) == 2)
				{
					ForceEditRate.Numerator = N;
					ForceEditRate.Denominator = D;
				}
				else
				{
					error("Invalid edit rate format \"%s\"\n", Val);
				}
			}
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

	if(argc < 3)
	{
		printf("Usage:    mxfwrap <inputfiles> <mxffile>\n\n");

		printf("Syntax for input files:\n");
		printf("         a,b = file a followed by file b\n");
		printf("         a+b = file a ganged with file b\n");
		printf("     a+b,c+d = file a ganged with file b\n");
		printf("               followed by file c ganged with file d\n\n");

		printf("Note: There must be the same number of ganged files in each sequential set\n");
		printf("      Also all files in each set must be the same duration\n\n");

		printf("Options:\n");
		printf("    -a         = Force OP-Atom\n");
		printf("    -e         = Only start body partitions at edit points\n");
		printf("    -f         = Frame-wrap and group in one container\n");
		printf("    -h=<size>  = Leave at lease <size> bytes of expansion space in the header\n");
		printf("    -i         = Write index tables\n");
		printf("    -ip        = Write sparse index tables with one entry per partition\n");
		printf("    -k=<size>  = Set KAG size (default=1)\n");
		printf("   -pd=<dur>   = Body partition every <dur> frames\n");
		printf("   -ps=<size>  = Body partition roughly every <size> bytes\n");
		printf("                 (early rather than late)\n");
		printf("    -r=<n>/<d> = Force edit rate (if possible)\n");
		printf("    -s         = Interleave essence containers for streaming\n");
		printf("    -v         = Verbose mode\n\n");

		return false;
	}

	InFileGangSize = 1;
	InFileGangCount = 1;

	strncpy(InFilenameSet, argv[1], 510);

	int InCount = 0;
	char *ps = InFilenameSet;
	for(;;)
	{
		char *LastDot = NULL;
		char *pd = InFilename[InCount];

		// Find the position of last dot in the input filename
		while(*ps)
		{
			if(*ps == '.') LastDot = ps;
			if(*ps == ',') { InFileGangCount++; break; }
			if(*ps == '+')
			{
				if(InFileGangCount == 1) InFileGangSize++;
				break;
			}
			*pd++ = *ps++;
		};
		*pd = '\0';
		InCount++;

//		// If input filename specified no extension add ".mpg"
//		if(LastDot == NULL)	strcpy(pd, ".mpg");

		// If all files progessed end scan
		if(*ps == '\0') break;

		// Otherwise we found ',' or '+' so skip it
		ps++;
	}

	strncpy(OutFilenameSet, argv[2], 510);
	OutFileCount = 0;
	ps = OutFilenameSet;
	for(;;)
	{
		char *LastDot = NULL;
		char *pd = OutFilename[OutFileCount];

		// Find the position of last dot in the input filename
		while(*ps)
		{
			if(*ps == '.') LastDot = ps;
			if(*ps == ',') { break; }
			if(*ps == '+') { break; }
			*pd++ = *ps++;
		};
		*pd = '\0';
		OutFileCount++;

		// If input filename specified no extension add ".mxf"
		if(LastDot == NULL)	strcpy(pd, ".mxf");

		// If all files progessed end scan
		if(*ps == '\0') break;

		// Otherwise we found ',' or '+' so skip it
		ps++;
	}


	// Detail the options

	debug("** Verbose Mode **\n\n");
	
	printf("KAGSize     = %u\n\n", KAGSize);
	
	if(InFileGangSize == 1)
	{
		if(InFileGangCount == 1) printf("Input file  = %s\n", InFilename[0]);
		else
		{
			printf("Input files = ");
			int i;
			for(i=0; i<InFileGangCount; i++) 
			{ 
				if(i != 0) printf(" then ");
				printf("%s", InFilename[i]);
			}
			printf("\n");
		}
	}
	else
	{
		printf("Input files = ");

		int i;
		for(i=0; i<InFileGangCount; i++) 
		{ 
			if(i != 0) printf(" followed by: ");
			int j;
			for(j=0; j<InFileGangSize; j++) 
			{ 
				if(j != 0) printf(" with ");
				printf("%s", InFilename[i*InFileGangSize + j]);
			}
			printf("\n");
		}
		if(InFileGangCount > 1) printf("\n");
	}

	if(OutFileCount == 1)
	{
		printf("Output file = %s\n\n", OutFilename[0]);
	}
	else
	{
		printf("Output files = ");
		int i;
		for(i=0; i<OutFileCount; i++) 
		{ 
			if(i != 0) printf(" with ");
			printf("%s", OutFilename[i]);
		}
		printf("\n");
	}

	if(OPAtom)
	{
		printf("Output OP = OP-Atom\n");
		
		// We will need some extra space in the header
		if(HeaderPadding == 0) HeaderPadding = 16384;

		OPUL = OPAtomUL;

		if((InFileGangCount * InFileGangSize) != OutFileCount) error("OP-Atom can only output a single essence container per file so requires as many output files as input files\n");
		
		if(BodyMode != BodyWrapping::Body_None) 
		{
			warning("Body partitions are forbidden in OP-Atom\n");
			BodyMode = BodyWrapping::Body_None;
		}

//		warning("OP-Atom not yet fully supported\n");
	}
	else
	{
		if(InFileGangSize == 1)
		{
			if(InFileGangCount == 1) { printf("Output OP = OP1a\n"); OPUL = OP1aUL; }
			else { printf("Output OP = OP2a\n"); OPUL = OP2aUL; }
		}
		else
		{
			if(InFileGangCount == 1) { printf("Output OP = OP1b\n"); OPUL = OP1bUL; }
			else { printf("Output OP = OP2b\n"); OPUL = OP2bUL; }
		}

		if((InFileGangCount * InFileGangSize) > 1) error("Only OP1a currently supported\n");
	}

	if(StreamMode && (InFileGangSize == 1))
	{
		warning("Essence containers will not be interleaved for streaming as none are ganged\n");
		StreamMode = false;
	}

	if(StreamMode) 
	{
		printf("Essence containers will be interleaved for streaming\n");

		if(BodyMode != BodyWrapping::Body_None) 
		{
			warning("Body partitions will be inserted for interleaving - this overrides other body partitioning options\n");
			BodyMode = BodyWrapping::Body_None;
		}

		error("Stream mode not yet supported\n");
	}
	else
	{
		if(BodyMode == BodyWrapping::Body_Duration)
		{
			if(EditAlign)
				printf("A new body partition will be inserted at the first new GOP after each %d frame%s\n", BodyRate, BodyRate==1 ? "" : "s");
			else
				printf("A new body partition will be inserted every %d frame%s\n", BodyRate, BodyRate==1 ? "" : "s");
		}

		if(BodyMode == BodyWrapping::Body_Size)
			printf("Partitions will be limited to %d byte%s (if possible)\n", BodyRate, BodyRate==1 ? "" : "s");
	}

	if(UseIndex)
	{
		if(SparseIndex) printf("Sparse index tables will be written for each frame wrapped essence container\n");
		else printf("Index tables will be written for each frame wrapped essence container\n");
	}

	// Check for stray parameters as a space in the wrong place can otherise cause us to overwrite input files!
	if(argc > 3)
	{
		printf("\nThere appear to be too many filenames on the command line\n");
		return false;
	}

	return true;
}


//! Write a set of essence containers
/*! IMPLEMENTATION NOTES:
 *		Wraping more than one stream in a single container is achieved by using the same BodySID (but they must be contiguous)
 *		The current BodySID is read for ThisPartition
 *		Headermetadata is currently not repeated
 *
 */
Int64 WriteBody(MXFFilePtr Out, BodyWrappingList WrappingList, PartitionPtr ThisPartition, Int64 Duration /*=0*/)
{
	Int64 Ret = 0;

	ThisPartition->ChangeType("ClosedCompleteBodyPartition");
	Uint32 CurrentBodySID = ThisPartition->GetUint("BodySID");

	// Enable index tables on frame wrapped essence
	if(UseIndex)
	{
		BodyWrappingList::iterator WrappingList_it = WrappingList.begin();
		int i=0;
		while(WrappingList_it != WrappingList.end())
		{
			if((*WrappingList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Frame)
			{
				(*WrappingList_it).Writer->EnableIndex(0);
				(*WrappingList_it).Writer->Index->EditRate.Numerator = (*WrappingList_it).Config->EditRate.Numerator;
				(*WrappingList_it).Writer->Index->EditRate.Denominator = (*WrappingList_it).Config->EditRate.Denominator;
				(*WrappingList_it).Writer->Index->IndexSID = i + 129;
				(*WrappingList_it).Writer->Index->BodySID = i + 1;

				if(SparseIndex) (*WrappingList_it).Config->WrapOpt->Handler->SetOption("SelectiveIndex", 1);
			}
			i++;
			WrappingList_it++;
		}
	}

	//! Partition size to allow maximum body partition size to be set
	//  Start by calculating where the current partition starts
	Uint64 PartitionSize = 0;
	if(BodyMode == BodyWrapping::Body_Size) PartitionSize = Out->Tell() - ThisPartition->GetUint64("ThisPartition");

	// Assume done until we find out there is some data to write (in case the list is empty)
	bool Done = true;
	int ThisEditUnit = 0;
	for(;;)
	{
//@printf("\nOuter:");
		int i = 0;						//  BodySID index
		BodyWrappingList::iterator WrappingList_it = WrappingList.begin();
		while(WrappingList_it != WrappingList.end())
		{
//@printf("Inner:");
			Int64 Dur;
			if((*WrappingList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Clip) Dur = Duration; else Dur = 1;

			if((*WrappingList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Clip)
			{
				// Force a single pass...
				Done = true;

				EssenceSource *Source = (*WrappingList_it).Config->WrapOpt->Handler->GetEssenceSource((*WrappingList_it).InFile, (*WrappingList_it).Config->Stream, 0, (*WrappingList_it).Writer->Index);

				// Ensure this clip is indexed in sparse mode
				if(UseIndex && SparseIndex)
				{
					(*WrappingList_it).Config->WrapOpt->Handler->SetOption("AddIndexEntry");
				}

				(*WrappingList_it).Writer->AddEssenceData((*WrappingList_it).EssenceID, Source);
			}
			else
			{
				DataChunkPtr Dat = (*WrappingList_it).Config->WrapOpt->Handler->Read((*WrappingList_it).InFile, (*WrappingList_it).Config->Stream, Dur, (*WrappingList_it).Writer->Index);

				if(Dat->Size == 0)
				{
					Done = true;
					break;
				}
				else Done = false;

				// Ensure first frame is indexed in sparse mode
				if(UseIndex && SparseIndex && (ThisEditUnit == 0))
				{
					(*WrappingList_it).Config->WrapOpt->Handler->SetOption("AddIndexEntry");
				}

				(*WrappingList_it).Writer->AddEssenceData((*WrappingList_it).EssenceID, Dat);
			}

			// Only allow starting a new partition by size or duration on first essence of a set
			if(i == 0)
			{
				if((*WrappingList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Frame)
				{
					if((*WrappingList_it).BodyMode == BodyWrapping::Body_Size)
					{
						Int64 NewPartitionSize = PartitionSize + (*WrappingList_it).Writer->CalcWriteSize();
//@	printf("%d,",PartitionSize);
						if((!EditAlign) || ((*WrappingList_it).Config->WrapOpt->Handler->SetOption("EditPoint")))
						{
							if(NewPartitionSize > (*WrappingList_it).BodyRate)
							{
								// Force a new partition pack by clearing CurrentBodySID
								CurrentBodySID = 0;
								
								if(SparseIndex) (*WrappingList_it).Config->WrapOpt->Handler->SetOption("AddIndexEntry");
//@	printf("##\n");
							}
						}
					}

					if((*WrappingList_it).BodyMode == BodyWrapping::Body_Duration)
					{
						if(Dur) PartitionSize += Dur;
						else PartitionSize++;			// DRAGONS: What should we do here?

//@	printf("%d,",PartitionSize);
						if((!EditAlign) || ((*WrappingList_it).Config->WrapOpt->Handler->SetOption("EditPoint")))
						{
							if(PartitionSize >= (*WrappingList_it).BodyRate)
							{
								// Force a new partition pack by clearing CurrentBodySID
								CurrentBodySID = 0;

								if(SparseIndex) (*WrappingList_it).Config->WrapOpt->Handler->SetOption("AddIndexEntry");
//@	printf("##\n");
							}
						}
					}
				}
			}

			// Start a new partition if required
			if(CurrentBodySID != (*WrappingList_it).BodySID)
			{
//@printf("Body Part at 0x%08x\n", (int)Out->Tell());
				CurrentBodySID = (*WrappingList_it).BodySID;
				ThisPartition->SetUint("BodySID", CurrentBodySID);
				ThisPartition->SetUint64("BodyOffset",(*WrappingList_it).Writer->GetStreamOffset());
				PartitionSize = 0;

				Int64 Pos = Out->Tell();
				Out->WritePartition(ThisPartition, false);

				// If partitioning by size take into account the partition pack size
				if(BodyMode == BodyWrapping::Body_Size) PartitionSize = Out->Tell() - Pos;
			}

			// Fix index table stream offsets
			if(UseIndex && (Dur == 1))
			{
printf("Fixing index: %d ", ThisEditUnit);
				IndexEntryPtr Entry = (*WrappingList_it).Writer->Index->IndexEntryByEssenceOrder(ThisEditUnit);
				if(Entry)
				{
printf("Found @ 0x%08x\n", (int)(*WrappingList_it).Writer->GetStreamOffset());
					Entry->StreamOffset = (*WrappingList_it).Writer->GetStreamOffset();
				}
else printf("not\n");
			}

//@printf("Frame at 0x%08x - ", (int)Out->Tell());
			Int64 Pos = Out->Tell();
			(*WrappingList_it).Writer->StartNewCP();
			if((*WrappingList_it).BodyMode == BodyWrapping::Body_Size) PartitionSize += (Out->Tell() - Pos);

//@printf("0x%08x\n", (int)Out->Tell());
			WrappingList_it++;
			i++;
		}

		ThisEditUnit++;

		if(Done) break;

		Ret++;
	}

	// Perform any global index table building work
	if(UseIndex)
	{
		BodyWrappingList::iterator WrappingList_it = WrappingList.begin();
		while(WrappingList_it != WrappingList.end())
		{
			if((*WrappingList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Frame)
			{
				IndexTablePtr Table = (*WrappingList_it).Writer->Index;

				if(Table) Table->CommitIndexEntries();
			}
			WrappingList_it++;
		}
	}

#if 0
	// Perform any global index table building work
	if(UseIndex)
	{
		BodyWrappingList::iterator WrappingList_it = WrappingList.begin();
		while(WrappingList_it != WrappingList.end())
		{
			if((*WrappingList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Frame)
			{
				IndexTablePtr Table = (*WrappingList_it).Writer->Index;

				if(Table)
				{
					EssenceSubParserBase::IndexEntryMapPtr IndexMap;
					IndexMap = (*WrappingList_it).Config->WrapOpt->Handler->BuildIndexTable((*WrappingList_it).InFile, (*WrappingList_it).Config->Stream);

					if(IndexMap)
					{
						EssenceSubParserBase::IndexEntryMap::iterator it = IndexMap->begin();
						Int64 Picture = 0;
						while(it != IndexMap->end())
						{
							Table->Correct(Picture, (*it).second.TemporalOffset, (*it).second.AnchorOffset, (*it).second.Flags);
							it++;
							Picture++;
						}
					}
				}
			}
			WrappingList_it++;
		}
	}
#endif // 0

/*
	WrappingList_it = WrappingList.begin();
	while(WrappingList_it != WrappingList.end())
	{
		if((*WrappingList_it).Config->WrapOpt->ThisWrapType == WrappingOption::Frame)
		{
			int i;
			for(i=0; i<4; i++)
			{
				IndexPosPtr Pos = (*WrappingList_it).Writer->Index->Lookup(i);
				printf("EditUnit %d is at 0x%08x\n", (int)Pos->ThisPos, (int)Pos->Location);
			}
		}
		WrappingList_it++;
	}
*/

	// DRAGONS - this needs work!! It doesn't work for clip wrap!
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


