// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetRelativeTransform.h"
#include "RigVMFunctions/Math/RigVMFunction_MathTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetRelativeTransform.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetRelativeTransform)

FTLLRN_RigUnit_SetRelativeTransformForItem_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (Weight < SMALL_NUMBER)
	{
		return;
	}

	FTransform ParentTransform = FTransform::Identity;
	FTransform GlobalTransform = FTransform::Identity;

	FTLLRN_RigUnit_GetTransform::StaticExecute(ExecuteContext, Parent, ERigVMTransformSpace::GlobalSpace, bParentInitial, ParentTransform, CachedParent);
	FRigVMFunction_MathTransformMakeAbsolute::StaticExecute(ExecuteContext, Value, ParentTransform, GlobalTransform);
	FTLLRN_RigUnit_SetTransform::StaticExecute(ExecuteContext, Child, ERigVMTransformSpace::GlobalSpace, false, GlobalTransform, Weight, bPropagateToChildren, CachedChild);
}

FTLLRN_RigUnit_SetRelativeTranslationForItem_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (Weight < SMALL_NUMBER)
	{
		return;
	}

	FTransform ParentTransform = FTransform::Identity;
	FTransform LocalTransform = FTransform::Identity;
	FTransform GlobalTransform = FTransform::Identity;

	FTLLRN_RigUnit_GetTransform::StaticExecute(ExecuteContext, Parent, ERigVMTransformSpace::GlobalSpace, bParentInitial, ParentTransform, CachedParent);
	FTLLRN_RigUnit_GetRelativeTransformForItem::StaticExecute(ExecuteContext, Child, false, Parent, bParentInitial, LocalTransform, CachedChild, CachedParent);
	LocalTransform.SetTranslation(Value);
	FRigVMFunction_MathTransformMakeAbsolute::StaticExecute(ExecuteContext, LocalTransform, ParentTransform, GlobalTransform);
	FTLLRN_RigUnit_SetTransform::StaticExecute(ExecuteContext, Child, ERigVMTransformSpace::GlobalSpace, false, GlobalTransform, Weight, bPropagateToChildren, CachedChild);
}

FTLLRN_RigUnit_SetRelativeRotationForItem_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (Weight < SMALL_NUMBER)
	{
		return;
	}

	FTransform ParentTransform = FTransform::Identity;
	FTransform LocalTransform = FTransform::Identity;
	FTransform GlobalTransform = FTransform::Identity;

	FTLLRN_RigUnit_GetTransform::StaticExecute(ExecuteContext, Parent, ERigVMTransformSpace::GlobalSpace, bParentInitial, ParentTransform, CachedParent);
	FTLLRN_RigUnit_GetRelativeTransformForItem::StaticExecute(ExecuteContext, Child, false, Parent, bParentInitial, LocalTransform, CachedChild, CachedParent);
	LocalTransform.SetRotation(Value);
	LocalTransform.NormalizeRotation();
	FRigVMFunction_MathTransformMakeAbsolute::StaticExecute(ExecuteContext, LocalTransform, ParentTransform, GlobalTransform);
	FTLLRN_RigUnit_SetTransform::StaticExecute(ExecuteContext, Child, ERigVMTransformSpace::GlobalSpace, false, GlobalTransform, Weight, bPropagateToChildren, CachedChild);
}

#if WITH_EDITOR

bool FTLLRN_RigUnit_SetRelativeTransformForItem::UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}
	
	if(InInfo->Target.Name == GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_SetRelativeTransformForItem, Value))
	{
		const FTransform ParentTransform = Hierarchy->GetGlobalTransform(Parent, false);
		Hierarchy->SetControlOffsetTransform(InInfo->ControlKey, ParentTransform, false);
		Hierarchy->SetLocalTransform(InInfo->ControlKey, Value, false);
		if(!InInfo->bInitialized)
		{
			Hierarchy->SetLocalTransform(InInfo->ControlKey, Value, true);
		}
		return true;
	}
	return FTLLRN_RigUnitMutable::UpdateHierarchyForDirectManipulation(InNode, InInstance, InContext, InInfo);
}

bool FTLLRN_RigUnit_SetRelativeTransformForItem::UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}

	if(InInfo->Target.Name == GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_SetRelativeTransformForItem, Value))
	{
		Value = Hierarchy->GetLocalTransform(InInfo->ControlKey, false);
		return true;
	}
	return FTLLRN_RigUnitMutable::UpdateDirectManipulationFromHierarchy(InNode, InInstance, InContext, InInfo);
}

bool FTLLRN_RigUnit_SetRelativeTranslationForItem::UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}

	if(InInfo->Target.Name == GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_SetRelativeTranslationForItem, Value))
	{
		const FTransform ParentTransform = Hierarchy->GetGlobalTransform(Parent, false);
		Hierarchy->SetControlOffsetTransform(InInfo->ControlKey, ParentTransform, false);
		Hierarchy->SetLocalTransform(InInfo->ControlKey, FTransform(Value), false);
		if(!InInfo->bInitialized)
		{
			Hierarchy->SetLocalTransform(InInfo->ControlKey, FTransform(Value), true);
		}
		return true;
	}
	return FTLLRN_RigUnitMutable::UpdateHierarchyForDirectManipulation(InNode, InInstance, InContext, InInfo);
}

bool FTLLRN_RigUnit_SetRelativeTranslationForItem::UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}

	if(InInfo->Target.Name == GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_SetRelativeTranslationForItem, Value))
	{
		Value = Hierarchy->GetLocalTransform(InInfo->ControlKey, false).GetTranslation();
		return true;
	}
	return FTLLRN_RigUnitMutable::UpdateDirectManipulationFromHierarchy(InNode, InInstance, InContext, InInfo);
}

bool FTLLRN_RigUnit_SetRelativeRotationForItem::UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}

	if(InInfo->Target.Name == GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_SetRelativeRotationForItem, Value))
	{
		FTransform ParentTransform = Hierarchy->GetGlobalTransform(Parent, false);
		ParentTransform.SetTranslation(Hierarchy->GetGlobalTransform(Child).GetTranslation());
		Hierarchy->SetControlOffsetTransform(InInfo->ControlKey, ParentTransform, false);
		Hierarchy->SetLocalTransform(InInfo->ControlKey, FTransform(Value), false);
		if(!InInfo->bInitialized)
		{
			Hierarchy->SetLocalTransform(InInfo->ControlKey, FTransform(Value), true);
		}
		return true;
	}
	return FTLLRN_RigUnitMutable::UpdateHierarchyForDirectManipulation(InNode, InInstance, InContext, InInfo);
}

bool FTLLRN_RigUnit_SetRelativeRotationForItem::UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo)
{
	UTLLRN_RigHierarchy* Hierarchy = InContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return false;
	}

	if(InInfo->Target.Name == GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_SetRelativeRotationForItem, Value))
	{
		Value = Hierarchy->GetLocalTransform(InInfo->ControlKey, false).GetRotation();
		return true;
	}
	return FTLLRN_RigUnitMutable::UpdateDirectManipulationFromHierarchy(InNode, InInstance, InContext, InInfo);
}

#endif
