/*! \file	metadata.h
 *	\brief	Definition of Header Metadata classes
 *
 *			\note The classes hold the minimum other than an MDObject to reduce
 *			the amount of parsing required when reading
 *
 *			- The Metadata class holds data about a set of Header Metadata and contains a Preface object.
 *			- The Package class holds data about a package.
 *			- The Track class holds data about a track.
 *
 *	\version $Id: metadata.h,v 1.2 2004/11/12 09:20:44 matt-beard Exp $
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
		MetadataPtr() : SmartPtr<Metadata>() {}
		MetadataPtr(IRefCount<Metadata> * ptr) : SmartPtr<Metadata>(ptr) {};
		
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
		PackagePtr() : SmartPtr<Package>() {}
		PackagePtr(IRefCount<Package> * ptr) : SmartPtr<Package>(ptr) {}
		
		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	//! A parent pointer to a Package object (with operator[] overload)
	class PackageParent : public ParentPtr<Package>
	{
	public:
		PackageParent() : ParentPtr<Package>() {}
		PackageParent(IRefCount<Package> * ptr) : ParentPtr<Package>(ptr) {}

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
		TrackPtr() : SmartPtr<Track>() {}
		TrackPtr(IRefCount<Track> * ptr) : SmartPtr<Track>(ptr) {}

		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	//! A list of smart pointers to track objects
	typedef std::list<TrackPtr> TrackList;


	//! A parent pointer to a Track object (with operator[] overload)
	class TrackParent : public ParentPtr<Track>
	{
	public:
		TrackParent() : ParentPtr<Track>() {}
		TrackParent(IRefCount<Track> * ptr) : ParentPtr<Track>(ptr) {}

		//! Set value from a TrackPtr
		TrackParent & operator=(const TrackPtr &sp) { __Assign(sp.GetRef()); return *this;}

		//! Set value from a Track*
		TrackParent & operator=(IRefCount<Track> *ptr) { __Assign(ptr); return *this;}

		//! Child access operators that overcome dereferencing problems with ParentPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};


	//! Generic component super-class
	/*! \note Derived classes MUST NOT be separately derived from RefCount<> */
	class Component : public ObjectInterface, public RefCount<Component>
	{
	protected:
		TrackParent Parent;					// Track containing this component

		//! Protected constructor used to create from an existing MDObject
		Component(MDObjectPtr BaseObject) : ObjectInterface(BaseObject) {}

	public:
		Component(std::string BaseType) { Object = new MDObject(BaseType); if(Object) Object->SetOuter(this); }
		Component(MDOTypePtr BaseType) { Object = new MDObject(BaseType); if(Object) Object->SetOuter(this); }
		Component(ULPtr BaseUL) { Object = new MDObject(BaseUL); if(Object) Object->SetOuter(this); }

		//! Get the track containing this component
		TrackParent GetParent(void) { return Parent; };

		//! Set the track containing this component
		void SetParent(TrackPtr &NewParent) { Parent = NewParent; }

		//! Set the track containing this component
		void SetParent(IRefCount<Track> *NewParent) { Parent = TrackParent(NewParent); }

		//! Allow polymorphic destructors
		virtual ~Component() {};
	};

	//! Smart pointer to a generic component super-class
	typedef SmartPtr<Component> ComponentPtr;

	//! List of smart pointers to generic component super-classes
	typedef std::list<ComponentPtr> ComponentList;


	class SourceClip;
	//! A smart pointer to a SourceClip object (with operator[] overload)
	class SourceClipPtr : public SmartPtr<SourceClip>
	{
	public:
		//! Build a NULL pointer
		SourceClipPtr() : SmartPtr<SourceClip>() {}

		//! Build a pointer to an existing object
		/*! \note The IRefCount has to be to a Component because that is what SourceClip is derived from */
		SourceClipPtr(IRefCount<Component> * ptr) : SmartPtr<SourceClip>((IRefCount<SourceClip> *)ptr) {}

		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	class TimecodeComponent;
	//! A smart pointer to a SourceClip object (with operator[] overload)
	class TimecodeComponentPtr : public SmartPtr<TimecodeComponent>
	{
	public:
		//! Build a NULL pointer
		TimecodeComponentPtr() : SmartPtr<TimecodeComponent>() {}

		//! Build a pointer to an existing object
		/*! \note The IRefCount has to be to a Component because that is what TimecodeComponent is derived from */
		TimecodeComponentPtr(IRefCount<Component> * ptr) : SmartPtr<TimecodeComponent>((IRefCount<TimecodeComponent> *)ptr) {}

		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};

	class DMSegment;
	//! A smart pointer to a DMSegment object (with operator[] overload)
	class DMSegmentPtr : public SmartPtr<DMSegment>
	{
	public:
		//! Build a NULL pointer
		DMSegmentPtr() : SmartPtr<DMSegment>() {}

		//! Build a pointer to an existing object
		/*! \note The IRefCount has to be to a Component because that is what DMSegment is derived from */
		DMSegmentPtr(IRefCount<Component> * ptr) : SmartPtr<DMSegment>((IRefCount<DMSegment> *)ptr) {};

		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
	};
}



