// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tools/TLLRN_CreateControlAssetRigSettings.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_CreateControlAssetRigSettings)


UTLLRN_CreateControlPoseAssetRigSettings::UTLLRN_CreateControlPoseAssetRigSettings(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	AssetName = FString("TLLRN_ControlTLLRN_RigPose");
}

