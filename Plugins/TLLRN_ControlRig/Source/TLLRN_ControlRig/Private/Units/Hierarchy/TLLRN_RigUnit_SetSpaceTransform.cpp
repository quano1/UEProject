// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetSpaceTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetSpaceTransform)

FTLLRN_RigUnit_SetSpaceTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey SpaceKey(Space, ETLLRN_RigElementType::Null);

		if (CachedSpaceIndex.UpdateCache(SpaceKey, Hierarchy))
		{
			switch (SpaceType)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					if(FMath::IsNearlyEqual(Weight, 1.f))
					{
						Hierarchy->SetGlobalTransform(CachedSpaceIndex, Transform, true);
					}
					else
					{
						const FTransform PreviousTransform = Hierarchy->GetGlobalTransform(CachedSpaceIndex);
						Hierarchy->SetGlobalTransform(CachedSpaceIndex, FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, Transform, FMath::Clamp<float>(Weight, 0.f, 1.f)), true);
					}
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					if(FMath::IsNearlyEqual(Weight, 1.f))
					{
						Hierarchy->SetLocalTransform(CachedSpaceIndex, Transform, true);
					}
					else
					{
						const FTransform PreviousTransform = Hierarchy->GetLocalTransform(CachedSpaceIndex);
						Hierarchy->SetLocalTransform(CachedSpaceIndex, FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, Transform, FMath::Clamp<float>(Weight, 0.f, 1.f)), true);
					}
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

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SetSpaceTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_SetTransform NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Null);
	NewNode.Space = SpaceType;
	NewNode.Value = Transform;
	NewNode.Weight = Weight;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Space"), TEXT("Item.Name"));
	Info.AddRemappedPin(TEXT("SpaceType"), TEXT("Space"));
	Info.AddRemappedPin(TEXT("Transform"), TEXT("Value"));
	return Info;
}

