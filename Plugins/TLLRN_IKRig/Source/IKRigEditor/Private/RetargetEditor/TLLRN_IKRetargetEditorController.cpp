// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_IKRetargetEditorController.h"

#include "AnimPose.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "EditorModeManager.h"
#include "IContentBrowserSingleton.h"
#include "SkeletalDebugRendering.h"
#include "Widgets/Input/SButton.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Animation/PoseAsset.h"
#include "Dialog/SCustomDialog.h"
#include "Preferences/PersonaOptions.h"
#include "TLLRN_IKRigEditor.h"
#include "MeshDescription.h"
#include "SkeletalMeshAttributes.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "RetargetEditor/TLLRN_IKRetargetAnimInstance.h"
#include "RetargetEditor/TLLRN_IKRetargetDefaultMode.h"
#include "RetargetEditor/TLLRN_IKRetargetEditPoseMode.h"
#include "RetargetEditor/TLLRN_IKRetargetEditor.h"
#include "RetargetEditor/TLLRN_IKRetargetEditorStyle.h"
#include "RetargetEditor/TLLRN_IKRetargetHitProxies.h"
#include "RetargetEditor/TLLRN_SIKRetargetChainMapList.h"
#include "RetargetEditor/TLLRN_SIKRetargetHierarchy.h"
#include "Retargeter/TLLRN_IKRetargeter.h"
#include "Retargeter/TLLRN_IKRetargetOps.h"
#include "RetargetEditor/TLLRN_SRetargetOpStack.h"
#include "RigEditor/TLLRN_SIKRigOutputLog.h"
#include "RigEditor/TLLRN_IKRigController.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRetargetEditorController"


FTLLRN_BoundIKRig::FTLLRN_BoundIKRig(UTLLRN_IKRigDefinition* InIKRig, const FTLLRN_IKRetargetEditorController& InController)
{
	check(InIKRig);
	IKRig = InIKRig;
	UTLLRN_IKRigController* TLLRN_IKRigController = UTLLRN_IKRigController::GetController(InIKRig);
	ReInitIKDelegateHandle = TLLRN_IKRigController->OnIKRigNeedsInitialized().AddSP(&InController, &FTLLRN_IKRetargetEditorController::HandleIKRigNeedsInitialized);
	AddedChainDelegateHandle = TLLRN_IKRigController->OnRetargetChainAdded().AddSP(&InController, &FTLLRN_IKRetargetEditorController::HandleRetargetChainAdded);
	RemoveChainDelegateHandle = TLLRN_IKRigController->OnRetargetChainRemoved().AddSP(&InController, &FTLLRN_IKRetargetEditorController::HandleRetargetChainRemoved);
	RenameChainDelegateHandle = TLLRN_IKRigController->OnRetargetChainRenamed().AddSP(&InController, &FTLLRN_IKRetargetEditorController::HandleRetargetChainRenamed);
}

void FTLLRN_BoundIKRig::UnBind() const
{
	if (!IKRig.IsValid())
	{
		return;
	}
	
	UTLLRN_IKRigController* TLLRN_IKRigController = UTLLRN_IKRigController::GetController(IKRig.Get());
	TLLRN_IKRigController->OnIKRigNeedsInitialized().Remove(ReInitIKDelegateHandle);
	TLLRN_IKRigController->OnRetargetChainAdded().Remove(AddedChainDelegateHandle);
	TLLRN_IKRigController->OnRetargetChainRemoved().Remove(RemoveChainDelegateHandle);
	TLLRN_IKRigController->OnRetargetChainRenamed().Remove(RenameChainDelegateHandle);
}

FTLLRN_RetargetPlaybackManager::FTLLRN_RetargetPlaybackManager(const TWeakPtr<FTLLRN_IKRetargetEditorController>& InEditorController)
{
	check(InEditorController.Pin())
	EditorController = InEditorController;
}

void FTLLRN_RetargetPlaybackManager::PlayAnimationAsset(UAnimationAsset* AssetToPlay)
{
	UTLLRN_IKRetargetAnimInstance* AnimInstance = EditorController.Pin()->SourceAnimInstance.Get();
	if (!AnimInstance)
	{
		return;
	}
	
	if (AssetToPlay)
	{
		AnimInstance->SetAnimationAsset(AssetToPlay);
		AnimInstance->SetPlaying(true);
		AnimThatWasPlaying = AssetToPlay;
		// ensure we are running the retargeter so you can see the animation
		if (FTLLRN_IKRetargetEditorController* Controller = EditorController.Pin().Get())
		{
			Controller->SetRetargeterMode(ETLLRN_RetargeterOutputMode::RunRetarget);
		}
	}
}

void FTLLRN_RetargetPlaybackManager::StopPlayback()
{
	UTLLRN_IKRetargetAnimInstance* AnimInstance = EditorController.Pin()->SourceAnimInstance.Get();
	if (!AnimInstance)
	{
		return;
	}

	AnimThatWasPlaying = AnimInstance->GetAnimationAsset();
	AnimInstance->SetPlaying(false);
	AnimInstance->SetAnimationAsset(nullptr);
}

void FTLLRN_RetargetPlaybackManager::PausePlayback()
{
	UTLLRN_IKRetargetAnimInstance* AnimInstance = EditorController.Pin()->SourceAnimInstance.Get();
	if (!AnimInstance)
	{
		return;
	}
	
	if (AnimThatWasPlaying)
	{
		TimeWhenPaused = AnimInstance->GetCurrentTime();
	}

	AnimThatWasPlaying = AnimInstance->GetAnimationAsset();
	AnimInstance->SetPlaying(false);
}

void FTLLRN_RetargetPlaybackManager::ResumePlayback() const
{
	UTLLRN_IKRetargetAnimInstance* AnimInstance = EditorController.Pin()->SourceAnimInstance.Get();
	if (!AnimInstance)
	{
		return;
	}
	
	if (AnimThatWasPlaying)
	{
		AnimInstance->SetAnimationAsset(AnimThatWasPlaying);
		AnimInstance->SetPlaying(true);
		AnimInstance->SetPosition(TimeWhenPaused);	
	}
}

bool FTLLRN_RetargetPlaybackManager::IsStopped() const
{
	const UTLLRN_IKRetargetAnimInstance* AnimInstance = EditorController.Pin()->SourceAnimInstance.Get();
	if (!AnimInstance)
	{
		return true;
	}
	
	return !AnimInstance->GetAnimationAsset();
}

void FTLLRN_IKRetargetEditorController::Initialize(TSharedPtr<FTLLRN_IKRetargetEditor> InEditor, UTLLRN_IKRetargeter* InAsset)
{
	Editor = InEditor;
	AssetController = UTLLRN_IKRetargeterController::GetController(InAsset);
	CurrentlyEditingSourceOrTarget = ETLLRN_RetargetSourceOrTarget::Target;
	OutputMode = ETLLRN_RetargeterOutputMode::EditRetargetPose;
	PreviousMode = ETLLRN_RetargeterOutputMode::EditRetargetPose;
	PoseExporter = MakeShared<FTLLRN_IKRetargetPoseExporter>();
	PoseExporter->Initialize(SharedThis(this));
	RefreshPoseList();

	PlaybackManager = MakeUnique<FTLLRN_RetargetPlaybackManager>(SharedThis(this));

	SelectedBoneNames.Add(ETLLRN_RetargetSourceOrTarget::Source);
	SelectedBoneNames.Add(ETLLRN_RetargetSourceOrTarget::Target);
	LastSelectedItem = ETLLRN_RetargetSelectionType::NONE;

	// clean the asset before editing
	AssetController->CleanAsset();

	// bind callbacks when SOURCE or TARGET IK Rigs are modified
	BindToIKRigAssets();

	// bind callback when retargeter needs reinitialized
	RetargeterReInitDelegateHandle = AssetController->OnRetargeterNeedsInitialized().AddSP(this, &FTLLRN_IKRetargetEditorController::HandleRetargeterNeedsInitialized);
	// bind callback when IK Rig asset is replaced with a different asset
	IKRigReplacedDelegateHandle = AssetController->OnIKRigReplaced().AddSP(this, &FTLLRN_IKRetargetEditorController::HandleIKRigReplaced);
	// bind callback when Preview Mesh asset is replaced with a different asset
	PreviewMeshReplacedDelegateHandle = AssetController->OnPreviewMeshReplaced().AddSP(this, &FTLLRN_IKRetargetEditorController::HandlePreviewMeshReplaced);
}

