/*! \file	waveheader.h
 *	\brief	waveheader - a very simple wave format class
 *
 *	\version $Id: waveheader.h,v 1.1 2004/01/06 14:14:44 terabrit Exp $
 *
 */
/*
 *	Copyright (c) 2003, Metaglue Corporation
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

#ifndef _waveheader_h_
#define _waveheader_h_

// DRAGONS: utterly non-portable

namespace mxflib
{
	// waveformat - a very simple wave format class

	class fourcc
	{
	public:
		inline fourcc() { memset( data, 0, 4 ); }
		inline fourcc& operator= (const fourcc & s) { memcpy( this->data, s.data, 4); return *this; }
		inline fourcc( char *const v ) { memcpy( this->data, v, 4 ); }

	private:
		char data[4];
	};

	class LEUint32
	{
	public:
		inline LEUint32() { memset( data, 0, 4 ); }
		inline LEUint32& operator= (const LEUint32 & s) { memcpy( this->data, s.data, 4); return *this; }
		inline LEUint32( const Uint32 v )
		{
			Uint32 t = v;
		  data[0] = (t) & 0xff;
			data[1] = (t >>= 8) & 0xff;
			data[2] = (t >>= 8) & 0xff;
			data[3] = (t >>= 8) & 0xff;
		}
		inline operator Uint32()
		{ 
			Uint32 ret = data[3];
			( ret <<= 8 ) |= data[2];
			( ret <<= 8 ) |= data[1];
			( ret <<= 8 ) |= data[0];
			return ret;
		}

	private:
		unsigned char data[4];
	};

	class LEUint16
	{
	public:
		inline LEUint16() { memset( data, 0, 2 ); }
		inline LEUint16& operator= (const LEUint16 & s) { memcpy( this->data, s.data, 2); return *this; }
		inline LEUint16( const Uint16 v )
		{
			Uint16 t = v;
		  data[0] = (t) & 0xff;
			data[1] = (t >>= 8) & 0xff;
		}
		inline operator Uint16()
		{
			Uint16 ret = data[1];
			( ret <<= 8 ) |= data[0];
			return ret;
		}

	private:
		unsigned char data[2];
	};

	class waveheader_t
	{
	public:
		fourcc		fRIFF;
		LEUint32	RIFF_len;
		fourcc		fWAVE;

		fourcc		ffmt_;
		LEUint32	fmt__len;

		LEUint16	format;
		LEUint16	nchannels;
		LEUint32	samplespersec;
		LEUint32	avgbps;
		LEUint16	blockalign;
		LEUint16	bitspersample;
		LEUint16	cbsize;

		fourcc		data;
		LEUint32	data_len;

		waveheader_t( Uint16 nc =2, Uint32 sa =48000, Uint16 bi =24 )
		{
			fRIFF = fourcc( "RIFF" );
			fWAVE = fourcc( "WAVE" );

			ffmt_ = fourcc( "fmt " );
			fmt__len = sizeof( format )
							 + sizeof( nchannels )
							 + sizeof( samplespersec )
							 + sizeof( avgbps )
							 + sizeof( blockalign )
							 + sizeof( bitspersample )
							 + sizeof( cbsize );

			format = 1;
			nchannels = nc;
			samplespersec = sa;
			avgbps = sa*nc*((bi+7)/8);
			blockalign = nc*((bi+7)/8);
			bitspersample = bi;
			cbsize = 0;

			data = fourcc( "data" );
			data_len = 0;

			RIFF_len = data_len + sizeof( waveheader_t ) - sizeof( fRIFF ) - sizeof( RIFF_len );
		}

		void SetDataLength( Uint32 len )
		{ 
			data_len = len;
			RIFF_len = data_len + sizeof( waveheader_t ) - sizeof( fRIFF ) - sizeof( RIFF_len );
		}
	};
}

#endif // _waveheader_h_
