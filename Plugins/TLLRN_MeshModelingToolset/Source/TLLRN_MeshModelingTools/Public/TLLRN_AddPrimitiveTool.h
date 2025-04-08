// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseTools/SingleClickTool.h"
#include "CoreMinimal.h"
#include "InteractiveToolBuilder.h"
#include "TLLRN_PreviewMesh.h"
#include "Properties/TLLRN_MeshMaterialProperties.h"
#include "PropertySets/TLLRN_CreateMeshObjectTypeProperties.h"
#include "UObject/NoExportTypes.h"
#include "InteractiveToolQueryInterfaces.h"

#include "TLLRN_AddPrimitiveTool.generated.h"

PREDECLARE_USE_GEOMETRY_CLASS(FDynamicMesh3);
class UCombinedTransformGizmo;
class UTLLRN_DragAlignmentMechanic;

/**
 * Builder
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_AddPrimitiveToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	enum class EMakeMeshShapeType : uint32
	{
		Box,
		Cylinder,
		Cone,
		Arrow,
		Rectangle,
		Disc,
		Torus,
		Sphere,
		Stairs,
		Capsule
	};

	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	EMakeMeshShapeType ShapeType{EMakeMeshShapeType::Box};
};

/** Placement Target Types */
UENUM()
enum class ETLLRN_MakeMeshPlacementType : uint8
{
	GroundPlane = 0,
	OnScene     = 1,
	AtOrigin	= 2
};

/** Placement Pivot Location */
UENUM()
enum class ETLLRN_MakeMeshPivotLocation : uint8
{
	Base,
	Centered,
	Top
};

/** PolyGroup mode for shape */
UENUM()
enum class ETLLRN_MakeMeshPolygroupMode : uint8
{
	/** One PolyGroup for the entire shape */
	PerShape,
	/** One PolyGroup for each geometric face */
	PerFace,
	/** One PolyGroup for each quad/triangle */
	PerQuad
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralShapeToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:

	/** How PolyGroups are assigned to shape primitives. */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (ProceduralShapeSetting, DisplayName = "PolyGroup Mode"))
	ETLLRN_MakeMeshPolygroupMode PolygroupMode = ETLLRN_MakeMeshPolygroupMode::PerFace;

	/** How the shape is placed in the scene. */
	UPROPERTY(EditAnywhere, Category = Positioning, meta = (DisplayName = "Target Position"))
	ETLLRN_MakeMeshPlacementType TargetSurface = ETLLRN_MakeMeshPlacementType::OnScene;

	/** Location of pivot within the shape */
	UPROPERTY(EditAnywhere, Category = Positioning, meta = (ProceduralShapeSetting))
	ETLLRN_MakeMeshPivotLocation PivotLocation = ETLLRN_MakeMeshPivotLocation::Base;

	/** Initial rotation of the shape around its up axis, before placement. After placement, use the gizmo to control rotation. */
	UPROPERTY(EditAnywhere, Category = Positioning, DisplayName = "Initial Rotation", meta = (UIMin = "0.0", UIMax = "360.0", EditCondition = "!bShowGizmoOptions", HideEditConditionToggle))
	float Rotation = 0.0;

	/** If true, aligns the shape along the normal of the surface it is placed on. */
	UPROPERTY(EditAnywhere, Category = Positioning, meta = (EditCondition = "TargetSurface == ETLLRN_MakeMeshPlacementType::OnScene"))
	bool bAlignToNormal = true;

	/** Show a gizmo to allow the mesh to be repositioned after the initial placement click. */
	UPROPERTY(EditAnywhere, Category = Positioning, meta = (EditCondition = "bShowGizmoOptions", EditConditionHides, HideEditConditionToggle))
	bool bShowGizmo = true;

	//~ Not user visible- used to hide the bShowGizmo option when not yet placed mesh.
	UPROPERTY(meta = (TransientToolProperty))
	bool bShowGizmoOptions = false;
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralBoxToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:
	/** Width of the box */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Width = 100.f;

	/** Depth of the box */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Depth = 100.f;

	/** Height of the box */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Height = 100.f;

	/** Number of subdivisions along the width */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting))
	int WidthSubdivisions = 1;

	/** Number of subdivisions along the depth */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting))
	int DepthSubdivisions = 1;

	/** Number of subdivisions along the height */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting))
	int HeightSubdivisions = 1;
};

UENUM()
enum class ETLLRN_ProceduralRectType
{
	/** Create a rectangle */
	Rectangle,
	/** Create a rounded rectangle */
	RoundedRectangle
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralRectangleToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:
	/** Type of rectangle */
	UPROPERTY(EditAnywhere, Category = Shape)
	ETLLRN_ProceduralRectType RectangleType = ETLLRN_ProceduralRectType::Rectangle;

