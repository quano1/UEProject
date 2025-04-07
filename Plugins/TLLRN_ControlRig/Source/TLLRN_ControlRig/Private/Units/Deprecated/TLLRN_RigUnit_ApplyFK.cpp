// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Deprecated/TLLRN_RigUnit_ApplyFK.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "HelperUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_ApplyFK)

FTLLRN_RigUnit_ApplyFK_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Joint, ETLLRN_RigElementType::Bone);
		int32 Index = Hierarchy->GetIndex(Key);
		if (Index != INDEX_NONE)
		{
			// first filter input transform
			FTransform InputTransform = Transform;
			Filter.FilterTransform(InputTransform);

			FTransform InputBaseTransform = UtilityHelpers::GetBaseTransformByMode(
				ApplyTransformSpace,
				[Hierarchy](const FTLLRN_RigElementKey& BoneKey) { return Hierarchy->GetGlobalTransform(BoneKey); },
				Hierarchy->GetFirstParent(Key),
				FTLLRN_RigElementKey(BaseJoint, ETLLRN_RigElementType::Bone),
				BaseTransform
			);

			// now get override or additive
			// whether I'd like to apply whole thing or not
			if (TLLRN_ApplyTransformMode == ETLLRN_ApplyTransformMode::Override)
			{
				// get base transform
				FTransform ApplyTransform = InputTransform * InputBaseTransform;
				Hierarchy->SetGlobalTransform(Index, ApplyTransform);
			}
			else
			{
				// if additive, we get current transform and calculate base transform and apply in their local space
				FTransform CurrentTransform = Hierarchy->GetGlobalTransform(Index);
				FTransform LocalTransform = InputTransform * CurrentTransform.GetRelativeTransform(InputBaseTransform);
				// apply additive
				Hierarchy->SetGlobalTransform(Index, LocalTransform * InputBaseTransform);
			}
		}
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ApplyFK::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_ApplyFK)
{
	const FTLLRN_RigElementKey Root = Controller->AddBone(TEXT("Root"), FTLLRN_RigElementKey(), FTransform(FVector(1.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneA = Controller->AddBone(TEXT("BoneA"), Root, FTransform(FVector(1.f, 2.f, 3.f)), true, ETLLRN_RigBoneType::User);

	Unit.ExecuteContext = ExecuteContext;
	Unit.Joint = TEXT("BoneA");
	Unit.TLLRN_ApplyTransformMode = ETLLRN_ApplyTransformMode::Override;
	Unit.ApplyTransformSpace = ETLLRN_TransformSpaceMode::GlobalSpace;
	Unit.Transform = FTransform(FVector(0.f, 5.f, 0.f));

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(0.f, 5.f, 0.f)), TEXT("unexpected global transform"));
	AddErrorIfFalse(Hierarchy->GetLocalTransform(1).GetTranslation().Equals(FVector(-1.f, 5.f, 0.f)), TEXT("unexpected local transform"));

	Unit.TLLRN_ApplyTransformMode = ETLLRN_ApplyTransformMode::Override;
	Unit.ApplyTransformSpace = ETLLRN_TransformSpaceMode::LocalSpace;

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(1.f, 5.f, 0.f)), TEXT("unexpected global transform"));
	AddErrorIfFalse(Hierarchy->GetLocalTransform(1).GetTranslation().Equals(FVector(0.f, 5.f, 0.f)), TEXT("unexpected local transform"));

	Unit.TLLRN_ApplyTransformMode = ETLLRN_ApplyTransformMode::Additive;

	Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
	Execute();
	AddErrorIfFalse(Hierarchy->GetGlobalTransform(1).GetTranslation().Equals(FVector(1.f, 7.f, 3.f)), TEXT("unexpected global transform"));
	AddErrorIfFalse(Hierarchy->GetLocalTransform(1).GetTranslation().Equals(FVector(0.f, 7.f, 3.f)), TEXT("unexpected local transform"));
	return true;
}
#endif
