// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_IKRetargetAnimInstance.h"
#include "Animation/AnimStats.h"
#include "Engine/SkeletalMesh.h"
#include "RetargetEditor/TLLRN_IKRetargetAnimInstanceProxy.h"
#include "RetargetEditor/TLLRN_IKRetargeterController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRetargetAnimInstance)


void FTLLRN_AnimNode_PreviewRetargetPose::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)

	const FBoneContainer& RequiredBones = Context.AnimInstanceProxy->GetRequiredBones();
	if (!RequiredBones.IsValid())
	{
		return;
	}
	
	if (!Context.AnimInstanceProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset())
	{
		return;
	}

	// rebuild mapping
	RequiredBoneToMeshBoneMap.Reset();

	const FReferenceSkeleton& RefSkeleton = RequiredBones.GetReferenceSkeleton();
	const FReferenceSkeleton& TargetSkeleton = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset()->GetRefSkeleton();
	const TArray<FBoneIndexType>& RequiredBonesArray = RequiredBones.GetBoneIndicesArray();
	for (int32 RequiredBoneIndex = 0; RequiredBoneIndex < RequiredBonesArray.Num(); ++RequiredBoneIndex)
	{
		const FBoneIndexType ReqBoneIndex = RequiredBonesArray[RequiredBoneIndex]; 
		if (ReqBoneIndex == INDEX_NONE)
		{
			continue;
		}
		
		const FName BoneName = RefSkeleton.GetBoneName(ReqBoneIndex);
		const int32 TargetBoneIndex = TargetSkeleton.FindBoneIndex(BoneName);
		if (TargetBoneIndex == INDEX_NONE)
		{
			continue;
		}

		// store require bone to target bone indices
		RequiredBoneToMeshBoneMap.Emplace(TargetBoneIndex);
	}
}

void FTLLRN_AnimNode_PreviewRetargetPose::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	ANIM_MT_SCOPE_CYCLE_COUNTER_VERBOSE(PreviewRetargetPose, !IsInGameThread());

	Output.Pose.ResetToRefPose();
	
	if (!TLLRN_IKRetargeterAsset)
	{
		return;
	}
	
	const FTLLRN_IKRetargetPose* RetargetPose = TLLRN_IKRetargeterAsset->GetCurrentRetargetPose(SourceOrTarget);
	if (!RetargetPose)
	{
		return;	
	}
	
	// generate full (no LOD) LOCAL retarget pose by applying the local rotation offsets from the stored retarget pose
	// these poses are read back by the editor and need to be complete (not culled)
	const FReferenceSkeleton& RefSkeleton = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset()->GetRefSkeleton();
	const TArray<FTransform>& RefPose = RefSkeleton.GetRefBonePose(); 
	RetargetLocalPose = RefPose;
	for (const TTuple<FName, FQuat>& BoneDelta : RetargetPose->GetAllDeltaRotations())
	{
		const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneDelta.Key);
		if (BoneIndex != INDEX_NONE)
		{
			FTransform& LocalBoneTransform = RetargetLocalPose[BoneIndex];
			LocalBoneTransform.SetRotation(LocalBoneTransform.GetRotation() * BoneDelta.Value);
		}
	}

	// generate GLOBAL space pose (for editor to query)
	FAnimationRuntime::FillUpComponentSpaceTransforms(RefSkeleton, RetargetLocalPose, RetargetGlobalPose);

	// apply root translation offset from the retarget pose (done in global space)
	const UTLLRN_IKRetargeterController* Controller = UTLLRN_IKRetargeterController::GetController(TLLRN_IKRetargeterAsset);
	const FName RetargetRootBoneName = Controller->GetRetargetRootBone(SourceOrTarget);
	const int32 RootBoneIndex = RefSkeleton.FindBoneIndex(RetargetRootBoneName);
	if (RootBoneIndex != INDEX_NONE)
	{
		FTransform& RootTransform = RetargetGlobalPose[RootBoneIndex];
		RootTransform.AddToTranslation(RetargetPose->GetRootTranslationDelta());
		
		// update local transform of retarget root
		const int32 ParentIndex = RefSkeleton.GetParentIndex(RootBoneIndex);
		if (ParentIndex == INDEX_NONE)
		{
			RetargetLocalPose[RootBoneIndex] = RootTransform;
		}
		else
		{
			const FTransform& ChildGlobalTransform = RetargetGlobalPose[RootBoneIndex];
			const FTransform& ParentGlobalTransform = RetargetGlobalPose[ParentIndex];
			RetargetLocalPose[RootBoneIndex] = ChildGlobalTransform.GetRelativeTransform(ParentGlobalTransform);
		}
	}

	// update GLOBAL space pose after root translation (for editor to query)
	FAnimationRuntime::FillUpComponentSpaceTransforms(RefSkeleton, RetargetLocalPose, RetargetGlobalPose);
	
	// copy to the compact output pose
	const int32 NumBones = Output.Pose.GetNumBones();
	for (int32 Index = 0; Index<NumBones; ++Index)
	{
		FCompactPoseBoneIndex BoneIndex(Index);
		const int32 MeshBoneIndex = RequiredBoneToMeshBoneMap[Index];

		BlendTransform<ETransformBlendMode::Overwrite>(RefPose[MeshBoneIndex], Output.Pose[BoneIndex], 1.f - RetargetPoseBlend);
		BlendTransform<ETransformBlendMode::Accumulate>(RetargetLocalPose[MeshBoneIndex], Output.Pose[BoneIndex], RetargetPoseBlend);
	}
}

