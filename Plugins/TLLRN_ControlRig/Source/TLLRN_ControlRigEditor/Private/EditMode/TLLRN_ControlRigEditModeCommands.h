// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "TLLRN_ControlRigEditorStyle.h"

class FTLLRN_ControlRigEditModeCommands : public TCommands<FTLLRN_ControlRigEditModeCommands>
{

public:
	FTLLRN_ControlRigEditModeCommands() : TCommands<FTLLRN_ControlRigEditModeCommands>
	(
		"TLLRN_ControlRigEditMode",
		NSLOCTEXT("Contexts", "RigAnimation", "Rig Animation"),
		NAME_None, // "MainFrame" // @todo Fix this crash
		FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName() // Icon Style Set
	)
	{}
	

	/** Toggles hiding all manipulators on active control rig in the viewport */
	TSharedPtr< FUICommandInfo > ToggleManipulators;

	/** Toggles hiding all manipulators on all control rigs in the viewport */
	TSharedPtr< FUICommandInfo > ToggleAllManipulators;

	/** Drag Anim Slider Tool*/
	TSharedPtr< FUICommandInfo > DragAnimSliderTool;

	/**  Change current Anim Slider Tool */
	TSharedPtr< FUICommandInfo > ChangeAnimSliderTool;

	/** Invert Transforms to Rest Pose */
	TSharedPtr< FUICommandInfo > InvertTransforms;

	/** Invert All Transforms to Rest Pose */
	TSharedPtr< FUICommandInfo > InvertAllTransforms;

	/** Set Transforms to Zero */
	TSharedPtr< FUICommandInfo > ZeroTransforms;

	/** Set All Transforms to Zero */
	TSharedPtr< FUICommandInfo > ZeroAllTransforms;

	/** Clear Selection*/
	TSharedPtr< FUICommandInfo > ClearSelection;

	/** Frame selected elements */
	TSharedPtr<FUICommandInfo> FrameSelection;

	/** Increase Shape Size */
	TSharedPtr< FUICommandInfo > IncreaseControlShapeSize;

	/** Decrease Shape Size */
	TSharedPtr< FUICommandInfo > DecreaseControlShapeSize;

	/** Reset Shape Size */
	TSharedPtr< FUICommandInfo > ResetControlShapeSize;

	/** Toggle Shape Transform Edit*/
	TSharedPtr< FUICommandInfo > ToggleControlShapeTransformEdit;

	/** Sets Passthrough Key on selected anim layers */
	TSharedPtr< FUICommandInfo > SetTLLRN_AnimLayerPassthroughKey;

	/** Opens up the space picker widget */
	TSharedPtr< FUICommandInfo > OpenSpacePickerWidget;

	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() override;
};