namespace mxflib
{
	//! Holds data relating to a SourceClip
	class SourceClip : public Component
	{
	protected:
		//! Protected constructor used to create from an existing MDObject
		SourceClip(MDObjectPtr BaseObject) : Component(BaseObject) {}

	public:
		SourceClip(std::string BaseType) : Component(BaseType) {};
		SourceClip(MDOTypePtr BaseType) : Component(BaseType) {};
		SourceClip(ULPtr BaseUL) : Component(BaseUL) {};

		//! Set the duration for this SourceClip and update the track's sequence duration
		/*! \param Duration The duration of this SourceClip, -1 or omitted for unknown */
		void SetDuration(Int64 Duration = -1);

		//! Make a link to a specified track
		bool MakeLink(TrackPtr SourceTrack, Int64 StartPosition = 0);

		//! Make a link to a UMID and TrackID
		bool MakeLink(UMIDPtr LinkUMID, Uint32 LinkTrackID, Int64 StartPosition = 0);

		//! Return the containing "SourceClip" object for this MDObject
		/*! \return NULL if MDObject is not contained in a SourceClip object
		 */
		static SourceClipPtr GetSourceClip(MDObjectPtr Object);

		//! Parse an existing MDObject into a SourceClip object
		static SourceClipPtr Parse(MDObjectPtr BaseObject);
	};
}


namespace mxflib
{
	//! Holds data relating to a Timecode Component
	class TimecodeComponent : public Component
	{
	protected:
		//! Protected constructor used to create from an existing MDObject
		TimecodeComponent(MDObjectPtr BaseObject) : Component(BaseObject) {}

	public:
		TimecodeComponent(std::string BaseType) : Component(BaseType) {};
		TimecodeComponent(MDOTypePtr BaseType) : Component(BaseType) {};
		TimecodeComponent(ULPtr BaseUL) : Component(BaseUL) {};

		//! Set the duration for this Timecode Component and update the track's sequence duration
		/*! \param Duration The duration of this Timecode Component, -1 or omitted for unknown */
		void SetDuration(Int64 Duration = -1);

		//! Return the containing "TimecodeComponent" object for this MDObject
		/*! \return NULL if MDObject is not contained in a TimecodeComponent object
		 */
		static TimecodeComponentPtr GetTimecodeComponent(MDObjectPtr Object);

		//! Parse an existing MDObject into a TimecodeComponent object
		static TimecodeComponentPtr Parse(MDObjectPtr BaseObject);
	};
}

namespace mxflib
{
	//! Holds data relating to a DMSegment
	class DMSegment : public Component
	{
	protected:
		//! Protected constructor used to create from an existing MDObject
		DMSegment(MDObjectPtr BaseObject) : Component(BaseObject) {}

