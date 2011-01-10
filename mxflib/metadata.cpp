/*! \file	metadata.cpp
 *	\brief	Implementation of Metadata class
 *
 *			The Metadata class holds data about a set of Header Metadata.
 *			The class holds a Preface set object
 *
 *	\version $Id: metadata.cpp,v 1.13 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2003, Matt Beard
 *	Portions Copyright (c) 2003, Metaglue Corporation
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

/* Define metadata static members */

Track::TrackTypeMap Track::TrackTypes;		//!< List of known track type definitions
bool Track::TrackTypesInited;				//!< Set true once TrackTypeList has been initialized


//! Construct a basic Metadata object with current timestamp
mxflib::Metadata::Metadata()
{
	ModificationTime = Now2String();

	Init();
}

//! Construct a basic Metadata object with specified timestamp
mxflib::Metadata::Metadata(std::string TimeStamp)
{
	ModificationTime = TimeStamp;

	Init();
}


//! Common part of constructor
void Metadata::Init(void)
{
	Object = new MDObject(Preface_UL);

	// Even though it isn't used the preface needs an InstanceUID
	// as it is derived from InterchangeObject
	UUIDPtr ThisInstance = new UUID;
	Object->AddChild(InstanceUID_UL)->SetValue(DataChunk(16, ThisInstance->GetValue()));

	Object->AddChild(LastModifiedDate_UL)->SetString(ModificationTime);
	Object->AddChild(Version_UL)->SetInt(258);

	Object->AddChild(Identifications_UL);
	// To set later: OperationalPattern
	Object->AddChild(EssenceContainers_UL);
	Object->AddChild(DMSchemes_UL);

	// Add a content storage object
	MDObjectPtr Content = new MDObject(ContentStorage_UL);
	mxflib_assert(Content);
	Content->AddChild(Packages_UL);
	Content->AddChild(EssenceDataObjects_UL);

	Object->AddChild(ContentStorageObject_UL)->MakeRef(Content);
}


//! Add a DMScheme to the listed schemes
void Metadata::AddDMScheme(ULPtr Scheme)
{
	// Read the string value of this scheme once only
	std::string SchemeString = Scheme->GetString();

	// Get a list of current schemes
	MDObjectPtr SchemeList = Object->Child(DMSchemes_UL);

	// Compare the string value of all existing schemes to see if this is a new one
	MDObject::iterator it = SchemeList->begin();
	while(it != SchemeList->end())
	{
		if((*it).second->GetString() == SchemeString)
		{
			// Scheme already in list
			return;
		}

		it++;
	}

	// Not there so add it
	MDObjectPtr Ptr = SchemeList->AddChild();
	if(Ptr) Ptr->SetString(SchemeString);
}


// Add a package of the specified type to the matadata
PackagePtr mxflib::Metadata::AddPackage(const UL &PackageType, std::string PackageName, UMIDPtr PackageUMID, UInt32 BodySID /*=0*/)
{
	PackagePtr Ret;

	// If no UMID is supplied generate a general purpose UMID
	if(!PackageUMID) PackageUMID = MakeUMID(4);

	// Build the new package
	Ret = new Package(PackageType);
	if(!Ret) return Ret;

	// Set the package name if one supplied
	if(PackageName.length()) Ret->SetString(PackageName_UL, PackageName);

	// Set the package's properties
	Ret->AddChild(PackageUID_UL)->SetValue(PackageUMID->GetValue(), 32);
	Ret->SetString(PackageCreationDate_UL, ModificationTime);
	Ret->SetString(PackageModifiedDate_UL, ModificationTime);
	Ret->AddChild(Tracks_UL);

	// Add to the content storage set
	MDObjectPtr Ptr = Object->Child(ContentStorageObject_UL);
	if(Ptr) Ptr = Ptr->GetLink();
	if(Ptr) Ptr = Ptr[Packages_UL];
	if(Ptr) Ptr->AddChild()->MakeRef(Ret->Object);

	if(BodySID) AddEssenceContainerData(PackageUMID, BodySID);

	Ret->SetParent(this);

	// Add this package to our "owned" packages
	Packages.push_back(Ret);

	return Ret;
}


//! Get a pointer to the primary package
PackagePtr Metadata::GetPrimaryPackage(void)
{
	MDObjectPtr PrimaryPackage;

	MDObjectPtr PackageRef = Child(PrimaryPackage_UL);
	if(PackageRef)
	{
		PrimaryPackage = PackageRef->GetLink();
	}
	else
	{
		MDObjectPtr Packages = Child(ContentStorageObject_UL);
		if(Packages) Packages = Packages->GetLink();
		if(Packages) Packages = Packages[Packages_UL];
		if(!Packages)
		{
			error("Could not locate a ContentStorage/Packages in the header metadata!\n");
			return NULL;
		}

		// Look for the (first) material package
		MDObject::iterator it = Packages->begin();
		while(it != Packages->end())
		{
			MDObjectPtr ThisPackage = (*it).second->GetLink();
			if(ThisPackage && (ThisPackage->IsA(MaterialPackage_UL)))
			{
				PrimaryPackage = ThisPackage;
				break;
			}
			it++;
		}
	}

	// Couldn't locate the primary package!
	if(!PrimaryPackage)	return NULL;

	// Get the containing Package object
	return Package::GetPackage(PrimaryPackage);
}


//! Make a link to a specified track
/*! \return true if the link was made, else false */
bool SourceClip::MakeLink(TrackPtr SourceTrack, Int64 StartPosition /*=0*/)
{
	if(!SourceTrack) return false;

	SetInt64(StartPosition_UL, StartPosition);
	SetUInt(SourceTrackID_UL, SourceTrack->GetInt(TrackID_UL));
	SetValue(SourcePackageID_UL,SourceTrack->GetParent()[PackageUID_UL]);

	return true;
}


