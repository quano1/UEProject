// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/TLLRN_SingleTargetWithSelectionTool.h"
#include "TLLRN_MeshOpPreviewHelpers.h"
#include "ToolDataVisualizer.h"
#include "ParameterizationOps/TLLRN_UVProjectionOp.h"
#include "Properties/TLLRN_MeshMaterialProperties.h"
#include "Properties/TLLRN_MeshUVChannelProperties.h"
#include "Drawing/TLLRN_PreviewGeometryActor.h"
#include "Selection/SelectClickedAction.h"
#include "OrientedBoxTypes.h"

#include "TLLRN_UVProjectionTool.generated.h"


// Forward declarations
struct FMeshDescription;
class UDynamicMeshComponent;
class UCombinedTransformGizmo;
class UTransformProxy;
class USingleClickInputBehavior;
class UTLLRN_UVProjectionTool;

/**
 *
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_UVProjectionToolBuilder : public UTLLRN_SingleTargetWithSelectionToolBuilder
{
	GENERATED_BODY()
public:
	virtual UTLLRN_SingleTargetWithSelectionTool* CreateNewTool(const FToolBuilderState& SceneState) const override;

	virtual bool RequiresInputSelection() const override { return false; }
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
};


UENUM()
enum class ETLLRN_UVProjectionToolActions
{
	NoAction,
	AutoFit,
	AutoFitAlign,
	Reset
};


UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_UVProjectionToolEditActions : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UTLLRN_UVProjectionTool> ParentTool;

	void Initialize(UTLLRN_UVProjectionTool* ParentToolIn) { ParentTool = ParentToolIn; }
	void PostAction(ETLLRN_UVProjectionToolActions Action);

	/** Automatically fit the UV Projection Dimensions based on the current projection orientation */
	UFUNCTION(CallInEditor, Category = Actions, meta = (DisplayName = "AutoFit", DisplayPriority = 1))
	void AutoFit()
	{
		PostAction(ETLLRN_UVProjectionToolActions::AutoFit);
	}

	/** Automatically orient the projection and then automatically fit the UV Projection Dimensions */
	UFUNCTION(CallInEditor, Category = Actions, meta = (DisplayName = "AutoFitAlign", DisplayPriority = 2))
	void AutoFitAlign()
	{
		PostAction(ETLLRN_UVProjectionToolActions::AutoFitAlign);
	}

	/** Re-initialize the projection based on the UV Projection Initialization property */
	UFUNCTION(CallInEditor, Category = Actions, meta = (DisplayName = "Reset", DisplayPriority = 3))
	void Reset()
	{
		PostAction(ETLLRN_UVProjectionToolActions::Reset);
	}
};


UENUM()
enum class ETLLRN_UVProjectionToolInitializationMode
{
	/** Initialize projection to bounding box center */
	Default,
	/** Initialize projection based on previous usage of the Project tool */
	UsePrevious,
	/** Initialize projection using Auto Fitting for the initial projection type */
	AutoFit,
	/** Initialize projection using Auto Fitting with Alignment for the initial projection type */
	AutoFitAlign
};


/**
 * Standard properties
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_UVProjectionToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** Shape and/or algorithm to use for UV projection */
	UPROPERTY(EditAnywhere, Category = "UV Projection")
	ETLLRN_UVProjectionMethod ProjectionType = ETLLRN_UVProjectionMethod::Plane;

	/** Width, length, and height of the projection shape before rotation */
	UPROPERTY(EditAnywhere, Category = "UV Projection", meta = (Delta = 0.5, LinearDeltaSensitivity = 1))
	FVector Dimensions = FVector(100.0f, 100.0f, 100.0f);

	/** If true, changes to Dimensions result in all components be changed proportionally */
	UPROPERTY(EditAnywhere, Category = "UV Projection")
	bool bProportionalDimensions = false;

	/** Determines how projection settings will be initialized; this only takes effect if the projection shape dimensions or position are unchanged */
	UPROPERTY(EditAnywhere, Category = "UV Projection")
	ETLLRN_UVProjectionToolInitializationMode Initialization = ETLLRN_UVProjectionToolInitializationMode::Default;

	//
	// Cylinder projection options
	//

	/** Angle in degrees to determine whether faces should be assigned to the cylinder or the flat end caps */
	UPROPERTY(EditAnywhere, Category = CylinderProjection, meta = (DisplayName = "Split Angle", UIMin = "0", UIMax = "90",
		EditCondition = "ProjectionType == ETLLRN_UVProjectionMethod::Cylinder", EditConditionHides))
	float CylinderSplitAngle = 45.0f;

	//
	// ExpMap projection options
	//

	/** Blend between surface normals and projection normal; ExpMap projection becomes Plane projection when this value is 1 */
	UPROPERTY(EditAnywhere, Category = "ExpMap Projection", meta = (DisplayName = "Normal Blending", UIMin = "0", UIMax = "1",
		EditCondition = "ProjectionType == ETLLRN_UVProjectionMethod::ExpMap", EditConditionHides))
	float ExpMapNormalBlending = 0.0f;

	/** Number of smoothing steps to apply; this slightly increases distortion but produces more stable results. */
	UPROPERTY(EditAnywhere, Category = "ExpMap Projection", meta = (DisplayName = "Smoothing Steps", UIMin = "0", UIMax = "100",
		EditCondition = "ProjectionType == ETLLRN_UVProjectionMethod::ExpMap", EditConditionHides))
	int ExpMapSmoothingSteps = 0;

	/** Smoothing parameter; larger values result in faster smoothing in each step. */
	UPROPERTY(EditAnywhere, Category = "ExpMap Projection", meta = (DisplayName = "Smoothing Alpha", UIMin = "0", UIMax = "1",
		EditCondition = "ProjectionType == ETLLRN_UVProjectionMethod::ExpMap", EditConditionHides))
	float ExpMapSmoothingAlpha = 0.25f;

	//
	// UV-space transform options
	//

	/** Rotation in degrees applied after computing projection */
	UPROPERTY(EditAnywhere, Category = "UV Transform", meta = (ClampMin = -360, ClampMax = 360))
	float Rotation = 0.0;

	/** Scaling applied after computing projection */
	UPROPERTY(EditAnywhere, Category = "UV Transform", meta = (Delta = 0.01, LinearDeltaSensitivity = 1))
	FVector2D Scale = FVector2D::UnitVector;

	/** Translation applied after computing projection */
	UPROPERTY(EditAnywhere, Category = "UV Transform")
	FVector2D Translation = FVector2D::ZeroVector;

	//
	// Saved State. These are used internally to support UsePrevious initialization mode
	//

	UPROPERTY()
	FVector SavedDimensions = FVector::ZeroVector;

	UPROPERTY()
	bool bSavedProportionalDimensions = false;

	UPROPERTY()
	FTransform SavedTransform;
};


