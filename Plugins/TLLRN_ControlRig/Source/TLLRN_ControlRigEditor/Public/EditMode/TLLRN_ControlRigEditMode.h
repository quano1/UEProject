// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "RigVMModel/RigVMGraph.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UObject/StrongObjectPtr.h"
#include "UnrealWidgetFwd.h"
#include "IPersonaEditMode.h"
#include "Misc/Guid.h"
#include "EditorDragTools.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "IDetailKeyframeHandler.h"
#include "WidgetFocusUtils.h"
#include "TLLRN_ControlRigEditMode.generated.h"

class FEditorViewportClient;
class FRigVMEditor;
class FViewport;
class UActorFactory;
struct FViewportClick;
class UTLLRN_ControlRig;
class FTLLRN_ControlRigInteractionScope;
class ISequencer;
class UControlManipulator;
class FUICommandList;
class FPrimitiveDrawInterface;
class FToolBarBuilder;
class FExtender;
class IMovieScenePlayer;
class ATLLRN_ControlRigShapeActor;
class UDefaultTLLRN_ControlRigManipulationLayer;
class UTLLRN_ControlRigDetailPanelControlProxies;
class UTLLRN_ControlTLLRN_RigControlsProxy;
struct FTLLRN_RigControl;
class ITLLRN_ControlRigManipulatable;
class ISequencer;
enum class ETLLRN_ControlRigSetKey : uint8;
class UToolMenu;
struct FGizmoState;
enum class EMovieSceneDataChangeType;
struct FMovieSceneChannelMetaData;
class UMovieSceneSection;
class UTLLRN_ControlRigEditModeSettings;

DECLARE_DELEGATE_RetVal_ThreeParams(FTransform, FOnGetTLLRN_RigElementTransform, const FTLLRN_RigElementKey& /*TLLRN_RigElementKey*/, bool /*bLocal*/, bool /*bOnDebugInstance*/);
DECLARE_DELEGATE_ThreeParams(FOnSetTLLRN_RigElementTransform, const FTLLRN_RigElementKey& /*TLLRN_RigElementKey*/, const FTransform& /*Transform*/, bool /*bLocal*/);
DECLARE_DELEGATE_RetVal(TSharedPtr<FUICommandList>, FNewMenuCommandsDelegate);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTLLRN_ControlRigAddedOrRemoved, UTLLRN_ControlRig*, bool /*true if added, false if removed*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTLLRN_ControlRigSelected, UTLLRN_ControlRig*, const FTLLRN_RigElementKey& /*TLLRN_RigElementKey*/,const bool /*bIsSelected*/);
DECLARE_DELEGATE_RetVal(UToolMenu*, FOnGetContextMenu);

class FTLLRN_ControlRigEditMode;

enum class ERecreateTLLRN_ControlRigShape
{
	RecreateNone,
	RecreateAll,
	RecreateSpecified
};


UCLASS()
class UTLLRN_ControlRigEditModeDelegateHelper : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void OnPoseInitialized();

	UFUNCTION()
	void PostPoseUpdate();

	void AddDelegates(USkeletalMeshComponent* InSkeletalMeshComponent);
	void RemoveDelegates();

	TWeakObjectPtr<USkeletalMeshComponent> BoundComponent;
	FTLLRN_ControlRigEditMode* EditMode = nullptr;

private:
	FDelegateHandle OnBoneTransformsFinalizedHandle;
};


struct FDetailKeyFrameCacheAndHandler: public IDetailKeyframeHandler
{
	FDetailKeyFrameCacheAndHandler() { UnsetDelegates(); }

	/** IDetailKeyframeHandler interface*/
	virtual bool IsPropertyKeyable(const UClass* InObjectClass, const class IPropertyHandle& PropertyHandle) const override;
	virtual bool IsPropertyKeyingEnabled() const override;
	virtual void OnKeyPropertyClicked(const IPropertyHandle& KeyedPropertyHandle) override;
	virtual bool IsPropertyAnimated(const class IPropertyHandle& PropertyHandle, UObject* ParentObject) const override;
	virtual EPropertyKeyedStatus GetPropertyKeyedStatus(const IPropertyHandle& PropertyHandle) const override;

