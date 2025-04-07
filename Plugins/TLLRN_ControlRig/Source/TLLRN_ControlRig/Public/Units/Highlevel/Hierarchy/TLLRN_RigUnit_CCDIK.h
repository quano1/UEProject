// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/Highlevel/TLLRN_RigUnit_HighlevelBase.h"
#include "CCDIK.h"
#include "TLLRN_RigUnit_CCDIK.generated.h"

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CCDIK_RotationLimit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CCDIK_RotationLimit()
	{
		Bone = NAME_None;
		Limit = 30.f;
	}

	/**
	 * The name of the bone to apply the rotation limit to.
	 */
	UPROPERTY(meta = (Input))
	FName Bone;

	/**
	 * The limit of the rotation in degrees.
	 */
	UPROPERTY(meta = (Input, Constant))
	float Limit;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CCDIK_RotationLimitPerItem
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CCDIK_RotationLimitPerItem()
	{
		Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		Limit = 30.f;
	}

	/**
	 * The name of the item to apply the rotation limit to.
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Item;

	/**
	 * The limit of the rotation in degrees.
	 */
	UPROPERTY(meta = (Input, Constant))
	float Limit;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CCDIK_WorkData
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CCDIK_WorkData()
	{
		CachedEffector = FTLLRN_CachedTLLRN_RigElement();
	}

	UPROPERTY()
	TArray<FCCDIKChainLink> Chain;

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> CachedItems;

	UPROPERTY()
	TArray<int32> RotationLimitIndex;

	UPROPERTY()
	TArray<float> RotationLimitsPerItem;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedEffector;
};

/**
 * The CCID solver can solve N-Bone chains using 
 * the Cyclic Coordinate Descent Inverse Kinematics algorithm.
 * For now this node supports single effector chains only.
 */
