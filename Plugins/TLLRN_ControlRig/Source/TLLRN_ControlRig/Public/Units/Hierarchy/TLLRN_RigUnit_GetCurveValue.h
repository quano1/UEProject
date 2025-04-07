// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_GetCurveValue.generated.h"

/**
 * GetCurveValue is used to retrieve a single float from a Curve
 */
USTRUCT(meta=(DisplayName="Get Curve Value", Category="Curve", Keywords="GetCurveValue,float", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetCurveValue : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetCurveValue()
		: Curve(NAME_None)
		, Valid(true)
		, Value(0.f)
		, CachedCurveIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Curve to retrieve the transform for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "CurveName"))
	FName Curve;
	
	UPROPERTY(meta=(Output)) 
	bool Valid;

	// The current transform of the given Curve - or identity in case it wasn't found.
	UPROPERTY(meta=(Output, UIMin = 0.f, UIMax = 1.f))
	float Value;

private:
	// Used to cache the internally used Curve index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedCurveIndex;
};
