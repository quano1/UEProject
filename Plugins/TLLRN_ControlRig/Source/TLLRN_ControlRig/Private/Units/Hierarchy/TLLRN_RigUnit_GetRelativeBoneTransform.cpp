// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_GetRelativeBoneTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetRelativeTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_GetRelativeBoneTransform)

FTLLRN_RigUnit_GetRelativeBoneTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedBone.UpdateCache(FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Bone '%s' is not valid."), *Bone.ToString());
		}
		else if (!CachedSpace.UpdateCache(FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Bone), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Space '%s' is not valid."), *Space.ToString());
		}
		else
		{
			const FTransform SpaceTransform = Hierarchy->GetGlobalTransform(CachedSpace);
			const FTransform BoneTransform = Hierarchy->GetGlobalTransform(CachedBone);
			Transform = BoneTransform.GetRelativeTransform(SpaceTransform);
		}
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_GetRelativeBoneTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_GetRelativeTransformForItem NewNode;
	NewNode.Parent = FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Bone);
	NewNode.Child = FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone);

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Space"), TEXT("Parent.Name"));
	Info.AddRemappedPin(TEXT("Bone"), TEXT("Child.Name"));
	Info.AddRemappedPin(TEXT("Transform"), TEXT("RelativeTransform"));
	return Info;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_GetRelativeBoneTransform)
{
	const FTLLRN_RigElementKey Root = Controller->AddBone(TEXT("Root"), FTLLRN_RigElementKey(), FTransform(FVector(1.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneA = Controller->AddBone(TEXT("BoneA"), Root, FTransform(FVector(1.f, 2.f, 3.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneB = Controller->AddBone(TEXT("BoneB"), Root, FTransform(FVector(-4.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);

	Unit.Bone = TEXT("Unknown");
	Unit.Space = TEXT("Root");
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(0.f, 0.f, 0.f)), TEXT("unexpected transform"));

	Unit.Bone = TEXT("BoneA");
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(0.f, 2.f, 3.f)), TEXT("unexpected transform"));

	Unit.Space = TEXT("BoneB");
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(5.f, 2.f, 3.f)), TEXT("unexpected transform"));

	return true;
}
#endif
