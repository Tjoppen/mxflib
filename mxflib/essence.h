/*! \file	essence.h
 *	\brief	Definition of classes that handle essence reading and writing
 *
 *	\version $Id: essence.h,v 1.3 2004/11/12 09:20:43 matt-beard Exp $
 *
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
#ifndef MXFLIB__ESSENCE_H
#define MXFLIB__ESSENCE_H


#include <map>
#include <list>


// Forward refs
namespace mxflib
{
	//! Smart pointer to a GCWriter
	class GCWriter;
	typedef SmartPtr<GCWriter> GCWriterPtr;

	class GCReader;
	typedef SmartPtr<GCReader> GCReaderPtr;

	// Type used to identify stream
	typedef int GCStreamID;
}


namespace mxflib
{
	//! Abstract super-class for objects that supply large quantities of essence data
	/*! This is used when clip-wrapping to prevent large quantities of data being loaded into memory 
	 *! \note Classes derived from this class <b>must not</b> include their own RefCount<> derivation
	 */
	class EssenceSource : public RefCount<EssenceSource>
	{
	protected:
		//! Holds the stream ID for this essence stream when added to a GCWriter
		/*! This value is persisted here between calls to a GCWriter via BodyWriter or similar.
		 *  Set to -1 if no stream ID yet set.
		 */
		GCStreamID StreamID;
		
		//! Index manager to use if we can index the essence
		IndexManagerPtr IndexMan;

		//! Sub-stream ID to use for our index data if we can index the essence
		int IndexStreamID;

	public:
		// Base constructor
		EssenceSource() : StreamID(-1) {};

		//! Virtual destructor to allow polymorphism
		virtual ~EssenceSource() { };

		//! Get the size of the essence data in bytes
		/*! \note There is intentionally no support for an "unknown" response */
		virtual Length GetEssenceDataSize(void) = 0;

		//! Get the next "installment" of essence data
		/*! This will attempt to return an entire wrapping unit (e.g. a full frame for frame-wrapping) but will return it in
		 *  smaller chunks if this would break the MaxSize limit. If a Size is specified then the chunk returned will end at
		 *  the first wrapping unit end encountered before Size. On no account will portions of two or more different wrapping
		 *  units be returned together. The mechanism for selecting a type of wrapping (e.g. frame, line or clip) is not 
		 *  (currently) part of the common EssenceSource interface.
		 *  \return Pointer to a data chunk holding the next data or a NULL pointer when no more remains
		 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
		 *	\note If Size = 0 the object will decide the size of the chunk to return
		 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
		 */
		virtual DataChunkPtr GetEssenceData(Uint64 Size = 0, Uint64 MaxSize = 0) = 0;

		//! Did the last call to GetEssenceData() return the end of a wrapping item
		/*! \return true if the last call to GetEssenceData() returned an entire wrapping unit.
		 *  \return true if the last call to GetEssenceData() returned the last chunk of a wrapping unit.
		 *  \return true if the last call to GetEssenceData() returned the end of a clip-wrapped clip.
		 *  \return false if there is more data pending for the current wrapping unit.
		 *  \return false if the source is to be clip-wrapped and there is more data pending for the clip
		 */
		virtual bool EndOfItem(void) = 0;

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual Uint8 GetGCEssenceType(void) = 0;

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual Uint8 GetGCElementType(void) = 0;

		//! Set the stream ID for this stream of sub-stream
		void SetStreamID(GCStreamID NewID) { StreamID = NewID; }

		//! Get the stream ID for this stream of sub-stream
		GCStreamID GetStreamID(void) { return StreamID; }

		//! Is the last data read the start of an edit point?
		virtual bool IsEditPoint(void) { return true; }

		//! Get the edit rate of this wrapping of the essence
		/*! \note This may not be the same as the original "native" edit rate of the
		 *        essence if this EssenceSource is wrapping to a different edit rate 
		 */
		virtual Rational GetEditRate(void) = 0;

		//! Get the current position in GetEditRate() sized edit units
		/*! This is relative to the start of the stream, so the first edit unit is always 0.
		 *  This is the same as the number of edit units read so far, so when the essence is 
		 *  exhausted the value returned shall be the size of the essence
		 */
		virtual Position GetCurrentPosition(void) = 0;

		//! Set a source type or parser specific option
		/*! \return true if the option was successfully set */
		virtual bool SetOption(std::string Option, Int64 Param = 0) { return false; } ;

		//! Get BytesPerEditUnit if Constant, else 0
		/*! \note This value may be useful even if CanIndex() returns false
		 */
		virtual Uint32 GetBytesPerEditUnit() { return 0; }

		//! Can this stream provide indexing
		/*! If true then SetIndex Manager can be used to set the index manager that will receive indexing data
		 */
		virtual bool CanIndex() { return false; }

		//! Set the index manager to use for building index tables for this essence
		/*! \note The values are stored even if this stream does not support indexing as a derived stream may do
		 */
		virtual void SetIndexManager(IndexManagerPtr &Manager, int StreamID)
		{
			IndexMan = Manager;
			IndexStreamID = StreamID;
		}

		//! Get the index manager
		virtual IndexManagerPtr &GetIndexManager(void) { return IndexMan; }

		//! Get the index manager sub-stream ID
		virtual int &GetIndexStreamID(void) { return IndexStreamID; }
	};

	// Smart pointer to an EssenceSource object
	typedef SmartPtr<EssenceSource> EssenceSourcePtr;

	// List of smart pointer to EssenceSource objects
	typedef std::list<EssenceSourcePtr> EssenceSourceList;
}



namespace mxflib
{
	//! Default "Multiple Essence Types in the Generic Container" Label
	const Uint8 GCMulti_Data[16] = { 0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x03, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x7F, 0x01, 0x00 };
}


/*
namespace mxflib
{
	//! Class that manages writing of essence containers
	class ECWriter : public RefCount<ECWriter>
	{
		MXFFilePtr LinkedFile;				//!< File that will be written to
		Uint32 TheBodySID;					//!< Body SID for this Essence Container

	public:
		//! Constructor
		ECWriter(MXFFilePtr File, Uint32 BodySID = 0);

		//! Add an essence container (mapping) UL to those used by this essence container
		void AddEssenceUL(ULPtr EssenceUL) {};

		//! Write essence data
		void Write(Uint64 Size, const Uint8 *Data) {};
	};
}
*/


namespace mxflib
{
	//! Structure to hold information about each stream in a GC
	struct GCStreamData
	{
		Uint8 Type;							//!< Item type
		Uint8 SchemeOrCount;				//!< Scheme if system or element count if essence
		Uint8 Element;						//!< Element identifier or type
		Uint8 SubOrNumber;					//!< Sub ID if system or element number if essence
		Uint8 RegDes;						//!< The registry designator if this is a system item
		Uint8 RegVer;						//!< The registry version number for the item key
		IndexManagerPtr IndexMan;			//!< If indexing this stream a pointer to the index manager, else NULL
		int IndexSubStream;					//!< If indexing this stream the sub stream number, else undefined
		bool IndexFiller;					//!< If indexing this stream true if filler <b>preceeding</b> this stream is to be indexed, else undefined
		bool CountFixed;					//!< True once the essence element count has been fixed
											/*!< The count is fixed the first time either a key is written
											 *   or a track number is reported */
		Uint32 WriteOrder;					//!< The (default) write order for this stream
											/*!< Elements with a lower WriteOrder are written first when the
											 *   content package is written */
	};

	//! Class that manages writing of generic container essence
	class GCWriter : public RefCount<GCWriter>
	{
	protected:
		MXFFilePtr LinkedFile;				//!< File that will be written to
		Uint32 TheBodySID;					//!< Body SID for this Essence Container

		int	StreamTableSize;				//!< Size of StreamTable
		int	StreamCount;					//!< Number of entries in use in StreamTable

		int StreamBase;						//!< Base of all stream numbers in keys

		GCStreamData *StreamTable;			//!< Table of data for streams for this GC

		Uint32 KAGSize;						//!< KAGSize for this Essence Container
		bool ForceFillerBER4;				//!< True if filler items must have BER lengths forced to 4-byte BER

		int NextWriteOrder;					//!< The "WriteOrder" to use for the next auto "SetWriteOrder()"

		Position IndexEditUnit;				//!< Edit unit of the current CP for use if indexing
											/*!< This property starts at zero and is incremented with each CP written, however the value
											 *   can be changed by calling SetIndexEditUnit() before calling StartNewCP()
											 */

		Uint64 StreamOffset;				//!< Current stream offset within this essence container

	public:
		//! Constructor
		GCWriter(MXFFilePtr File, Uint32 BodySID = 0, int Base = 0);

		//! Destructor - free the stream table
		~GCWriter()
		{
			delete[] StreamTable;
		}

		//! Set the KAG for this Essence Container
		void SetKAG(Uint32 KAG, bool ForceBER4 = false) { KAGSize = KAG; ForceFillerBER4 = ForceBER4; };

