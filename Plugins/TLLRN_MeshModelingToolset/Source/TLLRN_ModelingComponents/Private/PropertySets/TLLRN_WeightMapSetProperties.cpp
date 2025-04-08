// Copyright Epic Games, Inc. All Rights Reserved.

#include "PropertySets/TLLRN_WeightMapSetProperties.h"
#include "WeightMapUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_WeightMapSetProperties)


void UTLLRN_WeightMapSetProperties::InitializeWeightMaps(const TArray<FName>& WeightMapNames)
{
	// populate weight maps list
	WeightMapsList.Add(TEXT("None"));
	for (FName Name : WeightMapNames)
	{
		WeightMapsList.Add(Name.ToString());
	}
	if (WeightMapNames.Contains(WeightMap) == false)		// discard restored value if it doesn't apply
	{
		WeightMap = FName(WeightMapsList[0]);
	}
}


void UTLLRN_WeightMapSetProperties::InitializeFromMesh(const FMeshDescription* Mesh)
{
	TArray<FName> TargetWeightMaps;
	UE::WeightMaps::FindVertexWeightMaps(Mesh, TargetWeightMaps);
	InitializeWeightMaps(TargetWeightMaps);
}

bool UTLLRN_WeightMapSetProperties::HasSelectedWeightMap() const
{
	return WeightMap != FName(WeightMapsList[0]);
}


TArray<FString> UTLLRN_WeightMapSetProperties::GetWeightMapsFunc()
{
	return WeightMapsList;
}

void UTLLRN_WeightMapSetProperties::SetSelectedFromWeightMapIndex(int32 Index)
{
	if (Index < 0)
	{
		WeightMap = FName(WeightMapsList[0]);
	}
	else
	{
		WeightMap = FName(WeightMapsList[Index+1]);
	}
}
