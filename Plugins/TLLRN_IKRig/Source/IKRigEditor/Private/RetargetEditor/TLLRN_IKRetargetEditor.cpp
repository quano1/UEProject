// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_IKRetargetEditor.h"

#include "AnimationEditorViewportClient.h"
#include "AnimationEditorPreviewActor.h"
#include "EditorModeManager.h"
#include "GameFramework/WorldSettings.h"
#include "Modules/ModuleManager.h"
#include "PersonaModule.h"
#include "IPersonaToolkit.h"
#include "IAssetFamily.h"
#include "ISkeletonEditorModule.h"
#include "Preferences/PersonaOptions.h"
#include "AnimCustomInstanceHelper.h"
#include "IPersonaViewport.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Retargeter/TLLRN_IKRetargeter.h"
#include "Retargeter/TLLRN_IKRetargetOps.h"
#include "RetargetEditor/TLLRN_IKRetargetAnimInstance.h"
#include "RetargetEditor/TLLRN_IKRetargetCommands.h"
#include "RetargetEditor/TLLRN_IKRetargetEditPoseMode.h"
#include "RetargetEditor/TLLRN_IKRetargetApplicationMode.h"
#include "RetargetEditor/TLLRN_IKRetargetDefaultMode.h"
#include "RetargetEditor/TLLRN_IKRetargetEditorController.h"

#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRetargeterEditor"

const FName TLLRN_IKRetargetApplicationModes::TLLRN_IKRetargetApplicationMode("TLLRN_IKRetargetApplicationMode");
const FName TLLRN_IKRetargetEditorAppName = FName(TEXT("TLLRN_IKRetargetEditorApp"));

FTLLRN_IKRetargetEditor::FTLLRN_IKRetargetEditor()
	: EditorController(MakeShared<FTLLRN_IKRetargetEditorController>())
	, PreviousTime(-1.0f)
{
}

void FTLLRN_IKRetargetEditor::InitAssetEditor(
	const EToolkitMode::Type Mode,
    const TSharedPtr<IToolkitHost>& InitToolkitHost, 
    UTLLRN_IKRetargeter* InAsset)
{
	EditorController->Initialize(SharedThis(this), InAsset);

	BindCommands();
	
	FPersonaToolkitArgs PersonaToolkitArgs;
	PersonaToolkitArgs.OnPreviewSceneCreated = FOnPreviewSceneCreated::FDelegate::CreateSP(this, &FTLLRN_IKRetargetEditor::HandlePreviewSceneCreated);
	PersonaToolkitArgs.OnPreviewSceneSettingsCustomized = FOnPreviewSceneSettingsCustomized::FDelegate::CreateSP(this, &FTLLRN_IKRetargetEditor::HandleOnPreviewSceneSettingsCustomized);
	
	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	PersonaToolkit = PersonaModule.CreatePersonaToolkit(InAsset, PersonaToolkitArgs);

	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(
		Mode, 
		InitToolkitHost, 
		TLLRN_IKRetargetEditorAppName, 
		FTabManager::FLayout::NullLayout, 
		bCreateDefaultStandaloneMenu, 
		bCreateDefaultToolbar, 
		InAsset);

	// this sets the application mode which defines the tab factory that builds the editor layout
	AddApplicationMode(
		TLLRN_IKRetargetApplicationModes::TLLRN_IKRetargetApplicationMode,
		MakeShareable(new FTLLRN_IKRetargetApplicationMode(SharedThis(this), PersonaToolkit->GetPreviewScene())));
	SetCurrentMode(TLLRN_IKRetargetApplicationModes::TLLRN_IKRetargetApplicationMode);

	// set the default editing mode to use in the editor
	GetEditorModeManager().SetDefaultMode(FTLLRN_IKRetargetDefaultMode::ModeName);
	
	// give default editing mode a pointer to the editor controller
	GetEditorModeManager().ActivateMode(FTLLRN_IKRetargetDefaultMode::ModeName);
	FTLLRN_IKRetargetDefaultMode* DefaultMode = GetEditorModeManager().GetActiveModeTyped<FTLLRN_IKRetargetDefaultMode>(FTLLRN_IKRetargetDefaultMode::ModeName);
	DefaultMode->SetEditorController(EditorController);
	GetEditorModeManager().DeactivateMode(FTLLRN_IKRetargetDefaultMode::ModeName);

	// give edit pose mode a pointer to the editor controller
	GetEditorModeManager().ActivateMode(FTLLRN_IKRetargetEditPoseMode::ModeName);
	FTLLRN_IKRetargetEditPoseMode* EditPoseMode = GetEditorModeManager().GetActiveModeTyped<FTLLRN_IKRetargetEditPoseMode>(FTLLRN_IKRetargetEditPoseMode::ModeName);
	EditPoseMode->SetEditorController(EditorController);
	GetEditorModeManager().DeactivateMode(FTLLRN_IKRetargetEditPoseMode::ModeName);

	ExtendToolbar();
	RegenerateMenusAndToolbars();

	// run retarget by default
	EditorController->SetRetargeterMode(ETLLRN_RetargeterOutputMode::RunRetarget);
}

