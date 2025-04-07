// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_InteractionExecution.generated.h"

/**
 * Event for executing logic during an interaction
 */
USTRUCT(meta=(DisplayName="Interaction", Category="Events", NodeColor="1, 0, 0", Keywords="Manipulation,Event,During,Interacting"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_InteractionExecution : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	virtual FName GetEventName() const override { return EventName; }

	// The execution result
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Execute", Category = "BeginExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	static inline const FLazyName EventName = FLazyName(TEXT("Interaction"));
};
