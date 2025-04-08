// Copyright Epic Games, Inc. All Rights Reserved.

#include "Drawing/TLLRN_UVLayoutPreview.h"
#include "SceneManagement.h"
#include "ToolSetupUtil.h"
#include "Drawing/TLLRN_MeshElementsVisualizer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVLayoutPreview)

using namespace UE::Geometry;

UTLLRN_UVLayoutPreview::~UTLLRN_UVLayoutPreview()
{
	checkf(TLLRN_PreviewMesh == nullptr, TEXT("You must explicitly Disconnect() TLLRN_UVLayoutPreview before it is GCd"));
}



void UTLLRN_UVLayoutPreview::CreateInWorld(UWorld* World)
{
	TLLRN_PreviewMesh = NewObject<UTLLRN_PreviewMesh>(this);
	TLLRN_PreviewMesh->CreateInWorld(World, FTransform::Identity);
	TLLRN_PreviewMesh->SetShadowsEnabled(false);

	TLLRN_MeshElementsVisualizer = NewObject<UTLLRN_MeshElementsVisualizer>(this);
	TLLRN_MeshElementsVisualizer->CreateInWorld(World, FTransform::Identity);
	TLLRN_MeshElementsVisualizer->SetMeshAccessFunction([this](UTLLRN_MeshElementsVisualizer::ProcessDynamicMeshFunc ProcessFunc) {
		TLLRN_PreviewMesh->ProcessMesh(ProcessFunc);
	});	

	TriangleComponent = NewObject<UTLLRN_TriangleSetComponent>(TLLRN_PreviewMesh->GetActor());
	TriangleComponent->SetupAttachment(TLLRN_PreviewMesh->GetRootComponent());
	TriangleComponent->RegisterComponent();

	Settings = NewObject<UTLLRN_UVLayoutPreviewProperties>(this);
	Settings->WatchProperty(Settings->bEnabled, [this](bool) { bSettingsModified = true; });
	Settings->WatchProperty(Settings->bShowWireframe, [this](bool) { bSettingsModified = true; });
	//Settings->WatchProperty(Settings->bWireframe, [this](bool) { bSettingsModified = true; });
	bSettingsModified = true;

	TLLRN_MeshElementsVisualizer->Settings->bVisible = Settings->bEnabled;
	TLLRN_MeshElementsVisualizer->Settings->bShowWireframe = Settings->bShowWireframe;
	TLLRN_MeshElementsVisualizer->Settings->bShowBorders = Settings->bShowWireframe;
	TLLRN_MeshElementsVisualizer->Settings->bShowUVSeams = false;
	TLLRN_MeshElementsVisualizer->Settings->bShowNormalSeams = false;
	TLLRN_MeshElementsVisualizer->Settings->bShowColorSeams = false;
	TLLRN_MeshElementsVisualizer->Settings->ThicknessScale = 0.5;
	TLLRN_MeshElementsVisualizer->Settings->DepthBias = 0.5;
	TLLRN_MeshElementsVisualizer->Settings->bAdjustDepthBiasUsingMeshSize = false;
	TLLRN_MeshElementsVisualizer->Settings->CheckAndUpdateWatched();

	BackingRectangleMaterial = ToolSetupUtil::GetSelectionMaterial(FLinearColor::White, nullptr);
	if (BackingRectangleMaterial == nullptr)
	{
		BackingRectangleMaterial = ToolSetupUtil::GetDefaultMaterial();
	}
}


void UTLLRN_UVLayoutPreview::Disconnect()
{
	TLLRN_PreviewMesh->Disconnect();
	TLLRN_PreviewMesh = nullptr;

	TLLRN_MeshElementsVisualizer->Disconnect();
	TLLRN_MeshElementsVisualizer = nullptr;
}


void UTLLRN_UVLayoutPreview::SetSourceMaterials(const FComponentMaterialSet& MaterialSet)
{
	SourceMaterials = MaterialSet;

	TLLRN_PreviewMesh->SetMaterials(SourceMaterials.Materials);
}


