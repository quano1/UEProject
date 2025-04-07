// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_ControlRig.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "TLLRN_ControlRigSkeletalMeshComponent.generated.h"

UCLASS(MinimalAPI)
class UTLLRN_ControlRigSkeletalMeshComponent : public UDebugSkelMeshComponent
{
	GENERATED_UCLASS_BODY()

	// USkeletalMeshComponent interface
	virtual void InitAnim(bool bForceReinit) override;

	// BEGIN UDebugSkeletalMeshComponent interface
	virtual void SetCustomDefaultPose() override;
	virtual const FReferenceSkeleton& GetReferenceSkeleton() const override
	{
		return DebugDrawSkeleton;
	}

	virtual const TArray<FBoneIndexType>& GetDrawBoneIndices() const override
	{
		return DebugDrawBones;
	}

	virtual FTransform GetDrawTransform(int32 BoneIndex) const override;

	virtual int32 GetNumDrawTransform() const
	{
		return DebugDrawBones.Num();
	}

	virtual void EnablePreview(bool bEnable, class UAnimationAsset * PreviewAsset) override;

	// return true if preview animation is active 
	virtual bool IsPreviewOn() const override;
	// END UDebugSkeletalMeshComponent interface

public:

	/*
	 * React to a control rig being debugged
	 */
	void SetTLLRN_ControlRigBeingDebugged(UTLLRN_ControlRig* InTLLRN_ControlRig);
	
	/*
	 *	Rebuild debug draw skeleton 
	 */
	void RebuildDebugDrawSkeleton();

	void OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);
	void OnHierarchyModified_AnyThread(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);

	void OnPreConstruction_AnyThread(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName);
	void OnPostConstruction_AnyThread(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName);

private:
	FReferenceSkeleton DebugDrawSkeleton;
	TArray<FBoneIndexType> DebugDrawBones;
	TArray<int32> DebugDrawBoneIndexInHierarchy;
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRigBeingDebuggedPtr;
	int32 HierarchyInteractionBracket;
	bool bRebuildDebugDrawSkeletonRequired;
	bool bIsConstructionEventRunning;
};
