// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigRetargetChainTabSummoner.h"

#include "IDocumentation.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "RigEditor/TLLRN_SIKRigSolverStack.h"

#define LOCTEXT_NAMESPACE "TLLRN_RetargetChainTabSummoner"

const FName FTLLRN_IKRigRetargetChainTabSummoner::TabID(TEXT("TLLRN_RetargetingChains"));

FTLLRN_IKRigRetargetChainTabSummoner::FTLLRN_IKRigRetargetChainTabSummoner(const TSharedRef<FTLLRN_IKRigEditorToolkit>& InTLLRN_IKRigEditor)
	: FWorkflowTabFactory(TabID, InTLLRN_IKRigEditor)
	, TLLRN_IKRigEditor(InTLLRN_IKRigEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("IKRigRetargetChainTabLabel", "IK Retargeting");
	TabIcon = FSlateIcon(FTLLRN_IKRigEditorStyle::Get().GetStyleSetName(), "TLLRN_IKRig.IKRetargeting");

	ViewMenuDescription = LOCTEXT("IKRigRetargetChain_ViewMenu_Desc", "IK Rig Retarget Chains");
	ViewMenuTooltip = LOCTEXT("IKRigRetargetChain_ViewMenu_ToolTip", "Show the IK Rig Retarget Chains Tab");
}

TSharedPtr<SToolTip> FTLLRN_IKRigRetargetChainTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("IKRigRetargetChainTooltip", "A list of labeled bone chains for retargeting animation between different skeletons. Used by the IK Retargeter asset."), NULL, TEXT("Shared/Editors/Persona"), TEXT("IKRigRetargetChain_Window"));
}

TSharedRef<SWidget> FTLLRN_IKRigRetargetChainTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(TLLRN_SIKRigRetargetChainList, TLLRN_IKRigEditor.Pin()->GetController());
}

#undef LOCTEXT_NAMESPACE 
