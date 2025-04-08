// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "DeformationOps/ExtrudeOp.h" // EPolyEditExtrudeMode
#include "GeometryBase.h"
#include "GroupTopology.h" // FGroupTopologySelection
#include "TLLRN_InteractiveToolActivity.h"
#include "FrameTypes.h"

#include "TLLRN_PolyEditExtrudeActivity.generated.h"

class UTLLRN_PolyEditActivityContext;
class UTLLRN_PolyEditTLLRN_PreviewMesh;
class UTLLRN_PlaneDistanceFromHitMechanic;
PREDECLARE_GEOMETRY(class FDynamicMesh3);

UENUM()
enum class ETLLRN_PolyEditExtrudeDirection
{
	SelectionNormal,
	WorldX,
	WorldY,
	WorldZ,
	LocalX,
	LocalY,
	LocalZ
};

UENUM()
enum class ETLLRN_PolyEditExtrudeDistanceMode
{
	/** Set distance by clicking in the viewport. */
	ClickInViewport,

	/** Set distance with an explicit numerical value, then explictly accept. */
	Fixed,

	//~ TODO: Add someday
	// Gizmo,
};


// There is a lot of overlap in the options for Extrude, Offset, and Push/Pull, and they map to
// the same op behind the scenes. However, we want to keep them as separate buttons to keep some
// amount of shallowness in the UI, to make it more likely that new users will find the setting
// they are looking for.
// A couple of settings are entirely replicated: namely, doing an offset or "extrude" with SelectedTriangleNormals
// or SelectedTriangleNormalsEven as the movement direction is actually equivalent. Properly speaking, these
// two should only be options under Offset, not Extrude, but we keep them as (non-default) options 
// in both because an "extrude along local normals" is a common operation that some users are likely
// to look for under extrude, regardless of it not lining up with the physical meaning of extrusion.

// We use different property set objects so that we can customize category names, etc, as well as
// have different defaults and saved settings.

UENUM()
enum class ETLLRN_PolyEditExtrudeModeOptions
{
	// Extrude all triangles in the same direction regardless of their facing.
	SingleDirection = static_cast<int>(UE::Geometry::FExtrudeOp::EDirectionMode::SingleDirection),

	// Take the angle-weighed average of the selected triangles around each
	// extruded vertex to determine vertex movement direction.
	SelectedTriangleNormals = static_cast<int>(UE::Geometry::FExtrudeOp::EDirectionMode::SelectedTriangleNormals),

	// Like Selected Triangle Normals, but also adjusts the distances moved in
	// an attempt to keep triangles parallel to their original facing.
	SelectedTriangleNormalsEven = static_cast<int>(UE::Geometry::FExtrudeOp::EDirectionMode::SelectedTriangleNormalsEven),
};

UENUM()
enum class ETLLRN_PolyEditOffsetModeOptions
{
	// Vertex normals, regardless of selection.
	VertexNormals = static_cast<int>(UE::Geometry::FExtrudeOp::EDirectionMode::VertexNormals),

	// Take the angle-weighed average of the selected triangles around
	// offset vertex to determine vertex movement direction.
	SelectedTriangleNormals = static_cast<int>(UE::Geometry::FExtrudeOp::EDirectionMode::SelectedTriangleNormals),

	// Like Selected Triangle Normals, but also adjusts the distances moved in
	// an attempt to keep triangles parallel to their original facing.
	SelectedTriangleNormalsEven = static_cast<int>(UE::Geometry::FExtrudeOp::EDirectionMode::SelectedTriangleNormalsEven),
};

UENUM()
enum class ETLLRN_PolyEditPushPullModeOptions
{
	// Take the angle-weighed average of the selected triangles around
	// offset vertex to determine vertex movement direction.
	SelectedTriangleNormals = static_cast<int>(ETLLRN_PolyEditOffsetModeOptions::SelectedTriangleNormals),

	// Like Selected Triangle Normals, but also adjusts the distances moved in
	// an attempt to keep triangles parallel to their original facing.
	SelectedTriangleNormalsEven = static_cast<int>(UE::Geometry::FExtrudeOp::EDirectionMode::SelectedTriangleNormalsEven),

	// Move all triangles in the same direction regardless of their facing.
	SingleDirection = static_cast<int>(UE::Geometry::FExtrudeOp::EDirectionMode::SingleDirection),

	// Vertex normals, regardless of selection.
	VertexNormals = static_cast<int>(UE::Geometry::FExtrudeOp::EDirectionMode::VertexNormals),
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditExtrudeProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** How the extrude distance is set. */
	UPROPERTY(EditAnywhere, Category = Extrude)
	ETLLRN_PolyEditExtrudeDistanceMode DistanceMode = ETLLRN_PolyEditExtrudeDistanceMode::ClickInViewport;