void UTLLRN_UVLayoutPreview::SetSourceWorldPosition(FTransform WorldTransform, FBox WorldBounds)
{
	SourceObjectWorldBounds = FAxisAlignedBox3d(WorldBounds);

	SourceObjectFrame = FFrame3d(WorldTransform);
}


void UTLLRN_UVLayoutPreview::SetCurrentCameraState(const FViewCameraState& CameraStateIn)
{
	CameraState = CameraStateIn;
}


void UTLLRN_UVLayoutPreview::SetTransform(const FTransform& UseTransform)
{
	TLLRN_PreviewMesh->SetTransform(UseTransform);
	TLLRN_MeshElementsVisualizer->SetTransform(UseTransform);
}


void UTLLRN_UVLayoutPreview::SetVisible(bool bVisible)
{
	TLLRN_PreviewMesh->SetVisible(bVisible);
	TLLRN_MeshElementsVisualizer->Settings->bVisible = bVisible;
}


void UTLLRN_UVLayoutPreview::Render(IToolsContextRenderAPI* RenderAPI)
{
	SetCurrentCameraState(RenderAPI->GetCameraState());

	RecalculatePosition();

	if (Settings->bEnabled)
	{
		float ScaleFactor = GetCurrentScale();
		FVector Origin = (FVector)CurrentWorldFrame.Origin;
		FVector DX = ScaleFactor * (FVector)CurrentWorldFrame.X();
		FVector DY = ScaleFactor * (FVector)CurrentWorldFrame.Y();

		RenderAPI->GetPrimitiveDrawInterface()->DrawLine(
			Origin, Origin+DX, FLinearColor::Black, SDPG_Foreground, 0.5f, 0.0f, true);
		RenderAPI->GetPrimitiveDrawInterface()->DrawLine(
			Origin+DX, Origin+DX+DY, FLinearColor::Black, SDPG_Foreground, 0.5f, 0.0f, true);
		RenderAPI->GetPrimitiveDrawInterface()->DrawLine(
			Origin+DX+DY, Origin+DY, FLinearColor::Black, SDPG_Foreground, 0.5f, 0.0f, true);
		RenderAPI->GetPrimitiveDrawInterface()->DrawLine(
			Origin+DY, Origin, FLinearColor::Black, SDPG_Foreground, 0.5f, 0.0f, true);
	}
}



void UTLLRN_UVLayoutPreview::OnTick(float DeltaTime)
{
	if (bSettingsModified)
	{
		SetVisible(Settings->bEnabled);

		TLLRN_MeshElementsVisualizer->Settings->bShowWireframe = Settings->bShowWireframe;
		TLLRN_MeshElementsVisualizer->Settings->bShowBorders = Settings->bShowWireframe;
		TLLRN_MeshElementsVisualizer->Settings->CheckAndUpdateWatched();

		bSettingsModified = false;
	}

	TLLRN_MeshElementsVisualizer->OnTick(DeltaTime);
}


float UTLLRN_UVLayoutPreview::GetCurrentScale()
{
	return Settings->Scale * SourceObjectWorldBounds.Height();
}


