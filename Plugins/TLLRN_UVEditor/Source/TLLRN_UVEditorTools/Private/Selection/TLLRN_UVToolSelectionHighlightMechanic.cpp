// Copyright Epic Games, Inc. All Rights Reserved.

#include "Selection/TLLRN_UVToolSelectionHighlightMechanic.h"

#include "Actions/TLLRN_UVSeamSewAction.h"
#include "Drawing/LineSetComponent.h"
#include "Drawing/TLLRN_BasicPointSetComponent.h"
#include "Drawing/TLLRN_BasicLineSetComponent.h"
#include "Drawing/TLLRN_BasicTriangleSetComponent.h"
#include "Drawing/PreviewGeometryActor.h"
#include "Drawing/TriangleSetComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Engine/World.h"
#include "MeshOpPreviewHelpers.h" // for the preview meshes
#include "Selection/UVToolSelection.h"
#include "ToolSetupUtil.h"
#include "ToolTargets/TLLRN_UVEditorToolMeshInput.h"
#include "TLLRN_UVEditorUXSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVToolSelectionHighlightMechanic)

#define LOCTEXT_NAMESPACE "UTLLRN_UVToolSelectionHighlightMechanic"

using namespace UE::Geometry;

namespace TLLRN_UVToolSelectionHighlightMechanicLocals
{
	FVector2f ToFVector2f(const FVector& VectorIn)
    {
         return FVector2f( static_cast<float>(VectorIn.X), static_cast<float>(VectorIn.Y) );
    }

	FVector3f ToFVector3f(const FVector3d& VectorIn)
    {
         return FVector3f( static_cast<float>(VectorIn.X), static_cast<float>(VectorIn.Y), static_cast<float>(VectorIn.Z));
    }
}

