// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/RigValidationTabSummoner.h"
#include "Editor/STLLRN_RigHierarchy.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Editor/STLLRN_ControlRigValidationWidget.h"

#define LOCTEXT_NAMESPACE "RigValidationTabSummoner"

const FName FRigValidationTabSummoner::TabID(TEXT("RigValidation"));

FRigValidationTabSummoner::FRigValidationTabSummoner(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor)
	: FWorkflowTabFactory(TabID, InTLLRN_ControlRigEditor)
	, WeakTLLRN_ControlRigEditor(InTLLRN_ControlRigEditor)
{
	TabLabel = LOCTEXT("RigValidationTabLabel", "Rig Validation");
	TabIcon = FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "RigValidation.TabIcon");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("RigValidation_ViewMenu_Desc", "Rig Validation");
	ViewMenuTooltip = LOCTEXT("RigValidation_ViewMenu_ToolTip", "Show the Rig Validation tab");
}

TSharedRef<SWidget> FRigValidationTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	ensure(WeakTLLRN_ControlRigEditor.IsValid());

	UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(WeakTLLRN_ControlRigEditor.Pin()->GetBlueprintObj());
	check(RigBlueprint);
	
	UTLLRN_ControlRigValidator* Validator = RigBlueprint->Validator;
	check(Validator);

	TSharedRef<STLLRN_ControlRigValidationWidget> ValidationWidget = SNew(STLLRN_ControlRigValidationWidget, Validator);
	Validator->SetTLLRN_ControlRig(Cast<UTLLRN_ControlRig>(RigBlueprint->GetObjectBeingDebugged()));
	return ValidationWidget;
}

#undef LOCTEXT_NAMESPACE 
