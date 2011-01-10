/*! \file	metadict.cpp
 *	\brief	Basic MXF metadictionary functions
 *
 *	\version $Id: metadict.cpp,v 1.1 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2010, Metaglue Corporation
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

#include "mxflib/mxflib.h"

using namespace mxflib;

#include <stdio.h>
#include <iostream>

using namespace std;



namespace
{
	//! Build a type definition record from a TypeDefinition MDObject that is a sub-item in a compound
	/*! \return A smart pointer to the new record, or NULL on a fatal failure
	*  Most failures will cause a valid, but imperfect, definition to be built - ideally with reduced but valid functionality
	*/
	TypeRecordPtr BuildSubTypeRecord(MDObjectPtr &TypeDef)
	{
		TypeRecordPtr ThisType = new TypeRecord;

		/* Build a randmom type ID (metadict does not give IDs to sub-items) */
		mxflib::UUIDPtr Temp = new mxflib::UUID;
		ULPtr TypeID = new UL(Temp);

		ThisType->Class = TypeSub;
		ThisType->UL = TypeID;

		MDObjectPtr BaseTypeDef = TypeDef[MetaDefinitionIdentification_UL];

		if(!BaseTypeDef)
		{
			error("TypeDefinition for record item \"%s\" has no valid base type\n", ThisType->Type.c_str());
			ThisType->Base = "UnknownType";
		}
		else
		{
			ThisType->Base = BaseTypeDef->GetString();
		}

		return ThisType;
	}


	//! Build a type definition record from a TypeDefinition MDObject
	/*! \return A smart pointer to the new record, or NULL on a fatal failure
	*  Most failures will cause a valid, but imperfect, definition to be built - ideally with reduced but valid functionality
	*/
	TypeRecordPtr BuildTypeRecord(MDObjectPtr &TypeDef)
	{
		TypeRecordPtr ThisType = new TypeRecord;

		/* Locate the type ID */
		UUIDPtr TypeID;
		DataChunkPtr IDData;
		MDObjectPtr TypeIDObject = TypeDef[MetaDefinitionIdentification_UL];
		if(TypeIDObject) IDData = TypeIDObject->PutData();
		if((!TypeIDObject) || (IDData->Size < 16))
		{
			error("No valid Type ID for %s at %s\n", TypeDef->FullName().c_str(), TypeDef->GetSourceLocation().c_str());
			
			// Make a random UUID to use instead
			TypeID = new mxflib::UUID;
		}
		else
		{
			ULPtr Temp = new mxflib::UL(IDData->Data);
			TypeID = new mxflib::UUID(Temp);
		}

		ThisType->Type = TypeDef->GetString(MetaDefinitionName_UL, TypeID->GetString());
		ThisType->Detail = TypeDef->GetString(MetaDefinitionDescription_UL, ThisType->Type);
		ThisType->UL = new UL(TypeID);

		if(TypeDef->IsA(TypeDefinitionInteger_UL))
		{
			ThisType->Class = TypeBasic;
			ThisType->Endian = true;
			ThisType->Size = TypeDef->GetInt(Size_UL, 1);

			/* DRAGONS: Nothing here handles isSigned! */
		}
		else if(TypeDef->IsA(TypeDefinitionCharacter_UL))
		{
			ThisType->Class = TypeBasic;
			ThisType->Endian = true;
			
			// FIXME: Non-standard extension to allow non-UTF16 characters
			ThisType->Size = TypeDef->GetInt(Size_UL, 2);

			/* DRAGONS: Nothing here handles isSigned! */
		}
		else if(TypeDef->IsA(TypeDefinitionStrongObjectReference_UL))
		{
			ThisType->Class = TypeInterpretation;
			ThisType->Base = StrongRef_UL.GetString();
			ThisType->RefType = TypeRefStrong;

			MDObjectPtr RefTargetDef = TypeDef[ReferencedType_UL];
			if(RefTargetDef) RefTargetDef = RefTargetDef->GetLink();

			if(!RefTargetDef)
			{
				error("TypeDefinition for Strong Object Reference \"%s\" has no valid base ReferencedType\n", ThisType->Type.c_str());
				ThisType->RefTarget = InterchangeObject_UL.GetString();
			}
			else
			{
				ThisType->RefTarget = RefTargetDef->GetString(MetaDefinitionIdentification_UL);
			}
		}
		else if(TypeDef->IsA(TypeDefinitionWeakObjectReference_UL))
		{
			ThisType->Class = TypeInterpretation;
			ThisType->Base = WeakRef_UL.GetString();

			MDObjectPtr RefTargetDef = TypeDef[WeakReferencedType_UL];
			if(RefTargetDef) RefTargetDef = RefTargetDef->GetLink();

			if(RefTargetDef)
			{
				ThisType->RefType = TypeRefWeak;
				ThisType->RefTarget = RefTargetDef->GetString(MetaDefinitionIdentification_UL);
			}
			else
			{
				ThisType->RefType = TypeRefGlobal;
			}
		}
		else if(TypeDef->IsA(TypeDefinitionFixedArray_UL))
		{
			ThisType->Class = TypeMultiple;

			/* DRAGONS - no way to know this!! */
			ThisType->ArrayClass = ARRAYEXPLICIT;

			ThisType->Base = TypeDef->GetString(FixedArrayElementType_UL);
			ThisType->Size = TypeDef->GetUInt(ElementCount_UL);
		}
		else if(TypeDef->IsA(TypeDefinitionVariableArray_UL))
		{
			ThisType->Class = TypeMultiple;
			
			/* DRAGONS - no way to know this!! */
			ThisType->ArrayClass = ARRAYEXPLICIT;

			MDObjectPtr BaseTypeDef = TypeDef[VariableArrayElementType_UL];
			if(BaseTypeDef) BaseTypeDef = BaseTypeDef->GetLink();

			if(!BaseTypeDef)
			{
				error("TypeDefinition for Variable Array \"%s\" has no valid base type\n", ThisType->Type.c_str());
				ThisType->Class = TypeInterpretation;
				ThisType->Base = "UnknownType";
			}
			else
			{
				ThisType->Base = BaseTypeDef->GetString(MetaDefinitionIdentification_UL);
			}

			ThisType->Size = 0;

		}
		else if(TypeDef->IsA(TypeDefinitionSet_UL))
		{
			ThisType->Class = TypeMultiple;
			
			/* DRAGONS - no way to know this!! */
			ThisType->ArrayClass = ARRAYEXPLICIT;

			MDObjectPtr BaseTypeDef = TypeDef[SetElementType_UL];
			if(BaseTypeDef) BaseTypeDef = BaseTypeDef->GetLink();

			if(!BaseTypeDef)
			{
				error("TypeDefinition for Set \"%s\" has no valid base type\n", ThisType->Type.c_str());
				ThisType->Class = TypeInterpretation;
				ThisType->Base = "UnknownType";
			}
			else
			{
				ThisType->Base = BaseTypeDef->GetString(MetaDefinitionIdentification_UL);
			}

			ThisType->Size = 0;

		}
		else if(TypeDef->IsA(TypeDefinitionString_UL))
		{
			ThisType->Class = TypeMultiple;
			ThisType->ArrayClass = ARRAYSTRING;

			MDObjectPtr BaseTypeDef = TypeDef[StringElementType_UL];
			if(BaseTypeDef) BaseTypeDef = BaseTypeDef->GetLink();

			if(!BaseTypeDef)
			{
				error("TypeDefinition for String \"%s\" has no valid base type\n", ThisType->Type.c_str());
				ThisType->Class = TypeInterpretation;
				//ThisType->Base = "UnknownType";

				// DRAGONS: Hack for strange AAF derived files
				ThisType->Base = "UTF16";
			}
			else
			{
				ThisType->Base = BaseTypeDef->GetString(MetaDefinitionIdentification_UL);
			}

			ThisType->Size = 0;

		}
		else if(TypeDef->IsA(TypeDefinitionRename_UL))
		{
			ThisType->Class = TypeInterpretation;

			// DRAGONS: Sort multiple possible ways this could work
			MDObjectPtr Ptr = TypeDef[RenamedType_UL];
			if(Ptr)
			{
				if(Ptr->GetLink())
				{
					// Weak ref to another definition object
					ThisType->Base = Ptr->GetLink()->GetString(MetaDefinitionIdentification_UL);
				}
				else
				{
					// The type UL of the renamed type
					ThisType->Base = Ptr->GetString();
				}
			}
		}
		else if(TypeDef->IsA(TypeDefinitionStream_UL))
		{
			ThisType->Class = TypeInterpretation;
			ThisType->Base = "RAW";
		}
		else if(TypeDef->IsA(TypeDefinitionRecord_UL))
		{
			ThisType->Class = TypeCompound;
			
			MDObjectPtr MemberTypes = TypeDef[MemberTypes_UL];
			if(!MemberTypes)
			{
				error("TypeDefinition for Record \"%s\" has no list of member types\n", ThisType->Type.c_str());
				ThisType->Class = TypeInterpretation;
				ThisType->Base = "UnknownType";
			}
			else
			{
				MDObjectPtr MemberNames = TypeDef[MemberNames_UL];
				std::list<std::string> NamesList;
				if(MemberNames) NamesList = SplitStringArray(MemberNames);

				if(MemberTypes->size() != NamesList.size())
				{
					error("Mismatch of MemberTypes and MemberNames counts: Types = %d, Names = %d\n", (int)MemberTypes->size(), (int)NamesList.size());
					ThisType->Class = TypeInterpretation;
					ThisType->Base = "UnknownType";
				}
				else
				{
					std::list<std::string>::iterator Names_it = NamesList.begin();
					MDObject::iterator Types_it = MemberTypes->begin();
					while(Names_it != NamesList.end())
					{
						MDObjectPtr SubDef = (*Types_it).second->GetLink();
						if(!SubDef)
						{
							error("%s has sub-item %s which does not reference a valid definition\n", TypeDef->FullName().c_str(), (*Names_it).c_str());
						}
						else
						{
							TypeRecordPtr Child = BuildSubTypeRecord(SubDef);
							Child->Type = (*Names_it);
							Child->Detail = (*Names_it);
							ThisType->Children.push_back(Child);
						}

						Names_it++;
						Types_it++;
					}
				}
			}
		}
		else if(TypeDef->IsA(TypeDefinitionEnumeration_UL))
		{
			ThisType->Class = TypeEnum;
			
			MDObjectPtr ElementType = TypeDef[ElementType_UL];
			MDObjectPtr ElementValues = TypeDef[ElementValues_UL];
			if(ElementType) ElementType = ElementType->GetLink();
			if(!ElementType)
			{
				error("TypeDefinition for Enumaration \"%s\" has no valid base type\n", ThisType->Type.c_str());
				ThisType->Class = TypeInterpretation;
				ThisType->Base = "UnknownType";
			}
			else if(!ElementValues)
			{
				error("TypeDefinition for Enumaration \"%s\" has no list of element values\n", ThisType->Type.c_str());
			}
			else
			{
				ThisType->Base = ElementType->GetString(MetaDefinitionIdentification_UL);

				MDObjectPtr ElementNames = TypeDef[ElementNames_UL];
				std::list<std::string> NamesList;
				if(ElementNames) NamesList = SplitStringArray(ElementNames);

				if(ElementValues->Value->size() != NamesList.size())
				{
					error("Mismatch of ElementValues and ElementNames in enumerated type %s counts: Values = %d, Names = %d\n", ThisType->Type.c_str(), (int)ElementValues->Value->size(), (int)NamesList.size());
				}
				else
				{
					std::list<std::string>::iterator Names_it = NamesList.begin();
					int Index = 0;
					while(Names_it != NamesList.end())
					{
						TypeRecordPtr Child = new TypeRecord;

						/* Build a randmom type ID (metadict does not give IDs to sub-items) */
						mxflib::UUIDPtr Temp = new mxflib::UUID;
						ULPtr TypeID = new UL(Temp);

						Child->Class = TypeSub;
						Child->UL = TypeID;
						Child->Type = (*Names_it);
						Child->Detail = (*Names_it);
						Child->Value = ElementValues[Index]->GetString();

						ThisType->Children.push_back(Child);

						Names_it++;
						Index++;
					}
				}
			}
		}
		else
		{
			debug("Skipping %s\n", ThisType->Type.c_str());
			return NULL;
		}

		return ThisType;
	}
}


