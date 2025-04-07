// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RigVMFunctions/Debug/RigVMFunction_DebugBase.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "TLLRN_RigUnit_DebugHierarchy.generated.h"

UENUM()
namespace ETLLRN_ControlRigDrawHierarchyMode
{
	enum Type : int
	{
		/** Draw as axes */
		Axes,

		/** MAX - invalid */
		Max UMETA(Hidden),
	};
}

/**
 * Draws vectors on each bone in the viewport across the entire hierarchy
 */
USTRUCT(meta=(DisplayName="Draw Hierarchy"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugHierarchy : public FRigVMFunction_DebugBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugHierarchy()
	{
		Scale = 10.f;
		Color = FLinearColor::White;
		Thickness = 0.f;
		WorldOffset = FTransform::Identity;
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(DisplayName = "Execute", Transient, meta = (Input, Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	// the items to draw the pose for.
	// if this is empty we'll draw the whole hierarchy
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> Items;

	UPROPERTY(meta = (Input))
	float Scale;

	UPROPERTY(meta = (Input))
	FLinearColor Color;

	UPROPERTY(meta = (Input))
	float Thickness;

	UPROPERTY(meta = (Input))
	FTransform WorldOffset;

	UPROPERTY(meta = (Input))
	bool bEnabled;

	static void DrawHierarchy(const FRigVMExecuteContext& InContext, const FTransform& WorldOffset, UTLLRN_RigHierarchy* Hierarchy, ETLLRN_ControlRigDrawHierarchyMode::Type Mode, float Scale, const FLinearColor& Color, float Thickness, const FTLLRN_RigPose* InPose, const TArrayView<const FTLLRN_RigElementKey>* InItems);
};

/**
* Draws vectors on each bone in the viewport across the entire pose
*/
USTRUCT(meta=(DisplayName="Draw Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugPose : public FRigVMFunction_DebugBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_DebugPose()
	{
		Scale = 10.f;
		Color = FLinearColor::White;
		Thickness = 0.f;
		WorldOffset = FTransform::Identity;
		bEnabled = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(DisplayName = "Execute", Transient, meta = (Input, Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose Pose;

	// the items to draw the pose cache for.
	// if this is empty we'll draw the whole pose cache
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> Items;

	UPROPERTY(meta = (Input))
	float Scale;

	UPROPERTY(meta = (Input))
	FLinearColor Color;

	UPROPERTY(meta = (Input))
	float Thickness;

	UPROPERTY(meta = (Input))
	FTransform WorldOffset;

	UPROPERTY(meta = (Input))
	bool bEnabled;
};