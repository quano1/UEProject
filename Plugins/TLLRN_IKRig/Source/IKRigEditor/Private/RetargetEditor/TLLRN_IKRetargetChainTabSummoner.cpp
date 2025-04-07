// Copyright Epic Games, Inc. All Rights Reserved.
#include "RetargetEditor/TLLRN_IKRetargetChainTabSummoner.h"

#include "IDocumentation.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"
#include "RetargetEditor/TLLRN_IKRetargetEditor.h"
#include "RetargetEditor/TLLRN_IKRetargetEditorStyle.h"
#include "RetargetEditor/TLLRN_SIKRetargetChainMapList.h"

#define LOCTEXT_NAMESPACE "TLLRN_RetargetChainTabSummoner"

const FName FTLLRN_IKRetargetChainTabSummoner::TabID(TEXT("TLLRN_ChainMapping"));

FTLLRN_IKRetargetChainTabSummoner::FTLLRN_IKRetargetChainTabSummoner(const TSharedRef<FTLLRN_IKRetargetEditor>& InRetargetEditor)
	: FWorkflowTabFactory(TabID, InRetargetEditor),
	TLLRN_IKRetargetEditor(InRetargetEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("IKRetargetChainTabLabel", "Chain Mapping");
	TabIcon = FSlateIcon(FTLLRN_IKRetargetEditorStyle::Get().GetStyleSetName(), "TLLRN_IKRetarget.TLLRN_ChainMapping");

	ViewMenuDescription = LOCTEXT("IKRetargetChain_ViewMenu_Desc", "Chain Mapping");
	ViewMenuTooltip = LOCTEXT("IKRetargetChain_ViewMenu_ToolTip", "Show the Chain Mapping Tab");
}

TSharedPtr<SToolTip> FTLLRN_IKRetargetChainTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("IKRetargetChainTooltip", "Map the bone chains between the source and target IK rigs."), NULL, TEXT("Shared/Editors/Persona"), TEXT("IKRetargetChain_Window"));
}

TSharedRef<SWidget> FTLLRN_IKRetargetChainTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(TLLRN_SIKRetargetChainMapList, TLLRN_IKRetargetEditor.Pin()->GetController());
}

#undef LOCTEXT_NAMESPACE 
