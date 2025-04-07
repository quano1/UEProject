// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorModule.h"

#include "Features/IModularFeatures.h"
#include "ContentBrowserMenuContexts.h"
#include "EditorModeRegistry.h"
#include "LevelEditorMenuContext.h"
#include "PropertyEditorModule.h"
#include "Selection.h"
#include "TLLRN_UVEditor.h"
#include "TLLRN_UVEditor3DViewportMode.h"
#include "TLLRN_UVEditorCommands.h"
#include "TLLRN_UVEditorMode.h"
#include "TLLRN_UVEditorStyle.h"
#include "TLLRN_UVEditorSubsystem.h"
#include "TLLRN_UVSelectTool.h"

#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FTLLRN_UVEditorModule"

void FTLLRN_UVEditorModule::StartupModule()
{
	TLLRN_UVEditorAssetEditor = MakeShared<UE::Geometry::FTLLRN_UVEditorModularFeature>();

	if (TLLRN_UVEditorAssetEditor.IsValid())
	{
		IModularFeatures::Get().RegisterModularFeature(IUVEditorModularFeature::GetModularFeatureName(), TLLRN_UVEditorAssetEditor.Get());
	}

	FTLLRN_UVEditorStyle::Get(); // Causes the constructor to be called
	FTLLRN_UVEditorCommands::Register();
	UE::Geometry::FTLLRN_UVEditorToolActionCommands::RegisterAllToolActions();

	// Menus need to be registered in a callback to make sure the system is ready for them.
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTLLRN_UVEditorModule::RegisterMenus));

	// Register details view customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	ClassesToUnregisterOnShutdown.Reset();
	// Customizations get registered like this:
	//PropertyModule.RegisterCustomClassLayout(USelectToolActionPropertySet::StaticClass()->GetFName(), 
	//	FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_UVSelectToolActionPropertySetDetails::MakeInstance));
	//ClassesToUnregisterOnShutdown.Add(USelectToolActionPropertySet::StaticClass()->GetFName());
}

void FTLLRN_UVEditorModule::ShutdownModule()
{
	if (TLLRN_UVEditorAssetEditor.IsValid())
	{
		IModularFeatures::Get().UnregisterModularFeature(IUVEditorModularFeature::GetModularFeatureName(), TLLRN_UVEditorAssetEditor.Get());
		TLLRN_UVEditorAssetEditor = nullptr;
	}

	// Clean up menu things
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	UE::Geometry::FTLLRN_UVEditorToolActionCommands::UnregisterAllToolActions();
	FTLLRN_UVEditorCommands::Unregister();

	FEditorModeRegistry::Get().UnregisterMode(UTLLRN_UVEditorMode::EM_TLLRN_UVEditorModeId);
	FEditorModeRegistry::Get().UnregisterMode(UTLLRN_UVEditor3DViewportMode::EM_ModeID);

	// Unregister customizations
	FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");
	if (PropertyEditorModule)
	{
		for (FName ClassName : ClassesToUnregisterOnShutdown)
		{
			PropertyEditorModule->UnregisterCustomClassLayout(ClassName);
		}
	}
}

