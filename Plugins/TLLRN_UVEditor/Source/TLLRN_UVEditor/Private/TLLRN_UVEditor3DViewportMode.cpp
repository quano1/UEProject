// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditor3DViewportMode.h"

#define LOCTEXT_NAMESPACE "UTLLRN_UVEditor3DViewportMode"

const FEditorModeID UTLLRN_UVEditor3DViewportMode::EM_ModeID = TEXT("EM_TLLRN_UVEditor3DViewportModeId");

UTLLRN_UVEditor3DViewportMode::UTLLRN_UVEditor3DViewportMode()
{
	Info = FEditorModeInfo(
		EM_ModeID,
		LOCTEXT("ModeName", "UV 3D Viewport Dummy Mode"),
		FSlateIcon(),
		false);
}

#undef LOCTEXT_NAMESPACE