//! Load classes and types from a Metadictionary object
/*! At the point where this function is called, you need to have all the component parts loaded and
 *  all the strong references within the metadictionary need to be satisfied
 */
bool mxflib::LoadMetadictionary(MDObjectPtr &Meta, SymbolSpacePtr &SymSpace)
{
	bool Ret = true;

	// Is this a KLV Encoded Extension Syntax extension rather than a Metadictionary?
	bool KXSMetadict = Meta->IsA(ExtensionScheme_UL);

	/* First we load any types */
	MDObjectPtr TypeDefList;

	// FIXME: Use UL when available
	if(KXSMetadict) TypeDefList = Meta["MetaDefinitions"];
	else TypeDefList = Meta[TypeDefinitions_UL];
	
	if(!TypeDefList)
	{
		error("Unable to load types from %s at %s, no TypeDefinitions list found\n", Meta->FullName().c_str(), Meta->GetSourceLocation().c_str());
		Ret = false;
	}
	else
	{
		TypeRecordList TypeList;

		MDObject::iterator it = TypeDefList->begin();
		while(it != TypeDefList->end())
		{
			MDObjectPtr TypeDef = (*it).second->GetLink();
			if(!TypeDef)
			{
				error("Missing target for type definition strong reference at 0x%s\n", Int64toHexString((*it).second->GetLocation(),8).c_str());
			}
			else
			{
				if((!KXSMetadict) || TypeDef->IsA(TypeDefinition_UL))
				{
					TypeRecordPtr ThisType = BuildTypeRecord(TypeDef);
					if(ThisType) TypeList.push_back(ThisType);
				}
			}

			it++;
		}

		// Load the types and update the running status
		if(!LoadTypes(TypeList, SymSpace)) Ret = false;
	}



	/* WHAT DO WE DO ABOUT TRAITS? */
	/* =========================== */

	/* Next load the classes */
	MDObjectPtr ClassDefList;

	// FIXME: Use UL when available
	if(KXSMetadict) ClassDefList = Meta["MetaDefinitions"];
	else ClassDefList = Meta[ClassDefinitions_UL];

	if(!ClassDefList)
	{
		error("Unable to load classes from %s at %s, no ClassDefinitions list found\n", Meta->FullName().c_str(), Meta->GetSourceLocation().c_str());
		Ret = false;
	}
	else
	{
		ClassRecordList ClassList;

		MDObject::iterator it = ClassDefList->begin();
		while(it != ClassDefList->end())
		{
			MDObjectPtr ClassDef = (*it).second->GetLink();
			if(!ClassDef)
			{
				error("Missing target for class definition strong reference at 0x%s\n", Int64toHexString((*it).second->GetLocation(),8).c_str());
			}
			else
			{
				if((!KXSMetadict) || ClassDef->IsA(ClassDefinition_UL))
				{
					ClassRecordPtr ThisClass = new ClassRecord;

					/* Locate the class ID */
					UUIDPtr ClassID;
					DataChunkPtr IDData;
					MDObjectPtr ClassIDObject = ClassDef[MetaDefinitionIdentification_UL];
					if(ClassIDObject) IDData = ClassIDObject->PutData();
					if((!ClassIDObject) || (IDData->Size < 16))
					{
						error("No valid Class ID for %s at %s\n", ClassDef->FullName().c_str(), ClassDef->GetSourceLocation().c_str());
						
						// Make a random UUID to use instead
						ClassID = new mxflib::UUID;
					}
					else
					{
						ClassID = new mxflib::UUID(IDData->Data);
					}

					ThisClass->Class = ClassSet;
					ThisClass->Name = ClassDef->GetString(MetaDefinitionName_UL, ClassID->GetString());
					ThisClass->Detail = ClassDef->GetString(MetaDefinitionDescription_UL, ThisClass->Name);
					ThisClass->Usage = ClassUsageOptional;

					MDObjectPtr ParentClass = ClassDef[ParentClass_UL];
					if(ParentClass) ParentClass = ParentClass->GetLink();
					if(!ParentClass)
					{
						error("No parent class specified for %s of %s at %s\n", ClassDef->FullName().c_str(), ClassDef->GetString(MetaDefinitionName_UL, "unnamed class").c_str(), ClassDef->GetSourceLocation().c_str());
					}
					else
					{
						if(ParentClass == ClassDef)
						{
							/* This is the base definition for InterchangeObject */
							debug("Found a self referenced ClassDef for class %s\n", ClassDef->GetString(MetaDefinitionName_UL, "unnamed class").c_str());
						}
						else
						{
							ThisClass->Base = ParentClass->GetString(MetaDefinitionIdentification_UL);
							if(ThisClass->Base.empty())
							{
								// We couldn't find the base class ID, so to enable us to be derived we must add one!
								mxflib::UUIDPtr Temp;
								ThisClass->Base = Temp->GetString();
								ParentClass->SetString(MetaDefinitionIdentification_UL, ThisClass->Base);
							}
						}
					}

					// We only support 2-byte tag and 2-byte length local sets here!
					ThisClass->MinSize = 2;
					ThisClass->MinSize = 2;

					ThisClass->UL = new UL(ClassID->GetValue());
					ThisClass->HasDefault = false;
					ThisClass->HasDValue = false;
					ThisClass->RefType = ClassRefUndefined;
					ThisClass->ExtendSubs = true;

					/* Now we need to add any properties */
					
					MDObjectPtr PropDefList = ClassDef[Properties_UL];
					
					// DRAGONS: It is perfectly valid to have no properties defined - they could all be inherited (or this could be an abstract class)
					if(PropDefList)
					{
						MDObject::iterator Prop_it = PropDefList->begin();
						while(Prop_it != PropDefList->end())
						{
							MDObjectPtr PropertyDef = (*Prop_it).second->GetLink();
							if(!PropertyDef)
							{
								error("Missing target for class definition strong reference at 0x%s\n", Int64toHexString((*Prop_it).second->GetLocation(),8).c_str());
							}
							else
							{
								ClassRecordPtr ThisProperty = new ClassRecord;

								/* Locate the property ID */
								UUIDPtr PropertyID;
								DataChunkPtr IDData;
								MDObjectPtr PropertyIDObject = PropertyDef[MetaDefinitionIdentification_UL];
								if(PropertyIDObject) IDData = PropertyIDObject->PutData();
								if((!PropertyIDObject) || (IDData->Size < 16))
								{
									error("No valid Property ID for %s at %s\n", PropertyDef->FullName().c_str(), PropertyDef->GetSourceLocation().c_str());
									
									// Make a random UUID to use instead
									PropertyID = new mxflib::UUID;
								}
								else
								{
									PropertyID = new mxflib::UUID(IDData->Data);
								}

								ThisProperty->Class = ClassItem;
								ThisProperty->MinSize = 0;
								ThisProperty->MaxSize = 0;
								ThisProperty->Name = PropertyDef->GetString(MetaDefinitionName_UL, PropertyID->GetString());
								ThisProperty->Detail = PropertyDef->GetString(MetaDefinitionDescription_UL, ThisProperty->Name);

								// DRAGONS: IsOptional is a required property - if it's missing we assume optional (can't hurt I guess!)
								ThisProperty->Usage = PropertyDef->GetInt(IsOptional_UL, 1) != 0 ? ClassUsageOptional : ClassUsageRequired;
								ThisProperty->Base = PropertyDef->GetString(PropertyType_UL);

								// FIXME: We shouldn't need to do this!!
								/* End swap the UL! */
								ThisProperty->UL = new UL(PropertyID->GetValue());
//								ThisProperty->UL = new UL(PropertyID);

								if(KXSMetadict)
									ThisProperty->Tag = 0;		// FIXME: Do we need to set this?
								else
									ThisProperty->Tag = PropertyDef->GetUInt(LocalIdentification_UL);

								ThisProperty->HasDefault = false;
								ThisProperty->HasDValue = false;
								ThisProperty->RefType = PropertyDef->GetInt(IsUniqueIdentifier_UL, 0) != 0 ? ClassRefTarget : ClassRefUndefined;
								ThisProperty->ExtendSubs = true;

								ThisClass->Children.push_back(ThisProperty);
							}

							Prop_it++;
						}
					}

					// Now add the class to the build list
					ClassList.push_back(ThisClass);
				}			
			}

			it++;
		}

		// Load the classes and update the running status
		if(!LoadClasses(ClassList, SymSpace)) Ret = false;
	}

	return Ret;
}


