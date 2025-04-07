// Copyright Epic Games, Inc. All Rights Reserved.
#include "Tools/TLLRN_ControlTLLRN_RigPoseProjectSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlTLLRN_RigPoseProjectSettings)

UTLLRN_ControlTLLRN_RigPoseProjectSettings::UTLLRN_ControlTLLRN_RigPoseProjectSettings()
{
	FDirectoryPath RootSaveDir;
 	RootSaveDir.Path = TEXT("TLLRN_ControlRig/Pose");
	RootSaveDirs.Add(RootSaveDir);
}

TArray<FString> UTLLRN_ControlTLLRN_RigPoseProjectSettings::GetAssetPaths() const
{
	TArray<FString> Paths;
	for (const FDirectoryPath& Path : RootSaveDirs)
	{
		Paths.Add(Path.Path);
	}
	return Paths;
}

