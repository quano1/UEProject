// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_UnsetCurveValue.generated.h"


/**
 * UnsetCurveValue is used to perform a change in the curve container by invalidating a single Curve value.
 */
USTRUCT(meta=(DisplayName="Unset Curve Value", Category="Curve", Keywords = "UnsetCurveValue", NodeColor = "0.0 0.36470600962638855 1.0"))
struct FTLLRN_RigUnit_UnsetCurveValue : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_UnsetCurveValue()
		: CachedCurveIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Curve to set the Value for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "CurveName"))
	FName Curve;

private:
	// Used to cache the internally used curve index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedCurveIndex;
};
