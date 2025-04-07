// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FrameTypes.h"
#include "GeometryBase.h"
#include "InteractiveTool.h"
#include "InteractiveToolBuilder.h"
#include "Selection/UVToolSelection.h"
#include "Selection/TLLRN_UVToolSelectionAPI.h" // ITLLRN_UVToolSupportsSelection
#include "InteractiveToolQueryInterfaces.h" // IInteractiveToolNestedAcceptCancelAPI

#include "TLLRN_UVSelectTool.generated.h"

class UCombinedTransformGizmo;
class UTransformProxy;
class UTLLRN_UVEditorToolMeshInput;
class UTLLRN_UVToolEmitChangeAPI;
class UTLLRN_UVToolViewportButtonsAPI;

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVSelectToolBuilder : public UInteractiveToolBuilder
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
 * The tool in the UV editor that secretly runs when other tools are not running. It uses the
 * selection api to allow the user to select elements, and has a gizmo that can be used to
 * transform these elements.
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVSelectTool : public UInteractiveTool, public IInteractiveToolNestedAcceptCancelAPI, public ITLLRN_UVToolSupportsSelection
{
	GENERATED_BODY()

public:
	
	/**
	 * The tool will operate on the meshes given here.
	 */
	virtual void SetTargets(const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn)
	{
		Targets = TargetsIn;
	}

	void SelectAll();

	// Used by undo/redo.
	FTransform GetGizmoTransform() const;
	void SetGizmoTransform(const FTransform& NewTransform);

	// UInteractiveTool
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void DrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI) override;
	virtual void OnTick(float DeltaTime) override;
	virtual bool HasCancel() const override { return false; }
	virtual bool HasAccept() const override { return false; }

	// IInteractiveToolNestedAcceptCancelAPI
	virtual bool SupportsNestedCancelCommand() override { return true; }
	virtual bool CanCurrentlyNestedCancel() override;
	virtual bool ExecuteNestedCancelCommand() override;

protected:
	virtual void OnSelectionChanged(bool bEmitChangeAllowed, uint32 SelectionChangeType);

	// Callbacks we'll receive from the gizmo proxy
	virtual void GizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform);
	virtual void GizmoTransformStarted(UTransformProxy* Proxy);
	virtual void GizmoTransformEnded(UTransformProxy* Proxy);

	virtual void ApplyGizmoTransform();
	virtual void UpdateGizmo(bool bForceRecomputeSelectionCenters = false);

	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>> Targets;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolViewportButtonsAPI> ViewportButtonsAPI = nullptr;
	
	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolEmitChangeAPI> EmitChangeAPI = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolSelectionAPI> SelectionAPI = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorMeshSelectionMechanic> SelectionMechanic = nullptr;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> TransformGizmo = nullptr;

	UE::Geometry::FFrame3d InitialGizmoFrame;
	FTransform UnappliedGizmoTransform;
	bool bInDrag = false;
	bool bUpdateGizmoOnCanonicalChange = true;
	bool bGizmoTransformNeedsApplication = false;

	TArray<UE::Geometry::FUVToolSelection> CurrentSelections;
	// The outer arrays are 1:1 with the Selections array obtained from the Selection API
	TArray<TArray<int32>> RenderUpdateTidsPerSelection;
	// Inner arrays for these two are 1:1 with each other
	TArray<TArray<int32>> MovingVidsPerSelection;
	TArray<TArray<FVector3d>> MovingVertOriginalPositionsPerSelection;

private:
	void ReinitializeFromSelection(bool bForceRecomputeSelectionCenters);
};

