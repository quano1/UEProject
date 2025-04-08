// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_AddPrimitiveTool.h"

#include "BaseGizmos/TransformGizmoUtil.h"
#include "BaseGizmos/TransformProxy.h"
#include "ToolBuilderUtil.h"
#include "InteractiveToolManager.h"
#include "SceneQueries/SceneSnappingManager.h"
#include "BaseBehaviors/MouseHoverBehavior.h"
#include "Selection/ToolSelectionUtil.h"
#include "Mechanics/TLLRN_DragAlignmentMechanic.h"
#include "TLLRN_ModelingObjectsCreationAPI.h"
#include "ToolSceneQueriesUtil.h"
#include "ToolSetupUtil.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"

#include "Generators/CapsuleGenerator.h"
#include "Generators/SweepGenerator.h"
#include "Generators/GridBoxMeshGenerator.h"
#include "Generators/RectangleMeshGenerator.h"
#include "Generators/SphereGenerator.h"
#include "Generators/BoxSphereGenerator.h"
#include "Generators/DiscMeshGenerator.h"
#include "Generators/StairGenerator.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "FaceGroupUtil.h"

#include "DynamicMeshEditor.h"
#include "UObject/PropertyIterator.h"
#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_AddPrimitiveTool)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UTLLRN_AddPrimitiveTool"

/*
 * ToolBuilder
 */
bool UTLLRN_AddPrimitiveToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return true;
}

UInteractiveTool* UTLLRN_AddPrimitiveToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_AddPrimitiveTool* NewTool = nullptr;
	switch (ShapeType)
	{
	case EMakeMeshShapeType::Box:
		NewTool = NewObject<UTLLRN_AddBoxPrimitiveTool>(SceneState.ToolManager);
		break;
	case EMakeMeshShapeType::Cylinder:
		NewTool = NewObject<UTLLRN_AddCylinderPrimitiveTool>(SceneState.ToolManager);
		break;
	case EMakeMeshShapeType::Cone:
		NewTool = NewObject<UTLLRN_AddConePrimitiveTool>(SceneState.ToolManager);
		break;
	case EMakeMeshShapeType::Arrow:
		NewTool = NewObject<UTLLRN_AddArrowPrimitiveTool>(SceneState.ToolManager);
		break;
	case EMakeMeshShapeType::Rectangle:
		NewTool = NewObject<UTLLRN_AddRectanglePrimitiveTool>(SceneState.ToolManager);
		break;
	case EMakeMeshShapeType::Disc:
		NewTool = NewObject<UTLLRN_AddDiscPrimitiveTool>(SceneState.ToolManager);
		break;
	case EMakeMeshShapeType::Torus:
		NewTool = NewObject<UTLLRN_AddTorusPrimitiveTool>(SceneState.ToolManager);
		break;
	case EMakeMeshShapeType::Sphere:
		NewTool = NewObject<UTLLRN_AddSpherePrimitiveTool>(SceneState.ToolManager);
		break;
	case EMakeMeshShapeType::Stairs:
		NewTool = NewObject<UTLLRN_AddStairsPrimitiveTool>(SceneState.ToolManager);
		break;
	case EMakeMeshShapeType::Capsule:
		NewTool = NewObject<UTLLRN_AddCapsulePrimitiveTool>(SceneState.ToolManager);
		break;
	default:
		break;
	}
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void UTLLRN_AddPrimitiveTool::SetWorld(UWorld* World)
{
	this->TargetWorld = World;
}

UTLLRN_AddPrimitiveTool::UTLLRN_AddPrimitiveTool(const FObjectInitializer&)
{
	ShapeSettings = CreateDefaultSubobject<UTLLRN_ProceduralShapeToolProperties>(TEXT("ShapeSettings"));
	// CreateDefaultSubobject automatically sets RF_Transactional flag, we need to clear it so that undo/redo doesn't affect tool properties
	ShapeSettings->ClearFlags(RF_Transactional);
}

bool UTLLRN_AddPrimitiveTool::CanAccept() const
{
	return CurrentState == EState::AdjustingSettings;
}

