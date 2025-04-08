// Copyright Epic Games, Inc. All Rights Reserved.

#include "TargetInterfaces/TLLRN_DynamicMeshProvider.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "TargetInterfaces/MeshTargetInterfaceTypes.h"

#define LOCTEXT_NAMESPACE "TLLRN_DynamicMeshProvider"

using namespace UE::Geometry;

FDynamicMesh3 ITLLRN_DynamicMeshProvider::GetDynamicMesh(bool bRequestTangents)
{
	FGetMeshParameters GetMeshParams;
	GetMeshParams.bWantMeshTangents = bRequestTangents;
	return GetDynamicMesh(GetMeshParams);
}

FDynamicMesh3 ITLLRN_DynamicMeshProvider::GetDynamicMesh(const FGetMeshParameters& InGetMeshParams)
{
	return GetDynamicMesh();
}

#undef LOCTEXT_NAMESPACE 