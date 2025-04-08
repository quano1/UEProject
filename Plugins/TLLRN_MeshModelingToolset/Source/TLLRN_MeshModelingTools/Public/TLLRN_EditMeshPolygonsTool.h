// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Changes/MeshVertexChange.h" // IMeshVertexCommandChangeTarget
#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "DynamicMesh/DynamicMeshChangeTracker.h" // FDynamicMeshChange for TUniquePtr
#include "TLLRN_InteractiveToolActivity.h" // ITLLRN_ToolActivityHost
#include "InteractiveToolBuilder.h"
#include "InteractiveToolQueryInterfaces.h" // IInteractiveToolNestedAcceptCancelAPI
#include "Internationalization/Text.h"
#include "Operations/GroupTopologyDeformer.h"
#include "BaseTools/TLLRN_SingleTargetWithSelectionTool.h"

#include "GeometryBase.h"

#include "TLLRN_EditMeshPolygonsTool.generated.h"

PREDECLARE_GEOMETRY(class FGroupTopology);
PREDECLARE_GEOMETRY(struct FGroupTopologySelection);
PREDECLARE_GEOMETRY(class FDynamicMeshChangeTracker);

struct FSlateBrush;
class UCombinedTransformGizmo;
class UTLLRN_DragAlignmentMechanic;
class UTLLRN_MeshOpPreviewWithBackgroundCompute; 
class FMeshVertexChangeBuilder;
class UTLLRN_EditMeshPolygonsTool;
class UTLLRN_PolyEditActivityContext;
class UTLLRN_PolyEditInsertEdgeActivity;
class UTLLRN_PolyEditInsertEdgeLoopActivity;
class UTLLRN_PolyEditExtrudeActivity;
class UTLLRN_PolyEditInsetOutsetActivity;
class UTLLRN_PolyEditCutFacesActivity;
class UTLLRN_PolyEditPlanarProjectionUVActivity;
class UTLLRN_PolyEditBevelEdgeActivity;
class UTLLRN_PolyEditExtrudeEdgeActivity;
class UTLLRN_PolygonSelectionMechanic;
class UTransformProxy;


/**
 * ToolBuilder
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsToolBuilder : public UTLLRN_SingleTargetWithSelectionToolBuilder
{
	GENERATED_BODY()
public:
	bool bTriangleMode = false;

	virtual UTLLRN_SingleTargetWithSelectionTool* CreateNewTool(const FToolBuilderState& SceneState) const override;
	virtual void InitializeNewTool(UTLLRN_SingleTargetWithSelectionTool* Tool, const FToolBuilderState& SceneState) const override;

	virtual bool RequiresInputSelection() const override { return false; }
};


UENUM()
enum class ETLLRN_LocalFrameMode
{
	FromObject,
	FromGeometry
};


/** 
 * These are properties that do not get enabled/disabled based on the action 
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditCommonProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Options)
	bool bShowWireframe = false;

	UPROPERTY(EditAnywhere, Category = Options)
	bool bShowSelectableCorners = true;

	/** When true, allows the transform gizmo to be rendered */
	UPROPERTY(EditAnywhere, Category = Gizmo)
	bool bGizmoVisible = true;

	/** Determines whether, on selection changes, the gizmo's rotation is taken from the object transform, or from the geometry
	 elements selected. Only relevant with a local coordinate system and when rotation is not locked. */
	UPROPERTY(EditAnywhere, Category = Gizmo, meta = (HideEditConditionToggle, EditCondition = "bLocalCoordSystem && !bLockRotation"))
	ETLLRN_LocalFrameMode TLLRN_LocalFrameMode = ETLLRN_LocalFrameMode::FromGeometry;

	/** When true, keeps rotation of gizmo constant through selection changes and manipulations 
	 (but not middle-click repositions). Only active with a local coordinate system.*/
	UPROPERTY(EditAnywhere, Category = Gizmo, meta = (HideEditConditionToggle, EditCondition = "bLocalCoordSystem"))
	bool bLockRotation = false;

	/** This gets updated internally so that properties can respond to whether the coordinate system is set to local or global */
	UPROPERTY()
	bool bLocalCoordSystem = true;
};


