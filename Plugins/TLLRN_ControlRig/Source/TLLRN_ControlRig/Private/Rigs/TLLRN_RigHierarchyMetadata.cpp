// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigHierarchyMetadata.h"

#include "TLLRN_ControlRigObjectVersion.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigHierarchyMetadata)

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigBaseMetadata
////////////////////////////////////////////////////////////////////////////////

UScriptStruct* FTLLRN_RigBaseMetadata::GetMetadataStruct() const
{
	return GetMetadataStruct(GetType());
}

UScriptStruct* FTLLRN_RigBaseMetadata::GetMetadataStruct(const ETLLRN_RigMetadataType& InType)
{
	switch(InType)
	{
		case ETLLRN_RigMetadataType::Bool:
		{
			return FTLLRN_RigBoolMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::BoolArray:
		{
			return FTLLRN_RigBoolArrayMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::Float:
		{
			return FTLLRN_RigFloatMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::FloatArray:
		{
			return FTLLRN_RigFloatArrayMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::Int32:
		{
			return FTLLRN_RigInt32Metadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::Int32Array:
		{
			return FTLLRN_RigInt32ArrayMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::Name:
		{
			return FTLLRN_RigNameMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::NameArray:
		{
			return FTLLRN_RigNameArrayMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::Vector:
		{
			return FTLLRN_RigVectorMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::VectorArray:
		{
			return FTLLRN_RigVectorArrayMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::Rotator:
		{
			return FTLLRN_RigRotatorMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::RotatorArray:
		{
			return FTLLRN_RigRotatorArrayMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::Quat:
		{
			return FTLLRN_RigQuatMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::QuatArray:
		{
			return FTLLRN_RigQuatArrayMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::Transform:
		{
			return FTLLRN_RigTransformMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::TransformArray:
		{
			return FTLLRN_RigTransformArrayMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::LinearColor:
		{
			return FTLLRN_RigLinearColorMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::LinearColorArray:
		{
			return FTLLRN_RigLinearColorArrayMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::TLLRN_RigElementKey:
		{
			return FTLLRN_RigElementKeyMetadata::StaticStruct();
		}
		case ETLLRN_RigMetadataType::TLLRN_RigElementKeyArray:
		{
			return FTLLRN_RigElementKeyArrayMetadata::StaticStruct();
		}
		default:
		{
			break;
		}
	}
	return StaticStruct();
}

FTLLRN_RigBaseMetadata* FTLLRN_RigBaseMetadata::MakeMetadata(const FName& InName, ETLLRN_RigMetadataType InType)
{
	check(InType != ETLLRN_RigMetadataType::Invalid);
	
	const UScriptStruct* Struct = GetMetadataStruct(InType);
	check(Struct);
	
	FTLLRN_RigBaseMetadata* Md = static_cast<FTLLRN_RigBaseMetadata*>(FMemory::Malloc(Struct->GetStructureSize()));
	Struct->InitializeStruct(Md, 1);

	Md->Name = InName;
	Md->Type = InType;
	return Md;
}

void FTLLRN_RigBaseMetadata::DestroyMetadata(FTLLRN_RigBaseMetadata** Metadata)
{
	check(Metadata);
	FTLLRN_RigBaseMetadata* Md = *Metadata;
	check(Md);
	if(const UScriptStruct* Struct = Md->GetMetadataStruct())
	{
		Struct->DestroyStruct(Md, 1);
	}
	FMemory::Free(Md);
	Md = nullptr;
}

void FTLLRN_RigBaseMetadata::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);
	Ar << Name;
	Ar << Type;
}
