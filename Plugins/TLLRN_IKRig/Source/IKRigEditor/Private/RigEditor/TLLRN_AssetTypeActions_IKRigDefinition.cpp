// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_AssetTypeActions_IKRigDefinition.h"
#include "Rig/TLLRN_IKRigDefinition.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "ContentBrowserMenuContexts.h"
#include "RigEditor/TLLRN_IKRigDefinitionFactory.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"
#include "Engine/SkeletalMesh.h"
#include "ToolMenus.h"
#include "AssetRegistry/AssetData.h"

#define LOCTEXT_NAMESPACE "TLLRN_AssetTypeActions"


UClass* FTLLRN_AssetTypeActions_IKRigDefinition::GetSupportedClass() const
{
	return UTLLRN_IKRigDefinition::StaticClass();
}

UThumbnailInfo* FTLLRN_AssetTypeActions_IKRigDefinition::GetThumbnailInfo(UObject* Asset) const
{
	UTLLRN_IKRigDefinition* IKRig = CastChecked<UTLLRN_IKRigDefinition>(Asset);
	return NewObject<USceneThumbnailInfo>(IKRig, NAME_None, RF_Transactional);
}

void FTLLRN_AssetTypeActions_IKRigDefinition::ExtendSkeletalMeshMenuToMakeIKRig()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.SkeletalMesh.CreateSkeletalMeshSubmenu");
	if (Menu == nullptr)
	{
		return;
	}

	FToolMenuSection& Section = Menu->AddSection("IKRig", LOCTEXT("IKRigSectionName", "TLLRN_IK Rig"));
	Section.AddDynamicEntry("TLLRN_CreateIKRig", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
	{
		UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
		if (Context)
		{
			if (Context->SelectedAssets.Num() > 0)
			{
				InSection.AddMenuEntry(
					"TLLRN_CreateIKRig",
					LOCTEXT("CreateIKRig", "TLLRN_IK Rig"),
					LOCTEXT("CreateIKRig_ToolTip", "Creates an IK rig for this skeletal mesh."),
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
							CreateNewIKRigFromSkeletalMesh(SelectedObject);
						}
					})
				);
			}
		}
	}));
}

void FTLLRN_AssetTypeActions_IKRigDefinition::CreateNewIKRigFromSkeletalMesh(UObject* InSelectedObject)
{
	// validate skeletal mesh
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(InSelectedObject);
	if(!SkeletalMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateNewIKRigFromSkeletalMesh: Provided object has to be a SkeletalMesh."));
		return;
	}

	// remove the standard "SK_" prefix which is standard for these asset types (convenience)
	FString AssetName = InSelectedObject->GetName();
	if (AssetName.Len() > 3)
	{
		AssetName.RemoveFromStart("SK_");	
	}

	// create unique package and asset names
	FString UniquePackageName;
	FString UniqueAssetName = FString::Printf(TEXT("IK_%s"), *AssetName);
	
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
	
	// create the new IK Rig asset
	UTLLRN_IKRigDefinitionFactory* Factory = NewObject<UTLLRN_IKRigDefinitionFactory>();
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(*UniqueAssetName, *UniquePackageName, nullptr, Factory);
	const UTLLRN_IKRigDefinition* IKRig = Cast<UTLLRN_IKRigDefinition>(NewAsset);

	// assign skeletal mesh to the ik rig
	const UTLLRN_IKRigController* Controller = UTLLRN_IKRigController::GetController(IKRig);
	Controller->SetSkeletalMesh(SkeletalMesh);
}

void FTLLRN_AssetTypeActions_IKRigDefinition::OpenAssetEditor(
	const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
    
    for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
    {
    	if (UTLLRN_IKRigDefinition* Asset = Cast<UTLLRN_IKRigDefinition>(*ObjIt))
    	{
    		TSharedRef<FTLLRN_IKRigEditorToolkit> NewEditor(new FTLLRN_IKRigEditorToolkit());
    		NewEditor->InitAssetEditor(Mode, EditWithinLevelEditor, Asset);
    	}
    }
}

const TArray<FText>& FTLLRN_AssetTypeActions_IKRigDefinition::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AnimRetargetingSubMenu", "Retargeting")
	};
	return SubMenus;
}

#undef LOCTEXT_NAMESPACE
