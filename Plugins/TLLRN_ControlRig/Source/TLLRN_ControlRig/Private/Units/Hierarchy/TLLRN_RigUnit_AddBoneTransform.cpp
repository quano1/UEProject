// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_AddBoneTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_OffsetTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_AddBoneTransform)

FTLLRN_RigUnit_AddBoneTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Bone, ETLLRN_RigElementType::Bone); 
		if (!CachedBone.UpdateCache(Key, Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Bone '%s' is not valid."), *Bone.ToString());
		}
		else
		{
			FTransform TargetTransform;
			const FTransform PreviousTransform = Hierarchy->GetGlobalTransform(CachedBone);

			if (bPostMultiply)
			{
				TargetTransform = PreviousTransform * Transform;
			}
			else
			{
				TargetTransform = Transform * PreviousTransform;
			}

			if (!FMath::IsNearlyEqual(Weight, 1.f))
			{
				float T = FMath::Clamp<float>(Weight, 0.f, 1.f);
				TargetTransform = FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, TargetTransform, T);
			}

			Hierarchy->SetGlobalTransform(CachedBone, TargetTransform, bPropagateToChildren);
		}
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_AddBoneTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_OffsetTransformForItem NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone);
	NewNode.Weight = Weight;
	NewNode.bPropagateToChildren = bPropagateToChildren;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Bone"), TEXT("Item.Name"));
	return Info;
}
