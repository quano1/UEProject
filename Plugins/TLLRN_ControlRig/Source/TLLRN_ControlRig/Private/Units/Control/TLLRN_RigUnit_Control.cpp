// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Control/TLLRN_RigUnit_Control.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_Control)

FTLLRN_RigUnit_Control_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (!bIsInitialized)
	{
		Transform.FromFTransform(InitTransform);
		bIsInitialized = true;
	}

	Result = StaticGetResultantTransform(Transform, Base, Filter);
}

FTransform FTLLRN_RigUnit_Control::GetResultantTransform() const
{
	return StaticGetResultantTransform(Transform, Base, Filter);
}

FTransform FTLLRN_RigUnit_Control::StaticGetResultantTransform(const FEulerTransform& InTransform, const FTransform& InBase, const FTransformFilter& InFilter)
{
	return StaticGetFilteredTransform(InTransform, InFilter).ToFTransform() * InBase;
}

FMatrix FTLLRN_RigUnit_Control::GetResultantMatrix() const
{
	return StaticGetResultantMatrix(Transform, Base, Filter);
}

FMatrix FTLLRN_RigUnit_Control::StaticGetResultantMatrix(const FEulerTransform& InTransform, const FTransform& InBase, const FTransformFilter& InFilter)
{
	const FEulerTransform FilteredTransform = StaticGetFilteredTransform(InTransform, InFilter);
	return FScaleRotationTranslationMatrix(FilteredTransform.Scale, FilteredTransform.Rotation, FilteredTransform.Location) * InBase.ToMatrixWithScale();
}

void FTLLRN_RigUnit_Control::SetResultantTransform(const FTransform& InResultantTransform)
{
	StaticSetResultantTransform(InResultantTransform, Base, Transform);
}

void FTLLRN_RigUnit_Control::StaticSetResultantTransform(const FTransform& InResultantTransform, const FTransform& InBase, FEulerTransform& OutTransform)
{
	OutTransform.FromFTransform(InResultantTransform.GetRelativeTransform(InBase));
}

void FTLLRN_RigUnit_Control::SetResultantMatrix(const FMatrix& InResultantMatrix)
{
	StaticSetResultantMatrix(InResultantMatrix, Base, Transform);
}

void FTLLRN_RigUnit_Control::StaticSetResultantMatrix(const FMatrix& InResultantMatrix, const FTransform& InBase, FEulerTransform& OutTransform)
{
	const FMatrix RelativeTransform = InResultantMatrix * InBase.ToMatrixWithScale().Inverse();

	OutTransform.Location = RelativeTransform.GetOrigin();
	OutTransform.Rotation = RelativeTransform.Rotator();
	OutTransform.Scale = RelativeTransform.GetScaleVector();
}

FEulerTransform FTLLRN_RigUnit_Control::GetFilteredTransform() const
{
	return StaticGetFilteredTransform(Transform, Filter);
}

FEulerTransform FTLLRN_RigUnit_Control::StaticGetFilteredTransform(const FEulerTransform& InTransform, const FTransformFilter& InFilter)
{
	FEulerTransform FilteredTransform = InTransform;
	InFilter.FilterTransform(FilteredTransform);
	return FilteredTransform;
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_Control::GetUpgradeInfo() const
{
	return FTLLRN_RigUnit::GetUpgradeInfo();
}

