// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigSchema.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "RigVMModel/RigVMController.h"
#include "Rigs/TLLRN_RigHierarchyPose.h"
#include "Curves/CurveFloat.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Modules/TLLRN_RigUnit_ConnectorExecution.h"
#include "Units/Modules/TLLRN_RigUnit_ConnectionCandidates.h"

UTLLRN_ControlRigSchema::UTLLRN_ControlRigSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetExecuteContextStruct(FTLLRN_ControlRigExecuteContext::StaticStruct());
}

bool UTLLRN_ControlRigSchema::ShouldUnfoldStruct(URigVMController* InController, const UStruct* InStruct) const
{
	RIGVMSCHEMA_DEFAULT_FUNCTION_BODY
	
	if(!Super::ShouldUnfoldStruct(InController, InStruct))
	{
		return false;
	}
	if (InStruct == TBaseStructure<FQuat>::Get())
	{
		return false;
	}
	if (InStruct == FRuntimeFloatCurve::StaticStruct())
	{
		return false;
	}
	if (InStruct == FTLLRN_RigPose::StaticStruct())
	{
		return false;
	}
	if (InStruct == FTLLRN_RigPhysicsSolverID::StaticStruct())
	{
		return false;
	}
	
	return true;
}

bool UTLLRN_ControlRigSchema::SupportsUnitFunction(URigVMController* InController, const FRigVMFunction* InUnitFunction) const
{
	if (InUnitFunction->Struct == FTLLRN_RigUnit_ConnectorExecution::StaticStruct() ||
		InUnitFunction->Struct == FTLLRN_RigUnit_GetCandidates::StaticStruct() ||
		InUnitFunction->Struct == FTLLRN_RigUnit_DiscardMatches::StaticStruct())
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(InController->GetOuter()))
		{
			if (Blueprint->IsTLLRN_ControlTLLRN_RigModule())
			{
				return true;
			}
			return false;
		}
	}
	return Super::SupportsUnitFunction(InController, InUnitFunction);
}
