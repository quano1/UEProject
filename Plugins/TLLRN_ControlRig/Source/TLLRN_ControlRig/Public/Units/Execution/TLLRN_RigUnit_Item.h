// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigDefines.h" 
#include "TLLRN_RigUnit_Item.generated.h"

USTRUCT(meta = (Abstract, NodeColor = "0.7 0.05 0.5", Category = "Items"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemBase : public FTLLRN_RigUnit
{
	GENERATED_BODY()
};

USTRUCT(meta = (Abstract, NodeColor = "0.7 0.05 0.5", Category = "Items"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemBaseMutable : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()
};

/**
 * Returns true or false if a given item exists
 */
USTRUCT(meta=(DisplayName="Item Exists", Keywords=""))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemExists : public FTLLRN_RigUnit_ItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ItemExists()
	{
		Item = FTLLRN_RigElementKey();
		Exists = false;
		CachedIndex = FTLLRN_CachedTLLRN_RigElement();
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	UPROPERTY(meta = (Output))
	bool Exists;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
 * Replaces the text within the name of the item
 */
USTRUCT(meta=(DisplayName="Item Replace", Keywords="Replace,Name"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemReplace : public FTLLRN_RigUnit_ItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ItemReplace()
	{
		Item = Result = FTLLRN_RigElementKey();
		Old = New = NAME_None;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKey Item;

	UPROPERTY(meta = (Input))
	FName Old;

	UPROPERTY(meta = (Input))
	FName New;

	UPROPERTY(meta = (Output))
	FTLLRN_RigElementKey Result;
};

/**
* Returns true if the two items are equal
*/
USTRUCT(meta=(DisplayName="Item Equals", Keywords="", Deprecated="5.1"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemEquals : public FTLLRN_RigUnit_ItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ItemEquals()
	{
		A = B = FTLLRN_RigElementKey();
		Result = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey A;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey B;

	UPROPERTY(meta = (Output))
	bool Result;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Returns true if the two items are not equal
*/
USTRUCT(meta=(DisplayName="Item Not Equals", Keywords="", Deprecated="5.1"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemNotEquals : public FTLLRN_RigUnit_ItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ItemNotEquals()
	{
		A = B = FTLLRN_RigElementKey();
		Result = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey A;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey B;

	UPROPERTY(meta = (Output))
	bool Result;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* Returns true if the two items' types are equal
*/
USTRUCT(meta=(DisplayName="Item Type Equals", Keywords=""))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemTypeEquals : public FTLLRN_RigUnit_ItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ItemTypeEquals()
	{
		A = B = FTLLRN_RigElementKey();
		Result = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey A;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey B;

	UPROPERTY(meta = (Output))
	bool Result;
};

/**
* Returns true if the two items's types are not equal
*/
USTRUCT(meta=(DisplayName="Item Type Not Equals", Keywords=""))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemTypeNotEquals : public FTLLRN_RigUnit_ItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ItemTypeNotEquals()
	{
		A = B = FTLLRN_RigElementKey();
		Result = false;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey A;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey B;

	UPROPERTY(meta = (Output))
	bool Result;
};

/**
 * Casts the provided item key to its name
 */
USTRUCT(meta=(DisplayName="To Name", Keywords="", TemplateName="Cast", ExecuteContext="FRigVMExecuteContext"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_ItemToName : public FTLLRN_RigUnit_ItemBase
{
	GENERATED_BODY()

	FTLLRN_RigUnit_ItemToName()
	{
		Value = FTLLRN_RigElementKey();
		Result = NAME_None;
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Value;

	UPROPERTY(meta = (Output))
	FName Result;
};