		//! Define a new non-CP system element for this container
		GCStreamID AddSystemElement(unsigned int RegistryDesignator, unsigned int SchemeID, unsigned int ElementID, unsigned int SubID = 0)	{ return AddSystemElement(false, RegistryDesignator, SchemeID, ElementID, SubID); }

		//! Define a new CP-compatible system element for this container
		GCStreamID AddCPSystemElement(unsigned int RegistryDesignator, unsigned int SchemeID, unsigned int ElementID, unsigned int SubID = 0)	{ return AddSystemElement(true, RegistryDesignator, SchemeID, ElementID, SubID); }

		//! Define a new system element for this container
		GCStreamID AddSystemElement(bool CPCompatible, unsigned int RegistryDesignator, unsigned int SchemeID, unsigned int ElementID, unsigned int SubID = 0);
		
		//! Define a new non-CP picture element for this container
		GCStreamID AddPictureElement(unsigned int ElementType) { return AddPictureElement(false, ElementType); }

		//! Define a new CP-compatible picture element for this container
		GCStreamID AddCPPictureElement(unsigned int ElementType) { return AddPictureElement(true, ElementType); }

		//! Define a new picture element for this container
		GCStreamID AddPictureElement(bool CPCompatible, unsigned int ElementType) { return AddEssenceElement( CPCompatible ? 0x05 : 0x15, ElementType); }

		//! Define a new non-CP sound element for this container
		GCStreamID AddSoundElement(unsigned int ElementType) { return AddPictureElement(false, ElementType); }

		//! Define a new CP-compatible sound element for this container
		GCStreamID AddCPSoundElement(unsigned int ElementType) { return AddPictureElement(true, ElementType); }

		//! Define a new sound element for this container
		GCStreamID AddSoundElement(bool CPCompatible, unsigned int ElementType) { return AddEssenceElement( CPCompatible ? 0x06 : 0x16, ElementType); }

		//! Define a new non-CP data element for this container
		GCStreamID AddDataElement(unsigned int ElementType) { return AddPictureElement(false, ElementType); }

		//! Define a new CP-compatible data element for this container
		GCStreamID AddCPDataElement(unsigned int ElementType) { return AddPictureElement(true, ElementType); }

		//! Define a new data element for this container
		GCStreamID AddDataElement(bool CPCompatible, unsigned int ElementType) { return AddEssenceElement( CPCompatible ? 0x07 : 0x17, ElementType); }

		//! Define a new compound element for this container
		GCStreamID AddCompoundElement(unsigned int ElementType) { return AddEssenceElement( 0x18, ElementType); }

		//! Define a new essence element for this container
		GCStreamID AddEssenceElement(unsigned int EssenceType, unsigned int ElementType);

		//! Allow this data stream to be indexed and set the index manager
		void AddStreamIndex(GCStreamID ID, IndexManagerPtr &IndexMan, int IndexSubStream, bool IndexFiller = false);

		//! Get the track number associated with the specified stream
		Uint32 GetTrackNumber(GCStreamID ID);

		//! Assign an essence container (mapping) UL to the specified stream
		void AssignEssenceUL(GCStreamID ID, ULPtr EssenceUL);

		//! Start a new content package (and write out the prevous one if required)
		void StartNewCP(void);

		//! Calculate how much data will be written if "Flush" is called now
		Uint64 CalcWriteSize(void);

		//! Flush any remaining data
		void Flush(void);

		//! Get the current stream offset
		Int64 GetStreamOffset(void) { return StreamOffset; }

		//! Set the index position for the current CP
		void SetIndexEditUnit(Position EditUnit) { IndexEditUnit = EditUnit; }

		//! Get the index position of the current CP
		Position GetIndexEditUnit(void) { return IndexEditUnit; }

		//! Add system item data to the current CP
		void AddSystemData(GCStreamID ID, Uint64 Size, const Uint8 *Data);

		//! Add system item data to the current CP
		void AddSystemData(GCStreamID ID, DataChunkPtr Chunk) { AddSystemData(ID, Chunk->Size, Chunk->Data); }

		//! Add encrypted system item data to the current CP
		void AddSystemData(GCStreamID ID, Uint64 Size, const Uint8 *Data, UUIDPtr ContextID, Length PlaintextOffset = 0);
		
		//! Add encrypted system item data to the current CP
		void AddSystemData(GCStreamID ID, DataChunkPtr Chunk, UUIDPtr ContextID, Length PlaintextOffset = 0) { AddSystemData(ID, Chunk->Size, Chunk->Data, ContextID, PlaintextOffset); }

		//! Add essence data to the current CP
		void AddEssenceData(GCStreamID ID, Uint64 Size, const Uint8 *Data);

		//! Add essence data to the current CP
		void AddEssenceData(GCStreamID ID, DataChunkPtr Chunk) { AddEssenceData(ID, Chunk->Size, Chunk->Data); }

		//! Add essence data to the current CP
		void AddEssenceData(GCStreamID ID, EssenceSource* Source);

		//! Add encrypted essence data to the current CP
		void AddEssenceData(GCStreamID ID, Uint64 Size, const Uint8 *Data, UUIDPtr ContextID, Length PlaintextOffset = 0);
		
		//! Add encrypted essence data to the current CP
		void AddEssenceData(GCStreamID ID, DataChunkPtr Chunk, UUIDPtr ContextID, Length PlaintextOffset = 0)  { AddEssenceData(ID, Chunk->Size, Chunk->Data, ContextID, PlaintextOffset); }

		//! Add encrypted essence data to the current CP
		void AddEssenceData(GCStreamID ID, EssenceSource* Source, UUIDPtr ContextID, Length PlaintextOffset = 0);

		//! Add an essence item to the current CP with the essence to be read from a KLVObject
		void AddEssenceData(GCStreamID ID, KLVObjectPtr Source);


		//! Calculate how many bytes would be written if the specified object were written with WriteRaw()
		Length CalcRawSize(KLVObjectPtr Object);

		//! Write a raw KLVObject to the file - this is written immediately and not buffered in the WriteQueue
		void WriteRaw(KLVObjectPtr Object);


		//! Structure for items to be written
		struct WriteBlock
		{
			Uint64 Size;				//!< Number of bytes of data to write
			Uint8 *Buffer;				//!< Pointer to bytes to write
			EssenceSource *Source;		//!< Pointer to an EssenceSource object or NULL
			KLVObjectPtr KLVSource;		//!< Pointer to a KLVObject as source - or NULL
			IndexManagerPtr IndexMan;	//!< Index manager that wants to know about this data
			int IndexSubStream;			//!< Sub-stream ID of data for indexing
			bool IndexFiller;			//!< If true filler will also be indexed with SubStream -1
			bool WriteEncrypted;		//!< True if the data is to be written as encrypted data (via a KLVEObject)
		};

		//! Type for holding the write queue in write order
		typedef std::map<Uint32,WriteBlock> WriteQueueMap;

		//! Queue of items for the current content package in write order
		WriteQueueMap WriteQueue;


		//! Set the WriteOrder for the specified stream
		void SetWriteOrder(GCStreamID ID, int WriteOrder = -1, int Type =-1);

		//! Read the count of streams
		int GetStreamCount(void) { return StreamCount; };
	};
}


namespace mxflib
{
	class EssenceSubParserBase;
	typedef SmartPtr<EssenceSubParserBase> EssenceSubParserPtr;
	typedef ParentPtr<EssenceSubParserBase> EssenceSubParserParent;

	class WrappingOption : public RefCount<WrappingOption>
	{
	public:
		//! Wrapping type
		/*! \note "None" is only for use as a default condition */
		enum WrapType { None, Frame, Clip, Line, Other } ;

		EssenceSubParserParent Handler;			//!< Pointer to the object that can parse this wrapping option - parent pointer because the parser holds a copy of this!
		std::string Description;				//!< Human readable description of this wrapping option (to allow user selection)
		ULPtr	WrappingUL;						//!< UL for this wrapping
		ULList	RequiredPartners;				//!< List of other items that *MUST* accompany this item to use this wrapping
		Uint8	GCEssenceType;					//!< The Generic Container essence type, or 0 if not a GC wrapping
		Uint8	GCElementType;					//!< The Generic Container element value, or 0 if not a GC wrapping
		WrapType ThisWrapType;					//!< The type of this wrapping (frame, clip etc.)
		bool	CanSlave;						//!< True if this wrapping can be a "slave" which allows it to be used at a different edit rate than its own
		bool	CanIndex;						//!< True if this wrapping can be VBR indexed by the handler (CBR essence may need VBR indexing when interleaved)
		bool	CBRIndex;						//!< True if this wrapping will use a CBR index table (and therefore has a non-zero return value from GetBytesPerEditUnit() )
		Uint8	BERSize;						//!< The BER length size to use for this wrapping (or 0 for any)
		Uint32 BytesPerEditUnit;				//!< set non zero for ConstSamples
	};

	typedef SmartPtr<WrappingOption> WrappingOptionPtr;
	typedef std::list<WrappingOptionPtr> WrappingOptionList;