namespace
{
	//! Add any base classes for a given class to a map of classes - without duplicating
	/*! TODO: Make this a private member function?
	*/
	void AddBaseClassesToMap(MDOTypeULMap &Classes, const MDOType *ThisClass)
	{
		MDOTypePtr BaseType = ThisClass->GetBase();

		if(BaseType)
		{
			const ULPtr &BaseUL = BaseType->GetTypeUL();

			if(BaseUL)
			{
				MDOTypeULMap::iterator it = Classes.find(BaseUL);

				// Add only if we don't already know this class			
				if(it == Classes.end())
				{
					Classes[BaseUL] = BaseType;
					AddBaseClassesToMap(Classes, BaseType);
				}
			}
		}
	}
}


namespace
{
	/* Definitions used by the BuildTypesDefs function */
	/***************************************************/

	// Enumeration to keep track of what kind of type definition we are building, without needint to do 16-byte UL compares
	enum DefinitionType
	{ 
		DefinitionInteger,
		DefinitionCharacter,
		DefinitionRename,
		DefinitionVariableArray,
		DefinitionSet,
		DefinitionFixedArray,
		DefinitionString,
		DefinitionRecord,
		DefinitionStream,
		DefinitionEnum,
		DefinitionExtEnum,
		DefinitionStrongRef,
		DefinitionWeakRef,
		DefinitionIndirect,
		DefinitionOpaque,
		DefinitionError				//! Used to flag that we cannot build a TypeDefinition for this type
	};

	//! List of classes that are required for the metadictionary - used to add these to the metadictionary which has not got them otherwise!
	//  DRAGONS: It is vitally important that this list remains in step with the bit values defined below that index the entries
	UL const *MetadictClasses[] = 
	{
		&DataDefinition_UL,								// 0x00000001
		&ContainerDefinition_UL,						// 0x00000002
		&CodecDefinition_UL,							// 0x00000004
		&Root_UL,										// 0x00000008
		&ExtensionScheme_UL,							// 0x00000010
		&ClassDefinition_UL,							// 0x00000020
		&PropertyDefinition_UL,							// 0x00000040
		&PropertyWrapperDefinition_UL,					// 0x00000080
		&TypeDefinitionInteger_UL,						// 0x00000100
		&TypeDefinitionCharacter_UL,					// 0x00000200
		&TypeDefinitionString_UL,						// 0x00000400
		&TypeDefinitionStream_UL,						// 0x00000800
		&TypeDefinitionRecord_UL,						// 0x00001000
		&TypeDefinitionEnumeration_UL,					// 0x00002000
		&TypeDefinitionExtendibleEnumeration_UL,		// 0x00004000
		&ExtendibleEnumerationElement_UL,				// 0x00008000
		&TypeDefinitionRename_UL,						// 0x00010000
		&TypeDefinitionIndirect_UL,						// 0x00020000
		&TypeDefinitionOpaque_UL,						// 0x00040000
		&TypeDefinitionStrongObjectReference_UL,		// 0x00080000
		&TypeDefinitionWeakObjectReference_UL,			// 0x00100000
		&TypeDefinitionFixedArray_UL,					// 0x00200000
		&TypeDefinitionVariableArray_UL,				// 0x00400000
		&TypeDefinitionSet_UL,							// 0x00800000
		&MetaDictionary_UL,								// 0x01000000
		NULL
	};

	//! Bitmap of classes that need to be added to the metadictionary - because we are using them in the metadictionary
	int DefineMetadictClasses = 0;

	// DRAGONS: It is vitally important that these bit values remain in step with the list defined above, into which they act as index values
	const int Def_DataDefinition = 0x00000001;
	const int Def_ContainerDefinition = 0x00000002;
	const int Def_CodecDefinition = 0x00000004;
	const int Def_Root = 0x00000008;
	const int Def_ExtensionScheme = 0x00000010;
	const int Def_ClassDefinition = 0x00000020;
	const int Def_PropertyDefinition = 0x00000040;
	const int Def_PropertyWrapperDefinition = 0x00000080;
	const int Def_TypeDefinitionInteger = 0x00000100;
	const int Def_TypeDefinitionCharacter = 0x00000200;
	const int Def_TypeDefinitionString = 0x00000400;
	const int Def_TypeDefinitionStream = 0x00000800;
	const int Def_TypeDefinitionRecord = 0x00001000;
	const int Def_TypeDefinitionEnumeration = 0x00002000;
	const int Def_TypeDefinitionExtendibleEnumeration = 0x00004000;
	const int Def_ExtendibleEnumerationElement = 0x00008000;
	const int Def_TypeDefinitionRename = 0x00010000;
	const int Def_TypeDefinitionIndirect = 0x00020000;
	const int Def_TypeDefinitionOpaque = 0x00040000;
	const int Def_TypeDefinitionStrongObjectReference = 0x00080000;
	const int Def_TypeDefinitionWeakObjectReference = 0x00100000;
	const int Def_TypeDefinitionFixedArray = 0x00200000;
	const int Def_TypeDefinitionVariableArray = 0x00400000;
	const int Def_TypeDefinitionSet = 0x00800000;
	const int Def_MetaDictionary = 0x01000000;
}


namespace
{
	//! Decide what kind of TypeDefinition will be required to define a given type
	DefinitionType SelectDefinitionType(MDType const *Type)
	{
		switch(Type->GetClass())
		{
			case BASIC:
				// DRAGONS: Can't deal with floats!!
				if(Type->Name().find("loat") != std::string::npos)
				{
					error("Type %s apears to be a floating point type - not supported!\n", Type->Name().c_str());
					return DefinitionError;
				}

				if(Type->IsCharacter())	return DefinitionCharacter;

				return DefinitionInteger;

			case INTERPRETATION:
			{
				// If the interpretation makes the item a strong or weak reference, make sure it becomes a strong ref
				TypeRef RefType = Type->EffectiveRefType();
				if(RefType == TypeRefStrong)
				{
					// We only do this if an actual strong reference, not an array of references or something more complex
					if(Type->EffectiveSize() == 16)
					{
						return DefinitionStrongRef;
					}
					else
					{
						// Not an actual (single) strong reference
						// TODO: Probably need to add a new layer to make this work?
						if(Type->GetRefType() != TypeRefStrong)
						{
							error("Interpretation type %s adds RefType TypeRefStrong but is not a single UUID - this is not supported\n", Type->Name().c_str());
							return DefinitionError;
						}
					}
				}
				else if(IsRefSource(RefType))
				{
					// We only do if this is an actual weak or global reference, not an array of references or something more complex
					if(Type->EffectiveSize() == 16)
					{
						return DefinitionWeakRef;
					}
					else
					{
						// Not an actual (single) weak / meta / global reference
						// TODO: Probably need to add a new layer to make this work?
						if(Type->GetRefType() != TypeRefStrong)
						{
							error("Interpretation type %s adds a non-strong referenced RefType but is not a single UUID - this is not supported\n", Type->Name().c_str());
						}
					}
				}

				// If we haven't just detected this is a reference type, it will be a rename or character
				if(Type->IsCharacter()) return DefinitionCharacter;
				else return DefinitionRename;
			}
			case TYPEARRAY:
				if(Type->GetArrayClass() == ARRAYSTRING)
				{
					return DefinitionString;
				}
				else
				{
					if(Type->GetSize() == 0)
					{
						return DefinitionVariableArray;
					}
					else
					{
						return DefinitionFixedArray;
					}
				}

			case COMPOUND:
				return DefinitionRecord;

			case ENUM:
			{
				int BaseSize = 0;
				MDTypeParent BaseType = Type->EffectiveBase();
				if(BaseType) BaseSize = BaseType->EffectiveSize();

				// We assume that any 16-byte enum is an ext enum as 16-byte ints would be silly!
				if(BaseSize == 16) return DefinitionExtEnum;
				else return DefinitionEnum;
			}
			default:
				error("No idea how to build TypeDefinition object for type %s - skipping\n", Type->Name().c_str());
				return DefinitionError;
		}
	}


