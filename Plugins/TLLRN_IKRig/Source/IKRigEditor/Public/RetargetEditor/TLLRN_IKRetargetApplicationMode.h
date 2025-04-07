// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

class FTLLRN_IKRetargetEditor;

class FTLLRN_IKRetargetApplicationMode : public FApplicationMode
{
	
public:
	
	FTLLRN_IKRetargetApplicationMode(TSharedRef<class FWorkflowCentricApplication> InHostingApp, TSharedRef<class IPersonaPreviewScene> InPreviewScene);

	/** FApplicationMode interface */
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;
	/** END FApplicationMode interface */

protected:
	
	/** The hosting app */
	TWeakPtr<FTLLRN_IKRetargetEditor> TLLRN_IKRetargetEditorPtr;

	/** The tab factories we support */
	FWorkflowAllowedTabSet TabFactories;
};
