// Copyright Epic Games, Inc. All Rights Reserved.

#include "PropertySets/TLLRN_PolygroupLayersProperties.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Polygroups/PolygroupUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_PolygroupLayersProperties)

using namespace UE::Geometry;

void UTLLRN_PolygroupLayersProperties::InitializeGroupLayers(const FDynamicMesh3* Mesh)
{
	GroupLayersList.Reset();
	GroupLayersList.Add(TEXT("Default"));		// always have standard group
	if (Mesh->Attributes())
	{
		for (int32 k = 0; k < Mesh->Attributes()->NumPolygroupLayers(); k++)
		{
			FName Name = Mesh->Attributes()->GetPolygroupLayer(k)->GetName();
			GroupLayersList.Add(Name.ToString());
		}
	}

	if (GroupLayersList.Contains(ActiveGroupLayer.ToString()) == false)		// discard restored value if it doesn't apply
	{
		ActiveGroupLayer = FName(GroupLayersList[0]);
	}
}

void UTLLRN_PolygroupLayersProperties::InitializeGroupLayers(const TSet<FName>& LayerNames)
{
	GroupLayersList.Reset();
	GroupLayersList.Add(TEXT("Default"));		// always have standard group
	for (const FName& Name : LayerNames)
	{
		GroupLayersList.Add(Name.ToString());
	}

	if (GroupLayersList.Contains(ActiveGroupLayer.ToString()) == false)		// discard restored value if it doesn't apply
	{
		ActiveGroupLayer = FName(GroupLayersList[0]);
	}
}


bool UTLLRN_PolygroupLayersProperties::HasSelectedPolygroup() const
{
	return ActiveGroupLayer != FName(GroupLayersList[0]);
}


void UTLLRN_PolygroupLayersProperties::SetSelectedFromPolygroupIndex(int32 Index)
{
	if (Index < 0)
	{
		ActiveGroupLayer = FName(GroupLayersList[0]);
	}
	else
	{
		ActiveGroupLayer = FName(GroupLayersList[Index+1]);
	}
}


UE::Geometry::FPolygroupLayer UTLLRN_PolygroupLayersProperties::GetSelectedLayer(const FDynamicMesh3& FromMesh)
{
	if (HasSelectedPolygroup() == false)
	{
		return FPolygroupLayer::Default();
	}
	else
	{
		int32 Index = UE::Geometry::FindPolygroupLayerIndexByName(FromMesh, ActiveGroupLayer);
		return (Index >= 0) ? FPolygroupLayer::Layer(Index) : FPolygroupLayer::Default();
	}
}
