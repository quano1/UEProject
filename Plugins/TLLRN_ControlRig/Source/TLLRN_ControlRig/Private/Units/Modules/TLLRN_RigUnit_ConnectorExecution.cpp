// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Modules/TLLRN_RigUnit_ConnectorExecution.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_ConnectorExecution)

FTLLRN_RigUnit_ConnectorExecution_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	ExecuteContext.SetEventName(FTLLRN_RigUnit_ConnectorExecution::EventName);
}