	/*! An MDObject with an associated stream identifier 
	 *  (used to differentiate multiple streams in an essence file)
	 *  and a human-readable description
	 */
	struct EssenceStreamDescriptor
	{
		Uint32 ID;								//!< ID for this essence stream
		std::string Description;				//!< Description of this essence stream
		MDObjectPtr Descriptor;					//!< Pointer to an actual essence descriptor for this stream
	};
	typedef std::list<EssenceStreamDescriptor> EssenceStreamDescriptorList;

	
	//! Abstract base class for all essence parsers
	/*! \note It is important that no derived class has its own derivation of RefCount<> */
	class EssenceSubParserBase : public RefCount<EssenceSubParserBase>
	{
	protected:
		//! The wrapping options selected
		WrappingOptionPtr SelectedWrapping;

		//! The index manager in use
		IndexManagerPtr Manager;

		//! This essence stream's stream ID in the index manager
		int	ManagedStreamID;

	public:

		//! Base class for essence parser EssenceSource objects
		/*! Still abstract as there is no generic way to determine the data size */
		class ESP_EssenceSource : public EssenceSource
		{
		protected:
			EssenceSubParserPtr Caller;
			FileHandle File;
			Uint32 Stream;
			Uint64 RequestedCount;
			IndexTablePtr Index;
			DataChunkPtr RemainingData;
			bool EndOfData;
			bool Started;

		public:
			//! Construct and initialise for essence parsing/sourcing
			ESP_EssenceSource(EssenceSubParserPtr TheCaller, FileHandle InFile, Uint32 UseStream, Uint64 Count = 1)
			{
				Caller = TheCaller;
				File = InFile;
				Stream = UseStream;
				RequestedCount = Count;
				EndOfData = false;
				Started = false;
			};

			//! Get the next "installment" of essence data
			/*! This will attempt to return an entire wrapping unit (e.g. a full frame for frame-wrapping) but will return it in
			 *  smaller chunks if this would break the MaxSize limit. If a Size is specified then the chunk returned will end at
			 *  the first wrapping unit end encountered before Size. On no account will portions of two or more different wrapping
			 *  units be returned together. The mechanism for selecting a type of wrapping (e.g. frame, line or clip) is not 
			 *  (currently) part of the common EssenceSource interface.
			 *  \return Pointer to a data chunk holding the next data or a NULL pointer when no more remains
			 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
			 *	\note If Size = 0 the object will decide the size of the chunk to return
			 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
			 */
			virtual DataChunkPtr GetEssenceData(Uint64 Size = 0, Uint64 MaxSize = 0) { return BaseGetEssenceData(Size, MaxSize); };

			//! Non-virtual basic version of GetEssenceData() that can be called by derived classes
			DataChunkPtr BaseGetEssenceData(Uint64 Size = 0, Uint64 MaxSize = 0)
			{
				// Allow us to differentiate the first call
				if(!Started) Started = true;

				DataChunkPtr Data;

				if(RemainingData)
				{
					Data = RemainingData;
					RemainingData = NULL;
				}
				else
				{
					Data = Caller->Read(File, Stream, 1);
				}
				if(Data)
				{
					if(Data->Size == 0) Data = NULL;
					else
					{
						if((MaxSize) && (Data->Size > MaxSize))
						{
							RemainingData = new DataChunk(Data->Size - MaxSize, &Data->Data[MaxSize]);
							Data->Resize((Uint32)MaxSize);
						}
					}
				}

				// Record when we hit the end of all data
				if(!Data) EndOfData = true;

				return Data;
			}

			//! Did the last call to GetEssenceData() return the end of a wrapping item
			/*! \return true if the last call to GetEssenceData() returned an entire wrapping unit.
			 *  \return true if the last call to GetEssenceData() returned the last chunk of a wrapping unit.
			 *  \return true if the last call to GetEssenceData() returned the end of a clip-wrapped clip.
			 *  \return false if there is more data pending for the current wrapping unit.
			 *  \return false if the source is to be clip-wrapped and there is more data pending for the clip
			 */
			virtual bool EndOfItem(void) 
			{ 
				// If we are clip wrapping then we only end when no more data
				if(Caller->GetWrapType() == WrappingOption::Clip) return EndOfData;

				// Otherwise items end when there is no data remaining from the last read
				return !RemainingData;
			}

			//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
			virtual Uint8 GetGCEssenceType(void) { return Caller->GetGCEssenceType(); }

			//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
			virtual Uint8 GetGCElementType(void) { return Caller->GetGCElementType(); }

			//! Is the last data read the start of an edit point?
			virtual bool IsEditPoint(void) { return true; }

			//! Get the edit rate of this wrapping of the essence
			/*! \note This may not be the same as the original "native" edit rate of the
			 *        essence if this EssenceSource is wrapping to a different edit rate 
			 */
			virtual Rational GetEditRate(void) { return Caller->GetEditRate(); }

			//! Get the current position in GetEditRate() sized edit units
			/*! This is relative to the start of the stream, so the first edit unit is always 0.
			 *  This is the same as the number of edit units read so far, so when the essence is 
			 *  exhausted the value returned shall be the size of the essence
			 */
			virtual Position GetCurrentPosition(void) { return Caller->GetCurrentPosition(); }

			//! Set a parser specific option
			/*! \return true if the option was successfully set */
			virtual bool SetOption(std::string Option, Int64 Param = 0) { return Caller->SetOption(Option, Param); } ;

			//! Get BytesPerEditUnit if Constant, else 0
			/*! \note This value may be useful even if CanIndex() returns false
			 */
			virtual Uint32 GetBytesPerEditUnit() { return Caller->GetBytesPerEditUnit(); }

			//! Can this stream provide indexing
			/*! If true then SetIndex Manager can be used to set the index manager that will receive indexing data
			 */
			virtual bool CanIndex() { return Caller->SelectedWrapping->CanIndex; }

			//! Set the index manager to use for building index tables for this essence
			virtual void SetIndexManager(IndexManagerPtr &Manager, int StreamID)
			{
				// Set the manager in our containing parser
				Caller->SetIndexManager(Manager, StreamID);
			}
		};

		// Allow embedded essence source to access our protected properties
		friend class ESP_EssenceSource;


	protected:

	public:
		//! Base destructor (to allow polymorphism)
		virtual ~EssenceSubParserBase() {};

		//! Build a new parser of this type and return a pointer to it
		virtual EssenceSubParserPtr NewParser(void) const = 0;

		//! Report the extensions of files this sub-parser is likely to handle
		virtual StringList HandledExtensions(void) { StringList Ret; return Ret; };

		//! Examine the open file and return a list of essence descriptors
		/*! This function should fail as fast as possible if the essence if not identifyable by this object 
		 *	\return A list of EssenceStreamDescriptors where each essence stream identified in the input file has
		 *			an identifier (to allow it to be referenced later) and an MXF File Descriptor
		 */
		virtual EssenceStreamDescriptorList IdentifyEssence(FileHandle InFile)
		{
			EssenceStreamDescriptorList Ret;
			return Ret;
		}

		//! Examine the open file and return the wrapping options known by this parser
		/*! \param InFile The open file to examine (if the descriptor does not contain enough info)
		 *	\param Descriptor An essence stream descriptor (as produced by function IdentifyEssence)
		 *		   of the essence stream requiring wrapping
		 *	\note The options should be returned in an order of preference as the caller is likely to use the first that it can support
		 */
		virtual WrappingOptionList IdentifyWrappingOptions(FileHandle InFile, EssenceStreamDescriptor Descriptor)
		{
			WrappingOptionList Ret;
			return Ret;
		}

		//! Set a wrapping option for future Read and Write calls
		virtual void Use(Uint32 Stream, WrappingOptionPtr &UseWrapping)
		{
			// DRAGONS: Any derived version of Use() must also set SelectedWrapping
			SelectedWrapping = UseWrapping;
		}

		//! Get the wrapping type that has been selected by Use()
		WrappingOption::WrapType GetWrapType(void)
		{
			if(!SelectedWrapping) return WrappingOption::None;

			return SelectedWrapping->ThisWrapType;
		}

		//! Set a non-native edit rate
		/*! \return true if this rate is acceptable */
		virtual bool SetEditRate(Rational EditRate)
		{
			return false;
		}

		//! Get the current edit rate
		virtual Rational GetEditRate(void) = 0;

		//! Get the preferred edit rate (if one is known)
		/*! \return The prefered edit rate or 0/0 if note known
		 */
		virtual Rational GetPreferredEditRate(void)
		{
			// By default we don't know the preferred rate
			return Rational(0,0);
		}

		//! Get BytesPerEditUnit, if Constant
		virtual Uint32 GetBytesPerEditUnit() { return 0; }

		//! Get the current position in SetEditRate() sized edit units
		/*! This is relative to the start of the stream, so the first edit unit is always 0.
		 *  This is the same as the number of edit units read so far, so when the essence is 
		 *  exhausted the value returned shall be the size of the essence
		 */
		virtual Position GetCurrentPosition(void) = 0;

		//! Set the IndexManager for this essence stream (and the stream ID if we are not the main stream)
		virtual void SetIndexManager(IndexManagerPtr &TheManager, int StreamID = 0)
		{
			Manager = TheManager;
			ManagedStreamID = StreamID;
		}