//! Make a link to a UMID and TrackID
bool SourceClip::MakeLink(UMIDPtr LinkUMID, UInt32 LinkTrackID, Int64 StartPosition /*=0*/)
{
	SetInt64(StartPosition_UL, StartPosition);
	SetUInt(SourceTrackID_UL, LinkTrackID);
	SetValue(SourcePackageID_UL, DataChunk(32, LinkUMID->GetValue()));

	return true;
}




//! Set the duration for this Component and update the track's sequence duration
/*! \param Duration The duration of this Component, -1 or omitted for unknown */
void Component::SetDuration(Int64 Duration /*=-1*/)
{
	if(Duration < 0)
		SetDValue(ComponentLength_UL);
	else
		SetInt64(ComponentLength_UL, Duration);

	// Update the duration in the sequence
	if(Duration < 0) 
	{
		MDObjectPtr Sequence = Parent[TrackSegment_UL]->GetLink();
		if(Sequence) Sequence->SetDValue(ComponentLength_UL);
	}
	else
	{
		Parent->UpdateDuration();
	}
}


//! Add an entry into the essence container data set for a given essence stream
bool Metadata::AddEssenceContainerData(UMIDPtr TheUMID, UInt32 BodySID, UInt32 IndexSID /*=0*/)
{
	MDObjectPtr EssenceContainerData = new MDObject(EssenceContainerData_UL);
	mxflib_assert(EssenceContainerData);

	EssenceContainerData->SetValue(LinkedPackageUID_UL, DataChunk(TheUMID.GetPtr()));
	EssenceContainerData->SetUInt(BodySID_UL, BodySID);
	if(IndexSID) EssenceContainerData->SetUInt(IndexSID_UL, IndexSID);

	MDObjectPtr Content = Object[ContentStorageObject_UL];
	if(Content) Content = Content->GetLink();

	if(!Content) return false;

	MDObjectPtr Ptr = Content[EssenceDataObjects_UL];
	if(!Ptr) return false;

	Ptr->AddChild()->MakeRef(EssenceContainerData);

	return true;
}


//! Update the Generation UID of all modified sets and add the specified Ident set
/*! \return true if one or more sets updated, false if none updated (and hence Ident not added)
	\note The preface does not get modified simply to add the new identification set
	\note The identification set added to the file is a <b>copy</b> of Ident
	\note If the identification set has no ModificationDate property it is set 
	      to UpdateTime (if specified) else the packages ModificationTime
*/
bool Metadata::UpdateGenerations(MDObjectPtr Ident, std::string UpdateTime /*=""*/)
{
	// No modified sets found yet
	bool Mod = false;

	// GenerationUID for this update
	UUIDPtr ThisGeneration;
	ThisGeneration = new UUID;

	MDObjectPtr Identifications = Object[Identifications_UL];
	if(Identifications->empty())
	{
		// Clear all modified flags to prevent unwanted GenerationUID properties first time
		ClearModified_Internal(Object);
	}
	else
	{
		// Update the GenerationUID in the preface
		Object->SetGenerationUID(ThisGeneration);

		MDObjectULList::iterator it = Object->begin();
		while(it != Object->end())
		{
			if(!((*it).second->empty()))
			{
				Mod = UpdateGenerations_Internal((*it).second, ThisGeneration) || Mod;
			}
			else
			{
				MDObjectPtr Link = (*it).second->GetLink();
				if(Link)
				{
					if((*it).second->GetRefType() == DICT_REF_STRONG)
					{
						Mod = UpdateGenerations_Internal(Link, ThisGeneration) || Mod;
					}
				}
			}
			it++;
		}

		// If no sub-sets are modified...
		if(!Mod)
		{
			// And we are not modified... then do nothing
			if(!IsModified()) return false;
		}

	}

	// Update dates and add the new identification set
	MDObjectPtr NewIdent = Ident->MakeCopy();

	if(UpdateTime.length()) 
	{
		if(!NewIdent[ModificationDate_UL]) NewIdent->SetString(ModificationDate_UL, UpdateTime);
		ModificationTime = UpdateTime;
	}
	else
	{
		if(!NewIdent[ModificationDate_UL]) NewIdent->SetString(ModificationDate_UL, ModificationTime);
	}

	Object->SetString(LastModifiedDate_UL, ModificationTime);
	Identifications->AddChild()->MakeRef(NewIdent);
	NewIdent->SetValue(ThisGenerationUID_UL, DataChunk(16, ThisGeneration->GetValue()));

	// It's just too confusing to record Identification as being modified!
	NewIdent->ClearModified();

	// Clear the modified flag for the preface
	Object->ClearModified();

	return true;
}

//! Update the Generation UID of a set if modified - then iterate through strongly linked sets
/*! \return true if any of the sets have been modified, else false
	\note true is returned even if a set has been modified but doesn't support GenerationUID
		  this will force an Identification set to be added to show something was updated
*/
bool Metadata::UpdateGenerations_Internal(MDObjectPtr Obj, UUIDPtr ThisGeneration)
{
	bool Mod = Obj->IsModified();
	
	if(Mod)
	{
		Obj->SetGenerationUID(ThisGeneration);
	}

	MDObjectULList::iterator it = Obj->begin();
	while(it != Obj->end())
	{
		if(!((*it).second->empty()))
		{
			Mod = UpdateGenerations_Internal((*it).second, ThisGeneration) || Mod;
		}
		else
		{
			MDObjectPtr Link = (*it).second->GetLink();
			if(Link)
			{
				if((*it).second->GetRefType() == DICT_REF_STRONG) 
				{
					Mod = UpdateGenerations_Internal(Link, ThisGeneration) || Mod;
				}
			}
			else Mod = Mod || (*it).second->IsModified();
		}
		it++;
	}

	// Now we have updated the GenerationUID clear the modified flags
	Obj->ClearModified();

	return Mod;
}


