// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_TLLRN_RigCurveContainer.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "HelperUtil.h"
#include "Animation/Skeleton.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_TLLRN_RigCurveContainer)

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_TLLRN_RigCurveContainer
////////////////////////////////////////////////////////////////////////////////

FTLLRN_TLLRN_RigCurveContainer::FTLLRN_TLLRN_RigCurveContainer()
{
}

FTLLRN_RigCurve& FTLLRN_TLLRN_RigCurveContainer::Add(const FName& InNewName, float InValue)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

    FTLLRN_RigCurve NewCurve;
	NewCurve.Name = InNewName;
	NewCurve.Value = InValue;
	FName NewCurveName = NewCurve.Name;
	const int32 Index = Curves.Add(NewCurve);
	return Curves[Index];
}

