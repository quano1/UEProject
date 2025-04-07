// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimPreviewInstance.h"
#include "Animation/AnimNode_LinkedInputPose.h"
#include "AnimNodes/TLLRN_AnimNode_IKRig.h"

#include "TLLRN_IKRigAnimInstanceProxy.generated.h"

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct FTLLRN_IKRigAnimInstanceProxy : public FAnimPreviewInstanceProxy
{
	GENERATED_BODY()

public:
	
	FTLLRN_IKRigAnimInstanceProxy() = default;
	FTLLRN_IKRigAnimInstanceProxy(UAnimInstance* InAnimInstance, FTLLRN_AnimNode_IKRig* IKRigNode);
	virtual ~FTLLRN_IKRigAnimInstanceProxy() override {};

	/** FAnimPreviewInstanceProxy interface */
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	/** END FAnimPreviewInstanceProxy interface */

	/** FAnimInstanceProxy interface */
	/** Called when the anim instance is being initialized. These nodes can be provided */
	virtual FAnimNode_Base* GetCustomRootNode() override;
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override;
	/** END FAnimInstanceProxy interface */

	void SetIKRigAsset(UTLLRN_IKRigDefinition* InIKRigAsset);

	FTLLRN_AnimNode_IKRig* IKRigNode;
};