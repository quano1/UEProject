// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Simulation/TLLRN_RigUnit_SpringInterp.h"
#include "TLLRN_ControlRigDefines.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SpringInterp)

namespace TLLRN_RigUnitSpringInterpConstants
{
	static const float FixedTimeStep = 1.0f / 60.0f;
	static const float MaxTimeStep = 0.1f;
	static const float Mass = 1.0f;
}

FTLLRN_RigUnit_SpringInterp_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
 
	// Clamp to avoid large time deltas.
	float RemainingTime = FMath::Min(ExecuteContext.GetDeltaTime(), TLLRN_RigUnitSpringInterpConstants::MaxTimeStep);

	Result = Current;
	while (RemainingTime >= TLLRN_RigUnitSpringInterpConstants::FixedTimeStep)
	{
		Result = UKismetMathLibrary::FloatSpringInterp(Result, Target, SpringState, Stiffness, CriticalDamping, TLLRN_RigUnitSpringInterpConstants::FixedTimeStep, Mass);
		RemainingTime -= TLLRN_RigUnitSpringInterpConstants::FixedTimeStep;
	}

	Result = UKismetMathLibrary::FloatSpringInterp(Result, Target, SpringState, Stiffness, CriticalDamping, RemainingTime, Mass);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SpringInterp::GetUpgradeInfo() const
{
	// this node is no longer supported and the new implementation is vastly different
	return FRigVMStructUpgradeInfo();
}
 
FTLLRN_RigUnit_SpringInterpVector_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
 
	// Clamp to avoid large time deltas.
	float RemainingTime = FMath::Min(ExecuteContext.GetDeltaTime(), TLLRN_RigUnitSpringInterpConstants::MaxTimeStep);

	Result = Current;
	while (RemainingTime >= TLLRN_RigUnitSpringInterpConstants::FixedTimeStep)
	{
		Result = UKismetMathLibrary::VectorSpringInterp(Result, Target, SpringState, Stiffness, CriticalDamping, TLLRN_RigUnitSpringInterpConstants::FixedTimeStep, Mass);
		RemainingTime -= TLLRN_RigUnitSpringInterpConstants::FixedTimeStep;
	}

	Result = UKismetMathLibrary::VectorSpringInterp(Result, Target, SpringState, Stiffness, CriticalDamping, RemainingTime, Mass);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SpringInterpVector::GetUpgradeInfo() const
{
	// this node is no longer supported and the new implementation is vastly different
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_SpringInterpV2_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	// Treat the input as a frequency in Hz
	float AngularFrequency = Strength * 2.0f * PI;
	float Stiffness = AngularFrequency * AngularFrequency;
	float AdjustedTarget = Target;
	if (!FMath::IsNearlyZero(Stiffness))
	{
		AdjustedTarget += Force / (Stiffness * TLLRN_RigUnitSpringInterpConstants::Mass);
	}
	else
	{
		SpringState.Velocity += Force * (ExecuteContext.GetDeltaTime() / TLLRN_RigUnitSpringInterpConstants::Mass);
	}
	SimulatedResult = UKismetMathLibrary::FloatSpringInterp(
		bUseCurrentInput ? Current : SimulatedResult, AdjustedTarget, SpringState, Stiffness, CriticalDamping,
		ExecuteContext.GetDeltaTime(), TLLRN_RigUnitSpringInterpConstants::Mass, TargetVelocityAmount, 
		false, 0.0f, 0.0f, !bUseCurrentInput || bInitializeFromTarget);

	Result = SimulatedResult;
	Velocity = SpringState.Velocity;
}

FTLLRN_RigUnit_SpringInterpVectorV2_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	// Treat the input as a frequency in Hz
	float AngularFrequency = Strength * 2.0f * PI;
	float Stiffness = AngularFrequency * AngularFrequency;
	FVector AdjustedTarget = Target;
	if (!FMath::IsNearlyZero(Stiffness))
	{
		AdjustedTarget += Force / (Stiffness * TLLRN_RigUnitSpringInterpConstants::Mass);
	}
	else
	{
		SpringState.Velocity += Force * (ExecuteContext.GetDeltaTime() / TLLRN_RigUnitSpringInterpConstants::Mass);
	}
	SimulatedResult = UKismetMathLibrary::VectorSpringInterp(
		bUseCurrentInput ? Current : SimulatedResult, AdjustedTarget, SpringState, Stiffness, CriticalDamping,
		ExecuteContext.GetDeltaTime(), TLLRN_RigUnitSpringInterpConstants::Mass, TargetVelocityAmount,
		false, FVector(), FVector(), !bUseCurrentInput || bInitializeFromTarget);
	Result = SimulatedResult;
}

FTLLRN_RigUnit_SpringInterpQuaternionV2_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	// Treat the input as a frequency in Hz
	float AngularFrequency = Strength * 2.0f * PI;
	float Stiffness = AngularFrequency * AngularFrequency;
	SpringState.AngularVelocity += Torque * (ExecuteContext.GetDeltaTime() / TLLRN_RigUnitSpringInterpConstants::Mass);
	SimulatedResult = UKismetMathLibrary::QuaternionSpringInterp(
		bUseCurrentInput ? Current : SimulatedResult, Target, SpringState, Stiffness, CriticalDamping,
		ExecuteContext.GetDeltaTime(), TLLRN_RigUnitSpringInterpConstants::Mass, TargetVelocityAmount, 
		!bUseCurrentInput || bInitializeFromTarget);
	Result = SimulatedResult;
	AngularVelocity = SpringState.AngularVelocity;
}


