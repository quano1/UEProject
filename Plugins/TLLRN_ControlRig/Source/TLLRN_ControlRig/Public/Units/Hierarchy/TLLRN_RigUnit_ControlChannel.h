// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_ControlChannel.generated.h"

/**
 * Get Animation Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(Abstract, Category="Controls", DocumentationPolicy = "Strict", NodeColor="0.462745, 1,0, 0.329412",Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetAnimationChannelBase : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetAnimationChannelBase()
	{
		Control = Channel = NAME_None;
		bInitial = false;
		CachedChannelKey = FTLLRN_RigElementKey();
		CachedChannelHash = INDEX_NONE;
	}

	static bool UpdateCache(const UTLLRN_RigHierarchy* InHierarchy, const FName& Control, const FName& Channel, FTLLRN_RigElementKey& Key, int32& Hash);

	/**
	 * The name of the Control to retrieve the value for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "ControlName"))
	FName Control;

	/**
	 * The name of the animation channel to retrieve the value for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "AnimationChannelName"))
	FName Channel;

	/**
	 * If set to true the initial value will be returned
	 */
	UPROPERTY(meta = (Input))
	bool bInitial;

	// Used to cache the internally used bone index
	UPROPERTY()
	FTLLRN_RigElementKey CachedChannelKey;

	// A hash combining the control, channel and topology identifiers
	UPROPERTY()
	int32 CachedChannelHash;
};

/**
 * Get Bool Channel is used to retrieve a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Get Bool Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetBoolAnimationChannel : public FTLLRN_RigUnit_GetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetBoolAnimationChannel()
		: FTLLRN_RigUnit_GetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Get Float Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetFloatAnimationChannel : public FTLLRN_RigUnit_GetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetFloatAnimationChannel()
		: FTLLRN_RigUnit_GetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Get Int Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetIntAnimationChannel : public FTLLRN_RigUnit_GetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetIntAnimationChannel()
		: FTLLRN_RigUnit_GetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Get Vector2D Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetVector2DAnimationChannel : public FTLLRN_RigUnit_GetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetVector2DAnimationChannel()
		: FTLLRN_RigUnit_GetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Get Vector Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetVectorAnimationChannel : public FTLLRN_RigUnit_GetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetVectorAnimationChannel()
		: FTLLRN_RigUnit_GetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Get Rotator Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetRotatorAnimationChannel : public FTLLRN_RigUnit_GetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetRotatorAnimationChannel()
		: FTLLRN_RigUnit_GetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Get Transform Channel", Keywords="Animation,Channel", TemplateName="GetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetTransformAnimationChannel : public FTLLRN_RigUnit_GetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetTransformAnimationChannel()
		: FTLLRN_RigUnit_GetAnimationChannelBase()
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
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetAnimationChannelBase : public FTLLRN_RigUnit_GetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetAnimationChannelBase()
		:FTLLRN_RigUnit_GetAnimationChannelBase()
	{
	}

	UPROPERTY(DisplayName = "Execute", Transient, meta = (Input, Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;
};

/**
 * Set Bool Channel is used to set a control's animation channel value
 */
USTRUCT(meta=(DisplayName="Set Bool Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetBoolAnimationChannel : public FTLLRN_RigUnit_SetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetBoolAnimationChannel()
		: FTLLRN_RigUnit_SetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Set Float Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetFloatAnimationChannel : public FTLLRN_RigUnit_SetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetFloatAnimationChannel()
		: FTLLRN_RigUnit_SetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Set Int Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetIntAnimationChannel : public FTLLRN_RigUnit_SetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetIntAnimationChannel()
		: FTLLRN_RigUnit_SetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Set Vector2D Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetVector2DAnimationChannel : public FTLLRN_RigUnit_SetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetVector2DAnimationChannel()
		: FTLLRN_RigUnit_SetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Set Vector Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetVectorAnimationChannel : public FTLLRN_RigUnit_SetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetVectorAnimationChannel()
		: FTLLRN_RigUnit_SetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Set Rotator Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetRotatorAnimationChannel : public FTLLRN_RigUnit_SetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetRotatorAnimationChannel()
		: FTLLRN_RigUnit_SetAnimationChannelBase()
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
USTRUCT(meta=(DisplayName="Set Transform Channel", Keywords="Animation,Channel", TemplateName="SetAnimationChannel"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetTransformAnimationChannel : public FTLLRN_RigUnit_SetAnimationChannelBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetTransformAnimationChannel()
		: FTLLRN_RigUnit_SetAnimationChannelBase()
		, Value(FTransform::Identity)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	// The new value of the animation channel
	UPROPERTY(meta=(Input))
	FTransform Value;
};