UENUM()
enum class ETLLRN_EditMeshPolygonsToolActions
{
	NoAction,
	AcceptCurrent,
	CancelCurrent,
	Extrude,
	PushPull,
	Offset,
	Inset,
	Outset,
	BevelFaces,
	InsertEdge,
	InsertEdgeLoop,
	Complete,

	PlaneCut,
	Merge,
	Delete,
	CutFaces,
	RecalculateNormals,
	FlipNormals,
	Retriangulate,
	Decompose,
	Disconnect,
	Duplicate,

	CollapseEdge,
	WeldEdges,
	WeldEdgesCentered,
	StraightenEdge,
	FillHole,
	BridgeEdges,
	ExtrudeEdges,
	BevelEdges,
	SimplifyAlongEdges,

	PlanarProjectionUV,

	SimplifyByGroups,
	RegenerateExtraCorners,

	// triangle-specific edits
	PokeSingleFace,
	SplitSingleEdge,
	FlipSingleEdge,
	CollapseSingleEdge,

	// for external use
	BevelAuto
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsActionModeToolBuilder : public UTLLRN_EditMeshPolygonsToolBuilder
{
	GENERATED_BODY()
public:
	ETLLRN_EditMeshPolygonsToolActions StartupAction = ETLLRN_EditMeshPolygonsToolActions::Extrude;

	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual void InitializeNewTool(UTLLRN_SingleTargetWithSelectionTool* Tool, const FToolBuilderState& SceneState) const override;
};

UENUM()
enum class ETLLRN_EditMeshPolygonsToolSelectionMode
{
	Faces,
	Edges,
	Vertices,
	Loops,
	Rings,
	FacesEdgesVertices
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsSelectionModeToolBuilder : public UTLLRN_EditMeshPolygonsToolBuilder
{
	GENERATED_BODY()
public:
	ETLLRN_EditMeshPolygonsToolSelectionMode SelectionMode = ETLLRN_EditMeshPolygonsToolSelectionMode::Faces;

	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual void InitializeNewTool(UTLLRN_SingleTargetWithSelectionTool* Tool, const FToolBuilderState& SceneState) const override;
};



UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsToolActionPropertySet : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UTLLRN_EditMeshPolygonsTool> ParentTool;

	void Initialize(UTLLRN_EditMeshPolygonsTool* ParentToolIn) { ParentTool = ParentToolIn; }

	void PostAction(ETLLRN_EditMeshPolygonsToolActions Action);
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditTopologyProperties : public UTLLRN_EditMeshPolygonsToolActionPropertySet
{
	GENERATED_BODY()

public:
	/** 
	 * When true, adds extra corners at sharp group edge bends (in addition to the normal corners that
	 * are placed at junctures of three or more group edges). For instance, a single disconnected quad-like group
	 * would normally have a single group edge with no corners, since it has no neighboring groups, but this
	 * setting will allow for the generation of corners at the quad corners, which is very useful for editing.
	 * Note that the setting takes effect only after clicking Regenerate Extra Corners or performing some
	 * operation that changes the group topology.
	 */
	UPROPERTY(EditAnywhere, Category = TopologyOptions)
	bool bAddExtraCorners = true;

	UFUNCTION(CallInEditor, Category = TopologyOptions)
	void RegenerateExtraCorners() { PostAction(ETLLRN_EditMeshPolygonsToolActions::RegenerateExtraCorners); }

	/** 
	 * When generating extra corners, how sharp the angle needs to be to warrant an extra corner placement there. Lower values require
	 * sharper corners, so are more tolerant of curved group edges. For instance, 180 will place corners at every vertex along a group
	 * edge even if the edge is perfectly straight, and 135 will place a vertex only once the edge bends 45 degrees off the straight path
	 * (i.e. 135 degrees to the previous edge). 
	 * The setting is applied either when Regenerate Extra Corners is clicked, or after any operation that modifies topology.
	 */
	UPROPERTY(EditAnywhere, Category = TopologyOptions, meta = (ClampMin = "0", ClampMax = "180", EditCondition = "bAddExtraCorners"))
	double ExtraCornerAngleThresholdDegrees = 135;
};

/** PolyEdit Actions */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsToolActions : public UTLLRN_EditMeshPolygonsToolActionPropertySet
{
	GENERATED_BODY()
public:
	/** Extrude the current set of selected faces by moving and stitching them. */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Extrude", DisplayPriority = 1))
	void Extrude() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Extrude); }

