// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditMode/TLLRN_ControlRigEditMode.h"

class UPersonaOptions;

class FTLLRN_ControlRigEditorEditMode : public FTLLRN_ControlRigEditMode
{
public:
	static inline const FLazyName ModeName = FLazyName(TEXT("EditMode.TLLRN_ControlRigEditor"));
	
	virtual bool IsInLevelEditor() const override { return false; }
	virtual bool AreEditingTLLRN_ControlRigDirectly() const override { return true; }

	// FEdMode interface
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;

	/* IPersonaEditMode interface */
	virtual bool GetCameraTarget(FSphere& OutTarget) const override;
	virtual class IPersonaPreviewScene& GetAnimPreviewScene() const override;
	virtual void GetOnScreenDebugInfo(TArray<FText>& OutDebugInfo) const override;

	/** If set to true the edit mode will additionally render all bones */
	bool bDrawHierarchyBones = false;

	/** Drawing options */
	UPersonaOptions* ConfigOption = nullptr;
};

class FTLLRN_ModularRigEditorEditMode : public FTLLRN_ControlRigEditorEditMode
{
public:

	static inline const FLazyName ModeName = FLazyName(TEXT("EditMode.TLLRN_ModularRigEditor"));
};
