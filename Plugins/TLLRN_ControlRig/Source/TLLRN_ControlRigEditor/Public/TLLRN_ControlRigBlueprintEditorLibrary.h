// Copyright Epic Games, Inc. All Rights Reserved.

/**
 *
 * This thumbnail renderer displays a given Control Rig
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "RigVMEditorBlueprintLibrary.h"
#include "TLLRN_ControlRigBlueprintEditorLibrary.generated.h"

UENUM(BlueprintType)
enum class ECastToTLLRN_ControlRigBlueprintCases : uint8
{
	CastSucceeded,
	CastFailed
};

UCLASS(BlueprintType, meta=(ScriptName="TLLRN_ControlRigBlueprintLibrary"))
class TLLRN_CONTROLRIGEDITOR_API UTLLRN_ControlRigBlueprintEditorLibrary : public URigVMEditorBlueprintLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig Blueprint", Meta = (ExpandEnumAsExecs = "Branches"))
	static void CastToTLLRN_ControlRigBlueprint(
		UObject* Object, 
		ECastToTLLRN_ControlRigBlueprintCases& Branches,
		UTLLRN_ControlRigBlueprint*& AsTLLRN_ControlRigBlueprint);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig Blueprint")
	static void SetPreviewMesh(UTLLRN_ControlRigBlueprint* InRigBlueprint, USkeletalMesh* PreviewMesh, bool bMarkAsDirty = true);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TLLRN Control Rig Blueprint")
	static USkeletalMesh* GetPreviewMesh(UTLLRN_ControlRigBlueprint* InRigBlueprint);

	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig Blueprint")
	static void RequestTLLRN_ControlRigInit(UTLLRN_ControlRigBlueprint* InRigBlueprint);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TLLRN Control Rig Blueprint")
	static TArray<UTLLRN_ControlRigBlueprint*> GetCurrentlyOpenRigBlueprints();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TLLRN Control Rig Blueprint")
	static TArray<UStruct*> GetAvailableTLLRN_RigUnits();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TLLRN Control Rig Blueprint")
	static UTLLRN_RigHierarchy* GetHierarchy(UTLLRN_ControlRigBlueprint* InRigBlueprint);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TLLRN Control Rig Blueprint")
	static UTLLRN_RigHierarchyController* GetHierarchyController(UTLLRN_ControlRigBlueprint* InRigBlueprint);

	UFUNCTION(BlueprintCallable, BlueprintCallable, Category = "TLLRN Control Rig Blueprint")
	static void SetupAllEditorMenus();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TLLRN Control Rig Blueprint")
	static TArray<FTLLRN_RigModuleDescription> GetAvailableTLLRN_RigModules();
};

