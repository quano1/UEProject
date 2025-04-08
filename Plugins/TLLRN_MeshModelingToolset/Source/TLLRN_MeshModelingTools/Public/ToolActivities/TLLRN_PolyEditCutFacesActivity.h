// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "TLLRN_InteractiveToolActivity.h"
#include "InteractiveToolChange.h" //FToolCommandChange
#include "ToolContextInterfaces.h" // FViewCameraState

#include "TLLRN_PolyEditCutFacesActivity.generated.h"

class UTLLRN_PolyEditActivityContext;
class UTLLRN_PolyEditTLLRN_PreviewMesh;
class UTLLRN_CollectSurfacePathMechanic;

UENUM()
enum class ETLLRN_PolyEditCutPlaneOrientation
{
	FaceNormals,
	ViewDirection
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditCutProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Cut)
	ETLLRN_PolyEditCutPlaneOrientation Orientation = ETLLRN_PolyEditCutPlaneOrientation::FaceNormals;

	UPROPERTY(EditAnywhere, Category = Cut)
	bool bSnapToVertices = true;
};


/**
 *
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditCutFacesActivity : public UTLLRN_InteractiveToolActivity,
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
	void BeginCutFaces();
	void ApplyCutFaces();

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditCutProperties> CutProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditTLLRN_PreviewMesh> EditPreview;

	UPROPERTY()
	TObjectPtr<UTLLRN_CollectSurfacePathMechanic> SurfacePathMechanic = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditActivityContext> ActivityContext;

	bool bIsRunning = false;
	int32 ActivityStamp = 1;

	FViewCameraState CameraState;

	friend class FTLLRN_PolyEditCutFacesActivityFirstPointChange;
};

/**
 * This should get emitted when setting the first point so that we can undo it.
 */
class TLLRN_MESHMODELINGTOOLS_API FTLLRN_PolyEditCutFacesActivityFirstPointChange : public FToolCommandChange
{
public:
	FTLLRN_PolyEditCutFacesActivityFirstPointChange(int32 CurrentActivityStamp)
		: ActivityStamp(CurrentActivityStamp)
	{};

	virtual void Apply(UObject* Object) override {};
	virtual void Revert(UObject* Object) override;
	virtual bool HasExpired(UObject* Object) const override
	{
		return bHaveDoneUndo || Cast<UTLLRN_PolyEditCutFacesActivity>(Object)->ActivityStamp != ActivityStamp;
	}
	virtual FString ToString() const override
	{
		return TEXT("FTLLRN_PolyEditCutFacesActivityFirstPointChange");
	}

protected:
	int32 ActivityStamp;
	bool bHaveDoneUndo = false;
};