	//! Structure holding info used when building a metadictionary
	struct  MetaDictInfo : public RefCount<MetaDictInfo>
	{
	public:
		MDObjectList &MetaList;						//!< The list of objects in the current file, used when describing dark metadata items
		MDObjectMap Classes;						//!< Map of classes by UL that need to be defined
		MDObjectMap Properties;						//!< Map of properties by UL that need to be defined
		MDTypeULMap TypeMap;						//!< Map of MDTypes by UL for each type used by properties in the metadictionary
		MDObjectPtr MetaDict;						//!< The metadictionary which is currently being built
		MDObjectMap ClassMap;						//!< A map of class definition objects by class UL

		PrimerPtr MetaPrimer;						//!< The primer in use for building this metadictionary (required if we need to add new dynamic tags)

		std::list<ULPtr> TypeOrder;					//!< AVMETA: Order in which to add types when writing metadictionary

		// Need to have a construnctor as we contain a reference
		MetaDictInfo(MDObjectList &MetaList, PrimerPtr Primer = NULL) : MetaList(MetaList), MetaPrimer(Primer) {}


		//! Build lists of all classes and types to be added to a metadictionary to describe a given list of metadata trees
		void BuildMetaLists(void)
		{
			// Sanity check - if nothing supplied, we don't build anything
			if(MetaList.empty()) return;

			// Build full metadata list
			if(Feature(FeatureUsedMetadict))
			{
				MDObjectList::iterator it = MetaList.begin();
				while(it != MetaList.end())
				{
					AddClassOrProperty(*it);

					it++;
				}
			}
			else if(Feature(FeatureSaveMetadict))
			{
				MDObjectList::iterator it = MetaList.begin();
				while(it != MetaList.end())
				{
					if(!(*it)->IsBaseline()) AddClassOrProperty(*it);

					it++;
				}
			}
			else
			{
				error("Unknown metadictionary type\n");
			}

			/* Now we need to add the definitions required by the metadictionary itself by walking the bitmask and definition UL list */

			// If we have anything to define, we will need a basic metadictionary structure
			if(DefineMetadictClasses != 0) 
			{
				if(Feature(FeatureKXSMetadict))
					DefineMetadictClasses |= Def_Root + Def_ExtensionScheme + Def_ClassDefinition;
				else
					DefineMetadictClasses |= Def_MetaDictionary + Def_ClassDefinition;
			}

			int OriginalDefineMetadictClasses = DefineMetadictClasses;
			while(DefineMetadictClasses != 0)
			{
				int Mask = 1;
				UL const **pClassUL = MetadictClasses;
				while(*pClassUL)
				{
					if(DefineMetadictClasses & Mask) 
					{
						if(Classes.find(**pClassUL) == Classes.end())
						{
							MDOType *Class = MDOType::Find(**pClassUL);
							if(Class)
							{
								if(Feature(FeatureUsedMetadict) || (!(Class->IsBaseline())))
								{
									AddClass(Class);
								}
							}
						}
					}
					
					Mask <<= 1;
					pClassUL++;
				}

				// Remove all those classes that we originally knew were needed
				// If this does not leave zero, we found something else that we needed in the process of adding the metadict classes, so we go around again
				DefineMetadictClasses -= OriginalDefineMetadictClasses;
			}

		}



		//! Determine if a given object is a class (i.e. a set or pack) or a property and add to the correct list
		void AddClassOrProperty(MDObject *ThisItem)
		{
			if(!ThisItem) return;

			// DRAGONS: We take the UL from the instance, not the type as this may have been overridden (such as when parse-dark bases an object on Unknown)
			const ULPtr &ItemUL = ThisItem->GetUL();

			// This is a value, rather than a container, so it is a property
			if(ThisItem->GetValueType())
			{
// FIXME: Check features to only selectively add properties here??
				// Add the property to the list, if it is not already there
				if(Properties.find(ItemUL) == Properties.end()) AddProperty(ThisItem);
			}
			else
			{
				// It must be a class, add it if we haven't already done so
				if(Classes.find(ItemUL) == Classes.end())
				{
					AddClass(ThisItem);
					if(!ThisItem->empty())
					{
						if(Feature(FeatureUsedMetadict))
						{
							MDObject::const_iterator it = ThisItem->begin();
							while(it != ThisItem->end())
							{
								AddClassOrProperty((*it).second);
								it++;
							}
						}
						else if(Feature(FeatureSaveMetadict))
						{
							MDObject::const_iterator it = ThisItem->begin();
							while(it != ThisItem->end())
							{
								if(!(*it).second->IsBaseline()) AddClassOrProperty((*it).second);
								it++;
							}
						}
					}
				}
			}
		}

		//! Add a given class to the classes list, by instance
		void AddClass(MDObject *ThisItem)
		{
			if(!ThisItem) return;
			Classes[*(ThisItem->GetUL())] = ThisItem;
			
			const MDOType *ThisClass = ThisItem->GetType();
			if(ThisClass && ThisClass->GetBase())
			{
				if(Feature(FeatureUsedMetadict) || (!(ThisClass->GetBase()->IsBaseline())))
				{
					if(Classes.find(ThisClass->GetBase()->GetUL()) == Classes.end())
					{
						AddClass(ThisClass->GetBase());
					}
				}
			}
		}

		//! Add a given class to the classes list, by class only
		void AddClass(const MDOType *ThisClass)
		{
			if(!ThisClass) return;
			Classes[*(ThisClass->GetUL())] = NULL;

			if(ThisClass->GetBase())
			{
				if(Feature(FeatureUsedMetadict) || (!(ThisClass->GetBase()->IsBaseline())))
				{
					if(Classes.find(ThisClass->GetBase()->GetUL()) == Classes.end())
					{
						AddClass(ThisClass->GetBase());
					}
				}
			}
		}

		//! Add a given property to the properties list
		void AddProperty(MDObject *ThisItem)
		{
			if(!ThisItem) return;
			Properties[*(ThisItem->GetUL())] = ThisItem;

			// We will be using PropertyDefinitions
			DefineMetadictClasses |= Def_PropertyDefinition;

			if(true) // Allow for future expansion...
			{
				if(Feature(FeatureUsedMetadict) || (!ThisItem->GetValueType()->IsBaseline()))
				{
					// Make sure that the type is listed for this property
					if(TypeMap.find(ThisItem->GetTypeUL()) == TypeMap.end())
					{
						AddType(ThisItem->GetValueType());
					}
				}
			}
		}

		//! Add a given type to the types list
		void AddType(MDType *ThisItem)
		{
			if(!ThisItem) return;
			TypeMap[*(ThisItem->GetTypeUL())] = ThisItem;

			// See if this type is based on another type - if so, we may need to add that too
			if(ThisItem->GetBase())
			{
				if(Feature(FeatureUsedMetadict) || (!ThisItem->GetBase()->IsBaseline()))
				{
					if((ThisItem->GetBase()->GetTypeUL()) && (TypeMap.find(ThisItem->GetBase()->GetTypeUL()) == TypeMap.end()))
					{
						AddType(ThisItem->GetBase());
					}
				}
			}

			// Also do the same for members of compounds
			if(ThisItem->GetClass() == COMPOUND)
			{
				MDType::iterator it = ThisItem->begin();
				while(it != ThisItem->end())
				{
					if(((*it).second->GetTypeUL()) && (TypeMap.find((*it).second->GetTypeUL()) == TypeMap.end()))
					{
						AddType((*it).second);
					}
					it++;
				}
			}

			// If this type is a reference source, check that the target class is added
			if(IsRefSource(ThisItem->GetRefType()))
			{
				// Establish the target class - by name lookup if necessary
				MDOType *Target = ThisItem->GetRefTarget();
				if(!Target)
				{
					Target = MDOType::Find(ThisItem->GetRefTargetName());
				}

				if(Target && (Target->GetUL()) && (Classes.find(Target->GetUL()) == Classes.end()))
				{
					AddClass(Target);
				}
			}

			//! Record that we will need a type def for this type
			DefinitionType ThisDefinitionType = SelectDefinitionType(ThisItem);
			switch(ThisDefinitionType)
			{
				case DefinitionInteger:			DefineMetadictClasses |= Def_TypeDefinitionInteger; break;
				case DefinitionCharacter:		DefineMetadictClasses |= Def_TypeDefinitionCharacter; break;
				case DefinitionRename:			DefineMetadictClasses |= Def_TypeDefinitionRename; break;
				case DefinitionVariableArray:	DefineMetadictClasses |= Def_TypeDefinitionVariableArray; break;
				case DefinitionSet:				DefineMetadictClasses |= Def_TypeDefinitionSet; break;
				case DefinitionFixedArray:		DefineMetadictClasses |= Def_TypeDefinitionFixedArray; break;
				case DefinitionString:			DefineMetadictClasses |= Def_TypeDefinitionString; break;
				case DefinitionRecord:			DefineMetadictClasses |= Def_TypeDefinitionRecord; break;
				case DefinitionEnum:			DefineMetadictClasses |= Def_TypeDefinitionEnumeration; break;
				case DefinitionExtEnum:			DefineMetadictClasses |= Def_TypeDefinitionExtendibleEnumeration; break;
				case DefinitionStrongRef:		DefineMetadictClasses |= Def_TypeDefinitionStrongObjectReference; break;
				case DefinitionWeakRef:			DefineMetadictClasses |= Def_TypeDefinitionWeakObjectReference; break;

				default:
					error("No idea how to build TypeDefinition object kind %d for type %s\n", ThisDefinitionType, ThisItem->Name().c_str());
					break;
			}
		}
	};

