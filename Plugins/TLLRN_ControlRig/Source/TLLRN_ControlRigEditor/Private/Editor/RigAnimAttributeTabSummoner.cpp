// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/RigAnimAttributeTabSummoner.h"
#include "Editor/STLLRN_ControlRigAnimAttributeView.h"

#include "Editor/TLLRN_ControlRigEditor.h"

#define LOCTEXT_NAMESPACE "RigAnimAttributeTabSummoner"

const FName FRigAnimAttributeTabSummoner::TabID(TEXT("RigAnimAttribute"));

FRigAnimAttributeTabSummoner::FRigAnimAttributeTabSummoner(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor)
	: FWorkflowTabFactory(TabID, InTLLRN_ControlRigEditor)
	, TLLRN_ControlRigEditor(InTLLRN_ControlRigEditor)
{
	TabLabel = LOCTEXT("RigAnimAttributeTabLabel", "Animation Attributes");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("RigAnimAttribute_ViewMenu_Desc", "Animation Attribute");
	ViewMenuTooltip = LOCTEXT("RigAnimAttribute_ViewMenu_ToolTip", "Show the Animation Attribute tab");
}

TSharedRef<SWidget> FRigAnimAttributeTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(STLLRN_ControlRigAnimAttributeView, TLLRN_ControlRigEditor.Pin().ToSharedRef());
}

#undef LOCTEXT_NAMESPACE 
