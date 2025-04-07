// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_UnsetCurveValue.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_UnsetCurveValue)

FTLLRN_RigUnit_UnsetCurveValue_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey Key(Curve, ETLLRN_RigElementType::Curve);
		if (CachedCurveIndex.UpdateCache(Key, Hierarchy))
		{
			Hierarchy->UnsetCurveValueByIndex(CachedCurveIndex);
		}
	}
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_UnsetCurveValue)
{
	const FTLLRN_RigElementKey Curve = Controller->AddCurve(TEXT("Curve"), 0.f);
	Unit.ExecuteContext.Hierarchy = Hierarchy.Get();
	
	Hierarchy->ResetCurveValues();
	Unit.Curve = TEXT("Curve");
	Execute();

	AddErrorIfFalse(!Hierarchy->IsCurveValueSet(Curve), TEXT("unexpected value"));

	return true;
}
#endif

