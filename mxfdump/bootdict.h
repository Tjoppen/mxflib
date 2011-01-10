
	// Types definitions converted from file mxfdump/bootdict.xml
	MXFLIB_TYPE_START(BootDict_Types)
		MXFLIB_TYPE_BASIC("Int8", "8 bit integer", "urn:x-ul:060E2B34.0104.0101.01010500.00000000", 1, false, false)
		MXFLIB_TYPE_BASIC("Int16", "16 bit integer", "urn:x-ul:060E2B34.0104.0101.01010600.00000000", 2, true, false)
		MXFLIB_TYPE_BASIC("Int24", "24 bit integer", "", 3, true, false)
		MXFLIB_TYPE_BASIC("Int32", "32 bit integer", "urn:x-ul:060E2B34.0104.0101.01010700.00000000", 4, true, false)
		MXFLIB_TYPE_BASIC("Int64", "64 bit integer", "urn:x-ul:060E2B34.0104.0101.01010800.00000000", 8, true, false)
		MXFLIB_TYPE_BASIC("UInt8", "8 bit unsigned integer", "urn:x-ul:060E2B34.0104.0101.01010100.00000000", 1, false, false)
		MXFLIB_TYPE_BASIC("UInt16", "16 bit unsigned integer", "urn:x-ul:060E2B34.0104.0101.01010200.00000000", 2, true, false)
		MXFLIB_TYPE_BASIC("UInt24", "24 bit unsigned integer", "", 3, true, false)
		MXFLIB_TYPE_BASIC("UInt32", "32 bit unsigned integer", "urn:x-ul:060E2B34.0104.0101.01010300.00000000", 4, true, false)
		MXFLIB_TYPE_BASIC("UInt64", "64 bit unsigned integer", "urn:x-ul:060E2B34.0104.0101.01010400.00000000", 8, true, false)
		MXFLIB_TYPE_BASIC("UL", "SMPTE Universal Label", "", 16, false, false)
		MXFLIB_TYPE_BASIC("UUID", "Universally Unique ID, as per RFC 4122", "", 16, false, false)
		MXFLIB_TYPE_INTERPRETATION("VersionType", "Version number (created from major*256 + minor)", "UInt16", "urn:x-ul:060E2B34.0104.0101.03010300.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("UTF16", "Unicode UTF-16 coded character", "UInt16", "urn:x-ul:060E2B34.0104.0101.01100100.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("Boolean", "Boolean", "urn:x-ul:060E2B34.0104.0101.01010100.00000000", "urn:x-ul:060E2B34.0104.0101.01040100.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("ISO7", "ISO 7-Bit Coded Character", "UInt8", "urn:x-ul:060E2B34.0104.0101.01100300.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("UTF", "Byte of a Unicode string of unknown format", "UInt8", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("Length", "Length in Edit Units", "Int64", "urn:x-ul:060E2B34.0104.0101.01012002.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("Position", "Position measured in Edit Units", "Int64", "urn:x-ul:060E2B34.0104.0101.01012001.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("RGBACode", "Enumerated value specifying component in an RGBALayoutItem", "UInt8", "urn:x-ul:060E2B34.0104.0101.0201010e.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("UTF7", "RFC 2152 7-Bit Coded UNICODE Character", "ISO7", "", 0, false)
		MXFLIB_TYPE_MULTIPLE("UTFString", "Unicode coded string - unknown format", "UTF", "", ARRAYSTRING, 0)
		MXFLIB_TYPE_MULTIPLE("UTF16String", "Unicode UTF-16 coded string", "UTF16", "urn:x-ul:060E2B34.0104.0101.01100200.00000000", ARRAYSTRING, 0)
		MXFLIB_TYPE_MULTIPLE("Int32Array", "Array of Int32", "Int32", "urn:x-ul:060E2B34.0104.0101.04010300.00000000", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("UInt32Array", "Array of UInt32", "UInt32", "", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("Int64Array", "Array of Int64", "Int64", "urn:x-ul:060E2B34.0104.0101.04010400.00000000", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("Int64Batch", "Batch of Int64", "Int64", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("UInt8Array", "Array of UInt8", "UInt8", "urn:x-ul:060E2B34.0104.0101.04010100.00000000", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("ISO7String", "ISO 7-Bit coded string", "ISO7", "urn:x-ul:060E2B34.0104.0101.01100400.00000000", ARRAYSTRING, 0)
		MXFLIB_TYPE_MULTIPLE("UTF7String", "RFC 2152 7-Bit Coded UNICODE string", "UTF7", "urn:x-ul:060E2B34.0104.0101.01200500.00000000", ARRAYSTRING, 0)
		MXFLIB_TYPE_MULTIPLE("Int32Batch", "Batch of Int32 values", "Int32", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("UInt32Batch", "Batch of UInt32 values", "UInt32", "urn:x-ul:060E2B34.0104.0101.04030200.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("RAW", "Raw data bytes, unknown representation", "UInt8", "", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("RAWBatch", "Batch of Raw data items", "RAW", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_INTERPRETATION("Identifier", "Generic Identifier", "UInt8Array", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("UID", "UID", "Identifier", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("Label", "SMPTE Universal Label", "UL", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("AUID", "AUID is a Label or a swapped UUID", "Label", "urn:x-ul:060E2B34.0104.0101.01030100.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION_REF("StrongRef", "Strong Reference", "UUID", "urn:x-ul:060E2B34.0104.0101.05020000.00000000", 0, false, ClassRefStrong, "")
		MXFLIB_TYPE_INTERPRETATION_REF("WeakRef", "Weak Reference", "UUID", "urn:x-ul:060E2B34.0104.0101.05010000.00000000", 0, false, ClassRefWeak, "")
		MXFLIB_TYPE_INTERPRETATION("LocalTagType", "Local Tag for 2-byte tagged SMPTE 336M set", "UInt16", "urn:x-ul:060E2B34.0104.0101.01012004.00000000", 0, false)
		MXFLIB_TYPE_MULTIPLE("AUIDArray", "Array of AUIDs", "AUID", "urn:x-ul:060E2B34.0104.0101.04010600.00000000", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("AUIDSet", "Set of AUIDs", "AUID", "urn:x-ul:060E2B34.0104.0101.04030100.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("LabelBatch", "Batch of SMPTE Universal Labels", "Label", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongRefArray", "Array of StrongRefs", "StrongRef", "", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongRefBatch", "Batch of StrongRefs", "StrongRef", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("WeakRefArray", "Array of WeakRefs", "StrongRef", "", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("WeakRefBatch", "Batch of WeakRefs", "WeakRef", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_COMPOUND("Rational", "Rational", "urn:x-ul:060E2B34.0104.0101.03010100.00000000")
			MXFLIB_TYPE_COMPOUND_ITEM("Numerator", "Numerator", "urn:x-ul:060E2B34.0104.0101.01010700.00000000", "urn:x-ul:060E2B34.0104.0101.03010101.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Denominator", "Denominator", "Int32", "urn:x-ul:060E2B34.0104.0101.03010102.00000000", 0)
		MXFLIB_TYPE_COMPOUND_END
	MXFLIB_TYPE_END

	// Class definitions converted from file mxfdump/bootdict.xml
	MXFLIB_CLASS_START(BootDict_Classes)
		MXFLIB_CLASS_ITEM("KLVFill", "KLV Filler packet", ClassUsageOptional, "RAW", 0, 0, 0x0000, "06 0E 2B 34 01 01 01 02 03 01 02 10 01 00 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK("PartitionMetadata", "Identifies a Partition Pack", "", "urn:x-ul:060E2B34.0206.0101.0D010200.00000000")
			MXFLIB_CLASS_ITEM("MajorVersion", "Major Version number of MXF byte-level format (non-backwards compatible version number)", ClassUsageRequired, "UInt16", 2, 2, 0x0000, "06 0e 2b 34 01 01 01 04  03 01 02 01 06 00 00 00", "0001h", NULL)
			MXFLIB_CLASS_ITEM("MinorVersion", "Minor Version number of MXF byte-level format (backwards compatible version number)", ClassUsageRequired, "UInt16", 2, 2, 0x0000, "06 0e 2b 34 01 01 01 04  03 01 02 01 07 00 00 00", "0002h", NULL)
			MXFLIB_CLASS_ITEM("KAGSize", "Size of the KLV Alignment Grid (KAG) for this partition, in bytes", ClassUsageRequired, "UInt32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 05  03 01 02 01 09 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ThisPartition", "Byte offset of the start of This Partition, relative to the start of the Header Partition", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  06 10 10 03 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PreviousPartition", "Byte offset of the start of the Previous Partition, relative to the start of the Header Partition", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  06 10 10 02 01 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("FooterPartition", "Byte offset of the start of the Footer Partition, relative to the start of the Header Partition", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  06 10 10 05 01 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("HeaderByteCount", "Count of Bytes used for Header Metadata. This starts at the byte following the Partition pack and includes any trailing filler which is part of the Header Metadata.", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  04 06 09 01 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("IndexByteCount", "Count of Bytes used for Index Table Segments. This starts at the byte following the Header Metadata and includes any trailing filler which is part of the Index Table.", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  04 06 09 02 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("IndexSID", "Index Table Segment Identifier in this partition. The value 0 defines that there are no Index Table segments in this partition.", ClassUsageRequired, "UInt32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  01 03 04 05 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("BodyOffset", "Byte offset of the first byte in the following Essence Container data relative to the start of the Essence Container identified by this BodySID", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  06 08 01 02 01 03 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BodySID", "Identifier of the Essence Container data found in this partition. The value 0 indicates there is no Essence Container data in this partition.", ClassUsageRequired, "UInt32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  01 03 04 04 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("OperationalPattern", "Universal Label of the Operational Pattern to which this file complies", ClassUsageRequired, "Label", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 05  01 02 02 03 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EssenceContainers", "The unordered batch of Universal Labels of Essence Containers used in or referenced by this file", ClassUsageRequired, "AUIDSet", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  01 02 02 10 02 01 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("OpenHeader", "Open Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 01 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("OpenCompleteHeader", "Open Complete Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 03 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("ClosedHeader", "Closed Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 02 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("ClosedCompleteHeader", "Closed Complete Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 04 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("OpenBodyPartition", "Open Body Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 03 01 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("OpenCompleteBodyPartition", "Open Complete Body Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 03 03 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("ClosedBodyPartition", "Closed Body Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 03 02 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("ClosedCompleteBodyPartition", "Closed Complete Body Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 03 04 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("Footer", "Footer Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 04 02 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("CompleteFooter", "Complete Footer Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 04 04 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("Primer", "Primer Pack", "", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 05 01 00")
			MXFLIB_CLASS_ITEM("LocalTagEntryBatch", "Local Tag Entry Batch", ClassUsageRequired, "LocalTagEntryBatch", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  06 01 01 07 15 00 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_SET("AbstractObject", "", "", "urn:x-ul:060E2B34.027F.0101.0D010101.01017f00")
			MXFLIB_CLASS_ITEM_REF("InstanceUID", "Unique ID of this instance", ClassUsageRequired, "UUID", 16, 16, 0x3c0a, "06 0e 2b 34 01 01 01 01  01 01 15 02 00 00 00 00", ClassRefTarget, "", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("InterchangeObject", "", "AbstractObject", "urn:x-ul:060E2B34.027F.0101.0D010101.01010100")
			MXFLIB_CLASS_ITEM("ObjectClass", "Specifies a reference to the definition of a class of object", ClassUsageDecoderRequired, "Label", 0, 0, 0x0101, "urn:x-ul:060E2B34.0101.0102.06010104.01010000", NULL, NULL)
			MXFLIB_CLASS_ITEM("GenerationUID", "Generation Instance", ClassUsageOptional, "UUID", 16, 16, 0x0102, "06 0e 2b 34 01 01 01 02  05 20 07 01 08 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("Preface", "Preface Set", "InterchangeObject", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 2f 00")
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END

	// Types definitions converted from file mxfdump/bootdict.xml
	MXFLIB_TYPE_START(BootDict_Types_2)
		MXFLIB_TYPE_COMPOUND("LocalTagEntry", "Mapping of Local Tag to UL or UUID", "")
			MXFLIB_TYPE_COMPOUND_ITEM("LocalTag", "The value of the Local Tag", "LocalTagType", "06 0e 2b 34 01 01 01 05  01 03 06 02 00 00 00 00", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("UID", "The UID of which the local tag is an alias", "AUID", "06 0e 2b 34 01 01 01 05  01 03 06 03 00 00 00 00", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND("DeltaEntryType", "Map Elements onto Slices", "")
			MXFLIB_TYPE_COMPOUND_ITEM("PosTableIndex", "Index into PosTable (or Apply Temporta Reordering if -1)", "Int8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Slice", "Slice number in IndexEntry", "UInt8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("ElementDelta", "Delta from start of slice to this Element", "UInt32", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND("IndexEntryType", "Index from Edit Unit number to stream offset", "")
			MXFLIB_TYPE_COMPOUND_ITEM("TemporalOffset", "Offset in edit units from Display Order to Coded Order", "Int8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("AnchorOffset", "Offset in edit units to previous Anchor Frame. The value is zero if this is an anchor frame.", "Int8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Flags", "Flags for this Edit Unit", "UInt8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("StreamOffset", "Offset in bytes from the first KLV element in this Edit Unit within the Essence Container", "UInt64", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("SliceOffsetArray", "Array of offsets in bytes from the Stream Offset to the start of each slice.", "UInt32Array", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("PosTableArray", "Array of fractional position offsets from the start of the content package to the synchronized sample in the Content Package", "RationalArray", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND("PartitionInfo", "Partition Start Position Info", "")
			MXFLIB_TYPE_COMPOUND_ITEM("BodySID", "Stream ID of the Body in this partition", "UInt32", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("ByteOffset", "Byte offset from file start (1st byte of the file which is numbered 0) to the 1st byte of the Partition Pack Key", "UInt64", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_MULTIPLE("RationalArray", "Array of Rational", "Rational", "urn:x-ul:060E2B34.0104.0101.04020200.00000000", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("LocalTagEntryBatch", "Batch of Local Tag mappings", "LocalTagEntry", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("DeltaEntryArray", "Array of DeltaEntryTypes", "DeltaEntryType", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("IndexEntryArray", "Array of IndexEntryTypes", "IndexEntryType", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("PartitionArray", "Array of Partition Start Positions", "PartitionInfo", "", ARRAYIMPLICIT, 0)
	MXFLIB_TYPE_END

	// Class definitions converted from file mxfdump/bootdict.xml
	MXFLIB_CLASS_START(BootDict_Classes_2)
		MXFLIB_CLASS_SET("IndexTableSegment", "A segment of an Index Table", "InterchangeObject", "06 0e 2b 34 02 53 01 01  0d 01 02 01 01 10 01 00")
			MXFLIB_CLASS_ITEM("IndexEditRate", "Edit Rate copied from the tracks of the Essence Container", ClassUsageRequired, "Rational", 8, 8, 0x3f0b, "06 0e 2b 34 01 01 01 05  05 30 04 06 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexStartPosition", "The first editable unit indexed by this Index Table segment measured in File Package Edit Units", ClassUsageRequired, "Position", 8, 8, 0x3f0c, "06 0e 2b 34 01 01 01 05  07 02 01 03 01 0a 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexDuration", "Time duration of this table segment measured in Edit Unitsof the referenceg package", ClassUsageRequired, "Length", 8, 8, 0x3f0d, "06 0e 2b 34 01 01 01 05  07 02 02 01 01 02 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EditUnitByteCount", "Byte count of each and every Edit Unit. A value of 0 defines the byte count of Edit Units is only given in the Index Entry Array", ClassUsageDecoderRequired, "UInt32", 4, 4, 0x3f05, "06 0e 2b 34 01 01 01 04  04 06 02 01 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("IndexSID", "Stream Identifier (SID) of Index Stream", ClassUsageDecoderRequired, "UInt32", 4, 4, 0x3f06, "06 0e 2b 34 01 01 01 04  01 03 04 05 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BodySID", "Stream Identifier (SID) of Essence Container Stream", ClassUsageRequired, "UInt32", 4, 4, 0x3f07, "06 0e 2b 34 01 01 01 04  01 03 04 04 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SliceCount", "Number of slices minus 1 (NSL)", ClassUsageDecoderRequired, "UInt8", 1, 1, 0x3f08, "06 0e 2b 34 01 01 01 04  04 04 04 01 01 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("PosTableCount", "Number of PosTable Entries minus 1 (NPE)", ClassUsageOptional, "UInt8", 1, 1, 0x3f0e, "06 0e 2b 34 01 01 01 05  04 04 04 01 07 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("DeltaEntryArray", "Map Elements onto Slices", ClassUsageOptional, "DeltaEntryArray", 0, 0, 0x3f09, "06 0e 2b 34 01 01 01 05  04 04 04 01 06 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexEntryArray", "Index from Edit Unit number to stream offset", ClassUsageDecoderRequired, "IndexEntryArray", 0, 0, 0x3f0a, "06 0e 2b 34 01 01 01 05  04 04 04 02 05 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_FIXEDPACK("RandomIndexMetadata", "Random Index Pack", "", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 11 01 00")
			MXFLIB_CLASS_ITEM("PartitionArray", "Array of Partition Start Positions", ClassUsageRequired, "PartitionArray", 0, 0, 0x0000, "80 62 c1 08 a8 0d eb fe 3a 9d c8 e1 7e 83 b6 4b", NULL, NULL)
			MXFLIB_CLASS_ITEM("Length", "Overall Length of this Pack including the Set Key and BER Length fields", ClassUsageRequired, "UInt32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  04 06 10 01 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK_END
	MXFLIB_CLASS_END

	// Label definitions converted from file mxfdump/bootdict.xml
	MXFLIB_TYPE_START(BootDict_Types_3)
		MXFLIB_MASKED_LABEL("MXFOP1x", "MXF OP1x SingleItem", "06.0E.2B.34.04.01.01.01.0D.01.02.01.01.01.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.03.ff.ff")
		MXFLIB_MASKED_LABEL("MXFOP1a", "MXF OP1a SingleItem SinglePackage", "06.0E.2B.34.04.01.01.01.0D.01.02.01.01.01.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.00.0e.00")
		MXFLIB_MASKED_LABEL("MXFOP2a", "MXF OP2a PlaylistItems SinglePackage", "06.0E.2B.34.04.01.01.01.0D.01.02.01.02.01.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.00.0e.00")
		MXFLIB_MASKED_LABEL("MXFOP3a", "MXF OP3a EditItems SinglePackage", "06.0E.2B.34.04.01.01.01.0D.01.02.01.03.01.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.00.0e.00")
		MXFLIB_MASKED_LABEL("MXFOP1b", "MXF OP1b SingleItem GangedPackages", "06.0E.2B.34.04.01.01.01.0D.01.02.01.01.02.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.00.0e.00")
		MXFLIB_MASKED_LABEL("MXFOP2b", "MXF OP2b PlaylistItems GangedPackages", "06.0E.2B.34.04.01.01.01.0D.01.02.01.02.02.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.00.0e.00")
		MXFLIB_MASKED_LABEL("MXFOP3b", "MXF OP3b EditItems GangedPackages", "06.0E.2B.34.04.01.01.01.0D.01.02.01.03.02.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.00.0e.00")
		MXFLIB_MASKED_LABEL("MXFOP1c", "MXF OP1c SingleItem AlternatePackages", "06.0E.2B.34.04.01.01.01.0D.01.02.01.01.03.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.00.0e.00")
		MXFLIB_MASKED_LABEL("MXFOP2c", "MXF OP2c PlaylistItems AlternatePackages", "06.0E.2B.34.04.01.01.01.0D.01.02.01.02.03.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.00.0e.00")
		MXFLIB_MASKED_LABEL("MXFOP3c", "MXF OP3c EditItems AlternatePackages", "06.0E.2B.34.04.01.01.01.0D.01.02.01.03.03.01.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.00.0e.00")
		MXFLIB_MASKED_LABEL("MXFOPSpecialized", "MXF Specialized OP", "06.0E.2B.34.04.01.01.00.0D.01.02.01.00.00.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.7f.ff.ff.ff")
		MXFLIB_MASKED_LABEL("MXFOPAtom", "MXF Specialized OP Atom", "06.0E.2B.34.04.01.01.02.0D.01.02.01.10.00.00.00", "00.00.00.00.00.00.00.00.00.00.00.00.00.03.00.00")
		MXFLIB_MASKED_LABEL("MXFEC", "MXF Essence Containers", "06.0E.2B.34.04.01.01.00.0D.01.03.00.00.00.00.00", "00.00.00.00.00.00.00.ff.00.00.00.ff.ff.ff.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGC", "MXF Generic Container", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.00.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.ff.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCD10", "MXF-GC SMPTE D-10 Mappings", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.01.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCDV", "MXF-GC DV-DIF Mappings", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.02.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCD10", "MXF-GC SMPTE D-11 Mappings", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.03.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCMPEGES", "MXF-GC MPEG Elementary Streams", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.04.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCUncompressed", "MXF-GC Uncompressed Pictures", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.05.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCAESBWF", "MXF-GC AES-BWF Audio", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.06.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCMPEGPES", "MXF-GC MPEG Packetised Elementary Streams", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.07.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCMPEGPS", "MXF-GC MPEG Program Streams", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.08.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCMPEGTS", "MXF-GC MPEG Transport Streams", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.09.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCALaw", "MXF-GC A-law Audio Mappings", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.0A.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCEncrypted", "MXF-GC Encrypted Data Mappings", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.0B.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCJP2K", "MXF-GC JPEG-2000 Picture Mappings", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.0C.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCVBI", "MXF-GC Generic VBI Data Mapping Undefined Payload", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.0D.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_LABEL("MXFGCMultiple", "MXF-GC Generic Essence Multiple Mappings", "06.0E.2B.34.04.01.01.03.0D.01.03.01.02.7f.01.00")
		MXFLIB_LABEL("SMPTE12MTimecodeTrack", "SMPTE 12M Timecode Track", "06.0E.2B.34.04.01.01.01.01.03.02.01.01.00.00.00")
		MXFLIB_LABEL("SMPTE12MTimecodeTrackActiveUserBits", "SMPTE 12M Timecode Track with active user bits", "06.0E.2B.34.04.01.01.01.01.03.02.01.02.00.00.00")
		MXFLIB_LABEL("SMPTE309MTimecodeTrack", "SMPTE 309M Timecode Track", "06.0E.2B.34.04.01.01.01.01.03.02.01.03.00.00.00")
		MXFLIB_LABEL("DescriptiveMetadataTrack", "Descriptive Metadata Track", "06.0E.2B.34.04.01.01.01.01.03.02.01.10.00.00.00")
		MXFLIB_LABEL("PictureEssenceTrack", "Picture Essence Track", "06.0E.2B.34.04.01.01.01.01.03.02.02.01.00.00.00")
		MXFLIB_LABEL("SoundEssenceTrack", "Sound Essence Track", "06.0E.2B.34.04.01.01.01.01.03.02.02.02.00.00.00")
		MXFLIB_LABEL("DataEssenceTrack", "Data Essence Track", "06.0E.2B.34.04.01.01.01.01.03.02.02.03.00.00.00")
		MXFLIB_MASKED_LABEL("DMS1", "MXF Descriptive Metadata Scheme 1", "06.0E.2B.34.04.01.01.01.0D.01.04.01.01.00.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.ff.ff.ff")
		MXFLIB_LABEL("DMS1Production", "MXF DMS-1 Production Framework constrained to the standard version", "06.0E.2B.34.04.01.01.04.0D.01.04.01.01.02.01.01")
		MXFLIB_LABEL("DMS1ProductionExtended", "MXF DMS-1 Clip Framework constrained to the extended version", "06.0E.2B.34.04.01.01.04.0D.01.04.01.01.02.01.02")
		MXFLIB_LABEL("DMS1Clip", "MXF DMS-1 Clip Framework constrained to the standard version", "06.0E.2B.34.04.01.01.04.0D.01.04.01.01.02.02.01")
		MXFLIB_LABEL("DMS1ClipExtended", "MXF DMS-1 Production Framework constrained to the extended version", "06.0E.2B.34.04.01.01.04.0D.01.04.01.01.02.02.02")
		MXFLIB_LABEL("DMS1Scene", "MXF DMS-1 Scene Framework constrained to the standard version", "06.0E.2B.34.04.01.01.04.0D.01.04.01.01.02.03.01")
		MXFLIB_LABEL("DMS1SceneExtended", "MXF DMS-1 Scene Framework constrained to the extended version", "06.0E.2B.34.04.01.01.04.0D.01.04.01.01.02.03.02")
		MXFLIB_MASKED_LABEL("DMSCrypto", "MXF Cryptographic DM Scheme", "06.0E.2B.34.04.01.01.07.0D.01.04.01.02.00.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.ff.ff.ff")
		MXFLIB_LABEL("DMSCryptoFramework", "Cryptographic framework for the DC28 MXF cryptographic DM scheme", "06.0E.2B.34.04.01.01.07.0D.01.04.01.02.01.01.00")
	MXFLIB_TYPE_END

	// Types definitions converted from file mxfdump/bootdict.xml
	MXFLIB_TYPE_START(BootDict_Types_4)
		MXFLIB_TYPE_INTERPRETATION("StringArray", "Unicode UTF-16 coded strings, each zero terminated in a single outer string", "UTF16String", "urn:x-ul:060E2B34.0104.0101.04010500.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION_REF("WeakReferenceTypeDefinition", "", "WeakRef", "", 0, false, ClassRefUndefined, "TypeDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("WeakReferenceClassDefinition", "", "WeakRef", "", 0, false, ClassRefUndefined, "ClassDefinition")
		MXFLIB_TYPE_MULTIPLE_REF("StrongReferenceSetClassDefinition", "Set of StrongReferences to ClassDefinition sets", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05050100.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, "ClassDefinition")
		MXFLIB_TYPE_MULTIPLE_REF("StrongReferenceSetTypeDefinition", "Set of StrongReferences to TypeDefinition sets", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05050c00.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, "TypeDefinition")
		MXFLIB_TYPE_MULTIPLE_REF("StrongReferenceSetPropertyDefinition", "Set of StrongReferences to PropertyDefinition sets", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05050B00.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, "PropertyDefinition")
		MXFLIB_TYPE_MULTIPLE_REF("WeakReferenceVectorTypeDefinition", "Vector of WeakReferences to TypeDefinition sets", "WeakRef", "urn:x-ul:060E2B34.0104.0101.05040200.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, "TypeDefinition")
	MXFLIB_TYPE_END

	// Class definitions converted from file mxfdump/bootdict.xml
	MXFLIB_CLASS_START(BootDict_Classes_3)
		MXFLIB_CLASS_SET("MetaDictionary", "[060E2B34.0253.0101.0D010101.02250000] From AAF", "GenerationInterchangeObject", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 25 00 00")
			MXFLIB_CLASS_ITEM("ClassDefinitions", "urn:x-ul:060E2B34.0101.0102.06010107.07000000 from AAF", ClassUsageRequired, "StrongReferenceSetClassDefinition", 0, 0, 0x0003, "06 0E 2B 34 01 01 01 02 06 01 01 07 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TypeDefinitions", "urn:x-ul:060E2B34.0101.0102.06010107.08000000 from AAF", ClassUsageRequired, "StrongReferenceSetTypeDefinition", 0, 0, 0x0004, "06 0E 2B 34 01 01 01 02 06 01 01 07 08 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("MetaDefinition", "", "InterchangeObject", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 24 00 00")
			MXFLIB_CLASS_ITEM("Identification", "", ClassUsageRequired, "AUID", 0, 0, 0x0005, "06 0E 2B 34 01 01 01 02 06 01 01 07 13 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Name", "", ClassUsageRequired, "UTF16String", 0, 0, 0x0006, "06 0E 2B 34 01 01 01 02 03 02 04 01 02 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Description", "", ClassUsageOptional, "UTF16String", 0, 0, 0x0007, "06 0E 2B 34 01 01 01 02 06 01 01 07 14 01 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("ClassDefinition", "", "MetaDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 01 00 00")
			MXFLIB_CLASS_ITEM_REF("ParentClass", "", ClassUsageRequired, "UUID", 0, 0, 0x0008, "06 0E 2B 34 01 01 01 02 06 01 01 07 01 00 00 00", ClassRefWeak, "", NULL, NULL)
			MXFLIB_CLASS_ITEM("Properties", "", ClassUsageOptional, "StrongReferenceSetPropertyDefinition", 0, 0, 0x0009, "06 0E 2B 34 01 01 01 02 06 01 01 07 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IsConcrete", "", ClassUsageRequired, "Boolean", 0, 0, 0x000a, "06 0E 2B 34 01 01 01 02 06 01 01 07 03 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("PropertyDefinition", "", "MetaDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 02 00 00")
			MXFLIB_CLASS_ITEM("Type", "", ClassUsageRequired, "AUID", 0, 0, 0x000b, "06 0E 2B 34 01 01 01 02 06 01 01 07 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IsOptional", "", ClassUsageRequired, "Boolean", 0, 0, 0x000c, "06 0E 2B 34 01 01 01 02 03 01 02 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("LocalIdentification", "", ClassUsageRequired, "UInt16", 0, 0, 0x000d, "06 0E 2B 34 01 01 01 02 06 01 01 07 05 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IsUniqueIdentifier", "", ClassUsageOptional, "Boolean", 0, 0, 0x000e, "06 0E 2B 34 01 01 01 02 06 01 01 07 06 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinition", "", "MetaDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 03 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionInteger", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 04 00 00")
			MXFLIB_CLASS_ITEM("Size", "", ClassUsageRequired, "UInt8", 0, 0, 0x000f, "06 0E 2B 34 01 01 01 02 03 01 02 03 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IsSigned", "", ClassUsageRequired, "Boolean", 0, 0, 0x0010, "06 0E 2B 34 01 01 01 02 03 01 02 03 02 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionStrongObjectReference", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 05 00 00")
			MXFLIB_CLASS_ITEM("ReferencedType", "", ClassUsageRequired, "WeakReferenceTypeDefinition", 0, 0, 0x0011, "06 0E 2B 34 01 01 01 02 06 01 01 07 09 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionWeakObjectReference", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 06 00 00")
			MXFLIB_CLASS_ITEM("ReferencedType", "", ClassUsageRequired, "WeakReferenceTypeDefinition", 0, 0, 0x0012, "06 0E 2B 34 01 01 01 02 06 01 01 07 0A 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TargetSet", "", ClassUsageRequired, "AUIDSet", 0, 0, 0x0013, "06 0E 2B 34 01 01 01 02 03 01 02 03 0B 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionEnumeration", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 07 00 00")
			MXFLIB_CLASS_ITEM("ElementType", "", ClassUsageRequired, "WeakReferenceTypeDefinition", 0, 0, 0x0014, "06 0E 2B 34 01 01 01 02 06 01 01 07 0B 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ElementNames", "", ClassUsageRequired, "StringArray", 0, 0, 0x0015, "06 0E 2B 34 01 01 01 02 03 01 02 03 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ElementValues", "", ClassUsageRequired, "Int64Batch", 0, 0, 0x0016, "06 0E 2B 34 01 01 01 02 03 01 02 03 05 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionFixedArray", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 08 00 00")
			MXFLIB_CLASS_ITEM("ElementType", "", ClassUsageRequired, "WeakReferenceTypeDefinition", 0, 0, 0x0017, "06 0E 2B 34 01 01 01 02 06 01 01 07 0C 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ElementCount", "", ClassUsageRequired, "UInt32", 0, 0, 0x0018, "06 0E 2B 34 01 01 01 02 03 01 02 03 03 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionVariableArray", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 09 00 00")
			MXFLIB_CLASS_ITEM("ElementType", "", ClassUsageRequired, "WeakReferenceTypeDefinition", 0, 0, 0x0019, "06 0E 2B 34 01 01 01 02 06 01 01 07 0D 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionSet", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0A 00 00")
			MXFLIB_CLASS_ITEM("ElementType", "", ClassUsageRequired, "WeakReferenceTypeDefinition", 0, 0, 0x001a, "06 0E 2B 34 01 01 01 02 06 01 01 07 0E 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionString", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0B 00 00")
			MXFLIB_CLASS_ITEM("ElementType", "", ClassUsageRequired, "WeakReferenceTypeDefinition", 0, 0, 0x001b, "06 0E 2B 34 01 01 01 02 06 01 01 07 0F 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionStream", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0C 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionRecord", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0D 00 00")
			MXFLIB_CLASS_ITEM("MemberTypes", "", ClassUsageRequired, "WeakReferenceVectorTypeDefinition", 0, 0, 0x001c, "06 0E 2B 34 01 01 01 02 06 01 01 07 11 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("MemberNames", "", ClassUsageRequired, "StringArray", 0, 0, 0x001d, "06 0E 2B 34 01 01 01 02 03 01 02 03 06 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionRename", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0E 00 00")
			MXFLIB_CLASS_ITEM("RenamedType", "", ClassUsageRequired, "WeakReferenceTypeDefinition", 0, 0, 0x001e, "06 0E 2B 34 01 01 01 02 06 01 01 07 12 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionExtendibleEnumeration", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 20 00 00")
			MXFLIB_CLASS_ITEM("ElementNames", "", ClassUsageRequired, "StringArray", 0, 0, 0x001f, "06 0E 2B 34 01 01 01 02 03 01 02 03 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ElementValues", "", ClassUsageRequired, "AUIDSet", 0, 0, 0x0020, "06 0E 2B 34 01 01 01 02 03 01 02 03 08 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionIndirect", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 21 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionOpaque", "", "TypeDefinitionIndirect", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 22 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionCharacter", "", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 23 00 00")
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END

	// Types definitions converted from file mxfdump/bootdict.xml
	MXFLIB_TYPE_START(BootDict_Types_5)
		MXFLIB_TYPE_COMPOUND("Root_Offsets_Struct", "", "")
			MXFLIB_TYPE_COMPOUND_ITEM("UnknownA_Offset", "", "UInt64", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("UnknownB_Offset", "", "UInt64", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("ObjDir_Offset", "", "UInt64", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND("ObjectPosition_Struct", "", "")
			MXFLIB_TYPE_COMPOUND_ITEM("ObjectID", "", "AUID", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("ObjectOffset", "", "UInt64", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("ObjectFlags", "", "UInt8", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_MULTIPLE("ObjectPosition", "", "ObjectPosition_Struct", "", ARRAYEXPLICIT, 0)
	MXFLIB_TYPE_END

	// Class definitions converted from file mxfdump/bootdict.xml
	MXFLIB_CLASS_START(BootDict_Classes_4)
		MXFLIB_CLASS_SET("AvidRoot", "", "", "80 53 08 00 36 21 08 04 B3 B3 98 A5 1C 90 11 D4")
			MXFLIB_CLASS_ITEM_REF("Root_MetaDictionary", "Strong ref to MetaDictionary", ClassUsageOptional, "UUID", 16, 16, 0x0001, "06 0E 2B 34 01 01 01 02 0D 01 03 01 01 01 00 00", ClassRefStrong, "MetaDictionary", NULL, NULL)
			MXFLIB_CLASS_ITEM_REF("Root_Preface", "Strong ref to Preface", ClassUsageRequired, "UUID", 16, 16, 0x0002, "06 0E 2B 34 01 01 01 02 0D 01 03 01 02 01 00 00", ClassRefStrong, "Preface", NULL, NULL)
			MXFLIB_CLASS_ITEM("Root_Offsets", "", ClassUsageRequired, "RAW", 0, 0, 0x0003, "06 0E 2B 34 01 01 01 02 0D 01 03 01 03 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Root_Unknown", "", ClassUsageOptional, "RAW", 0, 0, 0x0004, "06 0E 2B 34 01 01 01 02 0D 01 03 01 04 01 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_FIXEDPACK("ObjectDirectory", "", "", "96 13 B3 8A 87 34 87 46 F1 02 96 F0 56 E0 4D 2A")
			MXFLIB_CLASS_ITEM("ObjectDetails", "", ClassUsageRequired, "ObjectPosition", 0, 0, 0x0000, "38 26 61 0B 00 EA 48 0c A6 F3 B7 A7 D4 79 F6 86", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK_END
	MXFLIB_CLASS_END

	// Build a complete dictionary from above types and classes
	MXFLIB_DICTIONARY_START(BootDict)
		MXFLIB_DICTIONARY_TYPES(BootDict_Types)
		MXFLIB_DICTIONARY_TYPES(BootDict_Types_2)
		MXFLIB_DICTIONARY_TYPES(BootDict_Types_3)
		MXFLIB_DICTIONARY_TYPES(BootDict_Types_4)
		MXFLIB_DICTIONARY_TYPES(BootDict_Types_5)
		MXFLIB_DICTIONARY_CLASSES(BootDict_Classes)
		MXFLIB_DICTIONARY_CLASSES(BootDict_Classes_2)
		MXFLIB_DICTIONARY_CLASSES(BootDict_Classes_3)
		MXFLIB_DICTIONARY_CLASSES(BootDict_Classes_4)
	MXFLIB_DICTIONARY_END
