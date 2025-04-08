// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mechanics/TLLRN_CollisionPrimitivesMechanic.h"

#include "BaseBehaviors/SingleClickOrDragBehavior.h"
#include "BaseBehaviors/MouseHoverBehavior.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "BaseGizmos/TransformProxy.h"
#include "Drawing/TLLRN_PreviewGeometryActor.h"
#include "Drawing/TLLRN_LineSetComponent.h"
#include "Engine/World.h"
#include "InteractiveToolManager.h"
#include "ToolSceneQueriesUtil.h"
#include "ToolSetupUtil.h"
#include "Transforms/TLLRN_MultiTransformer.h"
#include "Physics/PhysicsDataCollection.h"
#include "Util/ColorConstants.h"
#include "Generators/LineSegmentGenerators.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_CollisionPrimitivesMechanic)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UTLLRN_CollisionPrimitivesMechanic"

namespace TLLRN_CollisionPrimitivesMechanicLocals
{
	static const FText CollisionPrimitiveDeselectionTransactionText = LOCTEXT("CollisionPrimitiveDeselection", "Collision Primitive Deselection");
	static const FText CollisionPrimitiveSelectionTransactionText = LOCTEXT("CollisionPrimitiveSelection", "Collision Primitive Selection");
	static const FText CollisionPrimitiveMovementTransactionText = LOCTEXT("CollisionPrimitiveGeometryChange", "Collision Primitive Geometry Change");

	void Toggle(TSet<int>& Selection, const TSet<int>& NewSelection)
	{
		for (const int& NewElement : NewSelection)
		{
			if (Selection.Remove(NewElement) == 0)
			{
				Selection.Add(NewElement);
			}
		}
	}

	// TODO Move to SetUtil namespace and call it in FGroupTopologySelection::operator==
	bool IsEqual(const TSet<int32>& A, const TSet<int32>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}

		// No need to also check B.Includes(A) here since counts match and sets contain unique elements
		return A.Includes(B);
	}
}


void UTLLRN_CollisionPrimitivesMechanic::Setup(UInteractiveTool* ParentToolIn)
{
	UInteractionMechanic::Setup(ParentToolIn);

	MarqueeMechanic = NewObject<UTLLRN_RectangleMarqueeMechanic>(this);
	MarqueeMechanic->bUseExternalClickDragBehavior = true;
	MarqueeMechanic->Setup(ParentToolIn);
	MarqueeMechanic->OnDragRectangleStarted.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::OnDragRectangleStarted);
	MarqueeMechanic->OnDragRectangleChanged.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::OnDragRectangleChanged);
	MarqueeMechanic->OnDragRectangleFinished.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::OnDragRectangleFinished);

	USingleClickOrDragInputBehavior* ClickOrDragBehavior = NewObject<USingleClickOrDragInputBehavior>();
	ClickOrDragBehavior->Initialize(this, MarqueeMechanic);
	ClickOrDragBehavior->Modifiers.RegisterModifier(ShiftModifierID, FInputDeviceState::IsShiftKeyDown);
	ClickOrDragBehavior->Modifiers.RegisterModifier(CtrlModifierID, FInputDeviceState::IsCtrlKeyDown);
	ParentTool->AddInputBehavior(ClickOrDragBehavior);

	UMouseHoverBehavior* HoverBehavior = NewObject<UMouseHoverBehavior>();
	HoverBehavior->Initialize(this);
	ParentTool->AddInputBehavior(HoverBehavior);

	NormalSegmentColor = FColor::Red;
	HoverColor = FColor::Green;
	SelectedColor = FColor::Yellow;
	SegmentsThickness = 1.0f;

	float UseSegmentRadius = 5.0f;
	GeometrySetToleranceTest = [UseSegmentRadius](const FVector3d& A, const FVector3d& B) { return DistanceSquared(A, B) < UseSegmentRadius * UseSegmentRadius; };

	UInteractiveGizmoManager* GizmoManager = GetParentTool()->GetToolManager()->GetPairedGizmoManager();
	TranslateTransformProxy = NewObject<UTransformProxy>(this);
	SphereTransformProxy = NewObject<UTransformProxy>(this);
	BoxTransformProxy = NewObject<UTransformProxy>(this);
	CapsuleTransformProxy = NewObject<UTransformProxy>(this);
	FullTransformProxy = NewObject<UTransformProxy>(this);
	
	// TODO: Maybe don't have the gizmo's axes flip around when it crosses the origin, if possible?
	TranslateTransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GizmoManager,
		ETransformGizmoSubElements::TranslateAllAxes | ETransformGizmoSubElements::TranslateAllPlanes, this);

	SphereTransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GizmoManager,
		ETransformGizmoSubElements::TranslateAllAxes | ETransformGizmoSubElements::TranslateAllPlanes | ETransformGizmoSubElements::ScaleUniform, this);

	BoxTransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GizmoManager,
		ETransformGizmoSubElements::FullTranslateRotateScale, this);

	CapsuleTransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GizmoManager,
		ETransformGizmoSubElements::TranslateAllAxes | ETransformGizmoSubElements::TranslateAllPlanes | ETransformGizmoSubElements::ScaleAxisX | ETransformGizmoSubElements::ScaleAxisZ | ETransformGizmoSubElements::RotateAxisY | ETransformGizmoSubElements::RotateAxisX, this);

	FullTransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GizmoManager,
		ETransformGizmoSubElements::FullTranslateRotateScale, this);

	TranslateTransformProxy->OnTransformChanged.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformChanged);
	TranslateTransformProxy->OnBeginTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformStarted);
	TranslateTransformProxy->OnEndTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformEnded);

	TranslateTransformGizmo->SetActiveTarget(TranslateTransformProxy);
	TranslateTransformGizmo->SetVisibility(false);
	TranslateTransformGizmo->bUseContextCoordinateSystem = false;
	TranslateTransformGizmo->CurrentCoordinateSystem = EToolContextCoordinateSystem::Local;

	SphereTransformProxy->OnTransformChanged.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformChanged);
	SphereTransformProxy->OnBeginTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformStarted);
	SphereTransformProxy->OnEndTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformEnded);

	SphereTransformGizmo->SetActiveTarget(SphereTransformProxy);
	SphereTransformGizmo->SetVisibility(false);
	SphereTransformGizmo->bUseContextCoordinateSystem = false;
	SphereTransformGizmo->CurrentCoordinateSystem = EToolContextCoordinateSystem::Local;

	BoxTransformProxy->OnTransformChanged.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformChanged);
	BoxTransformProxy->OnBeginTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformStarted);
	BoxTransformProxy->OnEndTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformEnded);

	BoxTransformGizmo->SetActiveTarget(BoxTransformProxy);
	BoxTransformGizmo->SetVisibility(false);
	BoxTransformGizmo->bUseContextCoordinateSystem = false;
	BoxTransformGizmo->CurrentCoordinateSystem = EToolContextCoordinateSystem::Local;

	CapsuleTransformProxy->OnTransformChanged.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformChanged);
	CapsuleTransformProxy->OnBeginTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformStarted);
	CapsuleTransformProxy->OnEndTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformEnded);

	CapsuleTransformGizmo->SetActiveTarget(CapsuleTransformProxy);
	CapsuleTransformGizmo->SetVisibility(false);
	CapsuleTransformGizmo->bUseContextCoordinateSystem = false;
	CapsuleTransformGizmo->CurrentCoordinateSystem = EToolContextCoordinateSystem::Local;

	FullTransformProxy->OnTransformChanged.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformChanged);
	FullTransformProxy->OnBeginTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformStarted);
	FullTransformProxy->OnEndTransformEdit.AddUObject(this, &UTLLRN_CollisionPrimitivesMechanic::GizmoTransformEnded);

	FullTransformGizmo->SetActiveTarget(FullTransformProxy);
	FullTransformGizmo->SetVisibility(false);
	FullTransformGizmo->bUseContextCoordinateSystem = false;
	FullTransformGizmo->CurrentCoordinateSystem = EToolContextCoordinateSystem::Local;

	// Hold the active proxy, so we can capture undo information from whatever gizmo got moved by the user.
	CurrentActiveProxy = TranslateTransformProxy;
}

void UTLLRN_CollisionPrimitivesMechanic::Initialize(TSharedPtr<FPhysicsDataCollection> InPhysicsData,
											   const FAxisAlignedBox3d& MeshBoundsIn,
											   const FTransform3d& InLocalToWorldTransform)
{
	LocalToWorldTransform = InLocalToWorldTransform;
	PhysicsData = InPhysicsData;
	MeshBounds = MeshBoundsIn;
	SelectedPrimitiveIDs.Empty();
	UpdateGizmoLocation();
	RebuildDrawables();
	++CurrentChangeStamp;
}

