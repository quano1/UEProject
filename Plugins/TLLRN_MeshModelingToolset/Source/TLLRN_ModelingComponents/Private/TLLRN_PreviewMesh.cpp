// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_PreviewMesh.h"

#include "TargetInterfaces/MaterialProvider.h" //FComponentMaterialSet

#include "DynamicMeshToMeshDescription.h"
#include "Engine/World.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "SceneInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_PreviewMesh)

using namespace UE::Geometry;

ATLLRN_PreviewMeshActor::ATLLRN_PreviewMeshActor()
{
#if WITH_EDITORONLY_DATA
	// hide this actor in the scene outliner
	bListedInSceneOutliner = false;
#endif
}


UTLLRN_PreviewMesh::UTLLRN_PreviewMesh()
{
	bBuildSpatialDataStructure = false;
}

UTLLRN_PreviewMesh::~UTLLRN_PreviewMesh()
{
	checkf(DynamicMeshComponent == nullptr, TEXT("You must explicitly Disconnect() TLLRN_PreviewMesh before it is GCd"));
	checkf(TemporaryParentActor == nullptr, TEXT("You must explicitly Disconnect() TLLRN_PreviewMesh before it is GCd"));
}


void UTLLRN_PreviewMesh::CreateInWorld(UWorld* World, const FTransform& WithTransform)
{
	FRotator Rotation(0.0f, 0.0f, 0.0f);
	FActorSpawnParameters SpawnInfo;
	TemporaryParentActor = World->SpawnActor<ATLLRN_PreviewMeshActor>(FVector::ZeroVector, Rotation, SpawnInfo);

	DynamicMeshComponent = NewObject<UDynamicMeshComponent>(TemporaryParentActor);

	// Disable VerifyUsedMaterials on the DynamicMeshSceneProxy. Material verification is prone
	// to data races when materials are subject to change at a high frequency. Since the
	// preview mesh material usage (override render materials) is particularly prone to these
	// races and we are certain the component materials are updated appropriately, we disable
	// used material verification.
	DynamicMeshComponent->SetSceneProxyVerifyUsedMaterials(false);
	
	TemporaryParentActor->SetRootComponent(DynamicMeshComponent);
	//DynamicMeshComponent->SetupAttachment(TemporaryParentActor->GetRootComponent());
	DynamicMeshComponent->RegisterComponent();

	TemporaryParentActor->SetActorTransform(WithTransform);
	//Builder.NewMeshComponent->SetWorldTransform(PlaneFrame.ToFTransform());
}


void UTLLRN_PreviewMesh::Disconnect()
{
	if (DynamicMeshComponent != nullptr)
	{
		DynamicMeshComponent->UnregisterComponent();
		DynamicMeshComponent->DestroyComponent();
		DynamicMeshComponent = nullptr;
	}

	if (TemporaryParentActor != nullptr)
	{
		TemporaryParentActor->Destroy();
		TemporaryParentActor = nullptr;
	}
}


void UTLLRN_PreviewMesh::SetMaterial(UMaterialInterface* Material)
{
	SetMaterial(0, Material);
}

void UTLLRN_PreviewMesh::SetMaterial(int MaterialIndex, UMaterialInterface* Material)
{
	check(DynamicMeshComponent);
	DynamicMeshComponent->SetMaterial(MaterialIndex, Material);

	// force rebuild because we can't change materials yet - surprisingly complicated
	DynamicMeshComponent->NotifyMeshUpdated();

	// if we change materials we have to force a decomposition update because it decomposes by material
	if (bDecompositionEnabled)
	{
		UpdateRenderMeshDecomposition();
	}
}

void UTLLRN_PreviewMesh::SetMaterials(const TArray<UMaterialInterface*>& Materials)
{
	check(DynamicMeshComponent);
	for (int k = 0; k < Materials.Num(); ++k)
	{
		DynamicMeshComponent->SetMaterial(k, Materials[k]);
	}

	// force rebuild because we can't change materials yet - surprisingly complicated
	DynamicMeshComponent->NotifyMeshUpdated();

	// if we change materials we have to force a decomposition update because it decomposes by material
	if (bDecompositionEnabled)
	{
		UpdateRenderMeshDecomposition();
	}
}

