// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "Constraint.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_AimConstraint.generated.h"

 /*
 ENUM: Aim Mode (Default: Aim At Target Transform )  # How to perform an aim
 Aim At Target Transforms
 Orient To Target Transforms
 */

UENUM()
enum class ETLLRN_AimMode : uint8
{
	/** Aim at Target Transform*/
	AimAtTarget,

	/** Orient to Target Transform */
	OrientToTarget,

	/** MAX - invalid */
	MAX,
};

USTRUCT(BlueprintType)
struct FTLLRN_AimTarget
{
	GENERATED_BODY()

	// # Target Weight
	UPROPERTY(EditAnywhere, Category = FTLLRN_AimTarget)
	float Weight = 0.f;

	// # Aim at/Align to this Transform
	UPROPERTY(EditAnywhere, Category = FTLLRN_AimTarget)
	FTransform Transform;

	//# Orient To Target Transforms mode only : Vector in the space of Target Transform to which the Aim Vector will be aligned
	UPROPERTY(EditAnywhere, Category = FTLLRN_AimTarget)
	FVector AlignVector = FVector(0.f);
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimConstraint_WorkData
{
	GENERATED_BODY()

	// note that Targets.Num () != ConstraintData.Num()
	UPROPERTY()
	TArray<FConstraintData>	ConstraintData;
};

USTRUCT(meta=(DisplayName="Aim Constraint", Category="Transforms", Deprecated = "4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimConstraint : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = FTLLRN_RigUnit_AimConstraint, meta = (Input))
	FName Joint;

	//# How to perform an aim
	UPROPERTY(EditAnywhere, Category = FTLLRN_RigUnit_AimConstraint, meta = (Input))
	ETLLRN_AimMode TLLRN_AimMode = ETLLRN_AimMode::AimAtTarget;

	//# How to perform an upvector stabilization
	UPROPERTY(EditAnywhere, Category = FTLLRN_RigUnit_AimConstraint, meta = (Input))
	ETLLRN_AimMode UpMode = ETLLRN_AimMode::AimAtTarget;

	// # Vector in the space of Named joint which will be aligned to the aim target
	UPROPERTY(EditAnywhere, Category = FTLLRN_RigUnit_AimConstraint, meta = (Input))
	FVector AimVector = FVector(0.f);

	//# Vector in the space of Named joint which will be aligned to the up target for stabilization
	UPROPERTY(EditAnywhere, Category = FTLLRN_RigUnit_AimConstraint, meta = (Input))
	FVector UpVector = FVector(0.f);

	UPROPERTY(EditAnywhere, Category = FTLLRN_RigUnit_AimConstraint, meta = (Input))
	TArray<FTLLRN_AimTarget> TLLRN_AimTargets;

	UPROPERTY(EditAnywhere, Category = FTLLRN_RigUnit_AimConstraint, meta = (Input))
	TArray<FTLLRN_AimTarget> UpTargets;

	// note that Targets.Num () != ConstraintData.Num()
	UPROPERTY()
	FTLLRN_RigUnit_AimConstraint_WorkData WorkData;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
