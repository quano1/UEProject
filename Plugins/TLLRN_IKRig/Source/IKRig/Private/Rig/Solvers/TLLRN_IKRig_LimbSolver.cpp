// Copyright Epic Games, Inc. All Rights Reserved.


#include "Rig/Solvers/TLLRN_IKRig_LimbSolver.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRig_LimbSolver)

#define LOCTEXT_NAMESPACE "TLLRN_IKRig_LimbSolver"

UTLLRN_IKRig_LimbSolver::UTLLRN_IKRig_LimbSolver()
{
	Effector = CreateDefaultSubobject<UTLLRN_IKRig_LimbEffector>(TEXT("Effector"));
}

void UTLLRN_IKRig_LimbSolver::Initialize(const FTLLRN_IKRigSkeleton& InSkeleton)
{
	Solver.Reset();
	ChildrenToUpdate.Empty();
	
	if (Effector->GoalName == NAME_None || Effector->BoneName == NAME_None || RootName == NAME_None)
	{
		return;
	}

	int32 BoneIndex = InSkeleton.GetBoneIndexFromName(Effector->BoneName);
	const int32 RootIndex = InSkeleton.GetBoneIndexFromName(RootName);
	if (BoneIndex == INDEX_NONE || RootIndex == INDEX_NONE)
	{
		return;
	}

	// populate indices
	TArray<int32> BoneIndices( {BoneIndex} );
	BoneIndex = InSkeleton.GetParentIndex(BoneIndex);
	while (BoneIndex != INDEX_NONE && BoneIndex >= RootIndex)
	{
		BoneIndices.Add(BoneIndex);
		BoneIndex = InSkeleton.GetParentIndex(BoneIndex);
	};

	// if chain is not long enough
	if (BoneIndices.Num() < 3)
	{
		return;
	}

	// sort the chain from root to end
	Algo::Reverse(BoneIndices);

	// initialize solver
	for (int32 Index: BoneIndices)
	{
		const FVector Location = InSkeleton.CurrentPoseGlobal[Index].GetLocation();
		Solver.AddLink(Location, Index);
	}

	const bool bInitialized = Solver.Initialize();
	if (bInitialized)
	{
		// store children that needs propagation once solved
		TArray<int32> Children;
		for (int32 Index = 0; Index < BoneIndices.Num()-1; ++Index)
		{
			// store children if not already handled by the solver (part if the links)
			InSkeleton.GetChildIndices(BoneIndices[Index], Children);
			const int32 NextIndex = BoneIndices[Index+1];
			for (const int32 ChildIndex: Children)
			{
				if (ChildIndex != NextIndex)
				{
					ChildrenToUpdate.Add(ChildIndex);
					GatherChildren(ChildIndex, InSkeleton, ChildrenToUpdate);
				}
			}
		}
		// store end bone children
		GatherChildren(BoneIndices.Last(), InSkeleton, ChildrenToUpdate);
	}
}

void UTLLRN_IKRig_LimbSolver::Solve(FTLLRN_IKRigSkeleton& InOutRigSkeleton, const FTLLRN_IKRigGoalContainer& InGoals)
{
	if (Solver.NumLinks() < 3)
	{
		return;
	}
	
	const FTLLRN_IKRigGoal* IKGoal = InGoals.FindGoalByName(Effector->GoalName);
	if (!IKGoal)
	{
		return;
	}

	// update settings
	FTLLRN_LimbSolverSettings Settings;
	Settings.ReachPrecision = ReachPrecision;
	Settings.MaxIterations = MaxIterations;
	Settings.bEnableLimit = bEnableLimit;
	Settings.MinRotationAngle = MinRotationAngle;
	Settings.EndBoneForwardAxis = EndBoneForwardAxis; 
	Settings.HingeRotationAxis = HingeRotationAxis;
	Settings.bEnableTwistCorrection = bEnableTwistCorrection;
	Settings.ReachStepAlpha = ReachStepAlpha;
	Settings.PullDistribution = PullDistribution;
	Settings.bAveragePull = bAveragePull;

	// update settings
	const FVector& GoalLocation = IKGoal->FinalBlendedPosition;
	const FQuat& GoalRotation = IKGoal->FinalBlendedRotation;
	const bool bModifiedLimb = Solver.Solve(
		InOutRigSkeleton.CurrentPoseGlobal,
		GoalLocation,
		GoalRotation,
		Settings);

	// propagate if needed
	if (bModifiedLimb)
	{
		// update chain bones local transform
		for (int32 Index = 0; Index < Solver.NumLinks(); Index++)
		{
			InOutRigSkeleton.UpdateLocalTransformFromGlobal(Solver.GetBoneIndex(Index));
		}

		// propagate to children
		for (const int32 ChildIndex: ChildrenToUpdate)
		{
			InOutRigSkeleton.UpdateGlobalTransformFromLocal(ChildIndex);
		}
	}
}

