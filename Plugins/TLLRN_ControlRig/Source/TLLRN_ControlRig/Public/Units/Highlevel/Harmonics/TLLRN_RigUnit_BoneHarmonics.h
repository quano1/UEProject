// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/Highlevel/TLLRN_RigUnit_HighlevelBase.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "TLLRN_RigUnit_BoneHarmonics.generated.h"

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_BoneHarmonics_BoneTarget
{
	GENERATED_BODY()

	FTLLRN_RigUnit_BoneHarmonics_BoneTarget()
	{
		Bone = NAME_None;
		Ratio = 0.f;
	}

	/**
	 * The name of the bone to drive
	 */
	UPROPERTY(meta = (Input))
	FName Bone;

	/**
	 * The ratio of where the bone sits within the harmonics system.
	 * Valid values reach from 0.0 to 1.0
	 */
	UPROPERTY(meta = (Input, Constant))
	float Ratio;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_Harmonics_TargetItem
{
	GENERATED_BODY()

	FTLLRN_RigUnit_Harmonics_TargetItem()
	{
		Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		Ratio = 0.f;
	}

	/**
	 * The name of the item to drive
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The ratio of where the item sits within the harmonics system.
	 * Valid values reach from 0.0 to 1.0
	 */
	UPROPERTY(meta = (Input, Constant))
	float Ratio;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_BoneHarmonics_WorkData
{
	GENERATED_BODY()

	FTLLRN_RigUnit_BoneHarmonics_WorkData()
	{
		WaveTime = FVector::ZeroVector;
	}

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> CachedItems;

	UPROPERTY()
	FVector WaveTime;
};

/**
 * Performs point based simulation
 */
USTRUCT(meta=(DisplayName="Harmonics", Keywords="Sin,Wave", Deprecated = 4.25))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_BoneHarmonics : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()
	
	FTLLRN_RigUnit_BoneHarmonics()
	{
		WaveSpeed = FVector::OneVector;
		WaveAmplitude = FVector(0.0f, 70.f, 0.f);
		WaveFrequency = FVector(1.f, 0.6f, 0.8f);
		WaveOffset = FVector(0.f, 1.f, 2.f);
		WaveNoise = FVector::ZeroVector;
		WaveEase = ERigVMAnimEasingType::Linear;
		WaveMinimum = 0.5f;
		WaveMaximum = 1.f;
		RotationOrder = EEulerRotationOrder::YZX;
		bPropagateToChildren = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/** The bones to drive. */
	UPROPERTY(meta = (Input, Constant))
	TArray<FTLLRN_RigUnit_BoneHarmonics_BoneTarget> Bones;

	UPROPERTY(meta = (Input))
	FVector WaveSpeed;

	UPROPERTY(meta = (Input))
	FVector WaveFrequency;

	/** The amplitude in degrees per axis */
	UPROPERTY(meta = (Input))
	FVector WaveAmplitude;

	UPROPERTY(meta = (Input))
	FVector WaveOffset;

	UPROPERTY(meta = (Input))
	FVector WaveNoise;

	UPROPERTY(meta = (Input))
	ERigVMAnimEasingType WaveEase;

	UPROPERTY(meta = (Input))
	float WaveMinimum;

	UPROPERTY(meta = (Input))
	float WaveMaximum;

	UPROPERTY(meta = (Input))
	EEulerRotationOrder RotationOrder;

	/**
	 * If set to true all of the global transforms of the children
	 * of this bone will be recalculated based on their local transforms.
	 * Note: This is computationally more expensive than turning it off.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bPropagateToChildren;

	UPROPERTY(transient)
	FTLLRN_RigUnit_BoneHarmonics_WorkData WorkData;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Drives an array of items through a harmonics spectrum
 */
USTRUCT(meta=(DisplayName="Harmonics", Keywords="Sin,Wave", Category = "Simulation"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemHarmonics : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()
	
	FTLLRN_RigUnit_ItemHarmonics()
	{
		WaveSpeed = FVector::OneVector;
		WaveAmplitude = FVector(0.0f, 70.f, 0.f);
		WaveFrequency = FVector(1.f, 0.6f, 0.8f);
		WaveOffset = FVector(0.f, 1.f, 2.f);
		WaveNoise = FVector::ZeroVector;
		WaveEase = ERigVMAnimEasingType::Linear;
		WaveMinimum = 0.5f;
		WaveMaximum = 1.f;
		RotationOrder = EEulerRotationOrder::YZX;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/** The items to drive. */
	UPROPERTY(meta = (Input, Constant))
	TArray<FTLLRN_RigUnit_Harmonics_TargetItem> Targets;

	UPROPERTY(meta = (Input))
	FVector WaveSpeed;

	UPROPERTY(meta = (Input))
	FVector WaveFrequency;

	/** The amplitude in degrees per axis */
	UPROPERTY(meta = (Input))
	FVector WaveAmplitude;

	UPROPERTY(meta = (Input))
	FVector WaveOffset;

	UPROPERTY(meta = (Input))
	FVector WaveNoise;

	UPROPERTY(meta = (Input))
	ERigVMAnimEasingType WaveEase;

	UPROPERTY(meta = (Input))
	float WaveMinimum;

	UPROPERTY(meta = (Input))
	float WaveMaximum;

	UPROPERTY(meta = (Input))
	EEulerRotationOrder RotationOrder;

	UPROPERTY(transient)
	FTLLRN_RigUnit_BoneHarmonics_WorkData WorkData;
};
