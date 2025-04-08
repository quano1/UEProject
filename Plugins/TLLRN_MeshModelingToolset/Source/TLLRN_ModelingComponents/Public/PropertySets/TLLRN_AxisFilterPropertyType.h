// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_AxisFilterPropertyType.generated.h"


USTRUCT(BlueprintType)
struct TLLRN_MODELINGCOMPONENTS_API FTLLRN_ModelingToolsAxisFilter
{
	GENERATED_BODY()

	/** X Axis */
	UPROPERTY(EditAnywhere, Category = AxisFilters, meta = (DisplayName = "X"))
	bool bAxisX = true;

	/** Y Axis */
	UPROPERTY(EditAnywhere, Category = AxisFilters, meta = (DisplayName = "Y"))
	bool bAxisY = true;

	/** Z Axis */
	UPROPERTY(EditAnywhere, Category = AxisFilters, meta = (DisplayName = "Z"))
	bool bAxisZ = true;


	// helper functions

	bool AnyAxisFiltered() const { return bAxisX == false || bAxisY == false || bAxisZ == false; }
};