int32 UTLLRN_PreviewMesh::GetNumMaterials() const
{
	check(DynamicMeshComponent);
	return DynamicMeshComponent->GetNumMaterials();
}

UMaterialInterface* UTLLRN_PreviewMesh::GetMaterial(int MaterialIndex) const
{
	check(DynamicMeshComponent);
	return DynamicMeshComponent->GetMaterial(MaterialIndex);
}

void UTLLRN_PreviewMesh::GetMaterials(TArray<UMaterialInterface*>& OutMaterials) const
{
	check(DynamicMeshComponent);
	for (int32 i = 0; i < DynamicMeshComponent->GetNumMaterials(); ++i)
	{
		OutMaterials.Add(DynamicMeshComponent->GetMaterial(i));
	}
}

void UTLLRN_PreviewMesh::SetOverrideRenderMaterial(UMaterialInterface* Material)
{
	check(DynamicMeshComponent);
	return DynamicMeshComponent->SetOverrideRenderMaterial(Material);
}

void UTLLRN_PreviewMesh::ClearOverrideRenderMaterial()
{
	check(DynamicMeshComponent);
	return DynamicMeshComponent->ClearOverrideRenderMaterial();
}


UMaterialInterface* UTLLRN_PreviewMesh::GetActiveMaterial(int MaterialIndex) const
{
	return DynamicMeshComponent->HasOverrideRenderMaterial(MaterialIndex) ?
		DynamicMeshComponent->GetOverrideRenderMaterial(MaterialIndex) : GetMaterial(MaterialIndex);
}



void UTLLRN_PreviewMesh::SetSecondaryRenderMaterial(UMaterialInterface* Material)
{
	check(DynamicMeshComponent);
	return DynamicMeshComponent->SetSecondaryRenderMaterial(Material);
}

void UTLLRN_PreviewMesh::ClearSecondaryRenderMaterial()
{
	check(DynamicMeshComponent);
	return DynamicMeshComponent->ClearSecondaryRenderMaterial();
}



void UTLLRN_PreviewMesh::EnableSecondaryTriangleBuffers(TUniqueFunction<bool(const FDynamicMesh3*, int32)> TriangleFilterFunc)
{
	check(DynamicMeshComponent);
	DynamicMeshComponent->EnableSecondaryTriangleBuffers(MoveTemp(TriangleFilterFunc));
}

void UTLLRN_PreviewMesh::DisableSecondaryTriangleBuffers()
{
	check(DynamicMeshComponent);
	DynamicMeshComponent->DisableSecondaryTriangleBuffers();
}

void UTLLRN_PreviewMesh::SetSecondaryBuffersVisibility(bool bSecondaryVisibility)
{
	check(DynamicMeshComponent);
	DynamicMeshComponent->SetSecondaryBuffersVisibility(bSecondaryVisibility);
}

void UTLLRN_PreviewMesh::FastNotifySecondaryTrianglesChanged()
{
	check(DynamicMeshComponent);
	DynamicMeshComponent->FastNotifySecondaryTrianglesChanged();
}

void UTLLRN_PreviewMesh::SetTangentsMode(EDynamicMeshComponentTangentsMode TangentsType)
{
	check(DynamicMeshComponent);
	DynamicMeshComponent->SetTangentsType(TangentsType);
}