		//! Get the IndexManager for this essence stream
		virtual IndexManagerPtr &GetIndexManager(void) { return Manager; };

		//! Get the IndexManager StreamID for this essence stream
		virtual int GetIndexStreamID(void) { return ManagedStreamID; };

		//! Set the stream offset for a specified edit unit into the current index manager
		virtual void SetStreamOffset(Position EditUnit, Uint64 Offset)
		{
			if(Manager) Manager->SetOffset(ManagedStreamID, EditUnit, Offset);
		}

		//! Offer the stream offset for a specified edit unit to the current index manager
		virtual bool OfferStreamOffset(Position EditUnit, Uint64 Offset)
		{
			if(!Manager) return false;
			return Manager->OfferOffset(ManagedStreamID, EditUnit, Offset);
		}

		//! Instruct index manager to accept the next edit unit
		virtual void IndexNext(void)
		{
			if(Manager) Manager->AcceptNext();
		}

		//! Instruct index manager to accept and log the next edit unit
		virtual int IndexLogNext(void)
		{
			if(Manager) return Manager->AcceptLogNext();
			return -1;
		}

		//! Instruct index manager to log the next edit unit
		virtual int LogNext(void)
		{
			if(Manager) return Manager->LogNext();
			return -1;
		}

		//! Read an edit unit from the index manager's log
		virtual Position ReadLog(int LogID)
		{
			if(Manager) return Manager->ReadLog(LogID);
			return -1;
		}

		//! Instruct index manager to accept provisional entry
		/*! \return The edit unit of the entry accepted - or -1 if none available */
		virtual Position AcceptProvisional(void)
		{
			if(Manager) return Manager->AcceptProvisional();
			return -1;
		}

		//! Read the edit unit of the last entry added via the index manager (or -1 if none added)
		Position GetLastNewEditUnit(void) 
		{ 
			if(Manager) return Manager->GetLastNewEditUnit();
			return -1;
		}

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual Uint8 GetGCEssenceType(void) { return SelectedWrapping->GCEssenceType; }

		//! Get the GCEssenceType to use when wrapping this essence in a Generic Container
		virtual Uint8 GetGCElementType(void) { return SelectedWrapping->GCElementType; }


		//! Read a number of wrapping items from the specified stream and return them in a data chunk
		/*! If frame or line mapping is used the parameter Count is used to
		 *	determine how many items are read. In frame wrapping it is in
		 *	units of EditRate, as specified in the call to Use(), which may
		 *  not be the frame rate of this essence
		 *	\note This is going to take a lot of memory in clip wrapping! 
		 */
		virtual DataChunkPtr Read(FileHandle InFile, Uint32 Stream, Uint64 Count = 1) = 0;

		//! Build an EssenceSource to read a number of wrapping items from the specified stream
		virtual ESP_EssenceSource *GetEssenceSource(FileHandle InFile, Uint32 Stream, Uint64 Count = 1) = 0;

		//! Write a number of wrapping items from the specified stream to an MXF file
		/*! If frame or line mapping is used the parameter Count is used to
		 *	determine how many items are read. In frame wrapping it is in
		 *	units of EditRate, as specified in the call to Use(), which may
		 *  not be the frame rate of this essence stream
		 *	\note This is the only safe option for clip wrapping
		 *	\return Count of bytes transferred
		 */
		virtual Length Write(FileHandle InFile, Uint32 Stream, MXFFilePtr OutFile, Uint64 Count = 1) = 0;

		//! Set a parser specific option
		/*! \return true if the option was successfully set */
		virtual bool SetOption(std::string Option, Int64 Param = 0) { return false; } ;
	};
}


namespace mxflib
{
	//! Base class for handlers to receive notification of the next file about to be opened
	class NewFileHandler : public RefCount<NewFileHandler>
	{
	public:
		virtual ~NewFileHandler();

		//! Receive notification of a new file about to be opened
		/*! \param FileName - reference to a std::string containing the name of the file about to be opened - <b>may be changed by this function if required</b>
		 */
		virtual void NewFile(std::string &FileName) = 0;
	};

	//! Smart pointer to a NewFileHandler
	typedef SmartPtr<NewFileHandler> NewFileHandlerPtr;

	//! File parser
	class FileParser : public RefCount<FileParser>
	{
	protected:
		NewFileHandlerPtr Handler;
		std::string BaseFileName;
		bool IsFileList;
		int ListOrigin;
		int ListIncrement;

	public:
		//! Construct a FileParser and optionally set a single source filename
		FileParser(std::string FileName = "") 
		{ 
			BaseFileName = FileName; 
			IsFileList = false; 
		}

		//! Set a single source filename
		void SetFileName(std::string &FileName) { BaseFileName = FileName; IsFileList = false; }

		//! Set a source file list
		void SetFileList(std::string &FileList, int Origin = 0, int Increment = 1)
		{
			BaseFileName = FileList;
			IsFileList = true;
			ListOrigin = Origin;
			ListIncrement = Increment;
		}

		//! Set a handler to receive notification of all file open actions
		void SetNewFileHandler(NewFileHandlerPtr &NewHandler) { Handler = NewHandler; }

		//!####
		EssenceStreamDescriptorList IdentifyEssence(void);

		//!####
		WrappingOptionList IdentifyWrappingOptions(EssenceStreamDescriptor Descriptor);

		//! Read a number of wrapping items from the specified stream and return them in a data chunk
		/*! If frame or line mapping is used the parameter Count is used to
		 *	determine how many items are read. In frame wrapping it is in
		 *	units of EditRate, as specified in the call to Use(), which may
		 *  not be the frame rate of this essence
		 *	\note This is going to take a lot of memory in clip wrapping! 
		 */
		DataChunkPtr Read(Uint32 Stream, Uint64 Count = 1);

		//! Build an EssenceSource to read a number of wrapping items from the specified stream
		EssenceSubParserBase::ESP_EssenceSource *GetEssenceSource(Uint32 Stream, Uint64 Count = 1);

		//! Write a number of wrapping items from the specified stream to an MXF file
		/*! If frame or line mapping is used the parameter Count is used to
		 *	determine how many items are read. In frame wrapping it is in
		 *	units of EditRate, as specified in the call to Use(), which may
		 *  not be the frame rate of this essence stream
		 *	\note This is the only safe option for clip wrapping
		 *	\return Count of bytes transferred
		 */
		Uint64 Write(Uint32 Stream, MXFFilePtr OutFile, Uint64 Count = 1);
	};
}

namespace mxflib
{
	//! Pair containing a pointer to an essence parser and its associated essence descriptors
	typedef std::pair<EssenceSubParserPtr, EssenceStreamDescriptorList> ParserDescriptorPair;

	//! List of pairs of essence parser pointers with associated file descriptors
	class ParserDescriptorList : public RefCount<ParserDescriptorList>, public std::list<ParserDescriptorPair> {};
	typedef SmartPtr<ParserDescriptorList> ParserDescriptorListPtr;


	class EssenceParser
	{
	private:
		//! List of pointers to essence parsers
		typedef std::list<EssenceSubParserPtr> EssenceParserList;

		//! List of pointers to known parsers
		/*! Used only for building parsers to parse essence - the parses 
		 *  in this list must not themselves be used for essence parsing 
		 *
		 *	DRAGONS: Should this be static?
		 */
		EssenceParserList EPList;

	public:
		//! Build an essence parser with all known sub-parsers
		EssenceParser();

		//! Add a new EssenceSubParser type
		/*! This adds an instance of a sub parser type that can be used to identify essence 
		 *  and will act as a factory to build more instances of that sub parser type if required
		 *  to parse an essence stream
		 */
		void AddSubParserType(EssenceSubParserPtr NewType)
		{
			EPList.push_back(NewType);
		}

		//! Build a list of parsers with their descriptors for a given essence file
		ParserDescriptorListPtr IdentifyEssence(FileHandle InFile);

		//! Configuration data for an essence parser with a specific wrapping option
		/*! \note No parser must contain one of these that includes a pointer to that parser otherwise it will never be deleted (circular reference)
		 */
		class WrappingConfig : public RefCount<WrappingConfig>
		{
		public:
			EssenceSubParserPtr Parser;					//!< The parser that parses this essence - true smart pointer not a parent pointer to keep parser alive
			WrappingOptionPtr WrapOpt;					//!< The wrapping options
			MDObjectPtr EssenceDescriptor;				//!< The essence descriptior for the essence as parsed
			Uint32 Stream;
			Rational EditRate;
		};
		typedef SmartPtr<WrappingConfig> WrappingConfigPtr;
		typedef std::list<WrappingConfigPtr> WrappingConfigList;

		//! Select the best wrapping option
		WrappingConfigPtr SelectWrappingOption(FileHandle InFile, ParserDescriptorListPtr PDList, Rational ForceEditRate, WrappingOption::WrapType ForceWrap = WrappingOption::None);
	};
}


// GCReader and associated structures
namespace mxflib
{
	//! Base class for GCReader handlers
	/*! \note Classes derived from this class <b>must not</b> include their own RefCount<> derivation
	 */
	class GCReadHandler_Base : public RefCount<GCReadHandler_Base>
	{
	public:
		//! Base destructor
		virtual ~GCReadHandler_Base() {};

