/*! \file	mxfcrypt.cpp
 *	\brief	MXF en/decrypt utility for MXFLib
 *
 *	\version $Id: mxfcrypt.cpp,v 1.18 2011/01/10 10:42:08 matt-beard Exp $
 *
 */
/*
 *  Copyright (c) 2004, Matt Beard
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

#include "mxflib/mxflib.h"

using namespace mxflib;

#include <stdio.h>
#include <stdlib.h>


// Include the AS-DCP crypto header
#include "crypto_asdcp.h"

#include "mxflib/dict.h"


using namespace std;


// Product GUID and version text for this release
UInt8 ProductGUID_Data[16] = { 0x84, 0x62, 0x40, 0xf1, 0x47, 0xed, 0xde, 0x40, 0x86, 0xdc, 0xe0, 0x99, 0xda, 0x7f, 0xd0, 0x53 };
string CompanyName = "freeMXF.org";
string ProductName = "mxfcrypt file de/encrypt utility";
string ProductVersion = "Based on " + LibraryVersion();
string PlatformName = "MXFLib (" + OSName() + ")";

//! Plaintext offset to use when encrypting
int PlaintextOffset = 0;

//! Name of keyfile or directoy to search for keyfiles with autogenerated names
std::string KeyFileName;


//! Process a set of header metadata
bool ProcessMetadata(bool DecryptMode, MetadataPtr HMeta, BodyReaderPtr BodyParser, GCWriterPtr Writer, bool LoadInfo = false);

//! Process the metadata for a given package on an encryption pass
bool ProcessPackageForEncrypt(BodyReaderPtr BodyParser, GCWriterPtr Writer, UInt32 BodySID, PackagePtr ThisPackage, bool LoadInfo = false);

//! Process the metadata for a given package on a decryption pass
bool ProcessPackageForDecrypt(BodyReaderPtr BodyParser, GCWriterPtr Writer, UInt32 BodySID, PackagePtr ThisPackage, bool LoadInfo = false);



//! MXFLib debug flag
bool DebugMode = false;

//! Flag set when we are updating the header in the output file to be closed if it is open in the source file (default)
bool ClosingHeader = true;

//! Flag for decrypt rather than encrypt
bool DecryptMode = false;

//! Flag for preserving the index table (non complient!)
bool PreserveIndex = false;

//! Flag for preserving the essence containers labels batch
bool PreserveECBatch = false;

//! Flag for preserving the essence containers label in the descriptor
bool PreserveECLabel = true;

//! The original IndexSID
UInt32 IndexSID;

//! Index table to update
IndexTablePtr Index;

//! Original index data (if preserving the index unchanged)
DataChunkPtr OriginalIndexData;


#include <time.h>

int main(int argc, char *argv[])
{
	printf("MXF en/decrypt utility\n");

	int num_options = 0;
	for(int i=1; i<argc; i++)
	{
		if(argv[i][0] == '-')
		{
			num_options++;
			if((argv[i][1] == 'v') || (argv[i][1] == 'V'))
				DebugMode = true;
			else if((argv[i][1] == 'd') || (argv[i][1] == 'D'))
				DecryptMode = true;
			else if((argv[i][1] == 'f') || (argv[i][1] == 'F'))
				ForceKeyMode = true;
			else if((argv[i][1] == 'h') || (argv[i][1] == 'H'))
				Hashing = true;
			else if((argv[i][1] == 'k') || (argv[i][1] == 'K'))
			{
				if((argv[i][2] != '=') && (argv[i][2] != ':'))
				{
					error("-k option syntax = -k=<key-file or directory>\n");
					return 1;
				}
				KeyFileName = std::string(&argv[i][3]);
			}
			else if((argv[i][1] == 'l') || (argv[i][1] == 'L'))
			{
				if(argv[i][2] == '+') PreserveECLabel = false;
				else if(argv[i][2] == '-') PreserveECBatch = true;
			}
			else if((argv[i][1] == 'i') || (argv[i][1] == 'I'))
			{
				if((argv[i][2] == 'p') && (argv[i][2] != 'P'))
				{
					PreserveIndex = true;
					printf("Preserving index table from the input file (non-complient behaviour)\n");
				}
			}
			else if((argv[i][1] == 'p') || (argv[i][1] == 'P'))
			{
				if((argv[i][2] != '=') && (argv[i][2] != ':'))
				{
					error("-p option syntax = -p=<plaintextbytes>\n");
					return 1;
				}
				PlaintextOffset = atoi(&argv[i][3]);
				printf("\nPlaintext Offset = %d\n", PlaintextOffset);
			}
		}
	}

	// Load the dictionaries
	LoadDictionary(DictData);

	if (argc - num_options < 3)
	{
		printf("\nUsage:  %s [options] <in-filename> <out-filename>\n\n", argv[0] );

		printf("Options:\n");
		printf("  -d         Decrypt (rather than encrypt)\n");
		printf("  -h         Perform HMAC hashing\n");
		printf("  -k=keyfile Use the specified key file\n");
		printf("  -p=offset  Leave plaintext bytes at the start\n");
		printf("  -ip        Preserve the existing index table values\n");
		printf("  -l-        Don't update the EssenceContainers batch\n");
		printf("  -l+        Do update the EssenceContainer value in the descriptor\n");
		printf("\n");

		return 1;
	}


	MXFFilePtr InFile = new MXFFile;
	if(!InFile->Open(argv[num_options+1], true))
	{
		error("Can't open input file\n");
		return 1;
	}

	// Open the output file
	MXFFilePtr OutFile = new MXFFile;
	if(!OutFile->OpenNew(argv[num_options+2]))
	{
		error("Can't open output file\n");
		return 1;
	}

	/* Generate a key-file if not given and we are encrypting */
	if(!DecryptMode)
	{
		if(KeyFileName.empty() || (KeyFileName[KeyFileName.length()-1] == DIR_SEPARATOR))
		{
			char NameBuffer[45];

			unsigned char Key[16];
			
			// TODO: Add decent random number generator here... this one is from the system.h UUID gen
			// DRAGONS: Strange double-casting is to remove pointer conversion warning in 64-bit systems
			srand(static_cast<unsigned int>(time(NULL)) ^ static_cast<unsigned int>(reinterpret_cast<UInt64>(&(*(OutFile)))) ^ (clock() << 2) ^ rand());

			int i;
			for(i=0; i<16; i++) Key[i] = rand() % 256;

			UUIDPtr FileNameData = new mxflib::UUID();
			const UInt8 *p = FileNameData->GetValue();
			sprintf(NameBuffer, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
							p[0], p[1], p[2], p[3],     p[4], p[5],      p[6], p[7],     p[8], p[9],
							p[10], p[11], p[12], p[13], p[14], p[15] );

			if(KeyFileName.empty())
				KeyFileName = NameBuffer;
			else
				KeyFileName += NameBuffer;

			FileHandle KeyFile = FileOpenNew(KeyFileName.c_str());
			if(!FileValid(KeyFile))
			{
				error("Failed to create key-file \"%s\"\n", KeyFileName.c_str());
				return 1;
			}

			sprintf(NameBuffer, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
							Key[0], Key[1], Key[2], Key[3], Key[4], Key[5], Key[6], Key[7],
							Key[8], Key[9],	Key[10], Key[11], Key[12], Key[13], Key[14], Key[15] );

			FileWrite(KeyFile, (UInt8*)NameBuffer, 32);
			FileClose(KeyFile);

			printf("Generated key-file \"%s\"\n", KeyFileName.c_str());
		}
	}

	/* Locate an index table to update (Requires seeking!) */

	InFile->GetRIP();
	if(InFile->FileRIP.empty())
	{
		warning("Unable to get a RIP for the input file - so not able to locate an index table\n");
	}
	else
	{
		RIP::iterator it = InFile->FileRIP.end();
		while(it != InFile->FileRIP.begin())
		{
			it--;
			InFile->Seek((*it).second->ByteOffset);
			PartitionPtr ThisPartition = InFile->ReadPartition();

			// Read the first index table we find (scanning backwards)
			if(ThisPartition->GetInt64(IndexByteCount_UL) != 0)
			{
				IndexSID= ThisPartition->GetUInt(IndexSID_UL);

				if(PreserveIndex)
				{
					OriginalIndexData = ThisPartition->ReadIndexChunk();
				}
				else
				{
					Index = new IndexTable;
					ThisPartition->ReadIndex(Index);
				}
				break;
			}
		}
	}

	// Read the master partition pack
	PartitionPtr MasterPartition = InFile->ReadMasterPartition();

	if(!MasterPartition)
	{
		InFile->Seek(0);
		MasterPartition = InFile->ReadPartition();

		if(!MasterPartition)
		{
			error("Could not read the Header!\n");
			return 1;
		}

		warning("Could not locate a closed partition containing header metadata - attempting to process using open header\n");
	}

	// Read the metadata from the header
	MasterPartition->ReadMetadata();

	MetadataPtr HMeta = MasterPartition->ParseMetadata();

	if(!HMeta)
	{
		error("Could not load the Header Metadata!\n");
		return 1;
	}

	// Set up a body readyer for the source file
	BodyReaderPtr BodyParser = new BodyReader(InFile);

	// And a writer for the destination file
	// Note that we use a GCWriter rather than a BodyWriter as this allows us to match the
	// layout of the original file body without complications
	GCWriterPtr Writer = new GCWriter(OutFile);

	// Update the header metadata as required - quit if that process failed
	if(!ProcessMetadata(DecryptMode, HMeta, BodyParser, Writer, true)) return 1;

	/* Write the header partition with updated closed metadata if required */

	if(ClosingHeader)
	{
		// If the master partition is not from the header then change it to be a header
		if(MasterPartition->GetUInt64(ThisPartition_UL) > 0)
		{
			if(MasterPartition->IsClosed())
			{
				if(MasterPartition->IsComplete()) 
					MasterPartition->ChangeType(ClosedCompleteHeader_UL);
				else
					MasterPartition->ChangeType(ClosedHeader_UL);
			}
			else
			{
				if(MasterPartition->IsComplete()) 
					MasterPartition->ChangeType(OpenCompleteHeader_UL);
				else
					MasterPartition->ChangeType(OpenHeader_UL);
			}

			// Read the old header partition
			InFile->Seek(0);
			PartitionPtr OldHeader = InFile->ReadPartition();

			// Set the header to have the same KAG and BodySID as before
			MasterPartition->SetKAG(OldHeader->GetUInt(KAGSize_UL));
			MasterPartition->SetUInt("BodySID", OldHeader->GetUInt(BodySID_UL));
		}

		// We don't yet know where the footer is...
		MasterPartition->SetUInt64(FooterPartition_UL, 0);

		// Write the new header
		OutFile->WritePartition(MasterPartition);
	}

	// Process the file...

	bool WriteMetadataInFooter = false;

	// Start at the beginning of the file
	InFile->Seek(0);

	// Loop until all is done...
	for(;;)
	{
		if(!BodyParser->IsAtPartition())
		{
			BodyParser->ReSync();
		}

		// Move the main file pointer to the current body partition pack
		Position CurrentPos = BodyParser->Tell();
		InFile->Seek(CurrentPos);

		// Read the partition pack
		PartitionPtr CurrentPartition = InFile->ReadPartition();
		if(!CurrentPartition) break;

		/* Update the partition pack ?? */
		
		// Work out if we should do anything with this partition at all
		bool UpdatePartition = true;
		
		// Don't update the header if we have just written an updated closed version 
		if((CurrentPos==0) && (ClosingHeader)) UpdatePartition = false;
		else
		{
			// Don't update the footer (if it has metadata) - we will write that later
			if(CurrentPartition->IsA(CompleteFooter_UL) || CurrentPartition->IsA(Footer_UL)) 
			{
				if(CurrentPartition->GetInt64(HeaderByteCount_UL) != 0) 
				{
					WriteMetadataInFooter = true;
					UpdatePartition = false;
				}
			}
		}

		if(UpdatePartition)
		{
			// TODO: We should probably insert updated metadata here if the input file has it
			CurrentPartition->SetUInt64(FooterPartition_UL, 0);
			
			// Update essence containers
			MDObjectPtr DstECBatch = CurrentPartition->AddChild(EssenceContainers_UL, true);
			if(DstECBatch)
			{
				DstECBatch->clear();
				MDObjectPtr SrcECBatch = HMeta[EssenceContainers_UL];

				if(SrcECBatch)
				{
					MDObjectULList::iterator it = SrcECBatch->begin();
					while(it != SrcECBatch->end())
					{
						DstECBatch->AddChild()->SetValue((*it).second->Value->PutData());
						it++;
					}
				}
			}

			OutFile->WritePartition(CurrentPartition, false);
		}

		// Ensure we match the KAG
		Writer->SetKAG(CurrentPartition->GetUInt(KAGSize_UL));

		// Parse the file until next partition or an error
		if (!BodyParser->ReadFromFile()) break;
	}

	// Write the footer partition

	if(WriteMetadataInFooter)
	{
	if(MasterPartition->IsComplete()) 
		MasterPartition->ChangeType(CompleteFooter_UL);
	else
		MasterPartition->ChangeType(Footer_UL);

	// Ensure we maintain the same KAG as the previous footer
	MasterPartition->SetKAG(Writer->GetKAG());

	if(PreserveIndex)
	{
		MasterPartition->SetUInt(IndexSID_UL, IndexSID);
		OutFile->WritePartitionWithIndex(MasterPartition, OriginalIndexData);
	}
	else if(Index)
	{
		MasterPartition->SetUInt(IndexSID_UL, IndexSID);
		DataChunkPtr IndexData = new DataChunk;
		Index->WriteIndex(*IndexData);
		OutFile->WritePartitionWithIndex(MasterPartition, IndexData);
	}
	else
		OutFile->WritePartition(MasterPartition);
	}

	// Add a RIP
	OutFile->WriteRIP();

	InFile->Close();

	OutFile->Close();

	printf("Done\n");

	return 0;
	
	// TODO: WE NEED TO HAVE ONE WRITER PER BODY-SID!!
}


