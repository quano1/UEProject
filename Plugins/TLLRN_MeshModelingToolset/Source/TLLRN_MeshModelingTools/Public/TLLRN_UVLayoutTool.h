// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BaseTools/TLLRN_MultiSelectionMeshEditingTool.h"
#include "GeometryBase.h"
#include "InteractiveToolBuilder.h"
#include "TLLRN_MeshOpPreviewHelpers.h"
#include "ToolDataVisualizer.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Properties/TLLRN_MeshMaterialProperties.h"
#include "Properties/TLLRN_MeshUVChannelProperties.h"
#include "Drawing/TLLRN_UVLayoutPreview.h"

#include "TLLRN_UVLayoutTool.generated.h"


// Forward declarations
struct FMeshDescription;
class UDynamicMeshComponent;
class UTLLRN_UVLayoutProperties;
class UTLLRN_UVLayoutOperatorFactory;
PREDECLARE_GEOMETRY(class FDynamicMesh3);

/**
 *
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_UVLayoutToolBuilder : public UTLLRN_MultiSelectionMeshEditingToolBuilder
{
	GENERATED_BODY()

public:
	virtual UTLLRN_MultiSelectionMeshEditingTool* CreateNewTool(const FToolBuilderState& SceneState) const override;
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;

protected:
	virtual const FToolTargetTypeRequirements& GetTargetRequirements() const override;
};


/**
 * The level editor version of the UV layout tool.
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_UVLayoutTool : public UTLLRN_MultiSelectionMeshEditingTool, public IInteractiveToolManageGeometrySelectionAPI
{
	GENERATED_BODY()

public:

	UTLLRN_UVLayoutTool();

	virtual void Setup() override;
	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	int32 GetSelectedUVChannel() const;

	// IInteractiveToolManageGeometrySelectionAPI -- this tool won't update external geometry selection or change selection-relevant mesh IDs
	virtual bool IsInputSelectionValidOnOutput() override
	{
		return true;
	}

protected:

	UPROPERTY()
	TObjectPtr<UTLLRN_MeshUVChannelProperties> UVChannelProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVLayoutProperties> BasicProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_ExistingTLLRN_MeshMaterialProperties> MaterialSettings = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_MeshOpPreviewWithBackgroundCompute>> Previews;

	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_UVLayoutOperatorFactory>> Factories;

	TArray<TSharedPtr<UE::Geometry::FDynamicMesh3, ESPMode::ThreadSafe>> OriginalDynamicMeshes;

	FViewCameraState CameraState;

	void UpdateNumPreviews();

	void UpdateVisualization();
	
	void UpdatePreviewMaterial();

	void OnTLLRN_PreviewMeshUpdated(UTLLRN_MeshOpPreviewWithBackgroundCompute* Compute);

	void GenerateAsset(const TArray<FDynamicMeshOpResult>& Results);

	UPROPERTY()
	TObjectPtr<UTLLRN_UVLayoutPreview> UVLayoutView = nullptr;
};
