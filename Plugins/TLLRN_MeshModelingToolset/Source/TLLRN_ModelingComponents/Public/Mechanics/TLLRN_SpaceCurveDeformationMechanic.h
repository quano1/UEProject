// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "FrameTypes.h"
#include "InteractionMechanic.h"
#include "InteractiveToolChange.h"
#include "Spatial/GeometrySet3.h"
#include "ToolContextInterfaces.h" //FViewCameraState
#include "VectorTypes.h"

#include "TLLRN_SpaceCurveDeformationMechanic.generated.h"

class ATLLRN_PreviewGeometryActor;
class UTLLRN_LineSetComponent;
class UMouseHoverBehavior;
class UTLLRN_PointSetComponent;
class USingleClickInputBehavior;
class UCombinedTransformGizmo;
class UTransformProxy;




class TLLRN_MODELINGCOMPONENTS_API FSpaceCurveSource
{
public:
	TUniqueFunction<int32()> GetPointCount;
	TUniqueFunction<UE::Geometry::FFrame3d(int32)> GetPoint;
	TUniqueFunction<bool()> IsLoop;
};





UENUM()
enum class ETLLRN_SpaceCurveControlPointTransformMode
{
	Shared,
	PerVertex
};


UENUM()
enum class ETLLRN_SpaceCurveControlPointOriginMode
{
	Shared,
	First,
	Last
};

UENUM()
enum class ETLLRN_SpaceCurveControlPointFalloffType
{
	Linear,
	Smooth
};



UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_SpaceCurveDeformationMechanicPropertySet : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Options)
	ETLLRN_SpaceCurveControlPointTransformMode TransformMode = ETLLRN_SpaceCurveControlPointTransformMode::PerVertex;

	UPROPERTY(EditAnywhere, Category = Options)
	ETLLRN_SpaceCurveControlPointOriginMode TransformOrigin = ETLLRN_SpaceCurveControlPointOriginMode::First;

	UPROPERTY(EditAnywhere, Category = Options, meta = (UIMin = "0", UIMax = "1"))
	float Softness = 0.5;

	UPROPERTY(EditAnywhere, Category = Options)
	ETLLRN_SpaceCurveControlPointFalloffType SoftFalloff = ETLLRN_SpaceCurveControlPointFalloffType::Smooth;
};





/**

 */
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_SpaceCurveDeformationMechanic : public UInteractionMechanic, public IClickBehaviorTarget, public IHoverBehaviorTarget
{
	GENERATED_BODY()
public:

	// Behaviors used for moving points around and hovering them
	UPROPERTY()
	TObjectPtr<USingleClickInputBehavior> ClickBehavior = nullptr;
	UPROPERTY()
	TObjectPtr<UMouseHoverBehavior> HoverBehavior = nullptr;

	// This delegate is called every time the control point sequence is altered.
	DECLARE_MULTICAST_DELEGATE(OnPointsChangedEvent);
	OnPointsChangedEvent OnPointsChanged;


	void SetCurveSource(TSharedPtr<FSpaceCurveSource> CurveSource);
	void ClearCurveSource();

	void ClearSelection();
	void SelectionGrowToNext();
	void SelectionGrowToPrev();
	void SelectionGrowToEnd();
	void SelectionGrowToStart();
	void SelectionFill();
	void SelectionClear();

	void GetCurrentCurvePoints(TArray<UE::Geometry::FFrame3d>& PointsOut) { PointsOut = CurvePoints; }


	// Some other standard functions
	virtual ~UTLLRN_SpaceCurveDeformationMechanic();
	virtual void Setup(UInteractiveTool* ParentTool) override;
	virtual void Shutdown() override;
	void SetWorld(UWorld* World);
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void Tick(float DeltaTime) override;

	// IClickBehaviorTarget implementation
	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos) override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;

	// IHoverBehaviorTarget implementation
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override;
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override;

	// IModifierToggleBehaviorTarget implementation, inherited through IClickBehaviorTarget
	virtual void OnUpdateModifierState(int ModifierID, bool bIsOn) override;

public:
	UPROPERTY()
	TObjectPtr<UTLLRN_SpaceCurveDeformationMechanicPropertySet> TransformProperties;

