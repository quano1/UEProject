// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GeometryBase.h" // Predeclare macros

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "InteractionMechanic.h"
#include "InteractiveTool.h"
#include "Selection/UVToolSelection.h"
#include "Selection/TLLRN_UVToolSelectionAPI.h" // ETLLRN_UVEditorSelectionMode
#include "Mechanics/RectangleMarqueeMechanic.h"
#include "ToolContextInterfaces.h" //FViewCameraState

#include "TLLRN_UVEditorMeshSelectionMechanic.generated.h"

class APreviewGeometryActor;
struct FCameraRectangle;
class ULineSetComponent;
class UMaterialInstanceDynamic;
class UPointSetComponent;
class UTriangleSetComponent;
class UTLLRN_UVToolViewportButtonsAPI;
class ULocalSingleClickInputBehavior;
class ULocalMouseHoverBehavior;

/**
 * Mechanic for selecting elements of a dynamic mesh in the UV editor. Interacts
 * heavily with UTLLRN_UVToolSelectionAPI, which actually stores selections.
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorMeshSelectionMechanic : public UInteractionMechanic
{
	GENERATED_BODY()

public:
	using FDynamicMeshAABBTree3 = UE::Geometry::FDynamicMeshAABBTree3;
	using FUVToolSelection = UE::Geometry::FUVToolSelection;

	virtual ~UTLLRN_UVEditorMeshSelectionMechanic() {}

	virtual void Setup(UInteractiveTool* ParentTool) override;
	virtual void Shutdown() override;

	// Initialization functions.
	// The selection API is provided as a parameter rather than being grabbed out of the context
	// store mainly because TLLRN_UVToolSelectionAPI itself sets up a selection mechanic, and is not 
	// yet in the context store when it does this. 
	void Initialize(UWorld* World, UWorld* LivePreviewWorld, UTLLRN_UVToolSelectionAPI* SelectionAPI);
	void SetTargets(const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn);

	void SetIsEnabled(bool bIsEnabled);
	bool IsEnabled() { return bIsEnabled; };

	struct FRaycastResult
	{
		int32 AssetID = IndexConstants::InvalidID;
		int32 Tid = IndexConstants::InvalidID;
		FVector3d HitPosition;
	};
	/**
	 * This is a helper method that doesn't get used in normal selection mechanic operation, but
	 *  can be used by clients if they need to raycast canonical meshes, since the mechanic already
	 *  keeps aabb trees for them. The mechanic does not need to be enabled to use this (and typically
	 *  will not be, if a tool is doing its own raycasting).
	 */
	bool RaycastCanonicals(const FRay& WorldRay, bool bRaycastIsForUnwrap,
		bool bPreferSelected, FRaycastResult& HitOut) const;

	TArray<FUVToolSelection> GetAllCanonicalTrianglesInUnwrapRadius(const FVector2d& UnwrapWorldHitPoint, double Radius) const;

	void SetShowHoveredElements(bool bShow);

	using ESelectionMode = UTLLRN_UVToolSelectionAPI::ETLLRN_UVEditorSelectionMode;
	using FModeChangeOptions = UTLLRN_UVToolSelectionAPI::FSelectionMechanicModeChangeOptions;
	/**
	 * Sets selection mode for the mechanic.
	 */
	void SetSelectionMode(ESelectionMode TargetMode,
		const FModeChangeOptions& Options = FModeChangeOptions());
	
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void DrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI);
	virtual void LivePreviewRender(IToolsContextRenderAPI* RenderAPI);
	virtual void LivePreviewDrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI);
	// IClickBehaviorTarget implementation
	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos, bool bSourceIsLivePreview);
	virtual void OnClicked(const FInputDeviceRay& ClickPos, bool bSourceIsLivePreview);

	// IModifierToggleBehaviorTarget implementation
	virtual void OnUpdateModifierState(int ModifierID, bool bIsOn);

	// IHoverBehaviorTarget implementation
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos, bool bSourceIsLivePreview);
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos);
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos, bool bSourceIsLivePreview);
	virtual void OnEndHover();

	/**
	 * Broadcasted whenever the marquee mechanic rectangle is changed, since these changes
	 * don't trigger normal selection broadcasts.
	 */ 
	FSimpleMulticastDelegate OnDragSelectionChanged;

