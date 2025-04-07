// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "TLLRN_ControlRigEditorStyle.h"

class FTLLRN_ControlRigTLLRN_ModularRigCommands : public TCommands<FTLLRN_ControlRigTLLRN_ModularRigCommands>
{
public:
	FTLLRN_ControlRigTLLRN_ModularRigCommands() : TCommands<FTLLRN_ControlRigTLLRN_ModularRigCommands>
	(
		"TLLRN_ControlRigTLLRN_ModularRigModel",
		NSLOCTEXT("Contexts", "TLLRN_ModularRigModel", "Modular Rig Modules"),
		NAME_None, // "MainFrame" // @todo Fix this crash
		FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName() // Icon Style Set
	)
	{}
	
	/** Add Module at root */
	TSharedPtr< FUICommandInfo > AddModuleItem;

	/** Rename Module */
	TSharedPtr< FUICommandInfo > RenameModuleItem;

	/** Delete Module */
	TSharedPtr< FUICommandInfo > DeleteModuleItem;

	/** Mirror Module */
	TSharedPtr< FUICommandInfo > MirrorModuleItem;

	/** Reresolve Module */
	TSharedPtr< FUICommandInfo > ReresolveModuleItem;

	/** Swap Module Class */
	TSharedPtr< FUICommandInfo > SwapModuleClassItem;

	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() override;
};
