// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigBlueprintEditorLibrary.h"
#include "Editor/STLLRN_RigHierarchy.h"
#include "Editor/STLLRN_ModularRigModel.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "RigVMModel/RigVMPin.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigBlueprintEditorLibrary)

FAutoConsoleCommand FCmdTLLRN_ControlRigLoadAllAssets
(
	TEXT("TLLRN_ControlRig.LoadAllAssets"),
	TEXT("Loads all control rig assets."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UTLLRN_ControlRigBlueprintEditorLibrary::LoadAssetsByClass(UTLLRN_ControlRigBlueprint::StaticClass());
	})
);

void UTLLRN_ControlRigBlueprintEditorLibrary::CastToTLLRN_ControlRigBlueprint(
	UObject* Object,
	ECastToTLLRN_ControlRigBlueprintCases& Branches,
	UTLLRN_ControlRigBlueprint*& AsTLLRN_ControlRigBlueprint)
{
	AsTLLRN_ControlRigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(Object);
	Branches = AsTLLRN_ControlRigBlueprint == nullptr ? 
		ECastToTLLRN_ControlRigBlueprintCases::CastFailed : 
		ECastToTLLRN_ControlRigBlueprintCases::CastSucceeded;
}

void UTLLRN_ControlRigBlueprintEditorLibrary::SetPreviewMesh(UTLLRN_ControlRigBlueprint* InRigBlueprint, USkeletalMesh* PreviewMesh, bool bMarkAsDirty)
{
	if(InRigBlueprint == nullptr)
	{
		return;
	}
	InRigBlueprint->SetPreviewMesh(PreviewMesh, bMarkAsDirty);
}

USkeletalMesh* UTLLRN_ControlRigBlueprintEditorLibrary::GetPreviewMesh(UTLLRN_ControlRigBlueprint* InRigBlueprint)
{
	if(InRigBlueprint == nullptr)
	{
		return nullptr;
	}
	return InRigBlueprint->GetPreviewMesh();
}

void UTLLRN_ControlRigBlueprintEditorLibrary::RequestTLLRN_ControlRigInit(UTLLRN_ControlRigBlueprint* InRigBlueprint)
{
	if(InRigBlueprint == nullptr)
	{
		return;
	}
	InRigBlueprint->RequestRigVMInit();
}

TArray<UTLLRN_ControlRigBlueprint*> UTLLRN_ControlRigBlueprintEditorLibrary::GetCurrentlyOpenRigBlueprints()
{
	return UTLLRN_ControlRigBlueprint::GetCurrentlyOpenRigBlueprints();
}

TArray<UStruct*> UTLLRN_ControlRigBlueprintEditorLibrary::GetAvailableTLLRN_RigUnits()
{
	UTLLRN_ControlRigBlueprint* CDO = CastChecked<UTLLRN_ControlRigBlueprint>(UTLLRN_ControlRigBlueprint::StaticClass()->GetDefaultObject());
	return CDO->GetAvailableRigVMStructs();
}

UTLLRN_RigHierarchy* UTLLRN_ControlRigBlueprintEditorLibrary::GetHierarchy(UTLLRN_ControlRigBlueprint* InRigBlueprint)
{
	if(InRigBlueprint == nullptr)
	{
		return nullptr;
	}
	return InRigBlueprint->Hierarchy;
}

UTLLRN_RigHierarchyController* UTLLRN_ControlRigBlueprintEditorLibrary::GetHierarchyController(UTLLRN_ControlRigBlueprint* InRigBlueprint)
{
	if(InRigBlueprint == nullptr)
	{
		return nullptr;
	}
	return InRigBlueprint->GetHierarchyController();
}

void UTLLRN_ControlRigBlueprintEditorLibrary::SetupAllEditorMenus()
{
	STLLRN_RigHierarchy::CreateContextMenu();
	STLLRN_RigHierarchy::CreateDragDropMenu();
	STLLRN_ModularRigModel::CreateContextMenu();
}

TArray<FTLLRN_RigModuleDescription> UTLLRN_ControlRigBlueprintEditorLibrary::GetAvailableTLLRN_RigModules()
{
	TArray<FTLLRN_RigModuleDescription> ModuleDescriptions;
	
	// Load the asset registry module
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Collect a full list of assets with the specified class
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByClass(UTLLRN_ControlRigBlueprint::StaticClass()->GetClassPathName(), AssetDataList, true);

	for(const FAssetData& AssetData : AssetDataList)
	{
		static const FName ModuleSettingsName = GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, TLLRN_RigModuleSettings);
		if(AssetData.FindTag(ModuleSettingsName))
		{
			const FString TagValue = AssetData.GetTagValueRef<FString>(ModuleSettingsName);
			if(TagValue.IsEmpty())
			{
				FTLLRN_RigModuleDescription ModuleDescription;
				ModuleDescription.Path = AssetData.ToSoftObjectPath();
				
				FRigVMPinDefaultValueImportErrorContext ErrorPipe;
				FTLLRN_RigModuleSettings::StaticStruct()->ImportText(*TagValue, &ModuleDescription.Settings, nullptr, PPF_None, &ErrorPipe, FString());
				if(ErrorPipe.NumErrors == 0)
				{
					if(ModuleDescription.Settings.IsValidModule())
					{
						ModuleDescriptions.Add(ModuleDescription);
					}
				}
			}
		}
	}
	
	return ModuleDescriptions;
}