	/** Distance to extrude. */
	UPROPERTY(EditAnywhere, Category = Extrude, meta = (UIMin = "-1000", UIMax = "1000", ClampMin = "-10000", ClampMax = "10000",
		EditConditionHides, EditCondition = "DistanceMode == ETLLRN_PolyEditExtrudeDistanceMode::Fixed"))
	double Distance = 100;

	/** How to move the vertices during the extrude */
	UPROPERTY(EditAnywhere, Category = Extrude)
	ETLLRN_PolyEditExtrudeModeOptions DirectionMode = ETLLRN_PolyEditExtrudeModeOptions::SingleDirection;

	/** Direction in which to extrude. */
	UPROPERTY(EditAnywhere, Category = Extrude,
		meta = (EditConditionHides, EditCondition = "DirectionMode == ETLLRN_PolyEditExtrudeModeOptions::SingleDirection"))
	ETLLRN_PolyEditExtrudeDirection Direction = ETLLRN_PolyEditExtrudeDirection::SelectionNormal;

	/** Controls the maximum distance vertices can move from the target distance in order to stay parallel with their source triangles. */
	UPROPERTY(EditAnywhere, Category = Extrude,
		meta = (ClampMin = "1", EditConditionHides, EditCondition = "DirectionMode == ETLLRN_PolyEditExtrudeModeOptions::SelectedTriangleNormalsEven"))
	double MaxDistanceScaleFactor = 4.0;

	/** Controls whether extruding an entire open-border patch should create a solid or an open shell */
	UPROPERTY(EditAnywhere, Category = Extrude)
	bool bShellsToSolids = true;
	
	/** What axis to measure the extrusion distance along. */
	UPROPERTY(EditAnywhere, Category = Extrude, AdvancedDisplay, meta = (EditConditionHides, 
		EditCondition = "DirectionMode != ETLLRN_PolyEditExtrudeModeOptions::SingleDirection && DistanceMode == ETLLRN_PolyEditExtrudeDistanceMode::ClickInViewport"))
	ETLLRN_PolyEditExtrudeDirection MeasureDirection = ETLLRN_PolyEditExtrudeDirection::SelectionNormal;

	/** 
	 * When extruding regions that touch the mesh border, assign the side groups (groups on the 
	 * stitched side of the extrude) in a way that considers edge colinearity. For instance, when
	 * true, extruding a flat rectangle will give four different groups on its sides rather than
	 * one connected group.
	 */
	UPROPERTY(EditAnywhere, Category = Extrude, AdvancedDisplay)
	bool bUseColinearityForSettingBorderGroups = true;
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditOffsetProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** How the offset distance is set. */
	UPROPERTY(EditAnywhere, Category = Offset)
	ETLLRN_PolyEditExtrudeDistanceMode DistanceMode = ETLLRN_PolyEditExtrudeDistanceMode::ClickInViewport;

	/** Offset distance. */
	UPROPERTY(EditAnywhere, Category = Offset,
		meta = (EditConditionHides, EditCondition = "DistanceMode == ETLLRN_PolyEditExtrudeDistanceMode::Fixed"))
	double Distance = 100;

	/** Which way to move vertices during the offset */
	UPROPERTY(EditAnywhere, Category = Offset)
	ETLLRN_PolyEditOffsetModeOptions DirectionMode = ETLLRN_PolyEditOffsetModeOptions::VertexNormals;

	/** Controls the maximum distance vertices can move from the target distance in order to stay parallel with their source triangles. */
	UPROPERTY(EditAnywhere, Category = Offset,
		meta = (ClampMin = "1", EditConditionHides, EditCondition = "DirectionMode == ETLLRN_PolyEditOffsetModeOptions::SelectedTriangleNormalsEven"))
	double MaxDistanceScaleFactor = 4.0;

	/** Controls whether offsetting an entire open-border patch should create a solid or an open shell */
	UPROPERTY(EditAnywhere, Category = Offset)
	bool bShellsToSolids = true;

	/** What axis to measure the offset distance along. */
	UPROPERTY(EditAnywhere, Category = Offset, AdvancedDisplay, meta = (EditConditionHides,
		EditCondition = "DistanceMode == ETLLRN_PolyEditExtrudeDistanceMode::ClickInViewport"))
	ETLLRN_PolyEditExtrudeDirection MeasureDirection = ETLLRN_PolyEditExtrudeDirection::SelectionNormal;

	/**
	 * When offsetting regions that touch the mesh border, assign the side groups (groups on the
	 * stitched side of the offset) in a way that considers edge colinearity. For instance, when
	 * true, extruding a flat rectangle will give four different groups on its sides rather than
	 * one connected group.
	 */
	UPROPERTY(EditAnywhere, Category = Offset, AdvancedDisplay)
	bool bUseColinearityForSettingBorderGroups = true;
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditPushPullProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** How the extrusion distance is set. */
	UPROPERTY(EditAnywhere, Category = ExtrusionOptions)
	ETLLRN_PolyEditExtrudeDistanceMode DistanceMode = ETLLRN_PolyEditExtrudeDistanceMode::ClickInViewport;

