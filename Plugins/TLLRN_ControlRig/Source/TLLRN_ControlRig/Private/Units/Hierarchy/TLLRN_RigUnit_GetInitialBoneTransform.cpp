// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_GetInitialBoneTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_GetInitialBoneTransform)

FTLLRN_RigUnit_GetInitialBoneTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedBone.UpdateCache(FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Bone '%s' is not valid."), *Bone.ToString());
		}
		else
		{
			switch (Space)
			{
				case ERigVMTransformSpace::GlobalSpace:
				{
					Transform = Hierarchy->GetInitialGlobalTransform(CachedBone);
					break;
				}
				case ERigVMTransformSpace::LocalSpace:
				{
					Transform = Hierarchy->GetInitialLocalTransform(CachedBone);
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

FRigVMStructUpgradeInfo FTLLRN_RigUnit_GetInitialBoneTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_GetTransform NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone);
	NewNode.Space = Space;
	NewNode.bInitial = true;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Bone"), TEXT("Item.Name"));
	return Info;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_GetInitialBoneTransform)
{
	const FTLLRN_RigElementKey Root = Controller->AddBone(TEXT("Root"), FTLLRN_RigElementKey(), FTransform(FVector(1.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneA = Controller->AddBone(TEXT("BoneA"), Root, FTransform(FVector(1.f, 2.f, 3.f)), true, ETLLRN_RigBoneType::User);

	Unit.Bone = TEXT("Root");
	Unit.Space = ERigVMTransformSpace::GlobalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected global transform"));
	Unit.Space = ERigVMTransformSpace::LocalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(1.f, 0.f, 0.f)), TEXT("unexpected local transform"));

	Unit.Bone = TEXT("BoneA");
	Unit.Space = ERigVMTransformSpace::GlobalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(1.f, 2.f, 3.f)), TEXT("unexpected global transform"));
	Unit.Space = ERigVMTransformSpace::LocalSpace;
	Execute();
	AddErrorIfFalse(Unit.Transform.GetTranslation().Equals(FVector(0.f, 2.f, 3.f)), TEXT("unexpected local transform"));

	return true;
}
#endif