void UTLLRN_AddPrimitiveTool::Setup()
{
	USingleClickTool::Setup();

	UMouseHoverBehavior* HoverBehavior = NewObject<UMouseHoverBehavior>(this);
	HoverBehavior->Initialize(this);
	AddInputBehavior(HoverBehavior);

	OutputTypeProperties = NewObject<UTLLRN_CreateMeshObjectTypeProperties>(this);
	OutputTypeProperties->RestoreProperties(this);
	OutputTypeProperties->InitializeDefault();
	OutputTypeProperties->WatchProperty(OutputTypeProperties->OutputType, [this](FString) { OutputTypeProperties->UpdatePropertyVisibility(); });
	AddToolPropertySource(OutputTypeProperties);

	AddToolPropertySource(ShapeSettings);
	
	ShapeSettings->WatchProperty(ShapeSettings->TargetSurface, [this](ETLLRN_MakeMeshPlacementType){UpdateTargetSurface();});
	ShapeSettings->PolygroupMode = GetDefaultPolygroupMode();
	ShapeSettings->RestoreProperties(this);

	MaterialProperties = NewObject<UTLLRN_NewTLLRN_MeshMaterialProperties>(this);
	AddToolPropertySource(MaterialProperties);
	MaterialProperties->RestoreProperties(this);

	// create preview mesh object
	TLLRN_PreviewMesh = NewObject<UTLLRN_PreviewMesh>(this);
	TLLRN_PreviewMesh->CreateInWorld(TargetWorld, FTransform::Identity);
	ToolSetupUtil::ApplyRenderingConfigurationToPreview(TLLRN_PreviewMesh, nullptr); 
	TLLRN_PreviewMesh->SetVisible(false);
	TLLRN_PreviewMesh->SetMaterial(MaterialProperties->Material.Get());
	TLLRN_PreviewMesh->EnableWireframe(MaterialProperties->bShowWireframe);

	UTransformProxy* TransformProxy = NewObject<UTransformProxy>(this);
	TransformProxy->OnTransformChanged.AddWeakLambda(this, [this](UTransformProxy*, FTransform NewTransform) 
		{
			TLLRN_PreviewMesh->SetTransform(NewTransform);
		});

	// TODO: It might be nice to use a repositionable gizmo, but the drag alignment mechanic can't currently 
	// hit the preview mesh, which makes middle click repositioning feel broken and not very useful.
	Gizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GetToolManager(),
		ETransformGizmoSubElements::StandardTranslateRotate, this);
	Gizmo->SetActiveTarget(TransformProxy, GetToolManager());

	TLLRN_DragAlignmentMechanic = NewObject<UTLLRN_DragAlignmentMechanic>(this);
	TLLRN_DragAlignmentMechanic->Setup(this);
	TLLRN_DragAlignmentMechanic->AddToGizmo(Gizmo);

	UpdateTLLRN_PreviewMesh();
	SetState(EState::PlacingPrimitive);
}

void UTLLRN_AddPrimitiveTool::SetState(EState NewState)
{
	CurrentState = NewState;

	bool bGizmoActive = (CurrentState == EState::AdjustingSettings);
	Gizmo->SetVisibility(bGizmoActive && ShapeSettings->bShowGizmo);
	ShapeSettings->bShowGizmoOptions = bGizmoActive;
	NotifyOfPropertyChangeByTool(ShapeSettings);

	if (CurrentState == EState::PlacingPrimitive)
	{
		GetToolManager()->DisplayMessage(
			LOCTEXT("OnStartTLLRN_AddPrimitiveTool", "This Tool creates new shapes. Click in the scene to choose initial placement of mesh."),
			EToolMessageLevel::UserNotification);
	}
	else
	{
		// Initialize gizmo to current preview location
		Gizmo->ReinitializeGizmoTransform(TLLRN_PreviewMesh->GetTransform());

		GetToolManager()->DisplayMessage(
			LOCTEXT("OnStartTLLRN_AddPrimitiveTool2", "Alter shape settings in the detail panel or modify placement with gizmo, then accept the tool."),
			EToolMessageLevel::UserNotification);
	}
}


void UTLLRN_AddPrimitiveTool::Shutdown(EToolShutdownType ShutdownType)
{
	if (ShutdownType == EToolShutdownType::Accept)
	{
		GenerateAsset();
	}

	TLLRN_DragAlignmentMechanic->Shutdown();
	TLLRN_DragAlignmentMechanic = nullptr;
	GetToolManager()->GetPairedGizmoManager()->DestroyAllGizmosByOwner(this);
	Gizmo = nullptr;

	TLLRN_PreviewMesh->SetVisible(false);
	TLLRN_PreviewMesh->Disconnect();
	TLLRN_PreviewMesh = nullptr;

	OutputTypeProperties->SaveProperties(this);
	ShapeSettings->SaveProperties(this);
	MaterialProperties->SaveProperties(this);
}


void UTLLRN_AddPrimitiveTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	TLLRN_DragAlignmentMechanic->Render(RenderAPI);
}



void UTLLRN_AddPrimitiveTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	// Because of how the ShapeSettings property set is implemented in this Tool, changes to it are transacted,
	// and if the user exits the Tool and then tries to undo/redo those transactions, this function will end up being called.
	// So we need to ensure that we handle this case.
	if (TLLRN_PreviewMesh)
	{
		TLLRN_PreviewMesh->EnableWireframe(MaterialProperties->bShowWireframe);
		TLLRN_PreviewMesh->SetMaterial(MaterialProperties->Material.Get());
		UpdateTLLRN_PreviewMesh();
	}

	if (Gizmo)
	{
		Gizmo->SetVisibility(CurrentState == EState::AdjustingSettings && ShapeSettings->bShowGizmo);
	}
}




