// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigSkeletonTabSummoner.h"

#include "IDocumentation.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "RigEditor/TLLRN_SIKRigHierarchy.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRigSkeletonTabSummoner"

const FName FTLLRN_IKRigSkeletonTabSummoner::TabID(TEXT("TLLRN_IKRigSkeleton"));

FTLLRN_IKRigSkeletonTabSummoner::FTLLRN_IKRigSkeletonTabSummoner(const TSharedRef<FTLLRN_IKRigEditorToolkit>& InTLLRN_IKRigEditor)
	: FWorkflowTabFactory(TabID, InTLLRN_IKRigEditor)
	, TLLRN_IKRigEditor(InTLLRN_IKRigEditor)
{
	bIsSingleton = true; // only allow a single instance of this tab
	
	TabLabel = LOCTEXT("TLLRN_IKRigSkeletonTabLabel", "Hierarchy");
	TabIcon = FSlateIcon(FTLLRN_IKRigEditorStyle::Get().GetStyleSetName(), "TLLRN_IKRig.Hierarchy");

	ViewMenuDescription = LOCTEXT("TLLRN_IKRigSkeleton_ViewMenu_Desc", "Hierarchy");
	ViewMenuTooltip = LOCTEXT("TLLRN_IKRigSkeleton_ViewMenu_ToolTip", "Show the IK Rig Hierarchy Tab");
}

TSharedPtr<SToolTip> FTLLRN_IKRigSkeletonTabSummoner::CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const
{
	return  IDocumentation::Get()->CreateToolTip(LOCTEXT("TLLRN_IKRigSkeletonTooltip", "The IK Rig Hierarchy tab lets you view the rig hierarchy."), NULL, TEXT("Shared/Editors/Persona"), TEXT("TLLRN_IKRigSkeleton_Window"));
}

TSharedRef<SWidget> FTLLRN_IKRigSkeletonTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(TLLRN_SIKRigHierarchy, TLLRN_IKRigEditor.Pin()->GetController());
}

#undef LOCTEXT_NAMESPACE 
