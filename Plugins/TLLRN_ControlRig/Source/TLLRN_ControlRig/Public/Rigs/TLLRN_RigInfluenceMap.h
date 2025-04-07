// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigInfluenceMap.generated.h"

USTRUCT(BlueprintType)
struct FTLLRN_RigInfluenceEntry
{
	GENERATED_BODY()

	FTLLRN_RigInfluenceEntry()
		: Source()
		, AffectedList()
	{
	}

	const FTLLRN_RigElementKey& GetSource() const { return Source; }

	int32 Num() const { return AffectedList.Num(); }
	const FTLLRN_RigElementKey& operator[](int32 InIndex) const { return AffectedList[InIndex]; }
	FTLLRN_RigElementKey& operator[](int32 InIndex) { return AffectedList[InIndex]; }

	TArray<FTLLRN_RigElementKey>::RangedForIteratorType      begin() { return AffectedList.begin(); }
	TArray<FTLLRN_RigElementKey>::RangedForConstIteratorType begin() const { return AffectedList.begin(); }
	TArray<FTLLRN_RigElementKey>::RangedForIteratorType      end() { return AffectedList.end(); }
	TArray<FTLLRN_RigElementKey>::RangedForConstIteratorType end() const { return AffectedList.end(); }

	int32 AddUnique(const FTLLRN_RigElementKey& InKey)
	{
		return AffectedList.AddUnique(InKey);
	}

	void Remove(const FTLLRN_RigElementKey& InKey)
	{
		AffectedList.Remove(InKey);
	}

	bool Contains(const FTLLRN_RigElementKey& InKey) const
	{
		return AffectedList.Contains(InKey);
	}

	bool Merge(const FTLLRN_RigInfluenceEntry& Other)
	{
		if(Other.Source != Source)
		{
			return false;
		}

		for(const FTLLRN_RigElementKey& OtherAffected : Other)
		{
			AddUnique(OtherAffected);
		}

		return true;
	}

	void OnKeyRemoved(const FTLLRN_RigElementKey& InKey);

	void OnKeyRenamed(const FTLLRN_RigElementKey& InOldKey, const FTLLRN_RigElementKey& InNewKey);

protected:

	UPROPERTY()
	FTLLRN_RigElementKey Source;

	UPROPERTY()
	TArray<FTLLRN_RigElementKey> AffectedList;

	friend struct FTLLRN_RigInfluenceMap;
};

USTRUCT()
struct FTLLRN_RigInfluenceEntryModifier
{
	GENERATED_BODY()

	FTLLRN_RigInfluenceEntryModifier()
		: AffectedList()
	{
	}

	UPROPERTY(EditAnywhere, Category = "Inversion")
	TArray<FTLLRN_RigElementKey> AffectedList;

	int32 Num() const { return AffectedList.Num(); }
	const FTLLRN_RigElementKey& operator[](int32 InIndex) const { return AffectedList[InIndex]; }
	FTLLRN_RigElementKey& operator[](int32 InIndex) { return AffectedList[InIndex]; }

	TArray<FTLLRN_RigElementKey>::RangedForIteratorType      begin() { return AffectedList.begin(); }
	TArray<FTLLRN_RigElementKey>::RangedForConstIteratorType begin() const { return AffectedList.begin(); }
	TArray<FTLLRN_RigElementKey>::RangedForIteratorType      end() { return AffectedList.end(); }
	TArray<FTLLRN_RigElementKey>::RangedForConstIteratorType end() const { return AffectedList.end(); }
};


USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigInfluenceMap
{
	GENERATED_BODY()

public:

	FTLLRN_RigInfluenceMap()
	:EventName(NAME_None)
	{}

	const FName& GetEventName() const { return EventName; }

	int32 Num() const { return Entries.Num(); }
	const FTLLRN_RigInfluenceEntry& operator[](int32 InIndex) const { return Entries[InIndex]; }
	FTLLRN_RigInfluenceEntry& operator[](int32 InIndex) { return Entries[InIndex]; }
	const FTLLRN_RigInfluenceEntry& operator[](const FTLLRN_RigElementKey& InKey) const { return Entries[GetIndex(InKey)]; }
	FTLLRN_RigInfluenceEntry& operator[](const FTLLRN_RigElementKey& InKey) { return FindOrAdd(InKey); }

	TArray<FTLLRN_RigInfluenceEntry>::RangedForIteratorType      begin()       { return Entries.begin(); }
	TArray<FTLLRN_RigInfluenceEntry>::RangedForConstIteratorType begin() const { return Entries.begin(); }
	TArray<FTLLRN_RigInfluenceEntry>::RangedForIteratorType      end()         { return Entries.end();   }
	TArray<FTLLRN_RigInfluenceEntry>::RangedForConstIteratorType end() const   { return Entries.end();   }

	FTLLRN_RigInfluenceEntry& FindOrAdd(const FTLLRN_RigElementKey& InKey);

	const FTLLRN_RigInfluenceEntry* Find(const FTLLRN_RigElementKey& InKey) const;

	void Remove(const FTLLRN_RigElementKey& InKey);

	bool Contains(const FTLLRN_RigElementKey& InKey) const
	{
		return GetIndex(InKey) != INDEX_NONE;
	}

	int32 GetIndex(const FTLLRN_RigElementKey& InKey) const
	{
		const int32* Index = KeyToIndex.Find(InKey);
		if (Index)
		{
			return *Index;
		}
		return INDEX_NONE;
	}

	FTLLRN_RigInfluenceMap Inverse() const;

	bool Merge(const FTLLRN_RigInfluenceMap& Other);

	FTLLRN_RigInfluenceEntryModifier GetEntryModifier(const FTLLRN_RigElementKey& InKey) const;

	void SetEntryModifier(const FTLLRN_RigElementKey& InKey, const FTLLRN_RigInfluenceEntryModifier& InModifier);

	void OnKeyRemoved(const FTLLRN_RigElementKey& InKey);

	void OnKeyRenamed(const FTLLRN_RigElementKey& InOldKey, const FTLLRN_RigElementKey& InNewKey);

protected:

	bool Merge(const FTLLRN_RigInfluenceMap& Other, bool bIgnoreEventName);

	UPROPERTY()
	FName EventName;

	UPROPERTY()
	TArray<FTLLRN_RigInfluenceEntry> Entries;

	UPROPERTY()
	TMap<FTLLRN_RigElementKey, int32> KeyToIndex;

	friend struct FTLLRN_RigInfluenceMapPerEvent;
	friend class UTLLRN_ControlRig;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigInfluenceMapPerEvent
{
	GENERATED_BODY()

public:

	int32 Num() const { return Maps.Num(); }
	const FTLLRN_RigInfluenceMap& operator[](int32 InIndex) const { return Maps[InIndex]; }
	FTLLRN_RigInfluenceMap& operator[](int32 InIndex) { return Maps[InIndex]; }
	const FTLLRN_RigInfluenceMap& operator[](const FName& InEventName) const { return Maps[GetIndex(InEventName)]; }
	FTLLRN_RigInfluenceMap& operator[](const FName& InEventName) { return FindOrAdd(InEventName); }

	TArray<FTLLRN_RigInfluenceMap>::RangedForIteratorType      begin()       { return Maps.begin(); }
	TArray<FTLLRN_RigInfluenceMap>::RangedForConstIteratorType begin() const { return Maps.begin(); }
	TArray<FTLLRN_RigInfluenceMap>::RangedForIteratorType      end()         { return Maps.end();   }
	TArray<FTLLRN_RigInfluenceMap>::RangedForConstIteratorType end() const   { return Maps.end();   }

	FTLLRN_RigInfluenceMap& FindOrAdd(const FName& InEventName);

	const FTLLRN_RigInfluenceMap* Find(const FName& InEventName) const;

	void Remove(const FName& InEventName);

	bool Contains(const FName& InEventName) const
	{
		return GetIndex(InEventName) != INDEX_NONE;
	}

	int32 GetIndex(const FName& InEventName) const
	{
		const int32* Index = EventToIndex.Find(InEventName);
		if (Index)
		{
			return *Index;
		}
		return INDEX_NONE;
	}

	FTLLRN_RigInfluenceMapPerEvent Inverse() const;

	bool Merge(const FTLLRN_RigInfluenceMapPerEvent& Other);

	FTLLRN_RigInfluenceEntryModifier GetEntryModifier(const FName& InEventName, const FTLLRN_RigElementKey& InKey) const;

	void SetEntryModifier(const FName& InEventName, const FTLLRN_RigElementKey& InKey, const FTLLRN_RigInfluenceEntryModifier& InModifier);

	void OnKeyRemoved(const FTLLRN_RigElementKey& InKey);

	void OnKeyRenamed(const FTLLRN_RigElementKey& InOldKey, const FTLLRN_RigElementKey& InNewKey);

protected:

	UPROPERTY()
	TArray<FTLLRN_RigInfluenceMap> Maps;

	UPROPERTY()
	TMap<FName, int32> EventToIndex;
};

