// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_PolygonSelectionMechanic.h"
#include "TLLRN_BoundarySelectionMechanic.generated.h"

namespace UE::Geometry { class FMeshBoundaryLoops; }
class FBoundarySelector;

UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_BoundarySelectionMechanic : public UTLLRN_MeshTopologySelectionMechanic
{
	GENERATED_BODY()

public:

	void Initialize(
		const FDynamicMesh3* MeshIn,
		FTransform3d TargetTransformIn,
		UWorld* WorldIn,
		const UE::Geometry::FMeshBoundaryLoops* BoundaryLoopsIn,
		TFunction<FDynamicMeshAABBTree3* ()> GetSpatialSourceFuncIn);

	virtual bool UpdateHighlight(const FRay& WorldRay) override;

	virtual bool UpdateSelection(const FRay& WorldRay, FVector3d& LocalHitPositionOut, FVector3d& LocalHitNormalOut) override;

};
