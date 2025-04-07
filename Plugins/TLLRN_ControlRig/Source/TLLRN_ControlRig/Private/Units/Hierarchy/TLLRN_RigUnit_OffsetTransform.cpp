// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_OffsetTransform.h"
#include "RigVMFunctions/Math/RigVMFunction_MathTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_OffsetTransform)

FTLLRN_RigUnit_OffsetTransformForItem_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (Weight < SMALL_NUMBER)
	{
		return;
	}

	FTransform PreviousTransform = FTransform::Identity;
	FTransform GlobalTransform = FTransform::Identity;

	if(!CachedIndex.UpdateCache(Item, ExecuteContext.Hierarchy))
	{
		return;
	}

	const FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(CachedIndex.GetElement());
	if(TransformElement == nullptr)
	{
		return;
	}

	// figure out which transform type has already been computed (is clean) to avoid compute
	ERigVMTransformSpace TransformTypeToUse = ERigVMTransformSpace::GlobalSpace;
	if(TransformElement->GetDirtyState().IsDirty(ETLLRN_RigTransformType::CurrentGlobal))
	{
		check(!TransformElement->GetDirtyState().IsDirty(ETLLRN_RigTransformType::CurrentLocal));
		TransformTypeToUse = ERigVMTransformSpace::LocalSpace;
	}

	FTLLRN_RigUnit_GetTransform::StaticExecute(ExecuteContext, Item, TransformTypeToUse, false, PreviousTransform, CachedIndex);
	FRigVMFunction_MathTransformMakeAbsolute::StaticExecute(ExecuteContext, OffsetTransform, PreviousTransform, GlobalTransform);
	FTLLRN_RigUnit_SetTransform::StaticExecute(ExecuteContext, Item, TransformTypeToUse, false, GlobalTransform, Weight, bPropagateToChildren, CachedIndex);
}

#if WITH_EDITOR

bool FTLLRN_RigUnit_OffsetTransformForItem::UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}
	
	if(InInfo->Target.Name == GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_OffsetTransformForItem, OffsetTransform))
	{
		check(InInfo.IsValid());

		if(!InInfo->bInitialized)
		{
			InInfo->Reset();
			const FTransform ParentTransform = Hierarchy->GetParentTransform(Item, false);
			InInfo->OffsetTransform = OffsetTransform.Inverse() * Hierarchy->GetLocalTransform(Item) * ParentTransform;
		}

		Hierarchy->SetControlOffsetTransform(InInfo->ControlKey, InInfo->OffsetTransform, false);
		Hierarchy->SetLocalTransform(InInfo->ControlKey, OffsetTransform, false);
		if(!InInfo->bInitialized)
		{
			Hierarchy->SetLocalTransform(InInfo->ControlKey, OffsetTransform, true);
		}
		return true;
	}
	return FTLLRN_RigUnitMutable::UpdateHierarchyForDirectManipulation(InNode, InInstance, InContext, InInfo);
}

bool FTLLRN_RigUnit_OffsetTransformForItem::UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}
	
	if(InInfo->Target.Name == GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_OffsetTransformForItem, OffsetTransform))
	{
		OffsetTransform = Hierarchy->GetLocalTransform(InInfo->ControlKey, false);
		return true;
	}
	return FTLLRN_RigUnitMutable::UpdateDirectManipulationFromHierarchy(InNode, InInstance, InContext, InInfo);
}

#endif