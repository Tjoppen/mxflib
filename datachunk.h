/*! \file	datachunk.h
 *	\brief	Simple re-sizable data chunk object
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

#ifndef MXFLIB__DATACHUNK_H
#define MXFLIB__DATACHUNK_H

namespace mxflib
{
	// Forward declare so the class can be used
	class DataChunk;

	//! A smart pointer to an DataChunk object
	typedef SmartPtr<DataChunk> DataChunkPtr;

	//! A list of smart pointers to DataChunk objects
	typedef std::list<DataChunkPtr> DataChunkList;
}


namespace mxflib
{
	class DataChunk
	{
	private:
		Uint32 DataSize;

	public:
		Uint32 Size;
		Uint8 *Data;

		//! Construct an empty data chunk
		DataChunk() : DataSize(0), Size(0) , Data(NULL) {};

		//! Construct a data chunk with a pre-allocated buffer
		DataChunk(Uint64 BufferSize) : DataSize(0), Size(0) , Data(NULL) { Resize(BufferSize); };

		//! Construct a data chunk with contents
		DataChunk(Uint64 MemSize, const Uint8 *Buffer) : DataSize(0), Size(0), Data(NULL) { Set(MemSize, Buffer); };

		//! Construct a data chunk from an identifier
		template<int SIZE> DataChunk(const Identifier<SIZE> *ID)  : DataSize(0), Size(0) , Data(NULL) { Set(ID->Size(), ID->GetValue() ); };

		//! Data chunk copy constructor
		DataChunk(const DataChunk &Chunk) : DataSize(0), Size(0) , Data(NULL) { Set(Chunk.Size, Chunk.Data); };

		~DataChunk() { if(Data) delete[] Data; };

		//! Resize the data chunk, preserving contents
		void Resize(Uint32 NewSize)
		{
			if(Size == NewSize) return;

			if(DataSize >= NewSize) 
			{
				Size = NewSize;
				return;
			}

			Uint8 *NewData = new Uint8[NewSize];
			if(Size) memcpy(NewData, Data, Size);
			if(Data) delete[] Data;
			Data = NewData;
			DataSize = Size = NewSize;
		}

		//! Resize the data buffer, preserving contents
		/*! The buffer is resized to <b>at least</b> NewSize, but Size remains unchanged */
		void ResizeBuffer(Uint32 NewSize)
		{
			if(DataSize >= NewSize) return;

			Uint8 *NewData = new Uint8[NewSize];
			if(Size) memcpy(NewData, Data, Size);
			if(Data) delete[] Data;
			Data = NewData;
			DataSize = NewSize;
		}

		//! Set some data into a data chunk (expanding it if required)
		void Set(DataChunk &Buffer, Uint32 Start = 0)
		{
			Set(Buffer.Size, Buffer.Data, Start);
		}

		//! Set some data into a data chunk (expanding it if required)
		void Set(Uint32 MemSize, const Uint8 *Buffer, Uint32 Start = 0)
		{
			if(Size < (MemSize + Start)) Resize(MemSize + Start);

			memcpy(&Data[Start], Buffer, MemSize);
		}

		//! Append some data to a data chunk
		void Append(DataChunk &Buffer)
		{
			Set(Buffer.Size, Buffer.Data, Size);
		}

		//! Append some data to a data chunk
		void Append(Uint32 MemSize, const Uint8 *Buffer)
		{
			Set(MemSize, Buffer, Size);
		}

		DataChunk& operator=(const DataChunk &Right)
		{
			Set(Right.Size, Right.Data);

			return *this;
		}

		bool operator==(const DataChunk &Right) const
		{
			if(Size != Right.Size) return false;
			
			if(memcmp(Data, Right.Data, Size) == 0) return true;

			return false;
		}

		bool operator!=(const DataChunk &Right) const { return !operator==(Right); };

		Uint32 fread(FILE *fp, size_t Size, Uint32 Start = 0)
		{
			int Ret;
			Resize(Size);
			Ret=::fread(&Data[Start],1,Size,fp);
			Size = Ret;
			return Ret;
		};

		std::string GetString(void)
		{
			std::string Ret;
			int i;
			for(i=0; i<Size; i++) 
			{
				if(i != 0) Ret += " ";
				Ret += Int2HexString(Data[i], 2);
			}

			return Ret;
		};
	};
}

#endif MXFLIB__DATACHUNK_H
