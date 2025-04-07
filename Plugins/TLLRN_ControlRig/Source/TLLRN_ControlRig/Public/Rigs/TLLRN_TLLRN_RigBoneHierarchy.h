// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigHierarchyPose.h"
#include "ReferenceSkeleton.h"
#include "TLLRN_TLLRN_RigBoneHierarchy.generated.h"

class UTLLRN_ControlRig;
struct FTLLRN_RigHierarchyContainer;

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigBone: public FTLLRN_RigElement
{
	GENERATED_BODY()

	FTLLRN_RigBone()
		: FTLLRN_RigElement()
		, ParentName(NAME_None)
		, ParentIndex(INDEX_NONE)
		, InitialTransform(FTransform::Identity)
		, GlobalTransform(FTransform::Identity)
		, LocalTransform(FTransform::Identity)
		, Dependents()
		, Type(ETLLRN_RigBoneType::Imported)
	{
	}
	virtual ~FTLLRN_RigBone() {}

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = FTLLRN_RigElement)
	FName ParentName;

	UPROPERTY(transient)
	int32 ParentIndex;

	/* Initial global transform that is saved in this rig */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = FTLLRN_RigElement)
	FTransform InitialTransform;

	UPROPERTY(BlueprintReadOnly, transient, EditAnywhere, Category = FTLLRN_RigElement)
	FTransform GlobalTransform;

	UPROPERTY(BlueprintReadOnly, transient, EditAnywhere, Category = FTLLRN_RigElement)
	FTransform LocalTransform;

	/** dependent list - direct dependent for child or anything that needs to update due to this */
	UPROPERTY(transient)
	TArray<int32> Dependents;

	/** the source of the bone to differentiate procedurally generated, imported etc */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = FTLLRN_RigElement)
	ETLLRN_RigBoneType Type;

	virtual ETLLRN_RigElementType GetElementType() const override
	{
		return ETLLRN_RigElementType::Bone;
	}

	virtual FTLLRN_RigElementKey GetParentElementKey(bool bForce = false) const
	{
		return FTLLRN_RigElementKey(ParentName, GetElementType());
	}
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_TLLRN_RigBoneHierarchy
{
	GENERATED_BODY()

	FTLLRN_TLLRN_RigBoneHierarchy();

	TArray<FTLLRN_RigBone>::RangedForIteratorType      begin()       { return Bones.begin(); }
	TArray<FTLLRN_RigBone>::RangedForConstIteratorType begin() const { return Bones.begin(); }
	TArray<FTLLRN_RigBone>::RangedForIteratorType      end()         { return Bones.end();   }
	TArray<FTLLRN_RigBone>::RangedForConstIteratorType end() const   { return Bones.end();   }

	FTLLRN_RigBone& Add(const FName& InNewName, const FName& InParentName, ETLLRN_RigBoneType InType, const FTransform& InInitTransform, const FTransform& InLocalTransform, const FTransform& InGlobalTransform);

	// Pretty weird that this type is copy/move assignable (needed for USTRUCTs) but not copy/move constructible
	FTLLRN_TLLRN_RigBoneHierarchy(FTLLRN_TLLRN_RigBoneHierarchy&& InOther) = delete;
	FTLLRN_TLLRN_RigBoneHierarchy(const FTLLRN_TLLRN_RigBoneHierarchy& InOther) = delete;
	FTLLRN_TLLRN_RigBoneHierarchy& operator=(FTLLRN_TLLRN_RigBoneHierarchy&& InOther) = default;
	FTLLRN_TLLRN_RigBoneHierarchy& operator=(const FTLLRN_TLLRN_RigBoneHierarchy& InOther) = default;

private:
	UPROPERTY(EditAnywhere, Category = FTLLRN_TLLRN_RigBoneHierarchy)
	TArray<FTLLRN_RigBone> Bones;

	friend struct FTLLRN_RigHierarchyContainer;
	friend struct FTLLRN_CachedTLLRN_RigElement;
	friend class UTLLRN_ControlTLLRN_RigHierarchyModifier;
	friend class UTLLRN_ControlRig;
};
