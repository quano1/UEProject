// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_GetCurveValue.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_GetCurveValue)

FTLLRN_RigUnit_GetCurveValue_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
    if (const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy)
	{
		if(CachedCurveIndex.UpdateCache(FTLLRN_RigElementKey(Curve, ETLLRN_RigElementType::Curve), Hierarchy))
		{
			Valid = Hierarchy->IsCurveValueSetByIndex(CachedCurveIndex);
			Value = Hierarchy->GetCurveValueByIndex(CachedCurveIndex);
			return;
		}
	}
	Valid = false;
	Value = 0.0f;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_GetCurveValue)
{
	static const FName ValidCurveName("ValidCurve");
	static const FName InvalidCurveName("InvalidCurve");
	static const FName UndefinedCurveName(NAME_None);
	
	const FTLLRN_RigElementKey ValidCurve = Controller->AddCurve(ValidCurveName, 1.f);
	const FTLLRN_RigElementKey InvalidCurve = Controller->AddCurve(InvalidCurveName, 1.f);

	Hierarchy->ResetCurveValues();
	Hierarchy->UnsetCurveValue(InvalidCurve);
	
	Unit.Curve = ValidCurveName;
	Execute();
	AddErrorIfFalse(!Unit.Valid, TEXT("Expected curve hold a unset value"));
	Execute();
	AddErrorIfFalse(Unit.Value != 1.0f, TEXT("Expected curve's value to be 1.0"));

	Hierarchy->SetCurveValue({ValidCurveName, ETLLRN_RigElementType::Curve}, 2.0f);
	Execute();
	AddErrorIfFalse(Unit.Valid, TEXT("Expected curve hold a valid value"));
	Execute();
	AddErrorIfFalse(Unit.Value == 2.0f, TEXT("Expected curve's value to be 2.0"));

	Unit.Curve = InvalidCurveName;
	Execute();
	AddErrorIfFalse(!Unit.Valid, TEXT("Expected curve hold an invalid value"));

	Unit.Curve = UndefinedCurveName;
	Execute();
	AddErrorIfFalse(!Unit.Valid, TEXT("Expected undefined curve hold an invalid value"));
	
	return true;
}
#endif
