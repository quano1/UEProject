// Copyright Epic Games, Inc. All Rights Reserved.
#include "Retargeter/RetargetOps/TLLRN_RootMotionGeneratorOp.h"

#include "Engine/SkeletalMesh.h"

#define LOCTEXT_NAMESPACE "TLLRN_RootMotionGeneratorOp"

#if WITH_EDITOR
void UTLLRN_RootMotionGeneratorOp::OnAddedToStack(const UTLLRN_IKRetargeter* Asset)
{
	// get the root of the SOURCE skeleton
	if (const USkeletalMesh* Mesh = Asset->GetPreviewMesh(ETLLRN_RetargetSourceOrTarget::Source))
	{
		SourceRootBone = Mesh->GetRefSkeleton().GetBoneName(0);
	}
	
	// get the root of the TARGET skeleton
	if (const USkeletalMesh* Mesh = Asset->GetPreviewMesh(ETLLRN_RetargetSourceOrTarget::Target))
	{
		TargetRootBone = Mesh->GetRefSkeleton().GetBoneName(0);
	}

	// get the retarget root bone
	if (const UTLLRN_IKRigDefinition* IKRig = Asset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target))
	{
		TargetPelvisBone = IKRig->GetRetargetRoot();
	}
}
#endif

bool UTLLRN_RootMotionGeneratorOp::Initialize(
	const UTLLRN_IKRetargetProcessor* Processor,
	const FTLLRN_RetargetSkeleton& SourceSkeleton,
	const FTargetSkeleton& TargetSkeleton,
	FTLLRN_IKRigLogger& Log)
{
	SourceRootIndex = SourceSkeleton.BoneNames.Find(SourceRootBone);
	bool bHasAllPrerequisites = true;
	if (SourceRootIndex == INDEX_NONE)
	{
		Log.LogWarning(FText::Format(LOCTEXT("MissingSourceRootBone", "Root Motion Remap Op, missing source root bone {0}."), FText::FromName(SourceRootBone)));
		bHasAllPrerequisites = false;
	}
	
	TargetRootIndex = TargetSkeleton.BoneNames.Find(TargetRootBone);
	if (TargetRootIndex == INDEX_NONE)
	{
		Log.LogWarning(FText::Format(LOCTEXT("MissingTargetRootBone", "Root Motion Remap Op, missing target root bone {0}."), FText::FromName(TargetRootBone)));
		bHasAllPrerequisites = false;
	}

	TargetPelvisIndex = TargetSkeleton.BoneNames.Find(TargetPelvisBone);
	if (TargetPelvisIndex == INDEX_NONE)
	{
		Log.LogWarning(FText::Format(LOCTEXT("MissingPelvisBone", "Root Motion Remap Op, missing target pelvis bone {0}."), FText::FromName(TargetPelvisBone)));
		bHasAllPrerequisites = false;
	}

#if WITH_EDITOR
	if (bHasAllPrerequisites)
	{
		Message = FText::Format(LOCTEXT("ReadyToRun", "Generating root motion on {0}."), FText::FromName(TargetRootBone));
	}
	else
	{
		Message = FText(LOCTEXT("MissingBonesWarning", "Bone(s) not found. See output log."));
	}
#endif

	// can't cache everything unless all prerequisites are met
	if (!bHasAllPrerequisites)
	{
		return false;
	}

	TargetPelvisInRefPose = TargetSkeleton.RetargetGlobalPose[TargetPelvisIndex];
	SourceRootInRefPose = SourceSkeleton.RetargetGlobalPose[SourceRootIndex];
	TargetRootInRefPose = TargetSkeleton.RetargetGlobalPose[TargetRootIndex];
	TargetPelvisRelativeToTargetRootRefPose = TargetRootInRefPose.GetRelativeTransform(TargetPelvisInRefPose);

	// generate list of non-retargeted child indices
	const int32 NumTargetBones = TargetSkeleton.BoneNames.Num();
	NonRetargetedChildrenOfRoot.Reset();
	for (int32 BoneIndex=1; BoneIndex<NumTargetBones; ++BoneIndex)
	{
		bool bHasNoRetargetedParent = true;
		
		int32 ParentIndex = BoneIndex;
		while(ParentIndex != INDEX_NONE)
		{
			if (TargetSkeleton.IsBoneRetargeted[ParentIndex])
			{
				bHasNoRetargetedParent = false;
				break;
			}
			ParentIndex = TargetSkeleton.ParentIndices[ParentIndex];
		}

		if (bHasNoRetargetedParent)
		{
			NonRetargetedChildrenOfRoot.Add(BoneIndex);
		}
	}
	
	return true;
}

