// Copyright Epic Games, Inc. All Rights Reserved.

#include "Math/Simulation/TLLRN_CRSimPointContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_CRSimPointContainer)

void FTLLRN_CRSimPointContainer::Reset()
{
	FTLLRN_CRSimContainer::Reset();
	Points.Reset();
	Springs.Reset();
	Constraints.Reset();
	PreviousStep.Reset();
}

void FTLLRN_CRSimPointContainer::ResetTime()
{
	FTLLRN_CRSimContainer::ResetTime();
	PreviousStep.Reset();
	for (FRigVMSimPoint& Point : Points)
	{
		Point.LinearVelocity = FVector::ZeroVector;
	}
}

FRigVMSimPoint FTLLRN_CRSimPointContainer::GetPointInterpolated(int32 InIndex) const
{
	if (TimeLeftForStep <= SMALL_NUMBER || PreviousStep.Num() != Points.Num())
	{
		return Points[InIndex];
	}

	float T = 1.f - TimeLeftForStep / TimeStep;
	const FRigVMSimPoint& PrevPoint = PreviousStep[InIndex];
	FRigVMSimPoint Point = Points[InIndex];
	Point.Position = FMath::Lerp<FVector>(PrevPoint.Position, Point.Position, T);
	Point.Size = FMath::Lerp<float>(PrevPoint.Size, Point.Size, T);
	Point.LinearVelocity = FMath::Lerp<FVector>(PrevPoint.LinearVelocity, Point.LinearVelocity, T);
	return Point;
}

void FTLLRN_CRSimPointContainer::CachePreviousStep()
{
	PreviousStep = Points;
	for (FRigVMSimPoint& Point : Points)
	{
		Point.LinearVelocity = FVector::ZeroVector;
	}
}

void FTLLRN_CRSimPointContainer::IntegrateVerlet(float InBlend)
{
	IntegrateSprings();
	IntegrateForcesAndVolumes();
	IntegrateVelocityVerlet(InBlend);
	ApplyConstraints();
}

void FTLLRN_CRSimPointContainer::IntegrateSemiExplicitEuler()
{
	IntegrateSprings();
	IntegrateForcesAndVolumes();
	IntegrateVelocitySemiExplicitEuler();
	ApplyConstraints();
}

void FTLLRN_CRSimPointContainer::IntegrateSprings()
{
	for (int32 SpringIndex = 0; SpringIndex < Springs.Num(); SpringIndex++)
	{
		const FTLLRN_CRSimLinearSpring& Spring = Springs[SpringIndex];
		if (Spring.SubjectA == INDEX_NONE || Spring.SubjectB == INDEX_NONE)
		{
			continue;
		}
		const FRigVMSimPoint& PrevPointA = PreviousStep[Spring.SubjectA];
		const FRigVMSimPoint& PrevPointB = PreviousStep[Spring.SubjectB];

		FVector ForceA = FVector::ZeroVector;
		FVector ForceB = FVector::ZeroVector;
		Spring.CalculateForPoints(PrevPointA, PrevPointB, ForceA, ForceB);

		FRigVMSimPoint& PointA = Points[Spring.SubjectA];
		FRigVMSimPoint& PointB = Points[Spring.SubjectB];
		PointA.LinearVelocity += ForceA;
		PointB.LinearVelocity += ForceB;
	}
}

void FTLLRN_CRSimPointContainer::IntegrateForcesAndVolumes()
{
	for (int32 PointIndex = 0; PointIndex < Points.Num(); PointIndex++)
	{
		for (int32 ForceIndex = 0; ForceIndex < Forces.Num(); ForceIndex++)
		{
			Points[PointIndex].LinearVelocity += Forces[ForceIndex].Calculate(PreviousStep[PointIndex], TimeStep);
		}
		for (int32 VolumeIndex = 0; VolumeIndex < CollisionVolumes.Num(); VolumeIndex++)
		{
			Points[PointIndex].LinearVelocity += CollisionVolumes[VolumeIndex].CalculateForPoint(PreviousStep[PointIndex], TimeStep);
		}
	}
}

void FTLLRN_CRSimPointContainer::IntegrateVelocityVerlet(float InBlend)
{
	for (int32 PointIndex = 0; PointIndex < Points.Num(); PointIndex++)
	{
		Points[PointIndex] = PreviousStep[PointIndex].IntegrateVerlet(Points[PointIndex].LinearVelocity, InBlend, TimeStep);
	}
}

void FTLLRN_CRSimPointContainer::IntegrateVelocitySemiExplicitEuler()
{
	for (int32 PointIndex = 0; PointIndex < Points.Num(); PointIndex++)
	{
		Points[PointIndex] = PreviousStep[PointIndex].IntegrateSemiExplicitEuler(Points[PointIndex].LinearVelocity, TimeStep);
	}
}

void FTLLRN_CRSimPointContainer::ApplyConstraints()
{
	for (int32 ConstraintIndex = 0; ConstraintIndex < Constraints.Num(); ConstraintIndex++)
	{
		const FTLLRN_CRSimPointConstraint& Constraint = Constraints[ConstraintIndex];
		if (Constraint.SubjectA == INDEX_NONE)
		{
			continue;
		}

		FRigVMSimPoint& PointA = Points[Constraint.SubjectA];
		FRigVMSimPoint& PointB = Points[Constraint.SubjectB == INDEX_NONE ? Constraint.SubjectA : Constraint.SubjectB];
		Constraint.Apply(PointA, PointB);
	}
}