void UTLLRN_UVLayoutPreview::UpdateUVMesh(const FDynamicMesh3* SourceMesh, int32 SourceUVLayer)
{
	FDynamicMesh3 UVMesh;
	UVMesh.EnableAttributes();
	FDynamicMeshUVOverlay* NewUVOverlay = UVMesh.Attributes()->GetUVLayer(0);
	FDynamicMeshNormalOverlay* NewNormalOverlay = UVMesh.Attributes()->PrimaryNormals();

	FAxisAlignedBox2f Bounds = FAxisAlignedBox2f(FVector2f::Zero(), FVector2f::One());
	const FDynamicMeshUVOverlay* UVOverlay = SourceMesh->Attributes()->GetUVLayer(SourceUVLayer);
	for (int32 tid : SourceMesh->TriangleIndicesItr())
	{
		if (UVOverlay->IsSetTriangle(tid))
		{
			FIndex3i UVTri = UVOverlay->GetTriangle(tid);

			FVector2f UVs[3];
			for (int32 j = 0; j < 3; ++j)
			{
				UVs[j] = UVOverlay->GetElement(UVTri[j]);
				Bounds.Contain(UVs[j]);
			}

			FIndex3i NewTri, NewUVTri, NewNormalTri;
			for (int32 j = 0; j < 3; ++j)
			{
				NewUVTri[j] = NewUVOverlay->AppendElement(UVs[j]);
				NewTri[j] = UVMesh.AppendVertex(FVector3d(UVs[j].X, UVs[j].Y, 0));
				NewNormalTri[j] = NewNormalOverlay->AppendElement( FVector3f::UnitZ() );
			}

			FVector2f EdgeUV1 = UVs[1] - UVs[0];
			FVector2f EdgeUV2 = UVs[2] - UVs[0];
			float SignedUVArea = 0.5f * (EdgeUV1.X * EdgeUV2.Y - EdgeUV1.Y * EdgeUV2.X);

			if (SignedUVArea > 0)
			{
				Swap(NewTri.A, NewTri.B);
				Swap(NewUVTri.A, NewUVTri.B);
			}


			int32 NewTriID = UVMesh.AppendTriangle(NewTri);
			NewUVOverlay->SetTriangle(NewTriID, NewUVTri);
			NewNormalOverlay->SetTriangle(NewTriID, NewNormalTri);
		}
	}

	Bounds.Expand(0.01);

	float BackZ = -0.01;
	TriangleComponent->Clear();
	if (bShowBackingRectangle)
	{
		TriangleComponent->AddQuad(
			FVector(Bounds.Min.X, Bounds.Min.Y, BackZ), 
			FVector(Bounds.Min.X, Bounds.Max.Y, BackZ), 
			FVector(Bounds.Max.X, Bounds.Max.Y, BackZ), 
			FVector(Bounds.Max.X, Bounds.Min.Y, BackZ),
			FVector(0, 0, -1), FColor::White, BackingRectangleMaterial);
	}

	TLLRN_PreviewMesh->UpdatePreview(MoveTemp(UVMesh));
	TLLRN_MeshElementsVisualizer->NotifyMeshChanged();
}




void UTLLRN_UVLayoutPreview::RecalculatePosition()
{
	FFrame3d ObjFrame;
	ObjFrame.AlignAxis(2, -(FVector3d)CameraState.Forward());
	ObjFrame.ConstrainedAlignAxis(0, (FVector3d)CameraState.Right(), ObjFrame.Z());
	ObjFrame.Origin = SourceObjectWorldBounds.Center(); // SourceObjectFrame.Origin;

	FAxisAlignedBox2d ProjectedBounds = FAxisAlignedBox2d::Empty();
	for (int32 k = 0; k < 8; ++k)
	{
		ProjectedBounds.Contain(ObjFrame.ToPlaneUV(SourceObjectWorldBounds.GetCorner(k)));
	}

	double UseScale = GetCurrentScale();

	double ShiftRight = Settings->Offset.X * (ProjectedBounds.Max.X + (ProjectedBounds.Width() * 0.1));
	if (Settings->Side == ETLLRN_UVLayoutPreviewSide::Left)
	{
		ShiftRight = ProjectedBounds.Min.X - Settings->Offset.X * (UseScale + (ProjectedBounds.Width() * 0.1));
	}

	double ShiftUp = Settings->Offset.Y * UseScale;
	ObjFrame.Origin += ShiftRight * ObjFrame.X() - ShiftUp * ObjFrame.Y();

	CurrentWorldFrame = ObjFrame;

	FTransformSRT3d Transform(ObjFrame.Rotation, ObjFrame.Origin);
	Transform.SetScale(UseScale * FVector3d::One());

	SetTransform((FTransform)Transform);
}