		//! Handle a "chunk" of data that has been read from the file
		/*! \return true if all OK, false on error 
		 */
		virtual bool HandleData(GCReaderPtr Caller, KLVObjectPtr Object) = 0;
	};

	// Smart pointer for the base GCReader read handler
	typedef SmartPtr<GCReadHandler_Base> GCReadHandlerPtr;

	//! Class that reads data from an MXF file
	class GCReader : public RefCount<GCReader>
	{
	protected:
		MXFFilePtr File;								//!< File from which to read
		Position FileOffset;							//!< The offset of the start of the current (or next) KLV within the file. Current KLV during HandleData() and next at other times.
		Position StreamOffset;							//!< The offset of the start of the current KLV within the data stream

		bool StopNow;									//!< True if no more KLVs should be read - set by StopReading() and ReadFromFile() with SingleKLV=true
		bool StopCalled;								//!< True if StopReading() called while processing the current KLV
		bool PushBackRequested;							//!< True if StopReading() called with PushBackKLV = true

		GCReadHandlerPtr DefaultHandler;				//!< The default handler to receive all KLVs without a specific handler
		GCReadHandlerPtr FillerHandler;					//!< The hanlder to receive all filler KLVs
		GCReadHandlerPtr EncryptionHandler;				//!< The hanlder to receive all encrypted KLVs

		std::map<Uint32, GCReadHandlerPtr> Handlers;	//!< Map of read handlers indexed by track number

	public:
		//! Create a new GCReader, optionally with a given default item handler and filler handler
		/*! \note The default handler receives all KLVs without a specific handler (except fillers)
		 *        The filler handler receives all filler KLVs
		 */
		GCReader( MXFFilePtr File, GCReadHandlerPtr DefaultHandler = NULL, GCReadHandlerPtr FillerHandler = NULL );

		//! Set the default read handler 
		/*! This handler receives all KLVs without a specific data handler assigned
		 *  including KLVs that do not appear to be standard GC KLVs.  If not default handler
		 *  is set KLVs with no specific handler will be discarded.
		 */
		void SetDefaultHandler(GCReadHandlerPtr DefaultHandler = NULL)
		{
			this->DefaultHandler = DefaultHandler;
		}

		//! Set the filler handler
		/*! If no filler handler is set all filler KLVs are discarded
		 *  \note Filler KLVs are <b>never</b> sent to the default handler
		 *        unless it is also set as the filler handler
		 */
		void SetFillerHandler(GCReadHandlerPtr FillerHandler = NULL)
		{
			this->FillerHandler = FillerHandler;
		}

		//! Set encryption handler
		/*! This handler will receive all encrypted KLVs and after decrypting them will
		 *  resubmit the decrypted version for handling using function HandleData()
		 */
		void SetEncryptionHandler(GCReadHandlerPtr EncryptionHandler = NULL)
		{
			this->EncryptionHandler = EncryptionHandler;
		}

		//! Set data handler for a given track number
		void SetDataHandler(Uint32 TrackNumber, GCReadHandlerPtr DataHandler = NULL)
		{
			if(DataHandler)
			{
				Handlers[TrackNumber] = DataHandler;
			}
			else
			{
				Handlers.erase(TrackNumber);
			}
		}

		//! Read from file - and specify a start location
		/*! All KLVs are dispatched to handlers
		 *  Stops reading at the next partition pack unless SingleKLV is true when only one KLV is dispatched
		 *  \param FilePos Location within the file to start this read
		 *  \param StreamPos Stream offset of the first KLV to be read
		 *  \param SingleKLV True if only a single KLV is to be read
		 *  \return true if all went well, false end-of-file, an error occured or StopReading() was called
		 */
		bool ReadFromFile(Position FilePos, Position StreamPos, bool SingleKLV = false)
		{
			FileOffset = FilePos;					// Record the file location
			StreamOffset = StreamPos;				// Record the stream location

			return ReadFromFile(SingleKLV);			// Then do a "continue" read
		}

		//! Read from file - continuing from a previous read
		/*! All KLVs are dispatched to handlers
		 *  Stops reading at the next partition pack unless SingleKLV is true when only one KLV is dispatched
		 *  \return true if all went well, false end-of-file, an error occured or StopReading() was called
		 */
		bool ReadFromFile(bool SingleKLV = false);

		//! Set the offset of the start of the next KLV within this GC stream
		/*! Generally this will only be called as a result of parsing a partition pack
		 *  \note The offset will start at zero and increment automatically as data is read.
		 *        If a seek is performed the offset will need to be adjusted.
		 */
		void SetStreamOffset(Position NewOffset) 
		{ 
			StreamOffset = NewOffset; 
		};

		//! Get the file offset of the next read (or the current KLV if inside ReadFromFile)
		/*! \note This is not the correct way to access the raw KLV in the file - that should be done via the KLVObject.
		 *        This function allows the caller to determine where the file pointer ended up after a read.
		 */
		Position GetFileOffset(void) { return FileOffset; }


		/*** Functions for use by read handlers ***/

		//! Force a KLVObject to be handled
		/*! \note This is not the normal way that the GCReader is used, but allows the encryption handler
		 *        to push the decrypted data back to the GCReader to pass to the appropriate handler
		 *  \return true if all OK, false on error 
		 */
		bool HandleData(KLVObjectPtr Object);

		//! Stop reading even though there appears to be valid data remaining
		/*! This function can be called from a handler if it detects that the current KLV is either the last
		 *  KLV in partition, or does not belong in this partition at all.  If the KLV belongs to another
		 *  partition, or handling should be deferred for some reason, PushBackKLV can be set to true
		 */
		void StopReading(bool PushBackKLV = false);

		//! Get the offset of the start of the current KLV within this GC stream
		Position GetStreamOffset(void) { return StreamOffset; };
	};
}


// BodyReader and associated structures
namespace mxflib
{
	//! BodyReader class - reads from an MXF file (reads data is "pulled" from the file)
	class BodyReader : public RefCount<BodyReader>
	{
	protected:
		MXFFilePtr File;						//!< File from which to read
		Position CurrentPos;					//!< Current position within file

		bool NewPos;							//!< The value of CurrentPos has been updated by a seek - therefore reading must be reinitialized!
		bool SeekInited;						//!< True once the per SID seek system has been initialized
		bool AtPartition;						//!< Are we (to our knowledge) at the start of a partition pack?
		bool AtEOF;								//!< Are we (to our knowledge) at the end of the file?

		Uint32 CurrentBodySID;					//!< The currentBodySID being processed
		
		GCReadHandlerPtr GCRDefaultHandler;		//!< Default handler to use for new GCReaders
		GCReadHandlerPtr GCRFillerHandler;		//!< Filler handler to use for new GCReaders
		GCReadHandlerPtr GCREncryptionHandler;	//!< Encryption handler to use for new GCReaders

		std::map<Uint32, GCReaderPtr> Readers;	//!< Map of GCReaders indexed by BodySID

	public:
		//! Construct a body reader and associate it with an MXF file
		BodyReader(MXFFilePtr File);

		//! Seek to a specific point in the file
		/*! \return New location or -1 on seek error
		 */
		Position Seek(Position Pos = 0);

		//! Tell the current file location
		Position Tell(void) { return CurrentPos; };

		//! Seek to a specific byte offset in a given stream
		/*! \return New file offset or -1 on seek error
		 */
		Position Seek(Uint32 BodySID, Position Pos);

		//! Set the default handler for all new GCReaders
		/*! Each time a new GCReader is created this default handler will be used if no other is specified
		 */
		void SetDefaultHandler(GCReadHandlerPtr DefaultHandler = NULL) { GCRDefaultHandler = DefaultHandler; };

		//! Set the filler handler for all new GCReaders
		/*! Each time a new GCReader is created this filler handler will be used if no other is specified
		 */
		void SetFillerHandler(GCReadHandlerPtr FillerHandler = NULL) { GCRFillerHandler = FillerHandler; };

		//! Set the encryption handler for all new GCReaders
		/*! Each time a new GCReader is created this encryption handler will be used
		 */
		void SetEncryptionHandler(GCReadHandlerPtr EncryptionHandler = NULL) { GCREncryptionHandler = EncryptionHandler; };

		//! Make a GCReader for the specified BodySID
		/*! \return true on success, false on error (such as there is already a GCReader for this BodySID)
		 */
		bool MakeGCReader(Uint32 BodySID, GCReadHandlerPtr DefaultHandler = NULL, GCReadHandlerPtr FillerHandler = NULL);

		//! Get a pointer to the GCReader used for the specified BodySID
		GCReaderPtr GetGCReader(Uint32 BodySID)
		{
			// See if we have a GCReader for this BodySID
			std::map<Uint32, GCReaderPtr>::iterator it = Readers.find(BodySID);

			// If not found return NULL
			if(it == Readers.end()) return NULL;

			// Return the pointer
			return (*it).second;
		}

