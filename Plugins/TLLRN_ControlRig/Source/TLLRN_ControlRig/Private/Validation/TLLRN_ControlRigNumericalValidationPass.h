// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "TLLRN_ControlRigValidationPass.h"

#include "TLLRN_ControlRigNumericalValidationPass.generated.h"


/** Used to perform a numerical comparison of the poses */
UCLASS(DisplayName="Numerical Comparison")
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigNumericalValidationPass : public UTLLRN_ControlRigValidationPass
{
	GENERATED_UCLASS_BODY()

public:

	// UTLLRN_ControlRigValidationPass interface
	virtual void OnSubjectChanged(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_ControlRigValidationContext* InContext) override;
	virtual void OnInitialize(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_ControlRigValidationContext* InContext) override;
	virtual void OnEvent(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName, FTLLRN_ControlRigValidationContext* InContext) override;

	// If set to true the pass will validate the poses of all bones
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bCheckControls;

	// If set to true the pass will validate the poses of all bones
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bCheckBones;

	// If set to true the pass will validate the values of all curves
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bCheckCurves;

	// The threshold under which we'll ignore a precision issue in the pass
	UPROPERTY(EditAnywhere, Category = "Settings")
	float TranslationPrecision;

	// The threshold under which we'll ignore a precision issue in the pass (in degrees)
	UPROPERTY(EditAnywhere, Category = "Settings")
	float RotationPrecision;

	// The threshold under which we'll ignore a precision issue in the pass
	UPROPERTY(EditAnywhere, Category = "Settings")
	float ScalePrecision;

	// The threshold under which we'll ignore a precision issue in the pass
	UPROPERTY(EditAnywhere, Category = "Settings")
	float CurvePrecision;

private:

	UPROPERTY(transient)
	FName EventNameA;
	
	UPROPERTY(transient)
	FName EventNameB;
	
	UPROPERTY(transient)
	FTLLRN_RigPose Pose;
};
