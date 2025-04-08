// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "TLLRN_ModelingOperators.h" //IDynamicMeshOperatorFactory
#include "InteractiveTool.h" //UInteractiveToolPropertySet
#include "TLLRN_InteractiveToolActivity.h"
#include "InteractiveToolBuilder.h" //UInteractiveToolBuilder
#include "InteractiveToolChange.h" //FToolCommandChange
#include "TLLRN_MeshOpPreviewHelpers.h" //FDynamicMeshOpResult
#include "Selection/MeshTopologySelector.h"
#include "SingleSelectionTool.h"
#include "ToolDataVisualizer.h"

#include "TLLRN_PolyEditInsertEdgeLoopActivity.generated.h"

PREDECLARE_USE_GEOMETRY_CLASS(FDynamicMeshChange);
class UTLLRN_PolyEditActivityContext;

UENUM()
enum class ETLLRN_EdgeLoopPositioningMode
{
	/** Edge loops will be evenly centered within a group. Allows for multiple insertions at a time. */
	Even,

	/** Edge loops will fall at the same length proportion at each edge they intersect (e.g., a quarter way down). */
	ProportionOffset,

	/** Edge loops will fall a constant distance away from the start of each edge they intersect
	 (e.g., 20 units down). Clamps to end if edge is too short. */
	 DistanceOffset
};

UENUM()
enum class ETLLRN_EdgeLoopInsertionMode
{
	/** Existing groups will be deleted and new triangles will be created for the new groups.
	 Keeps topology simple but breaks non-planar groups. */
	Retriangulate,

	/** Keeps existing triangles and cuts them to create a new path. May result in fragmented triangles over time.*/
	PlaneCut
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EdgeLoopInsertionProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** Determines how edge loops position themselves vertically relative to loop direction. */
	UPROPERTY(EditAnywhere, Category = InsertEdgeLoop)
	ETLLRN_EdgeLoopPositioningMode PositionMode = ETLLRN_EdgeLoopPositioningMode::ProportionOffset;

	/** Determines how edge loops are added to the geometry */
	UPROPERTY(EditAnywhere, Category = InsertEdgeLoop)
	ETLLRN_EdgeLoopInsertionMode InsertionMode = ETLLRN_EdgeLoopInsertionMode::PlaneCut;

	/** How many loops to insert at a time. Only used with "even" positioning mode. */
	UPROPERTY(EditAnywhere, Category = InsertEdgeLoop, meta = (UIMin = "0", UIMax = "20", ClampMin = "0", ClampMax = "500",
		EditCondition = "PositionMode == ETLLRN_EdgeLoopPositioningMode::Even", EditConditionHides))
	int32 NumLoops = 1;

	UPROPERTY(EditAnywhere, Category = InsertEdgeLoop, AdvancedDisplay, meta = (UIMin = "0", UIMax = "1.0", ClampMin = "0", ClampMax = "1.0",
		EditCondition = "PositionMode == ETLLRN_EdgeLoopPositioningMode::ProportionOffset && !bInteractive", EditConditionHides))
	double ProportionOffset = 0.5;

	UPROPERTY(EditAnywhere, Category = InsertEdgeLoop, AdvancedDisplay, meta = (UIMin = "0", ClampMin = "0",
		EditCondition = "PositionMode == ETLLRN_EdgeLoopPositioningMode::DistanceOffset && !bInteractive", EditConditionHides))
	double DistanceOffset = 10.0;

	/** When false, the distance/proportion offset is numerically specified, and mouse clicks just choose the edge. */
	UPROPERTY(EditAnywhere, Category = InsertEdgeLoop, AdvancedDisplay, meta = (
		EditCondition = "PositionMode != ETLLRN_EdgeLoopPositioningMode::Even", EditConditionHides))
	bool bInteractive = true;

	/** Measure the distance offset from the opposite side of the edges. */
	UPROPERTY(EditAnywhere, Category = InsertEdgeLoop, meta = (
		EditCondition = "PositionMode == ETLLRN_EdgeLoopPositioningMode::DistanceOffset", EditConditionHides))
	bool bFlipOffsetDirection = false;

