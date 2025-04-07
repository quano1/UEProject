// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "Constraint.h"
#include "TLLRN_ControlRigDefines.h"
#include "TLLRN_RigUnit_GetJointTransform.generated.h"

USTRUCT(meta=(DisplayName="Get Joint Transform", Category="Transforms", Deprecated = "4.23.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetJointTransform : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetJointTransform()
		: Type(ETLLRN_TransformGetterType::Current)
		, TransformSpace(ETLLRN_TransformSpaceMode::GlobalSpace)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FName Joint;

	UPROPERTY(meta = (Input))
	ETLLRN_TransformGetterType Type;

	UPROPERTY(meta = (Input))
	ETLLRN_TransformSpaceMode TransformSpace;

	// Transform op option. Use if ETransformSpace is BaseTransform
	UPROPERTY(meta = (Input))
	FTransform BaseTransform;

	// Transform op option. Use if ETransformSpace is BaseJoint
	UPROPERTY(meta = (Input))
	FName BaseJoint;

	// possibly space, relative transform so on can be input
	UPROPERTY(meta=(Output))
	FTransform Output;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
