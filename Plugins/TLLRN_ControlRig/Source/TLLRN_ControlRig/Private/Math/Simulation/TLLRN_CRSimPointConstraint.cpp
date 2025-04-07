// Copyright Epic Games, Inc. All Rights Reserved.

#include "Math/Simulation/TLLRN_CRSimPointConstraint.h"
#include "Math/Simulation/CRSimUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_CRSimPointConstraint)

void FTLLRN_CRSimPointConstraint::Apply(FRigVMSimPoint& OutPointA, FRigVMSimPoint& OutPointB) const
{
	switch (Type)
	{
		case ETLLRN_CRSimConstraintType::Distance:
		case ETLLRN_CRSimConstraintType::DistanceFromA:
		case ETLLRN_CRSimConstraintType::DistanceFromB:
		{
			float WeightA = 0.f, WeightB = 0.f;
			switch (Type)
			{
				case ETLLRN_CRSimConstraintType::Distance:
				{
					FCRSimUtils::ComputeWeightsFromMass(OutPointA.Mass, OutPointB.Mass, WeightA, WeightB);
					break;
				}
				case ETLLRN_CRSimConstraintType::DistanceFromA:
				{
					WeightA = 0.f;
					WeightB = 1.f;
					break;
				}
				default:
				case ETLLRN_CRSimConstraintType::DistanceFromB:
				{
					WeightA = 1.f;
					WeightB = 0.f;
					break;
				}
			}
			if (WeightA + WeightB <= SMALL_NUMBER)
			{
				return;
			}

			const float Equilibrium = DataA.X;
			const FVector Direction = OutPointA.Position - OutPointB.Position;
			const float Distance = Direction.Size();
			if (Distance < SMALL_NUMBER)
			{
				break;
			}
			const FVector Displacement = Direction * (Equilibrium - Distance) / Distance;
			OutPointA.Position += Displacement * WeightA;
			OutPointB.Position -= Displacement * WeightB;
			break;
		}
		case ETLLRN_CRSimConstraintType::Plane:
		{
			const FVector& PlanePoint = DataA;
			const FVector& PlaneNormal = DataB;
			FVector Direction = OutPointA.Position - PlanePoint;
			OutPointA.Position -= FVector::DotProduct(PlaneNormal, Direction) * PlaneNormal;
			break;
		}
	}
}

