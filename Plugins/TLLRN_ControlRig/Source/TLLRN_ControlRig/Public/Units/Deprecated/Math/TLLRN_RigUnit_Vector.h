// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "MathLibrary.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "RigVMFunctions/Math/RigVMFunction_MathVector.h"
#include "TLLRN_RigUnit_Vector.generated.h"

/** Two args and a result of Vector type */
USTRUCT(meta=(Abstract, NodeColor = "0.1 0.7 0.1"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_BinaryVectorOp : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FVector Argument0 = FVector(0.f);

	UPROPERTY(meta=(Input))
	FVector Argument1 = FVector(0.f);

	UPROPERTY(meta=(Output))
	FVector Result = FVector(0.f);
};

USTRUCT(meta=(DisplayName="Multiply(Vector)", Category="Math|Vector", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_Multiply_VectorVector : public FTLLRN_RigUnit_BinaryVectorOp
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta=(DisplayName="Add(Vector)", Category="Math|Vector", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_Add_VectorVector : public FTLLRN_RigUnit_BinaryVectorOp
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta=(DisplayName="Subtract(Vector)", Category="Math|Vector", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_Subtract_VectorVector : public FTLLRN_RigUnit_BinaryVectorOp
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta=(DisplayName="Divide(Vector)", Category="Math|Vector", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_Divide_VectorVector : public FTLLRN_RigUnit_BinaryVectorOp
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "Distance", Category = "Math|Vector", Deprecated="4.23.0", NodeColor = "0.1 0.7 0.1"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_Distance_VectorVector : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FVector Argument0 = FVector(0.f);

	UPROPERTY(meta=(Input))
	FVector Argument1 = FVector(0.f);

	UPROPERTY(meta=(Output))
	float Result = 0.f;

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

