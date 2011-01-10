/*! \file	process.h
 *	\brief	MXF wrapping functions
 *
 *	\version $Id: process.cpp,v 1.2 2011/01/10 10:59:12 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2010, Metaglue Corporation
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

#include <stdio.h>
#include <iostream>
#include <string>
#include <sys/types.h> 
#include <sys/stat.h>

//defines to make stat cross platform
#ifdef _WIN32
#define STAT _stat
#define S_IFDIR _S_IFDIR
#else
#define STAT stat
#endif

using namespace std;


#include "mxflib/mxflib.h"
using namespace mxflib;


#ifndef _WIN32
//headers for high resolution timer
#include <sys/time.h>

#endif

#ifdef _WIN32

#else
#include <pthread.h>
#endif

#include "libprocesswrap/process.h"


#include "productIDs.h"


struct _EssenceStreamInfo
{
	GCStreamID EssenceID;						// Essence stream ID for each essence stream
	TimecodeComponentPtr FPTimecodeComponent;	// Timecode component for each file package
	TrackPtr MPTrack;							// Material Package track for each essence stream
	TrackPtr SPEdgecodeTrack;					// Material Package track for edgecode if DPX name is used
	TrackPtr FPTrack;							// File Package track for each essence stream
	TrackPtr TSPTrack;							// Physical Source Package track for each essence stream
	TrackPtr FSPTrack;							// Physical Source Package track for each essence stream
	TrackPtr SoundRollTrack;					// Track in Avid special Sound source package
	SourceClipPtr MPClip;						// Material Package SourceClip for each essence stream 
	ComponentPtr FPClip;						// File Package SourceClip for each essence stream 
	ComponentPtr TSPClip;						// Physical Source Package SourceClip for each essence stream 
	ComponentPtr FSPClip;						// Physical Source Package SourceClip for each essence stream 
	BodyStreamPtr Stream;						// BodyStream for each stream being built (master stream if frame-grouping)

};
typedef struct _EssenceStreamInfo  EssenceStreamInfo;



// Example dark metadata
namespace mxflib
{
	const UInt8 ProMPEGForum_Dark_UL_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x05, 0x0d, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
	const UL ProMPEGForum_Dark_UL(ProMPEGForum_Dark_UL_Data);
	ULPtr ProMPEGForum_Dark_ULPtr = new UL( ProMPEGForum_Dark_UL_Data );
}




//! Convert a frame-count timecode from one edit rate to another
/*! \param Timecode The timecode in CurrentEditRate
*  \param CurrentEditRate The edit rate of the input timecode
*  \param DesiredEditRate The desired output edit rate
*  \param AllowErrors If set to false error messages will be suppressed
*  \return The frame-count edit rate, or 0 if any error occured
*  \note Error messages will be produced if you attempt to convert a non-zero timecode to or from a zero edit rate, or beyond the range of 64-bit maths.
*  \note No error message will be produced if there is a zero input Timecode, even if other parameters are bad
*/
Position ConvertTimecode(Position Timecode, Rational CurrentEditRate, Rational DesiredEditRate, bool AllowErrors = true)
{
	// No need to adjust if timecode starts at zero
	if(Timecode != 0)
	{
		// Must check the data before correcting to avoid divide by 0!
		if(CurrentEditRate.Numerator == 0)
		{
			if(AllowErrors) error("Can't convert timecode to a zero edit rate, setting to zero\n");
			Timecode = 0;
		}
		else if(DesiredEditRate.Denominator == 0)
		{
			if(AllowErrors) error("Can't convert timecode from zero edit rate, setting to zero\n");
			Timecode = 0;
		}
		else
		{
			// Ensure we are working on a level ground
			CurrentEditRate.Reduce();
			DesiredEditRate.Reduce();

			// Should we correct?
			if(!(CurrentEditRate == DesiredEditRate))
			{
				// Is it safe to do the adjustment
				// TODO: This range check is over-cautious, it could be better
				Int64 TCMultiply = CurrentEditRate.Denominator;
				TCMultiply *= DesiredEditRate.Numerator;
				if((Timecode > INT64_C(0x00000000ffffffff)) || (TCMultiply > INT64_C(0x00000000ffffffff)))
				{
					if(AllowErrors) error("Unable to frame-rate correct timecode due to huge timecode or extreme edit-rate, setting to zero\n");
					Timecode = 0;
				}
				else
				{
					Timecode *= TCMultiply;
					Timecode /= CurrentEditRate.Numerator * DesiredEditRate.Denominator;
				}
			}
		}
	}

	// Return the adjusted timecode
	return Timecode;
}


//! Short term hack to allow per-BodySID GCWriters
/*! DRAGONS: This needs to be tidied a little when there is time! */
GCWriterPtr AddGCWriter(std::map<int, GCWriterPtr> &Map, MXFFilePtr &File, int BodySID)
{
	// First try and return an existing GCWriter
	std::map<int, GCWriterPtr>::iterator it = Map.find(BodySID);
	if(it != Map.end()) return (*it).second;

	// Insert a new writer
	Map.insert(std::map<int, GCWriterPtr>::value_type(BodySID, new GCWriter(File, BodySID)));

	// Find and return the new entry (not hugely efficient!)
	it = Map.find(BodySID);

	return (*it).second;
}


