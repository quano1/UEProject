// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FTLLRN_ControlRigEditor;

struct FTLLRN_ModularRigModelTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabID;
	
public:
	FTLLRN_ModularRigModelTabSummoner(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor);
	
	virtual FTabSpawnerEntry& RegisterTabSpawner(TSharedRef<FTabManager> TabManager, const FApplicationMode* CurrentApplicationMode) const;
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedRef<SDockTab> SpawnTab(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FTLLRN_ControlRigEditor> TLLRN_ControlRigEditor;
};