void UTLLRN_CollisionPrimitivesMechanic::SetWorld(UWorld* World)
{
	// It may be unreasonable to worry about SetWorld being called more than once, but let's be safe anyway
	if (TLLRN_PreviewGeometry)
	{
		TLLRN_PreviewGeometry->Disconnect();
		TLLRN_PreviewGeometry = nullptr;
	} 

	// We need the world so we can create the geometry actor in the right place
	FRotator Rotation(0.0f, 0.0f, 0.0f);
	FActorSpawnParameters SpawnInfo;
	TLLRN_PreviewGeometry = NewObject<UTLLRN_PreviewGeometry>(this);
	TLLRN_PreviewGeometry->CreateInWorld(World, FTransform());

	DrawnPrimitiveEdges = TLLRN_PreviewGeometry->AddLineSet("CollisionPrimitives");
	DrawnPrimitiveEdges->SetLineMaterial(ToolSetupUtil::GetDefaultLineComponentMaterial(GetParentTool()->GetToolManager(), false));

}

void UTLLRN_CollisionPrimitivesMechanic::Shutdown()
{
	LongTransactions.CloseAll(GetParentTool()->GetToolManager());

	if (TLLRN_PreviewGeometry)
	{
		TLLRN_PreviewGeometry->Disconnect();
		TLLRN_PreviewGeometry = nullptr;
	}

	// Calls shutdown on gizmo and destroys it.
	UInteractiveGizmoManager* GizmoManager = GetParentTool()->GetToolManager()->GetPairedGizmoManager();
	GizmoManager->DestroyAllGizmosByOwner(this);
}


void UTLLRN_CollisionPrimitivesMechanic::Render(IToolsContextRenderAPI* RenderAPI)
{
	// Cache the camera state
	GetParentTool()->GetToolManager()->GetContextQueriesAPI()->GetCurrentViewState(CachedCameraState);

	MarqueeMechanic->Render(RenderAPI);
}

void UTLLRN_CollisionPrimitivesMechanic::DrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI)
{
	MarqueeMechanic->DrawHUD(Canvas, RenderAPI);
}

