// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Units/TLLRN_RigUnit.h"
#include "EulerTransform.h"
#include "TLLRN_RigUnit_Converter.generated.h"

USTRUCT(meta = (DisplayName = "ConvertToEulerTransform", Category = "Math|Convert", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConvertTransform : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FTransform Input;

	UPROPERTY(meta=(Output))
	FEulerTransform Result;

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "ConvertToTransform", Category = "Math|Convert", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConvertEulerTransform : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FEulerTransform Input;

	UPROPERTY(meta=(Output))
	FTransform Result;

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "ConvertToQuaternion", Category = "Math|Convert", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConvertRotation : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FRotator Input = FRotator(0.f);

	UPROPERTY(meta = (Output))
	FQuat	Result = FQuat::Identity;

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "ConvertToQuaternionDeprecated", Category = "Math|Convert", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConvertVectorRotation : public FTLLRN_RigUnit_ConvertRotation
{
	GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "ConvertToRotation", Category = "Math|Convert", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConvertQuaternion: public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FQuat	Input = FQuat::Identity;

	UPROPERTY(meta = (Output))
	FRotator	Result = FRotator(0.f);

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "ConvertVectorToRotation", Category = "Math|Convert", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConvertVectorToRotation: public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta = (Input))
	FVector Input = FVector(0.f);

	UPROPERTY(meta = (Output))
	FRotator	Result = FRotator(0.f);

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "ConvertVectorToQuaternion", Category = "Math|Convert", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConvertVectorToQuaternion: public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta = (Input))
	FVector Input = FVector(0.f);

	UPROPERTY(meta = (Output))
	FQuat	Result = FQuat::Identity;

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "ConvertRotationToVector", Category = "Math|Convert", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConvertRotationToVector: public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta = (Input))
	FRotator Input = FRotator(0.f);

	UPROPERTY(meta = (Output))
	FVector Result = FVector(0.f);

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "ConvertQuaternionToVector", Category = "Math|Convert", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConvertQuaternionToVector: public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta = (Input))
	FQuat	Input = FQuat::Identity;

	UPROPERTY(meta = (Output))
	FVector Result = FVector(0.f);

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "ToSwingAndTwist", Category = "Math|Transform", NodeColor = "0.1 0.1 0.7", Deprecated="4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ToSwingAndTwist : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FQuat	Input;

	UPROPERTY(meta = (Input))
	FVector TwistAxis;

	UPROPERTY(meta = (Output))
	FQuat	Swing;

	UPROPERTY(meta = (Output))
	FQuat	Twist;

	FTLLRN_RigUnit_ToSwingAndTwist()
		: Input(ForceInitToZero)
		, TwistAxis(FVector(1.f, 0.f, 0.f))
		, Swing(ForceInitToZero)
		, Twist(ForceInitToZero)
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
