// Copyright Epic Games, Inc. All Rights Reserved.
#include "RetargetEditor/TLLRN_IKRetargetAssetBrowserTabSummoner.h"

#include "IDocumentation.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"
#include "RetargetEditor/TLLRN_IKRetargetEditor.h"
#include "RetargetEditor/TLLRN_SIKRetargetAssetBrowser.h"

#define LOCTEXT_NAMESPACE "TLLRN_RetargetAssetBrowserTabSummoner"

const FName FTLLRN_IKRetargetAssetBrowserTabSummoner::TabID(TEXT("AssetBrowser"));

FTLLRN_IKRetargetAssetBrowserTabSummoner::FTLLRN_IKRetargetAssetBrowserTabSummoner(const TSharedRef<FTLLRN_IKRetargetEditor>& InRetargetEditor)
	: FWorkflowTabFactory(TabID, InRetargetEditor),
	TLLRN_IKRetargetEditor(InRetargetEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("IKRetargetAssetBrowserTabLabel", "Asset Browser");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.TabIcon");

	ViewMenuDescription = LOCTEXT("IKRetargetAssetBrowser_ViewMenu_Desc", "Asset Browser");
	ViewMenuTooltip = LOCTEXT("IKRetargetAssetBrowser_ViewMenu_ToolTip", "Show the Asset Browser Tab");
}

TSharedPtr<SToolTip> FTLLRN_IKRetargetAssetBrowserTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("IKRetargetAssetBrowserTooltip", "Select an animation asset to preview the retarget results."), NULL, TEXT("Shared/Editors/Persona"), TEXT("IKRetargetAssetBrowser_Window"));
}

TSharedRef<SWidget> FTLLRN_IKRetargetAssetBrowserTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(TLLRN_SIKRetargetAssetBrowser, TLLRN_IKRetargetEditor.Pin()->GetController());
}

#undef LOCTEXT_NAMESPACE 
