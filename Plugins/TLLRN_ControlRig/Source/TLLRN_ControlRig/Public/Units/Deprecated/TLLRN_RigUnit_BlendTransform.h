// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_BlendTransform.generated.h"

USTRUCT(BlueprintType)
struct FTLLRN_BlendTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "FTLLRN_ConstraintTarget")
	FTransform	Transform;

	UPROPERTY(EditAnywhere, Category = "FTLLRN_ConstraintTarget")
	float Weight;

	FTLLRN_BlendTarget()
		: Weight (1.f)
	{}
};

USTRUCT(meta = (DisplayName = "Blend(Transform)", Category = "Blend", Deprecated = "4.26.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_BlendTransform : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta = (Input))
	FTransform Source;

	UPROPERTY(meta = (Input))
	TArray<FTLLRN_BlendTarget> Targets;

	UPROPERTY(meta = (Output))
	FTransform Result;

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};