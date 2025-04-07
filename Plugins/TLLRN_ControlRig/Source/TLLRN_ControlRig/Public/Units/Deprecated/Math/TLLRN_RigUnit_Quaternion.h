// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "MathLibrary.h"
#include "TLLRN_RigUnit_Quaternion.generated.h"

/** Two args and a result of Quaternion type */
USTRUCT(meta=(Abstract, NodeColor = "0.1 0.7 0.1"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_BinaryQuaternionOp : public FTLLRN_RigUnit
{
	GENERATED_BODY()

		FTLLRN_RigUnit_BinaryQuaternionOp()
		: Argument0(FQuat::Identity)
		, Argument1(FQuat::Identity)
		, Result(FQuat::Identity)
	{}

	UPROPERTY(meta=(Input))
	FQuat Argument0;

	UPROPERTY(meta=(Input))
	FQuat Argument1;

	UPROPERTY(meta=(Output))
	FQuat Result;
};

USTRUCT(meta=(DisplayName="Multiply(Quaternion)", Category="Math|Quaternion", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_MultiplyQuaternion : public FTLLRN_RigUnit_BinaryQuaternionOp
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/** Two args and a result of Quaternion type */
USTRUCT(meta=(Abstract, NodeColor = "0.1 0.7 0.1"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_UnaryQuaternionOp : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FQuat Argument = FQuat::Identity;

	UPROPERTY(meta=(Output))
	FQuat Result = FQuat::Identity;
};

USTRUCT(meta = (DisplayName = "Inverse(Quaternion)", Category = "Math|Quaternion", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_InverseQuaterion: public FTLLRN_RigUnit_UnaryQuaternionOp
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "To Axis And Angle(Quaternion)", Category = "Math|Quaternion", NodeColor = "0.1 0.7 0.1", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_QuaternionToAxisAndAngle : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta = (Input))
	FQuat Argument = FQuat::Identity;

	UPROPERTY(meta = (Output))
	FVector Axis = FVector(0.f);

	UPROPERTY(meta = (Output))
	float Angle = 0.f;

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "From Axis And Angle(Quaternion)", Category = "Math|Quaternion", NodeColor = "0.1 0.7 0.1", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_QuaternionFromAxisAndAngle : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_QuaternionFromAxisAndAngle()
		: Axis(1.f, 0.f, 0.f)
		, Angle(0.f)
		, Result(ForceInitToZero)
	{}

	UPROPERTY(meta = (Input))
	FVector Axis;

	UPROPERTY(meta = (Input))
	float Angle;

	UPROPERTY(meta = (Output))
	FQuat Result;

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "Get Angle Around Axis", Category = "Math|Quaternion", NodeColor = "0.1 0.7 0.1", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_QuaternionToAngle : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_QuaternionToAngle()
		: Axis(1.f, 0.f, 0.f)
		, Argument(ForceInitToZero)
		, Angle(0.f)
	{}

	UPROPERTY(meta = (Input))
	FVector Axis;

	UPROPERTY(meta = (Input))
	FQuat Argument;

	UPROPERTY(meta = (Output))
	float Angle;

	RIGVM_METHOD()
	virtual void Execute() override;
	
	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

