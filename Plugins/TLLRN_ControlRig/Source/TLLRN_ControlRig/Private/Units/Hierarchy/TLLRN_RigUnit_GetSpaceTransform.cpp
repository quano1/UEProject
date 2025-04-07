// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_GetSpaceTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_GetSpaceTransform)

FTLLRN_RigUnit_GetSpaceTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (CachedSpaceIndex.UpdateCache(FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Null), Hierarchy))
		{
			switch (SpaceType)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					Transform = Hierarchy->GetGlobalTransform(CachedSpaceIndex);
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					Transform = Hierarchy->GetLocalTransform(CachedSpaceIndex);
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_GetSpaceTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_GetTransform NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Null);
	NewNode.Space = SpaceType;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Space"), TEXT("Item.Name"));
	Info.AddRemappedPin(TEXT("SpaceType"), TEXT("Space"));
	return Info;
}

