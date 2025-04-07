// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigAnimInstanceProxy.h"
#include "RigEditor/TLLRN_IKRigAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRigAnimInstanceProxy)


FTLLRN_IKRigAnimInstanceProxy::FTLLRN_IKRigAnimInstanceProxy(UAnimInstance* InAnimInstance, FTLLRN_AnimNode_IKRig* InIKRigNode)
	: FAnimPreviewInstanceProxy(InAnimInstance),
	IKRigNode(InIKRigNode)
{
}

void FTLLRN_IKRigAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	Super::Initialize(InAnimInstance);
	IKRigNode->bDriveWithSourceAsset = true; // force this instance of the IK Rig evaluation to copy settings from the source IK Rig asset 
}

bool FTLLRN_IKRigAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	Super::Evaluate(Output);
	IKRigNode->Evaluate_AnyThread(Output);
	return true;
}

FAnimNode_Base* FTLLRN_IKRigAnimInstanceProxy::GetCustomRootNode()
{
	return IKRigNode;
}

void FTLLRN_IKRigAnimInstanceProxy::GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes)
{
	OutNodes.Add(IKRigNode);
}

void FTLLRN_IKRigAnimInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	Super::UpdateAnimationNode(InContext);
	IKRigNode->Update_AnyThread(InContext);
}

void FTLLRN_IKRigAnimInstanceProxy::SetIKRigAsset(UTLLRN_IKRigDefinition* InIKRigAsset)
{
	IKRigNode->RigDefinitionAsset = InIKRigAsset;
}


