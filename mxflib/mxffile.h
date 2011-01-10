/*! \file	mxffile.h
 *	\brief	Definition of MXFFile class
 *
 *			The MXFFile class holds data about an MXF file, either loaded 
 *          from a physical file or built in memory
 *
 *	\version $Id: mxffile.h,v 1.13 2011/01/10 10:42:09 matt-beard Exp $
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
#ifndef MXFLIB__MXFFILE_H
#define MXFLIB__MXFFILE_H


// For find()
#include <algorithm>


// KLUDGE!! MSVC can't cope with template member functions!!!
namespace mxflib
{
	class MXFFile;
	template<class TP, class T> TP MXFFile__ReadObjectBase(MXFFilePtr This, PrimerPtr UsePrimer = NULL);
}

namespace mxflib
{
	//! Holds data relating to an MXF file
	class MXFFile : public RefCount<MXFFile>
	{
	protected:
		bool isOpen;					//!< True when the file is open
		bool isMemoryFile;				//!< True is the file is a "memory file"
		bool isHandleFile;				//!< True if the file handle is managed externally (we don't open or close it ourselves)
		bool TruncatedKnown;			//!< True if the state of "Truncated" has been determined
		bool Truncated;					//!< True if we have determined that this file has been truncated
		FileHandle Handle;				//!< File handle
		UInt32 RunInSize;				//!< Size of run-in in physical file

		DataChunkPtr Buffer;			//!< Memory file buffer pointer
		UInt64 BufferOffset;			//!< Offset of the start of the buffer from the start of the memory file
		UInt64 BufferCurrentPos;		//!< Offset of the current position from the start of the memory file

		UInt32 BlockAlign;				//!< Some systems can run more efficiently if the essence and index data start on a block boundary - if used this is the block size
		Int32 BlockAlignEssenceOffset;	//!< Fixed distance from the block grid at which to align essence (+ve is after the grid, -ve before)
		Int32 BlockAlignIndexOffset;	//!< Fixed distance from the block grid at which to align index (+ve is after the grid, -ve before)

		//DRAGONS: There should probably be a property to say that in-memory values have changed?
		//DRAGONS: Should we have a flush() function
	public:
		RIP FileRIP;
		DataChunk RunIn;
		std::string Name;

	public:
		MXFFile() : isOpen(false), isMemoryFile(false), TruncatedKnown(false), Truncated(false), BlockAlign(0) {};
		~MXFFile() { if(isOpen) Close(); };

		virtual bool Open(std::string FileName, bool ReadOnly = false );
		virtual bool OpenNew(std::string FileName);
		virtual bool OpenMemory(DataChunkPtr Buff = NULL, Position Offset = 0);
		virtual bool OpenFromHandle(FileHandle Handle);
		virtual bool Close(void);

		bool ReadRunIn(void);

		// RIP Readers
		bool ReadRIP(void);
		bool ScanRIP(Length MaxScan = 1024*1024);
		bool BuildRIP(void);
		bool GetRIP(Length MaxScan = 1024*1024);

		
		//! Locate and read a partition containing closed header metadata
		/*! \ret NULL if none found
		 *  DRAGONS: A significant number fo invalid files exist with a closed incomplete header and a closed complete footer. CheckForCompleteFooter=true will return the footer for these files.
		 */
		PartitionPtr ReadMasterPartition(bool CheckForCompleteFooter = false)
		{
			return ReadMasterPartition(1024 * 1024, CheckForCompleteFooter);
		}

		//! Locate and read a partition containing closed header metadata
		/*! \ret NULL if none found
		 *  DRAGONS: A significant number fo invalid files exist with a closed incomplete header and a closed complete footer. CheckForCompleteFooter=true will return the footer for these files.
		 */
		PartitionPtr MXFFile::ReadMasterPartition(Length MaxScan, bool CheckForCompleteFooter = false);

		//! Locate and read the footer partition
		/*! \ret NULL if not found
		 */
		PartitionPtr ReadFooterPartition(Length MaxScan = 1024*1024);

		//! Report the position of the file pointer
		Position Tell(void) 
		{ 
			if(!isOpen) return 0;
			if(isMemoryFile) return BufferCurrentPos-RunInSize;
			return UInt64(mxflib::FileTell(Handle))-RunInSize;
		}

		//! Move the file pointer
		/* \return 0 if no error, else non-zero
		 * DRAGONS: This is where we need to insert code to handle file discontinuities
		 *          If a file has one or more chunks mising then we can build a list of
		 *			discontinuities based on where partition packs start compared with
		 *			where in the file they claim to be. This allows us to modify seeks
		 *			so that they find the data originally at that part of the file even
		 *			though they are now in a different position
		 */
		int Seek(Position Pos)
		{ 
			if(!isOpen) return 0;
			if(isMemoryFile)
			{
				BufferCurrentPos = Pos+RunInSize;
				return 0;
			}

			return mxflib::FileSeek(Handle, Pos+RunInSize);
		}

		int SeekEnd(void)
		{ 
			if(!isOpen) return 0;
			if(isMemoryFile)
			{
				error("MXFFile::SeekEnd() not supported on memory files\n");

				// Seek to the end of the current buffer
				BufferCurrentPos = BufferOffset + Buffer->Size;
				return (int)Tell();
			}

			return mxflib::FileSeekEnd(Handle);
		}


		//! Determine if the file pointer is at the end of the file
		bool Eof(void) 
		{ 
			if(!isOpen) return true;
			if(isMemoryFile)
			{
				error("MXFFile::Eof() not supported on memory files\n");

				// Return true if at the end of the current buffer
				if((BufferCurrentPos - BufferOffset) <= Buffer->Size) return true; else return false;
			}
		
			return mxflib::FileEof(Handle) ? true : false; 
		};


		//! Return the size of the MXF file, or -1 if not known
		Length Size(void)
		{
			if(!isOpen) return -1;
			if(isMemoryFile) return -1;
			return FileSize(Handle);
		}

		DataChunkPtr Read(size_t Size);
		size_t Read(UInt8 *Buffer, size_t Size);

