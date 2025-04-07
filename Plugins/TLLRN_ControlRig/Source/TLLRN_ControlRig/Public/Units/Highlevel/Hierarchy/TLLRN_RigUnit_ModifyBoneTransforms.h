// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "Units/Highlevel/TLLRN_RigUnit_HighlevelBase.h"
#include "TLLRN_RigUnit_ModifyTransforms.h"
#include "TLLRN_RigUnit_ModifyBoneTransforms.generated.h"

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ModifyBoneTransforms_PerBone
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ModifyBoneTransforms_PerBone()
		: Bone(NAME_None)
	{
	}

	/**
	 * The name of the Bone to set the transform for.
	 */
	UPROPERTY(EditAnywhere, meta = (Input), Category = FTLLRN_RigUnit_ModifyBoneTransforms_PerBone)
	FName Bone;

	/**
	 * The transform value to set for the given Bone.
	 */
	UPROPERTY(EditAnywhere, meta = (Input), Category = FTLLRN_RigUnit_ModifyBoneTransforms_PerBone)
	FTransform Transform;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ModifyBoneTransforms_WorkData : public FTLLRN_RigUnit_ModifyTransforms_WorkData
{
	GENERATED_BODY()
};

/**
 * ModifyBonetransforms is used to perform a change in the hierarchy by setting one or more bones' transforms.
 */
USTRUCT(meta=(DisplayName="Modify Transforms", Category="Hierarchy", DocumentationPolicy="Strict", Keywords = "ModifyBone", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ModifyBoneTransforms : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ModifyBoneTransforms()
		: Weight(1.f)
		, WeightMinimum(0.f)
		, WeightMaximum(1.f)
		, Mode(ETLLRN_ControlRigModifyBoneMode::AdditiveLocal)
	{
		BoneToModify.Add(FTLLRN_RigUnit_ModifyBoneTransforms_PerBone());
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The bones to modify.
	 */
	UPROPERTY(meta = (Input, ExpandByDefault, DefaultArraySize=1))
	TArray<FTLLRN_RigUnit_ModifyBoneTransforms_PerBone> BoneToModify;

	/**
	 * At 1 this sets the transform, between 0 and 1 the transform is blended with previous results.
	 */
	UPROPERTY(meta = (Input, ClampMin=0.f, ClampMax=1.f, UIMin = 0.f, UIMax = 1.f))
	float Weight;

	/**
	 * The minimum of the weight - defaults to 0.0
	 */
	UPROPERTY(meta = (Input, Constant, ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f))
	float WeightMinimum;

	/**
	 * The maximum of the weight - defaults to 1.0
	 */
	UPROPERTY(meta = (Input, Constant, ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f))
	float WeightMaximum;

	/**
	 * Defines if the bone's transform should be set
	 * in local or global space, additive or override.
	 */
	UPROPERTY(meta = (Input, Constant))
	ETLLRN_ControlRigModifyBoneMode Mode;

	// Used to cache the internally used bone index
	UPROPERTY(transient)
	FTLLRN_RigUnit_ModifyBoneTransforms_WorkData WorkData;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
