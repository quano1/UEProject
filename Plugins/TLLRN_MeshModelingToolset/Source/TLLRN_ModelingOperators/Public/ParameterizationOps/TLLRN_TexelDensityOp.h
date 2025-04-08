// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Util/ProgressCancel.h"
#include "TLLRN_ModelingOperators.h"
#include "InteractiveTool.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

#include "TLLRN_TexelDensityOp.generated.h"

UENUM()
enum class ETLLRN_TexelDensityToolMode
{
	ApplyToIslands,
	ApplyToWhole,
	Normalize
};

UCLASS()
class TLLRN_MODELINGOPERATORS_API UTLLRN_UVEditorTexelDensitySettings : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:

	UFUNCTION()
	virtual bool InSamplingMode() const;

	UPROPERTY(EditAnywhere, Category = "TexelDensity", meta = (DisplayName = "Scale Mode", EditCondition = "!InSamplingMode" ))
	ETLLRN_TexelDensityToolMode TexelDensityMode = ETLLRN_TexelDensityToolMode::ApplyToIslands;

	UPROPERTY(EditAnywhere, Category = "TexelDensity", meta = (DisplayName = "World Units", EditCondition = "TexelDensityMode != ETLLRN_TexelDensityToolMode::Normalize"))
	float TargetWorldUnits = 100;

	UPROPERTY(EditAnywhere, Category = "TexelDensity", meta = (DisplayName = "Pixels", EditCondition = "TexelDensityMode != ETLLRN_TexelDensityToolMode::Normalize && !InSamplingMode"))
	float TargetPixelCount = 1024;

	UPROPERTY(EditAnywhere, Category = "TexelDensity", meta = (DisplayName = "Texture Dimensions", EditCondition = "TexelDensityMode != ETLLRN_TexelDensityToolMode::Normalize && !InSamplingMode", LinearDeltaSensitivity = 1000, Delta = 64, SliderExponent = 1, UIMin = 2, UIMax = 16384))
	float TextureResolution = 1024;

	UPROPERTY(EditAnywhere, Category = "TexelDensity", meta = (DisplayName = "Use UDIMs", EditCondition = "TexelDensityMode != ETLLRN_TexelDensityToolMode::Normalize && !InSamplingMode", TransientToolProperty))
	bool bEnableUDIMLayout = false;
};


namespace UE
{
	namespace Geometry
	{
		class FDynamicMesh3;
		class FDynamicMeshUVPacker;

		enum class EUVTLLRN_TexelDensityOpModes
		{
			ScaleGlobal = 0,
			ScaleIslands = 1,
			Normalize = 2
		};

		class TLLRN_MODELINGOPERATORS_API FUVEditorTLLRN_TexelDensityOp : public FDynamicMeshOperator
		{
		public:
			virtual ~FUVEditorTLLRN_TexelDensityOp() {}

			// inputs
			TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe> OriginalMesh;

			EUVTLLRN_TexelDensityOpModes TexelDensityMode = EUVTLLRN_TexelDensityOpModes::ScaleGlobal;

			int UVLayerIndex = 0;
			int TextureResolution = 1024;
			int TargetWorldSpaceMeasurement = 100;
			int TargetPixelCountMeasurement = 1024;

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
			void ScaleMeshSubRegionByDensity(FDynamicMeshUVOverlay* UVLayer, const TArray<int32>& Tids, TSet<int32>& UVElements, int32 TileResolution);


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
class TLLRN_MODELINGOPERATORS_API UTLLRN_UVTLLRN_TexelDensityOperatorFactory : public UObject, public UE::Geometry::IDynamicMeshOperatorFactory
{
	GENERATED_BODY()

public:
	// IDynamicMeshOperatorFactory API
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorTexelDensitySettings> Settings;

	TOptional<TSet<int32>> Selection;
	TSharedPtr<UE::Geometry::FDynamicMesh3, ESPMode::ThreadSafe> OriginalMesh;
	TUniqueFunction<int32()> GetSelectedUVChannel = []() { return 0; };
	FTransform TargetTransform;
	TOptional<TMap<int32, int32>> TextureResolutionPerUDIM;
};