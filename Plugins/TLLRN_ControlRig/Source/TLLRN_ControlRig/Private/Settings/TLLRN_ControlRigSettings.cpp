// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/TLLRN_ControlRigSettings.h"
#include "TLLRN_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigSettings)

UTLLRN_ControlRigSettings::UTLLRN_ControlRigSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	DefaultShapeLibrary = LoadObject<UTLLRN_ControlRigShapeLibrary>(nullptr, TEXT("/TLLRN_ControlRig/Controls/DefaultGizmoLibraryNormalized.DefaultGizmoLibraryNormalized"));
	DefaultRootModule = TEXT("/TLLRN_ControlRig/Modules/Root.Root");
#endif
}

UTLLRN_ControlRigEditorSettings::UTLLRN_ControlRigEditorSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
#if WITH_EDITORONLY_DATA
    , bResetControlTransformsOnCompile(true)
#endif
{
#if WITH_EDITORONLY_DATA
	bResetControlsOnCompile = true;
	bResetControlsOnPinValueInteraction = false;
	bResetPoseWhenTogglingEventQueue = true;
	bEnableUndoForPoseInteraction = true;

	ConstructionEventBorderColor = FLinearColor::Red;
	BackwardsSolveBorderColor = FLinearColor::Yellow;
	BackwardsAndForwardsBorderColor = FLinearColor::Blue;
	bShowStackedHierarchy = false;
	MaxStackSize = 16;
	bLeftMouseDragDoesMarquee = true;
#endif
}

