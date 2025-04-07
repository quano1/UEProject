// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h" 
#include "TLLRN_RigUnit_Collection.generated.h"

USTRUCT(meta = (Abstract, NodeColor = "0.4627450108528137 1.0 0.3294120132923126", Category = "Items"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionBase : public FTLLRN_RigUnit
{
	GENERATED_BODY()
};

USTRUCT(meta = (Abstract, NodeColor = "0.4627450108528137 1.0 0.3294120132923126", Category = "Items"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionBaseMutable : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()
};

/**
 * Creates a collection based on a first and last item within a chain.
 * Chains can refer to bone chains or chains within a control hierarchy.
 */
USTRUCT(meta=(DisplayName="Item Chain", Keywords="Bone,Joint,Collection", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionChain : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionChain()
	{
		FirstItem = LastItem = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		Reverse = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey FirstItem;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey LastItem;

	UPROPERTY(meta = (Input))
	bool Reverse;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Collection;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Creates an item array based on a first and last item within a chain.
* Chains can refer to bone chains or chains within a control hierarchy.
*/
USTRUCT(meta=(DisplayName="Item Chain", Keywords="Bone,Joint,Collection", Varying, Deprecated = "5.4"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionChainArray : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionChainArray()
	{
		FirstItem = LastItem = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		Reverse = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey FirstItem;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey LastItem;

	UPROPERTY(meta = (Input))
	bool Reverse;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Items;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Creates a collection based on a name search.
 * The name search is case sensitive.
 */
USTRUCT(meta = (DisplayName = "Item Name Search", Keywords = "Bone,Joint,Collection,Filter", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionNameSearch : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionNameSearch()
	{
		PartialName = NAME_None;
		TypeToSearch = ETLLRN_RigElementType::All;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FName PartialName;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType TypeToSearch;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Collection;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Creates an item array based on a name search.
* The name search is case sensitive.
*/
USTRUCT(meta = (DisplayName = "Item Name Search", Keywords = "Bone,Joint,Collection,Filter", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionNameSearchArray : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionNameSearchArray()
	{
		PartialName = NAME_None;
		TypeToSearch = ETLLRN_RigElementType::All;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FName PartialName;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType TypeToSearch;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
 * Creates a collection based on the direct or recursive children
 * of a provided parent item. Returns an empty collection for an invalid parent item.
 */
USTRUCT(meta = (DisplayName = "Get Children", Keywords = "Bone,Joint,Collection,Filter,Parent", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionChildren : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionChildren()
	{
		Parent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		bIncludeParent = false;
		bRecursive = false;
		TypeToSearch = ETLLRN_RigElementType::All;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Parent;

	UPROPERTY(meta = (Input))
	bool bIncludeParent;

	UPROPERTY(meta = (Input))
	bool bRecursive;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType TypeToSearch;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Collection;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Creates an item array based on the direct or recursive children
* of a provided parent item. Returns an empty array for an invalid parent item.
*/
USTRUCT(meta = (DisplayName = "Get Children", Category = "Hierarchy", Keywords = "Bone,Joint,Collection,Filter,Parent", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionChildrenArray : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionChildrenArray()
	{
		Parent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		bIncludeParent = false;
		bRecursive = false;
		bDefaultChildren = true;
		TypeToSearch = ETLLRN_RigElementType::All;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Parent;

	UPROPERTY(meta = (Input))
	bool bIncludeParent;

	UPROPERTY(meta = (Input))
	bool bRecursive;

	/** When true, it will return all children, regardless of whether the parent is active or not.
	 * When false, will return only the children which are influenced by this parent */
	UPROPERTY(meta = (Input))
	bool bDefaultChildren;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType TypeToSearch;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
* Creates an item array for all elements given the filter.
*/
USTRUCT(meta = (DisplayName = "Get All", Category = "Hierarchy", Keywords = "Bone,Joint,Collection,Filter,Parent", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionGetAll : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionGetAll()
	{
		TypeToSearch = ETLLRN_RigElementType::All;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType TypeToSearch;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
 * Replaces all names within the collection
 */
USTRUCT(meta = (DisplayName = "Replace Items", Keywords = "Replace,Find,Collection", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionReplaceItems : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionReplaceItems()
	{
		Old = New = NAME_None;
		RemoveInvalidItems = false;
		bAllowDuplicates = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Items;

	UPROPERTY(meta = (Input))
	FName Old;

	UPROPERTY(meta = (Input))
	FName New;

	UPROPERTY(meta = (Input))
	bool RemoveInvalidItems;

	UPROPERTY(meta = (Input))
	bool bAllowDuplicates;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Collection;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Replaces all names within the item array
*/
USTRUCT(meta = (DisplayName = "Replace Items", Keywords = "Replace,Find,Collection", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionReplaceItemsArray : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionReplaceItemsArray()
	{
		Old = New = NAME_None;
		RemoveInvalidItems = false;
		bAllowDuplicates = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> Items;

	UPROPERTY(meta = (Input))
	FName Old;

	UPROPERTY(meta = (Input))
	FName New;

	UPROPERTY(meta = (Input))
	bool RemoveInvalidItems;

	UPROPERTY(meta = (Input))
	bool bAllowDuplicates;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Result;
};

/**
 * Returns a collection provided a specific array of items.
 */
USTRUCT(meta = (DisplayName = "Collection from Items", Category = "Items|Collections", Keywords = "Collection,Array", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionItems : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionItems()
	{
		Items.Add(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone));
		bAllowDuplicates = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	TArray<FTLLRN_RigElementKey> Items;

	UPROPERTY(meta = (Input))
	bool bAllowDuplicates;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Collection;
};

/**
* Returns an array of items provided a collection
*/
USTRUCT(meta = (DisplayName = "Get Items from Collection", Category = "Items|Collections", Keywords = "Collection,Array", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionGetItems : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionGetItems()
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Collection;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
 * Returns an array of relative parent indices for each item. Several options here
 * a) If an item has multiple parents the major parent (based on the weights) will be returned.
 * b) If an item has a parent that's not part of the collection INDEX_NONE will be returned.
 * c) If an item has a parent that's not part of the collection, but a grand parent is we'll use that index instead.
 */
USTRUCT(meta = (DisplayName = "Get Parent Indices", Keywords = "Collection,Array", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionGetParentIndices : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionGetParentIndices()
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Collection;

	UPROPERTY(meta = (Output))
	TArray<int32> ParentIndices;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Returns an array of relative parent indices for each item. Several options here
* a) If an item has multiple parents the major parent (based on the weights) will be returned.
* b) If an item has a parent that's not part of the collection INDEX_NONE will be returned.
* c) If an item has a parent that's not part of the collection, but a grand parent is we'll use that index instead.
*/
USTRUCT(meta = (DisplayName = "Get Parent Indices", Keywords = "Collection,Array", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionGetParentIndicesItemArray : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionGetParentIndicesItemArray()
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> Items;

	UPROPERTY(meta = (Output))
	TArray<int32> ParentIndices;
};

/**
 * Returns the union of two provided collections
 * (the combination of all items from both A and B).
 */
USTRUCT(meta = (DisplayName = "Union", Keywords = "Combine,Add,Merge,Collection,Hierarchy", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionUnion : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionUnion()
	{
		bAllowDuplicates = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection A;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection B;

	UPROPERTY(meta = (Input))
	bool bAllowDuplicates;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Collection;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Returns the intersection of two provided collections
 * (the items present in both A and B).
 */
USTRUCT(meta = (DisplayName = "Intersection", Keywords = "Combine,Merge,Collection,Hierarchy", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionIntersection : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionIntersection()
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection A;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection B;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Collection;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Returns the difference between two collections
 * (the items present in A but not in B).
 */
USTRUCT(meta = (DisplayName = "Difference", Keywords = "Collection,Exclude,Subtract", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionDifference : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionDifference()
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection A;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection B;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Collection;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Returns the collection in reverse order
 */
USTRUCT(meta = (DisplayName = "Reverse", Keywords = "Direction,Order,Reverse", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionReverse : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionReverse()
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Collection;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Reversed;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Returns the number of elements in a collection
 */
USTRUCT(meta = (DisplayName = "Count", Keywords = "Collection,Array,Count,Num,Length,Size", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionCount : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionCount()
	{
		Collection = FTLLRN_RigElementKeyCollection();
		Count = 0;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Collection;

	UPROPERTY(meta = (Output))
	int32 Count;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Returns a single item within a collection by index
 */
USTRUCT(meta = (DisplayName = "Item At Index", Keywords = "Item,GetIndex,AtIndex,At,ForIndex,[]", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionItemAtIndex : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionItemAtIndex()
	{
		Collection = FTLLRN_RigElementKeyCollection();
		Index = 0;
		Item = FTLLRN_RigElementKey();
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Collection;

	UPROPERTY(meta = (Input))
	int32 Index;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKey Item;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Given a collection of items, execute iteratively across all items in a given collection
 */
USTRUCT(meta=(DisplayName="For Each Item", Keywords="Collection,Loop,Iterate", Icon="EditorStyle|GraphEditor.Macro.ForEach_16x", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionLoop : public FTLLRN_RigUnit_CollectionBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionLoop()
	{
		BlockToRun = NAME_None;
		Count = 0;
		Index = 0;
		Ratio = 0.f;
	}

	// FRigVMStruct overrides
	virtual const TArray<FName>& GetControlFlowBlocks_Impl() const override
	{
		static const TArray<FName> Blocks = {ExecuteContextName, ForLoopCompletedPinName};
		return Blocks;
	}
	virtual const bool IsControlFlowBlockSliced(const FName& InBlockName) const { return InBlockName == ExecuteContextName; }
	virtual int32 GetNumSlices() const override { return Count; }

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Singleton))
	FName BlockToRun;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Collection;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKey Item;

	UPROPERTY(meta = (Singleton, Output))
	int32 Index;

	UPROPERTY(meta = (Singleton, Output))
	int32 Count;

	/**
	 * Ranging from 0.0 (first item) and 1.0 (last item)
	 * This is useful to drive a consecutive node with a 
	 * curve or an ease to distribute a value.
	 */
	UPROPERTY(meta = (Singleton, Output))
	float Ratio;

	UPROPERTY(meta = (Output))
	FTLLRN_ControlRigExecuteContext Completed;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Adds an element to an existing collection
*/
USTRUCT(meta = (DisplayName = "Add Item", Keywords = "Item,Add,Push,Insert", Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_CollectionAddItem : public FTLLRN_RigUnit_CollectionBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_CollectionAddItem()
	{
		Collection = Result = FTLLRN_RigElementKeyCollection();
		Item = FTLLRN_RigElementKey();
	}

	RIGVM_METHOD()
    virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Collection;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Item;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Result;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};