void UTLLRN_UVToolSelectionHighlightMechanic::Initialize(UWorld* UnwrapWorld, UWorld* LivePreviewWorld)
{
	// Initialize shouldn't be called more than once...
	if (!ensure(!UnwrapGeometryActor))
	{
		UnwrapGeometryActor->Destroy();
	}
	if (!ensure(!LivePreviewGeometryActor))
	{
		LivePreviewGeometryActor->Destroy();
	}

	// Owns most of the unwrap geometry except for the unselected paired edges, since we don't
	// want those to move if we change the actor transform via SetUnwrapHighlightTransform
	UnwrapGeometryActor = UnwrapWorld->SpawnActor<APreviewGeometryActor>(
		FVector::ZeroVector, FRotator(0, 0, 0), FActorSpawnParameters());

	UnwrapTriangleSet = NewObject<UTLLRN_Basic2DTriangleSetComponent>(UnwrapGeometryActor);
	// We are setting the TranslucencySortPriority here to handle the UV editor's use case in 2D
	// where multiple translucent layers are drawn on top of each other but still need depth sorting.
	UnwrapTriangleSet->TranslucencySortPriority = static_cast<int32>(FTLLRN_UVEditorUXSettings::SelectionTriangleDepthBias);
	TriangleSetMaterial = ToolSetupUtil::GetCustomTwoSidedDepthOffsetMaterial(GetParentTool()->GetToolManager(),
		FTLLRN_UVEditorUXSettings::SelectionTriangleFillColor,
		FTLLRN_UVEditorUXSettings::SelectionTriangleDepthBias,
		FTLLRN_UVEditorUXSettings::SelectionTriangleOpacity);
	UnwrapTriangleSet->SetTriangleMaterial(TriangleSetMaterial);
	UnwrapTriangleSet->SetTriangleSetParameters(FTLLRN_UVEditorUXSettings::SelectionTriangleFillColor, FVector3f(0, 0, 1));
	UnwrapGeometryActor->SetRootComponent(UnwrapTriangleSet.Get());
	UnwrapTriangleSet->RegisterComponent();

	UnwrapLineSet = NewObject<UTLLRN_Basic2DLineSetComponent>(UnwrapGeometryActor);
	UnwrapLineSet->SetLineMaterial(ToolSetupUtil::GetDefaultLineComponentMaterial(GetParentTool()->GetToolManager(), true));
	UnwrapLineSet->SetLineSetParameters(FTLLRN_UVEditorUXSettings::SelectionTriangleWireframeColor,
							     			 FTLLRN_UVEditorUXSettings::SelectionLineThickness,
										     FTLLRN_UVEditorUXSettings::SelectionWireframeDepthBias);
	UnwrapLineSet->AttachToComponent(UnwrapTriangleSet.Get(), FAttachmentTransformRules::KeepWorldTransform);
	UnwrapLineSet->RegisterComponent();

	UnwrapPairedEdgeLineSet = NewObject<UTLLRN_Basic2DLineSetComponent>(UnwrapGeometryActor);
	UnwrapPairedEdgeLineSet->SetLineMaterial(ToolSetupUtil::GetDefaultLineComponentMaterial(GetParentTool()->GetToolManager(), true));
	UnwrapPairedEdgeLineSet->AttachToComponent(UnwrapTriangleSet.Get(), FAttachmentTransformRules::KeepWorldTransform);
	UnwrapPairedEdgeLineSet->RegisterComponent();

	SewEdgePairingLeftLineSet = NewObject<UTLLRN_Basic2DLineSetComponent>(UnwrapGeometryActor);
	SewEdgePairingLeftLineSet->SetLineMaterial(ToolSetupUtil::GetDefaultLineComponentMaterial(
		GetParentTool()->GetToolManager(), /*bDepthTested*/ true));
	SewEdgePairingLeftLineSet->SetLineSetParameters(FTLLRN_UVEditorUXSettings::SewSideLeftColor,
		                                                 FTLLRN_UVEditorUXSettings::SewLineHighlightThickness,
		                                                 FTLLRN_UVEditorUXSettings::SewLineDepthOffset);
	SewEdgePairingLeftLineSet->AttachToComponent(UnwrapTriangleSet.Get(), FAttachmentTransformRules::KeepWorldTransform);
	SewEdgePairingLeftLineSet->RegisterComponent();
	SewEdgePairingLeftLineSet->SetVisibility(bPairedEdgeHighlightsEnabled);

	SewEdgePairingRightLineSet = NewObject<UTLLRN_Basic2DLineSetComponent>(UnwrapGeometryActor);
	SewEdgePairingRightLineSet->SetLineMaterial(ToolSetupUtil::GetDefaultLineComponentMaterial(
		GetParentTool()->GetToolManager(), /*bDepthTested*/ true));
	SewEdgePairingRightLineSet->SetLineSetParameters(FTLLRN_UVEditorUXSettings::SewSideRightColor,
		                                                  FTLLRN_UVEditorUXSettings::SewLineHighlightThickness,
		                                                  FTLLRN_UVEditorUXSettings::SewLineDepthOffset);
	SewEdgePairingRightLineSet->AttachToComponent(UnwrapTriangleSet.Get(), FAttachmentTransformRules::KeepWorldTransform);
	SewEdgePairingRightLineSet->RegisterComponent();
	SewEdgePairingRightLineSet->SetVisibility(bPairedEdgeHighlightsEnabled);

	// The unselected paired edges get their own, stationary, actor.
	UnwrapStationaryGeometryActor = UnwrapWorld->SpawnActor<APreviewGeometryActor>(
		FVector::ZeroVector, FRotator(0, 0, 0), FActorSpawnParameters());
	SewEdgeUnselectedPairingLineSet = NewObject<UTLLRN_Basic2DLineSetComponent>(UnwrapStationaryGeometryActor);
	SewEdgeUnselectedPairingLineSet->SetLineMaterial(ToolSetupUtil::GetDefaultLineComponentMaterial(
		GetParentTool()->GetToolManager(), /*bDepthTested*/ true));
	SewEdgeUnselectedPairingLineSet->SetLineSetParameters(FTLLRN_UVEditorUXSettings::SewSideRightColor,
		                                                       FTLLRN_UVEditorUXSettings::SewLineHighlightThickness,
		                                                       FTLLRN_UVEditorUXSettings::SewLineDepthOffset);
	UnwrapStationaryGeometryActor->SetRootComponent(SewEdgeUnselectedPairingLineSet.Get());
	SewEdgeUnselectedPairingLineSet->RegisterComponent();
	SewEdgeUnselectedPairingLineSet->SetVisibility(bPairedEdgeHighlightsEnabled);

	UnwrapPointSet = NewObject<UTLLRN_Basic2DPointSetComponent>(UnwrapGeometryActor);
	UnwrapPointSet->SetPointMaterial(ToolSetupUtil::GetDefaultPointComponentMaterial(GetParentTool()->GetToolManager(), true));
	UnwrapPointSet->SetPointSetParameters(FTLLRN_UVEditorUXSettings::SelectionTriangleWireframeColor,
											   FTLLRN_UVEditorUXSettings::SelectionPointThickness,
											   FTLLRN_UVEditorUXSettings::SelectionWireframeDepthBias);
	UnwrapPointSet->AttachToComponent(UnwrapTriangleSet.Get(), FAttachmentTransformRules::KeepWorldTransform);
	UnwrapPointSet->RegisterComponent();

	// Owns the highlights in the live preview.
	LivePreviewGeometryActor = LivePreviewWorld->SpawnActor<APreviewGeometryActor>(
		FVector::ZeroVector, FRotator(0, 0, 0), FActorSpawnParameters());

	LivePreviewLineSet = NewObject<UTLLRN_Basic3DLineSetComponent>(LivePreviewGeometryActor);
	LivePreviewLineSet->SetLineMaterial(ToolSetupUtil::GetDefaultLineComponentMaterial(
		GetParentTool()->GetToolManager(), /*bDepthTested*/ true));
	LivePreviewLineSet->SetLineSetParameters(FTLLRN_UVEditorUXSettings::SelectionTriangleWireframeColor,
	                                               FTLLRN_UVEditorUXSettings::LivePreviewHighlightThickness,
		                                           FTLLRN_UVEditorUXSettings::LivePreviewHighlightDepthOffset);
	LivePreviewGeometryActor->SetRootComponent(LivePreviewLineSet.Get());
	LivePreviewLineSet->RegisterComponent();

	LivePreviewPointSet = NewObject<UTLLRN_Basic3DPointSetComponent>(LivePreviewGeometryActor);
	LivePreviewPointSet->SetPointMaterial(ToolSetupUtil::GetDefaultPointComponentMaterial(
		GetParentTool()->GetToolManager(), /*bDepthTested*/ true));
	LivePreviewPointSet->SetPointSetParameters(FTLLRN_UVEditorUXSettings::SelectionTriangleWireframeColor,
													FTLLRN_UVEditorUXSettings::LivePreviewHighlightPointSize,
													FTLLRN_UVEditorUXSettings::LivePreviewHighlightDepthOffset);
	LivePreviewPointSet->AttachToComponent(LivePreviewLineSet.Get(), FAttachmentTransformRules::KeepWorldTransform);
	LivePreviewPointSet->RegisterComponent();
}