protected:

	TSharedPtr<FSpaceCurveSource> CurveSource;
	TSharedPtr<FSpaceCurveSource> EmptyCurveSource;

	TArray<UE::Geometry::FFrame3d> CurvePoints;

	// Used for spatial queries
	UE::Geometry::FGeometrySet3 GeometrySet;

	bool bSpatialValid = false;
	void UpdateSpatial();

	/** Used for displaying points/segments */
	UPROPERTY()
	TObjectPtr<ATLLRN_PreviewGeometryActor> TLLRN_PreviewGeometryActor;
	UPROPERTY()
	TObjectPtr<UTLLRN_PointSetComponent> RenderPoints;
	UPROPERTY()
	TObjectPtr<UTLLRN_LineSetComponent> RenderSegments;

	bool bRenderGeometryValid = false;
	void UpdateRenderGeometry();

	// Variables for drawing
	FColor NormalCurveColor;
	FColor CurrentSegmentsColor;
	FColor CurrentPointsColor;
	float SegmentsThickness;
	float PointsSize;
	float DepthBias;
	FColor PreviewColor;
	FColor HoverColor;
	FColor SelectedColor;

	// Support for Shift and Ctrl toggles
	bool bAddToSelectionToggle = false;
	int32 ShiftModifierId = 1;
	int32 CtrlModifierId = 2;

	// Support for gizmo. Since the points aren't individual components, we don't actually use UTransformProxy
	// for the transform forwarding- we just use it for the callbacks.
	UPROPERTY()
	TObjectPtr<UTransformProxy> PointTransformProxy;
	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> PointTransformGizmo;

	// Used to make it easy to tell whether the gizmo was moved by the user or by undo/redo or
	// some other change that we shoulnd't respond to. Basing our movement undo/redo on the
	// gizmo turns out to be quite a pain, though may someday be easier if the transform proxy
	// is able to manage arbitrary objects.
	bool bGizmoBeingDragged = false;

	// Callbacks we'll receive from the gizmo proxy
	void GizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform);
	void GizmoTransformStarted(UTransformProxy* Proxy);
	void GizmoTransformEnded(UTransformProxy* Proxy);

	// Support for hovering
	FViewCameraState CameraState;
	TFunction<bool(const FVector3d&, const FVector3d&)> GeometrySetToleranceTest;
	int32 HoveredPointID = -1;
	void ClearHover();

	// Support for selection
	TArray<int32> SelectedPointIDs;
	// We need the selected point start positions so we can move multiple points appropriately.
	TArray<UE::Geometry::FFrame3d> CurveStartPositions;
	// The starting point of the gizmo is needed to determine the offset by which to move the points.
	UE::Geometry::FFrame3d GizmoStartPosition;

	// These issue undo/redo change objects, and must therefore not be called in undo/redo code.
	void ChangeSelection(int32 NewPointID, bool AddToSelection);

	// All of the following do not issue undo/redo change objects.
	bool HitTest(const FInputDeviceRay& ClickPos, FInputRayHit& ResultOut);
	void SelectPoint(int32 PointID);
	bool DeselectPoint(int32 PointID);
	void UpdateSelection(const TArray<int32>& NewSelection);
	void UpdateGizmoLocation();
	void UpdateCurve(const TArray<UE::Geometry::FFrame3d>& NewPositions);

	// Used for expiring undo/redo changes, which compare this to their stored value and expire themselves if they do not match.
	int32 CurrentChangeStamp = 0;

	friend class FTLLRN_SpaceCurveDeformationMechanicSelectionChange;
	friend class FTLLRN_SpaceCurveDeformationMechanicMovementChange;
};


// Undo/redo support:

class TLLRN_MODELINGCOMPONENTS_API FTLLRN_SpaceCurveDeformationMechanicSelectionChange : public FToolCommandChange
{
public:
	FTLLRN_SpaceCurveDeformationMechanicSelectionChange(const TArray<int32>& FromIDs, const TArray<int32>& ToIDs);
	virtual void Apply(UObject* Object) override;
	virtual void Revert(UObject* Object) override;
	virtual FString ToString() const override;

protected:
	TArray<int32> From, To;
};


class TLLRN_MODELINGCOMPONENTS_API FTLLRN_SpaceCurveDeformationMechanicMovementChange : public FToolCommandChange
{
public:
	FTLLRN_SpaceCurveDeformationMechanicMovementChange(const TArray<UE::Geometry::FFrame3d>& FromPositions, const TArray<UE::Geometry::FFrame3d>& ToPositions);

	virtual void Apply(UObject* Object) override;
	virtual void Revert(UObject* Object) override;
	virtual FString ToString() const override;

protected:
	TArray<UE::Geometry::FFrame3d> From;
	TArray<UE::Geometry::FFrame3d> To;
};