void FTLLRN_IKRetargetEditorController::Close()
{
	AssetController->OnRetargeterNeedsInitialized().Remove(RetargeterReInitDelegateHandle);
	AssetController->OnIKRigReplaced().Remove(IKRigReplacedDelegateHandle);
	AssetController->OnPreviewMeshReplaced().Remove(PreviewMeshReplacedDelegateHandle);
	GetRetargetProcessor()->OnRetargeterInitialized().Remove(RetargeterInitializedDelegateHandle);

	for (const FTLLRN_BoundIKRig& BoundIKRig : BoundIKRigs)
	{
		BoundIKRig.UnBind();
	}
}

void FTLLRN_IKRetargetEditorController::BindToIKRigAssets()
{
	const UTLLRN_IKRetargeter* Asset = AssetController->GetAsset();
	if (!Asset)
	{
		return;
	}
	
	// unbind previously bound IK Rigs
	for (const FTLLRN_BoundIKRig& BoundIKRig : BoundIKRigs)
	{
		BoundIKRig.UnBind();
	}

	BoundIKRigs.Empty();
	
	if (UTLLRN_IKRigDefinition* SourceIKRig = Asset->GetIKRigWriteable(ETLLRN_RetargetSourceOrTarget::Source))
	{
		BoundIKRigs.Emplace(FTLLRN_BoundIKRig(SourceIKRig, *this));
	}

	if (UTLLRN_IKRigDefinition* TargetIKRig = Asset->GetIKRigWriteable(ETLLRN_RetargetSourceOrTarget::Target))
	{
		BoundIKRigs.Emplace(FTLLRN_BoundIKRig(TargetIKRig, *this));
	}
}

void FTLLRN_IKRetargetEditorController::HandleIKRigNeedsInitialized(UTLLRN_IKRigDefinition* ModifiedIKRig) const
{
	const UTLLRN_IKRetargeter* Retargeter = AssetController->GetAsset();
	check(Retargeter)
	HandleRetargeterNeedsInitialized();
}

void FTLLRN_IKRetargetEditorController::HandleRetargetChainAdded(UTLLRN_IKRigDefinition* ModifiedIKRig) const
{
	check(ModifiedIKRig)
	AssetController->HandleRetargetChainAdded(ModifiedIKRig);
	RefreshAllViews();
}

void FTLLRN_IKRetargetEditorController::HandleRetargetChainRenamed(UTLLRN_IKRigDefinition* ModifiedIKRig, FName OldName, FName NewName) const
{
	check(ModifiedIKRig)
	AssetController->HandleRetargetChainRenamed(ModifiedIKRig, OldName, NewName);
}

void FTLLRN_IKRetargetEditorController::HandleRetargetChainRemoved(UTLLRN_IKRigDefinition* ModifiedIKRig, const FName InChainRemoved) const
{
	check(ModifiedIKRig)
	AssetController->HandleRetargetChainRemoved(ModifiedIKRig, InChainRemoved);
	RefreshAllViews();
}

void FTLLRN_IKRetargetEditorController::HandleRetargeterNeedsInitialized() const
{
	// check for "zero height" retarget roots, and prompt user to fix
	FixZeroHeightRetargetRoot(ETLLRN_RetargetSourceOrTarget::Source);
	FixZeroHeightRetargetRoot(ETLLRN_RetargetSourceOrTarget::Target);
	
	ReinitializeRetargeterNoUIRefresh();
	
	// refresh all the UI views
	RefreshAllViews();	
}

void FTLLRN_IKRetargetEditorController::ReinitializeRetargeterNoUIRefresh() const
{
	// clear the output log
	ClearOutputLog();

	// force running instances to reinitialize on next tick
	AssetController->GetAsset()->IncrementVersion();
}

void FTLLRN_IKRetargetEditorController::HandleIKRigReplaced(ETLLRN_RetargetSourceOrTarget SourceOrTarget)
{
	BindToIKRigAssets();
}

void FTLLRN_IKRetargetEditorController::HandlePreviewMeshReplaced(ETLLRN_RetargetSourceOrTarget SourceOrTarget)
{
	// pause playback so we can resume after mesh swapped out
	PlaybackManager->PausePlayback();
	
	// set the source and target skeletal meshes on the component
	// NOTE: this must be done AFTER setting the AnimInstance so that the correct root anim node is loaded
	USkeletalMesh* SourceMesh = GetSkeletalMesh(ETLLRN_RetargetSourceOrTarget::Source);
	USkeletalMesh* TargetMesh = GetSkeletalMesh(ETLLRN_RetargetSourceOrTarget::Target);
	SourceSkelMeshComponent->SetSkeletalMesh(SourceMesh);
	TargetSkelMeshComponent->SetSkeletalMesh(TargetMesh);

	// clean bone selections in case of incompatible indices
	CleanSelection(ETLLRN_RetargetSourceOrTarget::Source);
	CleanSelection(ETLLRN_RetargetSourceOrTarget::Target);

	// apply mesh to the preview scene
	TSharedRef<IPersonaPreviewScene> PreviewScene = Editor.Pin()->GetPersonaToolkit()->GetPreviewScene();
	if (PreviewScene->GetPreviewMesh() != SourceMesh)
	{
		PreviewScene->SetPreviewMeshComponent(SourceSkelMeshComponent);
		PreviewScene->SetPreviewMesh(SourceMesh);
		SourceSkelMeshComponent->bCanHighlightSelectedSections = false;
	}

	// re-initializes the anim instances running in the viewport
	if (SourceAnimInstance)
	{
		Editor.Pin()->SetupAnimInstance();	
	}

	// continue playing where we left off
	PlaybackManager->ResumePlayback();
}

UDebugSkelMeshComponent* FTLLRN_IKRetargetEditorController::GetSkeletalMeshComponent(
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	return SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? SourceSkelMeshComponent : TargetSkelMeshComponent;
}

UTLLRN_IKRetargetAnimInstance* FTLLRN_IKRetargetEditorController::GetAnimInstance(
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	return SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? SourceAnimInstance.Get() : TargetAnimInstance.Get();
}

void FTLLRN_IKRetargetEditorController::UpdateMeshOffset(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	const UTLLRN_IKRetargeter* Asset = AssetController->GetAsset();
	if (!Asset)
	{
		return;
	}

	const bool bIsSource  = SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source;
	USceneComponent* Component = bIsSource ? SourceRootComponent : GetSkeletalMeshComponent(ETLLRN_RetargetSourceOrTarget::Target);
    if (!Component)
    {
    	return;
    }
	
	const FVector Position = bIsSource ? Asset->SourceMeshOffset : Asset->TargetMeshOffset;
	const float Scale = bIsSource ? 1.0f : Asset->TargetMeshScale;
	constexpr bool bSweep = false;
	constexpr FHitResult* OutSweepHitResult = nullptr;
	constexpr ETeleportType Teleport = ETeleportType::ResetPhysics;
	Component->SetWorldLocation(Position, bSweep, OutSweepHitResult, Teleport);
	Component->SetWorldScale3D(FVector(Scale,Scale,Scale));
}

