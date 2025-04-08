// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoxTypes.h"
#include "HAL/Platform.h" // Required by TLLRN_MODELINGCOMPONENTS_API

class UTLLRN_PreviewGeometry;
class UTLLRN_GeometrySelectionVisualizationProperties;

namespace UE::Geometry
{
	struct FGeometrySelection;
	class FDynamicMesh3;
	class FGroupTopology;

	/**
	 * Initialize the geometry selection preview geometry. Call this in your tool Setup function
	 */
	void TLLRN_MODELINGCOMPONENTS_API InitializeGeometrySelectionVisualization(
		UTLLRN_PreviewGeometry* PreviewGeom,
		UTLLRN_GeometrySelectionVisualizationProperties* Settings,
		const FDynamicMesh3& SourceMesh,
		const FGeometrySelection& Selection,
		const FGroupTopology* GroupTopology = nullptr,
		const FGeometrySelection* TriangleVertexSelection = nullptr,
		const TArray<int>* TriangleROI = nullptr,
		const FTransform* ApplyTransform = nullptr);

	/**
	 * Update the geometry selection preview geometry according to Settings. Call this in your tool OnTick function
	 */
	void TLLRN_MODELINGCOMPONENTS_API UpdateGeometrySelectionVisualization(
		UTLLRN_PreviewGeometry* PreviewGeom,
		UTLLRN_GeometrySelectionVisualizationProperties* Settings);

	/**
	 * Computes the axis aligned bounding box from a given triangle region of interest, useful to compute a DepthBias
	 */
	FAxisAlignedBox3d TLLRN_MODELINGCOMPONENTS_API ComputeBoundsFromTriangleROI(
		const FDynamicMesh3& Mesh,
		const TArray<int32>& TriangleROI,
		const FTransform3d* Transform = nullptr);

	/**
	 * Computes the axis aligned bounding box from a given vertex region of interest, useful to compute a DepthBias
	 */
	FAxisAlignedBox3d TLLRN_MODELINGCOMPONENTS_API ComputeBoundsFromVertexROI(
		const FDynamicMesh3& Mesh,
		const TArray<int32>& VertexROI,
		const FTransform3d* Transform = nullptr);

} // namespace UE::Geometry