//! Clear all modified flags for this set and strongly linked sets - used when adding initial Identification set
void Metadata::ClearModified_Internal(MDObjectPtr Obj)
{
	MDObjectULList::iterator it = Obj->begin();
	while(it != Obj->end())
	{
		if(!((*it).second->empty()))
		{
			ClearModified_Internal((*it).second);
		}
		else
		{
			MDObjectPtr Link = (*it).second->GetLink();
			if(Link)
			{
				if((*it).second->GetRefType() == DICT_REF_STRONG) ClearModified_Internal(Link);
			}
		}

		it++;
	}

	Obj->ClearModified();
}



//! Add a SourceClip to a track
/*! \param Duration The duration of this SourceClip, -1 or omitted for unknown */
SourceClipPtr Track::AddSourceClip(Int64 Duration /*=-1*/)
{
	// DRAGONS: If the track is a DM track should we add a DM SourceClip?
	SourceClipPtr Ret = new SourceClip(SourceClip_UL);
	if(!Ret) return Ret;

	// Set the duration
	if(Duration < 0)
		Ret->SetDValue(ComponentLength_UL);
	else
		Ret->SetInt64(ComponentLength_UL, Duration);
	
	// Add zero package and track IDs
	Ret->AddChild(SourcePackageID_UL);
	Ret->AddChild(SourceTrackID_UL);

	// Initially assume the SourceClip starts at the start of the referenced essence
	Ret->AddChild(StartPosition_UL, 0);

		// Add this SourceClip to the sequence for this track
		MDObjectPtr Sequence = Child(TrackSegment_UL)->GetLink();
		Sequence[StructuralComponents_UL]->AddChild()->MakeRef(Ret->Object);
	
		// Copy the data definition from the sequence
		Ret->AddChild(ComponentDataDefinition_UL)->SetValue(Sequence[ComponentDataDefinition_UL]->PutData());
	
		// Add this sequence to the list of "owned" components
		Components.push_back(SmartPtr_Cast(Ret, Component));
	//	Components.push_back(Ret);
	
		// Record the track as the parent of the new SourceClip
		Ret->SetParent(this);
	
		// Update the duration in the sequence
		if(Duration < 0) 
		{
			Sequence->SetDValue(ComponentLength_UL);
		}
		else
		{
			UpdateDuration();
		}

	return Ret;
}


//! Add a Timecode Component to a track
/*! \param FPS The rounded integer timebase of the track in frames per second
 *	\param DropFrame = true if dropframe is to be use with this timecode
 *	\param Start The starting timecode converted to an integer frame count since 00:00:00:00
 *	\param Duration The duration of this SourceClip, -1 or omitted for unknown
 */
TimecodeComponentPtr Track::AddTimecodeComponent(UInt16 FPS, bool DropFrame, Int64 Start /*=0*/, Int64 Duration /*=-1*/)
{
	// DRAGONS: If the track is a DM track should we add a DM SourceClip?
	TimecodeComponentPtr Ret = new TimecodeComponent(TimecodeComponent_UL);
	if(!Ret) return Ret;

	//! Set the framerate
	Ret->SetUInt(RoundedTimecodeBase_UL, FPS);
	if(DropFrame) Ret->SetUInt(DropFrame_UL, 1); else Ret->SetUInt(DropFrame_UL, 0);

	//! Set the initial timecode
	Ret->SetInt64(StartTimecode_UL, Start);

	// Set the duration
	if(Duration < 0)
		Ret->SetDValue(ComponentLength_UL);
	else
		Ret->SetInt64(ComponentLength_UL, Duration);

		// Add this Timecode Component to the sequence for this track
		MDObjectPtr Sequence = Child(TrackSegment_UL)->GetLink();
		MDObjectPtr S1=Sequence[StructuralComponents_UL];
		MDObjectPtr C1=S1->AddChild();
		C1->MakeRef(Ret->Object);

		// Copy the data definition from the sequence
		Ret->AddChild(ComponentDataDefinition_UL)->SetValue(Sequence[ComponentDataDefinition_UL]->PutData());

		// Record the track as the parent of the new Timecode Component
		Ret->SetParent(this);

		// Update the duration in the sequence
		if(Duration < 0) 
		{
			Sequence->SetDValue(ComponentLength_UL);
		}
		else
		{
			UpdateDuration();
		}

	return Ret;
}


//! Add a DMSegment to a track
/*! \param EventStart The start position of this Segemnt, -1 or omitted for static or timeline
 *  \param Duration The duration of this SourceClip, -1 or omitted for static 
 */
DMSegmentPtr Track::AddDMSegment(Int64 EventStart /*=-1*/,Int64 Duration /*=-1*/)
{
	DMSegmentPtr Ret = new DMSegment(DMSegment_UL);
	if(!Ret) return Ret;

	// Set the duration - or not if there is none
	if(Duration >= 0)
		Ret->SetInt64(ComponentLength_UL, Duration);
	
	// Add zero linked track IDs and DMFramework
	Ret->AddChild(TrackIDs_UL);
	Ret->AddChild(DMFramework_UL);

	// Initially assume the SourceClip starts at the start of the referenced essence
	if( EventStart >= 0 )
		Ret->AddChild(EventStartPosition_UL, 0);

	// Add this SourceClip to the sequence for this track
	MDObjectPtr Sequence = Child(TrackSegment_UL)->GetLink();
	Sequence[StructuralComponents_UL]->AddChild()->MakeRef(Ret->Object);

	// Copy the data definition from the sequence
	Ret->AddChild(ComponentDataDefinition_UL)->SetValue(Sequence[ComponentDataDefinition_UL]->PutData());

	// Record the track as the parent of the new DMSegment
	Ret->SetParent(this);

	// Update the duration in the sequence
	if(Duration >= 0) 
	{
		UpdateDuration();
	}

	return Ret;
}