//		MDObjectPtr ReadObject(void);
//		template<class TP, class T> TP ReadObjectBase(void) { TP x; return x; };
//		template<> MDObjectPtr ReadObjectBase<MDObjectPtr, MDObject>(void) { MDObjectPtr x; return x; };
		MDObjectPtr ReadObject(PrimerPtr UsePrimer = NULL) { return MXFFile__ReadObjectBase<MDObjectPtr, MDObject>(this, UsePrimer); };
		PartitionPtr ReadPartition(void) { return MXFFile__ReadObjectBase<PartitionPtr, Partition>(this); };

		//! Read a KLVObject from the file
		KLVObjectPtr ReadKLV(void);

		//! Write a partition pack to the file
		void WritePartitionPack(PartitionPtr ThisPartition, PrimerPtr UsePrimer = NULL);

		//! Write a partition pack and associated metadata (no index table segments)
		void WritePartition(PartitionPtr ThisPartition, UInt32 Padding = 0, UInt32 MinPartitionSize = 0) { WritePartition(ThisPartition, true, NULL, Padding, MinPartitionSize); };

		//! Write a partition pack and associated metadata and preformatted index table segments
		/*! \note The value of IndexSID must be set prior to calling WritePartitionWithIndex */
		void WritePartitionWithIndex(PartitionPtr ThisPartition, DataChunkPtr IndexData, UInt32 Padding = 0, UInt32 MinPartitionSize = 0) { WritePartitionWithIndex(ThisPartition, IndexData, true, NULL, Padding, MinPartitionSize); };

		//! Write a partition pack and associated metadata (no index table segments)
		void WritePartition(PartitionPtr ThisPartition, PrimerPtr UsePrimer, UInt32 Padding = 0, UInt32 MinPartitionSize = 0) { WritePartition(ThisPartition, true, UsePrimer, Padding, MinPartitionSize); };

		//! Write a partition pack and associated metadata and preformatted index table segments
		/*! \note The value of IndexSID must be set prior to calling WritePartitionWithIndex */
		void WritePartitionWithIndex(PartitionPtr ThisPartition, DataChunkPtr IndexData, PrimerPtr UsePrimer, UInt32 Padding = 0, UInt32 MinPartitionSize = 0) { WritePartitionWithIndex(ThisPartition, IndexData, true, UsePrimer, Padding, MinPartitionSize); };

		//! Write a partition pack and (optionally) associated metadata (no index table segments)
		void WritePartition(PartitionPtr ThisPartition, bool IncludeMetadata, PrimerPtr UsePrimer = NULL, UInt32 Padding = 0, UInt32 MinPartitionSize = 0)
		{
			WritePartitionInternal(false, ThisPartition, IncludeMetadata, NULL, UsePrimer, Padding, MinPartitionSize);
		}

		//! Write a partition pack and (optionally) associated metadata and preformatted index table segments
		/*! \note The value of IndexSID must be set prior to calling WritePartitionWithIndex */
		void WritePartitionWithIndex(PartitionPtr ThisPartition, DataChunkPtr IndexData, bool IncludeMetadata, PrimerPtr UsePrimer = NULL, UInt32 Padding = 0, UInt32 MinPartitionSize = 0)
		{
			WritePartitionInternal(false, ThisPartition, IncludeMetadata, IndexData, UsePrimer, Padding, MinPartitionSize);
		}

		//! Re-write a partition pack and associated metadata (no index table segments)
		/*! \note Partition properties are updated from the linked metadata
		 *	\return true if re-write was successful, else false
		 */
		bool ReWritePartition(PartitionPtr ThisPartition, PrimerPtr UsePrimer = NULL)
		{
			return WritePartitionInternal(true, ThisPartition, true, NULL, UsePrimer, 0, 0);
		}

		//! Re-write a partition pack and associated metadata and preformatted index table segments
		/*! \note Partition properties are updated from the linked metadata
		 *	\return true if re-write was successful, else false
		 */
		bool ReWritePartitionWithIndex(PartitionPtr ThisPartition, DataChunkPtr IndexData, PrimerPtr UsePrimer = NULL)
		{
			return WritePartitionInternal(true, ThisPartition, true, IndexData, UsePrimer, 0, 0);
		}

		//! Is this file truncated?
		/*! If Details != NULL, the file will be re-tested and a details written into the string for truncated files
		 */
		bool IsTruncated(std::string *Details = NULL);

	protected:
		//! Write or re-write a partition pack and associated metadata (and index table segments?)
		bool WritePartitionInternal(bool ReWrite, PartitionPtr ThisPartition, bool IncludeMetadata, DataChunkPtr IndexData, PrimerPtr UsePrimer, UInt32 Padding, UInt32 MinPartitionSize);

	public:
		//! Write the RIP
		void WriteRIP(void);

		//! Calculate the size of a filler to align to a specified KAG
		UInt32 FillerSize(UInt64 FillPos, UInt32 KAGSize, UInt32 MinSize = 0) { return FillerSize(false, FillPos, KAGSize, MinSize); };
		UInt32 FillerSize(bool ForceBER4, UInt64 FillPos, UInt32 KAGSize, UInt32 MinSize = 0);

		//! Write a filler to align to a specified KAG
		UInt64 Align(UInt32 KAGSize, UInt32 MinSize = 0) { return Align(false, KAGSize, MinSize); };
		UInt64 Align(bool ForceBER4, UInt32 KAGSize, UInt32 MinSize = 0);

		ULPtr ReadKey(void);
		Length ReadBER(void);

		//! Write a BER length
		/*! \param Length	The length to be written
		 *	\param Size		The total number of bytes to use for BER length (or 0 for auto)
		 *	\note If the size is specified it will be overridden for lengths
		 *		  that will not fit. However an error message will be produced.
		 */
		UInt32 WriteBER(UInt64 Length, UInt32 Size = 0) { DataChunkPtr BER = MakeBER(Length, Size); Write(*BER); return static_cast<UInt32>(BER->Size); };

		//! Write raw data
		size_t Write(const UInt8 *Buffer, size_t Size) 
		{ 
			if(isMemoryFile) return MemoryWrite(Buffer, Size);

			return FileWrite(Handle, Buffer, Size); 
		};

		//! Write the contents of a DataChunk by reference
		size_t Write(const DataChunk &Data) 
		{ 
			if(isMemoryFile) return MemoryWrite(Data.Data, Data.Size);

			return FileWrite(Handle, Data.Data, Data.Size); 
		};

		void Flush()
		{
			FileFlush(Handle);
		}

		//! Write the contents of a DataChunk by SmartPtr
		size_t Write(DataChunkPtr Data)
		{ 
			if(isMemoryFile) return MemoryWrite(Data->Data, Data->Size);

			return static_cast<size_t>(FileWrite(Handle, Data->Data, Data->Size)); 
		};

		//! Write 8-bit unsigned integer
		void WriteU8(UInt8 Val) { unsigned char Buffer[1]; PutU8(Val, Buffer); Write(Buffer, 1); }

		//! Write 16-bit unsigned integer
		void WriteU16(UInt16 Val) { unsigned char Buffer[2]; PutU16(Val, Buffer); Write(Buffer, 2); }

		//! Write 32-bit unsigned integer
		void WriteU32(UInt32 Val) { unsigned char Buffer[4]; PutU32(Val, Buffer); Write(Buffer, 4); }

		//! Write 64-bit unsigned integer
		void WriteU64(UInt64 Val) { unsigned char Buffer[8]; PutU64(Val, Buffer); Write(Buffer, 8); }

		//! Write 8-bit signed integer
		void WriteI8(Int8 Val) { unsigned char Buffer[1]; PutI8(Val, Buffer); Write(Buffer, 1); }

		//! Write 16-bit signed integer
		void WriteI16(Int16 Val) { unsigned char Buffer[2]; PutI16(Val, Buffer); Write(Buffer, 2); }

		//! Write 32-bit signed integer
		void WriteI32(Int32 Val) { unsigned char Buffer[4]; PutI32(Val, Buffer); Write(Buffer, 4); }

		//! Write 64-bit signed integer
		void WriteI64(Int64 Val) { unsigned char Buffer[8]; PutI64(Val, Buffer); Write(Buffer, 8); }

		//! Read 8-bit unsigned integer
		UInt8 ReadU8(void) { unsigned char Buffer[1]; if(Read(Buffer, 1) == 1) return GetU8(Buffer); else return 0; }

		//! Read 16-bit unsigned integer
		UInt16 ReadU16(void) { unsigned char Buffer[2]; if(Read(Buffer, 2) == 2) return GetU16(Buffer); else return 0; }

		//! Read 32-bit unsigned integer
		UInt32 ReadU32(void) { unsigned char Buffer[4]; if(Read(Buffer, 4) == 4) return GetU32(Buffer); else return 0; }

		//! Read 64-bit unsigned integer
		UInt64 ReadU64(void) { unsigned char Buffer[8]; if(Read(Buffer, 8) == 8) return GetU64(Buffer); else return 0; }

		//! Read 8-bit signed integer (casts from unsigned version)
		Int8 ReadI8(void) { return (Int8)ReadU8(); }

		//! Read 16-bit signed integer (casts from unsigned version)
		Int16 ReadI16(void) { return (Int16)ReadU16(); }
		
		//! Read 32-bit signed integer (casts from unsigned version)
		Int32 ReadI32(void) { return (Int32)ReadU32(); }
		
		//! Read 64-bit signed integer (casts from unsigned version)
		Int64 ReadI64(void) { return (Int64)ReadU64(); }

		// Set a new buffer into this memory file
		void SetMemoryBuffer(DataChunkPtr Buff, UInt32 Offset)
		{
			if(isMemoryFile)
			{
				Buffer = Buff;
				BufferOffset = Offset;
			}
		}

		//! Set the block alignment block size
		void SetBlockAlign(UInt32 Size, Int32 EssenceOffset = 0, Int32 IndexOffset = 0)
		{
			BlockAlign = Size;
			BlockAlignEssenceOffset = EssenceOffset;
			BlockAlignIndexOffset = IndexOffset;
		}

		//! Determine if this file used block alignment
		bool IsBlockAligned(void) { return (BlockAlign != 0); }

		//! Determine the wrapping type for a given Essence Container in this file
		/*! \param BodySID SID of the EssenceContainer to examine
		 *  \param ThisPartition A Partition with metadata to use for determining wrapping type
		 *  \retval ClipWrap - Wrapping is Clip Wrap or similar (Wrapping size > Edit Unit)
		 *  \retval FrameWrap - Wrapping is Frame Wrap or similar (Wrapping size <= Edit Unit)
		 *	\retval UnknownWrap - Unable to determine
		 */
		WrapType GetWrapType(UInt32 BodySID, PartitionPtr ThisPartition);

		//! Determine the wrapping type for a given Essence Container in this file
		WrapType GetWrapType(UInt32 BodySID)
		{
			// Default answer is 'unknown'
			WrapType Ret = UnknownWrap;

			// Record the current position so that this method does not move the pointer
			Position Pos = Tell();

			// Read the master partition
			PartitionPtr Master = ReadMasterPartition();
			if(!Master)
			{
				// If that fails, try the header
				Seek(0);
				Master = ReadPartition();
			}

			// If we have a partition, determine the wrapping type
			if(Master) Ret = GetWrapType(BodySID, Master);

			// Return to the original position
			Seek(Pos);
			
			return Ret;
		}

	protected:
		Position ScanRIP_FindFooter(Length MaxScan);

		//! Write to memory file buffer
		/*! \note This can be overridden in classes derived from MXFFile to give different memory write behaviour */
		virtual size_t MemoryWrite(UInt8 const *Data, size_t Size);

		//! Read from a memory file buffer
		/*! \note This can be overridden in classes derived from MXFFile to give different memory read behaviour */
		virtual size_t MemoryRead(UInt8 *Data, size_t Size);
	};
}


