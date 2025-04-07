// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "Rig/TLLRN_IKRigDataTypes.h"
#include "TLLRN_IKRigInterface.h"

#include "TLLRN_IKRigComponent.generated.h"


UCLASS(ClassGroup = IKRig, BlueprintType, Blueprintable, EditInlineNew, meta = (BlueprintSpawnableComponent))
class TLLRN_IKRIG_API UTLLRN_IKRigComponent : public UActorComponent, public ITLLRN_IKGoalCreatorInterface
{
	GENERATED_BODY()

public:

	/** Set an IK Rig Goal position and rotation (assumed in Component Space of Skeletal Mesh) with separate alpha values. */
	UFUNCTION(BlueprintCallable, Category=IKRigGoals)
    void SetIKRigGoalPositionAndRotation(
        const FName GoalName,
        const FVector Position,
        const FQuat Rotation,
        const float PositionAlpha,
        const float RotationAlpha)
	{
		GoalContainer.SetIKGoal(
			FTLLRN_IKRigGoal(
				GoalName,
				Position,
				Rotation,
				PositionAlpha,
				RotationAlpha,
				ETLLRN_IKRigGoalSpace::Component,
				ETLLRN_IKRigGoalSpace::Component));
	};

	/** Set an IK Rig Goal transform (assumed in Component Space of Skeletal Mesh) with separate alpha values. */
	UFUNCTION(BlueprintCallable, Category=IKRigGoals)
    void SetIKRigGoalTransform(
        const FName GoalName,
        const FTransform Transform,
        const float PositionAlpha,
        const float RotationAlpha)
	{
		const FVector Position = Transform.GetTranslation();
		const FQuat Rotation = Transform.GetRotation();
		GoalContainer.SetIKGoal(
			FTLLRN_IKRigGoal(
				GoalName,
				Position,
				Rotation,
				PositionAlpha,
				RotationAlpha,
				ETLLRN_IKRigGoalSpace::Component,
				ETLLRN_IKRigGoalSpace::Component));
	};
	
	/** Apply a IKRigGoal and store it on this rig. Goal transform assumed in Component Space of Skeletal Mesh. */
	UFUNCTION(BlueprintCallable, Category=IKRigGoals)
    void SetIKRigGoal(const FTLLRN_IKRigGoal& Goal)
	{
		GoalContainer.SetIKGoal(Goal);
	};

	/** Remove all stored goals in this component. */
	UFUNCTION(BlueprintCallable, Category=IKRigGoals)
    void ClearAllGoals(){ GoalContainer.Empty(); };

	// BEGIN IIKRigGoalCreator interface
	/** Called from the IK Rig Anim node to obtain all the goals that have been set on this actor.*/
	virtual void AddIKGoals_Implementation(TMap<FName, FTLLRN_IKRigGoal>& OutGoals) override
	{
		const TArray<FTLLRN_IKRigGoal>& Goals = GoalContainer.GetGoalArray();
		for (const FTLLRN_IKRigGoal& Goal : Goals)
		{
			OutGoals.Add(Goal.Name, Goal);
		}
	};
	// END IIKRigGoalCreator interface

private:
	FTLLRN_IKRigGoalContainer GoalContainer;
};
