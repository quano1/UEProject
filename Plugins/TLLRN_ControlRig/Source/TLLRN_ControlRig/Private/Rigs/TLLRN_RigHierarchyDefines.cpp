// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigHierarchyDefines.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Misc/WildcardString.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigHierarchyDefines)

#if WITH_EDITOR
#include "RigVMPythonUtils.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigControlLimitEnabled
////////////////////////////////////////////////////////////////////////////////

void FTLLRN_RigControlLimitEnabled::Serialize(FArchive& Ar)
{
	Ar << bMinimum;
	Ar << bMaximum;
}

bool FTLLRN_RigControlLimitEnabled::GetForValueType(ETLLRN_RigControlValueType InValueType) const
{
	if(InValueType == ETLLRN_RigControlValueType::Minimum)
	{
		return bMinimum;
	}
	return bMaximum;
}

void FTLLRN_RigControlLimitEnabled::SetForValueType(ETLLRN_RigControlValueType InValueType, bool InValue)
{
	if(InValueType == ETLLRN_RigControlValueType::Minimum)
	{
		bMinimum = InValue;
	}
	else
	{
		bMaximum = InValue;
	}
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigElementKey
////////////////////////////////////////////////////////////////////////////////

void FTLLRN_RigElementKey::Serialize(FArchive& Ar)
{
	if (Ar.IsSaving() || Ar.IsObjectReferenceCollector() || Ar.IsCountingMemory())
	{
		Save(Ar);
	}
	else if (Ar.IsLoading())
	{
		Load(Ar);
	}
	else
	{
		// remove due to FPIEFixupSerializer hitting this checkNoEntry();
	}
}

void FTLLRN_RigElementKey::Save(FArchive& Ar)
{
	static const UEnum* ElementTypeEnum = StaticEnum<ETLLRN_RigElementType>();

	FName TypeName = ElementTypeEnum->GetNameByValue((int64)Type);
	Ar << TypeName;
	Ar << Name;
}

void FTLLRN_RigElementKey::Load(FArchive& Ar)
{
	static const UEnum* ElementTypeEnum = StaticEnum<ETLLRN_RigElementType>();

	FName TypeName;
	Ar << TypeName;

	const int64 TypeValue = ElementTypeEnum->GetValueByName(TypeName);
	Type = (ETLLRN_RigElementType)TypeValue;

	Ar << Name;
}

FString FTLLRN_RigElementKey::ToPythonString() const
{
#if WITH_EDITOR
	return FString::Printf(TEXT("unreal.TLLRN_RigElementKey(type=%s, name='%s')"),
		*RigVMPythonUtils::EnumValueToPythonString<ETLLRN_RigElementType>((int64)Type),
		*Name.ToString());
#else
	return FString();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigElementKeyAndIndex
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigElementKeyAndIndex::FTLLRN_RigElementKeyAndIndex(const FTLLRN_RigBaseElement* InElement)
: Key(InElement->Key)
, Index(InElement->Index)
{
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigElementKeyCollection
////////////////////////////////////////////////////////////////////////////////

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::MakeFromChildren(
	UTLLRN_RigHierarchy* InHierarchy,
	const FTLLRN_RigElementKey& InParentKey,
	bool bRecursive,
	bool bIncludeParent,
	uint8 InElementTypes)
{
	check(InHierarchy);

	FTLLRN_RigElementKeyCollection Collection;

	int32 Index = InHierarchy->GetIndex(InParentKey);
	if (Index == INDEX_NONE)
	{
		return Collection;
	}

	if (bIncludeParent)
	{
		Collection.AddUnique(InParentKey);
	}

	TArray<FTLLRN_RigElementKey> ParentKeys;
	ParentKeys.Add(InParentKey);

	bool bAddBones = (InElementTypes & (uint8)ETLLRN_RigElementType::Bone) == (uint8)ETLLRN_RigElementType::Bone;
	bool bAddControls = (InElementTypes & (uint8)ETLLRN_RigElementType::Control) == (uint8)ETLLRN_RigElementType::Control;
	bool bAddNulls = (InElementTypes & (uint8)ETLLRN_RigElementType::Null) == (uint8)ETLLRN_RigElementType::Null;
	bool bAddCurves = (InElementTypes & (uint8)ETLLRN_RigElementType::Curve) == (uint8)ETLLRN_RigElementType::Curve;

	for (int32 ParentIndex = 0; ParentIndex < ParentKeys.Num(); ParentIndex++)
	{
		const FTLLRN_RigElementKey ParentKey = ParentKeys[ParentIndex];
		TArray<FTLLRN_RigElementKey> Children = InHierarchy->GetChildren(ParentKey);
		for(const FTLLRN_RigElementKey& Child : Children)
		{
			if((InElementTypes & (uint8)Child.Type) == (uint8)Child.Type)
			{
				const int32 PreviousSize = Collection.Num();
				if(PreviousSize == Collection.AddUnique(Child))
				{
					if(bRecursive)
					{
						ParentKeys.Add(Child);
					}
				}
			}
		}
	}

	return Collection;
}

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::MakeFromName(
	UTLLRN_RigHierarchy* InHierarchy,
	const FName& InPartialName,
	uint8 InElementTypes
)
{
	if (InPartialName.IsNone())
	{
		return MakeFromCompleteHierarchy(InHierarchy, InElementTypes);
	}

	check(InHierarchy);

	constexpr bool bTraverse = true;

	const FString PartialNameString = InPartialName.ToString();
	const FWildcardString WildcardString(PartialNameString);
	if(WildcardString.ContainsWildcards())
	{
		return InHierarchy->GetKeysByPredicate([WildcardString, InElementTypes](const FTLLRN_RigBaseElement& InElement) -> bool
		{
			return InElement.IsTypeOf(static_cast<ETLLRN_RigElementType>(InElementTypes)) &&
				   WildcardString.IsMatch(InElement.GetName());
		}, bTraverse);
	}
	
	return InHierarchy->GetKeysByPredicate([PartialNameString, InElementTypes](const FTLLRN_RigBaseElement& InElement) -> bool
	{
		return InElement.IsTypeOf(static_cast<ETLLRN_RigElementType>(InElementTypes)) &&
			   InElement.GetName().Contains(PartialNameString);
	}, bTraverse);
}

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::MakeFromChain(
	UTLLRN_RigHierarchy* InHierarchy,
	const FTLLRN_RigElementKey& InFirstItem,
	const FTLLRN_RigElementKey& InLastItem,
	bool bReverse
)
{
	check(InHierarchy);

	FTLLRN_RigElementKeyCollection Collection;

	int32 FirstIndex = InHierarchy->GetIndex(InFirstItem);
	int32 LastIndex = InHierarchy->GetIndex(InLastItem);

	if (FirstIndex == INDEX_NONE || LastIndex == INDEX_NONE)
	{
		return Collection;
	}

	FTLLRN_RigElementKey LastKey = InLastItem;
	while (LastKey.IsValid() && LastKey != InFirstItem)
	{
		Collection.Keys.Add(LastKey);
		LastKey = InHierarchy->GetFirstParent(LastKey);
	}

	if (LastKey != InFirstItem)
	{
		Collection.Reset();
	}
	else
	{
		Collection.AddUnique(InFirstItem);
	}

	if (!bReverse)
	{
		Algo::Reverse(Collection.Keys);
	}

	return Collection;
}

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::MakeFromCompleteHierarchy(
	UTLLRN_RigHierarchy* InHierarchy,
	uint8 InElementTypes
)
{
	check(InHierarchy);

	FTLLRN_RigElementKeyCollection Collection(InHierarchy->GetAllKeys(true));
	return Collection.FilterByType(InElementTypes);
}

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::MakeUnion(const FTLLRN_RigElementKeyCollection& A, const FTLLRN_RigElementKeyCollection& B, bool bAllowDuplicates)
{
	FTLLRN_RigElementKeyCollection Collection;
	for (const FTLLRN_RigElementKey& Key : A)
	{
		Collection.Add(Key);
	}
	for (const FTLLRN_RigElementKey& Key : B)
	{
		if(bAllowDuplicates)
		{
			Collection.Add(Key);
		}
		else
		{
			Collection.AddUnique(Key);
		}
	}
	return Collection;
}

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::MakeIntersection(const FTLLRN_RigElementKeyCollection& A, const FTLLRN_RigElementKeyCollection& B)
{
	FTLLRN_RigElementKeyCollection Collection;
	for (const FTLLRN_RigElementKey& Key : A)
	{
		if (B.Contains(Key))
		{
			Collection.Add(Key);
		}
	}
	return Collection;
}

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::MakeDifference(const FTLLRN_RigElementKeyCollection& A, const FTLLRN_RigElementKeyCollection& B)
{
	FTLLRN_RigElementKeyCollection Collection;
	for (const FTLLRN_RigElementKey& Key : A)
	{
		if (!B.Contains(Key))
		{
			Collection.Add(Key);
		}
	}
	return Collection;
}

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::MakeReversed(const FTLLRN_RigElementKeyCollection& InCollection)
{
	FTLLRN_RigElementKeyCollection Reversed = InCollection;
	Algo::Reverse(Reversed.Keys);
	return Reversed;
}

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::FilterByType(uint8 InElementTypes) const
{
	FTLLRN_RigElementKeyCollection Collection;
	for (const FTLLRN_RigElementKey& Key : *this)
	{
		if ((InElementTypes & (uint8)Key.Type) == (uint8)Key.Type)
		{
			Collection.Add(Key);
		}
	}
	return Collection;
}

FTLLRN_RigElementKeyCollection FTLLRN_RigElementKeyCollection::FilterByName(const FName& InPartialName) const
{
	FString SearchToken = InPartialName.ToString();

	FTLLRN_RigElementKeyCollection Collection;
	for (const FTLLRN_RigElementKey& Key : *this)
	{
		if (Key.Name == InPartialName)
		{
			Collection.Add(Key);
		}
		else if (Key.Name.ToString().Contains(SearchToken, ESearchCase::CaseSensitive, ESearchDir::FromStart))
		{
			Collection.Add(Key);
		}
	}
	return Collection;
}

FArchive& operator<<(FArchive& Ar, FTLLRN_RigControlValue& Value)
{
	Ar <<  Value.FloatStorage.Float00;
	Ar <<  Value.FloatStorage.Float01;
	Ar <<  Value.FloatStorage.Float02;
	Ar <<  Value.FloatStorage.Float03;
	Ar <<  Value.FloatStorage.Float10;
	Ar <<  Value.FloatStorage.Float11;
	Ar <<  Value.FloatStorage.Float12;
	Ar <<  Value.FloatStorage.Float13;
	Ar <<  Value.FloatStorage.Float20;
	Ar <<  Value.FloatStorage.Float21;
	Ar <<  Value.FloatStorage.Float22;
	Ar <<  Value.FloatStorage.Float23;
	Ar <<  Value.FloatStorage.Float30;
	Ar <<  Value.FloatStorage.Float31;
	Ar <<  Value.FloatStorage.Float32;
	Ar <<  Value.FloatStorage.Float33;
	Ar <<  Value.FloatStorage.Float00_2;
	Ar <<  Value.FloatStorage.Float01_2;
	Ar <<  Value.FloatStorage.Float02_2;
	Ar <<  Value.FloatStorage.Float03_2;
	Ar <<  Value.FloatStorage.Float10_2;
	Ar <<  Value.FloatStorage.Float11_2;
	Ar <<  Value.FloatStorage.Float12_2;
	Ar <<  Value.FloatStorage.Float13_2;
	Ar <<  Value.FloatStorage.Float20_2;
	Ar <<  Value.FloatStorage.Float21_2;
	Ar <<  Value.FloatStorage.Float22_2;
	Ar <<  Value.FloatStorage.Float23_2;
	Ar <<  Value.FloatStorage.Float30_2;
	Ar <<  Value.FloatStorage.Float31_2;
	Ar <<  Value.FloatStorage.Float32_2;
	Ar <<  Value.FloatStorage.Float33_2;

	return Ar;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigElementResolveResult
////////////////////////////////////////////////////////////////////////////////

bool FTLLRN_RigElementResolveResult::IsValid() const
{
	return State == ETLLRN_RigElementResolveState::PossibleTarget ||
		State == ETLLRN_RigElementResolveState::DefaultTarget;
}

void FTLLRN_RigElementResolveResult::SetInvalidTarget(const FText& InMessage)
{
	State = ETLLRN_RigElementResolveState::InvalidTarget;
	Message = InMessage;
}

void FTLLRN_RigElementResolveResult::SetPossibleTarget(const FText& InMessage)
{
	State = ETLLRN_RigElementResolveState::PossibleTarget;
	Message = InMessage;
}

void FTLLRN_RigElementResolveResult::SetDefaultTarget(const FText& InMessage)
{
	State = ETLLRN_RigElementResolveState::DefaultTarget;
	Message = InMessage;
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_ModularRigResolveResult
////////////////////////////////////////////////////////////////////////////////

bool FTLLRN_ModularRigResolveResult::IsValid() const
{
	return State == ETLLRN_ModularRigResolveState::Success && !Matches.IsEmpty();
}

bool FTLLRN_ModularRigResolveResult::ContainsMatch(const FTLLRN_RigElementKey& InKey, FString* OutErrorMessage) const
{
	if(Matches.ContainsByPredicate([InKey](const FTLLRN_RigElementResolveResult& InMatch) -> bool
	{
		return InMatch.GetKey() == InKey;
	}))
	{
		return true;
	}

	if(OutErrorMessage)
	{
		if(const FTLLRN_RigElementResolveResult* Mismatch = Excluded.FindByPredicate([InKey](const FTLLRN_RigElementResolveResult& InMatch) -> bool
		{
			return InMatch.GetKey() == InKey;
		}))
		{
			*OutErrorMessage = Mismatch->GetMessage().ToString();
		}
	}
	
	return false;
}

const FTLLRN_RigElementResolveResult* FTLLRN_ModularRigResolveResult::FindMatch(const FTLLRN_RigElementKey& InKey) const
{
	return Matches.FindByPredicate([InKey](const FTLLRN_RigElementResolveResult& InMatch) -> bool
	{
		return InMatch.GetKey() == InKey;
	});
}

const FTLLRN_RigElementResolveResult* FTLLRN_ModularRigResolveResult::GetDefaultMatch() const
{
	return Matches.FindByPredicate([](const FTLLRN_RigElementResolveResult& Match)
	{
		return Match.GetState() == ETLLRN_RigElementResolveState::DefaultTarget;
	});
}
