// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rig/TLLRN_IKRigDataTypes.h"
#include "Rig/TLLRN_IKRigDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRigDataTypes)

void FTLLRN_IKRigGoalContainer::SetIKGoal(const FTLLRN_IKRigGoal& InGoal)
{
	FTLLRN_IKRigGoal* Goal = FindGoalWriteable(InGoal.Name);
	if (!Goal)
	{
		// container hasn't seen this goal before, create new one, copying the input goal
		Goals.Emplace(InGoal);
		return;
	}

	// copy settings to existing goal
	*Goal = InGoal;
}

void FTLLRN_IKRigGoalContainer::SetIKGoal(const UTLLRN_IKRigEffectorGoal* InEffectorGoal)
{
	FTLLRN_IKRigGoal* Goal = FindGoalWriteable(InEffectorGoal->GoalName);
	if (!Goal)
	{
		// container hasn't seen this goal before, create new one, copying the Effector goal
		Goals.Emplace(InEffectorGoal);
		return;
	}

	// goals in editor have "preview mode" which allows them to be specified relative to their
	// initial pose to simulate an additive runtime behavior
#if WITH_EDITOR
	if (InEffectorGoal->PreviewMode == ETLLRN_IKRigGoalPreviewMode::Additive)
	{
		// transform to be interpreted as an offset, relative to input pose
		Goal->Position = InEffectorGoal->CurrentTransform.GetTranslation() - InEffectorGoal->InitialTransform.GetTranslation();
		const FQuat RelativeRotation = InEffectorGoal->CurrentTransform.GetRotation() * InEffectorGoal->InitialTransform.GetRotation().Inverse();
		Goal->Rotation = RelativeRotation.Rotator();
		Goal->PositionSpace = ETLLRN_IKRigGoalSpace::Additive;
		Goal->RotationSpace = ETLLRN_IKRigGoalSpace::Additive;
	}else
#endif
	{
		// transform to be interpreted as absolute and in component space
		Goal->Position = InEffectorGoal->CurrentTransform.GetTranslation();
		Goal->Rotation = InEffectorGoal->CurrentTransform.Rotator();
		Goal->PositionSpace = ETLLRN_IKRigGoalSpace::Component;
		Goal->RotationSpace = ETLLRN_IKRigGoalSpace::Component;
	}
	
    Goal->PositionAlpha = InEffectorGoal->PositionAlpha;
    Goal->RotationAlpha = InEffectorGoal->RotationAlpha;
}

const FTLLRN_IKRigGoal* FTLLRN_IKRigGoalContainer::FindGoalByName(const FName& GoalName) const
{
	return Goals.FindByPredicate([GoalName](const FTLLRN_IKRigGoal& Other)	{return Other.Name == GoalName;});
}

FTLLRN_IKRigGoal* FTLLRN_IKRigGoalContainer::FindGoalWriteable(const FName& GoalName) const
{
	return const_cast<FTLLRN_IKRigGoal*>(FindGoalByName(GoalName));
}

