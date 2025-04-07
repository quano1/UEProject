// Copyright Epic Games, Inc. All Rights Reserved.


/**
 * Default Settings for the Control Rig Asset, including the Default Asset Name
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "TLLRN_CreateControlAssetRigSettings.generated.h"

UCLASS(BlueprintType, config = EditorSettings)
class  UTLLRN_CreateControlPoseAssetRigSettings : public UObject
{
public:
	UTLLRN_CreateControlPoseAssetRigSettings(const FObjectInitializer& Initializer);

	GENERATED_BODY()

	/** Asset Name*/
	UPROPERTY(EditAnywhere, Category = "Control Pose")
	FString AssetName;

};
