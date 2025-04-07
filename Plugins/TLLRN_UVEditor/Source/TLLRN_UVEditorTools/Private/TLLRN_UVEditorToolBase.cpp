// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorToolBase.h"

#include "InteractiveTool.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h" // FToolBuilderState

void UGenericTLLRN_UVEditorToolBuilder::Initialize(TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn, TSubclassOf<UInteractiveTool> ToolClassIn)
{
	Targets = &TargetsIn;
	if (ensure(ToolClassIn && ToolClassIn->ImplementsInterface(UTLLRN_UVEditorGenericBuildableTool::StaticClass())))
	{
		ToolClass = ToolClassIn;
	}
}

bool UGenericTLLRN_UVEditorToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return ToolClass && Targets && Targets->Num() > 0;
}

UInteractiveTool* UGenericTLLRN_UVEditorToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UInteractiveTool* NewTool = NewObject<UInteractiveTool>(SceneState.ToolManager, ToolClass.Get());
	ITLLRN_UVEditorGenericBuildableTool* CastTool = Cast<ITLLRN_UVEditorGenericBuildableTool>(NewTool);
	if (ensure(CastTool))
	{
		CastTool->SetTargets(*Targets);
	}
	return NewTool;
}