void UTLLRN_CollisionPrimitivesMechanic::RebuildDrawables(bool bRegenerateCurveLists)
{
	// TODO: This code replicates a lot of the functionality found in CollisionGeometryVisualization.cpp, but with additional behavior needed for this class. Unification in the future would be good to explore.

	int32 CircleSteps = FMath::Max(24, 24);

	typedef TFunction<void(FPrimitiveRenderData& RenderData, TArray<FRenderableLine>& LinesOut, int32 PrimIndex, int32 CurveIndex, const FTransform& Transform, const FVector& A, const FVector& B, const FColor& Color)> WriteCurveFunc;
	WriteCurveFunc StoreCurve;
	if (bRegenerateCurveLists)
	{
		GeometrySet.Reset();
		DrawnPrimitiveEdges->Clear();
		PrimitiveRenderData.Empty();
		PrimitiveToCurveLookup.Empty();
		CurveToPrimitiveLookup.Empty();
		StoreCurve = [this](FPrimitiveRenderData& RenderData, TArray<FRenderableLine>& LinesOut, int32 PrimIndex, int32 CurveIndex, const FTransform& Transform, const FVector& A, const FVector& B, const FColor& Color) -> void
		{
			FVector A_World, B_World;
			A_World = Transform.TransformPosition(A);
			B_World = Transform.TransformPosition(B);
			LinesOut.Add(FRenderableLine(A_World, B_World, Color, SegmentsThickness, DepthBias));
			GeometrySet.AddCurve(CurveIndex, FPolyline3d(A_World, B_World));

			PrimitiveToCurveLookup.FindOrAdd(PrimIndex).Add(CurveIndex);
			CurveToPrimitiveLookup.FindOrAdd(CurveIndex) = PrimIndex;
		};
	}
	else
	{
		StoreCurve = [this](FPrimitiveRenderData& RenderData, TArray<FRenderableLine>& LinesOut, int32 PrimIndex, int32 CurveIndex, const FTransform& Transform, const FVector& A, const FVector& B, const FColor& Color) -> void
		{
			FVector A_World, B_World;
			A_World = Transform.TransformPosition(A);
			B_World = Transform.TransformPosition(B);

			DrawnPrimitiveEdges->SetLineStart(CurveIndex, A_World);
			DrawnPrimitiveEdges->SetLineEnd(CurveIndex, B_World);
			GeometrySet.UpdateCurve(CurveIndex, FPolyline3d(A_World, B_World));

			ensure(PrimitiveToCurveLookup[PrimIndex].Contains(CurveIndex));
			ensure(CurveToPrimitiveLookup[CurveIndex] == PrimIndex);
		};
	}


	const FKAggregateGeom& AggGeom = PhysicsData->AggGeom;
	int32 PrimIndex = 0;
	int32 GobalCurveIndex = 0;
	// spheres are draw as 3 orthogonal circles
	for (int32 Index = 0; Index < AggGeom.SphereElems.Num(); Index++)
	{
		FPrimitiveRenderData RenderData;
		RenderData.PrimIndex = PrimIndex;
		RenderData.LineIndexStart = GobalCurveIndex;
		FColor Color = SelectedPrimitiveIDs.Contains(PrimIndex) ? SelectedColor : NormalSegmentColor;
		RenderData.RenderColor = Color;
		RenderData.ShapeType = EAggCollisionShape::Sphere;

		int32 CurveIndex = 0;
		DrawnPrimitiveEdges->AddLines(1, [this, bRegenerateCurveLists, &StoreCurve, &RenderData, &AggGeom, &Color, CircleSteps, PrimIndex, &GobalCurveIndex, Index](int32 UnusedIndex, TArray<FRenderableLine>& LinesOut)
			{	
				const FKSphereElem& Sphere = AggGeom.SphereElems[Index];
				FTransform ElemTransform = Sphere.GetTransform();
				ElemTransform.ScaleTranslation(PhysicsData->ExternalScale3D);
				if (bRegenerateCurveLists)
				{					
					RenderData.Transform = ElemTransform;
				}
				float Radius = PhysicsData->ExternalScale3D.GetAbsMin() * Sphere.Radius;
				UE::Geometry::GenerateCircleSegments<float>(CircleSteps, Radius, FVector3f::Zero(), FVector3f::UnitX(), FVector3f::UnitY(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) {
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform * LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});
				UE::Geometry::GenerateCircleSegments<float>(CircleSteps, Radius, FVector3f::Zero(), FVector3f::UnitX(), FVector3f::UnitZ(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) {
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform * LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});
				UE::Geometry::GenerateCircleSegments<float>(CircleSteps, Radius, FVector3f::Zero(), FVector3f::UnitY(), FVector3f::UnitZ(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) { 
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform * LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});
			});
		RenderData.LineIndexEnd = GobalCurveIndex;
		if (bRegenerateCurveLists)
		{
			PrimitiveRenderData.Add(RenderData);
		}
		PrimIndex++;
	}


	// boxes are drawn as boxes
	for (int32 Index = 0; Index < AggGeom.BoxElems.Num(); Index++)
	{
		FPrimitiveRenderData RenderData;
		RenderData.PrimIndex = PrimIndex;
		RenderData.LineIndexStart = GobalCurveIndex;
		FColor Color = SelectedPrimitiveIDs.Contains(PrimIndex) ? SelectedColor : NormalSegmentColor;
		RenderData.RenderColor = Color;
		RenderData.ShapeType = EAggCollisionShape::Box;

		int32 CurveIndex = 0;
		DrawnPrimitiveEdges->AddLines(1, [this, bRegenerateCurveLists, &StoreCurve, &RenderData, &AggGeom, &Color, CircleSteps, PrimIndex, &GobalCurveIndex, Index](int32 UnusedIndex, TArray<FRenderableLine>& LinesOut)
			{
				const FKBoxElem& Box = AggGeom.BoxElems[Index];
				FTransform ElemTransform = Box.GetTransform();
				ElemTransform.ScaleTranslation(PhysicsData->ExternalScale3D);
				if (bRegenerateCurveLists)
				{
					RenderData.Transform = ElemTransform;
				}								
				FVector3f HalfDimensions(
					PhysicsData->ExternalScale3D.X * Box.X * 0.5f,
					PhysicsData->ExternalScale3D.Y * Box.Y * 0.5f,
					PhysicsData->ExternalScale3D.Z * Box.Z * 0.5f);
				UE::Geometry::GenerateBoxSegments<float>(HalfDimensions, FVector3f::Zero(), FVector3f::UnitX(), FVector3f::UnitY(), FVector3f::UnitZ(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) {
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});
			});
		RenderData.LineIndexEnd = GobalCurveIndex;
		if (bRegenerateCurveLists)
		{
			PrimitiveRenderData.Add(RenderData);
		}
		PrimIndex++;
	}


	// capsules are draw as two hemispheres (with 3 intersecting arcs/circles) and connecting lines
	for (int32 Index = 0; Index < AggGeom.SphylElems.Num(); Index++)
	{
		FPrimitiveRenderData RenderData;
		RenderData.PrimIndex = PrimIndex;
		RenderData.LineIndexStart = GobalCurveIndex;
		FColor Color = SelectedPrimitiveIDs.Contains(PrimIndex) ? SelectedColor : NormalSegmentColor;
		RenderData.RenderColor = Color;
		RenderData.ShapeType = EAggCollisionShape::Sphyl;

		int32 CurveIndex = 0;
		DrawnPrimitiveEdges->AddLines(1, [this, bRegenerateCurveLists, &StoreCurve, &RenderData, &AggGeom, &Color, CircleSteps, PrimIndex, &GobalCurveIndex, Index](int32 UnusedIndex, TArray<FRenderableLine>& LinesOut)
			{
				const FKSphylElem& Capsule = AggGeom.SphylElems[Index];
				FTransform ElemTransform = Capsule.GetTransform();
				ElemTransform.ScaleTranslation(PhysicsData->ExternalScale3D);
				if (bRegenerateCurveLists)
				{
					RenderData.Transform = ElemTransform;
				}					const float HalfLength = Capsule.GetScaledCylinderLength(PhysicsData->ExternalScale3D) * .5f;
				const float Radius = Capsule.GetScaledRadius(PhysicsData->ExternalScale3D);
				FVector3f Top(0, 0, HalfLength), Bottom(0, 0, -HalfLength);

				// top and bottom circles
				UE::Geometry::GenerateCircleSegments<float>(CircleSteps, Radius, Top, FVector3f::UnitX(), FVector3f::UnitY(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) { 
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});
				UE::Geometry::GenerateCircleSegments<float>(CircleSteps, Radius, Bottom, FVector3f::UnitX(), FVector3f::UnitY(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) { 
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});

				// top dome
				UE::Geometry::GenerateArcSegments<float>(CircleSteps, Radius, 0.0, PI, Top, FVector3f::UnitY(), FVector3f::UnitZ(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) { 
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});
				UE::Geometry::GenerateArcSegments<float>(CircleSteps, Radius, 0.0, PI, Top, FVector3f::UnitX(), FVector3f::UnitZ(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) { 
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});

				// bottom dome
				UE::Geometry::GenerateArcSegments<float>(CircleSteps, Radius, 0.0, -PI, Bottom, FVector3f::UnitY(), FVector3f::UnitZ(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) { 
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});
				UE::Geometry::GenerateArcSegments<float>(CircleSteps, Radius, 0.0, -PI, Bottom, FVector3f::UnitX(), FVector3f::UnitZ(), FTransformSRT3f(),
					[&](const FVector3f& A, const FVector3f& B) { 
						StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					});

				// connecting lines
				for (int k = 0; k < 2; ++k)
				{
					FVector A, B;

					FVector DX = (k < 1) ? FVector(-Radius, 0, 0) : FVector(Radius, 0, 0);
					A = (FVector)Top + DX;
					B = (FVector)Bottom + DX;
					StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);

					FVector DY = (k < 1) ? FVector(0, -Radius, 0) : FVector(0, Radius, 0);
					A = (FVector)Top + DY;
					B = (FVector)Bottom + DY;
					StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
				}
			});
		RenderData.LineIndexEnd = GobalCurveIndex;
		if (bRegenerateCurveLists)
		{
			PrimitiveRenderData.Add(RenderData);
		}
		PrimIndex++;
	}

	// convexes are drawn as mesh edges
	for (int32 Index = 0; Index < AggGeom.ConvexElems.Num(); Index++)
	{
		FPrimitiveRenderData RenderData;
		RenderData.PrimIndex = PrimIndex;
		RenderData.LineIndexStart = GobalCurveIndex;
		FColor Color = SelectedPrimitiveIDs.Contains(PrimIndex) ? SelectedColor : NormalSegmentColor;
		RenderData.RenderColor = Color;
		RenderData.ShapeType = EAggCollisionShape::Convex;

		int32 CurveIndex = 0;
		DrawnPrimitiveEdges->AddLines(1, [this, bRegenerateCurveLists, &StoreCurve, &RenderData, &AggGeom, &Color, CircleSteps, PrimIndex, &GobalCurveIndex, Index](int32 UnusedIndex, TArray<FRenderableLine>& LinesOut)
			{
				const FKConvexElem& Convex = AggGeom.ConvexElems[Index];
				FTransform ElemTransform = Convex.GetTransform();
				ElemTransform.ScaleTranslation(PhysicsData->ExternalScale3D);
				ElemTransform.MultiplyScale3D(PhysicsData->ExternalScale3D);
				if (bRegenerateCurveLists)
				{
					RenderData.Transform = ElemTransform;
				}				int32 NumTriangles = Convex.IndexData.Num() / 3;
				for (int32 k = 0; k < NumTriangles; ++k)
				{
					FVector A = Convex.VertexData[Convex.IndexData[3 * k]];
					FVector B = Convex.VertexData[Convex.IndexData[3 * k + 1]];
					FVector C = Convex.VertexData[Convex.IndexData[3 * k + 2]];
					StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
					StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)B, (FVector)C, Color);
					StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)C, (FVector)A, Color);
				}
			});
		RenderData.LineIndexEnd = GobalCurveIndex;
		if (bRegenerateCurveLists)
		{
			PrimitiveRenderData.Add(RenderData);
		}
		PrimIndex++;
	}

	// for Level Sets draw the grid cells where phi < 0
	for (int32 Index = 0; Index < AggGeom.LevelSetElems.Num(); Index++)
	{
		FPrimitiveRenderData RenderData;
		RenderData.PrimIndex = PrimIndex;
		RenderData.LineIndexStart = GobalCurveIndex;
		FColor Color = SelectedPrimitiveIDs.Contains(PrimIndex) ? SelectedColor : NormalSegmentColor;
		RenderData.RenderColor = Color;
		RenderData.ShapeType = EAggCollisionShape::LevelSet;

		int32 CurveIndex = 0;
		DrawnPrimitiveEdges->AddLines(1, [this, bRegenerateCurveLists, &StoreCurve, &RenderData, &AggGeom, &Color, CircleSteps, PrimIndex, &GobalCurveIndex, Index](int32 UnusedIndex, TArray<FRenderableLine>& LinesOut)
			{
				const FKLevelSetElem& LevelSet = AggGeom.LevelSetElems[Index];

				FTransform ElemTransform = LevelSet.GetTransform();
				ElemTransform.ScaleTranslation(PhysicsData->ExternalScale3D);
				ElemTransform.MultiplyScale3D(PhysicsData->ExternalScale3D);
				if (bRegenerateCurveLists)
				{
					RenderData.Transform = ElemTransform;
				}
				auto GenerateBoxSegmentsFromFBox = [&](const FBox& Box)
				{
					const FVector3d Center = Box.GetCenter();
					const FVector3d HalfDimensions = 0.5 * (Box.Max - Box.Min);

					UE::Geometry::GenerateBoxSegments<double>(HalfDimensions, Center, FVector3d::UnitX(), FVector3d::UnitY(), FVector3d::UnitZ(), FTransformSRT3d(),
						[&](const FVector3d& A, const FVector3d& B) {
							StoreCurve(RenderData, LinesOut, PrimIndex, GobalCurveIndex++, ElemTransform* LocalToWorldTransform, (FVector)A, (FVector)B, Color);
						});
				};

				const FBox TotalGridBox = LevelSet.UntransformedAABB();
				GenerateBoxSegmentsFromFBox(TotalGridBox);

				TArray<FBox> CellBoxes;
				const double Threshold = UE_KINDA_SMALL_NUMBER;		// allow slightly greater than zero for visualization purposes
				LevelSet.GetInteriorGridCells(CellBoxes, Threshold);

				for (const FBox& CellBox : CellBoxes)
				{
					GenerateBoxSegmentsFromFBox(CellBox);
				}

			});
		RenderData.LineIndexEnd = GobalCurveIndex;
		if (bRegenerateCurveLists)
		{
			PrimitiveRenderData.Add(RenderData);
		}
		PrimIndex++;
	}

	//  TODO: Unclear whether we actually use these in the Engine, for UBodySetup? Does not appear to be supported by UxX import system,
	// and online documentation suggests they may only be supported for cloth?
	ensure(AggGeom.TaperedCapsuleElems.Num() == 0);

}


