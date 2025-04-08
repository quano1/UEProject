// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/TLLRN_SingleSelectionMeshEditingTool.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "TLLRN_MeshOpPreviewHelpers.h"
#include "Properties/TLLRN_MeshMaterialProperties.h"
#include "Properties/TLLRN_MeshUVChannelProperties.h"
#include "PropertySets/TLLRN_PolygroupLayersProperties.h"
#include "Drawing/TLLRN_UVLayoutPreview.h"

#include "TLLRN_ParameterizeMeshTool.generated.h"

// Forward declarations
class UDynamicMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTLLRN_ParameterizeMeshToolProperties;
class UTLLRN_ParameterizeMeshToolUVAtlasProperties;
class UTLLRN_ParameterizeMeshToolXAtlasProperties;
class UTLLRN_ParameterizeMeshToolPatchBuilderProperties;
class UTLLRN_ParameterizeMeshOperatorFactory;


UCLASS()
class TLLRN_MESHMODELINGTOOLSEDITORONLY_API UTLLRN_ParameterizeMeshToolBuilder : public UTLLRN_SingleSelectionMeshEditingToolBuilder
{
	GENERATED_BODY()
public:
	virtual UTLLRN_SingleSelectionMeshEditingTool* CreateNewTool(const FToolBuilderState& SceneState) const override;
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
};


/**
 * UTLLRN_ParameterizeMeshTool automatically decomposes the input mesh into charts, solves for UVs,
 * and then packs the resulting charts
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLSEDITORONLY_API UTLLRN_ParameterizeMeshTool : public UTLLRN_SingleSelectionMeshEditingTool, public IInteractiveToolManageGeometrySelectionAPI
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
	TObjectPtr<UTLLRN_ParameterizeMeshToolProperties> Settings = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_ParameterizeMeshToolUVAtlasProperties> UVAtlasProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_ParameterizeMeshToolXAtlasProperties> XAtlasProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_ParameterizeMeshToolPatchBuilderProperties> PatchBuilderProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolygroupLayersProperties> PolygroupLayerProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_ExistingTLLRN_MeshMaterialProperties> MaterialSettings = nullptr;

	UPROPERTY()
	bool bCreateUVLayoutViewOnSetup = true;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVLayoutPreview> UVLayoutView = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_MeshOpPreviewWithBackgroundCompute> Preview = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_ParameterizeMeshOperatorFactory> Factory;

	TSharedPtr<UE::Geometry::FDynamicMesh3, ESPMode::ThreadSafe> InputMesh;

	void OnMethodTypeChanged();

	void OnTLLRN_PreviewMeshUpdated();
};
