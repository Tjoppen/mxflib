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

namespace mxflib
{
#ifdef MXFLIB_DEBUG
	void debug(const char *Fmt, ...);				//!< Display a general debug message
#else
	void debug(const char *Fmt, ...) { return 0; };	//!< Make debug messages optimise out
#endif

	void warning(const char *Fmt, ...);				//!< Display a warning message
	void error(const char *Fmt, ...);				//!< Display an error message
}

#endif MXFLIB__DEBUG_H
