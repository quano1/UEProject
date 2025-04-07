// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_BoneName.generated.h"

/**
 * The Item node is used to share a specific item across the graph
 */
USTRUCT(meta=(DisplayName="Item", Category="Hierarchy", NodeColor = "0.4627450108528137 1.0 0.3294120132923126", DocumentationPolicy = "Strict", Constant))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_Item : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_Item()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item
	 */
	UPROPERTY(meta = (Input, Output, ExpandByDefault))
	FTLLRN_RigElementKey Item;
};

/**
 * The Item Array node is used to share an array of items across the graph
 */
USTRUCT(meta=(DisplayName="Item Array", Category="Hierarchy", NodeColor = "0.4627450108528137 1.0 0.3294120132923126", DocumentationPolicy = "Strict", Constant))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemArray : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ItemArray()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The items
	 */
	UPROPERTY(meta = (Input, Output, ExpandByDefault))
	TArray<FTLLRN_RigElementKey> Items;
};

/**
 * BoneName is used to represent a bone name in the graph
 */
USTRUCT(meta=(DisplayName="Bone Name", Category="Hierarchy", DocumentationPolicy = "Strict", Constant, Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_BoneName : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_BoneName()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Bone
	 */
	UPROPERTY(meta = (Input, Output))
	FName Bone;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * SpaceName is used to represent a Space name in the graph
 */
USTRUCT(meta=(DisplayName="Space Name", Category="Hierarchy", DocumentationPolicy = "Strict", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_SpaceName : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_SpaceName()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Space
	 */
	UPROPERTY(meta = (Input, Output))
	FName Space;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
 * ControlName is used to represent a Control name in the graph
 */
USTRUCT(meta=(DisplayName="Control Name", Category="Hierarchy", DocumentationPolicy = "Strict", Deprecated = "4.25"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ControlName : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ControlName()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The name of the Control
	 */
	UPROPERTY(meta = (Input, Output))
	FName Control;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};
