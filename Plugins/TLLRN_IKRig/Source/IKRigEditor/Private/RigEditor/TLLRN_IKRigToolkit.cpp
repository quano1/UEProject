// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigToolkit.h"

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
#include "Animation/DebugSkelMeshComponent.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Rig/TLLRN_IKRigDefinition.h"
#include "IPersonaViewport.h"
#include "RigEditor/TLLRN_IKRigAnimInstance.h"
#include "RigEditor/TLLRN_IKRigCommands.h"
#include "RigEditor/TLLRN_IKRigEditMode.h"
#include "RigEditor/TLLRN_IKRigEditorController.h"
#include "RigEditor/TLLRN_IKRigMode.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRigEditorToolkit"

const FName TLLRN_IKRigEditorModes::TLLRN_IKRigEditorMode("TLLRN_IKRigEditorMode");
const FName TLLRN_IKRigEditorAppName = FName(TEXT("TLLRN_IKRigEditorApp"));

FTLLRN_IKRigEditorToolkit::FTLLRN_IKRigEditorToolkit()
	: EditorController(MakeShared<FTLLRN_IKRigEditorController>())
{
}

FTLLRN_IKRigEditorToolkit::~FTLLRN_IKRigEditorToolkit()
{
	if (PersonaToolkit.IsValid())
	{
		static constexpr bool bSetPreviewMeshInAsset = false;
		PersonaToolkit->SetPreviewMesh(nullptr, bSetPreviewMeshInAsset);
	}
}

void FTLLRN_IKRigEditorToolkit::InitAssetEditor(
	const EToolkitMode::Type Mode,
    const TSharedPtr<IToolkitHost>& InitToolkitHost, 
    UTLLRN_IKRigDefinition* IKRigAsset)
{
	EditorController->Initialize(SharedThis(this), IKRigAsset);

	BindCommands();
	
	FPersonaToolkitArgs PersonaToolkitArgs;
	PersonaToolkitArgs.OnPreviewSceneCreated = FOnPreviewSceneCreated::FDelegate::CreateSP(this, &FTLLRN_IKRigEditorToolkit::HandlePreviewSceneCreated);
	PersonaToolkitArgs.OnPreviewSceneSettingsCustomized = FOnPreviewSceneSettingsCustomized::FDelegate::CreateSP(this, &FTLLRN_IKRigEditorToolkit::HandleOnPreviewSceneSettingsCustomized);
	
	const FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	PersonaToolkit = PersonaModule.CreatePersonaToolkit(IKRigAsset, PersonaToolkitArgs);

	static constexpr bool bCreateDefaultStandaloneMenu = true;
	static constexpr bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(
		Mode, 
		InitToolkitHost, 
		TLLRN_IKRigEditorAppName, 
		FTabManager::FLayout::NullLayout, 
		bCreateDefaultStandaloneMenu, 
		bCreateDefaultToolbar, 
		IKRigAsset);

	TSharedRef<FTLLRN_IKRigMode> TLLRN_IKRigEditMode = MakeShareable(new FTLLRN_IKRigMode(SharedThis(this), PersonaToolkit->GetPreviewScene()));
	AddApplicationMode(TLLRN_IKRigEditorModes::TLLRN_IKRigEditorMode, TLLRN_IKRigEditMode);

	SetCurrentMode(TLLRN_IKRigEditorModes::TLLRN_IKRigEditorMode);

	GetEditorModeManager().SetDefaultMode(FTLLRN_IKRigEditMode::ModeName);
	GetEditorModeManager().ActivateDefaultMode();
	FTLLRN_IKRigEditMode* EditMode = GetEditorModeManager().GetActiveModeTyped<FTLLRN_IKRigEditMode>(FTLLRN_IKRigEditMode::ModeName);
	EditMode->SetEditorController(EditorController);

	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

void FTLLRN_IKRigEditorToolkit::OnClose()
{
	FPersonaAssetEditorToolkit::OnClose();
	EditorController->Close();
}

void FTLLRN_IKRigEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("TLLRN_WorkspaceMenu_IKRigEditor", "IK Rig Editor"));
	TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FTLLRN_IKRigEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

