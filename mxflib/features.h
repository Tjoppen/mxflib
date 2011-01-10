/*! \file	features.h
 *	\brief	Control compile-time and run-time selectable library features
 *
 *	\version $Id: features.h,v 1.9 2011/01/10 10:42:09 matt-beard Exp $
 *
 *  \detail 
 *  Library feature selection allows the behaviour of the library to be modified at run-time or compile-time.
 *  Run-time selection allows the application to select the desired behaviour, but code will be compiled for
 *  all options. Compile-time selection allows an optimizing compiler to remove all code specific to the
 *  disabled behaviour.
 *
 *  The selectable behaviours are categorized as "standard features" that comply with the MXF specification,
 *  but implement it differently, and "non-standard features" that do not strictly comply with the MXF
 *  specification, yet may be useful in controlled application areas.
 */






/*************************/
/* Compile-time settings */
/*************************/
/*
 * The following macros may be defined on the compiler command-line:
 *
 * MXFLIB_FEATURE_MASK - Only those features that match this mask are compiled
 *                       Default setting is to allow all features
 *
 * MXFLIB_FEATURE_DEFAULT - The initial state of the feature bitmap
 *                          Default setting is all features off
 *
 * MXFLIB_FEATURE_LOCK - Selects features that cannot be changed from their default state at run-time
 *                       Default setting is all features unlocked
 */

#ifndef MXFLIB__FEATURES_H
#define MXFLIB__FEATURES_H

// Those features that may be enabled
#ifndef MXFLIB_FEATURE_MASK
#define MXFLIB_FEATURE_MASK (~UINT64_C(0))
#endif

// Those features that are enabled by default
#ifndef MXFLIB_FEATURE_DEFAULT
#define MXFLIB_FEATURE_DEFAULT (UINT64_C(0))
#endif

// Those features that cannot be changed at run-time
#ifndef MXFLIB_FEATURE_LOCK
#define MXFLIB_FEATURE_LOCK (UINT64_C(0))
#endif


namespace mxflib
{
	/* Standard library features (bits 0 to 30) */

	const UInt64 FeatureVersion1KLVFill = UINT64_C(1) << 0;		//!< MXFLib feature: Write KLVFill items with the version 1 key
	const UInt64 FeatureUnknownsByUL2Name = UINT64_C(1) << 1;	//!< MXFLib feature: If an unknown UL is converted to a name during MDObject construction, using UL2NameFunc, check if this name is a known type

	/* This sub-range is currently used by temporary fixes (bits 16 to 30) */

	const UInt64 FeatureNegPrechargeIndex = UINT64_C(1) << 16;	//!< MXFLib feature: Use -ve indexing for precharge

	/* Reserve a sub-range for user-extensions */

	const UInt64 UserExtension = UINT64_C(1) << 31;		//!< MXFLib feature: Reserved to allow user extensions

	/* Non-Standard library functions - may cause non-compliant behaviour (bits 32 to 63) */

	const UInt64 FeatureLoadMetadict =		UINT64_C(1) << 48;	//!< Load any metadict when reading metadata
	const UInt64 FeatureSaveMetadict =		UINT64_C(1) << 49;	//!< Add a KLV metadict when writing metadata (Only contains extension data)
	const UInt64 FeatureUsedMetadict =		UINT64_C(1) << 50;	//!< Write any metadict as a complete version holding all types and sets used in the file along with all known properties of those sets
	const UInt64 FeatureFullMetadict =		UINT64_C(1) << 51;	//!< Write any metadict as a full version holding all known types, sets and properties
	const UInt64 FeatureKXSMetadict =		UINT64_C(1) << 52;	//!< Use version 1b of KLV Encoded Extension Syntax for any metadict


	const UInt64 FeatureNoHeaderIndex     = UINT64_C(1) << 58;	//!< Do not write index in header, mimic avid files

	// Declare the features bitmap
	extern UInt64 Features;


	//! Set an MXFLib library feature (or multiple features)
	/*! \return true if features set as requested
	 *  \note If multiple features are requested and any one is unavailable none will be set
	 *
	 *  DRAGONS: This code is written so that it will fold to:
	 *           - a simple bit-set if the value of SetValue is known at compile-time and it is enabled and unlocked
	 *           - an error message if the value of SetValue is known at compile-time and it is disabled or locked off
	 *           - a simple bit-set if the value of SetValue is non known at compile time, but no features are disabled or locked off
	 *           - the full function in all other cases
	 */
	inline bool SetFeature(const UInt64 SetValue)
	{
		// Fail if any of the features are disabled
		if((SetValue & MXFLIB_FEATURE_MASK) != SetValue)
		{
			error("Feature 0x%s is not enabled in the current library\n", Int64toHexString(SetValue).c_str());
			return false;
		}

		// Fail if any of the features are locked (unless they are locked enabled!)
		if(SetValue & MXFLIB_FEATURE_LOCK)
		{
			// Check if the locked bits are a problem (the locked value may be "on", which is fine)
			UInt64 Locked = SetValue & MXFLIB_FEATURE_LOCK;
			if(Locked & (~MXFLIB_FEATURE_DEFAULT))
			{
				error("Feature 0x%s is locked off in the current library\n", Int64toHexString(SetValue).c_str());
				return false;
			}
		}

		// Set the feature or features
		Features |= SetValue;

		// All OK
		return true;
	}


	//! Clear an MXFLib library feature (or multiple features)
	/*! \return true if features cleared as requested
	 *  \note If clearing of multiple features is requested and any one is locked on none will be cleared
	 *
	 *  DRAGONS: This code is written so that it will fold to:
	 *           - a simple bit-clear if the value of SetValue is known at compile-time and it is unlocked
	 *           - an error message if the value of SetValue is known at compile-time and it is locked on
	 *           - a simple bit-clear if the value of SetValue is non known at compile time, but no features are locked on
	 *           - the full function in all other cases
	 */
	inline bool ClearFeature(const UInt64 ClearValue)
	{
		// Fail if any of the features are locked (and they are locked enabled!)
		if(ClearValue & MXFLIB_FEATURE_LOCK)
		{
			// Check if the locked bits are a problem (the locked value may be "off", which is fine)
			UInt64 Locked = ClearValue & MXFLIB_FEATURE_LOCK;
			if(Locked & MXFLIB_FEATURE_DEFAULT)
			{
				error("Feature 0x%s is locked on in the current library\n", Int64toHexString(ClearValue).c_str());
				return false;
			}
		}

		// Clear the feature or features
		Features &= ~ClearValue;

		// All OK
		return true;
	}


	//! Determine if an MXFLibrary feature is selected (or combination of features are all selected)
	/*  DRAGONS: This code is written so that it will fold to:
	 *           - a compile-time result if the value of Value is known at compile-time and it is disabled or locked (on or off)
	 *           - a simple bit-test if the value of SetValue is non known at compile time, but no features are disabled or locked
	 *           - the full function in all other cases
	 */
	inline bool Feature(const UInt64 Value)
	{
		// If any of the features are disabled don't bother to read it
		if((Value & MXFLIB_FEATURE_MASK) != Value) return false;

		// If all of the features are locked simply return the compile-time setting
		if(Value & MXFLIB_FEATURE_LOCK)
		{
			if((Value & MXFLIB_FEATURE_DEFAULT) == Value) return true; else return false;
		}

		// Run-time test
		if((Value & Features) == Value) return true; else return false;
	}
}

#endif // MXFLIB__FEATURES_H
