// Copyright Epic Games, Inc. All Rights Reserved.

#include "PropertySets/TLLRN_CreateMeshObjectTypeProperties.h"
#include "TLLRN_ModelingObjectsCreationAPI.h"
#include "TLLRN_ModelingComponentsSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_CreateMeshObjectTypeProperties)


const FString UTLLRN_CreateMeshObjectTypeProperties::StaticMeshIdentifier = TEXT("Static Mesh");
const FString UTLLRN_CreateMeshObjectTypeProperties::VolumeIdentifier = TEXT("Volume");
const FString UTLLRN_CreateMeshObjectTypeProperties::DynamicMeshActorIdentifier = TEXT("Dynamic Mesh");
const FString UTLLRN_CreateMeshObjectTypeProperties::AutoIdentifier = TEXT("From Input");

bool UTLLRN_CreateMeshObjectTypeProperties::bEnableDynamicMeshActorSupport = false;
FString UTLLRN_CreateMeshObjectTypeProperties::DefaultObjectTypeIdentifier = UTLLRN_CreateMeshObjectTypeProperties::StaticMeshIdentifier;


void UTLLRN_CreateMeshObjectTypeProperties::InitializeDefault()
{
	bool bStaticMeshes = true;
	bool bDynamicMeshes = true;

#if WITH_EDITOR
	bool bVolumes = true;
#else
	bool bVolumes = false;
#endif

	Initialize(bStaticMeshes, bVolumes, bDynamicMeshes);
}

void UTLLRN_CreateMeshObjectTypeProperties::InitializeDefaultWithAuto()
{
	InitializeDefault();
	OutputTypeNamesList.Add(AutoIdentifier);
}

void UTLLRN_CreateMeshObjectTypeProperties::Initialize(bool bEnableStaticMeshes, bool bEnableVolumes, bool bEnableDynamicMeshActor)
{
	if (bEnableStaticMeshes)
	{
		OutputTypeNamesList.Add(StaticMeshIdentifier);
	}
	if (bEnableVolumes)
	{
		OutputTypeNamesList.Add(VolumeIdentifier);
	}
	if (bEnableDynamicMeshActor && bEnableDynamicMeshActorSupport)
	{
		OutputTypeNamesList.Add(DynamicMeshActorIdentifier);
	}

	if (OutputTypeNamesList.Num() == 0)
	{
		return;
	}

	if ((OutputType.Len() == 0) || (OutputType.Len() > 0 && OutputTypeNamesList.Contains(OutputType) == false))
	{
		if (OutputTypeNamesList.Contains(DefaultObjectTypeIdentifier))
		{
			OutputType = DefaultObjectTypeIdentifier;
		}
		else
		{
			OutputType = OutputTypeNamesList[0];
		}
	}
}

const TArray<FString>& UTLLRN_CreateMeshObjectTypeProperties::GetOutputTypeNamesFunc()
{
	return OutputTypeNamesList;
}



bool UTLLRN_CreateMeshObjectTypeProperties::ShouldShowPropertySet() const
{
	return (OutputTypeNamesList.Num() > 1)
		|| OutputTypeNamesList.Contains(VolumeIdentifier);
}

ETLLRN_CreateObjectTypeHint UTLLRN_CreateMeshObjectTypeProperties::GetCurrentCreateMeshType() const
{
	if (OutputType == StaticMeshIdentifier)
	{
		return ETLLRN_CreateObjectTypeHint::StaticMesh;
	}
	else if (OutputType == VolumeIdentifier)
	{
		return ETLLRN_CreateObjectTypeHint::Volume;
	}
	else if (OutputType == DynamicMeshActorIdentifier)
	{
		return ETLLRN_CreateObjectTypeHint::DynamicMeshActor;
	}
	return ETLLRN_CreateObjectTypeHint::Undefined;
}

void UTLLRN_CreateMeshObjectTypeProperties::UpdatePropertyVisibility()
{
	bShowVolumeList = (OutputType == VolumeIdentifier);
}


bool UTLLRN_CreateMeshObjectTypeProperties::ConfigureTLLRN_CreateMeshObjectParams(FTLLRN_CreateMeshObjectParams& ParamsOut) const
{
	// client has to handle this case
	ensure(OutputType != AutoIdentifier);

	if (OutputType == StaticMeshIdentifier)
	{
		ParamsOut.TypeHint = ETLLRN_CreateObjectTypeHint::StaticMesh;
		return true;
	}
	else if (OutputType == VolumeIdentifier)
	{
		ParamsOut.TypeHint = ETLLRN_CreateObjectTypeHint::Volume;
		ParamsOut.TypeHintClass = VolumeType.Get();
		return true;
	}
	else if (OutputType == DynamicMeshActorIdentifier)
	{
		ParamsOut.TypeHint = ETLLRN_CreateObjectTypeHint::DynamicMeshActor;
		return true;
	}
	return false;
}

