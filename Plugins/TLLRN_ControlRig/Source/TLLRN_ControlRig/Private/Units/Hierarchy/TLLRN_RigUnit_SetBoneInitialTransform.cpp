// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetBoneInitialTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetBoneInitialTransform)

FTLLRN_RigUnit_SetBoneInitialTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Bone, ETLLRN_RigElementType::Bone);
		if (!CachedBone.UpdateCache(Key, Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Bone '%s' is not valid."), *Bone.ToString());
			return;
		}

		if (Space == ERigVMTransformSpace::LocalSpace)
		{
			Hierarchy->SetInitialLocalTransform(CachedBone, Transform);
		}
		else
		{
			Hierarchy->SetInitialGlobalTransform(CachedBone, Transform);
		}
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SetBoneInitialTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_SetTransform NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone);
	NewNode.Space = Space;
	NewNode.Value = Transform;
	NewNode.bInitial = true;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Bone"), TEXT("Item.Name"));
	Info.AddRemappedPin(TEXT("Transform"), TEXT("Value"));
	return Info;
}