FInputRayHit UTLLRN_AddPrimitiveTool::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos)
{
	FInputRayHit Result(0);
	Result.bHit = (CurrentState == EState::PlacingPrimitive);
	return Result;
}

void UTLLRN_AddPrimitiveTool::OnBeginHover(const FInputDeviceRay& DevicePos)
{
	UpdatePreviewPosition(DevicePos);
}

bool UTLLRN_AddPrimitiveTool::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	UpdatePreviewPosition(DevicePos);
	return true;
}

void UTLLRN_AddPrimitiveTool::OnEndHover()
{
	// do nothing
}



void UTLLRN_AddPrimitiveTool::UpdatePreviewPosition(const FInputDeviceRay& DeviceClickPos)
{
	FRay ClickPosWorldRay = DeviceClickPos.WorldRay;

	// hit position (temp)
	bool bHit = false;

	auto RaycastPlaneWithFallback = [](const FVector3d& Origin, const FVector3d& Direction, const FPlane& Plane, double FallbackDistance = 1000) -> FVector3d
	{
		double IntersectTime = FMath::RayPlaneIntersectionParam(Origin, Direction, Plane);
		if (IntersectTime < 0 || !FMath::IsFinite(IntersectTime))
		{
			IntersectTime = FallbackDistance;
		}
		return Origin + Direction * IntersectTime;
	};

	FPlane DrawPlane(FVector::ZeroVector, FVector(0, 0, 1));
	if (ShapeSettings->TargetSurface == ETLLRN_MakeMeshPlacementType::GroundPlane)
	{
		FVector3d DrawPlanePos = RaycastPlaneWithFallback(ClickPosWorldRay.Origin, ClickPosWorldRay.Direction, DrawPlane);
		bHit = true;
		ShapeFrame = FFrame3d(DrawPlanePos);
	}
	else if (ShapeSettings->TargetSurface == ETLLRN_MakeMeshPlacementType::OnScene)
	{
		// cast ray into scene
		FHitResult Result;
		bHit = ToolSceneQueriesUtil::FindNearestVisibleObjectHit(this, Result, ClickPosWorldRay);
		if (bHit)
		{
			FVector3d Normal = (FVector3d)Result.ImpactNormal;
			if (!ShapeSettings->bAlignToNormal)
			{
				Normal = FVector3d::UnitZ();
			}
			ShapeFrame = FFrame3d((FVector3d)Result.ImpactPoint, Normal);
			ShapeFrame.ConstrainedAlignPerpAxes();
		}
		else
		{
			// fall back to ground plane if we don't have a scene hit
			FVector3d DrawPlanePos = RaycastPlaneWithFallback(ClickPosWorldRay.Origin, ClickPosWorldRay.Direction, DrawPlane);
			bHit = true;
			ShapeFrame = FFrame3d(DrawPlanePos);
		}
	}
	else
	{
		bHit = true;
		ShapeFrame = FFrame3d();
	}

	// Snap to grid
	USceneSnappingManager* SnapManager = USceneSnappingManager::Find(GetToolManager());
	if (SnapManager)
	{
		FSceneSnapQueryRequest Request;
		Request.RequestType = ESceneSnapQueryType::Position;
		Request.TargetTypes = ESceneSnapQueryTargetType::Grid;
		Request.Position = (FVector)ShapeFrame.Origin;
		TArray<FSceneSnapQueryResult> Results;
		if (SnapManager->ExecuteSceneSnapQuery(Request, Results))
		{
			ShapeFrame.Origin = (FVector3d)Results[0].Position;
		}
	}

	if (ShapeSettings->Rotation != 0)
	{
		ShapeFrame.Rotate(FQuaterniond(ShapeFrame.Z(), ShapeSettings->Rotation, true));
	}

	if (bHit)
	{
		TLLRN_PreviewMesh->SetVisible(true);
		TLLRN_PreviewMesh->SetTransform(ShapeFrame.ToFTransform());
	}
	else
	{
		TLLRN_PreviewMesh->SetVisible(false);
	}
}

