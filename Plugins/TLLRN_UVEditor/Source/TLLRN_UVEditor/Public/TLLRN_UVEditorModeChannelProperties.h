// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "InteractiveTool.h"
#include "GeometryBase.h"

#include "TLLRN_UVEditorModeChannelProperties.generated.h"

PREDECLARE_GEOMETRY(class FDynamicMesh3);

class UInteractiveToolPropertySet;
class UToolTarget;
class UTLLRN_UVEditorToolMeshInput;

/**
 * UV Layer Settings for the TLLRN_UVEditorMode
 */
UCLASS()
class TLLRN_UVEDITOR_API UTLLRN_UVEditorUVChannelProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category = UVChannel, meta = (DisplayName = "Asset", GetOptions = GetAssetNames))
	FString Asset;

	UFUNCTION()
	const TArray<FString>& GetAssetNames();

	UPROPERTY(EditAnywhere, Category = UVChannel, meta = (DisplayName = "UV Channel", GetOptions = GetUVChannelNames))
	FString UVChannel;

	UFUNCTION()
	const TArray<FString>& GetUVChannelNames();

	TArray<FString> UVChannelNames;
	TArray<FString> UVAssetNames;

	// 1:1 with UVAssetNames
	TArray<int32> NumUVLayersPerAsset;


public:
	void Initialize(
		TArray<TObjectPtr<UToolTarget>>& TargetObjects,
		TArray<TSharedPtr<UE::Geometry::FDynamicMesh3>> AppliedCanonicalMeshes,
		bool bInitializeSelection);

	/**
	 * Verify that the UV asset selection is valid
	 * @param bUpdateIfInvalid if selection is not valid, reset UVAsset to Asset0 or empty if no assets exist
	 * @return true if selection in UVAsset is an entry in UVAssetNames.
	 */
	bool ValidateUVAssetSelection(bool bUpdateIfInvalid);

	/**
	 * Verify that the UV channel selection is valid
	 * @param bUpdateIfInvalid if selection is not valid, UVChannel to UV0 or empty if no UV channels exist
	 * @return true if selection in UVChannel is an entry in UVChannelNamesList.
	 */
	bool ValidateUVChannelSelection(bool bUpdateIfInvalid);


	/**
	 * @return selected UV asset ID, or -1 if invalid selection
	 */
	int32 GetSelectedAssetID();

	/**
	 * @param bForceToZeroOnFailure if true, then instead of returning -1 we return 0 so calling code can fallback to default UV paths
	 * @return selected UV channel index, or -1 if invalid selection, or 0 if invalid selection and bool bForceToZeroOnFailure = true
	 */
	int32 GetSelectedChannelIndex(bool bForceToZeroOnFailure = false);
};
