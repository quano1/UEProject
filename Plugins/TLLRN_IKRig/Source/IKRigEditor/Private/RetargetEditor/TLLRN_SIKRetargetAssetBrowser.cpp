﻿// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_SIKRetargetAssetBrowser.h"

#include "SPositiveActionButton.h"
#include "ContentBrowserDataSource.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/PoseAsset.h"

#include "RetargetEditor/TLLRN_IKRetargetBatchOperation.h"
#include "RetargetEditor/TLLRN_IKRetargetEditorController.h"
#include "RetargetEditor/TLLRN_SRetargetAnimAssetsWindow.h"
#include "Retargeter/TLLRN_IKRetargeter.h"
#include "Retargeter/TLLRN_IKRetargetProcessor.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "Animation/Skeleton.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRetargeterAssetBrowser"

void TLLRN_SIKRetargetAssetBrowser::Construct(
	const FArguments& InArgs,
	TSharedRef<FTLLRN_IKRetargetEditorController> InEditorController)
{
	EditorController = InEditorController;
	EditorController.Pin()->SetAssetBrowserView(SharedThis(this));
	
	ChildSlot
    [
        SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .AutoHeight()
        [
			SNew(SPositiveActionButton)
			.IsEnabled(this, &TLLRN_SIKRetargetAssetBrowser::IsExportButtonEnabled)
			.Icon(FAppStyle::Get().GetBrush("Icons.Save"))
			.Text(LOCTEXT("ExportButtonLabel", "Export Selected Animations"))
			.ToolTipText(LOCTEXT("ExportButtonToolTip", "Generate new retargeted sequence assets on target skeletal mesh (uses current retargeting configuration)."))
			.OnClicked(this, &TLLRN_SIKRetargetAssetBrowser::OnExportButtonClicked)
        ]        

		+SVerticalBox::Slot()
		[
			SAssignNew(AssetBrowserBox, SBox)
		]
		
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SPositiveActionButton)
			.IsEnabled(this, &TLLRN_SIKRetargetAssetBrowser::IsPlayRefPoseEnabled)
			.Icon(FAppStyle::Get().GetBrush("GenericStop"))
			.Text(LOCTEXT("PlayRefPoseLabel", "Play Ref Pose"))
			.ToolTipText(LOCTEXT("PlayRefPoseTooltip", "Stop playback and set source to reference pose."))
			.OnClicked(this, &TLLRN_SIKRetargetAssetBrowser::OnPlayRefPoseClicked)
		]
    ];

	RefreshView();
}

void TLLRN_SIKRetargetAssetBrowser::RefreshView()
{
	FAssetPickerConfig AssetPickerConfig;
	
	// assign "referencer" asset for project filtering
    if (EditorController.IsValid())
    {
    	const TObjectPtr<UObject> Referencer = EditorController.Pin()->AssetController->GetAsset();
    	AssetPickerConfig.AdditionalReferencingAssets.Add(FAssetData(Referencer));
    }

	// setup filtering
	AssetPickerConfig.Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.ClassPaths.Add(UAnimMontage::StaticClass()->GetClassPathName());
	AssetPickerConfig.Filter.ClassPaths.Add(UPoseAsset::StaticClass()->GetClassPathName());
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Column;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowTypeInColumnView = true;
	AssetPickerConfig.HiddenColumnNames.Add(ContentBrowserItemAttributes::ItemDiskSize.ToString());
	AssetPickerConfig.HiddenColumnNames.Add(ContentBrowserItemAttributes::VirtualizedData.ToString());
	AssetPickerConfig.HiddenColumnNames.Add(TEXT("Class"));
	AssetPickerConfig.HiddenColumnNames.Add(TEXT("RevisionControl"));
	AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &TLLRN_SIKRetargetAssetBrowser::OnShouldFilterAsset);
	AssetPickerConfig.DefaultFilterMenuExpansion = EAssetTypeCategories::Animation;
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetSelected::CreateSP(this, &TLLRN_SIKRetargetAssetBrowser::OnAssetDoubleClicked);
	AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &TLLRN_SIKRetargetAssetBrowser::OnGetAssetContextMenu);
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.bFocusSearchBoxWhenOpened = false;

	// hide all asset registry columns by default (we only really want the name and path)
	UObject* AnimSequenceDefaultObject = UAnimSequence::StaticClass()->GetDefaultObject();
	FAssetRegistryTagsContextData TagsContext(AnimSequenceDefaultObject, EAssetRegistryTagsCaller::Uncategorized);
	AnimSequenceDefaultObject->GetAssetRegistryTags(TagsContext);
	for (const TPair<FName, UObject::FAssetRegistryTag>& TagPair : TagsContext.Tags)
	{
		AssetPickerConfig.HiddenColumnNames.Add(TagPair.Key.ToString());
	}

	// Also hide the type column by default (but allow users to enable it, so don't use bShowTypeInColumnView)
	AssetPickerConfig.HiddenColumnNames.Add(TEXT("Class"));
	AssetPickerConfig.HiddenColumnNames.Add(TEXT("Has Virtualized Data"));

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	AssetBrowserBox->SetContent(ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig));
}

