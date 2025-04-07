// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
/**
* Per Project user settings for Control Rig Poses(and maybe animations etc).
*/
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "TLLRN_ControlTLLRN_RigPoseProjectSettings.generated.h"


UCLASS(config = EditorPerProjectUserSettings)
class TLLRN_CONTROLRIG_API UTLLRN_ControlTLLRN_RigPoseProjectSettings : public UObject
{

public:
	GENERATED_BODY()

	UTLLRN_ControlTLLRN_RigPoseProjectSettings();

	/** The pose asset path  */
	TArray<FString> GetAssetPaths() const;

	/** The root of the directory in which to save poses */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "TLLRN Control Rig Poses", meta = (ContentDir))
	TArray<FDirectoryPath> RootSaveDirs;

	/** Not used but may put bad if we support other types.
	bool bFilterPoses;

	bool bFilterAnimations;

	bool bFilterSelectionSets;
	*/
};