void UTLLRN_CollisionPrimitivesMechanic::UpdateDrawables()
{
	UpdateGizmoLocation();
	UpdateGizmoVisibility();

	for (int32 PrimIndex = 0; PrimIndex < PrimitiveRenderData.Num(); ++PrimIndex)
	{
		PrimitiveRenderData[PrimIndex].RenderColor = SelectedPrimitiveIDs.Contains(PrimIndex) ? SelectedColor : NormalSegmentColor;

		for (int32 CurveId : PrimitiveToCurveLookup[PrimIndex])
		{
			DrawnPrimitiveEdges->SetLineColor(CurveId, PrimitiveRenderData[PrimIndex].RenderColor);
		}
	}
}


void UTLLRN_CollisionPrimitivesMechanic::GizmoTransformStarted(UTransformProxy* Proxy)
{
	ParentTool->GetToolManager()->BeginUndoTransaction(TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveMovementTransactionText);
	PrimitivePreTransform = MakeShared<FKAggregateGeom>(PhysicsData->AggGeom);

	GizmoStartPosition = Proxy->GetTransform().GetTranslation();
	GizmoStartRotation = Proxy->GetTransform().GetRotation();
	GizmoStartScale = Proxy->GetTransform().GetScale3D();

	bGizmoBeingDragged = true;
}

void UTLLRN_CollisionPrimitivesMechanic::GizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform)
{
	if (SelectedPrimitiveIDs.Num() == 0 || !bGizmoBeingDragged)
	{
		return;
	}

	if (Proxy->bSetPivotMode)
	{
		return;
	}

	FTransform LocalTransform = Transform * LocalToWorldTransform.Inverse();



	FQuaterniond DeltaRotation = (FQuaterniond) ( LocalToWorldTransform.InverseTransformRotation( Transform.GetRotation() ) * LocalToWorldTransform.InverseTransformRotation( GizmoStartRotation) .Inverse() );
	FVector DeltaScale = Transform.GetScale3D() / GizmoStartScale;

	FTransformSRT3d DeltaTransform;
	DeltaTransform.SetScale((FVector3d)DeltaScale);
	DeltaTransform.SetRotation((FQuaterniond)DeltaRotation);
	DeltaTransform.SetTranslation( (FVector3d)LocalTransform.GetTranslation() - (FVector3d)LocalToWorldTransform.InverseTransformPosition(GizmoStartPosition) );

	FKAggregateGeom& AggGeom = PhysicsData->AggGeom;
	FKAggregateGeom& PreTransformAggGeom = *PrimitivePreTransform;

	for (int32 PrimitiveID : SelectedPrimitiveIDs)
	{
		FKShapeElem* Prim = AggGeom.GetElement(PrimitiveID);
		FKShapeElem* PreTransformPrim = PreTransformAggGeom.GetElement(PrimitiveID);
		
		FTransform& ElemTM = PrimitiveRenderData[PrimitiveID].Transform;

		switch (Prim->GetShapeType())
		{
		case EAggCollisionShape::Sphere:
			ElemTM = StaticCast<FKSphereElem*>(PreTransformPrim)->GetTransform();
			ElemTM.Accumulate(DeltaTransform, ScalarRegister(1.0));
			ElemTM.NormalizeRotation();
			StaticCast<FKSphereElem*>(Prim)->Radius = StaticCast<FKSphereElem*>(PreTransformPrim)->Radius * ElemTM.GetScale3D().GetAbsMax();
			ElemTM.SetScale3D(FVector::One());
			StaticCast<FKSphereElem*>(Prim)->SetTransform(ElemTM);
			break;
		case EAggCollisionShape::Box:
			ElemTM = StaticCast<FKBoxElem*>(PreTransformPrim)->GetTransform();
			ElemTM.Accumulate(DeltaTransform, ScalarRegister(1.0));
			ElemTM.NormalizeRotation();
			StaticCast<FKBoxElem*>(Prim)->X = StaticCast<FKBoxElem*>(PreTransformPrim)->X * ElemTM.GetScale3D().X;
			StaticCast<FKBoxElem*>(Prim)->Y = StaticCast<FKBoxElem*>(PreTransformPrim)->Y * ElemTM.GetScale3D().Y;
			StaticCast<FKBoxElem*>(Prim)->Z = StaticCast<FKBoxElem*>(PreTransformPrim)->Z * ElemTM.GetScale3D().Z;
			ElemTM.SetScale3D(FVector::One());
			StaticCast<FKBoxElem*>(Prim)->SetTransform(ElemTM);
			break;
		case EAggCollisionShape::Sphyl:
			ElemTM = StaticCast<FKSphylElem*>(PreTransformPrim)->GetTransform();
			ElemTM.Accumulate(DeltaTransform, ScalarRegister(1.0));
			ElemTM.NormalizeRotation();
			StaticCast<FKSphylElem*>(Prim)->Radius = StaticCast<FKSphylElem*>(PreTransformPrim)->Radius * ElemTM.GetScale3D().GetAbs().X;
			StaticCast<FKSphylElem*>(Prim)->Length = StaticCast<FKSphylElem*>(PreTransformPrim)->Length * ElemTM.GetScale3D().GetAbs().Z;
			ElemTM.SetScale3D(FVector::One());
			StaticCast<FKSphylElem*>(Prim)->SetTransform(ElemTM);
			break;
		case EAggCollisionShape::Convex:
			ElemTM = StaticCast<FKConvexElem*>(PreTransformPrim)->GetTransform();
			ElemTM.Accumulate(DeltaTransform, ScalarRegister(1.0));
			ElemTM.NormalizeRotation();
			StaticCast<FKConvexElem*>(Prim)->SetTransform(ElemTM);
			break;
		case EAggCollisionShape::LevelSet:
			ElemTM = StaticCast<FKLevelSetElem*>(PreTransformPrim)->GetTransform();
			ElemTM.Accumulate(DeltaTransform, ScalarRegister(1.0));
			ElemTM.NormalizeRotation();
			StaticCast<FKLevelSetElem*>(Prim)->SetTransform(ElemTM);
			break;
		default:
			ensure(false); // If we encounter a non-supported primitive type,
			// we should catch this and figure out how to handle it when needed.
		};
	}

	RebuildDrawables(false);

	OnCollisionGeometryChanged.Broadcast();
}

void UTLLRN_CollisionPrimitivesMechanic::GizmoTransformEnded(UTransformProxy* Proxy)
{
	FKAggregateGeom& AggGeom = PhysicsData->AggGeom;
	for (int32 PrimitiveID : SelectedPrimitiveIDs)
	{
		FKShapeElem* Prim = AggGeom.GetElement(PrimitiveID);
		FTransform& ElemTM = PrimitiveRenderData[PrimitiveID].Transform;
		switch (Prim->GetShapeType())
		{
		case EAggCollisionShape::Sphere:
			StaticCast<FKSphereElem*>(Prim)->SetTransform(ElemTM);
			StaticCast<FKSphereElem*>(Prim)->Radius *= ElemTM.GetScale3D().GetAbsMax();
			break;
		case EAggCollisionShape::Box:
			StaticCast<FKBoxElem*>(Prim)->SetTransform(ElemTM);
			StaticCast<FKBoxElem*>(Prim)->X *= ElemTM.GetScale3D().X;
			StaticCast<FKBoxElem*>(Prim)->Y *= ElemTM.GetScale3D().Y;
			StaticCast<FKBoxElem*>(Prim)->Z *= ElemTM.GetScale3D().Z;
			break;
		case EAggCollisionShape::Sphyl:
			StaticCast<FKSphylElem*>(Prim)->SetTransform(ElemTM);
			StaticCast<FKSphylElem*>(Prim)->Radius *= ElemTM.GetScale3D().GetAbs().X;
			StaticCast<FKSphylElem*>(Prim)->Length *= ElemTM.GetScale3D().GetAbs().Z;
			break;
		case EAggCollisionShape::Convex:
			StaticCast<FKConvexElem*>(Prim)->SetTransform(ElemTM);
			break;
		case EAggCollisionShape::LevelSet:
			StaticCast<FKLevelSetElem*>(Prim)->SetTransform(ElemTM);
			break;
		default:
			ensure(false); // If we encounter a non-supported primitive type,
			// we should catch this and figure out how to handle it when needed.
		};
	}

	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicGeometryChange>(
		PrimitivePreTransform, MakeShared<FKAggregateGeom>(PhysicsData->AggGeom),
		CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveMovementTransactionText);

	// This just lets the tool know that the gizmo has finished moving and we've added it to the undo stack.
	// TODO: Add a different callback? "OnGizmoTransformChanged"?
	OnCollisionGeometryChanged.Broadcast();

	// TODO: When we implement snapping
	// We may need to reset the gizmo if our snapping caused the final Primitive position to differ from the gizmo position
	// UpdateGizmoLocation();

	ParentTool->GetToolManager()->EndUndoTransaction();		// was started in GizmoTransformStarted above
	
	RebuildDrawables(true);



	PrimitivePreTransform.Reset();
	bGizmoBeingDragged = false;
}

