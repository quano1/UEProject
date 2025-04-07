// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigHierarchyPose.h"
#include "Rigs/TLLRN_RigHierarchy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigHierarchyPose)


FTLLRN_RigPoseElement::FTLLRN_RigPoseElement(): Index()
                                  , GlobalTransform(FTransform::Identity)
                                  , LocalTransform(FTransform::Identity)
                                  , PreferredEulerAngle(FVector::ZeroVector)
                                  , ActiveParent(UTLLRN_RigHierarchy::GetDefaultParentKey())
                                  , CurveValue(0.f)
{
}
