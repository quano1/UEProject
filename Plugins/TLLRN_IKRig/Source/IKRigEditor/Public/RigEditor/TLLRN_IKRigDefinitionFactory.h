// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "TLLRN_IKRigDefinitionFactory.generated.h"

class SWindow;

UCLASS(BlueprintType, hidecategories=Object)
class TLLRN_IKRIGEDITOR_API UTLLRN_IKRigDefinitionFactory : public UFactory
{
	GENERATED_BODY()

public:

	UTLLRN_IKRigDefinitionFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(
        UClass* Class,
        UObject* InParent,
        FName InName,
        EObjectFlags InFlags,
        UObject* Context,
        FFeedbackContext* Warn) override;
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
	virtual FText GetToolTip() const override;
	virtual FString GetDefaultNewAssetName() const override;
	virtual bool ShouldShowInNewMenu() const override;
	// END UFactory interface
};
