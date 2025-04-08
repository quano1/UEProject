// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Util/ProgressCancel.h"
#include "TLLRN_ModelingOperators.h"


#include "TLLRN_BooleanMeshesOp.generated.h"


/** CSG operation types */
UENUM()
enum class ETLLRN_CSGOperation : uint8
{
	/** Subtract the second object from the first object */
	DifferenceAB = 0 UMETA(DisplayName = "Difference A - B"),

	/** Subtract the first object from the second object */
	DifferenceBA = 1 UMETA(DisplayName = "Difference B - A"),

	/** Intersection of the two objects, i.e. where both objects overlap */
	Intersect = 2 UMETA(DisplayName = "Intersection"),

	/** Union of the two objects, i.e. merger of both objects  */
	Union = 3 UMETA(DisplayName = "Union"),
};

UENUM()
enum class ETLLRN_TrimOperation : uint8
{
	/** Remove geometry from the first object using the second object */
	TrimA = 0 UMETA(DisplayName = "Trim A"),

	/** Remove geometry from the second object using the first object */
	TrimB = 1 UMETA(DisplayName = "Trim B"),
};


UENUM()
enum class ETLLRN_TrimSide : uint8
{
	RemoveInside = 0,
	RemoveOutside = 1,
};


namespace UE
{
namespace Geometry
{

class TLLRN_MODELINGOPERATORS_API FTLLRN_BooleanMeshesOp : public FDynamicMeshOperator
{
public:
	virtual ~FTLLRN_BooleanMeshesOp() override = default;

	// inputs
	ETLLRN_CSGOperation TLLRN_CSGOperation;
	ETLLRN_TrimOperation TLLRN_TrimOperation;
	ETLLRN_TrimSide TLLRN_TrimSide;
	bool bTrimMode = false; // if true, do a trim operation instead of a boolean
	TArray<TSharedPtr<const FDynamicMesh3, ESPMode::ThreadSafe>> Meshes;
	TArray<FTransformSRT3d> Transforms; // 1:1 with Meshes
	bool bAttemptFixHoles = false;
	double WindingThreshold = 0.5;

	/** If true, try to do edge-collapses along cut edges to remove unnecessary edges inserted by cut */
	bool bTryCollapseExtraEdges = false;
	/** Angle threshold in degrees used for testing if two triangles should be considered coplanar, or two lines collinear */
	float TryCollapseExtraEdgesPlanarThresh = 0.01f;

	void SetTransform(const FTransformSRT3d& Transform);

	//
	// FDynamicMeshOperator implementation
	// 

	virtual void CalculateResult(FProgressCancel* Progress) override;

	// IDs of any newly-created boundary edges in the result mesh
	const TArray<int>& GetCreatedBoundaryEdges() const
	{
		return CreatedBoundaryEdges;
	}

private:
	TArray<int> CreatedBoundaryEdges;
};


} // end namespace UE::Geometry
} // end namespace UE
