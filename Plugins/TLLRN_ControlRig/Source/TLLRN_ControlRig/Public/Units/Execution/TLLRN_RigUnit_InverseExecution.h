// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_InverseExecution.generated.h"

/**
 * Event for driving elements based off the skeleton hierarchy
 */
USTRUCT(meta=(DisplayName="Backwards Solve", Category="Events", NodeColor="1, 0, 0", Keywords="Inverse,Reverse,Backwards,Event"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_InverseExecution : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	virtual FName GetEventName() const override { return EventName; }
	virtual bool CanOnlyExistOnce() const override { return true; }

	// The execution result
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Execute", Category = "InverseExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	static inline const FLazyName EventName = FLazyName(TEXT("Backwards Solve"));
};