void FTLLRN_IKRetargetEditor::OnClose()
{
	FPersonaAssetEditorToolkit::OnClose();
	EditorController->Close();
}

void FTLLRN_IKRetargetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("TLLRN_WorkspaceMenu_IKRigEditor", "IK Rig Editor"));
	TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FTLLRN_IKRetargetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

void FTLLRN_IKRetargetEditor::BindCommands()
{
	const FTLLRN_IKRetargetCommands& Commands = FTLLRN_IKRetargetCommands::Get();

	//
	// Retarget output modes
	//
	ToolkitCommands->MapAction(
		Commands.RunRetargeter,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::SetRetargeterMode, ETLLRN_RetargeterOutputMode::RunRetarget),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsReadyToRetarget));

	ToolkitCommands->MapAction(
		Commands.EditRetargetPose,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::SetRetargeterMode, ETLLRN_RetargeterOutputMode::EditRetargetPose),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsCurrentMeshLoaded),
		FIsActionChecked());


	//
	// Show various settings in details panel
	//
	ToolkitCommands->MapAction(
		Commands.ShowAssetSettings,
		FExecuteAction::CreateLambda([this]()
		{
			UTLLRN_IKRetargeter* Asset = EditorController->AssetController->GetAsset();
			return EditorController->SetDetailsObject(Asset);
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() ->bool
		{
			const UTLLRN_IKRetargeter* Asset = EditorController->AssetController->GetAsset();
			return EditorController->IsObjectInDetailsView(Asset);	
		}));
	ToolkitCommands->MapAction(
		Commands.ShowGlobalSettings,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::ShowGlobalSettings),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() ->bool
		{
			const UTLLRN_IKRetargetGlobalSettings* GlobalSettings = EditorController->AssetController->GetAsset()->GetGlobalSettingsUObject();
			return EditorController->IsObjectInDetailsView(GlobalSettings);	
		}));
	ToolkitCommands->MapAction(
		Commands.ShowRootSettings,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::ShowRootSettings),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() ->bool
		{
			const UTLLRN_RetargetRootSettings* RootSettings = EditorController->AssetController->GetAsset()->GetRootSettingsUObject();
			return EditorController->IsObjectInDetailsView(RootSettings);	
		}));
	ToolkitCommands->MapAction(
		Commands.ShowPostSettings,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::ShowPostPhaseSettings),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() ->bool
		{
			const UTLLRN_RetargetOpStack* PostSettings = EditorController->AssetController->GetAsset()->GetPostSettingsUObject();
			return EditorController->IsObjectInDetailsView(PostSettings);	
		}));

	//
	// Edit pose commands
	//
	
	ToolkitCommands->MapAction(
		Commands.ResetAllBones,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleResetAllBones),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsEditingPose),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.ResetSelectedBones,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleResetSelectedBones),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsEditingPoseWithAnyBoneSelected),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.ResetSelectedAndChildrenBones,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleResetSelectedAndChildrenBones),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsEditingPoseWithAnyBoneSelected),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.NewRetargetPose,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleNewPose),
		FCanExecuteAction(),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.DuplicateRetargetPose,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleDuplicatePose),
		FCanExecuteAction(),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.DeleteRetargetPose,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleDeletePose),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::CanDeletePose),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.RenameRetargetPose,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleRenamePose),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::CanRenamePose),
		EUIActionRepeatMode::RepeatDisabled);

	//
	// Auto-gen retarget pose
	//
	
	ToolkitCommands->MapAction(
		Commands.AutoAlignAllBones,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleAlignAllBones),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsEditingPose),
		EUIActionRepeatMode::RepeatDisabled);
	
	ToolkitCommands->MapAction(
		Commands.AlignSelected,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleAlignSelectedBones, ETLLRN_RetargetAutoAlignMethod::ChainToChain, false /* no children*/),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsEditingPoseWithAnyBoneSelected),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.AlignSelectedAndChildren,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleAlignSelectedBones, ETLLRN_RetargetAutoAlignMethod::ChainToChain, true /* include children*/),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsEditingPoseWithAnyBoneSelected),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.AlignSelectedUsingMesh,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleAlignSelectedBones, ETLLRN_RetargetAutoAlignMethod::MeshToMesh, false /* no children*/),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsEditingPoseWithAnyBoneSelected),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.SnapCharacterToGround,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::HandleSnapToGround),
		FCanExecuteAction::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::IsEditingPose),
		EUIActionRepeatMode::RepeatDisabled);
	

	//
	// Pose exporter
	//
	const TSharedRef<FTLLRN_IKRetargetPoseExporter> PoseExporterRef = EditorController->PoseExporter.ToSharedRef();
	
	ToolkitCommands->MapAction(
		Commands.ImportRetargetPose,
		FExecuteAction::CreateSP(PoseExporterRef, &FTLLRN_IKRetargetPoseExporter::HandleImportFromPoseAsset),
		FCanExecuteAction(),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.ImportRetargetPoseFromAnim,
		FExecuteAction::CreateSP(PoseExporterRef, &FTLLRN_IKRetargetPoseExporter::HandleImportFromSequenceAsset),
		FCanExecuteAction(),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.ExportRetargetPose,
		FExecuteAction::CreateSP(PoseExporterRef, &FTLLRN_IKRetargetPoseExporter::HandleExportPoseAsset),
		FCanExecuteAction(),
		EUIActionRepeatMode::RepeatDisabled);
}