void UTLLRN_UVToolSelectionHighlightMechanic::Shutdown()
{
	if (UnwrapGeometryActor)
	{
		UnwrapGeometryActor->Destroy();
		UnwrapGeometryActor = nullptr;
	}
	if (UnwrapStationaryGeometryActor)
	{
		UnwrapStationaryGeometryActor->Destroy();
		UnwrapStationaryGeometryActor = nullptr;
	}
	if (LivePreviewGeometryActor)
	{
		LivePreviewGeometryActor->Destroy();
		LivePreviewGeometryActor = nullptr;
	}

	TriangleSetMaterial = nullptr;
}

void UTLLRN_UVToolSelectionHighlightMechanic::SetIsVisible(bool bUnwrapHighlightVisible, bool bLivePreviewHighlightVisible)
{
	if (UnwrapGeometryActor)
	{
		UnwrapGeometryActor->SetActorHiddenInGame(!bUnwrapHighlightVisible);
	}
	if (UnwrapStationaryGeometryActor)
	{
		UnwrapStationaryGeometryActor->SetActorHiddenInGame(!bUnwrapHighlightVisible);
	}
	if (LivePreviewGeometryActor)
	{
		LivePreviewGeometryActor->SetActorHiddenInGame(!bLivePreviewHighlightVisible);
	}
}

