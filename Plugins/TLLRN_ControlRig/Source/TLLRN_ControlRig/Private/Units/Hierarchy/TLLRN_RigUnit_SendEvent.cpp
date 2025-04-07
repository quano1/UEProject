// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SendEvent.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SendEvent)

FTLLRN_RigUnit_SendEvent_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

    if(!bEnable)
    {
    	return;
    }

	if (bOnlyDuringInteraction && !ExecuteContext.UnitContext.IsInteracting())
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		FTLLRN_RigEventContext EventContext;
		EventContext.Key = Item;
		EventContext.Event = Event;
		EventContext.SourceEventName = ExecuteContext.GetEventName();
		EventContext.LocalTime = ExecuteContext.GetAbsoluteTime() + OffsetInSeconds;
		Hierarchy->SendEvent(EventContext, false /* async */); //needs to be false for sequencer keying to work
	}
}

