// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_TLLRN_RigCurveContainerCommands.h"

#define LOCTEXT_NAMESPACE "CurveContainerCommands"

void FCurveContainerCommands::RegisterCommands()
{
	UI_COMMAND( AddCurve, "Add Curve", "Add a curve to the TLLRN_ControlRig", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
