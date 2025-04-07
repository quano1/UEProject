// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_TLLRN_RigControlHierarchy.h"
#include "Rigs/TLLRN_RigHierarchyElements.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_TLLRN_RigControlHierarchy)

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_RigControl
////////////////////////////////////////////////////////////////////////////////

void FTLLRN_RigControl::ApplyLimits(FTLLRN_RigControlValue& InOutValue) const
{
	FTLLRN_RigControlSettings Settings;
	Settings.ControlType = ControlType;
	Settings.SetupLimitArrayForType(bLimitTranslation, bLimitRotation, bLimitScale);
	InOutValue.ApplyLimits(Settings.LimitEnabled, ControlType, MinimumValue, MaximumValue);
}

FTransform FTLLRN_RigControl::GetTransformFromValue(ETLLRN_RigControlValueType InValueType) const
{
	return GetValue(InValueType).GetAsTransform(ControlType, PrimaryAxis);
}

void FTLLRN_RigControl::SetValueFromTransform(const FTransform& InTransform, ETLLRN_RigControlValueType InValueType)
{
	GetValue(InValueType).SetFromTransform(InTransform, ControlType, PrimaryAxis);

	if (InValueType == ETLLRN_RigControlValueType::Current)
	{
		ApplyLimits(GetValue(InValueType));
	}
}

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_TLLRN_RigControlHierarchy
////////////////////////////////////////////////////////////////////////////////

FTLLRN_TLLRN_RigControlHierarchy::FTLLRN_TLLRN_RigControlHierarchy()
{
}

FTLLRN_RigControl& FTLLRN_TLLRN_RigControlHierarchy::Add(
    const FName& InNewName,
    ETLLRN_RigControlType InControlType,
    const FName& InParentName,
    const FName& InSpaceName,
    const FTransform& InOffsetTransform,
    const FTLLRN_RigControlValue& InValue,
    const FName& InGizmoName,
    const FTransform& InGizmoTransform,
    const FLinearColor& InGizmoColor
)
{
	FTLLRN_RigControl NewControl;
	NewControl.Name = InNewName;
	NewControl.ControlType = InControlType;
	NewControl.ParentIndex = INDEX_NONE; // we don't support indices
	NewControl.ParentName = InParentName;
	NewControl.SpaceIndex = INDEX_NONE;
	NewControl.SpaceName = InSpaceName;
	NewControl.OffsetTransform = InOffsetTransform;
	NewControl.InitialValue = InValue;
	NewControl.Value = FTLLRN_RigControlValue();
	NewControl.GizmoName = InGizmoName;
	NewControl.GizmoTransform = InGizmoTransform;
	NewControl.GizmoColor = InGizmoColor;

	if (!NewControl.InitialValue.IsValid())
	{
		NewControl.SetValueFromTransform(FTransform::Identity, ETLLRN_RigControlValueType::Initial);
	}

	const int32 Index = Controls.Add(NewControl);
	return Controls[Index];
}

void FTLLRN_TLLRN_RigControlHierarchy::PostLoad()
{
#if WITH_EDITORONLY_DATA
	for (FTLLRN_RigControl& Control : Controls)
	{
		for (int32 ValueType = 0; ValueType <= (int32)ETLLRN_RigControlValueType::Maximum; ValueType++)
		{
			FTLLRN_RigControlValue& Value = Control.GetValue((ETLLRN_RigControlValueType)ValueType);
			if (!Value.IsValid())
			{
				Value.GetRef<FTLLRN_RigControlValue::FTransform_Float>() = Value.Storage_DEPRECATED;
			}
		}
	}
#endif
}