TSharedPtr<SWidget> TLLRN_SIKRetargetAssetBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const
{
	if (SelectedAssets.Num() <= 0)
	{
		return nullptr;
	}

	UObject* SelectedAsset = SelectedAssets[0].GetAsset();
	if (SelectedAsset == nullptr)
	{
		return nullptr;
	}
	
	FMenuBuilder MenuBuilder(true, MakeShared<FUICommandList>());

	MenuBuilder.BeginSection(TEXT("Asset"), LOCTEXT("AssetSectionLabel", "Asset"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Browse", "Browse to Asset"),
			LOCTEXT("BrowseTooltip", "Browses to the associated asset and selects it in the most recently used Content Browser (summoning one if necessary)"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SystemWideCommands.FindInContentBrowser.Small"),
			FUIAction(
				FExecuteAction::CreateLambda([SelectedAsset] ()
				{
					if (SelectedAsset)
					{
						const TArray<FAssetData>& Assets = { SelectedAsset };
						const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
						ContentBrowserModule.Get().SyncBrowserToAssets(Assets);
					}
				}),
				FCanExecuteAction::CreateLambda([] () { return true; })
			)
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

FReply TLLRN_SIKRetargetAssetBrowser::OnExportButtonClicked()
{
	const FTLLRN_IKRetargetEditorController* Controller = EditorController.Pin().Get();
	if (!Controller)
	{
		checkNoEntry();
		return FReply::Handled();
	}
	
	// assemble the context for the assets we want to batch duplicate/retarget
	FTLLRN_IKRetargetBatchOperationContext BatchContext;
	BatchContext.NameRule.FolderPath = PrevBatchOutputPath;
	BatchContext.SourceMesh = Controller->GetSkeletalMesh(ETLLRN_RetargetSourceOrTarget::Source);
	BatchContext.TargetMesh = Controller->GetSkeletalMesh(ETLLRN_RetargetSourceOrTarget::Target);
	BatchContext.IKRetargetAsset = Controller->AssetController->GetAsset();
	
	// get the export path from user
	const TSharedRef<SBatchExportPathDialog> PathDialog = SNew(SBatchExportPathDialog).BatchContext(&BatchContext).ExportRetargetAssets(false);
	if(PathDialog->ShowModal() == EAppReturnType::Cancel)
	{
		return FReply::Handled();
	}

	// store path for next time
	PrevBatchOutputPath = BatchContext.NameRule.FolderPath;

	// get the export options from user
	const TSharedRef<SBatchExportOptionsDialog> OptionsDialog = SNew(SBatchExportOptionsDialog).BatchContext(&BatchContext);
	if(OptionsDialog->ShowModal() == EAppReturnType::Cancel)
	{
		return FReply::Handled();
	}

	// add selected assets to dup/retarget
	TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();
	for (const FAssetData& Asset : SelectedAssets)
	{
		if (UObject* Object = Asset.GetAsset())
		{
			BatchContext.AssetsToRetarget.Add(Object);
		}
	}

	// run the batch retarget
	const TStrongObjectPtr<UTLLRN_IKRetargetBatchOperation> BatchOperation(NewObject<UTLLRN_IKRetargetBatchOperation>());
	BatchOperation->RunRetarget(BatchContext);
	
	return FReply::Handled();
}

bool TLLRN_SIKRetargetAssetBrowser::IsExportButtonEnabled() const
{
	if (!EditorController.Pin().IsValid())
	{
		return false; // editor in bad state
	}

	const UTLLRN_IKRetargetProcessor* Processor = EditorController.Pin()->GetRetargetProcessor();
	if (!Processor)
	{
		return false; // no retargeter running
	}

	if (!Processor->IsInitialized())
	{
		return false; // retargeter not loaded and valid
	}

	TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();
	if (SelectedAssets.IsEmpty())
	{
		return false; // nothing selected
	}

	return true;
}

FReply TLLRN_SIKRetargetAssetBrowser::OnPlayRefPoseClicked()
{
	FTLLRN_IKRetargetEditorController* Controller = EditorController.Pin().Get();
	if (!Controller)
	{
		checkNoEntry();
		return FReply::Handled();
	}

	Controller->SetRetargeterMode(ETLLRN_RetargeterOutputMode::RunRetarget);
	Controller->PlaybackManager->StopPlayback();
	return FReply::Handled();
}

bool TLLRN_SIKRetargetAssetBrowser::IsPlayRefPoseEnabled() const
{
	FTLLRN_IKRetargetEditorController* Controller = EditorController.Pin().Get();
	if (!Controller)
	{
		checkNoEntry();
		return false;
	}
	
	return Controller->GetSkeletalMesh(ETLLRN_RetargetSourceOrTarget::Source) != nullptr;
}

void TLLRN_SIKRetargetAssetBrowser::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	if (!AssetData.GetAsset())
	{
		return;
	}
	
	UAnimationAsset* NewAnimationAsset = Cast<UAnimationAsset>(AssetData.GetAsset());
	if (NewAnimationAsset && EditorController.Pin().IsValid())
	{
		EditorController.Pin()->PlaybackManager->PlayAnimationAsset(NewAnimationAsset);
	}
}

bool TLLRN_SIKRetargetAssetBrowser::OnShouldFilterAsset(const struct FAssetData& AssetData)
{
	// is this an animation asset?
	if (!AssetData.IsInstanceOf(UAnimationAsset::StaticClass()))
	{
		return true;
	}
	
	// controller setup
	const FTLLRN_IKRetargetEditorController* Controller = EditorController.Pin().Get();
	if (!Controller)
	{
		return true;
	}

	// get source skeleton
	const USkeleton* DesiredSkeleton = Controller->GetSkeleton(ETLLRN_RetargetSourceOrTarget::Source);
	if (!DesiredSkeleton)
	{
		return true;
	}

	return !DesiredSkeleton->IsCompatibleForEditor(AssetData);
}

#undef LOCTEXT_NAMESPACE
