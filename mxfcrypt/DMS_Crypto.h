	// Class definitions converted from file DMS_Crypto.xml
	MXFLIB_CLASS_START(CryptoDictData)
		MXFLIB_CLASS_ITEM("EncryptedContainerLabel", "DCP-Crypto Encrypted Essence Container, frame-wrapped", ClassUsageOptional, "UL", 0, 0, 0x0000, "06 0e 2b 34 04 01 01 07 0d 01 03 01 02 0b 01 00", NULL, NULL)
		MXFLIB_CLASS_ITEM("CryptographicFrameworkLabel", "DCP-Crypto Framework", ClassUsageOptional, "UL", 0, 0, 0x0000, "06 0e 2b 34 04 01 01 07 0d 01 04 01 02 01 01 00", NULL, NULL)
		MXFLIB_CLASS_SET("CryptographicFramework", "DCP-Encryption Cryptographic Framework", "DM_Framework", "06 0e 2b 34 02 53 01 01 0d 01 04 01 02 01 00 00")
			MXFLIB_CLASS_ITEM_REF("ContextSR", "Strong Reference to the associated Cryptographic Context", ClassUsageRequired, "UUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 04 02 0d 00 00 ", ClassRefStrong, "CryptographicContext", NULL, NULL)
		MXFLIB_CLASS_SET_END
		MXFLIB_CLASS_EXTEND("CryptographicContext", "cryptographic information that applies to encrypted essence tracks as a whole", "DM_Set", "06 0e 2b 34 02 53 01 01 0d 01 04 01 02 02 00 00")
			MXFLIB_CLASS_ITEM("ContextID", "Persistent Unique identifier for the context", ClassUsageRequired, "UUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 01 01 15 11 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SourceEssenceContainer", "Essence Container Label for the source essence, prior to encryption", ClassUsageRequired, "UL", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 02 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("CipherAlgorithm", "Algorithm used for Triplet encryption, if any", ClassUsageRequired, "UL", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 01 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("MICAlgorithm", "Algorithm used for Triplet integrity, if any", ClassUsageRequired, "UL", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 02 01 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("CryptographicKeyID", "Unique identifier for the cryptographic key", ClassUsageRequired, "UUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 01 02 00 00 00", NULL, NULL)
		MXFLIB_CLASS_EXTEND_END
		MXFLIB_CLASS_VARIABLEPACK("EncryptedTriplet", "encrypted data and cryptographic information specific to the Triplet", 0, "", "06 0e 2b 34 02 04 01 07 0d 01 03 01 02 7e 01 00")
			MXFLIB_CLASS_ITEM("ContextIDLink", "Persistent Unique identifier for the context.associated with this Triplet", ClassUsageRequired, "UUID", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 06 03 00 00 00 ", NULL, NULL)
			MXFLIB_CLASS_ITEM("PlaintextOffset", "Offset within the source at which encryption starts", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 09 06 09 02 01 03 00 00 00 ", NULL, NULL)
			MXFLIB_CLASS_ITEM("SourceKey", "Key of the source Triplet", ClassUsageRequired, "UL", 16, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 02 03 00 00 00 ", NULL, NULL)
			MXFLIB_CLASS_ITEM("SourceLength", "Length of the value of the source Triplet", ClassUsageRequired, "UInt64", 8, 8, 0x0000, "06 0e 2b 34 01 01 01 09 04 06 10 02 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("EncryptedSourceValue", "Encrypted Source value starting at Plaintext Offset", ClassUsageRequired, "RAW", 0, 0, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 01 03 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("TrackFileID", "The identifier of the AS-DCP Track File containing this Triplet", ClassUsageOptional, "UUID", 0, 16, 0x0000, "06 0e 2b 34 01 01 01 09 06 01 01 06 02 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("SequenceNumber", "Sequence number of this Triplet within the Track File", ClassUsageOptional, "UInt64", 0, 8, 0x0000, "06 0e 2b 34 01 01 01 09 06 10 05 00 00 00 00 00", NULL, NULL)
			MXFLIB_CLASS_ITEM("MIC", "Keyed HMAC", ClassUsageOptional, "DataValue", 0, 20, 0x0000, "06 0e 2b 34 01 01 01 09 02 09 03 02 02 00 00 00", NULL, NULL)
		MXFLIB_CLASS_VARIABLEPACK_END
		MXFLIB_CLASS_ITEM("CipherAlgorithmAES128CBC", "Identifes the use of AES128 CBC mode cipher algorithm", ClassUsageOptional, "UL", 0, 0, 0x0000, "06 0e 2b 34 04 01 01 07 02 09 02 01 01 00 00 00", NULL, NULL)
		MXFLIB_CLASS_ITEM("HMACAlgorithmSHA1128", "Identifes the use of SHA1 128 bit HMAC algorithm", ClassUsageOptional, "UL", 0, 0, 0x0000, "06 0e 2b 34 04 01 01 07 02 09 02 02 01 00 00 00", NULL, NULL)
	MXFLIB_CLASS_END
