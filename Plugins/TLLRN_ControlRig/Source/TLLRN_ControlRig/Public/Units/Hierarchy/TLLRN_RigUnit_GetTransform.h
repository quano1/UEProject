// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_GetTransform.generated.h"

/**
 * GetTransform is used to retrieve a single transform from a hierarchy.
 */
USTRUCT(meta=(DisplayName="Get Transform", Category="Transforms", DocumentationPolicy = "Strict", Keywords="GetBoneTransform,GetControlTransform,GetInitialTransform,GetSpaceTransform,GetTransform", NodeColor="0.462745, 1,0, 0.329412",Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetTransform : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetTransform()
		: Item(NAME_None, ETLLRN_RigElementType::Bone)
		, Space(ERigVMTransformSpace::GlobalSpace)
		, bInitial(false)
		, Transform(FTransform::Identity)
		, CachedIndex()
	{}

	virtual FString GetUnitLabel() const override;

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	 * The item to retrieve the transform for
	 */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FTLLRN_RigElementKey Item;

	/**
	 * Defines if the transform should be retrieved in local or global space
	 */ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	/**
	 * Defines if the transform should be retrieved as current (false) or initial (true).
	 * Initial transforms for bones and other elements in the hierarchy represent the reference pose's value.
	 */ 
	UPROPERTY(meta = (Input))
	bool bInitial;

	// The current transform of the given item - or identity in case it wasn't found.
	UPROPERTY(meta=(Output))
	FTransform Transform;

	// Used to cache the internally used index
	UPROPERTY()
	FTLLRN_CachedTLLRN_RigElement CachedIndex;
};

/**
* GetTransformArray is used to retrieve an array of transforms from the hierarchy.
*/
USTRUCT(meta=(DisplayName="Get Transform Array", Category="Hierarchy", DocumentationPolicy = "Strict", Keywords="GetBoneTransform,GetControlTransform,GetInitialTransform,GetSpaceTransform,GetTransform", NodeColor="0.462745, 1,0, 0.329412",Varying, Deprecated = "5.0"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetTransformArray : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetTransformArray()
		: Items()
		, Space(ERigVMTransformSpace::GlobalSpace)
		, bInitial(false)
		, Transforms()
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	* The items to retrieve the transforms for
	*/
	UPROPERTY(meta = (Input))
	FTLLRN_RigElementKeyCollection Items;

	/**
	* Defines if the transforms should be retrieved in local or global space
	*/ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	/**
	* Defines if the transforms should be retrieved as current (false) or initial (true).
	* Initial transforms for bones and other elements in the hierarchy represent the reference pose's value.
	*/ 
	UPROPERTY(meta = (Input))
	bool bInitial;

	// The current transform of the given item - or identity in case it wasn't found.
	UPROPERTY(meta=(Output))
	TArray<FTransform> Transforms;

	// Used to cache the internally used index
	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> CachedIndex;

	RIGVM_METHOD()
	virtual FRigVMStructUpgradeInfo GetUpgradeInfo() const override;
};

/**
* GetTransformArray is used to retrieve an array of transforms from the hierarchy.
*/
USTRUCT(meta=(DisplayName="Get Transform Array", Category="Transforms", DocumentationPolicy = "Strict", Keywords="GetBoneTransform,GetControlTransform,GetInitialTransform,GetSpaceTransform,GetTransform", NodeColor="0.462745, 1,0, 0.329412",Varying))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_GetTransformItemArray : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnit_GetTransformItemArray()
		: Items()
		, Space(ERigVMTransformSpace::GlobalSpace)
		, bInitial(false)
		, Transforms()
		, CachedIndex()
	{}

	RIGVM_METHOD()
	virtual void Execute() override;

	/**
	* The items to retrieve the transforms for
	*/
	UPROPERTY(meta = (Input))
	TArray<FTLLRN_RigElementKey> Items;

	/**
	* Defines if the transforms should be retrieved in local or global space
	*/ 
	UPROPERTY(meta = (Input))
	ERigVMTransformSpace Space;

	/**
	* Defines if the transforms should be retrieved as current (false) or initial (true).
	* Initial transforms for bones and other elements in the hierarchy represent the reference pose's value.
	*/ 
	UPROPERTY(meta = (Input))
	bool bInitial;

	// The current transform of the given item - or identity in case it wasn't found.
	UPROPERTY(meta=(Output))
	TArray<FTransform> Transforms;

	// Used to cache the internally used index
	UPROPERTY()
	TArray<FTLLRN_CachedTLLRN_RigElement> CachedIndex;
};
