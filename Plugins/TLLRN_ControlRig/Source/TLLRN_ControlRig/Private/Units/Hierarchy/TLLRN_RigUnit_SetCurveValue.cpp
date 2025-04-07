// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetCurveValue.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetCurveValue)

FTLLRN_RigUnit_SetCurveValue_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Curve, ETLLRN_RigElementType::Curve);
		if (CachedCurveIndex.UpdateCache(Key, Hierarchy))
		{
			Hierarchy->SetCurveValueByIndex(CachedCurveIndex, Value);
		}
	}
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_SetCurveValue)
{
	const FTLLRN_RigElementKey CurveA = Controller->AddCurve(TEXT("CurveA"), 0.f);
	const FTLLRN_RigElementKey CurveB = Controller->AddCurve(TEXT("CurveB"), 0.f);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();
	
	Hierarchy->ResetCurveValues();
	Unit.Curve = TEXT("CurveA");
	Unit.Value = 3.0f;
	Execute();

	AddErrorIfFalse(Hierarchy->GetCurveValue(CurveA) == 3.f, TEXT("unexpected value"));

	Hierarchy->ResetCurveValues();
	Unit.Curve = TEXT("CurveB");
	Unit.Value = 13.0f;
	Execute();

	AddErrorIfFalse(Hierarchy->GetCurveValue(CurveB) == 13.f, TEXT("unexpected value"));

	return true;
}
#endif
