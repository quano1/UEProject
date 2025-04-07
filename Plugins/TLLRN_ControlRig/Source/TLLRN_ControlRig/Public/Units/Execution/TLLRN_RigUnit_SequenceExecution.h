// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_SequenceExecution.generated.h"

/**
 * Allows for a single execution pulse to trigger a series of events in order.
 */
USTRUCT(meta=(DisplayName="Sequence", Category="Execution", TitleColor="1 0 0", NodeColor="1 1 1", Icon="EditorStyle|GraphEditor.Sequence_16x", Deprecated="5.1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SequenceExecution : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	RIGVM_METHOD()
	virtual void Execute() override;

	// The execution input
	UPROPERTY(EditAnywhere, Transient, DisplayName = "Execute", Category = "SequenceExecution", meta = (Input))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	// The execution result A
	UPROPERTY(EditAnywhere, Transient, Category = "SequenceExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext A;

	// The execution result B
	UPROPERTY(EditAnywhere, Transient, Category = "SequenceExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext B;

	// The execution result C
	UPROPERTY(EditAnywhere, Transient, Category = "SequenceExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext C;

	// The execution result D
	UPROPERTY(EditAnywhere, Transient, Category = "SequenceExecution", meta = (Output))
	FTLLRN_ControlRigExecuteContext D;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

