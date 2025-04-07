// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tools/TLLRN_ControlTLLRN_RigPoseThumbnailRenderer.h"
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "ThumbnailHelpers.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "AnimCustomInstanceHelper.h"
#include "SceneView.h"
#include "Sequencer/TLLRN_ControlRigLayerInstance.h"
#include "UObject/Package.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlTLLRN_RigPoseThumbnailRenderer)


/*
***************************************************************
FTLLRN_ControlTLLRN_RigPoseThumbnailScene
***************************************************************
*/

class FTLLRN_ControlTLLRN_RigPoseThumbnailScene : public FThumbnailPreviewScene
{
public:
	/** Constructor */
	FTLLRN_ControlTLLRN_RigPoseThumbnailScene();

	bool SetTLLRN_ControlTLLRN_RigPoseAsset(UTLLRN_ControlTLLRN_RigPoseAsset* PoseAsset);

protected:
	// FThumbnailPreviewScene implementation
	virtual void GetViewMatrixParameters(const float InFOVDegrees, FVector& OutOrigin, float& OutOrbitPitch, float& OutOrbitYaw, float& OutOrbitZoom) const override;

	//Clean up the children of this component
	void CleanupComponentChildren(USceneComponent* Component);

private:
	/** The skeletal mesh actor*/
	ASkeletalMeshActor* PreviewActor;

	/** TLLRN_ControlRig Pose Asset*/
	UTLLRN_ControlTLLRN_RigPoseAsset* PoseAsset;

};


FTLLRN_ControlTLLRN_RigPoseThumbnailScene::FTLLRN_ControlTLLRN_RigPoseThumbnailScene()
	: FThumbnailPreviewScene()
{
	bForceAllUsedMipsResident = false;

	// Create preview actor
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.bNoFail = true;
	SpawnInfo.ObjectFlags = RF_Transient;
	PreviewActor = GetWorld()->SpawnActor<ASkeletalMeshActor>(SpawnInfo);

	PreviewActor->SetActorEnableCollision(false);
}

bool FTLLRN_ControlTLLRN_RigPoseThumbnailScene::SetTLLRN_ControlTLLRN_RigPoseAsset(UTLLRN_ControlTLLRN_RigPoseAsset* InPoseAsset)
{
	PoseAsset = InPoseAsset;
	bool bSetSucessfully = false;

	if (PoseAsset)
	{
		USkeletalMesh* SkeletalMesh = nullptr; // PoseAsset->GetSkeletalMeshAsset();
		UTLLRN_ControlRig* TLLRN_ControlRig = nullptr; // PoseAsset->GetTLLRN_ControlRig();
		PreviewActor->GetSkeletalMeshComponent()->OverrideMaterials.Empty();


		if (SkeletalMesh && TLLRN_ControlRig)
		{

			PreviewActor->GetSkeletalMeshComponent()->SetSkeletalMesh(SkeletalMesh);
			PreviewActor->GetSkeletalMeshComponent()->AnimScriptInstance = nullptr;


			bSetSucessfully = true;
			//now set up the control rig and the anim instance fo rit
			UTLLRN_ControlRig* TempTLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(GetTransientPackage(), TLLRN_ControlRig->GetClass(),NAME_None, RF_Transient);
			TempTLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
			TempTLLRN_ControlRig->GetObjectBinding()->BindToObject(SkeletalMesh);
			TempTLLRN_ControlRig->GetDataSourceRegistry()->RegisterDataSource(UTLLRN_ControlRig::OwnerComponent, TempTLLRN_ControlRig->GetObjectBinding()->GetBoundObject());


			bool bWasCreated;
			if (UTLLRN_ControlRigLayerInstance* AnimInstance = FAnimCustomInstanceHelper::BindToSkeletalMeshComponent<UTLLRN_ControlRigLayerInstance>(PreviewActor->GetSkeletalMeshComponent(), bWasCreated))
			{

				if (bWasCreated)
				{
					AnimInstance->RecalcRequiredBones();
					AnimInstance->AddTLLRN_ControlRigTrack(TempTLLRN_ControlRig->GetUniqueID(), TempTLLRN_ControlRig);
					TempTLLRN_ControlRig->CreateTLLRN_RigControlsForCurveContainer();
				}
				TempTLLRN_ControlRig->Initialize();

				float Weight = 1.0f;
				FTLLRN_ControlRigIOSettings InputSettings;
				InputSettings.bUpdateCurves = false;
				InputSettings.bUpdatePose = true;
				AnimInstance->UpdateTLLRN_ControlRigTrack(TempTLLRN_ControlRig->GetUniqueID(), Weight, InputSettings, true);
				PoseAsset->PastePose(TempTLLRN_ControlRig);
				TempTLLRN_ControlRig->Evaluate_AnyThread();

			}

			PreviewActor->GetSkeletalMeshComponent()->TickAnimation(0.f, false);
			PreviewActor->GetSkeletalMeshComponent()->RefreshBoneTransforms();
			PreviewActor->GetSkeletalMeshComponent()->FinalizeBoneTransform();

			FTransform MeshTransform = FTransform::Identity;

			PreviewActor->SetActorLocation(FVector(0, 0, 0), false);
			PreviewActor->GetSkeletalMeshComponent()->UpdateBounds();

			// Center the mesh at the world origin then offset to put it on top of the plane
			const float BoundsZOffset = GetBoundsZOffset(PreviewActor->GetSkeletalMeshComponent()->Bounds);
			PreviewActor->SetActorLocation(-PreviewActor->GetSkeletalMeshComponent()->Bounds.Origin + FVector(0, 0, BoundsZOffset), false);
			PreviewActor->GetSkeletalMeshComponent()->RecreateRenderState_Concurrent();
			TempTLLRN_ControlRig->MarkAsGarbage();

		}

		if (!bSetSucessfully)
		{
			CleanupComponentChildren(PreviewActor->GetSkeletalMeshComponent());
			PreviewActor->GetSkeletalMeshComponent()->SetSkeletalMesh(nullptr);
			PreviewActor->GetSkeletalMeshComponent()->SetAnimInstanceClass(nullptr);
			PreviewActor->GetSkeletalMeshComponent()->AnimScriptInstance = nullptr;
		}

	}
	return bSetSucessfully;
}