void UTLLRN_AddPrimitiveTool::UpdateTLLRN_PreviewMesh() const
{
	FDynamicMesh3 NewMesh;
	GenerateMesh( &NewMesh );

	if (ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerShape)
	{
		FaceGroupUtil::SetGroupID(NewMesh, 0);
	}

	if (MaterialProperties->UVScale != 1.0 || MaterialProperties->bWorldSpaceUVScale)
	{
		FDynamicMeshEditor Editor(&NewMesh);
		const float WorldUnitsInMetersFactor = MaterialProperties->bWorldSpaceUVScale ? .01f : 1.0f;
		Editor.RescaleAttributeUVs(MaterialProperties->UVScale * WorldUnitsInMetersFactor, MaterialProperties->bWorldSpaceUVScale);
	}

	// set mesh position
	const FAxisAlignedBox3d Bounds = NewMesh.GetBounds(true);
	FVector3d TargetOrigin = Bounds.Center();
	if (!ShouldCenterXY())
	{
		TargetOrigin.X = TargetOrigin.Y = 0;
	}
	if (ShapeSettings->PivotLocation == ETLLRN_MakeMeshPivotLocation::Base)
	{
		TargetOrigin.Z = Bounds.Min.Z;
	}
	else if (ShapeSettings->PivotLocation == ETLLRN_MakeMeshPivotLocation::Top)
	{
		TargetOrigin.Z = Bounds.Max.Z;
	}
	for (const int Vid : NewMesh.VertexIndicesItr())
	{
		FVector3d Pos = NewMesh.GetVertex(Vid);
		Pos -= TargetOrigin;
		NewMesh.SetVertex(Vid, Pos);
	}

	TLLRN_PreviewMesh->UpdatePreview(&NewMesh);

	TLLRN_PreviewMesh->SetTangentsMode(EDynamicMeshComponentTangentsMode::AutoCalculated);
	const bool CalculateTangentsSuccessful = TLLRN_PreviewMesh->CalculateTangents();
	checkSlow(CalculateTangentsSuccessful);
}

void UTLLRN_AddPrimitiveTool::UpdateTargetSurface()
{
	if (ShapeSettings->TargetSurface == ETLLRN_MakeMeshPlacementType::AtOrigin)
	{
		// default ray is used as coordinates will not be needed to set position at origin
		const FInputDeviceRay DefaultRay = FInputDeviceRay();
		UpdatePreviewPosition(DefaultRay);

		SetState(EState::AdjustingSettings);
		GetToolManager()->EmitObjectChange(this, MakeUnique<FStateChange>(TLLRN_PreviewMesh->GetTransform()),
				LOCTEXT("PlaceMeshTransaction", "Place Mesh"));
	} else
	{
		SetState(EState::PlacingPrimitive);
	}
}



bool UTLLRN_AddPrimitiveTool::SupportsWorldSpaceFocusBox()
{
	return TLLRN_PreviewMesh != nullptr;
}

FBox UTLLRN_AddPrimitiveTool::GetWorldSpaceFocusBox()
{
	if (TLLRN_PreviewMesh)
	{
		if (UPrimitiveComponent* Component = TLLRN_PreviewMesh->GetRootComponent())
		{
			return Component->Bounds.GetBox();
		}
	}
	return FBox();
}

bool UTLLRN_AddPrimitiveTool::SupportsWorldSpaceFocusPoint()
{
	return TLLRN_PreviewMesh != nullptr;

}

bool UTLLRN_AddPrimitiveTool::GetWorldSpaceFocusPoint(const FRay& WorldRay, FVector& PointOut)
{
	if (TLLRN_PreviewMesh)
	{
		FHitResult HitResult;
		if (TLLRN_PreviewMesh->FindRayIntersection(WorldRay, HitResult))
		{
			PointOut = HitResult.ImpactPoint;
			return true;
		}
	}
	return false;
}


FInputRayHit UTLLRN_AddPrimitiveTool::IsHitByClick(const FInputDeviceRay& ClickPos)
{
	FInputRayHit Result(0);
	Result.bHit = (CurrentState == EState::PlacingPrimitive);
	return Result;
}

void UTLLRN_AddPrimitiveTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	if (!ensure(CurrentState == EState::PlacingPrimitive))
	{
		return;
	}

	UpdatePreviewPosition(ClickPos);
	SetState(EState::AdjustingSettings);
	GetToolManager()->EmitObjectChange(this, MakeUnique<FStateChange>(TLLRN_PreviewMesh->GetTransform()),
		LOCTEXT("PlaceMeshTransaction", "Place Mesh"));
}

