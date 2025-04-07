// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_ControlRigEditorMode.h" 
#include "BlueprintEditorTabs.h"
#include "SBlueprintEditorToolbar.h"
#include "PersonaModule.h"
#include "IPersonaToolkit.h"
#include "PersonaTabs.h"
#include "Editor/TLLRN_RigHierarchyTabSummoner.h"
#include "Editor/TLLRN_ModularRigModelTabSummoner.h"
#include "Editor/TLLRN_RigModuleAssetBrowserTabSummoner.h"
#include "Editor/RigVMExecutionStackTabSummoner.h"
#include "Editor/TLLRN_TLLRN_RigCurveContainerTabSummoner.h"
#include "Editor/RigValidationTabSummoner.h"
#include "Editor/RigAnimAttributeTabSummoner.h"
#include "ToolMenus.h"

FTLLRN_ControlRigEditorMode::FTLLRN_ControlRigEditorMode(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor, bool bCreateDefaultLayout)
	: FBlueprintEditorApplicationMode(InTLLRN_ControlRigEditor, FTLLRN_ControlRigEditorModes::TLLRN_ControlRigEditorMode, FTLLRN_ControlRigEditorModes::GetLocalizedMode, false, false)
{
	TLLRN_ControlRigBlueprintPtr = CastChecked<UTLLRN_ControlRigBlueprint>(InTLLRN_ControlRigEditor->GetBlueprintObj());

	TabFactories.RegisterFactory(MakeShared<FTLLRN_RigHierarchyTabSummoner>(InTLLRN_ControlRigEditor));
	TabFactories.RegisterFactory(MakeShared<FRigVMExecutionStackTabSummoner>(InTLLRN_ControlRigEditor));
	TabFactories.RegisterFactory(MakeShared<FTLLRN_TLLRN_RigCurveContainerTabSummoner>(InTLLRN_ControlRigEditor));
	TabFactories.RegisterFactory(MakeShared<FRigValidationTabSummoner>(InTLLRN_ControlRigEditor));
	TabFactories.RegisterFactory(MakeShared<FRigAnimAttributeTabSummoner>(InTLLRN_ControlRigEditor));

	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");

 	FPersonaViewportArgs ViewportArgs(InTLLRN_ControlRigEditor->GetPersonaToolkit()->GetPreviewScene());
 	ViewportArgs.BlueprintEditor = InTLLRN_ControlRigEditor;
 	ViewportArgs.bShowStats = false;
	ViewportArgs.bShowPlaySpeedMenu = false;
	ViewportArgs.bShowTimeline = true;
	ViewportArgs.bShowTurnTable = false;
	ViewportArgs.bAlwaysShowTransformToolbar = true;
	ViewportArgs.OnViewportCreated = FOnViewportCreated::CreateSP(InTLLRN_ControlRigEditor, &FTLLRN_ControlRigEditor::HandleViewportCreated);
 
 	TabFactories.RegisterFactory(PersonaModule.CreatePersonaViewportTabFactory(InTLLRN_ControlRigEditor, ViewportArgs));
	TabFactories.RegisterFactory(PersonaModule.CreateAdvancedPreviewSceneTabFactory(InTLLRN_ControlRigEditor, InTLLRN_ControlRigEditor->GetPersonaToolkit()->GetPreviewScene()));

	if(bCreateDefaultLayout)
	{
		TabLayout = FTabManager::NewLayout("Standalone_TLLRN_ControlRigEditMode_Layout_v1.8")
			->AddArea
			(
				// Main application area
				FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.2f)
						->Split
						(
							//	Left top - viewport
							FTabManager::NewStack()
							->SetSizeCoefficient(0.5f)
							->SetHideTabWell(true)
							->AddTab(FPersonaTabs::PreviewViewportID, ETabState::OpenedTab)
						
						)
						->Split
						(
							//	Left bottom - rig/hierarchy
							FTabManager::NewStack()
							->SetSizeCoefficient(0.5f)
							->AddTab(FTLLRN_RigHierarchyTabSummoner::TabID, ETabState::OpenedTab)
							->AddTab(FRigVMExecutionStackTabSummoner::TabID, ETabState::OpenedTab)
							->AddTab(FTLLRN_TLLRN_RigCurveContainerTabSummoner::TabID, ETabState::OpenedTab)
							->AddTab(FBlueprintEditorTabs::MyBlueprintID, ETabState::OpenedTab)
						)
					)
					->Split
					(
						// Middle 
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.6f)
						->Split
						(
							// Middle top - document edit area
							FTabManager::NewStack()
							->SetSizeCoefficient(0.8f)
							->AddTab("Document", ETabState::ClosedTab)
						)
						->Split
						(
							// Middle bottom - compiler results & find
							FTabManager::NewStack()
							->SetSizeCoefficient(0.2f)
							->AddTab(FBlueprintEditorTabs::CompilerResultsID, ETabState::ClosedTab)
							->AddTab(FBlueprintEditorTabs::FindResultsID, ETabState::ClosedTab)
						)
					)
					->Split
					(
						// Right side
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.2f)
						->Split
						(
							// Right top
							FTabManager::NewStack()
							->SetHideTabWell(false)
							->SetSizeCoefficient(1.f)
							->AddTab(FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab)
							->AddTab(FPersonaTabs::AdvancedPreviewSceneSettingsID, ETabState::OpenedTab)
							->AddTab(FRigAnimAttributeTabSummoner::TabID, ETabState::OpenedTab)
							->SetForegroundTab(FBlueprintEditorTabs::DetailsID)
						)
					)
				)
			);
	}
	
	if (UToolMenu* Toolbar = InTLLRN_ControlRigEditor->RegisterModeToolbarIfUnregistered(GetModeName()))
	{
		InTLLRN_ControlRigEditor->GetToolbarBuilder()->AddCompileToolbar(Toolbar);
		InTLLRN_ControlRigEditor->GetToolbarBuilder()->AddScriptingToolbar(Toolbar);
		InTLLRN_ControlRigEditor->GetToolbarBuilder()->AddBlueprintGlobalOptionsToolbar(Toolbar);
	}
}

