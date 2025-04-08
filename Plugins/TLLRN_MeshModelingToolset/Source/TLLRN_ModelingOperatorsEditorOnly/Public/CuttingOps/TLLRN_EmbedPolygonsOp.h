// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Util/ProgressCancel.h"
#include "TLLRN_ModelingOperators.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Polygon2.h"

#include "TLLRN_EmbedPolygonsOp.generated.h"


UENUM()
enum class ETLLRN_EmbeddedPolygonOpMethod : uint8
{
	TrimOutside,
	TrimInside,
	InsertPolygon,
	CutThrough,
	CutOutside
};

namespace UE
{
namespace Geometry
{


class TLLRN_MODELINGOPERATORSEDITORONLY_API FTLLRN_EmbedPolygonsOp : public FDynamicMeshOperator
{
public:
	virtual ~FTLLRN_EmbedPolygonsOp() {}

	
	// inputs
	FFrame3d PolygonFrame;
	FPolygon2d EmbedPolygon;
	bool bCutWithBoolean;
	bool bAttemptFixHolesOnBoolean;

	// TODO: switch to FGeneralPolygon2d?
	FPolygon2d GetPolygon()
	{
		return EmbedPolygon;
	}

	bool bDiscardAttributes;

	ETLLRN_EmbeddedPolygonOpMethod Operation;

	//float ExtrudeDistance; // TODO if we support extrude

	TSharedPtr<FDynamicMesh3, ESPMode::ThreadSafe> OriginalMesh;

	//
	// FDynamicMeshOperator implementation
	// 

	virtual void CalculateResult(FProgressCancel* Progress) override;


	// Outputs
	TArray<int> EdgesOnFailure; // edges to highlight on failure, to help visualize what happened (partial cut edges or hole edges)
	TArray<int> EmbeddedEdges;
	bool bOperationSucceeded;

private:
	void RecordEmbeddedEdges(TArray<int>& PathVertIDs);

	void BooleanPath(FProgressCancel* Progress);

	// Fix issues like self-intersection on an input polygon
	// Note: We could change this to return an array of general polygons in the future, to handle the most general case,
	// but currently the embedding process does not support holes
	TArray<FPolygon2d> CleanPolygon(const FPolygon2d& Input);
};

} // end namespace UE::Geometry
} // end namespace UE

