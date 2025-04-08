// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/TLLRN_BaseCreateFromSelectedTool.h"
#include "PropertySets/TLLRN_VoxelProperties.h"

#include "TLLRN_BaseVoxelTool.generated.h"

PREDECLARE_USE_GEOMETRY_CLASS(FDynamicMesh3);

/**
 * Base for Voxel tools
 */
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_BaseVoxelTool : public UTLLRN_BaseCreateFromSelectedTool
{
	GENERATED_BODY()

protected:

	/** Sets up VoxProperties; typically need to overload and call "Super::SetupProperties();" */
	virtual void SetupProperties() override;

	/** Saves VoxProperties; typically need to overload and call "Super::SaveProperties();" */
	virtual void SaveProperties() override;

	/** Sets up the default preview and converted inputs for voxel tools; Typically do not need to overload */
	virtual void ConvertInputsAndSetPreviewMaterials(bool bSetTLLRN_PreviewMesh = true) override;

	/** Sets the output material to the default "world grid" material */
	virtual TArray<UMaterialInterface*> GetOutputMaterials() const override;

	// Test whether any of the OriginalDynamicMeshes has any open boundary edges
	virtual bool HasOpenBoundariesInMeshInputs();


	UPROPERTY()
	TObjectPtr<UTLLRN_VoxelProperties> VoxProperties;

	TArray<TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe>> OriginalDynamicMeshes;
};