	public:
		DMSegment(std::string BaseType) : Component(BaseType) {};
		DMSegment(MDOTypePtr BaseType) : Component(BaseType) {};
		DMSegment(ULPtr BaseUL) : Component(BaseUL) {};

		//! Set the duration for this DMSegment and update the track's sequence duration
		/*! \param Duration The duration of this DMSegment, -1 or omitted for unknown */
		void SetDuration(Int64 Duration = -1);

		//! Make a link to a specified track (in the same Package)
		bool MakeLink(TrackPtr SourceTrack);

		//! Make a link to a specified DMFramework
		bool MakeLink(MDObjectPtr DMFramework);

		//! Return the containing "DMSegment" object for this MDObject
		/*! \return NULL if MDObject is not contained in a DMSegment object
		 */
		static DMSegmentPtr GetDMSegment(MDObjectPtr Object);

		//! Parse an existing MDObject into a DMSegment object
		static DMSegmentPtr Parse(MDObjectPtr BaseObject);
	};
}

namespace mxflib
{
	//! Holds data relating to a track
	class Track : public ObjectInterface, public RefCount<Track>
	{
	public:
		ComponentList Components;				//!< Each component on this track

	protected:
		PackageParent Parent;					//!< Package containing this track

		//! Protected constructor used to create from an existing MDObject
		Track(MDObjectPtr BaseObject) : ObjectInterface(BaseObject) {}

	public:
		Track(std::string BaseType) { Object = new MDObject(BaseType); }
		Track(MDOTypePtr BaseType) { Object = new MDObject(BaseType); }
		Track(ULPtr BaseUL) { Object = new MDObject(BaseUL); }

		//! Add a SourceClip to a track
		SourceClipPtr AddSourceClip(Int64 Duration = -1);

		//! Add a Timecode Component to a track
		TimecodeComponentPtr AddTimecodeComponent(Uint16 FPS, bool DropFrame, Int64 Start = 0, Int64 Duration = -1);

		//! Add a DMSegment to a track
		DMSegmentPtr AddDMSegment(Int64 EventStart = -1, Int64 Duration = -1);

		//! Update the duration field in the sequence for this track based on component durations
		Int64 UpdateDuration(void);

		//! Get the package containing this track
		PackageParent GetParent(void) { return Parent; };

		//! Set the package containing this track
		void SetParent(PackagePtr &NewParent) { Parent = PackageParent(NewParent.GetRef()); }

		//! Set the track containing this component
		void SetParent(IRefCount<Package> *NewParent) { Parent = PackageParent(NewParent); }

		//! Return the containing "Track" object for this MDObject
		/*! \return NULL if MDObject is not contained in a Track object
		 */
		static TrackPtr GetTrack(MDObjectPtr Object);

		//! Parse an existing MDObject into a Track object
		static TrackPtr Parse(MDObjectPtr BaseObject);
	};
}


namespace mxflib
{
	//! Special values of DefaultDuration for event tracks
    enum
    {
        DurationUnspecified = -1,      //!< Item is of an unspecified duration (unknown or not a timeline item)
        DurationInstantaneous = 0,     //!< Item is instantaneous
    };

	//! Holds data relating to a package
	/*! FIXME: There is currently no way to remove a track from a package */
	class Package : public ObjectInterface, public RefCount<Package>
	{
	public:
		TrackList Tracks;						//!< Each track in this package

	protected:
		Uint32 LastTrackID;						//!< Last auto-allocated track ID

		//! Protected constructor used to create from an existing MDObject
		Package(MDObjectPtr BaseObject) : ObjectInterface(BaseObject) {}

	private:
		// Can't create from nothing
		Package();

	public:
		Package(std::string BaseType) : LastTrackID(0) { Object = new MDObject(BaseType); if(Object) Object->SetOuter(this); }
		Package(MDOTypePtr BaseType) : LastTrackID(0) { Object = new MDObject(BaseType); if(Object) Object->SetOuter(this); }
		Package(ULPtr BaseUL) : LastTrackID(0) { Object = new MDObject(BaseUL); if(Object) Object->SetOuter(this); }

