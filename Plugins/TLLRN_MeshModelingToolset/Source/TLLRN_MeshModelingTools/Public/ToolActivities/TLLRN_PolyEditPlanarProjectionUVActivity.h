// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "BoxTypes.h"
#include "TLLRN_InteractiveToolActivity.h"
#include "ToolActivities/PolyEditActivityUtil.h"
#include "ToolContextInterfaces.h" // FViewCameraState

#include "TLLRN_PolyEditPlanarProjectionUVActivity.generated.h"

class UTLLRN_PolyEditActivityContext;
class UTLLRN_PolyEditTLLRN_PreviewMesh;
class UTLLRN_CollectSurfacePathMechanic;

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditSetUVProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = PlanarProjectUV)
	bool bShowMaterial = false;
};


/**
 *
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditPlanarProjectionUVActivity : public UTLLRN_InteractiveToolActivity,
	public IClickBehaviorTarget, public IHoverBehaviorTarget
{
	GENERATED_BODY()

public:
	// ITLLRN_InteractiveToolActivity
	virtual void Setup(UInteractiveTool* ParentTool) override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual bool CanStart() const override;
	virtual EToolActivityStartResult Start() override;
	virtual bool IsRunning() const override { return bIsRunning; }
	virtual bool CanAccept() const override;
	virtual EToolActivityEndResult End(EToolShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void Tick(float DeltaTime) override;

	// IClickBehaviorTarget API
	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos) override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;

	// IHoverBehaviorTarget implementation
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override {}
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override {}

protected:
	void Clear();
	void BeginSetUVs();
	void UpdateSetUVS();
	void ApplySetUVs();

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditSetUVProperties> SetUVProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditTLLRN_PreviewMesh> EditPreview;

	UPROPERTY()
	TObjectPtr<UTLLRN_CollectSurfacePathMechanic> SurfacePathMechanic = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditActivityContext> ActivityContext;

	bool bIsRunning = false;

	bool bPreviewUpdatePending = false;
	UE::Geometry::PolyEditActivityUtil::EPreviewMaterialType CurrentPreviewMaterial;
	UE::Geometry::FAxisAlignedBox3d ActiveSelectionBounds;
	FViewCameraState CameraState;
};
