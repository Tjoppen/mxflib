/*! \file	metadata.h
 *	\brief	Definition of Header Metadata classes
 *
 *			\note The classes hold the minimum other than an MDObject to reduce
 *			the amount of parsing required when reading
 *
 *			- The Metadata class holds data about a set of Header Metadata and contains a Preface object.
 *			- The Package class holds data about a package.
 *			- The Track class holds data about a track.
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
#ifndef MXFLIB__METADATA_H
#define MXFLIB__METADATA_H

namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class Metadata;
	class MetadataPtr;

	//! A smart pointer to a Metadata object (with operator[] overload)
	class MetadataPtr : public SmartPtr<Metadata>
	{
	public:
		MetadataPtr() : SmartPtr<Metadata>() {};
		MetadataPtr(Metadata * ptr) : SmartPtr<Metadata>(ptr) {};
		
		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};
}


// Forward definitions
namespace mxflib
{
	class Package;
	//! A smart pointer to a Package object (with operator[] overload)
	class PackagePtr : public SmartPtr<Package>
	{
	public:
		PackagePtr() : SmartPtr<Package>() {};
		PackagePtr(Package * ptr) : SmartPtr<Package>(ptr) {};
		
		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	//! A list of package pointers
	typedef std::list<PackagePtr> PackageList;


	class Track;
	//! A smart pointer to a Track object (with operator[] overload)
	class TrackPtr : public SmartPtr<Track>
	{
	public:
		TrackPtr() : SmartPtr<Track>() {};
		TrackPtr(Track * ptr) : SmartPtr<Track>(ptr) {};
		
		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	class SourceClip;
	//! A smart pointer to a SourceClip object (with operator[] overload)
	class SourceClipPtr : public SmartPtr<SourceClip>
	{
	public:
		SourceClipPtr() : SmartPtr<SourceClip>() {};
		SourceClipPtr(SourceClip * ptr) : SmartPtr<SourceClip>(ptr) {};
		
		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	class TimecodeComponent;
	//! A smart pointer to a SourceClip object (with operator[] overload)
	class TimecodeComponentPtr : public SmartPtr<TimecodeComponent>
	{
	public:
		TimecodeComponentPtr() : SmartPtr<TimecodeComponent>() {};
		TimecodeComponentPtr(TimecodeComponent * ptr) : SmartPtr<TimecodeComponent>(ptr) {};
		
		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};
}


namespace mxflib
{
	//! Holds data relating to a SourceClip
	class SourceClip : public ObjectInterface, public RefCount<SourceClip>
	{
	public:
		TrackPtr Parent;					// Track containing this SourceClip

	public:
		SourceClip(std::string BaseType) { Object = new MDObject(BaseType); };
		SourceClip(MDOTypePtr BaseType) { Object = new MDObject(BaseType); };
		SourceClip(ULPtr BaseUL) { Object = new MDObject(BaseUL); };

		//! Set the duration for this SourceClip and update the track's sequence duration
		/*! \param Duration The duration of this SourceClip, -1 or omitted for unknown */
		void SetDuration(Int64 Duration = -1);

		//! Make a link to a specified track
		bool MakeLink(TrackPtr SourceTrack, Int64 StartPosition = 0);

		//! Make a link to a UMID and TrackID
		bool MakeLink(UMIDPtr LinkUMID, Uint32 LinkTrackID, Int64 StartPosition = 0);
	};
}


namespace mxflib
{
	//! Holds data relating to a Timecode Component
	class TimecodeComponent : public ObjectInterface, public RefCount<TimecodeComponent>
	{
	public:
		TrackPtr Parent;					// Track containing this SourceClip

	public:
		TimecodeComponent(std::string BaseType) { Object = new MDObject(BaseType); };
		TimecodeComponent(MDOTypePtr BaseType) { Object = new MDObject(BaseType); };
		TimecodeComponent(ULPtr BaseUL) { Object = new MDObject(BaseUL); };

		//! Set the duration for this Timecode Component and update the track's sequence duration
		/*! \param Duration The duration of this Timecode Component, -1 or omitted for unknown */
		void SetDuration(Int64 Duration = -1);
	};
}