	//! Smart pointer to a MetaDictInfo object
	typedef SmartPtr<MetaDictInfo> MetaDictInfoPtr;

}


namespace
{
	//! Build a property definition for a given property and add it to the Properties list
	void AddPropertyDefinition(MDObjectPtr &Properties, std::string Name, std::string Description, const ULPtr &Identification, const ULPtr &Type, bool IsOptional, UInt16 LocalKey, bool IsUID)
	{
		/* Build the property definition */
		MDObjectPtr ThisDefinition = new MDObject(PropertyDefinition_UL);
		if(!ThisDefinition)
		{
			error("Unable to build PropertyDefinition object - Metadictionary will be incomplete\n");
			return;
		}

		ThisDefinition->SetString(MetaDefinitionIdentification_UL, Identification->GetString());
		ThisDefinition->SetString(MetaDefinitionName_UL, Name);

		ThisDefinition->SetString(PropertyType_UL, Type->GetString());
		ThisDefinition->SetUInt(IsOptional_UL, IsOptional ? 1 : 0);

		// No local identification in KXS (the primer sorts that)
		if(!Feature(FeatureKXSMetadict)) ThisDefinition->SetUInt(LocalIdentification_UL, LocalKey);

		if(IsUID) ThisDefinition->SetUInt(IsUniqueIdentifier_UL, 1);

		// Add this property definition to the list of properties for this class
		MDObjectPtr Ptr = Properties->AddChild();
		if(Ptr) Ptr->MakeLink(ThisDefinition);
		else
		{
			error("Unable to add entry in %s\n", Properties->FullName().c_str());
		}
	}
}


namespace
{
	//! Build a property definition for a given property and add it to the Properties list
	void AddPropertyDefinition(MDOTypePtr ThisProperty, MDObjectPtr Properties, MetaDictInfo &Info)
	{
		// Set the property name now so that if it is a proxy we use the proxy's name rather that the "_Item" version
		std::string PropertyName = ThisProperty->Name();

		// Locate the type UL
		ULPtr ValueTypeUL;
		MDTypePtr ValueType = ThisProperty->GetValueType();
		if(ValueType) ValueTypeUL = ValueType->GetTypeUL();
		if(!ValueTypeUL)
		{
			// If the property has no type UL defined we set a random UUID instead so the format of the definition is valid
			error("No TypeUL for property %s\n", PropertyName.c_str());
			ValueTypeUL = RandomUL();
		}
		else debug("Property %s has TypeUL %s\n", PropertyName.c_str(), ValueTypeUL->GetString().c_str());

		UInt16 LocalKey = 0;
		if(ThisProperty->GetKey().Size == 2)
		{
			LocalKey = GetU16(ThisProperty->GetKey().Data);
		}
		

		// Add this Property
		AddPropertyDefinition( Properties, PropertyName, ThisProperty->GetDetail(), ThisProperty->GetUL(), ValueTypeUL, 
							   (ThisProperty->GetUse() == ClassUsageOptional) || (ThisProperty->GetUse() == ClassUsageDecoderRequired), 
							   LocalKey, (ThisProperty->GetRefType() == ClassRefTarget));

	}
}


namespace
{
	//! Build a property definition for a given object and add it to the Properties list
	void AddPropertyDefinition(MDObjectPtr ThisObject, MDObjectPtr Properties, MetaDictInfo &Info)
	{
		// Set the property name now so that if it is a proxy we use the proxy's name rather that the "_Item" version
		// Locate the type UL
		ULPtr ValueTypeUL;
		MDTypePtr ValueType = ThisObject->GetType()->GetValueType();
		if(ValueType) ValueTypeUL = ValueType->GetTypeUL();
		if(!ValueTypeUL)
		{
			// Is the property has no type UL defined we set a random UUID instead so the format of the definition is valid
			error("No TypeUL for property %s\n", ThisObject->Name().c_str());
			ValueTypeUL = RandomUL();
		}

		UInt16 LocalKey = 0;
		LocalKey = static_cast<UInt16>(ThisObject->GetTag());

		// Add this Property - note that it is forced to be optional as it is not in the dictionary and so must be an extension!
		AddPropertyDefinition( Properties, ThisObject->Name(), ThisObject->GetDetail(), ThisObject->GetUL(), ValueTypeUL, 
							   true, LocalKey, (ThisObject->GetRefType() == ClassRefTarget));

	}
}


namespace
{
	//! Add all properties that exist in a specified class, but not its base class, to the classes list
	/*! \param ClassUL The UL of the class whose properties are being added
	 *  \param Class A smart pointer to the MDOType of this class
	 *  \param ClassDefinition A smart pointer to the class definition object to which to add property definitions
	 *  \param MetaList A list of all metadata items to check for extension properties (properties of a object not listed in the dictionary as a property of that class)
	 *  \param TypeMap A map of all types in use by type UL, updated for this class
	 */
	void AddProperties(const UL &ClassUL, const MDOType *Class, MDObjectPtr &ClassDefinition, MetaDictInfo &Info)
	{
		// Ensure that this class definition has a Properties batch, and record a pointer to it
		MDObjectPtr Properties = ClassDefinition[Properties_UL];
		if(!Properties)
		{
			Properties = ClassDefinition->AddChild(Properties_UL);
		}

		const MDOTypeParent &Parent = Class->GetBase();

		if(true) // For future expansion...
		{
			MDOType::const_iterator it = Class->begin();
			while(it != Class->end())
			{
				if(!((*it).second->GetTypeUL()))
				{
					error("Property %s has no type UL\n", (*it).second->FullName().c_str());

					// Skip the part of the loop where we add this property
					it++;
					continue;
				}

				// Check if this is a derived property
				if(Parent)
				{
					if(Parent->HasA((*it).second->GetTypeUL()))
					{
						// Skip the part of the loop where we add this property
						it++;
						continue;
					}
				}

				// Add the property
				AddPropertyDefinition((*it).second, Properties, Info);

				it++;
			}
		}

		/* Now we need to trawl for any properties that have been added to the class in this file,
		 * but are not declared as being members in the dictionary - these need to be treated as extensions
		 */
		MDObjectList::iterator Object_it = Info.MetaList.begin();
		while(Object_it != Info.MetaList.end())
		{
			// Attempt to match the UL of each object with this class' UL
			if(*(*Object_it)->GetUL() == ClassUL)
			{
				MDObject::iterator Child_it = (*Object_it)->begin();
				while(Child_it != (*Object_it)->end())
				{
					// Check if each property is a known one for this class, if not we add it as an extension
					if(!Class->HasA((*Child_it).second->GetUL()))
					{
						// Check if this extension property has already been defined
						MDObject::iterator Properties_it = Properties->begin();
						while(Properties_it != Properties->end())
						{
							MDObjectPtr Ptr = (*Properties_it).second->GetLink();
							if(Ptr) Ptr = Ptr[MetaDefinitionIdentification_UL];
							if(Ptr && (Ptr->GetString() == (*Child_it).second->GetUL()->GetString()))
							{
								break;
							}

							Properties_it++;
						}

						// Not already known if we hit the end of the scan
						if(Properties_it == Properties->end())
						{
							AddPropertyDefinition((*Child_it).second, Properties, Info);
						}
					}
					Child_it++;
				}
			}

			Object_it++;
		}
	}

