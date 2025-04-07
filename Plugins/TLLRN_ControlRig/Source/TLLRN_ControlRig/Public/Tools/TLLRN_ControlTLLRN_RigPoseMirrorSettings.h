// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
/**
* Per Project Settings for the Mirroring, in particular what axis to use to mirror and the matching strings.
*/
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "TLLRN_ControlTLLRN_RigPoseMirrorSettings.generated.h"


UCLASS(config = EditorPerProjectUserSettings)
class TLLRN_CONTROLRIG_API UTLLRN_ControlTLLRN_RigPoseMirrorSettings : public UObject
{

public:
	GENERATED_BODY()

	UTLLRN_ControlTLLRN_RigPoseMirrorSettings();

	/** The String to denote Controls on the Right Side */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Mirror Settings", meta = (ToolTip = "String To Denote Right Side"))
	FString RightSide;

	/** The String to denote Controls on the Left Side */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Mirror Settings", meta = (ToolTip = "String To Denote Left Side"))
	FString LeftSide;

	// the axis to mirror translations against
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Mirror Settings", meta = (ToolTip = "Axis to Mirror Translations"))
	TEnumAsByte<EAxis::Type> MirrorAxis;

	// the axis to flip for rotations
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Mirror Settings", meta = (ToolTip = "Axis to Flip for Rotations"))
	TEnumAsByte<EAxis::Type> AxisToFlip;

};


