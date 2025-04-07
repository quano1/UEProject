// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigHierarchyPose.h"
#include "TLLRN_TLLRN_RigSpaceHierarchy.generated.h"

class UTLLRN_ControlRig;
struct FTLLRN_RigHierarchyContainer;

UENUM(BlueprintType)
enum class ETLLRN_RigSpaceType : uint8
{
	/** Not attached to anything */
	Global,

	/** Attached to a bone */
	Bone,

	/** Attached to a control */
	Control,

	/** Attached to a space*/
	Space
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigSpace : public FTLLRN_RigElement
{
	GENERATED_BODY()

	FTLLRN_RigSpace()
		: FTLLRN_RigElement()
		, SpaceType(ETLLRN_RigSpaceType::Global)
		, ParentName(NAME_None)
		, ParentIndex(INDEX_NONE)
		, InitialTransform(FTransform::Identity)
		, LocalTransform(FTransform::Identity)
	{
	}
	virtual ~FTLLRN_RigSpace() {}

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = FTLLRN_RigElement)
	ETLLRN_RigSpaceType SpaceType;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = FTLLRN_RigElement)
	FName ParentName;

	UPROPERTY(BlueprintReadOnly, transient, Category = FTLLRN_RigElement)
	int32 ParentIndex;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = FTLLRN_RigElement)
	FTransform InitialTransform;

	UPROPERTY(BlueprintReadOnly, transient, EditAnywhere, Category = FTLLRN_RigElement)
	FTransform LocalTransform;

	virtual ETLLRN_RigElementType GetElementType() const override
	{
		return ETLLRN_RigElementType::Null;
	}

	virtual FTLLRN_RigElementKey GetParentElementKey() const
	{
		switch (SpaceType)
		{
			case ETLLRN_RigSpaceType::Bone:
			{
				return FTLLRN_RigElementKey(ParentName, ETLLRN_RigElementType::Bone);
			}
			case ETLLRN_RigSpaceType::Control:
			{
				return FTLLRN_RigElementKey(ParentName, ETLLRN_RigElementType::Control);
			}
			case ETLLRN_RigSpaceType::Space:
			{
				return FTLLRN_RigElementKey(ParentName, ETLLRN_RigElementType::Null);
			}
			default:
			{
				break;
			}
		}
		return FTLLRN_RigElementKey();
	}
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_TLLRN_RigSpaceHierarchy
{
	GENERATED_BODY()

	FTLLRN_TLLRN_RigSpaceHierarchy();

	TArray<FTLLRN_RigSpace>::RangedForIteratorType      begin()       { return Spaces.begin(); }
	TArray<FTLLRN_RigSpace>::RangedForConstIteratorType begin() const { return Spaces.begin(); }
	TArray<FTLLRN_RigSpace>::RangedForIteratorType      end()         { return Spaces.end();   }
	TArray<FTLLRN_RigSpace>::RangedForConstIteratorType end() const   { return Spaces.end();   }

	FTLLRN_RigSpace& Add(const FName& InNewName, ETLLRN_RigSpaceType InSpaceType, const FName& InParentName, const FTransform& InTransform);

	// Pretty weird that this type is copy/move assignable (needed for USTRUCTs) but not copy/move constructible
	FTLLRN_TLLRN_RigSpaceHierarchy(FTLLRN_TLLRN_RigSpaceHierarchy&& InOther) = delete;
	FTLLRN_TLLRN_RigSpaceHierarchy(const FTLLRN_TLLRN_RigSpaceHierarchy& InOther) = delete;
	FTLLRN_TLLRN_RigSpaceHierarchy& operator=(FTLLRN_TLLRN_RigSpaceHierarchy&& InOther) = default;
	FTLLRN_TLLRN_RigSpaceHierarchy& operator=(const FTLLRN_TLLRN_RigSpaceHierarchy& InOther) = default;

private:
	UPROPERTY(EditAnywhere, Category = FTLLRN_TLLRN_RigSpaceHierarchy)
	TArray<FTLLRN_RigSpace> Spaces;

	friend struct FTLLRN_RigHierarchyContainer;
	friend struct FTLLRN_CachedTLLRN_RigElement;
	friend class UTLLRN_ControlTLLRN_RigHierarchyModifier;
	friend class UTLLRN_ControlRig;
};