bool FTLLRN_IKRetargetEditorController::GetCameraTargetForSelection(FSphere& OutTarget) const
{
	// center the view on the last selected item
	switch (GetLastSelectedItemType())
	{
	case ETLLRN_RetargetSelectionType::BONE:
		{
			// target the selected bones
			const TArray<FName>& SelectedBones = GetSelectedBones();
			if (SelectedBones.IsEmpty())
			{
				return false;
			}

			TArray<FVector> TargetPoints;
			const UDebugSkelMeshComponent* SkeletalMeshComponent = GetSkeletalMeshComponent(GetSourceOrTarget());
			const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetReferenceSkeleton();
			TArray<int32> ChildrenIndices;
			for (const FName& SelectedBoneName : SelectedBones)
			{
				const int32 BoneIndex = RefSkeleton.FindBoneIndex(SelectedBoneName);
				if (BoneIndex == INDEX_NONE)
				{
					continue;
				}

				TargetPoints.Add(SkeletalMeshComponent->GetBoneTransform(BoneIndex).GetLocation());
				ChildrenIndices.Reset();
				RefSkeleton.GetDirectChildBones(BoneIndex, ChildrenIndices);
				for (const int32 ChildIndex : ChildrenIndices)
				{
					TargetPoints.Add(SkeletalMeshComponent->GetBoneTransform(ChildIndex).GetLocation());
				}
			}
	
			// create a sphere that contains all the target points
			if (TargetPoints.Num() == 0)
			{
				TargetPoints.Add(FVector::ZeroVector);
			}
			OutTarget = FSphere(&TargetPoints[0], TargetPoints.Num());
			return true;
		}
		
	case ETLLRN_RetargetSelectionType::CHAIN:
		{
			const ETLLRN_RetargetSourceOrTarget SourceOrTarget = GetSourceOrTarget();
			const UTLLRN_IKRigDefinition* IKRig = AssetController->GetIKRig(SourceOrTarget);
			if (!IKRig)
			{
				return false;
			}

			const UDebugSkelMeshComponent* SkeletalMeshComponent = GetSkeletalMeshComponent(GetSourceOrTarget());
			const FReferenceSkeleton& RefSkeleton = SkeletalMeshComponent->GetReferenceSkeleton();

			// get target points from start/end bone of all selected chains on the currently active skeleton (source or target)
			TArray<FVector> TargetPoints;
			const TArray<FName>& SelectedChainNames = GetSelectedChains();
			for (const FName SelectedChainName : SelectedChainNames)
			{
				const FName SourceChain = AssetController->GetSourceChain(SelectedChainName);
				if (SourceChain == NAME_None)
				{
					continue;
				}

				const FName& ChainName = SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Target ? SelectedChainName : SourceChain;
				if (ChainName == NAME_None)
				{
					continue;
				}

				const UTLLRN_IKRigController* RigController = UTLLRN_IKRigController::GetController(IKRig);
				const FTLLRN_BoneChain* BoneChain = RigController->GetRetargetChainByName(ChainName);
				if (!BoneChain)
				{
					continue;
				}

				const int32 StartBoneIndex = RefSkeleton.FindBoneIndex(BoneChain->StartBone.BoneName);
				if (StartBoneIndex != INDEX_NONE)
				{
					TargetPoints.Add(SkeletalMeshComponent->GetBoneTransform(StartBoneIndex).GetLocation());
				}

				const int32 EndBoneIndex = RefSkeleton.FindBoneIndex(BoneChain->EndBone.BoneName);
				if (EndBoneIndex != INDEX_NONE)
				{
					TargetPoints.Add(SkeletalMeshComponent->GetBoneTransform(EndBoneIndex).GetLocation());
				}
			}

			// create a sphere that contains all the target points
			if (TargetPoints.Num() == 0)
			{
				TargetPoints.Add(FVector::ZeroVector);
			}
			OutTarget = FSphere(&TargetPoints[0], TargetPoints.Num());
			return true;	
		}
	case ETLLRN_RetargetSelectionType::ROOT:
	case ETLLRN_RetargetSelectionType::MESH:
	case ETLLRN_RetargetSelectionType::NONE:
	default:
		// frame both meshes
		OutTarget = FSphere(0);
		if (const UPrimitiveComponent* SourceComponent = GetSkeletalMeshComponent(ETLLRN_RetargetSourceOrTarget::Source))
		{
			OutTarget += SourceComponent->Bounds.GetSphere();
		}
		if (const UPrimitiveComponent* TargetComponent = GetSkeletalMeshComponent(ETLLRN_RetargetSourceOrTarget::Target))
		{
			OutTarget += TargetComponent->Bounds.GetSphere();
		}
		return true;
	}
}

bool FTLLRN_IKRetargetEditorController::IsEditingPoseWithAnyBoneSelected() const
{
	return IsEditingPose() && !GetSelectedBones().IsEmpty();
}

bool FTLLRN_IKRetargetEditorController::IsBoneRetargeted(const FName& BoneName, ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	// get an initialized processor
	const UTLLRN_IKRetargetProcessor* Processor = GetRetargetProcessor();
	if (!(Processor && Processor->IsInitialized()))
	{
		return false;
	}

	// return if it's a retargeted bone
	return Processor->IsBoneRetargeted(BoneName, SourceOrTarget);
}

FName FTLLRN_IKRetargetEditorController::GetChainNameFromBone(const FName& BoneName, ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	// get an initialized processor
	const UTLLRN_IKRetargetProcessor* Processor = GetRetargetProcessor();
	if (!(Processor && Processor->IsInitialized()))
	{
		return NAME_None;
	}
	
	return Processor->GetChainNameForBone(BoneName, SourceOrTarget);
}

TObjectPtr<UTLLRN_IKRetargetBoneDetails> FTLLRN_IKRetargetEditorController::GetOrCreateBoneDetailsObject(const FName& BoneName)
{
	if (AllBoneDetails.Contains(BoneName))
	{
		return AllBoneDetails[BoneName];
	}

	// create and store a new one
	UTLLRN_IKRetargetBoneDetails* NewBoneDetails = NewObject<UTLLRN_IKRetargetBoneDetails>(AssetController->GetAsset(), FName(BoneName), RF_Transient );
	NewBoneDetails->SelectedBone = BoneName;
	NewBoneDetails->EditorController = SharedThis(this);

	// store it in the map
	AllBoneDetails.Add(BoneName, NewBoneDetails);
	
	return NewBoneDetails;
}

USkeletalMesh* FTLLRN_IKRetargetEditorController::GetSkeletalMesh(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	return AssetController ? AssetController->GetPreviewMesh(SourceOrTarget) : nullptr;
}

const USkeleton* FTLLRN_IKRetargetEditorController::GetSkeleton(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	if (const USkeletalMesh* Mesh = GetSkeletalMesh(SourceOrTarget))
	{
		return Mesh->GetSkeleton();
	}
	
	return nullptr;
}

UDebugSkelMeshComponent* FTLLRN_IKRetargetEditorController::GetEditedSkeletalMesh() const
{
	return CurrentlyEditingSourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? SourceSkelMeshComponent : TargetSkelMeshComponent;
}

const FTLLRN_RetargetSkeleton& FTLLRN_IKRetargetEditorController::GetCurrentlyEditedSkeleton(const UTLLRN_IKRetargetProcessor& Processor) const
{
	return Processor.GetSkeleton(CurrentlyEditingSourceOrTarget);
}

FTransform FTLLRN_IKRetargetEditorController::GetGlobalRetargetPoseOfBone(
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
	const int32& BoneIndex,
	const float& Scale,
	const FVector& Offset) const
{
	const UTLLRN_IKRetargetAnimInstance* AnimInstance = GetAnimInstance(SourceOrTarget);
	if (!AnimInstance)
	{
		return FTransform::Identity;
	}
	
	const TArray<FTransform>& GlobalRetargetPose = AnimInstance->GetGlobalRetargetPose();
	if (!GlobalRetargetPose.IsValidIndex(BoneIndex))
	{
		return FTransform::Identity;
	}
	
	// get transform of bone
	FTransform BoneTransform = GlobalRetargetPose[BoneIndex];

	// scale and offset
	BoneTransform.ScaleTranslation(Scale);
	BoneTransform.AddToTranslation(Offset);
	BoneTransform.NormalizeRotation();

	return BoneTransform;
}

