// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorModeChannelProperties.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "ModelingToolTargetUtil.h"
#include "ToolTargets/TLLRN_UVEditorToolMeshInput.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVEditorModeChannelProperties)

void UTLLRN_UVEditorUVChannelProperties::Initialize(
	TArray<TObjectPtr<UToolTarget>>& TargetObjects, 
	TArray<TSharedPtr<UE::Geometry::FDynamicMesh3>> AppliedCanonicalMeshes,
	bool bInitializeSelection)
{
	if (!ensure(TargetObjects.Num() == AppliedCanonicalMeshes.Num()))
	{
		return;
	}

	UVAssetNames.Reset(TargetObjects.Num());
	NumUVLayersPerAsset.Reset(TargetObjects.Num());

	for (int i = 0; i < TargetObjects.Num(); ++i)
	{
		UVAssetNames.Add(UE::ToolTarget::GetHumanReadableName(TargetObjects[i]));
		
		int32 NumUVChannels = AppliedCanonicalMeshes[i]->HasAttributes() ?
			AppliedCanonicalMeshes[i]->Attributes()->NumUVLayers() : 0;
		
		NumUVLayersPerAsset.Add(NumUVChannels);
	}

	UVChannelNames.Reset();
	if (bInitializeSelection)
	{
		Asset = UVAssetNames[0];
		GetUVChannelNames();
		UVChannel = UVChannelNames.Num() > 0 ? UVChannelNames[0] : TEXT("");
	}
}

const TArray<FString>& UTLLRN_UVEditorUVChannelProperties::GetAssetNames()
{
	return UVAssetNames;
}

const TArray<FString>& UTLLRN_UVEditorUVChannelProperties::GetUVChannelNames()
{
	int32 FoundAssetIndex = UVAssetNames.IndexOfByKey(Asset);

	if (FoundAssetIndex == INDEX_NONE)
	{
		UVChannelNames.Reset();
		return UVChannelNames;
	}

	if (UVChannelNames.Num() != NumUVLayersPerAsset[FoundAssetIndex])
	{
		UVChannelNames.Reset();
		for (int32 i = 0; i < NumUVLayersPerAsset[FoundAssetIndex]; ++i)
		{
			UVChannelNames.Add(FString::Printf(TEXT("UV%d"), i));
		}
	}

	return UVChannelNames;
}

bool UTLLRN_UVEditorUVChannelProperties::ValidateUVAssetSelection(bool bUpdateIfInvalid)
{
	int32 FoundIndex = UVAssetNames.IndexOfByKey(Asset);
	if (FoundIndex == INDEX_NONE)
	{
		if (bUpdateIfInvalid)
		{
			Asset = (UVAssetNames.Num() > 0) ? UVAssetNames[0] : TEXT("");
		}
		return false;
	}
	return true;
}

bool UTLLRN_UVEditorUVChannelProperties::ValidateUVChannelSelection(bool bUpdateIfInvalid)
{
	bool bValid = ValidateUVAssetSelection(bUpdateIfInvalid);

	int32 FoundAssetIndex = UVAssetNames.IndexOfByKey(Asset);
	if (FoundAssetIndex == INDEX_NONE)
	{
		if (bUpdateIfInvalid)
		{
			UVChannel = TEXT("");
		}
		return false;
	}

	int32 FoundIndex = UVChannelNames.IndexOfByKey(UVChannel);
	if (FoundIndex >= NumUVLayersPerAsset[FoundAssetIndex])
	{
		FoundIndex = INDEX_NONE;
	}

	if (FoundIndex == INDEX_NONE)
	{
		if (bUpdateIfInvalid)
		{
			UVChannel = (NumUVLayersPerAsset[FoundAssetIndex] > 0) ? UVChannelNames[0] : TEXT("");
		}
		return false;
	}

	return bValid;
}

int32 UTLLRN_UVEditorUVChannelProperties::GetSelectedAssetID()
{
	int32 FoundIndex = UVAssetNames.IndexOfByKey(Asset);
	if (FoundIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	return FoundIndex;
}

int32 UTLLRN_UVEditorUVChannelProperties::GetSelectedChannelIndex(bool bForceToZeroOnFailure)
{
	int32 FoundAssetIndex = UVAssetNames.IndexOfByKey(Asset);
	if (FoundAssetIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	int32 FoundUVIndex = UVChannelNames.IndexOfByKey(UVChannel);
	if (!ensure(FoundUVIndex < NumUVLayersPerAsset[FoundAssetIndex]))
	{
		return INDEX_NONE;
	}
	return FoundUVIndex;
}
