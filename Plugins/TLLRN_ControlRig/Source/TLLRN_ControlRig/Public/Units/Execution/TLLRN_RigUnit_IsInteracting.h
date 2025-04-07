// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_IsInteracting.generated.h"

/**
 * Returns true if the Control Rig is being interacted
 */
USTRUCT(meta=(DisplayName="Get Interaction", Category="Execution", TitleColor="1 0 0", NodeColor="1 1 1", Keywords="IsInteracting,Gizmo,Manipulation,Interaction,Tracking", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_IsInteracting : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	// True if there is currently an interaction happening
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Interacting", Category = "Execution", meta = (Output))
	bool bIsInteracting = false;

	// True if the current interaction is a translation
	UPROPERTY(EditAnywhere, Transient, Category = "Execution", meta = (Output))
	bool bIsTranslating = false;

	// True if the current interaction is a rotation
	UPROPERTY(EditAnywhere, Transient, Category = "Execution", meta = (Output))
	bool bIsRotating = false;

	// True if the current interaction is scaling
	UPROPERTY(EditAnywhere, Transient, Category = "Execution", meta = (Output))
	bool bIsScaling = false;

	// The items being interacted on
	UPROPERTY(EditAnywhere, Transient, Category = "Execution", meta = (Output))
	TArray<FTLLRN_RigElementKey> Items;
};
