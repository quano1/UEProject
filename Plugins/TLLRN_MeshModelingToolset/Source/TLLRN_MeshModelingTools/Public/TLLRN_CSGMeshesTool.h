// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/TLLRN_BaseCreateFromSelectedTool.h"
#include "CompositionOps/TLLRN_BooleanMeshesOp.h"
#include "Drawing/TLLRN_LineSetComponent.h"
#include "UObject/NoExportTypes.h"

#include "TLLRN_CSGMeshesTool.generated.h"

// Forward declarations
PREDECLARE_USE_GEOMETRY_CLASS(FDynamicMesh3);


/**
 * Standard properties of the CSG operation
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_CSGMeshesToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Type of Boolean operation */
	UPROPERTY(EditAnywhere, Category = Boolean)
	ETLLRN_CSGOperation Operation = ETLLRN_CSGOperation::DifferenceAB;

	/** Try to fill holes created by the Boolean operation, e.g. due to numerical errors */
	UPROPERTY(EditAnywhere, Category = Boolean, AdvancedDisplay)
	bool bTryFixHoles = false;

	/** Try to collapse extra edges created by the Boolean operation */
	UPROPERTY(EditAnywhere, Category = Boolean, AdvancedDisplay)
	bool bTryCollapseEdges = true;

	/** Threshold to determine whether a triangle in one mesh is inside or outside of the other */
	UPROPERTY(EditAnywhere, Category = Boolean, AdvancedDisplay, meta = (UIMin = "0", UIMax = "1"))
	float WindingThreshold = 0.5;

	/** Show boundary edges created by the Boolean operation, which might happen due to numerical errors */
	UPROPERTY(EditAnywhere, Category = Display)
	bool bShowNewBoundaries = true;

	/** Show a translucent version of the subtracted mesh, to help visualize geometry that is being removed */
	UPROPERTY(EditAnywhere, Category = Display, meta = (EditCondition = "Operation == ETLLRN_CSGOperation::DifferenceAB || Operation == ETLLRN_CSGOperation::DifferenceBA"))
	bool bShowSubtractedMesh = true;
	
	/** Opacity of the translucent subtracted mesh */
	UPROPERTY(EditAnywhere, Category = Display, meta = (DisplayName = "Opacity Subtracted Mesh", ClampMin = "0", ClampMax = "1",
		EditCondition = "bShowSubtractedMesh && Operation == ETLLRN_CSGOperation::DifferenceAB || bShowSubtractedMesh && Operation == ETLLRN_CSGOperation::DifferenceBA"))
	float SubtractedMeshOpacity = 0.2f;
	
	/** Color of the translucent subtracted mesh */
	UPROPERTY(EditAnywhere, Category = Display, meta = (DisplayName = "Color Subtracted Mesh", HideAlphaChannel, ClampMin = "0", ClampMax = "1",
		EditCondition = "bShowSubtractedMesh && Operation == ETLLRN_CSGOperation::DifferenceAB || bShowSubtractedMesh && Operation == ETLLRN_CSGOperation::DifferenceBA"))
	FLinearColor SubtractedMeshColor = FLinearColor::Black;

	/** If true, only the first mesh will keep its material assignments, and all other faces will have the first material assigned */
	UPROPERTY(EditAnywhere, Category = Materials)
	bool bUseFirstMeshMaterials = false;
};


