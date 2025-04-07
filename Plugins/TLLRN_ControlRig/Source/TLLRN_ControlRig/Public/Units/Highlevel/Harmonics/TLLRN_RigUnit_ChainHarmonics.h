// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/Highlevel/TLLRN_RigUnit_HighlevelBase.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "Curves/CurveFloat.h"
#include "TLLRN_RigUnit_ChainHarmonics.generated.h"

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ChainHarmonics_Reach
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ChainHarmonics_Reach()
	{
		bEnabled = true;
		ReachTarget = FVector::ZeroVector;
		ReachAxis = FVector(1.f, 0.f, 0.f);
		ReachMinimum = 0.0f;
		ReachMaximum = 0.0f;
		ReachEase = ERigVMAnimEasingType::Linear;
	}

	UPROPERTY(meta = (Input))
	bool bEnabled;

	UPROPERTY(meta = (Input))
	FVector ReachTarget;

	UPROPERTY(meta = (Input, Constant))
	FVector ReachAxis;

	UPROPERTY(meta = (Input))
	float ReachMinimum;

	UPROPERTY(meta = (Input))
	float ReachMaximum;

	UPROPERTY(meta = (Input))
	ERigVMAnimEasingType ReachEase;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ChainHarmonics_Wave
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ChainHarmonics_Wave()
	{
		bEnabled = true;
		WaveAmplitude = FVector(0.0f, 1.f, 0.f);
		WaveFrequency = FVector(1.f, 0.6f, 0.8f);
		WaveOffset = FVector(0.f, 1.f, 2.f);
		WaveNoise = FVector::ZeroVector;
		WaveMinimum = 0.f;
		WaveMaximum = 1.f;
		WaveEase = ERigVMAnimEasingType::Linear;
	}

	UPROPERTY(meta = (Input))
	bool bEnabled;

	UPROPERTY(meta = (Input))
	FVector WaveFrequency;

	UPROPERTY(meta = (Input))
	FVector WaveAmplitude;

	UPROPERTY(meta = (Input))
	FVector WaveOffset;

	UPROPERTY(meta = (Input))
	FVector WaveNoise;

	UPROPERTY(meta = (Input))
	float WaveMinimum;

	UPROPERTY(meta = (Input))
	float WaveMaximum;

	UPROPERTY(meta = (Input))
	ERigVMAnimEasingType WaveEase;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ChainHarmonics_Pendulum
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ChainHarmonics_Pendulum()
	{
		bEnabled = true;
		PendulumStiffness = 2.0f;
		PendulumGravity = FVector::ZeroVector;
		PendulumBlend = 0.75f;
		PendulumDrag = 0.98f;
		PendulumMinimum = 0.0f;
		PendulumMaximum = 0.1f;
		PendulumEase = ERigVMAnimEasingType::Linear;
		UnwindAxis = FVector(0.f, 1.f, 0.f);
		UnwindMinimum = 0.2f;
		UnwindMaximum = 0.05f;
	}

	UPROPERTY(meta = (Input))
	bool bEnabled;

	UPROPERTY(meta = (Input))
	float PendulumStiffness;

	UPROPERTY(meta = (Input))
	FVector PendulumGravity;

	UPROPERTY(meta = (Input))
	float PendulumBlend;

	UPROPERTY(meta = (Input))
	float PendulumDrag;

	UPROPERTY(meta = (Input))
	float PendulumMinimum;

	UPROPERTY(meta = (Input))
	float PendulumMaximum;

	UPROPERTY(meta = (Input))
	ERigVMAnimEasingType PendulumEase;

	UPROPERTY(meta = (Input))
	FVector UnwindAxis;

	UPROPERTY(meta = (Input))
	float UnwindMinimum;

	UPROPERTY(meta = (Input))
	float UnwindMaximum;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ChainHarmonics_WorkData
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ChainHarmonics_WorkData()
	{
		Time = FVector::ZeroVector;
	}

	UPROPERTY()
	FVector Time;

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> Items;

	UPROPERTY()
	TArray<float> Ratio;

	UPROPERTY()
	TArray<FVector> LocalTip;

	UPROPERTY()
	TArray<FVector> PendulumTip;

	UPROPERTY()
	TArray<FVector> PendulumPosition;

	UPROPERTY()
	TArray<FVector> PendulumVelocity;

	UPROPERTY()
	TArray<FVector> HierarchyLine;

	UPROPERTY()
	TArray<FVector> VelocityLines;
};

USTRUCT(meta=(DisplayName="ChainHarmonics", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ChainHarmonics : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ChainHarmonics()
	{
		ChainRoot = NAME_None;
		Speed = FVector::OneVector;
		
		Reach.bEnabled = false;
		Wave.bEnabled = true;
		Pendulum.bEnabled = false;

		WaveCurve = FRuntimeFloatCurve();
		WaveCurve.GetRichCurve()->AddKey(0.f, 0.f);
		WaveCurve.GetRichCurve()->AddKey(1.f, 1.f);

		bDrawDebug = true;
		DrawWorldOffset = FTransform::Identity;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FName ChainRoot;

	UPROPERTY(meta = (Input))
	FVector Speed;

	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_ChainHarmonics_Reach Reach;

	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_ChainHarmonics_Wave Wave;

	UPROPERTY(meta = (Input, Constant))
	FRuntimeFloatCurve WaveCurve;

	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_ChainHarmonics_Pendulum Pendulum;

	UPROPERTY(meta = (Input))
	bool bDrawDebug;

	UPROPERTY(meta = (Input))
	FTransform DrawWorldOffset;

	UPROPERTY(transient)
	FTLLRN_RigUnit_ChainHarmonics_WorkData WorkData;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Given a root will drive all items underneath in a chain based harmonics spectrum
 */
USTRUCT(meta=(DisplayName="Chain Harmonics", Category = "Simulation"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ChainHarmonicsPerItem : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ChainHarmonicsPerItem()
	{
		ChainRoot = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		Speed = FVector::OneVector;
		
		Reach.bEnabled = false;
		Wave.bEnabled = true;
		Pendulum.bEnabled = false;

		WaveCurve = FRuntimeFloatCurve();
		WaveCurve.GetRichCurve()->AddKey(0.f, 0.f);
		WaveCurve.GetRichCurve()->AddKey(1.f, 1.f);

		bDrawDebug = true;
		DrawWorldOffset = FTransform::Identity;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey ChainRoot;

	UPROPERTY(meta = (Input))
	FVector Speed;

	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_ChainHarmonics_Reach Reach;

	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_ChainHarmonics_Wave Wave;

	UPROPERTY(meta = (Input, Constant))
	FRuntimeFloatCurve WaveCurve;

	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_ChainHarmonics_Pendulum Pendulum;

	UPROPERTY(meta = (Input))
	bool bDrawDebug;

	UPROPERTY(meta = (Input))
	FTransform DrawWorldOffset;

	UPROPERTY(transient)
	FTLLRN_RigUnit_ChainHarmonics_WorkData WorkData;
};