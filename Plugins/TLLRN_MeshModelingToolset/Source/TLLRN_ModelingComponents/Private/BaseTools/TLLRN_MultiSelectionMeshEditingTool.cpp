// Copyright Epic Games, Inc. All Rights Reserved.

#include "BaseTools/TLLRN_MultiSelectionMeshEditingTool.h"

#include "Engine/World.h"

#include "TargetInterfaces/MaterialProvider.h"
#include "TargetInterfaces/TLLRN_DynamicMeshCommitter.h"
#include "TargetInterfaces/TLLRN_DynamicMeshProvider.h"
#include "TargetInterfaces/PrimitiveComponentBackedTarget.h"
#include "TargetInterfaces/AssetBackedTarget.h"
#include "ToolTargetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_MultiSelectionMeshEditingTool)

/*
 * ToolBuilder
 */
const FToolTargetTypeRequirements& UTLLRN_MultiSelectionMeshEditingToolBuilder::GetTargetRequirements() const
{
	static FToolTargetTypeRequirements TypeRequirements({
		UMaterialProvider::StaticClass(),
		UTLLRN_DynamicMeshCommitter::StaticClass(),
		UTLLRN_DynamicMeshProvider::StaticClass(),
		UPrimitiveComponentBackedTarget::StaticClass()
		});
	return TypeRequirements;
}

bool UTLLRN_MultiSelectionMeshEditingToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return SceneState.TargetManager->CountSelectedAndTargetable(SceneState, GetTargetRequirements()) > 0;
}

UInteractiveTool* UTLLRN_MultiSelectionMeshEditingToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_MultiSelectionMeshEditingTool* NewTool = CreateNewTool(SceneState);
	InitializeNewTool(NewTool, SceneState);
	return NewTool;
}

void UTLLRN_MultiSelectionMeshEditingToolBuilder::InitializeNewTool(UTLLRN_MultiSelectionMeshEditingTool* NewTool, const FToolBuilderState& SceneState) const
{
	const TArray<TObjectPtr<UToolTarget>> Targets = SceneState.TargetManager->BuildAllSelectedTargetable(SceneState, GetTargetRequirements());
	NewTool->SetTargets(Targets);
	NewTool->SetWorld(SceneState.World);
}


/**
 * Tool
 */

void UTLLRN_MultiSelectionMeshEditingTool::Shutdown(EToolShutdownType ShutdownType)
{
	OnShutdown(ShutdownType);
	TargetWorld = nullptr;
}

void UTLLRN_MultiSelectionMeshEditingTool::OnShutdown(EToolShutdownType ShutdownType)
{
}


void UTLLRN_MultiSelectionMeshEditingTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

UWorld* UTLLRN_MultiSelectionMeshEditingTool::GetTargetWorld()
{
	return TargetWorld.Get();
}


bool UTLLRN_MultiSelectionMeshEditingTool::GetMapToSharedSourceData(TArray<int32>& MapToFirstOccurrences)
{
	bool bSharesSources = false;
	MapToFirstOccurrences.SetNumUninitialized(Targets.Num());
	for (int32 ComponentIdx = 0; ComponentIdx < Targets.Num(); ComponentIdx++)
	{
		MapToFirstOccurrences[ComponentIdx] = -1;
	}
	for (int32 ComponentIdx = 0; ComponentIdx < Targets.Num(); ComponentIdx++)
	{
		if (MapToFirstOccurrences[ComponentIdx] >= 0) // already mapped
		{
			continue;
		}

		MapToFirstOccurrences[ComponentIdx] = ComponentIdx;

		IAssetBackedTarget* Target = Cast<IAssetBackedTarget>(Targets[ComponentIdx]);
		if (!Target)
		{
			continue;
		}
		for (int32 VsIdx = ComponentIdx + 1; VsIdx < Targets.Num(); VsIdx++)
		{
			IAssetBackedTarget* OtherTarget = Cast<IAssetBackedTarget>(Targets[VsIdx]);
			if (OtherTarget && OtherTarget->GetSourceData() == Target->GetSourceData())
			{
				bSharesSources = true;
				MapToFirstOccurrences[VsIdx] = ComponentIdx;
			}
		}
	}
	return bSharesSources;
}




