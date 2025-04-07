// Copyright Epic Games, Inc. All Rights Reserved.
#include "RetargetEditor/TLLRN_IKRetargetOutputLogTabSummoner.h"

#include "IDocumentation.h"
#include "RetargetEditor/TLLRN_IKRetargetEditor.h"
#include "RigEditor/TLLRN_SIKRigOutputLog.h"
#include "Retargeter/TLLRN_IKRetargetProcessor.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRetargetOutputLogTabSummoner"

const FName FTLLRN_IKRetargetOutputLogTabSummoner::TabID(TEXT("TLLRN_RetargetOutputLog"));

FTLLRN_IKRetargetOutputLogTabSummoner::FTLLRN_IKRetargetOutputLogTabSummoner(
	const TSharedRef<FTLLRN_IKRetargetEditor>& InRetargetEditor)
	: FWorkflowTabFactory(TabID, InRetargetEditor),
	TLLRN_IKRetargetEditor(InRetargetEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("IKRetargetOutputLogTabLabel", "Retarget Output Log");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.CompilerResults");

	ViewMenuDescription = LOCTEXT("IKRetargetOutputLog_ViewMenu_Desc", "Retarget Output Log");
	ViewMenuTooltip = LOCTEXT("IKRetargetOutputLog_ViewMenu_ToolTip", "Show the Retargeting Output Log Tab");
}

TSharedPtr<SToolTip> FTLLRN_IKRetargetOutputLogTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("IKRetargetOutputLogTooltip", "View warnings and errors while retargeting."), NULL, TEXT("Shared/Editors/Persona"), TEXT("IKRetargetOutputLog_Window"));
}

TSharedRef<SWidget> FTLLRN_IKRetargetOutputLogTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	const TSharedRef<FTLLRN_IKRetargetEditorController>& Controller = TLLRN_IKRetargetEditor.Pin()->GetController();
	const FName LogName = Controller->GetRetargetProcessor()->Log.GetLogTarget();
	TSharedRef<TLLRN_SIKRigOutputLog> LogView = SNew(TLLRN_SIKRigOutputLog, LogName);
	Controller->SetOutputLogView(LogView);
	return LogView;
}

#undef LOCTEXT_NAMESPACE 
