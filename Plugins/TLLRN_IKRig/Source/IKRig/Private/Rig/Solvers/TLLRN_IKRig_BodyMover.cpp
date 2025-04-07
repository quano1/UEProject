// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rig/Solvers/TLLRN_IKRig_BodyMover.h"
#include "Rig/TLLRN_IKRigDataTypes.h"
#include "Rig/TLLRN_IKRigSkeleton.h"
#include "Rig/Solvers/TLLRN_PointsToRotation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRig_BodyMover)

#define LOCTEXT_NAMESPACE "TLLRN_IKRig_BodyMover"

void UTLLRN_IKRig_BodyMover::Initialize(const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton)
{
	BodyBoneIndex = TLLRN_IKRigSkeleton.GetBoneIndexFromName(RootBone);
}

void UTLLRN_IKRig_BodyMover::Solve(FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton, const FTLLRN_IKRigGoalContainer& Goals)
{
	// no body bone specified
	if (BodyBoneIndex == INDEX_NONE)
	{
		return;
	}

	// no effectors added
	if (Effectors.IsEmpty())
	{
		return;
	}
	
	// ensure body bone exists
	check(TLLRN_IKRigSkeleton.RefPoseGlobal.IsValidIndex(BodyBoneIndex));
	
	// calculate a "best fit" transform from deformed goal locations
	TArray<FVector> InitialPoints;
	TArray<FVector> CurrentPoints;
	for (const UTLLRN_IKRig_BodyMoverEffector* Effector : Effectors)
	{
		const FTLLRN_IKRigGoal* Goal = Goals.FindGoalByName(Effector->GoalName);
		if (!Goal)
		{
			return;
		}

		const int32 BoneIndex = TLLRN_IKRigSkeleton.GetBoneIndexFromName(Effector->BoneName);
		const FVector InitialPosition = TLLRN_IKRigSkeleton.CurrentPoseGlobal[BoneIndex].GetTranslation();
		const FVector GoalPosition = Goal->FinalBlendedPosition;
		const FVector FinalPosition = FMath::Lerp(InitialPosition, GoalPosition, Effector->InfluenceMultiplier);
		
		InitialPoints.Add(InitialPosition);
		CurrentPoints.Add(FinalPosition);
	}
	
	FVector InitialCentroid;
	FVector CurrentCentroid;
	const FQuat RotationOffset = GetRotationFromDeformedPoints(
		InitialPoints,
		CurrentPoints,
		InitialCentroid,
		CurrentCentroid);

	// alpha blend the position offset and add it to the current bone location
	const FVector Offset = (CurrentCentroid - InitialCentroid);
	const FVector Weight(
		Offset.X > 0.f ? PositionPositiveX : PositionNegativeX,
		Offset.Y > 0.f ? PositionPositiveY : PositionNegativeY,
		Offset.Z > 0.f ? PositionPositiveZ : PositionNegativeZ);

	// the bone transform to modify
	FTransform& CurrentBodyTransform = TLLRN_IKRigSkeleton.CurrentPoseGlobal[BodyBoneIndex];
	CurrentBodyTransform.AddToTranslation(Offset * (Weight*PositionAlpha));

	// do per-axis alpha blend
	FVector Euler = RotationOffset.Euler() * FVector(RotateXAlpha, RotateYAlpha, RotateZAlpha);
	FQuat FinalRotationOffset = FQuat::MakeFromEuler(Euler);
	// alpha blend the entire rotation offset
	FinalRotationOffset = FQuat::FastLerp(FQuat::Identity, FinalRotationOffset, RotationAlpha).GetNormalized();
	// add rotation offset to original rotation
	CurrentBodyTransform.SetRotation(FinalRotationOffset * CurrentBodyTransform.GetRotation());

	// do FK update of children
	TLLRN_IKRigSkeleton.PropagateGlobalPoseBelowBone(BodyBoneIndex);
}

