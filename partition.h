/*! \file	partition.h
 *	\brief	Definition of Partition class
 *
 *			The Partition class holds data about a partition, either loaded 
 *          from a partition in the file or built in memory
 */
#ifndef MXFLIB__PARTITION_H
#define MXFLIB__PARTITION_H

#include "mxflib.h"

#include <list>

namespace mxflib
{
	//! Holds data relating to a single partition
	class Partition
	{
	public:
		//**********************
		// Partition Pack Items
		//**********************
		
		Uint16 MajorVersion;		//!< Major Version from partition pack
		Uint16 MinorVersion;		//!< Minor Version from partition pack
		Uint32 KAGSize;				//!< KLV Alignment Grid Size from partition pack
		Position ThisPartition;		//!< Byte Offset of the start of this partition from the start of the file
									/*!< Note: Version 11 of the MXF spec uses a Uint64 for this
									 *         field but we are using a Position type here as it
									 *         makes more sense, and allows distingushed value -1
									 *   Note: Distinguished value -1 is used where the location
									 *         in the file is not known
									 */
		Position PreviousPartition;	//!< Byte Offset of the start of the previous partition from the start of the file
									/*!< Note: Version 11 of the MXF spec uses a Uint64 for this
									 *         field but we are using a Position type here as it
									 *         makes more sense, and allows distingushed value -1
									 *   Note: Distinguished value -1 is used where the location
									 *         of the previous partition in the file is not known
									 */
		Position FooterPartition;	//!< Byte Offset of the start of the footer partition from the start of the file
									/*!< Note: Version 11 of the MXF spec uses a Uint64 for this
									 *         field but we are using a Position type here as it
									 *         makes more sense, and allows distingushed value -1
									 *   Note: Distinguished value -1 is used where the location
									 *         of the footer partition in the file is not known
									 *	 Note: The MXF specification uses 0 as the distinguished
									 *         value to be used in the partition pack for this 
									 *         purpose, but it is more convenient to use -1 for
									 *         all xxxPartition properties in this software
									 */
		Length HeaderByteCount;		//!< Count of Bytes used for Header Metadata and Primer Pack
									/*!< This value starts with the first byte of the key 
									 *   of the Primer Pack and includes any trailing filler
									 *   after the last header metadata set
									 *   Note: Version 11 of the MXF spec uses a Uint64 for this
									 *         field but we are using a Position type here as it
									 *         makes more sense, and allows distingushed value -1
									 *   Note: Distinguished value -1 is used where the number
									 *         of bytes used is not known
									 */
		Length IndexByteCount;		//!< Count of Bytes used for Index Table Segments
									/*!< This value starts with the first byte of the key 
									 *   of the Primer Pack and includes any trailing filler
									 *   after the last header metadata set
									 *   Note: Version 11 of the MXF spec uses a Uint64 for this
									 *         field but we are using a Position type here as it
									 *         makes more sense, and allows distingushed value -1
									 *   Note: Distinguished value -1 is used where the number
									 *         of bytes used is not known
									 */
		Uint32 IndexSID;			//!< Stream ID of the index table in this partition
									/*!< The value 0 is used if there are no index table
									 *   segments in this partition (or if it is not yet
									 *   known if there will be any)
									 */
		Position BodyOffset;		//!< Byte offset of the start of the Essence Container data in this partition, relative to the start of the Essence Container
									/*!< Note: Version 11 of the MXF spec uses a Uint64 for this
									 *         field but we are using a Position type here as it
									 *         makes more sense, and allows distingushed value -1
									 */
		Uint32 BodySID;				//!< Stream ID of the Essence Container data in this partition
									/*!< The value 0 is used if there is no Essence Container
									 *   data in this partition (or if it is not yet known
									 *   if there will be any, or what Stream ID that essence 
									 *	 will belong to)
									 */
		UL OperationalPattern;		//!< UL of the Operational Pattern to which this file conforms
									/*!< This will be "OPUnknown" if the OP is not yet known
									 */
		ULVector EssenceContainers;	//!< Unordered batch of ULs for each Essence Container type used in or referenced by this file
									/*!< This property only holds those known at the time it
									 *   was last updated
									 */

		//******************
		// Other Properties
		//******************

		Primer *PrimerPack;			//!< The Primer Pack for this partition
									/*!< Or NULL if no primer pack active (only valid
									 *   if there is no header metadata in this partition
									 *   OR it has not yet been written)
									 */

	public:
//		PartitionInfo(void *Part = NULL, Position Offset = -1, Uint32 SID = 0);

//		bool operator< (PartitionInfo &Other);
								//!< Comparison function to allow sorting
	};
}


#endif MXFLIB__PARTITION_H

