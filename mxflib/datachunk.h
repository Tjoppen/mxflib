/*! \file	datachunk.h
 *	\brief	Simple re-sizable data chunk object
 *
 *	\version $Id: datachunk.h,v 1.6 2006/07/02 13:27:50 matt-beard Exp $
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
	class DataChunk : public RefCount<DataChunk>
	{
	private:
		size_t DataSize;						//! Size of the data buffer
		size_t AllocationGranularity;			//! Granulatiry of new memory allocations
		bool ExternalBuffer;					//! True if the buffer is not owned by us

	public:
		size_t Size;							//! Size of the active data in the buffer
		UInt8 *Data;							//! The data buffer

		//! Construct an empty data chunk
		DataChunk() : DataSize(0), AllocationGranularity(0), ExternalBuffer(false), Size(0), Data(NULL) {};

		//! Construct a data chunk with a pre-allocated buffer
		DataChunk(size_t BufferSize) : DataSize(0), AllocationGranularity(0), ExternalBuffer(false), Size(0), Data(NULL) { Resize(BufferSize); };

		//! Construct a data chunk with contents
		DataChunk(size_t MemSize, const UInt8 *Buffer) : DataSize(0), AllocationGranularity(0), ExternalBuffer(false), Size(0), Data(NULL) { Set(MemSize, Buffer); };

		//! Construct a data chunk from an identifier
		template<int SIZE> DataChunk(const Identifier<SIZE> *ID)  : DataSize(0), AllocationGranularity(0), ExternalBuffer(false), Size(0), Data(NULL) { Set(ID->Size(), ID->GetValue() ); }

		//! Data chunk copy constructor
		DataChunk(const DataChunk &Chunk) : DataSize(0), AllocationGranularity(0), ExternalBuffer(false), Size(0), Data(NULL) { Set(Chunk.Size, Chunk.Data); };

		//! Data chunk construct from smart pointer
		DataChunk(const DataChunkPtr &Chunk) : DataSize(0), AllocationGranularity(0), ExternalBuffer(false), Size(0), Data(NULL) { Set(Chunk->Size, Chunk->Data); };

		~DataChunk() 
		{ 
			if((!ExternalBuffer) && (Data)) delete[] Data; 
		};

		//! Resize the data chunk, preserving contents if requested
		void Resize(size_t NewSize, bool PreserveContents = true);

		//! Resize the data buffer, preserving contents if requested
		/*! The buffer is resized to <b>at least</b> NewSize, but Size remains unchanged */
		void ResizeBuffer(size_t NewSize, bool PreserveContents = true);

		//! Steal the buffer belonging to this data chunk
		/*! The buffer is detached and ownership moves to the caller.
		 *	It is the caller's responsibility to free the buffer with <b>delete[]</b> at a later point.
		 *	If MakeEmpty is false the data chunk will not be empty after the call, but the 
		 *  ownership will still be transferred
		 *	\return pointer to the buffer or NULL if no buffer or not owned by this object
		 */
		UInt8 *StealBuffer(bool MakeEmpty = false);

		//! Set some data into a data chunk (expanding it if required)
		void Set(const DataChunk &Buffer, size_t Start = 0)
		{
			Set(Buffer.Size, Buffer.Data, Start);
		}

		//! Set some data into a data chunk (expanding it if required)
		void Set(const DataChunkPtr &Buffer, size_t Start = 0)
		{
			Set(Buffer->Size, Buffer->Data, Start);
		}

		//! Set some data into a data chunk (expanding it if required)
		void Set(size_t MemSize, const UInt8 *Buffer, size_t Start = 0)
		{
			if(Size < (MemSize + Start)) Resize(MemSize + Start);

			memcpy(&Data[Start], Buffer, MemSize);
		}

		//! Append some data to a data chunk
		void Append(const DataChunk &Buffer)
		{
			Set(Buffer.Size, Buffer.Data, Size);
		}

		//! Append some data to a data chunk
		void Append(const DataChunkPtr &Buffer)
		{
			Set(Buffer->Size, Buffer->Data, Size);
		}

		//! Append some data to a data chunk
		void Append(size_t MemSize, const UInt8 *Buffer)
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

		//! Get a (hex) string representation of the data in the buffer
		std::string GetString(void);

		//! Allocation granularity access functions
		void SetGranularity(size_t Gran) { AllocationGranularity = Gran; };
		size_t GetGranularity(void) { return AllocationGranularity; };

		//! Set an external buffer as the data buffer
		/*! \note If an external buffer has been set for a DataChunk it may not
		 *		  stay as the buffer in use. This is because there may not be
		 *		  enough room in the buffer to hold new data. Therefore it is
		 *		  important that the value of property <tt><b>Data</b></tt> is checked
		 *		  before assuming the external buffer is still in use.
		 */
		void SetBuffer(UInt8 *Buffer, size_t BuffSize, size_t AllocatedSize = 0);

		//! Transfer ownership of a data buffer from another DataChunk
		/*! This is a very efficient way to set one DataChunk to the value of another.
		 *  However it partially destroys the source DataChunk by stealing its buffer.
		 *  \return true on success, false on failure
		 */
		bool TakeBuffer(DataChunk &OldOwner, bool MakeEmpty = false);

		//! Transfer ownership of a data buffer from another DataChunk (via a smart pointer)
		/*! This is a very efficient way to set one DataChunk to the value of another.
		 *  However it partially destroys the source DataChunk by stealing its buffer.
		 *  \return true on success, false on failure
		 */
		bool TakeBuffer(DataChunkPtr &OldOwner, bool MakeEmpty = false);
	};
}

#endif // MXFLIB__DATACHUNK_H
