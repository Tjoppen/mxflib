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

// KLUDGE!! MSVC can't cope with template member functions!!!
namespace mxflib
{
	class MXFFile;
	template<class TP, class T> TP MXFFile__ReadObjectBase(MXFFile *This);
}

namespace mxflib
{
	//! Holds data relating to an MXF file
	class MXFFile
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
		bool BuildRIP(void);

		//! Report the position of the file pointer
		Uint64 Tell(void) { return isOpen ? (Uint64(KLVFileTell(Handle))-RunInSize) : 0; };

		//! Move the file pointer and report its new position
		Uint64 Seek(Uint64 Position) { return isOpen ? (Uint64) KLVFileSeek(Handle, Position+RunInSize) : 0; };

		//! Determine if the file pointer is at the end of the file
		bool Eof(void) { return KLVFileEof(Handle) ? true : false; };

		DataChunkPtr Read(Uint64 Size);

//		MDObjectPtr ReadObject(void);
//		template<class TP, class T> TP ReadObjectBase(void) { TP x; return x; };
//		template<> MDObjectPtr ReadObjectBase<MDObjectPtr, MDObject>(void) { MDObjectPtr x; return x; };
		MDObjectPtr ReadObject(void) { return MXFFile__ReadObjectBase<MDObjectPtr, MDObject>(this); };
		PartitionPtr ReadPartition(void) { return MXFFile__ReadObjectBase<PartitionPtr, Partition>(this); };

		ULPtr ReadKey(void);
		Uint64 ReadBER(void);
	};

	//! A smart pointer to an MXFFile object
	typedef SmartPtr<MXFFile> MXFFilePtr;
};

// DRAGONS: Why does this work in a header, but not in the file?
template<class TP, class T> /*inline*/ TP mxflib::MXFFile__ReadObjectBase(MXFFile *This)
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
		DataChunkPtr Data = This->Read(Length);

		if(Data->Size != Length)
		{
			error("Not enough data in file for object %s at 0x%s\n", Ret->Name().c_str(), Int64toHexString(Location,8).c_str());
		}

		Ret->ReadValue(Data->Data, Data->Size);
	}

	return Ret;
}
/*
template<class TP, class T> TP ReadObjectBase(void) { TP x; return X; };
mxflib::MDObjectPtr ReadObject(void) { ReadObjectBase<mxflib::MDObjectPtr, mxflib::MDObject>(); };
mxflib::PrimerPtr ReadPrimer(void) { ReadObjectBase<mxflib::PrimerPtr, mxflib::Primer>(); };

template<class TP, class T> TP ReadObjectTest(void) { T var; return (TP) &var; };
int* TestA(void) { return ReadObjectTest<int*, long>(); };
//int* TestA(void) { return ReadObjectTest(); };

template<class T> T* create();
void f()
{
	int *p = create<int>();
}

template<class T> T* f(T);
void g(char j) {
   int *p = f<int>(j);   //generate the specialization f(int)
}
*/



#endif MXFLIB__MXFFILE_H

