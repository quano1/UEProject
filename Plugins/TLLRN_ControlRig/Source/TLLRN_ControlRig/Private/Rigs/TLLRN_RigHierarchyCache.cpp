// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigHierarchyCache.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "TLLRN_ModularRigModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigHierarchyCache)

const FTLLRN_RigElementKey& FTLLRN_CachedTLLRN_RigElement::GetResolvedKey() const
{
	if(Element)
	{
		return Element->GetKey();
	}
	static FTLLRN_RigElementKey InvalidKey;
	return InvalidKey;
}

bool FTLLRN_CachedTLLRN_RigElement::UpdateCache(const UTLLRN_RigHierarchy* InHierarchy)
{
	if(InHierarchy)
	{
		if(!IsValid() || InHierarchy->GetTopologyVersionHash() != ContainerVersion || Element != InHierarchy->Get(Index))
		{
			return UpdateCache(GetKey(), InHierarchy);
		}
		return IsValid();
	}
	return false;
}

bool FTLLRN_CachedTLLRN_RigElement::UpdateCache(const FTLLRN_RigElementKey& InKey, const UTLLRN_RigHierarchy* InHierarchy)
{
	if(InHierarchy)
	{
		if(!IsValid() || !IsIdentical(InKey, InHierarchy) || Element != InHierarchy->Get(Index))
		{
			// have to create a copy since Reset below
			// potentially resets the InKey as well.
			const FTLLRN_RigElementKey KeyToResolve = InKey;

			// first try to resolve with the known index.
			// this happens a lot - where the topology version has
			// increased - but the known item is still valid
			if(InHierarchy->IsValidIndex(Index))
			{
				if(const FTLLRN_RigBaseElement* PreviousElement = InHierarchy->Get(Index))
				{
					if(PreviousElement->GetKey() == KeyToResolve)
					{
						Key = KeyToResolve;
						Element = PreviousElement;
						ContainerVersion = InHierarchy->GetTopologyVersionHash();
						return IsValid();
					}
				}
			}

			int32 Idx = InHierarchy->GetIndex(KeyToResolve);
			if(Idx != INDEX_NONE)
			{
				Key = KeyToResolve;
				Index = (uint16)Idx;
				Element = InHierarchy->Get(Index);
			}
			else
			{
				Reset();
				Key = KeyToResolve;
			}

			ContainerVersion = InHierarchy->GetTopologyVersionHash();
		}
		return IsValid();
	}
	return false;
}

bool FTLLRN_CachedTLLRN_RigElement::IsIdentical(const FTLLRN_RigElementKey& InKey, const UTLLRN_RigHierarchy* InHierarchy)
{
	return InKey == Key && InHierarchy->GetTopologyVersionHash() == ContainerVersion;
}

FTLLRN_RigElementKeyRedirector::FTLLRN_RigElementKeyRedirector(const TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& InMap, const UTLLRN_RigHierarchy* InHierarchy)
{
	check(InHierarchy);
	InternalKeyToExternalKey.Reserve(InMap.Num());
	ExternalKeys.Reserve(InMap.Num());

	Hash = 0;
	for(const TPair<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& Pair : InMap)
	{
		check(Pair.Key.IsValid());
		Add(Pair.Key, Pair.Value, InHierarchy);
	}
}

FTLLRN_RigElementKeyRedirector::FTLLRN_RigElementKeyRedirector(const FTLLRN_RigElementKeyRedirector& InOther, const UTLLRN_RigHierarchy* InHierarchy)
{
	check(InHierarchy);
	InternalKeyToExternalKey.Reserve(InOther.InternalKeyToExternalKey.Num());
	ExternalKeys.Reserve(InOther.ExternalKeys.Num());

	Hash = 0;
	for(const TPair<FTLLRN_RigElementKey, FTLLRN_CachedTLLRN_RigElement>& Pair : InOther.InternalKeyToExternalKey)
	{
		check(Pair.Key.IsValid());
		Add(Pair.Key, Pair.Value.GetKey(), InHierarchy);
	}
}

FTLLRN_RigElementKeyRedirector::FTLLRN_RigElementKeyRedirector(const FTLLRN_ModularRigConnections& InOther, const UTLLRN_RigHierarchy* InHierarchy)
{
	check(InHierarchy);
	InternalKeyToExternalKey.Reserve(InOther.Num());
	ExternalKeys.Reserve(InOther.Num());

	Hash = 0;
	for(const FTLLRN_ModularRigSingleConnection& Connection : InOther)
	{
		check(Connection.Connector.IsValid());
		check(Connection.Target.IsValid());
		Add(Connection.Connector, Connection.Target, InHierarchy);
	}
}

const FTLLRN_RigElementKey* FTLLRN_RigElementKeyRedirector::FindReverse(const FTLLRN_RigElementKey& InKey) const
{
	for(const TPair<FTLLRN_RigElementKey, FTLLRN_CachedTLLRN_RigElement>& Pair : InternalKeyToExternalKey)
	{
		if(Pair.Value.GetKey() == InKey)
		{
			return &Pair.Key;
		}
	}
	return nullptr;
}

void FTLLRN_RigElementKeyRedirector::Add(const FTLLRN_RigElementKey& InSource, const FTLLRN_RigElementKey& InTarget, const UTLLRN_RigHierarchy* InHierarchy)
{
	if(!InSource.IsValid() || InSource == InTarget)
	{
		return;
	}

	if (InTarget.IsValid())
	{
		InternalKeyToExternalKey.Add(InSource, FTLLRN_CachedTLLRN_RigElement(InTarget, InHierarchy, true));
	}
	ExternalKeys.Add(InSource, InTarget);
	Hash = HashCombine(Hash, HashCombine(GetTypeHash(InSource), GetTypeHash(InTarget)));
}

