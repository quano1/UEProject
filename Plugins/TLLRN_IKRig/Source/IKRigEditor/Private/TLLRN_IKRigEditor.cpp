// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_IKRigEditor.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "AssetToolsModule.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "EditorModeRegistry.h"
#include "Rig/TLLRN_IKRigDefinition.h"
#include "RigEditor/TLLRN_AssetTypeActions_IKRigDefinition.h"
#include "RetargetEditor/TLLRN_AssetTypeActions_IKRetargeter.h"
#include "RetargetEditor/TLLRN_IKRetargetCommands.h"
#include "RetargetEditor/TLLRN_IKRetargetDefaultMode.h"
#include "RetargetEditor/TLLRN_IKRetargetDetails.h"
#include "RetargetEditor/TLLRN_IKRetargetEditPoseMode.h"
#include "RetargetEditor/TLLRN_IKRetargeterThumbnailRenderer.h"
#include "Retargeter/TLLRN_IKRetargetOps.h"
#include "RigEditor/TLLRN_IKRigCommands.h"
#include "RigEditor/TLLRN_IKRigEditMode.h"
#include "RigEditor/TLLRN_IKRigSkeletonCommands.h"
#include "RigEditor/TLLRN_IKRigDetailCustomizations.h"
#include "RigEditor/TLLRN_IKRigEditorController.h"
#include "RigEditor/TLLRN_IKRigThumbnailRenderer.h"

DEFINE_LOG_CATEGORY(LogTLLRN_IKRigEditor);

IMPLEMENT_MODULE(FTLLRN_IKRigEditor, TLLRN_IKRigEditor)

#define LOCTEXT_NAMESPACE "TLLRN_IKRigEditor"

void FTLLRN_IKRigEditor::StartupModule()
{
	// register commands
	FTLLRN_IKRigCommands::Register();
	FTLLRN_IKRigSkeletonCommands::Register();
	FTLLRN_IKRetargetCommands::Register();
	
	// register custom asset type actions
	IAssetTools& ToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	//
	TLLRN_IKRigDefinitionAssetAction = MakeShareable(new FTLLRN_AssetTypeActions_IKRigDefinition);
	ToolsModule.RegisterAssetTypeActions(TLLRN_IKRigDefinitionAssetAction.ToSharedRef());
	//
	TLLRN_IKRetargeterAssetAction = MakeShareable(new FTLLRN_AssetTypeActions_IKRetargeter);
	ToolsModule.RegisterAssetTypeActions(TLLRN_IKRetargeterAssetAction.ToSharedRef());

	// extend the content browser menu
	FTLLRN_AssetTypeActions_IKRetargeter::ExtendAnimAssetMenusForBatchRetargeting();
	FTLLRN_AssetTypeActions_IKRetargeter::ExtendIKRigMenuToMakeRetargeter();
	FTLLRN_AssetTypeActions_IKRigDefinition::ExtendSkeletalMeshMenuToMakeIKRig();

	// register custom editor modes
	FEditorModeRegistry::Get().RegisterMode<FTLLRN_IKRigEditMode>(FTLLRN_IKRigEditMode::ModeName, LOCTEXT("TLLRN_IKRigEditMode", "IKRig"), FSlateIcon(), false);
	FEditorModeRegistry::Get().RegisterMode<FTLLRN_IKRetargetDefaultMode>(FTLLRN_IKRetargetDefaultMode::ModeName, LOCTEXT("TLLRN_IKRetargetDefaultMode", "IKRetargetDefault"), FSlateIcon(), false);
	FEditorModeRegistry::Get().RegisterMode<FTLLRN_IKRetargetEditPoseMode>(FTLLRN_IKRetargetEditPoseMode::ModeName, LOCTEXT("IKRetargetEditMode", "IKRetargetEditPose"), FSlateIcon(), false);

	// register detail customizations
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// custom IK rig bone details
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_IKRigBoneDetails::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FIKRigGenericDetailCustomization::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_IKRigBoneDetails::StaticClass()->GetFName());
	// custom IK rig goal details
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_IKRigEffectorGoal::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FIKRigGenericDetailCustomization::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_IKRigEffectorGoal::StaticClass()->GetFName());
	// custom retargeter bone details
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_IKRetargetBoneDetails::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FIKRetargetBoneDetailCustomization::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_IKRetargetBoneDetails::StaticClass()->GetFName());
	// custom retargeter chain details
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_RetargetChainSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FRetargetChainSettingsCustomization::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_RetargetChainSettings::StaticClass()->GetFName());
	// custom retargeter root details
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_RetargetRootSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FRetargetRootSettingsCustomization::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_RetargetRootSettings::StaticClass()->GetFName());
	// custom retargeter global details
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_IKRetargetGlobalSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_RetargetGlobalSettingsCustomization::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_IKRetargetGlobalSettings::StaticClass()->GetFName());
	// custom retargeter op stack details
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_RetargetOpStack::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FRetargetOpStackCustomization::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_RetargetOpStack::StaticClass()->GetFName());

	// register a thumbnail renderer for the assets
	UThumbnailManager::Get().RegisterCustomRenderer(UTLLRN_IKRigDefinition::StaticClass(), UTLLRN_IKRigThumbnailRenderer::StaticClass());
	UThumbnailManager::Get().RegisterCustomRenderer(UTLLRN_IKRetargeter::StaticClass(), UTLLRN_IKRetargeterThumbnailRenderer::StaticClass());
}

void FTLLRN_IKRigEditor::ShutdownModule()
{
	FTLLRN_IKRigCommands::Unregister();
	FTLLRN_IKRigSkeletonCommands::Unregister();
	FTLLRN_IKRetargetCommands::Unregister();
	
	FEditorModeRegistry::Get().UnregisterMode(FTLLRN_IKRigEditMode::ModeName);
	FEditorModeRegistry::Get().UnregisterMode(FTLLRN_IKRetargetDefaultMode::ModeName);
	FEditorModeRegistry::Get().UnregisterMode(FTLLRN_IKRetargetEditPoseMode::ModeName);

	// unregister asset actions
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& ToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		
		if (TLLRN_IKRigDefinitionAssetAction.IsValid())
		{
			ToolsModule.UnregisterAssetTypeActions(TLLRN_IKRigDefinitionAssetAction.ToSharedRef());
		}
		
		if (TLLRN_IKRetargeterAssetAction.IsValid())
		{
			ToolsModule.UnregisterAssetTypeActions(TLLRN_IKRetargeterAssetAction.ToSharedRef());
		}
	}

	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UTLLRN_IKRigDefinition::StaticClass());
		UThumbnailManager::Get().UnregisterCustomRenderer(UTLLRN_IKRetargeter::StaticClass());
	}
}

#undef LOCTEXT_NAMESPACE