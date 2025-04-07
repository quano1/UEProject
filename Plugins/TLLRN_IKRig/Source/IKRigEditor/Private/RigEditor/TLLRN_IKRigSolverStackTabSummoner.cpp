// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigSolverStackTabSummoner.h"

#include "IDocumentation.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "RigEditor/TLLRN_SIKRigSolverStack.h"

#define LOCTEXT_NAMESPACE "TLLRN_SolverStackTabSummoner"

const FName FTLLRN_IKRigSolverStackTabSummoner::TabID(TEXT("TLLRN_IKRigSolverStack"));

FTLLRN_IKRigSolverStackTabSummoner::FTLLRN_IKRigSolverStackTabSummoner(const TSharedRef<FTLLRN_IKRigEditorToolkit>& InTLLRN_IKRigEditor)
	: FWorkflowTabFactory(TabID, InTLLRN_IKRigEditor)
	, TLLRN_IKRigEditor(InTLLRN_IKRigEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("TLLRN_IKRigSolverStackTabLabel", "Solver Stack");
	TabIcon = FSlateIcon(FTLLRN_IKRigEditorStyle::Get().GetStyleSetName(), "TLLRN_IKRig.SolverStack");

	ViewMenuDescription = LOCTEXT("TLLRN_IKRigSolverStack_ViewMenu_Desc", "IK Rig Solver Stack");
	ViewMenuTooltip = LOCTEXT("TLLRN_IKRigSolverStack_ViewMenu_ToolTip", "Show the IK Rig Solver Stack Tab");
}

TSharedPtr<SToolTip> FTLLRN_IKRigSolverStackTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("TLLRN_IKRigSolverStackTooltip", "A stack of IK solvers executed from top to bottom. These are associated with goals to affect the skeleton."), NULL, TEXT("Shared/Editors/Persona"), TEXT("TLLRN_IKRigSolverStack_Window"));
}

TSharedRef<SWidget> FTLLRN_IKRigSolverStackTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(TLLRN_SIKRigSolverStack, TLLRN_IKRigEditor.Pin()->GetController());
}

#undef LOCTEXT_NAMESPACE
