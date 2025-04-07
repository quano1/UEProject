// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Engine/Blueprint.h"
#include "Factories/Factory.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigBlueprintFactory.generated.h"

UCLASS(HideCategories=Object)
class TLLRN_CONTROLRIGEDITOR_API UTLLRN_ControlRigBlueprintFactory : public UFactory
{
	GENERATED_BODY()

public:
	UTLLRN_ControlRigBlueprintFactory();

	// The parent class of the created blueprint
	UPROPERTY(EditAnywhere, Category="TLLRN Control Rig Factory", meta=(AllowAbstract = ""))
	TSubclassOf<UTLLRN_ControlRig> ParentClass;

	// UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	/**
	 * Create a new control rig asset within the contents space of the project.
	 * @param InDesiredPackagePath The package path to use for the control rig asset
	 * @param bTLLRN_ModularRig If true the rig will be created as a modular rig
	 */
	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig")
	static UTLLRN_ControlRigBlueprint* CreateNewTLLRN_ControlRigAsset(const FString& InDesiredPackagePath, const bool bTLLRN_ModularRig = false);

	/**
	 * Create a new control rig asset within the contents space of the project
	 * based on a skeletal mesh or skeleton object.
	 * @param InSelectedObject The SkeletalMesh / Skeleton object to base the control rig asset on
	 * @param bTLLRN_ModularRig If true the rig will be created as a modular rig
	 */
	UFUNCTION(BlueprintCallable, Category = "TLLRN Control Rig")
	static UTLLRN_ControlRigBlueprint* CreateTLLRN_ControlRigFromSkeletalMeshOrSkeleton(UObject* InSelectedObject, const bool bTLLRN_ModularRig = false);
};

