// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_ProjectTransformToNewParent.h"
#include "RigVMFunctions/Math/RigVMFunction_MathTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetTransform.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_ProjectTransformToNewParent)

FTLLRN_RigUnit_ProjectTransformToNewParent_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	FTransform ChildTransform = FTransform::Identity;
	FTransform OldParentTransform = FTransform::Identity;
	FTransform NewParentTransform = FTransform::Identity;
	FTransform RelativeTransform = FTransform::Identity;

	FTLLRN_RigUnit_GetTransform::StaticExecute(ExecuteContext, Child, ERigVMTransformSpace::GlobalSpace, bChildInitial, ChildTransform, CachedChild);
	FTLLRN_RigUnit_GetTransform::StaticExecute(ExecuteContext, OldParent, ERigVMTransformSpace::GlobalSpace, bOldParentInitial, OldParentTransform, CachedOldParent);
	FTLLRN_RigUnit_GetTransform::StaticExecute(ExecuteContext, NewParent, ERigVMTransformSpace::GlobalSpace, bNewParentInitial, NewParentTransform, CachedNewParent);
	FRigVMFunction_MathTransformMakeRelative::StaticExecute(ExecuteContext, ChildTransform, OldParentTransform, RelativeTransform);
	FRigVMFunction_MathTransformMakeAbsolute::StaticExecute(ExecuteContext, RelativeTransform, NewParentTransform, Transform);
}

