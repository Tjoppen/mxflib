/*! \file	uuid.cpp
 *	\brief	Implementation of Partition class
 *
 *			The Partition class holds data about a partition, either loaded 
 *          from a partition in the file or built in memory
 *
 *	\version $Id: uuid.cpp,v 1.1 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2008, Metaglue Corporation
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

#ifdef _WIN32
#error "uuid.cpp is intended only to be build on Linux"
#endif

#ifndef HAVE_UUID_GENERATE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in */
#include <sys/file.h>

#include "mxflib.h"

#include <pthread.h>

namespace mxflib
{

typedef struct _mac {

	unsigned char MAC[6];

	_mac()
	{
		memset(MAC,0,sizeof MAC);
	}
	unsigned char & operator[](int index)
	{
		assert( index<6);
		return MAC[index];
	}
	unsigned short getHash()
	{
		unsigned short retVal;
		unsigned short  *pS=(unsigned short *)MAC;
		retVal=*pS++;
		retVal=retVal^*pS++;
		retVal=retVal^*pS;

		return retVal;
	}

} MACAddress;


static bool GetMACAddress( MACAddress & MAC)
{

	int sock;

	struct ifreq ifr;
	int result;


	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return false;

	strcpy(ifr.ifr_name,"eth0");

	//Get the MAC address

	result=ioctl(sock,SIOCGIFHWADDR,&ifr);
	for(int i=0;i<6;i++)
	{
		MAC[i]=ifr.ifr_ifru.ifru_hwaddr.sa_data[i];
	}
	close(sock);
	return true;
}



static UInt64 GetGregorianEpochTime()
{
#define VAL_10E7 10000000

	struct timeval timeVal;
	UInt64 UnixEpoch;
	UInt64 retVal=12219292800LL;
	retVal=retVal*(VAL_10E7);
	//retval now holds nanoseconds from Greporian Start to unix epoch start

	gettimeofday(&timeVal, 0);
	UnixEpoch=timeVal.tv_usec*10;
	UnixEpoch+=((UInt64) timeVal.tv_sec)*VAL_10E7;

	retVal+=UnixEpoch;

	return retVal;
}

//do this so that mutex gets inialized at load time
class LocalMutex
{
	pthread_mutex_t mutex;

public:
	LocalMutex()
	{
		pthread_mutex_init(&mutex, NULL);
	}

	void Acquire()
	{
		pthread_mutex_lock( & mutex);
	}

	void Leave()
	{
		pthread_mutex_unlock( &mutex);
	}

};


void MakeUUID(UInt8 *Buffer)
{
	static LocalMutex Mutex;  //standard says you should globally lock the generation of a uuid

	Mutex.Acquire();
	UInt64 Gregorian=GetGregorianEpochTime();

	//instead of the clock sequence number - which we cannot obtain
	MACAddress MAC;
	Uint16 MACHash=0;
	if(!GetMACAddress(MAC))
		getRandnumbers(reinterpret_cast<Uint8*>(&MACHash),2);
	else
		MACHash=MAC.getHash();
	MACHash |= 0x8000;

	Uint32 ClockLow=Gregorian & 0xffffffff;
	Uint32 ClockMid=(Gregorian>>32) & 0xffff;
	Uint32 Clockhi_andflags=((Gregorian>>(32+16)) & 0x0fff) | 0x1000;

	Uint8* tmp=reinterpret_cast<Uint8*>(&ClockLow);
	Buffer[3]=tmp[0];
	Buffer[2]=tmp[1];
	Buffer[1]=tmp[2];
	Buffer[0]=tmp[3];
	tmp=reinterpret_cast<Uint8*>(&ClockMid);
	Buffer[5]=tmp[0];
	Buffer[4]=tmp[1];
	tmp=reinterpret_cast<Uint8*>(&Clockhi_andflags);
	Buffer[7]=tmp[0];
	Buffer[6]=tmp[1];
	tmp=reinterpret_cast<Uint8*>(&MACHash);
	Buffer[9]=tmp[0];
	Buffer[8]=tmp[1];
	getRandnumbers(&Buffer[10],6);

	Mutex.Leave();
}


}

#endif // HAVE_UUID_GENERATE