bool UTLLRN_PreviewMesh::CalculateTangents()
{
	check(DynamicMeshComponent);

	UDynamicMesh *const DynamicMesh = DynamicMeshComponent->GetDynamicMesh();
	FDynamicMesh3 *const Mesh = DynamicMesh ? DynamicMesh->GetMeshPtr() : nullptr;

	if (Mesh)
	{
		// Holds temporary tangents in case we don't have access to existing tangents and need to compute them within this function.
		FMeshTangentsf TempTangents;

		const FMeshTangentsf* Tangents = [this, Mesh, &TempTangents]() -> const FMeshTangentsf*
		{
			if (DynamicMeshComponent->GetTangentsType() == EDynamicMeshComponentTangentsMode::AutoCalculated)
			{
				if (const FMeshTangentsf* AutoCalculatedTangents = DynamicMeshComponent->GetAutoCalculatedTangents())
				{
					return AutoCalculatedTangents;
				}
			}

			const FDynamicMeshNormalOverlay* NormalOverlay = nullptr;
			const FDynamicMeshUVOverlay* UVOverlay = nullptr;

			if (const FDynamicMeshAttributeSet* Attributes = Mesh->Attributes())
			{
				if (Attributes->NumNormalLayers() > 0)
				{
					NormalOverlay = Attributes->GetNormalLayer(0);
				}

				if (Attributes->NumUVLayers() > 0)
				{
					UVOverlay = Attributes->GetUVLayer(0);
				}
			}

			if (NormalOverlay && UVOverlay)
			{
				TempTangents.ComputeTriVertexTangents(NormalOverlay, UVOverlay, {});

				return &TempTangents;
			}

			return nullptr;
		}();

		if (Tangents)
		{
			Mesh->Attributes()->EnableTangents();
			if (Tangents->CopyToOverlays(*Mesh))
			{
				DynamicMeshComponent->FastNotifyVertexAttributesUpdated(true, false, false);
				NotifyWorldPathTracedOutputInvalidated();

				return true;
			}
		}
	}

	return false;
}

void UTLLRN_PreviewMesh::EnableWireframe(bool bEnable)
{
	check(DynamicMeshComponent);
	DynamicMeshComponent->bExplicitShowWireframe = bEnable;
}

void UTLLRN_PreviewMesh::SetShadowsEnabled(bool bEnable)
{
	check(DynamicMeshComponent);
	DynamicMeshComponent->SetShadowsEnabled(bEnable);
}


FTransform UTLLRN_PreviewMesh::GetTransform() const
{
	if (TemporaryParentActor != nullptr)
	{
		return TemporaryParentActor->GetTransform();
	}
	return FTransform();
}

void UTLLRN_PreviewMesh::SetTransform(const FTransform& UseTransform)
{
	if (TemporaryParentActor != nullptr)
	{
		if (!TemporaryParentActor->GetActorTransform().Equals(UseTransform))
		{
			TemporaryParentActor->SetActorTransform(UseTransform);
			NotifyWorldPathTracedOutputInvalidated();
		}
	}
}

void UTLLRN_PreviewMesh::SetVisible(bool bVisible)
{
	if (DynamicMeshComponent != nullptr && IsVisible() != bVisible )
	{
		DynamicMeshComponent->SetVisibility(bVisible, true);
		NotifyWorldPathTracedOutputInvalidated();
	}
}


bool UTLLRN_PreviewMesh::IsVisible() const
{
	if (DynamicMeshComponent != nullptr)
	{
		return DynamicMeshComponent->IsVisible();
	}
	return false;
}



void UTLLRN_PreviewMesh::ClearPreview() 
{
	FDynamicMesh3 Empty;
	UpdatePreview(&Empty);
	
	if (bBuildSpatialDataStructure)
	{
		MeshAABBTree.SetMesh(&Empty, true);
	}
}


void UTLLRN_PreviewMesh::UpdatePreview(const FDynamicMesh3* Mesh, ERenderUpdateMode UpdateMode,
	EMeshRenderAttributeFlags ModifiedAttribs)
{
	DynamicMeshComponent->GetMesh()->Copy(*Mesh);

	NotifyDeferredEditCompleted(UpdateMode, ModifiedAttribs, bBuildSpatialDataStructure);
}

void UTLLRN_PreviewMesh::UpdatePreview(FDynamicMesh3&& Mesh, ERenderUpdateMode UpdateMode, 
	EMeshRenderAttributeFlags ModifiedAttribs)
{
	*(DynamicMeshComponent->GetMesh()) = MoveTemp(Mesh);

	NotifyDeferredEditCompleted(UpdateMode, ModifiedAttribs, bBuildSpatialDataStructure);
}


const FDynamicMesh3* UTLLRN_PreviewMesh::GetMesh() const
{
	if (DynamicMeshComponent != nullptr)
	{
		return DynamicMeshComponent->GetMesh();
	}
	return nullptr;
}


void UTLLRN_PreviewMesh::ProcessMesh(TFunctionRef<void(const UE::Geometry::FDynamicMesh3&)> ProcessFunc) const
{
	if (ensure(DynamicMeshComponent))
	{
		DynamicMeshComponent->ProcessMesh(ProcessFunc);
	}
}