bool UTLLRN_CollisionPrimitivesMechanic::HitTest(const FInputDeviceRay& ClickPos, FInputRayHit& ResultOut)
{
	FGeometrySet3::FNearest Nearest;

	// See if we hit a curve for selection
	if (GeometrySet.FindNearestCurveToRay((FRay3d)ClickPos.WorldRay, Nearest, GeometrySetToleranceTest))
	{
		ResultOut = FInputRayHit(Nearest.RayParam);
		return true;
	}
	return false;
}

FInputRayHit UTLLRN_CollisionPrimitivesMechanic::IsHitByClick(const FInputDeviceRay& ClickPos)
{
	FInputRayHit Result;
	if (HitTest(ClickPos, Result))
	{
		return Result;
	}

	// Return a hit so we always capture and can clear the selection
	return FInputRayHit(TNumericLimits<float>::Max());
}

void UTLLRN_CollisionPrimitivesMechanic::OnClicked(const FInputDeviceRay& ClickPos)
{
	using namespace TLLRN_CollisionPrimitivesMechanicLocals;
	
	ParentTool->GetToolManager()->BeginUndoTransaction(CollisionPrimitiveSelectionTransactionText);
	
	const TSet<int32> PreClickSelection = SelectedPrimitiveIDs;
	
	FGeometrySet3::FNearest Nearest;
	if (GeometrySet.FindNearestCurveToRay((FRay3d)ClickPos.WorldRay, Nearest, GeometrySetToleranceTest))
	{
		if (ShouldAddToSelectionFunc())
		{
			if (ShouldRemoveFromSelectionFunc())
			{
				Toggle(SelectedPrimitiveIDs, TSet<int>{CurveToPrimitiveLookup[Nearest.ID]});
			}
			else
			{
				SelectedPrimitiveIDs.Add(CurveToPrimitiveLookup[Nearest.ID]);
			}
		}
		else if (ShouldRemoveFromSelectionFunc())
		{
			SelectedPrimitiveIDs.Remove(CurveToPrimitiveLookup[Nearest.ID]);
		}
		else
		{
			// Neither key pressed.
			SelectedPrimitiveIDs = TSet<int>{ CurveToPrimitiveLookup[Nearest.ID] };
		}
	}
	else
	{
		SelectedPrimitiveIDs.Empty();
	}
	
	UpdateDrawables();

	if (!IsEqual(PreClickSelection, SelectedPrimitiveIDs))
	{
		checkSlow(CurrentActiveProxy);
		const FTransform Transform = CurrentActiveProxy->GetTransform();
		ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
			PreClickSelection, false, Transform, Transform, CurrentChangeStamp), CollisionPrimitiveDeselectionTransactionText);
		ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
			SelectedPrimitiveIDs, true, Transform, Transform, CurrentChangeStamp), CollisionPrimitiveSelectionTransactionText);
		UpdateGizmoLocation();
		OnSelectionChanged.Broadcast();
	}

	ParentTool->GetToolManager()->EndUndoTransaction();
}

void UTLLRN_CollisionPrimitivesMechanic::UpdateGizmoLocation()
{
	if (!TranslateTransformGizmo || !SphereTransformGizmo || !BoxTransformGizmo || !CapsuleTransformGizmo || !FullTransformGizmo)
	{
		return;
	}

	FVector3d NewGizmoLocation(0, 0, 0);

	auto ForAllGizmos = [this](TFunction<void(UCombinedTransformGizmo& Gizmo)> GizmoFunc)
	{
		GizmoFunc(*TranslateTransformGizmo);
		GizmoFunc(*SphereTransformGizmo);
		GizmoFunc(*BoxTransformGizmo);
		GizmoFunc(*CapsuleTransformGizmo);
		GizmoFunc(*FullTransformGizmo);
	};

	if (SelectedPrimitiveIDs.Num() == 0)
	{
		ForAllGizmos([](UCombinedTransformGizmo& Gizmo)
		{
			Gizmo.ReinitializeGizmoTransform(FTransform());
		});
	}
	else
	{
		for (int32 PrimitiveID : SelectedPrimitiveIDs)
		{
			NewGizmoLocation += PrimitiveRenderData[PrimitiveID].Transform.GetLocation();
		}
		NewGizmoLocation /= (double)SelectedPrimitiveIDs.Num();
	}

	FQuat GizmoRotation = LocalToWorldTransform.GetRotation();

	// We are handling one selected primitive specially because multiple selected primitives don't get rotation controls
	if (SelectedPrimitiveIDs.Num() == 1)
	{
		GizmoRotation = (PrimitiveRenderData[SelectedPrimitiveIDs.Array()[0]].Transform * LocalToWorldTransform).GetRotation();
	}

	UpdateGizmoVisibility();

	FTransform NewTransform(GizmoRotation, LocalToWorldTransform.TransformPosition((FVector)NewGizmoLocation));
	ForAllGizmos([&NewTransform](UCombinedTransformGizmo& Gizmo)
	{
		Gizmo.ReinitializeGizmoTransform(NewTransform);
	});

	// Clear the child scale
	ForAllGizmos([](UCombinedTransformGizmo& Gizmo)
	{
		FVector GizmoScale{ 1.0, 1.0, 1.0 };
		Gizmo.SetNewChildScale(GizmoScale);
	});
}


void UTLLRN_CollisionPrimitivesMechanic::UpdateGizmoVisibility()
{
	if (!TranslateTransformGizmo || !SphereTransformGizmo || !BoxTransformGizmo || !CapsuleTransformGizmo || !FullTransformGizmo)
	{
		return;
	}

	auto ForAllGizmos = [this](TFunction<void(UCombinedTransformGizmo& Gizmo)> GizmoFunc)
	{
		GizmoFunc(*TranslateTransformGizmo);
		GizmoFunc(*SphereTransformGizmo);
		GizmoFunc(*BoxTransformGizmo);
		GizmoFunc(*CapsuleTransformGizmo);
		GizmoFunc(*FullTransformGizmo);
	};
	ForAllGizmos([](UCombinedTransformGizmo& Gizmo)
	{
		Gizmo.SetVisibility(false);
	});
	CurrentActiveProxy = TranslateTransformProxy;

	if(!(bIsDraggingRectangle || SelectedPrimitiveIDs.IsEmpty() || (ShouldHideGizmo.IsBound() && ShouldHideGizmo.Execute())))
	{
		// We are handling one selected primitive specially because each primitive type gets it's own defined gizmo
		if (SelectedPrimitiveIDs.Num() == 1)
		{
			switch (PrimitiveRenderData[SelectedPrimitiveIDs.Array()[0]].ShapeType)
			{
			case EAggCollisionShape::Sphere:
				SphereTransformGizmo->SetVisibility(true);
				CurrentActiveProxy = SphereTransformProxy;
				break;
			case EAggCollisionShape::Box:
				BoxTransformGizmo->SetVisibility(true);
				CurrentActiveProxy = BoxTransformProxy;
				break;
			case EAggCollisionShape::Sphyl:
				CapsuleTransformGizmo->SetVisibility(true);
				CurrentActiveProxy = CapsuleTransformProxy;
				break;
			case EAggCollisionShape::Convex:
				FullTransformGizmo->SetVisibility(true);
				CurrentActiveProxy = FullTransformProxy;
				break;
			case EAggCollisionShape::LevelSet:
				FullTransformGizmo->SetVisibility(true);
				CurrentActiveProxy = FullTransformProxy;
				break;
			default:
				ensure(false);
			}
		}
		else
		{
			TranslateTransformGizmo->SetVisibility(true);
		}
	}
}