	/** Width of the rectangle */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Width = 100.f;

	/** Depth of the rectangle */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Depth = 100.f;

	/** Number of subdivisions along the width */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting))
	int WidthSubdivisions = 1;

	/** Number of subdivisions along the depth */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting))
	int DepthSubdivisions = 1;

	/** Whether to preserve the overall Width and Depth for a Rounded Rectangle, or to allow the rounded corners to extend outside those dimensions. */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (EditCondition = "RectangleType == ETLLRN_ProceduralRectType::RoundedRectangle"))
	bool bMaintainDimension = true;

	/** Radius of rounded corners. This is only available for Rounded Rectangles. */
	UPROPERTY(EditAnywhere, Category = Shape,
		meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting,
			EditCondition = "RectangleType == ETLLRN_ProceduralRectType::RoundedRectangle"))
	float CornerRadius = 25.f;

	/** Number of radial slices for each rounded corner. This is only available for Rounded Rectangles. */
	UPROPERTY(EditAnywhere, Category = Shape,
		meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "500", ProceduralShapeSetting,
			EditCondition = "RectangleType == ETLLRN_ProceduralRectType::RoundedRectangle"))
	int CornerSlices = 16;
};

UENUM()
enum class ETLLRN_ProceduralDiscType
{
	/** Create a disc */
	Disc,
	/** Create a disc with a hole */
	PuncturedDisc
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralDiscToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:
	/** Type of disc */
	UPROPERTY(EditAnywhere, Category = Shape)
	ETLLRN_ProceduralDiscType DiscType = ETLLRN_ProceduralDiscType::Disc;

	/** Radius of the disc */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Radius = 50.f;

	/** Number of radial slices for the disc */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "500", ProceduralShapeSetting))
	int RadialSlices = 16;

	/** Number of radial subdivisions for each radial slice of the disc */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting))
	int RadialSubdivisions = 1;

	/** Radius of the hole in the disc. This is only available for Punctured Discs. */
	UPROPERTY(EditAnywhere, Category = Shape,
		meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting,
			EditCondition = "DiscType == ETLLRN_ProceduralDiscType::PuncturedDisc"))
	float HoleRadius = 25.f;
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralTorusToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:
	/** Major radius of the torus, measured from the torus center to the center of the torus tube */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float MajorRadius = 50.f;

	/** Minor radius of the torus, measured from the center ot the torus tube to the tube surface */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float MinorRadius = 25.f;

	/** Number of radial slices along the torus tube */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "500", ProceduralShapeSetting))
	int MajorSlices = 16;

	/** Number of radial slices around the torus tube */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "500", ProceduralShapeSetting))
	int MinorSlices = 16;
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralCylinderToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:

	/** Radius of the cylinder */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Radius = 50.f;

	/** Height of the cylinder */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Height = 200.f;

	/** Number of radial slices for the cylinder */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "500", ProceduralShapeSetting))
	int RadialSlices = 16;

	/** Number of subdivisions along the height of the cylinder */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting))
	int HeightSubdivisions = 1;
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralConeToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:
	/** Radius of the cone */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Radius = 50.f;

	/** Height of the cone */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Height = 200.f;

	/** Number of radial slices for the cone */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "500", ProceduralShapeSetting))
	int RadialSlices = 16;

	/** Number of subdivisions along the height of the cone */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting))
	int HeightSubdivisions = 1;
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralArrowToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:
	/** Radius of the arrow shaft */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float ShaftRadius = 20.f;

	/** Height of arrow shaft */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float ShaftHeight = 200.f;

	/** Radius of the arrow head base */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float HeadRadius = 60.f;

	/** Height of arrow head */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float HeadHeight = 120.f;

	/** Number of radial slices for the arrow */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "3", UIMax = "100", ClampMin = "3", ClampMax = "500", ProceduralShapeSetting))
	int RadialSlices = 16;

	/** Number of subdivisions along each part of the arrow, i.e. shaft, head base, head cone */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting))
	int HeightSubdivisions = 1;
};

UENUM()
enum class ETLLRN_ProceduralSphereType
{
	/** Create a Sphere with Lat Long parameterization */
	LatLong,
	/** Create a Sphere with Box parameterization */
	Box
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralSphereToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:
	/** Radius of the sphere */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Radius = 50.f;

