// Copyright Epic Games, Inc. All Rights Reserved.

#include "STLLRN_UVEditor3DViewport.h"

#include "EditorViewportClient.h"
#include "STLLRN_UVEditor3DViewportToolBar.h"
#include "TLLRN_UVEditorCommands.h"
#include "TLLRN_UVEditor3DViewportClient.h"

#define LOCTEXT_NAMESPACE "STLLRN_UVEditor3DViewport"

void STLLRN_UVEditor3DViewport::BindCommands()
{
	SAssetEditorViewport::BindCommands();

	const FTLLRN_UVEditorCommands& CommandInfos = FTLLRN_UVEditorCommands::Get();
	CommandList->MapAction(
		CommandInfos.EnableOrbitCamera,
		FExecuteAction::CreateLambda([this]() {
			StaticCastSharedPtr<FTLLRN_UVEditor3DViewportClient>(Client)->SetCameraMode(ETLLRN_UVEditor3DViewportClientCameraMode::Orbit);
			}),
		FCanExecuteAction::CreateLambda([this]() { return true; }),
		FIsActionChecked::CreateLambda([this]() { return StaticCastSharedPtr<FTLLRN_UVEditor3DViewportClient>(Client)->GetCameraMode() == ETLLRN_UVEditor3DViewportClientCameraMode::Orbit; }));

	CommandList->MapAction(
		CommandInfos.EnableFlyCamera,
		FExecuteAction::CreateLambda([this]() {
			StaticCastSharedPtr<FTLLRN_UVEditor3DViewportClient>(Client)->SetCameraMode(ETLLRN_UVEditor3DViewportClientCameraMode::Fly);
			}),
		FCanExecuteAction::CreateLambda([this]() { return true; }),
		FIsActionChecked::CreateLambda([this]() { return StaticCastSharedPtr<FTLLRN_UVEditor3DViewportClient>(Client)->GetCameraMode() == ETLLRN_UVEditor3DViewportClientCameraMode::Fly; }));

	CommandList->MapAction(
		CommandInfos.SetFocusCamera,
		FExecuteAction::CreateLambda([this]() {
			StaticCastSharedPtr<FTLLRN_UVEditor3DViewportClient>(Client)->FocusCameraOnSelection();
		}),
		FCanExecuteAction::CreateLambda([this]() { 
			return true;
		}));
}

TSharedPtr<SWidget> STLLRN_UVEditor3DViewport::MakeViewportToolbar()
{
	return SNew(STLLRN_UVEditor3DViewportToolBar, SharedThis(this))
		.CommandList(CommandList);
}

#undef LOCTEXT_NAMESPACE