	/** Delegates Resetting Cached Data */
	void OnGlobalTimeChanged();
	void OnMovieSceneDataChanged(EMovieSceneDataChangeType);
	void OnChannelChanged(const FMovieSceneChannelMetaData*, UMovieSceneSection*);

	void SetDelegates(TWeakPtr<ISequencer>& InWeakSequencer, FTLLRN_ControlRigEditMode* InEditMode);
	void UnsetDelegates();
	void ResetCachedData();
	void UpdateIfDirty();
	/** Map to the last calculated property keyed status. Resets when Scrubbing, changing Movie Scene Data, etc */
	mutable TMap<const IPropertyHandle*, EPropertyKeyedStatus> CachedPropertyKeyedStatusMap;

	/* flag to specify that we need to update values, will poll this on edit mode tick for performance */
	bool bValuesDirty = false;
private:
	TWeakPtr<ISequencer> WeakSequencer;
	FTLLRN_ControlRigEditMode* EditMode = nullptr;

};



class TLLRN_CONTROLRIGEDITOR_API FTLLRN_ControlRigEditMode : public IPersonaEditMode
{
public:
	static inline const FLazyName ModeName = FLazyName(TEXT("EditMode.TLLRN_ControlRig"));

	FTLLRN_ControlRigEditMode();
	~FTLLRN_ControlRigEditMode();

	/** Set the Control Rig Object to be active in the edit mode. You set both the Control Rig and a possible binding together with an optional Sequencer
	 This will remove all other control rigs present and should be called for stand alone editors, like the Control Rig Editor*/
	void SetObjects(UTLLRN_ControlRig* InTLLRN_ControlRig, UObject* BindingObject, TWeakPtr<ISequencer> InSequencer);

	/** Add a Control Rig object if it doesn't exist, will return true if it was added, false if it wasn't since it's already there. You can also set the Sequencer.*/
	bool AddTLLRN_ControlRigObject(UTLLRN_ControlRig* InTLLRN_ControlRig, const TWeakPtr<ISequencer>& InSequencer);

	/* Remove control rig */
	void RemoveTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig);

	/*Replace old Control Rig with the New Control Rig, perhaps from a recompile in the level editor*/
	void ReplaceTLLRN_ControlRig(UTLLRN_ControlRig* OldTLLRN_ControlRig, UTLLRN_ControlRig* NewTLLRN_ControlRig);

	/** This edit mode is re-used between the level editor and the asset editors (control rig editor etc.). Calling this indicates which context we are in */
	virtual bool IsInLevelEditor() const;
	/** This is used to differentiate between the control rig editor and any other (asset/level) editors in which this edit mode is used */
	virtual bool AreEditingTLLRN_ControlRigDirectly() const { return false; }

	// FEdMode interface
	virtual bool UsesToolkits() const override;
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool BeginTransform(const FGizmoState& InState) override;
	virtual bool EndTransform(const FGizmoState& InState) override;
	virtual bool ProcessCapturedMouseMoves(FEditorViewportClient* InViewportClient, FViewport* InViewport, const TArrayView<FIntPoint>& CapturedMouseMoves) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click) override;
	virtual bool BoxSelect(FBox& InBox, bool InSelect = true) override;
	virtual bool FrustumSelect(const FConvexVolume& InFrustum, FEditorViewportClient* InViewportClient, bool InSelect = true) override;
	virtual void SelectNone() override;
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;
	virtual bool UsesTransformWidget() const override;
	virtual bool GetPivotForOrbit(FVector& OutPivot) const override;
	virtual bool UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const override;
	virtual FVector GetWidgetLocation() const override;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& OutMatrix, void* InData) override;
	virtual bool GetCustomInputCoordinateSystem(FMatrix& OutMatrix, void* InData) override;
	virtual bool ShouldDrawWidget() const override;
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override;
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;
	virtual bool MouseEnter( FEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y ) override;
	virtual bool MouseLeave(FEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual void PostUndo() override;

	/* IPersonaEditMode interface */
	virtual bool GetCameraTarget(FSphere& OutTarget) const override { return false; }
	virtual class IPersonaPreviewScene& GetAnimPreviewScene() const override { check(false); return *(IPersonaPreviewScene*)this; }
	virtual void GetOnScreenDebugInfo(TArray<FText>& OutDebugInfo) const override {}

	/** FGCObject interface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;

	/** Refresh our internal object list (they may have changed) */
	void RefreshObjects();

	/** Find the edit mode corresponding to the specified world context */
	static FTLLRN_ControlRigEditMode* GetEditModeFromWorldContext(UWorld* InWorldContext);

	/** Bone Manipulation Delegates */
	FOnGetTLLRN_RigElementTransform& OnGetTLLRN_RigElementTransform() { return OnGetTLLRN_RigElementTransformDelegate; }
	FOnSetTLLRN_RigElementTransform& OnSetTLLRN_RigElementTransform() { return OnSetTLLRN_RigElementTransformDelegate; }

	/** Context Menu Delegates */
	FOnGetContextMenu& OnGetContextMenu() { return OnGetContextMenuDelegate; }
	FNewMenuCommandsDelegate& OnContextMenuCommands() { return OnContextMenuCommandsDelegate; }
	FSimpleMulticastDelegate& OnAnimSystemInitialized() { return OnAnimSystemInitializedDelegate; }

	/* Control Rig Changed Delegate*/
	FTLLRN_ControlRigAddedOrRemoved& OnTLLRN_ControlRigAddedOrRemoved() { return OnTLLRN_ControlRigAddedOrRemovedDelegate; }

	/* Control Rig Selected Delegate*/
	FTLLRN_ControlRigSelected& OnTLLRN_ControlRigSelected() { return OnTLLRN_ControlRigSelectedDelegate; }

	// callback that gets called when rig element is selected in other view
	void OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);
	void OnHierarchyModified_AnyThread(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);
	void OnControlModified(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlModifiedContext& Context);
	void OnPreConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName);
	void OnPostConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName);

	/** return true if it can be removed from preview scene 
	- this is to ensure preview scene doesn't remove shape actors */
	bool CanRemoveFromPreviewScene(const USceneComponent* InComponent);

	FUICommandList* GetCommandBindings() const { return CommandBindings.Get(); }

	/** Requests to recreate the shape actors in the next tick. Will recreate only the ones for the specified
	Control Rig, otherwise will recreate all of them*/
	void RequestToRecreateControlShapeActors(UTLLRN_ControlRig* TLLRN_ControlRig = nullptr); 

	static uint32 ValidControlTypeMask()
	{
		return FTLLRN_RigElementTypeHelper::ToMask(ETLLRN_RigElementType::Control);
	}