// DRAGONS: MSVC: Why does this work in a header, but not in the file?
template<class TP, class T> /*inline*/ TP mxflib::MXFFile__ReadObjectBase(MXFFilePtr This, PrimerPtr UsePrimer /*=NULL*/)
{
	TP Ret;

	UInt64 Location = This->Tell();
	ULPtr Key = This->ReadKey();

	// If we couldn't read the key then bug out
	if(!Key) return Ret;

	// Build the object (it may come back as an "unknown")
	Ret = new T(Key);

	mxflib_assert(Ret);

	Length Length = This->ReadBER();
	if((sizeof(size_t) < 8) && Length > 0xffffffff)
	{
		error("Maximum read size on this platform is 4Gbytes - However, requested to read object %s at 0x%s which has size of 0x%s\n",
			  Ret->Name().c_str(), Int64toHexString(Location,8).c_str(), Int64toHexString(Length,8).c_str());

		// Skip over the item
		This->Seek(This->Tell() + Length);

		// Return an empty object
		return Ret;
	}

	if(Length > 0)
	{
		// Work out how big the key and length are in the file
		UInt32 KLSize = static_cast<UInt32>(This->Tell() - Location);

		// Read the actual data
		DataChunkPtr Data = This->Read(static_cast<size_t>(Length));

		if(Data->Size != static_cast<size_t>(Length))
		{
			error("Not enough data in file for object %s at 0x%s\n", Ret->Name().c_str(), Int64toHexString(Location,8).c_str());
		}

		Ret->SetParent(This, Location, KLSize);
		Ret->ReadValue(Data->Data, Data->Size, UsePrimer);
	}

	return Ret;
}

#endif // MXFLIB__MXFFILE_H

