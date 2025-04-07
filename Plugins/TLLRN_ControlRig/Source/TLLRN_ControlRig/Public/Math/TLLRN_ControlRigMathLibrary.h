// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_ControlRigDefines.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"

class TLLRN_CONTROLRIG_API FTLLRN_ControlRigMathLibrary : public FRigVMMathLibrary
{
public:
	static void SolveBasicTwoBoneIK(FTransform& BoneA, FTransform& BoneB, FTransform& Effector, const FVector& PoleVector, const FVector& PrimaryAxis, const FVector& SecondaryAxis, float SecondaryAxisWeight, float BoneALength, float BoneBLength, bool bEnableStretch, float StretchStartRatio, float StretchMaxRatio);
};