protected:

	// shape related functions wrt enable/selection
	/** Get the node name from the property path */
	ATLLRN_ControlRigShapeActor* GetControlShapeFromControlName(UTLLRN_ControlRig* InTLLRN_ControlRig,const FName& InControlName) const;

protected:
	/** Helper function: set TLLRN_ControlRigs array to the details panel */
	void SetObjects_Internal();

	/** Set up Details Panel based upon Selected Objects*/
	void SetUpDetailPanel();

	/** Updates cached pivot transforms */
	void UpdatePivotTransforms();
	bool ComputePivotFromEditedShape(UTLLRN_ControlRig* InTLLRN_ControlRig, FTransform& OutTransform) const;
	bool ComputePivotFromShapeActors(UTLLRN_ControlRig* InTLLRN_ControlRig, const bool bEachLocalSpace, const bool bIsParentSpace, FTransform& OutTransform) const;
	bool ComputePivotFromElements(UTLLRN_ControlRig* InTLLRN_ControlRig, FTransform& OutTransform) const;
	
	/** Get the current coordinate system space */
	ECoordSystem GetCoordSystemSpace() const;
	
	/** Helper function for box/frustum intersection */
	bool IntersectSelect(bool InSelect, const TFunctionRef<bool(const ATLLRN_ControlRigShapeActor*, const FTransform&)>& Intersects);

	/** Handle selection internally */
	void HandleSelectionChanged();

	/** Toggles visibility of acive control rig shapes in the viewport */
	void ToggleManipulators();

	/** Toggles visibility of all  control rig shapes in the viewport */
	void ToggleAllManipulators();

	/** Returns true if all control rig shapes are visible in the viewport */
	bool AreControlsVisible() const;

	/** If Anim Slider is open, got to the next tool*/
	void ChangeAnimSliderTool();

	/** If Anim Slider is open, then can drag*/
	bool CanChangeAnimSliderTool() const;
	/** Start Slider Tool*/
	void StartAnimSliderTool(int32 InX);
	/** If Anim Slider is open, drag the tool*/
	void DragAnimSliderTool(double Val);
	/** Reset and stop user the anim slider tool*/
	void ResetAnimSlider();

	/** If the Drag Anim Slider Tool is pressed*/
	bool IsDragAnimSliderToolPressed(FViewport* InViewport);

	bool HandleBeginTransform(const FEditorViewportClient* InViewportClient);
	bool HandleEndTransform(FEditorViewportClient* InViewportClient);

