// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_RigUnit_DebugBase.h"
#include "RigVMFunctions/Debug/RigVMFunction_VisualDebug.h"
#include "TLLRN_RigUnit_VisualDebug.generated.h"

USTRUCT(meta=(DisplayName = "Visual Debug Vector", Keywords = "Draw,Point", Deprecated = "4.25", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_VisualDebugVector : public FTLLRN_RigUnit_DebugBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_VisualDebugVector()
	{
		Value = FVector::ZeroVector;
		bEnabled = true;
		Mode = ERigUnitVisualDebugPointMode::Point;
		Color = FLinearColor::Red;
		Thickness = 10.f;
		Scale = 1.f;
		BoneSpace = NAME_None;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, Output))
	FVector Value;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	ERigUnitVisualDebugPointMode Mode;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	FLinearColor Color;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Thickness;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Scale;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	FName BoneSpace;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Debug draw parameters for a Point or Vector given a vector
 */
USTRUCT(meta=(DisplayName = "Visual Debug Vector", Keywords = "Draw,Point", Deprecated = "5.2", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_VisualDebugVectorItemSpace : public FTLLRN_RigUnit_DebugBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_VisualDebugVectorItemSpace()
	{
		Value = FVector::ZeroVector;
		bEnabled = true;
		Mode = ERigUnitVisualDebugPointMode::Point;
		Color = FLinearColor::Red;
		Thickness = 10.f;
		Scale = 1.f;
		Space = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, Output))
	FVector Value;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	ERigUnitVisualDebugPointMode Mode;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	FLinearColor Color;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Thickness;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Scale;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	FTLLRN_RigElementKey Space;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta = (DisplayName = "Visual Debug Quat", Keywords = "Draw,Rotation", Deprecated = "4.25", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_VisualDebugQuat : public FTLLRN_RigUnit_DebugBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_VisualDebugQuat()
	{
		Value = FQuat::Identity;
		bEnabled = true;
		Thickness = 0.f;
		Scale = 10.f;
		BoneSpace = NAME_None;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, Output))
	FQuat Value;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Thickness;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Scale;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	FName BoneSpace;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Debug draw parameters for an Axis given a quaternion
 */
USTRUCT(meta = (DisplayName = "Visual Debug Quat", Keywords = "Draw,Rotation", Deprecated = "5.2", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_VisualDebugQuatItemSpace : public FTLLRN_RigUnit_DebugBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_VisualDebugQuatItemSpace()
	{
		Value = FQuat::Identity;
		bEnabled = true;
		Thickness = 0.f;
		Scale = 10.f;
		Space = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, Output))
	FQuat Value;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Thickness;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Scale;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	FTLLRN_RigElementKey Space;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta=(DisplayName="Visual Debug Transform", Keywords = "Draw,Axes", Deprecated = "4.25", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_VisualDebugTransform : public FTLLRN_RigUnit_DebugBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_VisualDebugTransform()
	{
		Value = FTransform::Identity;
		bEnabled = true;
		Thickness = 0.f;
		Scale = 10.f;
		BoneSpace = NAME_None;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, Output))
	FTransform Value;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Thickness;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Scale;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	FName BoneSpace;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Debug draw parameters for an Axis given a transform
 */
USTRUCT(meta=(DisplayName="Visual Debug Transform", Keywords = "Draw,Axes", Deprecated = "5.2", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_VisualDebugTransformItemSpace : public FTLLRN_RigUnit_DebugBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_VisualDebugTransformItemSpace()
	{
		Value = FTransform::Identity;
		bEnabled = true;
		Thickness = 0.f;
		Scale = 10.f;
		Space = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, Output))
	FTransform Value;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Thickness;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	float Scale;

	UPROPERTY(meta = (Input, EditCondition = "bEnabled"))
	FTLLRN_RigElementKey Space;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};