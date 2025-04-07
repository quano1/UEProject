// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "TLLRN_IKRetargetFactory.generated.h"

class SWindow;

UCLASS(hidecategories=Object)
class TLLRN_IKRIGEDITOR_API UTLLRN_IKRetargetFactory : public UFactory
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<class UTLLRN_IKRigDefinition>	SourceIKRig;

public:

	UTLLRN_IKRetargetFactory();

	// UFactory Interface
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
	virtual FText GetToolTip() const override;
	virtual FString GetDefaultNewAssetName() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
};
