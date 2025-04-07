// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h" 
#include "TLLRN_RigUnit_DynamicHierarchy.generated.h"

USTRUCT(meta = (Abstract, NodeColor="0.262745, 0.8, 0, 0.229412", Category = "DynamicHierarchy"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DynamicHierarchyBase : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	static bool IsValidToRunInContext(
		const FTLLRN_ControlRigExecuteContext& InExecuteContext,
		bool bAllowOnlyConstructionEvent,
		FString* OutErrorMessage = nullptr);
};

USTRUCT(meta = (Abstract, NodeColor="0.262745, 0.8, 0, 0.229412", Category = "DynamicHierarchy"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DynamicHierarchyBaseMutable : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()
};

/**
* Adds a new parent to an element. The weight for the new parent will be 0.0.
* You can use the SetParentWeights node to change the parent weights later.
*/
USTRUCT(meta=(DisplayName="Add Parent", Keywords="Children,Parent,Constraint,Space", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_AddParent : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_AddParent()
	{
		Child = Parent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/*
	 * The child to be parented under the new parent
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/*
	 * The new parent to be added to the child
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Parent;
};

/**
 * Changes the default parent for an item - this removes all other current parents.
 */
USTRUCT(meta=(DisplayName="Set Default Parent", Keywords="Children,Parent,Constraint,Space,SetParent,AddParent", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetDefaultParent : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetDefaultParent()
	{
		Child = Parent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/*
	 * The child to be parented under the new default parent
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/*
	 * The default parent to be used for the child
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Parent;
};

/**
 * Allows an animation channel to be hosted by multiple controls
 */
USTRUCT(meta=(DisplayName="Set Channel Hosts", Keywords="Children,Parent,Constraint,Space,SetParent,AddParent,Channel", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetChannelHosts : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetChannelHosts()
	{
		Channel = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/*
	 * The channel to receive more hosts
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Channel;

	/*
	 * The hosts to add to the channel
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	TArray<FTLLRN_RigElementKey> Hosts;
};

UENUM(meta = (RigVMTypeAllowed))
enum class TLLRN_ERigSwitchParentMode : uint8
{
	/** Switches the element to be parented to the world */
	World,

	/** Switches back to the original / default parent */
	DefaultParent,

	/** Switches the child to the provided parent item */
	ParentItem
};

/**
* Switches an element to a new parent.
*/
USTRUCT(meta=(DisplayName="Switch Parent", Keywords="Children,Parent,Constraint,Space,Switch", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SwitchParent : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SwitchParent()
	{
		Mode = TLLRN_ERigSwitchParentMode::ParentItem;
		Child = Parent = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
		bMaintainGlobal = true;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/* Depending on this the child will switch to the world,
	 * back to its default or to the item provided by the Parent pin
	 */
	UPROPERTY(meta = (Input))
	TLLRN_ERigSwitchParentMode Mode;

	/* The child to switch to a new parent */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/* The optional parent to switch to. This is only used if the mode is set to 'Parent Item' */
	UPROPERTY(meta = (Input, ExpandByDefault, EditCondition="Mode==ParentItem"))
	FTLLRN_RigElementKey Parent;

	/* If set to true the item will maintain its global transform,
	 * otherwise it will maintain local
	 */
	UPROPERTY(meta = (Input))
	bool bMaintainGlobal;
};

/**
* Returns the item's parents' weights
*/
USTRUCT(meta=(DisplayName="Get Parent Weights", Keywords="Chain,Parents,Hierarchy", Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetParentWeights : public FTLLRN_RigUnit_DynamicHierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetParentWeights()
	{
		Child = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/*
	 * The child to retrieve the weights for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/*
	 * The weight of each parent
	 */
	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementWeight> Weights;

	/*
	 * The key for each parent
	 */
	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKeyCollection Parents;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Returns the item's parents' weights
*/
USTRUCT(meta=(DisplayName="Get Parent Weights", Keywords="Chain,Parents,Hierarchy", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetParentWeightsArray : public FTLLRN_RigUnit_DynamicHierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetParentWeightsArray()
	{
		Child = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/*
	 * The child to retrieve the weights for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/*
	 * The weight of each parent
	 */
	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementWeight> Weights;

	/*
	 * The key for each parent
	 */
	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Parents;
};

/**
* Sets the item's parents' weights
*/
USTRUCT(meta=(DisplayName="Set Parent Weights", Keywords="Chain,Parents,Hierarchy", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchySetParentWeights : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchySetParentWeights()
	{
		Child = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/*
	 * The child to set the parents' weights for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Child;

	/*
	 * The weights to set for the child's parents.
	 * The number of weights needs to match the current number of parents.
	 */
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementWeight> Weights;
};

/**
 * Removes all elements from the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Reset Hierarchy", Keywords="Delete,Erase,Clear,Empty,RemoveAll", Varying))
struct FTLLRN_RigUnit_HierarchyReset : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyReset()
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Imports all bones (and curves) from the currently assigned skeleton.
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Import Skeleton", Keywords="Skeleton,Mesh,AddBone,AddCurve,Spawn", Varying))
struct FTLLRN_RigUnit_HierarchyImportFromSkeleton : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyImportFromSkeleton()
	{
		NameSpace = NAME_None;
		bIncludeCurves = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FName NameSpace;

	UPROPERTY(meta = (Input))
	bool bIncludeCurves;

	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
 * Removes an element from the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Remove Item", Keywords="Delete,Erase,Joint,Skeleton", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyRemoveElement : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyRemoveElement()
	{
		Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		bSuccess = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/*
	 * The item to remove
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/*
	 * True if the element has been removed successfuly 
	 */
	UPROPERTY(meta = (Output, ExpandByDefault))
	bool bSuccess;
};

USTRUCT(meta=(Abstract))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddElement : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddElement()
	{
		Parent = Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone);
		Name = NAME_None;
	}

	virtual ETLLRN_RigElementType GetElementTypeToSpawn() const { return ETLLRN_RigElementType::None; }

	/*
	 * The parent of the new element to add
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Parent;

	/*
	 * The name of the new element to add
	 */
	UPROPERTY(meta = (Input))
	FName Name;

	/*
	 * The resulting item
	 */
	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKey Item;
};

/**
 * Adds a new bone to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Bone", Keywords="Construction,Create,New,Joint,Skeleton", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddBone : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddBone()
	{
		Name = TEXT("NewBone");
		Transform = FTransform::Identity;
		Space = ERigVMTransformSpace::LocalSpace;
	}

	virtual ETLLRN_RigElementType GetElementTypeToSpawn() const override { return ETLLRN_RigElementType::Bone; }

	/*
	 * The initial transform of the new element
	 */
	UPROPERTY(meta = (Input))
	FTransform Transform;

	/**
	 * Defines if the transform should be interpreted in local or global space
	 */ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Adds a new null to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Null", Keywords="Construction,Create,New,Locator,Group", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddNull : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddNull()
	{
		Name = TEXT("NewNull");
		Transform = FTransform::Identity;
		Space = ERigVMTransformSpace::LocalSpace;
	}

	virtual ETLLRN_RigElementType GetElementTypeToSpawn() const override { return ETLLRN_RigElementType::Null; }

	/*
	 * The initial transform of the new element
	 */
	UPROPERTY(meta = (Input))
	FTransform Transform;

	/**
	 * Defines if the transform should be interpreted in local or global space
	 */ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControl_Settings
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControl_Settings()
		: DisplayName(NAME_None)
	{}
	virtual ~FTLLRN_RigUnit_HierarchyAddControl_Settings(){}

	virtual void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings);
	virtual void Configure(FTLLRN_RigControlSettings& OutSettings) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FName DisplayName;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings()
		: bVisible(true)
		, Name(TEXT("Default"))
		, Color(FLinearColor::Red)
		, Transform(FTransform::Identity)
	{}

	void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings);
	void Configure(FTLLRN_RigControlSettings& OutSettings) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta=(CustomWidget = "ShapeName"))
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FLinearColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTransform Transform;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControl_ProxySettings
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControl_ProxySettings()
		: bIsProxy(false)
		, ShapeVisibility(ETLLRN_RigControlVisibility::BasedOnSelection)
	{}

	void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings);
	void Configure(FTLLRN_RigControlSettings& OutSettings) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bIsProxy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<FTLLRN_RigElementKey> DrivenControls;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	ETLLRN_RigControlVisibility ShapeVisibility;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlFloat_LimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddControlFloat_LimitSettings()
		: Limit(true, true)
		, MinValue(0.f)
		, MaxValue(100.f)
		, bDrawLimits(true)
	{}

	void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings);
	void Configure(FTLLRN_RigControlSettings& OutSettings) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled Limit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bDrawLimits;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlFloat_Settings : public FTLLRN_RigUnit_HierarchyAddControl_Settings
{
	GENERATED_BODY()
	
	FTLLRN_RigUnit_HierarchyAddControlFloat_Settings()
		: FTLLRN_RigUnit_HierarchyAddControl_Settings()
		, PrimaryAxis(ETLLRN_RigControlAxis::X)
		, bIsScale(false)
	{}
	virtual ~FTLLRN_RigUnit_HierarchyAddControlFloat_Settings() override {}

	virtual void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings) override;
	virtual void Configure(FTLLRN_RigControlSettings& OutSettings) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	ETLLRN_RigControlAxis PrimaryAxis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bIsScale;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControlFloat_LimitSettings Limits;;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ProxySettings Proxy;
};

/**
 * Adds a new control to the hierarchy
 */
USTRUCT(meta=(TemplateName="SpawnControl", Keywords="Construction,Create,New,AddControl,NewControl,CreateControl", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlElement : public FTLLRN_RigUnit_HierarchyAddElement
{
public:
	
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControlElement()
	{
		Name = TEXT("NewControl");
		OffsetSpace = ERigVMTransformSpace::LocalSpace;
	}

	virtual ETLLRN_RigElementType GetElementTypeToSpawn() const override { return ETLLRN_RigElementType::Control; }
	virtual ETLLRN_RigControlType GetControlTypeToSpawn() const { return ETLLRN_RigControlType::Bool; }

	/*
	 * The offset transform of the new control
	 */
	UPROPERTY(meta = (Input))
	FTransform OffsetTransform;

	/*
	 * The space the offset is in
	 */
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace OffsetSpace;

protected:
	
	static FTransform ProjectOffsetTransform(const FTransform& InOffsetTransform, ERigVMTransformSpace InOffsetSpace, const FTLLRN_RigElementKey& InParent, const UTLLRN_RigHierarchy* InHierarchy);
};

/**
 * Adds a new control to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Float Control", TemplateName="SpawnControl", Keywords="Construction,Create,New,AddControl,NewControl,CreateControl", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlFloat : public FTLLRN_RigUnit_HierarchyAddControlElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControlFloat()
	{
		InitialValue = 0.f;
	}

	virtual ETLLRN_RigControlType GetControlTypeToSpawn() const override { return Settings.bIsScale ? ETLLRN_RigControlType::ScaleFloat : ETLLRN_RigControlType::Float; }

	/*
	 * The initial value of the new control
	 */
	UPROPERTY(meta = (Input))
	float InitialValue;
	
	/*
	 * The settings for the control
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddControlFloat_Settings Settings;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlInteger_LimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddControlInteger_LimitSettings()
		: Limit(true, true)
		, MinValue(0)
		, MaxValue(100)
		, bDrawLimits(true)
	{}

	void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings);
	void Configure(FTLLRN_RigControlSettings& OutSettings) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled Limit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 MinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 MaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bDrawLimits;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlInteger_Settings : public FTLLRN_RigUnit_HierarchyAddControl_Settings
{
	GENERATED_BODY()
	
	FTLLRN_RigUnit_HierarchyAddControlInteger_Settings()
		: FTLLRN_RigUnit_HierarchyAddControl_Settings()
		, PrimaryAxis(ETLLRN_RigControlAxis::X)
	{}
	virtual ~FTLLRN_RigUnit_HierarchyAddControlInteger_Settings() override {}

	virtual void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings) override;
	virtual void Configure(FTLLRN_RigControlSettings& OutSettings) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	ETLLRN_RigControlAxis PrimaryAxis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TObjectPtr<UEnum> ControlEnum;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControlInteger_LimitSettings Limits;;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ProxySettings Proxy;
};

/**
 * Adds a new control to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Integer Control", TemplateName="SpawnControl", Keywords="Construction,Create,New,AddControl,NewControl,CreateControl", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlInteger : public FTLLRN_RigUnit_HierarchyAddControlElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControlInteger()
	{
		InitialValue = 0;
	}

	virtual ETLLRN_RigControlType GetControlTypeToSpawn() const override { return ETLLRN_RigControlType::Integer; }

	/*
	 * The initial value of the new control
	 */
	UPROPERTY(meta = (Input))
	int32 InitialValue;
	
	/*
	 * The settings for the control
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddControlInteger_Settings Settings;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlVector2D_LimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddControlVector2D_LimitSettings()
		: LimitX(true, true)
		, LimitY(true, true)
		, MinValue(FVector2D::ZeroVector)
		, MaxValue(FVector2D(1.f, 1.f))
		, bDrawLimits(true)
	{}

	void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings);
	void Configure(FTLLRN_RigControlSettings& OutSettings) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FVector2D MinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FVector2D MaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bDrawLimits;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlVector2D_Settings : public FTLLRN_RigUnit_HierarchyAddControl_Settings
{
	GENERATED_BODY()
	
	FTLLRN_RigUnit_HierarchyAddControlVector2D_Settings()
		: FTLLRN_RigUnit_HierarchyAddControl_Settings()
		, PrimaryAxis(ETLLRN_RigControlAxis::X)
	{
		FilteredChannels.Reset();
	}
	virtual ~FTLLRN_RigUnit_HierarchyAddControlVector2D_Settings() override {}

	virtual void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings) override;
	virtual void Configure(FTLLRN_RigControlSettings& OutSettings) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	ETLLRN_RigControlAxis PrimaryAxis;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControlVector2D_LimitSettings Limits;;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ProxySettings Proxy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<ETLLRN_RigControlTransformChannel> FilteredChannels;
};

/**
 * Adds a new control to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Vector2D Control", TemplateName="SpawnControl", Keywords="Construction,Create,New,AddControl,NewControl,CreateControl", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlVector2D : public FTLLRN_RigUnit_HierarchyAddControlElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControlVector2D()
	{
		InitialValue = FVector2D::ZeroVector;
	}

	virtual ETLLRN_RigControlType GetControlTypeToSpawn() const override { return ETLLRN_RigControlType::Vector2D; }

	/*
	 * The initial value of the new control
	 */
	UPROPERTY(meta = (Input))
	FVector2D InitialValue;
	
	/*
	 * The settings for the control
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddControlVector2D_Settings Settings;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlVector_LimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddControlVector_LimitSettings()
		: LimitX(false, false)
		, LimitY(false, false)
		, LimitZ(false, false)
		, MinValue(FVector::ZeroVector)
		, MaxValue(FVector::OneVector)
		, bDrawLimits(true)
	{}

	void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings);
	void Configure(FTLLRN_RigControlSettings& OutSettings) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FVector MinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FVector MaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bDrawLimits;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlVector_Settings : public FTLLRN_RigUnit_HierarchyAddControl_Settings
{
	GENERATED_BODY()
	
	FTLLRN_RigUnit_HierarchyAddControlVector_Settings()
		: FTLLRN_RigUnit_HierarchyAddControl_Settings()
		, InitialSpace(ERigVMTransformSpace::LocalSpace)
		, bIsPosition(true)
	{
		FilteredChannels.Reset();
	}
	virtual ~FTLLRN_RigUnit_HierarchyAddControlVector_Settings() override {}

	virtual void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings) override;
	virtual void Configure(FTLLRN_RigControlSettings& OutSettings) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	ERigVMTransformSpace InitialSpace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bIsPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControlVector_LimitSettings Limits;;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ProxySettings Proxy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<ETLLRN_RigControlTransformChannel> FilteredChannels;
};

/**
 * Adds a new control to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Vector Control", TemplateName="SpawnControl", Keywords="Construction,Create,New,AddControl,NewControl,CreateControl", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlVector : public FTLLRN_RigUnit_HierarchyAddControlElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControlVector()
	{
		InitialValue = FVector::ZeroVector;
	}

	virtual ETLLRN_RigControlType GetControlTypeToSpawn() const override
	{
		return Settings.bIsPosition ? ETLLRN_RigControlType::Position : ETLLRN_RigControlType::Scale;
	}

	/*
	 * The initial value of the new control
	 */
	UPROPERTY(meta = (Input))
	FVector InitialValue;

	/*
	 * The settings for the control
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddControlVector_Settings Settings;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlRotator_LimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddControlRotator_LimitSettings()
		: LimitPitch(false, false)
		, LimitYaw(false, false)
		, LimitRoll(false, false)
		, MinValue(FRotator(-180.f, -180.f, -180.f))
		, MaxValue(FRotator(180.f, 180.f, 180.f))
		, bDrawLimits(true)
	{}

	void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings);
	void Configure(FTLLRN_RigControlSettings& OutSettings) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitRoll;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FRotator MinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FRotator MaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bDrawLimits;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlRotator_Settings : public FTLLRN_RigUnit_HierarchyAddControl_Settings
{
	GENERATED_BODY()
	
	FTLLRN_RigUnit_HierarchyAddControlRotator_Settings()
		: FTLLRN_RigUnit_HierarchyAddControl_Settings()
		, InitialSpace(ERigVMTransformSpace::LocalSpace)
	{
		FilteredChannels.Reset();
	}
	virtual ~FTLLRN_RigUnit_HierarchyAddControlRotator_Settings() override {}

	virtual void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings) override;
	virtual void Configure(FTLLRN_RigControlSettings& OutSettings) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	ERigVMTransformSpace InitialSpace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControlRotator_LimitSettings Limits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ProxySettings Proxy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<ETLLRN_RigControlTransformChannel> FilteredChannels;
};

/**
 * Adds a new control to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Rotator Control", TemplateName="SpawnControl", Keywords="Construction,Create,New,AddControl,NewControl,CreateControl,Rotation", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlRotator : public FTLLRN_RigUnit_HierarchyAddControlElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControlRotator()
	{
		InitialValue = FRotator::ZeroRotator;
	}

	virtual ETLLRN_RigControlType GetControlTypeToSpawn() const override { return ETLLRN_RigControlType::Rotator; }

	/*
	 * The initial value of the new control
	 */
	UPROPERTY(meta = (Input))
	FRotator InitialValue;

	/*
	 * The settings for the control
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddControlRotator_Settings Settings;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlTransform_LimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddControlTransform_LimitSettings()
		: LimitTranslationX(false, false)
		, LimitTranslationY(false, false)
		, LimitTranslationZ(false, false)
		, LimitPitch(false, false)
		, LimitYaw(false, false)
		, LimitRoll(false, false)
		, LimitScaleX(false, false)
		, LimitScaleY(false, false)
		, LimitScaleZ(false, false)
		, MinValue(FEulerTransform(FVector(-100.f, -100.f, -100.f), FRotator(-180.f, -180.f, -180.f), FVector(0.f, 0.f, 0.f)))
		, MaxValue(FEulerTransform(FVector(100.f, 100.f, 100.f), FRotator(180.f, 180.f, 180.f), FVector(10.f, 10.f, 10.f)))
		, bDrawLimits(true)
	{}

	void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings);
	void Configure(FTLLRN_RigControlSettings& OutSettings) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitTranslationX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitTranslationY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitTranslationZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitRoll;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitScaleX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitScaleY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled LimitScaleZ;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FEulerTransform MinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FEulerTransform MaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bDrawLimits;
};


USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlTransform_Settings : public FTLLRN_RigUnit_HierarchyAddControl_Settings
{
	GENERATED_BODY()
	
	FTLLRN_RigUnit_HierarchyAddControlTransform_Settings()
		: FTLLRN_RigUnit_HierarchyAddControl_Settings()
		, InitialSpace(ERigVMTransformSpace::LocalSpace)
		, bUsePreferredRotationOrder(false)
		, PreferredRotationOrder(EEulerRotationOrder::YZX) 
	{
		FilteredChannels.Reset();
	}
	virtual ~FTLLRN_RigUnit_HierarchyAddControlTransform_Settings() override {}

	virtual void ConfigureFrom(const FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlSettings& InSettings) override;
	virtual void Configure(FTLLRN_RigControlSettings& OutSettings) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	ERigVMTransformSpace InitialSpace;

	// Enables overriding the preferred rotation order
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bUsePreferredRotationOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	EEulerRotationOrder PreferredRotationOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControlTransform_LimitSettings Limits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigUnit_HierarchyAddControl_ProxySettings Proxy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<ETLLRN_RigControlTransformChannel> FilteredChannels;
};

/**
 * Adds a new control to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Transform Control", TemplateName="SpawnControl", Keywords="Construction,Create,New,AddControl,NewControl,CreateControl", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddControlTransform : public FTLLRN_RigUnit_HierarchyAddControlElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddControlTransform()
	{
		InitialValue = FTransform::Identity;
	}

	virtual ETLLRN_RigControlType GetControlTypeToSpawn() const override { return ETLLRN_RigControlType::EulerTransform; }

	/*
	 * The initial value of the new control
	 */
	UPROPERTY(meta = (Input))
	FTransform InitialValue;
	
	/*
	 * The settings for the control
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddControlTransform_Settings Settings;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelEmptyLimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddAnimationChannelEmptyLimitSettings()
	{}
};

/**
 * Adds a new animation channel to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Bool Animation Channel", TemplateName="SpawnAnimationChannel", Keywords="Construction,Create,New,AddAnimationChannel,NewAnimationChannel,CreateAnimationChannel,AddChannel,NewChannel,CreateChannel,SpawnChannel", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelBool : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddAnimationChannelBool()
	{
		Name = TEXT("NewChannel");
		InitialValue = false;
		MinimumValue = false;
		MaximumValue = true;
	}

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	bool InitialValue;

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	bool MinimumValue;

	/*
	 * The maximum value for the animation channel
	 */
	UPROPERTY(meta = (Input))
	bool MaximumValue;

	/*
	 * The enable settings for the limits
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddAnimationChannelEmptyLimitSettings LimitsEnabled;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings : public FTLLRN_RigUnit_HierarchyAddAnimationChannelEmptyLimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings()
		: Enabled(true)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled Enabled;
};

/**
 * Adds a new animation channel to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Float Animation Channel", TemplateName="SpawnAnimationChannel", Keywords="Construction,Create,New,AddAnimationChannel,NewAnimationChannel,CreateAnimationChannel,AddChannel,NewChannel,CreateChannel,SpawnChannel", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddAnimationChannelFloat()
	{
		Name = TEXT("NewChannel");
		InitialValue = 0.f;
		MinimumValue = 0.f;
		MaximumValue = 1.f;
	}

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	float InitialValue;

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	float MinimumValue;

	/*
	 * The maximum value for the animation channel
	 */
	UPROPERTY(meta = (Input))
	float MaximumValue;

	/*
	 * The enable settings for the limits
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings LimitsEnabled;

	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Adds a new animation channel to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Scale Float Animation Channel", TemplateName="SpawnScaleAnimationChannel", Keywords="Construction,Create,New,AddAnimationChannel,NewAnimationChannel,CreateAnimationChannel,AddChannel,NewChannel,CreateChannel,SpawnChannel", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelScaleFloat : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddAnimationChannelScaleFloat()
	{
		Name = TEXT("NewChannel");
		InitialValue = 1.f;
		MinimumValue = 0.f;
		MaximumValue = 10.f;
	}

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	float InitialValue;

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	float MinimumValue;

	/*
	 * The maximum value for the animation channel
	 */
	UPROPERTY(meta = (Input))
	float MaximumValue;

	/*
	 * The enable settings for the limits
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings LimitsEnabled;

	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Adds a new animation channel to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Integer Animation Channel", Keywords="Construction,Create,New,AddAnimationChannel,NewAnimationChannel,CreateAnimationChannel,AddChannel,NewChannel,CreateChannel,SpawnChannel", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelInteger : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddAnimationChannelInteger()
	{
		Name = TEXT("NewChannel");
		InitialValue = 0;
		MinimumValue = 0;
		MaximumValue = 100;
	}

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	int32 InitialValue;

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	int32 MinimumValue;

	/*
	 * The maximum value for the animation channel
	 */
	UPROPERTY(meta = (Input))
	int32 MaximumValue;

	/*
	 * The enable settings for the limits
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddAnimationChannelSingleLimitSettings LimitsEnabled;

	/*
	 * The enum to use to find valid values
	 */
	UPROPERTY(meta = (Input))
	TObjectPtr<UEnum> ControlEnum;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannel2DLimitSettings : public FTLLRN_RigUnit_HierarchyAddAnimationChannelEmptyLimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddAnimationChannel2DLimitSettings()
		: X(true)
		, Y(true)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled Y;
};

/**
 * Adds a new animation channel to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Vector2D Animation Channel", TemplateName="SpawnAnimationChannel", Keywords="Construction,Create,New,AddAnimationChannel,NewAnimationChannel,CreateAnimationChannel,AddChannel,NewChannel,CreateChannel,SpawnChannel", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelVector2D : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddAnimationChannelVector2D()
	{
		Name = TEXT("NewChannel");
		InitialValue = FVector2D::ZeroVector;
		MinimumValue = FVector2D::ZeroVector;
		MaximumValue = FVector2D(1.f, 1.f);
	}

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	FVector2D InitialValue;

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	FVector2D MinimumValue;

	/*
	 * The maximum value for the animation channel
	 */
	UPROPERTY(meta = (Input))
	FVector2D MaximumValue;

	/*
	 * The enable settings for the limits
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddAnimationChannel2DLimitSettings LimitsEnabled;

	RIGVM_METHOD()
	virtual void Execute() override;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelVectorLimitSettings : public FTLLRN_RigUnit_HierarchyAddAnimationChannelEmptyLimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddAnimationChannelVectorLimitSettings()
		: X(true)
		, Y(true)
		, Z(true)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled Z;
};

/**
 * Adds a new animation channel to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Vector Animation Channel", TemplateName="SpawnAnimationChannel", Keywords="Construction,Create,New,AddAnimationChannel,NewAnimationChannel,CreateAnimationChannel,AddChannel,NewChannel,CreateChannel,SpawnChannel", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelVector : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddAnimationChannelVector()
	{
		Name = TEXT("NewChannel");
		InitialValue = FVector::ZeroVector;
		MinimumValue = FVector::ZeroVector;
		MaximumValue = FVector(1.f, 1.f, 1.f);
	}

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	FVector InitialValue;

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	FVector MinimumValue;

	/*
	 * The maximum value for the animation channel
	 */
	UPROPERTY(meta = (Input))
	FVector MaximumValue;

	/*
	 * The enable settings for the limits
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddAnimationChannelVectorLimitSettings LimitsEnabled;

	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Adds a new animation channel to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Vector Animation Channel", TemplateName="SpawnScaleAnimationChannel", Keywords="Construction,Create,New,AddAnimationChannel,NewAnimationChannel,CreateAnimationChannel,AddChannel,NewChannel,CreateChannel,SpawnChannel", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelScaleVector : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddAnimationChannelScaleVector()
	{
		Name = TEXT("NewChannel");
		InitialValue = FVector::OneVector;
		MinimumValue = FVector::ZeroVector;
		MaximumValue = FVector(10.f, 10.f, 10.f);
	}

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	FVector InitialValue;

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	FVector MinimumValue;

	/*
	 * The maximum value for the animation channel
	 */
	UPROPERTY(meta = (Input))
	FVector MaximumValue;

	/*
	 * The enable settings for the limits
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddAnimationChannelVectorLimitSettings LimitsEnabled;

	RIGVM_METHOD()
	virtual void Execute() override;
};


USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelRotatorLimitSettings : public FTLLRN_RigUnit_HierarchyAddAnimationChannelEmptyLimitSettings
{
	GENERATED_BODY();
	
	FTLLRN_RigUnit_HierarchyAddAnimationChannelRotatorLimitSettings()
		: Pitch(true)
		, Yaw(true)
		, Roll(true)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled Pitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled Yaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FTLLRN_RigControlLimitEnabled Roll;
};

/**
 * Adds a new animation channel to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Rotator Animation Channel", TemplateName="SpawnAnimationChannel", Keywords="Construction,Create,New,AddAnimationChannel,NewAnimationChannel,CreateAnimationChannel,AddChannel,NewChannel,CreateChannel,SpawnChannel", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddAnimationChannelRotator : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddAnimationChannelRotator()
	{
		Name = TEXT("NewChannel");
		InitialValue = FRotator::ZeroRotator;
		MinimumValue = FRotator(-180.f, -180.f, -180.f);
		MaximumValue = FRotator(180.f, 180.f, 180.f);
	}

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	FRotator InitialValue;

	/*
	 * The initial value of the new animation channel
	 */
	UPROPERTY(meta = (Input))
	FRotator MinimumValue;

	/*
	 * The maximum value for the animation channel
	 */
	UPROPERTY(meta = (Input))
	FRotator MaximumValue;

	/*
	 * The enable settings for the limits
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddAnimationChannelRotatorLimitSettings LimitsEnabled;

	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Retrieves the shape settings of a control
 */
USTRUCT(meta=(DisplayName="Get Shape Settings", Keywords="Construction,Create,New,Control", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyGetShapeSettings : public FTLLRN_RigUnit_DynamicHierarchyBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyGetShapeSettings()
	{
		Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
	}

	/*
	 * The item to change
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Item;

	/*
	 * The shape settings for the control
	 */
	UPROPERTY(meta = (Output))
	FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings Settings;

	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Changes the shape settings of a control
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Set Shape Settings", Keywords="Construction,Create,New,Control", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchySetShapeSettings : public FTLLRN_RigUnit_DynamicHierarchyBaseMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchySetShapeSettings()
	{
		Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
	}

	/*
	 * The item to change
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Item;

	/*
	 * The shape settings for the control
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigUnit_HierarchyAddControl_ShapeSettings Settings;

	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Adds a new socket to the hierarchy
 * Note: This node only runs as part of the construction event.
 */
USTRUCT(meta=(DisplayName="Spawn Socket", Keywords="Construction,Create,New,Locator,Group", Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_HierarchyAddSocket : public FTLLRN_RigUnit_HierarchyAddElement
{
	GENERATED_BODY()

	FTLLRN_RigUnit_HierarchyAddSocket()
	{
		Name = TEXT("NewSocket");
		Transform = FTransform::Identity;
		Space = ERigVMTransformSpace::LocalSpace;
		Color = FLinearColor::White;
		Description = FString();
	}

	virtual ETLLRN_RigElementType GetElementTypeToSpawn() const override { return ETLLRN_RigElementType::Socket; }

	/*
	 * The initial transform of the new element
	 */
	UPROPERTY(meta = (Input))
	FTransform Transform;

	/**
	 * Defines if the transform should be interpreted in local or global space
	 */ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	/*
	 * The color of the socket
	 */
	UPROPERTY(meta = (Input))
	FLinearColor Color;

	/*
	 * The (optional) description of the socket
	 */
	UPROPERTY(meta = (Input))
	FString Description;

	RIGVM_METHOD()
	virtual void Execute() override;
};
