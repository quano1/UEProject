// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "EditorViewportClient.h"

class UTLLRN_UVToolViewportButtonsAPI;

// Types of camera motion for the UV Editor 3D viewport
enum ETLLRN_UVEditor3DViewportClientCameraMode {
	Orbit,
	Fly
};

/**
 * Viewport client for the 3d live preview in the UV editor. Currently same as editor viewport
 * client but doesn't allow editor gizmos/widgets, and alters orbit camera control.
 */
class TLLRN_UVEDITOR_API FTLLRN_UVEditor3DViewportClient : public FEditorViewportClient
{
public:

	FTLLRN_UVEditor3DViewportClient(FEditorModeTools* InModeTools, FPreviewScene* InPreviewScene = nullptr,
		const TWeakPtr<SEditorViewport>& InEditorViewportWidget = nullptr, UTLLRN_UVToolViewportButtonsAPI* ViewportButtonsAPI = nullptr);

	virtual ~FTLLRN_UVEditor3DViewportClient() {}

	// FEditorViewportClient
	virtual bool ShouldOrbitCamera() const override;
	bool CanSetWidgetMode(UE::Widget::EWidgetMode NewMode) const override {	return false; }
	void SetWidgetMode(UE::Widget::EWidgetMode NewMode) override {}
	UE::Widget::EWidgetMode GetWidgetMode() const override { return UE::Widget::EWidgetMode::WM_None; }
	void FocusCameraOnSelection();
public:

	void SetCameraMode(ETLLRN_UVEditor3DViewportClientCameraMode CameraModeIn) { CameraMode = CameraModeIn; };
	ETLLRN_UVEditor3DViewportClientCameraMode GetCameraMode() const { return CameraMode; };

protected:

	// Enforce Orbit camera for UV editor live preview viewport. Use this instead of the base class orbit camera flag
	// to allow for expected behaviors of the base class when in fly camera mode.
	ETLLRN_UVEditor3DViewportClientCameraMode CameraMode = ETLLRN_UVEditor3DViewportClientCameraMode::Orbit;
	UTLLRN_UVToolViewportButtonsAPI* ViewportButtonsAPI;

};