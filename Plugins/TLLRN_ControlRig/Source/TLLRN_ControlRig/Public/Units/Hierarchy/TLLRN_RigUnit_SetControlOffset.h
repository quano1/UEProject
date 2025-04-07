// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_SetControlOffset.generated.h"

/**
 * SetControlOffset is used to perform a change in the hierarchy by setting a single control's transform.
 * This is typically only used during the Construction Event.
 */
USTRUCT(meta=(DisplayName="Set Control Offset", Category="Controls", DocumentationPolicy="Strict", Keywords = "SetControlOffset,Initial,InitialTransform,SetInitialTransform,SetInitialControlTransform", NodeColor="0, 0.364706, 1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetControlOffset : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetControlOffset()
		: Control(NAME_None)
		, Space(ERigVMTransformSpace::GlobalSpace)
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to set the transform for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "ControlName" ))
	FName Control;

	/**
	 * The offset transform to set for the control
	 */
	UPROPERTY(meta = (Input, Output))
	FTransform Offset;

	/**
	 * Defines if the bone's transform should be set
	 * in local or global space.
	 */
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	// user to internally cache the index of the bone
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedControlIndex;
};

/**
 * GetShapeTransform is used to retrieve single control's shape transform.
 * This is typically only used during the Construction Event.
 */
USTRUCT(meta=(DisplayName="Get Shape Transform", Category="Controls", DocumentationPolicy="Strict", Keywords = "GetControlShapeTransform,Gizmo,GizmoTransform,MeshTransform", NodeColor="0, 0.364706, 1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetShapeTransform : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetShapeTransform()
		: Control(NAME_None)
	{
		Transform = FTransform::Identity;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to set the transform for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "ControlName" ))
	FName Control;

	/**
	 * The shape transform to set for the control
	 */
	UPROPERTY(meta = (Output))
	FTransform Transform;

	// user to internally cache the index of the bone
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedControlIndex;
};

/**
 * SetShapeTransform is used to perform a change in the hierarchy by setting a single control's shape transform.
 * This is typically only used during the Construction Event.
 */
USTRUCT(meta=(DisplayName="Set Shape Transform", Category="Controls", DocumentationPolicy="Strict", Keywords = "SetControlShapeTransform,Gizmo,GizmoTransform,MeshTransform", NodeColor="0, 0.364706, 1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetShapeTransform : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetShapeTransform()
		: Control(NAME_None)
	{
		Transform = FTransform::Identity;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to set the transform for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "ControlName" ))
	FName Control;

	/**
	 * The shape transform to set for the control
	 */
	UPROPERTY(meta = (Input))
	FTransform Transform;

	// user to internally cache the index of the bone
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedControlIndex;
};
