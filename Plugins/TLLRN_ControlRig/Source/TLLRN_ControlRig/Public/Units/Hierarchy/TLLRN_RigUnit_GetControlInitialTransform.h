// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_GetControlInitialTransform.generated.h"

/**
 * GetControlTransform is used to retrieve a single transform from a hierarchy.
 */
USTRUCT(meta=(DisplayName="Get Control Initial Transform", Category="Controls", DocumentationPolicy = "Strict", Keywords="GetControlInitialTransform", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetControlInitialTransform : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetControlInitialTransform()
		: Space(ERigVMTransformSpace::LocalSpace)
		, CachedControlIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to retrieve the transform for.
	 */
	UPROPERTY(meta = (Input))
	FName Control;

	/**
	 * Defines if the Control's transform should be retrieved
	 * in local or global space.
	 */ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	// The current transform of the given bone - or identity in case it wasn't found.
	UPROPERTY(meta=(Output))
	FTransform Transform;

	// Used to cache the internally used bone index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedControlIndex;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
