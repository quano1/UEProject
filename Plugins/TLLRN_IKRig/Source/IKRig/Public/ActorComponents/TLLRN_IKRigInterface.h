// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Rig/TLLRN_IKRigDataTypes.h"
#include "UObject/Interface.h"

#include "TLLRN_IKRigInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UTLLRN_IKGoalCreatorInterface : public UInterface
{
	GENERATED_BODY()
};

class ITLLRN_IKGoalCreatorInterface
{    
	GENERATED_BODY()

public:
	
	/** Add your own goals to the OutGoals map (careful not to remove existing goals in the map!)*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category=IKRigGoals)
    void AddIKGoals(TMap<FName, FTLLRN_IKRigGoal>& OutGoals);
};