void FTLLRN_IKRetargetEditorController::GetGlobalRetargetPoseOfImmediateChildren(
	const FTLLRN_RetargetSkeleton& RetargetSkeleton,
	const int32& BoneIndex,
	const float& Scale,
	const FVector& Offset,
	TArray<int32>& OutChildrenIndices,
	TArray<FVector>& OutChildrenPositions)
{
	OutChildrenIndices.Reset();
	OutChildrenPositions.Reset();
	
	check(RetargetSkeleton.BoneNames.IsValidIndex(BoneIndex))

	// get indices of immediate children
	RetargetSkeleton.GetChildrenIndices(BoneIndex, OutChildrenIndices);

	// get the positions of the immediate children
	for (const int32& ChildIndex : OutChildrenIndices)
	{
		OutChildrenPositions.Emplace(RetargetSkeleton.RetargetGlobalPose[ChildIndex].GetTranslation());
	}

	// apply scale and offset to positions
	for (FVector& ChildPosition : OutChildrenPositions)
	{
		ChildPosition *= Scale;
		ChildPosition += Offset;
	}
}

UTLLRN_IKRetargetProcessor* FTLLRN_IKRetargetEditorController::GetRetargetProcessor() const
{	
	if(UTLLRN_IKRetargetAnimInstance* AnimInstance = TargetAnimInstance.Get())
	{
		return AnimInstance->GetRetargetProcessor();
	}

	return nullptr;	
}

void FTLLRN_IKRetargetEditorController::ResetIKPlantingState() const
{
	if (UTLLRN_IKRetargetProcessor* Processor = GetRetargetProcessor())
	{
		Processor->ResetPlanting();
	}
}

void FTLLRN_IKRetargetEditorController::ClearOutputLog() const
{
	if (OutputLogView.IsValid())
	{
		OutputLogView.Get()->ClearLog();
		if (const UTLLRN_IKRetargetProcessor* Processor = GetRetargetProcessor())
		{
			Processor->Log.Clear();
		}
	}
}

bool FTLLRN_IKRetargetEditorController::IsObjectInDetailsView(const UObject* Object)
{
	if (!DetailsView.IsValid())
	{
		return false;
	}

	if (!Object)
	{
		return false;
	}
	
	TArray<TWeakObjectPtr<UObject>> SelectedObjects = DetailsView->GetSelectedObjects();
	for (TWeakObjectPtr<UObject> SelectedObject : SelectedObjects)
	{
		if (SelectedObject.Get() == Object)
		{
			return true;
		}
	}

	return false;
}

void FTLLRN_IKRetargetEditorController::RefreshAllViews() const
{
	Editor.Pin()->RegenerateMenusAndToolbars();
	RefreshDetailsView();
	RefreshChainsView();
	RefreshAssetBrowserView();
	RefreshHierarchyView();
	RefreshOpStackView();
}

void FTLLRN_IKRetargetEditorController::RefreshDetailsView() const
{
	// refresh the details panel, cannot assume tab is not closed
	if (DetailsView.IsValid())
	{
		DetailsView->ForceRefresh();
	}
}

void FTLLRN_IKRetargetEditorController::RefreshChainsView() const
{
	// refesh chains view, cannot assume tab is not closed
	if (ChainsView.IsValid())
	{
		ChainsView.Get()->RefreshView();
	}
}

void FTLLRN_IKRetargetEditorController::RefreshAssetBrowserView() const
{
	// refresh the asset browser to ensure it shows compatible sequences
	if (AssetBrowserView.IsValid())
	{
		AssetBrowserView.Get()->RefreshView();
	}
}

void FTLLRN_IKRetargetEditorController::RefreshHierarchyView() const
{
	if (HierarchyView.IsValid())
	{
		HierarchyView.Get()->RefreshTreeView();
	}
}

void FTLLRN_IKRetargetEditorController::RefreshOpStackView() const
{
	if (OpStackView.IsValid())
	{
		OpStackView->RefreshStackView();
	}
}

void FTLLRN_IKRetargetEditorController::RefreshPoseList() const
{
	if (HierarchyView.IsValid())
	{
		HierarchyView.Get()->RefreshPoseList();
	}
}

void FTLLRN_IKRetargetEditorController::SetDetailsObject(UObject* DetailsObject) const
{
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(DetailsObject, true /*forceRefresh*/);
	}
}

void FTLLRN_IKRetargetEditorController::SetDetailsObjects(const TArray<UObject*>& DetailsObjects) const
{
	if (DetailsView.IsValid())
	{
		DetailsView->SetObjects(DetailsObjects);
	}
}

float FTLLRN_IKRetargetEditorController::GetRetargetPoseAmount() const
{
	return RetargetPosePreviewBlend;
}

void FTLLRN_IKRetargetEditorController::SetRetargetPoseAmount(float InValue)
{
	if (OutputMode==ETLLRN_RetargeterOutputMode::RunRetarget)
	{
		SetRetargeterMode(ETLLRN_RetargeterOutputMode::EditRetargetPose);
	}
	
	RetargetPosePreviewBlend = InValue;
	SourceAnimInstance->SetRetargetPoseBlend(RetargetPosePreviewBlend);
	TargetAnimInstance->SetRetargetPoseBlend(RetargetPosePreviewBlend);
}

void FTLLRN_IKRetargetEditorController::SetSourceOrTargetMode(ETLLRN_RetargetSourceOrTarget NewMode)
{
	// already in this mode, so do nothing
	if (NewMode == CurrentlyEditingSourceOrTarget)
	{
		return;
	}
	
	// store the new skeleton mode
	CurrentlyEditingSourceOrTarget = NewMode;

	// if we switch source/target while in edit mode we need to re-enter that mode
	if (GetRetargeterMode() == ETLLRN_RetargeterOutputMode::EditRetargetPose)
	{
		FEditorModeTools& EditorModeManager = Editor.Pin()->GetEditorModeManager();
		FTLLRN_IKRetargetEditPoseMode* EditMode = EditorModeManager.GetActiveModeTyped<FTLLRN_IKRetargetEditPoseMode>(FTLLRN_IKRetargetEditPoseMode::ModeName);
		if (EditMode)
		{
			// FTLLRN_IKRetargetEditPoseMode::Enter() is reentrant and written so we can switch between editing
			// source / target skeleton without having to enter/exit the mode; just call Enter() again
			EditMode->Enter();
		}
	}

	// make sure details panel updates with selected bone on OTHER skeleton
	if (LastSelectedItem == ETLLRN_RetargetSelectionType::BONE)
	{
		const TArray<FName>& SelectedBones = GetSelectedBones();
		EditBoneSelection(SelectedBones, ETLLRN_SelectionEdit::Replace);
	}
	
	RefreshAllViews();
	RefreshPoseList();
}

