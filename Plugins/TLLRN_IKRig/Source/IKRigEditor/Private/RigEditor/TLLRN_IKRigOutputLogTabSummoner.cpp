// Copyright Epic Games, Inc. All Rights Reserved.
#include "RigEditor/TLLRN_IKRigOutputLogTabSummoner.h"

#include "IDocumentation.h"
#include "Rig/TLLRN_IKRigProcessor.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "RigEditor/TLLRN_SIKRigOutputLog.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRigOutputLogTabSummoner"

const FName FTLLRN_IKRigOutputLogTabSummoner::TabID(TEXT("TLLRN_IKRigOutputLog"));

FTLLRN_IKRigOutputLogTabSummoner::FTLLRN_IKRigOutputLogTabSummoner(
	const TSharedRef<FTLLRN_IKRigEditorToolkit>& InRigEditor)
	: FWorkflowTabFactory(TabID, InRigEditor),
	TLLRN_IKRigEditor(InRigEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("IKRigOutputLogTabLabel", "IK Rig Output Log");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.CompilerResults");

	ViewMenuDescription = LOCTEXT("IKRigOutputLog_ViewMenu_Desc", "IK Rig Output Log");
	ViewMenuTooltip = LOCTEXT("IKRigOutputLog_ViewMenu_ToolTip", "Show the IK Rig Output Log Tab");
}

TSharedPtr<SToolTip> FTLLRN_IKRigOutputLogTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("IKRigOutputLogTooltip", "View warnings and errors from this rig."), NULL, TEXT("Shared/Editors/Persona"), TEXT("IKRigOutputLog_Window"));
}

TSharedRef<SWidget> FTLLRN_IKRigOutputLogTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	const TSharedRef<FTLLRN_IKRigEditorController>& Controller = TLLRN_IKRigEditor.Pin()->GetController();
	const FName LogName = Controller->GetTLLRN_IKRigProcessor()->Log.GetLogTarget();
	TSharedRef<TLLRN_SIKRigOutputLog> LogView = SNew(TLLRN_SIKRigOutputLog, LogName);
	Controller->SetOutputLogView(LogView);
	return LogView;
}

#undef LOCTEXT_NAMESPACE 
