// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"
#include "TLLRN_CRSimPointConstraint.generated.h"

UENUM()
enum class ETLLRN_CRSimConstraintType : uint8
{
	Distance,
	DistanceFromA,
	DistanceFromB,
	Plane
};

USTRUCT()
struct FTLLRN_CRSimPointConstraint
{
	GENERATED_BODY()

	FTLLRN_CRSimPointConstraint()
	{
		Type = ETLLRN_CRSimConstraintType::Distance;
		SubjectA = SubjectB = INDEX_NONE;
		DataA = DataB = FVector::ZeroVector;
	}

	/**
	 * The type of the constraint
	 */
	UPROPERTY()
	ETLLRN_CRSimConstraintType Type;

	/**
	 * The first point affected by this constraint
	 */
	UPROPERTY()
	int32 SubjectA;

	/**
	 * The (optional) second point affected by this constraint
	 * This is currently only used for the distance constraint
	 */
	UPROPERTY()
	int32 SubjectB;

	/**
	 * The first data member for the constraint.
	 */
	UPROPERTY()
	FVector DataA;

	/**
	 * The second data member for the constraint.
	 */
	UPROPERTY()
	FVector DataB;

	void Apply(FRigVMSimPoint& OutPointA, FRigVMSimPoint& OutPointB) const;
};
