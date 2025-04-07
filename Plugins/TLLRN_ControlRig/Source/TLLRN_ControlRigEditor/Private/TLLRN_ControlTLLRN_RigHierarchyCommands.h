// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "TLLRN_ControlRigEditorStyle.h"

class FTLLRN_ControlTLLRN_RigHierarchyCommands : public TCommands<FTLLRN_ControlTLLRN_RigHierarchyCommands>
{
public:
	FTLLRN_ControlTLLRN_RigHierarchyCommands() : TCommands<FTLLRN_ControlTLLRN_RigHierarchyCommands>
	(
		"TLLRN_ControlTLLRN_RigHierarchy",
		NSLOCTEXT("Contexts", "TLLRN_RigHierarchy", "Rig Hierarchy"),
		NAME_None, // "MainFrame" // @todo Fix this crash
		FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName() // Icon Style Set
	)
	{}
	
	/** Add Item at origin */
	TSharedPtr< FUICommandInfo > AddBoneItem;

	/** Add Item at origin */
	TSharedPtr< FUICommandInfo > AddControlItem;

	/** Add an animation channel */
	TSharedPtr< FUICommandInfo > AddAnimationChannelItem;

	/** Add Item at origin */
	TSharedPtr< FUICommandInfo > AddNullItem;

	/** Add Item at origin */
	TSharedPtr< FUICommandInfo > AddConnectorItem;

	/** Add Item at origin */
	TSharedPtr< FUICommandInfo > AddSocketItem;

	/** Find all references of an item in the graphs */
	TSharedPtr< FUICommandInfo > FindReferencesOfItem;
	
	/** Duplicate currently selected items */
	TSharedPtr< FUICommandInfo > DuplicateItem;

	/** Mirror currently selected items */
	TSharedPtr< FUICommandInfo > MirrorItem;

	/** Delete currently selected items */
	TSharedPtr< FUICommandInfo > DeleteItem;

	/** Rename selected item */
	TSharedPtr< FUICommandInfo > RenameItem;

	/** Copy the selected items. */
	TSharedPtr< FUICommandInfo > CopyItems;

	/** Paste the selected items. */
	TSharedPtr< FUICommandInfo > PasteItems;

	/** Paste Local Transform", "Paste the local transforms. */
	TSharedPtr< FUICommandInfo > PasteLocalTransforms;

	/** Paste Global Transform", "Paste the global transforms. */
	TSharedPtr< FUICommandInfo > PasteGlobalTransforms;

	/* Reset transform */
	TSharedPtr<FUICommandInfo> ResetTransform;

	/* Reset all transforms */
	TSharedPtr<FUICommandInfo> ResetAllTransforms;

	/* Reset space */
	TSharedPtr<FUICommandInfo> ResetNull;

	/* Set initial transform from current */
	TSharedPtr<FUICommandInfo> SetInitialTransformFromCurrentTransform;

	/* set initial transform from closest bone */
	TSharedPtr<FUICommandInfo> SetInitialTransformFromClosestBone;

	/* set shape transform from the control's current local transform and reset the control's transform */
	TSharedPtr<FUICommandInfo> SetShapeTransformFromCurrent;

	/* frames the selection in the tree */
	TSharedPtr<FUICommandInfo> FrameSelection;

	/* sets the bone transform using a shape */
	TSharedPtr<FUICommandInfo> ControlBoneTransform;

	/* sets the space transform using a shape */
	TSharedPtr<FUICommandInfo> ControlSpaceTransform;

	/* Unparents the selected elements from the hierarchy */
	TSharedPtr<FUICommandInfo> Unparent;

	/* Flatten hierarchy on filter */
	TSharedPtr< FUICommandInfo > FilteringFlattensHierarchy;

	/* Hide parents on filter */
	TSharedPtr< FUICommandInfo > HideParentsWhenFiltering;

	/* Show short names */
	TSharedPtr< FUICommandInfo > ShowShortNames;

	/* Show imported bones */
	TSharedPtr< FUICommandInfo > ShowImportedBones;

	/* Show bones */
	TSharedPtr< FUICommandInfo > ShowBones;

	/* Show controls */
	TSharedPtr< FUICommandInfo > ShowControls;

	/* Show spaces */
	TSharedPtr< FUICommandInfo > ShowNulls;

	/* Show physics */
	TSharedPtr< FUICommandInfo > ShowPhysics;

	/* Show references */
	TSharedPtr< FUICommandInfo > ShowReferences;

	/* Show sockets */
	TSharedPtr< FUICommandInfo > ShowSockets;

	/** Toggle Shape Transform Edit*/
	TSharedPtr< FUICommandInfo > ToggleControlShapeTransformEdit;

	/** Space switch as it would look like for animator */
	TSharedPtr< FUICommandInfo > SpaceSwitching;

	/** Whether to tint the icons with the element color */
	TSharedPtr< FUICommandInfo > ShowIconColors;

	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() override;
};
