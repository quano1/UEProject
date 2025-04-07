// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigHierarchyElements.h"

#include "TLLRN_RigHierarchyCache.generated.h"

class UTLLRN_RigHierarchy;
struct FTLLRN_ModularRigConnections;

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_CachedTLLRN_RigElement
{
	GENERATED_BODY()

public:

	FTLLRN_CachedTLLRN_RigElement()
		: Key()
		, Index(UINT16_MAX)
		, ContainerVersion(INDEX_NONE)
	{}

	FTLLRN_CachedTLLRN_RigElement(const FTLLRN_RigElementKey& InKey, const UTLLRN_RigHierarchy* InHierarchy, bool bForceStoreKey = false)
		: Key()
		, Index(UINT16_MAX)
		, ContainerVersion(INDEX_NONE)
	{
		UpdateCache(InKey, InHierarchy);
		if(bForceStoreKey)
		{
			Key = InKey;
		}
	}

	bool IsValid() const
	{
		return GetIndex() != INDEX_NONE && Key.IsValid();
	}

	void Reset()
	{
		Key = FTLLRN_RigElementKey();
		Index = UINT16_MAX;
		ContainerVersion = INDEX_NONE;
		Element = nullptr;
	}

	explicit operator bool() const
	{
		return IsValid();
	}

	operator int32() const
	{
		return GetIndex();
	}

	explicit operator FTLLRN_RigElementKey() const
	{
		return Key;
	}

	int32 GetIndex() const
	{
		if(Index == UINT16_MAX)
		{
			return INDEX_NONE;
		}
		return (int32)Index;
	}

	const FTLLRN_RigElementKey& GetKey() const
	{
		return Key;
	}

	const FTLLRN_RigElementKey& GetResolvedKey() const;

	const FTLLRN_RigBaseElement* GetElement() const
	{
		return Element;
	}

	bool UpdateCache(const UTLLRN_RigHierarchy* InHierarchy);

	bool UpdateCache(const FTLLRN_RigElementKey& InKey, const UTLLRN_RigHierarchy* InHierarchy);

	friend uint32 GetTypeHash(const FTLLRN_CachedTLLRN_RigElement& Cache)
	{
		return GetTypeHash(Cache.Key) * 13 + (uint32)Cache.Index;
	}

	bool IsIdentical(const FTLLRN_RigElementKey& InKey, const UTLLRN_RigHierarchy* InHierarchy);

	bool operator ==(const FTLLRN_CachedTLLRN_RigElement& Other) const
	{
		return Index == Other.Index && Key == Other.Key;
	}

	bool operator !=(const FTLLRN_CachedTLLRN_RigElement& Other) const
	{
		return Index != Other.Index || Key != Other.Key;
	}

	bool operator ==(const FTLLRN_RigElementKey& Other) const
	{
		return Key == Other;
	}

	bool operator !=(const FTLLRN_RigElementKey& Other) const
	{
		return Key != Other;
	}

	bool operator ==(const int32& Other) const
	{
		return GetIndex() == Other;
	}

	bool operator !=(const int32& Other) const
	{
		return GetIndex() != Other;
	}

	bool operator <(const FTLLRN_CachedTLLRN_RigElement& Other) const
	{
		if (Key < Other.Key)
		{
			return true;
		}
		return Index < Other.Index;
	}

	bool operator >(const FTLLRN_CachedTLLRN_RigElement& Other) const
	{
		if (Key > Other.Key)
		{
			return true;
		}
		return Index > Other.Index;
	}

private:

	UPROPERTY()
	FTLLRN_RigElementKey Key;

	UPROPERTY()
	uint16 Index;

	UPROPERTY()
	int32 ContainerVersion;

	const FTLLRN_RigBaseElement* Element;
};

class TLLRN_CONTROLRIG_API FTLLRN_RigElementKeyRedirector
{
public:

	FTLLRN_RigElementKeyRedirector()
		: InternalKeyToExternalKey()
		, Hash(UINT32_MAX)
	{}

	FTLLRN_RigElementKeyRedirector(const TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey>& InMap, const UTLLRN_RigHierarchy* InHierarchy);
	FTLLRN_RigElementKeyRedirector(const FTLLRN_RigElementKeyRedirector& InOther, const UTLLRN_RigHierarchy* InHierarchy);
	FTLLRN_RigElementKeyRedirector(const FTLLRN_ModularRigConnections& InOther, const UTLLRN_RigHierarchy* InHierarchy);

	bool Contains(const FTLLRN_RigElementKey& InKey) const { return InternalKeyToExternalKey.Contains(InKey); }
	const FTLLRN_CachedTLLRN_RigElement* Find(const FTLLRN_RigElementKey& InKey) const { return InternalKeyToExternalKey.Find(InKey); }
	const FTLLRN_RigElementKey* FindExternalKey(const FTLLRN_RigElementKey& InKey) const { return ExternalKeys.Find(InKey); }
	FTLLRN_CachedTLLRN_RigElement* Find(const FTLLRN_RigElementKey& InKey) { return InternalKeyToExternalKey.Find(InKey); }
	const FTLLRN_RigElementKey* FindReverse(const FTLLRN_RigElementKey& InKey) const;
	uint32 GetHash() const { return Hash; }
	
private:

	void Add(const FTLLRN_RigElementKey& InSource, const FTLLRN_RigElementKey& InTarget, const UTLLRN_RigHierarchy* InHierarchy);

	TMap<FTLLRN_RigElementKey, FTLLRN_CachedTLLRN_RigElement> InternalKeyToExternalKey;
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> ExternalKeys;
	uint32 Hash;

	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend class FTLLRN_RigModuleInstanceDetails;
	friend class UTLLRN_ModularRig;
	friend class FTLLRN_ControlRigSchematicModel;
};