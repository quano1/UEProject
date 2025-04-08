// Copyright Epic Games, Inc. All Rights Reserved.

#include "ToolActivities/TLLRN_PolyEditCutFacesActivity.h"

#include "BaseBehaviors/SingleClickBehavior.h"
#include "BaseBehaviors/MouseHoverBehavior.h"
#include "ContextObjectStore.h"
#include "Drawing/TLLRN_PolyEditTLLRN_PreviewMesh.h"
#include "InteractiveToolManager.h"
#include "Mechanics/TLLRN_CollectSurfacePathMechanic.h"
#include "DynamicMesh/MeshIndexUtil.h" // TriangleToVertexIDs
#include "TLLRN_MeshOpPreviewHelpers.h"
#include "Operations/MeshPlaneCut.h"
#include "Selections/MeshEdgeSelection.h"
#include "Selection/TLLRN_PolygonSelectionMechanic.h"
#include "ToolActivities/TLLRN_PolyEditActivityContext.h"
#include "ToolActivities/PolyEditActivityUtil.h"
#include "ToolSceneQueriesUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_PolyEditCutFacesActivity)

#define LOCTEXT_NAMESPACE "UTLLRN_PolyEditInsetOutsetActivity"

using namespace UE::Geometry;

void UTLLRN_PolyEditCutFacesActivity::Setup(UInteractiveTool* ParentToolIn)
{
	Super::Setup(ParentToolIn);

	CutProperties = NewObject<UTLLRN_PolyEditCutProperties>();
	CutProperties->RestoreProperties(ParentToolIn);
	AddToolPropertySource(CutProperties);
	SetToolPropertySourceEnabled(CutProperties, false);

	// Register ourselves to receive clicks and hover
	USingleClickInputBehavior* ClickBehavior = NewObject<USingleClickInputBehavior>();
	ClickBehavior->Initialize(this);
	ParentTool->AddInputBehavior(ClickBehavior);
	UMouseHoverBehavior* HoverBehavior = NewObject<UMouseHoverBehavior>();
	HoverBehavior->Initialize(this);
	ParentTool->AddInputBehavior(HoverBehavior);

	ActivityContext = ParentTool->GetToolManager()->GetContextObjectStore()->FindContext<UTLLRN_PolyEditActivityContext>();
}

void UTLLRN_PolyEditCutFacesActivity::Shutdown(EToolShutdownType ShutdownType)
{
	Clear();
	CutProperties->SaveProperties(ParentTool.Get());

	CutProperties = nullptr;
	ParentTool = nullptr;
	ActivityContext = nullptr;
}

bool UTLLRN_PolyEditCutFacesActivity::CanStart() const
{
	if (!ActivityContext)
	{
		return false;
	}
	const FGroupTopologySelection& Selection = ActivityContext->SelectionMechanic->GetActiveSelection();
	return !Selection.SelectedGroupIDs.IsEmpty();
}

EToolActivityStartResult UTLLRN_PolyEditCutFacesActivity::Start()
{
	if (!CanStart())
	{
		ParentTool->GetToolManager()->DisplayMessage(
			LOCTEXT("OnCutFacesFailedMessage", "Cannot cut without face selection."),
			EToolMessageLevel::UserWarning);
		return EToolActivityStartResult::FailedStart;
	}

	Clear();
	BeginCutFaces();
	bIsRunning = true;

	ActivityContext->EmitActivityStart(LOCTEXT("BeginCutFacesActivity", "Begin Cut Faces"));

	return EToolActivityStartResult::Running;
}

bool UTLLRN_PolyEditCutFacesActivity::CanAccept() const
{
	return false;
}

EToolActivityEndResult UTLLRN_PolyEditCutFacesActivity::End(EToolShutdownType)
{
	Clear();
	EToolActivityEndResult ToReturn = bIsRunning ? EToolActivityEndResult::Cancelled 
		: EToolActivityEndResult::ErrorDuringEnd;
	bIsRunning = false;
	return ToReturn;
}

