// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigInfluenceMap.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigInfluenceMap)

////////////////////////////////////////////////////////////////////////////////
// TLLRN_RigInfluenceEntry
////////////////////////////////////////////////////////////////////////////////

void FTLLRN_RigInfluenceEntry::OnKeyRemoved(const FTLLRN_RigElementKey& InKey)
{
	AffectedList.Remove(InKey);
}

void FTLLRN_RigInfluenceEntry::OnKeyRenamed(const FTLLRN_RigElementKey& InOldKey, const FTLLRN_RigElementKey& InNewKey)
{
	for (FTLLRN_RigElementKey& AffectedKey : AffectedList)
	{
		if (AffectedKey == InOldKey)
		{
			AffectedKey = InNewKey;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// TLLRN_RigInfluenceMap
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigInfluenceEntry& FTLLRN_RigInfluenceMap::FindOrAdd(const FTLLRN_RigElementKey& InKey)
{
	int32 Index = GetIndex(InKey);
	if(Index == INDEX_NONE)
	{
		FTLLRN_RigInfluenceEntry NewEntry;
		NewEntry.Source = InKey;
		Index = Entries.Add(NewEntry);
		KeyToIndex.Add(InKey, Index);
	}
	return Entries[Index];
}

const FTLLRN_RigInfluenceEntry* FTLLRN_RigInfluenceMap::Find(const FTLLRN_RigElementKey& InKey) const
{
	int32 Index = GetIndex(InKey);
	if(Index == INDEX_NONE)
	{
		return nullptr;
	}
	return &Entries[Index];
}

void FTLLRN_RigInfluenceMap::Remove(const FTLLRN_RigElementKey& InKey)
{
	int32 Index = GetIndex(InKey);
	if(Index == INDEX_NONE)
	{
		return;
	}

	Entries.RemoveAt(Index);
	for(TPair<FTLLRN_RigElementKey, int32>& Pair : KeyToIndex)
	{
		if(Pair.Value > Index)
		{
			Pair.Value--;
		}
	}
}

FTLLRN_RigInfluenceMap FTLLRN_RigInfluenceMap::Inverse() const
{
	FTLLRN_RigInfluenceMap Inverted;
	Inverted.EventName = EventName;

	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++)
	{
		const FTLLRN_RigInfluenceEntry& Entry = Entries[EntryIndex];
		for (int32 AffectedIndex = 0; AffectedIndex < Entry.Num(); AffectedIndex++)
		{
			Inverted.FindOrAdd(Entry[AffectedIndex]).AddUnique(Entry.Source);
		}
	}
	return Inverted;
}

bool FTLLRN_RigInfluenceMap::Merge(const FTLLRN_RigInfluenceMap& Other)
{
	return Merge(Other, false);
}

FTLLRN_RigInfluenceEntryModifier FTLLRN_RigInfluenceMap::GetEntryModifier(const FTLLRN_RigElementKey& InKey) const
{
	FTLLRN_RigInfluenceEntryModifier Modifier;
	if (const FTLLRN_RigInfluenceEntry* EntryPtr = Find(InKey))
	{
		const FTLLRN_RigInfluenceEntry& Entry = *EntryPtr;
		for (const FTLLRN_RigElementKey& Affected : Entry)
		{
			Modifier.AffectedList.Add(Affected);
		}
	}
	return Modifier;
}

void FTLLRN_RigInfluenceMap::SetEntryModifier(const FTLLRN_RigElementKey& InKey, const FTLLRN_RigInfluenceEntryModifier& InModifier)
{
	FTLLRN_RigInfluenceEntry& Entry = FindOrAdd(InKey);
	Entry.AffectedList.Reset();

	for (const FTLLRN_RigElementKey& AffectedKey : InModifier)
	{
		Entry.AddUnique(AffectedKey);
	}
}

void FTLLRN_RigInfluenceMap::OnKeyRemoved(const FTLLRN_RigElementKey& InKey)
{
	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++)
	{
		FTLLRN_RigInfluenceEntry& Entry = Entries[EntryIndex];
		if (Entry.Source != InKey)
		{
			Entry.OnKeyRemoved(InKey);
		}
	}
	Remove(InKey);
}

void FTLLRN_RigInfluenceMap::OnKeyRenamed(const FTLLRN_RigElementKey& InOldKey, const FTLLRN_RigElementKey& InNewKey)
{
	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++)
	{
		FTLLRN_RigInfluenceEntry& Entry = Entries[EntryIndex];
		if (Entry.Source != InOldKey)
		{
			Entry.OnKeyRenamed(InOldKey, InNewKey);
		}
	}

	int32 IndexToRename = GetIndex(InOldKey);
	if (IndexToRename != INDEX_NONE)
	{
		Entries[IndexToRename].Source = InNewKey;
		KeyToIndex.Remove(InOldKey);
		KeyToIndex.Add(InNewKey, IndexToRename);
	}
}

