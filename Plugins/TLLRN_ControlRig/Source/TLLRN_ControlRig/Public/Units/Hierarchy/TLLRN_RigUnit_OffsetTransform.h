// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_OffsetTransform.generated.h"

/**
 * Offset Transform is used to add an offset to an existing transform in the hierarchy. The offset is post multiplied.
 */
USTRUCT(meta=(DisplayName="Offset Transform", Category="Transforms", DocumentationPolicy = "Strict", Keywords = "Offset,Relative,AddBoneTransform", NodeColor="0, 0.364706, 1.0", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_OffsetTransformForItem : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_OffsetTransformForItem()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, OffsetTransform(FTransform::Identity)
		, Weight(1.f)
		, bPropagateToChildren(true)
		, CachedIndex()
	{}

#if WITH_EDITOR
	virtual bool UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
	virtual bool UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
#endif
	
	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to offset the transform for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	// The transform of the item relative to its previous transform
	UPROPERTY(meta=(Input))
	FTransform OffsetTransform;

	// Defines how much the change will be applied
	UPROPERTY(meta=(Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	// If set to true children of affected items in the hierarchy
	// will follow the transform change - otherwise only the parent will move.
	UPROPERTY(meta=(Input))
	bool bPropagateToChildren;

	// Used to cache the item internally
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};