	/** Type of subdivision for the sphere */
	UPROPERTY(EditAnywhere, Category = Shape)
	ETLLRN_ProceduralSphereType SubdivisionType = ETLLRN_ProceduralSphereType::Box;

	/** Number of subdivisions for each side of the sphere. This is only available for spheres with Box subdivision. */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1", UIMax = "100", ClampMin = "1", ClampMax = "500", ProceduralShapeSetting,
		EditCondition = "SubdivisionType == ETLLRN_ProceduralSphereType::Box"))
	int Subdivisions = 16;

	/** Number of horizontal slices of the sphere. This is only available for spheres with Lat Long subdivision. */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "3", UIMax = "100", ClampMin = "4", ClampMax = "500", ProceduralShapeSetting,
		EditCondition = "SubdivisionType == ETLLRN_ProceduralSphereType::LatLong"))
	int HorizontalSlices = 16;

	/** Number of vertical slices of the sphere. This is only available for spheres with Lat Long subdivision. */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "3", UIMax = "100", ClampMin = "4", ClampMax = "500", ProceduralShapeSetting,
		EditCondition = "SubdivisionType == ETLLRN_ProceduralSphereType::LatLong"))
	int VerticalSlices = 16;
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralCapsuleToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:
	/** Radius of the capsule */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float Radius = 25.f;

	/** Length of cylindrical section of the capsule */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float CylinderLength = 50.f;

	/** Number of slices of the hemispherical end caps. */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "2", UIMax = "100", ClampMin = "2", ClampMax = "500", ProceduralShapeSetting))
	int HemisphereSlices = 8;

	/** Number of radial slices of the cylindrical section. */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "3", UIMax = "100", ClampMin = "3", ClampMax = "500", ProceduralShapeSetting))
	int CylinderSlices = 16;

	/** Number of lengthwise subdivisions along cylindrical section */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "0", UIMax = "100", ClampMin = "0", ClampMax = "500", ProceduralShapeSetting))
	int CylinderSubdivisions = 1;
};

UENUM()
enum class ETLLRN_ProceduralStairsType
{
	/** Create a linear staircase */
	Linear,
	/** Create a floating staircase */
	Floating,
	/** Create a curved staircase */
	Curved,
	/** Create a spiral staircase */
	Spiral
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ProceduralStairsToolProperties : public UTLLRN_ProceduralShapeToolProperties
{
	GENERATED_BODY()

public:
	/** Type of staircase */
	UPROPERTY(EditAnywhere, Category = Shape)
	ETLLRN_ProceduralStairsType StairsType = ETLLRN_ProceduralStairsType::Linear;

	/** Number of steps */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (DisplayName = "Number of Steps", UIMin = "2", UIMax = "100", ClampMin = "2", ClampMax = "1000000", ProceduralShapeSetting))
	int NumSteps = 8;

	/** Width of each step */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float StepWidth = 150.0f;

	/** Height of each step */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting))
	float StepHeight = 20.0f;

	/** Depth of each step of linear stairs */
	UPROPERTY(EditAnywhere, Category = Shape,
		meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting,
			EditCondition =	"StairsType == ETLLRN_ProceduralStairsType::Linear || StairsType == ETLLRN_ProceduralStairsType::Floating"))
	float StepDepth = 30.0f;

	/** Angular length of curved stairs. Positive values are for clockwise and negative values are for counterclockwise. */
	UPROPERTY(EditAnywhere, Category = Shape,
		meta = (UIMin = "-360.0", UIMax = "360.0", ClampMin = "-360.0", ClampMax = "360.0", ProceduralShapeSetting,
			EditCondition =	"StairsType == ETLLRN_ProceduralStairsType::Curved"))
	float CurveAngle = 90.0f;

	/** Angular length of spiral stairs. Positive values are for clockwise and negative values are for counterclockwise. */
	UPROPERTY(EditAnywhere, Category = Shape,
		meta = (UIMin = "-720.0", UIMax = "720.0", ClampMin = "-360000.0", ClampMax = "360000.0", ProceduralShapeSetting,
			EditCondition =	"StairsType == ETLLRN_ProceduralStairsType::Spiral"))
	float SpiralAngle = 90.0f;

	/** Inner radius of curved and spiral stairs */
	UPROPERTY(EditAnywhere, Category = Shape,
		meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.0001", ClampMax = "1000000.0", ProceduralShapeSetting,
			EditCondition = "StairsType == ETLLRN_ProceduralStairsType::Curved || StairsType == ETLLRN_ProceduralStairsType::Spiral"))
	float InnerRadius = 150.0f;
};


