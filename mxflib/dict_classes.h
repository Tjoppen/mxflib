	// Class definitions converted from file ../xmldict.xml
	MXFLIB_CLASS_START(dict_classes)
		MXFLIB_CLASS_ITEM("KLVFill", "KLV Filler packet", ClassUsageOptional, "RAW", 0, 0, 0x0000, "06 0E 2B 34 01 01 01 01 03 01 02 10 01 00 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK("PartitionMetadata", "Identifies a Partition Pack", "", "")
			MXFLIB_CLASS_ITEM("MajorVersion", "Major Version number of MXF byte-level format (non-backwards compatible version number)", ClassUsageRequired, "Uint16", 2, 2, 0x0000, "06 0e 2b 34 01 01 01 04  03 01 02 01 06 00 00 00", "0001h", NULL)
			MXFLIB_CLASS_ITEM("MinorVersion", "Minor Version number of MXF byte-level format (backwards compatible version number)", ClassUsageRequired, "Uint16", 2, 2, 0x0000, "06 0e 2b 34 01 01 01 04  03 01 02 01 07 00 00 00", "0002h", NULL)
			MXFLIB_CLASS_ITEM("KAGSize", "Size of the KLV Alignment Grid (KAG) for this partition, in bytes", ClassUsageRequired, "Uint32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 05  03 01 02 01 09 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ThisPartition", "Byte offset of the start of This Partition, relative to the start of the Header Partition", ClassUsageRequired, "Uint64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  06 10 10 03 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PreviousPartition", "Byte offset of the start of the Previous Partition, relative to the start of the Header Partition", ClassUsageRequired, "Uint64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  06 10 10 02 01 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("FooterPartition", "Byte offset of the start of the Footer Partition, relative to the start of the Header Partition", ClassUsageRequired, "Uint64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  06 10 10 05 01 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("HeaderByteCount", "Count of Bytes used for Header Metadata. This starts at the byte following the Partition pack and includes any trailing filler which is part of the Header Metadata.", ClassUsageRequired, "Uint64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  04 06 09 01 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("IndexByteCount", "Count of Bytes used for Index Table Segments. This starts at the byte following the Header Metadata and includes any trailing filler which is part of the Index Table.", ClassUsageRequired, "Uint64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  04 06 09 02 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("IndexSID", "Index Table Segment Identifier in this partition. The value 0 defines that there are no Index Table segments in this partition.", ClassUsageRequired, "Uint32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  01 03 04 05 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("BodyOffset", "Byte offset of the first byte in the following Essence Container data relative to the start of the Essence Container identified by this BodySID", ClassUsageRequired, "Uint64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  06 08 01 02 01 03 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BodySID", "Identifier of the Essence Container data found in this partition. The value 0 indicates there is no Essence Container data in this partition.", ClassUsageRequired, "Uint32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  01 03 04 04 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("OperationalPattern", "Universal Label of the Operational Pattern to which this file complies", ClassUsageRequired, "Label", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 05  01 02 02 03 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_VECTOR("EssenceContainers", "The unordered batch of Universal Labels of Essence Containers used in or referenced by this file", ClassUsageRequired, 0x0000, "06 0e 2b 34 01 01 01 05  01 02 02 10 02 01 00 00")
				MXFLIB_CLASS_ITEM("EssenceContainer", "Universal Labels of Essence Container", ClassUsageRequired, "Label", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
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
			MXFLIB_CLASS_VECTOR("LocalTagEntryBatch", "Local Tag Entry Batch", ClassUsageRequired, 0x0000, "06 0e 2b 34 01 01 01 05  06 01 01 07 15 00 00 00")
				MXFLIB_CLASS_ITEM("LocalTag", "The value of the Local Tag", ClassUsageRequired, "Uint16", 2, 2, 0x0000, "06 0e 2b 34 01 01 01 05  01 03 06 02 00 00 00 00", NULL, NULL)
				MXFLIB_CLASS_ITEM("UID", "The UID of which the local tag is an alias", ClassUsageRequired, "Label", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 05  01 03 06 03 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_SET("InterchangeObject", "", "", "")
			MXFLIB_CLASS_ITEM_REF("InstanceUID", "Unique ID of this instance", ClassUsageRequired, "UUID", 16, 16, 0x3c0a, "06 0e 2b 34 01 01 01 01  01 01 15 02 00 00 00 00", ClassRefTarget, "", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("GenerationInterchangeObject", "", "InterchangeObject", "")
			MXFLIB_CLASS_ITEM("GenerationUID", "Generation Instance", ClassUsageOptional, "UUID", 16, 16, 0x0102, "06 0e 2b 34 01 01 01 02  05 20 07 01 08 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("DefaultObject", "", "GenerationInterchangeObject", "06 0e 2b 34 02 53 01 01  7f 00 00 00 00 00 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("IndexTableSegmentBase", "Base for IndexTableSegments", "InterchangeObject", "")
			MXFLIB_CLASS_ITEM("IndexEditRate", "Edit Rate copied from the tracks of the Essence Container", ClassUsageRequired, "Rational", 8, 8, 0x3f0b, "06 0e 2b 34 01 01 01 05  05 30 04 06 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexStartPosition", "The first editable unit indexed by this Index Table segment measured in File Package Edit Units", ClassUsageRequired, "Position", 8, 8, 0x3f0c, "06 0e 2b 34 01 01 01 05  07 02 01 03 01 0a 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexDuration", "Time duration of this table segment measured in Edit Unitsof the referenceg package", ClassUsageRequired, "Length", 8, 8, 0x3f0d, "06 0e 2b 34 01 01 01 05  07 02 02 01 01 02 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EditUnitByteCount", "Byte count of each and every Edit Unit. A value of 0 defines the byte count of Edit Units is only given in the Index Entry Array", ClassUsageDecoderRequired, "Uint32", 4, 4, 0x3f05, "06 0e 2b 34 01 01 01 04  04 06 02 01 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("IndexSID", "Stream Identifier (SID) of Index Stream", ClassUsageDecoderRequired, "Uint32", 4, 4, 0x3f06, "06 0e 2b 34 01 01 01 04  01 03 04 05 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BodySID", "Stream Identifier (SID) of Essence Container Stream", ClassUsageRequired, "Uint32", 4, 4, 0x3f07, "06 0e 2b 34 01 01 01 04  01 03 04 04 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SliceCount", "Number of slices minus 1 (NSL)", ClassUsageDecoderRequired, "Uint8", 1, 1, 0x3f08, "06 0e 2b 34 01 01 01 04  04 04 04 01 01 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("PosTableCount", "Number of PosTable Entries minus 1 (NPE)", ClassUsageOptional, "Uint8", 1, 1, 0x3f0e, "06 0e 2b 34 01 01 01 05  04 04 04 01 07 00 00 00", "0", NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("V10IndexTableSegment", "A segment of an Index Table (v10)", "IndexTableSegmentBase", "06 0e 2b 34 02 53 01 01  0d 01 02 01 01 10 00 00")
			MXFLIB_CLASS_VECTOR("V10DeltaEntryArray", "Map Elements onto Slices", ClassUsageOptional, 0x3f09, "06 0e 2b 34 01 01 01 05  04 04 04 01 06 00 00 00")
				MXFLIB_CLASS_ITEM("Reorder", "TRUE=Apply Temporal Reordering", ClassUsageRequired, "Boolean", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 01 04 00 00 00", "false", NULL)
				MXFLIB_CLASS_ITEM("Slice", "Slice number in IndexEntry", ClassUsageRequired, "Uint8", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 01 02 00 00 00", NULL, NULL)
				MXFLIB_CLASS_ITEM("ElementDelta", "Delta from start of slice to this Element", ClassUsageRequired, "Uint32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 01 03 00 00 00", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
			MXFLIB_CLASS_VECTOR("V10IndexEntryArray", "Index from Edit Unit number to stream offset", ClassUsageDecoderRequired, 0x3f0a, "06 0e 2b 34 01 01 01 05  04 04 04 02 05 00 00 00")
				MXFLIB_CLASS_ITEM("TemporalOffset", "Offset in edit units from Display Order to Coded Order", ClassUsageRequired, "Int8", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 02 03 00 00 00", "0", NULL)
				MXFLIB_CLASS_ITEM("AnchorOffset", "Offset in edit units to previous Anchor Frame. The value is zero if this is an anchor frame.", ClassUsageRequired, "Int8", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 02 04 00 00 00", NULL, NULL)
				MXFLIB_CLASS_ITEM("Flags", "Flags for this Edit Unit", ClassUsageRequired, "Uint8", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 02 02 00 00 00", "128", NULL)
				MXFLIB_CLASS_ITEM("StreamOffset", "Offset in bytes from the first KLV element in this Edit Unit within the Essence Container", ClassUsageRequired, "Uint64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 02 01 00 00 00", NULL, NULL)
				MXFLIB_CLASS_ARRAY("SliceOffsetArray", "Array of offsets in bytes from the Stream Offset to the start of each slice.", ClassUsageRequired, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 01 05 00 00 00")
					MXFLIB_CLASS_ITEM("SliceOffset", "The offset in bytes from the Stream Offset to the start of this slice.", ClassUsageRequired, "Uint32", 4, 4, 0x0000, "", NULL, NULL)
				MXFLIB_CLASS_ARRAY_END
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("IndexTableSegment", "A segment of an Index Table", "IndexTableSegmentBase", "06 0e 2b 34 02 53 01 01  0d 01 02 01 01 10 01 00")
			MXFLIB_CLASS_VECTOR("DeltaEntryArray", "Map Elements onto Slices", ClassUsageOptional, 0x3f09, "06 0e 2b 34 01 01 01 05  04 04 04 01 06 00 00 00")
				MXFLIB_CLASS_ITEM("PosTableIndex", "Index into PosTable (or Apply Temporta Reordering if -1)", ClassUsageRequired, "Int8", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 01 04 00 00 00", "0", NULL)
				MXFLIB_CLASS_ITEM("Slice", "Slice number in IndexEntry", ClassUsageRequired, "Uint8", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 01 02 00 00 00", "0", NULL)
				MXFLIB_CLASS_ITEM("ElementDelta", "Delta from start of slice to this Element", ClassUsageRequired, "Uint32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 01 03 00 00 00", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
			MXFLIB_CLASS_VECTOR("IndexEntryArray", "Index from Edit Unit number to stream offset", ClassUsageDecoderRequired, 0x3f0a, "06 0e 2b 34 01 01 01 05  04 04 04 02 05 00 00 00")
				MXFLIB_CLASS_ITEM("TemporalOffset", "Offset in edit units from Display Order to Coded Order", ClassUsageRequired, "Int8", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 02 03 00 00 00", "0", NULL)
				MXFLIB_CLASS_ITEM("AnchorOffset", "Offset in edit units to previous Anchor Frame. The value is zero if this is an anchor frame.", ClassUsageRequired, "Int8", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 02 04 00 00 00", "0", NULL)
				MXFLIB_CLASS_ITEM("Flags", "Flags for this Edit Unit", ClassUsageRequired, "Uint8", 1, 1, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 02 02 00 00 00", "128", NULL)
				MXFLIB_CLASS_ITEM("StreamOffset", "Offset in bytes from the first KLV element in this Edit Unit within the Essence Container", ClassUsageRequired, "Uint64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 02 01 00 00 00", NULL, NULL)
				MXFLIB_CLASS_ARRAY("SliceOffsetArray", "Array of offsets in bytes from the Stream Offset to the start of each slice.", ClassUsageRequired, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 01 05 00 00 00")
					MXFLIB_CLASS_ITEM("SliceOffset", "The offset in bytes from the Stream Offset to the start of this slice.", ClassUsageRequired, "Uint32", 4, 4, 0x0000, "", NULL, NULL)
				MXFLIB_CLASS_ARRAY_END
				MXFLIB_CLASS_ARRAY("PosTableArray", "Array of fractional position offsets from the start of the content package to the synchronized sample in the Content Package", ClassUsageRequired, 0x0000, "06 0e 2b 34 01 01 01 04  04 04 04 01 08 00 00 00")
					MXFLIB_CLASS_ITEM("PosTable", "The fractional position offset from the start of the content package to the synchronized sample in the Content Package", ClassUsageRequired, "Rational", 8, 8, 0x0000, "", NULL, NULL)
				MXFLIB_CLASS_ARRAY_END
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_FIXEDPACK("RandomIndexMetadata", "Random Index Pack", "", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 11 01 00")
			MXFLIB_CLASS_ARRAY("PartitionArray", "Array of Partition Start Positions", ClassUsageRequired, 0x0000, "")
				MXFLIB_CLASS_ITEM("BodySID", "Stream ID of the Body in this partition", ClassUsageRequired, "Uint32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  01 03 04 04 00 00 00 00", NULL, NULL)
				MXFLIB_CLASS_ITEM("ByteOffset", "Byte offset from file start (1st byte of the file which is numbered 0) to the 1st byte of the Partition Pack Key", ClassUsageRequired, "Uint64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 04  06 09 02 01 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ARRAY_END
			MXFLIB_CLASS_ITEM("Length", "Overall Length of this Pack including the Set Key and BER Length fields", ClassUsageRequired, "Uint32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  04 06 10 01 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("RandomIndexMetadataV10", "Random Index Pack (v10)", "RandomIndexMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 11 00 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_SET("Preface", "Preface Set", "GenerationInterchangeObject", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 2f 00")
			MXFLIB_CLASS_ITEM("LastModifiedDate", "Date & time of the last modification of the file", ClassUsageRequired, "Timestamp", 8, 8, 0x3b02, "06 0e 2b 34 01 01 01 02  07 02 01 10 02 04 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Version", "The value shall be 258 (i.e. v1.2)", ClassUsageRequired, "VersionType", 2, 2, 0x3b05, "06 0e 2b 34 01 01 01 02  03 01 02 01 05 00 00 00", "258", NULL)
			MXFLIB_CLASS_ITEM("ObjectModelVersion", "Simple integer version number of Object Model", ClassUsageOptional, "Uint32", 4, 4, 0x3b07, "06 0e 2b 34 01 01 01 02  03 01 02 01 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM_REF("PrimaryPackage", "The primary Package in this file", ClassUsageOptional, "UUID", 16, 16, 0x3b08, "06 0e 2b 34 01 01 01 04  06 01 01 04 01 08 00 00", ClassRefWeak, "GenericPackage", NULL, NULL)
			MXFLIB_CLASS_VECTOR_REF("Identifications", "Ordered array of strong references to Identification sets recording all modifications to the file", ClassUsageEncoderRequired, 0x3b06, "06 0e 2b 34 01 01 01 02  06 01 01 04 06 04 00 00", ClassRefStrong, "Identification")
				MXFLIB_CLASS_ITEM("Identification", "Reference to Identification Set", ClassUsageOptional, "UUID", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
			MXFLIB_CLASS_ITEM_REF("ContentStorage", "Strong reference to Content Storage object", ClassUsageRequired, "UUID", 16, 16, 0x3b03, "06 0e 2b 34 01 01 01 02  06 01 01 04 02 01 00 00", ClassRefStrong, "ContentStorage", NULL, NULL)
			MXFLIB_CLASS_ITEM("OperationalPattern", "Universal Label of the Operational Pattern which this file complies to (repeat of Partition Pack value)", ClassUsageRequired, "Label", 16, 16, 0x3b09, "06 0e 2b 34 01 01 01 05  01 02 02 03 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_VECTOR("EssenceContainers", "Unordered batch of ULs of Essence Containers used in or referenced by this file (repeat of Partition Pack value)", ClassUsageRequired, 0x3b0a, "06 0e 2b 34 01 01 01 05  01 02 02 10 02 01 00 00")
				MXFLIB_CLASS_ITEM("EssenceContainer", "Label of Essence Container", ClassUsageOptional, "Label", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
			MXFLIB_CLASS_VECTOR("DMSchemes", "An unordered batch of Universal Labels of all the Descriptive Metadata schemes used in this file", ClassUsageRequired, 0x3b0b, "06 0e 2b 34 01 01 01 05  01 02 02 10 02 02 00 00")
				MXFLIB_CLASS_ITEM("DMScheme", "Universal Labels of Descriptive Metadata Scheme", ClassUsageOptional, "Label", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("Identification", "Identification set", "InterchangeObject", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 30 00")
			MXFLIB_CLASS_ITEM("ThisGenerationUID", "This Generation Identifier", ClassUsageRequired, "UUID", 16, 16, 0x3c09, "06 0e 2b 34 01 01 01 02  05 20 07 01 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("CompanyName", "Manufacturer of the equipment or application that created or modified the file", ClassUsageRequired, "UTF16String", 0, 0, 0x3c01, "06 0e 2b 34 01 01 01 02  05 20 07 01 02 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ProductName", "Name of the application which created or modified this file", ClassUsageRequired, "UTF16String", 0, 0, 0x3c02, "06 0e 2b 34 01 01 01 02  05 20 07 01 03 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ProductVersion", "Maj.min.tweak.build.rel  version number of this application", ClassUsageOptional, "ProductVersion", 10, 10, 0x3c03, "06 0e 2b 34 01 01 01 02  05 20 07 01 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("VersionString", "Human readable name of the application version", ClassUsageRequired, "UTF16String", 0, 0, 0x3c04, "06 0e 2b 34 01 01 01 02  05 20 07 01 05 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ProductUID", "A unique identification for the product which created this file (defined by the manufacturer)", ClassUsageRequired, "UUID", 16, 16, 0x3c05, "06 0e 2b 34 01 01 01 02  05 20 07 01 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ModificationDate", "Time & date an application created or modified this file and created this Identification set", ClassUsageRequired, "Timestamp", 8, 8, 0x3c06, "06 0e 2b 34 01 01 01 02  07 02 01 10 02 03 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ToolkitVersion", "Maj.min.tweak.build.rel version of software or hardware codec used", ClassUsageOptional, "ProductVersion", 10, 10, 0x3c07, "06 0e 2b 34 01 01 01 02  05 20 07 01 0a 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Platform", "Human readable name of the operating system used.", ClassUsageOptional, "UTF16String", 0, 0, 0x3c08, "06 0e 2b 34 01 01 01 02  05 20 07 01 06 01 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("ContentStorage", "Content Storage set", "GenerationInterchangeObject", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 18 00")
			MXFLIB_CLASS_VECTOR_REF("Packages", "Unordered batch of all packages used in this file", ClassUsageRequired, 0x1901, "06 0e 2b 34 01 01 01 02  06 01 01 04 05 01 00 00", ClassRefStrong, "GenericPackage")
				MXFLIB_CLASS_ITEM("Package", "Label of Essence Container", ClassUsageOptional, "UUID", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
			MXFLIB_CLASS_VECTOR_REF("EssenceContainerData", "Unordered batch of strong references to Essence Container Data sets used in this file", ClassUsageOptional, 0x1902, "06 0e 2b 34 01 01 01 02  06 01 01 04 05 02 00 00", ClassRefStrong, "EssenceContainerData")
				MXFLIB_CLASS_ITEM("EssenceContainer", "Label of Essence Container", ClassUsageOptional, "UUID", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("ContentStorageKludge", "Kludge for Version 10 Content Storage set", "", "")
			MXFLIB_CLASS_VECTOR_REF("V10Packages", "Unordered batch of all packages used in this file", ClassUsageRequired, 0x1901, "06 0e 2b 34 01 01 01 02  06 01 01 04 05 00 00 00", ClassRefStrong, "GenericPackage")
				MXFLIB_CLASS_ITEM("V10Package", "Label of Essence Container", ClassUsageOptional, "UUID", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("EssenceContainerData", "Essence Container Data set", "GenerationInterchangeObject", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 23 00")
			MXFLIB_CLASS_ITEM("LinkedPackageUID", "Identifier of the Package to which this set is linked as a UMID", ClassUsageRequired, "UMID", 32, 32, 0x2701, "06 0e 2b 34 01 01 01 02  06 01 01 06 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexSID", "ID of the Index Table for the Essence Container to which this set is linked", ClassUsageOptional, "Uint32", 4, 4, 0x3f06, "06 0e 2b 34 01 01 01 04  01 03 04 05 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BodySID", "ID of the Essence Container to which this set is linked", ClassUsageRequired, "Uint32", 4, 4, 0x3f07, "06 0e 2b 34 01 01 01 04  01 03 04 04 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("GenericPackage", "Defines a Generic Package set", "GenerationInterchangeObject", "")
			MXFLIB_CLASS_ITEM("PackageUID", "Unique Package Identifier as a UMID", ClassUsageRequired, "UMID", 32, 32, 0x4401, "06 0e 2b 34 01 01 01 01  01 01 15 10 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Name", "Human readable package name", ClassUsageOptional, "UTF16String", 0, 0, 0x4402, "06 0e 2b 34 01 01 01 01  01 03 03 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PackageCreationDate", "The date & time of creation of this package", ClassUsageRequired, "Timestamp", 8, 8, 0x4405, "06 0e 2b 34 01 01 01 02  07 02 01 10 01 03 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PackageModifiedDate", "The date & time of last modification of this package", ClassUsageRequired, "Timestamp", 8, 8, 0x4404, "06 0e 2b 34 01 01 01 02  07 02 01 10 02 05 00 00", NULL, NULL)
			MXFLIB_CLASS_VECTOR_REF("Tracks", "Array of Unique IDs of Tracks", ClassUsageRequired, 0x4403, "06 0e 2b 34 01 01 01 02  06 01 01 04 06 05 00 00", ClassRefStrong, "GenericTrack")
				MXFLIB_CLASS_ITEM("Track", "Track ID", ClassUsageOptional, "UUID", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("Locator", "", "GenerationInterchangeObject", "")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("NetworkLocator", "Network Locator set for location with a URL", "Locator", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 32 00")
			MXFLIB_CLASS_ITEM("URLString", "A URL indicating where the essence may be found.", ClassUsageRequired, "UTF16String", 0, 0, 0x4001, "06 0e 2b 34 01 01 01 01  01 02 01 01 01 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TextLocator", "Text Locator set for location with a human-readable text string", "Locator", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 33 00")
			MXFLIB_CLASS_ITEM("LocatorName", "Value of a human-readable locator text string for manual location of essence", ClassUsageRequired, "UTF16String", 0, 0, 0x4101, "06 0e 2b 34 01 01 01 02  01 04 01 02 01 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("GenericTrack", "Generic Track", "GenerationInterchangeObject", "")
			MXFLIB_CLASS_ITEM("TrackID", "ID of the track in this package (for linking to a SourceTrackID in a segment)", ClassUsageRequired, "Uint32", 4, 4, 0x4801, "06 0e 2b 34 01 01 01 02  01 07 01 01 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TrackNumber", "Number used to link to the track in the Essence Container if it exists", ClassUsageRequired, "Uint32", 4, 4, 0x4804, "06 0e 2b 34 01 01 01 02  01 04 01 03 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("TrackName", "Human readable name of the track type", ClassUsageOptional, "UTF16String", 0, 0, 0x4802, "06 0e 2b 34 01 01 01 02  01 07 01 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM_REF("Sequence", "Strong Reference to Sequence Set", ClassUsageRequired, "UUID", 16, 16, 0x4803, "06 0e 2b 34 01 01 01 02  06 01 01 04 02 04 00 00", ClassRefStrong, "StructuralComponent", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("StaticTrack", "", "GenericTrack", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 3a 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("Track", "Track", "GenericTrack", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 3b 00")
			MXFLIB_CLASS_ITEM("EditRate", "Edit Rate of Track", ClassUsageRequired, "Rational", 8, 8, 0x4b01, "06 0e 2b 34 01 01 01 02  05 30 04 05 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Origin", "An Offset used to resolved timeline references to this track. The start of the track has this timeline value measured in Edit Units.", ClassUsageRequired, "Position", 8, 8, 0x4b02, "06 0e 2b 34 01 01 01 02  07 02 01 03 01 03 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("EventTrack", "Event Track", "GenericTrack", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 39 00")
			MXFLIB_CLASS_ITEM("EventEditRate", "Edit Rate of Event Track", ClassUsageRequired, "Rational", 8, 8, 0x4901, "06 0e 2b 34 01 01 01 02  05 30 04 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EventOrigin", "An Offset used to resolved timeline references to this event track. The start of the event track has this timeline value measured in Edit Units.", ClassUsageOptional, "Position", 8, 8, 0x4902, "06 0e 2b 34 01 01 01 05  07 02 01 03 01 0b 00 00", "0", NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("StructuralComponent", "Structural Component Superclass", "GenerationInterchangeObject", "")
			MXFLIB_CLASS_ITEM("DataDefinition", "Data Definition - kind of data or metadata this structure refers to", ClassUsageRequired, "Label", 16, 16, 0x0201, "06 0e 2b 34 01 01 01 02  04 07 01 00 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Duration", "Duration (in units of edit rate)", ClassUsageBestEffort, "Length", 8, 8, 0x0202, "06 0e 2b 34 01 01 01 02  07 02 02 01 01 03 00 00", NULL, "-1")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("Sequence", "Sequence", "StructuralComponent", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 0f 00")
			MXFLIB_CLASS_VECTOR_REF("StructuralComponents", "Ordered array of strong references to Structural Components", ClassUsageRequired, 0x1001, "06 0e 2b 34 01 01 01 02  06 01 01 04 06 09 00 00", ClassRefStrong, "StructuralComponent")
				MXFLIB_CLASS_ITEM("StructuralComponent", "Structural Component", ClassUsageOptional, "UUID", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TimecodeComponent", "Timecode Component", "StructuralComponent", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 14 00")
			MXFLIB_CLASS_ITEM("RoundedTimecodeBase", "Integer frames per second", ClassUsageRequired, "Uint16", 2, 2, 0x1502, "06 0e 2b 34 01 01 01 02  04 04 01 01 02 06 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("StartTimecode", "Starting timecode", ClassUsageRequired, "Position", 8, 8, 0x1501, "06 0e 2b 34 01 01 01 02  07 02 01 03 01 05 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DropFrame", "Drop frame flag", ClassUsageRequired, "Boolean", 1, 1, 0x1503, "06 0e 2b 34 01 01 01 01  04 04 01 01 05 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("SourceClip", "Source Clip", "StructuralComponent", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 11 00")
			MXFLIB_CLASS_ITEM("StartPosition", "Offset into Essence measured in edit units of the track containing this segment", ClassUsageRequired, "Position", 8, 8, 0x1201, "06 0e 2b 34 01 01 01 02  07 02 01 03 01 04 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SourcePackageID", "ID of referenced Package as a UMID", ClassUsageRequired, "UMID", 32, 32, 0x1101, "06 0e 2b 34 01 01 01 02  06 01 01 03 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SourceTrackID", "Track ID of the referenced Track within the referenced Package", ClassUsageRequired, "Uint32", 4, 4, 0x1102, "06 0e 2b 34 01 01 01 02  06 01 01 03 02 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("DMSegment", "Descriptive Metadata Segment", "StructuralComponent", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 41 00")
			MXFLIB_CLASS_ITEM("EventStartPosition", "Offset into the descriptive metadata track in edit units", ClassUsageRequired, "Position", 8, 8, 0x0601, "06 0e 2b 34 01 01 01 02  07 02 01 03 03 03 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EventComment", "Description of the Descriptive Metadata Framework", ClassUsageOptional, "UTF16String", 0, 0, 0x0602, "06 0e 2b 34 01 01 01 02  05 30 04 04 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_VECTOR("TrackIDs", "An unordered list of track ID values that identify the tracks in this Package to which this DM Framework refers (if omitted, refers to all essence tracks)", ClassUsageDecoderRequired, 0x6102, "06 0e 2b 34 01 01 01 04  01 07 01 05 00 00 00 00")
				MXFLIB_CLASS_ITEM("TrackID", "Uint32", ClassUsageOptional, "Uint32", 4, 4, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
			MXFLIB_CLASS_ITEM_REF("DMFramework", "Strong Reference to the Descriptive Metadata Framework", ClassUsageDecoderRequired, "UUID", 16, 16, 0x6101, "06 0e 2b 34 01 01 01 05  06 01 01 04 02 0c 00 00", ClassRefStrong, "DM_Framework", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("DMSourceClip", "Descriptive Metadata SourceClip", "SourceClip", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 45 00")
			MXFLIB_CLASS_VECTOR("DMSourceClipTrackIDs", "An unordered list of track ID values that identify the tracks in this Package to which the referenced Descriptive Metadata refers (if omitted, refers to all essence tracks)", ClassUsageOptional, 0x6103, "06 0e 2b 34 01 01 01 05  01 07 01 06 00 00 00 00")
				MXFLIB_CLASS_ITEM("DMSourceClipTrackID", "Uint32", ClassUsageOptional, "Uint32", 4, 4, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("MaterialPackage", "Material Package set", "GenericPackage", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 36 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("SourcePackage", "File Package set", "GenericPackage", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 37 00")
			MXFLIB_CLASS_ITEM_REF("Descriptor", "Strong Reference to the Descriptor", ClassUsageDecoderRequired, "UUID", 16, 16, 0x4701, "06 0e 2b 34 01 01 01 02  06 01 01 04 02 03 00 00", ClassRefStrong, "GenericDescriptor", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("GenericDescriptor", "Generic Descriptor", "GenerationInterchangeObject", "")
			MXFLIB_CLASS_VECTOR_REF("Locators", "Ordered array of strong references to Locator sets", ClassUsageOptional, 0x2f01, "06 0e 2b 34 01 01 01 02  06 01 01 04 06 03 00 00", ClassRefStrong, "Locator")
				MXFLIB_CLASS_ITEM("Locator", "Essence Locator", ClassUsageOptional, "UUID", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("FileDescriptor", "File Descriptor", "GenericDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 25 00")
			MXFLIB_CLASS_ITEM("LinkedTrackID", "Link to (i.e. value of) the Track ID of the Track in this Package to which the Descriptor applies", ClassUsageOptional, "Uint32", 0, 0, 0x3006, "06 0e 2b 34 01 01 01 05  06 01 01 03 05 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SampleRate", "The field or frame rate of the Essence Container (not the essence sampling clock rate)", ClassUsageRequired, "Rational", 8, 8, 0x3001, "06 0e 2b 34 01 01 01 01  04 06 01 01 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ContainerDuration", "Duration of Essence Container (measured in Edit Units)", ClassUsageOptional, "Length", 8, 8, 0x3002, "06 0e 2b 34 01 01 01 01  04 06 01 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EssenceContainer", "The UL identifying the Essence Container described by this Descriptor", ClassUsageRequired, "Label", 16, 16, 0x3004, "06 0e 2b 34 01 01 01 02  06 01 01 04 01 02 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Codec", "UL to identify a codec compatible with this Essence Container", ClassUsageOptional, "Label", 16, 16, 0x3005, "06 0e 2b 34 01 01 01 02  06 01 01 04 01 03 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("GenericPictureEssenceDescriptor", "Defines the Picture Essence Descriptor set", "FileDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 27 00")
			MXFLIB_CLASS_ITEM("SignalStandard", "Underlying signal standard", ClassUsageOptional, "Uint8", 1, 1, 0x3215, "06 0e 2b 34 01 01 01 05  04 05 01 13 00 00 00 00", "1", NULL)
			MXFLIB_CLASS_ITEM("FrameLayout", "Interlace or Progressive layout", ClassUsageBestEffort, "Uint8", 1, 1, 0x320c, "06 0e 2b 34 01 01 01 01  04 01 03 01 04 00 00 00", NULL, "255")
			MXFLIB_CLASS_ITEM("StoredWidth", "Horizontal Size of active picture", ClassUsageBestEffort, "Uint32", 4, 4, 0x3203, "06 0e 2b 34 01 01 01 01  04 01 05 02 02 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("StoredHeight", "Vertical Field Size of active picture", ClassUsageBestEffort, "Uint32", 4, 4, 0x3202, "06 0e 2b 34 01 01 01 01  04 01 05 02 01 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("StoredF2Offset", "Topness Adjustment for stored picture", ClassUsageOptional, "Int32", 4, 4, 0x3216, "06 0e 2b 34 01 01 01 05  04 01 03 02 08 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("SampledWidth", "Sampled width supplied to codec", ClassUsageOptional, "Uint32", 4, 4, 0x3205, "06 0e 2b 34 01 01 01 01  04 01 05 01 08 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SampledHeight", "Sampled height supplied to codec", ClassUsageOptional, "Uint32", 4, 4, 0x3204, "06 0e 2b 34 01 01 01 01  04 01 05 01 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SampledXOffset", "Offset from sampled to stored width", ClassUsageOptional, "Int32", 4, 4, 0x3206, "06 0e 2b 34 01 01 01 01  04 01 05 01 09 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("SampledYOffset", "Offset from sampled to stored", ClassUsageOptional, "Int32", 4, 4, 0x3207, "06 0e 2b 34 01 01 01 01  04 01 05 01 0A 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("DisplayHeight", "Displayed Height placed in Production Aperture", ClassUsageOptional, "Uint32", 4, 4, 0x3208, "06 0e 2b 34 01 01 01 01  04 01 05 01 0B 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DisplayWidth", "Displayed Width placed in Production Aperture", ClassUsageOptional, "Uint32", 4, 4, 0x3209, "06 0e 2b 34 01 01 01 01  04 01 05 01 0C 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DisplayXOffset", "The horizontal offset from the (in pixels) of the picture as displayed", ClassUsageOptional, "Int32", 4, 4, 0x320a, "06 0e 2b 34 01 01 01 01  04 01 05 01 0D 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DisplayYOffset", "The vertical offset (in pixels) of the picture as displayed", ClassUsageOptional, "Int32", 4, 4, 0x320b, "06 0e 2b 34 01 01 01 01  04 01 05 01 0E 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DisplayF2Offset", "Topness Adjustment for Displayed Picture", ClassUsageOptional, "Int32", 4, 4, 0x3217, "06 0e 2b 34 01 01 01 05  04 01 03 02 07 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("AspectRatio", "Specifies the horizontal to vertical aspect ratio of the whole image as it is to be presented to avoid geometric distortion (and hence including any black edges)", ClassUsageBestEffort, "Rational", 8, 8, 0x320e, "06 0e 2b 34 01 01 01 01  04 01 01 01 01 00 00 00", NULL, "0/0")
			MXFLIB_CLASS_ITEM("ActiveFormatDescriptor", "Specifies the intended framing of the content within the displayed image", ClassUsageOptional, "Uint8", 1, 1, 0x3218, "06 0e 2b 34 01 01 01 05  04 01 03 02 09 00 00 00", NULL, NULL)
			MXFLIB_CLASS_VECTOR("VideoLineMap", "First active line in each field", ClassUsageBestEffort, 0x320d, "06 0e 2b 34 01 01 01 02  04 01 03 02 05 00 00 00")
				MXFLIB_CLASS_ITEM("VideoLineMapEntry", "First active line in field", ClassUsageBestEffort, "Int32", 0, 0, 0x0000, "", NULL, "0")
			MXFLIB_CLASS_VECTOR_END
			MXFLIB_CLASS_ITEM("AlphaTransparency", "Is Alpha Inverted", ClassUsageOptional, "Uint8", 1, 1, 0x320f, "06 0e 2b 34 01 01 01 02  05 20 01 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Gamma", "Registered UL of known Gamma", ClassUsageOptional, "Label", 16, 16, 0x3210, "06 0e 2b 34 01 01 01 02  04 01 02 01 01 01 02 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ImageAlignmentOffset", "Byte Boundary alignment required for Low Level Essence Storage", ClassUsageOptional, "Uint32", 4, 4, 0x3211, "06 0e 2b 34 01 01 01 02  04 18 01 01 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ImageStartOffset", "Unused bytes before start of stored data", ClassUsageOptional, "Uint32", 4, 4, 0x3213, "06 0e 2b 34 01 01 01 02  04 18 01 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ImageEndOffset", "Unused bytes before start of stored data", ClassUsageOptional, "Uint32", 4, 4, 0x3214, "06 0e 2b 34 01 01 01 02  04 18 01 03 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("FieldDominance", "The number of the field which is considered temporally to come first", ClassUsageOptional, "Uint8", 1, 1, 0x3212, "06 0e 2b 34 01 01 01 02  04 01 03 01 06 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PictureEssenceCoding", "UL identifying the Picture Compression Scheme", ClassUsageDecoderRequired, "Label", 16, 16, 0x3201, "06 0e 2b 34 01 01 01 02  04 01 06 01 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("CDCIEssenceDescriptor", "Defines the CDCI Picture Essence Descriptor set", "GenericPictureEssenceDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 28 00")
			MXFLIB_CLASS_ITEM("ComponentDepth", "Number of active bits per sample (e.g. 8, 10, 16)", ClassUsageBestEffort, "Uint32", 4, 4, 0x3301, "06 0e 2b 34 01 01 01 02  04 01 05 03 0A 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("HorizontalSubsampling", "Specifies the H colour subsampling", ClassUsageBestEffort, "Uint32", 4, 4, 0x3302, "06 0e 2b 34 01 01 01 01  04 01 05 01 05 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("VerticalSubsampling", "Specifies the V colour subsampling", ClassUsageOptional, "Uint32", 4, 4, 0x3308, "06 0e 2b 34 01 01 01 02  04 01 05 01 10 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ColorSiting", "Enumerated value describing the color siting", ClassUsageOptional, "Uint8", 4, 4, 0x3303, "06 0e 2b 34 01 01 01 01  04 01 05 01 06 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ReversedByteOrder", "a FALSE value denotes Chroma followed by Luma pexels according to ITU Rec. 601", ClassUsageOptional, "Boolean", 1, 1, 0x330b, "06 0e 2b 34 01 01 01 05  03 01 02 01 0a 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PaddingBits", "Bits to round up each pixel to stored size", ClassUsageOptional, "Uint16", 2, 2, 0x3307, "06 0e 2b 34 01 01 01 02  04 18 01 04 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("AlphaSampleDepth", "Number of bits per alpha sample", ClassUsageOptional, "Uint32", 4, 4, 0x3309, "06 0e 2b 34 01 01 01 02  04 01 05 03 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BlackRefLevel", "Black refernece level e.g. 16 or 64 (8 or 10-bits)", ClassUsageOptional, "Uint32", 4, 4, 0x3304, "06 0e 2b 34 01 01 01 01  04 01 05 03 03 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("WhiteReflevel", "White reference level e.g. 235 or 943 (8 or 10 bits)", ClassUsageOptional, "Uint32", 4, 4, 0x3305, "06 0e 2b 34 01 01 01 01  04 01 05 03 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ColorRange", "Color range e.g. 225 or 897 (8 or 10 bits)", ClassUsageOptional, "Uint32", 4, 4, 0x3306, "06 0e 2b 34 01 01 01 02  04 01 05 03 05 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("RGBAEssenceDescriptor", "Defines the RGBA Picture Essence Descriptor set", "GenericPictureEssenceDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 29 00")
			MXFLIB_CLASS_ITEM("ComponentMaxRef", "Maximum value for RGB components, e.g. 235 or 940 (8 or 10 bits)", ClassUsageOptional, "Uint32", 4, 4, 0x3406, "06 0e 2b 34 01 01 01 05  04 01 05 03 0b 00 00 00", "255", NULL)
			MXFLIB_CLASS_ITEM("ComponentMinRef", "Minimum value for RGB components, e.g. 16 or 64 (8 or 10 bits)", ClassUsageOptional, "Uint32", 4, 4, 0x3407, "06 0e 2b 34 01 01 01 05  04 01 05 03 0c 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("AlphaMaxRef", "Maximum value for alpha component, e.g. 235 or 940 (8 or 10 bits)", ClassUsageOptional, "Uint32", 4, 4, 0x3408, "06 0e 2b 34 01 01 01 05  04 01 05 03 0d 00 00 00", "255", NULL)
			MXFLIB_CLASS_ITEM("AlphaMinRef", "Minimum value for alpha component, e.g. 16 or 64 (8 or 10 bits)", ClassUsageOptional, "Uint32", 4, 4, 0x3409, "06 0e 2b 34 01 01 01 05  04 01 05 03 0e 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("ScanningDirection", "Enumerated Scanning Direction", ClassUsageOptional, "Uint8", 1, 1, 0x3405, "06 0e 2b 34 01 01 01 05  04 01 04 04 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PixelLayout", "Pixel Layout", ClassUsageBestEffort, "RGBALayout", 16, 16, 0x3401, "06 0e 2b 34 01 01 01 02  04 01 05 03 06 00 00 00", NULL, "{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}")
			MXFLIB_CLASS_ITEM("Palette", "Palette", ClassUsageOptional, "DataValue", 0, 0, 0x3403, "06 0e 2b 34 01 01 01 02  04 01 05 03 08 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PaletteLayout", "Palette Layout", ClassUsageOptional, "RGBALayout", 16, 16, 0x3404, "06 0e 2b 34 01 01 01 02  04 01 05 03 09 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("GenericSoundEssenceDescriptor", "Defines the Sound Essence Descriptor set", "FileDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 42 00")
			MXFLIB_CLASS_ITEM("AudioSamplingRate", "Sampling rate of the audio essence", ClassUsageBestEffort, "Rational", 8, 8, 0x3d03, "06 0e 2b 34 01 01 01 05  04 02 03 01 01 01 00 00", "48000/1", "0/0")
			MXFLIB_CLASS_ITEM("Locked", "Boolean indicating that the Number of samples per frame is locked or unlocked (non-0 = locked)", ClassUsageRequired, "Boolean", 1, 1, 0x3d02, "06 0e 2b 34 01 01 01 04  04 02 03 01 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("AudioRefLevel", "Audio reference level which gives the number of dBm for 0VU", ClassUsageOptional, "Int8", 1, 1, 0x3d04, "06 0e 2b 34 01 01 01 01  04 02 01 01 03 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ElectroSpatialFormulation", "E.g. mono, dual mono, stereo, A,B etc (enum)", ClassUsageOptional, "Uint8", 1, 1, 0x3d05, "06 0e 2b 34 01 01 01 01  04 02 01 01 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ChannelCount", "Number of Sound Channels", ClassUsageBestEffort, "Uint32", 4, 4, 0x3d07, "06 0e 2b 34 01 01 01 05  04 02 01 01 04 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("QuantizationBits", "Number of quantization bits", ClassUsageBestEffort, "Uint32", 4, 4, 0x3d01, "06 0e 2b 34 01 01 01 04  04 02 03 03 04 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("DialNorm", "Gain to be applied to normalise perceived loudness of the clip, defined by ATSC A/53 (1dB per step)", ClassUsageOptional, "Int8", 1, 1, 0x3d0c, "06 0e 2b 34 01 01 01 05  04 02 07 01 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SoundEssenceCompression", "UL identifying the Sound Compression Scheme", ClassUsageDecoderRequired, "Label", 16, 16, 0x3d06, "06 0e 2b 34 01 01 01 02  04 02 04 02 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("GenericDataEssenceDescriptor", "Defines the Data Essence Descriptor set", "FileDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 43 00")
			MXFLIB_CLASS_ITEM("DataEssenceCoding", "Specifies the data essence coding type", ClassUsageDecoderRequired, "Label", 16, 16, 0x3e01, "06 0e 2b 34 01 01 01 05  04 03 03 02 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("MultipleDescriptor", "Defines the Multiple Descriptor set", "FileDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 44 00")
			MXFLIB_CLASS_VECTOR_REF("SubDescriptorUIDs", "Unordered array of strong references to File Descriptor sets (1 per interleaved item within the Essence Container)", ClassUsageRequired, 0x3f01, "06 0e 2b 34 01 01 01 04  06 01 01 04 06 0b 00 00", ClassRefStrong, "FileDescriptor")
				MXFLIB_CLASS_ITEM("SubDescriptorUID", "File Descriptor", ClassUsageOptional, "UUID", 16, 16, 0x0000, "", NULL, NULL)
			MXFLIB_CLASS_VECTOR_END
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("MPEG2VideoDescriptor", "Defines the MPEG2 Picture Essence Descriptor set", "CDCIEssenceDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 51 00")
			MXFLIB_CLASS_ITEM("SingleSequence", "TRUE if the essence consists of a single MPEG sequence. False if there are a number of sequences. This flag implies that the sequence header information is not varying in the essence stream.", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 02 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ConstantBFrames", "TRUE if the number of B frames is always constant", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 03 00 00", "false", NULL)
			MXFLIB_CLASS_ITEM("CodedContentType", "0= \"Unknown\",1= \"Progressive\", 2= \"Interlaced\", 3= \"Mixed\": an enumerated value which tells if the underlying content which was MPEG coded was of a known type", ClassUsageOptional, "Uint8", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 04 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("LowDelay", "TRUE if low delay mode was used in the sequence", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 05 00 00", "false", NULL)
			MXFLIB_CLASS_ITEM("ClosedGOP", "TRUE if closed_gop is set in all GOP Headers, per 13818-1 IBP descriptor", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 06 00 00", "false", NULL)
			MXFLIB_CLASS_ITEM("IdenticalGOP", "TRUE if every GOP in the sequence is constructed the same, per 13818-1 IBP descriptor", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 07 00 00", "false", NULL)
			MXFLIB_CLASS_ITEM("MaxGOP", "Specifies the maximum occurring spacing between I frames, per 13818-1 IBP descriptor. A value of 0 or the absence of this property implies no limit to the maximum GOP", ClassUsageOptional, "Uint16", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 08 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("BPictureCount", "Specifies the maximum number of B pictures between P or I frames, equivalent to 13818-2 annex D (M-1)", ClassUsageOptional, "Uint16", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 09 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BitRate", "Maximum bit rate of MPEG video elementary stream in bit/s as defined in ISO-13818-2 bit_rate property", ClassUsageOptional, "Uint32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 0b 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ProfileAndLevel", "Specifies the MPEG-2 video profile and level. The value is taken directly from the profile_and_level_indication in the MPEG-2 sequence header extension. For main profile @ main level, the value is 0x48. For 4:2:2 profile @ main level, the value is 0x85", ClassUsageOptional, "Uint8", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 0a 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("WaveAudioDescriptor", "Defines the Wave Audio Essence Descriptor Set", "GenericSoundEssenceDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 48 00")
			MXFLIB_CLASS_ITEM("BlockAlign", "Sample Block alignment", ClassUsageRequired, "Uint16", 0, 0, 0x3d0a, "06 0e 2b 34 01 01 01 05  04 02 03 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SequenceOffset", "Zero-based ordinal frame number of first essence data within five-frame sequence", ClassUsageOptional, "Uint8", 0, 0, 0x3d0b, "06 0e 2b 34 01 01 01 05  04 02 03 02 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("AvgBps", "Average Bytes per second", ClassUsageRequired, "Uint32", 0, 0, 0x3d09, "06 0e 2b 34 01 01 01 05  04 02 03 03 05 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PeakEnvelope", "Peak Envelope from <LEVL> Chunk", ClassUsageOptional, "Stream", 0, 0, 0x3d0e, "06 0e 2b 34 01 01 01 05  04 02 03 01 0e 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("DM_Framework", "Superclass for all concrete DM Frameworks", "GenerationInterchangeObject", "06 0e 2b 34 02 53 01 01  0d 01 04 01 00 00 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("DM_Set", "Superclass for all concrete DM Frameworks", "GenerationInterchangeObject", "06 0e 2b 34 02 53 01 01 0d 01 04 00 00 00 00 00")
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END
