// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/Highlevel/TLLRN_RigUnit_HighlevelBase.h"

#include "Units/Highlevel/Hierarchy/TLLRN_RigUnit_TransformConstraint.h"

#include "TLLRN_RigUnit_AimBone.generated.h"

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimBone_Target
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AimBone_Target()
	{
		Weight = 1.f;
		Axis = FVector(1.f, 0.f, 0.f);
		Target = FVector(1.f, 0.f, 0.f);
		Kind = ETLLRN_ControlTLLRN_RigVectorKind::Location;
		Space = NAME_None;
	}

	/**
	 * The amount of aim rotation to apply on this target.
	 */
	UPROPERTY(EditAnywhere, meta = (Input), Category = "TLLRN_AimTarget")
	float Weight;

	/**
	 * The axis to align with the aim on this target
	 */
	UPROPERTY(EditAnywhere, meta = (Input, EditCondition = "Weight > 0.0"), Category = "TLLRN_AimTarget")
	FVector Axis;

	/**
	 * The target to aim at - can be a direction or location based on the Kind setting
	 */
	UPROPERTY(EditAnywhere, meta = (Input, EditCondition = "Weight > 0.0"), Category = "TLLRN_AimTarget")
	FVector Target;

	/**
	 * The kind of target this is representing - can be a direction or a location
	 */
	UPROPERTY(EditAnywhere, meta = (Input, EditCondition = "Weight > 0.0"), Category = "TLLRN_AimTarget")
	ETLLRN_ControlTLLRN_RigVectorKind Kind;

	/**
	 * The space in which the target is expressed
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Target Space", Input, EditCondition = "Weight > 0.0" ), Category = "TLLRN_AimTarget")
	FName Space;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimItem_Target
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AimItem_Target()
	{
		Weight = 1.f;
		Axis = FVector(1.f, 0.f, 0.f);
		Target = FVector(1.f, 0.f, 0.f);
		Kind = ETLLRN_ControlTLLRN_RigVectorKind::Location;
		Space = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
	}

	/**
	 * The amount of aim rotation to apply on this target.
	 */
	UPROPERTY(EditAnywhere, meta = (Input), Category = "TLLRN_AimTarget")
	float Weight;

	/**
	 * The axis to align with the aim on this target
	 */
	UPROPERTY(EditAnywhere, meta = (Input, EditCondition = "Weight > 0.0"), Category = "TLLRN_AimTarget")
	FVector Axis;

	/**
	 * The target to aim at - can be a direction or location based on the Kind setting
	 */
	UPROPERTY(EditAnywhere, meta = (Input, EditCondition = "Weight > 0.0"), Category = "TLLRN_AimTarget")
	FVector Target;

	/**
	 * The kind of target this is representing - can be a direction or a location
	 */
	UPROPERTY(EditAnywhere, meta = (Input, EditCondition = "Weight > 0.0"), Category = "TLLRN_AimTarget")
	ETLLRN_ControlTLLRN_RigVectorKind Kind;

	/**
	 * The space in which the target is expressed
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Target Space", Input, EditCondition = "Weight > 0.0" ), Category = "TLLRN_AimTarget")
	FTLLRN_RigElementKey Space;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimBone_DebugSettings
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AimBone_DebugSettings()
	{
		bEnabled = false;
		Scale = 10.f;
		WorldOffset = FTransform::Identity;
	}

	/**
	 * If enabled debug information will be drawn 
	 */
	UPROPERTY(EditAnywhere, meta = (Input), Category = "DebugSettings")
	bool bEnabled;

	/**
	 * The size of the debug drawing information
	 */
	UPROPERTY(EditAnywhere, meta = (Input, EditCondition = "bEnabled"), Category = "DebugSettings")
	float Scale;

	/**
	 * The offset at which to draw the debug information in the world
	 */
	UPROPERTY(EditAnywhere, meta = (Input, EditCondition = "bEnabled"), Category = "DebugSettings")
	FTransform WorldOffset;
};

/**
 * Outputs an aligned transform of a primary and secondary axis of an input transform to a world target.
 * Note: This node operates in world space!
 */