void FTLLRN_IKRetargetEditorController::EditBoneSelection(
	const TArray<FName>& InBoneNames,
	ETLLRN_SelectionEdit EditMode,
	const bool bFromHierarchyView)
{
	// must have a skeletal mesh
	UDebugSkelMeshComponent* DebugComponent = GetEditedSkeletalMesh();
	if (!DebugComponent->GetSkeletalMeshAsset())
	{
		return;
	}

	LastSelectedItem = ETLLRN_RetargetSelectionType::BONE;
	
	SetRootSelected(false);
	
	switch (EditMode)
	{
		case ETLLRN_SelectionEdit::Add:
		{
			for (const FName& BoneName : InBoneNames)
			{
				SelectedBoneNames[CurrentlyEditingSourceOrTarget].AddUnique(BoneName);
			}
			
			break;
		}
		case ETLLRN_SelectionEdit::Remove:
		{
			for (const FName& BoneName : InBoneNames)
			{
				SelectedBoneNames[CurrentlyEditingSourceOrTarget].Remove(BoneName);
			}
			break;
		}
		case ETLLRN_SelectionEdit::Replace:
		{
			SelectedBoneNames[CurrentlyEditingSourceOrTarget] = InBoneNames;
			break;
		}
		default:
			checkNoEntry();
	}

	// update hierarchy view
	if (!bFromHierarchyView)
	{
		RefreshHierarchyView();
	}
	else
	{
		// If selection was made from the hierarchy view, the viewport must be invalidated for the
		// new widget hit proxies to be activated. Otherwise user has to click in the viewport first to gain focus.
		Editor.Pin()->GetPersonaToolkit()->GetPreviewScene()->InvalidateViews();
	}

	// update details
	if (SelectedBoneNames[CurrentlyEditingSourceOrTarget].IsEmpty())
	{
		SetDetailsObject(AssetController->GetAsset());
	}
	else
	{
		TArray<UObject*> SelectedBoneDetails;
		for (const FName& SelectedBone : SelectedBoneNames[CurrentlyEditingSourceOrTarget])
		{
			TObjectPtr<UTLLRN_IKRetargetBoneDetails> BoneDetails = GetOrCreateBoneDetailsObject(SelectedBone);
			SelectedBoneDetails.Add(BoneDetails);
		}
		SetDetailsObjects(SelectedBoneDetails);
	}
}

void FTLLRN_IKRetargetEditorController::EditChainSelection(
	const TArray<FName>& InChainNames,
	ETLLRN_SelectionEdit EditMode,
	const bool bFromChainsView)
{
	// deselect others
	SetRootSelected(false);

	LastSelectedItem = ETLLRN_RetargetSelectionType::CHAIN;
	
	// update selection set based on edit mode
	switch (EditMode)
	{
	case ETLLRN_SelectionEdit::Add:
		{
			for (const FName& ChainName : InChainNames)
			{
				SelectedChains.AddUnique(ChainName);
			}
			
			break;
		}
	case ETLLRN_SelectionEdit::Remove:
		{
			for (const FName& ChainName : InChainNames)
			{
				SelectedChains.Remove(ChainName);
			}
			break;
		}
	case ETLLRN_SelectionEdit::Replace:
		{
			SelectedChains = InChainNames;
			break;
		}
	default:
		checkNoEntry();
	}

	// update chains view with selected chains
	if (ChainsView)
	{
		if (!bFromChainsView)
		{
			ChainsView->RefreshView();
		}
	}

	// get selected chain UObjects to show in details view
	TArray<UObject*> SelectedChainSettings;
	const TArray<TObjectPtr<UTLLRN_RetargetChainSettings>>& AllChainSettings = AssetController->GetAsset()->GetAllChainSettings();
	for (const TObjectPtr<UTLLRN_RetargetChainSettings>& ChainSettings : AllChainSettings)
	{
		if (ChainSettings->EditorController.Pin().Get() != this)
		{
			ChainSettings->EditorController = SharedThis(this);	
		}
		
		if (SelectedChains.Contains(ChainSettings.Get()->TargetChain))
		{
			SelectedChainSettings.Add(ChainSettings);
		}
	}

	// no chains selected, then show asset settings in the details view
	SelectedChainSettings.IsEmpty() ? SetDetailsObject(AssetController->GetAsset()) : SetDetailsObjects(SelectedChainSettings);
}

void FTLLRN_IKRetargetEditorController::SetRootSelected(const bool bIsSelected)
{
	bIsRootSelected = bIsSelected;
	if (!bIsSelected)
	{
		return;
	}

	LastSelectedItem = ETLLRN_RetargetSelectionType::ROOT;
	ShowRootSettings();
}

void FTLLRN_IKRetargetEditorController::CleanSelection(ETLLRN_RetargetSourceOrTarget SourceOrTarget)
{
	USkeletalMesh* SkeletalMesh = GetSkeletalMesh(SourceOrTarget);
	if (!SkeletalMesh)
	{
		SelectedBoneNames[SourceOrTarget].Empty();
		return;
	}

	TArray<FName> CleanedSelection;
	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	for (const FName& SelectedBone : SelectedBoneNames[SourceOrTarget])
	{
		if (RefSkeleton.FindBoneIndex(SelectedBone))
		{
			CleanedSelection.Add(SelectedBone);
		}
	}

	SelectedBoneNames[SourceOrTarget] = CleanedSelection;
}

void FTLLRN_IKRetargetEditorController::ClearSelection(const bool bKeepBoneSelection)
{
	// clear root and mesh selection
	SetRootSelected(false);
	
	// deselect all chains
	if (ChainsView.IsValid())
	{
		ChainsView->ClearSelection();
		SelectedChains.Empty();
	}

	// clear bone selection
	if (!bKeepBoneSelection)
	{
		SetRootSelected(false);
		SelectedBoneNames[ETLLRN_RetargetSourceOrTarget::Source].Reset();
		SelectedBoneNames[ETLLRN_RetargetSourceOrTarget::Target].Reset();
	}

	LastSelectedItem = ETLLRN_RetargetSelectionType::NONE;

	RefreshDetailsView();
}

UTLLRN_RetargetOpBase* FTLLRN_IKRetargetEditorController::GetSelectedOp() const
{
	if (!OpStackView.IsValid())
	{
		return nullptr;
	}

	const int32 SelectedOpIndex = OpStackView->GetSelectedItemIndex();
	return AssetController->GetRetargetOpAtIndex(SelectedOpIndex);
}

void FTLLRN_IKRetargetEditorController::SetRetargeterMode(ETLLRN_RetargeterOutputMode Mode)
{
	if (OutputMode == Mode)
	{
		return;
	}
		
	PreviousMode = OutputMode;
	
	FEditorModeTools& EditorModeManager = Editor.Pin()->GetEditorModeManager();
	
	switch (Mode)
	{
		case ETLLRN_RetargeterOutputMode::EditRetargetPose:
			// enter edit mode
			EditorModeManager.DeactivateMode(FTLLRN_IKRetargetDefaultMode::ModeName);
			EditorModeManager.ActivateMode(FTLLRN_IKRetargetEditPoseMode::ModeName);
			OutputMode = ETLLRN_RetargeterOutputMode::EditRetargetPose;
			SourceAnimInstance->SetRetargetMode(ETLLRN_RetargeterOutputMode::EditRetargetPose);
			TargetAnimInstance->SetRetargetMode(ETLLRN_RetargeterOutputMode::EditRetargetPose);
			PlaybackManager->PausePlayback();
			SetRetargetPoseAmount(1.0f);
			break;
		
		case ETLLRN_RetargeterOutputMode::RunRetarget:
			EditorModeManager.DeactivateMode(FTLLRN_IKRetargetEditPoseMode::ModeName);
			EditorModeManager.ActivateMode(FTLLRN_IKRetargetDefaultMode::ModeName);
			OutputMode = ETLLRN_RetargeterOutputMode::RunRetarget;
			SourceAnimInstance->SetRetargetMode(ETLLRN_RetargeterOutputMode::RunRetarget);
			TargetAnimInstance->SetRetargetMode(ETLLRN_RetargeterOutputMode::RunRetarget);
			PlaybackManager->ResumePlayback();
			break;
		
		default:
			checkNoEntry();
	}

	// details view displays differently depending on output mode
	RefreshDetailsView();
}

FText FTLLRN_IKRetargetEditorController::GetRetargeterModeLabel()
{
	switch (GetRetargeterMode())
	{
	case ETLLRN_RetargeterOutputMode::RunRetarget:
		return FText::FromString("Running Retarget");
	case ETLLRN_RetargeterOutputMode::EditRetargetPose:
		return FText::FromString("Editing Retarget Pose");
	default:
		checkNoEntry();
		return FText::FromString("Unknown Mode.");
	}
}

