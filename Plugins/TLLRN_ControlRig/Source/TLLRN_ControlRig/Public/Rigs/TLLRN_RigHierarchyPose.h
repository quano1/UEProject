// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigHierarchyCache.h"
#include "TLLRN_RigHierarchyPose.generated.h"

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigPoseElement
{
public:

	GENERATED_BODY()

	FTLLRN_RigPoseElement();

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement Index;

	UPROPERTY()
	FTransform GlobalTransform;

	UPROPERTY()
	FTransform LocalTransform;

	UPROPERTY()
	FVector PreferredEulerAngle;

	UPROPERTY()
	FTLLRN_RigElementKey ActiveParent;

	UPROPERTY()
	float CurveValue;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigPose
{
public:

	GENERATED_BODY()

	FTLLRN_RigPose()
	: HierarchyTopologyVersion(INDEX_NONE)
	, PoseHash(INDEX_NONE)
	, CachedPoseHash(INDEX_NONE)
	{
	}

	void Reset() { Elements.Reset(); }

	int32 Num() const { return Elements.Num(); }
	bool IsValidIndex(int32 InIndex) const { return Elements.IsValidIndex(InIndex); }

	int32 GetIndex(const FTLLRN_RigElementKey& InKey) const
	{
		if(CachedPoseHash != PoseHash)
		{
			KeyToIndex.Reset();
			CachedPoseHash = PoseHash;
		}

		if(const int32* IndexPtr = KeyToIndex.Find(InKey))
		{
			return *IndexPtr;
		}
		
		for(int32 Index=0;Index<Elements.Num();Index++)
		{
			const FTLLRN_RigElementKey& Key = Elements[Index].Index.GetKey();

			KeyToIndex.Add(Key, Index);
			if(Key == InKey)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	bool Contains(const FTLLRN_RigElementKey& InKey) const
	{
		return GetIndex(InKey) != INDEX_NONE;
	}

	const FTLLRN_RigPoseElement& operator[](int32 InIndex) const { return Elements[InIndex]; }
	FTLLRN_RigPoseElement& operator[](int32 InIndex) { return Elements[InIndex]; }
	TArray<FTLLRN_RigPoseElement>::RangedForIteratorType      begin()       { return Elements.begin(); }
	TArray<FTLLRN_RigPoseElement>::RangedForConstIteratorType begin() const { return Elements.begin(); }
	TArray<FTLLRN_RigPoseElement>::RangedForIteratorType      end()         { return Elements.end();   }
	TArray<FTLLRN_RigPoseElement>::RangedForConstIteratorType end() const   { return Elements.end();   }

	UPROPERTY()
	TArray<FTLLRN_RigPoseElement> Elements;

	UPROPERTY()
	int32 HierarchyTopologyVersion;

	UPROPERTY()
	int32 PoseHash;

private:
	
	mutable int32 CachedPoseHash;
	mutable TMap<FTLLRN_RigElementKey, int32> KeyToIndex;
};