public:
	
	/** Clear Selection*/
	void ClearSelection();

	/** Frame to current Control Selection*/
	void FrameSelection();

	/** Frame a list of provided items*/
   	void FrameItems(const TArray<FTLLRN_RigElementKey>& InItems);

	/** Sets Passthrough Key on selected anim layers */
	void SetTLLRN_AnimLayerPassthroughKey();

	/** Opens up the space picker widget */
	void OpenSpacePickerWidget();

	/** Reset Transforms */
	void ZeroTransforms(bool bSelectionOnly);

	/** Invert Input Pose */
	void InvertInputPose(bool bSelectionOnly);

private:
	
	/** Whether or not we should Frame Selection or not*/
	bool CanFrameSelection();

	/** Increase Shape Size */
	void IncreaseShapeSize();

	/** Decrease Shape Size */
	void DecreaseShapeSize();

	/** Reset Shape Size */
	void ResetControlShapeSize();

	/** Pending focus handler */
	FPendingWidgetFocus PendingFocus;

	/** Pending focus cvar binding functions to enable/disable pending focus mode */
	void RegisterPendingFocusMode();
	void UnregisterPendingFocusMode();
	FDelegateHandle PendingFocusHandle;
	
public:
	
	/** Toggle Shape Transform Edit*/
	void ToggleControlShapeTransformEdit();

private:
	
	/** The hotkey text is passed to a viewport notification to inform users how to toggle shape edit*/
	FText GetToggleControlShapeTransformEditHotKey() const;

	/** Bind our keyboard commands */
	void BindCommands();

	/** It creates if it doesn't have it */
	void RecreateControlShapeActors();

	/** Let the preview scene know how we want to select components */
	bool ShapeSelectionOverride(const UPrimitiveComponent* InComponent) const;

	/** Enable editing of control's shape transform instead of control's transform*/
	bool bIsChangingControlShapeTransform;

protected:

	TWeakPtr<ISequencer> WeakSequencer;
	FGuid LastMovieSceneSig;


	/** The scope for the interaction, one per manipulated Control rig */
	TMap<UTLLRN_ControlRig*,FTLLRN_ControlRigInteractionScope*> InteractionScopes;

	/** True if there's tracking going on right now */
	bool bIsTracking;

	/** Whether a manipulator actually made a change when transacting */
	bool bManipulatorMadeChange;

	/** Guard value for selection */
	bool bSelecting;

	/** If selection was changed, we set up proxies on next tick */
	bool bSelectionChanged;

	/** Cached transform of pivot point for selected objects for each Control Rig */
	TMap<UTLLRN_ControlRig*,FTransform> PivotTransforms;

	/** Previous cached transforms, need this to check on tick if any transform changed, gizmo may have changed*/
	TMap<UTLLRN_ControlRig*, FTransform> LastPivotTransforms;

	/** Command bindings for keyboard shortcuts */
	TSharedPtr<FUICommandList> CommandBindings;

	/** Called from the editor when a blueprint object replacement has occurred */
	void OnObjectsReplaced(const TMap<UObject*, UObject*>& OldToNewInstanceMap);

	/** Return true if transform setter/getter delegates are available */
	bool IsTransformDelegateAvailable() const;

	FOnGetTLLRN_RigElementTransform OnGetTLLRN_RigElementTransformDelegate;
	FOnSetTLLRN_RigElementTransform OnSetTLLRN_RigElementTransformDelegate;
	FOnGetContextMenu OnGetContextMenuDelegate;
	FNewMenuCommandsDelegate OnContextMenuCommandsDelegate;
	FSimpleMulticastDelegate OnAnimSystemInitializedDelegate;
	FTLLRN_ControlRigAddedOrRemoved OnTLLRN_ControlRigAddedOrRemovedDelegate;
	FTLLRN_ControlRigSelected OnTLLRN_ControlRigSelectedDelegate;

	/** GetSelectedTLLRN_RigElements, if InTLLRN_ControlRig is nullptr get the first one */
	TArray<FTLLRN_RigElementKey> GetSelectedTLLRN_RigElements(UTLLRN_ControlRig* InTLLRN_ControlRig = nullptr) const;

	/* Flag to recreate shapes during tick */
	ERecreateTLLRN_ControlRigShape RecreateControlShapesRequired;
	/* List of Control Rigs we should recreate*/
	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigsToRecreate;

	/* Flag to temporarily disable handling notifs from the hierarchy */
	bool bSuspendHierarchyNotifs;

	/** Shape actors */
	TMap<UTLLRN_ControlRig*,TArray<TObjectPtr<ATLLRN_ControlRigShapeActor>>> TLLRN_ControlRigShapeActors;
	TObjectPtr<UTLLRN_ControlRigDetailPanelControlProxies> ControlProxy;

	/** Utility functions for UI/Some other viewport manipulation*/
	bool IsControlSelected() const;
	bool AreTLLRN_RigElementSelectedAndMovable(UTLLRN_ControlRig* InTLLRN_ControlRig) const;
	
	/** Set initial transform handlers */
	void OpenContextMenu(FEditorViewportClient* InViewportClient);

	/* params to handle mouse move for changing anim tools sliders*/
	bool bisTrackingAnimToolDrag = false;
	/** Starting X Value*/
	TOptional<int32> StartXValue;

	/** previous Gizmo(Widget) scale before we enter this mode, used to set it back*/
	float PreviousGizmoScale = 1.0f;
	