	/** Like Extrude/Offset, but performed in a boolean way, meaning that the faces can cut away the mesh or bridge mesh parts. */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Push/Pull", DisplayPriority = 1))
	void PushPull() { PostAction(ETLLRN_EditMeshPolygonsToolActions::PushPull); }

	/** Like Extrude, but defaults to moving verts along vertex normals instead of a single direction.*/
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Offset", DisplayPriority = 1))
	void Offset() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Offset); }

	/**
	 * Inset the current set of selected faces. Click in viewport to confirm inset distance.
	 * 
	 * (An Inset operation stitches in a smaller version of selected faces inside the existing ones)
	 */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Inset", DisplayPriority = 2))
	void Inset() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Inset);	}

	/**
	 * Outset the current set of selected faces. Click in viewport to confirm outset distance.
	 * 
	 * (An Outset operation stitches in a larger version of selected faces inside the existing ones)
	 */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Outset", DisplayPriority = 3))
	void Outset() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Outset);	}

	/** Bevel the edge loops around the selected faces, inserting edge-aligned faces that interpolate the normals of the selected faces */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Bevel", DisplayPriority = 4))
	void Bevel() { PostAction(ETLLRN_EditMeshPolygonsToolActions::BevelFaces); }

	/** Merge the current set of selected faces into a single face */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Merge", DisplayPriority = 4))
	void Merge() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Merge);	}

	/** Delete the current set of selected faces */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Delete Faces", DisplayPriority = 4))
	void Delete() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Delete); }

	/** Cut the current set of selected faces. Click twice in viewport to set cut line. */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "CutFaces", DisplayPriority = 5))
	void CutFaces() { PostAction(ETLLRN_EditMeshPolygonsToolActions::CutFaces); }

	/** Recalculate normals for the current set of selected faces */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "RecalcNormals", DisplayPriority = 6))
	void RecalcNormals() { PostAction(ETLLRN_EditMeshPolygonsToolActions::RecalculateNormals); }

	/** Flip normals and face orientation for the current set of selected faces */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Flip", DisplayPriority = 7))
	void Flip() { PostAction(ETLLRN_EditMeshPolygonsToolActions::FlipNormals); }

	/** Retriangulate each of the selected faces */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Retriangulate", DisplayPriority = 9))
	void Retriangulate() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Retriangulate);	}

	/** Split each of the selected faces into a separate polygon for each triangle */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Decompose", DisplayPriority = 10))
	void Decompose() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Decompose);	}

	/** Separate the selected faces at their borders */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Disconnect", DisplayPriority = 11))
	void Disconnect() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Disconnect); }

	/** Duplicate the selected faces at their borders */
	UFUNCTION(CallInEditor, Category = FaceEdits, meta = (DisplayName = "Duplicate", DisplayPriority = 12))
	void Duplicate() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Duplicate); }

	/** Insert a chain of edges across quads (faces with four edges) in the mesh. Due to ambiguity, edges will not be inserted on non-quad faces. */
	UFUNCTION(CallInEditor, Category = ShapeEdits, meta = (DisplayName = "InsertEdgeLoop", DisplayPriority = 13))
	void InsertEdgeLoop() { PostAction(ETLLRN_EditMeshPolygonsToolActions::InsertEdgeLoop); }

	/** Insert a new edge connecting existing edges or vertices on a single face */
	UFUNCTION(CallInEditor, Category = ShapeEdits, meta = (DisplayName = "Insert Edge", DisplayPriority = 14))
	void InsertEdge() { PostAction(ETLLRN_EditMeshPolygonsToolActions::InsertEdge); }

	/** Simplify every polygon group by removing vertices on shared straight edges and retriangulating */
	UFUNCTION(CallInEditor, Category = ShapeEdits, meta = (DisplayName = "SimplifyByGroups", DisplayPriority = 15))
	void SimplifyByGroups() { PostAction(ETLLRN_EditMeshPolygonsToolActions::SimplifyByGroups); }

};



UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsToolActions_Triangles : public UTLLRN_EditMeshPolygonsToolActionPropertySet
{
	GENERATED_BODY()
public:
	/** Extrude the current set of selected faces. Click in viewport to confirm extrude height. */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Extrude", DisplayPriority = 1))
	void Extrude() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Extrude); }

	/** Like Extrude/Offset, but performed in a boolean way, meaning that the faces can cut away the mesh or bridge mesh parts. */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Push/Pull", DisplayPriority = 1))
	void PushPull() { PostAction(ETLLRN_EditMeshPolygonsToolActions::PushPull); }

	/** Like Extrude, but defaults to moving verts along vertex normals instead of a single direction. */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Offset", DisplayPriority = 1))
	void Offset() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Offset); }

	/** Inset the current set of selected faces. Click in viewport to confirm inset distance. */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Inset", DisplayPriority = 2))
	void Inset() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Inset);	}

	/** Outset the current set of selected faces. Click in viewport to confirm outset distance. */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Outset", DisplayPriority = 3))
	void Outset() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Outset);	}

	/** Delete the current set of selected faces */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Delete Faces", DisplayPriority = 4))
	void Delete() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Delete); }

	/** Cut the current set of selected faces. Click twice in viewport to set cut line. */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "CutFaces", DisplayPriority = 5))
	void CutFaces() { PostAction(ETLLRN_EditMeshPolygonsToolActions::CutFaces); }

	/** Recalculate normals for the current set of selected faces */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "RecalcNormals", DisplayPriority = 6))
	void RecalcNormals() { PostAction(ETLLRN_EditMeshPolygonsToolActions::RecalculateNormals); }

	/** Flip normals and face orientation for the current set of selected faces */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Flip", DisplayPriority = 7))
	void Flip() { PostAction(ETLLRN_EditMeshPolygonsToolActions::FlipNormals); }

	/** Separate the selected faces at their borders */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Disconnect", DisplayPriority = 11))
	void Disconnect() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Disconnect); }

	/** Duplicate the selected faces */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Duplicate", DisplayPriority = 12))
	void Duplicate() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Duplicate); }

	/** Insert a new vertex at the center of each selected face */
	UFUNCTION(CallInEditor, Category = TriangleEdits, meta = (DisplayName = "Poke", DisplayPriority = 13))
	void Poke() { PostAction(ETLLRN_EditMeshPolygonsToolActions::PokeSingleFace); }
};





UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsToolUVActions : public UTLLRN_EditMeshPolygonsToolActionPropertySet
{
	GENERATED_BODY()

public:

	/** Assign planar-projection UVs to mesh */
	UFUNCTION(CallInEditor, Category = UVs, meta = (DisplayName = "PlanarProjection", DisplayPriority = 11))
	void PlanarProjection()
	{
		PostAction(ETLLRN_EditMeshPolygonsToolActions::PlanarProjectionUV);
	}
};





UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsToolEdgeActions : public UTLLRN_EditMeshPolygonsToolActionPropertySet
{
	GENERATED_BODY()
public:
	/** Merge selected boundary edges, centering the result */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Weld", DisplayPriority = 0))
	void WeldCentered() { PostAction(ETLLRN_EditMeshPolygonsToolActions::WeldEdgesCentered); }

	/** Merge selected boundary edges, moving the first edge to the second */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Weld To", DisplayPriority = 1))
	void Weld() { PostAction(ETLLRN_EditMeshPolygonsToolActions::WeldEdges); }

	/** Make each selected polygroup edge follow a straight path between its endpoints */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Straighten", DisplayPriority = 2))
	void Straighten() { PostAction(ETLLRN_EditMeshPolygonsToolActions::StraightenEdge); }

	/** Fill the adjacent hole for any selected boundary edges */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Fill Hole", DisplayPriority = 3))
	void FillHole()	{ PostAction(ETLLRN_EditMeshPolygonsToolActions::FillHole); }

	/** Bevel the selected edges, replacing them with angled faces */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Bevel", DisplayPriority = 4))
	void Bevel() { PostAction(ETLLRN_EditMeshPolygonsToolActions::BevelEdges); }
	
	/** Create a new face that connects the selected edges */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Bridge", DisplayPriority = 5))
	void Bridge() { PostAction(ETLLRN_EditMeshPolygonsToolActions::BridgeEdges); }

	/** Duplicate and move boundary edge vertices outwards and connect them to the original boundary to create new faces. */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayPriority = 6))
	void Extrude() { PostAction(ETLLRN_EditMeshPolygonsToolActions::ExtrudeEdges); }

	/** Simplify the underlying triangulation along the selected edges, when doing so won't change the shape or UVs, or make low-quality triangles */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayPriority = 7))
	void Simplify() { PostAction(ETLLRN_EditMeshPolygonsToolActions::SimplifyAlongEdges); }
	
	/** Delete selected edge, implicitly merging any connected faces */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Delete Edges", DisplayPriority = 8))
	void DeleteEdge() { PostAction(ETLLRN_EditMeshPolygonsToolActions::Delete); }

	/** Collapse the selected edges, deleting the attached triangles and merging their vertices into one */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Collapse", DisplayPriority = 5))
	void Collapse() { PostAction(ETLLRN_EditMeshPolygonsToolActions::CollapseEdge); }
};


UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsToolEdgeActions_Triangles : public UTLLRN_EditMeshPolygonsToolActionPropertySet
{
	GENERATED_BODY()
public:
	/** Merge selected boundary edges, centering the result */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Weld", DisplayPriority = 0))
	void WeldCentered() { PostAction(ETLLRN_EditMeshPolygonsToolActions::WeldEdgesCentered); }

	/** Merge selected boundary edges, moving the first edge to the second */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Weld To", DisplayPriority = 1))
	void Weld() { PostAction(ETLLRN_EditMeshPolygonsToolActions::WeldEdges); }

	/** Fill the adjacent hole for any selected boundary edges */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Fill Hole", DisplayPriority = 2))
	void FillHole() { PostAction(ETLLRN_EditMeshPolygonsToolActions::FillHole); }

	/** Create a new face that connects the selected edges */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Bridge", DisplayPriority = 3))
	void Bridge() { PostAction(ETLLRN_EditMeshPolygonsToolActions::BridgeEdges); }
	
	/** Duplicate and move boundary vertices outwards and connect them to the original boundary to create new faces. */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Extrude", DisplayPriority = 4))
	void Extrude() { PostAction(ETLLRN_EditMeshPolygonsToolActions::ExtrudeEdges); }

	/** Collapse the selected edges, deleting the attached triangles and merging its two vertices into one */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Collapse", DisplayPriority = 5))
	void Collapse() { PostAction(ETLLRN_EditMeshPolygonsToolActions::CollapseEdge); }

	/** Flip the selected (non-border, non-seam) edges, replacing them with new edges in the crossing direction */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Flip", DisplayPriority = 6))
	void Flip() { PostAction(ETLLRN_EditMeshPolygonsToolActions::FlipSingleEdge); }

	/** Split the selected edges, inserting a new vertex at each edge midpoint */
	UFUNCTION(CallInEditor, Category = EdgeEdits, meta = (DisplayName = "Split", DisplayPriority = 7))
	void Split() { PostAction(ETLLRN_EditMeshPolygonsToolActions::SplitSingleEdge); }
};