void UTLLRN_AddPrimitiveTool::GenerateAsset()
{
	UMaterialInterface* Material = TLLRN_PreviewMesh->GetMaterial();

	const FDynamicMesh3* CurMesh = TLLRN_PreviewMesh->GetPreviewDynamicMesh();

	GetToolManager()->BeginUndoTransaction(LOCTEXT("TLLRN_AddPrimitiveToolTransactionName", "Add Primitive Tool"));

	FTLLRN_CreateMeshObjectParams NewMeshObjectParams;
	NewMeshObjectParams.TargetWorld = TargetWorld;
	NewMeshObjectParams.Transform = TLLRN_PreviewMesh->GetTransform();
	NewMeshObjectParams.BaseName = AssetName;
	NewMeshObjectParams.Materials.Add(Material);
	NewMeshObjectParams.SetMesh(CurMesh);
	OutputTypeProperties->ConfigureTLLRN_CreateMeshObjectParams(NewMeshObjectParams);
	FTLLRN_CreateMeshObjectResult Result = UE::Modeling::CreateMeshObject(GetToolManager(), MoveTemp(NewMeshObjectParams));
	if (Result.IsOK() )
	{
		if (Result.NewActor != nullptr)
		{
			// select newly-created object
			ToolSelectionUtil::SetNewActorSelection(GetToolManager(), Result.NewActor);
		}
	}

	GetToolManager()->EndUndoTransaction();
}

void UTLLRN_AddPrimitiveTool::FStateChange::Apply(UObject* Object)
{
	UTLLRN_AddPrimitiveTool* Tool = Cast<UTLLRN_AddPrimitiveTool>(Object);

	// Set preview transform before changing state so that the adjustment gizmo is initialized properly
	Tool->TLLRN_PreviewMesh->SetTransform(MeshTransform);

	Tool->SetState(EState::AdjustingSettings);
}

void UTLLRN_AddPrimitiveTool::FStateChange::Revert(UObject* Object)
{
	UTLLRN_AddPrimitiveTool* Tool = Cast<UTLLRN_AddPrimitiveTool>(Object);
	Tool->SetState(EState::PlacingPrimitive);
}


UTLLRN_AddBoxPrimitiveTool::UTLLRN_AddBoxPrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralBoxToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Box");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("BoxToolName", "Create Box"));
}

void UTLLRN_AddBoxPrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	FGridBoxMeshGenerator BoxGen;
	const UTLLRN_ProceduralBoxToolProperties* BoxSettings = Cast<UTLLRN_ProceduralBoxToolProperties>(ShapeSettings);
	BoxGen.Box = UE::Geometry::FOrientedBox3d(FVector3d::Zero(), 0.5*FVector3d(BoxSettings->Depth, BoxSettings->Width, BoxSettings->Height));
	BoxGen.EdgeVertices = FIndex3i(BoxSettings->DepthSubdivisions + 1,
								   BoxSettings->WidthSubdivisions + 1,
								   BoxSettings->HeightSubdivisions + 1);
	BoxGen.bPolygroupPerQuad = ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad;
	BoxGen.Generate();
	OutMesh->Copy(&BoxGen);
}



UTLLRN_AddRectanglePrimitiveTool::UTLLRN_AddRectanglePrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralRectangleToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Rectangle");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("RectToolName", "Create Rectangle"));
}

void UTLLRN_AddRectanglePrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	auto* RectangleSettings = Cast<UTLLRN_ProceduralRectangleToolProperties>(ShapeSettings);
	switch (RectangleSettings->RectangleType)
	{
	case ETLLRN_ProceduralRectType::Rectangle:
	{
		FRectangleMeshGenerator RectGen;
		RectGen.Width = RectangleSettings->Depth;
		RectGen.Height = RectangleSettings->Width;
		RectGen.WidthVertexCount = RectangleSettings->DepthSubdivisions + 1;
		RectGen.HeightVertexCount = RectangleSettings->WidthSubdivisions + 1;
		RectGen.bSinglePolyGroup = (ShapeSettings->PolygroupMode != ETLLRN_MakeMeshPolygroupMode::PerQuad);
		RectGen.Generate();
		OutMesh->Copy(&RectGen);
		break;
	}
	case ETLLRN_ProceduralRectType::RoundedRectangle:
	{
		FRoundedRectangleMeshGenerator RectGen;
		RectGen.Width = RectangleSettings->Depth;
		RectGen.Height = RectangleSettings->Width;
		RectGen.Radius = RectangleSettings->CornerRadius;
		if (RectangleSettings->bMaintainDimension)
		{
			constexpr double MinSide = UE_DOUBLE_KINDA_SMALL_NUMBER;
			constexpr double MinRadius = MinSide * .5;
			double SmallSide = FMath::Min(RectGen.Width, RectGen.Height);
			RectGen.Radius = FMath::Clamp(RectGen.Radius, MinRadius, (SmallSide - MinSide) * .5);
			RectGen.Width = FMath::Max(MinSide, RectGen.Width - RectGen.Radius * 2);
			RectGen.Height = FMath::Max(MinSide, RectGen.Height - RectGen.Radius * 2);
			// Update the Radius in the UI, if it was clamped
			if (RectGen.Radius != RectangleSettings->CornerRadius)
			{
				RectangleSettings->CornerRadius = RectGen.Radius;
				NotifyOfPropertyChangeByTool(RectangleSettings);
			}
		}
		RectGen.WidthVertexCount = RectangleSettings->DepthSubdivisions + 1;
		RectGen.HeightVertexCount = RectangleSettings->WidthSubdivisions + 1;
		RectGen.bSinglePolyGroup = (ShapeSettings->PolygroupMode != ETLLRN_MakeMeshPolygroupMode::PerQuad);
		RectGen.AngleSamples = RectangleSettings->CornerSlices - 1;
		RectGen.Generate();
		OutMesh->Copy(&RectGen);
		break;
	}
	}
}