FDynamicMeshAABBTree3* UTLLRN_PreviewMesh::GetSpatial()
{
	if (DynamicMeshComponent != nullptr && bBuildSpatialDataStructure)
	{
		if (MeshAABBTree.IsValid(false))
		{
			return &MeshAABBTree;
		}
	}
	return nullptr;
}




TUniquePtr<FDynamicMesh3> UTLLRN_PreviewMesh::ExtractTLLRN_PreviewMesh() const
{
	if (DynamicMeshComponent != nullptr)
	{
		return DynamicMeshComponent->GetDynamicMesh()->ExtractMesh();
	}
	return nullptr;
}



bool UTLLRN_PreviewMesh::TestRayIntersection(const FRay3d& WorldRay)
{
	if (IsVisible() && TemporaryParentActor != nullptr && bBuildSpatialDataStructure)
	{
		FFrame3d TransformFrame(TemporaryParentActor->GetActorTransform());
		FRay3d LocalRay = TransformFrame.ToFrame(WorldRay);
		int HitTriID = MeshAABBTree.FindNearestHitTriangle(LocalRay);
		if (HitTriID != FDynamicMesh3::InvalidID)
		{
			return true;
		}
	}
	return false;
}



bool UTLLRN_PreviewMesh::FindRayIntersection(const FRay3d& WorldRay, FHitResult& HitOut)
{
	if (IsVisible() && TemporaryParentActor != nullptr && bBuildSpatialDataStructure)
	{
		FTransformSRT3d Transform(TemporaryParentActor->GetActorTransform());
		FRay3d LocalRay(Transform.InverseTransformPosition(WorldRay.Origin),
			Transform.InverseTransformVector(WorldRay.Direction));
		UE::Geometry::Normalize(LocalRay.Direction);
		int HitTriID = MeshAABBTree.FindNearestHitTriangle(LocalRay);
		if (HitTriID != FDynamicMesh3::InvalidID)
		{
			const FDynamicMesh3* UseMesh = GetPreviewDynamicMesh();
			FTriangle3d Triangle;
			UseMesh->GetTriVertices(HitTriID, Triangle.V[0], Triangle.V[1], Triangle.V[2]);
			FIntrRay3Triangle3d Query(LocalRay, Triangle);
			Query.Find();

			HitOut.FaceIndex = HitTriID;
			HitOut.Distance = Query.RayParameter;
			HitOut.Normal = (FVector)Transform.TransformNormal(Triangle.Normal());
			HitOut.ImpactNormal = HitOut.Normal;
			HitOut.ImpactPoint = (FVector)Transform.TransformPosition(LocalRay.PointAt(Query.RayParameter));
			return true;
		}
	}
	return false;
}


FVector3d UTLLRN_PreviewMesh::FindNearestPoint(const FVector3d& WorldPoint, bool bLinearSearch)
{
	const FDynamicMesh3* UseMesh = GetMesh();
	if (bLinearSearch)
	{
		return TMeshQueries<FDynamicMesh3>::FindNearestPoint_LinearSearch(*UseMesh, WorldPoint);
	}
	if (bBuildSpatialDataStructure)
	{
		return MeshAABBTree.FindNearestPoint(WorldPoint);
	}
	return WorldPoint;
}



void UTLLRN_PreviewMesh::ReplaceMesh(const FDynamicMesh3& NewMesh)
{
	ReplaceMesh(FDynamicMesh3(NewMesh));
}

void UTLLRN_PreviewMesh::ReplaceMesh(FDynamicMesh3&& NewMesh)
{
	DynamicMeshComponent->SetMesh(MoveTemp(NewMesh));

	if (bBuildSpatialDataStructure)
	{
		MeshAABBTree.SetMesh(DynamicMeshComponent->GetMesh(), true);
	}
	if (bDecompositionEnabled)
	{
		UpdateRenderMeshDecomposition();
	}

	NotifyWorldPathTracedOutputInvalidated();
}


