/*! \file	essence.h
 *	\brief	Definition of classes that handle essence reading and writing
 *
 *	\version $Id: essence.h,v 1.2.2.4 2004/05/19 11:10:31 matt-beard Exp $
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
}


namespace mxflib
{
	//! Abstract super-class for objects that supply large quantities of essence data
	/*! This is used when clip-wrapping to prevent large quantities of data being loaded into memory 
	/*! \note Classes derived from this class <b>must not</b> include their own RefCount<> derivation
	 */
	class EssenceSource : RefCount<EssenceSource>
	{
	public:
		//! Get the size of the essence data in bytes
		/*! \note There is intentionally no support for an "unknown" response */
		virtual Uint64 GetEssenceDataSize(void) = 0;

		//! Get the next "installment" of essence data
		/*! \ret Pointer to a data chunk holding the next data or a NULL pointer when no more remains
		 *	\note If there is more data to come but it is not currently available the return value will be a pointer to an empty data chunk
		 *	\note If Size = 0 the object will decide the size of the chunk to return
		 *	\note On no account will the returned chunk be larger than MaxSize (if MaxSize > 0)
		 */
		virtual DataChunkPtr GetEssenceData(Uint64 Size = 0, Uint64 MaxSize = 0) = 0;

		virtual ~EssenceSource() { };
	};
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
		bool CountFixed;					//!< True once the essence element count has been fixed
											/*!< The count is fixed the first time either a key is written
											 *   or a track number is reported */
		Uint32 WriteOrder;					//!< The (default) write order for this stream
											/*!< Elements with a lower WriteOrder are written first when the
											 *   content package is written */
	};

	// Type used to identify stream
	typedef int GCStreamID;

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

		Uint64 StreamOffset;				//!< Current stream offset within this essence container

	public:
		//! Constructor
		GCWriter(MXFFilePtr File, Uint32 BodySID = 0, int Base = 0);

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

		//! Add system item data to the current CP
		void AddSystemData(GCStreamID ID, Uint64 Size, const Uint8 *Data);

		//! Add system item data to the current CP
		void AddSystemData(GCStreamID ID, DataChunkPtr Chunk) { AddSystemData(ID, Chunk->Size, Chunk->Data); }

		//! Add encrypted system item data to the current CP
		void AddSystemData(GCStreamID ID, Uint64 Size, const Uint8 *Data, UUIDPtr ContextID, Uint64 PlaintextOffset = 0);
		
		//! Add encrypted system item data to the current CP
		void AddSystemData(GCStreamID ID, DataChunkPtr Chunk, UUIDPtr ContextID, Uint64 PlaintextOffset = 0) { AddSystemData(ID, Chunk->Size, Chunk->Data, ContextID, PlaintextOffset); }

		//! Add essence data to the current CP
		void AddEssenceData(GCStreamID ID, Uint64 Size, const Uint8 *Data);

		//! Add essence data to the current CP
		void AddEssenceData(GCStreamID ID, DataChunkPtr Chunk) { AddEssenceData(ID, Chunk->Size, Chunk->Data); }

		//! Add essence data to the current CP
		void AddEssenceData(GCStreamID ID, EssenceSource* Source);

		//! Add encrypted essence data to the current CP
		void AddEssenceData(GCStreamID ID, Uint64 Size, const Uint8 *Data, UUIDPtr ContextID, Uint64 PlaintextOffset = 0);
		
		//! Add encrypted essence data to the current CP
		void AddEssenceData(GCStreamID ID, DataChunkPtr Chunk, UUIDPtr ContextID, Uint64 PlaintextOffset = 0)  { AddEssenceData(ID, Chunk->Size, Chunk->Data, ContextID, PlaintextOffset); }

		//! Add encrypted essence data to the current CP
		void AddEssenceData(GCStreamID ID, EssenceSource* Source, UUIDPtr ContextID, Uint64 PlaintextOffset = 0);

		//#### Register an Encryption thingy...



		//! Structure for items to be written
		struct WriteBlock
		{
			Uint64 Size;				//! Number of bytes of data to write
			Uint8 *Buffer;				//! Pointer to bytes to write
			EssenceSource *Source;		//! Pointer to an EssenceSource object or NULL
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

	class WrappingOption : public RefCount<WrappingOption>
	{
	public:
		//! Wrapping type
		/*! \note "None" is only for use as a default condition */
		enum WrapType { None, Frame, Clip, Line, Other } ;

		EssenceSubParserBase *Handler;			//!< Pointer to the object that can parse this wrapping option
		std::string Description;				//!< Human readable description of this wrapping option (to allow user selection)
		ULPtr	WrappingUL;						//!< UL for this wrapping
		ULList	RequiredPartners;				//!< List of other items that *MUST* accompany this item to use this wrapping
		Uint8	GCEssenceType;					//!< The Generic Container essence type, or 0 if not a GC wrapping
		Uint8	GCElementType;					//!< The Generic Container element value, or 0 if not a GC wrapping
		WrapType	ThisWrapType;				//!< The type of this wrapping (frame, clip etc.)
		bool	CanSlave;						//!< True if this wrapping can be a "slave" which allows it to be used at a different edit rate than its own
		bool	CanIndex;						//!< True if this wrapping can be indexed by the handler
		bool	CBRIndex;						//!< True if this wrapping will use a CBR index table
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
	class EssenceSubParserBase
	{
	protected:
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
			EssenceSubParserBase *Caller;
			FileHandle File;
			Uint32 Stream;
			Uint64 RequestedCount;
			IndexTablePtr Index;
			DataChunkPtr RemainingData;
			bool Started;

		public:
			//! Construct and initialise for essence parsing/sourcing
			ESP_EssenceSource(EssenceSubParserBase *TheCaller, FileHandle InFile, Uint32 UseStream, Uint64 Count = 1 /*, IndexTablePtr UseIndex = NULL*/)
			{
				Caller = TheCaller;
				File = InFile;
				Stream = UseStream;
				RequestedCount = Count;
				/* Index = UseIndex; */
				Started = false;
			};

			//! Get the next "installment" of essence data
			/*! \ret Pointer to a data chunk holding the next data or a NULL pointer when no more remains
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
					Data = Caller->Read(File, Stream, 1 /*, Index*/);
				}
				if(Data)
				{
					if(Data->Size == 0) Data = NULL;
					else
					{
						if((MaxSize) && (Data->Size > MaxSize))
						{
							RemainingData = new DataChunk(Data->Size - MaxSize, &Data->Data[MaxSize]);
							Data->Resize(MaxSize);
						}
					}
				}

				return Data;
			}
		};

	protected:

	public:
		//! Base destructor
		virtual ~EssenceSubParserBase() {};

		//! Build a new parser of this type and return a pointer to it
		virtual EssenceSubParserBase *NewParser(void) const = 0;

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
		virtual void Use(Uint32 Stream, WrappingOptionPtr UseWrapping)
		{
		}

		//! Set a non-native edit rate
		/*! \return true if this rate is acceptable */
		virtual bool SetEditRate(Uint32 Stream, Rational EditRate)
		{
			return false;
		}

		//! Get BytesPerEditUnit, if Constant
		virtual Uint32 GetBytesPerEditUnit() { return 0; }

		//! Get the current position in SetEditRate() sized edit units
		/*! \return 0 if position not known
		 */
		virtual Int64 GetCurrentPosition(void)
		{
			return 0;
		}

		//! Set the IndexManager for this essence stream (and the stream ID if we are not the main stream)
		virtual void SetIndexManager(IndexManagerPtr TheManager, int StreamID = 0)
		{
			Manager = TheManager;
			ManagedStreamID = StreamID;
		}

		//! Get the IndexManager for this essence stream
		virtual IndexManagerPtr GetIndexManager(void) { return Manager; };

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


		//! Read a number of wrapping items from the specified stream and return them in a data chunk
		/*! If frame or line mapping is used the parameter Count is used to
		 *	determine how many items are read. In frame wrapping it is in
		 *	units of EditRate, as specified in the call to Use(), which may
		 *  not be the frame rate of this essence
		 *	\note This is going to take a lot of memory in clip wrapping! 
		 */
		virtual DataChunkPtr Read(FileHandle InFile, Uint32 Stream, Uint64 Count = 1 /*, IndexTablePtr Index = NULL*/) = 0;

		//! Build an EssenceSource to read a number of wrapping items from the specified stream
		virtual ESP_EssenceSource *GetEssenceSource(FileHandle InFile, Uint32 Stream, Uint64 Count = 1 /*, IndexTablePtr Index = NULL*/) = 0;

		//! Write a number of wrapping items from the specified stream to an MXF file
		/*! If frame or line mapping is used the parameter Count is used to
		 *	determine how many items are read. In frame wrapping it is in
		 *	units of EditRate, as specified in the call to Use(), which may
		 *  not be the frame rate of this essence stream
		 *	\note This is the only safe option for clip wrapping
		 *	\return Count of bytes transferred
		 */
		virtual Uint64 Write(FileHandle InFile, Uint32 Stream, MXFFilePtr OutFile, Uint64 Count = 1 /*, IndexTablePtr Index = NULL*/) = 0;

		//! Set a parser specific option
		/*! \return true if the option was successfully set */
		virtual bool SetOption(std::string Option, Int64 Param = 0) { return false; } ;

	};
}


