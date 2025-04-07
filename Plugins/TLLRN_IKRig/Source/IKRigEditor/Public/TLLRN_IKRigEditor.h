// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class TLLRN_IKRIGEDITOR_API FTLLRN_IKRigEditor : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;

private:
	TSharedPtr<class FAssetTypeActions_AnimationAssetRetarget> RetargetAnimationAssetAction;
	TSharedPtr<class FTLLRN_AssetTypeActions_IKRigDefinition> TLLRN_IKRigDefinitionAssetAction;
	TSharedPtr<class FTLLRN_AssetTypeActions_IKRetargeter> TLLRN_IKRetargeterAssetAction;

	TArray<FName> ClassesToUnregisterOnShutdown;
};

DECLARE_LOG_CATEGORY_EXTERN(LogTLLRN_IKRigEditor, Warning, All);
