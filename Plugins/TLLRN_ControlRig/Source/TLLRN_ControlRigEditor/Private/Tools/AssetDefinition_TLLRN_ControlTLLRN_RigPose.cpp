// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetDefinition_TLLRN_ControlTLLRN_RigPose.h"

#include "ContentBrowserModule.h"
#include "ContentBrowserMenuContexts.h"
#include "TLLRN_ControlRig.h"
#include "EditorModeManager.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditMode/STLLRN_ControlRigRenamePoseControls.h"
#include "ToolMenus.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

// Menu Extensions
//--------------------------------------------------------------------

namespace MenuExtension_AnimationAsset
{
	void ExecutePastePose(const FToolMenuContext& InContext)
	{
		const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext);
		TArray<UTLLRN_ControlTLLRN_RigPoseAsset*> TLLRN_ControlTLLRN_RigPoseAssets = Context->LoadSelectedObjects<UTLLRN_ControlTLLRN_RigPoseAsset>();

		FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
		if (!TLLRN_ControlRigEditMode)
		{
			return;
		}
		
		TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> AllSelectedControls;
		TLLRN_ControlRigEditMode->GetAllSelectedControls(AllSelectedControls);
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
		AllSelectedControls.GenerateKeyArray(TLLRN_ControlRigs);
			
		for (UTLLRN_ControlTLLRN_RigPoseAsset* TLLRN_ControlTLLRN_RigPoseAsset : TLLRN_ControlTLLRN_RigPoseAssets)
		{
			for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
			{
				TLLRN_ControlTLLRN_RigPoseAsset->PastePose(TLLRN_ControlRig, false, false);
			}
		}
	}

	void ExecuteSelectControls(const FToolMenuContext& InContext)
	{
		const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext);
		TArray<UTLLRN_ControlTLLRN_RigPoseAsset*> TLLRN_ControlTLLRN_RigPoseAssets = Context->LoadSelectedObjects<UTLLRN_ControlTLLRN_RigPoseAsset>();

		FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
		if (!TLLRN_ControlRigEditMode)
		{
			return;
		}

		TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> AllSelectedControls;
		TLLRN_ControlRigEditMode->GetAllSelectedControls(AllSelectedControls);
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
		AllSelectedControls.GenerateKeyArray(TLLRN_ControlRigs);

		for (UTLLRN_ControlTLLRN_RigPoseAsset* TLLRN_ControlTLLRN_RigPoseAsset : TLLRN_ControlTLLRN_RigPoseAssets)
		{
			for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
			{
				TLLRN_ControlTLLRN_RigPoseAsset->SelectControls(TLLRN_ControlRig);
			}
		}
	}

	void ExecuteUpdatePose(const FToolMenuContext& InContext)
	{
		const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext);
		TArray<UTLLRN_ControlTLLRN_RigPoseAsset*> TLLRN_ControlTLLRN_RigPoseAssets = Context->LoadSelectedObjects<UTLLRN_ControlTLLRN_RigPoseAsset>();

		FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
		if (!TLLRN_ControlRigEditMode)
		{
			return;
		}

		TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> AllSelectedControls;
		TLLRN_ControlRigEditMode->GetAllSelectedControls(AllSelectedControls);
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
		AllSelectedControls.GenerateKeyArray(TLLRN_ControlRigs);

		for (UTLLRN_ControlTLLRN_RigPoseAsset* TLLRN_ControlTLLRN_RigPoseAsset : TLLRN_ControlTLLRN_RigPoseAssets)
		{
			for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
			{
				TLLRN_ControlTLLRN_RigPoseAsset->SavePose(TLLRN_ControlRig, false);
			}
		}
	}

	void ExecuteRenameControls(const FToolMenuContext& InContext)
	{
		const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext);
		TArray<UTLLRN_ControlTLLRN_RigPoseAsset*> TLLRN_ControlTLLRN_RigPoseAssets = Context->LoadSelectedObjects<UTLLRN_ControlTLLRN_RigPoseAsset>();

		FTLLRN_ControlRigRenameControlsDialog::RenameControls(TLLRN_ControlTLLRN_RigPoseAssets);
	}

	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, [] {
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
	{
		FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
		UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UTLLRN_ControlTLLRN_RigPoseAsset::StaticClass());

		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
		Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{
			{
				const TAttribute<FText> Label = LOCTEXT("TLLRN_ControlTLLRN_RigPose_PastePose", "Paste Pose");
				const TAttribute<FText> ToolTip = LOCTEXT("TLLRN_ControlTLLRN_RigPose_PastePoseTooltip", "Paste the selected pose");

				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecutePastePose);
				InSection.AddMenuEntry("TLLRN_ControlTLLRN_RigPose_PastePose", Label, ToolTip, FSlateIcon(), UIAction);
			}
			{
				const TAttribute<FText> Label = LOCTEXT("TLLRN_ControlTLLRN_RigPose_SelectControls", "Select Controls");
				const TAttribute<FText> ToolTip = LOCTEXT("TLLRN_ControlTLLRN_RigPose_SelectControlsTooltip", "Select controls in this pose on active control rig");

				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteSelectControls);
				InSection.AddMenuEntry("TLLRN_ControlTLLRN_RigPose_SelectControls", Label, ToolTip, FSlateIcon(), UIAction);
			}
			{
				const TAttribute<FText> Label = LOCTEXT("TLLRN_ControlTLLRN_RigPose_UpdatePose", "Update Pose");
				const TAttribute<FText> ToolTip = LOCTEXT("TLLRN_ControlTLLRN_RigPose_UpdatePoseTooltip", "Update the pose based upon current control rig pose and selected controls");

				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteUpdatePose);
				InSection.AddMenuEntry("TLLRN_ControlTLLRN_RigPose_UpdatePose", Label, ToolTip, FSlateIcon(), UIAction);
			}
			{
				const TAttribute<FText> Label = LOCTEXT("TLLRN_ControlTLLRN_RigPose_RenameControls", "Rename Controls");
				const TAttribute<FText> ToolTip = LOCTEXT("TLLRN_ControlTLLRN_RigPose_RenameControlsTooltip", "Rename controls on selected poses");

				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteRenameControls);
				InSection.AddMenuEntry("TLLRN_ControlTLLRN_RigPose_RenameControls", Label, ToolTip, FSlateIcon(), UIAction);
			}

		}));
		}));
	});
}

#undef LOCTEXT_NAMESPACE
