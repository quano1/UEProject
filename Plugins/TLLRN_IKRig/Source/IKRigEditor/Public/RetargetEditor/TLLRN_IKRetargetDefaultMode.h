// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_IKRetargetEditorController.h"

#include "Retargeter/TLLRN_IKRetargeter.h"
#include "IPersonaEditMode.h"

class UTLLRN_IKRigProcessor;
class FTLLRN_IKRetargetEditorController;
class FTLLRN_IKRetargetEditor;
class FIKRetargetPreviewScene;
struct FGizmoState;

class FTLLRN_IKRetargetDefaultMode : public IPersonaEditMode
{
public:
	static FName ModeName;
	
	FTLLRN_IKRetargetDefaultMode() = default;

	/** glue for all the editor parts to communicate */
	void SetEditorController(const TSharedPtr<FTLLRN_IKRetargetEditorController> InEditorController) { EditorController = InEditorController; };

	/** IPersonaEditMode interface */
	virtual bool GetCameraTarget(FSphere& OutTarget) const override;
	virtual IPersonaPreviewScene& GetAnimPreviewScene() const override;
	/** END IPersonaEditMode interface */

	/** FEdMode interface */
	virtual void Initialize() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override { return true; };
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	virtual void Enter() override;
	virtual void Exit() override;
	/** END FEdMode interface */

private:
	void RenderDebugProxies(FPrimitiveDrawInterface* PDI, const FTLLRN_IKRetargetEditorController* Controller) const;
	
	// the skeleton currently being edited
	ETLLRN_RetargetSourceOrTarget SkeletonMode;
	
	/** The hosting app */
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;

	UE::Widget::EWidgetMode CurrentWidgetMode;
	bool bIsTranslating = false;

	bool bIsInitialized = false;
};