void FTLLRN_IKRigEditorToolkit::BindCommands()
{
	const FTLLRN_IKRigCommands& Commands = FTLLRN_IKRigCommands::Get();

	ToolkitCommands->MapAction(
        Commands.Reset,
        FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRigEditorController::Reset),
		EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.AutoRetargetChains,
		FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRigEditorController::AutoGenerateRetargetChains),
		EUIActionRepeatMode::RepeatDisabled);
	
	ToolkitCommands->MapAction(
			Commands.AutoSetupFBIK,
			FExecuteAction::CreateSP(EditorController, &FTLLRN_IKRigEditorController::AutoGenerateFBIK),
			EUIActionRepeatMode::RepeatDisabled);

	ToolkitCommands->MapAction(
		Commands.ShowAssetSettings,
		FExecuteAction::CreateLambda([this]()
		{
			return EditorController->ShowAssetDetails();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() ->bool
		{
			const UTLLRN_IKRigDefinition* Asset = EditorController->AssetController->GetAsset();
			return EditorController->IsObjectInDetailsView(Asset);	
		}));
}

void FTLLRN_IKRigEditorToolkit::ExtendToolbar()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	AddToolbarExtender(ToolbarExtender);

	ToolbarExtender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        GetToolkitCommands(),
        FToolBarExtensionDelegate::CreateSP(this, &FTLLRN_IKRigEditorToolkit::FillToolbar)
    );
}

void FTLLRN_IKRigEditorToolkit::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection("Reset");
	{
		ToolbarBuilder.AddToolBarButton(
			FTLLRN_IKRigCommands::Get().Reset,
			NAME_None,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(),"Icons.Refresh"));

		ToolbarBuilder.AddSeparator();

		ToolbarBuilder.AddToolBarButton(
			FTLLRN_IKRigCommands::Get().AutoRetargetChains,
			NAME_None,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FTLLRN_IKRigEditorStyle::Get().GetStyleSetName(),"TLLRN_IKRig.AutoRetarget"));

		ToolbarBuilder.AddToolBarButton(
			FTLLRN_IKRigCommands::Get().AutoSetupFBIK,
			NAME_None,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FTLLRN_IKRigEditorStyle::Get().GetStyleSetName(),"TLLRN_IKRig.AutoIK"));
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddWidget(SNew(SSpacer), NAME_None, true, HAlign_Right);

	ToolbarBuilder.BeginSection("Show Settings");
	{
		ToolbarBuilder.AddToolBarButton(
		FTLLRN_IKRigCommands::Get().ShowAssetSettings,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FTLLRN_IKRigEditorStyle::Get().GetStyleSetName(),"TLLRN_IKRig.AssetSettings"));
	}
	ToolbarBuilder.EndSection();
}

FName FTLLRN_IKRigEditorToolkit::GetToolkitFName() const
{
	return FName("TLLRN_IKRigEditor");
}

FText FTLLRN_IKRigEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("TLLRN_IKRigEditorAppLabel", "IK Rig Editor");
}

FText FTLLRN_IKRigEditorToolkit::GetToolkitName() const
{
	return FText::FromString(EditorController->AssetController->GetAsset()->GetName());
}

FLinearColor FTLLRN_IKRigEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FTLLRN_IKRigEditorToolkit::GetWorldCentricTabPrefix() const
{
	return TEXT("TLLRN_IKRigEditor");
}

void FTLLRN_IKRigEditorToolkit::Tick(float DeltaTime)
{
	// forces viewport to always update, even when mouse pressed down in other tabs
	GetPersonaToolkit()->GetPreviewScene()->InvalidateViews();
}

TStatId FTLLRN_IKRigEditorToolkit::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FTLLRN_IKRigEditorToolkit, STATGROUP_Tickables);
}

void FTLLRN_IKRigEditorToolkit::HandleOnPreviewSceneSettingsCustomized(IDetailLayoutBuilder& DetailBuilder) const
{
	DetailBuilder.HideCategory("Additional Meshes");
	DetailBuilder.HideCategory("Physics");
	DetailBuilder.HideCategory("Mesh");
	DetailBuilder.HideCategory("Animation Blueprint");
}

void FTLLRN_IKRigEditorToolkit::PostUndo(bool bSuccess)
{
	FTLLRN_ScopedReinitializeIKRig Reinitialize(EditorController->AssetController);
}

void FTLLRN_IKRigEditorToolkit::PostRedo(bool bSuccess)
{
	FTLLRN_ScopedReinitializeIKRig Reinitialize(EditorController->AssetController);
}

