// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_GetRelativeTransform.h"
#include "RigVMFunctions/Math/RigVMFunction_MathTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetTransform.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_GetRelativeTransform)

FTLLRN_RigUnit_GetRelativeTransformForItem_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	FTransform ChildTransform = FTransform::Identity;
	FTransform ParentTransform = FTransform::Identity;

	FTLLRN_RigUnit_GetTransform::StaticExecute(ExecuteContext, Child, ERigVMTransformSpace::GlobalSpace, bChildInitial, ChildTransform, CachedChild);
	FTLLRN_RigUnit_GetTransform::StaticExecute(ExecuteContext, Parent, ERigVMTransformSpace::GlobalSpace, bParentInitial, ParentTransform, CachedParent);
	FRigVMFunction_MathTransformMakeRelative::StaticExecute(ExecuteContext, ChildTransform, ParentTransform, RelativeTransform);
}