public: 
	/** Clear all selected TLLRN_RigElements */
	void ClearTLLRN_RigElementSelection(uint32 InTypes);

	/** Set a TLLRN_RigElement's selection state */
	void SetTLLRN_RigElementSelection(UTLLRN_ControlRig* TLLRN_ControlRig, ETLLRN_RigElementType Type, const FName& InTLLRN_RigElementName, bool bSelected);

	/** Set multiple TLLRN_RigElement's selection states */
	void SetTLLRN_RigElementSelection(UTLLRN_ControlRig* TLLRN_ControlRig, ETLLRN_RigElementType Type, const TArray<FName>& InTLLRN_RigElementNames, bool bSelected);

	/** Check if any TLLRN_RigElements are selected */
	bool AreTLLRN_RigElementsSelected(uint32 InTypes, UTLLRN_ControlRig* InTLLRN_ControlRig) const;

	/** Get the number of selected TLLRN_RigElements */
	int32 GetNumSelectedTLLRN_RigElements(uint32 InTypes, UTLLRN_ControlRig* InTLLRN_ControlRig) const;

	/** Get all of the selected Controls*/
	void GetAllSelectedControls(TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>>& OutSelectedControls) const;

	/** Get all of the TLLRN_ControlRigs, maybe not valid anymore */
	TArrayView<const TWeakObjectPtr<UTLLRN_ControlRig>> GetTLLRN_ControlRigs() const;
	TArrayView<TWeakObjectPtr<UTLLRN_ControlRig>> GetTLLRN_ControlRigs();
	/* Get valid  Control Rigs possibly just visible*/
	TArray<UTLLRN_ControlRig*> GetTLLRN_ControlRigsArray(bool bIsVisible);
	TArray<const UTLLRN_ControlRig*> GetTLLRN_ControlRigsArray(bool bIsVisible) const;

	/** Get the detail proxies control rig*/
	UTLLRN_ControlRigDetailPanelControlProxies* GetDetailProxies() { return ControlProxy; }

	/** Get Sequencer Driving This*/
	TWeakPtr<ISequencer> GetWeakSequencer() { return WeakSequencer; }

	/** Suspend Rig Hierarchy Notifies*/
	void SuspendHierarchyNotifs(bool bVal) { bSuspendHierarchyNotifs = bVal; }

	/** Request a certain transform widget for the next update */
	void RequestTransformWidgetMode(UE::Widget::EWidgetMode InWidgetMode);
	
	UTLLRN_ControlRigDetailPanelControlProxies* GetControlProxy() { return ControlProxy; }
