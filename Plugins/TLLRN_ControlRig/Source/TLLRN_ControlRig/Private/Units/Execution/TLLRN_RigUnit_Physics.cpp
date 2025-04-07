// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_Physics.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "TLLRN_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_Physics)

FTLLRN_RigUnit_HierarchyAddPhysicsSolver_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Solver = FTLLRN_RigPhysicsSolverID();

	if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ExecuteContext.Hierarchy->GetOuter()))
	{
		Solver = TLLRN_ControlRig->AddPhysicsSolver(Name, false, false);
	}
}

FTLLRN_RigUnit_HierarchyAddPhysicsJoint_Execute()
{
	FString ErrorMessage;
	if(!FTLLRN_RigUnit_DynamicHierarchyBase::IsValidToRunInContext(ExecuteContext, true, &ErrorMessage))
	{
		if(!ErrorMessage.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(TEXT("%s"), *ErrorMessage);
		}
		return;
	}

	Item.Reset();

	if(UTLLRN_RigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true))
	{
		FTLLRN_RigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());
		Item = Controller->AddPhysicsElement(Name, Parent, Solver, Settings, Transform, false, false);
	}
}