void UTLLRN_RootMotionGeneratorOp::Run(
	const UTLLRN_IKRetargetProcessor* Processor,
	const TArray<FTransform>& InSourceGlobalPose,
	TArray<FTransform>& OutTargetGlobalPose)
{
	// generate a new transform for the target root bone
	FTransform NewRootTransform;
	
	// either generate motion from target pelvis, or copy from source root
	if (RootMotionSource == ETLLRN_RootMotionSource::GenerateFromTargetPelvis)
	{
		GenerateRootMotionFromTargetPelvis(NewRootTransform, InSourceGlobalPose, OutTargetGlobalPose);
	}
	else
	{
		CopyRootMotionFromSourceRoot(NewRootTransform, Processor, InSourceGlobalPose);
	}

	// optionally propagate delta to all non-retargeted children
	if (bPropagateToNonRetargetedChildren)
	{
		const FTransform Delta = OutTargetGlobalPose[TargetRootIndex].Inverse() * NewRootTransform;
		for (const int32 BoneIndex : NonRetargetedChildrenOfRoot)
		{
			OutTargetGlobalPose[BoneIndex] = OutTargetGlobalPose[BoneIndex] * Delta;
		}
	}

	// apply to the target root bone
	OutTargetGlobalPose[TargetRootIndex] = NewRootTransform * GlobalOffset;
}

void UTLLRN_RootMotionGeneratorOp::GenerateRootMotionFromTargetPelvis(
	FTransform& OutRootTransform,
	const TArray<FTransform>& InSourceGlobalPose,
	const TArray<FTransform>& InTargetGlobalPose) const
{
	// In this case, we are generating root motion "from scratch"
	// using the target Pelvis bone as the source of the motion
	
	if (bMaintainOffsetFromPelvis)
	{
		// set root to the relative offset from the pelvis (recorded from ref pose)
		OutRootTransform = TargetPelvisRelativeToTargetRootRefPose * InTargetGlobalPose[TargetPelvisIndex];
	}
	else
	{
		// snap root to the pelvis directly
		OutRootTransform = InTargetGlobalPose[TargetPelvisIndex];
	}

	// optionally remove all rotation (use static ref pose orientation)
	if (!bRotateWithPelvis)
	{
		OutRootTransform.SetRotation(TargetRootInRefPose.GetRotation());
	}

	// adjust height of root
	if (RootHeightSource == ETLLRN_RootMotionHeightSource::SnapToGround)
	{
		// snap the root to the ground plane
		FVector RootTranslation = OutRootTransform.GetTranslation();
		RootTranslation.Z = 0.f;
		OutRootTransform.SetTranslation(RootTranslation);
	}else if (RootHeightSource == ETLLRN_RootMotionHeightSource::CopyHeightFromSource)
	{
		// snap the root to the height from the source
		FVector RootTranslation = OutRootTransform.GetTranslation();
		RootTranslation.Z = InSourceGlobalPose[SourceRootIndex].GetTranslation().Z;
		OutRootTransform.SetTranslation(RootTranslation);
	}
}

void UTLLRN_RootMotionGeneratorOp::CopyRootMotionFromSourceRoot(
	FTransform& OutRootTransform,
	const UTLLRN_IKRetargetProcessor* Processor,
	const TArray<FTransform>& InSourceGlobalPose) const
{
	// In this case, we are copying root motion from the source root
	// But we also scale it based on the relative height of the source/target Pelvis

	// rotation is the original target root rotation in ref pose plus the current rotation delta of the source
	const FQuat SourceRootRotationDelta = InSourceGlobalPose[SourceRootIndex].GetRotation() * SourceRootInRefPose.GetRotation().Inverse();
	OutRootTransform.SetRotation(SourceRootRotationDelta * TargetRootInRefPose.GetRotation());

	// scale root translation by same scale factor applied to Pelvis (and modified by settings)
	FVector NewRootLocation = InSourceGlobalPose[SourceRootIndex].GetLocation();
	FVector RootTranslationDelta = NewRootLocation - SourceRootInRefPose.GetTranslation();
	RootTranslationDelta *= Processor->GetRootRetargeter().GetGlobalScaleVector();
	NewRootLocation = SourceRootInRefPose.GetTranslation() + RootTranslationDelta;

	// optionally snap the root to the ground plane
	if (RootHeightSource == ETLLRN_RootMotionHeightSource::SnapToGround)
	{
		NewRootLocation.Z = 0.f;
	}

	// apply the modified translation
	OutRootTransform.SetTranslation(NewRootLocation);
}

#undef LOCTEXT_NAMESPACE