		//! Read from file
		/*! All KLVs are dispatched to handlers
		 *  Stops reading at the next partition pack unless SingleKLV is true when only one KLV is dispatched
		 *  \return true if all went well, false end-of-file, an error occured or StopReading() 
		 *          was called on the current GCReader
		 */
		bool ReadFromFile(bool SingleKLV = false);

		//! Resync after possible loss or corruption of body data
		/*! Searches for the next partition pack and moves file pointer to that point
		 *  \return false if an error (or EOF found)
		 */
		bool ReSync();

		//! Are we currently at the start of a partition pack?
		bool IsAtPartition(void);

		//! Are we currently at the end of the file?
		bool Eof(void);


		/*** Functions for use by read handlers ***/

		//! Get the BodySID of the current location (0 if not known)
		Uint32 GetBodySID(void) { return CurrentBodySID; }


	protected:
		//! Initialize the per SID seek system
		/*! To allow us to seek to byte offsets within a file we need to initialize 
		 *  various structures - seeking is not always possible!!
		 *  \return False if seeking could not be initialized (perhaps because the file is not seekable)
		 */
		bool InitSeek(void);
	};

	//! Smart pointer to a BodyReader
	typedef SmartPtr<BodyReader> BodyReaderPtr;
}


//*********************************************
//**                                         **
//**        General essence functions        **
//**                                         **
//*********************************************

namespace mxflib
{
	//! Structure to hold information about each stream in a GC
	struct GCElementKind
	{
		bool	IsValid;					//!< true if this is a GC Element
		Uint8	Item;						//!< Item type - byte 13
		Uint8	Count;						//!< Element count - byte 14
		Uint8	ElementType;				//!< Element type - byte 15
		Uint8	Number;						//!< Element number - byte 16
	};

	//! Get a GCElementKind structure from a key
	GCElementKind GetGCElementKind(ULPtr TheUL);

	//! Get the track number of this essence key (if it is a GC Key)
	/*! \return 0 if not a valid GC Key
	 */
	Uint32 GetGCTrackNumber(ULPtr TheUL);
}


/* BodyWriter and related classes */

namespace mxflib
{
	//! Class holding data relating to a stream to be written by BodyWriter
	/*! Sub-streams can be added as pointers to their essence sources as this class is derived from EssenceSourceList.
	 *  Sub-streams will be written in the same generic container as this stream.
	 *  This stream's essence source will appear as the first "child" when the EssenceSourceList is scanned
	 */
	class BodyStream : public RefCount<BodyStream>, public EssenceSourceList
	{
	public:
		//! Define the action required next for this stream
		enum StateType
		{
			BodyStreamStart = 0,									//!< This stream has not yet done anything - state unknown
			BodyStreamHeadIndex,									//!< Next action: Write a "header" index table - if required in an isolated partition following the header
			BodyStreamPreBodyIndex,									//!< Next action: Write an isolated index table before the next body partition
			BodyStreamBodyWithIndex,								//!< Next action: Write a body partition with an index table
			BodyStreamBodyNoIndex,									//!< Next action: Write a body partition without index table
			BodyStreamPostBodyIndex,								//!< Next action: Write an isolated index table after a body partition
			BodyStreamFootIndex,									//!< Next action: Write a "footer" index table - if required in an isolated partition before the footer
			BodyStreamDone											//!< All done - no more actions required
		};

		//! The index table type or types of this stream
		enum IndexType
		{
			StreamIndexNone = 0,									//!< No index table will be written
			StreamIndexFullFooter = 1,								//!< A full index table will be written in the footer if possible (or an isolated partition just before the footer if another index is going to use the footer)
			StreamIndexFullFooterIsolated = 2,						//!< A full index table will be written in an isolated partition just before the footer
			StreamIndexSparseFooter = 8,							//!< A sparse index table will be written in the footer if possible (or an isolated partition just before the footer if another index is going to use the footer)
			StreamIndexSparseFooterIsolated = 16,					//!< A sparse index table will be written in an isolated partition just before the footer
			StreamIndexSprinkled = 64,								//!< A full index table will be sprinkled through the file, one chunk in each of this essence's body partitions, and one in or just before the footer
			StreamIndexSprinkledIsolated = 128,						//!< A full index table will be sprinkled through the file, one chunk in an isolated partition following each of this essence's body partitions
			StreamIndexCBRHeader = 256,								//!< A CBR index table will be written in the header (or an isolated partition following the header if another index table exists in the header)
			StreamIndexCBRHeaderIsolated = 512,						//!< A CBR index table will be written in an isolated partition following the header
			StreamIndexCBRFooter = 1024,							//!< A CBR index table will be written in the footer if possible (or an isolated partition just before the footer if another index is going to use the footer)
			StreamIndexCBRFooterIsolated = 2048,					//!< A CBR index table will be written in an isolated partition just before the footer
			StreamIndexCBRBody = 4096,								//!< A CBR index table will be written in each body partition for this stream
			StreamIndexCBRIsolated = 8192,							//!< A CBR index table will be written in an isolated body partition following each partition of this stream
			StreamIndexCBRPreIsolated = 16384						//!< A CBR index table will be written in an isolated body partition before each partition of this stream
		};

		//! Wrapping types for streams
		enum WrapType
		{
			StreamWrapOther = 0,									//!< Other non-standard wrapping types - the essence source will supply one KLVs worth at a time (??)
			StreamWrapFrame,										//!< Frame wrapping
			StreamWrapClip											//!< Clip wrapping
		};

	protected:
		EssenceSourcePtr Source;									//!< The essence source for this stream
		EssenceSourceList SubStreams;								//!< Sources for each sub-stream
		EssenceSourceList::iterator SubStream_it;					//!< Current sub-stream
		bool SubStreamRestart;										//!< Flag true when the sub-stream iterator needs moving to the top of the list next time

		StateType State;											//!< The state of this stream

		IndexType StreamIndex;										//!< The index type(s) of this stream
		IndexType FooterIndexFlags;									//!< Set of flags for tracking footer index tables

		Uint32 BodySID;												//!< BodySID to use for this stream
		Uint32 IndexSID;											//!< IndexSID to use for indexing this stream

		WrapType StreamWrap;										//!< The wrapping type of this stream

		GCWriterPtr StreamWriter;									//!< The writer for this stream

		bool EssencePendingData;									//!< Is there any essence data pending in the writer?

		bool EndOfStream;											//!< No more essence available for this stream

		IndexManagerPtr IndexMan;									//!< The index manager for this stream
		
		Position NextSprinkled;										//!< The location of the first edit-unit to use for the next sprinkled index segment

		//! KLV Alignment Grid to use for this stream (of zero if default for this body is to be used)
		Uint32 KAG;

		//! Flag set if BER lengths for this stream should be forced to 4-byte (where possible)
		bool ForceBER4;

		//! Flag set if partitioning is to be done only on edit boundaries
		/*! \note Only the master stream is (currently) edit aligned, not all sub-streams */
		bool EditAlign;

		//! Prevent NULL construction
		BodyStream();

		//! Prevent copy construction
		BodyStream(BodyStream &);

	/* Public properties */
	public:
		std::list<Position> SparseList;								//!< List of edit units to include in sparse index tables

	public:
		//! Construct an body stream object with a given essence source
		BodyStream(Uint32 SID, EssenceSourcePtr &EssSource)
		{
			BodySID = SID;
			IndexSID = 0;
			Source = EssSource;
			State = BodyStreamStart;
			StreamIndex = StreamIndexNone;
			FooterIndexFlags = StreamIndexNone;
			StreamWrap = StreamWrapOther;
			SubStreamRestart = true;
			NextSprinkled = 0;
			EssencePendingData = false;
			EndOfStream = false;

			KAG = 0;
			ForceBER4 = false;
			EditAlign = false;

			// Set the master stream as one of the essence streams
			push_back(Source);
		}

		//! Get the essence source for this stream
		EssenceSourcePtr &GetSource(void) { return Source; }

		//! Get the number of sub-streams (includes the master stream)
		size_type SubStreamCount(void) { return size(); }

		//! Add a new sub-stream
		void AddSubStream(EssenceSourcePtr &SubSource) 
		{ 
			// Add the new stream
			push_back(SubSource); 

			// If the writer has already been defined add this stream to it
			if(StreamWriter)
			{
				GCStreamID EssenceID = StreamWriter->AddEssenceElement(SubSource->GetGCEssenceType(), SubSource->GetGCElementType());

				SubSource->SetStreamID(EssenceID);

				// Also add to the index manager (if we have one)
				if(IndexMan)
				{
					// TODO: Sort the PosTable!!
					int StreamID = IndexMan->AddSubStream(0, SubSource->GetBytesPerEditUnit() );
					SubSource->SetIndexManager(IndexMan, StreamID);

					// TODO: Currently no support for filler indexing here - needs adding
					StreamWriter->AddStreamIndex(EssenceID, IndexMan, StreamID, false);
				}
			}
		}

		//! Get this stream's BodySID
		Uint32 GetBodySID(void) { return BodySID; }

		//! Set this stream's IndexSID
		void SetIndexSID(Uint32 SID) { IndexSID = SID; }

		//! Get this stream's IndexSID
		Uint32 GetIndexSID(void) { return IndexSID; }