bool FTLLRN_RigInfluenceMap::Merge(const FTLLRN_RigInfluenceMap& Other, bool bIgnoreEventName)
{
	if(!bIgnoreEventName && (Other.EventName != EventName))
	{
		return false;
	}

	FTLLRN_RigInfluenceMap Temp = *this;
	for(const FTLLRN_RigInfluenceEntry& OtherEntry : Other)
	{
		FTLLRN_RigInfluenceEntry& Entry = Temp.FindOrAdd(OtherEntry.Source);
		if(!Entry.Merge(OtherEntry))
		{
			return false;
		}
	}

	*this = Temp;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigInfluenceMapPerEvent
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigInfluenceMap& FTLLRN_RigInfluenceMapPerEvent::FindOrAdd(const FName& InEventName)
{
	int32 Index = GetIndex(InEventName);
	if(Index == INDEX_NONE)
	{
		FTLLRN_RigInfluenceMap NewMap;
		NewMap.EventName = InEventName;
		Index = Maps.Add(NewMap);
		EventToIndex.Add(InEventName, Index);
	}
	return Maps[Index];
}

const FTLLRN_RigInfluenceMap* FTLLRN_RigInfluenceMapPerEvent::Find(const FName& InEventName) const
{
	int32 Index = GetIndex(InEventName);
	if(Index == INDEX_NONE)
	{
		return nullptr;
	}
	return &Maps[Index];
}

void FTLLRN_RigInfluenceMapPerEvent::Remove(const FName& InEventName)
{
	int32 Index = GetIndex(InEventName);
	if(Index == INDEX_NONE)
	{
		return;
	}

	Maps.RemoveAt(Index);
	for(TPair<FName, int32>& Pair : EventToIndex)
	{
		if(Pair.Value > Index)
		{
			Pair.Value--;
		}
	}
}

FTLLRN_RigInfluenceMapPerEvent FTLLRN_RigInfluenceMapPerEvent::Inverse() const
{
	FTLLRN_RigInfluenceMapPerEvent Inverted;
	for (int32 MapIndex = 0; MapIndex < Maps.Num(); MapIndex++)
	{
		const FTLLRN_RigInfluenceMap& Map = Maps[MapIndex];
		FTLLRN_RigInfluenceMap& InvertedMap = Inverted.FindOrAdd(Map.GetEventName());
		InvertedMap = Map.Inverse();
	}
	return Inverted;
}

bool FTLLRN_RigInfluenceMapPerEvent::Merge(const FTLLRN_RigInfluenceMapPerEvent& Other)
{
	FTLLRN_RigInfluenceMapPerEvent Temp = *this;
	for(const FTLLRN_RigInfluenceMap& OtherMap : Other)
	{
		FTLLRN_RigInfluenceMap& Map = Temp.FindOrAdd(OtherMap.EventName);
		if(!Map.Merge(OtherMap))
		{
			return false;
		}
	}

	*this = Temp;
	return true;
}

FTLLRN_RigInfluenceEntryModifier FTLLRN_RigInfluenceMapPerEvent::GetEntryModifier(const FName& InEventName, const FTLLRN_RigElementKey& InKey) const
{
	if (const FTLLRN_RigInfluenceMap* MapPtr = Find(InEventName))
	{
		return MapPtr->GetEntryModifier(InKey);
	}
	return FTLLRN_RigInfluenceEntryModifier();
}

void FTLLRN_RigInfluenceMapPerEvent::SetEntryModifier(const FName& InEventName, const FTLLRN_RigElementKey& InKey, const FTLLRN_RigInfluenceEntryModifier& InModifier)
{
	FindOrAdd(InEventName).SetEntryModifier(InKey, InModifier);
}

void FTLLRN_RigInfluenceMapPerEvent::OnKeyRemoved(const FTLLRN_RigElementKey& InKey)
{
	for (FTLLRN_RigInfluenceMap& Map : Maps)
	{
		Map.OnKeyRemoved(InKey);
	}
}

void FTLLRN_RigInfluenceMapPerEvent::OnKeyRenamed(const FTLLRN_RigElementKey& InOldKey, const FTLLRN_RigElementKey& InNewKey)
{
	for (FTLLRN_RigInfluenceMap& Map : Maps)
	{
		Map.OnKeyRenamed(InOldKey, InNewKey);
	}
}