	/** Extrusion distance. */
	UPROPERTY(EditAnywhere, Category = ExtrusionOptions,
		meta = (EditConditionHides, EditCondition = "DistanceMode == ETLLRN_PolyEditExtrudeDistanceMode::Fixed"))
	double Distance = 100;

	/** Which way to move vertices while extruding. */
	UPROPERTY(EditAnywhere, Category = ExtrusionOptions)
	ETLLRN_PolyEditPushPullModeOptions DirectionMode = ETLLRN_PolyEditPushPullModeOptions::SelectedTriangleNormals;

	/** Direction in which to extrude. */
	UPROPERTY(EditAnywhere, Category = ExtrusionOptions,
		meta = (DisplayName="Direction", EditConditionHides, EditCondition = "DirectionMode == ETLLRN_PolyEditPushPullModeOptions::SingleDirection"))
	ETLLRN_PolyEditExtrudeDirection SingleDirection = ETLLRN_PolyEditExtrudeDirection::SelectionNormal;

	/** Controls the maximum distance vertices can move from the target distance in order to stay parallel with their source triangles. */
	UPROPERTY(EditAnywhere, Category = ExtrusionOptions,
		meta = (ClampMin = "1", EditConditionHides, EditCondition = "DirectionMode == ETLLRN_PolyEditPushPullModeOptions::SelectedTriangleNormalsEven"))
	double MaxDistanceScaleFactor = 4.0;

	/** Controls whether offsetting an entire open-border patch should create a solid or an open shell */
	UPROPERTY(EditAnywhere, Category = ExtrusionOptions)
	bool bShellsToSolids = true;

	/** What axis to measure the extrusion distance along. */
	UPROPERTY(EditAnywhere, Category = ExtrusionOptions, AdvancedDisplay, meta = (EditConditionHides, 
		EditCondition = "DirectionMode != ETLLRN_PolyEditPushPullModeOptions::SingleDirection && DistanceMode == ETLLRN_PolyEditExtrudeDistanceMode::ClickInViewport"))
	ETLLRN_PolyEditExtrudeDirection MeasureDirection = ETLLRN_PolyEditExtrudeDirection::SelectionNormal;

	/**
	 * When offsetting regions that touch the mesh border, assign the side groups (groups on the
	 * stitched side of the extrude) in a way that considers edge colinearity. For instance, when
	 * true, extruding a flat rectangle will give four different groups on its sides rather than
	 * one connected group.
	 */
	UPROPERTY(EditAnywhere, Category = ExtrusionOptions, AdvancedDisplay)
	bool bUseColinearityForSettingBorderGroups = true;
};

/**
 * 
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditExtrudeActivity : public UTLLRN_InteractiveToolActivity,
	public UE::Geometry::IDynamicMeshOperatorFactory,
	public IClickBehaviorTarget, public IHoverBehaviorTarget

{
	GENERATED_BODY()

public:
	using FExtrudeOp = UE::Geometry::FExtrudeOp;

	enum class EPropertySetToUse
	{
		Extrude,
		Offset,
		PushPull
	};

	// Set to different values depending on whether we're using this activity on behalf of
	// extrude, offset, or push/pull
	FExtrudeOp::EExtrudeMode ExtrudeMode = FExtrudeOp::EExtrudeMode::MoveAndStitch;
	EPropertySetToUse PropertySetToUse = EPropertySetToUse::Extrude;

	// ITLLRN_InteractiveToolActivity
	virtual void Setup(UInteractiveTool* ParentTool) override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual bool CanStart() const override;
	virtual EToolActivityStartResult Start() override;
	virtual bool IsRunning() const override { return bIsRunning; }
	virtual bool HasAccept() const { return true; };
	virtual bool CanAccept() const override;
	virtual EToolActivityEndResult End(EToolShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void Tick(float DeltaTime) override;

	// IDynamicMeshOperatorFactory
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	// IClickBehaviorTarget API
	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos) override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;

	// IHoverBehaviorTarget implementation
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override {}
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override {}

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditExtrudeProperties> ExtrudeProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditOffsetProperties> OffsetProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditPushPullProperties> PushPullProperties = nullptr;

protected:

	FVector3d GetExtrudeDirection() const;
	virtual void BeginExtrude();
	virtual void ApplyExtrude();
	virtual void ReinitializeExtrudeHeightMechanic();
	virtual void EndInternal();

	UPROPERTY()
	TObjectPtr<UTLLRN_PlaneDistanceFromHitMechanic> ExtrudeHeightMechanic = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditActivityContext> ActivityContext;

	TSharedPtr<UE::Geometry::FDynamicMesh3> PatchMesh;
	TArray<int32> NewSelectionGids;

	bool bIsRunning = false;

	UE::Geometry::FGroupTopologySelection ActiveSelection;
	UE::Geometry::FFrame3d ActiveSelectionFrameWorld;
	float UVScaleFactor = 1.0f;

	bool bRequestedApply = false;
};
