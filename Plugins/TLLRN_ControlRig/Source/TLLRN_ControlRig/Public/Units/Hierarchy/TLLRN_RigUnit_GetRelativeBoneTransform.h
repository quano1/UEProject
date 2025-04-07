// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_GetRelativeBoneTransform.generated.h"

/**
 * GetBoneTransform is used to retrieve a single transform from a hierarchy.
 */
USTRUCT(meta=(DisplayName="Get Relative Transform", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords = "GetRelativeBoneTransform", Deprecated = "4.25", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetRelativeBoneTransform : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetRelativeBoneTransform()
		: CachedBone(FTLLRN_CachedTLLRN_RigElement())
		, CachedSpace(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Bone to retrieve the transform for.
	 */
	UPROPERTY(meta = (Input))
	FName Bone;

	/**
	 * The name of the Bone to retrieve the transform relative within.
	 */
	UPROPERTY(meta = (Input))
	FName Space;

	// The current transform of the given bone - or identity in case it wasn't found.
	UPROPERTY(meta=(Output))
	FTransform Transform;

	// Used to cache the internally used bone index
	UPROPERTY(transient)
	FTLLRN_CachedTLLRN_RigElement CachedBone;

	// Used to cache the internally used space index
	UPROPERTY(transient)
	FTLLRN_CachedTLLRN_RigElement CachedSpace;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
