// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_ParentSwitchConstraint.generated.h"


/**
 * The Parent Switch Constraint is used to have an item follow one of multiple parents,
 * and allowing to switch between the parent in question.
 */
USTRUCT(meta=(DisplayName="Parent Switch Constraint", Category="Hierarchy", DocumentationPolicy="Strict", Keywords = "SpaceSwitch", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ParentSwitchConstraint : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ParentSwitchConstraint()
		: Subject(NAME_None, ETLLRN_RigElementType::Null)
		, ParentIndex(0)
		, Weight(1.0f)
		, Switched(false)
		, CachedSubject(FTLLRN_CachedTLLRN_RigElement())
		, CachedParent(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The subject to constrain
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Subject;

	/**
	 * The parent index to use for constraining the subject
	 */
	UPROPERTY(meta = (Input))
	int32 ParentIndex;

	/**
	 * The list of parents to constrain to
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Parents;

	/**
	 * The initial global transform for the subject
	 */
	UPROPERTY(meta = (Input))
	FTransform InitialGlobalTransform;

	/**
	 * The weight of the change - how much the change should be applied
	 */
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	/**
	 * The transform result (full without weighting)
	 */
	UPROPERTY(meta = (Output))
	FTransform Transform;

	/**
	 * Returns true if the parent has changed
	 */
	UPROPERTY(meta = (Output))
	bool Switched;

	// Used to cache the internally used subject
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedSubject;

	// Used to cache the internally used parent
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedParent;

	// The cached relative offset between subject and parent
	UPROPERTY()
	FTransform RelativeOffset;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * The Parent Switch Constraint is used to have an item follow one of multiple parents,
 * and allowing to switch between the parent in question.
 */
USTRUCT(meta=(DisplayName="Parent Switch Constraint", Category="Constraints", DocumentationPolicy="Strict", Keywords = "SpaceSwitch", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ParentSwitchConstraintArray : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ParentSwitchConstraintArray()
		: Subject(NAME_None, ETLLRN_RigElementType::Null)
		, ParentIndex(0)
		, Weight(1.0f)
		, Switched(false)
		, CachedSubject(FTLLRN_CachedTLLRN_RigElement())
		, CachedParent(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The subject to constrain
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Subject;

	/**
	 * The parent index to use for constraining the subject
	 */
	UPROPERTY(meta = (Input))
	int32 ParentIndex;

	/**
	 * The list of parents to constrain to
	 */
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> Parents;

	/**
	 * The initial global transform for the subject
	 */
	UPROPERTY(meta = (Input))
	FTransform InitialGlobalTransform;

	/**
	 * The weight of the change - how much the change should be applied
	 */
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	/**
	 * The transform result (full without weighting)
	 */
	UPROPERTY(meta = (Output))
	FTransform Transform;

	/**
	 * Returns true if the parent has changed
	 */
	UPROPERTY(meta = (Output))
	bool Switched;

	// Used to cache the internally used subject
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedSubject;

	// Used to cache the internally used parent
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedParent;

	// The cached relative offset between subject and parent
	UPROPERTY()
	FTransform RelativeOffset;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