//! Process a set of header metadata
/*!	If encrypting a crypto context is added in each internal file package, otherwise crypto tracks are removed
 *	\return true if all OK, else false
 */
bool ProcessMetadata(bool DecryptMode, MetadataPtr HMeta, BodyReaderPtr BodyParser, GCWriterPtr Writer, bool LoadInfo /*=false*/)
{
	// Locate the Content Storage set
	MDObjectPtr ContentStorage = HMeta[ContentStorageObject_UL];
	if(ContentStorage) ContentStorage = ContentStorage->GetLink();

	if(!ContentStorage)
	{
		error("Header Metadata does not contain a ContentStorage set!\n");
		return false;
	}

	// And locate the Essence Container Data batch in the Content Storage set
	MDObjectPtr EssenceContainerData = ContentStorage[EssenceDataObjects_UL];

	if(!EssenceContainerData)
	{
		error("ContentStorage set does not contain an EssenceContainerData property!\n");
		return false;
	}

	// A map of PackageIDs of all contained essence, indexed by BodySID
	typedef std::map<UInt32, DataChunkPtr> DataChunkMap;
	DataChunkMap FilePackageMap;

	// Scan the essence containers
	MDObject::iterator it = EssenceContainerData->begin();
	while(it != EssenceContainerData->end())
	{
		MDObjectPtr ECDSet = (*it).second->GetLink();

		if(ECDSet)
		{
			// Add the package ID to the BodySID map
			UInt32 BodySID = ECDSet->GetUInt(BodySID_UL);
			MDObjectPtr PackageID = ECDSet[LinkedPackageUID_UL];
			if(PackageID) 
			{
				DataChunkPtr PackageIDData = PackageID->PutData();
				FilePackageMap.insert(DataChunkMap::value_type(BodySID, PackageIDData ));
			}
		}

		it++;
	}


	/* Add cryptographic context sets (one per internal file package) */

	// Count of number of packages being en/decrypted
	int CryptoCount = 0;

	PackageList::iterator Package_it = HMeta->Packages.begin();
	while(Package_it != HMeta->Packages.end())
	{
		// Locate the package ID
		MDObjectPtr ThisIDObject = (*Package_it)[PackageUID_UL];
		if(ThisIDObject)
		{
			// Build a datachunk of the UMID to compare
			DataChunkPtr PackageID = ThisIDObject->PutData();

			// Look for a matching BodySID (to see if this is an internal file package)
			DataChunkMap::iterator PackageMap_it = FilePackageMap.begin();
			while(PackageMap_it != FilePackageMap.end())
			{
				// If the package IDs match we are encrypting this package
				if(*((*PackageMap_it).second) == *PackageID)
				{
					bool Result;
					if(DecryptMode) Result = ProcessPackageForDecrypt(BodyParser, Writer, (*PackageMap_it).first, (*Package_it), LoadInfo);
					else Result = ProcessPackageForEncrypt(BodyParser, Writer, (*PackageMap_it).first, (*Package_it), LoadInfo);

					// Exit on error (ignore if we are forcing a key)
					if((!Result) && (!ForceKeyMode)) return false;

					CryptoCount++;
				}

				PackageMap_it++;
			}
		}

		Package_it++;
	}
	

	// Are we actually en/decrypting anything?
	if(CryptoCount == 0)
	{
		if(DecryptMode)
			error("Didn't find a file package for any encrypted essence to decrypt!\n");
		else
			error("Didn't find a file package for any essence to encrypt!\n");

		return false;
	}


	/* Update DMSchemes as required */

	// Locate the DMSchemes batch
	MDObjectPtr DMSchemes = HMeta[DMSchemes_UL];

	if(!DMSchemes)
	{
		error("Header Metadata does not contain a DMSchemes!\n");
		
		// Try and add one
		DMSchemes = HMeta->AddChild(DMSchemes_UL);
		
		// If that fails give up!
		if(!DMSchemes) return false;
	}

	if(DecryptMode)
	{
		bool Found = false;
		MDObject::iterator it = DMSchemes->begin();
		while(it != DMSchemes->end())
		{
			DataChunkPtr ThisLabel = (*it).second->PutData();
			ULPtr ThisUL = new UL(ThisLabel->Data);
			if(*ThisUL == CryptographicFrameworkLabel_UL)
			{
				DMSchemes->RemoveChild((*it).second);
				Found = true;
				break;
			}
			it++;
		}
		if(!Found)
		{
			error("Source file does not have a CryptographicFrameworkLabel in the DMSchemes list - is it really an AS-DCP encrypted file?\n");
		}
	}
	else
	{
		bool Found = false;
		MDObject::iterator it = DMSchemes->begin();
		while(it != DMSchemes->end())
		{
			DataChunkPtr ThisLabel = (*it).second->PutData();
			ULPtr ThisUL = new UL(ThisLabel->Data);
			if(*ThisUL == CryptographicFrameworkLabel_UL)
			{
				Found = true;
				break;
			}
			it++;
		}
		if(Found)
		{
			error("Source file already contains a CryptographicFrameworkLabel in the DMSchemes list - is it already encrypted?\n");
		}
		else
		{
			// Add the crypto scheme
			MDObjectPtr Ptr = DMSchemes->AddChild();
			if(Ptr) Ptr->SetValue(CryptographicFrameworkLabel_UL.GetValue(), 16);
		}
	}

	/* Update the EssenceContainers Batch */
	if(!PreserveECBatch)
	{
		MDObjectPtr ECBatch = HMeta[EssenceContainers_UL];
		if(!ECBatch) ECBatch = HMeta->AddChild(EssenceContainers_UL);

		// Clear the current list
		ECBatch->clear();

		if(!DecryptMode)
		{
			// In encrypting mode we are left with the encrypted container only
			HMeta->AddEssenceType(EncryptedContainerLabel_UL);
		}
		else
		{
			// Search through all packages
			PackageList::iterator Package_it = HMeta->Packages.begin();
			while(Package_it != HMeta->Packages.end())
			{
				// Locate the package ID
				MDObjectPtr ThisIDObject = (*Package_it)[PackageUID_UL];
				if(ThisIDObject)
				{
					// Build a datachunk of the UMID to compare
					DataChunkPtr PackageID = ThisIDObject->PutData();

					// Look for a matching BodySID (to see if this is an internal file package)
					DataChunkMap::iterator PackageMap_it = FilePackageMap.begin();
					while(PackageMap_it != FilePackageMap.end())
					{
						// If the package IDs match we will have encrypted this package
						if(*((*PackageMap_it).second) == *PackageID)
						{
							// Locate the descriptor for this package
							MDObjectPtr Ptr = (*Package_it)[Descriptor_UL];
							if(Ptr) Ptr = Ptr->GetLink();
							if(Ptr)
							{
								// If this is a multiple descriptor we need to scan the sub-descriptors
								if(Ptr->IsA(MultipleDescriptor_UL))
								{
									// Ensure that we have flagged a multiple descriptor if one is used
									ULPtr GCUL = new UL( mxflib::GCMulti_Data );
									HMeta->AddEssenceType( GCUL );

									MDObjectPtr SubPtr = Ptr[FileDescriptors_UL];
									if(SubPtr)
									{
										MDObject::iterator subit = SubPtr->begin();
										while(subit != SubPtr->end())
										{
											MDObjectPtr ECLabel = (*subit).second->GetLink();
											if(ECLabel) ECLabel = ECLabel[EssenceContainer_UL];
											if(ECLabel)
											{
												DataChunkPtr LabelData = ECLabel->PutData();
												ULPtr LabelUL = new UL(LabelData->Data);
												HMeta->AddEssenceType(LabelUL);
											}

											subit++;
										}
									}
								}
								else
								{
									MDObjectPtr ECLabel = Ptr[EssenceContainer_UL];
									if(ECLabel)
									{
										DataChunkPtr LabelData = ECLabel->PutData();
										ULPtr LabelUL = new UL(LabelData->Data);
										HMeta->AddEssenceType(LabelUL);
									}
								}

							}
						}
						PackageMap_it++;
					}
				}
				Package_it++;
			}
		}
	}

	// Build an Ident set describing us and link into the metadata
	MDObjectPtr Ident = new MDObject(Identification_UL);
	Ident->SetString(CompanyName_UL, CompanyName);
	Ident->SetString(ProductName_UL, ProductName);
	Ident->SetString(VersionString_UL, ProductVersion);
	Ident->SetString(ToolkitVersion_UL, LibraryProductVersion());
	Ident->SetString(Platform_UL, PlatformName);
	UUIDPtr ProductUID = new mxflib::UUID(ProductGUID_Data);

	// DRAGONS: -- Need to set a proper GUID per released version
	//             Non-released versions currently use a random GUID
	//			   as they are not a stable version...
	Ident->SetValue(ProductUID_UL, DataChunk(16,ProductUID->GetValue()));

	// Link the new Ident set with all new metadata
	// Note that this is done even for OP-Atom as the 'dummy' header written first
	// could have been read by another device. This flags that items have changed.
	HMeta->UpdateGenerations(Ident);

	return true;
}
	