		//! Set the stream's state
		void SetState(StateType NewState) { State = NewState; }

		//! Get the current state
		StateType GetState(void) 
		{
			if(State == BodyStreamStart) GetNextState();
			return State;
		}

		//! Get the next state
		/*! Sets State to the next state
		 *  \return The next state (now the current state)
		 */
		StateType GetNextState(void);

		//! Set the index type(s) to the desired value
		/*! \note This sets the complete value, it doesn't just add an option - to add "X" use SetIndexType(GetIndexType() | "X");
		 */
		void AddIndexType(IndexType NewIndexType) { StreamIndex = (IndexType) (StreamIndex | NewIndexType); }

		//! Set the index type(s) to the desired value
		/*! \note This sets the complete value, it doesn't just add an option - to add "X" use AddIndexType()
		 */
		void SetIndexType(IndexType NewIndexType) { StreamIndex = NewIndexType; }

		//! Get the index type(s)
		IndexType GetIndexType(void) { return StreamIndex; }

		//! Set the footer index flags to the desired value
		/*! \note This sets the complete value, it doesn't just add an option - to add "X" use SetFooterIndex(GetFooterIndex() | "X");
		 */
		void SetFooterIndex(IndexType NewIndexType) { FooterIndexFlags = NewIndexType; }

		//! Get the footer index flags
		IndexType GetFooterIndex(void) { return FooterIndexFlags; }

		//! Set the wrapping type for this stream
		void SetWrapType(WrapType NewWrapType) { StreamWrap = NewWrapType; }

		//! Get the wrapping type of this stream
		WrapType GetWrapType(void) { return StreamWrap; }

		//! Set the current GCWriter
		void SetWriter(GCWriterPtr &Writer) 
		{ 
			// Check that we haven't tried to add two writers
			if(StreamWriter)
			{
				error("BodyStream::SetWriter called - but this stream already has a GCWriter\n");
				return;
			}

			// Set the writer
			StreamWriter = Writer; 

			// Add each existing stream to the new writer
			BodyStream::iterator it = begin();
			while(it != end())
			{
				GCStreamID EssenceID = Writer->AddEssenceElement((*it)->GetGCEssenceType(), (*it)->GetGCElementType());
				
				(*it)->SetStreamID(EssenceID);

				// Also add to the index manager (if we have one)
				if(IndexMan)
				{
					// TODO: Sort the PosTable!!
					int StreamID = IndexMan->AddSubStream(0, (*it)->GetBytesPerEditUnit() );
					(*it)->SetIndexManager(IndexMan, StreamID);

					// TODO: Currently no support for filler indexing here - needs adding
					Writer->AddStreamIndex(EssenceID, IndexMan, StreamID, false);
				}

				it++;
			}
		}

		//! Get the current index manager
		IndexManagerPtr &GetIndexManager(void) 
		{ 
			if(!IndexMan) InitIndexManager();
			return IndexMan; 
		}

		//! Get a reference to the current GCWriter
		GCWriterPtr &GetWriter(void) { return StreamWriter; }

		//! Get the track number associated with this stream
		Uint32 GetTrackNumber(void) 
		{ 
			if(!Source) return 0;
			return StreamWriter->GetTrackNumber(Source->GetStreamID());
		}

		//! Set the pending essence data flag
		void SetPendingData(bool Value = true) { EssencePendingData = Value; }

		//! Find out if there is any essence data stored in the GCWriter pending a write
		bool HasPendingData(void) { return EssencePendingData; }
		
		//! Set the EndOfStream flag
		void SetEndOfStream(bool Value = true) { EndOfStream = Value; }

		//! Find out if there is any essence data remaining for this stream
		bool GetEndOfStream(void) { return EndOfStream; }
		
		//! Set the first edit unit for the next sprinkled index segment
		void SetNextSprinkled(Position Sprinkled) { NextSprinkled = Sprinkled; }

		//! Get the first edit unit for the next sprinkled index segment
		Position GetNextSprinkled(void) { return NextSprinkled; }

		//! Set the KLV Alignment Grid
		void SetKAG(Uint32 NewKAG) { KAG = NewKAG; }

		//! Get the KLV Alignment Grid
		Uint32 GetKAG(void) { return KAG; }

		//! Set flag if BER lengths should be forced to 4-byte (where possible)
		void SetForceBER4(bool Force) { ForceBER4 = Force; }

		//! Get flag stating whether BER lengths should be forced to 4-byte (where possible)
		bool GetForceBER4(void) { return ForceBER4; }

		//! Set edit align forced partitioning flag
		void SetEditAlign(bool Align) { EditAlign = Align; }

		//! Get edit align forced partitioning flag
		bool GetEditAlign(void) { return EditAlign; }

	protected:
		//! Initialize an index manager if required
// FIXME: Move to .cpp file!
		void InitIndexManager(void)
		{
			// Don't init if no indexing required
			if(StreamIndex == StreamIndexNone) return;

			// Don't init if no IndexSID set
			if(IndexSID == 0) return;

			// Don't init if no writer
			if(!StreamWriter) return;

			// Don't init if already done
			if(IndexMan) return; 

			// Add each stream
			BodyStream::iterator it = begin();
			while(it != end())
			{
				// Add to the index manager (create the index manager on first pass)
				// TODO: Sort the PosTable!!
				int StreamID = 0;
				if(!IndexMan)
				{
					IndexMan = new IndexManager(0, (*it)->GetBytesPerEditUnit());
					IndexMan->SetBodySID(BodySID);
					IndexMan->SetIndexSID(IndexSID);
					IndexMan->SetEditRate((*it)->GetEditRate());
				}
				else
					StreamID = IndexMan->AddSubStream(0, (*it)->GetBytesPerEditUnit());

				// Let the stream know about this index manager
				(*it)->SetIndexManager(IndexMan, StreamID);

				// TODO: Currently no support for filler indexing here - needs adding
				StreamWriter->AddStreamIndex((*it)->GetStreamID(), IndexMan, StreamID, false);

				it++;
			}
		}
	};

	//! Smart pointer to a BodyStream
	typedef SmartPtr<BodyStream> BodyStreamPtr;

	// Forward declare BodyWriterPtr to allow it to be used in BodyWriterHandler
	class BodyWriter;
	typedef SmartPtr<BodyWriter> BodyWriterPtr;

	//! Base class for partition candler callbacks
	class BodyWriterHandler : public RefCount<BodyWriterHandler>
	{
	public:
		//! Virtual destructor to allow polymorphism
		virtual ~BodyWriterHandler();

		//! Handler called before writing a partition pack
		/*! \param Caller - A pointer to the calling BodyWriter
		 *  \param BodySID - The Stream ID of the essence in this partition (0 if none)
		 *  \param IndexSID - The Stream ID of the index data in this partition (0 if none)
		 *  \note If metadata is to be written the partition type must be set accordingly by the handler - otherwise closed and complete will be used
		 *  \note If metadata is requested but the partition will contain index or essence data that is not permitted to share a partition
		 *        with metadata an extra partition pack will be written with no metadata after writing the metadata
		 *  \return true if metadata should be written with this partition pack
		 */
		virtual bool HandlePartition(BodyWriterPtr Caller, Uint32 BodySID, Uint32 IndexSID) = 0;
	};

	//! Smart pointer to a BodyWriterHandler
	typedef SmartPtr<BodyWriterHandler> BodyWriterHandlerPtr;


	//! Body writer class - manages multiplexing of essence
	class BodyWriter : public RefCount<BodyWriter>
	{
	protected:
		// States for BodyWriter
		enum BodyState
		{
			BodyStateStart = 0,										//!< The BodyWriter has not yet started writing
			BodyStateHeader,										//!< Writing the header (and/or post header indexes)
			BodyStateBody,											//!< Writing the body essence and indexes
			BodyStateFooter,										//!< Writing the footer (and/or pre-footer indexes or RIP)
			BodyStateDone											//!< All done
		};

		//! The state for this writer
		BodyState State;

		//! Class for holding info relating to a stream
		/*! This class holds medium-term info about a stream in comparision to BodyStream which holds
		 *  long-term info. This is because odd interleaving may cause a stream to be added and removed
		 *  from the writer during the course of the file.  Data that needs to survive through the whole
		 *  file lives in BodyStream and data relating to this phase lives in StreamInfo.
		 *  \note This class has public internals because it is basically a glorified struct!
		 */
		class StreamInfo : public RefCount<StreamInfo>
		{
		public:
			bool Active;											//!< True if active - set false once finished
			BodyStreamPtr Stream;									//!< The stream in question
			Length StopAfter;										//!< Number of edit units to output (or zero for no limit). Decremented each time data is written (unless zero).

		public:
			//! Construct an "empty" StreamInfo
			StreamInfo() { Active = false; }

			//! Copy constructor
			StreamInfo(const StreamInfo &rhs)
			{
				Active = rhs.Active;
				Stream = rhs.Stream;
				StopAfter = rhs.StopAfter;
			}
		};

		//! Smart pointer to a StreamInfo
		typedef SmartPtr<StreamInfo> StreamInfoPtr;

