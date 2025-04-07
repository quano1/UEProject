// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_SetControlColor.generated.h"

/**
 * GetControlColor is used to retrieve the color of control
 */
USTRUCT(meta=(DisplayName="Get Control Color", Category="Controls", DocumentationPolicy="Strict", Keywords = "GetControlColor,GetGizmoColor", TemplateName="GetControlColor", NodeColor = "0.0 0.36470600962638855 1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetControlColor : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetControlColor()
		: Control(NAME_None)
		, Color(FLinearColor::Black)
		, CachedControlIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to get the color for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "ControlName" ))
	FName Control;

	/**
	 * The color of the control
	 */
	UPROPERTY(meta = (Output))
	FLinearColor Color;

	// Used to cache the internally used bone index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedControlIndex;
};

/**
 * SetControlColor is used to change the control's color
 */
USTRUCT(meta=(DisplayName="Set Control Color", Category="Controls", DocumentationPolicy="Strict", Keywords = "SetControlColor,SetGizmoColor", TemplateName="SetControlColor", NodeColor = "0.0 0.36470600962638855 1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetControlColor : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetControlColor()
		: Control(NAME_None)
		, Color(FLinearColor::Black)
		, CachedControlIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to set the color for.
	 */
	UPROPERTY(meta = (Input, CustomWidget = "ControlName" ))
	FName Control;

	/**
	 * The color to set for the control
	 */
	UPROPERTY(meta = (Input))
	FLinearColor Color;

	// Used to cache the internally used bone index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedControlIndex;
};
