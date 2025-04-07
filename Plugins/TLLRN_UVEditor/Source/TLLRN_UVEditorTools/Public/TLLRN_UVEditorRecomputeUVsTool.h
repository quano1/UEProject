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

#include "TLLRN_UVEditorRecomputeUVsTool.generated.h"


// Forward declarations
class UDynamicMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTLLRN_UVEditorToolMeshInput;
class URecomputeUVsToolProperties;
class URecomputeUVsOpFactory;

/**
 *
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorRecomputeUVsToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	// This is a pointer so that it can be updated under the builder without
	// having to set it in the mode after initializing targets.
	const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>* Targets = nullptr;
};


/**
 * UTLLRN_UVEditorRecomputeUVsTool Recomputes UVs based on existing segmentations of the mesh
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorRecomputeUVsTool : public UInteractiveTool, public ITLLRN_UVToolSupportsSelection
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

protected:
	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>> Targets;

	UPROPERTY()
	TObjectPtr<URecomputeUVsToolProperties> Settings = nullptr;

	UPROPERTY()
	TObjectPtr<UPolygroupLayersProperties> PolygroupLayerProperties = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<URecomputeUVsOpFactory>> Factories;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolSelectionAPI> TLLRN_UVToolSelectionAPI = nullptr;


	TSharedPtr<UE::Geometry::FPolygroupSet, ESPMode::ThreadSafe> ActiveGroupSet;
	void OnSelectedGroupLayerChanged();
	void UpdateActiveGroupLayer(bool bUpdateFactories = true);

	//
	// Analytics
	//
	
	UE::Geometry::TLLRN_UVEditorAnalytics::FTargetAnalytics InputTargetAnalytics;
	FDateTime ToolStartTimeAnalytics;
	void RecordAnalytics();
};
