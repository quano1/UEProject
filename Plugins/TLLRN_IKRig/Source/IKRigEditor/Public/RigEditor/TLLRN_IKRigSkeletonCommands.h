// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "TLLRN_IKRigEditorStyle.h"

class FTLLRN_IKRigSkeletonCommands : public TCommands<FTLLRN_IKRigSkeletonCommands>
{
public:
	FTLLRN_IKRigSkeletonCommands() : TCommands<FTLLRN_IKRigSkeletonCommands>
	(
		"TLLRN_IKRigSkeleton",
		NSLOCTEXT("Contexts", "TLLRN_IKRigSkeleton", "IK Rig Skeleton"),
		NAME_None,
		FTLLRN_IKRigEditorStyle::Get().GetStyleSetName() // Icon Style Set
	)
	{}
	
	/** create new goal on selected bone */
	TSharedPtr< FUICommandInfo > NewGoal;
	/** delete selected element */
	TSharedPtr< FUICommandInfo > DeleteElement;
	/** add goal to selected solvers */
	TSharedPtr< FUICommandInfo > ConnectGoalToSolver;
	/** remove goal to selected solvers */
	TSharedPtr< FUICommandInfo > DisconnectGoalFromSolver;

	/** set root bone on selected solvers */
	TSharedPtr< FUICommandInfo > SetRootBoneOnSolver;

	/** set end bone on selected solvers (Pole Solvers only)*/
	TSharedPtr< FUICommandInfo > SetEndBoneOnSolver;
	
	/** add bone setting to bone */
	TSharedPtr< FUICommandInfo > AddBoneSettings;
	/** remove bone setting from bone */
	TSharedPtr< FUICommandInfo > RemoveBoneSettings;
	
	/** remove bone from solve */
	TSharedPtr< FUICommandInfo > ExcludeBone;
	/** add bone to solve */
	TSharedPtr< FUICommandInfo > IncludeBone;

	/** create retarget chain */
	TSharedPtr< FUICommandInfo > NewRetargetChain;
	/** set the retarget root */
	TSharedPtr< FUICommandInfo > SetRetargetRoot;
	/** clear the retarget root */
	TSharedPtr< FUICommandInfo > ClearRetargetRoot;

	/** rename selected goal */
	TSharedPtr< FUICommandInfo > RenameGoal;
	
	/** initialize commands */
	virtual void RegisterCommands() override;
};
