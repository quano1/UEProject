// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_AnimationHierarchy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_AnimationHierarchy)

FTransformConstraint* FTLLRN_ConstraintNodeData::FindConstraint(const FName& TargetNode)
{
	for (int32 ConstraintIdx = 0; ConstraintIdx < Constraints.Num(); ++ConstraintIdx)
	{
		if (Constraints[ConstraintIdx].TargetNode == TargetNode)
		{
			return &Constraints[ConstraintIdx];
		}
	}

	return nullptr;
}

void FTLLRN_ConstraintNodeData::AddConstraint(const FTransformConstraint& TransformConstraint)
{
	FTransformConstraint* ExistingConstraint = FindConstraint(TransformConstraint.TargetNode);
	if (ExistingConstraint)
	{
		*ExistingConstraint = TransformConstraint;
	}
	else
	{
		Constraints.Add(TransformConstraint);
	}
}

void FTLLRN_ConstraintNodeData::DeleteConstraint(const FName& TargetNode)
{
	for (int32 ConstraintIdx = 0; ConstraintIdx < Constraints.Num(); ++ConstraintIdx)
	{
		if (Constraints[ConstraintIdx].TargetNode == TargetNode)
		{
			Constraints.RemoveAt(ConstraintIdx);
			--ConstraintIdx;
		}
	}
}

