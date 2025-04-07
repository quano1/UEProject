// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Rig/TLLRN_IKRigDefinition.h"

class UTLLRN_IKRigController;

// what was the outcome of trying to automatically setup FBIK?
enum class EAutoFBIKResult
{
	AllOk,
	MissingMesh,
	UnknownSkeletonType,
	MissingChains,
	MissingRootBone
};


// the results of auto characterizing an input skeletal mesh
struct FTLLRN_AutoFBIKResults
{
	FTLLRN_AutoFBIKResults() : Outcome(EAutoFBIKResult::AllOk) {};
	
	EAutoFBIKResult Outcome;
	TArray<FName> MissingChains;
};

// given an IK Rig, will automatically generate an FBIK setup for use with retargeting
struct FTLLRN_AutoFBIKCreator
{
	FTLLRN_AutoFBIKCreator() = default;

	// call this function with any IK Rig controller to automatically create a FBIK setup
	void CreateFBIKSetup(const UTLLRN_IKRigController& TLLRN_IKRigController, FTLLRN_AutoFBIKResults& Results) const;
};