void UTLLRN_IKRig_LimbSolver::UpdateSolverSettings(UTLLRN_IKRigSolver* InSettings)
{
	if (UTLLRN_IKRig_LimbSolver* Settings = Cast<UTLLRN_IKRig_LimbSolver>(InSettings))
	{
		ReachPrecision = Settings->ReachPrecision;
		HingeRotationAxis = Settings->HingeRotationAxis;
		MaxIterations = Settings->MaxIterations;
		bEnableLimit = Settings->bEnableLimit;
		MinRotationAngle = Settings->MinRotationAngle;
		bAveragePull = Settings->bAveragePull;
		PullDistribution = Settings->PullDistribution;
		ReachStepAlpha = Settings->ReachStepAlpha;
		bEnableTwistCorrection = Settings->bEnableTwistCorrection;
		EndBoneForwardAxis = Settings->EndBoneForwardAxis; 
	}
}

void UTLLRN_IKRig_LimbSolver::RemoveGoal(const FName& GoalName)
{
	if (Effector->GoalName == GoalName)
	{
		Effector->Modify();
		Effector->GoalName = NAME_None;
		Effector->BoneName = NAME_None;
	}
}

bool UTLLRN_IKRig_LimbSolver::IsGoalConnected(const FName& GoalName) const
{
	return (Effector->GoalName == GoalName);
}

#if WITH_EDITOR

FText UTLLRN_IKRig_LimbSolver::GetNiceName() const
{
	return FText(LOCTEXT("SolverName", "Limb IK"));
}

bool UTLLRN_IKRig_LimbSolver::GetWarningMessage(FText& OutWarningMessage) const
{
	if (Effector->GoalName == NAME_None)
	{
		OutWarningMessage = LOCTEXT("MissingGoal", "Missing goal.");
		return true;
	}

	if (RootName == NAME_None)
	{
		OutWarningMessage = LOCTEXT("MissingRoot", "Missing root.");
		return true;
	}

	if (Solver.NumLinks() < 3)
	{
		OutWarningMessage = LOCTEXT("Requires3BonesChain", "Requires at least 3 bones between root and goal.");
		return true;
	}
	
	return false;
}

bool UTLLRN_IKRig_LimbSolver::IsBoneAffectedBySolver(const FName& BoneName, const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) const
{
	return TLLRN_IKRigSkeleton.IsBoneInDirectLineage(BoneName, RootName);
}

void UTLLRN_IKRig_LimbSolver::AddGoal(const UTLLRN_IKRigEffectorGoal& NewGoal)
{
	Effector->Modify();
	Effector->GoalName = NewGoal.GoalName;
	Effector->BoneName = NewGoal.BoneName;
}

void UTLLRN_IKRig_LimbSolver::RenameGoal(const FName& OldName, const FName& NewName)
{
	if (Effector->GoalName == OldName)
	{
		Effector->Modify();
		Effector->GoalName = NewName;
	}
}

void UTLLRN_IKRig_LimbSolver::SetGoalBone(const FName& GoalName, const FName& NewBoneName)
{
	if (Effector->GoalName == GoalName)
	{
		Effector->Modify();
		Effector->BoneName = NewBoneName;
	}
}

UObject* UTLLRN_IKRig_LimbSolver::GetGoalSettings(const FName& GoalName) const
{
	return (Effector->GoalName == GoalName) ? Effector : nullptr;
}

void UTLLRN_IKRig_LimbSolver::SetRootBone(const FName& RootBoneName)
{
	RootName = RootBoneName;
}

#endif

void UTLLRN_IKRig_LimbSolver::GatherChildren(const int32 BoneIndex, const FTLLRN_IKRigSkeleton& InSkeleton, TArray<int32>& OutChildren)
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