//! Update the duration field in the sequence for this track based on component durations
/*! \return The duration, or -1 if unknown */
Int64 Track::UpdateDuration(void)
{
	MDObjectPtr Sequence = Child(TrackSegment_UL)->GetLink();
	Int64 SeqDuration = 0;
	MDObjectPtr Structs = Sequence[StructuralComponents_UL];

	// if the Sequence is not a valid sequence, exit now
	if( !Structs ) return -1;

	MDObjectULList::iterator it = Structs->begin();
	while(it != Structs->end())
	{
		MDObjectPtr Link = (*it).second->GetLink();

		// Broken link!
		if(!Link)
		{
			SeqDuration = -1;
			break;
		}

		// If any component is unknown the sum is unknown
		if(Link->IsDValue(ComponentLength_UL))
		{
			SeqDuration = -1;
			break;
		}
		SeqDuration += Link->GetInt64(ComponentLength_UL);
		it++;
	}

	if(SeqDuration < 0)
		Sequence->SetDValue(ComponentLength_UL);
	else
		Sequence->SetInt64(ComponentLength_UL, SeqDuration);

	return SeqDuration;
}

namespace
{
	//! Locate the DataDef with a given Identification
	MDObjectPtr DictionaryLocate(const MDObjectPtr &Dictionary, const ULPtr &Identification)
	{
		MDObjectPtr DataDefs = Dictionary->Child(DataDefinitions_UL);
		if(DataDefs)
		{
			MDObject::const_iterator it = DataDefs->begin();
			while(it != DataDefs->end())
			{
				MDObjectPtr ThisDef = (*it).second->GetRef();
				MDObjectPtr Ident = ThisDef ? ThisDef->Child(DefinitionObjectIdentification_UL) : NULL;
				if(Ident)
				{
					DataChunkPtr IDValue = Ident->PutData();
					if(IDValue && (IDValue->Size == 16))
					{
						if(memcmp(IDValue->Data, Identification->GetValue(), 16) == 0) return ThisDef;
					}
				}

				it++;
			}
		}

		return NULL;
	}
}


//! Add a timeline track to the package
/*! \note If the TrackID is set manually it is the responsibility of the caller to prevent clashes */
TrackPtr Package::AddTrack(ULPtr DataDef, UInt32 TrackNumber, Rational EditRate, std::string TrackName /*=""*/, UInt32 TrackID /*=0*/)
{
	mxflib_assert( DataDef);

	// Smart pointer to the dictionary definition to make the target of this dict ref (or NULL for 337-1 style DataDef)
	MDObjectPtr DictRef = NULL;

	/* Check if this file uses a dictionary for track definitions */
	MetadataParent const &Meta = GetParent();
	if(Meta)
	{
		MDObjectPtr Dictionary = Meta->GetRef(Dictionaries_UL);
		if(Dictionary) DictRef = DictionaryLocate(Dictionary, DataDef);
	}

	TrackPtr Ret = new Track(Track_UL);
	if(!Ret) return Ret;

	if(TrackName.length()) Ret->SetString(TrackName_UL, TrackName);
	Ret->SetInt(TrackNumber_UL, TrackNumber);
	Ret->SetInt64(Origin_UL, 0);

	MDObjectPtr Ptr = Ret->AddChild(EditRate_UL);
	if(Ptr)
	{
		Ptr->SetInt("Numerator", EditRate.Numerator);
		Ptr->SetInt("Denominator", EditRate.Denominator);
	}

	// Auto set the track ID if not supplied
	if(TrackID == 0)
	{
		mxflib_assert(LastTrackID < 0xffffffff);

		LastTrackID++;
		TrackID = LastTrackID;
	}
	else
	{
		// save manually set TrackID
		LastTrackID = TrackID;
	}
	Ret->SetInt(TrackID_UL, TrackID);


	{
		// Build a new sequence for this track
		MDObjectPtr Sequence = new MDObject(Sequence_UL);
		mxflib_assert(Sequence);

		/* Initialise the sequence */
		if(DictRef)
			Sequence->MakeRef(ComponentDataDefinition_UL, DictRef);
		else
			Sequence->AddChild(ComponentDataDefinition_UL)->SetValue(DataDef->GetValue(), 16);

		Sequence->SetDValue(ComponentLength_UL);
		Sequence->AddChild(StructuralComponents_UL);

		// Add the sequence
		Ret->AddChild(TrackSegment_UL)->MakeRef(Sequence);
	}


	// Add this track to the package
	AddRef(Tracks_UL, Ret->Object);

	// Add this track to our "owned" tracks
	Tracks.push_back(Ret);

	// Record this package as the parent of the new track
	Ret->SetParent(this);

	return Ret;
}


//! Update the duration field in each sequence in each track for this package
void Package::UpdateDurations(void)
{
	MDObjectPtr Tracks = Child(Tracks_UL);
	if(!Tracks) return;

	MDObjectULList::iterator it = Tracks->begin();
	while(it != Tracks->end())
	{
		MDObjectPtr ThisLink = (*it).second->GetLink();
		if(ThisLink)
		{
			TrackPtr ThisTrack = Track::Parse(ThisLink);
			if(ThisTrack) ThisTrack->UpdateDuration();
		}
		it++;
	}
}




//! Add an event track to the package
/*! \note If the TrackID is set manually it is the responsibility of the caller to prevent clashes */
TrackPtr Package::AddTrack(ULPtr DataDef, UInt32 TrackNumber, Rational EditRate, Length DefaultDuration, std::string TrackName /* = "" */ , UInt32 TrackID /* = 0 */)
{
	TrackPtr Ret = new Track(EventTrack_UL);
	if(!Ret) return Ret;

	if(TrackName.length()) Ret->SetString(TrackName_UL, TrackName);
	Ret->SetInt(TrackNumber_UL, TrackNumber);
	Ret->SetInt64(EventOrigin_UL, 0);

	MDObjectPtr Ptr = Ret->AddChild(EventEditRate_UL);
	if(Ptr)
	{
		Ptr->SetInt("Numerator", EditRate.Numerator);
		Ptr->SetInt("Denominator", EditRate.Denominator);
	}

	// Auto set the track ID if not supplied
	if(TrackID == 0)
	{
		mxflib_assert(LastTrackID < 0xffffffff);

		LastTrackID++;
		TrackID = LastTrackID;
	}
	Ret->SetInt(TrackID_UL, TrackID);

	// Build a new sequence for this track
	MDObjectPtr Sequence = new MDObject(Sequence_UL);
	mxflib_assert(Sequence);

	// Initialise the sequence
	Sequence->AddChild(ComponentDataDefinition_UL)->SetValue(DataDef->GetValue(), 16);

	// Pass DefaultDuration on to the Sequence
	if( DefaultDuration == DurationUnspecified )
		Sequence->SetDValue(ComponentLength_UL);
	else
		Sequence->SetInt64(ComponentLength_UL, DefaultDuration);


	Sequence->AddChild(StructuralComponents_UL);

	// Add the sequence
	Ret->AddChild(TrackSegment_UL)->MakeRef(Sequence);

	// Add this track to the package
	Child(Tracks_UL)->AddChild()->MakeRef(Ret->Object);

	// Add this track to our "owned" tracks
	Tracks.push_back(Ret);

	// Record this package as the parent of the new track
	Ret->SetParent(this);

	return Ret;
}