namespace mxflib
{
	//! Pair containing a pointer to an essence parser and its associated essence descriptors
	typedef std::pair<EssenceSubParserBase*, EssenceStreamDescriptorList> ParserDescriptorPair;

	//! List of pairs of essence parser pointers with associated file descriptors
	/*! \note when the list is destroyed all parsers
	 *        in the list are deleted so if a copy is 
	 *        taken that will out-last the list it 
	 *		  must be removed from the list 
	 */
	class ParserDescriptorList : public RefCount<ParserDescriptorList>, public std::list<ParserDescriptorPair>
	{
	public:
		// Destructor deletes all owned parsers
		~ParserDescriptorList()
		{
			iterator it = begin();
			while(it != end())
			{
				delete (*it).first;
				it++;
			}
		}
	};
	typedef SmartPtr<ParserDescriptorList> ParserDescriptorListPtr;


	class EssenceParser
	{
	private:
		//! List of pointers to essence parsers
		/*! \note when the list is destroyed all parsers
		 *        in the list are deleted so if a copy is 
		 *        taken that will out-last the list it 
		 *		  must be removed from the list 
		 */
		class EssenceParserList : public std::list<EssenceSubParserBase*>
		{
		public:
			// Destructor deletes all owned parsers
			~EssenceParserList()
			{
				iterator it = begin();
				while(it != end())
				{
					delete (*it);
					it++;
				}
			}
		};

