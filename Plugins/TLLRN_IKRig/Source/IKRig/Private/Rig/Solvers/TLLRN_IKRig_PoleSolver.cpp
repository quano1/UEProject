// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rig/Solvers/TLLRN_IKRig_PoleSolver.h"
#include "Rig/TLLRN_IKRigDataTypes.h"
#include "Rig/TLLRN_IKRigSkeleton.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRig_PoleSolver)

#define LOCTEXT_NAMESPACE "TLLRN_IKRig_PoleSolver"

UTLLRN_IKRig_PoleSolver::UTLLRN_IKRig_PoleSolver()
{
	Effector = CreateDefaultSubobject<UTLLRN_IKRig_PoleSolverEffector>(TEXT("Effector"));
}

void UTLLRN_IKRig_PoleSolver::Initialize(const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton)
{
	Chain.Empty();
	
	int32 BoneIndex = TLLRN_IKRigSkeleton.GetBoneIndexFromName(EndName);
	const int32 RootIndex = TLLRN_IKRigSkeleton.GetBoneIndexFromName(RootName);

	if (BoneIndex == INDEX_NONE || RootIndex == INDEX_NONE)
	{
		return;
	}

	// populate chain
	Chain.Add(BoneIndex);
	BoneIndex = TLLRN_IKRigSkeleton.GetParentIndex(BoneIndex);
	while (BoneIndex != INDEX_NONE && BoneIndex >= RootIndex)
	{
		Chain.Add(BoneIndex);
		BoneIndex = TLLRN_IKRigSkeleton.GetParentIndex(BoneIndex);
	};

	// if chain is not long enough
	if (Chain.Num() < 3)
	{
		Chain.Empty();
		return;
	}
	
	// sort the chain from root to end
	Algo::Reverse(Chain);
	
	// store children that needs propagation once solved
	TArray<int32> Children;
	for (int32 Index = 0; Index < Chain.Num()-1; ++Index)
	{
		// store children if not already handled by the solver
		TLLRN_IKRigSkeleton.GetChildIndices(Chain[Index], Children);
		const int32 NextIndex = Chain[Index+1];
		for (const int32 ChildIndex: Children)
		{
			if (ChildIndex != NextIndex)
			{
				ChildrenToUpdate.Add(ChildIndex);
				GatherChildren(ChildIndex, TLLRN_IKRigSkeleton, ChildrenToUpdate);
			}
		}
	}
}

void UTLLRN_IKRig_PoleSolver::Solve(FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton, const FTLLRN_IKRigGoalContainer& Goals)
{
	if (Chain.Num() < 3)
	{
		return;
	}
	
	const FTLLRN_IKRigGoal* IKGoal = Goals.FindGoalByName(Effector->GoalName);
	if (!IKGoal)
	{
		return;
	}

	const bool bHasAlpha = Effector->Alpha > KINDA_SMALL_NUMBER;
	if (!bHasAlpha)
	{
		return;
	}

	const int32 RootIndex = Chain[0];
	const int32 KneeIndex = Chain[1];
	const int32 EndIndex = Chain.Last();

	TArray<FTransform>& InOutTransforms = TLLRN_IKRigSkeleton.CurrentPoseGlobal;

	// initial configuration
	const FVector RootLocation = InOutTransforms[RootIndex].GetLocation();
	const FVector KneeLocation = InOutTransforms[KneeIndex].GetLocation();
	const FVector EndLocation = InOutTransforms[EndIndex].GetLocation();
	
	const FVector RootToEnd = (EndLocation - RootLocation).GetSafeNormal();
	const FVector RootToKnee = (KneeLocation - RootLocation).GetSafeNormal();
	if (RootToEnd.IsZero() || RootToKnee.IsZero())
	{
		return;
	}
	const FVector InitPlane = FVector::CrossProduct(RootToEnd, RootToKnee).GetSafeNormal();

	// target configuration
	const FVector& GoalLocation = IKGoal->FinalBlendedPosition;
	const FVector RootToPole = (GoalLocation - RootLocation).GetSafeNormal();
	if (RootToPole.IsZero())
	{
		return;
	}
	const FVector TargetPlane = FVector::CrossProduct(RootToEnd, RootToPole).GetSafeNormal();

	// compute delta rotation from InitialPlane to TargetPlane
	if (InitPlane.IsZero() || InitPlane.Equals(TargetPlane))
	{
    	return;
    }
	
	const FQuat DeltaRotation = FQuat::FindBetweenNormals(InitPlane, TargetPlane);
	if (!DeltaRotation.IsIdentity())
	{
		// update transforms
		for (int32 Index = 0; Index < Chain.Num()-1; Index++)
		{
			int32 BoneIndex = Chain[Index];
			FTransform& BoneTransform = InOutTransforms[BoneIndex];

			// rotation
			const FQuat BoneRotation = BoneTransform.GetRotation();
			const FQuat TargetRotation = FMath::Lerp(BoneRotation, DeltaRotation * BoneRotation, Effector->Alpha);
			BoneTransform.SetRotation(TargetRotation);

			// translation
			const FVector BoneTranslation = BoneTransform.GetLocation();
			const FVector TargetTranslation = FMath::Lerp( BoneTranslation, RootLocation + DeltaRotation.RotateVector(BoneTranslation - RootLocation), Effector->Alpha);
			BoneTransform.SetTranslation(TargetTranslation);
		}

		// propagate to children
		for (const int32 ChildIndex: ChildrenToUpdate)
		{
			TLLRN_IKRigSkeleton.UpdateGlobalTransformFromLocal(ChildIndex);
		}
	}
}

