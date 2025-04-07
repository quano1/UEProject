// Copyright Epic Games, Inc. All Rights Reserved.
#include "RigEditor/TLLRN_IKRigAssetBrowserTabSummoner.h"

#include "IDocumentation.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "RigEditor/TLLRN_SIKRigAssetBrowser.h"

#define LOCTEXT_NAMESPACE "TLLRN_RetargetAssetBrowserTabSummoner"

const FName FTLLRN_IKRigAssetBrowserTabSummoner::TabID(TEXT("AssetBrowser"));

FTLLRN_IKRigAssetBrowserTabSummoner::FTLLRN_IKRigAssetBrowserTabSummoner(const TSharedRef<FTLLRN_IKRigEditorToolkit>& InRetargetEditor)
	: FWorkflowTabFactory(TabID, InRetargetEditor),
	TLLRN_IKRigEditor(InRetargetEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("IKRigAssetBrowserTabLabel", "Asset Browser");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.TabIcon");

	ViewMenuDescription = LOCTEXT("IKRigAssetBrowser_ViewMenu_Desc", "Asset Browser");
	ViewMenuTooltip = LOCTEXT("IKRigAssetBrowser_ViewMenu_ToolTip", "Show the Asset Browser Tab");
}

TSharedPtr<SToolTip> FTLLRN_IKRigAssetBrowserTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("IKRigAssetBrowserTooltip", "Select an animation asset to preview the IK results."), NULL, TEXT("Shared/Editors/Persona"), TEXT("IKRigAssetBrowser_Window"));
}

TSharedRef<SWidget> FTLLRN_IKRigAssetBrowserTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(TLLRN_SIKRigAssetBrowser, TLLRN_IKRigEditor.Pin()->GetController());
}

#undef LOCTEXT_NAMESPACE 