UTLLRN_AddDiscPrimitiveTool::UTLLRN_AddDiscPrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralDiscToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Disc");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("DiscToolName", "Create Disc"));
}

void UTLLRN_AddDiscPrimitiveTool::Setup()
{
	Super::Setup();


	// add watchers to ensure the hole radius is never larger than the disc radius when updating a punctured disc

	UTLLRN_ProceduralDiscToolProperties* DiscSettings = Cast<UTLLRN_ProceduralDiscToolProperties>(ShapeSettings);

	int32 HoleRadiusWatchID = DiscSettings->WatchProperty(DiscSettings->HoleRadius, 
															[DiscSettings](float HR) 
															{
																if (DiscSettings->DiscType == ETLLRN_ProceduralDiscType::PuncturedDisc)
																{
																	DiscSettings->HoleRadius = FMath::Min(DiscSettings->Radius, HR);
																}
															});
	DiscSettings->WatchProperty(DiscSettings->Radius, 
								[DiscSettings, HoleRadiusWatchID](float R) 
								{
									if (DiscSettings->DiscType == ETLLRN_ProceduralDiscType::PuncturedDisc)
									{
										DiscSettings->HoleRadius = FMath::Min(DiscSettings->HoleRadius, R);
										DiscSettings->SilentUpdateWatcherAtIndex(HoleRadiusWatchID);
									}
								});
}

void UTLLRN_AddDiscPrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	const UTLLRN_ProceduralDiscToolProperties* DiscSettings = Cast<UTLLRN_ProceduralDiscToolProperties>(ShapeSettings);
	switch (DiscSettings->DiscType)
	{
	case ETLLRN_ProceduralDiscType::Disc:
	{
		FDiscMeshGenerator Gen;
		Gen.Radius = DiscSettings->Radius;
		Gen.AngleSamples = DiscSettings->RadialSlices;
		Gen.RadialSamples = DiscSettings->RadialSubdivisions;
		Gen.bSinglePolygroup = (ShapeSettings->PolygroupMode != ETLLRN_MakeMeshPolygroupMode::PerQuad);
		Gen.Generate();
		OutMesh->Copy(&Gen);
		break;
	}
	case ETLLRN_ProceduralDiscType::PuncturedDisc:
	{
		FPuncturedDiscMeshGenerator Gen;
		Gen.Radius = DiscSettings->Radius;
		Gen.HoleRadius = FMath::Min(DiscSettings->HoleRadius, Gen.Radius * .999f); // hole cannot be bigger than outer radius
		Gen.AngleSamples = DiscSettings->RadialSlices;
		Gen.RadialSamples = DiscSettings->RadialSubdivisions;
		Gen.bSinglePolygroup = (ShapeSettings->PolygroupMode != ETLLRN_MakeMeshPolygroupMode::PerQuad);
		Gen.Generate();
		OutMesh->Copy(&Gen);
		break;
	}
	}
}



UTLLRN_AddTorusPrimitiveTool::UTLLRN_AddTorusPrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralTorusToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Torus");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("TorusToolName", "Create Torus"));
}

void UTLLRN_AddTorusPrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	FGeneralizedCylinderGenerator Gen;
	const UTLLRN_ProceduralTorusToolProperties* TorusSettings = Cast<UTLLRN_ProceduralTorusToolProperties>(ShapeSettings);
	Gen.CrossSection = FPolygon2d::MakeCircle(TorusSettings->MinorRadius, TorusSettings->MinorSlices);
	FPolygon2d PathCircle = FPolygon2d::MakeCircle(TorusSettings->MajorRadius, TorusSettings->MajorSlices);
	for (int Idx = 0; Idx < PathCircle.VertexCount(); Idx++)
	{
		Gen.Path.Add( FVector3d(PathCircle[Idx].X, PathCircle[Idx].Y, 0) );
	}
	Gen.bLoop = true;
	Gen.bCapped = false;
	Gen.bPolygroupPerQuad = ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad;
	Gen.InitialFrame = FFrame3d(Gen.Path[0]);
	Gen.Generate();
	OutMesh->Copy(&Gen);
}



