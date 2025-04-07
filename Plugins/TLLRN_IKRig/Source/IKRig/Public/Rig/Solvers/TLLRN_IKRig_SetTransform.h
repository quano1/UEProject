// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Rig/TLLRN_IKRigDefinition.h"
#include "TLLRN_IKRigSolver.h"

#include "TLLRN_IKRig_SetTransform.generated.h"

UCLASS(BlueprintType)
class TLLRN_IKRIG_API UTLLRN_IKRig_SetTransformEffector : public UObject
{
	GENERATED_BODY()

public:

	/** If true, Goal will drive the translation of the target bone. Default is true. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Set Transform Effector")
	bool bEnablePosition = true;

	/** If true, Goal will drive the rotation of the target bone. Default is true. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Set Transform Effector")
	bool bEnableRotation = true;

	/** Blend the effector on/off. Range is 0-1. Default is 1.0. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Set Transform Effector", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Alpha = 1.0f;
};

UCLASS(BlueprintType, EditInlineNew)
class TLLRN_IKRIG_API UTLLRN_IKRig_SetTransform : public UTLLRN_IKRigSolver
{
	GENERATED_BODY()

public:
	
	UTLLRN_IKRig_SetTransform();

	UPROPERTY(VisibleAnywhere, Category = "Set Transform Settings")
	FName Goal;

	UPROPERTY(VisibleAnywhere, Category = "Set Transform Settings")
	FName RootBone;
	
	UPROPERTY()
	TObjectPtr<UTLLRN_IKRig_SetTransformEffector> Effector;

	/** UTLLRN_IKRigSolver interface */
	virtual void Initialize(const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) override;
	virtual void Solve(FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton, const FTLLRN_IKRigGoalContainer& Goals) override;
	virtual void UpdateSolverSettings(UTLLRN_IKRigSolver* InSettings) override;
	virtual void RemoveGoal(const FName& GoalName) override;
	virtual bool IsGoalConnected(const FName& GoalName) const override;
	virtual FName GetRootBone() const override { return RootBone; };
#if WITH_EDITOR
	virtual FText GetNiceName() const override;
	virtual bool GetWarningMessage(FText& OutWarningMessage) const override;
	virtual void AddGoal(const UTLLRN_IKRigEffectorGoal& NewGoal) override;
	virtual void RenameGoal(const FName& OldName, const FName& NewName) override;
	virtual void SetGoalBone(const FName& GoalName, const FName& NewBoneName) override;
	virtual UObject* GetGoalSettings(const FName& GoalName) const override;
	virtual bool IsBoneAffectedBySolver(const FName& BoneName, const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) const override;
#endif
	/** END UTLLRN_IKRigSolver interface */

private:

	int32 BoneIndex;
};

