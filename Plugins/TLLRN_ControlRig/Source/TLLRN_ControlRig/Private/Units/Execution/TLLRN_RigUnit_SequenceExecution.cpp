// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_SequenceExecution.h"
#include "RigVMFunctions/Execution/RigVMFunction_Sequence.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SequenceExecution)

FTLLRN_RigUnit_SequenceExecution_Execute()
{
	// nothing to do here. the execute context is actually
	// the same shared memory for all pins
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SequenceExecution::GetUpgradeInfo() const
{
	FRigVMFunction_Sequence NewNode;
	
	FRigVMStructUpgradeInfo Info(*this, NewNode);

	// add two more pins
	Info.AddAggregatePin(); // "C"
	Info.AddAggregatePin(); // "D"

	return Info;
}


