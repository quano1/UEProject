// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Misc/PackageName.h"
#include "UObject/Object.h"
#include "TLLRN_ControlRig.h"
#include "Tools/TLLRN_ControlTLLRN_RigPoseProjectSettings.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "Components/SkeletalMeshComponent.h"

/**
Create One of the Control Rig Assets, Currently Only support Pose Assets but may add Animation/Selection Sets.
*/
class FTLLRN_ControlRigToolAsset
{
public:
	template< typename Type> static  UObject* SaveAsset(UTLLRN_ControlRig* InTLLRN_ControlRig, const FString& CurrentPath, const FString& InString, bool bUseAllControls)
	{
		if (InTLLRN_ControlRig)
		{
			FString InPackageName = CurrentPath;
			InPackageName += FString("/") + InString;
			if (!FPackageName::IsValidLongPackageName(InPackageName))
			{
				FText ErrorText = FText::Format(NSLOCTEXT("TLLRN_ControlRigToolAsset", "InvalidPathError", "{0} is not a valid asset path."), FText::FromString(InPackageName));
				FNotificationInfo NotifyInfo(ErrorText);
				NotifyInfo.ExpireDuration = 5.0f;
				FSlateNotificationManager::Get().AddNotification(NotifyInfo)->SetCompletionState(SNotificationItem::CS_Fail);
				return nullptr;
			}

			int32 UniqueIndex = 1;
			int32 BasePackageLength = InPackageName.Len();

			// Generate a unique control asset name for this take if there are already assets of the same name
			IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
			TArray<FAssetData> OutAssetData;
			AssetRegistry.GetAssetsByPackageName(*InPackageName, OutAssetData);
			while (OutAssetData.Num() > 0)
			{
				int32 TrimCount = InPackageName.Len() - BasePackageLength;
				if (TrimCount > 0)
				{
					InPackageName.RemoveAt(BasePackageLength, TrimCount, EAllowShrinking::No);
				}

				InPackageName += FString::Printf(TEXT("_%04d"), UniqueIndex++);
				OutAssetData.Empty();
				AssetRegistry.GetAssetsByPackageName(*InPackageName, OutAssetData);
			}

			// Create the asset to record into
			const FString NewAssetName = FPackageName::GetLongPackageAssetName(InPackageName);
			UPackage* NewPackage = CreatePackage(*InPackageName);


			// Create a new Pose From Scracth
			Type* Asset = NewObject<Type>(NewPackage, *NewAssetName, RF_Public | RF_Standalone | RF_Transactional);
			USkeletalMesh* SkelMesh = nullptr;
			if (USkeletalMeshComponent* RigMeshComp = Cast<USkeletalMeshComponent>(InTLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
			{
				SkelMesh = RigMeshComp->GetSkeletalMeshAsset();
			}

			Asset->SavePose(InTLLRN_ControlRig, bUseAllControls);
			Asset->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(Asset);
			return Asset;

		}
		return nullptr;
	}
};