void UTLLRN_UVToolSelectionHighlightMechanic::RebuildUnwrapHighlight(
	const TArray<FUVToolSelection>& Selections, const FTransform& StartTransform, 
	bool bUsePreviews)
{
    using namespace TLLRN_UVToolSelectionHighlightMechanicLocals;

	if (!ensure(UnwrapGeometryActor))
	{
		return;
	}

	UnwrapTriangleSet->Clear();
	UnwrapLineSet->Clear();
	UnwrapPointSet->Clear();
	SewEdgePairingRightLineSet->Clear();
	SewEdgePairingLeftLineSet->Clear();
	SewEdgeUnselectedPairingLineSet->Clear();
	StaticPairedEdgeVidsPerMesh.Reset();

	UnwrapGeometryActor->SetActorTransform(StartTransform);

	for (const FUVToolSelection& Selection : Selections)
	{
		if (!ensure(Selection.Target.IsValid() && Selection.Target->AreMeshesValid()))
		{
			return;
		}

		const FDynamicMesh3& Mesh = bUsePreviews ? *Selection.Target->UnwrapPreview->PreviewMesh->GetMesh()
			: *Selection.Target->UnwrapCanonical;

		if (Selection.Type == FUVToolSelection::EType::Triangle)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UTLLRN_UVToolSelectionHighlightMechanic::AppendUnwrapHighlight_Triangle);

			UTLLRN_Basic2DTriangleSetComponent* UnwrapTriangleSetPtr = UnwrapTriangleSet.Get();
			UTLLRN_Basic2DLineSetComponent* UnwrapLineSetPtr = UnwrapLineSet.Get();
			UnwrapTriangleSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());
			UnwrapLineSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num() * 3);
			for (int32 Tid : Selection.SelectedIDs)
			{
				if (!ensure(Mesh.IsTriangle(Tid)))
				{
					continue;
				}
				
				FVector Points[3];
				Mesh.GetTriVertices(Tid, Points[0], Points[1], Points[2]);
				for (int i = 0; i < 3; ++i)
				{
					Points[i] = StartTransform.InverseTransformPosition(Points[i]);
				}
				UnwrapTriangleSetPtr->AddElement(ToFVector2f(Points[0]),ToFVector2f(Points[1]),ToFVector2f(Points[2]));
					
				for (int i = 0; i < 3; ++i)
				{
					int NextIndex = (i + 1) % 3;
					UnwrapLineSetPtr->AddElement(ToFVector2f(Points[i]),ToFVector2f(Points[NextIndex]));
				}
			}
			UnwrapTriangleSetPtr->MarkRenderStateDirty();
			UnwrapLineSetPtr->MarkRenderStateDirty();
		}
		else if (Selection.Type == FUVToolSelection::EType::Edge)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(MeshSelectionMechanic_RebuildDrawnElements_Edge);

			StaticPairedEdgeVidsPerMesh.Emplace();
			StaticPairedEdgeVidsPerMesh.Last().Key = Selection.Target;

			const FDynamicMesh3& AppliedMesh = bUsePreviews ? *Selection.Target->AppliedPreview->PreviewMesh->GetMesh()
				: *Selection.Target->AppliedCanonical;

			UTLLRN_Basic2DLineSetComponent* UnwrapLineSetPtr = UnwrapLineSet.Get();
			UTLLRN_Basic2DLineSetComponent* SewEdgePairingLeftLineSetPtr = SewEdgePairingLeftLineSet.Get();
			UTLLRN_Basic2DLineSetComponent* SewEdgePairingRightLineSetPtr = SewEdgePairingRightLineSet.Get();
			UTLLRN_Basic2DLineSetComponent* SewEdgeUnselectedPairingLineSetPtr = SewEdgeUnselectedPairingLineSet.Get();

			UnwrapLineSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());
			SewEdgePairingLeftLineSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());
			SewEdgePairingRightLineSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());
			SewEdgeUnselectedPairingLineSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());
			for (int32 Eid : Selection.SelectedIDs)
			{
				if (!ensure(Mesh.IsEdge(Eid)))
				{
					continue;
				}
				
				FVector Points[2];
				Mesh.GetEdgeV(Eid, Points[0], Points[1]);
				Points[0] = StartTransform.InverseTransformPosition(Points[0]);
				Points[1] = StartTransform.InverseTransformPosition(Points[1]);
				UnwrapLineSetPtr->AddElement(ToFVector2f(Points[0]),ToFVector2f(Points[1]));

				if (bPairedEdgeHighlightsEnabled)
				{
					bool bWouldPreferReverse = false;
					int32 PairedEid = UTLLRN_UVSeamSewAction::FindSewEdgeOppositePairing(
						Mesh, AppliedMesh, Selection.Target->UVLayerIndex, Eid, bWouldPreferReverse);

					bool bPairedEdgeIsSelected = Selection.SelectedIDs.Contains(PairedEid);

					if (PairedEid == IndexConstants::InvalidID
						// When both sides are selected, merge order depends on adjacent tid values, so
						// deal with the pair starting with the other edge.
						|| (bPairedEdgeIsSelected && bWouldPreferReverse))
					{
						continue;
					}

					Mesh.GetEdgeV(Eid, Points[0], Points[1]);
					Points[0] = StartTransform.InverseTransformPosition(Points[0]);
					Points[1] = StartTransform.InverseTransformPosition(Points[1]);
					SewEdgePairingLeftLineSetPtr->AddElement(ToFVector2f(Points[0]),ToFVector2f(Points[1]));

					// The paired edge may need to go into a separate line set if it is not selected so that it does
					// not get affected by transformations of the selected highlights in SetUnwrapHighlightTransform
					FIndex2i Vids2 = Mesh.GetEdgeV(PairedEid);
					Mesh.GetEdgeV(PairedEid, Points[0], Points[1]);
					if (bPairedEdgeIsSelected)
					{
						Points[0] = StartTransform.InverseTransformPosition(Points[0]);
						Points[1] = StartTransform.InverseTransformPosition(Points[1]);
						SewEdgePairingRightLineSetPtr->AddElement(ToFVector2f(Points[0]),ToFVector2f(Points[1]));
					}
					else
					{
						StaticPairedEdgeVidsPerMesh.Last().Value.Add(TPair<int32, int32>(Vids2.A, Vids2.B));
						SewEdgeUnselectedPairingLineSetPtr->AddElement(ToFVector2f(Points[0]),
 							                                           ToFVector2f(Points[1]));
					}
				}//end if visualizing paired edges
			}//end for each edge
			UnwrapLineSetPtr->MarkRenderStateDirty();
			SewEdgePairingLeftLineSetPtr->MarkRenderStateDirty();
			SewEdgePairingRightLineSetPtr->MarkRenderStateDirty();
			SewEdgeUnselectedPairingLineSetPtr->MarkRenderStateDirty();
		}
		else if (Selection.Type == FUVToolSelection::EType::Vertex)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(MeshSelectionMechanic_RebuildDrawnElements_Vertex);

			UTLLRN_Basic2DPointSetComponent* UnwrapPointSetPtr = UnwrapPointSet.Get();
			UnwrapPointSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());

			for (const int32 Vid : Selection.SelectedIDs)
			{
				if (!ensure(Mesh.IsVertex(Vid)))
				{
					continue;
				}

				const FVector3d Position = StartTransform.InverseTransformPosition(Mesh.GetVertex(Vid));
				UnwrapPointSetPtr->AddElement(ToFVector2f(Position));
			}
			UnwrapPointSetPtr->MarkRenderStateDirty();
		}
	}
}

