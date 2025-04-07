// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Simulation/TLLRN_RigUnit_PointSimulation.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_PointSimulation)

FTLLRN_RigUnit_PointSimulation_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_PointSimulation::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