void FTLLRN_IKRetargetEditor::ExtendToolbar()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	AddToolbarExtender(ToolbarExtender);

	ToolbarExtender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        GetToolkitCommands(),
        FToolBarExtensionDelegate::CreateSP(this, &FTLLRN_IKRetargetEditor::FillToolbar)
    );
}

void FTLLRN_IKRetargetEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection("Retarget Modes");
	{
		ToolbarBuilder.AddToolBarButton(
			FExecuteAction::CreateLambda([this]{ EditorController->SetRetargetModeToPreviousMode(); }),
			NAME_None, 
			TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::GetRetargeterModeLabel)),
			TAttribute<FText>(), 
			TAttribute<FSlateIcon>::Create(TAttribute<FSlateIcon>::FGetter::CreateSP(EditorController, &FTLLRN_IKRetargetEditorController::GetCurrentRetargetModeIcon))
		);
		
		ToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP(this, &FTLLRN_IKRetargetEditor::GenerateRetargetModesMenu),
			LOCTEXT("RetargetMode_Label", "UI Modes"),
			LOCTEXT("RetargetMode_ToolTip", "Choose which mode to display in the viewport."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Recompile"),
			true);

		ToolbarBuilder.AddSeparator();
		const TSharedPtr<SHorizontalBox> RetargetPhaseButtons = GenerateRetargetPhaseButtons();
		ToolbarBuilder.AddWidget(RetargetPhaseButtons.ToSharedRef());
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddWidget(SNew(SSpacer), NAME_None, true, HAlign_Right);

	ToolbarBuilder.BeginSection("Show Settings");
	{
		ToolbarBuilder.AddToolBarButton(
		FTLLRN_IKRetargetCommands::Get().ShowAssetSettings,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FTLLRN_IKRetargetEditorStyle::Get().GetStyleSetName(),"TLLRN_IKRetarget.TLLRN_AssetSettings"));
		
		ToolbarBuilder.AddToolBarButton(
		FTLLRN_IKRetargetCommands::Get().ShowGlobalSettings,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FTLLRN_IKRetargetEditorStyle::Get().GetStyleSetName(),"TLLRN_IKRetarget.TLLRN_GlobalSettings"));

		ToolbarBuilder.AddToolBarButton(
		FTLLRN_IKRetargetCommands::Get().ShowRootSettings,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FTLLRN_IKRetargetEditorStyle::Get().GetStyleSetName(),"TLLRN_IKRetarget.TLLRN_RootSettings"));

		ToolbarBuilder.AddToolBarButton(
		FTLLRN_IKRetargetCommands::Get().ShowPostSettings,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FTLLRN_IKRetargetEditorStyle::Get().GetStyleSetName(),"TLLRN_IKRetarget.TLLRN_PostSettings"));
	}
	ToolbarBuilder.EndSection();
}

