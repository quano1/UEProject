// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "Units/Highlevel/TLLRN_RigUnit_HighlevelBase.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "Constraint.h"
#include "TLLRN_ControlRigDefines.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"

#include "TLLRN_RigUnit_TransformConstraint.generated.h"


USTRUCT()
struct FTLLRN_ConstraintTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "FTLLRN_ConstraintTarget")
	FTransform Transform;

	UPROPERTY(EditAnywhere, Category = "FTLLRN_ConstraintTarget")
	float Weight;

	UPROPERTY(EditAnywhere, Category = "FTLLRN_ConstraintTarget")
	bool bMaintainOffset;

	UPROPERTY(EditAnywhere, Category = "FTLLRN_ConstraintTarget", meta = (Constant))
	FTransformFilter Filter;

	FTLLRN_ConstraintTarget()
		: Weight (1.f)
		, bMaintainOffset(true)
	{}
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_TransformConstraint_WorkData
{
	GENERATED_BODY()

	// note that Targets.Num () != ConstraintData.Num()
	UPROPERTY()
	TArray<FConstraintData>	ConstraintData;

	UPROPERTY()
	TMap<int32, int32> ConstraintDataToTargets;
};

USTRUCT(meta=(DisplayName="Transform Constraint", Category="Transforms", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_TransformConstraint : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_TransformConstraint()
		: BaseTransformSpace(ETLLRN_TransformSpaceMode::GlobalSpace)
		, bUseInitialTransforms(true)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	FName Bone;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	ETLLRN_TransformSpaceMode BaseTransformSpace;

	// Transform op option. Use if ETransformSpace is BaseTransform
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	FTransform BaseTransform;

	// Transform op option. Use if ETransformSpace is BaseJoint
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	FName BaseBone;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintTarget> Targets;

	// If checked the initial transform will be used for the constraint data
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, Constant))
	bool bUseInitialTransforms;

private:

	UPROPERTY(transient)
	FTLLRN_RigUnit_TransformConstraint_WorkData WorkData;

public:
	
	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * Constrains an item's transform to multiple items' transforms
 */
USTRUCT(meta=(DisplayName="Transform Constraint", Category="Transforms", Keywords = "Parent,Orient,Scale", Deprecated="5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_TransformConstraintPerItem : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_TransformConstraintPerItem()
		: BaseTransformSpace(ETLLRN_TransformSpaceMode::GlobalSpace)
		, bUseInitialTransforms(true)
	{
		Item = BaseItem = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	ETLLRN_TransformSpaceMode BaseTransformSpace;

	// Transform op option. Use if ETransformSpace is BaseTransform
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	FTransform BaseTransform;

	// Transform op option. Use if ETransformSpace is BaseJoint
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	FTLLRN_RigElementKey BaseItem;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintTarget> Targets;

	// If checked the initial transform will be used for the constraint data
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, Constant))
	bool bUseInitialTransforms;

private:
	static void AddConstraintData(const TArrayView<const FTLLRN_ConstraintTarget>& Targets, ETransformConstraintType ConstraintType, const int32 TargetIndex, const FTransform& SourceTransform, const FTransform& InBaseTransform, TArray<FConstraintData>& OutConstraintData, TMap<int32, int32>& OutConstraintDataToTargets);

	UPROPERTY(transient)
	FTLLRN_RigUnit_TransformConstraint_WorkData WorkData;

public:

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

USTRUCT()
struct FTLLRN_ConstraintParent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "FTLLRN_ConstraintParent", meta = (Input))
	FTLLRN_RigElementKey Item;

	UPROPERTY(EditAnywhere, Category = "FTLLRN_ConstraintParent", meta = (Input))
	float Weight;

	FTLLRN_ConstraintParent()
        : Item(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone))
        , Weight (1.f)
	{}
	
	FTLLRN_ConstraintParent(const FTLLRN_RigElementKey InItem, const float InWeight)
		: Item(InItem)
		, Weight(InWeight)
	{}
};