//! Set the index options for a given body stream
void SetStreamIndex(BodyStreamPtr &ThisStream, bool isCBR, ProcessOptions		*pOpt)
{
	// Set CBR indexing flags
	if(isCBR)
	{
		// If this stream is not CBR indexable don't set any flags
		if(ThisStream->GetSource()->GetBytesPerEditUnit() == 0) return;

		if(pOpt->IsolatedIndex)
		{
			if(pOpt->UseIndex) ThisStream->SetIndexType( (BodyStream::IndexType) ( BodyStream::StreamIndexCBRHeaderIsolated 
				| BodyStream::StreamIndexCBRFooter) );
		}
		else
		{
			if(pOpt->UseIndex) ThisStream->SetIndexType( (BodyStream::IndexType) ( BodyStream::StreamIndexCBRHeader 
				| BodyStream::StreamIndexCBRBody 
				| BodyStream::StreamIndexCBRFooter) );
		}
	}
	// Set VBR indexing flags
	else
	{
		// If this stream is not VBR indexable don't set any flags
		if(!ThisStream->GetSource()->CanIndex()) return;

		if(pOpt->UseIndex) 
			ThisStream->AddIndexType(BodyStream::StreamIndexFullFooter);
		if(pOpt->SparseIndex) 
			ThisStream->AddIndexType(BodyStream::StreamIndexSparseFooter);
		if(pOpt->SprinkledIndex) 
		{
			if(pOpt->IsolatedIndex) 
				ThisStream->AddIndexType(BodyStream::StreamIndexSprinkledIsolated);
			else 
				ThisStream->AddIndexType(BodyStream::StreamIndexSprinkled);
		}

	}
}


//! Set the wrapping type for a stream
void SetStreamWrapType(BodyStreamPtr &ThisStream, WrappingOption::WrapType Type)
{
	if(Type == WrappingOption::Frame) ThisStream->SetWrapType(BodyStream::StreamWrapFrame);
	else if(Type == WrappingOption::Clip) ThisStream->SetWrapType(BodyStream::StreamWrapClip);
	else ThisStream->SetWrapType(BodyStream::StreamWrapOther);
}