namespace mxflib
{
	//! Holds data relating to a track
	class Track : public ObjectInterface, public RefCount<Track>
	{
	public:
		PackagePtr Parent;					// Package containing this track

	public:
		Track(std::string BaseType) { Object = new MDObject(BaseType); };
		Track(MDOTypePtr BaseType) { Object = new MDObject(BaseType); };
		Track(MDObjectPtr Obj) { Object = Obj; };
		Track(ULPtr BaseUL) { Object = new MDObject(BaseUL); };

		//! Add a SourceClip to a track
		SourceClipPtr AddSourceClip(Int64 Duration = -1);

		//! Add a Timecode Component to a track
		TimecodeComponentPtr AddTimecodeComponent(Uint16 FPS, bool DropFrame, Int64 Start = 0, Int64 Duration = -1);

		//! Update the duration field in the sequence for this track based on component durations
		Int64 UpdateDuration(void);
	};
}


namespace mxflib
{
	//! Holds data relating to a package
	class Package : public ObjectInterface, public RefCount<Package>
	{
	private:
		Uint32 LastTrackID;						// Last auto-allocated track ID

	private:
		// Can't create from nothing
		Package() { ASSERT(0); };

	public:
		Package(std::string BaseType) : LastTrackID(0) { Object = new MDObject(BaseType); };
		Package(MDOTypePtr BaseType) : LastTrackID(0) { Object = new MDObject(BaseType); };
		Package(ULPtr BaseUL) : LastTrackID(0) { Object = new MDObject(BaseUL); };

		//! Add a timeline track to the package
		TrackPtr AddTrack(ULPtr DataDef, Uint32 TrackNumber, Rational EditRate, std::string TrackName = "", Uint32 TrackID = 0);

		TrackPtr AddPictureTrack(Rational EditRate, std::string TrackName = "Picture Track", Uint32 TrackID = 0) { return AddPictureTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddPictureTrack(Uint32 TrackNumber, Rational EditRate, std::string TrackName = "Picture Track", Uint32 TrackID = 0)
		{
			static const Uint8 PictureDD_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00 };
			static const ULPtr PictureDD = new UL(PictureDD_Data);
			return AddTrack(PictureDD, TrackNumber, EditRate, TrackName, TrackID);
		};

		TrackPtr AddSoundTrack(Rational EditRate, std::string TrackName = "Sound Track", Uint32 TrackID = 0) { return AddSoundTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddSoundTrack(Uint32 TrackNumber, Rational EditRate, std::string TrackName = "Sound Track", Uint32 TrackID = 0)
		{
			static const Uint8 SoundDD_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00 };
			static const ULPtr SoundDD = new UL(SoundDD_Data);
			return AddTrack(SoundDD, TrackNumber, EditRate, TrackName, TrackID);
		};

		TrackPtr AddDataTrack(Rational EditRate, std::string TrackName = "Data Track", Uint32 TrackID = 0) { return AddDataTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddDataTrack(Uint32 TrackNumber, Rational EditRate, std::string TrackName = "Data Track", Uint32 TrackID = 0)
		{
			static const Uint8 DataDD_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00 };
			static const ULPtr DataDD = new UL(DataDD_Data);
			return AddTrack(DataDD, TrackNumber, EditRate, TrackName, TrackID);
		};

		TrackPtr AddTimecodeTrack(Rational EditRate, std::string TrackName = "Timecode Track", Uint32 TrackID = 0) { return AddTimecodeTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddTimecodeTrack(Uint32 TrackNumber, Rational EditRate, std::string TrackName = "Timecode Track", Uint32 TrackID = 0)
		{
			static const Uint8 TCDD_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00 };
			static const ULPtr TCDD = new UL(TCDD_Data);
			return AddTrack(TCDD, TrackNumber, EditRate, TrackName, TrackID);
		};

		//! Update the duration field in each sequence in each track for this package
		void UpdateDurations(void);
	};
}


