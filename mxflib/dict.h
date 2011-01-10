
	// Types definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types)
		MXFLIB_TYPE_BASIC("Float32", "32 bit IEEE Floating Point", "", 4, false, false)
		MXFLIB_TYPE_BASIC("Float64", "64 bit IEEE Floating Point", "", 8, false, false)
		MXFLIB_TYPE_BASIC("Float80", "80 bit IEEE Floating Point", "", 10, false, false)
		MXFLIB_TYPE_BASIC_EX("Int8", "8 bit integer", "urn:x-ul:060E2B34.0104.0101.01010500.00000000", 1, false, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_BASIC_EX("Int16", "16 bit integer", "urn:x-ul:060E2B34.0104.0101.01010600.00000000", 2, false, NULL, TypeFlags_Endian + TypeFlags_Baseline)
		MXFLIB_TYPE_BASIC("Int24", "24 bit integer", "", 3, true, false)
		MXFLIB_TYPE_BASIC_EX("Int32", "32 bit integer", "urn:x-ul:060E2B34.0104.0101.01010700.00000000", 4, false, NULL, TypeFlags_Endian + TypeFlags_Baseline)
		MXFLIB_TYPE_BASIC("Int64", "64 bit integer", "urn:x-ul:060E2B34.0104.0101.01010800.00000000", 8, true, false)
		MXFLIB_TYPE_BASIC_EX("UInt8", "8 bit unsigned integer", "urn:x-ul:060E2B34.0104.0101.01010100.00000000", 1, false, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_BASIC_EX("UInt16", "16 bit unsigned integer", "urn:x-ul:060E2B34.0104.0101.01010200.00000000", 2, false, NULL, TypeFlags_Endian + TypeFlags_Baseline)
		MXFLIB_TYPE_BASIC("UInt24", "24 bit unsigned integer", "", 3, true, false)
		MXFLIB_TYPE_BASIC_EX("UInt32", "32 bit unsigned integer", "urn:x-ul:060E2B34.0104.0101.01010300.00000000", 4, false, NULL, TypeFlags_Endian + TypeFlags_Baseline)
		MXFLIB_TYPE_BASIC("UInt64", "64 bit unsigned integer", "urn:x-ul:060E2B34.0104.0101.01010400.00000000", 8, true, false)
		MXFLIB_TYPE_BASIC_EX("UUID", "Universally Unique ID, as per RFC 4122", "urn:x-ul:060E2B34.0104.0101.01030300.00000000", 16, false, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_BASIC_EX("UL", "SMPTE Universal Label", "urn:x-ul:060E2B34.0104.0101.01030400.00000000", 16, false, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_BASIC("UID", "Generic Unique ID, used by DMS-1", "", 16, false, false)
		MXFLIB_TYPE_BASIC_EX("UMID", "SMPTE UMID", "urn:x-ul:060E2B34.0104.0101.01300100.00000000", 32, false, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION("AUID", "a UL or a swapped UUID", "UL", "urn:x-ul:060E2B34.0104.0101.01030100.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("IDAU", "a UUID or a swapped UL", "UUID", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION_EX("StrongRef", "Strong Reference", "UUID", "urn:x-ul:060E2B34.0104.0101.05020000.00000000", 0, false, ClassRefStrong, "InterchangeObject", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_REF("GlobalRef", "Abstract global reference to a UUID-identified object", "UUID", "", 0, false, ClassRefGlobal, "")
		MXFLIB_TYPE_INTERPRETATION_EX("WeakRef", "Weak Reference", "UUID", "urn:x-ul:060E2B34.0104.0101.05010000.00000000", 0, false, ClassRefWeak, "", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_REF("DictRef", "Dict Reference (to a DefinitionObject)", "UUID", "", 0, false, ClassRefDict, "DefinitionObject")
		MXFLIB_TYPE_INTERPRETATION_REF("MetaRef", "Meta Reference (to a MetaDefinitionObject)", "UUID", "", 0, false, ClassRefMeta, "MetaDefinition")
		MXFLIB_TYPE_INTERPRETATION("PackageID", "Package ID", "UMID", "urn:x-ul:060E2B34.0104.0101.01030200.00000000", 0, false)
		MXFLIB_TYPE_MULTIPLE("AUIDArray", "Array of AUIDs", "AUID", "urn:x-ul:060E2B34.0104.0101.04010600.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("AUIDSet", "Set of AUIDs", "AUID", "urn:x-ul:060E2B34.0104.0101.04030100.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE_EX("StrongRefArray", "Array of StrongRefs", "StrongRef", "", ARRAYIMPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("StrongRefBatch", "Batch of StrongRefs", "StrongRef", "", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("WeakRefArray", "Array of WeakRefs", "StrongRef", "", ARRAYIMPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("WeakRefBatch", "Batch of WeakRefs", "WeakRef", "", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION("AUL", "a UL or erroneously a swapped UUID", "AUID", "urn:x-ul:060E2B34.0104.0101.01030500.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("AULref", "an AUL or a FirmRef to a DefinitionObject", "UL", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("ULID", "a UL or erroneously an unswapped UUID", "UL", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("IDAUstruct", "a UUID or a swapped UL, stored as an end-sensitive struct", "UUID", "", 0, false)
		MXFLIB_TYPE_MULTIPLE("AULSet", "Set of AUL", "AUL", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_INTERPRETATION_EX("VersionType", "Version number (created from major*256 + minor)", "UInt16", "urn:x-ul:060E2B34.0104.0101.03010300.00000000", 0, false, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("UTF16", "Unicode UTF-16 coded character", "UInt16", "urn:x-ul:060E2B34.0104.0101.01100100.00000000", 0, false, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("Boolean", "Boolean", "urn:x-ul:060E2B34.0104.0101.01010100.00000000", "urn:x-ul:060E2B34.0104.0101.01040100.00000000", 0, false, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("ISO7", "ISO 7-Bit Coded Character", "UInt8", "urn:x-ul:060E2B34.0104.0101.01100300.00000000", 0, false, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("UTF", "Byte of a Unicode string of unknown format", "UInt8", "", 0, false, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("LengthType", "current~Length Length in Edit Units", "Int64", "urn:x-ul:060E2B34.0104.0101.01012002.00000000", 0, false, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("Position", "Position measured in Edit Units", "Int64", "urn:x-ul:060E2B34.0104.0101.01012001.00000000", 0, false, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("RGBACode", "Enumerated value specifying component in an RGBALayoutItem", "UInt8", "urn:x-ul:060E2B34.0104.0101.0201010e.00000000", 0, false, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION("UTF7", "RFC 2152 7-Bit Coded UNICODE Character", "ISO7", "", 0, false)
		MXFLIB_TYPE_MULTIPLE("UTFString", "Unicode coded string - unknown format", "UTF", "", ARRAYSTRING, 0)
		MXFLIB_TYPE_MULTIPLE_EX("UTF16String", "Unicode UTF-16 coded string", "UTF16", "urn:x-ul:060E2B34.0104.0101.01100200.00000000", ARRAYSTRING, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE("UInt32Array", "Array of UInt32", "UInt32", "urn:x-ul:060e2b34.0104.0101.04010900.00000000", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("Int64Array", "Array of Int64", "Int64", "urn:x-ul:060E2B34.0104.0101.04010400.00000000", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("Int64Batch", "Batch of Int64", "Int64", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("UInt8Array", "Array of UInt8", "UInt8", "urn:x-ul:060E2B34.0104.0101.04010100.00000000", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE_EX("ISO7String", "ISO 7-Bit coded string", "ISO7", "urn:x-ul:060E2B34.0104.0101.01100400.00000000", ARRAYSTRING, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE("UTF7String", "RFC 2152 7-Bit Coded UNICODE string", "UTF7", "urn:x-ul:060E2B34.0104.0101.01200500.00000000", ARRAYSTRING, 0)
		MXFLIB_TYPE_MULTIPLE("Int32Batch", "Batch of Int32 values", "Int32", "urn:x-ul:060E2B34.0104.0101.04010300.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("UInt32Batch", "Batch of UInt32 values", "UInt32", "urn:x-ul:060E2B34.0104.0101.04030200.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("RAW", "Raw data bytes, unknown representation", "UInt8", "", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("RAWBatch", "Batch of Raw data items", "RAW", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("ChannelStatusModeTypeBatch", "Batch of ChannelStatusModeTypes", "ChannelStatusModeType", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_ENUM_EX("LayoutType", "Frame Layout Type", "UInt8", "urn:x-ul:060e2b34.0104.0101.02010108.00000000", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_ENUM_VALUE("FullFrame", "Full Frame", "0")
			MXFLIB_TYPE_ENUM_VALUE("SeparateFields", "Separate Fields", "1")
			MXFLIB_TYPE_ENUM_VALUE("OneField", "One Field", "2")
			MXFLIB_TYPE_ENUM_VALUE("MixedFields", "Mixed Fields", "3")
			MXFLIB_TYPE_ENUM_VALUE("SegmentedFrame", "Segmented Frame", "4")
			MXFLIB_TYPE_ENUM_VALUE("UnknownLayout", "UnknownL Layout", "255")
		MXFLIB_TYPE_ENUM_END
		MXFLIB_TYPE_ENUM_EX("ProductReleaseKind", "Product Release Kind", "UInt16", "", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_ENUM_VALUE("VersionUnknown", "Unknown", "0")
			MXFLIB_TYPE_ENUM_VALUE("VersionReleased", "Released", "1")
			MXFLIB_TYPE_ENUM_VALUE("VersionDebug", "Debug", "2")
			MXFLIB_TYPE_ENUM_VALUE("VersionPatched", "Patched", "3")
			MXFLIB_TYPE_ENUM_VALUE("VersionBeta", "Beta", "4")
			MXFLIB_TYPE_ENUM_VALUE("VersionPrivateBuild", "Private Build", "5")
		MXFLIB_TYPE_ENUM_END
		MXFLIB_TYPE_ENUM_EX("ChannelStatusModeType", "Channel Status Mode", "UInt8", "urn:x-ul:060E2B34.0104.0101.02010125.00000000", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_ENUM_VALUE("ChannelStatusMode_None", "No channel status data is encoded", "0")
			MXFLIB_TYPE_ENUM_VALUE("ChannelStatusMode_Minimum", "AES3 Minimum (byte 0 bit 0 = '1')", "1")
			MXFLIB_TYPE_ENUM_VALUE("ChannelStatusMode_Standard", "AES3 Standard", "2")
			MXFLIB_TYPE_ENUM_VALUE("ChannelStatusMode_Fixed", "Fixed 24 byes of data in FixedChannelStatusData property", "3")
			MXFLIB_TYPE_ENUM_VALUE("ChannelStatusMode_Stream", "Stream of data within MXF Header Metadata", "4")
			MXFLIB_TYPE_ENUM_VALUE("ChannelStatusMode_Essence", "Stream of data multiplexed within MXF Body", "5")
		MXFLIB_TYPE_ENUM_END
		MXFLIB_TYPE_INTERPRETATION("Stream", "Data mapped to AAF Stream, MXF represents using a SID", "RAW", "urn:x-ul:060E2B34.0104.0101.04100200.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("DataValue", "Data represented as AAF varying array of UInt8", "UInt8Array", "urn:x-ul:060E2B34.0104.0101.04100100.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION("Opaque", "Opaque Data", "UInt8Array", "urn:x-ul:060E2B34.0104.0101.04100400.00000000", 0, false)
		MXFLIB_TYPE_INTERPRETATION_EX("LocalTagType", "Local Tag for 2-byte tagged SMPTE 336M set", "UInt16", "urn:x-ul:060E2B34.0104.0101.01012004.00000000", 0, false, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceTrack", "StrongReference to a Tracks", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05021400.00000000", 0, false, ClassRefUndefined, "GenericTrack", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceDictionary", "StrongReference to the Dictionary", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05020200.00000000", 0, false, ClassRefUndefined, "Dictionary")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceSourceReference", "StrongReference to a Source Reference", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05020800.00000000", 0, false, ClassRefUndefined, "SourceReference")
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceIdentification", "StrongReference to Identification Set", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05021000.00000000", 0, false, ClassRefUndefined, "Identification", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceParameter", "StrongReference to Parameter Set", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05021600.00000000", 0, false, ClassRefUndefined, "Parameter")
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceComponent", "StrongReference to Component Set", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05020b00.00000000", 0, false, ClassRefUndefined, "StructuralComponent", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceDMFramework", "StrongReference to DMFramework Set", "StrongRef", "", 0, false, ClassRefUndefined, "DM_Framework", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferencePackageMarkerObject", "StrongReference to PackageMarkerObject Set", "StrongRef", "", 0, false, ClassRefUndefined, "PackageMarkerObject")
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceEssenceData", "StrongReference to EssenceContainerData Set", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05020f00.00000000", 0, false, ClassRefUndefined, "EssenceContainerData", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferencePackage", "StrongReference to Package Set", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05021300.00000000", 0, false, ClassRefUndefined, "GenericPackage", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceContentStorage", "StrongReference to ContentStorage Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05020100.00000000", 0, false, ClassRefUndefined, "ContentStorage", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceNetworkLocator", "StrongReference to NetworkLocator Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05020400.00000000", 0, false, ClassRefUndefined, "NetworkLocator", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceLocator", "StrongReference to Locator Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05021200.00000000", 0, false, ClassRefUndefined, "Locator", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceSegment", "StrongReference to Segment Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05020600.00000000", 0, false, ClassRefUndefined, "Segment", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceKLVData", "StrongReference to KLVData Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05021c00.00000000", 0, false, ClassRefUndefined, "KLVData")
		MXFLIB_TYPE_INTERPRETATION_EX("StrongReferenceEssenceDescriptor", "StrongReference to EssenceDescriptor Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05020300.00000000", 0, false, ClassRefUndefined, "GenericDescriptor", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceDataDefinition", "StrongReference to DataDefinition Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05020e00.00000000", 0, false, ClassRefUndefined, "DataDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceKLVDataDefinition", "StrongReference to KLVDataDefinition Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05022000.00000000", 0, false, ClassRefUndefined, "KLVDataDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceCodecDefinition", "StrongReference to CodecDefinition Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05020a00.00000000", 0, false, ClassRefUndefined, "CodecDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferencePluginDefinition", "StrongReference to PluginDefinition Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05021800.00000000", 0, false, ClassRefUndefined, "PluginDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceParameterDefinition", "StrongReference to ParameterDefinition Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05021700.00000000", 0, false, ClassRefUndefined, "ParameterDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceInterpolationDefinition", "StrongReference to InterpolationDefinition Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05021100.00000000", 0, false, ClassRefUndefined, "InterpolationDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceTaggedValueDefinition", "StrongReference to TaggedValueDefinition Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05022100.00000000", 0, false, ClassRefUndefined, "TaggedValueDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("DictReferenceDataDefinition", "WeakReference to DataDefinition Set", "DictRef", "urn:x-ul:060E2B34.0104.0101.05010300.00000000", 0, false, ClassRefDict, "DataDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("DictReferenceContainerDefinition", "WeakReference to ContainerDefinition Set", "DictRef", "urn:x-ul:060e2b34.0104.0101.05010200.00000000", 0, false, ClassRefUndefined, "ContainerDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceContainerDefinition", "StrongReference to ContainerDefinition Set", "StrongRef", "urn:x-ul:060e2b34.0104.0101.05020c00.00000000", 0, false, ClassRefUndefined, "ContainerDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("DictReferenceCodecDefinition", "WeakReference to CodecDefinition Set", "DictRef", "urn:x-ul:060e2b34.0104.0101.05010b00.00000000", 0, false, ClassRefUndefined, "CodecDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("WeakReferenceParameterDefinition", "WeakReference to ParameterDefinition Set", "WeakRef", "urn:x-ul:060e2b34.0104.0101.05010800.00000000", 0, false, ClassRefUndefined, "ParameterDefinition")
		MXFLIB_TYPE_MULTIPLE_EX("StrongReferenceVectorTrack", "Vector of StrongReferences to Tracks", "StrongReferenceTrack", "urn:x-ul:060E2B34.0104.0101.05060500.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("StrongReferenceVectorIdentification", "Vector of StrongReferences to Identification Sets", "StrongReferenceIdentification", "urn:x-ul:060E2B34.0104.0101.05060300.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("StrongReferenceSetPackage", "Set of StrongReferences to Packages", "StrongReferencePackage", "urn:x-ul:060E2B34.0104.0101.05050700.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("StrongReferenceSetEssenceData", "Set of StrongReferences to EssenceContainerData sets", "StrongReferenceEssenceData", "urn:x-ul:060E2B34.0104.0101.05050500.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("StrongReferenceVectorComponent", "Vector of StrongReferences to StructuralComponent sets", "StrongReferenceComponent", "urn:x-ul:060E2B34.0104.0101.05060100.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_REF("StrongReferenceVectorSubDescriptor", "Vector of StrongReferences to SubDescriptor sets", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05060e00.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, "SubDescriptor")
		MXFLIB_TYPE_MULTIPLE_EX("StrongReferenceVectorLocator", "Vector of StrongReferences to Locator sets", "StrongReferenceLocator", "urn:x-ul:060E2B34.0104.0101.05060400.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("StrongReferenceVectorFileDescriptor", "Vector of StrongReferences to File Descriptor sets", "StrongRef", "urn:x-ul:060E2B34.0104.0101.05060B00.00000000", ARRAYEXPLICIT, 0, ClassRefUndefined, "FileDescriptor", NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceVectorParameter", "Vector of StrongReferences to ParameterDefinition sets", "StrongReferenceParameter", "urn:x-ul:060E2B34.0104.0101.05060a00.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetDataDefinition", "Set of StrongReferences to DataDefinition sets", "StrongReferenceDataDefinition", "urn:x-ul:060E2B34.0104.0101.05050400.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetKLVDataDefinition", "Set of StrongReferences to KLVDataDefinition sets", "StrongReferenceKLVDataDefinition", "urn:x-ul:060E2B34.0104.0101.05050d00.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetCodecDefinition", "Set of StrongReferences to CodecDefinition sets", "StrongReferenceCodecDefinition", "urn:x-ul:060E2B34.0104.0101.05050200.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetPluginDefinition", "Set of StrongReferences to PluginDefinition sets", "StrongReferencePluginDefinition", "urn:x-ul:060E2B34.0104.0101.05050a00.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetParameterDefinition", "Set of StrongReferences to ParameterDefinition sets", "StrongReferenceParameterDefinition", "urn:x-ul:060E2B34.0104.0101.05050900.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetInterpolationDefinition", "Set of StrongReferences to InterpolationDefinition sets", "StrongReferenceInterpolationDefinition", "urn:x-ul:060E2B34.0104.0101.05050600.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetContainerDefinition", "Set of StrongReferences to ContainerDefinition sets", "StrongReferenceContainerDefinition", "urn:x-ul:060E2B34.0104.0101.05050300.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetTaggedValueDefinition", "Set of StrongReferences to TaggedValueDefinition sets", "StrongReferenceTaggedValueDefinition", "urn:x-ul:060E2B34.0104.0101.05050e00.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceVectorSegment", "Vector of StrongReferences to Segment sets", "StrongReferenceSegment", "urn:x-ul:060e2b34.0104.0101.05060600.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("DictReferenceVectorDataDefinition", "Vector of DictReferences to DataDefinition sets", "DictReferenceDataDefinition", "urn:x-ul:060e2b34.0104.0101.05040300.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("WeakReferenceSetParameterDefinition", "Set of WeakReferences to ParameterDefinition sets", "WeakReferenceParameterDefinition", "urn:x-ul:060E2B34.0104.0101.05030e00.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_COMPOUND_EX("Rational", "Rational", "urn:x-ul:060E2B34.0104.0101.03010100.00000000", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("Numerator", "Numerator", "urn:x-ul:060E2B34.0104.0101.01010700.00000000", "urn:x-ul:060E2B34.0104.0101.03010101.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Denominator", "Denominator", "Int32", "urn:x-ul:060E2B34.0104.0101.03010102.00000000", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND_EX("DateStruct", "Date", "urn:x-ul:060E2B34.0104.0101.03010500.00000000", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("Year", "Year", "Int16", "urn:x-ul:060E2B34.0104.0101.03010501.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Month", "Month", "UInt8", "urn:x-ul:060E2B34.0104.0101.03010502.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Day", "Day", "UInt8", "urn:x-ul:060E2B34.0104.0101.03010503.00000000", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND_EX("TimeStruct", "Date and Time", "urn:x-ul:060E2B34.0104.0101.03010600.00000000", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("Hours", "Hours", "UInt8", "urn:x-ul:060E2B34.0104.0101.03010601.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Minutes", "Minutes", "UInt8", "urn:x-ul:060E2B34.0104.0101.03010602.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Seconds", "Seconds", "UInt8", "urn:x-ul:060E2B34.0104.0101.03010603.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("msBy4", "msBy4", "UInt8", "urn:x-ul:060E2B34.0104.0101.03010604.00000000", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND_EX("Timestamp", "Date and Time", "urn:x-ul:060E2B34.0104.0101.03010700.00000000", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("Date", "", "DateStruct", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Time", "", "TimeStruct", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND_EX("ProductVersionType", "current~ProductVersion Product Version Number", "urn:x-ul:060E2B34.0104.0101.03010200.00000000", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("Major", "Major", "UInt16", "urn:x-ul:060E2B34.0104.0101.03010201.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Minor", "Minor", "UInt16", "urn:x-ul:060E2B34.0104.0101.03010202.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Patch", "Patch", "UInt16", "urn:x-ul:060E2B34.0104.0101.03010203.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Build", "Build", "UInt16", "urn:x-ul:060E2B34.0104.0101.03010204.00000000", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Release", "Release", "ProductReleaseKind", "urn:x-ul:060E2B34.0104.0101.03010205.00000000", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND("Indirect", "Indirect Data", "urn:x-ul:060E2B34.0104.0101.04100300.00000000")
			MXFLIB_TYPE_COMPOUND_ITEM("Order", "Object Byte Order", "UInt8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Type", "Type of Data", "UUID", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Data", "Data", "UTF16String", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND_EX("RGBALayoutItem", "Item in an RGBALayout array", "urn:x-ul:060E2B34.0104.0101.03010400.00000000", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("Code", "Enumerated value specifying component", "RGBACode", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Depth", "Number of bits occupied", "UInt8", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND_EX("LocalTagEntry", "Mapping of Local Tag to UL or UUID", "", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("LocalTag", "The value of the Local Tag", "LocalTagType", "06 0e 2b 34 01 01 01 05  01 03 06 02 00 00 00 00", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("GlobalAUID", "The AUID of which the local tag is an alias", "AUID", "06 0e 2b 34 01 01 01 05  01 03 06 03 00 00 00 00", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND_EX("DeltaEntryType", "Map Elements onto Slices", "", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("PosTableIndex", "Index into PosTable (or Apply Temporta Reordering if -1)", "Int8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Slice", "Slice number in IndexEntry", "UInt8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("ElementDelta", "Delta from start of slice to this Element", "UInt32", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND_EX("IndexEntryType", "Index from Edit Unit number to stream offset", "", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("TemporalOffset", "Offset in edit units from Display Order to Coded Order", "Int8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("AnchorOffset", "Offset in edit units to previous Anchor Frame. The value is zero if this is an anchor frame.", "Int8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Flags", "Flags for this Edit Unit", "UInt8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("StreamOffset", "Offset in bytes from the first KLV element in this Edit Unit within the Essence Container", "UInt64", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("SliceOffsetArray", "Array of offsets in bytes from the Stream Offset to the start of each slice.", "UInt32Array", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("PosTableArray", "Array of fractional position offsets from the start of the content package to the synchronized sample in the Content Package", "RationalArray", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND_EX("PartitionInfo", "Partition Start Position Info", "", NULL, TypeFlags_Baseline)
			MXFLIB_TYPE_COMPOUND_ITEM("BodySID", "Stream ID of the Body in this partition", "UInt32", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("ByteOffset", "Byte offset from file start (1st byte of the file which is numbered 0) to the 1st byte of the Partition Pack Key", "UInt64", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_MULTIPLE_EX("RationalArray", "Array of Rational", "Rational", "urn:x-ul:060E2B34.0104.0101.04020200.00000000", ARRAYIMPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("RGBALayout", "Specifies the type, order and size of the components within the pixel", "RGBALayoutItem", "urn:x-ul:060E2B34.0104.0101.04020100.00000000", ARRAYIMPLICIT, 8, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("LocalTagEntryBatch", "Batch of Local Tag mappings", "LocalTagEntry", "", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("DeltaEntryArray", "Array of DeltaEntryTypes", "DeltaEntryType", "", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("IndexEntryArray", "Array of IndexEntryTypes", "IndexEntryType", "", ARRAYEXPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
		MXFLIB_TYPE_MULTIPLE_EX("PartitionArray", "Array of Partition Start Positions", "PartitionInfo", "", ARRAYIMPLICIT, 0, ClassRefUndefined, NULL, NULL, TypeFlags_Baseline)
	MXFLIB_TYPE_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes)
		MXFLIB_CLASS_ITEM("KLVFill", "KLV Filler packet", ClassUsageOptional, "RAW", 0, 0, 0x0000, "06 0E 2B 34 01 01 01 02 03 01 02 10 01 00 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK_EX("PartitionMetadata", "Identifies a Partition Pack", "", "urn:x-ul:060E2B34.0206.0101.0D010200.00000000", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
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
			MXFLIB_CLASS_ITEM("OperationalPattern", "Universal Label of the Operational Pattern to which this file complies", ClassUsageRequired, "AUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 05  01 02 02 03 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EssenceContainers", "The unordered batch of Universal Labels of Essence Containers used in or referenced by this file", ClassUsageRequired, "AUIDSet", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  01 02 02 10 02 01 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("OpenHeader", "Open Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 01 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("OpenCompleteHeader", "Open Complete Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 03 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("ClosedHeader", "Closed Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 02 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("ClosedCompleteHeader", "Closed Complete Header Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 02 04 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("OpenBodyPartition", "Open Body Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 03 01 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("OpenCompleteBodyPartition", "Open Complete Body Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 03 03 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("ClosedBodyPartition", "Closed Body Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 03 02 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("ClosedCompleteBodyPartition", "Closed Complete Body Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 03 04 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("Footer", "Footer Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 04 02 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("CompleteFooter", "Complete Footer Partition Pack", "PartitionMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 04 04 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK_EX("Primer", "Primer Pack", "", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 05 01 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("LocalTagEntries", "Local Tag Entry Batch", ClassUsageRequired, "LocalTagEntryBatch", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  06 01 01 07 15 00 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_SET_EX("AbstractObject", "", "", 2, 2, "urn:x-ul:060E2B34.0253.0101.0D010101.01017f00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM_REF("InstanceUID", "Unique ID of this instance", ClassUsageRequired, "UUID", 16, 16, 0x3c0a, "06 0e 2b 34 01 01 01 01  01 01 15 02 00 00 00 00", ClassRefTarget, "", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("InterchangeObject", "", "", 2, 2, "urn:x-ul:060E2B34.0253.0101.0D010101.01010100", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM_REF("InstanceUID", "Unique ID of this instance", ClassUsageRequired, "UUID", 16, 16, 0x3c0a, "06 0e 2b 34 01 01 01 01  01 01 15 02 00 00 00 00", ClassRefTarget, "", NULL, NULL)
			MXFLIB_CLASS_ITEM("ObjectClass", "Specifies a reference to the definition of a class of object", ClassUsageDecoderRequired, "AUID", 0, 0, 0x0101, "urn:x-ul:060E2B34.0101.0102.06010104.01010000", NULL, NULL)
			MXFLIB_CLASS_ITEM("GenerationUID", "Generation Instance", ClassUsageOptional, "UUID", 16, 16, 0x0102, "06 0e 2b 34 01 01 01 02  05 20 07 01 08 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("IndexTableSegment", "A segment of an Index Table", "AbstractObject", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 02 01 01 10 01 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("IndexEditRate", "Edit Rate copied from the tracks of the Essence Container", ClassUsageRequired, "Rational", 8, 8, 0x3f0b, "06 0e 2b 34 01 01 01 05  05 30 04 06 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexStartPosition", "The first editable unit indexed by this Index Table segment measured in File Package Edit Units", ClassUsageRequired, "Position", 8, 8, 0x3f0c, "06 0e 2b 34 01 01 01 05  07 02 01 03 01 0a 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexDuration", "Time duration of this table segment measured in Edit Unitsof the referenceg package", ClassUsageRequired, "LengthType", 8, 8, 0x3f0d, "06 0e 2b 34 01 01 01 05  07 02 02 01 01 02 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EditUnitByteCount", "Byte count of each and every Edit Unit. A value of 0 defines the byte count of Edit Units is only given in the Index Entry Array", ClassUsageDecoderRequired, "UInt32", 4, 4, 0x3f05, "06 0e 2b 34 01 01 01 04  04 06 02 01 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("IndexSID", "Stream Identifier (SID) of Index Stream", ClassUsageDecoderRequired, "UInt32", 4, 4, 0x3f06, "06 0e 2b 34 01 01 01 04  01 03 04 05 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BodySID", "Stream Identifier (SID) of Essence Container Stream", ClassUsageRequired, "UInt32", 4, 4, 0x3f07, "06 0e 2b 34 01 01 01 04  01 03 04 04 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SliceCount", "Number of slices minus 1 (NSL)", ClassUsageDecoderRequired, "UInt8", 1, 1, 0x3f08, "06 0e 2b 34 01 01 01 04  04 04 04 01 01 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("PosTableCount", "Number of PosTable Entries minus 1 (NPE)", ClassUsageOptional, "UInt8", 1, 1, 0x3f0e, "06 0e 2b 34 01 01 01 05  04 04 04 01 07 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("DeltaEntryArray", "Map Elements onto Slices", ClassUsageOptional, "DeltaEntryArray", 0, 0, 0x3f09, "06 0e 2b 34 01 01 01 05  04 04 04 01 06 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexEntryArray", "Index from Edit Unit number to stream offset", ClassUsageDecoderRequired, "IndexEntryArray", 0, 0, 0x3f0a, "06 0e 2b 34 01 01 01 05  04 04 04 02 05 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_FIXEDPACK_EX("RandomIndexMetadata", "Random Index Pack", "", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 11 01 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("PartitionArray", "Array of Partition Start Positions", ClassUsageRequired, "PartitionArray", 0, 0, 0x0000, "80 62 c1 08 a8 0d eb fe 3a 9d c8 e1 7e 83 b6 4b", NULL, NULL)
			MXFLIB_CLASS_ITEM("Length", "Overall Length of this Pack including the Set Key and BER Length fields", ClassUsageRequired, "UInt32", 4, 4, 0x0000, "06 0e 2b 34 01 01 01 04  04 06 10 01 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_FIXEDPACK("RandomIndexMetadataV10", "Random Index Pack (v10)", "RandomIndexMetadata", "06 0e 2b 34 02 05 01 01  0d 01 02 01 01 11 00 00")
		MXFLIB_CLASS_FIXEDPACK_END
		MXFLIB_CLASS_SET_EX("Preface", "Preface Set", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 2f 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("LastModifiedDate", "Date & time of the last modification of the file", ClassUsageRequired, "Timestamp", 8, 8, 0x3b02, "06 0e 2b 34 01 01 01 02  07 02 01 10 02 04 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Version", "The value shall be 258 (i.e. v1.2)", ClassUsageRequired, "VersionType", 2, 2, 0x3b05, "06 0e 2b 34 01 01 01 02  03 01 02 01 05 00 00 00", "258", NULL)
			MXFLIB_CLASS_ITEM("ObjectModelVersion", "Simple integer version number of Object Model", ClassUsageOptional, "UInt32", 4, 4, 0x3b07, "06 0e 2b 34 01 01 01 02  03 01 02 01 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM_REF("PrimaryPackage", "The primary Package in this file", ClassUsageOptional, "UUID", 16, 16, 0x3b08, "06 0e 2b 34 01 01 01 04  06 01 01 04 01 08 00 00", ClassRefWeak, "GenericPackage", NULL, NULL)
			MXFLIB_CLASS_ITEM("Identifications", "Ordered array of strong references to Identification sets recording all modifications to the file", ClassUsageEncoderRequired, "StrongReferenceVectorIdentification", 0, 0, 0x3b06, "06 0e 2b 34 01 01 01 02  06 01 01 04 06 04 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ContentStorageObject", "current~ContentStorageSet Strong reference to Content Storage object", ClassUsageRequired, "StrongReferenceContentStorage", 16, 16, 0x3b03, "06 0e 2b 34 01 01 01 02  06 01 01 04 02 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("OperationalPattern", "Universal Label of the Operational Pattern which this file complies to (repeat of Partition Pack value)", ClassUsageRequired, "AUID", 16, 16, 0x3b09, "06 0e 2b 34 01 01 01 05  01 02 02 03 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EssenceContainers", "Unordered batch of ULs of Essence Containers used in or referenced by this file (repeat of Partition Pack value)", ClassUsageRequired, "AUIDSet", 0, 0, 0x3b0a, "06 0e 2b 34 01 01 01 05  01 02 02 10 02 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DMSchemes", "An unordered batch of Universal Labels of all the Descriptive Metadata schemes used in this file", ClassUsageRequired, "AUIDSet", 0, 0, 0x3b0b, "06 0e 2b 34 01 01 01 05  01 02 02 10 02 02 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("Identification", "Identification set", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 30 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("ThisGenerationUID", "This Generation Identifier", ClassUsageRequired, "UUID", 16, 16, 0x3c09, "06 0e 2b 34 01 01 01 02  05 20 07 01 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("CompanyName", "Manufacturer of the equipment or application that created or modified the file", ClassUsageRequired, "UTF16String", 0, 0, 0x3c01, "06 0e 2b 34 01 01 01 02  05 20 07 01 02 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ProductName", "Name of the application which created or modified this file", ClassUsageRequired, "UTF16String", 0, 0, 0x3c02, "06 0e 2b 34 01 01 01 02  05 20 07 01 03 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ProductVersion", "Maj.min.tweak.build.rel  version number of this application", ClassUsageOptional, "ProductVersionType", 10, 10, 0x3c03, "06 0e 2b 34 01 01 01 02  05 20 07 01 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("VersionString", "Human readable name of the application version", ClassUsageRequired, "UTF16String", 0, 0, 0x3c04, "06 0e 2b 34 01 01 01 02  05 20 07 01 05 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ProductUID", "A unique identification for the product which created this file (defined by the manufacturer)", ClassUsageRequired, "AUID", 16, 16, 0x3c05, "06 0e 2b 34 01 01 01 02  05 20 07 01 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ModificationDate", "Time & date an application created or modified this file and created this Identification set", ClassUsageRequired, "Timestamp", 8, 8, 0x3c06, "06 0e 2b 34 01 01 01 02  07 02 01 10 02 03 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ToolkitVersion", "Maj.min.tweak.build.rel version of software or hardware codec used", ClassUsageOptional, "ProductVersionType", 10, 10, 0x3c07, "06 0e 2b 34 01 01 01 02  05 20 07 01 0a 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Platform", "Human readable name of the operating system used.", ClassUsageOptional, "UTF16String", 0, 0, 0x3c08, "06 0e 2b 34 01 01 01 02  05 20 07 01 06 01 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("ContentStorage", "Content Storage set", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 18 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("Packages", "Unordered batch of all packages used in this file", ClassUsageRequired, "StrongReferenceSetPackage", 0, 0, 0x1901, "06 0e 2b 34 01 01 01 02  06 01 01 04 05 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EssenceDataObjects", "current~EssenceContainerData Unordered batch of strong references to Essence Container Data sets used in this file", ClassUsageOptional, "StrongReferenceSetEssenceData", 0, 0, 0x1902, "06 0e 2b 34 01 01 01 02  06 01 01 04 05 02 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("EssenceContainerData", "Essence Container Data set", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 23 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("LinkedPackageUID", "Identifier of the Package to which this set is linked as a UMID", ClassUsageRequired, "PackageID", 32, 32, 0x2701, "06 0e 2b 34 01 01 01 02  06 01 01 06 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IndexSID", "ID of the Index Table for the Essence Container to which this set is linked", ClassUsageOptional, "UInt32", 4, 4, 0x3f06, "06 0e 2b 34 01 01 01 04  01 03 04 05 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BodySID", "ID of the Essence Container to which this set is linked", ClassUsageRequired, "UInt32", 4, 4, 0x3f07, "06 0e 2b 34 01 01 01 04  01 03 04 04 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("GenericPackage", "Defines a Generic Package set", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 34 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("PackageUID", "Unique Package Identifier as a UMID", ClassUsageRequired, "PackageID", 32, 32, 0x4401, "06 0e 2b 34 01 01 01 01  01 01 15 10 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PackageName", "current~Name Human readable package name", ClassUsageOptional, "UTF16String", 0, 0, 0x4402, "06 0e 2b 34 01 01 01 01  01 03 03 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PackageCreationDate", "The date & time of creation of this package", ClassUsageRequired, "Timestamp", 8, 8, 0x4405, "06 0e 2b 34 01 01 01 02  07 02 01 10 01 03 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PackageModifiedDate", "The date & time of last modification of this package", ClassUsageRequired, "Timestamp", 8, 8, 0x4404, "06 0e 2b 34 01 01 01 02  07 02 01 10 02 05 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Tracks", "Array of Unique IDs of Tracks", ClassUsageRequired, "StrongReferenceVectorTrack", 0, 0, 0x4403, "06 0e 2b 34 01 01 01 02  06 01 01 04 06 05 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("Locator", "", "InterchangeObject", "urn:x-ul:060e2b34.0253.0101.0d010101.01013100")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("NetworkLocator", "Network Locator set for location with a URL", "Locator", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 32 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("URLString", "A URL indicating where the essence may be found.", ClassUsageRequired, "UTF16String", 0, 0, 0x4001, "06 0e 2b 34 01 01 01 01  01 02 01 01 01 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("TextLocator", "Text Locator set for location with a human-readable text string", "Locator", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 33 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("LocatorName", "Value of a human-readable locator text string for manual location of essence", ClassUsageRequired, "UTF16String", 0, 0, 0x4101, "06 0e 2b 34 01 01 01 02  01 04 01 02 01 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("GenericTrack", "Generic Track", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 38 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("TrackID", "ID of the track in this package (for linking to a SourceTrackID in a segment)", ClassUsageRequired, "UInt32", 4, 4, 0x4801, "06 0e 2b 34 01 01 01 02  01 07 01 01 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TrackNumber", "Number used to link to the track in the Essence Container if it exists", ClassUsageRequired, "UInt32", 4, 4, 0x4804, "06 0e 2b 34 01 01 01 02  01 04 01 03 00 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("TrackName", "Human readable name of the track type", ClassUsageOptional, "UTF16String", 0, 0, 0x4802, "06 0e 2b 34 01 01 01 02  01 07 01 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TrackSegment", "current~Sequence Strong Reference to Sequence Set", ClassUsageRequired, "StrongReferenceSegment", 16, 16, 0x4803, "06 0e 2b 34 01 01 01 02  06 01 01 04 02 04 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("StaticTrack", "", "GenericTrack", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 3a 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("Track", "Track", "GenericTrack", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 3b 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("EditRate", "Edit Rate of Track", ClassUsageRequired, "Rational", 8, 8, 0x4b01, "06 0e 2b 34 01 01 01 02  05 30 04 05 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Origin", "An Offset used to resolved timeline references to this track. The start of the track has this timeline value measured in Edit Units.", ClassUsageRequired, "Position", 8, 8, 0x4b02, "06 0e 2b 34 01 01 01 02  07 02 01 03 01 03 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("EventTrack", "Event Track", "GenericTrack", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 39 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("EventEditRate", "Edit Rate of Event Track", ClassUsageRequired, "Rational", 8, 8, 0x4901, "06 0e 2b 34 01 01 01 02  05 30 04 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EventOrigin", "An Offset used to resolved timeline references to this event track. The start of the event track has this timeline value measured in Edit Units.", ClassUsageOptional, "Position", 8, 8, 0x4902, "06 0e 2b 34 01 01 01 05  07 02 01 03 01 0b 00 00", "0", NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("StructuralComponent", "Structural Component Superclass", "InterchangeObject", 2, 2, "urn:x-ul:060E2B34.0253.0101.0D010101.01010200", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("ComponentDataDefinition", "current~DataDefinition The kind of data or metadata this structure refers to", ClassUsageRequired, "DictReferenceDataDefinition", 0, 0, 0x0201, "06 0e 2b 34 01 01 01 02  04 07 01 00 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ComponentLength", "Duration (in units of edit rate)", ClassUsageBestEffort, "LengthType", 8, 8, 0x0202, "06 0e 2b 34 01 01 01 02  07 02 02 01 01 03 00 00", NULL, "-1")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("Segment", "", "StructuralComponent", 2, 2, "06 0E 2B 34 02 53 01 01 0d 01 01 01 01 01 03 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("Sequence", "Sequence of Segments", "Segment", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 0f 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("StructuralComponents", "Ordered array of strong references to Structural Components", ClassUsageRequired, "StrongReferenceVectorComponent", 0, 0, 0x1001, "06 0e 2b 34 01 01 01 02  06 01 01 04 06 09 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("Filler", "[060E2B34.0253.0101.0D010101.01010900]", "Segment", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 09 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("TimecodeComponent", "Timecode Component", "StructuralComponent", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 14 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("RoundedTimecodeBase", "Integer frames per second", ClassUsageRequired, "UInt16", 2, 2, 0x1502, "06 0e 2b 34 01 01 01 02  04 04 01 01 02 06 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("StartTimecode", "Starting timecode", ClassUsageRequired, "Position", 8, 8, 0x1501, "06 0e 2b 34 01 01 01 02  07 02 01 03 01 05 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DropFrame", "Drop frame flag", ClassUsageRequired, "Boolean", 1, 1, 0x1503, "06 0e 2b 34 01 01 01 01  04 04 01 01 05 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("SourceReference", "", "Segment", 2, 2, "06 0E 2B 34 02 53 01 01 0D 01 01 01 01 01 10 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("SourcePackageID", "ID of referenced Package as a UMID", ClassUsageRequired, "PackageID", 32, 32, 0x1101, "06 0e 2b 34 01 01 01 02  06 01 01 03 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SourceTrackID", "Track ID of the referenced Track within the referenced Package", ClassUsageRequired, "UInt32", 4, 4, 0x1102, "06 0e 2b 34 01 01 01 02  06 01 01 03 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ChannelIDs", "Array of channels from a multichannel source", ClassUsageOptional, "UInt32Array", 0, 0, 0x1103, "06 0E 2B 34 01 01 01 07 06 01 01 03 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("MonoSourceTrackIDs", "Array of mono source tracks tco compose a multichannel clip", ClassUsageOptional, "UInt32Array", 0, 0, 0x1104, "06 0e 2B 34 01 01 01 08 06 01 01 03 08 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("SourceClip", "Source Clip", "SourceReference", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 11 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("StartPosition", "Offset into Essence measured in edit units of the track containing this segment", ClassUsageRequired, "Position", 8, 8, 0x1201, "06 0e 2b 34 01 01 01 02  07 02 01 03 01 04 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("MaterialPackage", "Material Package set", "GenericPackage", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 36 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("SourcePackage", "File Package set", "GenericPackage", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 37 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("Descriptor", "Strong Reference to the Descriptor", ClassUsageDecoderRequired, "StrongReferenceEssenceDescriptor", 16, 16, 0x4701, "06 0e 2b 34 01 01 01 02  06 01 01 04 02 03 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("SubDescriptor", "Generic Sub-Descriptor", "InterchangeObject", 2, 2, "", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("GenericDescriptor", "Generic Descriptor", "InterchangeObject", 2, 2, "urn:x-ul:060e2b34.0253.0101.0d010101.01012400", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("Locators", "Ordered array of strong references to Locator sets", ClassUsageOptional, "StrongReferenceVectorLocator", 0, 0, 0x2f01, "06 0e 2b 34 01 01 01 02  06 01 01 04 06 03 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SubDescriptors", "Ordered array of strong references to sub descriptor sets", ClassUsageOptional, "StrongReferenceVectorSubDescriptor", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 09  06 01 01 04 06 10 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("FileDescriptor", "File Descriptor", "GenericDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 25 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("LinkedTrackID", "Link to (i.e. value of) the Track ID of the Track in this Package to which the Descriptor applies", ClassUsageOptional, "UInt32", 0, 0, 0x3006, "06 0e 2b 34 01 01 01 05  06 01 01 03 05 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SampleRate", "The field or frame rate of the Essence Container (not the essence sampling clock rate)", ClassUsageRequired, "Rational", 8, 8, 0x3001, "06 0e 2b 34 01 01 01 01  04 06 01 01 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ContainerDuration", "Duration of Essence Container (measured in Edit Units)", ClassUsageOptional, "LengthType", 8, 8, 0x3002, "06 0e 2b 34 01 01 01 01  04 06 01 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EssenceContainer", "The UL identifying the Essence Container described by this Descriptor", ClassUsageDecoderRequired, "DictReferenceContainerDefinition", 0, 0, 0x3004, "06 0e 2b 34 01 01 01 02  06 01 01 04 01 02 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Codec", "UL to identify a codec compatible with this Essence Container", ClassUsageOptional, "DictReferenceCodecDefinition", 0, 0, 0x3005, "06 0e 2b 34 01 01 01 02  06 01 01 04 01 03 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("GenericPictureEssenceDescriptor", "Defines the Picture Essence Descriptor set", "FileDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 27 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("SignalStandard", "Underlying signal standard", ClassUsageOptional, "UInt8", 1, 1, 0x3215, "06 0e 2b 34 01 01 01 05  04 05 01 13 00 00 00 00", "1", NULL)
			MXFLIB_CLASS_ITEM("FrameLayout", "Interlace or Progressive layout", ClassUsageBestEffort, "LayoutType", 1, 1, 0x320c, "06 0e 2b 34 01 01 01 01  04 01 03 01 04 00 00 00", NULL, "255")
			MXFLIB_CLASS_ITEM("StoredWidth", "Horizontal Size of active picture", ClassUsageBestEffort, "UInt32", 4, 4, 0x3203, "06 0e 2b 34 01 01 01 01  04 01 05 02 02 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("StoredHeight", "Vertical Field Size of active picture", ClassUsageBestEffort, "UInt32", 4, 4, 0x3202, "06 0e 2b 34 01 01 01 01  04 01 05 02 01 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("StoredF2Offset", "Topness Adjustment for stored picture", ClassUsageOptional, "Int32", 4, 4, 0x3216, "06 0e 2b 34 01 01 01 05  04 01 03 02 08 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("SampledWidth", "Sampled width supplied to codec", ClassUsageOptional, "UInt32", 4, 4, 0x3205, "06 0e 2b 34 01 01 01 01  04 01 05 01 08 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SampledHeight", "Sampled height supplied to codec", ClassUsageOptional, "UInt32", 4, 4, 0x3204, "06 0e 2b 34 01 01 01 01  04 01 05 01 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SampledXOffset", "Offset from sampled to stored width", ClassUsageOptional, "Int32", 4, 4, 0x3206, "06 0e 2b 34 01 01 01 01  04 01 05 01 09 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("SampledYOffset", "Offset from sampled to stored", ClassUsageOptional, "Int32", 4, 4, 0x3207, "06 0e 2b 34 01 01 01 01  04 01 05 01 0A 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("DisplayHeight", "Displayed Height placed in Production Aperture", ClassUsageOptional, "UInt32", 4, 4, 0x3208, "06 0e 2b 34 01 01 01 01  04 01 05 01 0B 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DisplayWidth", "Displayed Width placed in Production Aperture", ClassUsageOptional, "UInt32", 4, 4, 0x3209, "06 0e 2b 34 01 01 01 01  04 01 05 01 0C 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DisplayXOffset", "The horizontal offset from the (in pixels) of the picture as displayed", ClassUsageOptional, "Int32", 4, 4, 0x320a, "06 0e 2b 34 01 01 01 01  04 01 05 01 0D 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DisplayYOffset", "The vertical offset (in pixels) of the picture as displayed", ClassUsageOptional, "Int32", 4, 4, 0x320b, "06 0e 2b 34 01 01 01 01  04 01 05 01 0E 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DisplayF2Offset", "Topness Adjustment for Displayed Picture", ClassUsageOptional, "Int32", 4, 4, 0x3217, "06 0e 2b 34 01 01 01 05  04 01 03 02 07 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("AspectRatio", "Specifies the horizontal to vertical aspect ratio of the whole image as it is to be presented to avoid geometric distortion (and hence including any black edges)", ClassUsageBestEffort, "Rational", 8, 8, 0x320e, "06 0e 2b 34 01 01 01 01  04 01 01 01 01 00 00 00", NULL, "0/0")
			MXFLIB_CLASS_ITEM("ActiveFormatDescriptor", "Specifies the intended framing of the content within the displayed image", ClassUsageOptional, "UInt8", 1, 1, 0x3218, "06 0e 2b 34 01 01 01 05  04 01 03 02 09 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("VideoLineMap", "First active line in each field", ClassUsageBestEffort, "Int32Batch", 0, 0, 0x320d, "06 0e 2b 34 01 01 01 02  04 01 03 02 05 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("AlphaTransparency", "Is Alpha Inverted", ClassUsageOptional, "UInt8", 1, 1, 0x320f, "06 0e 2b 34 01 01 01 02  05 20 01 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TransferCharacteristic", "Specifies the opto-electric transfer characteristic ", ClassUsageOptional, "UL", 16, 16, 0x3210, "06 0e 2b 34 01 01 01 02  04 01 02 01 01 01 02 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ImageAlignmentOffset", "Byte Boundary alignment required for Low Level Essence Storage", ClassUsageOptional, "UInt32", 4, 4, 0x3211, "06 0e 2b 34 01 01 01 02  04 18 01 01 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ImageStartOffset", "Unused bytes before start of stored data", ClassUsageOptional, "UInt32", 4, 4, 0x3213, "06 0e 2b 34 01 01 01 02  04 18 01 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ImageEndOffset", "Unused bytes before start of stored data", ClassUsageOptional, "UInt32", 4, 4, 0x3214, "06 0e 2b 34 01 01 01 02  04 18 01 03 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("FieldDominance", "The number of the field which is considered temporally to come first", ClassUsageOptional, "UInt8", 1, 1, 0x3212, "06 0e 2b 34 01 01 01 02  04 01 03 01 06 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PictureEssenceCoding", "UL identifying the Picture Compression Scheme", ClassUsageDecoderRequired, "AUID", 0, 0, 0x3201, "06 0e 2b 34 01 01 01 02  04 01 06 01 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("CodingEquations", "UL identifying the fundamental color coding equations", ClassUsageOptional, "UL", 16, 16, 0x321a, "06 0e 2b 34 01 01 01 02  04 01 02 01 01 03 01 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ColorPrimaries", "UL identifying the color primaries", ClassUsageOptional, "UL", 16, 16, 0x3219, "06 0e 2b 34 01 01 01 02  04 01 02 01 01 06 01 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("CDCIEssenceDescriptor", "Defines the CDCI Picture Essence Descriptor set", "GenericPictureEssenceDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 28 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("ComponentDepth", "Number of active bits per sample (e.g. 8, 10, 16)", ClassUsageBestEffort, "UInt32", 4, 4, 0x3301, "06 0e 2b 34 01 01 01 02  04 01 05 03 0A 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("HorizontalSubsampling", "Specifies the H colour subsampling", ClassUsageBestEffort, "UInt32", 4, 4, 0x3302, "06 0e 2b 34 01 01 01 01  04 01 05 01 05 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("VerticalSubsampling", "Specifies the V colour subsampling", ClassUsageOptional, "UInt32", 4, 4, 0x3308, "06 0e 2b 34 01 01 01 02  04 01 05 01 10 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ColorSiting", "Enumerated value describing the color siting", ClassUsageOptional, "UInt8", 1, 1, 0x3303, "06 0e 2b 34 01 01 01 01  04 01 05 01 06 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ReversedByteOrder", "a FALSE value denotes Chroma followed by Luma pixels according to ITU Rec. 601", ClassUsageOptional, "Boolean", 1, 1, 0x330b, "06 0e 2b 34 01 01 01 05  03 01 02 01 0a 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PaddingBits", "Bits to round up each pixel to stored size", ClassUsageOptional, "UInt16", 2, 2, 0x3307, "06 0e 2b 34 01 01 01 02  04 18 01 04 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("AlphaSampleDepth", "Number of bits per alpha sample", ClassUsageOptional, "UInt32", 4, 4, 0x3309, "06 0e 2b 34 01 01 01 02  04 01 05 03 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BlackRefLevel", "Black refernece level e.g. 16 or 64 (8 or 10-bits)", ClassUsageOptional, "UInt32", 4, 4, 0x3304, "06 0e 2b 34 01 01 01 01  04 01 05 03 03 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("WhiteReflevel", "White reference level e.g. 235 or 943 (8 or 10 bits)", ClassUsageOptional, "UInt32", 4, 4, 0x3305, "06 0e 2b 34 01 01 01 01  04 01 05 03 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ColorRange", "Color range e.g. 225 or 897 (8 or 10 bits)", ClassUsageOptional, "UInt32", 4, 4, 0x3306, "06 0e 2b 34 01 01 01 02  04 01 05 03 05 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("RGBAEssenceDescriptor", "Defines the RGBA Picture Essence Descriptor set", "GenericPictureEssenceDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 29 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("ComponentMaxRef", "Maximum value for RGB components, e.g. 235 or 940 (8 or 10 bits)", ClassUsageOptional, "UInt32", 4, 4, 0x3406, "06 0e 2b 34 01 01 01 05  04 01 05 03 0b 00 00 00", "255", NULL)
			MXFLIB_CLASS_ITEM("ComponentMinRef", "Minimum value for RGB components, e.g. 16 or 64 (8 or 10 bits)", ClassUsageOptional, "UInt32", 4, 4, 0x3407, "06 0e 2b 34 01 01 01 05  04 01 05 03 0c 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("AlphaMaxRef", "Maximum value for alpha component, e.g. 235 or 940 (8 or 10 bits)", ClassUsageOptional, "UInt32", 4, 4, 0x3408, "06 0e 2b 34 01 01 01 05  04 01 05 03 0d 00 00 00", "255", NULL)
			MXFLIB_CLASS_ITEM("AlphaMinRef", "Minimum value for alpha component, e.g. 16 or 64 (8 or 10 bits)", ClassUsageOptional, "UInt32", 4, 4, 0x3409, "06 0e 2b 34 01 01 01 05  04 01 05 03 0e 00 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("ScanningDirection", "Enumerated Scanning Direction", ClassUsageOptional, "UInt8", 1, 1, 0x3405, "06 0e 2b 34 01 01 01 05  04 01 04 04 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PixelLayout", "Pixel Layout", ClassUsageBestEffort, "RGBALayout", 0, 0, 0x3401, "06 0e 2b 34 01 01 01 02  04 01 05 03 06 00 00 00", NULL, "{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}")
			MXFLIB_CLASS_ITEM("Palette", "Palette", ClassUsageOptional, "DataValue", 0, 0, 0x3403, "06 0e 2b 34 01 01 01 02  04 01 05 03 08 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PaletteLayout", "Palette Layout", ClassUsageOptional, "RGBALayout", 16, 16, 0x3404, "06 0e 2b 34 01 01 01 02  04 01 05 03 09 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("GenericSoundEssenceDescriptor", "Defines the Sound Essence Descriptor set", "FileDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 42 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("AudioSamplingRate", "Sampling rate of the audio essence", ClassUsageBestEffort, "Rational", 8, 8, 0x3d03, "06 0e 2b 34 01 01 01 05  04 02 03 01 01 01 00 00", "48000/1", "0/0")
			MXFLIB_CLASS_ITEM("Locked", "Boolean indicating that the Number of samples per frame is locked or unlocked (non-0 = locked)", ClassUsageDecoderRequired, "Boolean", 1, 1, 0x3d02, "06 0e 2b 34 01 01 01 04  04 02 03 01 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("AudioRefLevel", "Audio reference level which gives the number of dBm for 0VU", ClassUsageOptional, "Int8", 1, 1, 0x3d04, "06 0e 2b 34 01 01 01 01  04 02 01 01 03 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ElectroSpatialFormulation", "E.g. mono, dual mono, stereo, A,B etc (enum)", ClassUsageOptional, "UInt8", 1, 1, 0x3d05, "06 0e 2b 34 01 01 01 01  04 02 01 01 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ChannelCount", "Number of Sound Channels", ClassUsageBestEffort, "UInt32", 4, 4, 0x3d07, "06 0e 2b 34 01 01 01 05  04 02 01 01 04 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("QuantizationBits", "Number of quantization bits", ClassUsageBestEffort, "UInt32", 4, 4, 0x3d01, "06 0e 2b 34 01 01 01 04  04 02 03 03 04 00 00 00", NULL, "0")
			MXFLIB_CLASS_ITEM("DialNorm", "Gain to be applied to normalise perceived loudness of the clip, defined by ATSC A/53 (1dB per step)", ClassUsageOptional, "Int8", 1, 1, 0x3d0c, "06 0e 2b 34 01 01 01 05  04 02 07 01 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SoundEssenceCompression", "UL identifying the Sound Compression Scheme", ClassUsageDecoderRequired, "AUID", 0, 0, 0x3d06, "06 0e 2b 34 01 01 01 02  04 02 04 02 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("GenericDataEssenceDescriptor", "Defines the Data Essence Descriptor set", "FileDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 43 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("DataEssenceCoding", "Specifies the data essence coding type", ClassUsageDecoderRequired, "AUID", 0, 0, 0x3e01, "06 0e 2b 34 01 01 01 05  04 03 03 02 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("VBIDataDescriptor", "Defines the VBI Data Descriptor Set", "GenericDataEssenceDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 5b 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("ANCDataDescriptor", "Defines the VBI Data Descriptor Set", "GenericDataEssenceDescriptor", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 5c 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("MultipleDescriptor", "Defines the Multiple Descriptor set", "FileDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 44 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("FileDescriptors", "Unordered array of strong references to File Descriptor sets (1 per interleaved item within the Essence Container)", ClassUsageRequired, "StrongReferenceVectorFileDescriptor", 0, 0, 0x3f01, "06 0e 2b 34 01 01 01 04  06 01 01 04 06 0b 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("MPEG2VideoDescriptor", "Defines the MPEG2 Picture Essence Descriptor set", "CDCIEssenceDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 51 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("SingleSequence", "TRUE if the essence consists of a single MPEG sequence. False if there are a number of sequences. This flag implies that the sequence header information is not varying in the essence stream.", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 02 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ConstantBFrames", "TRUE if the number of B frames is always constant", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 03 00 00", "false", NULL)
			MXFLIB_CLASS_ITEM("CodedContentType", "0= \"Unknown\",1= \"Progressive\", 2= \"Interlaced\", 3= \"Mixed\": an enumerated value which tells if the underlying content which was MPEG coded was of a known type", ClassUsageOptional, "UInt8", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 04 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("LowDelay", "TRUE if low delay mode was used in the sequence", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 05 00 00", "false", NULL)
			MXFLIB_CLASS_ITEM("ClosedGOP", "TRUE if closed_gop is set in all GOP Headers, per 13818-1 IBP descriptor", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 06 00 00", "false", NULL)
			MXFLIB_CLASS_ITEM("IdenticalGOP", "TRUE if every GOP in the sequence is constructed the same, per 13818-1 IBP descriptor", ClassUsageOptional, "Boolean", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 07 00 00", "false", NULL)
			MXFLIB_CLASS_ITEM("MaxGOP", "Specifies the maximum occurring spacing between I frames, per 13818-1 IBP descriptor. A value of 0 or the absence of this property implies no limit to the maximum GOP", ClassUsageOptional, "UInt16", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 08 00 00", "0", NULL)
			MXFLIB_CLASS_ITEM("BPictureCount", "Specifies the maximum number of B pictures between P or I frames, equivalent to 13818-2 annex D (M-1)", ClassUsageOptional, "UInt16", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 09 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("BitRate", "Maximum bit rate of MPEG video elementary stream in bit/s as defined in ISO-13818-2 bit_rate property", ClassUsageOptional, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 0b 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ProfileAndLevel", "Specifies the MPEG-2 video profile and level. The value is taken directly from the profile_and_level_indication in the MPEG-2 sequence header extension. For main profile @ main level, the value is 0x48. For 4:2:2 profile @ main level, the value is 0x85", ClassUsageOptional, "UInt8", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 05  04 01 06 02 01 0a 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("WaveAudioDescriptor", "Defines the Wave Audio Essence Descriptor Set", "GenericSoundEssenceDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 48 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("BlockAlign", "Sample Block alignment", ClassUsageRequired, "UInt16", 0, 0, 0x3d0a, "06 0e 2b 34 01 01 01 05  04 02 03 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SequenceOffset", "Zero-based ordinal frame number of first essence data within five-frame sequence", ClassUsageOptional, "UInt8", 0, 0, 0x3d0b, "06 0e 2b 34 01 01 01 05  04 02 03 02 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("AvgBps", "Average Bytes per second", ClassUsageRequired, "UInt32", 0, 0, 0x3d09, "06 0e 2b 34 01 01 01 05  04 02 03 03 05 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ChannelAssignment", "UL enumerating the channel assignment in use", ClassUsageOptional, "UL", 0, 0, 0x3d32, "06 0e 2b 34 01 01 01 07  04 02 01 01 05 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PeakEnvelopeVersion", "Peak envelope version information (BWF dwVersion)", ClassUsageOptional, "UInt32", 0, 0, 0x3d29, "06 0e 2b 34 01 01 01 08  04 02 03 01 06 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PeakEnvelopeFormat", "Format of a peak point (BWF dwFormat)", ClassUsageOptional, "UInt32", 0, 0, 0x3d2a, "06 0e 2b 34 01 01 01 08  04 02 03 01 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PointsPerPeakValue", "Number of peak points per peak value (BWF dwPointsPerValue)", ClassUsageOptional, "UInt32", 0, 0, 0x3d2b, "06 0e 2b 34 01 01 01 08  04 02 03 01 08 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PeakEnvelopeBlockSize", "Number of audio samples used to generate each peak frame (BWF dwBlockSize)", ClassUsageOptional, "UInt32", 0, 0, 0x3d2c, "06 0e 2b 34 01 01 01 08  04 02 03 01 09 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PeakChannels", "Number of peak channels (BWF dwPeakChannels)", ClassUsageOptional, "UInt32", 0, 0, 0x3d2d, "06 0e 2b 34 01 01 01 08  04 02 03 01 0a 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PeakFrames", "Number of peak frames (BWF dwNumPeakFrames)", ClassUsageOptional, "UInt32", 0, 0, 0x3d2e, "06 0e 2b 34 01 01 01 08  04 02 03 01 0b 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PeakOfPeaksPosition", "Offset to the first audio sample whose absolute value is the maximum value of the entire audio file (BWF dwPosPeakOfPeaks, extended to 64 bits)", ClassUsageOptional, "Position", 0, 0, 0x3d2f, "06 0e 2b 34 01 01 01 08  04 02 03 01 0c 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PeakEnvelopeTimestamp", "Time stamp of the creation of the peak data (BWF strTimeStamp converted to TimeStamp)", ClassUsageOptional, "Timestamp", 0, 0, 0x3d30, "06 0e 2b 34 01 01 01 08  04 02 03 01 0d 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PeakEnvelopeData", "Peak envelope data (BWF peak_envelope_data)", ClassUsageOptional, "Stream", 0, 0, 0x3d31, "06 0e 2b 34 01 01 01 05  04 02 03 01 0e 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("AES3PCMDescriptor", "AES3 PCM Descriptor Set", "WaveAudioDescriptor", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 47 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("Emphasis", "AES3 Emphasis (aligned to LSB of this property)", ClassUsageOptional, "UInt8", 0, 0, 0x3d0d, "urn:x-ul:060E2B34.0101.0105.04020501.06000000", "0", NULL)
			MXFLIB_CLASS_ITEM("BlockStartOffset", "AES3 Position of first Z preamble in essence stream", ClassUsageOptional, "UInt16", 0, 0, 0x3d0f, "urn:x-ul:060E2B34.0101.0105.04020302.03000000", "0", NULL)
			MXFLIB_CLASS_ITEM("AuxBitsMode", "AES3 Use of Auxiliary Bits", ClassUsageOptional, "UInt8", 0, 0, 0x3d08, "urn:x-ul:060E2B34.0101.0105.04020501.01000000", "0", NULL)
			MXFLIB_CLASS_ITEM("ChannelStatusMode", "AES3 Enumerated mode of carriage of channel status data", ClassUsageOptional, "ChannelStatusModeTypeBatch", 0, 0, 0x3d10, "urn:x-ul:060E2B34.0101.0105.04020501.02000000", NULL, NULL)
			MXFLIB_CLASS_ITEM("FixedChannelStatusData", "AES3 Fixed data pattern for channel status data", ClassUsageOptional, "RAWBatch", 0, 0, 0x3d11, "urn:x-ul:060E2B34.0101.0105.04020501.03000000", NULL, NULL)
			MXFLIB_CLASS_ITEM("UserDataMode", "AES3 Enumerated mode of carriage of user data, defined by AES3 section 4.", ClassUsageOptional, "RAWBatch", 0, 0, 0x3d12, "urn:x-ul:060E2B34.0101.0105.04020501.04000000", NULL, NULL)
			MXFLIB_CLASS_ITEM("FixedUserData", "AES3 Fixed data pattern for user data", ClassUsageOptional, "RAWBatch", 0, 0, 0x3d13, "urn:x-ul:060E2B34.0101.0105.04020501.05000000", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("UnknownChunk", "", "InterchangeObject", "06 0E 2B 34 02 53 01 01 0D 01 01 01 01 01 4F 00")
			MXFLIB_CLASS_ITEM("ChunkID", "", ClassUsageRequired, "UInt32", 0, 0, 0x4f01, "06 0E 2B 34 01 01 01 08 04 06 08 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ChunkLength", "", ClassUsageRequired, "UInt32", 0, 0, 0x4f02, "06 0E 2B 34 01 01 01 08 04 06 09 03 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ChunkData", "", ClassUsageRequired, "Stream", 0, 0, 0x4f03, "06 0E 2B 34 01 01 01 08 04 07 04 00 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes_2)
		MXFLIB_CLASS_SET_EX("Event", "", "Segment", 2, 2, "06 0E 2B 34 02 53 01 01 0D 01 01 01 01 01 06 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("EventStartPosition", "Offset into the descriptive metadata track in edit units", ClassUsageRequired, "Position", 8, 8, 0x0601, "06 0e 2b 34 01 01 01 02  07 02 01 03 03 03 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EventComment", "Description of the Descriptive Metadata Framework", ClassUsageOptional, "UTF16String", 0, 0, 0x0602, "06 0e 2b 34 01 01 01 02  05 30 04 04 01 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("CommentMarker", "", "Event", 2, 2, "06 0E 2B 34 02 53 01 01 0D 01 01 01 01 01 08 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("AnnotationSource", "", ClassUsageOptional, "StrongReferenceSourceReference", 0, 0, 0x0901, "06 0E 2B 34 01 01 01 02 06 01 01 04 02 0A 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("DMSegment", "Descriptive Metadata Segment", "CommentMarker", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 41 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("TrackIDs", "An unordered list of track ID values that identify the tracks in this Package to which this DM Framework refers (if omitted, refers to all essence tracks)", ClassUsageDecoderRequired, "UInt32Batch", 0, 0, 0x6102, "06 0e 2b 34 01 01 01 04  01 07 01 05 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DMFramework", "Strong Reference to the Descriptive Metadata Framework", ClassUsageDecoderRequired, "StrongReferenceDMFramework", 16, 16, 0x6101, "06 0e 2b 34 01 01 01 05  06 01 01 04 02 0c 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("DMSourceClip", "Descriptive Metadata SourceClip", "SourceClip", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 45 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("DMSourceClipTrackIDs", "An unordered list of track ID values that identify the tracks in this Package to which the referenced Descriptive Metadata refers (if omitted, refers to all essence tracks)", ClassUsageOptional, "UInt32Batch", 0, 0, 0x6103, "06 0e 2b 34 01 01 01 05  01 07 01 06 00 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("DM_Set", "Superclass for all concrete DM Frameworks", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01 0d 01 04 00 00 00 00 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("DM_Framework", "Superclass for all concrete DM Frameworks", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 04 01 00 00 00 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END

	// Types definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types_2)
		MXFLIB_TYPE_INTERPRETATION("ComponentSamplePrecisionType", "JPEG 2000 component sample precision", "UInt8", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("CodingStyleType", "JPEG 2000 coding style", "UInt8", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("ProgressionOrderType", "JPEG 2000 progression order", "UInt8", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("MultipleComponentTransformationType", "JPEG 2000 multiple component transformation", "UInt8", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("CodeblockExponentType", "JPEG 2000 code-block exponent offset value", "UInt8", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("CodeblockStyleType", "JPEG 2000 style of code-block coding passes", "UInt8", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("WaveletTransformationType", "JPEG 2000 wavelet type", "UInt8", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("PrecinctSizeType", "JPEG 2000 precinct width and height", "UInt8", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION("QuantizationStyleType", "JPEG 2000 quantization style", "UInt8", "", 0, false)
		MXFLIB_TYPE_COMPOUND("ComponentSizing", "JPEG 2000 Component Sizing", "")
			MXFLIB_TYPE_COMPOUND_ITEM("Ssiz", "Component sample precision", "ComponentSamplePrecisionType", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("XRsiz", "Horizontal separation of a sample of this component with respect to the reference grid", "UInt8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("YRsiz", "Vertical separation of a sample of this component with respect to the reference grid", "UInt8", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND("SGcod", "SGcod parameters for coding style", "")
			MXFLIB_TYPE_COMPOUND_ITEM("ProgressionOrder", "Progression order", "ProgressionOrderType", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("NumberOfLayers", "Number of Layers", "UInt16", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("MultipleComponentTransformation", "Multiple component transformation", "MultipleComponentTransformationType", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND("SPcod", "SPcod parameters for coding style", "")
			MXFLIB_TYPE_COMPOUND_ITEM("DecompositionLevels", "Number of decomposition levels. Zero implies no transformation", "UInt8", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("CodeblockWidth", "Code-block width exponent offset value", "CodeblockExponentType", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("CodeblockHeight", "Code-block height exponent offset value", "CodeblockExponentType", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("CodeblockStyle", "Style of the code-block coding passes", "CodeblockStyleType", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("Transformation", "Wavelet transformation used", "WaveletTransformationType", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND("CodingStyleDefault", "Coding style default", "")
			MXFLIB_TYPE_COMPOUND_ITEM("Scod", "Coding style", "CodingStyleType", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("SGcod", "SGcod parameters for coding style", "SGcod", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("SPcod", "SPcod parameters for coding style", "SPcod", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("PrecinctSize", "Precinct width and height", "PrecinctSizeArray", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_COMPOUND("QuantizationDefault", "Quantization style default", "")
			MXFLIB_TYPE_COMPOUND_ITEM("Sqcd", "Quantization style for all components", "QuantizationStyleType", "", 0)
			MXFLIB_TYPE_COMPOUND_ITEM("SPqcd", "Quantization step size value for the each subband", "QuantizationStyleArray", "", 0)
		MXFLIB_TYPE_COMPOUND_END
		MXFLIB_TYPE_MULTIPLE("PrecinctSizeArray", "Array of JPEG 2000 PrecinctSize items", "PrecinctSizeType", "", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("QuantizationStyleArray", "Array of JPEG 2000 QuantizationStyle items", "QuantizationStyleType", "", ARRAYIMPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("ComponentSizingBatch", "Batch of Component Sizing details for JPEG 2000", "ComponentSizing", "", ARRAYEXPLICIT, 0)
	MXFLIB_TYPE_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes_3)
		MXFLIB_CLASS_SET("JPEG2000PictureSubDescriptor", "JPEG 2000 Picture Sub Descriptor", "InterchangeObject", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 5a 00")
			MXFLIB_CLASS_ITEM("Rsiz", "An enumerated value that defines the decoder capabilities", ClassUsageRequired, "UInt16", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Xsiz", "Width of the reference grid", ClassUsageRequired, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Ysiz", "Height of the reference grid", ClassUsageRequired, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 03 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("XOsiz", "Horizontal offset from the origin of the reference grid to the left side of the image area", ClassUsageRequired, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("YOsiz", "Vertical offset from the origin of the reference grid to the top side of the image area", ClassUsageRequired, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 05 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("XTsiz", "Width of one reference tile with respect to the reference grid", ClassUsageRequired, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 06 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("YTsiz", "Height of one reference tile with respect to the reference grid", ClassUsageRequired, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("XTOsiz", "Horizontal offset from the origin of the reference grid to the left side of the first tile", ClassUsageRequired, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 08 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("YTOsiz", "Vertical offset from the origin of the reference grid to the top side of the first tile", ClassUsageRequired, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 09 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Csiz", "The number of components in the picture", ClassUsageRequired, "UInt16", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 0a 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PictureComponentSizing", "Array of picture components", ClassUsageRequired, "ComponentSizingBatch", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 0b 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("CodingStyleDefault", "Default coding style for all components. Use this value only if static for all pictures in the Essence Container", ClassUsageOptional, "CodingStyleDefault", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 0c 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("QuantizationDefault", "Default quantization style for all components. Use this value only if static for all pictures in the Essence Container", ClassUsageOptional, "QuantizationDefault", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0a  04 01 06 03 0d 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes_4)
		MXFLIB_CLASS_SET("PackageMarkerObject", "Package Marker Object", "InterchangeObject", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 60 00")
			MXFLIB_CLASS_ITEM("TimebaseReferenceTrackID", "Timebase ReferenceTrack ID", ClassUsageRequired, "UInt32", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0C  06 01 01 03 0e 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PackageMarkInPosition", "Package Mark In Position", ClassUsageDecoderRequired, "Position", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0A  07 02 01 03 01 0e 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PackageMarkOutPosition", "Package Mark Out Position", ClassUsageDecoderRequired, "Position", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0A  07 02 01 03 02 04 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("MaterialPackage", "Material Package", "GenericPackage", "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 36 00")
			MXFLIB_CLASS_ITEM("PackageMarker", "Package Marker", ClassUsageOptional, "StrongReferencePackageMarkerObject", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 0C  06 01 01 04 02 0f 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END

	// Types definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types_3)
		MXFLIB_TYPE_INTERPRETATION_REF("GlobalReferenceApplicationPluginObject", "Global Reference to Application Plugin Object", "GlobalRef", "", 0, false, ClassRefUndefined, "ApplicationPluginObject")
		MXFLIB_TYPE_MULTIPLE_REF("StrongReferenceSetApplicationPluginObject", "Set of StrongReferences to Application Plug-In Objects", "StrongRef", "", ARRAYEXPLICIT, 0, ClassRefUndefined, "ApplicationPluginObject")
	MXFLIB_TYPE_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes_5)
		MXFLIB_CLASS_EXTEND("InterchangeObject", "", "", "")
			MXFLIB_CLASS_ITEM("ApplicationPluginBatch", "Application Plug-In Batch", ClassUsageOptional, "AUIDSet", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.06.01.01.04.02.0e.00.00", NULL, NULL)
		MXFLIB_CLASS_EXTEND_END
		MXFLIB_CLASS_EXTEND("Preface", "", "", "")
			MXFLIB_CLASS_ITEM("ApplicationSchemesBatch", "A Batch of Universal Labels of all the Application Metadata schemes used in this file. Individual UL values are listed in the Registry defined by SMPTE 400M (RP224)", ClassUsageOptional, "AUIDSet", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.01.02.02.10.02.03.00.00", NULL, NULL)
		MXFLIB_CLASS_EXTEND_END
		MXFLIB_CLASS_EXTEND("ApplicationObject", "Defines the Abstract Superclass of the Application Plug-in Objects and Application Referenced Objects", "InterchangeObject", "urn:x-ul:060E2B34.0253.0101.0D010101.01016600")
			MXFLIB_CLASS_ITEM("BaseClass", "Class Identifier of the base class that this object extends", ClassUsageOptional, "AUID", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.06.01.01.04.01.0B.00.00", NULL, NULL)
		MXFLIB_CLASS_EXTEND_END
		MXFLIB_CLASS_EXTEND("ApplicationPluginObject", "Defines the Application Plug-in Object Set", "ApplicationObject", "urn:x-ul:060E2B34.0253.0101.0D010101.01016100")
			MXFLIB_CLASS_ITEM("ApplicationPluginInstanceID", "ID of this application metadata plug-in", ClassUsageRequired, "UUID", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.05.20.07.01.0d.00.00.00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ApplicationScheme", "Universal Label of the Application Metadata scheme contained in this Plug-In Object", ClassUsageRequired, "UL", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.04.06.08.03.00.00.00.00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ApplicationEnvironmentID", "RFC 3986 Uniform Resource Identifier that identifies the application to which the information in this Plug-In Object applies", ClassUsageOptional, "UTF16String", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.05.20.07.01.0f.00.00.00", NULL, NULL)
		MXFLIB_CLASS_EXTEND_END
		MXFLIB_CLASS_EXTEND("ApplicationReferencedObject", "Defines the Application Referenced Object Set", "ApplicationObject", "urn:x-ul:060E2B34.0253.0101.0D010101.01016200")
			MXFLIB_CLASS_ITEM("LinkedApplicationPluginInstanceID", "global reference to the Application Plug-In Object that (directly or indirectly) strongly references this Application Metadata Referenced Object Set.", ClassUsageRequired, "GlobalReferenceApplicationPluginObject", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.05.20.07.01.0b.00.00.00", NULL, NULL)
		MXFLIB_CLASS_EXTEND_END
	MXFLIB_CLASS_END

	// Types definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types_4)
		MXFLIB_TYPE_INTERPRETATION("GlobalReferenceDescriptiveMetadataPlugin", "Global Reference to a Descriptive Metadata Plugin", "GlobalRef", "", 0, false)
	MXFLIB_TYPE_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes_6)
		MXFLIB_CLASS_SET_EX("DMSegment", "Descriptive Metadata Segment", "StructuralComponent", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 41 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("DescriptiveMetadataPluginID", "ID of this DM Plug-in instance", ClassUsageOptional, "GlobalReferenceDescriptiveMetadataPlugin", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.05.20.07.01.0e.00.00.00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DescriptiveMetadataScheme", "The Universal Label of the Descriptive Metadata scheme that is referenced by the DM Framework Property. Individual UL values are listed in the Registry defined by SMPTE 400M (RP224).", ClassUsageOptional, "UL", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.04.06.08.04.00.00.00.00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DescriptiveMetadataApplicationEnvironmentID", "The RFC 3986 Uniform Resource Identifier that identifies the application to which the information in this DM plug-in applies", ClassUsageOptional, "UTF16String", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.05.20.07.01.10.00.00.00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("DM_Framework", "Superclass for all concrete DM Frameworks", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01  0d 01 04 01 00 00 00 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("LinkedDescriptiveFrameworkPluginID", "In-File Weak Reference to the DM Segment that strongly references this Descriptive Framework instance", ClassUsageOptional, "GlobalReferenceDescriptiveMetadataPlugin", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.05.20.07.01.0c.00.00.00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET_EX("DM_Set", "Superclass for all concrete DM Frameworks", "InterchangeObject", 2, 2, "06 0e 2b 34 02 53 01 01 0d 01 04 00 00 00 00 00", NULL, ClassFlags_ExtendSubs + ClassFlags_Baseline)
			MXFLIB_CLASS_ITEM("LinkedDescriptiveObjectPluginID", "In-File Weak Reference to the DM Segment that indirectly strongly references this Descriptive Object instance", ClassUsageOptional, "GlobalReferenceDescriptiveMetadataPlugin", 0, 0, 0x0000, "06.0E.2B.34.01.01.01.0C.05.20.07.01.11.00.00.00", NULL, NULL)
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes_7)
		MXFLIB_CLASS_SET("CryptographicFramework", "DCP-Encryption Cryptographic Framework", "DM_Framework", "06 0e 2b 34 02 53 01 01 0d 01 04 01 02 01 00 00")
			MXFLIB_CLASS_ITEM_REF("ContextSR", "Strong Reference to the associated Cryptographic Context", ClassUsageRequired, "UUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 04 02 0d 00 00 ", ClassRefStrong, "CryptographicContext", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("CryptographicContext", "cryptographic information that applies to encrypted essence tracks as a whole", "DM_Set", "06 0e 2b 34 02 53 01 01 0d 01 04 01 02 02 00 00")
			MXFLIB_CLASS_ITEM("ContextID", "Persistent Unique identifier for the context", ClassUsageRequired, "UUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 01 01 15 11 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SourceEssenceContainer", "Essence Container Label for the source essence, prior to encryption", ClassUsageRequired, "AUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 02 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("CipherAlgorithm", "Algorithm used for Triplet encryption, if any", ClassUsageRequired, "UL", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 01 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("MICAlgorithm", "Algorithm used for Triplet integrity, if any", ClassUsageRequired, "UL", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("CryptographicKeyID", "Unique identifier for the cryptographic key", ClassUsageRequired, "UUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 01 02 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_VARIABLEPACK("EncryptedTriplet", "encrypted data and cryptographic information specific to the Triplet", 2, "", "06 0e 2b 34 02 04 01 07 0d 01 03 01 02 7e 01 00")
			MXFLIB_CLASS_ITEM("ContextIDLink", "Persistent Unique identifier for the context.associated with this Triplet", ClassUsageRequired, "UUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 06 03 00 00 00 ", NULL, NULL)
			MXFLIB_CLASS_ITEM("PlaintextOffset", "Offset within the source at which encryption starts", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 09 06 09 02 01 03 00 00 00 ", NULL, NULL)
			MXFLIB_CLASS_ITEM("SourceKey", "Key of the source Triplet", ClassUsageRequired, "UL", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 02 03 00 00 00 ", NULL, NULL)
			MXFLIB_CLASS_ITEM("SourceLength", "Length of the value of the source Triplet", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 09 04 06 10 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EncryptedSourceValue", "Encrypted Source value starting at Plaintext Offset", ClassUsageRequired, "RAW", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 01 03 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TrackFileID", "The identifier of the AS-DCP Track File containing this Triplet", ClassUsageOptional, "UUID", 0, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 06 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SequenceNumber", "Sequence number of this Triplet within the Track File", ClassUsageOptional, "UInt64", 0, 8, 0x0000, "06 0e 2b 34 01 01 01 09 06 10 05 00 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("MIC", "Keyed HMAC", ClassUsageOptional, "DataValue", 0, 20, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 02 02 00 00 00", NULL, NULL)
		MXFLIB_CLASS_VARIABLEPACK_END
	MXFLIB_CLASS_END

	// Label definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types_5)
		MXFLIB_LABEL("EncryptedContainerLabel", "DCP-Crypto Encrypted Essence Container, frame-wrapped", "06 0e 2b 34 04 01 01 07 0d 01 03 01 02 0b 01 00")
		MXFLIB_MASKED_LABEL("DMSCrypto", "MXF Cryptographic DM Scheme", "06.0E.2B.34.04.01.01.07.0D.01.04.01.02.00.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.ff.ff.ff")
		MXFLIB_LABEL("CryptographicFrameworkLabel", "DCP-Crypto Framework", "06 0e 2b 34 04 01 01 07 0d 01 04 01 02 01 01 00")
		MXFLIB_LABEL("CipherAlgorithmAES128CBC", "Identifes the use of AES128 CBC mode cipher algorithm", "06 0e 2b 34 04 01 01 07 02 09 02 01 01 00 00 00")
		MXFLIB_LABEL("HMACAlgorithmSHA1128", "Identifes the use of SHA1 128 bit HMAC algorithm", "06 0e 2b 34 04 01 01 07 02 09 02 02 01 00 00 00")
	MXFLIB_TYPE_END

	// Label definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types_6)
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
		MXFLIB_LABEL("GenericEssenceContainerMultipleWrappings", "Empty Generic Container", "06 0e 2b 34 04 01 01 03 0d 01 03 01 02 7f 01 00")
		MXFLIB_MASKED_LABEL("MXFGC", "MXF Generic Container", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.00.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.ff.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCD10", "MXF-GC SMPTE D-10 Mappings", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.01.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCDV", "MXF-GC DV-DIF Mappings", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.02.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
		MXFLIB_MASKED_LABEL("MXFGCD11", "MXF-GC SMPTE D-11 Mappings", "06.0E.2B.34.04.01.01.00.0D.01.03.01.02.03.00.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.ff.ff")
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
		MXFLIB_MASKED_LABEL("MXFGCMultiple", "MXF-GC Generic Essence Multiple Mappings", "06.0E.2B.34.04.01.01.03.0D.01.03.01.02.7f.01.00", "00.00.00.00.00.00.00.ff.00.00.00.00.00.00.00.ff")
		MXFLIB_LABEL("SMPTE12MTimecodeTrack", "SMPTE 12M Timecode Track", "06.0E.2B.34.04.01.01.01.01.03.02.01.01.00.00.00")
		MXFLIB_LABEL("SMPTE12MTimecodeActiveUserBitsTrack", "SMPTE 12M Timecode Track with active user bits", "06.0E.2B.34.04.01.01.01.01.03.02.01.02.00.00.00")
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
		MXFLIB_LABEL("ATSCA52", "ATSC A/52 Compressed Audio", "06.0E.2B.34.04.01.01.01.04.02.02.02.03.02.01.00")
	MXFLIB_TYPE_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes_8)
		MXFLIB_CLASS_EXTEND("Preface", "", "", "")
			MXFLIB_CLASS_ITEM("Dictionaries", "Link to KXS 377-2 Dictionary", ClassUsageOptional, "StrongReferenceDictionary", 0, 0, 0x3b04, "06 0E 2B 34 01 01 01 02 06 01 01 04 02 02 00 00", NULL, NULL)
		MXFLIB_CLASS_EXTEND_END
	MXFLIB_CLASS_END

	// Types definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types_7)
		MXFLIB_TYPE_INTERPRETATION("StringArray", "Unicode UTF-16 coded strings, each zero terminated in a single outer string", "UTF16String", "urn:x-ul:060E2B34.0104.0101.04010500.00000000", 0, false)
	MXFLIB_TYPE_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes_9)
		MXFLIB_CLASS_SET("Dictionary", "Dictionary", "InterchangeObject", "06 0e 2b 34 02 53 01 01 0d 01 01 01 01 01 22 00")
			MXFLIB_CLASS_ITEM("DataDefinitions", "DataDefinitions", ClassUsageOptional, "StrongReferenceSetDataDefinition", 0, 0, 0x2605, "06 0E 2B 34 01 01 01 02 06 01 01 04 05 05 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ContainerDefinitions", "ContainerDefinitions", ClassUsageOptional, "StrongReferenceSetContainerDefinition", 0, 0, 0x2608, "06 0E 2B 34 01 01 01 02 06 01 01 04 05 08 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("CodecDefinitions", "CodecDefinitions", ClassUsageOptional, "StrongReferenceSetCodecDefinition", 0, 0, 0x2607, "06 0E 2B 34 01 01 01 02 06 01 01 04 05 07 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("DefinitionObject", "DefinitionObject", "InterchangeObject", "06 0e 2b 34 02 53 01 01 0d 01 01 01 01 01 1a 00")
			MXFLIB_CLASS_ITEM_REF("DefinitionObjectIdentification", "DefinitionObjectIdentification", ClassUsageRequired, "AUID", 0, 0, 0x1b01, "06 0E 2B 34 01 01 01 02 01 01 15 03 00 00 00 00", ClassRefTarget, "", NULL, NULL)
			MXFLIB_CLASS_ITEM("DefinitionObjectName", "DefinitionObjectName", ClassUsageRequired, "UTF16String", 0, 0, 0x1b02, "06 0E 2B 34 01 01 01 02 01 07 01 02 03 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("DefinitionObjectDescription", "DefinitionObjectDescription", ClassUsageOptional, "UTF16String", 0, 0, 0x1b03, "06 0E 2B 34 01 01 01 02 03 02 03 01 02 01 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("DataDefinition", "DataDefinition", "DefinitionObject", "06 0e 2b 34 02 53 01 01 0d 01 01 01 01 01 1b 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("CodecDefinition", "CodecDefinition", "DefinitionObject", "06 0e 2b 34 02 53 01 01 0d 01 01 01 01 01 1f 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("ContainerDefinition", "ContainerDefinition", "DefinitionObject", "06 0e 2b 34 02 53 01 01 0d 01 01 01 01 01 20 00")
			MXFLIB_CLASS_ITEM("EssenceIsIdentified", "EssenceIsIdentified", ClassUsageOptional, "Boolean", 0, 0, 0x2401, "06 0E 2B 34 01 01 01 01 03 01 02 01 03 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END

	// Types definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types_8)
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceMetaDictionary", "StrongReference to the MetaDictionary", "StrongRef", "", 0, false, ClassRefUndefined, "MetaDictionary")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferencePreface", "StrongReference to the Preface", "StrongRef", "", 0, false, ClassRefUndefined, "Preface")
	MXFLIB_TYPE_END

	// Types definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types_9)
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceMetaDefinition", "StrongReference to a MetaDefinition", "StrongRef", "", 0, false, ClassRefUndefined, "MetaDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferencePropertyDefinition", "StrongReference to a PropertyDefinition", "StrongRef", "urn:x-ul:060E2B34.01040101.05021900.00000000", 0, false, ClassRefUndefined, "PropertyDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceClassDefinition", "StrongReference to a ClassDefinition", "StrongRef", "", 0, false, ClassRefUndefined, "ClassDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("WeakReferenceClassDefinition", "WeakReference to a ClassDefinition", "WeakRef", "", 0, false, ClassRefUndefined, "ClassDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceTypeDefinition", "StrongReference to a TypeDefinition", "StrongRef", "", 0, false, ClassRefUndefined, "TypeDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("WeakReferenceTypeDefinition", "WeakReference to a TypeDefinition", "WeakRef", "", 0, false, ClassRefUndefined, "TypeDefinition")
		MXFLIB_TYPE_MULTIPLE("WeakReferenceArrayTypeDefinition", "Array of WeakReferences to TypeDefinitions", "WeakReferenceTypeDefinition", "urn:x-ul:060e2b34.0104.0101.05040200.00000000", ARRAYEXPLICIT, 0)
	MXFLIB_TYPE_END

	// Types definitions converted from file dict.xml
	MXFLIB_TYPE_START(DictData_Types_10)
		MXFLIB_TYPE_INTERPRETATION_REF("MetaReference", "Generic MetaReference", "MetaRef", "", 0, false, ClassRefUndefined, "MetaDefinition")
		MXFLIB_TYPE_INTERPRETATION("URI", "Uniform Resource Identifier", "UTF16String", "", 0, false)
		MXFLIB_TYPE_INTERPRETATION_REF("StrongReferenceExtensionScheme", "StrongReference to an ExtensionScheme", "StrongRef", "", 0, false, ClassRefUndefined, "ExtensionScheme")
		MXFLIB_TYPE_INTERPRETATION_REF("MetaReferenceMetaDefinition", "MetaReference to a MetaDefinition", "MetaReference", "", 0, false, ClassRefUndefined, "MetaDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("MetaReferenceTypeDefinition", "MetaReference to a TypeDefinition", "MetaReference", "urn:x-ul:060e2b34.0104.0101.05010900.00000000", 0, false, ClassRefUndefined, "TypeDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("MetaReferenceTypeDefinitionExtendibleEnumeration", "MetaReference to a TypeDefinitionExtendibleEnumeration", "MetaReference", "", 0, false, ClassRefUndefined, "TypeDefinitionExtendibleEnumeration")
		MXFLIB_TYPE_INTERPRETATION_REF("MetaReferenceClassDefinition", "MetaReference to a ClassDefinition", "MetaReference", "urn:x-ul:060e2b34.0104.0101.05010100.00000000", 0, false, ClassRefUndefined, "ClassDefinition")
		MXFLIB_TYPE_INTERPRETATION_REF("MetaReferencePropertyDefinition", "MetaReference to a PropertyDefinition", "MetaReference", "", 0, false, ClassRefUndefined, "PropertyDefinition")
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetMetaDefinition", "Set of StrongReferences to MetaDefinitions", "StrongReferenceMetaDefinition", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetExtensionScheme", "Set of StrongReferences to ExtensionScheme", "StrongReferenceExtensionScheme", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("MetaReferenceArrayTypeDefinition", "Array of MetaReferences to TypeDefinitions", "MetaReferenceTypeDefinition", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("MetaReferenceSetTypeDefinitionExtendibleEnumeration", "Set of MetaReferences to TypeDefinitionExtendibleEnumerations", "MetaReferenceTypeDefinitionExtendibleEnumeration", "", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetPropertyDefinition", "Set of StrongReferences to PropertyDefinitions", "StrongReferencePropertyDefinition", "urn:x-ul:060E2B34.01040101.05050B00.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetClassDefinition", "Set of StrongReferences to ClassDefinition sets", "StrongReferenceClassDefinition", "urn:x-ul:060E2B34.0104.0101.05050100.00000000", ARRAYEXPLICIT, 0)
		MXFLIB_TYPE_MULTIPLE("StrongReferenceSetTypeDefinition", "Set of StrongReferences to TypeDefinition sets", "StrongReferenceTypeDefinition", "urn:x-ul:060E2B34.0104.0101.05050c00.00000000", ARRAYEXPLICIT, 0)
	MXFLIB_TYPE_END

	// Class definitions converted from file dict.xml
	MXFLIB_CLASS_START(DictData_Classes_10)
		MXFLIB_CLASS_SET("Root", "Defines the Root set", "AbstractObject", "urn:x-ul:060E2B34.0253.0101.0d010101.03000000")
			MXFLIB_CLASS_ITEM("RootPreface", "References the Preface of the file", ClassUsageRequired, "StrongReferencePreface", 0, 0, 0x0002, "urn:x-ul:060E2B34.0101.010a.06010107.17000000", NULL, NULL)
			MXFLIB_CLASS_ITEM("RootExtensions", "References the Extension Schemes that are used in the file", ClassUsageRequired, "StrongReferenceSetExtensionScheme", 0, 0, 0x0023, "urn:x-ul:060E2B34.0101.010a.06010107.7f010000", NULL, NULL)
			MXFLIB_CLASS_ITEM("RootFormatVersion", "Simple integer version number of MetaModel", ClassUsageOptional, "UInt32", 0, 0, 0x0022, "urn:x-ul:060E2B34.0101.010a.06010107.19000000", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("ExtensionScheme", "Defines the ExtensionScheme set", "AbstractObject", "urn:x-ul:060E2B34.0253.0101.0d010101.02260000")
			MXFLIB_CLASS_ITEM("ExtensionSchemeID", "Globally unique identification of the ExtensionScheme", ClassUsageRequired, "AUID", 0, 0, 0x0024, "urn:x-ul:060E2B34.0101.010d.06010107.1b000000", NULL, NULL)
			MXFLIB_CLASS_ITEM("SymbolSpaceURI", "Namespace URI of the Extension Scheme", ClassUsageRequired, "URI", 0, 0, 0x0025, "urn:x-ul:060E2B34.0101.010d.06010107.1c000000", NULL, NULL)
			MXFLIB_CLASS_ITEM("PreferredPrefix", "Preferred namespace tag when SMPTE CD2001 Reg-XML encoding is used", ClassUsageOptional, "UTF16String", 0, 0, 0x0026, "urn:x-ul:060E2B34.0101.010d.06010107.1d000000", NULL, NULL)
			MXFLIB_CLASS_ITEM("ExtensionDescription", "Description of the Extension Scheme", ClassUsageOptional, "UTF16String", 0, 0, 0x0027, "urn:x-ul:060E2B34.0101.010d.06010107.1e000000", NULL, NULL)
			MXFLIB_CLASS_ITEM("MetaDefinitions", "References all MetaDefinitions in this Extension Scheme when they are contained in the MXF file", ClassUsageOptional, "StrongReferenceSetMetaDefinition", 0, 0, 0x0028, "urn:x-ul:060E2B34.0101.010d.06010107.1f000000", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("ExtendibleEnumerationElement", "Defines the Extendible Enumeration Element set", "DefinitionObject", "06 0E 2B 34 01 01 01 0d 06 01 01 07 7e 00 00 00")
			MXFLIB_CLASS_ITEM("ElementOf", "References the ExtendibleEnumerations in which this element is known to be used", ClassUsageOptional, "MetaReferenceSetTypeDefinitionExtendibleEnumeration", 0, 0, 0x002a, "06 0E 2B 34 01 01 01 0d 06 01 01 07 21 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("MetaDictionary", "[060E2B34.0253.0101.0D010101.02250000] From AAF", "AbstractObject", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 25 00 00")
			MXFLIB_CLASS_ITEM("ClassDefinitions", "urn:x-ul:060E2B34.0101.0102.06010107.07000000 from AAF", ClassUsageRequired, "StrongReferenceSetClassDefinition", 0, 0, 0x0003, "06 0E 2B 34 01 01 01 02 06 01 01 07 07 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TypeDefinitions", "urn:x-ul:060E2B34.0101.0102.06010107.08000000 from AAF", ClassUsageRequired, "StrongReferenceSetTypeDefinition", 0, 0, 0x0004, "06 0E 2B 34 01 01 01 02 06 01 01 07 08 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("MetaDefinition", "Defines the abstract MetaDefinition class", "AbstractObject", "urn:x-ul:060E2B34.0253.0101.0d010101.02240000")
			MXFLIB_CLASS_ITEM_REF("MetaDefinitionIdentification", "Specifies the unique identification for the item being defined", ClassUsageRequired, "AUID", 0, 0, 0x0005, "06 0E 2B 34 01 01 01 02 06 01 01 07 13 00 00 00", ClassRefTarget, "", NULL, NULL)
			MXFLIB_CLASS_ITEM("MetaDefinitionName", "Specifies the display name of the item being defined", ClassUsageRequired, "UTF16String", 0, 0, 0x0006, "06 0E 2B 34 01 01 01 02 03 02 04 01 02 01 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("MetaDefinitionDescription", "Provides an explanation of the use of the item being defined", ClassUsageOptional, "UTF16String", 0, 0, 0x0007, "06 0E 2B 34 01 01 01 02 06 01 01 07 14 01 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinition", "Defines the abstract TypeDefinition class", "MetaDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 03 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("ClassDefinition", "Defines the ClassDefinition set", "MetaDefinition", "urn:x-ul:060E2B34.0253.0101.0d010101.02010000")
			MXFLIB_CLASS_ITEM("ParentClass", "Specifies the parent of the class being defined", ClassUsageRequired, "WeakReferenceClassDefinition", 0, 0, 0x0008, "06 0E 2B 34 01 01 01 02 06 01 01 07 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("Properties", "Specifies the set of PropertyDefinition objects that define the properties for a class", ClassUsageOptional, "StrongReferenceSetPropertyDefinition", 0, 0, 0x0009, "06 0E 2B 34 01 01 01 02 06 01 01 07 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IsConcrete", "Specifies if the class is concrete", ClassUsageRequired, "Boolean", 0, 0, 0x000a, "06 0E 2B 34 01 01 01 02 06 01 01 07 03 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("PropertyDefinition", "Defines the PropertyDefinition set", "MetaDefinition", "urn:x-ul:060E2B34.0253.0101.0d010101.02020000")
			MXFLIB_CLASS_ITEM("IsOptional", "Specifies whether object instances can omit a value for the property", ClassUsageRequired, "Boolean", 0, 0, 0x000c, "06 0E 2B 34 01 01 01 02 03 01 02 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IsUniqueIdentifier", "Specifies that this property provides a unique identification for this object", ClassUsageOptional, "Boolean", 0, 0, 0x000e, "06 0E 2B 34 01 01 01 02 06 01 01 07 06 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("PropertyType", "Specifies the property type", ClassUsageRequired, "MetaReferenceTypeDefinition", 0, 0, 0x000b, "06 0E 2B 34 01 01 01 02 06 01 01 07 04 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("PropertyWrapperDefinition", "Defines the PropertyWrapperDefinition set", "PropertyDefinition", "urn:x-ul:060E2B34.0253.0101.0d010101.02270000")
			MXFLIB_CLASS_ITEM("OriginalProperty", "References the original definition of the reused Property", ClassUsageRequired, "MetaReferencePropertyDefinition", 0, 0, 0x0029, "06 0E 2B 34 01 01 01 0d 06 01 01 07 20 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_EXTEND("PropertyDefinition", "", "", "urn:x-ul:060E2B34.0253.0101.0d010101.02020000")
			MXFLIB_CLASS_ITEM("MemberOf", "Specifies the class of which this property is a member", ClassUsageOptional, "MetaReferenceClassDefinition", 0, 0, 0x0000, "", NULL, NULL)
		MXFLIB_CLASS_EXTEND_END
		MXFLIB_CLASS_EXTEND("PropertyDefinition", "", "", "urn:x-ul:060E2B34.0253.0101.0d010101.02020000")
			MXFLIB_CLASS_ITEM("LocalIdentification", "", ClassUsageOptional, "LocalTagType", 0, 0, 0x000d, "06 0E 2B 34 01 01 01 02 06 01 01 07 05 00 00 00", NULL, NULL)
		MXFLIB_CLASS_EXTEND_END
		MXFLIB_CLASS_SET("TypeDefinitionCharacter", "Defines the TypeDefinitionCharacter set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 23 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionEnumeration", "Defines the TypeDefinitionEnumeration set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 07 00 00")
			MXFLIB_CLASS_ITEM("ElementType", "Specifies the TypeDefinition that defines the underlying integer type", ClassUsageRequired, "MetaReferenceTypeDefinition", 0, 0, 0x0014, "06 0E 2B 34 01 01 01 02 06 01 01 07 0B 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ElementNames", "Specifies the names associated with each enumerated value", ClassUsageRequired, "StringArray", 0, 0, 0x0015, "06 0E 2B 34 01 01 01 02 03 01 02 03 04 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ElementValues", "Specifies the valid enumerated values", ClassUsageRequired, "Int64Batch", 0, 0, 0x0016, "06 0E 2B 34 01 01 01 02 03 01 02 03 05 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionExtendibleEnumeration", "Defines the TypeDefinitionExtendibleEnumeration set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 20 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionFixedArray", "Defines the TypeDefinitionFixedArray set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 08 00 00")
			MXFLIB_CLASS_ITEM("FixedArrayElementType", "Specifies the TypeDefinition that defines the type of each element of the array", ClassUsageRequired, "MetaReferenceTypeDefinition", 0, 0, 0x0017, "06 0E 2B 34 01 01 01 02 06 01 01 07 0C 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("ElementCount", "Specifies the number of elements in the array", ClassUsageRequired, "UInt32", 0, 0, 0x0018, "06 0E 2B 34 01 01 01 02 03 01 02 03 03 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionIndirect", "Defines the TypeDefinitionIndirect set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 21 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionInteger", "Defines the TypeDefinitionInteger set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 04 00 00")
			MXFLIB_CLASS_ITEM("Size", "Specifies the number of bytes to store the integer. Legal values are 1, 2, 4, and 8", ClassUsageRequired, "UInt8", 0, 0, 0x000f, "06 0E 2B 34 01 01 01 02 03 01 02 03 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("IsSigned", "Specifies if the integer is signed (True) or unsigned (False)", ClassUsageRequired, "Boolean", 0, 0, 0x0010, "06 0E 2B 34 01 01 01 02 03 01 02 03 02 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionOpaque", "Defines the TypeDefinitionOpaque set", "TypeDefinitionIndirect", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 22 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionRecord", "Defines the TypeDefinitionRecord set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0D 00 00")
			MXFLIB_CLASS_ITEM("MemberTypes", "Specifies the type of each element of the record", ClassUsageRequired, "MetaReferenceArrayTypeDefinition", 0, 0, 0x001c, "06 0E 2B 34 01 01 01 02 06 01 01 07 11 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("MemberNames", "Specifies the name of each element of the record", ClassUsageRequired, "StringArray", 0, 0, 0x001d, "06 0E 2B 34 01 01 01 02 03 01 02 03 06 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionRename", "Defines the TypeDefinitionRename set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0E 00 00")
			MXFLIB_CLASS_ITEM("RenamedType", "Specifies the underlying type", ClassUsageRequired, "MetaReferenceTypeDefinition", 0, 0, 0x001e, "06 0E 2B 34 01 01 01 02 06 01 01 07 12 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionSet", "Defines the TypeDefinitionSet set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0A 00 00")
			MXFLIB_CLASS_ITEM("SetElementType", "Specifies the TypeDefinition that identifies the kind of object reference. This TypeDefinition shall be a TypeDefinitionStrongObjectReference or TypeDefinitionWeakObjectReference or a scalar type", ClassUsageRequired, "MetaReferenceTypeDefinition", 0, 0, 0x001a, "06 0E 2B 34 01 01 01 02 06 01 01 07 0E 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionStream", "Defines the TypeDefinitionStream set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0C 00 00")
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionString", "Defines the TypeDefinitionString set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 0B 00 00")
			MXFLIB_CLASS_ITEM("StringElementType", "Specifies the string element, which may be a character (TypeDefinitionCharacter) or integer (TypeDefinitionInteger)", ClassUsageRequired, "MetaReferenceTypeDefinition", 0, 0, 0x001b, "06 0E 2B 34 01 01 01 02 06 01 01 07 0F 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionStrongObjectReference", "Defines the TypeDefinitionStrongObjectReference set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 05 00 00")
			MXFLIB_CLASS_ITEM("ReferencedType", "Specifies the class that the referenced object shall belong to (the referenced object may also belong to a subclass of the referenced class)", ClassUsageRequired, "MetaReferenceClassDefinition", 0, 0, 0x0011, "06 0E 2B 34 01 01 01 02 06 01 01 07 09 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionVariableArray", "Defines the TypeDefinitionVariableArray set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 09 00 00")
			MXFLIB_CLASS_ITEM("VariableArrayElementType", "Specifies the type of the element of the array", ClassUsageRequired, "MetaReferenceTypeDefinition", 0, 0, 0x0019, "06 0E 2B 34 01 01 01 02 06 01 01 07 0D 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_SET("TypeDefinitionWeakObjectReference", "Defines the TypeDefinitionWeakObjectReference set", "TypeDefinition", "06 0E 2B 34 02 53 01 01 0D 01 01 01 02 06 00 00")
			MXFLIB_CLASS_ITEM("WeakReferencedType", "Specifies the class that the referenced object shall belong to (the referenced object may also belong to a subclass of the referenced class)", ClassUsageRequired, "MetaReferenceClassDefinition", 0, 0, 0x0012, "06 0E 2B 34 01 01 01 02 06 01 01 07 0A 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TargetSet", "Specifies the AUIDs that specify the properties from the root of the file to the property that has the StrongReferenceSet containing the uniquely identified objects that may be the target of the weak reference", ClassUsageRequired, "AUIDArray", 0, 0, 0x0013, "06 0E 2B 34 01 01 01 02 03 01 02 03 0B 00 00 00", NULL, NULL)
		MXFLIB_CLASS_SET_END
	MXFLIB_CLASS_END

	// Build a complete dictionary from above types and classes
	MXFLIB_DICTIONARY_START(DictData)
		MXFLIB_DICTIONARY_TYPES(DictData_Types)
		MXFLIB_DICTIONARY_TYPES(DictData_Types_2)
		MXFLIB_DICTIONARY_TYPES(DictData_Types_3)
		MXFLIB_DICTIONARY_TYPES(DictData_Types_4)
		MXFLIB_DICTIONARY_TYPES(DictData_Types_5)
		MXFLIB_DICTIONARY_TYPES(DictData_Types_6)
		MXFLIB_DICTIONARY_TYPES(DictData_Types_7)
		MXFLIB_DICTIONARY_TYPES(DictData_Types_8)
		MXFLIB_DICTIONARY_TYPES(DictData_Types_9)
		MXFLIB_DICTIONARY_TYPES(DictData_Types_10)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes_2)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes_3)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes_4)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes_5)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes_6)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes_7)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes_8)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes_9)
		MXFLIB_DICTIONARY_CLASSES(DictData_Classes_10)
	MXFLIB_DICTIONARY_END
