// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"
#include "TLLRN_CRSimLinearSpring.h"
#include "TLLRN_CRSimPointForce.h"
#include "TLLRN_CRSimPointConstraint.h"
#include "TLLRN_CRSimSoftCollision.h"
#include "TLLRN_CRSimContainer.h"
#include "TLLRN_CRSimPointContainer.generated.h"

USTRUCT()
struct FTLLRN_CRSimPointContainer : public FTLLRN_CRSimContainer
{
	GENERATED_BODY()

	FTLLRN_CRSimPointContainer()
	{
	}

	/**
	 * The points within the simulation
	 */
	UPROPERTY()
	TArray<FRigVMSimPoint> Points;

	/**
	 * The springs within the simulation
	 */
	UPROPERTY()
	TArray<FTLLRN_CRSimLinearSpring> Springs;

	/**
	 * The forces to apply to the points
	 */
	UPROPERTY()
	TArray<FTLLRN_CRSimPointForce> Forces;

	/**
     * The collision volumes for the simulation
     */
	UPROPERTY()
	TArray<FTLLRN_CRSimSoftCollision> CollisionVolumes;

	/**
	 * The constraints within the simulation
	 */
	UPROPERTY()
	TArray<FTLLRN_CRSimPointConstraint> Constraints;

	FRigVMSimPoint GetPointInterpolated(int32 InIndex) const;

	virtual void Reset() override;
	virtual void ResetTime() override;

protected:

	UPROPERTY()
	TArray<FRigVMSimPoint> PreviousStep;

	virtual void CachePreviousStep() override;
	virtual void IntegrateVerlet(float InBlend) override;
	virtual void IntegrateSemiExplicitEuler() override;
	void IntegrateSprings();
	void IntegrateForcesAndVolumes();
	void IntegrateVelocityVerlet(float InBlend);
	void IntegrateVelocitySemiExplicitEuler();
	void ApplyConstraints();
};