/**
 * Factory with enough info to spawn the background-thread Operator to do a chunk of work for the tool
 *  stores a pointer to the tool and enough info to know which specific operator it should spawn
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_UVProjectionOperatorFactory : public UObject, public UE::Geometry::IDynamicMeshOperatorFactory
{
	GENERATED_BODY()
public:
	// IDynamicMeshOperatorFactory API
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVProjectionTool> Tool;
};


/**
 * UV projection tool
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_UVProjectionTool : public UTLLRN_SingleTargetWithSelectionTool
{
	GENERATED_BODY()

public:

	friend UTLLRN_UVProjectionOperatorFactory;

	virtual void Setup() override;
	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;
	
	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void RequestAction(ETLLRN_UVProjectionToolActions ActionType);

protected:

	UPROPERTY()
	TObjectPtr<UTLLRN_MeshUVChannelProperties> UVChannelProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVProjectionToolProperties> BasicProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVProjectionToolEditActions> EditActions = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_ExistingTLLRN_MeshMaterialProperties> MaterialSettings = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_MeshOpPreviewWithBackgroundCompute> Preview = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CheckerMaterial = nullptr;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> TransformGizmo = nullptr;
	
	UPROPERTY()
	TObjectPtr<UTransformProxy> TransformProxy = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVProjectionOperatorFactory> OperatorFactory = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PreviewGeometry> EdgeRenderer = nullptr;

	TSharedPtr<UE::Geometry::FDynamicMesh3, ESPMode::ThreadSafe> InputMesh;
	TSharedPtr<TArray<int32>, ESPMode::ThreadSafe> TriangleROI;
	TSharedPtr<TArray<int32>, ESPMode::ThreadSafe> VertexROI;
	TSharedPtr<UE::Geometry::FDynamicMeshAABBTree3, ESPMode::ThreadSafe> InputMeshROISpatial;
	TSet<int32> TriangleROISet;

	FTransform3d WorldTransform;
	UE::Geometry::FAxisAlignedBox3d WorldBounds;

	FVector CachedDimensions;
	FVector InitialDimensions;
	bool bInitialProportionalDimensions;
	FTransform InitialTransform;
	int32 DimensionsWatcher = -1;
	int32 DimensionsModeWatcher = -1;
	bool bTransformModified = false;
	void OnInitializationModeChanged();
	void ApplyInitializationMode();

	FViewCameraState CameraState;

	FToolDataVisualizer ProjectionShapeVisualizer;

	void InitializeMesh();
	void UpdateNumPreviews();

	void TransformChanged(UTransformProxy* Proxy, FTransform Transform);

	void OnMaterialSettingsChanged();
	void OnMeshUpdated(UTLLRN_MeshOpPreviewWithBackgroundCompute* PreviewCompute);

	UE::Geometry::FOrientedBox3d GetProjectionBox() const;


	//
	// Support for ctrl+click to set plane from hit point
	//

	TUniquePtr<FSelectClickedAction> SetPlaneCtrlClickBehaviorTarget;

	UPROPERTY()
	TObjectPtr<USingleClickInputBehavior> ClickToSetPlaneBehavior;

	void UpdatePlaneFromClick(const FVector3d& Position, const FVector3d& Normal, bool bTransitionOnly);

	//
	// Support for Action Buttons
	//

	bool bHavePendingAction = false;
	ETLLRN_UVProjectionToolActions PendingAction;
	virtual void ApplyAction(ETLLRN_UVProjectionToolActions ActionType);
	void ApplyAction_AutoFit(bool bAlign);
	void ApplyAction_Reset();

};
