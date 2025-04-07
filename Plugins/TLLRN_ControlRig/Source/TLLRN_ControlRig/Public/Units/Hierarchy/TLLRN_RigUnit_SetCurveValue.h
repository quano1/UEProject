// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_SetCurveValue.generated.h"


/**
 * SetCurveValue is used to perform a change in the curve container by setting a single Curve value.
 */
USTRUCT(meta=(DisplayName="Set Curve Value", Category="Curve", Keywords = "SetCurveValue", NodeColor = "0.0 0.36470600962638855 1.0"))
struct FTLLRN_RigUnit_SetCurveValue : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetCurveValue()
		: Value(0.f)
		, CachedCurveIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Curve to set the Value for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "CurveName"))
	FName Curve;

	/**
	 * The value to set for the given Curve.
	 */
	UPROPERTY(meta = (Input))
	float Value;

private:
	// Used to cache the internally used curve index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedCurveIndex;
};
