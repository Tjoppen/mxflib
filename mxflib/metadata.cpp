/*! \file	metadata.cpp
 *	\brief	Implementation of Metadata class
 *
 *			The Metadata class holds data about a set of Header Metadata.
 *			The class holds a Preface set object
 *
 *	\version $Id: metadata.cpp,v 1.1.2.6 2004/11/09 15:15:41 matt-beard Exp $
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

#include <mxflib/mxflib.h>

using namespace mxflib;

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
	Object = new MDObject("Preface");

	// Even though it isn't used the preface needs an InstanceUID
	// as it is defived from GenerationInterchangeObject
	UUIDPtr ThisInstance = new UUID;
	Object->AddChild("InstanceUID")->ReadValue(DataChunk(16, ThisInstance->GetValue()));

	Object->AddChild("LastModifiedDate")->SetString(ModificationTime);
	Object->AddChild("Version")->SetInt(258);

	Object->AddChild("Identifications");
	// To set later: OperationalPattern
	Object->AddChild("EssenceContainers");
	Object->AddChild("DMSchemes");

	// Add a content storage object
	MDObjectPtr Content = new MDObject("ContentStorage");
	ASSERT(Content);
	Content->AddChild("Packages");
	Content->AddChild("EssenceContainerData");

	Object->AddChild("ContentStorage")->MakeLink(Content);
}

// Add a package of the specified type to the matadata
PackagePtr mxflib::Metadata::AddPackage(std::string PackageType, std::string PackageName, UMIDPtr PackageUMID, Uint32 BodySID /*=0*/)
{
	PackagePtr Ret;

	// If no UMID is supplied generate a general purpose UMID
	if(!PackageUMID) PackageUMID = MakeUMID(4);

	// Build the new package
	Ret = new Package(PackageType);
	if(!Ret) return Ret;

	// Set the package name if one supplied
	if(PackageName.length()) Ret->SetString("Name", PackageName);

	// Set the package's properties
	Ret->AddChild("PackageUID")->ReadValue(PackageUMID->GetValue(), 32);
	Ret->SetString("PackageCreationDate", ModificationTime);
	Ret->SetString("PackageModifiedDate", ModificationTime);
	Ret->AddChild("Tracks");

	// Add to the content storage set
	MDObjectPtr Ptr = Object->Child("ContentStorage");
	if(Ptr) Ptr = Ptr->GetLink();
	if(Ptr) Ptr = Ptr["Packages"];
	if(Ptr) Ptr->AddChild("Package", false)->MakeLink(Ret->Object);

	if(BodySID) AddEssenceContainerData(PackageUMID, BodySID);

	// Add this package to our "owned" packages
	Packages.push_back(Ret);

	return Ret;
}


//! Get a pointer to the primary package
PackagePtr Metadata::GetPrimaryPackage(void)
{
	MDObjectPtr PrimaryPackage;

	MDObjectPtr PackageRef = Child("PrimaryPackage");
	if(PackageRef)
	{
		PrimaryPackage = PackageRef->GetLink();
	}
	else
	{
		MDObjectPtr Packages = Child("ContentStorage");
		if(Packages) Packages = Packages["Packages"];
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
			if(ThisPackage && (ThisPackage->Name() == "MaterialPackage"))
			{
				PrimaryPackage = ThisPackage;
				break;
			}
			it++;
		}
	}

	// Couldn't locate the primary package!
	if(!PrimaryPackage)	return NULL;

	// Get the contasining Package object
	return Package::GetPackage(PrimaryPackage);
}


//! Set the duration for this SourceClip and update the track's sequence duration
/*! \param Duration The duration of this SourceClip, -1 or omitted for unknown */
void SourceClip::SetDuration(Int64 Duration /*=-1*/)
{
	if(Duration < 0)
		SetDValue("Duration");
	else
		SetInt64("Duration", Duration);

	// Update the duration in the sequence
	if(Duration < 0) 
	{
		MDObjectPtr Sequence = Parent["Sequence"]->GetLink();
		Sequence->SetDValue("Duration");
	}
	else
	{
		Parent->UpdateDuration();
	}
}