UTLLRN_IKRetargetAnimInstance::UTLLRN_IKRetargetAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseMultiThreadedAnimationUpdate = false;
}

void UTLLRN_IKRetargetAnimInstance::SetRetargetMode(const ETLLRN_RetargeterOutputMode& InOutputMode)
{
	FTLLRN_IKRetargetAnimInstanceProxy& Proxy = GetProxyOnGameThread<FTLLRN_IKRetargetAnimInstanceProxy>();
	Proxy.SetRetargetMode(InOutputMode);
}

void UTLLRN_IKRetargetAnimInstance::SetRetargetPoseBlend(const float& RetargetPoseBlend)
{
	FTLLRN_IKRetargetAnimInstanceProxy& Proxy = GetProxyOnGameThread<FTLLRN_IKRetargetAnimInstanceProxy>();
	Proxy.SetRetargetPoseBlend(RetargetPoseBlend);
}

void UTLLRN_IKRetargetAnimInstance::ConfigureAnimInstance(
	const ETLLRN_RetargetSourceOrTarget& InSourceOrTarget,
	UTLLRN_IKRetargeter* InAsset,
	TWeakObjectPtr<USkeletalMeshComponent> InSourceMeshComponent)
{
	FTLLRN_IKRetargetAnimInstanceProxy& Proxy = GetProxyOnGameThread<FTLLRN_IKRetargetAnimInstanceProxy>();
	Proxy.ConfigureAnimInstance(InSourceOrTarget, InAsset, InSourceMeshComponent);
}

UTLLRN_IKRetargetProcessor* UTLLRN_IKRetargetAnimInstance::GetRetargetProcessor()
{
	// depending on initialization order, the anim node may not have created the processor yet, so optionally force it to do so now
	RetargetNode.CreateRetargetProcessorIfNeeded(this);
	
	return RetargetNode.GetRetargetProcessor();
}

FAnimInstanceProxy* UTLLRN_IKRetargetAnimInstance::CreateAnimInstanceProxy()
{
	LLM_SCOPE_BYNAME(TEXT("Animation/IKRig"));
	return new FTLLRN_IKRetargetAnimInstanceProxy(this, &PreviewPoseNode, &RetargetNode);
}

