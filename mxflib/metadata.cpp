/*! \file	metadata.cpp
 *	\brief	Implementation of Metadata class
 *
 *			The Metadata class holds data about a set of Header Metadata.
 *			The class holds a Preface set object
 *
 *	\version $Id: metadata.cpp,v 1.1.2.3 2004/10/10 18:40:17 terabrit Exp $
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
	// as it is defivef from GenerationInterchangeObject
	Object->AddChild("InstanceUID")->ReadValue(DataChunk(new UUID));

	timestmp ts;

	struct tm * now;
	time_t tmp=time(NULL);
	now=gmtime(&tmp);

	ts.yr=now->tm_year;
	mxflib::Swap(ts.yr);
	ts.month=now->tm_mon;
	ts.day=now->tm_mday;
	ts.hour=now->tm_hour;
	ts.min=now->tm_min;
	ts.sec=now->tm_sec;


	DataChunk TimeStamp(sizeof(timestmp),reinterpret_cast<const Uint8 *>(&ts));
	Uint8 * p1, d1,d2;
	p1=TimeStamp.Data;

	d1=*p1;   //Endian problem with the year between  MXF and AAF
	d2=*(p1+1);
	*(p1+1)=d1;
	*p1=d2;

	Object->AddChild("LastModified")->SetValue(TimeStamp);

	version_t ver;
	ver.major=1;
	ver.minor=2;
	DataChunk Version(sizeof(version_t),reinterpret_cast<const Uint8 *>(&ver));
	
	Object->AddChild("Version")->SetValue(Version);

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
	SetValue("SourcePackageID",SourceTrack->Parent["PackageUID"]);

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
	UUIDPtr ThisGeneration = new UUID;

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

	// Record the track as the parent of the new SourceClip
	Ret->Parent = this;

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
	Ret->Parent = this;

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
/*! \param EventStart The start position of this Segemnt, -1 or omitted for static or timeline */
/*! \param Duration The duration of this SourceClip, -1 or omitted for static */
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
	Ret->Parent = this;

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

	// Record this package as the parent of the new track
	Ret->Parent = this;

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
			TrackPtr ThisTrack = new Track(ThisLink);
			ThisTrack->UpdateDuration();
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

	// Record this package as the parent of the new track
	Ret->Parent = this;

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

	// Record this package as the parent of the new track
	Ret->Parent = this;

	return Ret;
}


//! Return the containing "SourceClip" object for this MDObject
/*! \return NULL if MDObject is not contained in a SourceClip object
 */
SourceClipPtr SourceClip::GetSourceClip(MDObjectPtr Object)
{
	return Object->GetOuter() ? SourceClipPtr(dynamic_cast<SourceClip*>(Object->GetOuter())) : NULL;
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