TSharedRef<SWidget> FTLLRN_IKRetargetEditor::GenerateRetargetModesMenu()
{
	FMenuBuilder MenuBuilder(true, GetToolkitCommands());
	
	MenuBuilder.BeginSection(TEXT("Retarget Modes"));
	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().RunRetargeter, TEXT("Run Retargeter"), TAttribute<FText>(), TAttribute<FText>(),  EditorController->GetRetargeterModeIcon(ETLLRN_RetargeterOutputMode::RunRetarget));
	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().EditRetargetPose, TEXT("Edit Retarget Pose"), TAttribute<FText>(), TAttribute<FText>(), EditorController->GetRetargeterModeIcon(ETLLRN_RetargeterOutputMode::EditRetargetPose));
	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget();
}

TSharedPtr<SHorizontalBox> FTLLRN_IKRetargetEditor::GenerateRetargetPhaseButtons() const
{
	constexpr float PhaseButtonMargin = 2.f;
	
	TSharedPtr<SHorizontalBox> Box = SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	.AutoWidth()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString("Retarget Phases: "))
		]
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(PhaseButtonMargin))
		[
			SNew(SCheckBox)
			.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
			.IsChecked_Lambda([this]()
			{
				const bool bIsOn =  EditorController->GetGlobalSettings().bEnableRoot;
				return bIsOn ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this](ECheckBoxState InCheckBoxState)
			{
				FTLLRN_RetargetGlobalSettings& GlobalSettings = EditorController->GetGlobalSettings();
				GlobalSettings.bEnableRoot = !GlobalSettings.bEnableRoot;
			})
			[
				SNew(STextBlock).Text(FText::FromString("Root"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(PhaseButtonMargin))
		[
			SNew(SCheckBox)
			.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
			.IsChecked_Lambda([this]()
			{
				const bool bIsOn =  EditorController->GetGlobalSettings().bEnableFK;
				return bIsOn ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this](ECheckBoxState InCheckBoxState)
			{
				FTLLRN_RetargetGlobalSettings& GlobalSettings = EditorController->GetGlobalSettings();
				GlobalSettings.bEnableFK = !GlobalSettings.bEnableFK;
			})
			[
				SNew(STextBlock).Text(FText::FromString("FK"))
			]
		]
		
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(PhaseButtonMargin))
		[

			SNew(SCheckBox)
			.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
			.IsChecked_Lambda([this]()
			{
				const bool bIsOn =  EditorController->GetGlobalSettings().bEnableIK;
				return bIsOn ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this](ECheckBoxState InCheckBoxState)
			{
				FTLLRN_RetargetGlobalSettings& GlobalSettings = EditorController->GetGlobalSettings();
				GlobalSettings.bEnableIK = !GlobalSettings.bEnableIK;
			})
			[
				SNew(STextBlock).Text(FText::FromString("IK"))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(PhaseButtonMargin))
		[
			SNew(SCheckBox)
			.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
			.IsChecked_Lambda([this]()
			{
				const bool bIsOn =  EditorController->GetGlobalSettings().bEnablePost;
				return bIsOn ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this](ECheckBoxState InCheckBoxState)
			{
				FTLLRN_RetargetGlobalSettings& GlobalSettings = EditorController->GetGlobalSettings();
				GlobalSettings.bEnablePost = !GlobalSettings.bEnablePost;
			})
			[
				SNew(STextBlock).Text(FText::FromString("Post"))
			]
		]
	];
	
	return Box;
}

FName FTLLRN_IKRetargetEditor::GetToolkitFName() const
{
	return FName("TLLRN_IKRetargetEditor");
}

FText FTLLRN_IKRetargetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("TLLRN_IKRetargetEditorAppLabel", "IK Retarget Editor");
}