		//! List of pointers to known parsers
		/*! Used only for building parsers to parse essence - the parses 
		 *  in this list must not themselves be used for essence parsing 
		 *
		 *	DRAGONS: Should this be static?
		 */
		EssenceParserList EPList;

	public:
		// Build an essence parser with all known sub-parsers
		EssenceParser();


		// Build a list of parsers with their descriptors for a given essence file
		ParserDescriptorListPtr IdentifyEssence(FileHandle InFile)
		{
			ParserDescriptorListPtr Ret = new ParserDescriptorList;
		
			EssenceParserList::iterator it = EPList.begin();
			while(it != EPList.end())
			{
				EssenceSubParserBase *EP = (*it)->NewParser();
				EssenceStreamDescriptorList DescList = EP->IdentifyEssence(InFile);
				
				if(DescList.empty())
				{
					delete EP;
				}
				else
				{
					Ret->push_back(ParserDescriptorPair(EP, DescList));
				}

				it++;
			}

			return Ret;
		}

		class WrappingConfig : public RefCount<WrappingConfig>
		{
		public:
			WrappingOptionPtr WrapOpt;
			MDObjectPtr EssenceDescriptor;
			Uint32 Stream;
			Rational EditRate;
		};
		typedef SmartPtr<WrappingConfig> WrappingConfigPtr;
		typedef std::list<WrappingConfigPtr> WrappingConfigList;