USTRUCT(meta=(DisplayName="CCDIK", Category="Hierarchy", Keywords="N-Bone,IK", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CCDIK : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	FTLLRN_RigUnit_CCDIK()
	{
		EffectorTransform = FTransform::Identity;
		Precision = 1.f;
		Weight = 1.f;
		MaxIterations = 10;
		bStartFromTail = true;
		bPropagateToChildren = true;
		BaseRotationLimit = 30.f;
	}

	/**
	 * The first bone in the chain to solve
	 */
	UPROPERTY(meta = (Input))
	FName StartBone;

	/**
	 * The last bone in the chain to solve - the effector
	 */
	UPROPERTY(meta = (Input))
	FName EffectorBone;

	/**
	 * The transform of the effector in global space
	 */
	UPROPERTY(meta = (Input))
	FTransform EffectorTransform;

	/**
	 * The precision to use for the fabrik solver
	 */
	UPROPERTY(meta = (Input, Constant))
	float Precision;

	/**
	 * The weight of the solver - how much the IK should be applied.
	 */
	UPROPERTY(meta = (Input))
	float Weight;

	/**
	 * The maximum number of iterations. Values between 4 and 16 are common.
	 */
	UPROPERTY(meta = (Input))
	int32 MaxIterations;

	/**
	 * If set to true the direction of the solvers is flipped.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bStartFromTail;

	/**
	 * The general rotation limit to be applied to bones
	 */
	UPROPERTY(meta = (Input, Constant))
	float BaseRotationLimit;

	/**
	 * Defines the limits of rotation per bone.
	 */
	UPROPERTY(meta = (Input, Constant))
	TArray<FTLLRN_RigUnit_CCDIK_RotationLimit> RotationLimits;

	/**
	 * If set to true all of the global transforms of the children
	 * of this bone will be recalculated based on their local transforms.
	 * Note: This is computationally more expensive than turning it off.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bPropagateToChildren;

	UPROPERTY()
	FTLLRN_RigUnit_CCDIK_WorkData WorkData;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * The CCID solver can solve N-Bone chains using 
 * the Cyclic Coordinate Descent Inverse Kinematics algorithm.
 * For now this node supports single effector chains only.
 */
USTRUCT(meta=(DisplayName="CCDIK", Category="Hierarchy", Keywords="N-Bone,IK", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CCDIKPerItem : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	FTLLRN_RigUnit_CCDIKPerItem()
	{
		EffectorTransform = FTransform::Identity;
		Precision = 1.f;
		Weight = 1.f;
		MaxIterations = 10;
		bStartFromTail = true;
		bPropagateToChildren = true;
		BaseRotationLimit = 30.f;
	}

	/**
	 * The chain to use
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKeyCollection Items;

	/**
	 * The transform of the effector in global space
	 */
	UPROPERTY(meta = (Input))
	FTransform EffectorTransform;

	/**
	 * The precision to use for the fabrik solver
	 */
	UPROPERTY(meta = (Input, Constant))
	float Precision;

	/**
	 * The weight of the solver - how much the IK should be applied.
	 */
	UPROPERTY(meta = (Input))
	float Weight;

	/**
	 * The maximum number of iterations. Values between 4 and 16 are common.
	 */
	UPROPERTY(meta = (Input))
	int32 MaxIterations;

	/**
	 * If set to true the direction of the solvers is flipped.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bStartFromTail;

	/**
	 * The general rotation limit to be applied to bones
	 */
	UPROPERTY(meta = (Input, Constant))
	float BaseRotationLimit;

	/**
	 * Defines the limits of rotation per bone.
	 */
	UPROPERTY(meta = (Input, Constant))
	TArray<FTLLRN_RigUnit_CCDIK_RotationLimitPerItem> RotationLimits;

	/**
	 * If set to true all of the global transforms of the children
	 * of this bone will be recalculated based on their local transforms.
	 * Note: This is computationally more expensive than turning it off.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bPropagateToChildren;

	UPROPERTY()
	FTLLRN_RigUnit_CCDIK_WorkData WorkData;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * The CCID solver can solve N-Bone chains using 
 * the Cyclic Coordinate Descent Inverse Kinematics algorithm.
 * For now this node supports single effector chains only.
 */
USTRUCT(meta=(DisplayName="CCDIK", Category="Hierarchy", Keywords="N-Bone,IK", NodeColor = "0 1 1"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CCDIKItemArray : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	FTLLRN_RigUnit_CCDIKItemArray()
	{
		EffectorTransform = FTransform::Identity;
		Precision = 1.f;
		Weight = 1.f;
		MaxIterations = 10;
		bStartFromTail = true;
		bPropagateToChildren = true;
		BaseRotationLimit = 30.f;
	}

	/**
	 * The chain to use
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	TArray<FTLLRN_RigElementKey> Items;

	/**
	 * The transform of the effector in global space
	 */
	UPROPERTY(meta = (Input))
	FTransform EffectorTransform;

	/**
	 * The precision to use for the fabrik solver
	 */
	UPROPERTY(meta = (Input, Constant))
	float Precision;

	/**
	 * The weight of the solver - how much the IK should be applied.
	 */
	UPROPERTY(meta = (Input))
	float Weight;

	/**
	 * The maximum number of iterations. Values between 4 and 16 are common.
	 */
	UPROPERTY(meta = (Input))
	int32 MaxIterations;

	/**
	 * If set to true the direction of the solvers is flipped.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bStartFromTail;

	/**
	 * The general rotation limit to be applied to bones
	 */
	UPROPERTY(meta = (Input, Constant))
	float BaseRotationLimit;

	/**
	 * Defines the limits of rotation per bone.
	 */
	UPROPERTY(meta = (Input, Constant))
	TArray<FTLLRN_RigUnit_CCDIK_RotationLimitPerItem> RotationLimits;

	/**
	 * If set to true all of the global transforms of the children
	 * of this bone will be recalculated based on their local transforms.
	 * Note: This is computationally more expensive than turning it off.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bPropagateToChildren;

	UPROPERTY()
	FTLLRN_RigUnit_CCDIK_WorkData WorkData;
};
