// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetBoneTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetBoneTransform)

FTLLRN_RigUnit_SetBoneTransform_Execute()
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
					if (FMath::IsNearlyEqual(Weight, 1.f))
					{
						Result = Transform;
					}
					else
					{
						float T = FMath::Clamp<float>(Weight, 0.f, 1.f);
						const FTransform PreviousTransform = Hierarchy->GetGlobalTransform(CachedBone);
						Result = FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, Transform, T);
					}
					Hierarchy->SetGlobalTransform(CachedBone, Result, bPropagateToChildren);
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					if (FMath::IsNearlyEqual(Weight, 1.f))
					{
						Result = Transform;
					}
					else
					{
						float T = FMath::Clamp<float>(Weight, 0.f, 1.f);
						const FTransform PreviousTransform = Hierarchy->GetLocalTransform(CachedBone);
						Result = FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, Transform, T);
					}
					Hierarchy->SetLocalTransform(CachedBone, Result, bPropagateToChildren);
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

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SetBoneTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_SetTransform NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone);
	NewNode.Space = Space;
	NewNode.Value = Transform;
	NewNode.Weight = Weight;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Bone"), TEXT("Item.Name"));
	Info.AddRemappedPin(TEXT("Transform"), TEXT("Value"));
	return Info;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_SetBoneTransform)
{
	const FTLLRN_RigElementKey Root = Controller->AddBone(TEXT("Root"), FTLLRN_RigElementKey(), FTransform(FVector(1.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneA = Controller->AddBone(TEXT("BoneA"), Root, FTransform(FVector(1.f, 2.f, 3.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneB = Controller->AddBone(TEXT("BoneB"), BoneA, FTransform(FVector(1.f, 5.f, 3.f)), true, ETLLRN_RigBoneType::User);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Bone = TEXT("Root");
	Unit.Space = ERigVMTransformSpace::GlobalSpace;
	Unit.Transform = FTransform(FVector(0.f, 0.f, 7.f));
	Unit.bPropagateToChildren = false;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(0.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(1.f, 2.f, 3.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(1.f, 5.f, 3.f)), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Space = ERigVMTransformSpace::LocalSpace;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(0.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(1.f, 2.f, 3.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(1.f, 5.f, 3.f)), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.bPropagateToChildren = true;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(0.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(0.f, 2.f, 10.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(0.f, 5.f, 10.f)), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Bone = TEXT("BoneA");
	Unit.Space = ERigVMTransformSpace::GlobalSpace;
	Unit.bPropagateToChildren = false;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(0.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(1.f, 5.f, 3.f)), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Space = ERigVMTransformSpace::LocalSpace;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(1.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(1.f, 5.f, 3.f)), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.bPropagateToChildren = true;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(1.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(1.f, 3.f, 7.f)), TEXT("unexpected transform"));

	return true;
}
#endif