void UTLLRN_PolyEditCutFacesActivity::BeginCutFaces()
{
	const FGroupTopologySelection& ActiveSelection = ActivityContext->SelectionMechanic->GetActiveSelection();
	TArray<int32> ActiveTriangleSelection;
	ActivityContext->CurrentTopology->GetSelectedTriangles(ActiveSelection, ActiveTriangleSelection);

	FTransform3d WorldTransform(ActivityContext->Preview->TLLRN_PreviewMesh->GetTransform());

	EditPreview = PolyEditActivityUtil::CreateTLLRN_PolyEditTLLRN_PreviewMesh(*ParentTool, *ActivityContext);
	FTransform3d WorldTranslation, WorldRotateScale;
	EditPreview->ApplyTranslationToPreview(WorldTransform, WorldTranslation, WorldRotateScale);
	EditPreview->InitializeStaticType(ActivityContext->CurrentMesh.Get(), ActiveTriangleSelection, &WorldRotateScale);

	FDynamicMesh3 StaticHitTargetMesh;
	EditPreview->MakeInsetTypeTargetMesh(StaticHitTargetMesh);

	SurfacePathMechanic = NewObject<UTLLRN_CollectSurfacePathMechanic>(this);
	SurfacePathMechanic->Setup(ParentTool.Get());
	SurfacePathMechanic->InitializeMeshSurface(MoveTemp(StaticHitTargetMesh));
	SurfacePathMechanic->SetFixedNumPointsMode(2);
	SurfacePathMechanic->bSnapToTargetMeshVertices = true;
	double SnapTol = ToolSceneQueriesUtil::GetDefaultVisualAngleSnapThreshD();
	SurfacePathMechanic->SpatialSnapPointsFunc = [this, SnapTol](FVector3d Position1, FVector3d Position2)
	{
		return CutProperties->bSnapToVertices && ToolSceneQueriesUtil::PointSnapQuery(this->CameraState, Position1, Position2, SnapTol);
	};

	SetToolPropertySourceEnabled(CutProperties, true);
}

void UTLLRN_PolyEditCutFacesActivity::ApplyCutFaces()
{
	check(SurfacePathMechanic != nullptr && EditPreview != nullptr);

	const FGroupTopologySelection& ActiveSelection = ActivityContext->SelectionMechanic->GetActiveSelection();
	TArray<int32> ActiveTriangleSelection;
	ActivityContext->CurrentTopology->GetSelectedTriangles(ActiveSelection, ActiveTriangleSelection);

	// construct cut plane normal from line points
	FFrame3d Point0(SurfacePathMechanic->HitPath[0]), Point1(SurfacePathMechanic->HitPath[1]);
	FVector3d PlaneNormal;
	if (CutProperties->Orientation == ETLLRN_PolyEditCutPlaneOrientation::ViewDirection)
	{
		FVector3d Direction0 = UE::Geometry::Normalized(Point0.Origin - (FVector3d)CameraState.Position);
		FVector3d Direction1 = UE::Geometry::Normalized(Point1.Origin - (FVector3d)CameraState.Position);
		PlaneNormal = Direction1.Cross(Direction0);
	}
	else
	{
		FVector3d LineDirection = UE::Geometry::Normalized(Point1.Origin - Point0.Origin);
		FVector3d UpVector = UE::Geometry::Normalized(Point0.Z() + Point1.Z());
		PlaneNormal = LineDirection.Cross(UpVector);
	}
	FVector3d PlaneOrigin = 0.5 * (Point0.Origin + Point1.Origin);
	// map into local space of target mesh
	FTransformSRT3d WorldTransform(ActivityContext->Preview->TLLRN_PreviewMesh->GetTransform());
	PlaneOrigin = WorldTransform.InverseTransformPosition(PlaneOrigin);
	PlaneNormal = WorldTransform.InverseTransformNormal(PlaneNormal);
	UE::Geometry::Normalize(PlaneNormal);

	// track changes
	FDynamicMeshChangeTracker ChangeTracker(ActivityContext->CurrentMesh.Get());
	ChangeTracker.BeginChange();
	TArray<int32> VertexSelection;
	UE::Geometry::TriangleToVertexIDs(ActivityContext->CurrentMesh.Get(), ActiveTriangleSelection, VertexSelection);
	ChangeTracker.SaveVertexOneRingTriangles(VertexSelection, true);

	// apply the cut to edges of selected triangles
	FGroupTopologySelection OutputSelection;
	FMeshPlaneCut Cut(ActivityContext->CurrentMesh.Get(), PlaneOrigin, PlaneNormal);
	Cut.bSimplifyAlongNewEdges = !ActivityContext->bTriangleMode;
	Cut.SimplifySettings.bPreserveVertexNormals = false;
	FMeshEdgeSelection Edges(ActivityContext->CurrentMesh.Get());
	Edges.SelectTriangleEdges(ActiveTriangleSelection);
	Cut.EdgeFilterFunc = [&](int EdgeID) { return Edges.IsSelected(EdgeID); };
	TSet<int32> TriangleSelectionSet;
	if (ActivityContext->bTriangleMode)
	{
		TriangleSelectionSet.Append(ActiveTriangleSelection);
	}
	if (Cut.SplitEdgesOnly(true, ActivityContext->bTriangleMode ? &TriangleSelectionSet : nullptr))
	{
		if (!ActivityContext->bTriangleMode)
		{
			for (FMeshPlaneCut::FCutResultRegion& Region : Cut.ResultRegions)
			{
				OutputSelection.SelectedGroupIDs.Add(Region.GroupID);
			}
		}
		else
		{
			OutputSelection.SelectedGroupIDs.Append(TriangleSelectionSet);
		}
	}

	// Emit undo (also updates relevant structures)
	ActivityContext->EmitCurrentMeshChangeAndUpdate(LOCTEXT("PolyMeshCutFacesChange", "Cut Faces"),
		ChangeTracker.EndChange(), OutputSelection);

	// End activity
	Clear();
	bIsRunning = false;
	Cast<ITLLRN_ToolActivityHost>(ParentTool)->NotifyActivitySelfEnded(this);
}

