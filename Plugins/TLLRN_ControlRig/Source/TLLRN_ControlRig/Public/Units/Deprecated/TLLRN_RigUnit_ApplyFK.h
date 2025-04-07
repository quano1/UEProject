// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "Constraint.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_ApplyFK.generated.h"

UENUM()
enum class ETLLRN_ApplyTransformMode : uint8
{
	/** Override existing motion */
	Override,

	/** Additive to existing motion*/
	Additive,

	/** MAX - invalid */
	Max UMETA(Hidden),
};

USTRUCT(meta=(DisplayName="Apply FK", Category="Transforms", Deprecated = "4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ApplyFK : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "ApplyFK", meta = (Input))
	FName Joint;

	UPROPERTY(meta=(Input))
	FTransform Transform;

	/** The filter determines what axes can be manipulated by the in-viewport widgets */
	UPROPERTY(EditAnywhere, Category = "ApplyFK", meta = (Input))
	FTransformFilter Filter;

	UPROPERTY(EditAnywhere, Category = "ApplyFK", meta = (Input))
	ETLLRN_ApplyTransformMode TLLRN_ApplyTransformMode = ETLLRN_ApplyTransformMode::Override;

	UPROPERTY(EditAnywhere, Category = "ApplyFK", meta = (Input))
	ETLLRN_TransformSpaceMode ApplyTransformSpace = ETLLRN_TransformSpaceMode::LocalSpace;

	// Transform op option. Use if ETransformSpace is BaseTransform
	UPROPERTY(EditAnywhere, Category = "ApplyFK", meta = (Input))
	FTransform BaseTransform;

	// Transform op option. Use if ETransformSpace is BaseJoint
	UPROPERTY(EditAnywhere, Category = "ApplyFK", meta = (Input))
	FName BaseJoint;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