/**
*	Options for interpolating rotations
*/ 
UENUM(BlueprintType)
enum class ETLLRN_ConstraintInterpType : uint8
{ 
	/** Weighted Average of Quaternions by their X,Y,Z,W values, The Shortest Route is Respected. The Order of Parents Doesn't Matter */
	Average UMETA(DisplayName="Average(Order Independent)"),
   /** Perform Quaternion Slerp in Sequence, Different Orders of Parents can Produce Different Results */ 
    Shortest UMETA(DisplayName="Shortest(Order Dependent)"),
	
    Max UMETA(Hidden),
};

USTRUCT()
struct FTLLRN_RigUnit_ParentConstraint_AdvancedSettings
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ParentConstraint_AdvancedSettings()
		: InterpolationType(ETLLRN_ConstraintInterpType::Average)
		, RotationOrderForFilter(EEulerRotationOrder::XZY)
	{}
	
	/**
	*	Options for interpolating rotations
	*/
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	ETLLRN_ConstraintInterpType InterpolationType;
	
	/**
	*	Rotation is converted to euler angles using the specified order such that individual axes can be filtered.
	*/
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	EEulerRotationOrder RotationOrderForFilter;
};

/**
* Constrains an item's transform to multiple items' transforms
*/
USTRUCT(meta=(DisplayName="Parent Constraint", Category="Constraints", Keywords = "Parent,Orient,Scale"))
struct FTLLRN_RigUnit_ParentConstraint : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ParentConstraint()
        : Child(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone))
        , bMaintainOffset(true)
		, Weight(1.0f)
		, ChildCache()
		, ParentCaches()
	{
		Parents.Add(FTLLRN_ConstraintParent());
	}
	
	RIGVM_METHOD()
    virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	bool bMaintainOffset;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FTransformFilter Filter;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintParent> Parents;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FTLLRN_RigUnit_ParentConstraint_AdvancedSettings AdvancedSettings;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	float Weight;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement ChildCache;

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> ParentCaches;
};

USTRUCT()
struct FTLLRN_RigUnit_ParentConstraintMath_AdvancedSettings
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ParentConstraintMath_AdvancedSettings()
		: InterpolationType(ETLLRN_ConstraintInterpType::Average)
	{}
	
	/**
	*	Options for interpolating rotations
	*/
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	ETLLRN_ConstraintInterpType InterpolationType;
};

/**
* Computes the output transform by constraining the input transform to multiple parents 
*/
USTRUCT(meta=(DisplayName="Parent Constraint Math", Category="Constraints", Keywords = "Parent,Orient,Scale"))
struct FTLLRN_RigUnit_ParentConstraintMath : public FTLLRN_RigUnit_HighlevelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ParentConstraintMath()
		: Input(FTransform::Identity)
		, Output(FTransform::Identity)
	{
		Parents.Add(FTLLRN_ConstraintParent());
	}
	
	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	*	Input is used to calculate offsets from parents' initial transform
	*/
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault))
	FTransform Input;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintParent> Parents;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FTLLRN_RigUnit_ParentConstraintMath_AdvancedSettings AdvancedSettings;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Output))
	FTransform Output;

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> ParentCaches;
};

/**
* Constrains an item's position to multiple items' positions 
*/
USTRUCT(meta=(DisplayName="Position Constraint", Category="Constraints", Keywords = "Parent,Translation", Deprecated = "5.0"))
struct FTLLRN_RigUnit_PositionConstraint : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PositionConstraint()
        : Child(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone))
        , bMaintainOffset(true)
		, Weight(1.0f)
	{
		Parents.Add(FTLLRN_ConstraintParent());
	}
	
	RIGVM_METHOD()
    virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	bool bMaintainOffset;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FFilterOptionPerAxis Filter;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintParent> Parents;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	float Weight;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Constrains an item's position to multiple items' positions 
*/
USTRUCT(meta=(DisplayName="Position Constraint", Category="Constraints", Keywords = "Parent,Translation"))
struct FTLLRN_RigUnit_PositionConstraintLocalSpaceOffset : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_PositionConstraintLocalSpaceOffset()
		: Child(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone))
		, bMaintainOffset(true)
		, Weight(1.0f)
		, ChildCache()
		, ParentCaches()
	{
		Parents.Add(FTLLRN_ConstraintParent());
	}
	
	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	bool bMaintainOffset;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FFilterOptionPerAxis Filter;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintParent> Parents;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	float Weight;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement ChildCache;

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> ParentCaches;
};