void FTLLRN_IKRigEditorToolkit::HandlePreviewSceneCreated(const TSharedRef<IPersonaPreviewScene>& InPersonaPreviewScene)
{
	AAnimationEditorPreviewActor* Actor = InPersonaPreviewScene->GetWorld()->SpawnActor<AAnimationEditorPreviewActor>(AAnimationEditorPreviewActor::StaticClass(), FTransform::Identity);
	Actor->SetFlags(RF_Transient);
	InPersonaPreviewScene->SetActor(Actor);
	
	// create the preview skeletal mesh component
	EditorController->SkelMeshComponent = NewObject<UDebugSkelMeshComponent>(Actor);
	// turn off default bone rendering (we do our own in the IK Rig editor)
	EditorController->SkelMeshComponent->SkeletonDrawMode = ESkeletonDrawMode::Hidden;

	// setup and apply an anim instance to the skeletal mesh component
	UTLLRN_IKRigAnimInstance* AnimInstance = NewObject<UTLLRN_IKRigAnimInstance>(EditorController->SkelMeshComponent, TEXT("IKRigAnimScriptInstance"));
	EditorController->AnimInstance = AnimInstance;
	AnimInstance->SetIKRigAsset(EditorController->AssetController->GetAsset());

	// must set the animation mode to "AnimationCustomMode" to prevent USkeletalMeshComponent::InitAnim() from
	// replacing the custom ik rig anim instance with a generic preview anim instance.
	EditorController->SkelMeshComponent->PreviewInstance = AnimInstance;
    EditorController->SkelMeshComponent->SetAnimationMode(EAnimationMode::AnimationCustomMode);
    
    // must call AddComponent() BEFORE assigning the mesh to prevent auto-assignment of a default anim instance
    InPersonaPreviewScene->AddComponent(EditorController->SkelMeshComponent, FTransform::Identity);

	// apply mesh component to the preview scene
	InPersonaPreviewScene->SetPreviewMeshComponent(EditorController->SkelMeshComponent);
	InPersonaPreviewScene->SetAllowMeshHitProxies(false);
	InPersonaPreviewScene->SetAdditionalMeshesSelectable(false);
	EditorController->SkelMeshComponent->bSelectable = false;
	
	// set the skeletal mesh on the component
    // NOTE: this must be done AFTER setting the PreviewInstance so that it assigns it as the main anim instance
    USkeletalMesh* Mesh = EditorController->AssetController->GetSkeletalMesh();
    InPersonaPreviewScene->SetPreviewMesh(Mesh);
}

void FTLLRN_IKRigEditorToolkit::HandleDetailsCreated(const TSharedRef<class IDetailsView>& InDetailsView) const
{
	EditorController->SetDetailsView(InDetailsView);
}

void FTLLRN_IKRigEditorToolkit::HandleViewportCreated(const TSharedRef<IPersonaViewport>& InViewport)
{
	// register callbacks to allow the asset to store the Bone Size viewport setting
	FEditorViewportClient& ViewportClient = InViewport->GetViewportClient();
	if (FAnimationViewportClient* AnimViewportClient = static_cast<FAnimationViewportClient*>(&ViewportClient))
	{
		AnimViewportClient->OnSetBoneSize.BindLambda([this](float InBoneSize)
		{
			if (UTLLRN_IKRigDefinition* Asset = EditorController->AssetController->GetAsset())
			{
				Asset->Modify();
				Asset->BoneSize = InBoneSize;
			}
		});
		
		AnimViewportClient->OnGetBoneSize.BindLambda([this]() -> float
		{
			if (const UTLLRN_IKRigDefinition* Asset = EditorController->AssetController->GetAsset())
			{
				return Asset->BoneSize;
			}

			return 1.0f;
		});
	}

	// highlight viewport when processor disabled
	auto GetBorderColorAndOpacity = [this]()
	{
		// no processor or processor not initialized
		const UTLLRN_IKRigProcessor* Processor = EditorController.Get().GetTLLRN_IKRigProcessor();
		if (!Processor || !Processor->IsInitialized() )
		{
			return FLinearColor::Red;
		}

		// highlight viewport if warnings
		const TArray<FText>& Warnings = Processor->Log.GetWarnings();
		if (!Warnings.IsEmpty())
		{
			return FLinearColor::Yellow;
		}

		return FLinearColor::Transparent;
	};

	InViewport->AddOverlayWidget(
		SNew(SBorder)
		.BorderImage(FTLLRN_IKRigEditorStyle::Get().GetBrush("TLLRN_IKRig.Viewport.Border"))
		.BorderBackgroundColor_Lambda(GetBorderColorAndOpacity)
		.Visibility(EVisibility::HitTestInvisible)
		.Padding(0.0f)
		.ShowEffectWhenDisabled(false)
	);
}

#undef LOCTEXT_NAMESPACE
