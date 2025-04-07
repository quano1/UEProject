// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_WorldSpace.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_WorldSpace)

FTLLRN_RigUnit_ToWorldSpace_Transform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
    World = ExecuteContext.ToWorldSpace(Value);
}

FTLLRN_RigUnit_ToTLLRN_RigSpace_Transform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
    Global = ExecuteContext.ToVMSpace(Value);
}

FTLLRN_RigUnit_ToWorldSpace_Location_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
    World = ExecuteContext.ToWorldSpace(Value);
}

FTLLRN_RigUnit_ToTLLRN_RigSpace_Location_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
    Global = ExecuteContext.ToVMSpace(Value);
}

FTLLRN_RigUnit_ToWorldSpace_Rotation_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
    World = ExecuteContext.ToWorldSpace(Value);
}

FTLLRN_RigUnit_ToTLLRN_RigSpace_Rotation_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
    Global = ExecuteContext.ToVMSpace(Value);
}