bool UTLLRN_CollisionPrimitivesMechanic::DeselectPrimitive(int32 PrimitiveID)
{
	return SelectedPrimitiveIDs.Remove(PrimitiveID) != 0; // true if a primitive was removed
}

void UTLLRN_CollisionPrimitivesMechanic::SelectPrimitive(int32 PrimitiveID)
{
	SelectedPrimitiveIDs.Add(PrimitiveID);
}

FInputRayHit UTLLRN_CollisionPrimitivesMechanic::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos)
{
	FInputRayHit Result;
	HitTest(PressPos, Result);
	return Result;
}

void UTLLRN_CollisionPrimitivesMechanic::OnBeginHover(const FInputDeviceRay& DevicePos)
{
	OnUpdateHover(DevicePos);
}

void UTLLRN_CollisionPrimitivesMechanic::ClearHover()
{
	if (HoveredPrimitiveID >= 0)
	{
		for (int32 CurveId = PrimitiveRenderData[HoveredPrimitiveID].LineIndexStart; CurveId < PrimitiveRenderData[HoveredPrimitiveID].LineIndexEnd; ++CurveId)
		{
			DrawnPrimitiveEdges->SetLineColor(CurveId, PreHoverPrimitiveColor);
		}
		HoveredPrimitiveID = -1;
	}
}

bool UTLLRN_CollisionPrimitivesMechanic::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	FGeometrySet3::FNearest Nearest;

	// see if we're hovering a curve for selection
	if (GeometrySet.FindNearestCurveToRay((FRay3d)DevicePos.WorldRay, Nearest, GeometrySetToleranceTest))
	{
		// Only need to update the hover if we changed the curve
		if (Nearest.ID != HoveredPrimitiveID)
		{
			ClearHover();
			HoveredPrimitiveID = CurveToPrimitiveLookup[Nearest.ID];
			if (SelectedPrimitiveIDs.Contains(HoveredPrimitiveID))
			{
				PreHoverPrimitiveColor = SelectedColor;
			}
			else
			{
				PreHoverPrimitiveColor = PrimitiveRenderData[HoveredPrimitiveID].RenderColor;
			}
			for (int32 CurveId = PrimitiveRenderData[HoveredPrimitiveID].LineIndexStart; CurveId < PrimitiveRenderData[HoveredPrimitiveID].LineIndexEnd; ++CurveId)
			{
				DrawnPrimitiveEdges->SetLineColor(CurveId, HoverColor);
			}
		}
	}
	else
	{
		// Not hovering anything, so done hovering
		return false;
	}

	return true;
}

void UTLLRN_CollisionPrimitivesMechanic::OnEndHover()
{
	ClearHover();
}

// Detects Ctrl key state
void UTLLRN_CollisionPrimitivesMechanic::OnUpdateModifierState(int ModifierID, bool bIsOn)
{
	switch (ModifierID)
	{
	case ShiftModifierID:
		bShiftToggle = bIsOn;
		break;
	case CtrlModifierID:
		bCtrlToggle = bIsOn;
		break;
	default:
		break;
	}
}

// These get bound to the delegates on the marquee mechanic.
void UTLLRN_CollisionPrimitivesMechanic::OnDragRectangleStarted()
{
	using namespace TLLRN_CollisionPrimitivesMechanicLocals;

	PreDragSelection = SelectedPrimitiveIDs;
	bIsDraggingRectangle = true;
	UpdateGizmoVisibility();
	LongTransactions.Open(CollisionPrimitiveSelectionTransactionText, GetParentTool()->GetToolManager());
}

void UTLLRN_CollisionPrimitivesMechanic::OnDragRectangleChanged(const FCameraRectangle& Rectangle)
{
	using namespace TLLRN_CollisionPrimitivesMechanicLocals;
	
	TSet<int> CurveSelection, PrimitiveSelection;
	GeometrySet.ParallelFindAllCurvesSatisfying([&](const FPolyline3d& Segment)
	{
		return Rectangle.IsProjectedSegmentIntersectingRectangle(Segment.Start(), Segment.End());
	}, CurveSelection);

	for (int& Entry : CurveSelection)
	{
		PrimitiveSelection.Add(CurveToPrimitiveLookup[Entry]);
	}

	if (ShouldAddToSelectionFunc())
	{
		SelectedPrimitiveIDs = PreDragSelection;
		if (ShouldRemoveFromSelectionFunc())
		{
			Toggle(SelectedPrimitiveIDs, PrimitiveSelection);
		}
		else
		{
			SelectedPrimitiveIDs.Append(PrimitiveSelection);
		}
	}
	else if (ShouldRemoveFromSelectionFunc())
	{
		SelectedPrimitiveIDs = PreDragSelection.Difference(PrimitiveSelection);
	}
	else
	{
		// Neither key pressed.
		SelectedPrimitiveIDs = PrimitiveSelection;
	}

	UpdateDrawables();
}

void UTLLRN_CollisionPrimitivesMechanic::OnDragRectangleFinished(const FCameraRectangle& Rectangle, bool bCancelled)
{
	using namespace TLLRN_CollisionPrimitivesMechanicLocals;

	bIsDraggingRectangle = false;

	if (!IsEqual(PreDragSelection, SelectedPrimitiveIDs))
	{
		checkSlow(CurrentActiveProxy);
		const FTransform Transform = CurrentActiveProxy->GetTransform();
		ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
			PreDragSelection, false, Transform, Transform, CurrentChangeStamp), CollisionPrimitiveDeselectionTransactionText);
		ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
			SelectedPrimitiveIDs, true, Transform, Transform, CurrentChangeStamp), CollisionPrimitiveSelectionTransactionText);
		
		OnSelectionChanged.Broadcast();
	}
	// We hid the gizmo at rectangle start, so it needs updating now.
	UpdateGizmoLocation();

	LongTransactions.Close(GetParentTool()->GetToolManager());

	UpdateDrawables();
}

void UTLLRN_CollisionPrimitivesMechanic::AddSphere()
{
	// TOOD: Refactor these Add<Shape> methods into a single templated solution

	FKAggregateGeom& AggGeom = PhysicsData->AggGeom;
	TSharedPtr<FKAggregateGeom> PreAddGeometry = MakeShared<FKAggregateGeom>(PhysicsData->AggGeom);

	AggGeom.SphereElems.Add(FKSphereElem(MeshBounds.MaxDim()/2.0));

	TSet<int32> PreAddSelection = SelectedPrimitiveIDs;
	SelectedPrimitiveIDs.Empty();
	SelectedPrimitiveIDs.Add(AggGeom.SphereElems.Num() - 1);

	ParentTool->GetToolManager()->BeginUndoTransaction(LOCTEXT("AddSphereUndo", "Add Sphere Collision Primitive"));


	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicGeometryChange>(
		PreAddGeometry, MakeShared<FKAggregateGeom>(PhysicsData->AggGeom),
		CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveMovementTransactionText);
	OnCollisionGeometryChanged.Broadcast();

	checkSlow(CurrentActiveProxy);
	const FTransform Transform = CurrentActiveProxy->GetTransform();
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		PreAddSelection, false, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveDeselectionTransactionText);
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		SelectedPrimitiveIDs, true, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveSelectionTransactionText);

	OnSelectionChanged.Broadcast();

	ParentTool->GetToolManager()->EndUndoTransaction();


	RebuildDrawables(true);
	UpdateGizmoLocation();
	UpdateGizmoVisibility();
}

void UTLLRN_CollisionPrimitivesMechanic::AddBox()
{
	// TOOD: Refactor these Add<Shape> methods into a single templated solution

	FKAggregateGeom& AggGeom = PhysicsData->AggGeom;
	TSharedPtr<FKAggregateGeom> PreAddGeometry = MakeShared<FKAggregateGeom>(PhysicsData->AggGeom);

	AggGeom.BoxElems.Add(FKBoxElem(MeshBounds.Dimension(0), MeshBounds.Dimension(1), MeshBounds.Dimension(2)));

	TSet<int32> PreAddSelection = SelectedPrimitiveIDs;
	SelectedPrimitiveIDs.Empty();
	SelectedPrimitiveIDs.Add(AggGeom.SphereElems.Num() + AggGeom.BoxElems.Num() - 1);

	ParentTool->GetToolManager()->BeginUndoTransaction(LOCTEXT("AddBoxUndo", "Add Box Collision Primitive"));


	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicGeometryChange>(
		PreAddGeometry, MakeShared<FKAggregateGeom>(PhysicsData->AggGeom),
		CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveMovementTransactionText);
	OnCollisionGeometryChanged.Broadcast();

	checkSlow(CurrentActiveProxy);
	const FTransform Transform = CurrentActiveProxy->GetTransform();
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		PreAddSelection, false, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveDeselectionTransactionText);
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		SelectedPrimitiveIDs, true, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveSelectionTransactionText);

	OnSelectionChanged.Broadcast();

	ParentTool->GetToolManager()->EndUndoTransaction();


	RebuildDrawables(true);
	UpdateGizmoLocation();
	UpdateGizmoVisibility();
}

