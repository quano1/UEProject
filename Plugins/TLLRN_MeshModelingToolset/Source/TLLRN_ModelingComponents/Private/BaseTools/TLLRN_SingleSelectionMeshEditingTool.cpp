// Copyright Epic Games, Inc. All Rights Reserved.

#include "BaseTools/TLLRN_SingleSelectionMeshEditingTool.h"

#include "Engine/World.h"

#include "TargetInterfaces/MaterialProvider.h"
#include "TargetInterfaces/TLLRN_DynamicMeshCommitter.h"
#include "TargetInterfaces/TLLRN_DynamicMeshProvider.h"
#include "TargetInterfaces/PrimitiveComponentBackedTarget.h"
#include "ToolTargetManager.h"
#include "Selection/StoredMeshSelectionUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_SingleSelectionMeshEditingTool)

/*
 * ToolBuilder
 */
const FToolTargetTypeRequirements& UTLLRN_SingleSelectionMeshEditingToolBuilder::GetTargetRequirements() const
{
	static FToolTargetTypeRequirements TypeRequirements({
		UMaterialProvider::StaticClass(),
		UTLLRN_DynamicMeshCommitter::StaticClass(),
		UTLLRN_DynamicMeshProvider::StaticClass(),
		UPrimitiveComponentBackedTarget::StaticClass()
		});
	return TypeRequirements;
}

bool UTLLRN_SingleSelectionMeshEditingToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return SceneState.TargetManager->CountSelectedAndTargetable(SceneState, GetTargetRequirements()) == 1;
}

UInteractiveTool* UTLLRN_SingleSelectionMeshEditingToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_SingleSelectionMeshEditingTool* NewTool = CreateNewTool(SceneState);
	InitializeNewTool(NewTool, SceneState);
	return NewTool;
}

void UTLLRN_SingleSelectionMeshEditingToolBuilder::InitializeNewTool(UTLLRN_SingleSelectionMeshEditingTool* NewTool, const FToolBuilderState& SceneState) const
{
	UToolTarget* Target = SceneState.TargetManager->BuildFirstSelectedTargetable(SceneState, GetTargetRequirements());
	check(Target);
	NewTool->SetTarget(Target);
	NewTool->SetWorld(SceneState.World);
}


void UTLLRN_SingleSelectionMeshEditingTool::Shutdown(EToolShutdownType ShutdownType)
{
	OnShutdown(ShutdownType);
	TargetWorld = nullptr;
}

void UTLLRN_SingleSelectionMeshEditingTool::OnShutdown(EToolShutdownType ShutdownType)
{
}


void UTLLRN_SingleSelectionMeshEditingTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

UWorld* UTLLRN_SingleSelectionMeshEditingTool::GetTargetWorld()
{
	return TargetWorld.Get();
}





