// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetBoneTranslation.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetBoneTranslation)

FTLLRN_RigUnit_SetBoneTranslation_Execute()
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
			switch (Space)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					FTransform Transform = Hierarchy->GetGlobalTransform(CachedBone);

					if (FMath::IsNearlyEqual(Weight, 1.f))
					{
						Transform.SetTranslation(Translation);
					}
					else
					{
						float T = FMath::Clamp<float>(Weight, 0.f, 1.f);
						Transform.SetTranslation(FMath::Lerp<FVector>(Transform.GetTranslation(), Translation, T));
					}

					Hierarchy->SetGlobalTransform(CachedBone, Transform, bPropagateToChildren);
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					FTransform Transform = Hierarchy->GetLocalTransform(CachedBone);

					if (FMath::IsNearlyEqual(Weight, 1.f))
					{
						Transform.SetTranslation(Translation);
					}
					else
					{
						float T = FMath::Clamp<float>(Weight, 0.f, 1.f);
						Transform.SetTranslation(FMath::Lerp<FVector>(Transform.GetTranslation(), Translation, T));
					}

					Hierarchy->SetLocalTransform(CachedBone, Transform, bPropagateToChildren);
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

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SetBoneTranslation::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_SetTranslation NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone);
	NewNode.Space = Space;
	NewNode.Value = Translation;
	NewNode.Weight = Weight;	

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Bone"), TEXT("Item.Name"));
	Info.AddRemappedPin(TEXT("Translation"), TEXT("Value"));
	return Info;
}

