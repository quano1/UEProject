// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/SingleSelectionMeshEditingTool.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "PropertySets/PolygroupLayersProperties.h"
#include "Polygroups/PolygroupSet.h"
#include "Drawing/UVLayoutPreview.h"
#include "TLLRN_UVEditorToolAnalyticsUtils.h"
#include "Selection/TLLRN_UVToolSelectionAPI.h"
#include "ParameterizationOps/TexelDensityOp.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"

#include "TLLRN_UVEditorTexelDensityTool.generated.h"


// Forward declarations
class UDynamicMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTLLRN_UVEditorToolMeshInput;
class UTLLRN_UVEditorUVTransformProperties;
class UUVTexelDensityOperatorFactory;
class UTLLRN_UVEditorTexelDensityTool;
class UTLLRN_UVTool2DViewportAPI;
class UPreviewGeometry;


/**
 *
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorTexelDensityToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	// This is a pointer so that it can be updated under the builder without
	// having to set it in the mode after initializing targets.
	const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>* Targets = nullptr;
};

UENUM()
enum class ETLLRN_TexelDensityToolAction
{
	NoAction,
	Processing,

	BeginSamping,
	Sampling
};


UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorTexelDensityActionSettings : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:

	TWeakObjectPtr<UTLLRN_UVEditorTexelDensityTool> ParentTool;

	void Initialize(UTLLRN_UVEditorTexelDensityTool* ParentToolIn) { ParentTool = ParentToolIn; }
	void PostAction(ETLLRN_TexelDensityToolAction Action);

	UFUNCTION(CallInEditor, Category = Actions)
	void SampleTexelDensity();
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorTexelDensityToolSettings : public UUVEditorTexelDensitySettings
{
	GENERATED_BODY()
public:

	TWeakObjectPtr<UTLLRN_UVEditorTexelDensityTool> ParentTool;
	void Initialize(UTLLRN_UVEditorTexelDensityTool* ParentToolIn) { ParentTool = ParentToolIn; }

	virtual bool InSamplingMode() const override;
};

/**
 * UTLLRN_UVEditorRecomputeUVsTool Recomputes UVs based on existing segmentations of the mesh
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorTexelDensityTool : public UInteractiveTool, public ITLLRN_UVToolSupportsSelection
{
	GENERATED_BODY()

public:
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;

	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;


	/**
	 * The tool will operate on the meshes given here.
	 */
	virtual void SetTargets(const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn)
	{
		Targets = TargetsIn;
	}


	void RequestAction(ETLLRN_TexelDensityToolAction ActionType);
	ETLLRN_TexelDensityToolAction ActiveAction() const;

private:
	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>> Targets;

	UPROPERTY()
	TObjectPtr<UUVEditorTexelDensitySettings> Settings = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorTexelDensityActionSettings> ActionSettings = nullptr;

	ETLLRN_TexelDensityToolAction PendingAction = ETLLRN_TexelDensityToolAction::NoAction;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolSelectionAPI> TLLRN_UVToolSelectionAPI = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolLivePreviewAPI> LivePreviewAPI = nullptr;

	//~ For UDIM information access
	UPROPERTY()
	TObjectPtr< UTLLRN_UVTool2DViewportAPI> TLLRN_UVTool2DViewportAPI = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolEmitChangeAPI> EmitChangeAPI = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UUVTexelDensityOperatorFactory>> Factories;

	int32 RemainingTargetsToProcess;
	void PerformBackgroundScalingTask();

	UPROPERTY()
	TObjectPtr<UInputBehaviorSet> LivePreviewBehaviorSet = nullptr;

	UPROPERTY()
	TObjectPtr<ULocalInputBehaviorSource> LivePreviewBehaviorSource = nullptr;

	UPROPERTY()
	TObjectPtr<UPreviewGeometry> UnwrapGeometry = nullptr;

	UPROPERTY()
	TObjectPtr<UPreviewGeometry> LivePreviewGeometry = nullptr;

	TWeakObjectPtr<UInputRouter> LivePreviewInputRouter = nullptr;

	TArray<TSharedPtr<UE::Geometry::FDynamicMeshAABBTree3>> Spatials2D; // 1:1 with targets
	TArray<TSharedPtr<UE::Geometry::FDynamicMeshAABBTree3>> Spatials3D; // 1:1 with targets

	FViewCameraState LivePreviewCameraState;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> TriangleSetMaterial = nullptr;

	TSharedPtr<UE::Geometry::FPolygroupSet, ESPMode::ThreadSafe> ActiveGroupSet;
	void OnSelectedGroupLayerChanged();
	void UpdateActiveGroupLayer(bool bUpdateFactories = true);

	int32 Get2DHitTriangle(const FRay& WorldRayIn, int32* IndexOf2DSpatialOut = nullptr);
	int32 Get3DHitTriangle(const FRay& WorldRayIn, int32* IndexOf2DSpatialOut = nullptr);

	void OnMeshTriangleClicked(int32 Tid, int32 IndexOfMesh, bool bTidIsFromUnwrap);
	void OnMeshTriangleHovered(int32 Tid, int32 IndexOfMesh, bool bTidIsFromUnwrap);
	void UpdateHover();
	/** @param bClearHoverInfo If true, also clears HoverVid, etc in addition to just clearing display. */
	void ClearHover(bool bClearHoverInfo = true);

	void ApplyClick();
	void UpdateToolMessage();

	// Used to remember click info to apply on tick
	int32 ClickedTid = IndexConstants::InvalidID;
	int32 ClickedMeshIndex = -1;
	bool bClickWasInUnwrap = false;

	// Used to remember hover info to apply on tick
	int32 HoverTid = IndexConstants::InvalidID;
	int32 HoverMeshIndex = IndexConstants::InvalidID;
	bool bHoverTidIsFromUnwrap = false;
	int32 LastHoverTid = IndexConstants::InvalidID;
	int32 LastHoverMeshIndex = IndexConstants::InvalidID;
	bool bLastHoverTidWasFromUnwrap = false;

	//
	// Analytics
	//

	UE::Geometry::TLLRN_UVEditorAnalytics::FTargetAnalytics InputTargetAnalytics;
	FDateTime ToolStartTimeAnalytics;
	void RecordAnalytics();


};
