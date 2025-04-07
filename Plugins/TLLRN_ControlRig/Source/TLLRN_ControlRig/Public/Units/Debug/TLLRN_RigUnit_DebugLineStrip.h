// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_RigUnit_DebugBase.h"
#include "TLLRN_RigUnit_DebugLineStrip.generated.h"

USTRUCT(meta=(DisplayName="Draw Line Strip", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugLineStrip : public FTLLRN_RigUnit_DebugBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugLineStrip()
	{
		Color = FLinearColor::Red;
		Thickness = 0.f;
		WorldOffset = FTransform::Identity;
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	TArray<FVector> Points;

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
 * Draws a line strip in the viewport given any number of points
 */
USTRUCT(meta=(DisplayName="Draw Line Strip", Deprecated = "5.2"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugLineStripItemSpace : public FTLLRN_RigUnit_DebugBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugLineStripItemSpace()
	{
		Color = FLinearColor::Red;
		Thickness = 0.f;
		Space = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		WorldOffset = FTransform::Identity;
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	TArray<FVector> Points;

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
