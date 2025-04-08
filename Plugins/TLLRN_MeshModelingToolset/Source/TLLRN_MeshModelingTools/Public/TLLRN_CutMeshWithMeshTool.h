// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Drawing/TLLRN_LineSetComponent.h"
#include "BaseTools/TLLRN_BaseCreateFromSelectedTool.h"

#include "TLLRN_CutMeshWithMeshTool.generated.h"

// Forward declarations
class UTLLRN_PreviewMesh;
PREDECLARE_USE_GEOMETRY_CLASS(FDynamicMesh3);


/**
 * Standard properties of the CutMeshWithMesh operation
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_CutMeshWithMeshToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Try to fill holes created by the Boolean operation, e.g. due to numerical errors */
	UPROPERTY(EditAnywhere, Category = Boolean)
	bool bTryFixHoles = false;

	/** Try to collapse extra edges created by the Boolean operation */
	UPROPERTY(EditAnywhere, Category = Boolean)
	bool bTryCollapseEdges = true;

	/** Threshold to determine whether a triangle in one mesh is inside or outside of the other */
	UPROPERTY(EditAnywhere, Category = Boolean, AdvancedDisplay, meta = (UIMin = "0", UIMax = "1"))
	float WindingThreshold = 0.5;

	/** Show boundary edges created by the Boolean operation, which might happen due to numerical errors */
	UPROPERTY(EditAnywhere, Category = Display)
	bool bShowNewBoundaries = true;

	/** If true, only the first mesh will keep its material assignments, and all other faces will have the first material assigned */
	UPROPERTY(EditAnywhere, Category = Materials)
	bool bUseFirstMeshMaterials = false;
};


/**
 * UTLLRN_CutMeshWithMeshTool cuts an input mesh into two pieces based on a second input mesh.
 * Essentially this just both a Boolean Subtract and a Boolean Intersection. However
 * doing those as two separate operations involves quite a few steps, so this Tool
 * does it in a single step and with some improved efficiency.
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_CutMeshWithMeshTool : public UTLLRN_BaseCreateFromSelectedTool
{
	GENERATED_BODY()

public:

	UTLLRN_CutMeshWithMeshTool() {}

protected:

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	virtual void ConvertInputsAndSetPreviewMaterials(bool bSetTLLRN_PreviewMesh = true) override;

	virtual void SetupProperties() override;
	virtual void SaveProperties() override;
	virtual void SetPreviewCallbacks() override;

	virtual FString GetCreatedAssetName() const override;
	virtual FText GetActionName() const override;

	// IDynamicMeshOperatorFactory API
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	void UpdateVisualization();

	UPROPERTY()
	TObjectPtr<UTLLRN_CutMeshWithMeshToolProperties> CutProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_PreviewMesh> IntersectTLLRN_PreviewMesh;

	TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe> OriginalTargetMesh;
	TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe> OriginalCuttingMesh;

	UPROPERTY()
	TObjectPtr<UTLLRN_LineSetComponent> DrawnLineSet;

	// for visualization of any errors in the currently-previewed CSG operation
	TArray<int> CreatedSubtractBoundaryEdges;
	TArray<int> CreatedIntersectBoundaryEdges;

	FDynamicMesh3 IntersectionMesh;
};




UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_CutMeshWithMeshToolBuilder : public UTLLRN_BaseCreateFromSelectedToolBuilder
{
	GENERATED_BODY()

public:
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
		return NewObject<UTLLRN_CutMeshWithMeshTool>(SceneState.ToolManager);
	}
};



