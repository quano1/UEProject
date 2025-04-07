// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_RigUnit_DebugBase.h"
#include "RigVMFunctions/Debug/RigVMFunction_DebugTransform.h"
#include "TLLRN_RigUnit_DebugTransform.generated.h"

USTRUCT(meta=(DisplayName="Draw Transform In Place", Deprecated = "4.25.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugTransform : public FTLLRN_RigUnit_DebugBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugTransform()
	{
		Transform = WorldOffset = FTransform::Identity;
		Mode = ERigUnitDebugTransformMode::Axes;
		Color = FLinearColor::Red;
		Thickness = 0.f;
		Scale = 10.f;
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, Output))
	FTransform Transform;

	UPROPERTY(meta = (Input))
	ERigUnitDebugTransformMode Mode;

	UPROPERTY(meta = (Input))
	FLinearColor Color;

	UPROPERTY(meta = (Input))
	float Thickness;

	UPROPERTY(meta = (Input))
	float Scale;

	UPROPERTY(meta = (Input))
	FName Space;

	UPROPERTY(meta = (Input))
	FTransform WorldOffset;

	UPROPERTY(meta = (Input, Constant))
	bool bEnabled;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT(meta=(DisplayName="Draw Transform", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugTransformMutable : public FTLLRN_RigUnit_DebugBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugTransformMutable()
	{
		Transform = WorldOffset = FTransform::Identity;
		Mode = ERigUnitDebugTransformMode::Axes;
		Color = FLinearColor::Red;
		Thickness = 0.f;
		Scale = 10.f;
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTransform Transform;

	UPROPERTY(meta = (Input))
	ERigUnitDebugTransformMode Mode;

	UPROPERTY(meta = (Input))
	FLinearColor Color;

	UPROPERTY(meta = (Input))
	float Thickness;

	UPROPERTY(meta = (Input))
	float Scale;

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
 * Given a transform, will draw a point, axis, or a box in the viewport
 */
USTRUCT(meta=(DisplayName="Draw Transform", Deprecated = "5.2"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugTransformMutableItemSpace : public FTLLRN_RigUnit_DebugBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugTransformMutableItemSpace()
	{
		Transform = WorldOffset = FTransform::Identity;
		Mode = ERigUnitDebugTransformMode::Axes;
		Color = FLinearColor::Red;
		Thickness = 0.f;
		Scale = 10.f;
		Space = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTransform Transform;

	UPROPERTY(meta = (Input))
	ERigUnitDebugTransformMode Mode;

	UPROPERTY(meta = (Input))
	FLinearColor Color;

	UPROPERTY(meta = (Input))
	float Thickness;

	UPROPERTY(meta = (Input))
	float Scale;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Space;

	UPROPERTY(meta = (Input))
	FTransform WorldOffset;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugTransformArrayMutable_WorkData
{
	GENERATED_BODY()
		
	UPROPERTY()
	TArray<FTransform> DrawTransforms;
};

USTRUCT(meta=(DisplayName="Draw Transform Array", Deprecated = "4.25.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugTransformArrayMutable : public FTLLRN_RigUnit_DebugBaseMutable
{
 	GENERATED_BODY()

	FTLLRN_RigUnit_DebugTransformArrayMutable()
	{
		Mode = ERigUnitDebugTransformMode::Axes;
		Color = FLinearColor::Red;
		Thickness = 0.f;
		Scale = 10.f;
		WorldOffset = FTransform::Identity;
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	TArray<FTransform> Transforms;

	UPROPERTY(meta = (Input))
	ERigUnitDebugTransformMode Mode;

	UPROPERTY(meta = (Input))
	FLinearColor Color;

	UPROPERTY(meta = (Input))
	float Thickness;

	UPROPERTY(meta = (Input))
	float Scale;

	UPROPERTY(meta = (Input))
	FName Space;

	UPROPERTY(meta = (Input))
	FTransform WorldOffset;

	UPROPERTY(meta = (Input, Constant))
	bool bEnabled;

	UPROPERTY(transient)
	FTLLRN_RigUnit_DebugTransformArrayMutable_WorkData WorkData;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Given a transform array, will draw a point, axis, or a box in the viewport
*/
USTRUCT(meta=(DisplayName="Draw Transform Array", Deprecated = "5.2"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugTransformArrayMutableItemSpace : public FTLLRN_RigUnit_DebugBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugTransformArrayMutableItemSpace()
	{
		WorldOffset = FTransform::Identity;
		Mode = ERigUnitDebugTransformMode::Axes;
		Color = FLinearColor::White;
		Thickness = 0.f;
		Scale = 10.f;
		Space = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	TArray<FTransform> Transforms;

	UPROPERTY(meta = (Input))
	TArray<int32> ParentIndices;

	UPROPERTY(meta = (Input))
	ERigUnitDebugTransformMode Mode;

	UPROPERTY(meta = (Input))
	FLinearColor Color;

	UPROPERTY(meta = (Input))
	float Thickness;

	UPROPERTY(meta = (Input))
	float Scale;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Space;

	UPROPERTY(meta = (Input))
	FTransform WorldOffset;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
