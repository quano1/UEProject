// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_IKRetargetApplicationMode.h"

#include "IPersonaPreviewScene.h"
#include "PersonaModule.h"
#include "Modules/ModuleManager.h"
#include "PersonaTabs.h"
#include "RetargetEditor/TLLRN_IKRetargetAssetBrowserTabSummoner.h"

#include "RetargetEditor/TLLRN_IKRetargetChainTabSummoner.h"
#include "RetargetEditor/TLLRN_IKRetargetEditor.h"
#include "RetargetEditor/TLLRN_IKRetargetHierarchyTabSummoner.h"
#include "RetargetEditor/TLLRN_IKRetargetOutputLogTabSummoner.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRetargetMode"


FTLLRN_IKRetargetApplicationMode::FTLLRN_IKRetargetApplicationMode(
	TSharedRef<FWorkflowCentricApplication> InHostingApp,  
	TSharedRef<IPersonaPreviewScene> InPreviewScene)
	: FApplicationMode(TLLRN_IKRetargetApplicationModes::TLLRN_IKRetargetApplicationMode)
{
	TLLRN_IKRetargetEditorPtr = StaticCastSharedRef<FTLLRN_IKRetargetEditor>(InHostingApp);
	TSharedRef<FTLLRN_IKRetargetEditor> TLLRN_IKRetargetEditor = StaticCastSharedRef<FTLLRN_IKRetargetEditor>(InHostingApp);

	FPersonaViewportArgs ViewportArgs(InPreviewScene);
	ViewportArgs.bAlwaysShowTransformToolbar = true;
	ViewportArgs.bShowStats = false;
	ViewportArgs.bShowTurnTable = false;
	ViewportArgs.ContextName = TEXT("TLLRN_IKRetargetEditor.Viewport");
	ViewportArgs.OnViewportCreated = FOnViewportCreated::CreateSP(TLLRN_IKRetargetEditor, &FTLLRN_IKRetargetEditor::HandleViewportCreated);

	// register Persona tabs
	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	TabFactories.RegisterFactory(PersonaModule.CreatePersonaViewportTabFactory(InHostingApp, ViewportArgs));
	TabFactories.RegisterFactory(PersonaModule.CreateDetailsTabFactory(InHostingApp, FOnDetailsCreated::CreateSP(&TLLRN_IKRetargetEditor.Get(), &FTLLRN_IKRetargetEditor::HandleDetailsCreated)));
	TabFactories.RegisterFactory(PersonaModule.CreateAdvancedPreviewSceneTabFactory(InHostingApp, TLLRN_IKRetargetEditor->GetPersonaToolkit()->GetPreviewScene()));

	// register custom tabs
	TabFactories.RegisterFactory(MakeShared<FTLLRN_IKRetargetChainTabSummoner>(TLLRN_IKRetargetEditor));
	TabFactories.RegisterFactory(MakeShared<FTLLRN_IKRetargetAssetBrowserTabSummoner>(TLLRN_IKRetargetEditor));
	TabFactories.RegisterFactory(MakeShared<FTLLRN_IKRetargetOutputLogTabSummoner>(TLLRN_IKRetargetEditor));
	TabFactories.RegisterFactory(MakeShared<FTLLRN_IKRetargetHierarchyTabSummoner>(TLLRN_IKRetargetEditor));

	// create tab layout
	TabLayout = FTabManager::NewLayout("Standalone_TLLRN_IKRetargetEditor_Layout_v1.018")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.8f)
				->SetOrientation(Orient_Horizontal)
				->Split
				(
				FTabManager::NewSplitter()
					//->SetSizeCoefficient(0.8f)
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.2f)
						->SetHideTabWell(true)
						->AddTab(FTLLRN_IKRetargetHierarchyTabSummoner::TabID, ETabState::OpenedTab)
					)
					->Split
					(
					FTabManager::NewSplitter()
							->SetSizeCoefficient(0.8f)
							->SetOrientation(Orient_Vertical)
							->Split
							(
							FTabManager::NewStack()
								->SetSizeCoefficient(0.9f)
								->SetHideTabWell(true)
								->AddTab(FPersonaTabs::PreviewViewportID, ETabState::OpenedTab)
							)
							->Split
							(
							FTabManager::NewStack()
								->SetSizeCoefficient(0.1f)
								->AddTab(FTLLRN_IKRetargetOutputLogTabSummoner::TabID, ETabState::OpenedTab)
							)
					)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.2f)
					->SetOrientation(Orient_Vertical)
					->Split
					(
					FTabManager::NewStack()
						->SetSizeCoefficient(0.7f)
						->AddTab(FPersonaTabs::DetailsID, ETabState::OpenedTab)
						->AddTab(FPersonaTabs::AdvancedPreviewSceneSettingsID, ETabState::OpenedTab)
						->SetForegroundTab(FPersonaTabs::DetailsID)
					)
					->Split
					(
					FTabManager::NewStack()
						->SetSizeCoefficient(0.3f)
						->AddTab(FTLLRN_IKRetargetChainTabSummoner::TabID, ETabState::OpenedTab)
						->AddTab(FTLLRN_IKRetargetAssetBrowserTabSummoner::TabID, ETabState::OpenedTab)
						->SetForegroundTab(FTLLRN_IKRetargetChainTabSummoner::TabID)
					)
					
				)
			)
		);

	PersonaModule.OnRegisterTabs().Broadcast(TabFactories, InHostingApp);
	LayoutExtender = MakeShared<FLayoutExtender>();
	PersonaModule.OnRegisterLayoutExtensions().Broadcast(*LayoutExtender.Get());
	TabLayout->ProcessExtensions(*LayoutExtender.Get());
}

void FTLLRN_IKRetargetApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FTLLRN_IKRetargetEditor> TLLRN_IKRigEditor = TLLRN_IKRetargetEditorPtr.Pin();
	TLLRN_IKRigEditor->RegisterTabSpawners(InTabManager.ToSharedRef());
	TLLRN_IKRigEditor->PushTabFactories(TabFactories);
	FApplicationMode::RegisterTabFactories(InTabManager);
}

#undef LOCTEXT_NAMESPACE
