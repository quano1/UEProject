// Copyright Epic Games, Inc. All Rights Reserved.

#include "ToolActivities/TLLRN_PolyEditInsetOutsetActivity.h"

#include "BaseBehaviors/SingleClickBehavior.h"
#include "BaseBehaviors/MouseHoverBehavior.h"
#include "ContextObjectStore.h"
#include "Drawing/TLLRN_PolyEditTLLRN_PreviewMesh.h"
#include "DynamicMesh/MeshNormals.h"
#include "TLLRN_EditMeshPolygonsTool.h"
#include "InteractiveToolManager.h"
#include "Mechanics/TLLRN_SpatialCurveDistanceMechanic.h"
#include "MeshBoundaryLoops.h"
#include "TLLRN_MeshOpPreviewHelpers.h"
#include "Operations/InsetMeshRegion.h"
#include "Selection/TLLRN_PolygonSelectionMechanic.h"
#include "ToolActivities/TLLRN_PolyEditActivityContext.h"
#include "ToolActivities/PolyEditActivityUtil.h"
#include "ToolSceneQueriesUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_PolyEditInsetOutsetActivity)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UTLLRN_PolyEditInsetOutsetActivity"

void UTLLRN_PolyEditInsetOutsetActivity::Setup(UInteractiveTool* ParentToolIn)
{
	Super::Setup(ParentToolIn);

	Settings = NewObject<UTLLRN_PolyEditInsetOutsetProperties>();
	Settings->RestoreProperties(ParentTool.Get());
	AddToolPropertySource(Settings);
	SetToolPropertySourceEnabled(Settings, false);

	// Register ourselves to receive clicks and hover
	USingleClickInputBehavior* ClickBehavior = NewObject<USingleClickInputBehavior>();
	ClickBehavior->Initialize(this);
	ParentTool->AddInputBehavior(ClickBehavior);
	UMouseHoverBehavior* HoverBehavior = NewObject<UMouseHoverBehavior>();
	HoverBehavior->Initialize(this);
	ParentTool->AddInputBehavior(HoverBehavior);

	ActivityContext = ParentTool->GetToolManager()->GetContextObjectStore()->FindContext<UTLLRN_PolyEditActivityContext>();
}

void UTLLRN_PolyEditInsetOutsetActivity::Shutdown(EToolShutdownType ShutdownType)
{
	Clear();
	Settings->SaveProperties(ParentTool.Get());

	Settings = nullptr;
	ParentTool = nullptr;
	ActivityContext = nullptr;
}

bool UTLLRN_PolyEditInsetOutsetActivity::CanStart() const
{
	if (!ActivityContext)
	{
		return false;
	}
	const FGroupTopologySelection& Selection = ActivityContext->SelectionMechanic->GetActiveSelection();
	return !Selection.SelectedGroupIDs.IsEmpty();
}

EToolActivityStartResult UTLLRN_PolyEditInsetOutsetActivity::Start()
{
	if (!CanStart())
	{
		ParentTool->GetToolManager()->DisplayMessage(
			LOCTEXT("InsetOutsetNoSelectionMesssage", "Cannot inset or outset without face selection."),
			EToolMessageLevel::UserWarning);
		return EToolActivityStartResult::FailedStart;
	}

	Clear();
	if (!BeginInset())
	{
		return EToolActivityStartResult::FailedStart;
	}
	bIsRunning = true;

	ActivityContext->EmitActivityStart(LOCTEXT("BeginInsetOutsetActivity", "Begin Inset/Outset"));

	return EToolActivityStartResult::Running;
}

bool UTLLRN_PolyEditInsetOutsetActivity::CanAccept() const
{
	return true;
}

EToolActivityEndResult UTLLRN_PolyEditInsetOutsetActivity::End(EToolShutdownType ShutdownType)
{
	if (!bIsRunning)
	{
		Clear();
		return EToolActivityEndResult::ErrorDuringEnd;
	}

	if (ShutdownType == EToolShutdownType::Cancel)
	{
		Clear();
		bIsRunning = false;
		return EToolActivityEndResult::Cancelled;
	}
	else
	{
		ApplyInset();
		Clear();
		bIsRunning = false;
		return EToolActivityEndResult::Completed;
	}
}

