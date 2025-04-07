// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_CurveExists.generated.h"

/**
 * CurveExists is used to check whether a curve exists or not.
 */
USTRUCT(meta=(DisplayName="Curve Exists", Category="Curve", Keywords="CurveExists,bool", NodeColor="0.462745, 1,0, 0.329412", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CurveExists : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CurveExists()
		: Curve(NAME_None)
		, Exists(false)
		, CachedCurveIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Curve to retrieve the transform for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "CurveName"))
	FName Curve;

	/** Boolean indicating whether the named curve exists or not. 
	 *  Does not indicate whether the curve's value is valid or not.
	 */
	UPROPERTY(meta=(Output)) 
	bool Exists;

private:
	// Used to cache the internally used Curve index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedCurveIndex;
};
