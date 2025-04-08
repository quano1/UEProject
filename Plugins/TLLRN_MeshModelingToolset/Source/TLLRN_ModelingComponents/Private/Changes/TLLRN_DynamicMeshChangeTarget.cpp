// Copyright Epic Games, Inc. All Rights Reserved.

#include "Changes/TLLRN_DynamicMeshChangeTarget.h"
#include "DynamicMesh/DynamicMesh3.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_DynamicMeshChangeTarget)

using namespace UE::Geometry;

void UTLLRN_DynamicMeshReplacementChangeTarget::ApplyChange(const FMeshReplacementChange* Change, bool bRevert)
{
	Mesh = Change->GetMesh(bRevert);
	OnMeshChanged.Broadcast();
}

TUniquePtr<FMeshReplacementChange> UTLLRN_DynamicMeshReplacementChangeTarget::ReplaceMesh(const TSharedPtr<const FDynamicMesh3, ESPMode::ThreadSafe>& UpdateMesh)
{
	TUniquePtr<FMeshReplacementChange> Change = MakeUnique<FMeshReplacementChange>(Mesh, UpdateMesh);
	Mesh = UpdateMesh;
	return Change;
}