USTRUCT(meta = (DisplayName = "Aim Math", Category = "Hierarchy", Keywords = "Lookat"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimBoneMath : public FTLLRN_RigUnit_HighlevelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AimBoneMath()
	{
		InputTransform = FTransform::Identity;
		Primary = FTLLRN_RigUnit_AimItem_Target();
		Secondary = FTLLRN_RigUnit_AimItem_Target();
		Primary.Axis = FVector(1.f, 0.f, 0.f);
		Secondary.Axis = FVector(0.f, 0.f, 1.f);
		Weight = 1.f;
		DebugSettings = FTLLRN_RigUnit_AimBone_DebugSettings();
		PrimaryCachedSpace = FTLLRN_CachedTLLRN_RigElement();
		SecondaryCachedSpace = FTLLRN_CachedTLLRN_RigElement();
		bIsInitialized = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The transform (in global space) before the aim was applied (optional)
	 */
	UPROPERTY(meta = (Input))
	FTransform InputTransform;

	/**
	 * The primary target for the aim
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigUnit_AimItem_Target Primary;

	/**
	 * The secondary target for the aim - also referred to as PoleVector / UpVector
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_AimItem_Target Secondary;

	/**
	 * The weight of the change - how much the change should be applied
	 */
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	/**
	 * The resulting transform
	 */
	UPROPERTY(meta = (Output))
	FTransform Result;

	/** The debug setting for the node */
	UPROPERTY(meta = (Input, DetailsOnly))
	FTLLRN_RigUnit_AimBone_DebugSettings DebugSettings;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement PrimaryCachedSpace;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement SecondaryCachedSpace;

	UPROPERTY()
	bool bIsInitialized;
};

/**
 * Aligns the rotation of a primary and secondary axis of a bone to a global target.
 * Note: This node operates in global space!
 */
USTRUCT(meta=(DisplayName="Aim", Category="Hierarchy", Keywords="Lookat", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimBone : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AimBone()
	{
		Bone = NAME_None;
		Primary = FTLLRN_RigUnit_AimBone_Target();
		Secondary = FTLLRN_RigUnit_AimBone_Target();
		Primary.Axis = FVector(1.f, 0.f, 0.f);
		Secondary.Axis = FVector(0.f, 0.f, 1.f);
		Weight = 1.f;
		bPropagateToChildren = true;
		DebugSettings = FTLLRN_RigUnit_AimBone_DebugSettings();
		CachedBoneIndex = FTLLRN_CachedTLLRN_RigElement();
		PrimaryCachedSpace = FTLLRN_CachedTLLRN_RigElement();
		SecondaryCachedSpace = FTLLRN_CachedTLLRN_RigElement();
		bIsInitialized = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/** 
	 * The name of the bone to align
	 */
	UPROPERTY(meta = (Input))
	FName Bone;

	/**
	 * The primary target for the aim 
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigUnit_AimBone_Target Primary;

	/**
	 * The secondary target for the aim - also referred to as PoleVector / UpVector
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_AimBone_Target Secondary;

	/**
	 * The weight of the change - how much the change should be applied
	 */
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	/**
	 * If set to true all of the global transforms of the children
	 * of this bone will be recalculated based on their local transforms.
	 * Note: This is computationally more expensive than turning it off.
	 */
	UPROPERTY(meta = (Input, Constant))
	bool bPropagateToChildren;

	/** The debug setting for the node */
	UPROPERTY(meta = (Input, DetailsOnly))
	FTLLRN_RigUnit_AimBone_DebugSettings DebugSettings;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedBoneIndex;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement PrimaryCachedSpace;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement SecondaryCachedSpace;

	UPROPERTY()
	bool bIsInitialized;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Aligns the rotation of a primary and secondary axis of a bone to a global target.
 * Note: This node operates in global space!
 */
USTRUCT(meta=(DisplayName="Aim", Category="Hierarchy", Keywords="Lookat"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimItem: public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AimItem()
	{
		Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		Primary = FTLLRN_RigUnit_AimItem_Target();
		Secondary = FTLLRN_RigUnit_AimItem_Target();
		Primary.Axis = FVector(1.f, 0.f, 0.f);
		Secondary.Axis = FVector(0.f, 0.f, 1.f);
		Weight = 1.f;
		DebugSettings = FTLLRN_RigUnit_AimBone_DebugSettings();
		CachedItem = FTLLRN_CachedTLLRN_RigElement();
		PrimaryCachedSpace = FTLLRN_CachedTLLRN_RigElement();
		SecondaryCachedSpace = FTLLRN_CachedTLLRN_RigElement();
		bIsInitialized = false;
	}

#if WITH_EDITOR
	virtual bool GetDirectManipulationTargets(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, UTLLRN_RigHierarchy* InHierarchy, TArray<FRigDirectManipulationTarget>& InOutTargets, FString* OutFailureReason) const override;
	virtual TArray<const URigVMPin*> GetPinsForDirectManipulation(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget) const override;
	virtual bool UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
	virtual bool UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) override;
#endif
	
	RIGVM_METHOD()
	virtual void Execute() override;

	/** 
	 * The name of the item to align
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The primary target for the aim 
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigUnit_AimItem_Target Primary;

	/**
	 * The secondary target for the aim - also referred to as PoleVector / UpVector
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_AimItem_Target Secondary;

	/**
	 * The weight of the change - how much the change should be applied
	 */
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight;

	/** The debug setting for the node */
	UPROPERTY(meta = (Input, DetailsOnly))
	FTLLRN_RigUnit_AimBone_DebugSettings DebugSettings;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedItem;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement PrimaryCachedSpace;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement SecondaryCachedSpace;

	UPROPERTY()
	bool bIsInitialized;
};


USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimConstraint_WorldUp
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AimConstraint_WorldUp()
	{
		Target = FVector(0.f, 0.f, 1.f);
		Kind = ETLLRN_ControlTLLRN_RigVectorKind::Direction;
		Space = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::None);
	}

	/**
	 * The target to aim at - can be a direction or location based on the Kind setting
	 */
	UPROPERTY(meta = (Input))
	FVector Target;

	/**
	 * The kind of target this is representing - can be a direction or a location
	 */
	UPROPERTY(meta = (DisplayName="Target is ", Input))
	ETLLRN_ControlTLLRN_RigVectorKind Kind;

	/**
	 * The space in which the target is expressed, use None to indicate world space
	 */
	UPROPERTY(meta = (DisplayName = "Target Space", Input))
	FTLLRN_RigElementKey Space;
};


USTRUCT()
struct FTLLRN_RigUnit_AimConstraint_AdvancedSettings
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AimConstraint_AdvancedSettings()
		:RotationOrderForFilter(EEulerRotationOrder::XZY)
	{}
	
	/**
	*	Settings related to debug drawings
	*/
	UPROPERTY(meta = (Input,DetailsOnly))
	FTLLRN_RigUnit_AimBone_DebugSettings DebugSettings;
	
	/**
	*	Rotation is converted to euler angles using the specified order such that individual axes can be filtered.
	*/
	UPROPERTY(meta = (Input))
	EEulerRotationOrder RotationOrderForFilter;	
};

/**
 * Orients an item such that its aim axis points towards a global target.
 * Note: This node operates in global space!
 */
USTRUCT(meta=(DisplayName="Aim Constraint", Category="Constraints", Keywords="Lookat, Aim"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AimConstraintLocalSpaceOffset: public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AimConstraintLocalSpaceOffset()
		: Child(FTLLRN_RigElementKey())
		, bMaintainOffset(true)
		, Filter(FFilterOptionPerAxis())
		, AimAxis(FVector(1.0f,0.0f,0.0f))
		, UpAxis(FVector(0.0f,0.0f,1.0f))
		, WorldUp(FTLLRN_RigUnit_AimConstraint_WorldUp())
		, AdvancedSettings(FTLLRN_RigUnit_AimConstraint_AdvancedSettings())
		, Weight(1.0f)
		, bIsInitialized(false)
	{
		Parents.Add(FTLLRN_ConstraintParent());
	};
	
	RIGVM_METHOD()
	virtual void Execute() override;

	/** 
	 * The name of the item to apply aim
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/**
	 *	Maintains the offset between child and weighted average of parents based on initial transforms
	 */
	UPROPERTY(meta = (Input))
	bool bMaintainOffset;

	/**
	 * Filters the final rotation by axes based on the euler rotation order defined in the node's advanced settings
	 * If flipping is observed, try adjusting the rotation order
	 */
	UPROPERTY(meta = (Input))
	FFilterOptionPerAxis Filter;

	/**
	 * Child is rotated so that its AimAxis points to the parents
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FVector AimAxis;

	/**
	 * Child is rotated around the AimAxis so that its UpAxis points to/Aligns with the WorldUp target 
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FVector UpAxis;

	/**
	 * Defines how Child should rotate around the AimAxis. This is the aim target for the UpAxis
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigUnit_AimConstraint_WorldUp WorldUp;
	
	UPROPERTY(meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintParent> Parents;

	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_AimConstraint_AdvancedSettings AdvancedSettings;
	
	UPROPERTY(meta = (Input))
	float Weight;
	
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement WorldUpSpaceCache; 

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement ChildCache;

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> ParentCaches;
	
	UPROPERTY()
	bool bIsInitialized;
};
