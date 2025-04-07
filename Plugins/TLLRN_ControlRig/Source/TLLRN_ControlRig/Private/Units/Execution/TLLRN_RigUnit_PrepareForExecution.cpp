// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_PrepareForExecution)

FTLLRN_RigUnit_PrepareForExecution_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	ExecuteContext.SetEventName(FTLLRN_RigUnit_PrepareForExecution::EventName);
}

FTLLRN_RigUnit_PostPrepareForExecution_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	ExecuteContext.SetEventName(FTLLRN_RigUnit_PostPrepareForExecution::EventName);
}
