// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_GetSpaceTransform.generated.h"

/**
 * GetSpaceTransform is used to retrieve a single transform from a hierarchy.
 */
USTRUCT(meta=(DisplayName="Get Space Transform", Category="Spaces", DocumentationPolicy = "Strict", Keywords="GetSpaceTransform", Varying, Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetSpaceTransform : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetSpaceTransform()
		: SpaceType(ERigVMTransformSpace::GlobalSpace)
		, CachedSpaceIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Space to retrieve the transform for.
	 */
	UPROPERTY(meta = (Input))
	FName Space;

	/**
	 * Defines if the Space's transform should be retrieved
	 * in local or global space.
	 */ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace SpaceType;

	// The current transform of the given bone - or identity in case it wasn't found.
	UPROPERTY(meta=(Output))
	FTransform Transform;

	// Used to cache the internally used bone index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedSpaceIndex;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
