// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_ProjectTransformToNewParent.generated.h"

/**
 * Gets the relative offset between the child and the old parent, then multiplies by new parent's transform.
 */
USTRUCT(meta=(DisplayName="Project to new Parent", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords = "ProjectTransformToNewParent,Relative,Reparent,Offset", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ProjectTransformToNewParent : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ProjectTransformToNewParent()
		: CachedChild(FTLLRN_CachedTLLRN_RigElement())
		, CachedOldParent(FTLLRN_CachedTLLRN_RigElement())
		, CachedNewParent(FTLLRN_CachedTLLRN_RigElement())
	{
		Child = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		bChildInitial = true;
		OldParent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		bOldParentInitial = true;
		NewParent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		bNewParentInitial = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The element to project between parents
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/**
	 * If set to true the child will be retrieved in its initial transform
	 */
	UPROPERTY(meta = (Input))
	bool bChildInitial;

	/**
	 * The original parent of the child.
	 * Can be an actual parent in the hierarchy or any other
	 * item you want to use to compute to offset against.
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey OldParent;

	/**
	 * If set to true the old parent will be retrieved in its initial transform
	 */
	UPROPERTY(meta = (Input))
	bool bOldParentInitial;

	/**
	 * The new parent of the child.
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey NewParent;

	/**
	 * If set to true the new parent will be retrieved in its initial transform
	 */
	UPROPERTY(meta = (Input))
	bool bNewParentInitial;

	// The resulting transform
	UPROPERTY(meta=(Output))
	FTransform Transform;

	// Used to cache the internally used child
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedChild;

	// Used to cache the internally used old parent
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedOldParent;

	// Used to cache the internally used new parent
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedNewParent;
};
