// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_Collection.h"
#include "Units/Execution/TLLRN_RigUnit_Item.h"
#include "Units/Execution/TLLRN_RigUnit_Hierarchy.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_Collection)

#if WITH_EDITOR
#include "Units/TLLRN_RigUnitTest.h"
#endif

FTLLRN_RigUnit_CollectionChain_Execute()
{
	FTLLRN_RigUnit_CollectionChainArray::StaticExecute(ExecuteContext, FirstItem, LastItem, Reverse, Collection.Keys);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionChain::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_HierarchyGetChainItemArray NewNode;
	NewNode.Start = FirstItem;
	NewNode.End = LastItem;
	NewNode.bReverse = Reverse;
	NewNode.bIncludeStart = NewNode.bIncludeEnd = true;
	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_CollectionChainArray_Execute()
{
	FTLLRN_CachedTLLRN_RigElement FirstCache, LastCache;
	FTLLRN_RigElementKeyCollection CachedChain;
	FTLLRN_RigUnit_HierarchyGetChainItemArray::StaticExecute(ExecuteContext, FirstItem, LastItem, true, true, false, Items, FirstCache, LastCache, CachedChain);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionChainArray::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_HierarchyGetChainItemArray NewNode;
	NewNode.Start = FirstItem;
	NewNode.End = LastItem;
	NewNode.bReverse = Reverse;
	NewNode.bIncludeStart = NewNode.bIncludeEnd = true;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_CollectionNameSearch_Execute()
{
	FTLLRN_RigUnit_CollectionNameSearchArray::StaticExecute(ExecuteContext, PartialName, TypeToSearch, Collection.Keys);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionNameSearch::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_CollectionNameSearchArray NewNode;
	NewNode.PartialName = PartialName;
	NewNode.TypeToSearch = TypeToSearch;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_CollectionNameSearchArray_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if(ExecuteContext.Hierarchy == nullptr)
	{
		Items.Reset();
		return;
	}

	uint32 Hash = GetTypeHash(StaticStruct()) + ExecuteContext.Hierarchy->GetTopologyVersion() * 17;
	Hash = HashCombine(Hash, GetTypeHash(PartialName));
	Hash = HashCombine(Hash, (int32)TypeToSearch * 8);

	FTLLRN_RigElementKeyCollection Collection;
	if(const FTLLRN_RigElementKeyCollection* Cache = ExecuteContext.Hierarchy->FindCachedCollection(Hash))
	{
		Collection = *Cache;
	}
	else
	{
		Collection = FTLLRN_RigElementKeyCollection::MakeFromName(ExecuteContext.Hierarchy, PartialName, (uint8)TypeToSearch);
		ExecuteContext.Hierarchy->AddCachedCollection(Hash, Collection);
	}

	Items = Collection.Keys;
}

FTLLRN_RigUnit_CollectionChildren_Execute()
{
	FTLLRN_RigUnit_CollectionChildrenArray::StaticExecute(ExecuteContext, Parent, bIncludeParent, bRecursive, true, TypeToSearch, Collection.Keys);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionChildren::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_CollectionChildrenArray NewNode;
	NewNode.Parent = Parent;
	NewNode.bIncludeParent = bIncludeParent;
	NewNode.bRecursive = bRecursive;
	NewNode.TypeToSearch = TypeToSearch;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Collection.Keys"), TEXT("Items"));
	return Info;
}

FTLLRN_RigUnit_CollectionChildrenArray_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if(ExecuteContext.Hierarchy == nullptr)
	{
		Items.Reset();
		return;
	}

	if (bDefaultChildren)
	{
		uint32 Hash = GetTypeHash(StaticStruct()) + ExecuteContext.Hierarchy->GetTopologyVersion() * 17;
		Hash = HashCombine(Hash, GetTypeHash(ExecuteContext.Hierarchy->GetResolvedTarget(Parent)));
		Hash = HashCombine(Hash, bRecursive ? 2 : 0);
		Hash = HashCombine(Hash, bIncludeParent ? 1 : 0);
		Hash = HashCombine(Hash, (int32)TypeToSearch * 8);

		FTLLRN_RigElementKeyCollection Collection;
		if(const FTLLRN_RigElementKeyCollection* Cache = ExecuteContext.Hierarchy->FindCachedCollection(Hash))
		{
			Collection = *Cache;
		}
		else
		{
			Collection = FTLLRN_RigElementKeyCollection::MakeFromChildren(ExecuteContext.Hierarchy, Parent, bRecursive, bIncludeParent, (uint8)TypeToSearch);
			if (Collection.IsEmpty())
			{
				if (ExecuteContext.Hierarchy->GetIndex(Parent) == INDEX_NONE)
				{
					UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Parent '%s' is not valid."), *Parent.ToString());
				}
			}
			ExecuteContext.Hierarchy->AddCachedCollection(Hash, Collection);
		}

		Items = Collection.Keys;
	}
	else
	{
		const UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
		Items.Reset();

		if (bIncludeParent)
		{
			Items.Add(Parent);
		}

		FTLLRN_RigBaseElementChildrenArray Children = Hierarchy->GetActiveChildren(Hierarchy->Find(Parent), bRecursive);

		Children = Children.FilterByPredicate([TypeToSearch](const FTLLRN_RigBaseElement* Child)
		{
			return ((uint8)Child->GetType() & (uint8)TypeToSearch) != 0;
		});

		Items.Reserve(Items.Num() + Children.Num());
		for (FTLLRN_RigBaseElement* Child : Children)
		{
			Items.Add(Child->GetKey());
		}
	}
}

FTLLRN_RigUnit_CollectionGetAll_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if(ExecuteContext.Hierarchy == nullptr)
	{
		Items.Reset();
		return;
	}
	
	uint32 Hash = GetTypeHash(StaticStruct()) + ExecuteContext.Hierarchy->GetTopologyVersion() * 17;
	Hash = HashCombine(Hash, (int32)TypeToSearch * 8);

	FTLLRN_RigElementKeyCollection Collection;
	if(const FTLLRN_RigElementKeyCollection* Cache = ExecuteContext.Hierarchy->FindCachedCollection(Hash))
	{
		Collection = *Cache;
	}
	else
	{
		ExecuteContext.Hierarchy->Traverse([&Collection, TypeToSearch](FTLLRN_RigBaseElement* InElement, bool &bContinue)
		{
			bContinue = true;
			
			const FTLLRN_RigElementKey Key = InElement->GetKey();
			if(((uint8)TypeToSearch & (uint8)Key.Type) == (uint8)Key.Type)
			{
				Collection.AddUnique(Key);
			}
		});
		
		if (Collection.IsEmpty())
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("%s"), TEXT("No elements found for given filter."));
		}
		ExecuteContext.Hierarchy->AddCachedCollection(Hash, Collection);
	}

	Items = Collection.Keys;
}