void UTLLRN_UVToolSelectionHighlightMechanic::SetUnwrapHighlightTransform(const FTransform& Transform, 
	bool bRebuildStaticPairedEdges, bool bUsePreviews)
{
    using namespace TLLRN_UVToolSelectionHighlightMechanicLocals;

	if (ensure(UnwrapGeometryActor))
	{
		UnwrapGeometryActor->SetActorTransform(Transform);
	}
	if (bPairedEdgeHighlightsEnabled && bRebuildStaticPairedEdges)
	{		
		int32 PairedEdgeCount = 0;
		for (const TPair<TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput>,
			TArray<TPair<int32, int32>>>& MeshVidPairs : StaticPairedEdgeVidsPerMesh)
		{
			PairedEdgeCount += MeshVidPairs.Value.Num();
		}

		UTLLRN_Basic2DLineSetComponent* SewEdgeUnselectedPairingLineSetPtr = SewEdgeUnselectedPairingLineSet.Get();
		SewEdgeUnselectedPairingLineSetPtr->Clear();
		SewEdgeUnselectedPairingLineSetPtr->ReserveElements(PairedEdgeCount);

		for (const TPair<TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput>, 
			TArray<TPair<int32, int32>>>& MeshVidPairs : StaticPairedEdgeVidsPerMesh)
		{
			TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput> Target = MeshVidPairs.Key;
			if (!ensure(Target.IsValid()))
			{
				continue;
			}

			const FDynamicMesh3& Mesh = bUsePreviews ? *Target->UnwrapPreview->PreviewMesh->GetMesh()
				: *Target->UnwrapCanonical;

			for (const TPair<int32, int32>& VidPair : MeshVidPairs.Value)
			{
				if (!ensure(Mesh.IsVertex(VidPair.Key) && Mesh.IsVertex(VidPair.Value)))
				{
					continue;
				}
				FVector3d VertA = Mesh.GetVertex(VidPair.Key);
				FVector3d VertB = Mesh.GetVertex(VidPair.Value);
				SewEdgeUnselectedPairingLineSetPtr->AddElement(ToFVector2f(VertA),ToFVector2f(VertB));
			}
			SewEdgeUnselectedPairingLineSetPtr->MarkRenderStateDirty();
		}
	}
}

