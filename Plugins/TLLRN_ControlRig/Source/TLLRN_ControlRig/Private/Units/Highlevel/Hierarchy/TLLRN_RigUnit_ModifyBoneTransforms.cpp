// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Highlevel/Hierarchy/TLLRN_RigUnit_ModifyBoneTransforms.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_ModifyBoneTransforms)

FTLLRN_RigUnit_ModifyBoneTransforms_Execute()
{
	TArray<FTLLRN_RigUnit_ModifyTransforms_PerItem> ItemsToModify;
	for (int32 BoneIndex = 0; BoneIndex < BoneToModify.Num(); BoneIndex++)
	{
		FTLLRN_RigUnit_ModifyTransforms_PerItem ItemToModify;
		ItemToModify.Item = FTLLRN_RigElementKey(BoneToModify[BoneIndex].Bone, ETLLRN_RigElementType::Bone);
		ItemToModify.Transform = BoneToModify[BoneIndex].Transform;
		ItemsToModify.Add(ItemToModify);
	}

	FTLLRN_RigUnit_ModifyTransforms::StaticExecute(
		ExecuteContext,
		ItemsToModify,
		Weight,
		WeightMinimum,
		WeightMaximum,
		Mode,
		WorkData);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ModifyBoneTransforms::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_ModifyBoneTransforms)
{
	const FTLLRN_RigElementKey Root = Controller->AddBone(TEXT("Root"), FTLLRN_RigElementKey(), FTransform(FVector(1.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneA = Controller->AddBone(TEXT("BoneA"), Root, FTransform(FVector(1.f, 2.f, 3.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneB = Controller->AddBone(TEXT("BoneB"), Root, FTransform(FVector(5.f, 6.f, 7.f)), true, ETLLRN_RigBoneType::User);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();

	Unit.BoneToModify.SetNumZeroed(2);
	Unit.BoneToModify[0].Bone = TEXT("BoneA");
	Unit.BoneToModify[1].Bone = TEXT("BoneB");
	Unit.BoneToModify[0].Transform = Unit.BoneToModify[1].Transform = FTransform(FVector(10.f, 11.f, 12.f));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Mode = ETLLRN_ControlRigModifyBoneMode::AdditiveLocal;
	Execute();
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(0).GetTranslation() - FVector(1.f, 0.f, 0.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(1).GetTranslation() - FVector(11.f, 13.f, 15.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(2).GetTranslation() - FVector(15.f, 17.f, 19.f)).IsNearlyZero(), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Mode = ETLLRN_ControlRigModifyBoneMode::AdditiveGlobal;
	Execute();
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(0).GetTranslation() - FVector(1.f, 0.f, 0.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(1).GetTranslation() - FVector(11.f, 13.f, 15.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(2).GetTranslation() - FVector(15.f, 17.f, 19.f)).IsNearlyZero(), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Mode = ETLLRN_ControlRigModifyBoneMode::OverrideLocal;
	Execute();
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(0).GetTranslation() - FVector(1.f, 0.f, 0.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(1).GetTranslation() - FVector(11.f, 11.f, 12.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(2).GetTranslation() - FVector(11.f, 11.f, 12.f)).IsNearlyZero(), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Mode = ETLLRN_ControlRigModifyBoneMode::OverrideGlobal;
	Execute();
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(0).GetTranslation() - FVector(1.f, 0.f, 0.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(1).GetTranslation() - FVector(10.f, 11.f, 12.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(2).GetTranslation() - FVector(10.f, 11.f, 12.f)).IsNearlyZero(), TEXT("unexpected transform"));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Unit.Mode = ETLLRN_ControlRigModifyBoneMode::AdditiveLocal;
	Unit.Weight = 0.5f;
	Execute();
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(0).GetTranslation() - FVector(1.f, 0.f, 0.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(1).GetTranslation() - FVector(6.f, 7.5f, 9.f)).IsNearlyZero(), TEXT("unexpected transform"));
	AddErrorIfFalse((Hierarchy->GetGlobalTransform(2).GetTranslation() - FVector(10.f, 11.5f, 13.f)).IsNearlyZero(), TEXT("unexpected transform"));


	return true;
}
#endif
