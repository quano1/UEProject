// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h" 
#include "TLLRN_RigUnit_Hierarchy.generated.h"

USTRUCT(meta = (Abstract, NodeColor="0.462745, 1,0, 0.329412", Category = "Hierarchy"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyBase : public FTLLRN_RigUnit
{
	GENERATED_BODY()
};

USTRUCT(meta = (Abstract, NodeColor="0.462745, 1,0, 0.329412", Category = "Hierarchy"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyBaseMutable : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()
};

/**
 * Returns the item's parent
 */
USTRUCT(meta=(DisplayName="Get Parent", Keywords="Child,Parent,Root,Up,Top", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetParent : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetParent()
	{
		Child = Parent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		bDefaultParent = true;
		CachedChild = CachedParent = FTLLRN_CachedTLLRN_RigElement();
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/** When true, it will return the default parent, regardless of whether the parent incluences the element or not  */
	UPROPERTY(meta = (Input))
	bool bDefaultParent;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKey Parent;

	// Used to cache the internally used child
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedChild;

	// Used to cache the internally used parent
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedParent;
};

/**
 * Returns the item's parents
 */
USTRUCT(meta=(DisplayName="Get Parents", Keywords="Chain,Parents,Hierarchy", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetParents : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetParents()
	{
		Child = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		CachedChild = FTLLRN_CachedTLLRN_RigElement();
		Parents = CachedParents = FTLLRN_RigElementKeyCollection();
		bIncludeChild = false;
		bReverse = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	UPROPERTY(meta = (Input))
	bool bIncludeChild;

	UPROPERTY(meta = (Input))
	bool bReverse;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Parents;

	// Used to cache the internally used child
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedChild;

	// Used to cache the internally used parents
	UPROPERTY()
	FTLLRN_RigElementKeyCollection CachedParents;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Returns the item's parents
 */
USTRUCT(meta=(DisplayName="Get Parents", Keywords="Chain,Parents,Hierarchy", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetParentsItemArray : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetParentsItemArray()
	{
		Child = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		CachedChild = FTLLRN_CachedTLLRN_RigElement();
		CachedParents = FTLLRN_RigElementKeyCollection();
		bIncludeChild = false;
		bReverse = false;
		bDefaultParent = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	UPROPERTY(meta = (Input))
	bool bIncludeChild;

	UPROPERTY(meta = (Input))
	bool bReverse;

	UPROPERTY(meta = (Input))
	bool bDefaultParent;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Parents;

	// Used to cache the internally used child
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedChild;

	// Used to cache the internally used parents
	UPROPERTY()
	FTLLRN_RigElementKeyCollection CachedParents;
};


/**
 * Returns the item's children
 */
USTRUCT(meta=(DisplayName="Get Children", Keywords="Chain,Children,Hierarchy", Deprecated = "4.25.0", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetChildren : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetChildren()
	{
		Parent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		CachedParent = FTLLRN_CachedTLLRN_RigElement();
		Children = CachedChildren = FTLLRN_RigElementKeyCollection();
		bIncludeParent = false;
		bRecursive = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Parent;

	UPROPERTY(meta = (Input))
	bool bIncludeParent;

	UPROPERTY(meta = (Input))
	bool bRecursive;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Children;

	// Used to cache the internally used parent
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedParent;

	// Used to cache the internally used children
	UPROPERTY()
	FTLLRN_RigElementKeyCollection CachedChildren;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Returns the item's siblings
 */
USTRUCT(meta=(DisplayName="Get Siblings", Keywords="Chain,Siblings,Hierarchy", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetSiblings : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetSiblings()
	{
		Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		CachedItem = FTLLRN_CachedTLLRN_RigElement();
		Siblings = CachedSiblings = FTLLRN_RigElementKeyCollection();
		bIncludeItem = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	UPROPERTY(meta = (Input))
	bool bIncludeItem;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Siblings;

	// Used to cache the internally used item
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedItem;

	// Used to cache the internally used siblings
	UPROPERTY()
	FTLLRN_RigElementKeyCollection CachedSiblings;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Returns the item's siblings
 */
USTRUCT(meta=(DisplayName="Get Siblings", Keywords="Chain,Siblings,Hierarchy", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetSiblingsItemArray : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetSiblingsItemArray()
	{
		Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		CachedItem = FTLLRN_CachedTLLRN_RigElement();
		CachedSiblings = FTLLRN_RigElementKeyCollection();
		bIncludeItem = false;
		bDefaultSiblings = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	UPROPERTY(meta = (Input))
	bool bIncludeItem;

	/** When true, it will return all siblings, regardless of whether the parent is active or not.
	 * When false, will return only the siblings which are influenced by the same parent */
	UPROPERTY(meta = (Input))
	bool bDefaultSiblings;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Siblings;

	// Used to cache the internally used item
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedItem;

	// Used to cache the internally used siblings
	UPROPERTY()
	FTLLRN_RigElementKeyCollection CachedSiblings;
};

/**
 * Returns a chain between two items
 */
USTRUCT(meta=(DisplayName="Get Chain", Keywords="Chain,Siblings,Hierarchy", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetChainItemArray : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetChainItemArray()
	{
		Start = End = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		CachedStart = CachedEnd = FTLLRN_CachedTLLRN_RigElement();
		CachedChain = FTLLRN_RigElementKeyCollection();
		bIncludeStart = bIncludeEnd = true;
		bReverse = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Start;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey End;

	UPROPERTY(meta = (Input))
	bool bIncludeStart;

	UPROPERTY(meta = (Input))
	bool bIncludeEnd;

	UPROPERTY(meta = (Input))
	bool bReverse;
	
	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Chain;

	// Used to cache the internally used item
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedStart;

	// Used to cache the internally used item
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedEnd;

	// Used to cache the internally used siblings
	UPROPERTY()
	FTLLRN_RigElementKeyCollection CachedChain;
};

/**
 * Returns the hierarchy's pose
 */
USTRUCT(meta=(DisplayName="Get Pose Cache", Keywords="Hierarchy,Pose,State", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetPose : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetPose()
	{
		Initial = false;
		ElementType = ETLLRN_RigElementType::All;
		ItemsToGet = FTLLRN_RigElementKeyCollection();
		Pose = FTLLRN_RigPose();
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	bool Initial;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType ElementType;

	// An optional collection to filter against
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection ItemsToGet;

	UPROPERTY(meta = (Output))
	FTLLRN_RigPose Pose;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Returns the hierarchy's pose
 */
USTRUCT(meta=(DisplayName="Get Pose Cache", Keywords="Hierarchy,Pose,State", Varying, Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetPoseItemArray : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetPoseItemArray()
	{
		Initial = false;
		ElementType = ETLLRN_RigElementType::All;
		Pose = FTLLRN_RigPose();
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	bool Initial;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType ElementType;

	// An optional collection to filter against
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> ItemsToGet;

	UPROPERTY(meta = (Output))
	FTLLRN_RigPose Pose;
};


/**
 * Sets the hierarchy's pose
 */
USTRUCT(meta=(DisplayName="Apply Pose Cache", Keywords="Hierarchy,Pose,State", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchySetPose : public FTLLRN_RigUnit_HierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchySetPose()
	{
		Pose = FTLLRN_RigPose();
		ElementType = ETLLRN_RigElementType::All;
		Space = ERigVMTransformSpace::LocalSpace;
		ItemsToSet = FTLLRN_RigElementKeyCollection();
		Weight = 1.f;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose Pose;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType ElementType;

	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	// An optional collection to filter against
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection ItemsToSet;

	UPROPERTY(meta = (Input))
	float Weight;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Sets the hierarchy's pose
 */
USTRUCT(meta=(DisplayName="Apply Pose Cache", Keywords="Hierarchy,Pose,State", Varying, Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchySetPoseItemArray : public FTLLRN_RigUnit_HierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchySetPoseItemArray()
	{
		Pose = FTLLRN_RigPose();
		ElementType = ETLLRN_RigElementType::All;
		Space = ERigVMTransformSpace::LocalSpace;
		Weight = 1.f;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose Pose;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType ElementType;

	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	// An optional collection to filter against
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> ItemsToSet;

	UPROPERTY(meta = (Input))
	float Weight;
};

/**
* Returns true if the hierarchy pose is empty (has no items)
*/
USTRUCT(meta=(DisplayName="Is Pose Cache Empty", Keywords="Hierarchy,Pose,State", Varying, Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PoseIsEmpty : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PoseIsEmpty()
	{
		Pose = FTLLRN_RigPose();
		IsEmpty = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose Pose;

	UPROPERTY(meta = (Output))
	bool IsEmpty;
};

/**
* Returns the items in the hierarchy pose
*/
USTRUCT(meta=(DisplayName="Get Pose Cache Items", Keywords="Hierarchy,Pose,State", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PoseGetItems : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PoseGetItems()
	{
		Pose = FTLLRN_RigPose();
		ElementType = ETLLRN_RigElementType::All;
		Items = FTLLRN_RigElementKeyCollection();
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose Pose;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType ElementType;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Items;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Returns the items in the hierarchy pose
*/
USTRUCT(meta=(DisplayName="Get Pose Cache Items", Keywords="Hierarchy,Pose,State", Varying, Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PoseGetItemsItemArray : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PoseGetItemsItemArray()
	{
		Pose = FTLLRN_RigPose();
		ElementType = ETLLRN_RigElementType::All;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose Pose;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType ElementType;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
* Compares two pose caches and compares their values.
*/
USTRUCT(meta=(DisplayName="Get Pose Cache Delta", Keywords="Hierarchy,Pose,State", Varying, Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PoseGetDelta : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PoseGetDelta()
	{
		PoseA = PoseB = FTLLRN_RigPose();
		ElementType = ETLLRN_RigElementType::All;
		Space = ERigVMTransformSpace::LocalSpace;
		ItemsToCompare = ItemsWithDelta = FTLLRN_RigElementKeyCollection();
		PositionThreshold = 0.1f;
		RotationThreshold = ScaleThreshold = CurveThreshold = 0.f;
		PosesAreEqual = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose PoseA;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose PoseB;

	// The delta threshold for a translation / position difference. 0.0 disables position differences.
	UPROPERTY(meta = (Input))
	float PositionThreshold;

	// The delta threshold for a rotation difference (in degrees). 0.0 disables rotation differences.
	UPROPERTY(meta = (Input))
	float RotationThreshold;

	// The delta threshold for a scale difference. 0.0 disables scale differences.
	UPROPERTY(meta = (Input))
	float ScaleThreshold;

	// The delta threshold for curve value difference. 0.0 disables curve differences.
	UPROPERTY(meta = (Input))
	float CurveThreshold;

	UPROPERTY(meta = (Input))
	ETLLRN_RigElementType ElementType;
	
	// Defines in which space transform deltas should be computed
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	// An optional list of items to compare
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection ItemsToCompare;

	UPROPERTY(meta = (Output))
	bool PosesAreEqual;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection ItemsWithDelta;

	static bool ArePoseElementsEqual(
		const FTLLRN_RigPoseElement& A,
		const FTLLRN_RigPoseElement& B,
		ERigVMTransformSpace Space,
		float PositionU,
		float RotationU,
		float ScaleU,
		float CurveU);

	static bool AreTransformsEqual(
		const FTransform& A,
		const FTransform& B,
		float PositionU,
		float RotationU,
		float ScaleU);

	static bool AreCurvesEqual(
		float A,
		float B,
		float CurveU);
};

/**
* Returns the hierarchy's pose transform
*/
USTRUCT(meta=(DisplayName="Get Pose Cache Transform", Keywords="Hierarchy,Pose,State", Varying, Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PoseGetTransform : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PoseGetTransform()
	{
		Pose = FTLLRN_RigPose();
		Item = FTLLRN_RigElementKey();
		Space = ERigVMTransformSpace::GlobalSpace;
		Valid = false;
		Transform = FTransform::Identity;
		CurveValue = 0.f;
		CachedPoseElementIndex = CachedPoseHash = INDEX_NONE;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose Pose;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Item;

	/**
	* Defines if the transform should be retrieved in local or global space
	*/ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	UPROPERTY(meta = (Output))
	bool Valid;
	
	UPROPERTY(meta = (Output))
	FTransform Transform;

	UPROPERTY(meta = (Output))
	float CurveValue;

	UPROPERTY()
	int32 CachedPoseElementIndex;

	UPROPERTY()
	int32 CachedPoseHash;
};

/**
* Returns an array of transforms from a given hierarchy pose
*/
USTRUCT(meta=(DisplayName="Get Pose Cache Transform Array", Keywords="Hierarchy,Pose,State", Varying, Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PoseGetTransformArray : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PoseGetTransformArray()
	{
		Pose = FTLLRN_RigPose();
		Space = ERigVMTransformSpace::GlobalSpace;
		Valid = false;
		Transforms.Reset();
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose Pose;

	/**
	* Defines if the transform should be retrieved in local or global space
	*/ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	UPROPERTY(meta = (Output))
	bool Valid;
	
	UPROPERTY(meta = (Output))
	TArray<FTransform> Transforms;
};

/**
* Returns the hierarchy's pose curve value
*/
USTRUCT(meta=(DisplayName="Get Pose Cache Curve", Keywords="Hierarchy,Pose,State", Varying, Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PoseGetCurve : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PoseGetCurve()
	{
		Pose = FTLLRN_RigPose();
		Curve = NAME_None;
		Valid = false;
		CurveValue = 0.f;
		CachedPoseElementIndex = CachedPoseHash = INDEX_NONE;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigPose Pose;

	UPROPERTY(meta = (Input, CustomWidget = "CurveName"))
	FName Curve;

	UPROPERTY(meta = (Output))
	bool Valid;
	
	UPROPERTY(meta = (Output))
	float CurveValue;

	UPROPERTY()
	int32 CachedPoseElementIndex;

	UPROPERTY()
	int32 CachedPoseHash;
};

/**
* Given a pose, execute iteratively across all items in the pose
*/
USTRUCT(meta=(DisplayName="For Each Pose Cache Element", Keywords="Collection,Loop,Iterate", Icon="EditorStyle|GraphEditor.Macro.ForEach_16x", Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_PoseLoop : public FTLLRN_RigUnit_HierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PoseLoop()
	{
		BlockToRun = NAME_None;
		Pose = FTLLRN_RigPose();
		Item = FTLLRN_RigElementKey();
		GlobalTransform = LocalTransform = FTransform::Identity;
		CurveValue = 0.f;
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
	FTLLRN_RigPose Pose;

	UPROPERTY(meta = (Singleton, Output))
	FTLLRN_RigElementKey Item;

	UPROPERTY(meta = (Singleton, Output))
	FTransform GlobalTransform;

	UPROPERTY(meta = (Singleton, Output))
	FTransform LocalTransform;

	UPROPERTY(meta = (Singleton, Output))
	float CurveValue;

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
};

USTRUCT(BlueprintType)
struct FTLLRN_RigUnit_HierarchyCreatePoseItemArray_Entry
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyCreatePoseItemArray_Entry()
	: Item(NAME_None, ETLLRN_RigElementType::Bone)
	, LocalTransform(FTransform::Identity)
	, GlobalTransform(FTransform::Identity)
	, UseEulerAngles(false)
	, EulerAngles(FVector::ZeroVector)
	, CurveValue(0.f)
	{
	}

	UPROPERTY(BlueprintReadWrite, Category = "Entry")
	FTLLRN_RigElementKey Item;

	UPROPERTY(BlueprintReadWrite, Category = "Entry")
	FTransform LocalTransform;

	UPROPERTY(BlueprintReadWrite, Category = "Entry")
	FTransform GlobalTransform;

	// in case of a control this can be used to drive the preferred euler angles
	UPROPERTY(BlueprintReadWrite, Category = "Entry")
	bool UseEulerAngles;

	// in case of a control this can be used to drive the preferred euler angles
	UPROPERTY(BlueprintReadWrite, Category = "Entry")
	FVector EulerAngles;

	// in case of a curve this can be used to drive the curve value
	UPROPERTY(BlueprintReadWrite, Category = "Entry")
	float CurveValue;
};


/**
 * Creates the hierarchy's pose
 */
USTRUCT(meta=(DisplayName="Create Pose Cache", Keywords="Hierarchy,Pose,State,MakePoseCache,NewPoseCache,EmptyPoseCache", Varying, Category = "Pose Cache"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyCreatePoseItemArray : public FTLLRN_RigUnit_HierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyCreatePoseItemArray()
	{
		Pose = FTLLRN_RigPose();
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The entries to create
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigUnit_HierarchyCreatePoseItemArray_Entry> Entries;

	UPROPERTY(meta = (Output))
	FTLLRN_RigPose Pose;
};