/**
 * Properties of the trim mode
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_TrimMeshesToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Which object to trim */
	UPROPERTY(EditAnywhere, Category = Operation)
	ETLLRN_TrimOperation WhichMesh = ETLLRN_TrimOperation::TrimA;

	/** Whether to remove the surface inside or outside of the trimming geometry */
	UPROPERTY(EditAnywhere, Category = Operation)
	ETLLRN_TrimSide TLLRN_TrimSide = ETLLRN_TrimSide::RemoveInside;

	/** Threshold to determine whether a triangle in one mesh is inside or outside of the other */
	UPROPERTY(EditAnywhere, Category = Operation, AdvancedDisplay, meta = (UIMin = "0", UIMax = "1"))
	float WindingThreshold = 0.5;

	/** Whether to show a translucent version of the trimming mesh, to help visualize what is being cut */
	UPROPERTY(EditAnywhere, Category = Display)
	bool bShowTrimmingMesh = true;

	/** Opacity of translucent version of the trimming mesh */
	UPROPERTY(EditAnywhere, Category = Display, meta = (EditCondition = "bShowTrimmingMesh", ClampMin = "0", ClampMax = "1"))
	float OpacityOfTrimmingMesh = .2;

	/** Color of translucent version of the trimming mesh */
	UPROPERTY(EditAnywhere, Category = Display, meta = (EditCondition = "bShowTrimmingMesh"), AdvancedDisplay)
	FLinearColor ColorOfTrimmingMesh = FLinearColor::Black;

};


/**
 * Simple Mesh Plane Cutting Tool
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_CSGMeshesTool : public UTLLRN_BaseCreateFromSelectedTool
{
	GENERATED_BODY()

public:

	UTLLRN_CSGMeshesTool() {}

	void EnableTrimMode();

protected:

	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	virtual void ConvertInputsAndSetPreviewMaterials(bool bSetTLLRN_PreviewMesh = true) override;

	virtual void SetupProperties() override;
	virtual void SaveProperties() override;
	virtual void SetPreviewCallbacks() override;

	virtual FString GetCreatedAssetName() const;
	virtual FText GetActionName() const;

	// IDynamicMeshOperatorFactory API
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	void UpdateVisualization();

	UPROPERTY()
	TObjectPtr<UTLLRN_CSGMeshesToolProperties> CSGProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_TrimMeshesToolProperties> TrimProperties;

	TArray<TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe>> OriginalDynamicMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_PreviewMesh>> OriginalMeshPreviews;

	// Material used to show the otherwise-invisible cutting/trimming mesh
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PreviewsGhostMaterial;

	UPROPERTY()
	TObjectPtr<UTLLRN_LineSetComponent> DrawnLineSet;

	// for visualization of any errors in the currently-previewed CSG operation
	TArray<int> CreatedBoundaryEdges;

	bool bTrimMode = false;

	virtual int32 GetHiddenGizmoIndex() const;

	// Update visibility of ghostly preview meshes (used to show trimming or subtracting surface)
	void UpdatePreviewsVisibility();

	// update the material of ghostly preview meshes (used to show trimming or subtracting surface)
	void UpdatePreviewsMaterial();

	virtual bool KeepCollisionFrom(int32 TargetIdx) const override
	{
		if (bTrimMode)
		{
			return static_cast<int32>(TrimProperties->WhichMesh) == TargetIdx;
		}
		else if (CSGProperties->Operation == ETLLRN_CSGOperation::DifferenceAB)
		{
			return TargetIdx == 0;
		}
		else if (CSGProperties->Operation == ETLLRN_CSGOperation::DifferenceBA)
		{
			return TargetIdx == 1;
		}
		return true;
	}
};


UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_CSGMeshesToolBuilder : public UTLLRN_BaseCreateFromSelectedToolBuilder
{
	GENERATED_BODY()

public:

	bool bTrimMode = false;

	virtual TOptional<int32> MaxComponentsSupported() const override
	{
		return TOptional<int32>(2);
	}

	virtual int32 MinComponentsSupported() const override
	{
		return 2;
	}

	virtual UTLLRN_MultiSelectionMeshEditingTool* CreateNewTool(const FToolBuilderState& SceneState) const override
	{
		UTLLRN_CSGMeshesTool* Tool = NewObject<UTLLRN_CSGMeshesTool>(SceneState.ToolManager);
		if (bTrimMode)
		{
			Tool->EnableTrimMode();
		}
		return Tool;
	}
};
