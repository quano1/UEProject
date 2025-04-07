// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigMode.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "IPersonaPreviewScene.h"
#include "IPersonaToolkit.h"
#include "PersonaModule.h"
#include "ISkeletonEditorModule.h"
#include "Modules/ModuleManager.h"
#include "PersonaTabs.h"
#include "RigEditor/TLLRN_IKRigAssetBrowserTabSummoner.h"
#include "RigEditor/TLLRN_IKRigOutputLogTabSummoner.h"
#include "RigEditor/TLLRN_IKRigSkeletonTabSummoner.h"
#include "RigEditor/TLLRN_IKRigSolverStackTabSummoner.h"
#include "RigEditor/TLLRN_IKRigRetargetChainTabSummoner.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRigMode"

FTLLRN_IKRigMode::FTLLRN_IKRigMode(
	TSharedRef<FWorkflowCentricApplication> InHostingApp,  
	TSharedRef<IPersonaPreviewScene> InPreviewScene)
	: FApplicationMode(TLLRN_IKRigEditorModes::TLLRN_IKRigEditorMode)
{
	TLLRN_IKRigEditorPtr = StaticCastSharedRef<FTLLRN_IKRigEditorToolkit>(InHostingApp);
	TSharedRef<FTLLRN_IKRigEditorToolkit> TLLRN_IKRigEditor = StaticCastSharedRef<FTLLRN_IKRigEditorToolkit>(InHostingApp);

	FPersonaViewportArgs ViewportArgs(InPreviewScene);
	ViewportArgs.bAlwaysShowTransformToolbar = true;
	ViewportArgs.bShowStats = false;
	ViewportArgs.bShowTurnTable = false;
	ViewportArgs.ContextName = TEXT("TLLRN_IKRigEditor.Viewport");
	ViewportArgs.OnViewportCreated = FOnViewportCreated::CreateSP(TLLRN_IKRigEditor, &FTLLRN_IKRigEditorToolkit::HandleViewportCreated);

	// register Persona tabs
	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	TabFactories.RegisterFactory(PersonaModule.CreatePersonaViewportTabFactory(InHostingApp, ViewportArgs));
	TabFactories.RegisterFactory(PersonaModule.CreateDetailsTabFactory(InHostingApp, FOnDetailsCreated::CreateSP(&TLLRN_IKRigEditor.Get(), &FTLLRN_IKRigEditorToolkit::HandleDetailsCreated)));
	TabFactories.RegisterFactory(PersonaModule.CreateAdvancedPreviewSceneTabFactory(InHostingApp, TLLRN_IKRigEditor->GetPersonaToolkit()->GetPreviewScene()));

	// register custom tabs
	TabFactories.RegisterFactory(MakeShared<FTLLRN_IKRigAssetBrowserTabSummoner>(TLLRN_IKRigEditor));
	TabFactories.RegisterFactory(MakeShared<FTLLRN_IKRigSkeletonTabSummoner>(TLLRN_IKRigEditor));
	TabFactories.RegisterFactory(MakeShared<FTLLRN_IKRigSolverStackTabSummoner>(TLLRN_IKRigEditor));
	TabFactories.RegisterFactory(MakeShared<FTLLRN_IKRigRetargetChainTabSummoner>(TLLRN_IKRigEditor));
	TabFactories.RegisterFactory(MakeShared<FTLLRN_IKRigOutputLogTabSummoner>(TLLRN_IKRigEditor));

	// create tab layout
	TabLayout = FTabManager::NewLayout("Standalone_TLLRN_IKRigEditor_Layout_v1.127")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.9f)
				->SetOrientation(Orient_Horizontal)
				->Split
				(
				    FTabManager::NewSplitter()
				    ->SetSizeCoefficient(0.2f)
				    ->SetOrientation(Orient_Vertical)
				    ->Split
				    (
					    FTabManager::NewStack()
					    ->SetSizeCoefficient(0.6f)
					    ->AddTab(FTLLRN_IKRigSkeletonTabSummoner::TabID, ETabState::OpenedTab)
					)
					->Split
					(
					    FTabManager::NewStack()
					    ->SetSizeCoefficient(0.4f)
					    ->AddTab(FTLLRN_IKRigSolverStackTabSummoner::TabID, ETabState::OpenedTab)
					)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.8f)
					->SetOrientation(Orient_Vertical)
					->Split
					(
					FTabManager::NewStack()
						->SetSizeCoefficient(0.6f)
						->SetHideTabWell(true)
						->AddTab(FPersonaTabs::PreviewViewportID, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.2f)
						->AddTab(FTLLRN_IKRigOutputLogTabSummoner::TabID, ETabState::OpenedTab)
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
						->SetSizeCoefficient(0.6f)
						->AddTab(FPersonaTabs::DetailsID, ETabState::OpenedTab)
						->AddTab(FPersonaTabs::AdvancedPreviewSceneSettingsID, ETabState::OpenedTab)
						->SetForegroundTab(FPersonaTabs::DetailsID)
                    )
                    ->Split
                    (
						FTabManager::NewStack()
						->SetSizeCoefficient(0.6f)
						->AddTab(FTLLRN_IKRigAssetBrowserTabSummoner::TabID, ETabState::OpenedTab)
						->AddTab(FTLLRN_IKRigRetargetChainTabSummoner::TabID, ETabState::OpenedTab)
						->SetForegroundTab(FTLLRN_IKRigAssetBrowserTabSummoner::TabID)
                    )
				)
			)
		);

	PersonaModule.OnRegisterTabs().Broadcast(TabFactories, InHostingApp);
	LayoutExtender = MakeShared<FLayoutExtender>();
	PersonaModule.OnRegisterLayoutExtensions().Broadcast(*LayoutExtender.Get());
	TabLayout->ProcessExtensions(*LayoutExtender.Get());
}

void FTLLRN_IKRigMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FTLLRN_IKRigEditorToolkit> TLLRN_IKRigEditor = TLLRN_IKRigEditorPtr.Pin();
	TLLRN_IKRigEditor->RegisterTabSpawners(InTabManager.ToSharedRef());
	TLLRN_IKRigEditor->PushTabFactories(TabFactories);
	FApplicationMode::RegisterTabFactories(InTabManager);
}

#undef LOCTEXT_NAMESPACE