//! Process the metadata for a given package on an encryption pass
/*! \return true if all OK, else false
 */
bool ProcessPackageForEncrypt(BodyReaderPtr BodyParser, GCWriterPtr Writer, UInt32 BodySID, PackagePtr ThisPackage, bool LoadInfo /*=false*/)
{
	MDObjectPtr Descriptor = ThisPackage[Descriptor_UL];
	if(Descriptor) Descriptor = Descriptor->GetLink();

	if(!Descriptor)
	{
		error("Source file contains a File Package without a File Descriptor\n");
		return false;
	}
	
	MDObjectPtr ContainerUL = Descriptor[EssenceContainer_UL];
	if(!ContainerUL)
	{
		error("Source file contains a File Descriptor without an EssenceContainer label\n");
		return false;
	}

	// Record the original essence UL
	DataChunkPtr EssenceUL = ContainerUL->PutData();

	if(!PreserveECLabel)
	{
		// Change the essence UL in the descriptor to claim to be encrypted
		const UInt8 EncryptedEssenceUL[] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x07, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0b, 0x01, 0x00 };
		ContainerUL->SetValue(EncryptedEssenceUL, 16);
	}

	// Add a crypto track
	TrackPtr CryptoDMTrack = ThisPackage->AddDMTrack("Cryptographic DM Track");

	// Add metadata to the track
	DMSegmentPtr CryptoDMSegment = CryptoDMTrack->AddDMSegment();

	// Build the cryptographic framework
	MDObjectPtr CryptoFramework = new MDObject(CryptographicFramework_UL);

	// This is the first chance to sanity check the crypto dictionary
	if(!CryptoFramework)
	{
		// DRAGONS: These error messages should be folded by the compiler as they are identical
		error("Failed to build cryptographic metadata - has the correct dictionary been loaded?\n");
		return false;
	}

	// Link the framework to this track
	CryptoDMSegment->MakeLink(CryptoFramework);

	// Build the cryptographic context
	MDObjectPtr CryptoContext = new MDObject(CryptographicContext_UL);
	if(!CryptoContext)
	{
		// DRAGONS: These error messages should be folded by the compiler as they are identical
		error("Failed to build cryptographic metadata - has the correct dictionary been loaded?\n");
		return false;
	}

	// Build the context ID link
	MDObjectPtr ContextSR = CryptoFramework->AddChild(ContextSR_UL);
	if(!ContextSR)
	{
		// DRAGONS: These error messages should be folded by the compiler as they are identical
		error("Failed to build cryptographic metadata - has the correct dictionary been loaded?\n");
		return false;
	}

	// Link us to the framework
	ContextSR->MakeRef(CryptoContext);
	
	// Build a new UUID for the Crypto Context ID
	UUIDPtr ContextID = new mxflib::UUID;
	
	// Set the context ID
	MDObjectPtr Ptr = CryptoContext->AddChild(ContextID_UL);
	if(Ptr) Ptr->SetValue(ContextID->GetValue(), 16);

	// Set the original essence UL
	Ptr = CryptoContext->AddChild(SourceEssenceContainer_UL);
	if(Ptr) Ptr->SetValue(EssenceUL);

	// Set the encryption algorithm
	const UInt8 CypherLabel[] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x07, 0x02, 0x09, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00 };
	Ptr = CryptoContext->AddChild(CipherAlgorithm_UL);
	if(Ptr) Ptr->SetValue(CypherLabel, 16);

	// Specify no MIC
	const UInt8 MICLabel_NULL[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	const UInt8 MICLabel_HMAC_SHA1[] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x07, 0x02, 0x09, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00 };
	Ptr = CryptoContext->AddChild(MICAlgorithm_UL);
	if(Ptr) 
	{
		if(Hashing)
			Ptr->SetValue(MICLabel_HMAC_SHA1, 16);
		else
			Ptr->SetValue(MICLabel_NULL, 16);
	}


	// Use the specified key
	char *NameBuffer = new char[KeyFileName.size() + 1];
	strcpy(NameBuffer, KeyFileName.c_str());
	
	// Scan back for the last directory seperator to find the filename
	char *NamePtr = &NameBuffer[strlen(NameBuffer)];
	while(NamePtr > NameBuffer)
	{
		if((*NamePtr == '/') || (*NamePtr == DIR_SEPARATOR))
		{
			NamePtr++;
			break;
		}
		NamePtr--;
	}

	// DRAGONS: Build in an int array for type-safety
	int KeyBuff[16];
	int Count = sscanf(NamePtr, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
					   &KeyBuff[0], &KeyBuff[1], &KeyBuff[2], &KeyBuff[3], &KeyBuff[4], &KeyBuff[5], &KeyBuff[6], &KeyBuff[7], 
					   &KeyBuff[8], &KeyBuff[9], &KeyBuff[10], &KeyBuff[11], &KeyBuff[12], &KeyBuff[13], &KeyBuff[14], &KeyBuff[15] );

	delete[] NameBuffer;

	if(Count != 16)
	{
		error("Key filename is not in the correct hex format of: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\n");
		return false;
	}

	// Copy the data into a byte buffer
	UInt8 KeyBuffU8[16];
	{
		int i;
		for(i=0; i<16; i++) KeyBuffU8[i] = (UInt8)KeyBuff[i];
	}

	Ptr = CryptoContext->AddChild(CryptographicKeyID_UL);
	if(Ptr) Ptr->SetValue(KeyBuffU8, 16);

	/* Now set up the crypto handlers */

	// If we haven't already set up this BodySID, do it now
	if(LoadInfo && (!BodyParser->GetGCReader(BodySID)))
	{
		DataChunkPtr KeyID = new DataChunk(16, KeyBuffU8);
		Encrypt_GCReadHandler *pHandler = new Encrypt_GCReadHandler(Writer, BodySID, ContextID, KeyID, KeyFileName);
		pHandler->SetPlaintextOffset(PlaintextOffset);
		if(Index) pHandler->SetIndex(Index);
		GCReadHandlerPtr Handler = pHandler;
		GCReadHandlerPtr FillerHandler = new Basic_GCFillerHandler(Writer, BodySID);
		BodyParser->MakeGCReader(BodySID, Handler, FillerHandler);
	}

	return true;
}



