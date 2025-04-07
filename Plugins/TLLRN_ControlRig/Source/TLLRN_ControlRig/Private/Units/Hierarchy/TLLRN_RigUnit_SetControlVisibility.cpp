// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetControlVisibility.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetControlVisibility)

FTLLRN_RigUnit_GetControlVisibility_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

    bVisible = false;

	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (CachedControlIndex.UpdateCache(Item, Hierarchy))
		{
			if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Get<FTLLRN_RigControlElement>(CachedControlIndex))
			{
				bVisible = ControlElement->Settings.bShapeVisible;
			}
		}
	}
}

FTLLRN_RigUnit_SetControlVisibility_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		TArray<FTLLRN_RigElementKey> Keys;

		if (Item.IsValid())
		{
			if (Item.Type != ETLLRN_RigElementType::Control)
			{
				return;
			}

			Keys.Add(Item);
		}
		else if (!Pattern.IsEmpty())
		{
			Hierarchy->ForEach<FTLLRN_RigControlElement>([&Keys, Pattern](FTLLRN_RigControlElement* ControlElement) -> bool
			{
				if (ControlElement->GetFName().ToString().Contains(Pattern, ESearchCase::CaseSensitive))
				{
					Keys.Add(ControlElement->GetKey());
				}
				return true;
			});
		}

		if (CachedControlIndices.Num() != Keys.Num())
		{
			CachedControlIndices.Reset();
			CachedControlIndices.SetNumZeroed(Keys.Num());
		}

		for (int32 Index = 0; Index < Keys.Num(); Index++)
		{
			CachedControlIndices[Index].UpdateCache(Keys[Index], Hierarchy);
		}

		for (const FTLLRN_CachedTLLRN_RigElement& CachedControlIndex : CachedControlIndices)
		{
			if (CachedControlIndex.IsValid())
			{
#if WITH_EDITOR
				if (const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Hierarchy->Find(CachedControlIndex.GetKey())))
				{
					if (ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl &&
						ControlElement->Settings.ShapeVisibility == ETLLRN_RigControlVisibility::BasedOnSelection)
					{
						UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(
						TEXT("SetControlVisibility: visibility of '%s' is based on selection, and cannot be modified through SetControlVisibility. "
						"In order to be able to modify the control's visibility, set the ShapeVisibility to UserDefined."),
						*ControlElement->GetKey().ToString());
					}
				}
#endif
				Hierarchy->SetControlVisibility(CachedControlIndex, bVisible);
			}
		}
	}
}

