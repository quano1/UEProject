// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "AssetDefinitionDefault.h"

#include "AssetDefinition_TLLRN_ControlTLLRN_RigPose.generated.h"

UCLASS()
class UAssetDefinition_TLLRN_ControlTLLRN_RigPose : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_TLLRN_ControlTLLRN_RigPose", "TLLRN Control Rig Pose"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(222, 128, 64)); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UTLLRN_ControlTLLRN_RigPoseAsset::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { EAssetCategoryPaths::Animation };
		return Categories;
	}
	// UAssetDefinition End
};