UTLLRN_AddCylinderPrimitiveTool::UTLLRN_AddCylinderPrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralCylinderToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Cylinder");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("CylinderToolName", "Create Cylinder"));
}

void UTLLRN_AddCylinderPrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	FCylinderGenerator CylGen;
	const UTLLRN_ProceduralCylinderToolProperties* CylinderSettings = Cast<UTLLRN_ProceduralCylinderToolProperties>(ShapeSettings);
	CylGen.Radius[1] = CylGen.Radius[0] = CylinderSettings->Radius;
	CylGen.Height = CylinderSettings->Height;
	CylGen.AngleSamples = CylinderSettings->RadialSlices;
	CylGen.LengthSamples = CylinderSettings->HeightSubdivisions - 1;
	CylGen.bCapped = true;
	CylGen.bPolygroupPerQuad = ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad;
	CylGen.Generate();
	OutMesh->Copy(&CylGen);
}


UTLLRN_AddCapsulePrimitiveTool::UTLLRN_AddCapsulePrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralCapsuleToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Capsule");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("CapsuleToolName", "Create Capsule"));
}

void UTLLRN_AddCapsulePrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	FCapsuleGenerator CapGen;
	const UTLLRN_ProceduralCapsuleToolProperties* CapsuleSettings = Cast<UTLLRN_ProceduralCapsuleToolProperties>(ShapeSettings);
	CapGen.Radius = CapsuleSettings->Radius;
	CapGen.SegmentLength = CapsuleSettings->CylinderLength;
	CapGen.NumHemisphereArcSteps = CapsuleSettings->HemisphereSlices;
	CapGen.NumCircleSteps = CapsuleSettings->CylinderSlices;
	CapGen.NumSegmentSteps = CapsuleSettings->CylinderSubdivisions;
	CapGen.bPolygroupPerQuad = ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad;
	CapGen.Generate();
	OutMesh->Copy(&CapGen);
}


UTLLRN_AddConePrimitiveTool::UTLLRN_AddConePrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralConeToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Cone");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("ConeToolName", "Create Cone"));
}

void UTLLRN_AddConePrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	// Unreal's standard cone is just a cylinder with a very small top
	FCylinderGenerator CylGen;
	const UTLLRN_ProceduralConeToolProperties* ConeSettings = Cast<UTLLRN_ProceduralConeToolProperties>(ShapeSettings);
	CylGen.Radius[0] = ConeSettings->Radius;
	CylGen.Radius[1] = .01;
	CylGen.Height = ConeSettings->Height;
	CylGen.AngleSamples = ConeSettings->RadialSlices;
	CylGen.LengthSamples = ConeSettings->HeightSubdivisions - 1;
	CylGen.bCapped = true;
	CylGen.bPolygroupPerQuad = ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad;
	CylGen.Generate();
	OutMesh->Copy(&CylGen);
}


UTLLRN_AddArrowPrimitiveTool::UTLLRN_AddArrowPrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralArrowToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Arrow");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("ArrowToolName", "Create Arrow"));
}

void UTLLRN_AddArrowPrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	FArrowGenerator ArrowGen;
	const UTLLRN_ProceduralArrowToolProperties* ArrowSettings = Cast<UTLLRN_ProceduralArrowToolProperties>(ShapeSettings);
	ArrowGen.StickRadius = ArrowSettings->ShaftRadius;
	ArrowGen.StickLength = ArrowSettings->ShaftHeight;
	ArrowGen.HeadBaseRadius = ArrowSettings->HeadRadius;
	ArrowGen.HeadTipRadius = .01f;
	ArrowGen.HeadLength = ArrowSettings->HeadHeight;
	ArrowGen.AngleSamples = ArrowSettings->RadialSlices;
	ArrowGen.bCapped = true;
	ArrowGen.bPolygroupPerQuad = ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad;
	if (ArrowSettings->HeightSubdivisions > 1)
	{
		const int AdditionalLengthsSamples = ArrowSettings->HeightSubdivisions - 1;
		ArrowGen.AdditionalLengthSamples[0] = AdditionalLengthsSamples;
		ArrowGen.AdditionalLengthSamples[1] = AdditionalLengthsSamples;
		ArrowGen.AdditionalLengthSamples[2] = AdditionalLengthsSamples;
	}
	ArrowGen.Generate();
	OutMesh->Copy(&ArrowGen);
}



UTLLRN_AddSpherePrimitiveTool::UTLLRN_AddSpherePrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralSphereToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Sphere");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("SphereToolName", "Create Sphere"));
}