namespace mxflib
{
	//! Holds data relating to a single partition
	class Metadata : public ObjectInterface, public RefCount<Metadata>
	{
	private:
		std::string ModificationTime;				// Creation or modification time for this metadata, used for package times

	public:
		Metadata();
		Metadata(std::string TimeStamp);
		void Init(void);

		// Update the package modification time
		void SetTime(void) { ModificationTime = Now2String(); };
		void SetTime(std::string TimeStamp) { ModificationTime = TimeStamp; };

		//! Add a DMScheme to the listed schemes
		void AddDMScheme(ULPtr Scheme)
		{
			Object->Child("DMSchemes")->AddChild("DMScheme",false)->ReadValue(Scheme->GetValue(), 16);
		}

		//! Add an essence type UL to the listed essence types
		/*! Only added if it does not already appear in the list */
		void AddEssenceType(ULPtr ECType)
		{
			DataChunk ECTypeValue;
			ECTypeValue.Set(16, ECType->GetValue());

			// Get a list of known containers
			MDObjectListPtr ECTypeList = Object->Child("EssenceContainers")->ChildList("EssenceContainer");

			// Scan the list to see if we already have this type
			MDObjectList::iterator it = ECTypeList->begin();
			while(it != ECTypeList->end())
			{
				if(ECTypeValue == (*it)->PutData()) return;
				it++;
			}

			// New type, so add it
			Object->Child("EssenceContainers")->AddChild("EssenceContainer",false)->SetValue(ECTypeValue);
		}

		//! Set the operational pattern property of the preface
		void SetOP(ULPtr OP)
		{
			MDObjectPtr Ptr = Object->AddChild("OperationalPattern");
			Ptr->ReadValue(OP->GetValue(), 16);
		}

		// Add a material package to the metadata
		PackagePtr AddMaterialPackage(UMIDPtr PackageUMID) { return AddPackage("MaterialPackage", "", PackageUMID); };
		PackagePtr AddMaterialPackage(std::string PackageName = "", UMIDPtr PackageUMID = NULL) { return AddPackage("MaterialPackage", PackageName, PackageUMID); };

		// Add a top-level file package to the metadata
		PackagePtr AddFilePackage(Uint32 BodySID, UMIDPtr PackageUMID) { return AddPackage("SourcePackage", "", PackageUMID, BodySID); };
		PackagePtr AddFilePackage(Uint32 BodySID, std::string PackageName = "", UMIDPtr PackageUMID = NULL) { return AddPackage("SourcePackage", PackageName, PackageUMID, BodySID); };

		bool AddEssenceContainerData(UMIDPtr TheUMID, Uint32 BodySID, Uint32 IndexSID = 0);

		//! Set the primary package property of the preface
		void SetPrimaryPackage(PackagePtr Package) { SetPrimaryPackage(Package->Object); };

		//! Set the primary package property of the preface
		void SetPrimaryPackage(MDObjectPtr Package)
		{
			MDObjectPtr Ptr = Object->Child("PrimaryPackage");
			if(!Ptr) Ptr = Object->AddChild("PrimaryPackage");
			Ptr->MakeLink(Package);
		}

		//! Update the Generation UID of all modified sets and add the specified Ident set
		bool UpdateGenerations(MDObjectPtr Ident, std::string UpdateTime = "");

	private:
		//! Add a package of the specified type to the matadata
		PackagePtr AddPackage(std::string PackageType, std::string PackageName, UMIDPtr PackageUMID, Uint32 BidySID = 0);

		//! Update the Generation UID of a set if modified - then iterate through strongly linked sets
		bool UpdateGenerations_Internal(MDObjectPtr Obj, UUIDPtr ThisGeneration);

		//! Clear all modified flags for this set and strongly linked sets - used when adding initial Identification set
		void ClearModified_Internal(MDObjectPtr Obj);
	};
}


// These simple inlines need to be defined after the classes
namespace mxflib
{
inline MDObjectPtr PackagePtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr PackagePtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr TrackPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr TrackPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr SourceClipPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr SourceClipPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr MetadataPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr MetadataPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
}

#endif // MXFLIB__METADATA_H