		//! Type for list of streams to write
		/*! The list is kept in the order that the BodySIDs are added
		 */
		typedef std::list<StreamInfoPtr> StreamInfoList;

		//! Destination file
		MXFFilePtr File;

		//! List of streams to write
		StreamInfoList StreamList;

		//! KLV Alignment Grid to use
		Uint32 KAG;

		//! Flag set if BER lengths should be forced to 4-byte (where possible)
		bool ForceBER4;

		//! Partition pack to use when one is required
		PartitionPtr BasePartition;

		BodyWriterHandlerPtr PartitionHandler;					//!< The body partition handler

		Uint32 MinPartitionSize;								//!< The minimum size of the non-essence part of the next partition
		Uint32 MinPartitionFiller;								//!< The minimum size of filler before the essence part of the next partition

		bool IndexSharesWithMetadata;								//!< If true index tables may exist in the same partition as metadata
		bool EssenceSharesWithMetadata;								//!< If true essence may exist in the same partition as metadata

		//! The current BodySID, or 0 if not known (will move back to the start of the list)
		Uint32 CurrentBodySID;

		//! The current partition is done and must not be continued - any new data must start a new partition
		bool PartitionDone;

		//! Iterator for the current (or previous) stream data. Only valid if CurrentBodySID != 0
		StreamInfoList::iterator CurrentStream;


		/* Details about the pending partition, set but not yet written
		 * This is because the value of BodySID depends on whether any
		 * essence is going to be written in this partition which won't
		 * be known for certain until we are about to write the essence
		 */

		//! Flag set when a partition pack is ready to be written
		bool PartitionWritePending;

		//! Is the pending metadata a header?
		bool PendingHeader;

		//! Is the pending metadata a footer?
		bool PendingFooter;

		//! Is the next partition write going to have metadata?
		bool PendingMetadata;

		//! Pointer to a chunk of index table data for the pendinf partition or NULL if none is required
		DataChunkPtr PendingIndexData;

		//! BodySID of the essence or index data already written or pending for this partition
		/*! This is used to determine if particular essence can be added to this partition.
		 *  Set to zero if none yet written.
		 */
		Uint32 PartitionBodySID;

		//! Prevent NULL construction
		BodyWriter();

		//! Prevent copy construction
		BodyWriter(BodyWriter &);

	public:
		//! Construct a body writer for a specified file
		BodyWriter(MXFFilePtr &DestFile)
		{
			State = BodyStateStart;
			CurrentBodySID = 0;
			PartitionDone = false;

			File = DestFile;

			// By default index tables may share with metadata, but not essence
			IndexSharesWithMetadata = true;
			EssenceSharesWithMetadata = false;

			KAG = 0;
			ForceBER4 = false;

			MinPartitionSize = 0;
			MinPartitionFiller = 0;

			PartitionWritePending = false;
			PendingHeader = 0;
			PendingFooter = 0;
			PendingMetadata = false;
			PartitionBodySID = 0;
		}

		//! Clear any stream details ready to call AddStream()
		/*! This allows previously used streams to be removed before a call to WriteBody() or WriteNext()
		 */
		void ClearStreams(void) { StreamList.clear(); CurrentBodySID = 0; }

		//! Add a stream to the list of those to write
		/*! \param Stream - The stream to write
		 *  \param StopAfter - If > 0 the writer will stop writing this stream at the earliest opportunity after (at least) this number of edit units have been written
		 *  Streams will be written in the order that they were offered and the list is kept in this order.
		 *	\return false if unable to add this stream (for example this BodySID already in use)
		 */
		bool AddStream(BodyStreamPtr &Stream, Length StopAfter = 0);

		//! Set the KLV Alignment Grid
		void SetKAG(Uint32 NewKAG) { KAG = NewKAG; }

		//! Get the KLV Alignment Grid
		Uint32 GetKAG(void) { return KAG; }

		//! Set flag if BER lengths should be forced to 4-byte (where possible)
		void SetForceBER4(bool Force) { ForceBER4 = Force; }

		//! Get flag stating whether BER lengths should be forced to 4-byte (where possible)
		bool GetForceBER4(void) { return ForceBER4; }

		//! Set what sort of data may share with header metadata
		void SetMetadataSharing(bool IndexMayShare = true, bool EssenceMayShare = false)
		{
			IndexSharesWithMetadata = IndexMayShare;
			EssenceSharesWithMetadata = EssenceMayShare;
		}

		//! Set the template partition pack to use when partition packs are required
		/*! The byte counts and SIDs will be updated are required before writing.
		 *  FooterPosition will not be updated so it must either be 0 or the correct value.
		 *  Any associated metadata will be written for the header and if the handler (called just before the write) requests it.
		 *  \note The original object given will be modified - not a copy of it
		 */
		void SetPartition(PartitionPtr &ThePartition) { BasePartition = ThePartition; }

		//! Get a pointer to the current template partition pack
		PartitionPtr GetPartition(void) { return BasePartition; }

		//! Write the file header
		/*! No essence will be written, but CBR index tables will be written if required.
		 *  The partition will not be "ended" if only the header partition is written
		 *  meaning that essence will be added by the next call to WritePartition()
		 */
		void WriteHeader(bool IsClosed, bool IsComplete);

		//! End the current partition
		/*! Once "ended" no more essence will be added, even if otherwise valid.
		 *  A new partition will be started by the next call to WritePartition()
		 *  \note This function will also flush any pending partition writes
		 */
		void EndPartition(void);

		//! Write stream data
		/*! \param Duration If > 0 the stop writing at the earliest opportunity after (at least) this number of edit units have been written for each stream
		 *  \param MaxPartitionSize If > 0 the writer will attempt to keep the partition no larger than this size in bytes. There is no guarantee that it will succeed
		 *  \note Streams that have finished or hit thier own StopAfter value will be regarded as having written enough when judging whether to stop
		 */
		void WriteBody(Length Duration = 0, Length MaxPartitionSize = 0);

		//! Write the next partition or continue the current one (if not complete)
		/*! Will stop at the point where the next partition will start, or (if Duration > 0) at the earliest opportunity after (at least) Duration edit units have been written
		 */
		Length WritePartition(Length Duration = 0, Length MaxPartitionSize = 0);

		//! Determine if all body partitions have been written
		/*! Will be false until after the last required WritePartition() call
		 */
		bool BodyDone(void) { return (State == BodyStateFooter) || (State == BodyStateDone); }

		//! Write the file footer
		/*! No essence will be written, but index tables will be written if required.
		 */
		void WriteFooter(bool WriteMetadata = false, bool IsComplete = true);

		//! Set a handler to be called before writing a partition pack within the body
		/*! Will be called before a body partition is written
		 */
		void SetPartitionHandler(BodyWriterHandlerPtr &NewBodyHandler) { PartitionHandler = NewBodyHandler; }

		//! Set the minumum size of the non-essence part of the next partition
		/*! This will cause a filler KLV to be added (if required) after the partition pack, any header metadata and index table segments
		 *  in order to reach the specified size.  This is useful for reserving space for future metadata updates.
		 *  This value is read after calling the partition handlers so this function may safely be used in the handlers.
		 *  \note The size used will be the minimum size that satisfies the following:
		 *  - All required items are included (partition pack, metadata if required, index if required)
		 *  - The total size, excluding essence, is at least as big as the value specified by SetPartitionSize()
		 *  - The filler following the last non-essence item is at least as big as the value specified by SetPartitionFiller()
		 *  - The KAGSize value is obeyed
		 */
		void SetPartitionSize(Uint32 PartitionSize) { MinPartitionSize = PartitionSize; }

		//! Set the minumum size of filler between the non-essence part of the next partition and any following essence
		/*! If non-zero this will cause a filler KLV to be added after the partition pack, any header metadata and index table segments
		 *  of at least the size specified.  This is useful for reserving space for future metadata updates.
		 *  This value is read after calling the partition handlers so this function may safely be used in the handlers.
		 *  \note The size used will be the minimum size that satisfies the following:
		 *  - All required items are included (partition pack, metadata if required, index if required)
		 *  - The total size, excluding essence, is at least as big as the value specified by SetPartitionSize()
		 *  - The filler following the last non-essence item is at least as big as the value specified by SetPartitionFiller()
		 *  - The KAGSize value is obeyed
		 */
		void SetPartitionFiller(Uint32 PartitionFiller) { MinPartitionFiller = PartitionFiller; }

	protected:
		//! Move to the next active stream (will also advance State as required)
		/*! \note Will set CurrentBodySID to 0 if no more active streams
		 */
		void SetNextStream(void);

		//! Write a complete partition's worth of essence
		/*! Will stop if:
		 *    Frame or "other" wrapping and the "StopAfter" reaches zero or "Duration" reaches zero
		 *    Clip wrapping and the entire clip is wrapped
		 */
		Length WriteEssence(StreamInfoPtr &Info, Length Duration = 0, Length MaxPartitionSize = 0);
	};

	//! Smart pointer to a BodyWriter
	typedef SmartPtr<BodyWriter> BodyWriterPtr;
}

#endif // MXFLIB__ESSENCE_H