void FTLLRN_UVEditorModule::RegisterMenus()
{
	// Allows cleanup when module unloads.
	FToolMenuOwnerScoped OwnerScoped(this);

	// Extend the content browser context menu for static meshes and skeletal meshes
	auto AddToContextMenuSection = [this](FToolMenuSection& Section)
	{
		Section.AddDynamicEntry("OpenTLLRN_UVEditor", FNewToolMenuSectionDelegate::CreateLambda(
			[](FToolMenuSection& Section)
			{
				// We'll need to get the target assets out of the context
				if (UContentBrowserAssetContextMenuContext* Context = Section.FindContext<UContentBrowserAssetContextMenuContext>())
				{
					UTLLRN_UVEditorSubsystem* UVSubsystem = GEditor->GetEditorSubsystem<UTLLRN_UVEditorSubsystem>();
					check(UVSubsystem);

					// We are deliberately not using Context->GetSelectedObjects() here to avoid triggering a load from right clicking
					// an asset in the content browser.
					if (UVSubsystem->AreAssetsValidTargets(Context->SelectedAssets))
					{
						TSharedPtr<class FUICommandList> CommandListToBind = MakeShared<FUICommandList>();
						CommandListToBind->MapAction(
							FTLLRN_UVEditorCommands::Get().OpenTLLRN_UVEditor,
							FExecuteAction::CreateWeakLambda(UVSubsystem, [Context, UVSubsystem]()
							{
								// When we actually do want to open the editor, trigger the load to get the objects
								TArray<TObjectPtr<UObject>> AssetsToEdit;
								AssetsToEdit.Append(Context->LoadSelectedObjects<UObject>());

								// If we fail the ensure here, then there must be something that we're failing to check properly
								// in AreAssetsValidTargets that we would need to track down and check in the asset data.
								if (ensure(UVSubsystem->AreObjectsValidTargets(AssetsToEdit)))
								{
									UVSubsystem->StartTLLRN_UVEditor(AssetsToEdit);
								}
							}),
							FCanExecuteAction::CreateWeakLambda(Context, [Context]() { return Context->bCanBeModified; }));

						const TAttribute<FText> ToolTipOverride = Context->bCanBeModified ? TAttribute<FText>() : LOCTEXT("ReadOnlyAssetWarning", "The selected asset(s) are read-only and cannot be edited.");
						Section.AddMenuEntryWithCommandList(FTLLRN_UVEditorCommands::Get().OpenTLLRN_UVEditor, CommandListToBind, TAttribute<FText>(), ToolTipOverride, FSlateIcon(FTLLRN_UVEditorStyle::Get().GetStyleSetName(), "TLLRN_UVEditor.OpenTLLRN_UVEditor"));
					}
				}
			}));
	};
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.StaticMesh");
		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
		AddToContextMenuSection(Section);
	}
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.SkeletalMesh");
		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
		AddToContextMenuSection(Section);
	}

	// Extend the level editor menus

	// Helper struct to collect all necessary inputs for a menu entry.
	struct FMenuEntryParameters
	{
		TArray<TObjectPtr<UObject>> TargetObjects;
		UTLLRN_UVEditorSubsystem* UVSubsystem;
		bool bValidTargets;
	};

	// Sets up all parameters for a menu entry.
	auto SetupMenuEntryParameters = []
	{
		FMenuEntryParameters Result;

		for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
		{
			// We are interested in the (unique) assets backing the actor, or else the actor
			// itself if it is not asset backed (such as UDynamicMesh).
			const AActor* Actor = static_cast<AActor*>(*It);

			// We need to determine if there are components under this actor first,
			// to ensure we generate Component based targets. Otherwise we can use
			// the referenced content inside to generate more direct targets. This
			// is important because otherwise we won't have access to component
			// level data, such as transforms within the level editor, if we actually
			// do want it for tools.

			TInlineComponentArray<UActorComponent*> ActorComponents;
			Actor->GetComponents(ActorComponents);

			if (ActorComponents.Num() > 0)
			{
				Result.TargetObjects.Append(ActorComponents);
			}
			else
			{
				TArray<UObject*> ActorAssets;
				Actor->GetReferencedContentObjects(ActorAssets);
				for (UObject* Asset : ActorAssets)
				{
					Result.TargetObjects.AddUnique(Asset);
				}
			}
		}

		Result.UVSubsystem = GEditor->GetEditorSubsystem<UTLLRN_UVEditorSubsystem>();
		check(Result.UVSubsystem);

		Result.bValidTargets = Result.UVSubsystem->AreObjectsValidTargets(Result.TargetObjects);

		return Result;
	};

	// Adds a menu entry for the given parameters.
	auto AddMenuEntry = [](FToolMenuSection& Section, const FMenuEntryParameters& MenuEntryParameters)
	{
		const bool bValidTargets = MenuEntryParameters.bValidTargets;

		const TSharedPtr<class FUICommandList> CommandListToBind = MakeShared<FUICommandList>();
		CommandListToBind->MapAction(
			FTLLRN_UVEditorCommands::Get().OpenTLLRN_UVEditor,
			FExecuteAction::CreateUObject(MenuEntryParameters.UVSubsystem, &UTLLRN_UVEditorSubsystem::StartTLLRN_UVEditor, MenuEntryParameters.TargetObjects),
			FCanExecuteAction::CreateLambda([bValidTargets]() { return bValidTargets; }));

		Section.AddMenuEntryWithCommandList(FTLLRN_UVEditorCommands::Get().OpenTLLRN_UVEditor, CommandListToBind);
	};

	// Add UV Editor to actor context menu.
	{
		UToolMenu* ActorContextMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContextMenu");
		FToolMenuSection& ActorTypeToolsSection = ActorContextMenu->FindOrAddSection("ActorTypeTools");
		ActorTypeToolsSection.AddDynamicEntry("OpenTLLRN_UVEditor", FNewToolMenuSectionDelegate::CreateLambda(
			                                      [&SetupMenuEntryParameters, &AddMenuEntry](FToolMenuSection& Section)
			                                      {
				                                      const FMenuEntryParameters MenuEntryParameters = SetupMenuEntryParameters();
				                                      if (MenuEntryParameters.bValidTargets)
				                                      {
					                                      AddMenuEntry(Section, MenuEntryParameters);
				                                      }
			                                      }));
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTLLRN_UVEditorModule, TLLRN_UVEditor)