// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_RigUnit_DebugBase.h"
#include "TLLRN_RigUnit_DebugLine.generated.h"

USTRUCT(meta=(DisplayName="Draw Line", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugLine : public FTLLRN_RigUnit_DebugBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugLine()
	{
		A = B = FVector::ZeroVector;
		Color = FLinearColor::Red;
		Thickness = 0.f;
		WorldOffset = FTransform::Identity;
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FVector A;

	UPROPERTY(meta = (Input))
	FVector B;

	UPROPERTY(meta = (Input))
	FLinearColor Color;

	UPROPERTY(meta = (Input))
	float Thickness;

	UPROPERTY(meta = (Input))
	FName Space;

	UPROPERTY(meta = (Input))
	FTransform WorldOffset;

	UPROPERTY(meta = (Input, Constant))
	bool bEnabled;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Draws a line in the viewport given a start and end vector
 */
USTRUCT(meta=(DisplayName="Draw Line", Deprecated = "5.2"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugLineItemSpace : public FTLLRN_RigUnit_DebugBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugLineItemSpace()
	{
		A = B = FVector::ZeroVector;
		Color = FLinearColor::Red;
		Thickness = 0.f;
		Space = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		WorldOffset = FTransform::Identity;
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FVector A;

	UPROPERTY(meta = (Input))
	FVector B;

	UPROPERTY(meta = (Input))
	FLinearColor Color;

	UPROPERTY(meta = (Input))
	float Thickness;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Space;

	UPROPERTY(meta = (Input))
	FTransform WorldOffset;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};