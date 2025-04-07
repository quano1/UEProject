// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rig/Solvers/TLLRN_IKRig_SetTransform.h"
#include "Rig/TLLRN_IKRigDataTypes.h"
#include "Rig/TLLRN_IKRigSkeleton.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRig_SetTransform)

#define LOCTEXT_NAMESPACE "TLLRN_IKRig_SetTransform"

UTLLRN_IKRig_SetTransform::UTLLRN_IKRig_SetTransform()
{
	Effector = CreateDefaultSubobject<UTLLRN_IKRig_SetTransformEffector>(TEXT("Effector"));
}

void UTLLRN_IKRig_SetTransform::Initialize(const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton)
{
	BoneIndex = TLLRN_IKRigSkeleton.GetBoneIndexFromName(RootBone);
}

void UTLLRN_IKRig_SetTransform::Solve(FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton, const FTLLRN_IKRigGoalContainer& Goals)
{
	// BoneIndex is irrelevant
	if (BoneIndex == INDEX_NONE)
	{
		return;
	}
	
	const FTLLRN_IKRigGoal* InGoal = Goals.FindGoalByName(Goal);
	if (!InGoal)
	{
		return;
	}

	// check that settings are such that there is anything to do at all
	const bool bAnythingEnabled = Effector->bEnablePosition || Effector->bEnableRotation;
	const bool bHasAlpha = Effector->Alpha > KINDA_SMALL_NUMBER;
	if (!(bAnythingEnabled && bHasAlpha))
	{
		return;
	}

	FTransform& CurrentTransform = TLLRN_IKRigSkeleton.CurrentPoseGlobal[BoneIndex];	

	if (Effector->bEnablePosition)
	{
		const FVector TargetPosition = FMath::Lerp(CurrentTransform.GetTranslation(), InGoal->FinalBlendedPosition, Effector->Alpha);
		CurrentTransform.SetLocation(TargetPosition);
	}
	
	if (Effector->bEnableRotation)
	{
		const FQuat TargetRotation = FMath::Lerp(CurrentTransform.GetRotation(), InGoal->FinalBlendedRotation, Effector->Alpha);
		CurrentTransform.SetRotation(TargetRotation);
	}
	
	TLLRN_IKRigSkeleton.PropagateGlobalPoseBelowBone(BoneIndex);
}

void UTLLRN_IKRig_SetTransform::UpdateSolverSettings(UTLLRN_IKRigSolver* InSettings)
{
	if (UTLLRN_IKRig_SetTransform* Settings = Cast<UTLLRN_IKRig_SetTransform>(InSettings))
	{
		Effector->bEnablePosition = Settings->Effector->bEnablePosition;
		Effector->bEnableRotation = Settings->Effector->bEnableRotation;
		Effector->Alpha = Settings->Effector->Alpha;
	}
}

void UTLLRN_IKRig_SetTransform::RemoveGoal(const FName& GoalName)
{
	if (Goal == GoalName)
	{
		Goal = NAME_None;
		RootBone = NAME_None;
	}	
}

bool UTLLRN_IKRig_SetTransform::IsGoalConnected(const FName& GoalName) const
{
	return Goal == GoalName;
}

#if WITH_EDITOR

FText UTLLRN_IKRig_SetTransform::GetNiceName() const
{
	return FText(LOCTEXT("SolverName", "Set Transform"));
}

bool UTLLRN_IKRig_SetTransform::GetWarningMessage(FText& OutWarningMessage) const
{
	if (RootBone == NAME_None)
	{
		OutWarningMessage = LOCTEXT("MissingGoal", "Missing goal.");
		return true;
	}
	return false;
}

void UTLLRN_IKRig_SetTransform::AddGoal(const UTLLRN_IKRigEffectorGoal& NewGoal)
{
	Goal = NewGoal.GoalName;
	RootBone = NewGoal.BoneName;
}

void UTLLRN_IKRig_SetTransform::RenameGoal(const FName& OldName, const FName& NewName)
{
	if (Goal == OldName)
	{
		Goal = NewName;
	}
}

void UTLLRN_IKRig_SetTransform::SetGoalBone(const FName& GoalName, const FName& NewBoneName)
{
	if (Goal == GoalName)
	{
		RootBone = NewBoneName;
	}
}

UObject* UTLLRN_IKRig_SetTransform::GetGoalSettings(const FName& GoalName) const
{
	return Goal == GoalName ? Effector : nullptr;
}

bool UTLLRN_IKRig_SetTransform::IsBoneAffectedBySolver(const FName& BoneName, const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) const
{
	return TLLRN_IKRigSkeleton.IsBoneInDirectLineage(BoneName, RootBone);
}

#endif

#undef LOCTEXT_NAMESPACE

