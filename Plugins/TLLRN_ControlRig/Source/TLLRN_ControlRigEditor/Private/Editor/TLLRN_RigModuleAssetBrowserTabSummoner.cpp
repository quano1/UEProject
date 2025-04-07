// Copyright Epic Games, Inc. All Rights Reserved.
#include "TLLRN_RigModuleAssetBrowserTabSummoner.h"

#include "IDocumentation.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "TLLRN_ControlRigEditor.h"
#include "STLLRN_RigModuleAssetBrowser.h"

#define LOCTEXT_NAMESPACE "TLLRN_RigModuleAssetBrowserTabSummoner"

const FName FTLLRN_RigModuleAssetBrowserTabSummoner::TabID(TEXT("TLLRN_RigModuleAssetBrowser"));

FTLLRN_RigModuleAssetBrowserTabSummoner::FTLLRN_RigModuleAssetBrowserTabSummoner(const TSharedRef<FTLLRN_ControlRigEditor>& InEditor)
	: FWorkflowTabFactory(TabID, InEditor),
	TLLRN_ControlRigEditor(InEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("TLLRN_RigModuleAssetBrowserTabLabel", "Module Assets");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.TabIcon");

	ViewMenuDescription = LOCTEXT("TLLRN_RigModuleAssetBrowser_ViewMenu_Desc", "Module Assets");
	ViewMenuTooltip = LOCTEXT("TLLRN_RigModuleAssetBrowser_ViewMenu_ToolTip", "Show the Module Assets Tab");
}

TSharedPtr<SToolTip> FTLLRN_RigModuleAssetBrowserTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("TLLRN_RigModuleAssetBrowserTooltip", "Drag and drop the rig modules into the Modular Rig Hierarchy tab."), NULL, TEXT("Shared/Editors/Persona"), TEXT("TLLRN_RigModuleAssetBrowser_Window"));
}

TSharedRef<SWidget> FTLLRN_RigModuleAssetBrowserTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(STLLRN_RigModuleAssetBrowser, TLLRN_ControlRigEditor.Pin().ToSharedRef());
}

TSharedRef<SDockTab> FTLLRN_RigModuleAssetBrowserTabSummoner::SpawnTab(const FWorkflowTabSpawnInfo& Info) const
{
	return FWorkflowTabFactory::SpawnTab(Info);
}

#undef LOCTEXT_NAMESPACE 
