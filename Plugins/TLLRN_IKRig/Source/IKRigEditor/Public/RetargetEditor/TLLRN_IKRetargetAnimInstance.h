// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimPreviewInstance.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNodes/TLLRN_AnimNode_RetargetPoseFromMesh.h"

#include "TLLRN_IKRetargetAnimInstance.generated.h"

enum class ETLLRN_RetargeterOutputMode : uint8;
class UTLLRN_IKRetargeter;

// a node just to preview a retarget pose
USTRUCT(BlueprintInternalUseOnly)
struct FTLLRN_AnimNode_PreviewRetargetPose : public FAnimNode_Base
{
	GENERATED_BODY()

	FPoseLink InputPose;

	/** Retarget asset holding the pose to preview.*/
	TObjectPtr<UTLLRN_IKRetargeter> TLLRN_IKRetargeterAsset = nullptr;

	/** Whether to preview the source or the target */
	ETLLRN_RetargetSourceOrTarget SourceOrTarget;

	/** Amount to blend the retarget pose with the reference pose when shoing the retarget pose */
	float RetargetPoseBlend = 1.0f;

	// FAnimNode_Base interface
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	// End of FAnimNode_Base interface

	const TArray<FTransform>& GetLocalRetargetPose() const { return RetargetLocalPose; };
	const TArray<FTransform>& GetGlobalRetargetPose() const { return RetargetGlobalPose; };

private:
	// mapping from required bone indices (from LODs) to actual bone indices within the skeletal mesh 
	TArray<int32> RequiredBoneToMeshBoneMap;

	TArray<FTransform> RetargetLocalPose;
	TArray<FTransform> RetargetGlobalPose;
};

UCLASS(transient, NotBlueprintable)
class UTLLRN_IKRetargetAnimInstance : public UAnimPreviewInstance
{
	GENERATED_UCLASS_BODY()

public:

	void SetRetargetMode(const ETLLRN_RetargeterOutputMode& OutputMode);

	void SetRetargetPoseBlend(const float& RetargetPoseBlend);

	void ConfigureAnimInstance(
		const ETLLRN_RetargetSourceOrTarget& SourceOrTarget,
		UTLLRN_IKRetargeter* InIKRetargetAsset,
		TWeakObjectPtr<USkeletalMeshComponent> InSourceMeshComponent);

	UTLLRN_IKRetargetProcessor* GetRetargetProcessor();

	const TArray<FTransform>& GetLocalRetargetPose() const { return PreviewPoseNode.GetLocalRetargetPose(); };
	const TArray<FTransform>& GetGlobalRetargetPose() const { return PreviewPoseNode.GetGlobalRetargetPose(); };

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	UPROPERTY(Transient)
	FTLLRN_AnimNode_PreviewRetargetPose PreviewPoseNode;
	
	UPROPERTY(Transient)
	FTLLRN_AnimNode_RetargetPoseFromMesh RetargetNode;
};
