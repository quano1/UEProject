// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Rig/TLLRN_IKRigDefinition.h"
#include "Rig/Solvers/TLLRN_IKRigSolver.h"

#include "TLLRN_IKRig_PoleSolver.generated.h"

UCLASS(BlueprintType)
class TLLRN_IKRIG_API UTLLRN_IKRig_PoleSolverEffector : public UObject
{
	GENERATED_BODY()

public:

	UTLLRN_IKRig_PoleSolverEffector() { SetFlags(RF_Transactional); }
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Pole Solver Effector")
	FName GoalName = NAME_None;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Pole Solver Effector")
	FName BoneName = NAME_None;

	/** Blend the effector on/off. Range is 0-1. Default is 1.0. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pole Solver Effector", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Alpha = 1.0f;
};

UCLASS(BlueprintType, EditInlineNew)
class TLLRN_IKRIG_API UTLLRN_IKRig_PoleSolver : public UTLLRN_IKRigSolver
{
	GENERATED_BODY()

public:

	UTLLRN_IKRig_PoleSolver();
	
	UPROPERTY(VisibleAnywhere, DisplayName = "Root Bone", Category = "Pole Solver Settings")
	FName RootName = NAME_None;

	UPROPERTY(VisibleAnywhere, DisplayName = "End Bone", Category = "Pole Solver Settings")
	FName EndName = NAME_None;
	
	/** UTLLRN_IKRigSolver interface */
	virtual void Initialize(const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) override;
	virtual void Solve(FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton, const FTLLRN_IKRigGoalContainer& Goals) override;
	
	virtual void UpdateSolverSettings(UTLLRN_IKRigSolver* InSettings) override;
	virtual void RemoveGoal(const FName& GoalName) override;
	virtual bool IsGoalConnected(const FName& GoalName) const override;

	virtual FName GetRootBone() const override { return RootName; };

#if WITH_EDITOR
	virtual FText GetNiceName() const override;
	virtual bool GetWarningMessage(FText& OutWarningMessage) const override;
	virtual bool IsBoneAffectedBySolver(const FName& BoneName, const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) const override;
	// goals
	virtual void AddGoal(const UTLLRN_IKRigEffectorGoal& NewGoal) override;
	virtual void RenameGoal(const FName& OldName, const FName& NewName) override;
	virtual void SetGoalBone(const FName& GoalName, const FName& NewBoneName) override;
	virtual UObject* GetGoalSettings(const FName& GoalName) const override;
	// root bone can be set on this solver
	virtual bool RequiresRootBone() const override { return true; };
	virtual void SetRootBone(const FName& RootBoneName) override;
	// end bone can be set on this solver
	virtual void SetEndBone(const FName& EndBoneName) override;
	virtual FName GetEndBone() const override;
	virtual bool RequiresEndBone() const override { return true; };
	/** END UTLLRN_IKRigSolver interface */
#endif

private:

	static void GatherChildren(const int32 BoneIndex, const FTLLRN_IKRigSkeleton& InSkeleton, TArray<int32>& OutChildren);
	
	UPROPERTY()
	TObjectPtr<UTLLRN_IKRig_PoleSolverEffector>	Effector;
	
	TArray<int32>							Chain;
	TArray<int32>							ChildrenToUpdate;
};

