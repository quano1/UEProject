// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "BlueprintEditorModes.h"

class UTLLRN_ControlRigBlueprint;

class FTLLRN_ControlRigEditorMode : public FBlueprintEditorApplicationMode
{
public:
	FTLLRN_ControlRigEditorMode(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor, bool bCreateDefaultLayout = true);

	// FApplicationMode interface
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;

protected:
	// Set of spawnable tabs
	FWorkflowAllowedTabSet TabFactories;

private:
	TWeakObjectPtr<UTLLRN_ControlRigBlueprint> TLLRN_ControlRigBlueprintPtr;
};

class FTLLRN_ModularRigEditorMode : public FTLLRN_ControlRigEditorMode
{
public:
	FTLLRN_ModularRigEditorMode(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor);

	// FApplicationMode interface
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;

	// for now just don't open up the previous edited documents
	virtual void PostActivateMode() override {}
};

