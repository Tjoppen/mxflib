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
		bool isOpen;
		KLVFile Handle;
		Uint64 RunInSize;			// Size of run-in in physical file

		//DRAGONS: There should probably be a property to say that in-memory values have changed?
		//DRAGONS: Should we have a flush() function
	public:
		RIP FileRIP;
		DataChunk RunIn;
		std::string Name;

	public:
		MXFFile() : isOpen(false) {};
		~MXFFile() { if(isOpen) Close(); };

		bool Open(std::string FileName, bool ReadOnly = false );
		bool Close(void);

		bool ReadRunIn(void);

		// RIP Readers
		bool ReadRIP(void);
		bool ScanRIP(Uint64 MaxScan = 1024*1024);
		bool BuildRIP(void);
		bool GetRIP(Uint64 MaxScan = 1024*1024);

		//! Report the position of the file pointer
		Uint64 Tell(void) { return isOpen ? (Uint64(mxflib::FileTell(Handle))-RunInSize) : 0; };

		//! Move the file pointer and report its new position
		// DRAGONS: This is where we need to insert code to handle file discontinuities
		//          If a file has one or more chunks mising then we can build a list of
		//			discontinuities based on where partition packs start compared with
		//			where in the file they claim to be. This allows us to modify seeks
		//			so that they find the data originally at that part of the file even
		//			though they are now in a different position
		Uint64 Seek(Uint64 Position) { return isOpen ? Uint64(mxflib::FileSeek(Handle, Position+RunInSize)-RunInSize) : 0; };
		Uint64 SeekEnd(void) { return isOpen ? Uint64(mxflib::FileSeekEnd(Handle)-RunInSize) : 0; };

		//! Determine if the file pointer is at the end of the file
		bool Eof(void) { return mxflib::FileEof(Handle) ? true : false; };

		DataChunkPtr Read(Uint64 Size);

//		MDObjectPtr ReadObject(void);
//		template<class TP, class T> TP ReadObjectBase(void) { TP x; return x; };
//		template<> MDObjectPtr ReadObjectBase<MDObjectPtr, MDObject>(void) { MDObjectPtr x; return x; };
		MDObjectPtr ReadObject(PrimerPtr UsePrimer = NULL) { return MXFFile__ReadObjectBase<MDObjectPtr, MDObject>(this, UsePrimer); };
		PartitionPtr ReadPartition(void) { return MXFFile__ReadObjectBase<PartitionPtr, Partition>(this); };

		ULPtr ReadKey(void);
		Uint64 ReadBER(void);


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

