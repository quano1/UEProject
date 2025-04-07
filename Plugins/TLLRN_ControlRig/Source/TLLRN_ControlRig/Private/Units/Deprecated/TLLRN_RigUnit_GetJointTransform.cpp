// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Deprecated/TLLRN_RigUnit_GetJointTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "HelperUtil.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_GetJointTransform)

FTLLRN_RigUnit_GetJointTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
    
	if (UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Joint, ETLLRN_RigElementType::Bone);
		
		switch (Type)
		{
		case ETLLRN_TransformGetterType::Current:
			{
				const FTransform ComputedBaseTransform = UtilityHelpers::GetBaseTransformByMode(TransformSpace, [Hierarchy](const FTLLRN_RigElementKey& JointKey) { return Hierarchy->GetGlobalTransform(JointKey); },
				Hierarchy->GetFirstParent(Key), FTLLRN_RigElementKey(BaseJoint, ETLLRN_RigElementType::Bone), BaseTransform);

				Output = Hierarchy->GetInitialGlobalTransform(Key).GetRelativeTransform(ComputedBaseTransform);
				break;
			}
		case ETLLRN_TransformGetterType::Initial:
		default:
			{
				const FTransform ComputedBaseTransform = UtilityHelpers::GetBaseTransformByMode(TransformSpace, [Hierarchy](const FTLLRN_RigElementKey& JointKey) { return Hierarchy->GetInitialGlobalTransform(JointKey); },
				Hierarchy->GetFirstParent(Key), FTLLRN_RigElementKey(BaseJoint, ETLLRN_RigElementType::Bone), BaseTransform);

				Output = Hierarchy->GetInitialGlobalTransform(Key).GetRelativeTransform(ComputedBaseTransform);
				break;
			}
		}
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_GetJointTransform::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