void UTLLRN_IKRig_PoleSolver::UpdateSolverSettings(UTLLRN_IKRigSolver* InSettings)
{
	if (UTLLRN_IKRig_PoleSolver* Settings = Cast<UTLLRN_IKRig_PoleSolver>(InSettings))
	{
		Effector->Alpha = Settings->Effector->Alpha;
	}
}

void UTLLRN_IKRig_PoleSolver::RemoveGoal(const FName& GoalName)
{
	if (Effector->GoalName == GoalName)
	{
		Effector->Modify();
		Effector->GoalName = NAME_None;
		Effector->BoneName = NAME_None;
	}
}

bool UTLLRN_IKRig_PoleSolver::IsGoalConnected(const FName& GoalName) const
{
	return (Effector->GoalName == GoalName);
}

#if WITH_EDITOR

FText UTLLRN_IKRig_PoleSolver::GetNiceName() const
{
	return FText(LOCTEXT("SolverName", "Pole Solver"));
}

bool UTLLRN_IKRig_PoleSolver::GetWarningMessage(FText& OutWarningMessage) const
{
	if (RootName == NAME_None)
	{
		OutWarningMessage = LOCTEXT("MissingRoot", "Missing root bone.");
		return true;
	}
	
	if (EndName == NAME_None)
	{
		OutWarningMessage = LOCTEXT("MissingEnd", "Missing end bone.");
		return true;
	}
	
	if (Effector->GoalName == NAME_None)
	{
		OutWarningMessage = LOCTEXT("MissingGoal", "Missing goal.");
		return true;
	}
	
	if (Chain.Num() < 3)
	{
		OutWarningMessage = LOCTEXT("Requires3BonesChain", "Requires at least 3 bones between root and end bones.");
		return true;
	}

	return false;
}

void UTLLRN_IKRig_PoleSolver::AddGoal(const UTLLRN_IKRigEffectorGoal& NewGoal)
{
	Effector->Modify();
	Effector->GoalName = NewGoal.GoalName;
	Effector->BoneName = NewGoal.BoneName;
}

void UTLLRN_IKRig_PoleSolver::RenameGoal(const FName& OldName, const FName& NewName)
{
	if (Effector->GoalName == OldName)
	{
		Effector->Modify();
		Effector->GoalName = NewName;
	}
}

void UTLLRN_IKRig_PoleSolver::SetGoalBone(const FName& GoalName, const FName& NewBoneName)
{
	if (Effector->GoalName == GoalName)
	{
		Effector->Modify();
		Effector->BoneName = NewBoneName;
	}
}

UObject* UTLLRN_IKRig_PoleSolver::GetGoalSettings(const FName& GoalName) const
{
	return (Effector->GoalName == GoalName) ? Effector : nullptr;
}

bool UTLLRN_IKRig_PoleSolver::IsBoneAffectedBySolver(const FName& BoneName, const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) const
{
	const bool bAffected = TLLRN_IKRigSkeleton.IsBoneInDirectLineage(BoneName, RootName);
	if (!bAffected)
	{
		return false;
	}
	
	const int32 EndIndex = Chain.IsEmpty() ? INDEX_NONE : Chain.Last();
	if (EndIndex == INDEX_NONE)
	{
		return false;
	}

	const int32 ChildIndex = TLLRN_IKRigSkeleton.GetBoneIndexFromName(BoneName);
	return ChildIndex <= EndIndex;
}

void UTLLRN_IKRig_PoleSolver::SetRootBone(const FName& RootBoneName)
{
	RootName = RootBoneName;
}

void UTLLRN_IKRig_PoleSolver::SetEndBone(const FName& EndBoneName)
{
	EndName = EndBoneName;
}

FName UTLLRN_IKRig_PoleSolver::GetEndBone() const
{
	return EndName;
}

#endif

void UTLLRN_IKRig_PoleSolver::GatherChildren(const int32 BoneIndex, const FTLLRN_IKRigSkeleton& InSkeleton, TArray<int32>& OutChildren)
{
	TArray<int32> Children;
	InSkeleton.GetChildIndices(BoneIndex, Children);
	for (int32 ChildIndex: Children)
	{
		OutChildren.Add(ChildIndex);
		GatherChildren(ChildIndex, InSkeleton, OutChildren);
	}
}

#undef LOCTEXT_NAMESPACE