/**
 *
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_EditMeshPolygonsTool : public UTLLRN_SingleTargetWithSelectionTool,
	public ITLLRN_ToolActivityHost, 
	public IMeshVertexCommandChangeTarget,
	public IInteractiveToolNestedAcceptCancelAPI
{
	GENERATED_BODY()
	using FFrame3d = UE::Geometry::FFrame3d;

public:
	UTLLRN_EditMeshPolygonsTool();

	virtual void RegisterActions(FInteractiveToolActionSet& ActionSet) override;
	void EnableTriangleMode();

	// used by undo/redo
	void RebuildTopologyWithGivenExtraCorners(const TSet<int32>& Vids);

	virtual void Setup() override;
	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void DrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	// IInteractiveToolCameraFocusAPI implementation
	virtual FBox GetWorldSpaceFocusBox() override;
	virtual bool GetWorldSpaceFocusPoint(const FRay& WorldRay, FVector& PointOut) override;

	// ITLLRN_ToolActivityHost
	virtual void NotifyActivitySelfEnded(UTLLRN_InteractiveToolActivity* Activity) override;

	// IMeshVertexCommandChangeTarget
	virtual void ApplyChange(const FMeshVertexChange* Change, bool bRevert) override;

	// IInteractiveToolNestedAcceptCancelAPI
	virtual bool SupportsNestedCancelCommand() override { return true; }
	virtual bool CanCurrentlyNestedCancel() override;
	virtual bool ExecuteNestedCancelCommand() override;
	virtual bool SupportsNestedAcceptCommand() override { return true; }
	virtual bool CanCurrentlyNestedAccept() override;
	virtual bool ExecuteNestedAcceptCommand() override;

public:

	virtual void RequestAction(ETLLRN_EditMeshPolygonsToolActions ActionType);

	virtual void RequestSingleShotAction(ETLLRN_EditMeshPolygonsToolActions ActionType);

	void SetActionButtonsVisibility(bool bVisible);

protected:
	// If bTriangleMode = true, then we use a per-triangle FTriangleGroupTopology instead of polygroup topology.
	// This allows low-level mesh editing with mainly the same code, at a significant cost in overhead.
	// This is a fundamental mode switch, must be set before ::Setup() is called!
	bool bTriangleMode;		

	// TODO: This is a hack to allow us to disallow any actions inside the tool after Setup() is called. We
	// use it if the user tries to run the tool on a mesh that has too many edges for us to render, to avoid
	// hanging the editor.
	bool bToolDisabled = false;

	UPROPERTY()
	TObjectPtr<UTLLRN_MeshOpPreviewWithBackgroundCompute> Preview = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditCommonProperties> CommonProps = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_EditMeshPolygonsToolActions> EditActions = nullptr;
	UPROPERTY()
	TObjectPtr<UTLLRN_EditMeshPolygonsToolActions_Triangles> EditActions_Triangles = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_EditMeshPolygonsToolEdgeActions> EditEdgeActions = nullptr;
	UPROPERTY()
	TObjectPtr<UTLLRN_EditMeshPolygonsToolEdgeActions_Triangles> EditEdgeActions_Triangles = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_EditMeshPolygonsToolUVActions> EditUVActions = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditTopologyProperties> TopologyProperties = nullptr;

	/**
	 * Activity objects that handle multi-interaction operations
	 */
	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditExtrudeActivity> ExtrudeActivity = nullptr;
	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditInsetOutsetActivity> InsetOutsetActivity = nullptr;
	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditCutFacesActivity> CutFacesActivity = nullptr;
	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditPlanarProjectionUVActivity> PlanarProjectionUVActivity = nullptr;
	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditInsertEdgeActivity> InsertEdgeActivity = nullptr;
	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditInsertEdgeLoopActivity> InsertEdgeLoopActivity = nullptr;
	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditBevelEdgeActivity> BevelEdgeActivity = nullptr;
	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditExtrudeEdgeActivity> ExtrudeEdgeActivity = nullptr;

	TMap<UTLLRN_InteractiveToolActivity*, FText> ActivityLabels;
	TMap<UTLLRN_InteractiveToolActivity*, FName> ActivityIconNames;

	/**
	 * Points to one of the activities when it is active
	 */
	TObjectPtr<UTLLRN_InteractiveToolActivity> CurrentActivity = nullptr;

	TSharedPtr<UE::Geometry::FDynamicMesh3> CurrentMesh;
	TSharedPtr<UE::Geometry::FGroupTopology> Topology;
	TSharedPtr<UE::Geometry::FDynamicMeshAABBTree3> MeshSpatial;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditActivityContext> ActivityContext = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolygonSelectionMechanic> SelectionMechanic = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_DragAlignmentMechanic> TLLRN_DragAlignmentMechanic = nullptr;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> TransformGizmo = nullptr;

	UPROPERTY()
	TObjectPtr<UTransformProxy> TransformProxy = nullptr;

	FText DefaultMessage;

	void ResetUserMessage();

	bool bSelectionStateDirty = false;
	void OnSelectionModifiedEvent();

	void OnBeginGizmoTransform(UTransformProxy* Proxy);
	void OnEndGizmoTransform(UTransformProxy* Proxy);
	void OnGizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform);
	void UpdateGizmoFrame(const FFrame3d* UseFrame = nullptr);
	FFrame3d LastGeometryFrame;
	FFrame3d LastTransformerFrame;
	FFrame3d LockedTransfomerFrame;
	bool bInGizmoDrag = false;

	UE::Geometry::FTransformSRT3d BakedTransform; // We bake the scale part of the Target -> World transform
	UE::Geometry::FTransformSRT3d WorldTransform; // Transform from Baked to World

	FFrame3d InitialGizmoFrame;
	FVector3d InitialGizmoScale;
	bool bGizmoUpdatePending = false;
	FFrame3d LastUpdateGizmoFrame;
	FVector3d LastUpdateGizmoScale;
	bool bLastUpdateUsedWorldFrame = false;
	void ComputeUpdate_Gizmo();

	UE::Geometry::FDynamicMeshAABBTree3& GetSpatial();
	bool bSpatialDirty;

	// UV Scale factor to apply to texturing on any new geometry (e.g. new faces added by extrude)
	float UVScaleFactor = 1.0f;

	ETLLRN_EditMeshPolygonsToolActions PendingAction = ETLLRN_EditMeshPolygonsToolActions::NoAction;
	bool bTerminateOnPendingActionComplete = false;

	int32 ActivityTimestamp = 1;

	void StartActivity(TObjectPtr<UTLLRN_InteractiveToolActivity> Activity);
	void EndCurrentActivity(EToolShutdownType ShutdownType);
	void SetActionButtonPanelsVisible(bool bVisible);

	// Emit an undoable change to CurrentMesh and update related structures (preview, spatial, etc)
	void EmitCurrentMeshChangeAndUpdate(const FText& TransactionLabel,
		TUniquePtr<UE::Geometry::FDynamicMeshChange> MeshChangeIn,
		const UE::Geometry::FGroupTopologySelection& OutputSelection);

	// Emit an undoable start of an activity
	void EmitActivityStart(const FText& TransactionLabel);

	void UpdateGizmoVisibility();

	void ApplyDelete();
	
	void ApplyMerge();
	void ApplyDeleteFaces();
	void ApplyRecalcNormals();
	void ApplyFlipNormals();
	void ApplyRetriangulate();
	void ApplyDecompose();
	void ApplyDisconnect();
	void ApplyDuplicate();
	void ApplyPokeSingleFace();

	void ApplyCollapseEdge();
	void ApplyWeldEdges();
	void ApplyStraightenEdges();
	void ApplyDeleteEdges();
	void ApplyFillHole();
	void ApplyBridgeEdges();
	void ApplySimplifyAlongEdges();

	void ApplyFlipSingleEdge();
	UE_DEPRECATED(5.5, "Use ApplyCollapseEdge instead.")
	void ApplyCollapseSingleEdge();
	void ApplySplitSingleEdge();

	void SimplifyByGroups();

	void ApplyRegenerateExtraCorners();
	double ExtraCornerDotProductThreshold = -1;

	FFrame3d ActiveSelectionFrameLocal;
	FFrame3d ActiveSelectionFrameWorld;
	TArray<int32> ActiveTriangleSelection;
	UE::Geometry::FAxisAlignedBox3d ActiveSelectionBounds;

	struct FSelectedEdge
	{
		int32 EdgeTopoID;
		TArray<int32> EdgeIDs;
	};
	TArray<FSelectedEdge> ActiveEdgeSelection;

	enum class EPreviewMaterialType
	{
		SourceMaterials, PreviewMaterial, UVMaterial
	};
	void UpdateEditPreviewMaterials(EPreviewMaterialType MaterialType);
	EPreviewMaterialType CurrentPreviewMaterial;


	//
	// data for current drag
	//
	UE::Geometry::FGroupTopologyDeformer LinearDeformer;
	void UpdateDeformerFromSelection(const UE::Geometry::FGroupTopologySelection& Selection);

	FMeshVertexChangeBuilder* ActiveVertexChange;
	void UpdateDeformerChangeFromROI(bool bFinal);
	void BeginDeformerChange();
	void EndDeformerChange();

	bool BeginMeshFaceEditChange();

	bool BeginMeshEdgeEditChange();
	bool BeginMeshBoundaryEdgeEditChange(bool bOnlySimple);
	bool BeginMeshEdgeEditChange(TFunctionRef<bool(int32)> GroupEdgeIDFilterFunc);

	void UpdateFromCurrentMesh(bool bRebuildTopology);
	int32 ModifiedTopologyCounter = 0;
	bool bWasTopologyEdited = false;

	friend class FTLLRN_EditMeshPolygonsToolMeshChange;
	friend class FPolyEditActivityStartChange;

	// custom setup support
	friend class UTLLRN_EditMeshPolygonsSelectionModeToolBuilder;
	friend class UTLLRN_EditMeshPolygonsActionModeToolBuilder;
	TUniqueFunction<void(UTLLRN_EditMeshPolygonsTool*)> PostSetupFunction;
	void SetToSelectionModeInterface();

