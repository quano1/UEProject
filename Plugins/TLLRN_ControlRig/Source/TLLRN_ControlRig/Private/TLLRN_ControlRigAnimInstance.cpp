// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigAnimInstance.h"

#include "Animation/AnimCurveUtils.h"
#include "Animation/AnimNodeBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigAnimInstance)

////////////////////////////////////////////////////////////////////////////////////////
void FTLLRN_ControlRigAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);
}

bool FTLLRN_ControlRigAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	if (StoredTransforms.Num() == 0 && StoredCurves.Num() == 0 && !StoredAttributes.ContainsData())
	{
		return false;
	}

	FComponentSpacePoseContext CSContext(this);
	CSContext.Pose.InitPose(Output.Pose);

	TArray<FCompactPoseBoneIndex> ModifiedBones;
	
	const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();
	
	for (TPair<FMeshPoseBoneIndex, FTransform>& StoredTransform : StoredTransforms)
	{
		const FMeshPoseBoneIndex& MeshPoseBoneIndexToModify = StoredTransform.Key;
		FSkeletonPoseBoneIndex SkeletonPoseBoneIndex = BoneContainer.GetSkeletonPoseIndexFromMeshPoseIndex(MeshPoseBoneIndexToModify);
		FCompactPoseBoneIndex CompactIndex = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(SkeletonPoseBoneIndex);
		
		if (CompactIndex.GetInt() != INDEX_NONE)
		{
			CSContext.Pose.SetComponentSpaceTransform(CompactIndex, StoredTransform.Value);
			ModifiedBones.Add(CompactIndex);
		}
	}

	FCompactPose CompactPose(Output.Pose);
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(CSContext.Pose, CompactPose);

	// reset to ref pose before setting the pose to ensure if we don't have any missing bones
	Output.ResetToRefPose();

	for (const FCompactPoseBoneIndex& ModifiedBone : ModifiedBones)
	{
		Output.Pose[ModifiedBone] = CompactPose[ModifiedBone];
	}

	FBlendedCurve Curve;
	UE::Anim::FCurveUtils::BuildUnsorted(Curve, StoredCurves);
	Output.Curve.Combine(Curve);

	Output.CustomAttributes.CopyFrom(StoredAttributes);

	return true;
}

void FTLLRN_ControlRigAnimInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	// such that AnimScriptInstance->GetUpdateCounter().HasEverBeenUpdated() is accurate
	UpdateCounter.Increment();
}

FTLLRN_ControlRigAnimInstanceProxy::~FTLLRN_ControlRigAnimInstanceProxy()
{
}

////////////////////////////////////////////////////////////////////////////////////////

UTLLRN_ControlRigAnimInstance::UTLLRN_ControlRigAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseMultiThreadedAnimationUpdate = false;
}

FAnimInstanceProxy* UTLLRN_ControlRigAnimInstance::CreateAnimInstanceProxy()
{
	return new FTLLRN_ControlRigAnimInstanceProxy(this);
}


