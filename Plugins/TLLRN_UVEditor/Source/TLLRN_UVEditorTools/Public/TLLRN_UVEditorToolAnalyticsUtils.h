// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EngineAnalytics.h"
#include "ToolTargets/TLLRN_UVEditorToolMeshInput.h"

namespace UE
{
namespace Geometry
{
	
namespace TLLRN_UVEditorAnalytics
{
	
template <typename EnumType>
FAnalyticsEventAttribute AnalyticsEventAttributeEnum(const FString& AttributeName, EnumType EnumValue)
{
	static_assert(TIsEnum<EnumType>::Value, "");
	const FString EnumValueName = StaticEnum<EnumType>()->GetNameStringByValue(static_cast<int64>(EnumValue));
	return FAnalyticsEventAttribute(AttributeName, EnumValueName);
}

TLLRN_UVEDITORTOOLS_API FString TLLRN_UVEditorAnalyticsEventName(const FString& ToolEventName);

struct TLLRN_UVEDITORTOOLS_API FTargetAnalytics
{
	int64 AllAssetsNumVertices = 0;
	int64 AllAssetsNumTriangles = 0;
	int64 AllAssetsNumUVLayers = 0;
	
	TArray<int32> PerAssetNumVertices;
	TArray<int32> PerAssetNumTriangles;
	TArray<int32> PerAssetNumUVLayers;
	TArray<int32> PerAssetActiveUVChannelIndex;
	TArray<int32> PerAssetActiveUVChannelNumVertices;
	TArray<int32> PerAssetActiveUVChannelNumTriangles;

	void AppendToAttributes(TArray<FAnalyticsEventAttribute>& Attributes, FString Prefix = "") const;
};

TLLRN_UVEDITORTOOLS_API FTargetAnalytics CollectTargetAnalytics(const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& Targets);
	
} // end namespace TLLRN_UVEditorAnalytics
	
} // end namespace Geometry
} // end namespace UE
