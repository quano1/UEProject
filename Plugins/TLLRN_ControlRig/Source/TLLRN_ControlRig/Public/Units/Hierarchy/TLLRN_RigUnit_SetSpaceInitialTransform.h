// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_SetSpaceInitialTransform.generated.h"


/**
 * SetSpaceInitialTransform is used to perform a change in the hierarchy by setting a single space's initial transform.
 */
USTRUCT(meta=(DisplayName="Set Initial Space Transform", Category="Setup", DocumentationPolicy="Strict", Keywords = "SetSpaceInitialTransform", Deprecated="4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetSpaceInitialTransform : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetSpaceInitialTransform()
		: Space(ERigVMTransformSpace::GlobalSpace)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Space to set the transform for.
	 */
	UPROPERTY(meta = (Input))
	FName SpaceName;

	/**
	 * The transform value to set for the given space.
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

	// user to internally cache the index of the bone
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedSpaceIndex;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