FSlateIcon FTLLRN_IKRetargetEditorController::GetCurrentRetargetModeIcon() const
{
	return GetRetargeterModeIcon(GetRetargeterMode());
}

FSlateIcon FTLLRN_IKRetargetEditorController::GetRetargeterModeIcon(ETLLRN_RetargeterOutputMode Mode) const
{
	switch (Mode)
	{
	case ETLLRN_RetargeterOutputMode::RunRetarget:
		return FSlateIcon(FTLLRN_IKRetargetEditorStyle::Get().GetStyleSetName(), "TLLRN_IKRetarget.RunRetargeter");
	case ETLLRN_RetargeterOutputMode::EditRetargetPose:
		return FSlateIcon(FTLLRN_IKRetargetEditorStyle::Get().GetStyleSetName(), "TLLRN_IKRetarget.EditRetargetPose");
	default:
		checkNoEntry();
		return FSlateIcon(FTLLRN_IKRetargetEditorStyle::Get().GetStyleSetName(), "TLLRN_IKRetarget.ShowRetargetPose");
	}
}

bool FTLLRN_IKRetargetEditorController::IsReadyToRetarget() const
{
	return GetRetargetProcessor()->IsInitialized();
}

bool FTLLRN_IKRetargetEditorController::IsCurrentMeshLoaded() const
{
	return GetSkeletalMesh(GetSourceOrTarget()) != nullptr;
}

bool FTLLRN_IKRetargetEditorController::IsEditingPose() const
{
	return GetRetargeterMode() == ETLLRN_RetargeterOutputMode::EditRetargetPose;
}

FTLLRN_RetargetGlobalSettings& FTLLRN_IKRetargetEditorController::GetGlobalSettings() const
{
	return AssetController->GetAsset()->GetGlobalSettingsUObject()->Settings;
}

void FTLLRN_IKRetargetEditorController::HandleNewPose()
{
	SetRetargeterMode(ETLLRN_RetargeterOutputMode::EditRetargetPose);
	
	// get a unique pose name to use as suggestion
	const FString DefaultNewPoseName = LOCTEXT("NewRetargetPoseName", "CustomRetargetPose").ToString();
	const FName UniqueNewPoseName = AssetController->MakePoseNameUnique(DefaultNewPoseName, GetSourceOrTarget());
	
	SAssignNew(NewPoseWindow, SWindow)
	.Title(LOCTEXT("NewRetargetPoseOptions", "Create New Retarget Pose"))
	.ClientSize(FVector2D(300, 80))
	.HasCloseButton(true)
	.SupportsMinimize(false) .SupportsMaximize(false)
	[
		SNew(SBorder)
		.BorderImage( FAppStyle::GetBrush("Menu.Background") )
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(4)
			.AutoHeight()
			[
				SAssignNew(NewPoseEditableText, SEditableTextBox)
				.MinDesiredWidth(275)
				.Text(FText::FromName(UniqueNewPoseName))
			]

			+ SVerticalBox::Slot()
			.Padding(4)
			.HAlign(HAlign_Right)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4)
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.TextStyle( FAppStyle::Get(), "DialogButtonText" )
					.Text(LOCTEXT("OkButtonLabel", "Ok"))
					.OnClicked(this, &FTLLRN_IKRetargetEditorController::CreateNewPose)
				]
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4)
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.TextStyle( FAppStyle::Get(), "DialogButtonText" )
					.Text(LOCTEXT("CancelButtonLabel", "Cancel"))
					.OnClicked_Lambda( [this]()
					{
						NewPoseWindow->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]
		]
	];

	GEditor->EditorAddModalWindow(NewPoseWindow.ToSharedRef());
	NewPoseWindow.Reset();
}

bool FTLLRN_IKRetargetEditorController::CanCreatePose() const
{
	return !IsEditingPose();
}

FReply FTLLRN_IKRetargetEditorController::CreateNewPose() const
{
	const FName NewPoseName = FName(NewPoseEditableText.Get()->GetText().ToString());
	AssetController->CreateRetargetPose(NewPoseName, GetSourceOrTarget());
	NewPoseWindow->RequestDestroyWindow();
	RefreshPoseList();
	return FReply::Handled();
}

void FTLLRN_IKRetargetEditorController::HandleDuplicatePose()
{
	SetRetargeterMode(ETLLRN_RetargeterOutputMode::EditRetargetPose);
	
	// get a unique pose name to use as suggestion for duplicate
	const FString DuplicateSuffix = LOCTEXT("DuplicateSuffix", "_Copy").ToString();
	FString CurrentPoseName = GetCurrentPoseName().ToString();
	const FString DefaultDuplicatePoseName = CurrentPoseName.Append(*DuplicateSuffix);
	const FName UniqueNewPoseName = AssetController->MakePoseNameUnique(DefaultDuplicatePoseName, GetSourceOrTarget());
	
	SAssignNew(NewPoseWindow, SWindow)
	.Title(LOCTEXT("DuplicateRetargetPoseOptions", "Duplicate Retarget Pose"))
	.ClientSize(FVector2D(300, 80))
	.HasCloseButton(true)
	.SupportsMinimize(false) .SupportsMaximize(false)
	[
		SNew(SBorder)
		.BorderImage( FAppStyle::GetBrush("Menu.Background") )
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(4)
			.HAlign(HAlign_Right)
			.AutoHeight()
			[
				SAssignNew(NewPoseEditableText, SEditableTextBox)
				.MinDesiredWidth(275)
				.Text(FText::FromName(UniqueNewPoseName))
			]

			+ SVerticalBox::Slot()
			.Padding(4)
			.HAlign(HAlign_Right)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4)
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.TextStyle( FAppStyle::Get(), "DialogButtonText" )
					.Text(LOCTEXT("OkButtonLabel", "Ok"))
					.OnClicked(this, &FTLLRN_IKRetargetEditorController::CreateDuplicatePose)
				]
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4)
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.TextStyle( FAppStyle::Get(), "DialogButtonText" )
					.Text(LOCTEXT("CancelButtonLabel", "Cancel"))
					.OnClicked_Lambda( [this]()
					{
						NewPoseWindow->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]	
		]
	];

	GEditor->EditorAddModalWindow(NewPoseWindow.ToSharedRef());
	NewPoseWindow.Reset();
}

FReply FTLLRN_IKRetargetEditorController::CreateDuplicatePose() const
{
	const FName PoseToDuplicate = AssetController->GetCurrentRetargetPoseName(CurrentlyEditingSourceOrTarget);
	const FName NewPoseName = FName(NewPoseEditableText.Get()->GetText().ToString());
	AssetController->DuplicateRetargetPose(PoseToDuplicate, NewPoseName, GetSourceOrTarget());
	NewPoseWindow->RequestDestroyWindow();
	RefreshPoseList();
	return FReply::Handled();
}

void FTLLRN_IKRetargetEditorController::HandleDeletePose()
{
	SetRetargeterMode(ETLLRN_RetargeterOutputMode::EditRetargetPose);
	
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget = GetSourceOrTarget();
	const FName CurrentPose = AssetController->GetCurrentRetargetPoseName(SourceOrTarget);
	AssetController->RemoveRetargetPose(CurrentPose, SourceOrTarget);
	RefreshPoseList();
}

bool FTLLRN_IKRetargetEditorController::CanDeletePose() const
{	
	// cannot delete default pose
	return AssetController->GetCurrentRetargetPoseName(GetSourceOrTarget()) != UTLLRN_IKRetargeter::GetDefaultPoseName();
}

void FTLLRN_IKRetargetEditorController::HandleResetAllBones() const
{
	const FName CurrentPose = AssetController->GetCurrentRetargetPoseName(CurrentlyEditingSourceOrTarget);
	static TArray<FName> Empty; // empty list will reset all bones
	AssetController->ResetRetargetPose(CurrentPose, Empty, GetSourceOrTarget());
}