//! Make a link to a specified track
/*! \return true if the link was made, else false */
bool SourceClip::MakeLink(TrackPtr SourceTrack, Int64 StartPosition /*=0*/)
{
	if(!SourceTrack) return false;

	SetInt64("StartPosition", StartPosition);
	SetUint("SourceTrackID", SourceTrack->GetInt("TrackID"));
	SetValue("SourcePackageID",SourceTrack->GetParent()["PackageUID"]);

	return true;
}


//! Make a link to a UMID and TrackID
bool SourceClip::MakeLink(UMIDPtr LinkUMID, Uint32 LinkTrackID, Int64 StartPosition /*=0*/)
{
	SetInt64("StartPosition", StartPosition);
	SetUint("SourceTrackID", LinkTrackID);
	SetValue("SourcePackageID", DataChunk(32, LinkUMID->GetValue()));

	return true;
}


//! Set the duration for this Timecode Component and update the track's sequence duration
/*! \param Duration The duration of this Timecode Component, -1 or omitted for unknown */
void TimecodeComponent::SetDuration(Int64 Duration /*=-1*/)
{
	if(Duration < 0)
		SetDValue("Duration");
	else
		SetInt64("Duration", Duration);

	// Update the duration in the sequence
	if(Duration < 0) 
	{
		MDObjectPtr Sequence = Parent["Sequence"]->GetLink();
		Sequence->SetDValue("Duration");
	}
	else
	{
		Parent->UpdateDuration();
	}
}


bool Metadata::AddEssenceContainerData(UMIDPtr TheUMID, Uint32 BodySID, Uint32 IndexSID /*=0*/)
{
	MDObjectPtr EssenceContainerData = new MDObject("EssenceContainerData");
	ASSERT(EssenceContainerData);
	
	EssenceContainerData->SetValue("LinkedPackageUID", DataChunk(TheUMID.GetPtr()));
	EssenceContainerData->SetUint("BodySID", BodySID);
	if(IndexSID) EssenceContainerData->SetUint("IndexSID", IndexSID);

	MDObjectPtr Content = Object["ContentStorage"];
	if(Content) Content = Content->GetLink();

	if(!Content) return false;

	MDObjectPtr Ptr = Content["EssenceContainerData"];
	if(!Ptr) return false;

	Ptr->AddChild("EssenceContainer", false)->MakeLink(EssenceContainerData);

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

	MDObjectPtr Identifications = Object["Identifications"];
	if(Identifications->empty())
	{
		// Clear all modified flags to prevent unwanted GenerationUID properties first time
		ClearModified_Internal(Object);
	}
	else
	{
		// Update the GenerationUID in the preface
		Object->SetGenerationUID(ThisGeneration);

		MDObjectNamedList::iterator it = Object->begin();
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
		if(!NewIdent["ModificationDate"]) NewIdent->SetString("ModificationDate", UpdateTime);
		ModificationTime = UpdateTime;
	}
	else
	{
		if(!NewIdent["ModificationDate"]) NewIdent->SetString("ModificationDate", ModificationTime);
	}

	Object->SetString("LastModifiedDate", ModificationTime);
	Identifications->AddChild("Identification", false)->MakeLink(NewIdent);
	NewIdent->SetValue("ThisGenerationUID", DataChunk(16, ThisGeneration->GetValue()));

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
//		MDObjectPtr Ptr = Obj->Child("GenerationUID");
//		if(!Ptr) Ptr = Obj->AddChild("GenerationUID");
//		if(Ptr) Ptr->SetValue(DataChunk(16, ThisGeneration->GetValue()));
	}

	MDObjectNamedList::iterator it = Obj->begin();
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
	MDObjectNamedList::iterator it = Obj->begin();
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
	SourceClipPtr Ret = new SourceClip("SourceClip");
	if(!Ret) return Ret;

	// Set the duration
	if(Duration < 0)
		Ret->SetDValue("Duration");
	else
		Ret->SetInt64("Duration", Duration);
	
	// Add zero package and track IDs
	Ret->AddChild("SourcePackageID");
	Ret->AddChild("SourceTrackID");

	// Initially assume the SourceClip starts at the start of the referenced essence
	Ret->AddChild("StartPosition", 0);

	// Add this SourceClip to the sequence for this track
	MDObjectPtr Sequence = Child("Sequence")->GetLink();
	Sequence["StructuralComponents"]->AddChild("StructuralComponent", false)->MakeLink(Ret->Object);

	// Copy the data definition from the sequence
	Ret->AddChild("DataDefinition")->ReadValue(Sequence["DataDefinition"]->PutData().Data, 16);

	// Add this sequence to the list of "owned" components
	Components.push_back(SmartPtr_Cast(Ret, Component));