void ProcessMetadata(int OutFileNum,
					 ProcessOptions    *pOpt,
					 EssenceSourcePair *Source,
					 EssenceParser::WrappingConfigList WrapCfgList,
					 Rational		  EditRate,
					 BodyWriterPtr     Writer,
					 MetadataPtr       MData,
					 UMIDPtr			  MPUMID,
					 UMIDPtr			 *FPUMID,
					 UMIDPtr			 *SPUMID,
					 EssenceStreamInfo*EssStrInf,
					 PackagePtr           &FilePackage,  //OUT variable
					 TimecodeComponentPtr &MPTimecodeComponent //OUT variable
					 )
{

	// Avid special

	MDObjectPtr ContainerDef_MXF;

#if defined( NeedAvidCodecDef )
	// Not needed since Xpress Pro HD 5.
	MDObjectPtr CodecJPEG;
#endif



	EssenceParser::WrappingConfigList::iterator WrapCfgList_it;
	// Set the OP label
	// If we are writing OP-Atom we write the header as OP1a initially as another process
	// may try to read the file before it is complete and then it will NOT be a valid OP-Atom file
	// DRAGONS: This should be OPxx which matches the number of file packages...
	if(pOpt->OPAtom) MData->SetOP(OP1aUL); else MData->SetOP(pOpt->OPUL);

	// Infer dropframe from the edit rate
	bool DropFrame = false;

	// Work out the edit rate from the descriptor
	UInt32 FrameRate = EditRate.Numerator;

	// Use drop-frame for any non-integer frame rate
	if(EditRate.Denominator > 1)
	{
		// This is an integer equivalent of FrameRate = floor((FrameRate + 0.5) / Denominator)
		FrameRate += EditRate.Denominator - 1;
		FrameRate /= EditRate.Denominator;

		DropFrame = true;
	}


	// Build the Material Package
	PackagePtr MaterialPackage = MData->AddMaterialPackage(pOpt->MobName, MPUMID);

	// OP-Atom requires Primary Package, other OPs don't mandate
	MData->SetPrimaryPackage(MaterialPackage);		// This will be overwritten for OP-Atom


	/*	if(pOpt->ExpOption)
	{
	MDObjectPtr PackageMarkerObject = new MDObject(PackageMarkerObject_UL);
	PackageMarkerObject->SetUInt(TimebaseReferenceTrackID_UL, 1);

	MaterialPackage->MakeRef(PackageMarker_UL, PackageMarkerObject);
	}
	*/




	// Add a Timecode Track
	TrackPtr MPTimecodeTrack;
	if(pOpt->PutTCTrack)
		MPTimecodeTrack= MaterialPackage->AddTimecodeTrack(EditRate);

	// Add a single Timecode Component
	if(pOpt->PutTCTrack)
	{
		Position StartTimecode = TCtoFrames( FrameRate, DropFrame, 1, 0, 0, 0 );
		MPTimecodeComponent= MPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, StartTimecode);
	}


	// Set the writer's general parameters
	Writer->SetKAG(pOpt->KAGSize);
	Writer->SetForceBER4(true);

	// SMPTE 390M does not recommend placing Essence in the header partition
	if(pOpt->OPAtom && (!pOpt->OPAtom2Part))
	{
		// Index data can't share with metadata if very-isolated, essence can never share with metadata
		Writer->SetMetadataSharing(!pOpt->VeryIsolatedIndex, false);
	}
	else
	{
		// Index data can't share with metadata if very-isolated but essence can always share with metadata
		Writer->SetMetadataSharing(!pOpt->VeryIsolatedIndex, true);
	}







	/* Add essence streams to the writer */ 
	/* ================================= */
	/* These are all added before building the metadata tracks so that the track numbers have complete stream counts */
	/* DRAGONS: This code MUST be kept in step with the logic of the following loop */

	int PreviousFP = -1;							// The index of the previous file package used - allows us to know if we treat this is a sub-stream
	int iStream = -1;								// Stream index (note that it will be incremented to 0 in the first iteration)
	int iTrack = 0;									// Track index
	WrapCfgList_it = WrapCfgList.begin();
	while(WrapCfgList_it != WrapCfgList.end())
	{
		// Move on to a new stream if we are starting a new file package
		if(Source[iTrack].first != PreviousFP) iStream++;

		// Write File Packages except for externally ref'ed essence in OP-Atom
		bool WriteFP = (!pOpt->OPAtom) || (iStream == OutFileNum);

		if(WriteFP && (!(*WrapCfgList_it)->IsExternal)) // (iTrack == OutFileNum)
		{
			// DRAGONS: Always write a track if writing avid atom files, and this is the file holding the essence for the current track
			if((Source[iTrack].first != PreviousFP)
				)
			{
				// Build a stream object to write
				EssStrInf[iStream].Stream = new BodyStream(iStream + 1, Source[iTrack].second);
				SetStreamWrapType(EssStrInf[iStream].Stream, (*WrapCfgList_it)->WrapOpt->ThisWrapType);

				// Add this stream to the body writer
				Writer->AddStream(EssStrInf[iStream].Stream);
			}
			else
			{
				EssStrInf[iStream].Stream->AddSubStream(Source[iTrack].second);
			}

			// The source will be allocated a streamID when it is added to the BodyStream - we need that for track linking later
			EssStrInf[iTrack].EssenceID = Source[iTrack].second->GetStreamID();

			// Ensure that this stream gets zero-padded beyond the end of essence, if requested
			if(pOpt->ZeroPad) Source[iTrack].second->SetOption("EndPadding", 1);
		}

		// Record the file package index used this time
		PreviousFP = Source[iTrack].first;

		// Increment the track
		WrapCfgList_it++;
		iTrack++;
	}


	/* Build the File Packages and all essence tracks */
	/* ============================================== */
	/* DRAGONS: This code MUST be kept in step with the logic of the above loop */

	PreviousFP = -1;								// The index of the previous file package used - allows us to know if we treat this is a sub-stream
	iStream = -1;									// Stream index (note that it will be incremented to 0 in the first iteration)
	iTrack = 0;										// Track index
	WrapCfgList_it = WrapCfgList.begin();
	while(WrapCfgList_it != WrapCfgList.end())
	{

		TrackPtr FPTimecodeTrack;

		// Move on to a new stream if we are starting a new file package
		if(Source[iTrack].first != PreviousFP) iStream++;

		// Write File Packages except for externally ref'ed essence in OP-Atom
		bool WriteFP = (!pOpt->OPAtom) || (iStream == OutFileNum);

		if(WriteFP) // (iTrack == OutFileNum)
		{
			if(Source[iTrack].first != PreviousFP)
			{
				/* ====================================================================================================== */
				/* DRAGONS: This is the point that must duplicate the conditions for iStream and iTrack in the loop above */
				/* ====================================================================================================== */

				if(!(*WrapCfgList_it)->IsExternal)
				{
					// Force edit-unit align if requested
					if(pOpt->EditAlign) EssStrInf[iStream].Stream->SetEditAlign(true);

					// Set indexing options for this stream
					if(pOpt->UseIndex || pOpt->SparseIndex || pOpt->SprinkledIndex)
					{
						if((*WrapCfgList_it)->WrapOpt->CBRIndex && (EssStrInf[iStream].Stream->GetSource()->GetBytesPerEditUnit() != 0)) 
						{
							SetStreamIndex(EssStrInf[iStream].Stream, true, pOpt); 
						}
						else
						{
							if((*WrapCfgList_it)->WrapOpt->ThisWrapType == WrappingOption::Frame)
							{
								SetStreamIndex(EssStrInf[iStream].Stream, false, pOpt);
							}
							else
							{
								// Attempt to force the parser to use VBR indexing mode (i.e. return each edit unit individually)
								if(Source[iTrack].second->EnableVBRIndexMode())
								{
									SetStreamIndex(EssStrInf[iStream].Stream, false, pOpt);
								}
								else
								{
									if((*WrapCfgList_it)->WrapOpt->ThisWrapType == WrappingOption::Clip)
										warning("VBR Indexing not supported by \"%s\" when clip-wrapping\n", Source[iTrack].second->Name().c_str());
									else
										warning("VBR Indexing not supported by \"%s\" for the selected wrapping mode\n", Source[iTrack].second->Name().c_str());
								}
							}
						}
					}
				}

				// Add the file package
				UInt32 BodySID = (*WrapCfgList_it)->IsExternal ? 0 : iStream+1;
				FilePackage = MData->AddFilePackage(BodySID, std::string("File Package: ") + (*WrapCfgList_it)->WrapOpt->Description, FPUMID[iStream]);


				// Add a timecode track if requested
				if(pOpt->PutTCTrack)
				{
					FPTimecodeTrack = FilePackage->AddTimecodeTrack(EditRate);

					Position StartTimecode;
					if(!pOpt->ExtractTimecode)
					{
						StartTimecode = TCtoFrames( FrameRate, DropFrame, 1, 0, 0, 0 );
					}
					else
					{
						StartTimecode = ConvertTimecode( (*WrapCfgList_it)->StartTimecode, (*WrapCfgList_it)->EditRate, EditRate);
					}

					EssStrInf[iStream].FPTimecodeComponent = FPTimecodeTrack->AddTimecodeComponent(FrameRate, DropFrame, StartTimecode );
				}
			}
		}

		// Add the appropriate Track to the Material Package (if this track is required on the material package)
		if((iStream == OutFileNum) && (iStream < pOpt->InFileGangSize)) // first gang only
		{
			switch((*WrapCfgList_it)->WrapOpt->GCEssenceType)
			{
			case 0x18:	// Make DV compound essence in to picture tracks
			case 0x05: case 0x15: 
				EssStrInf[iTrack].MPTrack = MaterialPackage->AddPictureTrack(1, EditRate, "V1");
				break;
			case 0x06: case 0x16:
				{
					std::string TrackName="A";
					TrackName+='0'+iTrack;
					EssStrInf[iTrack].MPTrack = MaterialPackage->AddSoundTrack(iTrack,EditRate,TrackName);
				}
				break;
			case 0x07: case 0x17: default:
				EssStrInf[iTrack].MPTrack = MaterialPackage->AddDataTrack(EditRate);
				break;
			}
		}




		// Add the track to the file package
		if(WriteFP) 
		{
			if( 
				!pOpt->OPAtom ||  
				iStream==OutFileNum) 
			{
				// Use track number 0 for external essence
				UInt32 TrackID = (*WrapCfgList_it)->IsExternal ? 0 : EssStrInf[iStream].Stream->GetTrackNumber(Source[iTrack].second->GetStreamID());

				switch((*WrapCfgList_it)->WrapOpt->GCEssenceType)
				{
				case 0x18:	// Make DV compound essence in to picture tracks
				case 0x05: case 0x15:
					// PhysicalTrackNumber = stream), ++TrackID
					EssStrInf[iTrack].FPTrack = FilePackage->AddPictureTrack(TrackID, EditRate, "V1");
					break;
				case 0x06: case 0x16:
					{
						std::string TrackName="A";
						TrackName+='0'+iTrack;
							EssStrInf[iTrack].FPTrack = FilePackage->AddSoundTrack(TrackID, EditRate,TrackName);
					}
					break;
				case 0x07: case 0x17: default:
					EssStrInf[iTrack].FPTrack = FilePackage->AddDataTrack(TrackID, EditRate);
					break;
				}
			}
		}


		// Locate the material package track this essence is in
		int TrackNumber = iTrack;
		while(TrackNumber >= pOpt->InFileGangSize) TrackNumber -= pOpt->InFileGangSize;

		// Add a single Component to this Track of the Material Package
		int STID = -1;

		if(EssStrInf[TrackNumber].MPTrack)
		{
			EssStrInf[iTrack].MPClip = EssStrInf[TrackNumber].MPTrack->AddSourceClip();
			STID=EssStrInf[iTrack].MPClip[SourceTrackID_UL]->GetInt();
		}

		// Add a single Component to this Track of the File Package
		if(WriteFP)
		{
				EssStrInf[iTrack].FPClip = SmartPtr_Cast( EssStrInf[iTrack].FPTrack->AddSourceClip(), Component );
		}





		// Add the file descriptor to the file package
		// except for externally ref'ed essence in OP-Atom
		if( pOpt->OPAtom )
		{
			// Write a File Descriptor only on the internally ref'ed Track 
			if( WriteFP ) // (iTrack == OutFileNum)
			{
				MDObjectPtr Descriptor=(*WrapCfgList_it)->EssenceDescriptor;

				FilePackage->AddChild(Descriptor_UL)->MakeLink(Descriptor);



				MData->AddEssenceType((*WrapCfgList_it)->WrapOpt->WrappingUL);

				if((*WrapCfgList_it)->EssenceDescriptor->IsA(MultipleDescriptor_UL))
				{
					// Ensure that we have flagged a multiple descriptor if one is used
					ULPtr GCUL = new UL( mxflib::GCMulti_Data );
					MData->AddEssenceType( GCUL );
				}

				// Link the MP to the FP
				if(EssStrInf[iTrack].MPClip) EssStrInf[iTrack].MPClip->MakeLink(EssStrInf[iTrack].FPTrack, 0);
			}
			else // (!WriteFP)
			{
				// Link the MP to the external FP
				// DRAGONS: We must assume what the linked track will be... track 1 picked as that is what is needed for OP atom files
				if(EssStrInf[iTrack].MPClip) EssStrInf[iTrack].MPClip->MakeLink(FPUMID[iTrack], 1, 0);
			}
		}
		else if( pOpt->FrameGroup ) // !pOpt->OPAtom
		{
			if(WriteFP)
			{
				MDObjectPtr MuxDescriptor = FilePackage->GetRef(Descriptor_UL);

				// Write a MultipleDescriptor only on the first Iteration
				if( !MuxDescriptor )
				{
					MuxDescriptor = new MDObject(MultipleDescriptor_UL);
					MuxDescriptor->AddChild(SampleRate_UL)->SetInt("Numerator",(*WrapCfgList_it)->EssenceDescriptor[SampleRate_UL]->GetInt("Numerator"));
					MuxDescriptor->AddChild(SampleRate_UL)->SetInt("Denominator",(*WrapCfgList_it)->EssenceDescriptor[SampleRate_UL]->GetInt("Denominator"));

					MuxDescriptor->AddChild(EssenceContainer_UL,false)->SetValue(DataChunk(16,mxflib::GCMulti_Data));

					MuxDescriptor->AddChild(FileDescriptors_UL);
					FilePackage->AddChild(Descriptor_UL)->MakeLink(MuxDescriptor);
				}

				// Write a SubDescriptor
				(*WrapCfgList_it)->EssenceDescriptor->SetUInt(LinkedTrackID_UL, EssStrInf[iTrack].FPTrack->GetUInt(TrackID_UL));

				MuxDescriptor[FileDescriptors_UL]->AddChild()->MakeLink((*WrapCfgList_it)->EssenceDescriptor);

				MData->AddEssenceType((*WrapCfgList_it)->WrapOpt->WrappingUL);

				// Link the MP to the FP
				if(EssStrInf[iTrack].MPClip) EssStrInf[iTrack].MPClip->MakeLink(EssStrInf[iTrack].FPTrack, 0);
			}
		}
		else // !pOpt->OPAtom, !FrameGroup
		{
			if(WriteFP)
			{
				// Check that we are not about to add a second descriptor to this file package (e.g. because we are processing a sub-stream such as captions)
				MDObjectPtr Descriptor = FilePackage->GetRef(Descriptor_UL);
				if(!Descriptor)
				{
					// Write a FileDescriptor
					// DRAGONS Can we ever need a MultipleDescriptor?
					(*WrapCfgList_it)->EssenceDescriptor->SetUInt(LinkedTrackID_UL, EssStrInf[iTrack].FPTrack->GetUInt(TrackID_UL));
					FilePackage->AddChild(Descriptor_UL)->MakeLink((*WrapCfgList_it)->EssenceDescriptor);

					// Add the essence type
					MData->AddEssenceType((*WrapCfgList_it)->WrapOpt->WrappingUL);

					if((*WrapCfgList_it)->EssenceDescriptor->IsA(MultipleDescriptor_UL))
					{
						// Ensure that we have flagged a multiple descriptor if one is used
						ULPtr GCUL = new UL( mxflib::GCMulti_Data );
						MData->AddEssenceType( GCUL );
					}
				}
				else
				{
					/* Already added a descriptor to this file package */

					// If the existing descriptor is not a file descriptor, we need to convert it
					if(!Descriptor->IsA(MultipleDescriptor_UL))
					{
						// Build a new multiple descriptor
						MDObjectPtr MuxDescriptor = new MDObject(MultipleDescriptor_UL);

						MDObjectPtr SampleRate = Descriptor->Child(SampleRate_UL);
						if(SampleRate)
						{
							MuxDescriptor->AddChild(SampleRate_UL)->SetInt("Numerator", SampleRate->GetInt("Numerator"));
							MuxDescriptor->AddChild(SampleRate_UL)->SetInt("Denominator", SampleRate->GetInt("Denominator"));
						}

						MuxDescriptor->AddChild(EssenceContainer_UL,false)->SetValue(DataChunk(16,mxflib::GCMulti_Data));

						// Ensure that we have flagged a multiple descriptor
						ULPtr GCUL = new UL( mxflib::GCMulti_Data );
						MData->AddEssenceType( GCUL );

						// Add the existing descriptor as the first sub-descriptor
						MuxDescriptor->AddRef(FileDescriptors_UL, Descriptor);

						// Make the multi descriptor the new file descriptor
						FilePackage->MakeRef(Descriptor_UL, MuxDescriptor);
						Descriptor = MuxDescriptor;
					}

					// The new descriptor is a multiple descriptor already
					if((*WrapCfgList_it)->EssenceDescriptor->IsA(MultipleDescriptor_UL))
					{
						// Add each child of the new multiple descriptor to the old one
						MDObjectPtr FileDescriptors = (*WrapCfgList_it)->EssenceDescriptor->Child(FileDescriptors_UL);
						if(FileDescriptors)
						{
							MDObject::iterator it = FileDescriptors->begin();
							while(it != FileDescriptors->end())
							{
								MDObjectPtr Ptr = (*it).second->GetRef();
								if(Ptr) Descriptor->AddRef(FileDescriptors_UL, Ptr);
								it++;
							}
						}
					}
					else
					{
						// Add the new descriptor as a sub-descriptor
						Descriptor->AddRef(FileDescriptors_UL, (*WrapCfgList_it)->EssenceDescriptor);
					}
				}

				// Link the MP to the FP
				if(EssStrInf[iTrack].MPClip) EssStrInf[iTrack].MPClip->MakeLink(EssStrInf[iTrack].FPTrack, 0);
			}
		}

		// Record the file package index used this time
		PreviousFP = Source[iTrack].first;

		WrapCfgList_it++;
		iTrack++;
	}

	/* Ensure that building frame-wrap groups has not added a multiple descriptor containing a single descriptor in any File Package */
	if(pOpt->FrameGroup)
	{
		PackageList::iterator it = MData->Packages.begin();
		while(it != MData->Packages.end())
		{
			if((*it)->IsA(SourcePackage_UL))
			{
				MDObjectPtr Descriptor = (*it)->GetRef(Descriptor_UL);
				if(Descriptor && Descriptor->IsA(MultipleDescriptor_UL))
				{
					MDObjectPtr Descriptors = Descriptor[FileDescriptors_UL];

					if((!Descriptors) || Descriptors->empty())
					{
						error("Ended up with an empty MultipleDescriptor - are we building an emty file?\n");
					}
					else if(Descriptors->size() == 1)
					{
						// Link the single contained descriptor directly from the file package so the multiple descriptor will evaporate now it is unreferenced
						MDObjectPtr Link = Descriptors->front().second->GetRef();
						if(Link) FilePackage->MakeRef(Descriptor_UL, Link);
					}
					else
					{
						// Ensure that we have flagged a multiple descriptor
						// DRAGONS: We do this here so that it is only added when genuinely required
						ULPtr GCUL = new UL( mxflib::GCMulti_Data );
						MData->AddEssenceType( GCUL );
					}
				}
			}
			it++;
		}
	}

}



