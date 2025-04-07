// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetControlDrivenList.h"

#include "Rigs/TLLRN_RigHierarchyController.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetControlDrivenList)

FTLLRN_RigUnit_GetControlDrivenList_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

    Driven.Reset();

	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			const FTLLRN_RigControlElement* ControlElement = Hierarchy->GetChecked<FTLLRN_RigControlElement>(CachedControlIndex);
			Driven = ControlElement->Settings.DrivenControls;
		}
	}
}

FTLLRN_RigUnit_SetControlDrivenList_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			FTLLRN_RigControlElement* ControlElement = Hierarchy->GetChecked<FTLLRN_RigControlElement>(CachedControlIndex);
			if(ControlElement->Settings.DrivenControls != Driven)
			{
				Swap(ControlElement->Settings.DrivenControls, ControlElement->Settings.PreviouslyDrivenControls);
				ControlElement->Settings.DrivenControls = Driven;
				Hierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlDrivenListChanged, ControlElement);
			}
		}
	}
}