protected:

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolSelectionAPI> SelectionAPI = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolViewportButtonsAPI> ViewportButtonsAPI = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolEmitChangeAPI> EmitChangeAPI = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolLivePreviewAPI> LivePreviewAPI = nullptr;

	UPROPERTY()
	TObjectPtr<ULocalSingleClickInputBehavior> UnwrapClickTargetRouter = nullptr;

	UPROPERTY()
	TObjectPtr<ULocalSingleClickInputBehavior> LivePreviewClickTargetRouter = nullptr;

	UPROPERTY()
	TObjectPtr<ULocalMouseHoverBehavior> UnwrapHoverBehaviorTargetRouter = nullptr;

	UPROPERTY()
	TObjectPtr<ULocalMouseHoverBehavior> LivePreviewHoverBehaviorTargetRouter = nullptr;

	UPROPERTY()
	TObjectPtr<URectangleMarqueeMechanic> MarqueeMechanic = nullptr;
	
	UPROPERTY()
	TObjectPtr<URectangleMarqueeMechanic> LivePreviewMarqueeMechanic = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HoverTriangleSetMaterial = nullptr;

	UPROPERTY()
	TObjectPtr<APreviewGeometryActor> HoverGeometryActor = nullptr;
	// Weak pointers so that they go away when geometry actor is destroyed
	TWeakObjectPtr<UTriangleSetComponent> HoverTriangleSet = nullptr;
	TWeakObjectPtr<ULineSetComponent> HoverLineSet = nullptr;
	TWeakObjectPtr<UPointSetComponent> HoverPointSet = nullptr;

	UPROPERTY()
	TObjectPtr<UInputBehaviorSet> LivePreviewBehaviorSet = nullptr;

	UPROPERTY()
	TObjectPtr<ULocalInputBehaviorSource> LivePreviewBehaviorSource = nullptr;

	TWeakObjectPtr<UInputRouter> LivePreviewInputRouter = nullptr;

	UPROPERTY()
	TObjectPtr<APreviewGeometryActor> LivePreviewHoverGeometryActor = nullptr;
	// Weak pointers so that they go away when geometry actor is destroyed
	TWeakObjectPtr<UTriangleSetComponent> LivePreviewHoverTriangleSet = nullptr;
	TWeakObjectPtr<ULineSetComponent> LivePreviewHoverLineSet = nullptr;
	TWeakObjectPtr<UPointSetComponent> LivePreviewHoverPointSet = nullptr;

	// Should be the same as the mode-level targets array, indexed by AssetID
	TSharedPtr<FDynamicMeshAABBTree3> GetMeshSpatial(int32 TargetId, bool bUseUnwrap) const;
	TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>> Targets;
	TArray<TSharedPtr<FDynamicMeshAABBTree3>> UnwrapMeshSpatials; // 1:1 with Targets
	TArray<TSharedPtr<FDynamicMeshAABBTree3>> AppliedMeshSpatials; // 1:1 with Targets

	ESelectionMode SelectionMode;
	bool bIsEnabled = false;
	bool bShowHoveredElements = true;

	bool GetHitTid(const FInputDeviceRay& ClickPos, int32& TidOut,
		int32& AssetIDOut, bool bUseUnwrap, int32* ExistingSelectionObjectIndexOut = nullptr);
	void ModifyExistingSelection(TSet<int32>& SelectionSetToModify, const TArray<int32>& SelectedIDs);

	FViewCameraState CameraState;

	bool bShiftToggle = false;
	bool bCtrlToggle = false;
	static const int32 ShiftModifierID = 1;
	static const int32 CtrlModifierID = 2;

	// All four combinations of shift/ctrl down are assigned a behaviour
	bool ShouldAddToSelection() const { return !bCtrlToggle && bShiftToggle; }
	bool ShouldRemoveFromSelection() const { return bCtrlToggle && !bShiftToggle; }
	bool ShouldToggleFromSelection() const { return bCtrlToggle && bShiftToggle; }
	bool ShouldRestartSelection() const { return !bCtrlToggle && !bShiftToggle; }

	// For marquee mechanic
	void OnDragRectangleStarted();
	void OnDragRectangleChanged(const FCameraRectangle& CurrentRectangle, bool bSourceIsLivePreview);
	void OnDragRectangleFinished(const FCameraRectangle& Rectangle, bool bCancelled, bool bSourceIsLivePreview);

	TArray<FUVToolSelection> PreDragSelections;
	TArray<FUVToolSelection> PreDragUnsetSelections;
	// Maps asset id to a pre drag selection so that it is easy to tell which assets
	// started with a selection. 1:1 with Targets.
	TArray<const FUVToolSelection*> AssetIDToPreDragSelection;
	TArray<const FUVToolSelection*> AssetIDToPreDragUnsetSelection;
};

