// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "InteractiveTool.h"

#include "TLLRN_UVEditorMechanicAdapterTool.generated.h"

class UInteractiveToolManager;

/**
 * This is a dummy tool that only exists to make it possible for mechanics to be used in
 * outside of an actual tool. The tool can be passed in to the mechanics' Setup() calls. 
 * If necessary, the behaviors can be extracted via GetInputBehaviors() to be placed into 
 * an input router.
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorMechanicAdapterTool : public UInteractiveTool
{
	GENERATED_BODY()

public:
	UInteractiveToolManager* ToolManager = nullptr;

	virtual UInteractiveToolManager* GetToolManager() const override { return ToolManager; }
};