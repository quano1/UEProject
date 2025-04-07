// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SAssetEditorViewport.h"
#include "STLLRN_UVEditor3DViewportToolBar.h"

/**
 * Viewport used for live preview in UV editor. Has a custom toolbar overlay at the top.
 */
class TLLRN_UVEDITOR_API STLLRN_UVEditor3DViewport : public SAssetEditorViewport 
{
public:

	// SAssetEditorViewport
	virtual void BindCommands() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;

};
