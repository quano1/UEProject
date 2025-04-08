// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "TLLRN_ModelingOperators.h" //IDynamicMeshOperatorFactory
#include "InteractiveTool.h" //UInteractiveToolPropertySet
#include "InteractiveToolBuilder.h" //UInteractiveToolBuilder
#include "TLLRN_MeshOpPreviewHelpers.h" //FDynamicMeshOpResult
#include "Properties/TLLRN_MeshMaterialProperties.h"
#include "PropertySets/TLLRN_CreateMeshObjectTypeProperties.h"
#include "Properties/TLLRN_RevolveProperties.h"

#include "TLLRN_DrawAndRevolveTool.generated.h"

class UTLLRN_CollectSurfacePathMechanic;
class UTLLRN_ConstructionPlaneMechanic;
class UTLLRN_CurveControlPointsMechanic;

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_DrawAndRevolveToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_RevolveToolProperties : public UTLLRN_RevolveProperties
{
	GENERATED_BODY()

public:

	/** Determines how end caps are created. This is not relevant if the end caps are not visible or if the path is not closed. */
	UPROPERTY(EditAnywhere, Category = Revolve, AdvancedDisplay, meta = (DisplayAfter = "QuadSplitMode",
		EditCondition = "HeightOffsetPerDegree != 0 || RevolveDegrees != 360", ValidEnumValues = "None, CenterFan, Delaunay"))
	ETLLRN_RevolvePropertiesCapFillMode CapFillMode = ETLLRN_RevolvePropertiesCapFillMode::Delaunay;

	/** Connect the ends of an open path to the axis to add caps to the top and bottom of the revolved result.
	  * This is not relevant for paths that are already closed. */
	UPROPERTY(EditAnywhere, Category = Revolve, AdvancedDisplay)
	bool bClosePathToAxis = true;
	
	/** Sets the draw plane origin. The revolution axis is the X axis in the plane. */
	UPROPERTY(EditAnywhere, Category = DrawPlane, meta = (DisplayName = "Origin", EditCondition = "bAllowedToEditDrawPlane", HideEditConditionToggle,
		Delta = 5, LinearDeltaSensitivity = 1))
	FVector DrawPlaneOrigin = FVector(0, 0, 0);

	/** Sets the draw plane orientation. The revolution axis is the X axis in the plane. */
	UPROPERTY(EditAnywhere, Category = DrawPlane, meta = (DisplayName = "Orientation", EditCondition = "bAllowedToEditDrawPlane", HideEditConditionToggle, 
		UIMin = -180, UIMax = 180, ClampMin = -180000, ClampMax = 180000))
	FRotator DrawPlaneOrientation = FRotator(90, 0, 0);

	/** Enables snapping while editing the path. */
	UPROPERTY(EditAnywhere, Category = Snapping)
	bool bEnableSnapping = true;

	// Not user visible- used to disallow draw plane modification.
	UPROPERTY(meta = (TransientToolProperty))
	bool bAllowedToEditDrawPlane = true;

protected:
	virtual ETLLRN_RevolvePropertiesCapFillMode GetCapFillMode() const override
	{
		return CapFillMode;
	}
};

UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_RevolveOperatorFactory : public UObject, public UE::Geometry::IDynamicMeshOperatorFactory
{
	GENERATED_BODY()

public:
	// IDynamicMeshOperatorFactory API
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	UPROPERTY()
	TObjectPtr<UTLLRN_DrawAndRevolveTool> RevolveTool;
};


/** Draws a profile curve and revolves it around an axis. */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_DrawAndRevolveTool : public UInteractiveTool
{
	GENERATED_BODY()

public:
	virtual void SetWorld(UWorld* World) { TargetWorld = World; }

	virtual void RegisterActions(FInteractiveToolActionSet& ActionSet) override;
	virtual void OnPointDeletionKeyPress();

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	void SetInitialDrawFrame(UE::Geometry::FFrame3d InFrame)
	{
		InitialDrawFrame = InFrame;
	}

protected:

	UWorld* TargetWorld;

	FViewCameraState CameraState;

	// This information is replicated in the user-editable transform in the settings and in the PlaneMechanic
	// plane, but the tool turned out to be much easier to write and edit with this decoupling.
	FVector3d RevolutionAxisOrigin;
	FVector3d RevolutionAxisDirection;

	// The initial frame, used in tool setup to place the axis
	UE::Geometry::FFrame3d InitialDrawFrame;

	bool bProfileCurveComplete = false;

	void UpdateRevolutionAxis();

	UPROPERTY()
	TObjectPtr<UTLLRN_CurveControlPointsMechanic> ControlPointsMechanic = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_ConstructionPlaneMechanic> PlaneMechanic = nullptr;

	/** Property set for type of output object (StaticMesh, Volume, etc) */
	UPROPERTY()
	TObjectPtr<UTLLRN_CreateMeshObjectTypeProperties> OutputTypeProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_RevolveToolProperties> Settings = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_NewTLLRN_MeshMaterialProperties> MaterialProperties;

	UPROPERTY()
	TObjectPtr<UTLLRN_MeshOpPreviewWithBackgroundCompute> Preview = nullptr;

	void StartPreview();

	void GenerateAsset(const FDynamicMeshOpResult& Result);

	friend class UTLLRN_RevolveOperatorFactory;
};