void FTLLRN_ControlTLLRN_RigPoseThumbnailScene::CleanupComponentChildren(USceneComponent* Component)
{
	if (Component)
	{
		for (int32 ComponentIdx = Component->GetAttachChildren().Num() - 1; ComponentIdx >= 0; --ComponentIdx)
		{
			CleanupComponentChildren(Component->GetAttachChildren()[ComponentIdx]);
			Component->GetAttachChildren()[ComponentIdx]->DestroyComponent();
		}
		check(Component->GetAttachChildren().Num() == 0);
	}
}

void FTLLRN_ControlTLLRN_RigPoseThumbnailScene::GetViewMatrixParameters(const float InFOVDegrees, FVector& OutOrigin, float& OutOrbitPitch, float& OutOrbitYaw, float& OutOrbitZoom) const
{
	check(PreviewActor->GetSkeletalMeshComponent());
	check(PreviewActor->GetSkeletalMeshComponent()->GetSkeletalMeshAsset());

	const float HalfFOVRadians = FMath::DegreesToRadians<float>(InFOVDegrees) * 0.5f;
	// No need to add extra size to view slightly outside of the sphere to compensate for perspective since skeletal meshes already buffer bounds.
	const float HalfMeshSize = PreviewActor->GetSkeletalMeshComponent()->Bounds.SphereRadius;
	const float BoundsZOffset = GetBoundsZOffset(PreviewActor->GetSkeletalMeshComponent()->Bounds);
	const float TargetDistance = HalfMeshSize / FMath::Tan(HalfFOVRadians);

	USceneThumbnailInfo* ThumbnailInfo = nullptr;// Cast<USceneThumbnailInfo>(PoseAsset->ThumbnailInfo);
	if (ThumbnailInfo)
	{
		if (TargetDistance + ThumbnailInfo->OrbitZoom < 0)
		{
			ThumbnailInfo->OrbitZoom = -TargetDistance;
		}
	}
	else
	{
		ThumbnailInfo = USceneThumbnailInfo::StaticClass()->GetDefaultObject<USceneThumbnailInfo>();
	}

	OutOrigin = FVector(0, 0, -BoundsZOffset);
	OutOrbitPitch = ThumbnailInfo->OrbitPitch;
	OutOrbitYaw = ThumbnailInfo->OrbitYaw;
	OutOrbitZoom = TargetDistance + ThumbnailInfo->OrbitZoom;
}



/*
***************************************************************
UTLLRN_ControlTLLRN_RigPoseThumbnailRenderer
***************************************************************
*/

UTLLRN_ControlTLLRN_RigPoseThumbnailRenderer::UTLLRN_ControlTLLRN_RigPoseThumbnailRenderer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TLLRN_ControlTLLRN_RigPoseAsset = nullptr;
	ThumbnailScene = nullptr;
}

bool UTLLRN_ControlTLLRN_RigPoseThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	return Cast<UTLLRN_ControlTLLRN_RigPoseAsset>(Object) != nullptr;
}

void UTLLRN_ControlTLLRN_RigPoseThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UTLLRN_ControlTLLRN_RigPoseAsset* PoseAsset = Cast<UTLLRN_ControlTLLRN_RigPoseAsset>(Object);
	if (PoseAsset)
	{
		
		if (ThumbnailScene == nullptr)
		{
			ThumbnailScene = new FTLLRN_ControlTLLRN_RigPoseThumbnailScene();
		}

		ThumbnailScene->SetTLLRN_ControlTLLRN_RigPoseAsset(PoseAsset);

		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(RenderTarget, ThumbnailScene->GetScene(), FEngineShowFlags(ESFIM_Game))
			.SetTime(UThumbnailRenderer::GetTime())
			.SetAdditionalViewFamily(bAdditionalViewFamily));

		ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
		ViewFamily.EngineShowFlags.MotionBlur = 0;
		ViewFamily.EngineShowFlags.LOD = 0;

		RenderViewFamily(Canvas, &ViewFamily, ThumbnailScene->CreateView(&ViewFamily, X, Y, Width, Height));
		ThumbnailScene->SetTLLRN_ControlTLLRN_RigPoseAsset(nullptr);
	}
}

void UTLLRN_ControlTLLRN_RigPoseThumbnailRenderer::BeginDestroy()
{
	if (ThumbnailScene != nullptr)
	{
		delete ThumbnailScene;
		ThumbnailScene = nullptr;
	}

	Super::BeginDestroy();
}


