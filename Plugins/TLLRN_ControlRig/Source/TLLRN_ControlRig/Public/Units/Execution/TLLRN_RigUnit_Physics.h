// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_RigUnit_DynamicHierarchy.h"
#include "TLLRN_RigUnit_Physics.generated.h"

/**
 * Adds a new physics solver to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Physics Solver", Keywords="Construction,Create,New,Simulation", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddPhysicsSolver : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddPhysicsSolver()
	{
		Name = TEXT("Solver");
		Solver = FTLLRN_RigPhysicsSolverID();
	}

	/*
	 * The name of the new solver to add
	 */
	UPROPERTY(meta = (Input))
	FName Name;

	/*
	 * The identifier of the solver
	 */
	UPROPERTY(meta = (Output))
	FTLLRN_RigPhysicsSolverID Solver;

	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Adds a new physics joint to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Physics Joint", Keywords="Construction,Create,New,Joint,Skeleton", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddPhysicsJoint : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddPhysicsJoint()
	{
		Name = TEXT("NewPhysicsJoint");
		Parent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		Transform = FTransform::Identity;
		Solver = FTLLRN_RigPhysicsSolverID();
	}

	virtual ETLLRN_RigElementType GetElementTypeToSpawn() const override { return ETLLRN_RigElementType::Physics; }


	RIGVM_METHOD()
	virtual void Execute() override;

	/*
	 * The initial global transform of the spawned element
	 */
	UPROPERTY(meta = (Input))
	FTransform Transform;

	/*
	 * The solver to relate this new physics element to
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigPhysicsSolverID Solver;

	/*
	 * The settings of the new physics element
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigPhysicsSettings Settings;
};

