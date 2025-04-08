// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_MeshTopologySelectionMechanic.h"
#include "TLLRN_PolygonSelectionMechanic.generated.h"

namespace UE::Geometry { struct FGeometrySelection; }

// DEPRECATED: Use UTLLRN_MeshTopologySelectionMechanicProperties
UCLASS(Deprecated)
class TLLRN_MODELINGCOMPONENTS_API UDEPRECATED_TLLRN_PolygonSelectionMechanicProperties : public UTLLRN_MeshTopologySelectionMechanicProperties
{
	GENERATED_BODY()

public:

	void Initialize(UTLLRN_PolygonSelectionMechanic* MechanicIn)
	{
		Mechanic = MechanicIn;
	}
};


/**
 * UTLLRN_PolygonSelectionMechanic implements the interaction for selecting a set of faces/vertices/edges
 * from a FGroupTopology.
 */
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_PolygonSelectionMechanic : public UTLLRN_MeshTopologySelectionMechanic
{
	GENERATED_BODY()

public:

	void Initialize(const FDynamicMesh3* Mesh,
		FTransform3d TargetTransform,
		UWorld* World,
		const FGroupTopology* Topology,
		TFunction<FDynamicMeshAABBTree3* ()> GetSpatialSourceFunc
	);

	void Initialize(UDynamicMeshComponent* MeshComponent, const FGroupTopology* Topology,
		TFunction<FDynamicMeshAABBTree3* ()> GetSpatialSourceFunc
	);

	/*
	 * Expands selection at the borders.
	 * 
	 * @param bAsTriangleTopology Can be set true if the topology type is FTriangleGroupTopology, to
	 *  perform the operation a bit more efficiently by using the mesh topology directly.
	 */
	void GrowSelection(bool bAsTriangleTopology);
	/*
	 * Shrinks selection at the borders.
	 * 
	 * @param bAsTriangleTopology Can be set true if the topology type is FTriangleGroupTopology, to
	 *  perform the operation a bit more efficiently by using the mesh topology directly.
	 */
	void ShrinkSelection(bool bAsTriangleTopology);

	/**
	 * Converts selection to a vertex/corner selection of just the boundary vertices/corners
	 * 
	 * @param bAsTriangleTopology Can be set true if the topology type is FTriangleGroupTopology, to
	 *  perform the operation a bit more efficiently by using the mesh topology directly.
	 */
	void ConvertSelectionToBorderVertices(bool bAsTriangleTopology);
	
	/**
	 * Expands selection to encompass connected components.
	 */
	void FloodSelection();

	// UTLLRN_MeshTopologySelectionMechanic
	virtual bool UpdateHighlight(const FRay& WorldRay) override;
	virtual bool UpdateSelection(const FRay& WorldRay, FVector3d& LocalHitPositionOut, FVector3d& LocalHitNormalOut) override;

	/**
	 * Convert the Active Selection to a PolyGroup-topology FGeometrySelection, with optional CompactMaps
	 */
	void GetSelection_AsGroupTopology(UE::Geometry::FGeometrySelection& SelectionOut, const FCompactMaps* CompactMapsToApply = nullptr) const;

	/**
	 * Convert the Active Selection to a Triangle-topology FGeometrySelection, with optional CompactMaps
	 */
	void GetSelection_AsTriangleTopology(UE::Geometry::FGeometrySelection& SelectionOut, const FCompactMaps* CompactMapsToApply = nullptr) const;


	/**
	 * Initialize the Active Selection based on the provided PolyGroup-topology FGeometrySelection
	 */
	void SetSelection_AsGroupTopology(const UE::Geometry::FGeometrySelection& Selection);

	/**
	 * Initialize the Active Selection based on the provided Triangle-topology FGeometrySelection
	 */
	void SetSelection_AsTriangleTopology(const UE::Geometry::FGeometrySelection& Selection);


private:

	// TODO: Would be nice to get rid of this and write everything in terms of TopologySelector and TopologyProvider
	const FGroupTopology* Topology;

	// Helper to execute selection actions through existing geometry selection code.
	bool ExecuteActionThroughGeometrySelection(bool bAsTriangleTopology, const FText& TransactionName,
		TFunctionRef<bool(UE::Geometry::FGeometrySelection& SelectionToModifyInPlace)> SelectionProcessor);
};