//! Add a static track to the package
/*! \note If the TrackID is set manually it is the responsibility of the caller to prevent clashes */
TrackPtr Package::AddTrack(ULPtr DataDef, UInt32 TrackNumber, std::string TrackName /*=""*/, UInt32 TrackID /*=0*/)
{
	TrackPtr Ret = new Track(StaticTrack_UL);
	if(!Ret) return Ret;

	if(TrackName.length()) Ret->SetString(TrackName_UL, TrackName);
	Ret->SetInt(TrackNumber_UL, TrackNumber);

	// Auto set the track ID if not supplied
	if(TrackID == 0)
	{
		mxflib_assert(LastTrackID < 0xffffffff);

		LastTrackID++;
		TrackID = LastTrackID;
	}
	Ret->SetInt(TrackID_UL, TrackID);

	// Build a new sequence for this track
	MDObjectPtr Sequence = new MDObject(Sequence_UL);
	mxflib_assert(Sequence);

	// Initialise the sequence
	Sequence->AddChild(ComponentDataDefinition_UL)->SetValue(DataDef->GetValue(), 16);
	Sequence->AddChild(StructuralComponents_UL);

	// Add the sequence
	Ret->AddChild(TrackSegment_UL)->MakeRef(Sequence);

	// Add this track to the package
	Child(Tracks_UL)->AddChild()->MakeRef(Ret->Object);

	// Add this track to our "owned" tracks
	Tracks.push_back(Ret);

	// Record this package as the parent of the new track
	Ret->SetParent(this);

	return Ret;
}


//! Remove a track from this package
void Package::RemoveTrack(TrackPtr &Track)
{
	TrackList::iterator it = Tracks.begin();
	while(it != Tracks.end())
	{
		if((*it) == Track)
		{
			// Add this track to the package
			MDObjectPtr TrackList = Child(Tracks_UL);

			// Remove the track from the list of tracks in this package
			if(TrackList)
			{
				MDObject::iterator Track_it = TrackList->begin();
				while(Track_it != TrackList->end())
				{
					if((*Track_it).second->GetLink() == (*it)->Object)
					{
						TrackList->RemoveChild((*Track_it).second);
						break;
					}
					Track_it++;
				}
			}
			Tracks.erase(it);
			break;
		}
		it++;
	}
}



//! Return the containing "SourceClip" object for this MDObject
/*! \return NULL if MDObject is not contained in a SourceClip object
 */
SourceClipPtr SourceClip::GetSourceClip(MDObjectPtr Object)
{
	return Object->GetOuter() ? SourceClipPtr(dynamic_cast<SourceClip*>(Object->GetOuter())) : NULL;
}



//! Return the containing "DMSourceClip" object for this MDObject
/*! \return NULL if MDObject is not contained in a DMSourceClip object
 */
DMSourceClipPtr DMSourceClip::GetDMSourceClip(MDObjectPtr Object)
{
	return Object->GetOuter() ? DMSourceClipPtr(dynamic_cast<DMSourceClip*>(Object->GetOuter())) : NULL;
}


//! Return the containing "TimecodeComponent" object for this MDObject
/*! \return NULL if MDObject is not contained in a TimecodeComponent object
 */
TimecodeComponentPtr TimecodeComponent::GetTimecodeComponent(MDObjectPtr Object)
{
	return Object->GetOuter() ? TimecodeComponentPtr(dynamic_cast<TimecodeComponent*>(Object->GetOuter())) : NULL;
}


//! Return the containing "DMSegment" object for this MDObject
/*! \return NULL if MDObject is not contained in a DMSegment object
 */
DMSegmentPtr DMSegment::GetDMSegment(MDObjectPtr Object)
{
	return Object->GetOuter() ? DMSegmentPtr(dynamic_cast<DMSegment*>(Object->GetOuter())) : NULL;
}


//! Make a link to a given DMFramework
bool DMSegment::MakeLink(MDObjectPtr DMFramework)
{
	MDObjectPtr SourceFramework=Child(DMFramework_UL);

	if(!SourceFramework)
	{
		SourceFramework = AddChild(DMFramework_UL);
		// If this failed then exit with an error
		if(!SourceFramework)
		{
			error("Attempt to reference %s from %s failed\n", FullName().c_str(), DMFramework->FullName().c_str());
			return false;
		}
	}

	return SourceFramework->MakeLink(DMFramework);
}


//! Return the containing "Track" object for this MDObject
/*! \return NULL if MDObject is not contained in a Track object
 */
TrackPtr Track::GetTrack(MDObjectPtr Object)
{
	return Object->GetOuter() ? TrackPtr(dynamic_cast<Track*>(Object->GetOuter())) : NULL;
}


//! Return the containing "Package" object for this MDObject
/*! \return NULL if MDObject is not contained in a Package object
 */
PackagePtr Package::GetPackage(MDObjectPtr Object)
{
	return Object->GetOuter() ? PackagePtr(dynamic_cast<Package*>(Object->GetOuter())) : NULL;
}

