// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "MathLibrary.h"
#include "TLLRN_RigUnit_Transform.generated.h"

/** Two args and a result of Transform type */
USTRUCT(meta=(Abstract, NodeColor = "0.1 0.7 0.1"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_BinaryTransformOp : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FTransform Argument0;

	UPROPERTY(meta=(Input))
	FTransform Argument1;

	UPROPERTY(meta=(Output))
	FTransform Result;
};

USTRUCT(meta=(DisplayName="Multiply(Transform)", Category="Math|Transform", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_MultiplyTransform : public FTLLRN_RigUnit_BinaryTransformOp
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "GetRelativeTransform", Category = "Math|Transform", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetRelativeTransform : public FTLLRN_RigUnit_BinaryTransformOp
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