void UTLLRN_CollisionPrimitivesMechanic::AddCapsule()
{
	// TOOD: Refactor these Add<Shape> methods into a single templated solution

	FKAggregateGeom& AggGeom = PhysicsData->AggGeom;
	TSharedPtr<FKAggregateGeom> PreAddGeometry = MakeShared<FKAggregateGeom>(PhysicsData->AggGeom);

	AggGeom.SphylElems.Add(FKSphylElem(FMath::Max(MeshBounds.Dimension(0), MeshBounds.Dimension(1)) / 2.0, MeshBounds.Dimension(2)));

	TSet<int32> PreAddSelection = SelectedPrimitiveIDs;
	SelectedPrimitiveIDs.Empty();
	SelectedPrimitiveIDs.Add(AggGeom.SphereElems.Num() + AggGeom.BoxElems.Num() + AggGeom.SphylElems.Num() - 1);

	ParentTool->GetToolManager()->BeginUndoTransaction(LOCTEXT("AddCapsuleUndo", "Add Capsule Collision Primitive"));

	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicGeometryChange>(
		PreAddGeometry, MakeShared<FKAggregateGeom>(PhysicsData->AggGeom),
		CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveMovementTransactionText);
	OnCollisionGeometryChanged.Broadcast();

	checkSlow(CurrentActiveProxy);
	const FTransform Transform = CurrentActiveProxy->GetTransform();
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		PreAddSelection, false, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveDeselectionTransactionText);
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		SelectedPrimitiveIDs, true, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveSelectionTransactionText);

	OnSelectionChanged.Broadcast();

	ParentTool->GetToolManager()->EndUndoTransaction();


	RebuildDrawables(true);
	UpdateGizmoLocation();
	UpdateGizmoVisibility();
}

void UTLLRN_CollisionPrimitivesMechanic::DuplicateSelectedPrimitive()
{
	using namespace TLLRN_CollisionPrimitivesMechanicLocals;

	TArray<FKSphereElem> NewSphereElems;
	TArray<FKBoxElem> NewBoxElems;
	TArray<FKSphylElem> NewSphylElems;
	TArray<FKConvexElem> NewConvexElems;
	TArray<FKLevelSetElem> NewLevelSetElems;

	FKAggregateGeom& AggGeom = PhysicsData->AggGeom;
	TSharedPtr<FKAggregateGeom> PreAddGeometry = MakeShared<FKAggregateGeom>(PhysicsData->AggGeom);
	
	if (SelectedPrimitiveIDs.IsEmpty())
	{
		return;
	}

	for (int32 PrimIndex : SelectedPrimitiveIDs)
	{
		const FKShapeElem* Prim = AggGeom.GetElement(PrimIndex);
		switch (Prim->GetShapeType())
		{
		case EAggCollisionShape::Sphere:
			NewSphereElems.Add(FKSphereElem(*StaticCast<const FKSphereElem*>(Prim)));
			break;
		case EAggCollisionShape::Box:
			NewBoxElems.Add(FKBoxElem(*StaticCast<const FKBoxElem*>(Prim)));
			break;
		case EAggCollisionShape::Sphyl:
			NewSphylElems.Add(FKSphylElem(*StaticCast<const FKSphylElem*>(Prim)));
			break;
		case EAggCollisionShape::Convex:
			NewConvexElems.Add(FKConvexElem(*StaticCast<const FKConvexElem*>(Prim)));
			break;
		case EAggCollisionShape::LevelSet:
			NewLevelSetElems.Add(FKLevelSetElem(*StaticCast<const FKLevelSetElem*>(Prim)));
			break;
		default:
			ensure(false); // If we encounter a non-supported primitive type,
			// we should catch this and figure out how to handle it when needed.
		};
	}

	TSet<int32> NewSelectedIDs;
	TSet<int32> PreDuplicateSelection = SelectedPrimitiveIDs;
	int32 TotalIDs = 0;

	for (int32 NewID = AggGeom.SphereElems.Num(); NewID < AggGeom.SphereElems.Num() + NewSphereElems.Num(); ++NewID)
	{
		NewSelectedIDs.Add(NewID);
	}
	AggGeom.SphereElems.Append(NewSphereElems);
	TotalIDs += AggGeom.SphereElems.Num();

	for (int32 NewID = AggGeom.BoxElems.Num()+TotalIDs; NewID < AggGeom.BoxElems.Num() + NewBoxElems.Num() + TotalIDs; ++NewID)
	{
		NewSelectedIDs.Add(NewID);
	}
	AggGeom.BoxElems.Append(NewBoxElems);
	TotalIDs += AggGeom.BoxElems.Num();

	for (int32 NewID = AggGeom.SphylElems.Num() + TotalIDs; NewID < AggGeom.SphylElems.Num() + NewSphylElems.Num() + TotalIDs; ++NewID)
	{
		NewSelectedIDs.Add(NewID);
	}
	AggGeom.SphylElems.Append(NewSphylElems);
	TotalIDs += AggGeom.SphylElems.Num();

	for (int32 NewID = AggGeom.ConvexElems.Num() + TotalIDs; NewID < AggGeom.ConvexElems.Num() + NewConvexElems.Num() + TotalIDs; ++NewID)
	{
		NewSelectedIDs.Add(NewID);
	}
	AggGeom.ConvexElems.Append(NewConvexElems);
	TotalIDs += AggGeom.ConvexElems.Num();

	for (int32 NewID = AggGeom.LevelSetElems.Num() + TotalIDs; NewID < AggGeom.LevelSetElems.Num() + NewLevelSetElems.Num() + TotalIDs; ++NewID)
	{
		NewSelectedIDs.Add(NewID);
	}
	AggGeom.LevelSetElems.Append(NewLevelSetElems);
	TotalIDs += AggGeom.LevelSetElems.Num();

	ParentTool->GetToolManager()->BeginUndoTransaction(LOCTEXT("DuplicateSelectionUndo", "Duplicate Selected Collision Primitives"));

	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicGeometryChange>(
		PreAddGeometry, MakeShared<FKAggregateGeom>(PhysicsData->AggGeom),
		CurrentChangeStamp), CollisionPrimitiveMovementTransactionText);
	OnCollisionGeometryChanged.Broadcast();

	SelectedPrimitiveIDs = NewSelectedIDs;

	checkSlow(CurrentActiveProxy);
	const FTransform Transform = CurrentActiveProxy->GetTransform();
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		PreDuplicateSelection, false, Transform, Transform, CurrentChangeStamp), CollisionPrimitiveDeselectionTransactionText);
    ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		SelectedPrimitiveIDs, true, Transform, Transform, CurrentChangeStamp), CollisionPrimitiveSelectionTransactionText);

	OnSelectionChanged.Broadcast();

	ParentTool->GetToolManager()->EndUndoTransaction();


	RebuildDrawables(true);
	UpdateGizmoLocation();
	UpdateGizmoVisibility();
}

