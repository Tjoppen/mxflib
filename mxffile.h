/*! \file	mxffile.h
 *	\brief	Definition of MXFFile class
 *
 *			The MXFFile class holds data about an MXF file, either loaded 
 *          from a physical file or built in memory
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

// Include the KLVLib header
extern "C"
{
#include "KLV.h"						//!< The KLVLib header
}


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
	private:
		bool isOpen;				//! True when the file is open
		bool isMemoryFile;			//! True is the file is a "memory file"
		KLVFile Handle;				//! File hanlde
		Uint64 RunInSize;			//! Size of run-in in physical file

		DataChunkPtr Buffer;		//! Memory file buffer pointer
		Uint64 BufferOffset;		//! Offset of the start of the buffer from the start of the memory file
		Uint64 BufferCurrentPos;	//! Offset of the current position from the start of the memory file

		//DRAGONS: There should probably be a property to say that in-memory values have changed?
		//DRAGONS: Should we have a flush() function
	public:
		RIP FileRIP;
		DataChunk RunIn;
		std::string Name;

	public:
		MXFFile() : isOpen(false), isMemoryFile(false) {};
		~MXFFile() { if(isOpen) Close(); };

		bool Open(std::string FileName, bool ReadOnly = false );
		bool OpenNew(std::string FileName);
		bool OpenMemory(DataChunkPtr Buff = NULL, Uint64 Offset = 0);
		bool Close(void);

		bool ReadRunIn(void);

		// RIP Readers
		bool ReadRIP(void);
		bool ScanRIP(Uint64 MaxScan = 1024*1024);
		bool BuildRIP(void);
		bool GetRIP(Uint64 MaxScan = 1024*1024);

		//! Report the position of the file pointer
		Uint64 Tell(void) 
		{ 
			if(!isOpen) return 0;
			if(isMemoryFile) return BufferCurrentPos-RunInSize;
			return Uint64(mxflib::FileTell(Handle))-RunInSize;
		}

		//! Move the file pointer and report its new position
		// DRAGONS: This is where we need to insert code to handle file discontinuities
		//          If a file has one or more chunks mising then we can build a list of
		//			discontinuities based on where partition packs start compared with
		//			where in the file they claim to be. This allows us to modify seeks
		//			so that they find the data originally at that part of the file even
		//			though they are now in a different position
		Uint64 Seek(Uint64 Position)
		{ 
			if(!isOpen) return 0;
			if(isMemoryFile)
			{
				BufferCurrentPos = Position+RunInSize;
				return Position;
			}

			return Uint64(mxflib::FileSeek(Handle, Position+RunInSize)-RunInSize);
		}

		Uint64 SeekEnd(void)
		{ 
			if(!isOpen) return 0;
			if(isMemoryFile)
			{
				error("MXFFile::SeekEnd() not supported on memory files\n");

				// Seek to the end of the current buffer
				BufferCurrentPos = BufferOffset + Buffer->Size;
				return Tell();
			}

			return Uint64(mxflib::FileSeekEnd(Handle)-RunInSize);
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

		DataChunkPtr Read(Uint64 Size);

//		MDObjectPtr ReadObject(void);
//		template<class TP, class T> TP ReadObjectBase(void) { TP x; return x; };
//		template<> MDObjectPtr ReadObjectBase<MDObjectPtr, MDObject>(void) { MDObjectPtr x; return x; };
		MDObjectPtr ReadObject(PrimerPtr UsePrimer = NULL) { return MXFFile__ReadObjectBase<MDObjectPtr, MDObject>(this, UsePrimer); };
		PartitionPtr ReadPartition(void) { return MXFFile__ReadObjectBase<PartitionPtr, Partition>(this); };

/*		void WriteObject(MDObjectPtr Object, PrimerPtr UsePrimer = NULL) 
		{ 
			DataChunk Buffer;
			Object->WriteObject(Buffer, UsePrimer);
			FileWrite(Handle, Buffer.Data, Buffer.Size);
		};
*/
		//! Write a partition pack to the file
		void WritePartitionPack(PartitionPtr ThisPartition, PrimerPtr UsePrimer = NULL);

		//! Write a partition pack and associated metadata (and index table segments?)
		void WritePartition(PartitionPtr ThisPartition, Uint32 Padding = 0) { WritePartition(ThisPartition, true, NULL, Padding); };

		//! Write a partition pack and associated metadata (and index table segments?)
		void WritePartition(PartitionPtr ThisPartition, PrimerPtr UsePrimer, Uint32 Padding = 0) { WritePartition(ThisPartition, true, UsePrimer, Padding); };

		//! Write a partition pack and (optionally) associated metadata (and index table segments?)
		void WritePartition(PartitionPtr ThisPartition, bool IncludeMetadata, PrimerPtr UsePrimer = NULL, Uint32 Padding = 0)
		{
			WritePartitionInternal(false, ThisPartition, IncludeMetadata, UsePrimer, Padding);
		}

		//! Re-write a partition pack and associated metadata (and index table segments?)
		/*! \note Partition properties are updated from the linked metadata
		 *	\return true if re-write was successful, else false
		 */
		bool MXFFile::ReWritePartition(PartitionPtr ThisPartition, PrimerPtr UsePrimer = NULL) 
		{
			return WritePartitionInternal(true, ThisPartition, true, UsePrimer, 0);
		}

	private:
		//! Write or re-write a partition pack and associated metadata (and index table segments?)
		bool MXFFile::WritePartitionInternal(bool ReWrite, PartitionPtr ThisPartition, bool IncludeMetadata, PrimerPtr UsePrimer, Uint32 Padding);

	public:
		//! Write the RIP
		void WriteRIP(void)
		{
			MDObjectPtr RIPObject = new MDObject("RandomIndexMetadata");
			ASSERT(RIPObject);

			if(RIPObject)
			{
				MDObjectPtr PA = RIPObject->AddChild("PartitionArray");

				ASSERT(PA);
				if(PA)
				{
					RIP::iterator it = FileRIP.begin();
					while(it != FileRIP.end())
					{
						PA->AddChild("BodySID", false)->SetUint((*it).second->BodySID);
						PA->AddChild("ByteOffset", false)->SetUint((*it).second->ByteOffset);
						it++;
					}
				}
				
				// Calculate the pack length
				RIPObject->SetUint("Length", 16 + 4 + (FileRIP.size() * 12) + 4);

				DataChunk Buffer;
				RIPObject->WriteObject(Buffer);

				Write(Buffer.Data, Buffer.Size);
			}
		}

		//! Calculate the size of a filler to align to a specified KAG
		Uint32 FillerSize(Uint64 FillPos, Uint32 KAGSize, Uint32 MinSize = 0) { return FillerSize(false, FillPos, KAGSize, MinSize); };
		Uint32 FillerSize(bool ForceBER4, Uint64 FillPos, Uint32 KAGSize, Uint32 MinSize = 0);

		//! Write a filler to align to a specified KAG
		Uint64 Align(Uint32 KAGSize, Uint32 MinSize = 0) { return Align(false, KAGSize, MinSize); };
		Uint64 Align(bool ForceBER4, Uint32 KAGSize, Uint32 MinSize = 0);

		ULPtr ReadKey(void);
		Uint64 ReadBER(void);

		//! Write a BER length
		/*! \param Length	The length to be written
		 *	\param Size		The total number of bytes to use for BER length (or 0 for auto)
		 *	\note If the size is specified it will be overridden for lengths
		 *		  that will not fit. However an error message will be produced.
		 */
		Uint32 WriteBER(Uint64 Length, Uint32 Size = 0) { DataChunkPtr BER = MakeBER(Length, Size); Write(*BER); return BER->Size; };

		//! Write raw data
		Uint64 Write(Uint8 *Buffer, Uint32 Size) 
		{ 
			if(isMemoryFile) return MemoryWrite(Buffer, Size);

			return FileWrite(Handle, Buffer, Size); 
		};

		//! Write the contents of a DataChunk
		Uint64 Write(DataChunk &Data) 
		{ 
			if(isMemoryFile) return MemoryWrite(Data.Data, Data.Size);

			return FileWrite(Handle, Data.Data, Data.Size); 
		};

		//! Write 8-bit unsigned integer
		void WriteU8(Uint8 Val) { unsigned char Buffer[1]; PutU8(Val, Buffer); FileWrite(Handle, Buffer, 1); }

		//! Read 8-bit unsigned integer
		Uint8 ReadU8(void) { unsigned char Buffer[1]; if(FileRead(Handle, Buffer, 1) == 1) return GetU8(Buffer); else return 0; }

		//! Read 16-bit unsigned integer
		Uint16 ReadU16(void) { unsigned char Buffer[2]; if(FileRead(Handle, Buffer, 2) == 2) return GetU16(Buffer); else return 0; }

		//! Read 32-bit unsigned integer
		Uint32 ReadU32(void) { unsigned char Buffer[4]; if(FileRead(Handle, Buffer, 4) == 4) return GetU32(Buffer); else return 0; }

		//! Read 64-bit unsigned integer
		Uint64 ReadU64(void) { unsigned char Buffer[8]; if(FileRead(Handle, Buffer, 8) == 8) return GetU64(Buffer); else return 0; }

		//! Read 8-bit signed integer (casts from unsigned version)
		Int8 ReadI8(void) { return (Int8)ReadU8(); }

		//! Read 16-bit signed integer (casts from unsigned version)
		Int16 ReadI16(void) { return (Int16)ReadU16(); }
		
		//! Read 32-bit signed integer (casts from unsigned version)
		Int32 ReadI32(void) { return (Int32)ReadU32(); }
		
		//! Read 64-bit signed integer (casts from unsigned version)
		Int64 ReadI64(void) { return (Int64)ReadU64(); }

		// Set a new buffer into this memory file
		void SetMemoryBuffer(DataChunkPtr Buff, Uint32 Offset)
		{
			if(isMemoryFile)
			{
				Buffer = Buff;
				BufferOffset = Offset;
			}
		}

	protected:
		Uint64 ScanRIP_FindFooter(Uint64 MaxScan);

		//! Write to memory file buffer
		Uint32 MemoryWrite(Uint8 const *Data, Uint32 Size);

		//! Read from a memory file buffer
		Uint32 MemoryRead(Uint8 *Data, Uint32 Size);
	};
};


// DRAGONS: MSVC: Why does this work in a header, but not in the file?
template<class TP, class T> /*inline*/ TP mxflib::MXFFile__ReadObjectBase(MXFFilePtr This, PrimerPtr UsePrimer /*=NULL*/)
{
	TP Ret;

	Uint64 Location = This->Tell();
	ULPtr Key = This->ReadKey();

	// If we couldn't read the key then bug out
	if(!Key) return Ret;

	// Build the object (it may come back as an "unknown")
	Ret = new T(Key);

	ASSERT(Ret);

	Uint64 Length = This->ReadBER();
	if(Length > 0)
	{
		// Work out how big the key and length are in the file
		Uint32 KLSize = This->Tell() - Location;

		// Read the actual data
		DataChunkPtr Data = This->Read(Length);

		if(Data->Size != Length)
		{
			error("Not enough data in file for object %s at 0x%s\n", Ret->Name().c_str(), Int64toHexString(Location,8).c_str());
		}

		Ret->SetParent(This, Location, KLSize);
		Ret->ReadValue(Data->Data, Data->Size, UsePrimer);
	}

	return Ret;
}

#endif MXFLIB__MXFFILE_H


