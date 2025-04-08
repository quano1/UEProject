// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Util/ProgressCancel.h"
#include "TLLRN_ModelingOperators.h"
#include "OrientedBoxTypes.h"

#include "TLLRN_UVProjectionOp.generated.h"

UENUM()
enum class ETLLRN_UVProjectionMethod : uint8
{
	/** Calculate UVs by assigning triangles to faces of a box, and then apply per-box-face planar projection */
	Box,
	/** Calculate UVs by assigning triangles to a cylinder using radial projection or cylinder end caps using planar projection */
	Cylinder,
	/** Calculate UVs by projecting to a plane */
	Plane,
	/** Calculate UVs by Exponential Map projection centered at the surface point nearest to the center of the input plane */
	ExpMap UMETA(DisplayName = "ExpMap")
};

namespace UE
{
namespace Geometry
{

class TLLRN_MODELINGOPERATORS_API FTLLRN_UVProjectionOp : public FDynamicMeshOperator
{
public:
	virtual ~FTLLRN_UVProjectionOp() {}

	// inputs
	TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe> OriginalMesh;
	TSharedPtr<TArray<int32>, ESPMode::ThreadSafe> TriangleROI;
	int32 UseUVLayer = 0;

	// transform from Mesh into space of Projection Geometry
	FTransformSRT3d MeshToProjectionSpace;

	ETLLRN_UVProjectionMethod ProjectionMethod = ETLLRN_UVProjectionMethod::Box;
	FOrientedBox3d ProjectionBox;
	int32 MinRegionSize = 2;
	float CylinderSplitAngle = 45.0f;

	// ExpMap parameters for modifying normals
	double BlendWeight = 0.0;
	int32 SmoothingRounds = 0;
	double SmoothingAlpha = 0.5;

	// position to use as UV origin
	FVector2f UVOrigin = FVector2f(0.5f,0.5f);
	// rotation applied to computed UVs
	float UVRotationAngleDeg = 0.0;
	// scale applied to computed UVs
	FVector2f UVScale = FVector2f::One();
	// translation applied to computed UVs
	FVector2f UVTranslate = FVector2f::Zero();


	//
	// FDynamicMeshOperator implementation
	// 
	virtual void CalculateResult(FProgressCancel* Progress) override;


	virtual void CalculateResult_Box(FProgressCancel* Progress);
	virtual void CalculateResult_Plane(FProgressCancel* Progress);
	virtual void CalculateResult_ExpMap(FProgressCancel* Progress);
	virtual void CalculateResult_Cylinder(FProgressCancel* Progress);
};

} // end namespace UE::Geometry
} // end namespace UE