private:
	void ApplyWeldEdges(double InterpolationT);
	void ApplyWeldVertices(double InterpolationT);
	void CollapseGroupEdges(TSet<int32>& GroupEdgesToCollapse, 
		UE::Geometry::FDynamicMeshChangeTracker& ChangeTracker);
};


/**
 * Wraps a FDynamicMeshChange so that it can be expired and so that other data
 * structures in the tool can be updated. On apply/revert, the topology is rebuilt
 * using stored extra corner vids.
 */
class TLLRN_MESHMODELINGTOOLS_API FTLLRN_EditMeshPolygonsToolMeshChange : public FToolCommandChange
{
public:
	FTLLRN_EditMeshPolygonsToolMeshChange(TUniquePtr<UE::Geometry::FDynamicMeshChange> MeshChangeIn)
		: MeshChange(MoveTemp(MeshChangeIn))
	{};

	virtual void Apply(UObject* Object) override;
	virtual void Revert(UObject* Object) override;
	virtual FString ToString() const override;

	TSet<int32> ExtraCornerVidsBefore;
	TSet<int32> ExtraCornerVidsAfter;
protected:
	TUniquePtr<UE::Geometry::FDynamicMeshChange> MeshChange;
};



/**
 * FPolyEditActivityStartChange is used to cancel out of an active action on Undo. 
 * No action is taken on Redo, ie we do not re-start the Tool on Redo.
 */
class TLLRN_MESHMODELINGTOOLS_API FPolyEditActivityStartChange : public FToolCommandChange
{
public:
	FPolyEditActivityStartChange(int32 ActivityTimestampIn)
	{
		ActivityTimestamp = ActivityTimestampIn;
	}
	virtual void Apply(UObject* Object) override {}
	virtual void Revert(UObject* Object) override;
	virtual bool HasExpired(UObject* Object) const override;
	virtual FString ToString() const override;

protected:
	bool bHaveDoneUndo = false;
	int32 ActivityTimestamp = 0;
};