void FTLLRN_IKRetargetEditorController::HandleResetSelectedBones() const
{
	const FName CurrentPose = AssetController->GetCurrentRetargetPoseName(CurrentlyEditingSourceOrTarget);
	AssetController->ResetRetargetPose(CurrentPose, GetSelectedBones(), CurrentlyEditingSourceOrTarget);
}

void FTLLRN_IKRetargetEditorController::HandleResetSelectedAndChildrenBones() const
{
	// get all selected bones and their children (recursive)
	const TArray<FName> BonesToReset = GetSelectedBonesAndChildren();
	
	// reset the bones in the current pose
	const FName CurrentPose = AssetController->GetCurrentRetargetPoseName(CurrentlyEditingSourceOrTarget);
	AssetController->ResetRetargetPose(CurrentPose, BonesToReset, GetSourceOrTarget());
}

void FTLLRN_IKRetargetEditorController::HandleAlignAllBones() const
{
	AssetController->AutoAlignAllBones(GetSourceOrTarget());
}

void FTLLRN_IKRetargetEditorController::HandleAlignSelectedBones(const ETLLRN_RetargetAutoAlignMethod Method, const bool bIncludeChildren) const
{
	const TArray<FName> BonesToAlign = bIncludeChildren ? GetSelectedBonesAndChildren() : GetSelectedBones();
	AssetController->AutoAlignBones(BonesToAlign, Method, GetSourceOrTarget());
}

void FTLLRN_IKRetargetEditorController::HandleSnapToGround() const
{
	const TArray<FName> SelectedBones = GetSelectedBones();
	const FName FirstSelectedBone = SelectedBones.IsEmpty() ? NAME_None : SelectedBones[0];
	AssetController->SnapBoneToGround(FirstSelectedBone, GetSourceOrTarget());
}

void FTLLRN_IKRetargetEditorController::HandleRenamePose()
{
	SAssignNew(RenamePoseWindow, SWindow)
	.Title(LOCTEXT("RenameRetargetPoseOptions", "Rename Retarget Pose"))
	.ClientSize(FVector2D(250, 80))
	.HasCloseButton(true)
	.SupportsMinimize(false) .SupportsMaximize(false)
	[
		SNew(SBorder)
		.BorderImage( FAppStyle::GetBrush("Menu.Background") )
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.Padding(4)
			.AutoHeight()
			[
				SAssignNew(NewNameEditableText, SEditableTextBox)
				.Text(GetCurrentPoseName())
			]

			+ SVerticalBox::Slot()
			.Padding(4)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.TextStyle( FAppStyle::Get(), "DialogButtonText" )
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Text(LOCTEXT("OkButtonLabel", "Ok"))
					.IsEnabled_Lambda([this]()
					{
						return !GetCurrentPoseName().EqualTo(NewNameEditableText.Get()->GetText());
					})
					.OnClicked(this, &FTLLRN_IKRetargetEditorController::RenamePose)
				]
				
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.TextStyle( FAppStyle::Get(), "DialogButtonText" )
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Text(LOCTEXT("CancelButtonLabel", "Cancel"))
					.OnClicked_Lambda( [this]()
					{
						RenamePoseWindow->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]	
		]
	];

	GEditor->EditorAddModalWindow(RenamePoseWindow.ToSharedRef());
	RenamePoseWindow.Reset();
}

FReply FTLLRN_IKRetargetEditorController::RenamePose() const
{
	const FName NewPoseName = FName(NewNameEditableText.Get()->GetText().ToString());
	RenamePoseWindow->RequestDestroyWindow();

	const FName CurrentPoseName = AssetController->GetCurrentRetargetPoseName(GetSourceOrTarget());
	AssetController->RenameRetargetPose(CurrentPoseName, NewPoseName, GetSourceOrTarget());
	RefreshPoseList();
	return FReply::Handled();
}

bool FTLLRN_IKRetargetEditorController::CanRenamePose() const
{
	// cannot rename default pose
	const bool bNotUsingDefaultPose = AssetController->GetCurrentRetargetPoseName(GetSourceOrTarget()) != UTLLRN_IKRetargeter::GetDefaultPoseName();
	// cannot rename pose while editing
	return bNotUsingDefaultPose && !IsEditingPose();
}

void FTLLRN_IKRetargetEditorController::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (TTuple<FName, TObjectPtr<UTLLRN_IKRetargetBoneDetails>> Pair : AllBoneDetails)
	{
		Collector.AddReferencedObject(Pair.Value);
	}

	Collector.AddReferencedObject(SourceAnimInstance);
	Collector.AddReferencedObject(TargetAnimInstance);
}

void FTLLRN_IKRetargetEditorController::RenderSkeleton(FPrimitiveDrawInterface* PDI, ETLLRN_RetargetSourceOrTarget InSourceOrTarget) const
{
	const UDebugSkelMeshComponent* MeshComponent = GetSkeletalMeshComponent(InSourceOrTarget);
	const FTransform ComponentTransform = MeshComponent->GetComponentTransform();
	const FReferenceSkeleton& RefSkeleton = MeshComponent->GetReferenceSkeleton();
	const int32 NumBones = RefSkeleton.GetNum();

	// get world transforms of bones
	TArray<FBoneIndexType> RequiredBones;
	RequiredBones.AddUninitialized(NumBones);
	TArray<FTransform> WorldTransforms;
	WorldTransforms.AddUninitialized(NumBones);
	for (int32 Index=0; Index<NumBones; ++Index)
	{
		RequiredBones[Index] = Index;
		WorldTransforms[Index] = MeshComponent->GetBoneTransform(Index, ComponentTransform);
	}
	
	const UTLLRN_IKRetargeter* Asset = AssetController->GetAsset();
	const float BoneDrawSize = Asset->BoneDrawSize;
	const float MaxDrawRadius = MeshComponent->Bounds.SphereRadius * 0.01f;
	const float BoneRadius = FMath::Min(1.0f, MaxDrawRadius) * BoneDrawSize;
	const bool bIsSelectable = InSourceOrTarget == GetSourceOrTarget();
	const FLinearColor DefaultColor = bIsSelectable ? GetMutableDefault<UPersonaOptions>()->DefaultBoneColor : GetMutableDefault<UPersonaOptions>()->DisabledBoneColor;
	
	UPersonaOptions* ConfigOption = UPersonaOptions::StaticClass()->GetDefaultObject<UPersonaOptions>();
	
	FSkelDebugDrawConfig DrawConfig;
	DrawConfig.BoneDrawMode = (EBoneDrawMode::Type)ConfigOption->DefaultBoneDrawSelection;
	DrawConfig.BoneDrawSize = BoneRadius;
	DrawConfig.bAddHitProxy = bIsSelectable;
	DrawConfig.bForceDraw = false;
	DrawConfig.DefaultBoneColor = DefaultColor;
	DrawConfig.AffectedBoneColor = GetMutableDefault<UPersonaOptions>()->AffectedBoneColor;
	DrawConfig.SelectedBoneColor = GetMutableDefault<UPersonaOptions>()->SelectedBoneColor;
	DrawConfig.ParentOfSelectedBoneColor = GetMutableDefault<UPersonaOptions>()->ParentOfSelectedBoneColor;

	TArray<TRefCountPtr<HHitProxy>> HitProxies;
	TArray<int32> SelectedBones;
	
	// create hit proxies and selection set only for the currently active skeleton
	if (bIsSelectable)
	{
		HitProxies.Reserve(NumBones);
		for (int32 Index = 0; Index < NumBones; ++Index)
		{
			HitProxies.Add(new HTLLRN_IKRetargetEditorBoneProxy(RefSkeleton.GetBoneName(Index), Index, InSourceOrTarget));
		}

		// record selected bone indices
		for (const FName& SelectedBoneName : SelectedBoneNames[InSourceOrTarget])
		{
			int32 SelectedBoneIndex = RefSkeleton.FindBoneIndex(SelectedBoneName);
			SelectedBones.Add(SelectedBoneIndex);
		}
	}

	// generate bone colors, blue on selected chains
	TArray<FLinearColor> BoneColors;
	{
		// set default colors
		if (GetDefault<UPersonaOptions>()->bShowBoneColors)
		{
			SkeletalDebugRendering::FillWithMultiColors(BoneColors, RefSkeleton.GetNum());
		}
		else
		{
			BoneColors.Init(DefaultColor, RefSkeleton.GetNum());
		}

		// highlight selected chains in blue
		const TArray<FName>& SelectedChainNames = GetSelectedChains();
		const FLinearColor HighlightedColor = FLinearColor::Blue;
		const bool bIsSource = InSourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source;
		if (const UTLLRN_IKRetargetProcessor* Processor = GetRetargetProcessor())
		{
			const TArray<FTLLRN_RetargetChainPairFK>& FKChains = Processor->GetFKChainPairs();
			for (const FTLLRN_RetargetChainPairFK& FKChain : FKChains)
			{
				if (SelectedChainNames.Contains(FKChain.TargetBoneChainName))
				{
					const TArray<int32>& BoneIndices = bIsSource ? FKChain.SourceBoneIndices : FKChain.TargetBoneIndices;
					for (const int32& BoneIndex : BoneIndices)
					{
						BoneColors[BoneIndex] = HighlightedColor;
					}	
				}
			}
		}
	}

	SkeletalDebugRendering::DrawBones(
		PDI,
		ComponentTransform.GetLocation(),
		RequiredBones,
		RefSkeleton,
		WorldTransforms,
		SelectedBones,
		BoneColors,
		HitProxies,
		DrawConfig
	);
}

