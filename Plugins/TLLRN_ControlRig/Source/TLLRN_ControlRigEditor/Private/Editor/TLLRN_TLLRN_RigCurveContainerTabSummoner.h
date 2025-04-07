// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FTLLRN_ControlRigEditor;

struct FTLLRN_TLLRN_RigCurveContainerTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabID;
	
public:
	FTLLRN_TLLRN_RigCurveContainerTabSummoner(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor);
	
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	
protected:
	TWeakPtr<FTLLRN_ControlRigEditor> TLLRN_ControlRigEditor;
};
