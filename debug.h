/*! \file	debug.h
 *	\brief	Various debug related functions
 */

#ifndef MXFLIB__DEBUG_H
#define MXFLIB__DEBUG_H

//! Define this value here, or on the compiler command line to enable extra debug
#define MXFLIB_DEBUG

#ifdef MXFLIB_DEBUG
#define debug printf
#else
int debug(...) {};
#endif

#endif MXFLIB__DEBUG_H