void SetUpIndex(int OutFileNum,
				ProcessOptions    *pOpt,
				MetadataPtr       MData,
				EssenceSourcePair *Source,
				EssenceParser::WrappingConfigList WrapCfgList,
				EssenceStreamInfo*EssStrInf)
{
	EssenceParser::WrappingConfigList::iterator WrapCfgList_it;
	// Find all essence container data sets so we can update "IndexSID"
	MDObjectPtr ECDataSets = MData[ContentStorageObject_UL];
	if(ECDataSets) ECDataSets = ECDataSets->GetLink();
	if(ECDataSets) ECDataSets = ECDataSets[EssenceDataObjects_UL];

	int PreviousFP = -1;								// The index of the previous file package used - allows us to know if we treat this is a sub-stream
	int iStream = -1;									// Stream index (note that it will be incremented to 0 in the first iteration)
	int iTrack=0;
	WrapCfgList_it = WrapCfgList.begin();
	while(WrapCfgList_it != WrapCfgList.end())
	{
		// Only process the index for the first stream of a file package
		if((Source[iTrack].first != PreviousFP || pOpt->OPAtom) && (!(*WrapCfgList_it)->IsExternal))
		{
			iStream++;

			// Only index it if we can
			// Currently we can only VBR index frame wrapped essence
			// FIXME: We enable the VBR mode twice doing it this way, which is not ideal - should we cache the result? Or do we even need to check?
			if(    ((*WrapCfgList_it)->WrapOpt->CBRIndex && (Source[iTrack].second->GetBytesPerEditUnit() != 0))
				|| (((*WrapCfgList_it)->WrapOpt->CanIndex) && (pOpt->AvidMXF) 
				|| ((*WrapCfgList_it)->WrapOpt->ThisWrapType == WrappingOption::Frame
				|| Source[iTrack].second->EnableVBRIndexMode() )))
			{
				if(	 (pOpt->OPAtom	&& iTrack==OutFileNum)
					||	(!pOpt->OPAtom && pOpt->FrameGroup && iTrack==0)
					||	(!pOpt->OPAtom && !pOpt->FrameGroup) )
				{
					UInt32 BodySID;				// Body SID for this essence stream
					UInt32 IndexSID;			// Index SID for the index of this essence stream

					BodySID = EssStrInf[iStream].Stream->GetBodySID();
						IndexSID = BodySID + 128;

					EssStrInf[iStream].Stream->SetIndexSID(IndexSID);

					// Update IndexSID in essence container data set
					if(ECDataSets)
					{
						MDObject::iterator ECD_it = ECDataSets->begin();
						while(ECD_it != ECDataSets->end())
						{
							if((*ECD_it).second->GetLink())
							{
								if((*ECD_it).second->GetLink()->GetUInt(BodySID_UL) == BodySID)
								{
									(*ECD_it).second->GetLink()->SetUInt(IndexSID_UL, IndexSID);
									break;
								}
							}
							ECD_it++;
						}
					}
				}
			}
		}

		// Record the file package index used this time
		PreviousFP = Source[iTrack].first;

		WrapCfgList_it++;
		iTrack++;
	}
}
//!return Essence duration
Length ProcessEssence(int OutFileNum,
					  ProcessOptions    *pOpt,
					  EssenceSourcePair *Source,
					  EssenceParser::WrappingConfigList WrapCfgList,
					  BodyWriterPtr      Writer,
					  Rational			  EditRate,
					  MetadataPtr        MData,
					  EssenceStreamInfo *EssStrInf,
					  TimecodeComponentPtr MPTimecodeComponent
					  )
{
	EssenceParser::WrappingConfigList::iterator WrapCfgList_it;

#ifdef _WIN32
	LARGE_INTEGER  start;
	QueryPerformanceCounter(&start);
#else
	struct timeval start;
	struct timezone tz;
	gettimeofday(& start, &tz);
#endif

	// Write the body
	if(pOpt->BodyMode == Body_None)
	{
		Writer->WriteBody();
	}
	else
	{
		while(!Writer->BodyDone())
		{
			if(pOpt->BodyMode == Body_Duration)
				Writer->WritePartition(pOpt->BodyRate, 0);
			else
				Writer->WritePartition(0, pOpt->BodyRate);
		}
	}

	// Update the modification time
	MData->SetTime();

	// Update all durations
	Int32 IndexBaseTrack;
	if( pOpt->OPAtom ) IndexBaseTrack = OutFileNum;
	else if( pOpt->FrameGroup ) IndexBaseTrack = 0;
	else  IndexBaseTrack = 0;

	Length EssenceDuration;
	if(EssStrInf[IndexBaseTrack].Stream)
		EssenceDuration = (Length) EssStrInf[IndexBaseTrack].Stream->GetSource()->GetCurrentPosition();
	else
		EssenceDuration = -1;


	if(pOpt->PutTCTrack)
		MPTimecodeComponent->SetDuration(EssenceDuration);

#ifdef _WIN32
	LARGE_INTEGER  end;
	QueryPerformanceCounter(&end);
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	if(Freq.QuadPart!=0)
	{
		__int64 diff=end.QuadPart-start.QuadPart;
		float time=((float)diff)/Freq.QuadPart;
		float fps=EssenceDuration/time;

		printf("Completed %d frames at %4.3f fps\n",(int)EssenceDuration,fps);
	}
#else
	struct timeval end;
	gettimeofday(& end, &tz);
	time_t secs=end.tv_sec-start.tv_sec;
	int usecs=end.tv_usec-start.tv_usec;
	float time=(float)secs+(float)usecs/1000000.0;
	float fps=EssenceDuration/time;

	printf("Completed %d frames at %4.3f fps\n",(int)EssenceDuration,fps);

#endif


	/*	// Write the body
	if(pOpt->BodyMode == Body_None)
	{
	Writer->WriteBody();
	}
	else
	{
	while(!Writer->BodyDone())
	{
	if(pOpt->BodyMode == Body_Duration)
	Writer->WritePartition(pOpt->BodyRate, 0);
	else
	Writer->WritePartition(0, pOpt->BodyRate);
	}
	}
	*/

	int PreviousFP = -1;								// The index of the previous file package used - allows us to know if we treat this is a sub-stream
	int iStream = -1;									// Stream index (note that it will be incremented to 0 in the first iteration)
	int iTrack = 0;
	WrapCfgList_it = WrapCfgList.begin();
	while(WrapCfgList_it != WrapCfgList.end())
	{
		// Move on to a new stream if we are starting a new file package
		if(Source[iTrack].first != PreviousFP) iStream++;

		if(EssStrInf[iTrack].MPClip)
		{
			EssStrInf[iTrack].MPClip->SetDuration(EssenceDuration);

			if((!pOpt->OPAtom) || (iStream == OutFileNum))
			{
				if(pOpt->PutTCTrack)
					if((iTrack==0) || (!pOpt->FrameGroup))
						if(EssStrInf[iStream].FPTimecodeComponent)
							EssStrInf[iStream].FPTimecodeComponent->SetDuration(EssenceDuration);

				EssStrInf[iTrack].FPClip->SetDuration(EssenceDuration);
				(*WrapCfgList_it)->EssenceDescriptor->SetInt64(ContainerDuration_UL,EssenceDuration);

				// Update origin if required
				// DRAGONS: This is set in the File Package - the spec seems unclear about which Origin should be set!
				Position Origin = Source[iTrack].second->GetPrechargeSize();
				if(Origin)
				{
					TrackParent FPTrack = EssStrInf[iTrack].FPClip->GetParent();
					if(FPTrack)	FPTrack->SetInt64(Origin_UL, Origin);
				}
			}

		}


		// Record the file package index used this time
		PreviousFP = Source[iTrack].first;

		WrapCfgList_it++;
		iTrack++;
	}

	// return the finished length to the caller
	return EssenceDuration;
}


