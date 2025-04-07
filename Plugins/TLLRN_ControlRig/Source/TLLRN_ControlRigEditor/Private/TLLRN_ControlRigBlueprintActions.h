// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions/AssetTypeActions_Blueprint.h"
#include "TLLRN_ControlRigBlueprint.h"

class FMenuBuilder;
class UFactory;
class USkeletalMesh;
class AActor;

class FTLLRN_ControlRigBlueprintActions : public FAssetTypeActions_Blueprint
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "TLLRN_ControlRigBlueprintActions", "TLLRN Control Rig"); }
	virtual FColor GetTypeColor() const override { return FColor(140, 116, 0); }
	virtual UClass* GetSupportedClass() const override { return UTLLRN_ControlRigBlueprint::StaticClass(); }
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Animation; }
	virtual const TArray<FText>& GetSubMenus() const override;
	virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& AssetData) const override;
	virtual void PerformAssetDiff(UObject* Asset1, UObject* Asset2, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const override;

	// FAssetTypeActions_Blueprint interface
	virtual UFactory* GetFactoryForBlueprintType(UBlueprint* InBlueprint) const override;
	virtual bool CanCreateNewDerivedBlueprint() const override { return false; };

	static void ExtendSketalMeshToolMenu();

	static UTLLRN_ControlRigBlueprint* CreateNewTLLRN_ControlRigAsset(const FString& InDesiredPackagePath, const bool bTLLRN_ModularRig);
	static UTLLRN_ControlRigBlueprint* CreateTLLRN_ControlRigFromSkeletalMeshOrSkeleton(UObject* InSelectedObject, const bool bTLLRN_ModularRig);

	static USkeletalMesh* GetSkeletalMeshFromTLLRN_ControlRigBlueprint(const FAssetData& InAsset);
	static void PostSpawningSkeletalMeshActor(AActor* InSpawnedActor, UObject* InAsset);
	static void OnSpawnedSkeletalMeshActorChanged(UObject* InObject, FPropertyChangedEvent& InEvent, UObject* InAsset);

	static FDelegateHandle OnSpawnedSkeletalMeshActorChangedHandle;
};
