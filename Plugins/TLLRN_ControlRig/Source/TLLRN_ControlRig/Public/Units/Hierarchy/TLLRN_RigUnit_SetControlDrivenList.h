// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_SetControlDrivenList.generated.h"

/**
 * GetControlDrivenList is used to retrieve the list of affected controls of an indirect control
 */
USTRUCT(meta=(DisplayName="Get Driven Controls", Category="Controls", DocumentationPolicy="Strict", Keywords = "GetControlDrivenList,Interaction,Indirect", TemplateName="GetControlDrivenList", NodeColor = "0.0 0.36470600962638855 1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetControlDrivenList : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetControlDrivenList()
		: Control(NAME_None)
		, Driven()
		, CachedControlIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to get the list for
	 */
	UPROPERTY(meta = (Input, CustomWidget = "ControlName" ))
	FName Control;

	/**
	 * The list of affected controls
	 */
	UPROPERTY(meta = (Output))
	TArray<FTLLRN_RigElementKey> Driven;

	// Used to cache the internally used bone index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedControlIndex;
};

/**
 * SetControlDrivenList is used to change the list of affected controls of an indirect control
 */
USTRUCT(meta=(DisplayName="Set Driven Controls", Category="Controls", DocumentationPolicy="Strict", Keywords = "SetControlDrivenList,Interaction,Indirect", TemplateName="SetControlDrivenList", NodeColor = "0.0 0.36470600962638855 1.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SetControlDrivenList : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SetControlDrivenList()
		: Control(NAME_None)
		, Driven()
		, CachedControlIndex(FTLLRN_CachedTLLRN_RigElement())
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control to set the list for
	 */
	UPROPERTY(meta = (Input, CustomWidget = "ControlName" ))
	FName Control;

	/**
	 * The list of affected controls
	 */
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> Driven;

	// Used to cache the internally used bone index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedControlIndex;
};