void UTLLRN_PolyEditCutFacesActivity::Clear()
{
	if (EditPreview != nullptr)
	{
		EditPreview->Disconnect();
		EditPreview = nullptr;
	}

	SurfacePathMechanic = nullptr;
	SetToolPropertySourceEnabled(CutProperties, false);
}

void UTLLRN_PolyEditCutFacesActivity::Render(IToolsContextRenderAPI* RenderAPI)
{
	ParentTool->GetToolManager()->GetContextQueriesAPI()->GetCurrentViewState(CameraState);

	if (SurfacePathMechanic != nullptr)
	{
		SurfacePathMechanic->Render(RenderAPI);
	}
}

void UTLLRN_PolyEditCutFacesActivity::Tick(float DeltaTime)
{
}

FInputRayHit UTLLRN_PolyEditCutFacesActivity::IsHitByClick(const FInputDeviceRay& ClickPos)
{
	FInputRayHit OutHit;
	OutHit.bHit = bIsRunning;
	return OutHit;
}

void UTLLRN_PolyEditCutFacesActivity::OnClicked(const FInputDeviceRay& ClickPos)
{
	if (bIsRunning && SurfacePathMechanic->TryAddPointFromRay((FRay3d)ClickPos.WorldRay))
	{
		if (SurfacePathMechanic->IsDone())
		{
			++ActivityStamp;
			ApplyCutFaces();
		}
		else
		{
			ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_PolyEditCutFacesActivityFirstPointChange>(ActivityStamp),
				LOCTEXT("CutLineStart", "Cut Line Start"));
		}
	}
}

FInputRayHit UTLLRN_PolyEditCutFacesActivity::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos)
{
	FInputRayHit OutHit;
	OutHit.bHit = bIsRunning;
	return OutHit;
}

bool UTLLRN_PolyEditCutFacesActivity::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	SurfacePathMechanic->UpdatePreviewPoint((FRay3d)DevicePos.WorldRay);
	return bIsRunning;
}

void FTLLRN_PolyEditCutFacesActivityFirstPointChange::Revert(UObject* Object)
{
	UTLLRN_PolyEditCutFacesActivity* Activity = Cast<UTLLRN_PolyEditCutFacesActivity>(Object);
	if (ensure(Activity->SurfacePathMechanic))
	{
		Activity->SurfacePathMechanic->PopLastPoint();
	}
	bHaveDoneUndo = true;
}

#undef LOCTEXT_NAMESPACE

