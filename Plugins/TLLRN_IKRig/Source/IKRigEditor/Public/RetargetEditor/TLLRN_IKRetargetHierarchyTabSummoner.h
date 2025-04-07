// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FTLLRN_IKRetargetEditor;

struct FTLLRN_IKRetargetHierarchyTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabID;
	
	FTLLRN_IKRetargetHierarchyTabSummoner(const TSharedRef<FTLLRN_IKRetargetEditor>& InTLLRN_IKRigEditor);
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FTLLRN_IKRetargetEditor> RetargetEditor;
};