void UTLLRN_IKRig_BodyMover::UpdateSolverSettings(UTLLRN_IKRigSolver* InSettings)
{
	if(UTLLRN_IKRig_BodyMover* Settings = Cast<UTLLRN_IKRig_BodyMover>(InSettings))
	{
		// copy solver settings
		PositionAlpha = Settings->PositionAlpha;
		PositionPositiveX = Settings->PositionPositiveX;
		PositionPositiveY = Settings->PositionPositiveY;
		PositionPositiveZ = Settings->PositionPositiveZ;
		PositionNegativeX = Settings->PositionNegativeX;
		PositionNegativeY = Settings->PositionNegativeY;
		PositionNegativeZ = Settings->PositionNegativeZ;
		RotationAlpha = Settings->RotationAlpha;
		RotateXAlpha = Settings->RotateXAlpha;
		RotateYAlpha = Settings->RotateYAlpha;
		RotateZAlpha = Settings->RotateZAlpha;

		// copy effector settings
		for (const UTLLRN_IKRig_BodyMoverEffector* InEffector : Settings->Effectors)
		{
			for (UTLLRN_IKRig_BodyMoverEffector* Effector : Effectors)
			{
				if (Effector->GoalName == InEffector->GoalName)
				{
					Effector->InfluenceMultiplier = InEffector->InfluenceMultiplier;
					break;
				}
			}
		}
	}
}

void UTLLRN_IKRig_BodyMover::RemoveGoal(const FName& GoalName)
{
	// ensure goal even exists in this solver
	const int32 GoalIndex = GetIndexOfGoal(GoalName);
	if (GoalIndex == INDEX_NONE)
	{
		return;
	}

	// remove it
	Effectors.RemoveAt(GoalIndex);
}

bool UTLLRN_IKRig_BodyMover::IsGoalConnected(const FName& GoalName) const
{
	return GetIndexOfGoal(GoalName) != INDEX_NONE;
}

#if WITH_EDITOR

FText UTLLRN_IKRig_BodyMover::GetNiceName() const
{
	return FText(LOCTEXT("SolverName", "Body Mover"));
}

bool UTLLRN_IKRig_BodyMover::GetWarningMessage(FText& OutWarningMessage) const
{
	if (RootBone == NAME_None)
	{
		OutWarningMessage = LOCTEXT("MissingRoot", "Missing root bone.");
		return true;
	}

	if (Effectors.IsEmpty())
	{
		OutWarningMessage = LOCTEXT("MissingGoal", "Missing goals.");
		return true;
	}
	
	return false;
}

void UTLLRN_IKRig_BodyMover::AddGoal(const UTLLRN_IKRigEffectorGoal& NewGoal)
{
	UTLLRN_IKRig_BodyMoverEffector* NewEffector = NewObject<UTLLRN_IKRig_BodyMoverEffector>(this, UTLLRN_IKRig_BodyMoverEffector::StaticClass());
	NewEffector->GoalName = NewGoal.GoalName;
	NewEffector->BoneName = NewGoal.BoneName;
	Effectors.Add(NewEffector);
}

void UTLLRN_IKRig_BodyMover::RenameGoal(const FName& OldName, const FName& NewName)
{
	// ensure goal even exists in this solver
	const int32 GoalIndex = GetIndexOfGoal(OldName);
	if (GoalIndex == INDEX_NONE)
	{
		return;
	}

	// rename
	Effectors[GoalIndex]->Modify();
	Effectors[GoalIndex]->GoalName = NewName;
}

void UTLLRN_IKRig_BodyMover::SetGoalBone(const FName& GoalName, const FName& NewBoneName)
{
	// ensure goal even exists in this solver
	const int32 GoalIndex = GetIndexOfGoal(GoalName);
	if (GoalIndex == INDEX_NONE)
	{
		return;
	}

	// rename
	Effectors[GoalIndex]->Modify();
	Effectors[GoalIndex]->BoneName = NewBoneName;
}

UObject* UTLLRN_IKRig_BodyMover::GetGoalSettings(const FName& GoalName) const
{
	const int32 GoalIndex = GetIndexOfGoal(GoalName);
	if (GoalIndex == INDEX_NONE)
	{
		return nullptr;
	}

	return Effectors[GoalIndex];
}

bool UTLLRN_IKRig_BodyMover::IsBoneAffectedBySolver(const FName& BoneName, const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) const
{
	return TLLRN_IKRigSkeleton.IsBoneInDirectLineage(BoneName, RootBone);
}

void UTLLRN_IKRig_BodyMover::SetRootBone(const FName& RootBoneName)
{
	RootBone = RootBoneName;
}

#endif

int32 UTLLRN_IKRig_BodyMover::GetIndexOfGoal(const FName& OldName) const
{
	for (int32 i=0; i<Effectors.Num(); ++i)
	{
		if (Effectors[i]->GoalName == OldName)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

#undef LOCTEXT_NAMESPACE

