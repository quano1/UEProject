// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_AssetTypeActions_IKRetargeter.h"

#include "Animation/AnimationAsset.h"
#include "ContentBrowserMenuContexts.h"
#include "ToolMenus.h"
#include "RetargetEditor/TLLRN_IKRetargetEditor.h"
#include "RetargetEditor/TLLRN_IKRetargetEditorStyle.h"
#include "RetargetEditor/TLLRN_IKRetargetFactory.h"
#include "RetargetEditor/TLLRN_SRetargetAnimAssetsWindow.h"
#include "Retargeter/TLLRN_IKRetargeter.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"

#define LOCTEXT_NAMESPACE "TLLRN_AssetTypeActions"

UClass* FTLLRN_AssetTypeActions_IKRetargeter::GetSupportedClass() const
{
	return UTLLRN_IKRetargeter::StaticClass();
}

void FTLLRN_AssetTypeActions_IKRetargeter::OpenAssetEditor(
	const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
    
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UTLLRN_IKRetargeter* Asset = Cast<UTLLRN_IKRetargeter>(*ObjIt))
		{
			TSharedRef<FTLLRN_IKRetargetEditor> NewEditor(new FTLLRN_IKRetargetEditor());
			NewEditor->InitAssetEditor(Mode, EditWithinLevelEditor, Asset);
		}
	}
}

UThumbnailInfo* FTLLRN_AssetTypeActions_IKRetargeter::GetThumbnailInfo(UObject* Asset) const
{
	UTLLRN_IKRetargeter* TLLRN_IKRetargeter = CastChecked<UTLLRN_IKRetargeter>(Asset);
	return NewObject<USceneThumbnailInfo>(TLLRN_IKRetargeter, NAME_None, RF_Transactional);
}

void FTLLRN_AssetTypeActions_IKRetargeter::ExtendIKRigMenuToMakeRetargeter()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.TLLRN_IKRigDefinition");
	if (Menu == nullptr)
	{
		return;
	}

	FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
	Section.AddDynamicEntry("CreateTLLRN_IKRetargeter", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
	{
		UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
		if (Context)
		{
			if (Context->SelectedAssets.Num() > 0)
			{
				InSection.AddMenuEntry(
					"CreateRetargeter",
					LOCTEXT("CreateTLLRN_IKRetargeter", "Create IK Retargeter"),
					LOCTEXT("CreateTLLRN_IKRetargeter_ToolTip", "Creates an IK Retargeter using this IK Rig as the source."),
					FSlateIcon(FTLLRN_IKRigEditorStyle::Get().GetStyleSetName(), "TLLRN_IKRig", "ClassIcon.TLLRN_IKRigDefinition"),
					FExecuteAction::CreateLambda([InSelectedAssets = Context->SelectedAssets]()
					{
						TArray<UObject*> SelectedObjects;
						SelectedObjects.Reserve(InSelectedAssets.Num());

						for (const FAssetData& Asset : InSelectedAssets)
						{
							if (UObject* LoadedAsset = Asset.GetAsset())
							{
								SelectedObjects.Add(LoadedAsset);
							}
						}

						for (UObject* SelectedObject : SelectedObjects)
						{
							CreateNewTLLRN_IKRetargeterFromIKRig(SelectedObject);
						}
					})
				);
			}
		}
	}));
}

void FTLLRN_AssetTypeActions_IKRetargeter::CreateNewTLLRN_IKRetargeterFromIKRig(UObject* InSelectedObject)
{
	// validate ik rig
	UTLLRN_IKRigDefinition* IKRigAsset = Cast<UTLLRN_IKRigDefinition>(InSelectedObject);
	if(!IKRigAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateNewTLLRN_IKRetargeterFromIKRig: Provided object has to be an IK Rig."));
		return;
	}
	
	// remove the standard "IK_" prefix which is standard for these asset types (convenience)
	FString AssetName = InSelectedObject->GetName();
	if (AssetName.Len() > 3)
	{
		AssetName.RemoveFromStart("IK_");	
	}

	// create unique package and asset names
	FString UniquePackageName;
	FString UniqueAssetName = FString::Printf(TEXT("RTG_%s"), *AssetName);
	
	FString PackagePath = InSelectedObject->GetPathName(); 
	int32 LastSlashPos = INDEX_NONE;
	if (PackagePath.FindLastChar('/', LastSlashPos))
	{
		PackagePath = PackagePath.Left(LastSlashPos);
	}
	
	PackagePath = PackagePath / UniqueAssetName;
	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(PackagePath, TEXT(""), UniquePackageName, UniqueAssetName);
	if (UniquePackageName.EndsWith(UniqueAssetName))
	{
		UniquePackageName = UniquePackageName.LeftChop(UniqueAssetName.Len() + 1);
	}
	
	// create the new IK Retargeter asset
	UTLLRN_IKRetargetFactory* Factory = NewObject<UTLLRN_IKRetargetFactory>();
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(*UniqueAssetName, *UniquePackageName, nullptr, Factory);
	const UTLLRN_IKRetargeter* RetargetAsset = Cast<UTLLRN_IKRetargeter>(NewAsset);

	// assign ik rig to the retargeter
	const UTLLRN_IKRetargeterController* Controller = UTLLRN_IKRetargeterController::GetController(RetargetAsset);
	Controller->SetIKRig(ETLLRN_RetargetSourceOrTarget::Source, IKRigAsset);
}

void FTLLRN_AssetTypeActions_IKRetargeter::ExtendAnimAssetMenusForBatchRetargeting()
{
	static const TArray<FName> MenusToExtend 
	{
		"ContentBrowser.AssetContextMenu.AnimSequence",
		"ContentBrowser.AssetContextMenu.BlendSpace",
		"ContentBrowser.AssetContextMenu.PoseAsset",
		"ContentBrowser.AssetContextMenu.AnimBlueprint",
		"ContentBrowser.AssetContextMenu.AnimMontage"
	};

	for (const FName& MenuName : MenusToExtend)
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(MenuName);
		check(Menu);

		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
		Section.AddMenuEntry(
			"IKRetargetToDifferentSkeleton",
			LOCTEXT("RetargetAnimation_Label", "Retarget Animations"),
			LOCTEXT("RetargetAnimation_ToolTip", "Duplicate and retarget animation assets to a different skeleton. Works on sequences, blendspaces, pose assets, montages and animation blueprints."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCurveEditor.TabIcon"),
			FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& InContext)
			{
				if (const UContentBrowserAssetContextMenuContext* Context = InContext.FindContext<UContentBrowserAssetContextMenuContext>())
				{
					TLLRN_SRetargetAnimAssetsWindow::ShowWindow(Context->LoadSelectedObjects<UObject>());
				}
			})
		);
	}
}

const TArray<FText>& FTLLRN_AssetTypeActions_IKRetargeter::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AnimRetargetingSubMenu", "Retargeting")
	};
	return SubMenus;
}

#undef LOCTEXT_NAMESPACE
