// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigHierarchyPose.h"
#include "TransformNoScale.h"
#include "EulerTransform.h"
#include "TLLRN_TLLRN_RigControlHierarchy.generated.h"

class UTLLRN_ControlRig;
class UStaticMesh;
struct FTLLRN_RigHierarchyContainer;

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigControl : public FTLLRN_RigElement
{
	GENERATED_BODY()

		FTLLRN_RigControl()
		: FTLLRN_RigElement()
		, ControlType(ETLLRN_RigControlType::Transform)
		, DisplayName(NAME_None)
		, ParentName(NAME_None)
		, ParentIndex(INDEX_NONE)
		, SpaceName(NAME_None)
		, SpaceIndex(INDEX_NONE)
		, OffsetTransform(FTransform::Identity)
		, InitialValue()
		, Value()
		, PrimaryAxis(ETLLRN_RigControlAxis::X)
		, bIsCurve(false)
		, bAnimatable(true)
		, bLimitTranslation(false)
		, bLimitRotation(false)
		, bLimitScale(false)
		, bDrawLimits(true)
		, MinimumValue()
		, MaximumValue()
		, bGizmoEnabled(true)
		, bGizmoVisible(true)
		, GizmoName(TEXT("Gizmo"))
		, GizmoTransform(FTransform::Identity)
		, GizmoColor(FLinearColor::Red)
		, Dependents()
		, bIsTransientControl(false)
		, ControlEnum(nullptr)
	{
	}
	virtual ~FTLLRN_RigControl() {}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control)
	ETLLRN_RigControlType ControlType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control)
	FName DisplayName;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Control)
	FName ParentName;

	UPROPERTY(BlueprintReadOnly, transient, Category = Control)
	int32 ParentIndex;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Control)
	FName SpaceName;

	UPROPERTY(BlueprintReadOnly, transient, Category = Control)
	int32 SpaceIndex;

	/**
	 * Used to offset a control in global space. This can be useful
	 * to offset a float control by rotating it or translating it.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control, meta = (DisplayAfter = "ControlType"))
	FTransform OffsetTransform;

	/**
	 * The value that a control is reset to during begin play or when the
	 * control rig is instantiated.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Control)
	FTLLRN_RigControlValue InitialValue;

	/**
	 * The current value of the control.
	 */
	UPROPERTY(BlueprintReadOnly, transient, VisibleAnywhere, Category = Control)
	FTLLRN_RigControlValue Value;

	/** the primary axis to use for float controls */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control)
	ETLLRN_RigControlAxis PrimaryAxis;

	/** If Created from a Curve  Container*/
	UPROPERTY(transient)
	bool bIsCurve;

	/** If the control is animatable in sequencer */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control)
	bool bAnimatable;

	/** True if the control has to obey translation limits. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limits)
	bool bLimitTranslation;

	/** True if the control has to obey rotation limits. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limits)
	bool bLimitRotation;

	/** True if the control has to obey scale limits. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limits)
	bool bLimitScale;

	/** True if the limits should be drawn in debug. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limits, meta = (EditCondition = "bLimitTranslation || bLimitRotation || bLimitScale"))
	bool bDrawLimits;

	/** The minimum limit of the control's value */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limits, meta = (EditCondition = "bLimitTranslation || bLimitRotation || bLimitScale"))
	FTLLRN_RigControlValue MinimumValue;

	/** The maximum limit of the control's value */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limits, meta = (EditCondition = "bLimitTranslation || bLimitRotation || bLimitScale"))
	FTLLRN_RigControlValue MaximumValue;

	/** Set to true if the gizmo is enabled in 3d */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Gizmo)
	bool bGizmoEnabled;

	/** Set to true if the gizmo is currently visible in 3d */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Gizmo, meta = (EditCondition = "bGizmoEnabled"))
	bool bGizmoVisible;

	/* This is optional UI setting - this doesn't mean this is always used, but it is optional for manipulation layer to use this*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Gizmo, meta = (EditCondition = "bGizmoEnabled"))
	FName GizmoName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Gizmo, meta = (EditCondition = "bGizmoEnabled"))
	FTransform GizmoTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Gizmo, meta = (EditCondition = "bGizmoEnabled"))
	FLinearColor GizmoColor;

	/** dependent list - direct dependent for child or anything that needs to update due to this */
	UPROPERTY(transient)
	TArray<int32> Dependents;

	/** If the control is transient and only visible in the control rig editor */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control)
	bool bIsTransientControl;

	/** If the control is transient and only visible in the control rig editor */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Control)
	TObjectPtr<UEnum> ControlEnum;

	virtual ETLLRN_RigElementType GetElementType() const override
	{
		return ETLLRN_RigElementType::Control;
	}

	const FName& GetDisplayName() const
	{
		return DisplayName.IsNone() ? Name : DisplayName;
	}

	virtual FTLLRN_RigElementKey GetParentElementKey() const
	{
		return FTLLRN_RigElementKey(ParentName, GetElementType());
	}

	virtual FTLLRN_RigElementKey GetSpaceElementKey() const
	{
		return FTLLRN_RigElementKey(SpaceName, ETLLRN_RigElementType::Null);
	}

	const FTLLRN_RigControlValue& GetValue(ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current) const
	{
		switch(InValueType)
		{
			case ETLLRN_RigControlValueType::Initial:
			{
				return InitialValue;
			}
			case ETLLRN_RigControlValueType::Minimum:
			{
				return MinimumValue;
			}
			case ETLLRN_RigControlValueType::Maximum:
			{
				return MaximumValue;
			}
			default:
			{
				break;
			}
		}
		return Value;
	}

	FTLLRN_RigControlValue& GetValue(ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current)
	{
		switch(InValueType)
		{
			case ETLLRN_RigControlValueType::Initial:
			{
				return InitialValue;
			}
			case ETLLRN_RigControlValueType::Minimum:
			{
				return MinimumValue;
			}
			case ETLLRN_RigControlValueType::Maximum:
			{
				return MaximumValue;
			}
			default:
			{
				break;
			}
		}
		return Value;
	}

	void ApplyLimits(FTLLRN_RigControlValue& InOutValue) const;

	static FProperty* FindPropertyForValueType(ETLLRN_RigControlValueType InValueType)
	{
		switch (InValueType)
		{
			case ETLLRN_RigControlValueType::Current:
			{
				return FTLLRN_RigControl::StaticStruct()->FindPropertyByName(TEXT("Value"));
			}
			case ETLLRN_RigControlValueType::Initial:
			{
				return FTLLRN_RigControl::StaticStruct()->FindPropertyByName(TEXT("InitialValue"));
			}
			case ETLLRN_RigControlValueType::Minimum:
			{
				return FTLLRN_RigControl::StaticStruct()->FindPropertyByName(TEXT("MinimumValue"));
			}
			case ETLLRN_RigControlValueType::Maximum:
			{
				return FTLLRN_RigControl::StaticStruct()->FindPropertyByName(TEXT("MaximumValue"));
			}
		}
		return nullptr;
	}

	FTransform GetTransformFromValue(ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current) const;
	void SetValueFromTransform(const FTransform& InTransform, ETLLRN_RigControlValueType InValueType = ETLLRN_RigControlValueType::Current);

};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_TLLRN_RigControlHierarchy
{
	GENERATED_BODY()

	FTLLRN_TLLRN_RigControlHierarchy();

	int32 Num() const { return Controls.Num(); }
	TArray<FTLLRN_RigControl>::RangedForIteratorType      begin()       { return Controls.begin(); }
	TArray<FTLLRN_RigControl>::RangedForConstIteratorType begin() const { return Controls.begin(); }
	TArray<FTLLRN_RigControl>::RangedForIteratorType      end()         { return Controls.end();   }
	TArray<FTLLRN_RigControl>::RangedForConstIteratorType end() const   { return Controls.end();   }

	FTLLRN_RigControl& Add(
	    const FName& InNewName,
	    ETLLRN_RigControlType InControlType,
	    const FName& InParentName,
	    const FName& InSpaceName,
	    const FTransform& InOffsetTransform,
	    const FTLLRN_RigControlValue& InValue,
	    const FName& InGizmoName,
	    const FTransform& InGizmoTransform,
	    const FLinearColor& InGizmoColor
	);

	void PostLoad();

	// Pretty weird that this type is copy/move assignable (needed for USTRUCTs) but not copy/move constructible
	FTLLRN_TLLRN_RigControlHierarchy(FTLLRN_TLLRN_RigControlHierarchy&& InOther) = delete;
	FTLLRN_TLLRN_RigControlHierarchy(const FTLLRN_TLLRN_RigControlHierarchy& InOther) = delete;
	FTLLRN_TLLRN_RigControlHierarchy& operator=(FTLLRN_TLLRN_RigControlHierarchy&& InOther) = default;
	FTLLRN_TLLRN_RigControlHierarchy& operator=(const FTLLRN_TLLRN_RigControlHierarchy& InOther) = default;

private:
	UPROPERTY(EditAnywhere, Category = FTLLRN_TLLRN_RigControlHierarchy)
	TArray<FTLLRN_RigControl> Controls;

	friend struct FTLLRN_RigHierarchyContainer;
	friend struct FTLLRN_CachedTLLRN_RigElement;
	friend class UTLLRN_ControlTLLRN_RigHierarchyModifier;
	friend class UTLLRN_ControlRig;
	friend class UTLLRN_ControlRigBlueprint;
};
