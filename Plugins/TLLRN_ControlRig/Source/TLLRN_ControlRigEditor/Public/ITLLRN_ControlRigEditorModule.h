// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ITLLRN_ControlRigEditor.h"
#include "RigVMEditorModule.h"
#include "Kismet2/StructureEditorUtils.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTLLRN_ControlRigEditor, Log, All);

class IToolkitHost;
class UTLLRN_ControlRigBlueprint;
class UTLLRN_ControlRigGraphNode;
class UTLLRN_ControlRigGraphSchema;
class FConnectionDrawingPolicy;

class TLLRN_CONTROLRIGEDITOR_API FTLLRN_ControlRigClassFilter : public IClassViewerFilter
{
public:
	FTLLRN_ControlRigClassFilter(bool bInCheckSkeleton, bool bInCheckAnimatable, bool bInCheckInversion, USkeleton* InSkeleton);
	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override;
	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override;	

private:
	bool MatchesFilter(const FAssetData& AssetData);

private:
	bool bFilterAssetBySkeleton;
	bool bFilterExposesAnimatableControls;
	bool bFilterInversion;

	USkeleton* Skeleton;
	const IAssetRegistry& AssetRegistry;
};

class ITLLRN_ControlRigEditorModule : public FRigVMEditorModule
{
public:

	static ITLLRN_ControlRigEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked< ITLLRN_ControlRigEditorModule >(TEXT("TLLRN_ControlRigEditor"));
	}

	/**
	 * Creates an instance of a Control Rig editor.
	 *
	 * @param	Mode					Mode that this editor should operate in
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	Blueprint				The blueprint object to start editing.
	 *
	 * @return	Interface to the new Control Rig editor
	 */
	virtual TSharedRef<ITLLRN_ControlRigEditor> CreateTLLRN_ControlRigEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UTLLRN_ControlRigBlueprint* Blueprint) = 0;

};
