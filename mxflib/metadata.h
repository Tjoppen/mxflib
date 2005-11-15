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
 *	\version $Id: metadata.h,v 1.8 2005/11/15 12:50:44 matt-beard Exp $
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
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
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
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
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
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
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
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
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
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
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
		Component(const UL &BaseUL) { Object = new MDObject(BaseUL); if(Object) Object->SetOuter(this); }
		Component(ULPtr &BaseUL) { Object = new MDObject(BaseUL); if(Object) Object->SetOuter(this); }

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
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
	};

	class DMSourceClip;
	//! A smart pointer to a DMSourceClip object (with operator[] overload)
	class DMSourceClipPtr : public SmartPtr<DMSourceClip>
	{
	public:
		//! Build a NULL pointer
		DMSourceClipPtr() : SmartPtr<DMSourceClip>() {}

		//! Build a pointer to an existing object
		/*! \note The IRefCount has to be to a Component because that is what DMSourceClip is derived from */
		DMSourceClipPtr(IRefCount<Component> * ptr) : SmartPtr<DMSourceClip>((IRefCount<DMSourceClip> *)ptr) {}

		//! Child access operators that overcome dereferencing problems with SmartPtrs
		MDObjectPtr operator[](const char *ChildName);
		MDObjectPtr operator[](MDOTypePtr ChildType);
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
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
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
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
		MDObjectPtr operator[](const UL &ChildType);
		MDObjectPtr operator[](ULPtr &ChildType);
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
		SourceClip(const UL &BaseUL) : Component(BaseUL) {};
		SourceClip(ULPtr &BaseUL) : Component(BaseUL) {};

		//! Set the duration for this SourceClip and update the track's sequence duration
		/*! \param Duration The duration of this SourceClip, -1 or omitted for unknown */
		void SetDuration(Int64 Duration = -1);

		//! Make a link to a specified track
		bool MakeLink(TrackPtr SourceTrack, Int64 StartPosition = 0);

		//! Make a link to a UMID and TrackID
		bool MakeLink(UMIDPtr LinkUMID, UInt32 LinkTrackID, Int64 StartPosition = 0);

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
	//! Holds data relating to a DMSourceClip
	class DMSourceClip : public SourceClip
	{
	protected:
		//! Protected constructor used to create from an existing MDObject
		DMSourceClip(MDObjectPtr BaseObject) : SourceClip(BaseObject) {}

	public:
		DMSourceClip(std::string BaseType) : SourceClip(BaseType) {};
		DMSourceClip(MDOTypePtr BaseType) : SourceClip(BaseType) {};
		DMSourceClip(const UL &BaseUL) : SourceClip(BaseUL) {};
		DMSourceClip(ULPtr &BaseUL) : SourceClip(BaseUL) {};

		//! Return the containing "DMSourceClip" object for this MDObject
		/*! \return NULL if MDObject is not contained in a SourceClip object
		 */
		static DMSourceClipPtr GetDMSourceClip(MDObjectPtr Object);

		//! Parse an existing MDObject into a DMSourceClip object
		static DMSourceClipPtr Parse(MDObjectPtr BaseObject);
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
		TimecodeComponent(const UL &BaseUL) : Component(BaseUL) {};
		TimecodeComponent(ULPtr &BaseUL) : Component(BaseUL) {};

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
		DMSegment(const UL &BaseUL) : Component(BaseUL) {};
		DMSegment(ULPtr &BaseUL) : Component(BaseUL) {};

		//! Set the duration for this DMSegment and update the track's sequence duration
		/*! \param Duration The duration of this DMSegment, -1 or omitted for unknown */
		void SetDuration(Int64 Duration = -1);

//		//! Make a link to a specified track (in the same Package)
//		bool MakeLink(TrackPtr SourceTrack);

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
		//! Type of a track
		enum TrackType
		{
			TypeUndetermined = -1,				//!< Not yet checked
			TypeUnknown = 0,					//!< Not a known type
			TypeTimecode,						//!< Timecode track (of any type)
			TypeDM,								//!< Descriptive Metadata track
			TypePicture,						//!< Picture track
			TypeSound,							//!< Sound track
			TypeData,							//!< Data track
			TypeAux,							//!< Auxiliary track
			TypePText							//!< Parsed Text track
		};

	public:
		ComponentList Components;				//!< Each component on this track

	protected:
		PackageParent Parent;					//!< Package containing this track

		TrackType ThisTrackType;				//!< The type of this track

		//! Protected constructor used to create from an existing MDObject
		Track(MDObjectPtr BaseObject) : ObjectInterface(BaseObject) 
		{
			ThisTrackType = TypeUndetermined;
		}

		//! Structure for a single item in the track type list (for comparing data definitions)
		struct TrackTypeListItem
		{
			TrackType Type;						//!< The type that this label represents
			UInt8 Label[16];					//!< The label for this track type definition
			int CompareLength;					//!< The number of bytes to compare (to allow generic versions)
		};

		//! List of track type definitions
		typedef std::list<TrackTypeListItem> TrackTypeList;

		static TrackTypeList TrackTypes;		//!< List of known track type definitions
		static bool TrackTypesInited;			//!< Set true once TrackTypeList has been initialized

	public:
		Track(std::string BaseType) { Object = new MDObject(BaseType); ThisTrackType = TypeUndetermined; }
		Track(MDOTypePtr BaseType) { Object = new MDObject(BaseType); ThisTrackType = TypeUndetermined; }
		Track(const UL &BaseUL) { Object = new MDObject(BaseUL); ThisTrackType = TypeUndetermined; }
		Track(ULPtr &BaseUL) { Object = new MDObject(BaseUL); ThisTrackType = TypeUndetermined; }

		//! Add a SourceClip to a track
		SourceClipPtr AddSourceClip(Int64 Duration = -1);

		//! Add a Timecode Component to a track, taking the FPS and Dropframe settings from the track
		TimecodeComponentPtr AddTimecodeComponent(Int64 Start = 0, Int64 Duration = -1)
		{
			MDObjectPtr EditRate = Child("EditRate");
			if(EditRate)
			{
				double FPS = ((double)GetInt("Numerator", 1)) / ((double)GetInt("Denominator", 1));
				return AddTimecodeComponent((UInt16)FPS, (GetInt("Denominator") == 1001), Start, Duration);
			}

			// Default to an edit rate of 1 if no known edit rate
			return AddTimecodeComponent(1, false, Start, Duration);
		}

		//! Add a Timecode Component to a track
		TimecodeComponentPtr AddTimecodeComponent(UInt16 FPS, bool DropFrame, Int64 Start = 0, Int64 Duration = -1);

		//! Add a DMSegment to a track
		DMSegmentPtr AddDMSegment(Int64 EventStart = -1, Int64 Duration = -1);

		//! Add a DMSourceClip to a track
		DMSourceClipPtr AddDMSourceClip(Int64 Duration = -1);

		//! Update the duration field in the sequence for this track based on component durations
		Int64 UpdateDuration(void);

		//! Get the package containing this track
		PackageParent GetParent(void) { return Parent; };

		//! Set the package containing this track
		void SetParent(PackagePtr &NewParent) { Parent = PackageParent(NewParent.GetRef()); }

		//! Set the track containing this component
		void SetParent(IRefCount<Package> *NewParent) { Parent = PackageParent(NewParent); }

		//! Determine the type of this track
		TrackType GetType(void);

		//! Determine if this is an essence track
		bool IsEssenceTrack(void)
		{
			TrackType ThisType = GetType();

			switch(ThisType)
			{
				case TypePicture:				//!< Picture track
				case TypeSound:					//!< Sound track
				case TypeData:					//!< Data track

				// DRAGONS: We currently don't treat the following as essence
//				case TypeAux:					//!< Auxiliary track
//				case TypePText:					//!< Parsed Text track
					return true;

				default:
					return false;
			}
		}

		//! Determine if this is a timecode track
		bool IsTimecodeTrack(void)
		{
			if(GetType() == TypeTimecode) return true;
			return false;
		}

		//! Determine if this is a descriptive metadata track
		bool IsDMTrack(void)
		{
			if(GetType() == TypeDM) return true;
			return false;
		}

		//! Return the containing "Track" object for this MDObject
		/*! \return NULL if MDObject is not contained in a Track object
		 */
		static TrackPtr GetTrack(MDObjectPtr Object);

		//! Parse an existing MDObject into a Track object
		static TrackPtr Parse(MDObjectPtr BaseObject);

		//! Add a new track type definition label
		/*! \param Type The type of track that this new definition identifies
		 *  \param Label The label to compare with the data definition
		 *  \param CompareLength The number of bytes to compare in the label, this allows generic labels
		 */
		static void AddTrackType(TrackType Type, const UInt8 *Label, int CompareLength = 16);

	protected:
		//! Initialise the TrackTypes list with known track types
		static void InitTrackTypes(void);
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
		UInt32 LastTrackID;						//!< Last auto-allocated track ID

		//! Protected constructor used to create from an existing MDObject
		Package(MDObjectPtr BaseObject) : ObjectInterface(BaseObject) {}

	private:
		// Can't create from nothing
		Package();

	public:
		Package(std::string BaseType) : LastTrackID(0) { Object = new MDObject(BaseType); if(Object) Object->SetOuter(this); }
		Package(MDOTypePtr BaseType) : LastTrackID(0) { Object = new MDObject(BaseType); if(Object) Object->SetOuter(this); }
		Package(const UL &BaseUL) : LastTrackID(0) { Object = new MDObject(BaseUL); if(Object) Object->SetOuter(this); }
		Package(ULPtr &BaseUL) : LastTrackID(0) { Object = new MDObject(BaseUL); if(Object) Object->SetOuter(this); }

		//! Add a timeline track to the package
		TrackPtr AddTrack(ULPtr DataDef, UInt32 TrackNumber, Rational EditRate, std::string TrackName = "", UInt32 TrackID = 0);

		//! Add an event track to the package
		TrackPtr AddTrack(ULPtr DataDef, UInt32 TrackNumber, Rational EditRate, Length DefaultDuration, std::string TrackName = "", UInt32 TrackID = 0);

		//! Add a static track to the package
		TrackPtr AddTrack(ULPtr DataDef, UInt32 TrackNumber, std::string TrackName = "", UInt32 TrackID = 0);

		TrackPtr AddPictureTrack(Rational EditRate, std::string TrackName = "Picture Track", UInt32 TrackID = 0) { return AddPictureTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddPictureTrack(UInt32 TrackNumber, Rational EditRate, std::string TrackName = "Picture Track", UInt32 TrackID = 0)
		{
			static const ULPtr PictureDD = new UL(TrackTypeDataDefPicture);
			return AddTrack(PictureDD, TrackNumber, EditRate, TrackName, TrackID);
		}

		TrackPtr AddSoundTrack(Rational EditRate, std::string TrackName = "Sound Track", UInt32 TrackID = 0) { return AddSoundTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddSoundTrack(UInt32 TrackNumber, Rational EditRate, std::string TrackName = "Sound Track", UInt32 TrackID = 0)
		{
			static const ULPtr SoundDD = new UL(TrackTypeDataDefSound);
			return AddTrack(SoundDD, TrackNumber, EditRate, TrackName, TrackID);
		}

		TrackPtr AddDataTrack(Rational EditRate, std::string TrackName = "Data Track", UInt32 TrackID = 0) { return AddDataTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddDataTrack(UInt32 TrackNumber, Rational EditRate, std::string TrackName = "Data Track", UInt32 TrackID = 0)
		{
			static const ULPtr DataDD = new UL(TrackTypeDataDefData);
			return AddTrack(DataDD, TrackNumber, EditRate, TrackName, TrackID);
		}

		TrackPtr AddTimecodeTrack(Rational EditRate, std::string TrackName = "Timecode Track", UInt32 TrackID = 0) { return AddTimecodeTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddTimecodeTrack(UInt32 TrackNumber, Rational EditRate, std::string TrackName = "Timecode Track", UInt32 TrackID = 0)
		{
			static const ULPtr TCDD = new UL(TrackTypeDataDefTimecode12M);
			return AddTrack(TCDD, TrackNumber, EditRate, TrackName, TrackID);
		}

		//! Add an EVENT DM Track
		TrackPtr AddDMTrack(Rational EditRate, Length DefaultDuration, std::string TrackName = "Descriptive Track", UInt32 TrackID = 0) { return AddDMTrack(0, EditRate, DefaultDuration, TrackName, TrackID); }
		TrackPtr AddDMTrack(UInt32 TrackNumber, Rational EditRate, Length DefaultDuration, std::string TrackName = "Descriptive Track", UInt32 TrackID = 0)
		{
			static const ULPtr TCDM = new UL(TrackTypeDataDefDM);
			return AddTrack(TCDM, TrackNumber, EditRate, DefaultDuration, TrackName, TrackID);
		}

		//! Add a TIMELINE DM Track
		TrackPtr AddDMTrack(Rational EditRate, std::string TrackName = "Descriptive Track", UInt32 TrackID = 0) { return AddDMTrack(0, EditRate, TrackName, TrackID); }
		TrackPtr AddDMTrack(UInt32 TrackNumber, Rational EditRate, std::string TrackName = "Descriptive Track", UInt32 TrackID = 0)
		{
			static const ULPtr TCDM = new UL(TrackTypeDataDefDM);
			return AddTrack(TCDM, TrackNumber, EditRate, TrackName, TrackID);
		}

		//! Add a STATIC DM Track
		TrackPtr AddDMTrack(std::string TrackName = "Descriptive Track", UInt32 TrackID = 0) { return AddDMTrack(0, TrackName, TrackID); }
		TrackPtr AddDMTrack(UInt32 TrackNumber, std::string TrackName = "Descriptive Track", UInt32 TrackID = 0)
		{
			static const ULPtr TCDM = new UL(TrackTypeDataDefDM);
			return AddTrack(TCDM, TrackNumber, TrackName, TrackID);
		}

		//! Remove a track from this package
		void RemoveTrack(TrackPtr &Track);

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

		// Update the modification time for this file and any new packages
		void SetTime(void) { ModificationTime = Now2String(); }
		void SetTime(std::string TimeStamp) { ModificationTime = TimeStamp; }

		//! Add a DMScheme to the listed schemes
		void AddDMScheme(ULPtr Scheme)
		{
			Object->Child(DMSchemes_UL)->AddChild()->ReadValue(Scheme->GetValue(), 16);
		}

		//! Add an essence type UL to the listed essence types
		/*! Only added if it does not already appear in the list */
		void AddEssenceType(ULPtr ECType)
		{
			DataChunk ECTypeValue;
			ECTypeValue.Set(16, ECType->GetValue());

			// Get a list of known containers
			MDObjectPtr ECTypeList = Object->Child(EssenceContainers_UL);

			// Scan the list to see if we already have this type
			MDObjectULList::iterator it = ECTypeList->begin();
			while(it != ECTypeList->end())
			{
				if(ECTypeValue == *((*it).second->PutData())) return;
				it++;
			}

			// New type, so add it
			Object->Child(EssenceContainers_UL)->AddChild()->SetValue(ECTypeValue);
		}

		//! Set the operational pattern property of the preface
		void SetOP(ULPtr OP)
		{
			MDObjectPtr Ptr = Object->AddChild(OperationalPattern_UL);
			Ptr->ReadValue(OP->GetValue(), 16);
		}

		// Add a material package to the metadata
		PackagePtr AddMaterialPackage(UMIDPtr PackageUMID) { return AddPackage(MaterialPackage_UL, "", PackageUMID); }
		PackagePtr AddMaterialPackage(std::string PackageName = "", UMIDPtr PackageUMID = NULL) { return AddPackage(MaterialPackage_UL, PackageName, PackageUMID); }

		// Add a top-level file package to the metadata
		PackagePtr AddFilePackage(UInt32 BodySID, UMIDPtr PackageUMID) { return AddPackage(SourcePackage_UL, "", PackageUMID, BodySID); }
		PackagePtr AddFilePackage(UInt32 BodySID, std::string PackageName = "", UMIDPtr PackageUMID = NULL) { return AddPackage(SourcePackage_UL, PackageName, PackageUMID, BodySID); }

		// Add a lower-level source package to the metadata
		PackagePtr AddSourcePackage(UInt32 BodySID, UMIDPtr PackageUMID) { return AddPackage(SourcePackage_UL, "", PackageUMID, BodySID); }
		PackagePtr AddSourcePackage(UInt32 BodySID, std::string PackageName = "", UMIDPtr PackageUMID = NULL) { return AddPackage(SourcePackage_UL, PackageName, PackageUMID, BodySID); }

		bool AddEssenceContainerData(UMIDPtr TheUMID, UInt32 BodySID, UInt32 IndexSID = 0);

		//! Set the primary package property of the preface
		void SetPrimaryPackage(PackagePtr Package) { SetPrimaryPackage(Package->Object); }

		//! Set the primary package property of the preface
		void SetPrimaryPackage(MDObjectPtr Package)
		{
			MDObjectPtr Ptr = Object->Child(PrimaryPackage_UL);
			if(!Ptr) Ptr = Object->AddChild(PrimaryPackage_UL);
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
		PackagePtr AddPackage(const UL &PackageType, std::string PackageName, UMIDPtr PackageUMID, UInt32 BidySID = 0);

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
inline MDObjectPtr PackagePtr::operator[](const UL &ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr PackagePtr::operator[](ULPtr &ChildType) { return GetPtr()->Object[*ChildType]; }
inline MDObjectPtr PackageParent::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr PackageParent::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr PackageParent::operator[](const UL &ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr PackageParent::operator[](ULPtr &ChildType) { return GetPtr()->Object[*ChildType]; }
inline MDObjectPtr TrackPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr TrackPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr TrackPtr::operator[](const UL &ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr TrackPtr::operator[](ULPtr &ChildType) { return GetPtr()->Object[*ChildType]; }
inline MDObjectPtr TrackParent::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr TrackParent::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr TrackParent::operator[](const UL &ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr TrackParent::operator[](ULPtr &ChildType) { return GetPtr()->Object[*ChildType]; }
inline MDObjectPtr SourceClipPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr SourceClipPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr SourceClipPtr::operator[](const UL &ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr SourceClipPtr::operator[](ULPtr &ChildType) { return GetPtr()->Object[*ChildType]; }
inline MDObjectPtr DMSourceClipPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr DMSourceClipPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr DMSourceClipPtr::operator[](const UL &ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr DMSourceClipPtr::operator[](ULPtr &ChildType) { return GetPtr()->Object[*ChildType]; }
inline MDObjectPtr MetadataPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr MetadataPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr MetadataPtr::operator[](const UL &ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr MetadataPtr::operator[](ULPtr &ChildType) { return GetPtr()->Object[*ChildType]; }
inline MDObjectPtr DMSegmentPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr DMSegmentPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr DMSegmentPtr::operator[](const UL &ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr DMSegmentPtr::operator[](ULPtr &ChildType) { return GetPtr()->Object[*ChildType]; }
}

#endif // MXFLIB__METADATA_H
