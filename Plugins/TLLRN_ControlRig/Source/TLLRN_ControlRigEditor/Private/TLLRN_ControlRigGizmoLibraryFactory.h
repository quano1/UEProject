// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "TLLRN_ControlRigGizmoLibrary.h"
#include "TLLRN_ControlRigGizmoLibraryFactory.generated.h"

UCLASS(MinimalAPI, HideCategories=Object)
class UTLLRN_ControlRigShapeLibraryFactory : public UFactory
{
	GENERATED_BODY()

public:
	UTLLRN_ControlRigShapeLibraryFactory();

	// UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
};