void UTLLRN_PreviewMesh::EditMesh(TFunctionRef<void(FDynamicMesh3&)> EditFunc)
{
	FDynamicMesh3* Mesh = DynamicMeshComponent->GetMesh();
	EditFunc(*Mesh);

	DynamicMeshComponent->NotifyMeshUpdated();

	if (bBuildSpatialDataStructure)
	{
		MeshAABBTree.SetMesh(DynamicMeshComponent->GetMesh(), true);
	}
	if (bDecompositionEnabled)
	{
		UpdateRenderMeshDecomposition();
	}

	NotifyWorldPathTracedOutputInvalidated();
}


void UTLLRN_PreviewMesh::DeferredEditMesh(TFunctionRef<void(FDynamicMesh3&)> EditFunc, bool bRebuildSpatial)
{
	FDynamicMesh3* Mesh = DynamicMeshComponent->GetMesh();
	EditFunc(*Mesh);

	if (bBuildSpatialDataStructure && bRebuildSpatial)
	{
		MeshAABBTree.SetMesh(DynamicMeshComponent->GetMesh(), true);
	}
}


void UTLLRN_PreviewMesh::ForceRebuildSpatial()
{
	if (bBuildSpatialDataStructure)
	{
		MeshAABBTree.SetMesh(DynamicMeshComponent->GetMesh(), true);
	}
}


void UTLLRN_PreviewMesh::NotifyDeferredEditCompleted(ERenderUpdateMode UpdateMode, EMeshRenderAttributeFlags ModifiedAttribs, bool bRebuildSpatial)
{
	if (bBuildSpatialDataStructure && bRebuildSpatial)
	{
		MeshAABBTree.SetMesh(DynamicMeshComponent->GetMesh(), true);
	}

	if (UpdateMode == ERenderUpdateMode::FullUpdate)
	{
		DynamicMeshComponent->NotifyMeshUpdated();
		if (bDecompositionEnabled)
		{
			UpdateRenderMeshDecomposition();
		}
	}
	else if (UpdateMode == ERenderUpdateMode::FastUpdate)
	{
		bool bPositions = (ModifiedAttribs & EMeshRenderAttributeFlags::Positions) != EMeshRenderAttributeFlags::None;
		bool bNormals = (ModifiedAttribs & EMeshRenderAttributeFlags::VertexNormals) != EMeshRenderAttributeFlags::None;
		bool bColors = (ModifiedAttribs & EMeshRenderAttributeFlags::VertexColors) != EMeshRenderAttributeFlags::None;
		bool bUVs = (ModifiedAttribs & EMeshRenderAttributeFlags::VertexUVs) != EMeshRenderAttributeFlags::None;
		if (bPositions)
		{
			DynamicMeshComponent->FastNotifyPositionsUpdated(bNormals, bColors, bUVs);
		}
		else
		{
			DynamicMeshComponent->FastNotifyVertexAttributesUpdated(bNormals, bColors, bUVs);
		}
	}

	NotifyWorldPathTracedOutputInvalidated();
}


TUniquePtr<FMeshChange> UTLLRN_PreviewMesh::TrackedEditMesh(TFunctionRef<void(FDynamicMesh3&, FDynamicMeshChangeTracker&)> EditFunc)
{
	FDynamicMesh3* Mesh = DynamicMeshComponent->GetMesh();

	FDynamicMeshChangeTracker ChangeTracker(Mesh);
	ChangeTracker.BeginChange();
	EditFunc(*Mesh, ChangeTracker);
	TUniquePtr<FMeshChange> Change = MakeUnique<FMeshChange>(ChangeTracker.EndChange());

	DynamicMeshComponent->NotifyMeshUpdated();
	if (bBuildSpatialDataStructure)
	{
		MeshAABBTree.SetMesh(DynamicMeshComponent->GetMesh(), true);
	}
	if (bDecompositionEnabled)
	{
		UpdateRenderMeshDecomposition();
	}

	NotifyWorldPathTracedOutputInvalidated();

	return MoveTemp(Change);
}