FText FTLLRN_IKRetargetEditor::GetToolkitName() const
{
	return FText::FromString(EditorController->AssetController->GetAsset()->GetName());
}

FLinearColor FTLLRN_IKRetargetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FTLLRN_IKRetargetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("TLLRN_IKRetargetEditor");
}

void FTLLRN_IKRetargetEditor::Tick(float DeltaTime)
{
	// update with latest offsets
	EditorController->UpdateMeshOffset(ETLLRN_RetargetSourceOrTarget::Source);
	EditorController->UpdateMeshOffset(ETLLRN_RetargetSourceOrTarget::Target);

	// retargeter IK planting must be reset when time is reversed or playback jumps ahead 
	const float CurrentTime = EditorController->SourceAnimInstance->GetCurrentTime();
	constexpr float MaxSkipTimeBeforeReset = 0.25f;
	if (CurrentTime < PreviousTime || CurrentTime > PreviousTime + MaxSkipTimeBeforeReset)
	{
		EditorController->ResetIKPlantingState();
	}
	PreviousTime = CurrentTime;
	
	// forces viewport to always update, even when mouse pressed down in other tabs
	GetPersonaToolkit()->GetPreviewScene()->InvalidateViews();
}

TStatId FTLLRN_IKRetargetEditor::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FTLLRN_IKRetargetEditor, STATGROUP_Tickables);
}

void FTLLRN_IKRetargetEditor::PostUndo(bool bSuccess)
{
	EditorController->AssetController->CleanAsset();
	EditorController->HandlePreviewMeshReplaced(ETLLRN_RetargetSourceOrTarget::Source);
	EditorController->HandleRetargeterNeedsInitialized();
}

void FTLLRN_IKRetargetEditor::PostRedo(bool bSuccess)
{
	EditorController->AssetController->CleanAsset();
	EditorController->HandlePreviewMeshReplaced(ETLLRN_RetargetSourceOrTarget::Source);
	EditorController->HandleRetargeterNeedsInitialized();
}

void FTLLRN_IKRetargetEditor::HandleViewportCreated(const TSharedRef<class IPersonaViewport>& InViewport)
{
	// register callbacks to allow the asset to store the Bone Size viewport setting
	FEditorViewportClient& ViewportClient = InViewport->GetViewportClient();
	if (FAnimationViewportClient* AnimViewportClient = static_cast<FAnimationViewportClient*>(&ViewportClient))
	{
		AnimViewportClient->OnSetBoneSize.BindLambda([this](float InBoneSize)
		{
			if (UTLLRN_IKRetargeter* Asset = EditorController->AssetController->GetAsset())
			{
				Asset->Modify();
				Asset->BoneDrawSize = InBoneSize;
			}
		});
		
		AnimViewportClient->OnGetBoneSize.BindLambda([this]() -> float
		{
			if (const UTLLRN_IKRetargeter* Asset = EditorController->AssetController->GetAsset())
			{
				return Asset->BoneDrawSize;
			}

			return 1.0f;
		});
	}

	auto GetBorderColorAndOpacity = [this]()
	{
		// no processor or processor not initialized
		const UTLLRN_IKRetargetProcessor* Processor = EditorController.Get().GetRetargetProcessor();
		if (!Processor || !Processor->IsInitialized() )
		{
			return FLinearColor::Red;
		}

		const ETLLRN_RetargeterOutputMode OutputMode = EditorController.Get().GetRetargeterMode();
		if (OutputMode == ETLLRN_RetargeterOutputMode::RunRetarget)
		{
			return FLinearColor::Transparent;
		}

		return FLinearColor::Blue;
	};
	
	InViewport->AddOverlayWidget(
		SNew(SBorder)
		.BorderImage(FTLLRN_IKRetargetEditorStyle::Get().GetBrush( "TLLRN_IKRetarget.Viewport.Border"))
		.BorderBackgroundColor_Lambda(GetBorderColorAndOpacity)
		.Visibility(EVisibility::HitTestInvisible)
		.Padding(0.0f)
		.ShowEffectWhenDisabled(false)
	);
}

