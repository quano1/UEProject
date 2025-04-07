// Copyright Epic Games, Inc. All Rights Reserved.

#include "STLLRN_UVEditor2DViewport.h"

#include "STLLRN_UVEditor2DViewportToolBar.h"
#include "TLLRN_UVEditor2DViewportClient.h"
#include "TLLRN_UVEditorCommands.h"
#include "ContextObjects/TLLRN_UVToolContextObjects.h" // UTLLRN_UVToolViewportButtonsAPI::ESelectionMode

#define LOCTEXT_NAMESPACE "STLLRN_UVEditor2DViewport"

void STLLRN_UVEditor2DViewport::BindCommands()
{
	SAssetEditorViewport::BindCommands();

	const FTLLRN_UVEditorCommands& CommandInfos = FTLLRN_UVEditorCommands::Get();
	CommandList->MapAction(
		CommandInfos.VertexSelection,
		FExecuteAction::CreateLambda([this]() {
			StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->SetSelectionMode(UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Vertex); 
		}),
		FCanExecuteAction::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->AreSelectionButtonsEnabled(); 
		}),
		FIsActionChecked::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->GetSelectionMode() == UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Vertex;
		}));

	CommandList->MapAction(
		CommandInfos.EdgeSelection,
		FExecuteAction::CreateLambda([this]() {
			StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->SetSelectionMode(UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Edge); 
		}),
		FCanExecuteAction::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->AreSelectionButtonsEnabled(); 
		}),
		FIsActionChecked::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->GetSelectionMode() == UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Edge;
		}));

	CommandList->MapAction(
		CommandInfos.TriangleSelection,
		FExecuteAction::CreateLambda([this]() {
			StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->SetSelectionMode(UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Triangle); 
		}),
		FCanExecuteAction::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->AreSelectionButtonsEnabled(); 
		}),
		FIsActionChecked::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->GetSelectionMode() == UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Triangle;
		}));

	CommandList->MapAction(
		CommandInfos.IslandSelection,
		FExecuteAction::CreateLambda([this]() {
			StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->SetSelectionMode(UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Island); 
		}),
		FCanExecuteAction::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->AreSelectionButtonsEnabled(); 
		}),
		FIsActionChecked::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->GetSelectionMode() == UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Island;
		}));

	CommandList->MapAction(
		CommandInfos.FullMeshSelection,
		FExecuteAction::CreateLambda([this]() {
			StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->SetSelectionMode(UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Mesh); 
		}),
		FCanExecuteAction::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->AreSelectionButtonsEnabled(); 
		}),
		FIsActionChecked::CreateLambda([this]() { 
			return StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client)->GetSelectionMode() == UTLLRN_UVToolViewportButtonsAPI::ESelectionMode::Mesh;
		}));
}

void STLLRN_UVEditor2DViewport::AddOverlayWidget(TSharedRef<SWidget> OverlaidWidget, int32 ZOrder)
{
	ViewportOverlay->AddSlot(ZOrder)
	[
		OverlaidWidget
	];
}

void STLLRN_UVEditor2DViewport::RemoveOverlayWidget(TSharedRef<SWidget> OverlaidWidget)
{
	ViewportOverlay->RemoveSlot(OverlaidWidget);
}

TSharedPtr<SWidget> STLLRN_UVEditor2DViewport::MakeViewportToolbar()
{
	return SNew(STLLRN_UVEditor2DViewportToolBar)
		.CommandList(CommandList)
		.Viewport2DClient(StaticCastSharedPtr<FTLLRN_UVEditor2DViewportClient>(Client));
}

bool STLLRN_UVEditor2DViewport::IsWidgetModeActive(UE::Widget::EWidgetMode Mode) const
{
	return static_cast<FTLLRN_UVEditor2DViewportClient*>(Client.Get())->AreWidgetButtonsEnabled() 
		&& Client->GetWidgetMode() == Mode;
}

#undef LOCTEXT_NAMESPACE