//	Components.push_back(Ret);

	// Record the track as the parent of the new SourceClip
	Ret->SetParent(this);

	// Update the duration in the sequence
	if(Duration < 0) 
	{
		Sequence->SetDValue("Duration");
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
TimecodeComponentPtr Track::AddTimecodeComponent(Uint16 FPS, bool DropFrame, Int64 Start /*=0*/, Int64 Duration /*=-1*/)
{
	// DRAGONS: If the track is a DM track should we add a DM SourceClip?
	TimecodeComponentPtr Ret = new TimecodeComponent("TimecodeComponent");
	if(!Ret) return Ret;

	//! Set the framerate
	Ret->SetUint("RoundedTimecodeBase", FPS);
	if(DropFrame) Ret->SetUint("DropFrame", 1); else Ret->SetUint("DropFrame", 0);

	//! Set the initial timecode
	Ret->SetInt64("StartTimecode", Start);

	// Set the duration
	if(Duration < 0)
		Ret->SetDValue("Duration");
	else
		Ret->SetInt64("Duration", Duration);

	// Add this Timecode Component to the sequence for this track
	MDObjectPtr Sequence = Child("Sequence")->GetLink();
	Sequence["StructuralComponents"]->AddChild("StructuralComponent", false)->MakeLink(Ret->Object);

	// Copy the data definition from the sequence
	Ret->AddChild("DataDefinition")->ReadValue(Sequence["DataDefinition"]->PutData().Data, 16);

	// Record the track as the parent of the new Timecode Component
	Ret->SetParent(this);

	// Update the duration in the sequence
	if(Duration < 0) 
	{
		Sequence->SetDValue("Duration");
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
	DMSegmentPtr Ret = new DMSegment("DMSegment");
	if(!Ret) return Ret;

	// Set the duration - or not if there is none
	if(Duration >= 0)
		Ret->SetInt64("Duration", Duration);
	
	// Add zero linked track IDs and DMFramework
	Ret->AddChild("TrackIDs");
	Ret->AddChild("DMFramework");

	// Initially assume the SourceClip starts at the start of the referenced essence
	if( EventStart >= 0 )
		Ret->AddChild("EventStartPosition", 0);

	// Add this SourceClip to the sequence for this track
	MDObjectPtr Sequence = Child("Sequence")->GetLink();
	Sequence["StructuralComponents"]->AddChild("StructuralComponent", false)->MakeLink(Ret->Object);

	// Copy the data definition from the sequence
	Ret->AddChild("DataDefinition")->ReadValue(Sequence["DataDefinition"]->PutData().Data, 16);

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
	MDObjectPtr Sequence = Child("Sequence")->GetLink();
	Int64 SeqDuration = 0;
	MDObjectPtr Structs = Sequence["StructuralComponents"];
	MDObjectNamedList::iterator it = Structs->begin();
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
		if(Link->IsDValue("Duration"))
		{
			SeqDuration = -1;
			break;
		}
		SeqDuration += Link->GetInt64("Duration");
		it++;
	}

	if(SeqDuration < 0)
		Sequence->SetDValue("Duration");
	else
		Sequence->SetInt64("Duration", SeqDuration);

	return SeqDuration;
}


//! Add a timeline track to the package
/*! \note If the TrackID is set manually it is the responsibility of the caller to prevent clashes */
TrackPtr Package::AddTrack(ULPtr DataDef, Uint32 TrackNumber, Rational EditRate, std::string TrackName /*=""*/, Uint32 TrackID /*=0*/)
{
	TrackPtr Ret = new Track("Track");
	if(!Ret) return Ret;

	if(TrackName.length()) Ret->SetString("TrackName", TrackName);
	Ret->SetInt("TrackNumber", TrackNumber);
	Ret->SetInt64("Origin", 0);

	MDObjectPtr Ptr = Ret->AddChild("EditRate");
	if(Ptr)
	{
		Ptr->SetInt("Numerator", EditRate.Numerator);
		Ptr->SetInt("Denominator", EditRate.Denominator);
	}

	// Auto set the track ID if not supplied
	if(TrackID == 0)
	{
		ASSERT(LastTrackID < 0xffffffff);

		LastTrackID++;
		TrackID = LastTrackID;
	}
	Ret->SetInt("TrackID", TrackID);

	// Build a new sequence for this track
	MDObjectPtr Sequence = new MDObject("Sequence");
	ASSERT(Sequence);

	// Initialise the sequence
	Sequence->AddChild("DataDefinition")->ReadValue(DataDef->GetValue(), 16);
	Sequence->SetDValue("Duration");
	Sequence->AddChild("StructuralComponents");

	// Add the sequence
	Ret->AddChild("Sequence")->MakeLink(Sequence);

	// Add this track to the package
	Child("Tracks")->AddChild("Track", false)->MakeLink(Ret->Object);

	// Add this track to our "owned" tracks
	Tracks.push_back(Ret);

	// Record this package as the parent of the new track
	Ret->SetParent(this);

	return Ret;
}


//! Update the duration field in each sequence in each track for this package
void Package::UpdateDurations(void)
{
	MDObjectPtr Tracks = Child("Tracks");
	if(!Tracks) return;

	MDObjectNamedList::iterator it = Tracks->begin();
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
TrackPtr Package::AddTrack(ULPtr DataDef, Uint32 TrackNumber, Rational EditRate, Int64 DefaultDuration, std::string TrackName /* = "" */ , Uint32 TrackID /* = 0 */)
{
	TrackPtr Ret = new Track("EventTrack");
	if(!Ret) return Ret;

	if(TrackName.length()) Ret->SetString("TrackName", TrackName);
	Ret->SetInt("TrackNumber", TrackNumber);
	Ret->SetInt64("EventOrigin", 0);

	MDObjectPtr Ptr = Ret->AddChild("EventEditRate");
	if(Ptr)
	{
		Ptr->SetInt("Numerator", EditRate.Numerator);
		Ptr->SetInt("Denominator", EditRate.Denominator);
	}

	// Auto set the track ID if not supplied
	if(TrackID == 0)
	{
		ASSERT(LastTrackID < 0xffffffff);

		LastTrackID++;
		TrackID = LastTrackID;
	}
	Ret->SetInt("TrackID", TrackID);

	// Build a new sequence for this track
	MDObjectPtr Sequence = new MDObject("Sequence");
	ASSERT(Sequence);

	// Initialise the sequence
	Sequence->AddChild("DataDefinition")->ReadValue(DataDef->GetValue(), 16);

	// Pass DefaultDuration on to the Sequence
	if( DefaultDuration == DurationUnspecified )
		Sequence->SetDValue("Length");
	else
		Sequence->SetInt64("Length", DefaultDuration);


	Sequence->AddChild("StructuralComponents");

	// Add the sequence
	Ret->AddChild("Sequence")->MakeLink(Sequence);

	// Add this track to the package
	Child("Slots")->AddChild("Track", false)->MakeLink(Ret->Object);

	// Add this track to our "owned" tracks
	Tracks.push_back(Ret);

	// Record this package as the parent of the new track
	Ret->SetParent(this);

	return Ret;
}

//! Add a static track to the package
/*! \note If the TrackID is set manually it is the responsibility of the caller to prevent clashes */
TrackPtr Package::AddTrack(ULPtr DataDef, Uint32 TrackNumber, std::string TrackName /*=""*/, Uint32 TrackID /*=0*/)
{
	TrackPtr Ret = new Track("StaticTrack");
	if(!Ret) return Ret;

	if(TrackName.length()) Ret->SetString("TrackName", TrackName);
	Ret->SetInt("TrackNumber", TrackNumber);

	// Auto set the track ID if not supplied
	if(TrackID == 0)
	{
		ASSERT(LastTrackID < 0xffffffff);

		LastTrackID++;
		TrackID = LastTrackID;
	}
	Ret->SetInt("TrackID", TrackID);

	// Build a new sequence for this track
	MDObjectPtr Sequence = new MDObject("Sequence");
	ASSERT(Sequence);

	// Initialise the sequence
	Sequence->AddChild("DataDefinition")->ReadValue(DataDef->GetValue(), 16);
	Sequence->AddChild("StructuralComponents");

	// Add the sequence
	Ret->AddChild("Sequence")->MakeLink(Sequence);

	// Add this track to the package
	Child("Tracks")->AddChild("Track", false)->MakeLink(Ret->Object);

	// Add this track to our "owned" tracks
	Tracks.push_back(Ret);

	// Record this package as the parent of the new track
	Ret->SetParent(this);

	return Ret;
}


//! Return the containing "SourceClip" object for this MDObject
/*! \return NULL if MDObject is not contained in a SourceClip object
 */
SourceClipPtr SourceClip::GetSourceClip(MDObjectPtr Object)
{
	return Object->GetOuter() ? SourceClipPtr(dynamic_cast<SourceClip*>(Object->GetOuter())) : NULL;
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


bool DMSegment::MakeLink(MDObjectPtr DMFramework)
{
	MDObjectPtr SourceFramework=Child("DMFramework");

	if(!SourceFramework)
	{
		SourceFramework = AddChild("DMFramework");
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
	if(!BaseObject->IsA("Preface")) return Ret;

	// If this is already part of a Metadata object then return that object
	if(BaseObject->GetOuter()) return Metadata::GetMetadata(BaseObject);

	// Build the basic Metadata object
	Ret = new Metadata(BaseObject);

	// Set the most recent modification time to now
	// Not the value from the MDObject as anything we now do is a new modification
	Ret->ModificationTime = Now2String();

	// Locate the content storage set
	MDObjectPtr ContentStorage = BaseObject["ContentStorage"];
	if(ContentStorage) ContentStorage = ContentStorage->GetLink();

	// Can't go any further if there is no content storage set!
	// DRAGONS: Should this cause an error to be reported?
	if(!ContentStorage) return Ret;

	// Get the list of Packages
	MDObjectPtr PackageList = ContentStorage["Packages"];

	// Can't go any further if there is no package list in the content storage set!
	// DRAGONS: Should this cause an error to be reported?
	if(!PackageList) return Ret;

	// Search for packages and parse them
	MDObject::iterator it = PackageList->begin();
	while(it != PackageList->end())
	{
		// Parse this package
		PackagePtr ThisPackage = Package::Parse((*it).second);

		// Add it to the list of packages for this metadata
		if(ThisPackage) Ret->Packages.push_back(ThisPackage);

		it++;
	}

	return Ret;
}


//! Parse an existing MDObject into a Package object
PackagePtr Package::Parse(MDObjectPtr BaseObject)
{
	PackagePtr Ret;

	// We can only build a Package object from a GenericPackage derived set
	if(!BaseObject->IsA("GenericPackage")) return Ret;

	// If this is already part of a Package object then return that object
	if(BaseObject->GetOuter()) return Package::GetPackage(BaseObject);

	// Build the basic Package object
	Ret = new Package(BaseObject);

	// Clear the LastTrackID - we will search for the highest value in the parsed tracks
	Ret->LastTrackID = 0;

	// Get the list of tracks
	MDObjectPtr TrackList = Ret["Tracks"];

	// Can't go any further if there is no track list
	// DRAGONS: Should this cause an error to be reported?
	if(!TrackList) return Ret;

	// Search for tracks and parse them
	MDObject::iterator it = TrackList->begin();
	while(it != TrackList->end())
	{
		// Parse this track
		TrackPtr ThisTrack = Track::Parse((*it).second);

		if(ThisTrack) 
		{
			// Set the track's parent pointer
			ThisTrack->SetParent(Ret);

			// Get the ID of this track and update LastTrackID if required
			Uint32 ThisID = ThisTrack->GetUint("TrackID");
			if(ThisID > Ret->LastTrackID) Ret->LastTrackID = ThisID;

			// Add it to the list of tracks for this package
			Ret->Tracks.push_back(ThisTrack);
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
	if(!BaseObject->IsA("GenericTrack")) return Ret;

	// If this is already part of a Track object then return that object
	if(BaseObject->GetOuter()) return Track::GetTrack(BaseObject);

	// Build the basic Track object
	Ret = new Track(BaseObject);

	// Get the sequence
	MDObjectPtr Sequence = Ret["Sequence"];
	if(Sequence) Sequence = Sequence->GetLink();

	// Can't go any further if there is no sequence
	// DRAGONS: Should this cause an error to be reported?
	if(!Sequence) return Ret;

	// Get the list of components
	MDObjectPtr ComponentList = Sequence["StructuralComponents"];

	// Can't go any further if there is no component list
	// DRAGONS: Should this cause an error to be reported?
	if(!ComponentList) return Ret;

	// Search for components and parse them
	MDObject::iterator it = ComponentList->begin();
	while(it != ComponentList->end())
	{
		ComponentPtr ThisComponent;

		// Parse all the known component types
		if((*it).second->IsA("SourceClip")) ThisComponent = SourceClip::Parse((*it).second);
		else if((*it).second->IsA("TimecodeComponent")) ThisComponent = TimecodeComponent::Parse((*it).second);
		else if((*it).second->IsA("DMSegment")) ThisComponent = DMSegment::Parse((*it).second);

		if(ThisComponent)
		{
			// Set the component's parent pointer
			ThisComponent->SetParent(Ret);

			// Add it to the list of components for this track
			Ret->Components.push_back(ThisComponent);
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
	if(!BaseObject->IsA("SourceClip")) return Ret;

	// If this is already part of a SourceClip object then return that object
	if(BaseObject->GetOuter()) return SourceClip::GetSourceClip(BaseObject);

	// Build the basic SourceClip object
	Ret = new SourceClip(BaseObject);

	return Ret;
}


//! Parse an existing MDObject into a TimecodeComponent object
TimecodeComponentPtr TimecodeComponent::Parse(MDObjectPtr BaseObject)
{
	TimecodeComponentPtr Ret;

	// We can only build a TimecodeComponent object from a TimecodeComponent
	if(!BaseObject->IsA("TimecodeComponent")) return Ret;

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
	if(!BaseObject->IsA("DMSegment")) return Ret;

	// If this is already part of a DMSegment object then return that object
	if(BaseObject->GetOuter()) return DMSegment::GetDMSegment(BaseObject);

	// Build the basic DMSegment object
	Ret = new DMSegment(BaseObject);

	return Ret;
}


