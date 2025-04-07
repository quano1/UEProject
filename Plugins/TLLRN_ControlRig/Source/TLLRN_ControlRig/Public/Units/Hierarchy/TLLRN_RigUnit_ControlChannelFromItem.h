// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_ControlChannelFromItem.generated.h"

/**
 * Get Animation Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(Abstract, Category="Controls", DocumentationPolicy = "Strict", NodeColor="0.462745, 1,0, 0.329412",Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetAnimationChannelFromItemBase : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetAnimationChannelFromItemBase()
	{
		Item = FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Control);
		bInitial = false;
	}

	/**
	 * The item representing the channel
	 */
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Item;

	/**
	 * If set to true the initial value will be returned
	 */
	UPROPERTY(meta = (Input))
	bool bInitial;
};

/**
 * Get Bool Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Get Bool Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetBoolAnimationChannelFromItem : public FTLLRN_RigUnit_GetAnimationChannelFromItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetBoolAnimationChannelFromItem()
		: FTLLRN_RigUnit_GetAnimationChannelFromItemBase()
		, Value(false)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The current value of the animation channel
	UPROPERTY(meta=(Output))
	bool Value;
};

/**
 * Get Float Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Get Float Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetFloatAnimationChannelFromItem : public FTLLRN_RigUnit_GetAnimationChannelFromItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetFloatAnimationChannelFromItem()
		: FTLLRN_RigUnit_GetAnimationChannelFromItemBase()
		, Value(0.f)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The current value of the animation channel
	UPROPERTY(meta=(Output, UIMin=0, UIMax=1))
	float Value;
};

/**
 * Get Int Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Get Int Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetIntAnimationChannelFromItem : public FTLLRN_RigUnit_GetAnimationChannelFromItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetIntAnimationChannelFromItem()
		: FTLLRN_RigUnit_GetAnimationChannelFromItemBase()
		, Value(0)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The current value of the animation channel
	UPROPERTY(meta=(Output))
	int32 Value;
};

/**
 * Get Vector2D Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Get Vector2D Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetVector2DAnimationChannelFromItem : public FTLLRN_RigUnit_GetAnimationChannelFromItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetVector2DAnimationChannelFromItem()
		: FTLLRN_RigUnit_GetAnimationChannelFromItemBase()
		, Value(FVector2D::ZeroVector)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The current value of the animation channel
	UPROPERTY(meta=(Output))
	FVector2D Value;
};

/**
 * Get Vector Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Get Vector Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetVectorAnimationChannelFromItem : public FTLLRN_RigUnit_GetAnimationChannelFromItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetVectorAnimationChannelFromItem()
		: FTLLRN_RigUnit_GetAnimationChannelFromItemBase()
		, Value(FVector::ZeroVector)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The current value of the animation channel
	UPROPERTY(meta=(Output))
	FVector Value;
};

/**
 * Get Rotator Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Get Rotator Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetRotatorAnimationChannelFromItem : public FTLLRN_RigUnit_GetAnimationChannelFromItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetRotatorAnimationChannelFromItem()
		: FTLLRN_RigUnit_GetAnimationChannelFromItemBase()
		, Value(FRotator::ZeroRotator)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The current value of the animation channel
	UPROPERTY(meta=(Output))
	FRotator Value;
};

/**
 * Get Transform Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Get Transform Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetTransformAnimationChannelFromItem : public FTLLRN_RigUnit_GetAnimationChannelFromItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetTransformAnimationChannelFromItem()
		: FTLLRN_RigUnit_GetAnimationChannelFromItemBase()
		, Value(FTransform::Identity)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The current value of the animation channel
	UPROPERTY(meta=(Output))
	FTransform Value;
};

/**
 * Set Animation Channel is used to change a control's animation channel value
 */
USTRUCT(meta = (Abstract))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetAnimationChannelBaseFromItem : public FTLLRN_RigUnit_GetAnimationChannelFromItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetAnimationChannelBaseFromItem()
		:FTLLRN_RigUnit_GetAnimationChannelFromItemBase()
	{
	}

	UPROPERTY(DisplayName = "Execute", Transient, meta = (Input, Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;
};

/**
 * Set Bool Channel is used to set a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Set Bool Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetBoolAnimationChannelFromItem : public FTLLRN_RigUnit_SetAnimationChannelBaseFromItem
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetBoolAnimationChannelFromItem()
		: FTLLRN_RigUnit_SetAnimationChannelBaseFromItem()
		, Value(false)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The new value of the animation channel
	UPROPERTY(meta=(Input))
	bool Value;
};

/**
 * Set Float Channel is used to set a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Set Float Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetFloatAnimationChannelFromItem : public FTLLRN_RigUnit_SetAnimationChannelBaseFromItem
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetFloatAnimationChannelFromItem()
		: FTLLRN_RigUnit_SetAnimationChannelBaseFromItem()
		, Value(0.f)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The new value of the animation channel
	UPROPERTY(meta=(Input, UIMin=0, UIMax=1))
	float Value;
};

/**
 * Set Int Channel is used to set a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Set Int Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetIntAnimationChannelFromItem : public FTLLRN_RigUnit_SetAnimationChannelBaseFromItem
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetIntAnimationChannelFromItem()
		: FTLLRN_RigUnit_SetAnimationChannelBaseFromItem()
		, Value(0)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The new value of the animation channel
	UPROPERTY(meta=(Input))
	int32 Value;
};

/**
 * Set Vector2D Channel is used to set a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Set Vector2D Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetVector2DAnimationChannelFromItem : public FTLLRN_RigUnit_SetAnimationChannelBaseFromItem
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetVector2DAnimationChannelFromItem()
		: FTLLRN_RigUnit_SetAnimationChannelBaseFromItem()
		, Value(FVector2D::ZeroVector)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The new value of the animation channel
	UPROPERTY(meta=(Input))
	FVector2D Value;
};

/**
 * Set Vector Channel is used to set a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Set Vector Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetVectorAnimationChannelFromItem : public FTLLRN_RigUnit_SetAnimationChannelBaseFromItem
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetVectorAnimationChannelFromItem()
		: FTLLRN_RigUnit_SetAnimationChannelBaseFromItem()
		, Value(FVector::ZeroVector)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The new value of the animation channel
	UPROPERTY(meta=(Input))
	FVector Value;
};

/**
 * Set Rotator Channel is used to set a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Set Rotator Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetRotatorAnimationChannelFromItem : public FTLLRN_RigUnit_SetAnimationChannelBaseFromItem
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetRotatorAnimationChannelFromItem()
		: FTLLRN_RigUnit_SetAnimationChannelBaseFromItem()
		, Value(FRotator::ZeroRotator)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The new value of the animation channel
	UPROPERTY(meta=(Input))
	FRotator Value;
};

/**
 * Set Transform Channel is used to set a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Set Transform Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannelFromItem"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetTransformAnimationChannelFromItem : public FTLLRN_RigUnit_SetAnimationChannelBaseFromItem
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetTransformAnimationChannelFromItem()
		: FTLLRN_RigUnit_SetAnimationChannelBaseFromItem()
		, Value(FTransform::Identity)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The new value of the animation channel
	UPROPERTY(meta=(Input))
	FTransform Value;
};
