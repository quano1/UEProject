// Copyright Epic Games, Inc. All Rights Reserved.

#include "Properties/TLLRN_MeshUVChannelProperties.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_MeshUVChannelProperties)


void UTLLRN_MeshUVChannelProperties::Initialize(int32 NumUVChannels, bool bInitializeSelection)
{
	UVChannelNamesList.Reset();
	for (int32 k = 0; k < NumUVChannels; ++k)
	{
		UVChannelNamesList.Add(FString::Printf(TEXT("UV %d"), k));
	}
	if (bInitializeSelection)
	{
		UVChannel = (NumUVChannels > 0) ? UVChannelNamesList[0] : TEXT("");
	}
}


const TArray<FString>& UTLLRN_MeshUVChannelProperties::GetUVChannelNamesFunc() const
{
	return UVChannelNamesList;
}


void UTLLRN_MeshUVChannelProperties::Initialize(const FMeshDescription* MeshDescription, bool bInitializeSelection)
{
	TVertexInstanceAttributesConstRef<FVector2f> InstanceUVs =
		MeshDescription->VertexInstanceAttributes().GetAttributesRef<FVector2f>(MeshAttribute::VertexInstance::TextureCoordinate);
	Initialize(InstanceUVs.GetNumChannels(), bInitializeSelection);
}

void UTLLRN_MeshUVChannelProperties::Initialize(const FDynamicMesh3* Mesh, bool bInitializeSelection)
{
	int32 NumUVChannels = Mesh->HasAttributes() ? Mesh->Attributes()->NumUVLayers() : 0;
	Initialize(NumUVChannels, bInitializeSelection);
}



bool UTLLRN_MeshUVChannelProperties::ValidateSelection(bool bUpdateIfInvalid)
{
	int32 FoundIndex = UVChannelNamesList.IndexOfByKey(UVChannel);
	if (FoundIndex == INDEX_NONE)
	{
		if (bUpdateIfInvalid)
		{
			UVChannel = (UVChannelNamesList.Num() > 0) ? UVChannelNamesList[0] : TEXT("");
		}
		return false;
	}
	return true;
}

int32 UTLLRN_MeshUVChannelProperties::GetSelectedChannelIndex(bool bForceToZeroOnFailure)
{
	int32 FoundIndex = UVChannelNamesList.IndexOfByKey(UVChannel);
	if (FoundIndex == INDEX_NONE)
	{
		return (bForceToZeroOnFailure ) ? 0 : -1;
	}
	return FoundIndex;
}