	//! Add a given property to the metadictionary
	void AddProperty(MDObjectPtr &Property, MetaDictInfo &Info)
	{
		// The UL of the class that contains this property
		ULPtr ClassUL;

		// We check the definition of this property rather than the property's actual parent as that may be a derived class
		MDOType const *Type = Property->GetType();
		
		if((!Type) || ((*(Type->GetUL())) != (*(Property->GetUL()))))
		{
			// Unable to find type, or we don't have the same UL as the type (such as when a new type is derived from "Unknown")
			// so just look at our own parent and use that instead
			const MDObject *Class = Property->GetParent();
			if(!Class)
			{
				error("Tried to add property %s to metadictionary, but no parent class defined\n", Property->FullName().c_str());
				return;
			}

			ClassUL = Class->GetUL();
		}
		else
		{
			if(Type->GetParent()) ClassUL =  Type->GetParent()->GetUL();
		}

		// Try and locate the class that holds the original property definition
		MDObjectMap::iterator Class_it = ClassUL ? Info.ClassMap.find(*ClassUL) : Info.ClassMap.end();

		// It is possible that we don't have a class definition for the class that defines the original property
		// This can happen in a few special cases, such as where single-inheritance is broken (there is an example at
		// the time of writing caused by InstanceUID being defined in both InterchangeObject and AbstractObject for
		// AAF SDK compatibility). In cases such as this we add the property to the earliest class we have for it
		if(Class_it == Info.ClassMap.end())
		{
			MDOType const *Base = Property->GetParent() ? Property->GetParent()->GetType() : NULL;
			for(;;)
			{
				// Stop if we have reached the end of the derivation tree
				if(!Base) break;

				// Stop if the base class does not have one of these properties
				if(!Base->HasA(Property->GetUL())) break;

				// ...or if we don't have a UL for the base class
				if(!Base->GetUL()) break;

				// ...or if we don't have a class definition for the base class
				MDObjectMap::iterator it = Info.ClassMap.find(Base->GetUL());
				if(it == Info.ClassMap.end()) break;

				// All ok so far, move on up the tree (recording this class UL as the best bet so far)
				Class_it = it;
				Base = Base->GetBase();
			}
		}

		// All attempts failed - exit with an error
		if(Class_it == Info.ClassMap.end())
		{
			error("Tried to add property %s to metadictionary, but could not find ClassDefinition for parent %s\n", Property->FullName().c_str(), ClassUL->GetString().c_str());
			return;
		}

		// Make a handy reference to the class definition object
		MDObjectPtr &ClassDefinition = (*Class_it).second;

		// Ensure that this class definition has a Properties batch, and record a pointer to it
		MDObjectPtr Properties = ClassDefinition[Properties_UL];
		if(!Properties)
		{
			Properties = ClassDefinition->AddChild(Properties_UL);
		}

		// Add the property
		AddPropertyDefinition(Property, Properties, Info);
	}
}


namespace
{
	//! Build class definitions for each specified class and add to a given metadictionary (also add all property definitions for the classes)
	/*! \return true if all went well, false if there was a fatal error
	 */
	bool BuildClassDefs(MetaDictInfo &Info)
	{
		// Add the class definitions batch
		MDObjectPtr ClassDefinitions;

		if(Feature(FeatureKXSMetadict))
			ClassDefinitions = Info.MetaDict["MetaDefinitions"];		// FIXME: Use UL when defined
		else
			ClassDefinitions = Info.MetaDict->AddChild(ClassDefinitions_UL);

		if(!ClassDefinitions) return false;

		/* For each class, build a class definition and add it to the list of classes in the metadictionary */
		MDObjectMap::iterator Map_it = Info.Classes.begin();
		while(Map_it != Info.Classes.end())
		{
			// Skip any classes that we have already defined
			if(Info.ClassMap.find((*Map_it).first) != Info.ClassMap.end())
			{
				Map_it++;
				continue;
			}

			MDObjectPtr ThisDefinition = new MDObject(ClassDefinition_UL);
			if(!ThisDefinition)
			{
				error("Unable to build ClassDefinition object\n");
				return false;
			}

			ThisDefinition->SetString(MetaDefinitionIdentification_UL, (*Map_it).first.GetString());

			// Are we building from an instance, or an MDOType?
			if((*Map_it).second)
			{
				// Build from Instance
				ThisDefinition->SetString(MetaDefinitionName_UL, (*Map_it).second->Name());
				ThisDefinition->SetString(MetaDefinitionDescription_UL, (*Map_it).second->GetDetail());
			}
			else
			{
				/* Build from MDOType */

				// Find the type from the UL
				const MDOType *Type = MDOType::Find((*Map_it).first);

				if(Type)
				{
					ThisDefinition->SetString(MetaDefinitionName_UL, Type->Name());
					ThisDefinition->SetString(MetaDefinitionDescription_UL, Type->GetDetail());
				}
				else
				{
					error("Failed to locate class %s when building ClassDefinition\n", (*Map_it).first.GetString().c_str());
				}
			}

			// DRAGONS: We don't destinguish abstract classes, so we flag all as concrete
			// FIXME: Probably need to update the dictionary to cope with this
			int IsConcrete = 1;
			ThisDefinition->SetUInt(IsConcrete_UL, IsConcrete);

			// Add to map of Class UL to definition record so that parent references can be tied up later
			Info.ClassMap[(*Map_it).first] = ThisDefinition;

			// InterchangeObject has its very own circular reference by definition!
			if((*Map_it).first == InterchangeObject_UL)
			{
				MDObjectPtr ParentLink = ThisDefinition->AddChild(ParentClass_UL);
				if(ParentLink) ParentLink->MakeRef(ThisDefinition);
			}

			// Add this class definition to the list of classes
			MDObjectPtr Ptr = ClassDefinitions->AddChild();
			if(Ptr) Ptr->MakeLink(ThisDefinition);

			Map_it++;
		}

		// Add Parent references to all derived classes
		MDObjectMap::iterator Def_it = Info.ClassMap.begin();
		while(Def_it != Info.ClassMap.end())
		{
			MDOTypePtr ThisClass = MDOType::Find((*Def_it).first);
			if(ThisClass->GetBase())
			{
				ULPtr BaseUL = ThisClass->GetBase()->GetUL();
				if(BaseUL)
				{
					MDObjectMap::iterator Base_it = Info.ClassMap.find(*BaseUL);
					if(Base_it == Info.ClassMap.end())
					{
						if(Feature(FeatureUsedMetadict))
						{
							error("Unable to find class definition with ID %s as a base for %s\n", BaseUL->GetString().c_str(), ThisClass->FullName().c_str());
						}
						else
						{
							// If we are building an incomplete KXS metadictionary, the parent link may be to a "known" definition rather than a definition in this file
							MDObjectPtr ParentLink = (*Def_it).second->AddChild(ParentClass_UL);
							if(ParentLink) ParentLink->SetString(BaseUL->GetString());
						}
					}
					else
					{
						MDObjectPtr ParentLink = (*Def_it).second->AddChild(ParentClass_UL);
						if(ParentLink) ParentLink->MakeRef((*Base_it).second);
					}
				}
			}

			Def_it++;
		}

		// All went well
		return true;
	}


	//! Build property definitions and add to a given metadictionary
	/*! \return true if all went well, false if there was a fatal error
	 */
	bool BuildPropertyDefs(MetaDictInfo &Info)
	{
		/* For each property, build a property definition and add it to the correct class definition for this property in the metadictionary */
		MDObjectMap::iterator Map_it = Info.Properties.begin();
		while(Map_it != Info.Properties.end())
		{
			AddProperty((*Map_it).second, Info);
			Map_it++;
		}

		// All went well
		return true;
	}
}


namespace
{
	// Structure holding info on a particular typedef object
	struct TypeDefInfo
	{
		DefinitionType DefType;
		MDTypePtr Type;
		MDObjectPtr Definition;
	};

	// Map of typedef info objects indexed by UL
	typedef std::map<UL, TypeDefInfo> TypeDefInfoMap;


