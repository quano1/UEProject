// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetControlColor.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetControlColor)

FTLLRN_RigUnit_GetControlColor_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

    Color = FLinearColor::Black;

	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			const FTLLRN_RigControlElement* ControlElement = Hierarchy->GetChecked<FTLLRN_RigControlElement>(CachedControlIndex);
			Color = ControlElement->Settings.ShapeColor;
		}
	}
}

FTLLRN_RigUnit_SetControlColor_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			FTLLRN_RigControlElement* ControlElement = Hierarchy->GetChecked<FTLLRN_RigControlElement>(CachedControlIndex);
			ControlElement->Settings.ShapeColor = Color;
			Hierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
		}
	}
}

