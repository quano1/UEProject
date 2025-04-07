// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/AnimSequenceFactory.h"
#include "TLLRN_AnimSequenceConverterFactory.generated.h"

UCLASS(MinimalAPI)
class UTLLRN_AnimSequenceConverterFactory : public UAnimSequenceFactory
{
	GENERATED_BODY()

	// Act just like the regular UAnimSequenceFactory, but with a pre-configured skeleton
	virtual bool ConfigureProperties() override { return true; }
};

