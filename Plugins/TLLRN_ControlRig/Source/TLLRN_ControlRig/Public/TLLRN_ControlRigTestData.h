// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Rigs/TLLRN_RigHierarchyPose.h"

#include "TLLRN_ControlRigTestData.generated.h"

class UTLLRN_ControlRigTestData;
class UTLLRN_ControlRig;

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigTestDataVariable
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_ControlRigTestDataVariable()
	{
		Name = NAME_None;
		CPPType = NAME_None;
		Value = FString();
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestDataVariable")
	FName Name;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestDataVariable")
	FName CPPType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestDataVariable")
	FString Value;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigTestDataFrame
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_ControlRigTestDataFrame()
	{
		AbsoluteTime = 0.0;
		DeltaTime = 0.0;
		Variables.Reset();
		Pose.Reset();
	}

	bool Store(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bInitial = false);
	bool Restore(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bInitial = false) const;
	bool RestoreVariables(UTLLRN_ControlRig* InTLLRN_ControlRig) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestDataFrame")
	double AbsoluteTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestDataFrame")
	double DeltaTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestDataFrame")
	TArray<FTLLRN_ControlRigTestDataVariable> Variables;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestDataFrame")
	FTLLRN_RigPose Pose;
};

UENUM()
enum class ETLLRN_ControlRigTestDataPlaybackMode : uint8
{
	Live,
	ReplayInputs,
	GroundTruth,
	Max UMETA(Hidden),
};

UCLASS(BlueprintType)
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigTestData : public UObject
{
	GENERATED_BODY()

public:

	UTLLRN_ControlRigTestData()
		: Tolerance(0.001)
		, LastFrameIndex(INDEX_NONE)
		, DesiredRecordingDuration(0.0)
		, TimeAtStartOfRecording(0.0)
		, bIsApplyingOutputs(false)
	{}

	virtual void Serialize(FArchive& Ar) override;

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRigTestData")
	static UTLLRN_ControlRigTestData* CreateNewAsset(FString InDesiredPackagePath, FString InBlueprintPathName);

	UFUNCTION(BlueprintPure, Category = "TLLRN_ControlRigTestData")
	FVector2D GetTimeRange(bool bInput = false) const;

	UFUNCTION(BlueprintPure, Category = "TLLRN_ControlRigTestData")
	int32 GetFrameIndexForTime(double InSeconds, bool bInput = false) const;

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRigTestData")
	bool Record(UTLLRN_ControlRig* InTLLRN_ControlRig, double InRecordingDuration = 0.0);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRigTestData")
	bool SetupReplay(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bGroundTruth = true);

	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRigTestData")
	void ReleaseReplay();

	UFUNCTION(BlueprintPure, Category = "TLLRN_ControlRigTestData")
	ETLLRN_ControlRigTestDataPlaybackMode GetPlaybackMode() const;

	UFUNCTION(BlueprintPure, Category = "TLLRN_ControlRigTestData")
	bool IsReplaying() const;

	UFUNCTION(BlueprintPure, Category = "TLLRN_ControlRigTestData")
	bool IsRecording() const { return DesiredRecordingDuration >= SMALL_NUMBER; }

	UPROPERTY(AssetRegistrySearchable, VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestData")
	FSoftObjectPath TLLRN_ControlRigObjectPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestData")
	FTLLRN_ControlRigTestDataFrame Initial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestData")
	TArray<FTLLRN_ControlRigTestDataFrame> InputFrames;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestData")
	TArray<FTLLRN_ControlRigTestDataFrame> OutputFrames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestData")
	TArray<int32> FramesToSkip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TLLRN_ControlRigTestData")
	double Tolerance;

private:

	void ClearDelegates(UTLLRN_ControlRig* InTLLRN_ControlRig);

	mutable int32 LastFrameIndex;
	double DesiredRecordingDuration;
	double TimeAtStartOfRecording;
	TWeakObjectPtr<UTLLRN_ControlRig> ReplayTLLRN_ControlRig;
	bool bIsApplyingOutputs;
	FDelegateHandle PreConstructionHandle;
	FDelegateHandle PreForwardHandle;
	FDelegateHandle PostForwardHandle;
};