// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FTLLRN_IKRetargetEditor;

struct FTLLRN_IKRetargetChainTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabID;
	
	FTLLRN_IKRetargetChainTabSummoner(const TSharedRef<FTLLRN_IKRetargetEditor>& InTLLRN_IKRetargetEditor);
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FTLLRN_IKRetargetEditor> TLLRN_IKRetargetEditor;
};
