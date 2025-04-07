// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetControlOffset.h"

#include "GeometryCollection/GeometryCollectionAlgo.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetControlOffset)

FTLLRN_RigUnit_SetControlOffset_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
			return;
		}

		FTLLRN_RigControlElement* ControlElement = Hierarchy->Get<FTLLRN_RigControlElement>(CachedControlIndex);
		if (Space == ERigVMTransformSpace::GlobalSpace)
		{
			Hierarchy->SetControlOffsetTransform(ControlElement, Offset, ETLLRN_RigTransformType::CurrentGlobal, true, false);
			Hierarchy->SetControlOffsetTransform(ControlElement, Offset, ETLLRN_RigTransformType::InitialGlobal, true, false);
		}
		else
		{
			Hierarchy->SetControlOffsetTransform(ControlElement, Offset, ETLLRN_RigTransformType::CurrentLocal, true, false);
			Hierarchy->SetControlOffsetTransform(ControlElement, Offset, ETLLRN_RigTransformType::InitialLocal, true, false);
		}
	}
}

FTLLRN_RigUnit_GetShapeTransform_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
			return;
		}

		const FTLLRN_RigControlElement* ControlElement = Hierarchy->Get<FTLLRN_RigControlElement>(CachedControlIndex);
		Transform = Hierarchy->GetControlShapeTransform((FTLLRN_RigControlElement*)ControlElement, ETLLRN_RigTransformType::CurrentLocal);
	}
}

FTLLRN_RigUnit_SetShapeTransform_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		if (!CachedControlIndex.UpdateCache(FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control), Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Control '%s' is not valid."), *Control.ToString());
			return;
		}

		FTLLRN_RigControlElement* ControlElement = Hierarchy->Get<FTLLRN_RigControlElement>(CachedControlIndex);
		Hierarchy->SetControlShapeTransform(ControlElement, Transform, ETLLRN_RigTransformType::InitialLocal);
	}
}
