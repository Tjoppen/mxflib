/*! \file	debug.h
 *	\brief	Debug and error handling declarations
 *
 *			The implementation of these functions is an application
 *			issue so they are not contained in the MXFLib library
 */

#ifndef MXFLIB__DEBUG_H
#define MXFLIB__DEBUG_H

//! Define this value here, or on the compiler command line to enable debug() function
#define MXFLIB_DEBUG

#ifdef MXFLIB_DEBUG
int debug(const char *Fmt, ...);				//!< Display a general debug message
#else
int debug(const char *Fmt, ...) { return 0; };	//!< Make debug messages optimise out
#endif

int warning(const char *Fmt, ...);				//!< Display a warning message
int error(const char *Fmt, ...);				//!< Display an error message

#endif MXFLIB__DEBUG_H
