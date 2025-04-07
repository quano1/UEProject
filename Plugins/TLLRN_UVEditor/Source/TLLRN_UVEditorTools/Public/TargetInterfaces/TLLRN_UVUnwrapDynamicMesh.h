// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GeometryBase.h"
#include "UObject/Interface.h"

#include "TLLRN_UVUnwrapDynamicMesh.generated.h"

PREDECLARE_USE_GEOMETRY_CLASS(FDynamicMesh3);

UINTERFACE()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVUnwrapDynamicMesh : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface that is able to use the UV's of a target to generate a flat FDynamicMesh
 * representing a given layer, and to bake a modified such mesh back to the UV's of
 * the target provided that the triangle id's have all stayed the same.
 */
class TLLRN_UVEDITORTOOLS_API ITLLRN_UVUnwrapDynamicMesh
{
	GENERATED_BODY()

public:
	virtual int32 GetNumUVLayers() = 0;
	virtual TSharedPtr<FDynamicMesh3> GetMesh(int32 LayerIndex) = 0;
	virtual void SaveBackToUVs(const FDynamicMesh3* MeshToSave, int32 LayerIndex) = 0;
};