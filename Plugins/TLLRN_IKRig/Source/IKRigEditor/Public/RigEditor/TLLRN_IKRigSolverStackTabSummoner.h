// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FTLLRN_IKRigEditorToolkit;

struct FTLLRN_IKRigSolverStackTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabID;
	
	FTLLRN_IKRigSolverStackTabSummoner(const TSharedRef<FTLLRN_IKRigEditorToolkit>& InTLLRN_IKRigEditor);
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FTLLRN_IKRigEditorToolkit> TLLRN_IKRigEditor;
};
