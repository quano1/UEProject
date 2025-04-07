// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_IKRetargetAnimInstanceProxy.h"
#include "RetargetEditor/TLLRN_IKRetargetAnimInstance.h"
#include "AnimNodes/TLLRN_AnimNode_RetargetPoseFromMesh.h"
#include "RetargetEditor/TLLRN_IKRetargetEditorController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRetargetAnimInstanceProxy)

FTLLRN_IKRetargetAnimInstanceProxy::FTLLRN_IKRetargetAnimInstanceProxy(
	UAnimInstance* InAnimInstance,
	FTLLRN_AnimNode_PreviewRetargetPose* InPreviewPoseNode,
	FTLLRN_AnimNode_RetargetPoseFromMesh* InRetargetNode)
	: FAnimPreviewInstanceProxy(InAnimInstance),
	PreviewPoseNode(InPreviewPoseNode),
	RetargetNode(InRetargetNode),
	OutputMode(ETLLRN_RetargeterOutputMode::EditRetargetPose)
{
	// retargeting is all done in world space, moving the source component breaks root motion retargeting
	bIgnoreRootMotion = true;
}

void FTLLRN_IKRetargetAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimPreviewInstanceProxy::Initialize(InAnimInstance);
	PreviewPoseNode->InputPose.SetLinkNode(RetargetNode);

	FAnimationInitializeContext InitContext(this);
	PreviewPoseNode->Initialize_AnyThread(InitContext);
	RetargetNode->Initialize_AnyThread(InitContext);
}

void FTLLRN_IKRetargetAnimInstanceProxy::CacheBones()
{
	if (bBoneCachesInvalidated)
	{
		FAnimationCacheBonesContext Context(this);
		SingleNode.CacheBones_AnyThread(Context);
		RetargetNode->CacheBones_AnyThread(Context);
		PreviewPoseNode->CacheBones_AnyThread(Context);
		bBoneCachesInvalidated = false;
	}
}

bool FTLLRN_IKRetargetAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	if (PreviewPoseNode->TLLRN_IKRetargeterAsset)
	{
		bIgnoreRootLock = PreviewPoseNode->TLLRN_IKRetargeterAsset->bIgnoreRootLockInPreview;	
	}
	
	switch (OutputMode)
	{
	case ETLLRN_RetargeterOutputMode::RunRetarget:
		{
			if (SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source)
			{
				FAnimPreviewInstanceProxy::Evaluate(Output);
			}
			else
			{
				RetargetNode->Evaluate_AnyThread(Output);
			}
			break;
		}
	case ETLLRN_RetargeterOutputMode::EditRetargetPose:
		{
			PreviewPoseNode->Evaluate_AnyThread(Output);
			break;
		}
	default:
		checkNoEntry();
	}
	
	return true;
}

FAnimNode_Base* FTLLRN_IKRetargetAnimInstanceProxy::GetCustomRootNode()
{
	return PreviewPoseNode;
}

void FTLLRN_IKRetargetAnimInstanceProxy::GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes)
{
	OutNodes.Add(RetargetNode);
	OutNodes.Add(PreviewPoseNode);
}

void FTLLRN_IKRetargetAnimInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	if (CurrentAsset != nullptr)
	{
		FAnimPreviewInstanceProxy::UpdateAnimationNode(InContext);
	}
	else
	{
		PreviewPoseNode->Update_AnyThread(InContext);
		RetargetNode->Update_AnyThread(InContext);
	}
}

void FTLLRN_IKRetargetAnimInstanceProxy::ConfigureAnimInstance(
	const ETLLRN_RetargetSourceOrTarget& InSourceOrTarget,
	UTLLRN_IKRetargeter* InIKRetargetAsset,
	TWeakObjectPtr<USkeletalMeshComponent> InSourceMeshComponent)
{
	SourceOrTarget = InSourceOrTarget;

	PreviewPoseNode->SourceOrTarget = InSourceOrTarget;
	PreviewPoseNode->TLLRN_IKRetargeterAsset = InIKRetargetAsset;

	if (SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Target)
	{
		RetargetNode->TLLRN_IKRetargeterAsset = InIKRetargetAsset;
		RetargetNode->bUseAttachedParent = false;
		RetargetNode->SourceMeshComponent = InSourceMeshComponent;
		if (UTLLRN_IKRetargetProcessor* Processor = RetargetNode->GetRetargetProcessor())
		{
			Processor->SetNeedsInitialized();
		}
	}
}

void FTLLRN_IKRetargetAnimInstanceProxy::SetRetargetMode(const ETLLRN_RetargeterOutputMode& InOutputMode)
{
	OutputMode = InOutputMode;
	
	if (UTLLRN_IKRetargetProcessor* Processor = RetargetNode->GetRetargetProcessor())
	{
		Processor->SetNeedsInitialized();
	}
}

void FTLLRN_IKRetargetAnimInstanceProxy::SetRetargetPoseBlend(const float& InRetargetPoseBlend) const
{
	PreviewPoseNode->RetargetPoseBlend = InRetargetPoseBlend;
}


