// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Rig/TLLRN_IKRigDefinition.h"
#include "TLLRN_IKRigSolver.h"

#include "Rig/Solvers/TLLRN_LimbSolver.h"

#include "TLLRN_IKRig_LimbSolver.generated.h"

UCLASS(BlueprintType)
class TLLRN_IKRIG_API UTLLRN_IKRig_LimbEffector : public UObject
{
	GENERATED_BODY()

public:
	UTLLRN_IKRig_LimbEffector() { SetFlags(RF_Transactional); }

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Limb IK Effector")
	FName GoalName = NAME_None;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Limb IK Effector")
	FName BoneName = NAME_None;
};

UCLASS(BlueprintType, EditInlineNew)
class TLLRN_IKRIG_API UTLLRN_IKRig_LimbSolver : public UTLLRN_IKRigSolver
{
	GENERATED_BODY()

public:

	UTLLRN_IKRig_LimbSolver();
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, DisplayName = "Root Bone", Category = "Limb IK Settings")
	FName RootName = NAME_None;

	/** Precision (distance to the target) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings", meta = (ClampMin = "0.0"))
	float ReachPrecision = 0.01f;
	
	// TWO BONES SETTINGS
	
	/** Hinge Bones Rotation Axis. This is essentially the plane normal for (hip - knee - foot). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings|Two Bones")
	TEnumAsByte<EAxis::Type> HingeRotationAxis = EAxis::None;

	// FABRIK SETTINGS

	/** Number of Max Iterations to reach the target */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings|FABRIK", meta = (ClampMin = "0", UIMin = "0", UIMax = "100"))
	int32 MaxIterations = 12;

	/** Enable/Disable rotational limits */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings|FABRIK|Limits")
	bool bEnableLimit = false;

	/** Only used if bEnableRotationLimit is enabled. Prevents the leg from folding onto itself,
	* and forces at least this angle between Parent and Child bone. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings|FABRIK|Limits", meta = (EditCondition="bEnableLimit", ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0"))
	float MinRotationAngle = 15.f;

	/** Pull averaging only has a visual impact when we have more than 2 bones (3 links). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings|FABRIK|Pull Averaging")
	bool bAveragePull = true;

	/** Re-position limb to distribute pull: 0 = foot, 0.5 = balanced, 1.f = hip */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings|FABRIK|Pull Averaging", meta = (ClampMin = "0.0", ClampMax = "1.0",UIMin = "0.0", UIMax = "1.0"))
	float PullDistribution = 0.5f;
	
	/** Move end effector towards target. If we are compressing the chain, limit displacement.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings|FABRIK|Pull Averaging", meta = (ClampMin = "0.0", ClampMax = "1.0",UIMin = "0.0", UIMax = "1.0"))
	float ReachStepAlpha = 0.7f;

	// TWIST SETTINGS
	
	/** Enable Knee Twist correction, by comparing Foot FK with Foot IK orientation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings|Twist")
	bool bEnableTwistCorrection = false;
	
	/** Forward Axis for Foot bone. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limb IK Settings|Twist", meta = (EditCondition="bEnableTwistCorrection"))
	TEnumAsByte<EAxis::Type> EndBoneForwardAxis = EAxis::Y;
	
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
#endif
	/** END UTLLRN_IKRigSolver interface */
	
private:

	static void GatherChildren(const int32 BoneIndex, const FTLLRN_IKRigSkeleton& InSkeleton, TArray<int32>& OutChildren);
	
	UPROPERTY()
	TObjectPtr<UTLLRN_IKRig_LimbEffector> Effector;
	
	FTLLRN_LimbSolver						Solver;
	TArray<int32>					ChildrenToUpdate;
};
