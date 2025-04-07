// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_SetBoneTransform.generated.h"


/**
 * SetBoneTransform is used to perform a change in the hierarchy by setting a single bone's transform.
 */
USTRUCT(meta=(DisplayName="Set Transform", Category="Hierarchy", DocumentationPolicy="Strict", Keywords = "SetBoneTransform", Deprecated="4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetBoneTransform : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetBoneTransform()
		: Space(ERigVMTransformSpace::LocalSpace)
		, Weight(1.f)
		, bPropagateToChildren(true)
		, CachedBone(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Bone to set the transform for.
	 */
	UPROPERTY(meta = (Input))
	FName Bone;

	/**
	 * The transform value to set for the given Bone.
	 */
	UPROPERTY(meta = (Input))
	FTransform Transform;

	/**
	 * The transform value result (after weighting)
	 */
	UPROPERTY(meta = (Output))
	FTransform Result;

	/**
	 * Defines if the bone's transform should be set
	 * in local or global space.
	 */
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;
	
	/**
	 * The weight of the change - how much the change should be applied
	 */
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	/**
	 * If set to true all of the global transforms of the children 
	 * of this bone will be recalculated based on their local transforms.
	 * Note: This is computationally more expensive than turning it off.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bPropagateToChildren;

	// Used to cache the internally used bone index
	UPROPERTY(transient)
	FTLLRN_CachedTLLRN_RigElement CachedBone;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