		//! Add a timeline track to the package
		TrackPtr AddTrack(ULPtr DataDef, Uint32 TrackNumber, Rational EditRate, std::string TrackName = "", Uint32 TrackID = 0);

		//! Add an event track to the package
		TrackPtr AddTrack(ULPtr DataDef, Uint32 TrackNumber, Rational EditRate, Int64 DefaultDuration, std::string TrackName = "", Uint32 TrackID = 0);

		//! Add a static track to the package
		TrackPtr AddTrack(ULPtr DataDef, Uint32 TrackNumber, std::string TrackName = "", Uint32 TrackID = 0);

		TrackPtr AddPictureTrack(Rational EditRate, std::string TrackName = "Picture Track", Uint32 TrackID = 0) { return AddPictureTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddPictureTrack(Uint32 TrackNumber, Rational EditRate, std::string TrackName = "Picture Track", Uint32 TrackID = 0)
		{
			static const Uint8 PictureDD_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00 };
			static const ULPtr PictureDD = new UL(PictureDD_Data);
			return AddTrack(PictureDD, TrackNumber, EditRate, TrackName, TrackID);
		}

		TrackPtr AddSoundTrack(Rational EditRate, std::string TrackName = "Sound Track", Uint32 TrackID = 0) { return AddSoundTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddSoundTrack(Uint32 TrackNumber, Rational EditRate, std::string TrackName = "Sound Track", Uint32 TrackID = 0)
		{
			static const Uint8 SoundDD_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00 };
			static const ULPtr SoundDD = new UL(SoundDD_Data);
			return AddTrack(SoundDD, TrackNumber, EditRate, TrackName, TrackID);
		}

		TrackPtr AddDataTrack(Rational EditRate, std::string TrackName = "Data Track", Uint32 TrackID = 0) { return AddDataTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddDataTrack(Uint32 TrackNumber, Rational EditRate, std::string TrackName = "Data Track", Uint32 TrackID = 0)
		{
			static const Uint8 DataDD_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00 };
			static const ULPtr DataDD = new UL(DataDD_Data);
			return AddTrack(DataDD, TrackNumber, EditRate, TrackName, TrackID);
		}

		TrackPtr AddTimecodeTrack(Rational EditRate, std::string TrackName = "Timecode Track", Uint32 TrackID = 0) { return AddTimecodeTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddTimecodeTrack(Uint32 TrackNumber, Rational EditRate, std::string TrackName = "Timecode Track", Uint32 TrackID = 0)
		{
			static const Uint8 TCDD_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00 };
			static const ULPtr TCDD = new UL(TCDD_Data);
			return AddTrack(TCDD, TrackNumber, EditRate, TrackName, TrackID);
		}

		// Add an EVENT DM Track
		TrackPtr AddDMTrack(Rational EditRate, Int64 DefaultDuration = DurationUnspecified, std::string TrackName = "Descriptive Track", Uint32 TrackID = 0) { return AddDMTrack(0, EditRate, DefaultDuration, TrackName, TrackID); }
		TrackPtr AddDMTrack(Uint32 TrackNumber, Rational EditRate, Int64 DefaultDuration, std::string TrackName = "Descriptive Track", Uint32 TrackID = 0)
		{
			static const Uint8 TCDM_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00 };
			static const ULPtr TCDM = new UL(TCDM_Data);
			return AddTrack(TCDM, TrackNumber, EditRate, DefaultDuration, TrackName, TrackID);
		}

		// Add a TIMELINE DM Track
		TrackPtr AddDMTrack(Rational EditRate, std::string TrackName = "Descriptive Track", Uint32 TrackID = 0) { return AddDMTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddDMTrack(Uint32 TrackNumber, Rational EditRate, std::string TrackName = "Descriptive Track", Uint32 TrackID = 0)
		{
			static const Uint8 TCDM_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00 };
			static const ULPtr TCDM = new UL(TCDM_Data);
			return AddTrack(TCDM, TrackNumber, EditRate, TrackName, TrackID);
		}