	//! Build type definitions for each specified type and add to a given metadictionary
	/*! \return true if all went well, false if there was a fatal error
	 */
	bool BuildTypeDefs(MetaDictInfo &Info)
	{
		// Add the type definitions batch
		MDObjectPtr TypeDefinitions;

		if(Feature(FeatureKXSMetadict))
			TypeDefinitions = Info.MetaDict["MetaDefinitions"];		// FIXME: Use UL when defined
		else
			TypeDefinitions = Info.MetaDict->AddChild(TypeDefinitions_UL);

		if(!TypeDefinitions) return false;

		// Map of partially completed typedefs indexed by thier Type UL
		TypeDefInfoMap InfoMap;

		/*  AVMETA: For an Avid metadictionary the types are written in strict order as defined by TypeOrder,
		 *          otherwise they are written in the order they are found in Info.TypeMap
		 */

		MDTypeULMap::iterator Types_it;								// Current item in Info.TypeMap 

		// For each type, build a type definition and add it to the list of types in the metadictionary

		Types_it = Info.TypeMap.begin();
		while(Types_it != Info.TypeMap.end())
		{

			if(Types_it !=  Info.TypeMap.end())
			{
				MDObjectPtr ThisDefinition = NULL;
				DefinitionType ThisDefinitionType = SelectDefinitionType((*Types_it).second);

				if(ThisDefinitionType == DefinitionError)
				{
					Types_it++;
					continue;
				}

				switch(ThisDefinitionType)
				{
					case DefinitionInteger:
						ThisDefinition = new MDObject(TypeDefinitionInteger_UL);
						break;

					case DefinitionCharacter:
						ThisDefinition = new MDObject(TypeDefinitionCharacter_UL);
						break;

					case DefinitionRename:
						ThisDefinition = new MDObject(TypeDefinitionRename_UL);
						break;

					case DefinitionVariableArray:
						ThisDefinition = new MDObject(TypeDefinitionVariableArray_UL);
						break;

					case DefinitionSet:
						ThisDefinition = new MDObject(TypeDefinitionSet_UL);
						break;

					case DefinitionFixedArray:
						ThisDefinition = new MDObject(TypeDefinitionFixedArray_UL);
						break;

					case DefinitionString:
						ThisDefinition = new MDObject(TypeDefinitionString_UL);
						break;

					case DefinitionRecord:
						ThisDefinition = new MDObject(TypeDefinitionRecord_UL);
						break;

					case DefinitionEnum:
						ThisDefinition = new MDObject(TypeDefinitionEnumeration_UL);
						break;

					case DefinitionExtEnum:
						ThisDefinition = new MDObject(TypeDefinitionExtendibleEnumeration_UL);
						break;

					case DefinitionStrongRef:
						ThisDefinition = new MDObject(TypeDefinitionStrongObjectReference_UL);
						break;

					case DefinitionWeakRef:
						ThisDefinition = new MDObject(TypeDefinitionWeakObjectReference_UL);
						break;

					default:
						error("No idea how to build TypeDefinition object kind %d for type %s - skipping\n", ThisDefinitionType, (*Types_it).second->Name().c_str());

						Types_it++;
						continue;
				}

				if(!ThisDefinition)
				{
					error("Unable to build TypeDefinition object for type %s - skipping\n", (*Types_it).second->Name().c_str());

					Types_it++;
					continue;
				}

				/* Add basic info that is common to all typedefs.
				 * Type specific properties are added after all typedefs have been built so weakrefs can be added where required
				 */

				ThisDefinition->SetString(MetaDefinitionIdentification_UL, (*Types_it).first.GetString());
				ThisDefinition->SetString(MetaDefinitionName_UL, (*Types_it).second->Name());
				ThisDefinition->SetString(MetaDefinitionDescription_UL, (*Types_it).second->GetDetail());

				// Add this type definition to the list of types
				MDObjectPtr Ptr = TypeDefinitions->AddChild();
				if(Ptr) Ptr->MakeLink(ThisDefinition);

				// Build a descriptor for the lookup-list
				TypeDefInfo TypeInfo;
				TypeInfo.Definition = ThisDefinition;
				TypeInfo.DefType = ThisDefinitionType;
				TypeInfo.Type = (*Types_it).second;

				// Add this partial type to the look-up list
				InfoMap[(*Types_it).first] = TypeInfo;
			}

				Types_it++;
		}

		// Complete partial typedefs now we have built all as this allows us to make weak refs between typedefs
		TypeDefInfoMap::iterator Info_it = InfoMap.begin();
		while(Info_it != InfoMap.end())
		{
			// Set some references to make the following code more readable
			MDTypePtr &ThisType = (*Info_it).second.Type;
			MDObjectPtr &ThisDefinition = (*Info_it).second.Definition;
			DefinitionType &ThisDefinitionType = (*Info_it).second.DefType;

			switch(ThisDefinitionType)
			{
				case DefinitionInteger:
					ThisDefinition->SetUInt(Size_UL, ThisType->GetSize());
					// FIXME: Need a way to make this work!
					// FIXME: Not done yet as this requires a dictionary format update!
					//ThisDefinition->SetUInt(IsSigned_UL, static_cast<UInt32>((*Types_it).second->IsSigned() ? 1 : 0));
					
					// FIXME: Remove this WTF hack!
					{
						MDObjectPtr Val = new MDObject("Unknown");
						if(Val)
						{
							Val->SetTraits(MDType::LookupTraitsMapping(ThisType->GetTypeUL()));
							Val->SetString("-1");
							if(Val->GetString() == "-1") ThisDefinition->SetUInt(IsSigned_UL,1); else ThisDefinition->SetUInt(IsSigned_UL,0);
						}
					}

					break;

				case DefinitionCharacter:
					ThisDefinition->SetUInt(Size_UL, ThisType->GetSize());
					// FIXME: This is not right, but AAF only has UTF16!!

					break;

				case DefinitionRename:
				{
					MDTypePtr BaseType = ThisType->GetBase();
					if(!BaseType)
					{
						error("Interpretation type %s has no known base type\n", ThisType->Name().c_str());
						break;
					}

					// String version of target UL if we are based on a "known" type rather that using an in-file definition
					std::string TargetString;
					TypeDefInfoMap::iterator Find_it = InfoMap.find(*(BaseType->GetTypeUL()));
					if(Find_it == InfoMap.end())
					{

						if(Feature(FeatureUsedMetadict))
						{
							error("Interpretation type %s is based on type %s, which is not defined in the current metadictionary or has no valid UL\n", ThisType->Name().c_str(), BaseType->Name().c_str());
							break;
						}
						else
						{
							// If we are building an incomplete KXS metadictionary, the base type may be a "known" definition rather than a definition in this file
							TargetString = BaseType->GetTypeUL()->GetString();
						}
					}

					MDObjectPtr Ptr = ThisDefinition->AddChild(RenamedType_UL);
					if(Ptr)
					{
						if(TargetString.empty())
						{
							if(Feature(FeatureKXSMetadict))
								Ptr->MakeRef((*Find_it).second.Definition, MetaDefinitionIdentification_UL);
							else
								Ptr->MakeRef((*Find_it).second.Definition);
						}
						else
							Ptr->SetString(TargetString);
					}

					break;
				}

				case DefinitionVariableArray:
				case DefinitionFixedArray:
				case DefinitionSet:
				{
					MDTypePtr BaseType = ThisType->GetBase();
					if(!BaseType)
					{
						error("Multiple type %s has no known base type\n", ThisType->Name().c_str());
						break;
					}

					// String version of target UL if we are based on a "known" type rather that using an in-file definition
					std::string TargetString;
					TypeDefInfoMap::iterator Find_it = InfoMap.find(*(BaseType->GetTypeUL()));
					if(Find_it == InfoMap.end())
					{

						if(Feature(FeatureUsedMetadict))
						{
							error("Multiple type %s is of type %s, which is not defined in the current metadictionary or has no valid UL\n", ThisType->Name().c_str(), BaseType->Name().c_str());
							break;
						}
						else
						{
							// If we are building an incomplete KXS metadictionary, the base type may be a "known" definition rather than a definition in this file
							TargetString = BaseType->GetTypeUL()->GetString();
						}
					}

					MDObjectPtr Ptr;
					if(ThisDefinitionType == DefinitionVariableArray)
					{
						Ptr = ThisDefinition->AddChild(VariableArrayElementType_UL);
					}
					else if(ThisDefinitionType == DefinitionSet)
					{
						Ptr = ThisDefinition->AddChild(SetElementType_UL);
					}
					else
					{
						ThisDefinition->SetUInt(ElementCount_UL, static_cast<UInt32>(ThisType->GetSize()));
						Ptr = ThisDefinition->AddChild(FixedArrayElementType_UL);
					}
					if(Ptr)
					{
						if(TargetString.empty())
						{
							if(Feature(FeatureKXSMetadict))
								Ptr->MakeRef((*Find_it).second.Definition, MetaDefinitionIdentification_UL);
							else
								Ptr->MakeRef((*Find_it).second.Definition);
						}
						else
							Ptr->SetString(TargetString);
					}

					break;
				}

				case DefinitionString:
				{
					MDTypePtr BaseType = ThisType->GetBase();
					if(!BaseType)
					{
						error("String type %s has no known base type\n", ThisType->Name().c_str());
						break;
					}

					// String version of target UL if we are based on a "known" type rather that using an in-file definition
					std::string TargetString;
					TypeDefInfoMap::iterator Find_it = InfoMap.find(*(BaseType->GetTypeUL()));
					if(Find_it == InfoMap.end())
					{

						if(Feature(FeatureUsedMetadict))
						{
							error("String type %s is of type %s, which is not defined in the current metadictionary or has no valid UL\n", ThisType->Name().c_str(), BaseType->Name().c_str());
							break;
						}
						else
						{
							// If we are building an incomplete KXS metadictionary, the base type may be a "known" definition rather than a definition in this file
							TargetString = BaseType->GetTypeUL()->GetString();
						}
					}

					MDObjectPtr Ptr;
					Ptr = ThisDefinition->AddChild(StringElementType_UL);
					if(Ptr)
					{
						if(TargetString.empty())
						{
							if(Feature(FeatureKXSMetadict))
								Ptr->MakeRef((*Find_it).second.Definition, MetaDefinitionIdentification_UL);
							else
								Ptr->MakeRef((*Find_it).second.Definition);
						}
						else
							Ptr->SetString(TargetString);
					}

					break;
				}
				case DefinitionRecord:
				{
					std::list<std::string> NameList;

					MDObjectPtr MemberTypes = ThisDefinition->AddChild(MemberTypes_UL);
					if(!MemberTypes) break;

					// Flag to allow us to abandon an entry if it seems invalid
					bool Abort = false;

					MDTypeList::const_iterator it = ThisType->GetChildList().begin();
					while(it != ThisType->GetChildList().end())
					{
						NameList.push_back((*it)->Name());

						ULPtr TypeUL;
						TypeDefInfoMap::iterator Find_it;

						// String version of target UL if we are based on a "known" type rather that using an in-file definition
						std::string TargetString = "";

						Find_it = InfoMap.find(*((*it)->GetTypeUL()));

						if(Find_it == InfoMap.end())
						{
							if(Feature(FeatureUsedMetadict))
							{
								error("Compound type %s contains member %s, which is not defined in the current metadictionary or has no valid UL\n", ThisType->Name().c_str(), (*it)->Name().c_str());
								Abort = true;
								break;
							}
							else
							{
								// If we are building an incomplete KXS metadictionary, the base type may be a "known" definition rather than a definition in this file
								TargetString = (*it)->GetTypeUL()->GetString();
							}
						}

						MDObjectPtr Ptr = MemberTypes->AddChild();
						if(Ptr)
						{
							if(TargetString.empty())
							{
								if(Feature(FeatureKXSMetadict))
									Ptr->MakeRef((*Find_it).second.Definition, MetaDefinitionIdentification_UL);
								else
									Ptr->MakeRef((*Find_it).second.Definition);
							}
							else
								Ptr->SetString(TargetString);
						}

						it++;
					}

					// Did something go wrong? If so don't add the names as we will have an invalid typedef anyway
					if(Abort) break;

					MDObjectPtr Ptr = ThisDefinition->AddChild(MemberNames_UL);
					if(Ptr) SetStringArray(Ptr, NameList);

					break;
				}

				case DefinitionEnum:
				{
					MDTypePtr BaseType = ThisType->GetBase();
					if(!BaseType)
					{
						error("Enumeration type %s has no known base type\n", ThisType->Name().c_str());
						break;
					}

					// String version of target UL if we are based on a "known" type rather that using an in-file definition
					std::string TargetString;
					TypeDefInfoMap::iterator Find_it = InfoMap.find(*(BaseType->GetTypeUL()));
					if(Find_it == InfoMap.end())
					{
						if(Feature(FeatureUsedMetadict))
						{
							error("Enumeration type %s is of type %s, which is not defined in the current metadictionary or has no valid UL\n", ThisType->Name().c_str(), BaseType->Name().c_str());
							break;
						}
						else
						{
							// If we are building an incomplete KXS metadictionary, the base type may be a "known" definition rather than a definition in this file
							TargetString = BaseType->GetTypeUL()->GetString();
						}
					}

					MDObjectPtr Ptr = ThisDefinition->AddChild(ElementType_UL);
					if(Ptr)
					{
						if(TargetString.empty())
						{
							if(Feature(FeatureKXSMetadict))
								Ptr->MakeRef((*Find_it).second.Definition, MetaDefinitionIdentification_UL);
							else
								Ptr->MakeRef((*Find_it).second.Definition);
						}
						else
							Ptr->SetString(TargetString);
					}
				
					MDObjectPtr ElementValues = ThisDefinition->AddChild(ElementValues_UL);
					if(!ElementValues) break;

					// Build the names list and add each value to the values property
					std::list<std::string> NameList;
					int Index = 0;
					MDType::NamedValueList::const_iterator it = ThisType->GetEnumValues().begin();
					while(it != ThisType->GetEnumValues().end())
					{
						NameList.push_back((*it).first);
						ElementValues->AddChild()->SetInt64((*it).second->GetInt64());
						it++;
					}

					Ptr = ThisDefinition->AddChild(ElementNames_UL);
					if(Ptr) SetStringArray(Ptr, NameList);

					break;
				}

				case DefinitionExtEnum:
				{
					// FIXME: We are currently not adding the enumerated values to the dictionary
					break;
				}

				case DefinitionStrongRef:
				{
					// Build an iterator that will point to the target class def, when found
					MDObjectMap::iterator Find_it = Info.ClassMap.end();

					// String version of target UL if we are based on a "known" type rather that using an in-file definition
					std::string TargetString;
					MDOTypePtr TargetType = MDOType::Find(ThisType->EffectiveRefTargetName());
					if(!TargetType)
					{
						error("StrongRef type %s has no known target type\n", ThisType->Name().c_str());
					}
					else
					{
						Find_it = Info.ClassMap.find(*(TargetType->GetTypeUL()));
						if(Find_it == Info.ClassMap.end())
						{
							if(Feature(FeatureUsedMetadict))
							{
								error("StrongRef type %s has target type %s, which is not defined in the current metadictionary or has no valid UL\n", ThisType->Name().c_str(), TargetType->Name().c_str());
							}
							else
							{
								// If we are building an incomplete KXS metadictionary, the target type may be a "known" definition rather than a definition in this file
								TargetString = TargetType->GetTypeUL()->GetString();
							}
						}
					}

					// If we failed to locate the target info, build a fallback to InterchangeObject
					if(TargetString.empty() && (Find_it == Info.ClassMap.end()))
					{
						Find_it = Info.ClassMap.find(InterchangeObject_UL);
						if(Find_it == Info.ClassMap.end())
						{
							error("InterchangeObject is not known - unable to build a fall-back reference target\n");
							break;
						}
					}

					MDObjectPtr Ptr = ThisDefinition->AddChild(ReferencedType_UL);
					if(Ptr)
					{
						if(TargetString.empty())
						{
							if(Feature(FeatureKXSMetadict))
								Ptr->MakeRef((*Find_it).second, MetaDefinitionIdentification_UL);
							else
								Ptr->MakeRef((*Find_it).second);
						}
						else
							Ptr->SetString(TargetString);
					}

					break;
				}

				case DefinitionWeakRef:
				{
					// Build an iterator that will point to the target class def, when found
					MDObjectMap::iterator Find_it = Info.ClassMap.end();

					MDOTypePtr TargetType = MDOType::Find(ThisType->EffectiveRefTargetName());
					if(TargetType) Find_it = Info.ClassMap.find(*(TargetType->GetTypeUL()));

					// String version of target UL if we are based on a "known" type rather that using an in-file definition
					std::string TargetString;
					if(Find_it == Info.ClassMap.end())
					{
						if(TargetType)
						{
							if(Feature(FeatureUsedMetadict))
							{
								error("WeakRef type %s has target type %s, which is not defined in the current metadictionary or has no valid UL\n", ThisType->Name().c_str(), TargetType->Name().c_str());
							}
							else
							{
								// If we are building an incomplete KXS metadictionary, the target type may be a "known" definition rather than a definition in this file
								TargetString = TargetType->GetTypeUL()->GetString();
							}
						}
						else
							error("WeakRef type %s has no target specified\n", ThisType->Name().c_str());

						// If we failed to locate the target info, build a fallback to InterchangeObject
						if(TargetString.empty())
						{
							Find_it = Info.ClassMap.find(InterchangeObject_UL);
							if(Find_it == Info.ClassMap.end())
							{
								error("InterchangeObject is not known - unable to build a fall-back reference target\n");
								break;
							}
						}
					}

					/* TODO: Investigate why the following clause was used - it would always be true!
					// We only add this property if a target is specified - otherwise this will be a global reference
					if(Find_it != Info.ClassMap.end())
					{
					*/
					MDObjectPtr Ptr = ThisDefinition->AddChild(WeakReferencedType_UL);
					if(Ptr)
					{
						if(TargetString.empty())
						{
							if(Feature(FeatureKXSMetadict))
								Ptr->MakeRef((*Find_it).second, MetaDefinitionIdentification_UL);
							else
								Ptr->MakeRef((*Find_it).second);
						}
						else
							Ptr->SetString(TargetString);
					}

					break;
				}

			}

			Info_it++;
		}

		// All went well
		return true;
	}
}



