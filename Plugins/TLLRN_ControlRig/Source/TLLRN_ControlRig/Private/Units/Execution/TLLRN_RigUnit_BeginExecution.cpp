// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_BeginExecution)

FTLLRN_RigUnit_BeginExecution_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	ExecuteContext.SetEventName(FTLLRN_RigUnit_BeginExecution::EventName);
}

FTLLRN_RigUnit_PreBeginExecution_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	ExecuteContext.SetEventName(FTLLRN_RigUnit_PreBeginExecution::EventName);
}

FTLLRN_RigUnit_PostBeginExecution_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	ExecuteContext.SetEventName(FTLLRN_RigUnit_PostBeginExecution::EventName);
}