//! Return the containing "Metadata" object for this MDObject
/*! \return NULL if MDObject is not contained in a Metadata object
 */
MetadataPtr Metadata::GetMetadata(MDObjectPtr Object)
{
	return Object->GetOuter() ? MetadataPtr(dynamic_cast<Metadata*>(Object->GetOuter())) : NULL;
}


//! Parse an existing MDObject into a Metadata object
MetadataPtr Metadata::Parse(MDObjectPtr BaseObject)
{
	MetadataPtr Ret;

	// We can only build a Metadata object from a Preface
	if(!BaseObject->IsA(Preface_UL)) return Ret;

	// If this is already part of a Metadata object then return that object
	if(BaseObject->GetOuter()) return Metadata::GetMetadata(BaseObject);

	// Build the basic Metadata object
	Ret = new Metadata(BaseObject);

	// Set the most recent modification time to now
	// Not the value from the MDObject as anything we now do is a new modification
	Ret->ModificationTime = Now2String();

	// Locate the content storage set
	MDObjectPtr ContentStorage = BaseObject[ContentStorageObject_UL];
	if(ContentStorage) ContentStorage = ContentStorage->GetLink();

	// Can't go any further if there is no content storage set!
	// DRAGONS: Should this cause an error to be reported?
	if(!ContentStorage) return Ret;

	// Get the list of Packages
	MDObjectPtr PackageList = ContentStorage[Packages_UL];

	// Can't go any further if there is no package list in the content storage set!
	// DRAGONS: Should this cause an error to be reported?
	if(!PackageList) return Ret;

	// Search for packages and parse them
	MDObject::iterator it = PackageList->begin();
	while(it != PackageList->end())
	{
		// Follow the link
		MDObjectPtr LinkedPackage = (*it).second->GetLink();
		
		if(LinkedPackage)
		{
			// Parse this package
			PackagePtr ThisPackage = Package::Parse(LinkedPackage);

			// Add it to the list of packages for this metadata
			if(ThisPackage) 
			{
				// Set the package's parent pointer
				ThisPackage->SetParent(Ret);

				Ret->Packages.push_back(ThisPackage);
			}
		}

		it++;
	}

	return Ret;
}


//! Parse an existing MDObject into a Package object
PackagePtr Package::Parse(MDObjectPtr BaseObject)
{
	PackagePtr Ret;

	// We can only build a Package object from a GenericPackage derived set
	if(!BaseObject->IsA(GenericPackage_UL)) return Ret;

	// If this is already part of a Package object then return that object
	if(BaseObject->GetOuter()) return Package::GetPackage(BaseObject);

	// Build the basic Package object
	Ret = new Package(BaseObject);

	// Clear the LastTrackID - we will search for the highest value in the parsed tracks
	Ret->LastTrackID = 0;

	// Get the list of tracks
	MDObjectPtr TrackList = Ret[Tracks_UL];

	// Can't go any further if there is no track list
	// DRAGONS: Should this cause an error to be reported?
	if(!TrackList) return Ret;

	// Search for tracks and parse them
	MDObject::iterator it = TrackList->begin();
	while(it != TrackList->end())
	{
		// Follow the link
		MDObjectPtr LinkedTrack = (*it).second->GetLink();
		
		if(LinkedTrack)
		{
			// Parse this track
			TrackPtr ThisTrack = Track::Parse(LinkedTrack);

			if(ThisTrack)
			{
				// Set the track's parent pointer
				ThisTrack->SetParent(Ret);

				// Get the ID of this track and update LastTrackID if required
				UInt32 ThisID = ThisTrack->GetUInt(TrackID_UL);
				if(ThisID > Ret->LastTrackID) Ret->LastTrackID = ThisID;

				// Add it to the list of tracks for this package
				Ret->Tracks.push_back(ThisTrack);
			}
		}

		it++;
	}

	return Ret;
}


//! Parse an existing MDObject into a Track object
TrackPtr Track::Parse(MDObjectPtr BaseObject)
{
	TrackPtr Ret;

	// We can only build a Track object from a GenericTrack derived set
	if(!BaseObject->IsA(GenericTrack_UL)) return Ret;

	// If this is already part of a Track object then return that object
	if(BaseObject->GetOuter()) return Track::GetTrack(BaseObject);

	// Build the basic Track object
	Ret = new Track(BaseObject);

	// Get the sequence
	MDObjectPtr Sequence = Ret[TrackSegment_UL];
	if(Sequence) Sequence = Sequence->GetLink();

	// Can't go any further if there is no sequence
	// DRAGONS: Should this cause an error to be reported?
	if(!Sequence) return Ret;

	// Get the list of components
	MDObjectPtr ComponentList = Sequence[StructuralComponents_UL];

	// Can't go any further if there is no component list
	// DRAGONS: Should this cause an error to be reported?
	if(!ComponentList) return Ret;

	// Search for components and parse them
	MDObject::iterator it = ComponentList->begin();
	while(it != ComponentList->end())
	{
		// Follow the link
		MDObjectPtr LinkedComponent = (*it).second->GetLink();
		
		if(LinkedComponent)
		{
			ComponentPtr ThisComponent;

			// Parse all the known component types
			if(LinkedComponent->IsA(SourceClip_UL)) ThisComponent = SourceClip::Parse(LinkedComponent);


			else if(LinkedComponent->IsA(TimecodeComponent_UL)) ThisComponent = TimecodeComponent::Parse(LinkedComponent);
			else if(LinkedComponent->IsA(DMSegment_UL)) ThisComponent = DMSegment::Parse(LinkedComponent);

			if(ThisComponent)
			{
				// Set the component's parent pointer
				ThisComponent->SetParent(Ret);

				// Add it to the list of components for this track
				Ret->Components.push_back(ThisComponent);
			}
		}

		it++;
	}

	return Ret;
}