//! Process the metadata for a given package on a decryption pass
/*! \return true if all OK, else false
 */
bool ProcessPackageForDecrypt(BodyReaderPtr BodyParser, GCWriterPtr Writer, UInt32 BodySID, PackagePtr ThisPackage, bool LoadInfo /*=false*/)
{
	// Decryption Key
	DataChunkPtr Key;

	// Original Essence Key
	DataChunkPtr OriginalEssenceUL;

	// Search for the crypto context
	TrackList::iterator it = ThisPackage->Tracks.begin();
	while(it!= ThisPackage->Tracks.end())
	{
//printf("Track: %s\n", (*it)->GetString(TrackName_UL).c_str());
		ComponentList::iterator comp_it = (*it)->Components.begin();
		while(comp_it!= (*it)->Components.end())
		{
//printf("  Comp: %s\n", (*comp_it)->FullName().c_str());
			// Found a DM segment?
			if((*comp_it)->IsA(DMSegment_UL))
			{
				MDObjectPtr Framework = (*comp_it)->Child(DMFramework_UL);
				if(Framework) Framework = Framework->GetLink();

				// Found a Crypto Framework on the segment?
				if(Framework && Framework->IsA(CryptographicFramework_UL))
				{
					MDObjectPtr Context = Framework->Child(ContextSR_UL);
					if(Context) Context = Context->GetLink();

					if(Context)
					{
						// Read the key ID
						Key = Context[CryptographicKeyID_UL]->PutData();

						// Read the original essence UL
						OriginalEssenceUL = Context[SourceEssenceContainer_UL]->PutData();

						// Remove the crypto track
						ThisPackage->RemoveTrack(*it);

						break;
					}
				}
			}
			comp_it++;
		}

		// Stop looking once we find the key
		if(Key) break;

		it++;
	}


	/* Replace the original Essence UL */

	MDObjectPtr Descriptor = ThisPackage[Descriptor_UL];
	if(Descriptor) Descriptor = Descriptor->GetLink();

	if(!Descriptor)
	{
		error("Source file contains a File Package without a File Descriptor\n");
		return false;
	}
	
	MDObjectPtr ContainerUL = Descriptor[EssenceContainer_UL];
	if(!ContainerUL)
	{
		error("Source file contains a File Descriptor without an EssenceContainer label\n");
		return false;
	}

	if(!PreserveECLabel)
	{
		// Change the essence UL in the descriptor back to the original version
		ContainerUL->SetValue(OriginalEssenceUL);
	}

	// Don't validate or set up crypto if not loading data
	if(!LoadInfo) return true;

//## warning("Not checking if this package is actually encrypted or not!!\n");

	if(!Key)
	{
		error("Coundn't find CryptographicKeyID in the encrypted file\n");
		if(!ForceKeyMode) return false;
	}

	Decrypt_GCReadHandler *pHandler = new Decrypt_GCReadHandler(Writer, BodySID);
	if(Index) pHandler->SetIndex(Index);
	GCReadHandlerPtr Handler = pHandler;
	GCReadHandlerPtr FillerHandler = new Basic_GCFillerHandler(Writer, BodySID);
	GCReadHandlerPtr EncHandler = new Decrypt_GCEncryptionHandler(BodySID, Key, KeyFileName);

	Decrypt_GCEncryptionHandler *Test = SmartPtr_Cast(EncHandler, Decrypt_GCEncryptionHandler);
	if(!Test->KeyValid()) return false;

	BodyParser->MakeGCReader(BodySID, Handler, FillerHandler);
	GCReaderPtr Reader = BodyParser->GetGCReader(BodySID);
	if(Reader) Reader->SetEncryptionHandler(EncHandler);

	return true;
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

