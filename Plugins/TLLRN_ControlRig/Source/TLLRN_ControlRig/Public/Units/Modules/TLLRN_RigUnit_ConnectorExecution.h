// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_ConnectorExecution.generated.h"

/**
 * Event for filtering connection candidates
 */
USTRUCT(meta=(DisplayName="Connector", Category="Events", NodeColor="1, 0, 0", Keywords="Event,During,Resolve,Connect,Filter"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ConnectorExecution : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	virtual FName GetEventName() const override { return EventName; }

	// The execution result
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Execute", Category = "BeginExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	static inline const FLazyName EventName = FLazyName(TEXT("Connector"));
};
