// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_PrepareForExecution.generated.h"

/**
 * Event to create / configure elements before any other event
 */
USTRUCT(meta=(DisplayName="Construction Event", Category="Events", NodeColor="0.6, 0, 1", Keywords="Create,Build,Spawn,Setup,Init,Fit"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PrepareForExecution : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	virtual FName GetEventName() const override { return EventName; }
	virtual bool CanOnlyExistOnce() const override { return true; }

	// The execution result
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Execute", Category = "PrepareForExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	static inline const FLazyName EventName = FLazyName(TEXT("Construction"));
};

/**
 * Event to further configure elements. Runs after the Construction Event
 */
USTRUCT(meta=(DisplayName="Post Construction", Category="Events", NodeColor="0.6, 0, 1", Keywords="Create,Build,Spawn,Setup,Init,Fit"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PostPrepareForExecution : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	virtual FName GetEventName() const override { return EventName; }
	virtual bool CanOnlyExistOnce() const override { return true; }

	// The execution result
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Execute", Category = "PostPrepareForExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	static inline const FLazyName EventName = FLazyName(TEXT("PostConstruction"));
};
