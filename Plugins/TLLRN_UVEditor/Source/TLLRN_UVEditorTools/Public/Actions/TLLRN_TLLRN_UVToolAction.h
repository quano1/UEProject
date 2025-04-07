// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "TLLRN_TLLRN_UVToolAction.generated.h"

class UTLLRN_UVToolEmitChangeAPI;
class UTLLRN_UVToolSelectionAPI;
class UInteractiveToolManager;

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_TLLRN_UVToolAction : public UObject
{
	GENERATED_BODY()
public:

	virtual void Setup(UInteractiveToolManager* ToolManager);
	virtual void Shutdown();
	virtual bool CanExecuteAction() const { return false;}
	virtual bool ExecuteAction() { return false; }

protected:
	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolSelectionAPI> SelectionAPI = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolEmitChangeAPI> EmitChangeAPI = nullptr;

	TWeakObjectPtr<UInteractiveToolManager> ToolManager;

	virtual UInteractiveToolManager* GetToolManager() const { return ToolManager.Get(); }
};