void FTLLRN_IKRetargetEditor::HandlePreviewSceneCreated(const TSharedRef<IPersonaPreviewScene>& InPersonaPreviewScene)
{	
	AAnimationEditorPreviewActor* Actor = InPersonaPreviewScene->GetWorld()->SpawnActor<AAnimationEditorPreviewActor>(AAnimationEditorPreviewActor::StaticClass(), FTransform::Identity);
	Actor->SetFlags(RF_Transient);
	InPersonaPreviewScene->SetActor(Actor);
	
	// create the skeletal mesh components
	EditorController->SourceSkelMeshComponent = NewObject<UDebugSkelMeshComponent>(Actor);
	EditorController->TargetSkelMeshComponent = NewObject<UDebugSkelMeshComponent>(Actor);

	// do not process root motion, we need all motion in world space for retargeting to work correctly
	EditorController->SourceSkelMeshComponent->SetProcessRootMotionMode(EProcessRootMotionMode::Ignore);
	EditorController->TargetSkelMeshComponent->SetProcessRootMotionMode(EProcessRootMotionMode::Ignore);

	// hide skeletons, we want to do custom rendering
	EditorController->SourceSkelMeshComponent->SkeletonDrawMode = ESkeletonDrawMode::Hidden;
	EditorController->TargetSkelMeshComponent->SkeletonDrawMode = ESkeletonDrawMode::Hidden;

	// don't want selectable meshes as it gets in the way of bone selection
	EditorController->SourceSkelMeshComponent->bSelectable = false;
	EditorController->TargetSkelMeshComponent->bSelectable = false;
	
	// setup and apply an anim instance to the skeletal mesh component
	EditorController->SourceAnimInstance = NewObject<UTLLRN_IKRetargetAnimInstance>(EditorController->SourceSkelMeshComponent, TEXT("IKRetargetSourceAnimScriptInstance"));
	EditorController->TargetAnimInstance = NewObject<UTLLRN_IKRetargetAnimInstance>(EditorController->TargetSkelMeshComponent, TEXT("IKRetargetTargetAnimScriptInstance"));
	SetupAnimInstance();
	
	// set components to use custom animation mode
	EditorController->SourceSkelMeshComponent->SetAnimationMode(EAnimationMode::AnimationCustomMode);
	EditorController->TargetSkelMeshComponent->SetAnimationMode(EAnimationMode::AnimationCustomMode);
	
	// must call AddComponent() BEFORE assigning the mesh to prevent auto-assignment of a default anim instance
	EditorController->SourceRootComponent = NewObject<USceneComponent>(Actor);
	EditorController->SourceSkelMeshComponent->AttachToComponent(EditorController->SourceRootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	InPersonaPreviewScene->AddComponent(EditorController->SourceRootComponent, FTransform::Identity);
	InPersonaPreviewScene->AddComponent(EditorController->SourceSkelMeshComponent, FTransform::Identity);
    InPersonaPreviewScene->AddComponent(EditorController->TargetSkelMeshComponent, FTransform::Identity);
    
    // apply component to the preview scene (must be done BEFORE setting mesh)
    InPersonaPreviewScene->SetPreviewMeshComponent(EditorController->SourceSkelMeshComponent);
    InPersonaPreviewScene->SetAdditionalMeshesSelectable(false);
    
    // assign source mesh to the preview scene (applies the mesh to the source component)
	// (must be done AFTER adding the component to prevent InitAnim() from overriding anim instance)
    USkeletalMesh* SourceMesh = EditorController->GetSkeletalMesh(ETLLRN_RetargetSourceOrTarget::Source);
    InPersonaPreviewScene->SetPreviewMesh(SourceMesh);
    
    // assign target mesh directly to the target component
    USkeletalMesh* TargetMesh = EditorController->GetSkeletalMesh(ETLLRN_RetargetSourceOrTarget::Target);
    EditorController->TargetSkelMeshComponent->SetSkeletalMesh(TargetMesh);
	
	EditorController->FixZeroHeightRetargetRoot(ETLLRN_RetargetSourceOrTarget::Source);
	EditorController->FixZeroHeightRetargetRoot(ETLLRN_RetargetSourceOrTarget::Target);

	// bind a callback to update the UI whenever the retarget processor in the target anim instance is initialized
	EditorController->RetargeterInitializedDelegateHandle = EditorController->GetRetargetProcessor()->OnRetargeterInitialized().AddLambda([this]()
	{
		EditorController->RefreshHierarchyView();
	});
}

void FTLLRN_IKRetargetEditor::SetupAnimInstance()
{
	UTLLRN_IKRetargeter* Asset = EditorController->AssetController->GetAsset();
	
	// configure SOURCE anim instance (will only output retarget pose)
	EditorController->SourceAnimInstance->ConfigureAnimInstance(ETLLRN_RetargetSourceOrTarget::Source, Asset, nullptr);
	// configure TARGET anim instance (will output retarget pose AND retarget pose from source skel mesh component)
	EditorController->TargetAnimInstance->ConfigureAnimInstance(ETLLRN_RetargetSourceOrTarget::Target, Asset, EditorController->SourceSkelMeshComponent);

	EditorController->SourceSkelMeshComponent->PreviewInstance = EditorController->SourceAnimInstance.Get();
	EditorController->TargetSkelMeshComponent->PreviewInstance = EditorController->TargetAnimInstance.Get();
}

void FTLLRN_IKRetargetEditor::HandleOnPreviewSceneSettingsCustomized(IDetailLayoutBuilder& DetailBuilder) const
{
	DetailBuilder.HideCategory("Additional Meshes");
	DetailBuilder.HideCategory("Physics");
	DetailBuilder.HideCategory("Mesh");
	DetailBuilder.HideCategory("Animation Blueprint");
}

void FTLLRN_IKRetargetEditor::HandleDetailsCreated(const TSharedRef<class IDetailsView>& InDetailsView)
{
	InDetailsView->OnFinishedChangingProperties().AddSP(this, &FTLLRN_IKRetargetEditor::OnFinishedChangingDetails);
	InDetailsView->SetObject(EditorController->AssetController->GetAsset());
	EditorController->SetDetailsView(InDetailsView);
}

void FTLLRN_IKRetargetEditor::OnFinishedChangingDetails(const FPropertyChangedEvent& PropertyChangedEvent)
{
	UTLLRN_IKRetargeterController* AssetController = EditorController->AssetController;

	// determine which properties were modified
	const bool bSourceIKRigChanged = PropertyChangedEvent.GetPropertyName() == UTLLRN_IKRetargeter::GetSourceIKRigPropertyName();
	const bool bTargetIKRigChanged = PropertyChangedEvent.GetPropertyName() == UTLLRN_IKRetargeter::GetTargetIKRigPropertyName();
	const bool bSourcePreviewChanged = PropertyChangedEvent.GetPropertyName() == UTLLRN_IKRetargeter::GetSourcePreviewMeshPropertyName();
	const bool bTargetPreviewChanged = PropertyChangedEvent.GetPropertyName() == UTLLRN_IKRetargeter::GetTargetPreviewMeshPropertyName();

	// if no override target mesh has been specified, update the override to reflect the mesh in the ik rig asset
	if (bTargetIKRigChanged)
	{
		UTLLRN_IKRigDefinition* NewIKRig = AssetController->GetIKRigWriteable(ETLLRN_RetargetSourceOrTarget::Target);
		AssetController->SetIKRig(ETLLRN_RetargetSourceOrTarget::Target, NewIKRig);
	}

	// if no override source mesh has been specified, update the override to reflect the mesh in the ik rig asset
	if (bSourceIKRigChanged)
	{
		UTLLRN_IKRigDefinition* NewIKRig = AssetController->GetIKRigWriteable(ETLLRN_RetargetSourceOrTarget::Source);
		AssetController->SetIKRig(ETLLRN_RetargetSourceOrTarget::Source, NewIKRig);
	}

	// if either the source or target meshes are possibly modified, update scene components, anim instance and UI
	if (bSourcePreviewChanged)
	{
		USkeletalMesh* Mesh = AssetController->GetPreviewMesh(ETLLRN_RetargetSourceOrTarget::Source);
		AssetController->SetPreviewMesh(ETLLRN_RetargetSourceOrTarget::Source, Mesh);
	}
	
	if (bTargetPreviewChanged)
	{
		USkeletalMesh* Mesh = AssetController->GetPreviewMesh(ETLLRN_RetargetSourceOrTarget::Target);
		AssetController->SetPreviewMesh(ETLLRN_RetargetSourceOrTarget::Target, Mesh);
	}
}

#undef LOCTEXT_NAMESPACE