		// DRAGONS: Currently destroys PDList to preserve the essence handler
		WrappingConfigPtr SelectWrappingOption(FileHandle InFile, ParserDescriptorListPtr PDList, Rational ForceEditRate, WrappingOption::WrapType ForceWrap = WrappingOption::None)
		{
			WrappingConfigPtr Ret;

			// No options!
			if(PDList->empty()) return Ret;

			// Identify the wrapping options for each descriptor
			ParserDescriptorList::iterator pdit = PDList->begin();
			while(pdit != PDList->end())
			{
				EssenceStreamDescriptorList::iterator it = (*pdit).second.begin();
				while(it != (*pdit).second.end())
				{
					WrappingOptionList WO = (*pdit).first->IdentifyWrappingOptions(InFile, (*it));

					WrappingOptionList::iterator it2 = WO.begin();
					while(it2 != WO.end())
					{
						// Only accept wrappings of the specified type
						if(ForceWrap != WrappingOption::None)
						{
							if((*it2)->ThisWrapType != ForceWrap)
							{
								it2++;
								continue;
							}
						}

						Ret = new WrappingConfig;

						// DRAGONS: Default to the first valid option!
						Ret->EssenceDescriptor = (*it).Descriptor;
						MDObjectPtr Ptr = Ret->EssenceDescriptor["SampleRate"];

						if((!Ptr) || (ForceEditRate.Numerator != 0))
						{
							Ret->EditRate.Numerator = ForceEditRate.Numerator;
							Ret->EditRate.Denominator = ForceEditRate.Denominator;
						}
						else
						{
							std::string Rate = Ptr->GetString();
							Ret->EditRate.Numerator = Ptr->GetInt("Numerator");
							Ret->EditRate.Denominator = Ptr->GetInt("Denominator");
						}

						Ret->WrapOpt = (*it2);
						Ret->Stream = (*it).ID;

						Ret->WrapOpt->Handler->Use(Ret->Stream, Ret->WrapOpt);
						if( Ret->WrapOpt->Handler->SetEditRate(0, Ret->EditRate) )
						{
							// All OK, including requested edit rate
							
							Ret->WrapOpt->BytesPerEditUnit = Ret->WrapOpt->Handler->GetBytesPerEditUnit();

							// Remove all entries that index this handler to prevent it being deleted
							ParserDescriptorList::iterator pdit2 = PDList->begin();
							while(pdit2 != PDList->end())
							{
								if((*pdit2).first == Ret->WrapOpt->Handler)
								{
									pdit2 = PDList->erase(pdit2);
								}
							}

							return Ret;
						}

						// We failed to match - scrub the part made config
						Ret = NULL;

						it2++;
					}
					it++;
				}
				pdit++;
			}

			return Ret;
		}
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
		virtual ~GCReadHandler_Base();

		//! Handle a "chunk" of data that has been read from the file
		/*! \return true if all OK, false on error 
		 */
		virtual bool HandleData(GCReaderPtr Caller, KLVObjectPtr Object) = 0;
	};

	// Smart pointer for the base GCReader read handler
	typedef SmartPtr<GCReadHandler_Base> GCReadHandlerPtr;

	//! Class that reads data from an MXF file
	class GCReader
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
		/*        to push the decrypted data back to the GCReader to pass to the appropriate handler
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
		bool Resync();

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
}



#endif // MXFLIB__ESSENCE_H