private:
	/** Whether or not Pivot Transforms have changed, in which case we need to redraw viewport*/
	bool HasPivotTransformsChanged() const;
	/** Set a TLLRN_RigElement's selection state */
	void SetTLLRN_RigElementSelectionInternal(UTLLRN_ControlRig* TLLRN_ControlRig, ETLLRN_RigElementType Type, const FName& InTLLRN_RigElementName, bool bSelected);
	/** Updates the pivot transforms before ticking to ensure that they are up-to-date when needed. */
	void UpdatePivotTransformsIfNeeded(UTLLRN_ControlRig* InTLLRN_ControlRig, FTransform& InOutTransform) const;
	
	FEditorViewportClient* CurrentViewportClient;
	TArray<UE::Widget::EWidgetMode> RequestedWidgetModes;

/* store coordinate system per widget mode*/
private:
	void OnWidgetModeChanged(UE::Widget::EWidgetMode InWidgetMode);
	void OnCoordSystemChanged(ECoordSystem InCoordSystem);
	TArray<ECoordSystem> CoordSystemPerWidgetMode;
	bool bIsChangingCoordSystem;

	bool CanChangeControlShapeTransform();

	void OnSettingsChanged(const UTLLRN_ControlRigEditModeSettings* InSettings);
	
public:
	//Toolbar functions
	void SetOnlySelectTLLRN_RigControls(bool val);
	bool GetOnlySelectTLLRN_RigControls()const;
	bool SetSequencer(TWeakPtr<ISequencer> InSequencer);

private:
	TSet<FName> GetActiveControlsFromSequencer(UTLLRN_ControlRig* TLLRN_ControlRig);

	/** Create/Delete/Update shape actors for the specified TLLRN_ControlRig */
	void CreateShapeActors(UTLLRN_ControlRig* InTLLRN_ControlRig);
	void DestroyShapesActors(UTLLRN_ControlRig* InTLLRN_ControlRig);
	bool TryUpdatingControlsShapes(UTLLRN_ControlRig* InTLLRN_ControlRig);

	/*Internal function for adding TLLRN_ControlRig*/
	void AddTLLRN_ControlRigInternal(UTLLRN_ControlRig* InTLLRN_ControlRig);
	void TickManipulatableObjects(float DeltaTime);

	/* Check on tick to see if movie scene has changed, returns true if it has*/
	bool CheckMovieSceneSig();
	void SetControlShapeTransform( const ATLLRN_ControlRigShapeActor* InShapeActor, const FTransform& InGlobalTransform,
		const FTransform& InToWorldTransform, const FTLLRN_RigControlModifiedContext& InContext, const bool bPrintPython,
		const bool bFixEulerFlips = true) const;
	static FTransform GetControlShapeTransform(const ATLLRN_ControlRigShapeActor* ShapeActor);
	void MoveControlShape(ATLLRN_ControlRigShapeActor* ShapeActor, const bool bTranslation, FVector& InDrag, 
		const bool bRotation, FRotator& InRot, const bool bScale, FVector& InScale, const FTransform& ToWorldTransform,
		bool bUseLocal, bool bCalcLocal, FTransform& InOutLocal);

	void ChangeControlShapeTransform(ATLLRN_ControlRigShapeActor* ShapeActor, const bool bTranslation, FVector& InDrag,
		const bool bRotation, FRotator& InRot, const bool bScale, FVector& InScale, const FTransform& ToWorldTransform);

	void TickControlShape(ATLLRN_ControlRigShapeActor* ShapeActor, const FTransform& ComponentTransform) const;
	bool ModeSupportedByShapeActor(const ATLLRN_ControlRigShapeActor* ShapeActor, UE::Widget::EWidgetMode InMode) const;

public:
	//notify driven controls, should this be inside CR instead?
	static void NotifyDrivenControls(UTLLRN_ControlRig* InTLLRN_ControlRig, const FTLLRN_RigElementKey& InKey, const FTLLRN_RigControlModifiedContext& InContext);

protected:
	
	/** Get bindings to a runtime object */
	//If the passed in TLLRN_ControlRig is nullptr we use the first Control Rig(this can happen from the BP Editors).
	USceneComponent* GetHostingSceneComponent(const UTLLRN_ControlRig* TLLRN_ControlRig = nullptr) const;
	FTransform	GetHostingSceneComponentTransform(const UTLLRN_ControlRig* TLLRN_ControlRig =  nullptr) const;

	//Get if the hosted component is visible
	bool IsTLLRN_ControlRigSkelMeshVisible(const UTLLRN_ControlRig* InTLLRN_ControlRig) const;
	
