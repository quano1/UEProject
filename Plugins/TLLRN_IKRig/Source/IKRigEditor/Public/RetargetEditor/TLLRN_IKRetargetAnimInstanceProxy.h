// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimPreviewInstance.h"
#include "TLLRN_IKRetargetAnimInstance.h"
#include "Animation/AnimNode_LinkedInputPose.h"

#include "TLLRN_IKRetargetAnimInstanceProxy.generated.h"

class UTLLRN_IKRetargeter;
enum class ETLLRN_RetargetSourceOrTarget : uint8;
struct FTLLRN_AnimNode_PreviewRetargetPose;
struct FTLLRN_AnimNode_RetargetPoseFromMesh;

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct FTLLRN_IKRetargetAnimInstanceProxy : public FAnimPreviewInstanceProxy
{
	GENERATED_BODY()

public:
	
	FTLLRN_IKRetargetAnimInstanceProxy() = default;
	FTLLRN_IKRetargetAnimInstanceProxy(
		UAnimInstance* InAnimInstance,
		FTLLRN_AnimNode_PreviewRetargetPose* InPreviewPoseNode,
		FTLLRN_AnimNode_RetargetPoseFromMesh* InRetargetNode);
	virtual ~FTLLRN_IKRetargetAnimInstanceProxy() override = default;

	/** FAnimPreviewInstanceProxy interface */
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void CacheBones() override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	/** END FAnimPreviewInstanceProxy interface */

	/** FAnimInstanceProxy interface */
	/** Called when the anim instance is being initialized. These nodes can be provided */
	virtual FAnimNode_Base* GetCustomRootNode() override;
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override;
	/** END FAnimInstanceProxy interface */

	void ConfigureAnimInstance(
		const ETLLRN_RetargetSourceOrTarget& InSourceOrTarget,
		UTLLRN_IKRetargeter* InIKRetargetAsset,
		TWeakObjectPtr<USkeletalMeshComponent> InSourceMeshComponent);

	void SetRetargetMode(const ETLLRN_RetargeterOutputMode& InOutputMode);

	void SetRetargetPoseBlend(const float& InRetargetPoseBlend) const;

	FTLLRN_AnimNode_PreviewRetargetPose* PreviewPoseNode;
	FTLLRN_AnimNode_RetargetPoseFromMesh* RetargetNode;

	ETLLRN_RetargetSourceOrTarget SourceOrTarget;
	ETLLRN_RetargeterOutputMode OutputMode;
};