void FTLLRN_ControlRigEditorMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FBlueprintEditor> BP = MyBlueprintEditor.Pin();
	
	BP->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	BP->PushTabFactories(CoreTabFactories);
	BP->PushTabFactories(BlueprintEditorTabFactories);
	BP->PushTabFactories(TabFactories);
}

FTLLRN_ModularRigEditorMode::FTLLRN_ModularRigEditorMode(const TSharedRef<FTLLRN_ControlRigEditor>& InTLLRN_ControlRigEditor)
	: FTLLRN_ControlRigEditorMode(InTLLRN_ControlRigEditor, false)
{

	TabFactories.RegisterFactory(MakeShared<FTLLRN_ModularRigModelTabSummoner>(InTLLRN_ControlRigEditor));
	TabFactories.RegisterFactory(MakeShared<FTLLRN_RigModuleAssetBrowserTabSummoner>(InTLLRN_ControlRigEditor));
	
	TabLayout = FTabManager::NewLayout("Standalone_TLLRN_ModularRigEditMode_Layout_v1.3")
		->AddArea
		(
			// Main application area
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.2f)
					->Split
					(
						// Left top - Module Library
						FTabManager::NewStack()
						->SetHideTabWell(false)
						->SetSizeCoefficient(0.5f)
						->AddTab(FTLLRN_RigModuleAssetBrowserTabSummoner::TabID, ETabState::OpenedTab)
						->SetForegroundTab(FBlueprintEditorTabs::DetailsID)
					)
					->Split
					(
						//	Left bottom - rig/hierarchy/modules
						FTabManager::NewStack()
						->SetHideTabWell(false)
						->SetSizeCoefficient(0.5f)
						->SetForegroundTab(FTLLRN_ModularRigModelTabSummoner::TabID)
						->AddTab(FTLLRN_ModularRigModelTabSummoner::TabID, ETabState::OpenedTab)
						->AddTab(FBlueprintEditorTabs::MyBlueprintID, ETabState::OpenedTab)
					)
				)
				->Split
				(
					//	Center - viewport
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->SetHideTabWell(true)
					->AddTab(FPersonaTabs::PreviewViewportID, ETabState::OpenedTab)
				)
				->Split
				(
					// Right side
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->SetHideTabWell(false)
					->SetForegroundTab(FBlueprintEditorTabs::DetailsID)
					->AddTab(FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab)
					->AddTab(FPersonaTabs::AdvancedPreviewSceneSettingsID, ETabState::OpenedTab)
					->AddTab(FTLLRN_RigHierarchyTabSummoner::TabID, ETabState::OpenedTab)
				)
			)
		);
}

void FTLLRN_ModularRigEditorMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FBlueprintEditor> BP = MyBlueprintEditor.Pin();
	
	BP->RegisterToolbarTab(InTabManager.ToSharedRef());

	static const TArray<FName> DisallowedTabs = {
		FBlueprintEditorTabs::PaletteID,
		FBlueprintEditorTabs::ReplaceNodeReferencesID,
		FBlueprintEditorTabs::CompilerResultsID,
		FBlueprintEditorTabs::FindResultsID,
		FBlueprintEditorTabs::BookmarksID,
		FRigVMExecutionStackTabSummoner::TabID
	};

	auto PushTabFactories = [&](FWorkflowAllowedTabSet& Tabs)
	{
		for (auto FactoryIt = Tabs.CreateIterator(); FactoryIt; ++FactoryIt)
		{
			if (DisallowedTabs.Contains(FactoryIt->Key))
			{
				continue;
			}
			FactoryIt.Value()->RegisterTabSpawner(InTabManager.ToSharedRef(), BP->GetCurrentModePtr().Get());
		}
	};

	// Mode-specific setup
	PushTabFactories(CoreTabFactories);
	PushTabFactories(BlueprintEditorTabFactories);
	PushTabFactories(TabFactories);
}
