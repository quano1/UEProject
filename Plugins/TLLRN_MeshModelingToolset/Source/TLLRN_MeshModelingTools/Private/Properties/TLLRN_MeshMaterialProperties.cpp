// Copyright Epic Games, Inc. All Rights Reserved.

#include "Properties/TLLRN_MeshMaterialProperties.h"

#include "DynamicMesh/DynamicMesh3.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_MeshMaterialProperties)

#define LOCTEXT_NAMESPACE "UMeshMaterialProperites"

UTLLRN_NewTLLRN_MeshMaterialProperties::UTLLRN_NewTLLRN_MeshMaterialProperties()
{
	Material = CreateDefaultSubobject<UMaterialInterface>(TEXT("MATERIAL"));
}

const TArray<FString>& UTLLRN_ExistingTLLRN_MeshMaterialProperties::GetUVChannelNamesFunc() const
{
	return UVChannelNamesList;
}

void UTLLRN_ExistingTLLRN_MeshMaterialProperties::RestoreProperties(UInteractiveTool* RestoreToTool, const FString& CacheIdentifier)
{
	Super::RestoreProperties(RestoreToTool, CacheIdentifier);
	Setup();
}

void UTLLRN_ExistingTLLRN_MeshMaterialProperties::Setup()
{
	UMaterial* CheckerMaterialBase = LoadObject<UMaterial>(nullptr, TEXT("/TLLRN_MeshModelingToolsetExp/Materials/CheckerMaterial"));
	if (CheckerMaterialBase != nullptr)
	{
		CheckerMaterial = UMaterialInstanceDynamic::Create(CheckerMaterialBase, NULL);
		if (CheckerMaterial != nullptr)
		{
			CheckerMaterial->SetScalarParameterValue("Density", CheckerDensity);
			CheckerMaterial->SetScalarParameterValue("UVChannel", static_cast<float>(UVChannelNamesList.IndexOfByKey(UVChannel)));
		}
	}
}

void UTLLRN_ExistingTLLRN_MeshMaterialProperties::UpdateMaterials()
{
	if (CheckerMaterial != nullptr)
	{
		CheckerMaterial->SetScalarParameterValue("Density", CheckerDensity);
		CheckerMaterial->SetScalarParameterValue("UVChannel", static_cast<float>(UVChannelNamesList.IndexOfByKey(UVChannel)));
	}
}


UMaterialInterface* UTLLRN_ExistingTLLRN_MeshMaterialProperties::GetActiveOverrideMaterial() const
{
	if (MaterialMode == ETLLRN_SetMeshMaterialMode::Checkerboard && CheckerMaterial != nullptr)
	{
		return CheckerMaterial;
	}
	if (MaterialMode == ETLLRN_SetMeshMaterialMode::Override && OverrideMaterial != nullptr)
	{
		return OverrideMaterial;
	}
	return nullptr;
}

void UTLLRN_ExistingTLLRN_MeshMaterialProperties::UpdateUVChannels(int32 UVChannelIndex, const TArray<FString>& UVChannelNames, bool bUpdateSelection)
{
	UVChannelNamesList = UVChannelNames;
	if (bUpdateSelection)
	{
		UVChannel = 0 <= UVChannelIndex && UVChannelIndex < UVChannelNames.Num() ? UVChannelNames[UVChannelIndex] : TEXT("");
	}
}

#undef LOCTEXT_NAMESPACE