bool UTLLRN_PolyEditInsetOutsetActivity::BeginInset()
{
	const FGroupTopologySelection& ActiveSelection = ActivityContext->SelectionMechanic->GetActiveSelection();
	TArray<int32> ActiveTriangleSelection;
	ActivityContext->CurrentTopology->GetSelectedTriangles(ActiveSelection, ActiveTriangleSelection);

	FTransform3d WorldTransform(ActivityContext->Preview->TLLRN_PreviewMesh->GetTransform());

	EditPreview = PolyEditActivityUtil::CreateTLLRN_PolyEditTLLRN_PreviewMesh(*ParentTool, *ActivityContext);
	FTransform3d WorldTranslation, WorldRotateScale;
	EditPreview->ApplyTranslationToPreview(WorldTransform, WorldTranslation, WorldRotateScale);
	EditPreview->InitializeInsetType(ActivityContext->CurrentMesh.Get(), ActiveTriangleSelection, &WorldRotateScale);

	// Hide the selected triangles (that are being replaced by the inset/outset portion)
	ActivityContext->Preview->TLLRN_PreviewMesh->SetSecondaryBuffersVisibility(false);

	// make infinite-extent hit-test mesh
	FDynamicMesh3 InsetHitTargetMesh;
	EditPreview->MakeInsetTypeTargetMesh(InsetHitTargetMesh);

	CurveDistMechanic = NewObject<UTLLRN_SpatialCurveDistanceMechanic>(this);
	CurveDistMechanic->Setup(ParentTool.Get());
	CurveDistMechanic->WorldPointSnapFunc = [this](const FVector3d& WorldPos, FVector3d& SnapPos)
	{
		return ToolSceneQueriesUtil::FindWorldGridSnapPoint(ParentTool.Get(), WorldPos, SnapPos);
	};
	CurveDistMechanic->CurrentDistance = 1.0f;  // initialize to something non-zero...prob should be based on polygon bounds maybe?

	FMeshBoundaryLoops Loops(&InsetHitTargetMesh);
	if (Loops.Loops.Num() == 0)
	{
		ParentTool->GetToolManager()->DisplayMessage(
			LOCTEXT("InsetOutsetNoBorderMesssage", "Cannot inset or outset when selection has no border."),
			EToolMessageLevel::UserWarning);
		return false;
	}
	TArray<FVector3d> LoopVertices;
	Loops.Loops[0].GetVertices(LoopVertices);
	CurveDistMechanic->InitializePolyLoop(LoopVertices, FTransformSRT3d::Identity());

	SetToolPropertySourceEnabled(Settings, true);

	float BoundsMaxDim = ActivityContext->CurrentMesh->GetBounds().MaxDim();
	if (BoundsMaxDim > 0)
	{
		UVScaleFactor = 1.0 / BoundsMaxDim;
	}

	bPreviewUpdatePending = true;

	return true;
}

void UTLLRN_PolyEditInsetOutsetActivity::ApplyInset()
{
	check(CurveDistMechanic != nullptr && EditPreview != nullptr);

	const FGroupTopologySelection& ActiveSelection = ActivityContext->SelectionMechanic->GetActiveSelection();
	TArray<int32> ActiveTriangleSelection;
	ActivityContext->CurrentTopology->GetSelectedTriangles(ActiveSelection, ActiveTriangleSelection);

	FInsetMeshRegion Inset(ActivityContext->CurrentMesh.Get());
	Inset.UVScaleFactor = UVScaleFactor;
	Inset.Triangles = ActiveTriangleSelection;
	Inset.InsetDistance = (Settings->bOutset) ? -CurveDistMechanic->CurrentDistance
		: CurveDistMechanic->CurrentDistance;
	Inset.bReproject = (Settings->bOutset) ? false : Settings->bReproject;
	Inset.Softness = Settings->Softness;
	Inset.bSolveRegionInteriors = !Settings->bBoundaryOnly;
	Inset.AreaCorrection = Settings->AreaScale;

	Inset.ChangeTracker = MakeUnique<FDynamicMeshChangeTracker>(ActivityContext->CurrentMesh.Get());
	Inset.ChangeTracker->BeginChange();
	Inset.Apply();

	FMeshNormals::QuickComputeVertexNormalsForTriangles(*ActivityContext->CurrentMesh, Inset.AllModifiedTriangles);

	// Emit undo (also updates relevant structures)
	ActivityContext->EmitCurrentMeshChangeAndUpdate(LOCTEXT("PolyMeshInsetOutsetChange", "Inset/Outset"),
		Inset.ChangeTracker->EndChange(), ActiveSelection);
}

void UTLLRN_PolyEditInsetOutsetActivity::Clear()
{
	if (EditPreview != nullptr)
	{
		EditPreview->Disconnect();
		EditPreview = nullptr;
	}

	ActivityContext->Preview->TLLRN_PreviewMesh->SetSecondaryBuffersVisibility(true);

	CurveDistMechanic = nullptr;
	SetToolPropertySourceEnabled(Settings, false);
}

void UTLLRN_PolyEditInsetOutsetActivity::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (CurveDistMechanic != nullptr)
	{
		CurveDistMechanic->Render(RenderAPI);
	}
}

void UTLLRN_PolyEditInsetOutsetActivity::Tick(float DeltaTime)
{
	if (EditPreview && bPreviewUpdatePending)
	{
		double Sign = Settings->bOutset ? -1.0 : 1.0;
		bool bReproject = (Settings->bOutset) ? false : Settings->bReproject;
		double Softness = Settings->Softness;
		bool bBoundaryOnly = Settings->bBoundaryOnly;
		double AreaCorrection = Settings->AreaScale;
		EditPreview->UpdateInsetType(Sign * CurveDistMechanic->CurrentDistance,
			bReproject, Softness, AreaCorrection, bBoundaryOnly);

		bPreviewUpdatePending = false;
	}
}

FInputRayHit UTLLRN_PolyEditInsetOutsetActivity::IsHitByClick(const FInputDeviceRay& ClickPos)
{
	FInputRayHit OutHit;
	OutHit.bHit = bIsRunning;
	return OutHit;
}

void UTLLRN_PolyEditInsetOutsetActivity::OnClicked(const FInputDeviceRay& ClickPos)
{
	if (bIsRunning)
	{
		ApplyInset();

		// End activity
		Clear();
		bIsRunning = false;
		Cast<ITLLRN_ToolActivityHost>(ParentTool)->NotifyActivitySelfEnded(this);
	}
}

FInputRayHit UTLLRN_PolyEditInsetOutsetActivity::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos)
{
	FInputRayHit OutHit;
	OutHit.bHit = bIsRunning;
	return OutHit;
}

bool UTLLRN_PolyEditInsetOutsetActivity::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	CurveDistMechanic->UpdateCurrentDistance(DevicePos.WorldRay);
	bPreviewUpdatePending = true;
	return bIsRunning;
}

#undef LOCTEXT_NAMESPACE