TArray<FName> FTLLRN_IKRetargetEditorController::GetSelectedBonesAndChildren() const
{
	// get the reference skeleton we're operating on
	const USkeletalMesh* SkeletalMesh = GetSkeletalMesh(GetSourceOrTarget());
	if (!SkeletalMesh)
	{
		return {};
	}
	const FReferenceSkeleton RefSkeleton = SkeletalMesh->GetRefSkeleton();

	// get list of all children of selected bones
	TArray<int32> AllChildrenIndices;
	for (const FName& SelectedBone : SelectedBoneNames[CurrentlyEditingSourceOrTarget])
	{
		const int32 SelectedBoneIndex = RefSkeleton.FindBoneIndex(SelectedBone);
		AllChildrenIndices.Add(SelectedBoneIndex);
		
		for (int32 ChildIndex = 0; ChildIndex < RefSkeleton.GetNum(); ++ChildIndex)
		{
			const int32 ParentIndex = RefSkeleton.GetParentIndex(ChildIndex);
			if (ParentIndex != INDEX_NONE && AllChildrenIndices.Contains(ParentIndex))
			{
				AllChildrenIndices.Add(ChildIndex);
			}
		}
	}

	// merge total list of all selected bones and their children
	TArray<FName> BonesToReturn = SelectedBoneNames[CurrentlyEditingSourceOrTarget];
	for (const int32 ChildIndex : AllChildrenIndices)
	{
		BonesToReturn.AddUnique(RefSkeleton.GetBoneName(ChildIndex));
	}

	return BonesToReturn;
}

void FTLLRN_IKRetargetEditorController::FixZeroHeightRetargetRoot(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	// is there a mesh to check?
	USkeletalMesh* SkeletalMesh = GetSkeletalMesh(SourceOrTarget);
	if (!SkeletalMesh)
	{
		return;
	}

	// have we already nagged the user about fixing this mesh?
	if (AssetController->GetAskedToFixRootHeightForMesh(SkeletalMesh))
	{
		return;
	}

	const FName CurrentRetargetPoseName = AssetController->GetCurrentRetargetPoseName(SourceOrTarget);
	FTLLRN_IKRetargetPose& CurrentRetargetPose = AssetController->GetCurrentRetargetPose(SourceOrTarget);
	const FName RetargetRootBoneName = AssetController->GetRetargetRootBone(SourceOrTarget);
	if (RetargetRootBoneName == NAME_None)
	{
		return;
	}

	FTLLRN_RetargetSkeleton DummySkeleton;
	DummySkeleton.Initialize(
		SkeletalMesh,
		TArray<FTLLRN_BoneChain>(),
		CurrentRetargetPoseName,
		&CurrentRetargetPose,
		RetargetRootBoneName);

	const int32 RootBoneIndex = DummySkeleton.FindBoneIndexByName(RetargetRootBoneName);
	if (RootBoneIndex == INDEX_NONE)
	{
		return;
	}

	const FTransform& RootTransform = DummySkeleton.RetargetGlobalPose[RootBoneIndex];
	if (RootTransform.GetLocation().Z < 1.0f)
	{
		if (PromptToFixRootHeight(SourceOrTarget))
		{
			// move it up based on the height of the mesh
			const float FixedHeight = FMath::Abs(SkeletalMesh->GetBounds().GetBoxExtrema(-1).Z);
			// update the current retarget pose
			CurrentRetargetPose.SetRootTranslationDelta(FVector(0.f, 0.f,FixedHeight));
		}
	}

	AssetController->SetAskedToFixRootHeightForMesh(SkeletalMesh, true);
}

bool FTLLRN_IKRetargetEditorController::PromptToFixRootHeight(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	const FText SourceOrTargetText = SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? FText::FromString("Source") : FText::FromString("Target");

	TSharedRef<SCustomDialog> Dialog = SNew(SCustomDialog)
		.Title(FText(LOCTEXT("FixRootHeightTitle", "Add Height to Retarget Root Pose")))
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("FixRootHeightLabel", "The {0} skeleton has a retarget root bone on the ground. Apply a vertical offset to root bone in the current retarget pose?"), SourceOrTargetText))
		]
		.Buttons({
			SCustomDialog::FButton(LOCTEXT("ApplyOffset", "Apply Offset")),
			SCustomDialog::FButton(LOCTEXT("No", "No"))
	});

	if (Dialog->ShowModal() != 0)
	{
		return false; // cancel button pressed, or window closed
	}

	return true;
}

FText FTLLRN_IKRetargetEditorController::GetCurrentPoseName() const
{
	return FText::FromName(AssetController->GetCurrentRetargetPoseName(GetSourceOrTarget()));
}

void FTLLRN_IKRetargetEditorController::OnPoseSelected(TSharedPtr<FName> InPose, ESelectInfo::Type SelectInfo) const
{
	if (InPose.IsValid())
	{
		AssetController->SetCurrentRetargetPose(*InPose.Get(), GetSourceOrTarget());
	}
}

void FTLLRN_IKRetargetEditorController::ShowGlobalSettings()
{
	UTLLRN_IKRetargetGlobalSettings* GlobalSettings = AssetController->GetAsset()->GetGlobalSettingsUObject();
	if (GlobalSettings->EditorController.Pin().Get() != this)
	{
		GlobalSettings->EditorController = SharedThis(this);
	}
	
	SetDetailsObject(GlobalSettings);
}

void FTLLRN_IKRetargetEditorController::ShowPostPhaseSettings()
{
	UTLLRN_RetargetOpStack* PostSettings = AssetController->GetAsset()->GetPostSettingsUObject();
	if (PostSettings->EditorController.Pin().Get() != this)
	{
		PostSettings->EditorController = SharedThis(this);
	}
	
	SetDetailsObject(PostSettings);
}

void FTLLRN_IKRetargetEditorController::ShowRootSettings()
{
	UTLLRN_RetargetRootSettings* RootSettings = AssetController->GetAsset()->GetRootSettingsUObject();
	if (RootSettings->EditorController.Pin().Get() != this)
	{
		RootSettings->EditorController = SharedThis(this);	
	}
	
	SetDetailsObject(RootSettings);
}

#undef LOCTEXT_NAMESPACE
