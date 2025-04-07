// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetRelativeBoneTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetRelativeTransform.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetRelativeBoneTransform)

FTLLRN_RigUnit_SetRelativeBoneTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey BoneKey(Bone, ETLLRN_RigElementType::Bone);
		const FTLLRN_RigElementKey SpaceKey(Space, ETLLRN_RigElementType::Bone);
		if (!CachedBone.UpdateCache(BoneKey, Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Bone '%s' is not valid."), *Bone.ToString());
		}
		else if (!CachedSpaceIndex.UpdateCache(SpaceKey, Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Bone '%s' is not valid."), *Bone.ToString());
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Space '%s' is not valid."), *Space.ToString());
		}
		else
		{
			const FTransform SpaceTransform = Hierarchy->GetGlobalTransform(CachedSpaceIndex);
			FTransform TargetTransform = Transform * SpaceTransform;

			if (!FMath::IsNearlyEqual(Weight, 1.f))
			{
				float T = FMath::Clamp<float>(Weight, 0.f, 1.f);
				const FTransform PreviousTransform = Hierarchy->GetGlobalTransform(CachedBone);
				TargetTransform = FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, TargetTransform, T);
			}

			Hierarchy->SetGlobalTransform(CachedBone, TargetTransform, bPropagateToChildren);
		}
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SetRelativeBoneTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_SetRelativeTransformForItem NewNode;
	NewNode.Parent = FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Bone);
	NewNode.Child = FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone);
	NewNode.Value = Transform;
	NewNode.Weight = Weight;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Space"), TEXT("Parent.Name"));
	Info.AddRemappedPin(TEXT("Bone"), TEXT("Child.Name"));
	Info.AddRemappedPin(TEXT("RelativeTransform"), TEXT("Transform"));
	return Info;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_SetRelativeBoneTransform)
{
	const FTLLRN_RigElementKey Root = Controller->AddBone(TEXT("Root"), FTLLRN_RigElementKey(), FTransform(FVector(1.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneA = Controller->AddBone(TEXT("BoneA"), Root, FTransform(FVector(1.f, 2.f, 3.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneB = Controller->AddBone(TEXT("BoneB"), BoneA, FTransform(FVector(1.f, 5.f, 3.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneC = Controller->AddBone(TEXT("BoneC"), Root, FTransform(FVector(-4.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Bone = TEXT("BoneA");
	Unit.Space = TEXT("Root");
	Unit.Transform = FTransform(FVector(0.f, 0.f, 7.f));
	Unit.bPropagateToChildren = false;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(1.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(1.f, 5.f, 3.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(3).GetTranslation().Equals(FVector(-4.f, 0.f, 0.f)), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.bPropagateToChildren = true;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(1.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(1.f, 3.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(3).GetTranslation().Equals(FVector(-4.f, 0.f, 0.f)), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Space = TEXT("BoneC");
	Unit.bPropagateToChildren = false;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(-4.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(1.f, 5.f, 3.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(3).GetTranslation().Equals(FVector(-4.f, 0.f, 0.f)), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.bPropagateToChildren = true;
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(0).GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(-4.f, 0.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(2).GetTranslation().Equals(FVector(-4.f, 3.f, 7.f)), TEXT("unexpected transform"));
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(3).GetTranslation().Equals(FVector(-4.f, 0.f, 0.f)), TEXT("unexpected transform"));

	return true;
}
#endif
