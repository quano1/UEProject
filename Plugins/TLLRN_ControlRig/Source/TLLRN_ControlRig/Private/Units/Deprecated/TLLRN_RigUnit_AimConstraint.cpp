// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Deprecated/TLLRN_RigUnit_AimConstraint.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "HelperUtil.h"
#include "AnimationCoreLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_AimConstraint)

FTLLRN_RigUnit_AimConstraint_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	TArray<FConstraintData>& ConstraintData = WorkData.ConstraintData;

	if (ConstraintData.Num() != TLLRN_AimTargets.Num())
	{
		ConstraintData.Reset();

		UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
		if (Hierarchy)
		{
			const FTLLRN_RigElementKey Key(Joint, ETLLRN_RigElementType::Bone); 
			int32 BoneIndex = Hierarchy->GetIndex(Key);
			if (BoneIndex != INDEX_NONE)
			{
				const int32 TargetNum = TLLRN_AimTargets.Num();
				if (TargetNum > 0)
				{
					const FTransform SourceTransform = Hierarchy->GetGlobalTransform(BoneIndex);
					for (int32 TargetIndex = 0; TargetIndex < TargetNum; ++TargetIndex)
					{
						const FTLLRN_AimTarget& Target = TLLRN_AimTargets[TargetIndex];

						int32 NewIndex = ConstraintData.AddDefaulted();
						check(NewIndex != INDEX_NONE);
						FConstraintData& NewData = ConstraintData[NewIndex];
						NewData.Constraint = FAimConstraintDescription();
						NewData.bMaintainOffset = false; // for now we don't support maintain offset for aim
						NewData.Weight = Target.Weight;
					}
				}
			}
		}
	}

	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Joint, ETLLRN_RigElementType::Bone); 
		int32 BoneIndex = Hierarchy->GetIndex(Key);
		if (BoneIndex != INDEX_NONE)
		{
			const int32 TargetNum = TLLRN_AimTargets.Num();
			if (TargetNum > 0)
			{
				for (int32 ConstraintIndex= 0; ConstraintIndex< ConstraintData.Num(); ++ConstraintIndex)
				{
					FAimConstraintDescription* AimConstraintDesc = ConstraintData[ConstraintIndex].Constraint.GetTypedConstraint<FAimConstraintDescription>();
					AimConstraintDesc->LookAt_Axis = FAxis(AimVector);

					if (UpTargets.IsValidIndex(ConstraintIndex))
					{
						AimConstraintDesc->LookUp_Axis = FAxis(UpVector);
						AimConstraintDesc->bUseLookUp = UpVector.Size() > 0.f;
						AimConstraintDesc->LookUpTarget = UpTargets[ConstraintIndex].Transform.GetLocation();
					}

					ConstraintData[ConstraintIndex].CurrentTransform = TLLRN_AimTargets[ConstraintIndex].Transform;
					ConstraintData[ConstraintIndex].Weight = TLLRN_AimTargets[ConstraintIndex].Weight;
				}

				FTransform BaseTransform = FTransform::Identity;
				int32 ParentIndex = Hierarchy->GetIndex(Hierarchy->GetFirstParent(Key));
				if (ParentIndex != INDEX_NONE)
				{
					BaseTransform = Hierarchy->GetGlobalTransform(ParentIndex);
				}

				FTransform SourceTransform = Hierarchy->GetGlobalTransform(BoneIndex);

				// @todo: ignore maintain offset for now
				FTransform ConstrainedTransform = AnimationCore::SolveConstraints(SourceTransform, BaseTransform, ConstraintData);

				Hierarchy->SetGlobalTransform(BoneIndex, ConstrainedTransform);
			}
		}
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_AimConstraint::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

/*
void FTLLRN_RigUnit_AimConstraint::AddConstraintData(EAimConstraintType ConstraintType, const int32 TargetIndex, const FTransform& SourceTransform, const FTransform& InBaseTransform)
{
	const FTLLRN_ConstraintTarget& Target = Targets[TargetIndex];

	int32 NewIndex = ConstraintData.AddDefaulted();
	check(NewIndex != INDEX_NONE);
	FConstraintData& NewData = ConstraintData[NewIndex];
	NewData.Constraint = FAimConstraintDescription(ConstraintType);
	NewData.bMaintainOffset = Target.bMaintainOffset;
	NewData.Weight = Target.Weight;

	if (Target.bMaintainOffset)
	{
		NewData.SaveInverseOffset(SourceTransform, Target.Transform, InBaseTransform);
	}

	ConstraintDataToTargets.Add(NewIndex, TargetIndex);
}*/
