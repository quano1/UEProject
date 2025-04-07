// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Retargeter/TLLRN_IKRetargetSettings.h"

#include "TLLRN_IKRetargetProfile.generated.h"

class UTLLRN_IKRetargeter;
class UTLLRN_RetargetChainSettings;

USTRUCT(BlueprintType)
struct FTLLRN_RetargetProfile
{
	GENERATED_BODY()
	
public:

	// If true, the TARGET Retarget Pose specified in this profile will be applied to the Retargeter (when plugged into the Retargeter).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RetargetPoses, meta = (DisplayName = "Override Target Retarget Pose"))
	bool bApplyTargetRetargetPose = false;
	
	// Override the TARGET Retarget Pose to use when this profile is active.
	// The pose must be present in the Retarget Asset and is not applied unless bApplyTargetRetargetPose is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RetargetPoses, meta=(EditCondition="bApplyTargetRetargetPose"))
	FName TargetRetargetPoseName;

	// If true, the Source Retarget Pose specified in this profile will be applied to the Retargeter (when plugged into the Retargeter).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RetargetPoses, meta = (DisplayName = "Override Source Retarget Pose"))
	bool bApplySourceRetargetPose = false;

	// Override the SOURCE Retarget Pose to use when this profile is active.
	// The pose must be present in the Retarget Asset and is not applied unless bApplySourceRetargetPose is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RetargetPoses, meta=(EditCondition="bApplySourceRetargetPose"))
	FName SourceRetargetPoseName;

	// If true, the Chain Settings stored in this profile will be applied to the Retargeter (when plugged into the Retargeter).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ChainSettings, meta = (DisplayName = "Override Chain Settings"))
	bool bApplyChainSettings = false;
	
	// A (potentially sparse) set of setting overrides for the target chains (only applied when bApplyChainSettings is true).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ChainSettings, meta=(EditCondition="bApplyChainSettings"))
	TMap<FName, FTLLRN_TargetChainSettings> ChainSettings;

	// If true, the root settings stored in this profile will be applied to the Retargeter (when plugged into the Retargeter).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RootSettings, meta = (DisplayName = "Override Root Settings"))
	bool bApplyRootSettings = false;

	// Retarget settings to control behavior of the retarget root motion (not applied unless bApplyRootSettings is true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RootSettings, meta=(EditCondition="bApplyRootSettings"))
	FTLLRN_TargetRootSettings RootSettings;

	// If true, the global settings stored in this profile will be applied to the Retargeter (when plugged into the Retargeter).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GlobalSettings, meta = (DisplayName = "Override Global Settings"))
	bool bApplyGlobalSettings = false;

	// Retarget settings to control global behavior, like Stride Warping (not applied unless bApplyGlobalSettings is true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GlobalSettings, meta=(EditCondition="bApplyGlobalSettings"))
	FTLLRN_RetargetGlobalSettings GlobalSettings;

	void MergeWithOtherProfile(const FTLLRN_RetargetProfile& OtherProfile)
	{
		if (OtherProfile.bApplyTargetRetargetPose)
		{
			TargetRetargetPoseName = OtherProfile.TargetRetargetPoseName;
		}

		if (OtherProfile.bApplySourceRetargetPose)
		{
			SourceRetargetPoseName = OtherProfile.SourceRetargetPoseName;
		}

		if (OtherProfile.bApplyChainSettings)
		{
			for (const TPair<FName,FTLLRN_TargetChainSettings>& Pair : OtherProfile.ChainSettings)
			{
				ChainSettings.Add(Pair.Key, Pair.Value);
			}
		}

		if (OtherProfile.bApplyRootSettings)
		{
			RootSettings = OtherProfile.RootSettings;
		}

		if (OtherProfile.bApplyGlobalSettings)
		{
			GlobalSettings = OtherProfile.GlobalSettings;
		}
	}
};
