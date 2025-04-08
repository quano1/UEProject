// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/TLLRN_SingleSelectionMeshEditingTool.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "TLLRN_MeshOpPreviewHelpers.h"
#include "Properties/TLLRN_MeshMaterialProperties.h"
#include "Properties/TLLRN_MeshUVChannelProperties.h"
#include "Properties/TLLRN_RecomputeUVsProperties.h"
#include "PropertySets/TLLRN_PolygroupLayersProperties.h"
#include "Polygroups/PolygroupSet.h"
#include "Drawing/TLLRN_UVLayoutPreview.h"

#include "TLLRN_RecomputeUVsTool.generated.h"


// Forward declarations
class UDynamicMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class TLLRN_RecomputeUVsOp;
class UTLLRN_RecomputeUVsOpFactory;


/**
 *
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_RecomputeUVsToolBuilder : public UTLLRN_SingleSelectionMeshEditingToolBuilder
{
	GENERATED_BODY()

public:
	virtual UTLLRN_SingleSelectionMeshEditingTool* CreateNewTool(const FToolBuilderState& SceneState) const override;
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
};


/**
 * UTLLRN_RecomputeUVsTool Recomputes UVs based on existing segmentations of the mesh
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_RecomputeUVsTool : public UTLLRN_SingleSelectionMeshEditingTool, public IInteractiveToolManageGeometrySelectionAPI
{
	GENERATED_BODY()

public:
	virtual void Setup() override;
	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void OnTick(float DeltaTime) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	// IInteractiveToolManageGeometrySelectionAPI -- this tool won't update external geometry selection or change selection-relevant mesh IDs
	virtual bool IsInputSelectionValidOnOutput() override
	{
		return true;
	}

protected:
	UPROPERTY()
	TObjectPtr<UTLLRN_MeshUVChannelProperties> UVChannelProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_RecomputeUVsToolProperties> Settings = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolygroupLayersProperties> PolygroupLayerProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_ExistingTLLRN_MeshMaterialProperties> MaterialSettings = nullptr;

	UPROPERTY()
	bool bCreateUVLayoutViewOnSetup = true;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVLayoutPreview> UVLayoutView = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_RecomputeUVsOpFactory> TLLRN_RecomputeUVsOpFactory;

	UPROPERTY()
	TObjectPtr<UTLLRN_MeshOpPreviewWithBackgroundCompute> Preview = nullptr;

	TSharedPtr<UE::Geometry::FDynamicMesh3, ESPMode::ThreadSafe> InputMesh;

	TSharedPtr<UE::Geometry::FPolygroupSet, ESPMode::ThreadSafe> ActiveGroupSet;
	void OnSelectedGroupLayerChanged();
	void UpdateActiveGroupLayer();
	int32 GetSelectedUVChannel() const;
	void OnTLLRN_PreviewMeshUpdated();
};
