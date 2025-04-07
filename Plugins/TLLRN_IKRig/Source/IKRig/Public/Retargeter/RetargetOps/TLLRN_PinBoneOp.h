// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Retargeter/TLLRN_IKRetargetOps.h"

#include "TLLRN_PinBoneOp.generated.h"

#define LOCTEXT_NAMESPACE "TLLRN_RetargetOps"

// which skeleton are we referring to?
UENUM()
enum class ETLLRN_PinBoneType : uint8
{
	FullTransform,
	TranslateOnly,
	RotateOnly,
	ScaleOnly
};

USTRUCT(BlueprintType)
struct FTLLRN_PinBoneData
{
	GENERATED_BODY()

	FTLLRN_PinBoneData() = default;
	
	FTLLRN_PinBoneData(FName InBoneToPin, FName InBoneToPinTo)
	: BoneToPin(InBoneToPin)
	, BoneToPinTo(InBoneToPinTo)
	, BoneToPinIndex(INDEX_NONE)
	, BoneToPinToIndex(INDEX_NONE)
	, OffsetInRefPose(FTransform::Identity){}

	// The bone copy animation onto.
	UPROPERTY(EditAnywhere, Category=Settings, DisplayName="CopyToBone")
	FName BoneToPin;

	// The bone to copy animation from.
	UPROPERTY(EditAnywhere, Category=Settings, DisplayName="CopyFromBone")
	FName BoneToPinTo;
	
	int32 BoneToPinIndex;
	int32 BoneToPinToIndex;
	FTransform OffsetInRefPose;
};

UCLASS(BlueprintType, EditInlineNew)
class TLLRN_IKRIG_API UTLLRN_PinBoneOp : public UTLLRN_RetargetOpBase
{
	GENERATED_BODY()

public:
	
	virtual bool Initialize(
	const UTLLRN_IKRetargetProcessor* Processor,
		const FTLLRN_RetargetSkeleton& SourceSkeleton,
		const FTargetSkeleton& TargetSkeleton,
		FTLLRN_IKRigLogger& Log) override;
	
	virtual void Run(
		const UTLLRN_IKRetargetProcessor* Processor,
		const TArray<FTransform>& InSourceGlobalPose,
		TArray<FTransform>& OutTargetGlobalPose) override;

	UPROPERTY(EditAnywhere, Category="OpSettings")
	TArray<FTLLRN_PinBoneData> BonesToPin;

	// The bone, on the target skeleton to pin to.
	UPROPERTY(EditAnywhere, Category=Settings)
	ETLLRN_RetargetSourceOrTarget PinTo = ETLLRN_RetargetSourceOrTarget::Target;

	// Apply this pin to the full transform, or just translation or rotation only.
	UPROPERTY(EditAnywhere, Category=Settings)
	ETLLRN_PinBoneType PinType = ETLLRN_PinBoneType::FullTransform;

	// Maintain the original offset between the source and target bone
	UPROPERTY(EditAnywhere, Category=Settings)
	bool bMaintainOffset = true;

	// A manual offset to apply in global space
	UPROPERTY(EditAnywhere, Category=Settings)
	FTransform GlobalOffset;

	// A manual offset to apply in local space
	UPROPERTY(EditAnywhere, Category=Settings)
	FTransform LocalOffset;
	
#if WITH_EDITOR
	virtual FText GetNiceName() const override { return FText(LOCTEXT("OpName", "Pin Bone")); };
	virtual FText WarningMessage() const override { return Message; };
	FText Message;
#endif
};

#undef LOCTEXT_NAMESPACE
