// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Util/ProgressCancel.h"
#include "TLLRN_ModelingOperators.h"

#include "TLLRN_UVLayoutOp.generated.h"

class UTLLRN_UVLayoutProperties;

namespace UE
{
namespace Geometry
{
	class FDynamicMesh3;
	class FDynamicMeshUVPacker;

enum class ETLLRN_UVLayoutOpLayoutModes
{
	TransformOnly = 0,
	RepackToUnitRect = 1,
	StackInUnitRect = 2,
	Normalize = 3
};

class TLLRN_MODELINGOPERATORS_API FTLLRN_UVLayoutOp : public FDynamicMeshOperator
{
public:
	virtual ~FTLLRN_UVLayoutOp() {}

	// inputs
	TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe> OriginalMesh;

	ETLLRN_UVLayoutOpLayoutModes UVLayoutMode = ETLLRN_UVLayoutOpLayoutModes::RepackToUnitRect;

	int UVLayerIndex = 0;
	int TextureResolution = 128;
	bool bPreserveScale = false;
	bool bPreserveRotation = false;
	bool bAllowFlips = false;
	bool bAlwaysSplitBowties = true;
	float UVScaleFactor = 1.0;
	float GutterSize = 1.0;
	bool bMaintainOriginatingUDIM = false;
	TOptional<TSet<int32>> Selection;
	TOptional<TMap<int32, int32>> TextureResolutionPerUDIM;
	FVector2f UVTranslation = FVector2f::Zero();

	void SetTransform(const FTransformSRT3d& Transform);

	//
	// FDynamicMeshOperator implementation
	// 

	virtual void CalculateResult(FProgressCancel* Progress) override;

protected:
	void ExecutePacker(FDynamicMeshUVPacker& Packer);
};

} // end namespace UE::Geometry
} // end namespace UE

/**
 * Can be hooked up to a UTLLRN_MeshOpPreviewWithBackgroundCompute to perform UV Layout operations.
 *
 * Inherits from UObject so that it can hold a strong pointer to the settings UObject, which
 * needs to be a UObject to be displayed in the details panel.
 */
UCLASS()
class TLLRN_MODELINGOPERATORS_API UTLLRN_UVLayoutOperatorFactory : public UObject, public UE::Geometry::IDynamicMeshOperatorFactory
{
	GENERATED_BODY()

public:
	// IDynamicMeshOperatorFactory API
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVLayoutProperties> Settings;
	TOptional<TSet<int32>> Selection;
	TSharedPtr<UE::Geometry::FDynamicMesh3, ESPMode::ThreadSafe> OriginalMesh;
	TUniqueFunction<int32()> GetSelectedUVChannel = []() { return 0; };
	FTransform TargetTransform;
	TOptional<TMap<int32, int32>> TextureResolutionPerUDIM;
};