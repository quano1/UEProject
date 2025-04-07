// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_CurveExists.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_CurveExists)

FTLLRN_RigUnit_CurveExists_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	
	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;

	Exists = Hierarchy && CachedCurveIndex.UpdateCache(FTLLRN_RigElementKey(Curve, ETLLRN_RigElementType::Curve), Hierarchy);
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Units/TLLRN_RigUnitTest.h"

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_CurveExists)
{
	static const FName CurveName("Curve");
	Controller->AddCurve(CurveName, 0.f);
	
	Unit.Curve = CurveName;
	Execute();

	AddErrorIfFalse(Unit.Exists, TEXT("Expected curve to exist"));

	Unit.Curve = TEXT("NonExistentCurve");
	Execute();

	AddErrorIfFalse(!Unit.Exists, TEXT("Expected curve to not exist."));

	Unit.Curve = NAME_None;
	Execute();

	AddErrorIfFalse(!Unit.Exists, TEXT("Expected curve to not exist."));
	
	return true;
}
#endif
