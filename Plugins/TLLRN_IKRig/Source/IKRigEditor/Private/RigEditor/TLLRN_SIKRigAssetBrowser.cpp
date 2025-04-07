// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_SIKRigAssetBrowser.h"

#include "AnimPreviewInstance.h"
#include "ContentBrowserDataSource.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/PoseAsset.h"
#include "RigEditor/TLLRN_IKRigController.h"
#include "RigEditor/TLLRN_IKRigEditorController.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/AssetRegistryTagsContext.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRigAssetBrowser"

void TLLRN_SIKRigAssetBrowser::Construct(
	const FArguments& InArgs,
	TSharedRef<FTLLRN_IKRigEditorController> InEditorController)
{
	EditorController = InEditorController;
	EditorController.Pin()->AssetBrowserView = SharedThis(this);
	
	ChildSlot
    [
        SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
			SAssignNew(AssetBrowserBox, SBox)
		]
    ];

	RefreshView();
}

void TLLRN_SIKRigAssetBrowser::RefreshView()
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
	AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &TLLRN_SIKRigAssetBrowser::OnShouldFilterAsset);
	AssetPickerConfig.DefaultFilterMenuExpansion = EAssetTypeCategories::Animation;
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetSelected::CreateSP(this, &TLLRN_SIKRigAssetBrowser::OnAssetDoubleClicked);
	AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &TLLRN_SIKRigAssetBrowser::OnGetAssetContextMenu);
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

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	AssetBrowserBox->SetContent(ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig));
}

TSharedPtr<SWidget> TLLRN_SIKRigAssetBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const
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

void TLLRN_SIKRigAssetBrowser::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	if (!AssetData.GetAsset())
	{
		return;
	}
	
	UAnimationAsset* NewAnimationAsset = Cast<UAnimationAsset>(AssetData.GetAsset());
	if (NewAnimationAsset && EditorController.Pin().IsValid())
	{
		EditorController.Pin()->PlayAnimationAsset(NewAnimationAsset);
	}
}

bool TLLRN_SIKRigAssetBrowser::OnShouldFilterAsset(const struct FAssetData& AssetData)
{
	// is this an animation asset?
	if (!AssetData.IsInstanceOf(UAnimationAsset::StaticClass()))
	{
		return true;
	}
	
	// controller setup
	FTLLRN_IKRigEditorController* Controller = EditorController.Pin().Get();
	if (!Controller)
	{
		return true;
	}

	// get skeletal mesh
	USkeletalMesh* SkeletalMesh = Controller->AssetController->GetSkeletalMesh();
	if (!SkeletalMesh)
	{
		return true;
	}
	
	// get skeleton
	USkeleton* DesiredSkeleton = SkeletalMesh->GetSkeleton();
	if (!DesiredSkeleton)
	{
		return true;
	}

	return !DesiredSkeleton->IsCompatibleForEditor(AssetData);
}

#undef LOCTEXT_NAMESPACE
