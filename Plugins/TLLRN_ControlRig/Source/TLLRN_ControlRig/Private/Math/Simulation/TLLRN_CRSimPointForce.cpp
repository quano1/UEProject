// Copyright Epic Games, Inc. All Rights Reserved.

#include "Math/Simulation/TLLRN_CRSimPointForce.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_CRSimPointForce)

FVector FTLLRN_CRSimPointForce::Calculate(const FRigVMSimPoint& InPoint, float InDeltaTime) const
{
	FVector Force = FVector::ZeroVector;
	if(InPoint.Mass > SMALL_NUMBER)
	{
		switch(ForceType)
		{
			case ETLLRN_CRSimPointForceType::Direction:
			{
				if(bNormalize)
				{
					Force = Vector.GetSafeNormal();
				}
				else
				{
					Force = Vector;
				}
				Force = Force * Coefficient;
				break;
			}
		}
	}
	return Force;
}

