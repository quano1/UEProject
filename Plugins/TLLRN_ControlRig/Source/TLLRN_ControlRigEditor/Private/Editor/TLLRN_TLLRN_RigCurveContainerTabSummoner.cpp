// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_TLLRN_RigCurveContainerTabSummoner.h"
#include "Editor/STLLRN_TLLRN_RigCurveContainer.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "Editor/TLLRN_ControlRigEditor.h"

#define LOCTEXT_NAMESPACE "TLLRN_TLLRN_RigCurveContainerTabSummoner"

const FName FTLLRN_TLLRN_RigCurveContainerTabSummoner::TabID(TEXT("TLLRN_TLLRN_RigCurveContainer"));

FTLLRN_TLLRN_RigCurveContainerTabSummoner::FTLLRN_TLLRN_RigCurveContainerTabSummoner(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor)
	: FWorkflowTabFactory(TabID, InTLLRN_ControlRigEditor)
	, TLLRN_ControlRigEditor(InTLLRN_ControlRigEditor)
{
	TabLabel = LOCTEXT("TLLRN_TLLRN_RigCurveContainerTabLabel", "Curve Container");
	TabIcon = FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "CurveContainer.TabIcon");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("TLLRN_TLLRN_RigCurveContainer_ViewMenu_Desc", "Curve Container");
	ViewMenuTooltip = LOCTEXT("TLLRN_TLLRN_RigCurveContainer_ViewMenu_ToolTip", "Show the Rig Curve Container tab");
}

TSharedRef<SWidget> FTLLRN_TLLRN_RigCurveContainerTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(STLLRN_TLLRN_RigCurveContainer, TLLRN_ControlRigEditor.Pin().ToSharedRef());
}

#undef LOCTEXT_NAMESPACE 
