// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_BeginExecution.generated.h"

/**
 * Event for driving the skeleton hierarchy with variables and rig elements
 */
USTRUCT(meta=(DisplayName="TLLRN1 Forwards Solve", Category="Events", NodeColor="1, 0, 0", Keywords="Begin,Update,Tick,Forward,Event"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_BeginExecution : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	virtual FName GetEventName() const override { return EventName; }
	virtual bool CanOnlyExistOnce() const override { return true; }

	// The execution result
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Execute", Category = "BeginExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	static inline const FLazyName EventName = FLazyName(TEXT("TLLRN2 Forwards Solve"));
};

/**
 * Event always executed before the forward solve
 */
USTRUCT(meta=(DisplayName="Pre Forwards Solve", Category="Events", NodeColor="1, 0, 0", Keywords="Begin,Update,Tick,PreForward,Event"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PreBeginExecution : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	virtual FName GetEventName() const override { return EventName; }
	virtual bool CanOnlyExistOnce() const override { return true; }

	// The execution result
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Execute", Category = "BeginExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	static inline const FLazyName EventName = FLazyName(TEXT("Pre Forwards Solve"));
};

/**
 * Event always executed after the forward solve
 */
USTRUCT(meta=(DisplayName="Post Forwards Solve", Category="Events", NodeColor="1, 0, 0", Keywords="Begin,Update,Tick,PostForward,Event"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PostBeginExecution : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	virtual FName GetEventName() const override { return EventName; }
	virtual bool CanOnlyExistOnce() const override { return true; }

	// The execution result
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Execute", Category = "BeginExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	static inline const FLazyName EventName = FLazyName(TEXT("Post Forwards Solve"));
};
