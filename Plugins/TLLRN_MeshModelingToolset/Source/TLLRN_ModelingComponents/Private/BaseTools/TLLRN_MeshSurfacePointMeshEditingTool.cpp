// Copyright Epic Games, Inc. All Rights Reserved.

#include "BaseTools/TLLRN_MeshSurfacePointMeshEditingTool.h"
#include "InteractiveToolManager.h"
#include "ToolBuilderUtil.h"

#include "TargetInterfaces/PrimitiveComponentBackedTarget.h"
#include "TargetInterfaces/TLLRN_DynamicMeshProvider.h"
#include "TargetInterfaces/TLLRN_DynamicMeshCommitter.h"
#include "TargetInterfaces/MaterialProvider.h"
#include "ToolTargetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_MeshSurfacePointMeshEditingTool)


/*
 * ToolBuilder
 */

const FToolTargetTypeRequirements& UTLLRN_MeshSurfacePointMeshEditingToolBuilder::GetTargetRequirements() const
{
	static FToolTargetTypeRequirements TypeRequirements({
		UMaterialProvider::StaticClass(),
		UTLLRN_DynamicMeshProvider::StaticClass(),
		UTLLRN_DynamicMeshCommitter::StaticClass(),
		UPrimitiveComponentBackedTarget::StaticClass()
		});
	return TypeRequirements;
}

UMeshSurfacePointTool* UTLLRN_MeshSurfacePointMeshEditingToolBuilder::CreateNewTool(const FToolBuilderState& SceneState) const
{
	return NewObject<UMeshSurfacePointTool>(SceneState.ToolManager);
}