/**
 * Base tool to create primitives
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_AddPrimitiveTool : public USingleClickTool, public IHoverBehaviorTarget, public IInteractiveToolCameraFocusAPI
{
	GENERATED_BODY()

public:
	explicit UTLLRN_AddPrimitiveTool(const FObjectInitializer&);

	virtual void SetWorld(UWorld* World);

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	// USingleClickTool
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos) override;

	// IHoverBehaviorTarget interface
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override;
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override;

	// IInteractiveToolCameraFocusAPI implementation
	virtual bool SupportsWorldSpaceFocusBox() override;
	virtual FBox GetWorldSpaceFocusBox() override;
	virtual bool SupportsWorldSpaceFocusPoint() override;
	virtual bool GetWorldSpaceFocusPoint(const FRay& WorldRay, FVector& PointOut) override;


protected:
	virtual ETLLRN_MakeMeshPolygroupMode GetDefaultPolygroupMode() const { return ETLLRN_MakeMeshPolygroupMode::PerQuad; }

	enum class EState
	{
		PlacingPrimitive,
		AdjustingSettings
	};

	EState CurrentState = EState::PlacingPrimitive;
	void SetState(EState NewState);

	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const {}
	virtual UTLLRN_ProceduralShapeToolProperties* CreateShapeSettings(){return nullptr;}

	virtual void GenerateAsset();

	/** Property set for type of output object (StaticMesh, Volume, etc) */
	UPROPERTY()
	TObjectPtr<UTLLRN_CreateMeshObjectTypeProperties> OutputTypeProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_ProceduralShapeToolProperties> ShapeSettings;

	UPROPERTY()
	TObjectPtr<UTLLRN_NewTLLRN_MeshMaterialProperties> MaterialProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_PreviewMesh> TLLRN_PreviewMesh;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> Gizmo = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_DragAlignmentMechanic> TLLRN_DragAlignmentMechanic = nullptr;

	UPROPERTY()
	FString AssetName = TEXT("GeneratedAsset");

	UWorld* TargetWorld;

	void UpdatePreviewPosition(const FInputDeviceRay& ClickPos);
	UE::Geometry::FFrame3d ShapeFrame;

	void UpdateTLLRN_PreviewMesh() const;

	void UpdateTargetSurface();

	// @return true if the primitive needs to be centered in the XY plane when placed.
	virtual bool ShouldCenterXY() const
	{
		// Most primitives are already XY centered, and re-centering them only introduces issues at very low samplings where the bounds center is offset from the intended center.
		return false;
	}

	// Used to make the initial placement of the mesh undoable
	class FStateChange : public FToolCommandChange
	{
	public:
		FStateChange(const FTransform& MeshTransformIn)
			: MeshTransform(MeshTransformIn)
		{
		}

		virtual void Apply(UObject* Object) override;
		virtual void Revert(UObject* Object) override;
		virtual FString ToString() const override
		{
			return TEXT("UTLLRN_AddPrimitiveTool::FStateChange");
		}

	protected:
		FTransform MeshTransform;
	};
};


UCLASS()
class UTLLRN_AddBoxPrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddBoxPrimitiveTool(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
};

UCLASS()
class UTLLRN_AddCylinderPrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddCylinderPrimitiveTool(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
};

UCLASS()
class UTLLRN_AddCapsulePrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddCapsulePrimitiveTool(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
};

UCLASS()
class UTLLRN_AddConePrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddConePrimitiveTool(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
};

UCLASS()
class UTLLRN_AddRectanglePrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddRectanglePrimitiveTool(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
};

UCLASS()
class UTLLRN_AddDiscPrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddDiscPrimitiveTool(const FObjectInitializer& ObjectInitializer);
	virtual void Setup() override;
protected:
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
};

UCLASS()
class UTLLRN_AddTorusPrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddTorusPrimitiveTool(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
};

UCLASS()
class UTLLRN_AddArrowPrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddArrowPrimitiveTool(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
};

UCLASS()
class UTLLRN_AddSpherePrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddSpherePrimitiveTool(const FObjectInitializer& ObjectInitializer);
protected:
	virtual ETLLRN_MakeMeshPolygroupMode GetDefaultPolygroupMode() const override 
	{ 
		return ETLLRN_MakeMeshPolygroupMode::PerFace; 
	}
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
};

UCLASS()
class UTLLRN_AddStairsPrimitiveTool : public UTLLRN_AddPrimitiveTool
{
	GENERATED_BODY()
public:
	explicit UTLLRN_AddStairsPrimitiveTool(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void GenerateMesh(FDynamicMesh3* OutMesh) const override;
	virtual bool ShouldCenterXY() const override
	{
		return true;
	}
};