void UTLLRN_AddSpherePrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	const UTLLRN_ProceduralSphereToolProperties* SphereSettings = Cast<UTLLRN_ProceduralSphereToolProperties>(ShapeSettings);
	switch (SphereSettings->SubdivisionType)
	{
	case ETLLRN_ProceduralSphereType::LatLong:
	{
		FSphereGenerator SphereGen;
		SphereGen.Radius = SphereSettings->Radius;
		SphereGen.NumTheta = SphereSettings->VerticalSlices;
		SphereGen.NumPhi = SphereSettings->HorizontalSlices + 1;
		// In FSphereGenerator, PerFace is effectively ignored, and does the same thing as 
		//  PerShape, which is unlikely to be useful. So we might as well have PerFace do
		//  the same thing as PerQuad.
		SphereGen.bPolygroupPerQuad = (ShapeSettings->PolygroupMode != ETLLRN_MakeMeshPolygroupMode::PerShape);
		SphereGen.Generate();
		OutMesh->Copy(&SphereGen);
		break;
	}
	case ETLLRN_ProceduralSphereType::Box:
	{
		FBoxSphereGenerator SphereGen;
		SphereGen.Radius = SphereSettings->Radius;
		SphereGen.Box = FOrientedBox3d(FVector3d::Zero(),
			0.5 * FVector3d(SphereSettings->Subdivisions + 1,
				SphereSettings->Subdivisions + 1,
				SphereSettings->Subdivisions + 1));
		int EdgeNum = SphereSettings->Subdivisions + 1;
		SphereGen.EdgeVertices = FIndex3i(EdgeNum, EdgeNum, EdgeNum);
		SphereGen.bPolygroupPerQuad = (ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad);
		SphereGen.Generate();
		OutMesh->Copy(&SphereGen);
		break;
	}
	}
}



UTLLRN_AddStairsPrimitiveTool::UTLLRN_AddStairsPrimitiveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTLLRN_ProceduralStairsToolProperties>(TEXT("ShapeSettings")))
{
	AssetName = TEXT("Stairs");
	UInteractiveTool::SetToolDisplayName(LOCTEXT("StairsToolName", "Create Stairs"));
}

void UTLLRN_AddStairsPrimitiveTool::GenerateMesh(FDynamicMesh3* OutMesh) const
{
	const UTLLRN_ProceduralStairsToolProperties* StairSettings = Cast<UTLLRN_ProceduralStairsToolProperties>(ShapeSettings);
	switch (StairSettings->StairsType)
	{
	case ETLLRN_ProceduralStairsType::Linear:
	{
		FLinearStairGenerator StairGen;
		StairGen.StepWidth = StairSettings->StepWidth;
		StairGen.StepHeight = StairSettings->StepHeight;
		StairGen.StepDepth = StairSettings->StepDepth;
		StairGen.NumSteps = StairSettings->NumSteps;
		StairGen.bPolygroupPerQuad = (ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad);
		StairGen.Generate();
		OutMesh->Copy(&StairGen);
		break;
	}
	case ETLLRN_ProceduralStairsType::Floating:
	{
		FFloatingStairGenerator StairGen;
		StairGen.StepWidth = StairSettings->StepWidth;
		StairGen.StepHeight = StairSettings->StepHeight;
		StairGen.StepDepth = StairSettings->StepDepth;
		StairGen.NumSteps = StairSettings->NumSteps;
		StairGen.bPolygroupPerQuad = (ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad);
		StairGen.Generate();
		OutMesh->Copy(&StairGen);
		break;
	}
	case ETLLRN_ProceduralStairsType::Curved:
	{
		FCurvedStairGenerator StairGen;
		StairGen.StepWidth = StairSettings->StepWidth;
		StairGen.StepHeight = StairSettings->StepHeight;
		StairGen.NumSteps = StairSettings->NumSteps;
		StairGen.InnerRadius = StairSettings->InnerRadius;
		StairGen.CurveAngle = StairSettings->CurveAngle;
		StairGen.bPolygroupPerQuad = (ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad);
		StairGen.Generate();
		OutMesh->Copy(&StairGen);
		break;
	}
	case ETLLRN_ProceduralStairsType::Spiral:
	{
		FSpiralStairGenerator StairGen;
		StairGen.StepWidth = StairSettings->StepWidth;
		StairGen.StepHeight = StairSettings->StepHeight;
		StairGen.NumSteps = StairSettings->NumSteps;
		StairGen.InnerRadius = StairSettings->InnerRadius;
		StairGen.CurveAngle = StairSettings->SpiralAngle;
		StairGen.bPolygroupPerQuad = (ShapeSettings->PolygroupMode == ETLLRN_MakeMeshPolygroupMode::PerQuad);
		StairGen.Generate();
		OutMesh->Copy(&StairGen);
		break;
	}
	}
}


#undef LOCTEXT_NAMESPACE

