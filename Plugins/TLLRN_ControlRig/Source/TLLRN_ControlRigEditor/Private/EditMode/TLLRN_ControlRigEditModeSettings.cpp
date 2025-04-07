// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/TLLRN_ControlRigEditModeSettings.h"
#include "EditorModeManager.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigEditModeSettings)

UTLLRN_ControlRigEditModeSettings::FOnUpdateSettings UTLLRN_ControlRigEditModeSettings::OnSettingsChange;

void UTLLRN_ControlRigEditModeSettings::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UTLLRN_ControlRigEditModeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
#if WITH_EDITOR
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigEditModeSettings, GizmoScale))
	{
		GizmoScaleDelegate.Broadcast(GizmoScale);
	}
#endif
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		// Dragging spinboxes causes this to be called every frame so we wait until they've finished dragging before saving.
		SaveConfig();
	}
	OnSettingsChange.Broadcast(this);
}

#if WITH_EDITOR
void UTLLRN_ControlRigEditModeSettings::PostEditUndo()
{
	GizmoScaleDelegate.Broadcast(GizmoScale);
	OnSettingsChange.Broadcast(this);
}
#endif


