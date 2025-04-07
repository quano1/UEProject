// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_IKRetargetHierarchyTabSummoner.h"

#include "RetargetEditor/TLLRN_SIKRetargetHierarchy.h"
#include "RetargetEditor/TLLRN_IKRetargetEditor.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"

#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRetargetHierarchyTabSummoner"

const FName FTLLRN_IKRetargetHierarchyTabSummoner::TabID(TEXT("TLLRN_IKRetargetHierarchy"));

FTLLRN_IKRetargetHierarchyTabSummoner::FTLLRN_IKRetargetHierarchyTabSummoner(const TSharedRef<FTLLRN_IKRetargetEditor>& InEditor)
	: FWorkflowTabFactory(TabID, InEditor)
	, RetargetEditor(InEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("IKRetargetHierarchy_TabLabel", "Hierarchy");
	TabIcon = FSlateIcon(FTLLRN_IKRigEditorStyle::Get().GetStyleSetName(), "TLLRN_IKRig.Hierarchy");

	ViewMenuDescription = LOCTEXT("IKRetargetHierarchy_ViewMenu_Desc", "Hierarchy");
	ViewMenuTooltip = LOCTEXT("IKRetargetHierarchy_ViewMenu_ToolTip", "Show the Retarget Hierarchy Tab");
}

TSharedPtr<SToolTip> FTLLRN_IKRetargetHierarchyTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("IKRetargetHierarchyTooltip", "The IK Retargeting Hierarchy tab lets you view the skeleton hierarchy."), NULL, TEXT("Shared/Editors/Persona"), TEXT("TLLRN_IKRigSkeleton_Window"));
}

TSharedRef<SWidget> FTLLRN_IKRetargetHierarchyTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(TLLRN_SIKRetargetHierarchy, RetargetEditor.Pin()->GetController());
}

#undef LOCTEXT_NAMESPACE 