public:  
		TSharedPtr<FDetailKeyFrameCacheAndHandler> DetailKeyFrameCache;

private:

	// Post pose update handler
	void OnPoseInitialized();
	void PostPoseUpdate() const;
	void UpdateSelectabilityOnSkeletalMeshes(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bEnabled);

	bool IsMovingCamera(const FViewport* InViewport) const;
	bool IsDoingDrag(const FViewport* InViewport) const;

	// world clean up handlers
	FDelegateHandle OnWorldCleanupHandle;
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	UWorld* WorldPtr = nullptr;

	void OnEditorClosed();

	struct FMarqueeDragTool
    {
    	FMarqueeDragTool();
    	~FMarqueeDragTool() {};
    
    	bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport);
    	bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport);
    	void MakeDragTool(FEditorViewportClient* InViewportClient);
    	bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
    
    	bool UsingDragTool() const;
    	void Render3DDragTool(const FSceneView* View, FPrimitiveDrawInterface* PDI);
    	void RenderDragTool(const FSceneView* View, FCanvas* Canvas);
    
    private:
    	
    	/**
    	 * If there is a dragging tool being used, this will point to it.
    	 * Gets newed/deleted in StartTracking/EndTracking.
    	 */
    	TSharedPtr<FDragTool> DragTool;
    
    	/** Tracks whether the drag tool is in the process of being deleted (to protect against reentrancy) */
    	bool bIsDeletingDragTool = false;

		FTLLRN_ControlRigEditMode* EditMode;
    };

	FMarqueeDragTool DragToolHandler;
	
	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> RuntimeTLLRN_ControlRigs;
	TMap<UTLLRN_ControlRig*,TStrongObjectPtr<UTLLRN_ControlRigEditModeDelegateHelper>> DelegateHelpers;

	TArray<FTLLRN_RigElementKey> DeferredItemsToFrame;

	/** Computes the current interaction types based on the widget mode */
	static uint8 GetInteractionType(const FEditorViewportClient* InViewportClient);
	uint8 InteractionType;
	bool bShowControlsAsOverlay;

	bool bPivotsNeedUpdate = true;

	bool bIsConstructionEventRunning;
	TArray<uint32> LastHierarchyHash;
	TArray<uint32> LastShapeLibraryHash;


	friend class FTLLRN_ControlRigEditorModule;
	friend class FTLLRN_ControlRigEditor;
	friend class FTLLRN_ControlRigEditModeGenericDetails;
	friend class UTLLRN_ControlRigEditModeDelegateHelper;
	friend class STLLRN_ControlRigEditModeTools;
};

namespace TLLRN_ControlRigEditMode::Shapes
{
	// Returns the list of controls for which a shape is expected
	void GetControlsEligibleForShapes(UTLLRN_ControlRig* InTLLRN_ControlRig, TArray<FTLLRN_RigControlElement*>& OutControls);

	// Destroys shape actors and removes them from their UWorld
	void DestroyShapesActorsFromWorld(const TArray<TObjectPtr<ATLLRN_ControlRigShapeActor>>& InShapeActorsToDestroy);

	// Parameters used to update shape actors (transform, visibility, etc.)
	struct FShapeUpdateParams
	{
		FShapeUpdateParams(const UTLLRN_ControlRig* InTLLRN_ControlRig, const FTransform& InComponentTransform, const bool InSkeletalMeshVisible);
		const UTLLRN_ControlRig* TLLRN_ControlRig = nullptr;
		const UTLLRN_RigHierarchy* Hierarchy = nullptr;
		const UTLLRN_ControlRigEditModeSettings* Settings = nullptr;
		const FTransform& ComponentTransform = FTransform::Identity;
		const bool bIsSkeletalMeshVisible = false;
		bool IsValid() const;
	};

	// Updates shape actors transform, visibility, etc.
	void UpdateControlShape(ATLLRN_ControlRigShapeActor* InShapeActor, FTLLRN_RigControlElement* InControlElement, const FShapeUpdateParams& InUpdateParams);
}