		// Add a STATIC DM Track
		TrackPtr AddDMTrack(std::string TrackName = "Descriptive Track", Uint32 TrackID = 0) { return AddDMTrack(0, TrackName, TrackID); }
		TrackPtr AddDMTrack(Uint32 TrackNumber, std::string TrackName = "Descriptive Track", Uint32 TrackID = 0)
		{
			static const Uint8 TCDM_Data[16] = { 0x06, 0x0e, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00 };
			static const ULPtr TCDM = new UL(TCDM_Data);
			return AddTrack(TCDM, TrackNumber, TrackName, TrackID);
		}

		//! Update the duration field in each sequence in each track for this package
		void UpdateDurations(void);

		//! Return the containing "Package" object for this MDObject
		/*! \return NULL if MDObject is not contained in a Package object
		 */
		static PackagePtr GetPackage(MDObjectPtr Object);

		//! Parse an existing MDObject into a Package object
		static PackagePtr Parse(MDObjectPtr BaseObject);
	};
}


namespace mxflib
{
	//! Holds data relating to a single partition
	class Metadata : public ObjectInterface, public RefCount<Metadata>
	{
	public:
		PackageList Packages;						//!< Each package in this metadata

	protected:
		std::string ModificationTime;				//!< Creation or modification time for this metadata, used for package times

		//! Protected constructor used to create from an existing MDObject
		Metadata(MDObjectPtr BaseObject) : ObjectInterface(BaseObject) {}

	public:
		Metadata();
		Metadata(std::string TimeStamp);
		void Init(void);

		// Update the package modification time
		void SetTime(void) { ModificationTime = Now2String(); }
		void SetTime(std::string TimeStamp) { ModificationTime = TimeStamp; }

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
		PackagePtr AddMaterialPackage(UMIDPtr PackageUMID) { return AddPackage("MaterialPackage", "", PackageUMID); }
		PackagePtr AddMaterialPackage(std::string PackageName = "", UMIDPtr PackageUMID = NULL) { return AddPackage("MaterialPackage", PackageName, PackageUMID); }

		// Add a top-level file package to the metadata
		PackagePtr AddFilePackage(Uint32 BodySID, UMIDPtr PackageUMID) { return AddPackage("SourcePackage", "", PackageUMID, BodySID); }
		PackagePtr AddFilePackage(Uint32 BodySID, std::string PackageName = "", UMIDPtr PackageUMID = NULL) { return AddPackage("SourcePackage", PackageName, PackageUMID, BodySID); }

		bool AddEssenceContainerData(UMIDPtr TheUMID, Uint32 BodySID, Uint32 IndexSID = 0);

		//! Set the primary package property of the preface
		void SetPrimaryPackage(PackagePtr Package) { SetPrimaryPackage(Package->Object); }

		//! Set the primary package property of the preface
		void SetPrimaryPackage(MDObjectPtr Package)
		{
			MDObjectPtr Ptr = Object->Child("PrimaryPackage");
			if(!Ptr) Ptr = Object->AddChild("PrimaryPackage");
			Ptr->MakeLink(Package);
		}

		//! Get a pointer to the primary package
		PackagePtr GetPrimaryPackage(void);

		//! Update the Generation UID of all modified sets and add the specified Ident set
		bool UpdateGenerations(MDObjectPtr Ident, std::string UpdateTime = "");

		//! Return the containing "Metadata" object for this MDObject
		/*! \return NULL if MDObject is not contained in a Metadata object
		 */
		static MetadataPtr GetMetadata(MDObjectPtr Object);

		//! Parse an existing MDObject into a Metadata object
		static MetadataPtr Parse(MDObjectPtr BaseObject);

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
inline MDObjectPtr PackageParent::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr PackageParent::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr TrackPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr TrackPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr TrackParent::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr TrackParent::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr SourceClipPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr SourceClipPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr MetadataPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr MetadataPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr DMSegmentPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr DMSegmentPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
}

#endif // MXFLIB__METADATA_H
