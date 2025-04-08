// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "GeometryBase.h"
#include "GroupTopology.h" // FGroupTopologySelection
#include "TLLRN_InteractiveToolActivity.h"
#include "FrameTypes.h"
#include "TLLRN_ModelingOperators.h"

#include "TLLRN_PolyEditBevelEdgeActivity.generated.h"

class UTLLRN_PolyEditActivityContext;
class UTLLRN_PolyEditTLLRN_PreviewMesh;
PREDECLARE_GEOMETRY(class FDynamicMesh3);



UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditBevelEdgeProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** Distance that each beveled mesh edge is inset from its initial position */
	UPROPERTY(EditAnywhere, Category = Bevel, meta = (UIMin = ".001", UIMax = "100", ClampMin = "0.0001", ClampMax = "10000", SliderExponent = "3"))
	double BevelDistance = 4.0;

	/** Number of edge loops added along the bevel faces */
	UPROPERTY(EditAnywhere, Category = Bevel, meta=(UIMax="10", ClampMin="0", ClampMax="1000"))
	int Subdivisions = 0;

	/** Roundness of the bevel. Ignored if Subdivisions = 0. */
	UPROPERTY(EditAnywhere, Category = Bevel, meta = (UIMin = "-2", UIMax="2"))
	float RoundWeight = 1.0;

	/** If true, when faces on either side of a beveled mesh edges have the same Material ID, beveled edge will be set to that Material ID. Otherwise SetMaterialID is used. */
	UPROPERTY(EditAnywhere, Category = Bevel)
	bool bInferMaterialID = true;

	/** Material ID to set on the new faces introduced by bevel operation, unless bInferMaterialID=true and non-ambiguous MaterialID can be inferred from adjacent faces */
	UPROPERTY(EditAnywhere, Category = Bevel, meta = (ClampMin="0", NoSpinbox))
	int SetMaterialID = 0;
};


/**
 * 
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_PolyEditBevelEdgeActivity : public UTLLRN_InteractiveToolActivity,
	public UE::Geometry::IDynamicMeshOperatorFactory

{
	GENERATED_BODY()

public:

	// ITLLRN_InteractiveToolActivity
	virtual void Setup(UInteractiveTool* ParentTool) override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual bool CanStart() const override;
	virtual EToolActivityStartResult Start() override;
	virtual bool IsRunning() const override { return bIsRunning; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;
	virtual EToolActivityEndResult End(EToolShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void Tick(float DeltaTime) override;

	// IDynamicMeshOperatorFactory
	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditBevelEdgeProperties> BevelProperties = nullptr;

protected:

	virtual void BeginBevel();
	virtual void ApplyBevel();
	virtual void EndInternal();

	UPROPERTY()
	TObjectPtr<UTLLRN_PolyEditActivityContext> ActivityContext;

	bool bIsRunning = false;

	UE::Geometry::FGroupTopologySelection ActiveSelection;
};