void UTLLRN_PreviewMesh::ApplyChange(const FMeshVertexChange* Change, bool bRevert)
{
	check(DynamicMeshComponent != nullptr);
	DynamicMeshComponent->ApplyChange(Change, bRevert);
	if (bBuildSpatialDataStructure)
	{
		MeshAABBTree.SetMesh(DynamicMeshComponent->GetMesh(), true);
	}
	// should not need to update render mesh decomposition here...
}
void UTLLRN_PreviewMesh::ApplyChange(const FMeshChange* Change, bool bRevert)
{
	check(DynamicMeshComponent != nullptr);
	DynamicMeshComponent->ApplyChange(Change, bRevert);
	if (bBuildSpatialDataStructure)
	{
		MeshAABBTree.SetMesh(DynamicMeshComponent->GetMesh(), true);
	}
	if (bDecompositionEnabled)
	{
		UpdateRenderMeshDecomposition();
	}

	NotifyWorldPathTracedOutputInvalidated();
}
void UTLLRN_PreviewMesh::ApplyChange(const FMeshReplacementChange* Change, bool bRevert)
{
	check(DynamicMeshComponent != nullptr);
	DynamicMeshComponent->ApplyChange(Change, bRevert);
	if (bBuildSpatialDataStructure)
	{
		MeshAABBTree.SetMesh(DynamicMeshComponent->GetMesh(), true);
	}
	if (bDecompositionEnabled)
	{
		UpdateRenderMeshDecomposition();
	}

	NotifyWorldPathTracedOutputInvalidated();
}

FSimpleMulticastDelegate& UTLLRN_PreviewMesh::GetOnMeshChanged()
{
	check(DynamicMeshComponent != nullptr);
	return DynamicMeshComponent->OnMeshChanged;
}



void UTLLRN_PreviewMesh::SetTriangleColorFunction(TFunction<FColor(const FDynamicMesh3*, int)> TriangleColorFunc, ERenderUpdateMode UpdateMode)
{
	DynamicMeshComponent->SetTriangleColorFunction(TriangleColorFunc,  (EDynamicMeshComponentRenderUpdateMode)(int32)UpdateMode );
}

void UTLLRN_PreviewMesh::ClearTriangleColorFunction(ERenderUpdateMode UpdateMode)
{
	DynamicMeshComponent->ClearTriangleColorFunction((EDynamicMeshComponentRenderUpdateMode)(int32)UpdateMode);
}


void UTLLRN_PreviewMesh::SetEnableRenderMeshDecomposition(bool bEnable)
{
	if (bDecompositionEnabled != bEnable)
	{
		bDecompositionEnabled = bEnable;

		if (bDecompositionEnabled)
		{
			UpdateRenderMeshDecomposition();
		}
		else
		{
			DynamicMeshComponent->SetExternalDecomposition(nullptr);
		}
	}
}


void UTLLRN_PreviewMesh::UpdateRenderMeshDecomposition()
{
	check(bDecompositionEnabled);

	const FDynamicMesh3* Mesh = DynamicMeshComponent->GetMesh();
	FComponentMaterialSet MaterialSet;
	GetMaterials(MaterialSet.Materials);

	TUniquePtr<FMeshRenderDecomposition> Decomp = MakeUnique<FMeshRenderDecomposition>();
	FMeshRenderDecomposition::BuildChunkedDecomposition(Mesh, &MaterialSet, *Decomp);
	Decomp->BuildAssociations(Mesh);
	DynamicMeshComponent->SetExternalDecomposition(MoveTemp(Decomp));
}


void UTLLRN_PreviewMesh::NotifyRegionDeferredEditCompleted(const TArray<int32>& Triangles, EMeshRenderAttributeFlags ModifiedAttribs)
{
	DynamicMeshComponent->FastNotifyTriangleVerticesUpdated(Triangles, ModifiedAttribs);
	NotifyWorldPathTracedOutputInvalidated();
}

void UTLLRN_PreviewMesh::NotifyRegionDeferredEditCompleted(const TSet<int32>& Triangles, EMeshRenderAttributeFlags ModifiedAttribs)
{
	DynamicMeshComponent->FastNotifyTriangleVerticesUpdated(Triangles, ModifiedAttribs);
	NotifyWorldPathTracedOutputInvalidated();
}


void UTLLRN_PreviewMesh::NotifyWorldPathTracedOutputInvalidated()
{
	if (TemporaryParentActor != nullptr)
	{
		UWorld* World = TemporaryParentActor->GetWorld();
		if (World && World->Scene && FApp::CanEverRender())
		{
			World->Scene->InvalidatePathTracedOutput();
		}
	}
}