//! Parse an existing MDObject into a SourceClip object
SourceClipPtr SourceClip::Parse(MDObjectPtr BaseObject)
{
	SourceClipPtr Ret;

	// We can only build a SourceClip object from a SourceClip
	if(!BaseObject->IsA(SourceClip_UL)) return Ret;

	// If this is already part of a SourceClip object then return that object
	if(BaseObject->GetOuter()) return SourceClip::GetSourceClip(BaseObject);

	// Build the basic SourceClip object
	Ret = new SourceClip(BaseObject);

	return Ret;
}




//! Parse an existing MDObject into a DMSourceClip object
DMSourceClipPtr DMSourceClip::Parse(MDObjectPtr BaseObject)
{
	DMSourceClipPtr Ret;

	// We can only build a DMSourceClip object from a DMSourceClip
	if(!BaseObject->IsA(DMSourceClip_UL)) return Ret;

	// If this is already part of a DMSourceClip object then return that object
	if(BaseObject->GetOuter()) return DMSourceClip::GetDMSourceClip(BaseObject);

	// Build the basic DMSourceClip object
	Ret = new DMSourceClip(BaseObject);

	return Ret;
}


//! Parse an existing MDObject into a TimecodeComponent object
TimecodeComponentPtr TimecodeComponent::Parse(MDObjectPtr BaseObject)
{
	TimecodeComponentPtr Ret;

	// We can only build a TimecodeComponent object from a TimecodeComponent
	if(!BaseObject->IsA(TimecodeComponent_UL)) return Ret;

	// If this is already part of a TimecodeComponent object then return that object
	if(BaseObject->GetOuter()) return TimecodeComponent::GetTimecodeComponent(BaseObject);

	// Build the basic TimecodeComponent object
	Ret = new TimecodeComponent(BaseObject);

	return Ret;
}


//! Parse an existing MDObject into a DMSegment object
DMSegmentPtr DMSegment::Parse(MDObjectPtr BaseObject)
{
	DMSegmentPtr Ret;

	// We can only build a DMSegment object from a DMSegment
	if(!BaseObject->IsA(DMSegment_UL)) return Ret;

	// If this is already part of a DMSegment object then return that object
	if(BaseObject->GetOuter()) return DMSegment::GetDMSegment(BaseObject);

	// Build the basic DMSegment object
	Ret = new DMSegment(BaseObject);

	return Ret;
}


//! Determine the type of this track
Track::TrackType Track::GetTrackType(void)
{
	/* Find the data def in the sequence */

	MDObjectPtr Sequence = Child(TrackSegment_UL);
	if(Sequence) Sequence = Sequence->GetLink();
	
	MDObjectPtr DataDef;
	if(Sequence) DataDef = Sequence[ComponentDataDefinition_UL];

	// Check for dictionary reference
	MDObjectPtr DataDefLink;
	if(DataDef)	DataDefLink=DataDef->GetRef();

	// Take the actual data def value from the dictionary entry
	if(DataDefLink)	DataDef=DataDefLink[DefinitionObjectIdentification_UL];

	// If we dont seem to have one return the last known value rather than unknown (it may still end up as undetermined)
	if(!DataDef) return ThisTrackType;

	// If we have already determined the type and it has not changed leave it as it is
	if((ThisTrackType != TrackTypeUndetermined) && (!DataDef->IsModified()))
	{
		return ThisTrackType;
	}

	// Get the actual data definition bytes
	DataChunkPtr Data = DataDef->PutData();

	// Sanity check the result
	if(!Data || Data->Size == 16)
	{
		// Initialise the track type list if required
		if(!TrackTypesInited) InitTrackTypes();

		// Check all known types
		ThisTrackType = Track::GetTrackType( UL(Data->Data) );
	}

	// If the type is still unknowm, and it is in the dictionary, try parsing the text (both name and description)
	if((ThisTrackType == TrackTypeUndetermined) && DataDefLink)
	{
		std::string DataDefText = DataDefLink->GetString(DefinitionObjectName_UL) + " " + DataDefLink->GetString(DefinitionObjectDescription_UL);
		ThisTrackType = Track::ParseTrackTypeText(DataDefText);
	}

	return ThisTrackType;
}



//! Get the single word description for the type of this track
std::string Track::GetTrackWord(void)
{
	// Try and determine the track type if not yet known
	if(ThisTrackType == TrackTypeUndetermined) GetTrackType();

	// Initialise the track type list if required
	if(!TrackTypesInited) InitTrackTypes();

	TrackTypeMap::iterator it = TrackTypes.begin();
	while(it != TrackTypes.end())
	{
		if((*it).second.Type == ThisTrackType) return (*it).second.Word;
		it++;
	}

	return "Undetermined";
}


//! Determine the type of this track
/*! \param Label The UL that identifies this type of track
 */
Track::TrackType Track::GetTrackType( const UL Label )
{
	// Initialise the track type list if required
	if(!TrackTypesInited) InitTrackTypes();

	TrackTypeMap::iterator it = TrackTypes.find( Label );

	if( it != TrackTypes.end() ) return (*it).second.Type;
	else return TrackTypeUndetermined;
}

//! Determine the type of this track by Name or Word
/*! \param Text The text Name or Word that identifies this type of track
 */
Track::TrackType Track::GetTrackType( const char* Text )
{
	if( !Text ) return TrackTypeUndetermined;

	// Initialise the track type list if required
	if(!TrackTypesInited) InitTrackTypes();

	// linear search of TrackTypes
	TrackTypeMap::iterator it = TrackTypes.begin();

	while( it != TrackTypes.end() )
	{
		if( (*it).second.Word == Text ) return (*it).second.Type;
		it++;
	}

	// if Word seacrh failed, translate Text into UL by search of the LabelMap
	LabelPtr Ret = Label::Find( Text );

	// then search for that UL
	if( Ret ) return Track::GetTrackType( Ret->GetValue() );

	// abject failure
	return TrackTypeUndetermined;
}




//! Add a new track type definition label
/*! \param Type The type of track that this new definition identifies
 *  \param Label The label to compare with the data definition
 *  \param Word The single word abbreviated name to use for non-propeller-heads
 */