FTransform UTLLRN_UVToolSelectionHighlightMechanic::GetUnwrapHighlightTransform()
{
	if (ensure(UnwrapGeometryActor))
	{
		return UnwrapGeometryActor->GetActorTransform();
	}
	return FTransform::Identity;
}

void UTLLRN_UVToolSelectionHighlightMechanic::RebuildAppliedHighlightFromUnwrapSelection(
	const TArray<FUVToolSelection>& UnwrapSelections, bool bUsePreviews)
{
    using namespace TLLRN_UVToolSelectionHighlightMechanicLocals;
	if (!ensure(LivePreviewGeometryActor))
	{
		return;
	}

	UTLLRN_Basic3DLineSetComponent* LivePreviewLineSetPtr = LivePreviewLineSet.Get();
	UTLLRN_Basic3DPointSetComponent* LivePreviewPointSetPtr = LivePreviewPointSet.Get();

	LivePreviewLineSetPtr->Clear();
	LivePreviewPointSetPtr->Clear();

	for (const FUVToolSelection& Selection : UnwrapSelections)
	{
		if (!ensure(Selection.Target.IsValid() && Selection.Target->AreMeshesValid()))
		{
			return;
		}

		UTLLRN_UVEditorToolMeshInput* Target = Selection.Target.Get();

		const FDynamicMesh3& AppliedMesh = bUsePreviews ? *Target->AppliedPreview->PreviewMesh->GetMesh()
			: *Target->AppliedCanonical;
		const FDynamicMesh3& UnwrapMesh = bUsePreviews ? *Target->UnwrapPreview->PreviewMesh->GetMesh()
			: *Target->UnwrapCanonical;

		FTransform MeshTransform = Target->AppliedPreview->PreviewMesh->GetTransform();

		auto AppendEdgeLine = [this, &AppliedMesh, &MeshTransform, LivePreviewLineSetPtr](int32 AppliedEid)
		{
			FVector Points[2];
			AppliedMesh.GetEdgeV(AppliedEid, Points[0], Points[1]);
			Points[0] = MeshTransform.TransformPosition(Points[0]);
			Points[1] = MeshTransform.TransformPosition(Points[1]);
			LivePreviewLineSetPtr->AddElement(ToFVector3f(Points[0]),ToFVector3f(Points[1]));
		};

		if (Selection.Type == FUVToolSelection::EType::Triangle)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Triangle);

			LivePreviewLineSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num()*3);
			for (int32 Tid : Selection.SelectedIDs)
			{
				if (!ensure(AppliedMesh.IsTriangle(Tid)))
				{
					continue;
				}

				// Gather the boundary edges for the live preview
				FIndex3i TriEids = AppliedMesh.GetTriEdges(Tid);
				for (int i = 0; i < 3; ++i)
				{
					FIndex2i EdgeTids = AppliedMesh.GetEdgeT(TriEids[i]);
					for (int j = 0; j < 2; ++j)
					{
						if (EdgeTids[j] != Tid && !Selection.SelectedIDs.Contains(EdgeTids[j]))
						{
							AppendEdgeLine(TriEids[i]);
							break;
						}
					}
				}//end for tri edges
			}//end for selection tids
			LivePreviewLineSetPtr->MarkRenderStateDirty();
		}
		else if (Selection.Type == FUVToolSelection::EType::Edge)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Edge);
			LivePreviewLineSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());

			for (int32 UnwrapEid : Selection.SelectedIDs)
			{
				if (!ensure(UnwrapMesh.IsEdge(UnwrapEid)))
				{
					continue;
				}

				FDynamicMesh3::FEdge Edge = UnwrapMesh.GetEdge(UnwrapEid);

				int32 AppliedEid = AppliedMesh.FindEdgeFromTri(
					Target->UnwrapVidToAppliedVid(Edge.Vert.A),
					Target->UnwrapVidToAppliedVid(Edge.Vert.B),
					Edge.Tri.A);

				AppendEdgeLine(AppliedEid);
			}
			LivePreviewLineSetPtr->MarkRenderStateDirty();
		}
		else if (Selection.Type == FUVToolSelection::EType::Vertex)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Vertex);

			LivePreviewPointSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());

			for (const int32 UnwrapVid : Selection.SelectedIDs)
			{
				LivePreviewPointSetPtr->AddElement(static_cast<FVector3f>(AppliedMesh.GetVertex(Target->UnwrapVidToAppliedVid(UnwrapVid))));
			}
			LivePreviewPointSetPtr->MarkRenderStateDirty();
		}
	}//end for selection
}

