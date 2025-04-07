// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_SetRelativeTransform.generated.h"

/**
 * SetRelativeTransform is used to set a single transform from a hierarchy in the space of another item
 */
USTRUCT(meta=(DisplayName="Set Relative Transform", Category="Transforms", TemplateName="Set Relative Transform", DocumentationPolicy = "Strict", Keywords = "Offset,Local", NodeColor="0, 0.364706, 1.0", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetRelativeTransformForItem : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetRelativeTransformForItem()
		: Child(NAME_None, ETLLRN_RigElementType::Bone)
		, Parent(NAME_None, ETLLRN_RigElementType::Bone)
		, bParentInitial(false)
		, Value(FTransform::Identity)
		, Weight(1.f)
		, bPropagateToChildren(true)
		, CachedChild()
		, CachedParent()
	{}

#if WITH_EDITOR
	virtual bool UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
	virtual bool UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
#endif
	
	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The child item to set the transform for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/**
	 * The parent item to use.
	 * The child transform will be set in the space of the parent.
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Parent;

	/**
	 * Defines if the parent's transform should be determined as current (false) or initial (true).
	 * Initial transforms for bones and other elements in the hierarchy represent the reference pose's value.
	 */ 
	UPROPERTY(meta = (Input))
	bool bParentInitial;

	// The transform of the child item relative to the provided parent
	UPROPERTY(meta=(Input))
	FTransform Value;

	// Defines how much the change will be applied
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	// If set to true children of affected items in the hierarchy
	// will follow the transform change - otherwise only the parent will move.
	UPROPERTY(meta=(Input))
	bool bPropagateToChildren;

	// Used to cache the child internally
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedChild;

	// Used to cache the parent internally
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedParent;
};

/**
 * SetRelativeTranslation is used to set a single translation from a hierarchy in the space of another item
 */
USTRUCT(meta=(DisplayName="Set Relative Translation", Category="Transforms", TemplateName="Set Relative Transform", DocumentationPolicy = "Strict", Keywords = "Offset,Local", NodeColor="0, 0.364706, 1.0", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetRelativeTranslationForItem : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetRelativeTranslationForItem()
		: Child(NAME_None, ETLLRN_RigElementType::Bone)
		, Parent(NAME_None, ETLLRN_RigElementType::Bone)
		, bParentInitial(false)
		, Value(FVector::ZeroVector)
		, Weight(1.f)
		, bPropagateToChildren(true)
		, CachedChild()
		, CachedParent()
	{}

#if WITH_EDITOR
	virtual bool UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
	virtual bool UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
#endif
	
	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The child item to set the transform for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/**
	 * The parent item to use.
	 * The child transform will be set in the space of the parent.
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Parent;

	/**
	 * Defines if the parent's transform should be determined as current (false) or initial (true).
	 * Initial transforms for bones and other elements in the hierarchy represent the reference pose's value.
	 */ 
	UPROPERTY(meta = (Input))
	bool bParentInitial;

	// The transform of the child item relative to the provided parent
	UPROPERTY(meta=(Input))
	FVector Value;

	// Defines how much the change will be applied
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	// If set to true children of affected items in the hierarchy
	// will follow the transform change - otherwise only the parent will move.
	UPROPERTY(meta=(Input))
	bool bPropagateToChildren;

	// Used to cache the child internally
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedChild;

	// Used to cache the parent internally
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedParent;
};

/**
 * SetRelativeRotation is used to set a single rotation from a hierarchy in the space of another item
 */
USTRUCT(meta=(DisplayName="Set Relative Rotation", Category="Transforms", TemplateName="Set Relative Transform", DocumentationPolicy = "Strict", Keywords = "Offset,Local", NodeColor="0, 0.364706, 1.0", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetRelativeRotationForItem : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetRelativeRotationForItem()
		: Child(NAME_None, ETLLRN_RigElementType::Bone)
		, Parent(NAME_None, ETLLRN_RigElementType::Bone)
		, bParentInitial(false)
		, Value(FQuat::Identity)
		, Weight(1.f)
		, bPropagateToChildren(true)
		, CachedChild()
		, CachedParent()
	{}

#if WITH_EDITOR
	virtual bool UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
	virtual bool UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
#endif
	
	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The child item to set the transform for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/**
	 * The parent item to use.
	 * The child transform will be set in the space of the parent.
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Parent;

	/**
	 * Defines if the parent's transform should be determined as current (false) or initial (true).
	 * Initial transforms for bones and other elements in the hierarchy represent the reference pose's value.
	 */ 
	UPROPERTY(meta = (Input))
	bool bParentInitial;

	// The transform of the child item relative to the provided parent
	UPROPERTY(meta=(Input))
	FQuat Value;

	// Defines how much the change will be applied
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	// If set to true children of affected items in the hierarchy
	// will follow the transform change - otherwise only the parent will move.
	UPROPERTY(meta=(Input))
	bool bPropagateToChildren;

	// Used to cache the child internally
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedChild;

	// Used to cache the parent internally
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedParent;
};