void Track::AddTrackType(TrackType Type, const UL Label, const char* Word)
{ 
	TrackTypeMapItem Item;
	Item.Type = Type;
	Item.Word = Word;

	TrackTypeMapItemPair ItemPair;
	ItemPair.first = Label;
	ItemPair.second = Item;

	TrackTypes.insert(ItemPair);
}



//! Initialise the TrackTypes list with known track types
void mxflib::Track::InitTrackTypes(void)
{
	// Don't initialize twice
	if(TrackTypesInited) return;

	AddTrackType(TrackTypeTimecode, SMPTE12MTimecodeTrack_UL,				"Timecode"		);
	AddTrackType(TrackTypeTimecode, SMPTE12MTimecodeActiveUserBitsTrack_UL,	"Timecode"		);
	AddTrackType(TrackTypeTimecode, SMPTE309MTimecodeTrack_UL,				"Timecode"		);
	AddTrackType(TrackTypePictureEssence, PictureEssenceTrack_UL,			"Picture"		);
	AddTrackType(TrackTypeSoundEssence, SoundEssenceTrack_UL,				"Sound"			);
	AddTrackType(TrackTypeDataEssence, DataEssenceTrack_UL,					"DataEssence"	);
	AddTrackType(TrackTypeDescriptiveMetadata, DescriptiveMetadataTrack_UL,	"Metadata"		);

	// add other TrackTypes as the need arises. also add to Track::TrackType in metadata.h


	TrackTypesInited = true;
}

//! Determine the one-word Track name from the Track Type
std::string mxflib::Track::GetTrackWord( const TrackType Trk )
{
	// Initialise the track type list if required
	if(!TrackTypesInited) InitTrackTypes();

	// linear search of TrackTypes
	TrackTypeMap::iterator it = TrackTypes.begin();

	while( it != TrackTypes.end() )
	{
		// succeeds if Text contains the Word
		if( (*it).second.Type == Trk ) return (*it).second.Word;
		it++;
	}

	return "Unknown";
}


//! Compare the hex digits of two strings disregarding punctuation and whitespace
// return 0 if they are equal
int strxcmp( const char *s1, const char *s2 )
{
	int c1=0;
	int c2=0;

	do
	{
		// skip non-hex
		while( s1 && (c1=tolower(*s1++)) && !isxdigit(c1) );
		while( s2 && (c2=tolower(*s2++)) && !isxdigit(c2) );

		if( c2 && c1<c2 ) return -1;
		if( c1 && c2<c1 ) return 1;
	}
	while( c1 || c2 );

	return 0;
}


//! Parse the text of a track description and try and determine the track type
Track::TrackType mxflib::Track::ParseTrackTypeText(std::string Text)
{
	//! Type for each entry in TrackWordList
	struct TrackWordType
	{
		char *Word;						//!< A descriptive word
		Track::TrackType Type;			//!< The track type this word implies
	};

	//! List of track type word meanings
	const TrackWordType TrackWordList[] =
	{
		{ "TIMECODE",		TrackTypeTimecode },
		{ "PICTURE",		TrackTypePictureEssence },
		{ "VIDEO",			TrackTypePictureEssence },
		{ "SOUND",			TrackTypeSoundEssence },
		{ "AUDIO",			TrackTypeSoundEssence },
		{ "DATA",			TrackTypeDataEssence },
		{ "DESCRIPTIVE",	TrackTypeDescriptiveMetadata },
		{ "METADATA",		TrackTypeDescriptiveMetadata },
		{ "DM",				TrackTypeDescriptiveMetadata },

	};

	//! The count of entries in the track word list
	const int TrackWordCount = (int)(sizeof(TrackWordList) / sizeof(TrackWordList[0]));

	std::string::iterator it = Text.begin();
	while(it != Text.end())
	{
		// Skip any non-letters before this word
		while(!isalpha(*it)) 
		{
			// Quit if we hit the end without reaching a letter
			if(++it == Text.end()) return TrackTypeUndetermined;
		}
	
		std::string Word = "";
		Word += toupper(*(it++));
		while((it != Text.end()) && isalpha(*it))
		{
			Word += toupper(*(it++));
		}

		/* We now have a word */
		for(int i=0; i<TrackWordCount; i++)
		{
			if(Word == TrackWordList[i].Word) return TrackWordList[i].Type;
		}
	}

	// the following comparison is done because all other attempts to resolve an AUID into String may have failed
	// and the only representation that could be delivered is a hexbinary string
	// AND there are many possible formats, including un-reversed UL shown as a UUID in {}
	// so the best test is the string of hex digits, ignoring punctuation and whitespace

	//! Type for each entry in TrackWordList
	struct TrackHexType
	{
		char *Hex;						//!< A descriptive word
		Track::TrackType Type;			//!< The track type this word implies
	};

	//! List of track type hex strings
	const TrackHexType TrackHexList[] =
	{
		{ "{060e2b34-0401-0101-0103-020101000000}",		TrackTypeTimecode },
		{ "{060e2b34-0401-0101-0103-020201000000}",		TrackTypePictureEssence },
		{ "{060e2b34-0401-0101-0103-020202000000}",		TrackTypeSoundEssence },
		{ "{060e2b34-0401-0101-0103-020303000000}",		TrackTypeDataEssence },
		{ "{060e2b34-0401-0101-0103-020210000000}",		TrackTypeDescriptiveMetadata },
		{ "{060e2b34-0401-0101-0103-020301000000}",		TrackTypeAuxiliary },
		{ "{060e2b34-0401-0101-0103-020302000000}",		TrackTypeParsedText },

	};

	//! The count of entries in the track hex list
	const int TrackHexCount = (int)(sizeof(TrackHexList) / sizeof(TrackHexList[0]));

	for(int j=0; j<TrackHexCount; j++)
	{
		if( 0==strxcmp( Text.c_str(), TrackHexList[j].Hex ) ) return TrackHexList[j].Type;
	}

	return TrackTypeUnknown;
}