void UTLLRN_UVToolSelectionHighlightMechanic::AppendAppliedHighlight(const TArray<FUVToolSelection>& AppliedSelections, bool bUsePreviews)
{
    using namespace TLLRN_UVToolSelectionHighlightMechanicLocals;
	if (!ensure(LivePreviewGeometryActor))
	{
		return;
	}

	UTLLRN_Basic3DLineSetComponent* LivePreviewLineSetPtr = LivePreviewLineSet.Get();
	UTLLRN_Basic3DPointSetComponent* LivePreviewPointSetPtr = LivePreviewPointSet.Get();

	for (const FUVToolSelection& Selection : AppliedSelections)
	{
		if (!ensure(Selection.Target.IsValid() && Selection.Target->IsValid()))
		{
			return;
		}

		UTLLRN_UVEditorToolMeshInput* Target = Selection.Target.Get();

		const FDynamicMesh3& AppliedMesh = bUsePreviews ? *Target->AppliedPreview->PreviewMesh->GetMesh()
			: *Target->AppliedCanonical;

		FTransform MeshTransform = Target->AppliedPreview->PreviewMesh->GetTransform();

		auto AppendEdgeLine = [this, &AppliedMesh, &MeshTransform, LivePreviewLineSetPtr](int32 AppliedEid)
		{
			FVector Points[2];
			AppliedMesh.GetEdgeV(AppliedEid, Points[0], Points[1]);
			Points[0] = MeshTransform.TransformPosition(Points[0]);
			Points[1] = MeshTransform.TransformPosition(Points[1]);
			LivePreviewLineSetPtr->AddElement(ToFVector3f(Points[0]),ToFVector3f(Points[1]));
		};

		if (Selection.Type == FUVToolSelection::EType::Triangle)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Triangle);

			LivePreviewLineSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num() * 3);
			for (int32 Tid : Selection.SelectedIDs)
			{
				if (!ensure(AppliedMesh.IsTriangle(Tid)))
				{
					continue;
				}

				// Gather the boundary edges for the live preview
				FIndex3i TriEids = AppliedMesh.GetTriEdges(Tid);
				for (int i = 0; i < 3; ++i)
				{
					FIndex2i EdgeTids = AppliedMesh.GetEdgeT(TriEids[i]);
					for (int j = 0; j < 2; ++j)
					{
						if (EdgeTids[j] != Tid && !Selection.SelectedIDs.Contains(EdgeTids[j]))
						{
							AppendEdgeLine(TriEids[i]);
							break;
						}
					}
				}//end for tri edges
			}//end for selection tids
			LivePreviewLineSetPtr->MarkRenderStateDirty();
		}
		else if (Selection.Type == FUVToolSelection::EType::Edge)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Edge);

			LivePreviewLineSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());
			for (int32 Eid : Selection.SelectedIDs)
			{
				if (!ensure(AppliedMesh.IsEdge(Eid)))
				{
					continue;
				}

				AppendEdgeLine(Eid);
			}
			LivePreviewLineSetPtr->MarkRenderStateDirty();
		}
		else if (Selection.Type == FUVToolSelection::EType::Vertex)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Vertex);

			LivePreviewPointSetPtr->ReserveAdditionalElements(Selection.SelectedIDs.Num());

			for (const int32 Vid : Selection.SelectedIDs)
			{
				LivePreviewPointSetPtr->AddElement(static_cast<FVector3f>(AppliedMesh.GetVertex(Vid)));
			}
			LivePreviewPointSetPtr->MarkRenderStateDirty();
		}
	}//end for selection
}


void UTLLRN_UVToolSelectionHighlightMechanic::SetEnablePairedEdgeHighlights(bool bEnable)
{
	bPairedEdgeHighlightsEnabled = bEnable;
	SewEdgePairingLeftLineSet->SetVisibility(bEnable);
	SewEdgePairingRightLineSet->SetVisibility(bEnable);
	SewEdgeUnselectedPairingLineSet->SetVisibility(bEnable);
}

void UTLLRN_UVToolSelectionHighlightMechanic::SetColor(const FColor Color) const
{
	LivePreviewLineSet->SetColor(Color);
	LivePreviewPointSet->SetColor(Color);
}

void UTLLRN_UVToolSelectionHighlightMechanic::SetLineThickness(const float LineSize) const
{
	LivePreviewLineSet->SetLineThickness(LineSize);
}

void UTLLRN_UVToolSelectionHighlightMechanic::SetPointSize(const float PointSize) const
{
	LivePreviewPointSet->SetPointSize(PointSize);
}

#undef LOCTEXT_NAMESPACE