#if WITH_EDITOR

IMPLEMENT_RIGUNIT_AUTOMATION_TEST(FTLLRN_RigUnit_CollectionChildren)
{
	const FTLLRN_RigElementKey Root = Controller->AddBone(TEXT("Root"), FTLLRN_RigElementKey(), FTransform(FVector(0.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneA = Controller->AddBone(TEXT("BoneA"), Root, FTransform(FVector(0.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneB = Controller->AddBone(TEXT("BoneB"), BoneA, FTransform(FVector(0.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);
	const FTLLRN_RigElementKey BoneC = Controller->AddBone(TEXT("BoneC"), Root, FTransform(FVector(0.f, 0.f, 0.f)), true, ETLLRN_RigBoneType::User);

	Unit.Parent = Root;
	Unit.bIncludeParent = false;
	Unit.bRecursive = false;
	Execute();
	AddErrorIfFalse(Unit.Collection.Num() == 2, TEXT("unexpected result"));
	AddErrorIfFalse(Unit.Collection[0] == BoneA, TEXT("unexpected result"));
	AddErrorIfFalse(Unit.Collection[1] == BoneC, TEXT("unexpected result"));

	Unit.bIncludeParent = true;
	Unit.bRecursive = false;
	Execute();
	AddErrorIfFalse(Unit.Collection.Num() == 3, TEXT("unexpected result"));
	AddErrorIfFalse(Unit.Collection[0] == Root, TEXT("unexpected result"));
	AddErrorIfFalse(Unit.Collection[1] == BoneA, TEXT("unexpected result"));
	AddErrorIfFalse(Unit.Collection[2] == BoneC, TEXT("unexpected result"));

	Unit.bIncludeParent = true;
	Unit.bRecursive = true;
	Execute();
	AddErrorIfFalse(Unit.Collection.Num() == 4, TEXT("unexpected result"));
	AddErrorIfFalse(Unit.Collection[0] == Root, TEXT("unexpected result"));
	AddErrorIfFalse(Unit.Collection[1] == BoneA, TEXT("unexpected result"));
	AddErrorIfFalse(Unit.Collection[2] == BoneC, TEXT("unexpected result"));
	AddErrorIfFalse(Unit.Collection[3] == BoneB, TEXT("unexpected result"));

	return true;
}

#endif

FTLLRN_RigUnit_CollectionReplaceItems_Execute()
{
	FTLLRN_RigUnit_CollectionReplaceItemsArray::StaticExecute(ExecuteContext, Items.Keys, Old, New, RemoveInvalidItems, bAllowDuplicates, Collection.Keys);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionReplaceItems::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_CollectionReplaceItemsArray NewNode;
	NewNode.Items = Items.Keys;
	NewNode.Old = Old;
	NewNode.New = New;
	NewNode.RemoveInvalidItems = RemoveInvalidItems;
	NewNode.bAllowDuplicates = bAllowDuplicates;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_CollectionReplaceItemsArray_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if(ExecuteContext.Hierarchy == nullptr)
	{
		Result.Reset();
		return;
	}
	
	uint32 Hash = GetTypeHash(StaticStruct()) + ExecuteContext.Hierarchy->GetTopologyVersion() * 17;
	Hash = HashCombine(Hash, GetTypeHash(Items));
	Hash = HashCombine(Hash, 12 * GetTypeHash(Old));
	Hash = HashCombine(Hash, 13 * GetTypeHash(New));
	Hash = HashCombine(Hash, RemoveInvalidItems ? 14 : 0);

	FTLLRN_RigElementKeyCollection Collection;
	if(const FTLLRN_RigElementKeyCollection* Cache = ExecuteContext.Hierarchy->FindCachedCollection(Hash))
	{
		Collection = *Cache;
	}
	else
	{
		Collection.Reset();
		
		for (int32 Index = 0; Index < Items.Num(); Index++)
		{
			FTLLRN_RigElementKey Key = Items[Index];
			FTLLRN_RigUnit_ItemReplace::StaticExecute(ExecuteContext, Key, Old, New, Key);

			if (ExecuteContext.Hierarchy->GetIndex(Key) != INDEX_NONE)
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
			else if(!RemoveInvalidItems)
			{
				Collection.Add(FTLLRN_RigElementKey());
			}
		}

		ExecuteContext.Hierarchy->AddCachedCollection(Hash, Collection);
	}
	
	Result = Collection.Keys;
}

FTLLRN_RigUnit_CollectionItems_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	Collection.Reset();
	for (const FTLLRN_RigElementKey& Key : Items)
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
}

FTLLRN_RigUnit_CollectionGetItems_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	Items = Collection.GetKeys();
}

FTLLRN_RigUnit_CollectionGetParentIndices_Execute()
{
	FTLLRN_RigUnit_CollectionGetParentIndicesItemArray::StaticExecute(ExecuteContext, Collection.Keys, ParentIndices);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionGetParentIndices::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_CollectionGetParentIndicesItemArray NewNode;
	NewNode.Items = Collection.Keys;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Collection"), TEXT("Items"));
	return Info;
}

FTLLRN_RigUnit_CollectionGetParentIndicesItemArray_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if(ExecuteContext.Hierarchy == nullptr)
	{
		ParentIndices.Reset();
		return;
	}
	
	ParentIndices.SetNumUninitialized(Items.Num());

	for(int32 Index=0;Index<Items.Num();Index++)
	{
		ParentIndices[Index] = INDEX_NONE;

		const int32 ItemIndex = ExecuteContext.Hierarchy->GetIndex(Items[Index]);
		if(ItemIndex == INDEX_NONE)
		{
			continue;;
		}

		switch(Items[Index].Type)
		{
			case ETLLRN_RigElementType::Curve:
			{
				continue;
			}
			case ETLLRN_RigElementType::Bone:
			{
				ParentIndices[Index] = ExecuteContext.Hierarchy->GetFirstParent(ItemIndex);
				break;
			}
			default:
			{
				if(const FTLLRN_RigBaseElement* ChildElement = ExecuteContext.Hierarchy->Get(ItemIndex))
				{
					TArray<int32> ItemParents = ExecuteContext.Hierarchy->GetParents(ItemIndex);
					for(int32 ParentIndex = 0; ParentIndex < ItemParents.Num(); ParentIndex++)
					{
						const FTLLRN_RigElementWeight Weight = ExecuteContext.Hierarchy->GetParentWeight(ChildElement, ParentIndex, false);
						if(!Weight.IsAlmostZero())
						{
							ParentIndices[Index] = ItemParents[ParentIndex];
						}
					}
				}
				break;
			}
		}

		if(ParentIndices[Index] != INDEX_NONE)
		{
			int32 ParentIndex = ParentIndices[Index];
			ParentIndices[Index] = INDEX_NONE;

			while(ParentIndices[Index] == INDEX_NONE && ParentIndex != INDEX_NONE)
			{
				ParentIndices[Index] = Items.Find(ExecuteContext.Hierarchy->GetKey(ParentIndex));
				ParentIndex = ExecuteContext.Hierarchy->GetFirstParent(ParentIndex);
			}
		}
	}
}

FTLLRN_RigUnit_CollectionUnion_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	Collection = FTLLRN_RigElementKeyCollection::MakeUnion(A, B, bAllowDuplicates);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionUnion::GetUpgradeInfo() const
{
	// this node is no longer supported. you can rely on generic array nodes for this now
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_CollectionIntersection_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	Collection = FTLLRN_RigElementKeyCollection::MakeIntersection(A, B);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionIntersection::GetUpgradeInfo() const
{
	// this node is no longer supported. you can rely on generic array nodes for this now
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_CollectionDifference_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	Collection = FTLLRN_RigElementKeyCollection::MakeDifference(A, B);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionDifference::GetUpgradeInfo() const
{
	// this node is no longer supported. you can rely on generic array nodes for this now
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_CollectionReverse_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	Reversed = FTLLRN_RigElementKeyCollection::MakeReversed(Collection);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionReverse::GetUpgradeInfo() const
{
	// this node is no longer supported. you can rely on generic array nodes for this now
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_CollectionCount_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	Count = Collection.Num();
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionCount::GetUpgradeInfo() const
{
	// this node is no longer supported. you can rely on generic array nodes for this now
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_CollectionItemAtIndex_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (Collection.IsValidIndex(Index))
	{
		Item = Collection[Index];
	}
	else
	{
		Item = FTLLRN_RigElementKey();
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionItemAtIndex::GetUpgradeInfo() const
{
	// this node is no longer supported. you can rely on generic array nodes for this now
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_CollectionLoop_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	
	if(BlockToRun.IsNone())
	{
	    Count = Collection.Num();
		Index = 0;
		BlockToRun = ExecuteContextName;
	}
	else if(BlockToRun == ExecuteContextName)
	{
		Index++;
	}

	if(Collection.IsValidIndex(Index))
	{
		Item = Collection[Index];
	}
	else
	{
		Item = FTLLRN_RigElementKey();
		BlockToRun = ControlFlowCompletedName;
	}
	
	Ratio = GetRatioFromIndex(Index, Count);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionLoop::GetUpgradeInfo() const
{
	// this node is no longer supported. you can rely on generic array nodes for this now
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_CollectionAddItem_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT();

	Result = Collection;
	Result.AddUnique(Item);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_CollectionAddItem::GetUpgradeInfo() const
{
	// this node is no longer supported. you can rely on generic array nodes for this now
	return FRigVMStructUpgradeInfo();
}
