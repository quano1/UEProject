// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_RigInfluenceMapTabSummoner.h"
#include "Editor/STLLRN_RigHierarchy.h"
#include "Editor/RigVMEditorStyle.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "SKismetInspector.h"
#include "TLLRN_ControlRigBlueprint.h"

#define LOCTEXT_NAMESPACE "TLLRN_RigInfluenceMapTabSummoner"

const FName FTLLRN_RigInfluenceMapTabSummoner::TabID(TEXT("TLLRN_RigInfluenceMap"));

FTLLRN_RigInfluenceMapTabSummoner::FTLLRN_RigInfluenceMapTabSummoner(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor)
	: FWorkflowTabFactory(TabID, InTLLRN_ControlRigEditor)
	, TLLRN_ControlRigEditor(InTLLRN_ControlRigEditor)
{
	TabLabel = LOCTEXT("TLLRN_RigInfluenceMapTabLabel", "Rig Influence Map");
	TabIcon = FSlateIcon(FRigVMEditorStyle::Get().GetStyleSetName(), "RigVM.TabIcon");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("TLLRN_RigInfluenceMap_ViewMenu_Desc", "Rig Influence Map");
	ViewMenuTooltip = LOCTEXT("TLLRN_RigInfluenceMap_ViewMenu_ToolTip", "Show the Rig Influence Map tab");
}

TSharedRef<SWidget> FTLLRN_RigInfluenceMapTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedRef<SKismetInspector> KismetInspector = SNew(SKismetInspector);

	if (TLLRN_ControlRigEditor.IsValid())
	{
		if (UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(TLLRN_ControlRigEditor.Pin()->GetBlueprintObj()))
		{
			TSharedPtr<FStructOnScope> StructToDisplay = MakeShareable(new FStructOnScope(
				FTLLRN_RigInfluenceMapPerEvent::StaticStruct(), (uint8*)&RigBlueprint->Influences));
			StructToDisplay->SetPackage(RigBlueprint->GetOutermost());
			KismetInspector->ShowSingleStruct(StructToDisplay);
		}
	}

	return KismetInspector;
}

#undef LOCTEXT_NAMESPACE 
