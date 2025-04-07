// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_SetControlVisibility.generated.h"

/**
 * GetControlVisibility is used to retrieve the visibility of a control
 */
USTRUCT(meta=(DisplayName="Get Control Visibility", Category="Controls", DocumentationPolicy="Strict", Keywords = "GetControlVisibility,Visibility,Hide,Show,Hidden,Visible,SetGizmoVisibility", TemplateName="GetControlVisibility", NodeColor="0, 0.364706, 1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetControlVisibility : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetControlVisibility()
		: Item(NAME_None, ETLLRN_RigElementType::Control)
		, bVisible(true)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to set the visibility for.
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * The visibility of the control
	 */
	UPROPERTY(meta = (Output))
	bool bVisible;

	// Used to cache the internally used control index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedControlIndex;
};

/**
 * SetControlVisibility is used to change the visibility on a control at runtime
 */
USTRUCT(meta=(DisplayName="Set Control Visibility", Category="Controls", DocumentationPolicy="Strict", Keywords = "SetControlVisibility,Visibility,Hide,Show,Hidden,Visible,SetGizmoVisibility", TemplateName="SetControlVisibility", NodeColor="0, 0.364706, 1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetControlVisibility : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetControlVisibility()
		: Item(NAME_None, ETLLRN_RigElementType::Control)
		, Pattern(FString())
		, bVisible(true)
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to set the visibility for.
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * If the ControlName is set to None this can be used to look for a series of Controls
	 */
	UPROPERTY(meta = (Input, Constant))
	FString Pattern;

	/**
	 * The visibility to set for the control
	 */
	UPROPERTY(meta = (Input))
	bool bVisible;

	// Used to cache the internally used control index
	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> CachedControlIndices;
};