//! Process an output file
Length Process(
			   int					OutFileNum,
			   MXFFilePtr			Out,
			   ProcessOptions		*pOpt,
			   EssenceParser::WrappingConfigList WrapCfgList,
			   EssenceSourcePair	*Source,
			   Rational			EditRate,
			   UMIDPtr				MPUMID,
			   UMIDPtr				*FPUMID,
			   UMIDPtr				*SPUMID,
			   bool				*pReadyForEssenceFlag  /* =NULL */
			   )

{
	EssenceParser::WrappingConfigList::iterator WrapCfgList_it;
	TimecodeComponentPtr MPTimecodeComponent ;

	Length Ret = 0;

	EssenceStreamInfo EssStrInf[ProcessOptions::MaxInFiles];
	// FP UMIDs are the same for all OutFiles, so they are supplied as a parameter


	PackagePtr FilePackage;


	/* Step: Create a set of header metadata */

	MetadataPtr MData = new Metadata();
	mxflib_assert(MData);
	mxflib_assert(MData->Object);

	// Build the body writer
	BodyWriterPtr Writer = new BodyWriter(Out);


#if defined FORCEGCMULTI 
	// 377M MultipleDescriptor (D.5) requires an EssenceContainer label (D.1), which must be this
	// degenerate label (see mxfIG FAQ). Therefore the degenerate value must also appear in the
	// Header (A.1) and partition pack...
	// also, explicitly required by AS-CNN sec 2.1.6

	// DRAGONS: Why is this here? It unconditionally adds "Used to describe multiple wrappings not
	//          otherwise covered under the MXF Generic Container node" to all MXF files!!


	// Assume we are doing GC
	ULPtr GCUL = new UL( mxflib::GCMulti_Data );
	MData->AddEssenceType( GCUL );

	// This appears to be acceptable to Avid XpressProHD 5.1.2
#endif


	ProcessMetadata(OutFileNum,pOpt,Source, WrapCfgList,EditRate,Writer,
		MData,MPUMID,FPUMID,SPUMID,EssStrInf,
		FilePackage, MPTimecodeComponent  //OUT variables
		);

	//
	// ** Set up IndexSID **
	//
	if(pOpt->UseIndex || pOpt->SparseIndex || pOpt->SprinkledIndex)
	{
		SetUpIndex(OutFileNum,pOpt, MData,Source,WrapCfgList,EssStrInf);
	}

	//
	// ** Set up the base partition pack **
	//

	PartitionPtr ThisPartition = new Partition(OpenHeader_UL);
	mxflib_assert(ThisPartition);
	ThisPartition->SetKAG(pOpt->KAGSize);			// Everything else can stay at default
	ThisPartition->SetUInt(BodySID_UL, 1);

	std::string PlatformName="MXFactory ( built "  __DATE__ " " __TIME__ " on " + OSName() + " using "+LibraryName() +" with Metaglue Extensions )";

	// Build an Ident set describing us and link into the metadata
	MDObjectPtr Ident = new MDObject(Identification_UL);

	Ident->SetString(ProductName_UL, ProductName);
	Ident->SetString(ProductVersion_UL, ProductProductVersion);
	Ident->SetString(VersionString_UL, ProductVersionString);
	Ident->SetString(ToolkitVersion_UL, LibraryProductVersion());
	Ident->SetString(Platform_UL, PlatformName);
	Ident->SetValue(ProductUID_UL, DataChunk(16,Product_UL.GetValue()));

	// Link the new Ident set with all new metadata
	// Note that this is done even for OP-Atom as the 'dummy' header written first
	// could have been read by another device. This flags that items have changed.
	MData->UpdateGenerations(Ident);

	ThisPartition->AddMetadata(MData);

	// Add the template partition to the body writer
	Writer->SetPartition(ThisPartition);

	//
	// ** Process Essence **
	//

	// Write the header (open and incomplete so far)

	// Set block alignment for Avid compatibility
	// with an extra -ve offset for essence to align the V rather than the K

	const UInt32 PartitionPackLength = 0x7c;

	const UInt32 AvidBlockSize = 0x60000;
	const UInt32 AvidKAGSize = 512;
	const UInt32 AvidIndexBERSize = 9;

	const UInt32 ULSize = 16;
	int DynamicOffset = 0-ULSize;


	// Kludge to find the most likely BERSize
	EssenceSourcePtr Stream0 = EssStrInf[ OutFileNum ].Stream ? *(EssStrInf[ OutFileNum ].Stream->begin()) : NULL;
	if( !Stream0 || Stream0->GetBERSize() == 0) DynamicOffset -= 4; else DynamicOffset -= Stream0->GetBERSize();

		if( pOpt->BlockSize )
		{
			// set dynamic default if -ko=-1000
			if( pOpt->BlockOffset == -1000 ) pOpt->BlockOffset = DynamicOffset;

			Out->SetBlockAlign( pOpt->BlockSize, pOpt->BlockOffset, pOpt->BlockIndexOffset );
		}

		// Use padding per command line - even for block aligned files
		if(pOpt->HeaderPadding) Writer->SetPartitionFiller(pOpt->HeaderPadding);
		if(pOpt->HeaderSize) Writer->SetPartitionSize(pOpt->HeaderSize);

		// DRAGONS: would be nice to have an even length Header Partition
		//if(pOpt->HeaderSize) Writer->SetPartitionSize(pOpt->HeaderSize - PartitionPackLength);

		Writer->WriteHeader(false, false);


		// If we are writing OP-Atom update the OP label so that body partition packs claim to be OP-Atom
		// The header will remain as a generalized OP until it is re-written after the footer
		if(pOpt->OPAtom) 
		{
			MData->SetOP(OPAtomUL);

			// Set top-level file package correctly for OP-Atom
			// DRAGONS: This will need to be changed if we ever write more than one File Package for OP-Atom!
			MData->SetPrimaryPackage(FilePackage);
		}


		if(pReadyForEssenceFlag)
			*pReadyForEssenceFlag=true;

		Ret=ProcessEssence(OutFileNum,pOpt,Source,WrapCfgList,
			Writer,EditRate, MData,EssStrInf,MPTimecodeComponent
			);

		// Update SourcePackage Timecode Duration
		// DRAGONS: since we are assuming a 24 hour Source, don't need this
		// if( SPTimecodeComponent ) SPTimecodeComponent->SetDuration(EssenceDuration);

		// Update SourcePackage Edgecode Duration
		// DRAGONS: since we are assuming a 10000 foot Source, don't need this
		// if( SPEdgecodeComponent ) SPEdgecodeComponent->SetDuration(EssenceDuration);

		// Update the generation UIDs in the metadata to reflect the changes
		MData->UpdateGenerations(Ident);

		// Make sure any new sets are linked in
		ThisPartition->UpdateMetadata(MData);

		// Actually write the footer
		// Note: No metadata in OP-Atom footer
		if(pOpt->OPAtom) Writer->WriteFooter(false);
		else Writer->WriteFooter(true, true);

		//
		// ** Update the header ** 
		//
		// For generalized OPs update the value of "FooterPartition" in the header pack
		// For OP-Atom re-write the entire header
		//

		UInt64 FooterPos = ThisPartition->GetUInt64(FooterPartition_UL);
		Out->Seek(0);

		DataChunkPtr IndexData;

		if(pOpt->UpdateHeader)
		{
#ifndef WIN32
			static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
			pthread_mutex_lock(&mutex);
#endif

			// Read the old partition to allow us to keep the same KAG and SIDs
			PartitionPtr OldHeader = Out->ReadPartition();

			// Read any index table data
			IndexData = OldHeader->ReadIndexChunk();

			// Now update the partition we are about to write (the one with the metadata)
			ThisPartition->ChangeType(ClosedCompleteHeader_UL);
			ThisPartition->SetUInt64(FooterPartition_UL, FooterPos);
			ThisPartition->SetKAG(OldHeader->GetUInt(KAGSize_UL));
			ThisPartition->SetUInt(IndexSID_UL, OldHeader->GetUInt(IndexSID_UL));
			ThisPartition->SetUInt64(BodySID_UL, OldHeader->GetUInt(BodySID_UL));

			Out->Seek(0);
			if(IndexData)
				Out->ReWritePartitionWithIndex(ThisPartition, IndexData);
			else
				Out->ReWritePartition(ThisPartition);
#ifndef WIN32
			pthread_mutex_unlock(&mutex);
#endif
		}
		else
		{
			ThisPartition = Out->ReadPartition();
			ThisPartition->SetUInt64(FooterPartition_UL, FooterPos);
			Out->Seek(0);
			Out->WritePartitionPack(ThisPartition);
		}


		return Ret;
}