	/** When true, non-quad-like groups that stop the loop will be highlighted, with X's marking the corners. */
	UPROPERTY(EditAnywhere, Category = InsertEdgeLoop)
	bool bHighlightProblemGroups = true;

	/** How close a new loop edge needs to pass next to an existing vertex to use that vertex rather than creating a new one. */
	UPROPERTY(EditAnywhere, Category = InsertEdgeLoop, AdvancedDisplay, meta = (UIMin = "0", UIMax = "0.01", ClampMin = "0", ClampMax = "10"))
	double VertexTolerance = 0.001;
};

/** Interactive activity for inserting (group) edge loops into a mesh. */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditInsertEdgeLoopActivity : public UTLLRN_InteractiveToolActivity, 
	public UE::Geometry::IDynamicMeshOperatorFactory,
	public IHoverBehaviorTarget, public IClickBehaviorTarget
{
	GENERATED_BODY()
public:

	friend class FEdgeLoopInsertionChange;

	UTLLRN_PolyEditInsertEdgeLoopActivity() {};

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property);

	// ITLLRN_InteractiveToolActivity
	virtual void Setup(UInteractiveTool* ParentTool) override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual bool CanStart() const override;
	virtual EToolActivityStartResult Start() override;
	virtual bool IsRunning() const override { return bIsRunning; }
	virtual bool CanAccept() const override;
	virtual EToolActivityEndResult End(EToolShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void Tick(float DeltaTime) override;

	// IDynamicMeshOperatorFactory
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	// IClickBehaviorTarget
	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos) override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;

	// IHoverBehaviorTarget
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override {}
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override;

protected:
	UPROPERTY()
	TObjectPtr<UTLLRN_EdgeLoopInsertionProperties> Settings = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditActivityContext> ActivityContext;

	bool bIsRunning = false;

	FTransform TargetTransform;
	TSharedPtr<FMeshTopologySelector, ESPMode::ThreadSafe> TopologySelector;

	TArray<TPair<FVector3d, FVector3d>> PreviewEdges;

	// Used to highlight problematic topology (non-quad groups) when it stops a loop.
	TArray<TPair<FVector3d, FVector3d>> ProblemTopologyEdges;
	TArray<FVector3d> ProblemTopologyVerts;

	FViewCameraState CameraState;

	FToolDataVisualizer PreviewEdgeRenderer;
	FToolDataVisualizer ProblemTopologyRenderer;
	FMeshTopologySelector::FSelectionSettings TopologySelectorSettings;
	float ProblemVertTickWidth = 8;

	void SetupPreview();

	FInputRayHit HitTest(const FRay& WorldRay);
	bool UpdateHoveredItem(const FRay& WorldRay);

	// Safe inputs for the background compute to use, untouched by undo/redo/other CurrentMesh updates.
	TSharedPtr<const UE::Geometry::FDynamicMesh3, ESPMode::ThreadSafe> ComputeStartMesh;
	TSharedPtr<const UE::Geometry::FGroupTopology, ESPMode::ThreadSafe> ComputeStartTopology;
	void UpdateComputeInputs();

	void ConditionallyUpdatePreview(int32 NewGroupID, double* NewInputLength = nullptr);
	void ClearPreview();

	// Taken from user interaction, read as inputs by the op factory
	int32 InputGroupEdgeID = FDynamicMesh3::InvalidID;
	double InteractiveInputLength = 0;

	// On valid clicks, we wait to finish the background op and apply it before taking more input.
	// Gets reset OnTick when the result is ready.
	bool bWaitingForInsertionCompletion = false;

	// Copied over on op completion
	bool bLastComputeSucceeded = false;
	TSharedPtr<UE::Geometry::FGroupTopology, ESPMode::ThreadSafe> LatestOpTopologyResult;
	TSharedPtr<TSet<int32>, ESPMode::ThreadSafe> LatestOpChangedTids;
};