void UTLLRN_CollisionPrimitivesMechanic::DeleteSelectedPrimitive()
{
	int32 PrimsDeleted = 0;
	FKAggregateGeom& AggGeom = PhysicsData->AggGeom;
	TSharedPtr<FKAggregateGeom> PreAddGeometry = MakeShared<FKAggregateGeom>(PhysicsData->AggGeom);


	TArray<int32> SortedSelectedIDs = SelectedPrimitiveIDs.Array();
	SortedSelectedIDs.Sort();

	for (int32 PrimIndex : SortedSelectedIDs)
	{
		int Index = PrimIndex - PrimsDeleted;
		ensure(Index < AggGeom.GetElementCount());
		PrimsDeleted++;
		if (Index < AggGeom.SphereElems.Num())
		{
			AggGeom.SphereElems.RemoveAt(Index);
			continue;
		}
		Index -= AggGeom.SphereElems.Num();
		if (Index < AggGeom.BoxElems.Num())
		{
			AggGeom.BoxElems.RemoveAt(Index);
			continue;
		}
		Index -= AggGeom.BoxElems.Num();
		if (Index < AggGeom.SphylElems.Num())
		{
			AggGeom.SphylElems.RemoveAt(Index);
			continue;
		}
		Index -= AggGeom.SphylElems.Num();
		if (Index < AggGeom.ConvexElems.Num())
		{
			AggGeom.ConvexElems.RemoveAt(Index);
			continue;
		}
		Index -= AggGeom.ConvexElems.Num();
		if (Index < AggGeom.TaperedCapsuleElems.Num())
		{
			AggGeom.TaperedCapsuleElems.RemoveAt(Index);
			continue;
		}
		Index -= AggGeom.TaperedCapsuleElems.Num();
		if (Index < AggGeom.LevelSetElems.Num())
		{
			AggGeom.LevelSetElems.RemoveAt(Index);
			continue;
		}
		Index -= AggGeom.LevelSetElems.Num();
		if (Index < AggGeom.SkinnedLevelSetElems.Num())
		{
			AggGeom.SkinnedLevelSetElems.RemoveAt(Index);
			continue;
		}		
	}

	ParentTool->GetToolManager()->BeginUndoTransaction(LOCTEXT("DeleteSelectionUndo", "Delete Selected Collision Primitives"));

	TSet<int32> PreDeleteSelection = SelectedPrimitiveIDs;
	SelectedPrimitiveIDs.Empty();

	checkSlow(CurrentActiveProxy);
	const FTransform Transform = CurrentActiveProxy->GetTransform();
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		PreDeleteSelection, false, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveDeselectionTransactionText);
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		SelectedPrimitiveIDs, true, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveSelectionTransactionText);
	OnSelectionChanged.Broadcast();

	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicGeometryChange>(
		PreAddGeometry, MakeShared<FKAggregateGeom>(PhysicsData->AggGeom),
		CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveMovementTransactionText);
	OnCollisionGeometryChanged.Broadcast();

	ParentTool->GetToolManager()->EndUndoTransaction();


	RebuildDrawables(true);
	UpdateGizmoLocation();
	UpdateGizmoVisibility();
}

void UTLLRN_CollisionPrimitivesMechanic::DeleteAllPrimitives()
{
	FKAggregateGeom& AggGeom = PhysicsData->AggGeom;
	TSharedPtr<FKAggregateGeom> PreAddGeometry = MakeShared<FKAggregateGeom>(PhysicsData->AggGeom);

	AggGeom.EmptyElements();

	ParentTool->GetToolManager()->BeginUndoTransaction(LOCTEXT("DeleteAllUndo", "Delete All Collision Primitives"));

	TSet<int32> PreDeleteSelection = SelectedPrimitiveIDs;
	SelectedPrimitiveIDs.Empty();

	checkSlow(CurrentActiveProxy);
	const FTransform Transform = CurrentActiveProxy->GetTransform();
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		PreDeleteSelection, false, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveDeselectionTransactionText);
	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicSelectionChange>(
		SelectedPrimitiveIDs, true, Transform, Transform, CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveSelectionTransactionText);
	OnSelectionChanged.Broadcast();

	ParentTool->GetToolManager()->EmitObjectChange(this, MakeUnique<FTLLRN_CollisionPrimitivesMechanicGeometryChange>(
		PreAddGeometry, MakeShared<FKAggregateGeom>(PhysicsData->AggGeom),
		CurrentChangeStamp), TLLRN_CollisionPrimitivesMechanicLocals::CollisionPrimitiveMovementTransactionText);
	OnCollisionGeometryChanged.Broadcast();

	ParentTool->GetToolManager()->EndUndoTransaction();


	RebuildDrawables(true);
	UpdateGizmoLocation();
	UpdateGizmoVisibility();
}

void UTLLRN_CollisionPrimitivesMechanic::UpdateCollisionGeometry(const FKAggregateGeom& NewGeometryIn)
{
	PhysicsData->AggGeom = NewGeometryIn;

	RebuildDrawables(true);
	UpdateGizmoLocation();
	UpdateGizmoVisibility();
}

// ==================== Undo/redo object functions ====================

UTLLRN_CollisionPrimitivesMechanic::FTLLRN_CollisionPrimitivesMechanicSelectionChange::FTLLRN_CollisionPrimitivesMechanicSelectionChange(int32 PrimitiveIDIn,
																						   bool bAddedIn,
																						   const FTransform& PreviousTransformIn,
																						   const FTransform& NewTransformIn,
																						   int32 ChangeStampIn)
	: PrimitiveIDs{ PrimitiveIDIn }
	, bAdded(bAddedIn)
	, PreviousTransform(PreviousTransformIn)
	, NewTransform(NewTransformIn)
	, ChangeStamp(ChangeStampIn)
{}

UTLLRN_CollisionPrimitivesMechanic::FTLLRN_CollisionPrimitivesMechanicSelectionChange::FTLLRN_CollisionPrimitivesMechanicSelectionChange(const TSet<int32>& PrimitiveIDsIn,
																						   bool bAddedIn, 
																						   const FTransform& PreviousTransformIn,
																						   const FTransform& NewTransformIn, 
																						   int32 ChangeStampIn)
	: PrimitiveIDs(PrimitiveIDsIn)
	, bAdded(bAddedIn)
	, PreviousTransform(PreviousTransformIn)
	, NewTransform(NewTransformIn)
	, ChangeStamp(ChangeStampIn)
{}

void UTLLRN_CollisionPrimitivesMechanic::FTLLRN_CollisionPrimitivesMechanicSelectionChange::Apply(UObject* Object)
{
	UTLLRN_CollisionPrimitivesMechanic* Mechanic = Cast<UTLLRN_CollisionPrimitivesMechanic>(Object);
	
	for (int32 PrimitiveID : PrimitiveIDs)
	{
		if (bAdded)
		{
			Mechanic->SelectPrimitive(PrimitiveID);
		}
		else
		{
			Mechanic->DeselectPrimitive(PrimitiveID);
		}
	}

	Mechanic->UpdateDrawables();
	Mechanic->OnSelectionChanged.Broadcast();
}

void UTLLRN_CollisionPrimitivesMechanic::FTLLRN_CollisionPrimitivesMechanicSelectionChange::Revert(UObject* Object)
{
	UTLLRN_CollisionPrimitivesMechanic* Mechanic = Cast<UTLLRN_CollisionPrimitivesMechanic>(Object);
	for (int32 PrimitiveID : PrimitiveIDs)
	{
		if (bAdded)
		{
			Mechanic->DeselectPrimitive(PrimitiveID);
		}
		else
		{
			Mechanic->SelectPrimitive(PrimitiveID);
		}
	}
	
	Mechanic->UpdateDrawables();
	Mechanic->OnSelectionChanged.Broadcast();
}

FString UTLLRN_CollisionPrimitivesMechanic::FTLLRN_CollisionPrimitivesMechanicSelectionChange::ToString() const
{
	return TEXT("FTLLRN_CollisionPrimitivesMechanicSelectionChange");
}


UTLLRN_CollisionPrimitivesMechanic::FTLLRN_CollisionPrimitivesMechanicGeometryChange::FTLLRN_CollisionPrimitivesMechanicGeometryChange(
	TSharedPtr<FKAggregateGeom> GeometryPreviousIn,
	TSharedPtr<FKAggregateGeom> GeometryNewIn,
	int32 ChangeStampIn) :
	GeometryPrevious{ GeometryPreviousIn }
	, GeometryNew{ GeometryNewIn }
	, ChangeStamp(ChangeStampIn)
{
}

void UTLLRN_CollisionPrimitivesMechanic::FTLLRN_CollisionPrimitivesMechanicGeometryChange::Apply(UObject* Object)
{
	UTLLRN_CollisionPrimitivesMechanic* Mechanic = Cast<UTLLRN_CollisionPrimitivesMechanic>(Object);
	Mechanic->UpdateCollisionGeometry(*GeometryNew);	
	Mechanic->OnCollisionGeometryChanged.Broadcast();
}

void UTLLRN_CollisionPrimitivesMechanic::FTLLRN_CollisionPrimitivesMechanicGeometryChange::Revert(UObject* Object)
{
	UTLLRN_CollisionPrimitivesMechanic* Mechanic = Cast<UTLLRN_CollisionPrimitivesMechanic>(Object);
	Mechanic->UpdateCollisionGeometry(*GeometryPrevious);
	Mechanic->OnCollisionGeometryChanged.Broadcast();
}

FString UTLLRN_CollisionPrimitivesMechanic::FTLLRN_CollisionPrimitivesMechanicGeometryChange::ToString() const
{
	return TEXT("FTLLRN_CollisionPrimitivesMechanicGeometryChange");
}

#undef LOCTEXT_NAMESPACE

