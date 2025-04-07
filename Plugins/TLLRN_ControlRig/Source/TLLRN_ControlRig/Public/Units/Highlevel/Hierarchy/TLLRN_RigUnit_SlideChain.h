// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/Highlevel/TLLRN_RigUnit_HighlevelBase.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "TLLRN_RigUnit_SlideChain.generated.h"

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SlideChain_WorkData
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SlideChain_WorkData()
	{
		ChainLength = 0.f;
	}

	UPROPERTY()
	float ChainLength;

	UPROPERTY()
	TArray<float> ItemSegments;

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> CachedItems;

	UPROPERTY()
	TArray<FTransform> Transforms;

	UPROPERTY()
	TArray<FTransform> BlendedTransforms;
};

/**
 * Slides an existing chain along itself with control over extrapolation.
 */
USTRUCT(meta=(DisplayName="Slide Chain", Category="Hierarchy", Keywords="Fit,Refit", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SlideChain: public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SlideChain()
	{
		StartBone = EndBone = NAME_None;
		SlideAmount = 0.f;
		bPropagateToChildren = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/** 
	 * The name of the first bone to slide
	 */
	UPROPERTY(meta = (Input))
	FName StartBone;

	/** 
	 * The name of the last bone to slide
	 */
	UPROPERTY(meta = (Input))
	FName EndBone;

	/** 
	 * The amount of sliding. This unit is multiple of the chain length.
	 */
	UPROPERTY(meta = (Input))
	float SlideAmount;

	/**
	 * If set to true all of the global transforms of the children
	 * of this bone will be recalculated based on their local transforms.
	 * Note: This is computationally more expensive than turning it off.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bPropagateToChildren;

	UPROPERTY(transient)
	FTLLRN_RigUnit_SlideChain_WorkData WorkData;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Slides an existing chain along itself with control over extrapolation.
 */
USTRUCT(meta=(DisplayName="Slide Chain", Category="Hierarchy", Keywords="Fit,Refit", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SlideChainPerItem: public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SlideChainPerItem()
	{
		SlideAmount = 0.f;
		bPropagateToChildren = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/** 
	 * The items to slide
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Items;

	/** 
	 * The amount of sliding. This unit is multiple of the chain length.
	 */
	UPROPERTY(meta = (Input))
	float SlideAmount;

	/**
	 * If set to true all of the global transforms of the children
	 * of this bone will be recalculated based on their local transforms.
	 * Note: This is computationally more expensive than turning it off.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bPropagateToChildren;

	UPROPERTY(transient)
	FTLLRN_RigUnit_SlideChain_WorkData WorkData;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Slides an existing chain along itself with control over extrapolation.
 */
USTRUCT(meta=(DisplayName="Slide Chain", Category="Hierarchy", Keywords="Fit,Refit"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SlideChainItemArray: public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SlideChainItemArray()
	{
		SlideAmount = 0.f;
		bPropagateToChildren = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/** 
	 * The items to slide
	 */
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> Items;

	/** 
	 * The amount of sliding. This unit is multiple of the chain length.
	 */
	UPROPERTY(meta = (Input))
	float SlideAmount;

	/**
	 * If set to true all of the global transforms of the children
	 * of this bone will be recalculated based on their local transforms.
	 * Note: This is computationally more expensive than turning it off.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bPropagateToChildren;

	UPROPERTY(transient)
	FTLLRN_RigUnit_SlideChain_WorkData WorkData;
};
