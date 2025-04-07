// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "TLLRN_IKRigEditorStyle.h"

class FTLLRN_IKRigCommands : public TCommands<FTLLRN_IKRigCommands>
{
public:
	FTLLRN_IKRigCommands() : TCommands<FTLLRN_IKRigCommands>
	(
		"IKRig",
		NSLOCTEXT("Contexts", "IKRig", "TLLRN_IK Rig"),
		NAME_None, // "MainFrame"
		FTLLRN_IKRigEditorStyle::Get().GetStyleSetName() // Icon Style Set
	)
	{}
	
	// reset whole system to initial state
	TSharedPtr< FUICommandInfo > Reset;

	// automatically generate retarget chains
	TSharedPtr< FUICommandInfo > AutoRetargetChains;

	// automatically setup full body ik
	TSharedPtr< FUICommandInfo > AutoSetupFBIK;

	// show settings of the asset in the details panel
	TSharedPtr< FUICommandInfo > ShowAssetSettings;

	// initialize commands
	virtual void RegisterCommands() override;
};