//! Build a metadictionary from current classes and types used in a given list of metadata trees
/*! If Meta is supplied, then all classes in the trees strongly linked from it are written to the metadictionary, 
 *  including all properties of those classes (whether used or not) and any types used by those properties
 */
MDObjectPtr mxflib::BuildMetadictionary(MDObjectList &MetaList, Primer *UsePrimer)
{
	// Build an info structure to keep track of the process of building the metadictionary
	MetaDictInfo Info(MetaList, UsePrimer);

	// Build a list of what needs to be added to this metadictionary
	Info.BuildMetaLists();

	// Did we fail to find anything to build?
	if(Info.Classes.empty() && Info.Properties.empty() && Info.TypeMap.empty()) return NULL;

	// Build the container object
	if(Feature(FeatureKXSMetadict))
	{
		Info.MetaDict = new MDObject(ExtensionScheme_UL);
		if(Info.MetaDict)
		{
			UUIDPtr NewID = new mxflib::UUID;
			std::string NewIDString = NewID->GetString();
			Info.MetaDict->SetString(ExtensionSchemeID_UL, NewIDString);
			Info.MetaDict->SetString(SymbolSpaceURI_UL, std::string("urn:uuid:") + NewIDString.substr(1,36));
			Info.MetaDict->AddChild(MetaDefinitions_UL);
		}
	}
	else
		Info.MetaDict = new MDObject(MetaDictionary_UL);

	// Quit, returning NULL, if unable to build the metadictionary
	if(!Info.MetaDict) return Info.MetaDict;

	// Build class definitions
	if(!BuildClassDefs(Info))
	{
		return NULL;
	}

	// Build property definitions
	if(!BuildPropertyDefs(Info))
	{
		return NULL;
	}


	/* Add type definitions */

	// Build class definitions (adding property defs as well)
	if(!BuildTypeDefs(Info))
	{
		return NULL;
	}

	return Info.MetaDict;
}