USTRUCT()
struct FTLLRN_RigUnit_RotationConstraint_AdvancedSettings
{
	GENERATED_BODY()

	FTLLRN_RigUnit_RotationConstraint_AdvancedSettings()
        : InterpolationType(ETLLRN_ConstraintInterpType::Average)
        , RotationOrderForFilter(EEulerRotationOrder::XZY)
	{}
	
	/**
	*	Options for interpolating rotations
	*/
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	ETLLRN_ConstraintInterpType InterpolationType;
	
	/**
	*	Rotation is converted to euler angles using the specified order such that individual axes can be filtered.
	*/
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	EEulerRotationOrder RotationOrderForFilter;
};

/**
* Constrains an item's rotation to multiple items' rotations 
*/
USTRUCT(meta=(DisplayName="Rotation Constraint", Category="Constraints", Keywords = "Parent,Orientation,Orient,Rotate", Deprecated = "5.0"))
struct FTLLRN_RigUnit_RotationConstraint : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_RotationConstraint()
        : Child(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone))
        , bMaintainOffset(true)
		, Weight(1.0f)
	{
		Parents.Add(FTLLRN_ConstraintParent());
	}
	
	RIGVM_METHOD()
    virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	bool bMaintainOffset;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FFilterOptionPerAxis Filter;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintParent> Parents;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FTLLRN_RigUnit_RotationConstraint_AdvancedSettings AdvancedSettings;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	float Weight;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Constrains an item's rotation to multiple items' rotations 
*/
USTRUCT(meta=(DisplayName="Rotation Constraint", Category="Constraints", Keywords = "Parent,Orientation,Orient,Rotate"))
struct FTLLRN_RigUnit_RotationConstraintLocalSpaceOffset : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_RotationConstraintLocalSpaceOffset()
		: Child(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone))
		, bMaintainOffset(true)
		, Weight(1.0f)
		, ChildCache()
		, ParentCaches()
	{
		Parents.Add(FTLLRN_ConstraintParent());
	}
	
	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	bool bMaintainOffset;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FFilterOptionPerAxis Filter;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintParent> Parents;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FTLLRN_RigUnit_RotationConstraint_AdvancedSettings AdvancedSettings;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	float Weight;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement ChildCache;

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> ParentCaches;
};

/**
* Constrains an item's scale to multiple items' scales
*/
USTRUCT(meta=(DisplayName="Scale Constraint", Category="Constraints", Keywords = "Parent, Size", Deprecated = "5.0"))
struct FTLLRN_RigUnit_ScaleConstraint : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ScaleConstraint()
        : Child(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone))
        , bMaintainOffset(true)
		, Weight(1.0f)
	{
		Parents.Add(FTLLRN_ConstraintParent());
	}
	
	RIGVM_METHOD()
    virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	bool bMaintainOffset;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FFilterOptionPerAxis Filter;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintParent> Parents;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	float Weight;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Constrains an item's scale to multiple items' scales
*/
USTRUCT(meta=(DisplayName="Scale Constraint", Category="Constraints", Keywords = "Parent, Size"))
struct FTLLRN_RigUnit_ScaleConstraintLocalSpaceOffset : public FTLLRN_RigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ScaleConstraintLocalSpaceOffset()
		: Child(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone))
		, bMaintainOffset(true)
		, Weight(1.0f)
		, ChildCache()
		, ParentCaches()
	{
		Parents.Add(FTLLRN_ConstraintParent());
	}
	
	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;
	
	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	bool bMaintainOffset;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input)) 
	FFilterOptionPerAxis Filter;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input, ExpandByDefault, DefaultArraySize = 1))
	TArray<FTLLRN_ConstraintParent> Parents;

	UPROPERTY(EditAnywhere, Category = "Constraint", meta = (Input))
	float Weight;

	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement ChildCache;

	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> ParentCaches;
};
