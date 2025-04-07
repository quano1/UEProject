// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "InteractiveTool.h"
#include "InteractiveToolBuilder.h"
#include "TLLRN_UVEditorToolAnalyticsUtils.h"
#include "Selection/TLLRN_UVToolSelectionAPI.h"
#include "Operators/TLLRN_UVEditorUVTransformOp.h"

#include "TLLRN_UVEditorTransformTool.generated.h"

class UTLLRN_UVEditorToolMeshInput;
class UTLLRN_UVEditorUVTransformProperties;
enum class ETLLRN_UVEditorUVTransformType;
class UTLLRN_UVEditorUVTransformOperatorFactory;
class UCanvas;
class UTLLRN_UVEditorTransformTool;

/**
 * Visualization settings for the TransformTool
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorTransformToolDisplayProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Draw the tool's active pivot location if needed.*/
	UPROPERTY(EditAnywhere, Category = "Tool Display Settings", meta = (DisplayName = "Display Tool Pivots"))
	bool bDrawPivots = true;
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorBaseTransformToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	// This is a pointer so that it can be updated under the builder without
	// having to set it in the mode after initializing targets.
	const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>* Targets = nullptr;

protected:
	virtual void ConfigureTool(UTLLRN_UVEditorTransformTool* NewTool) const;
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorTransformToolBuilder : public UTLLRN_UVEditorBaseTransformToolBuilder
{
	GENERATED_BODY()

protected:
	virtual void ConfigureTool(UTLLRN_UVEditorTransformTool* NewTool) const override;
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorAlignToolBuilder : public UTLLRN_UVEditorBaseTransformToolBuilder
{
	GENERATED_BODY()

protected:
	virtual void ConfigureTool(UTLLRN_UVEditorTransformTool* NewTool) const  override;
};

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorDistributeToolBuilder : public UTLLRN_UVEditorBaseTransformToolBuilder
{
	GENERATED_BODY()

protected:
	virtual void ConfigureTool(UTLLRN_UVEditorTransformTool* NewTool) const  override;
};

/**
 * 
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorTransformTool : public UInteractiveTool, public ITLLRN_UVToolSupportsSelection
{
	GENERATED_BODY()

public:

	/**
	 * The tool will operate on the meshes given here.
	 */
	virtual void SetTargets(const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn)
	{
		Targets = TargetsIn;
	}

	void SetToolMode(const ETLLRN_UVEditorUVTransformType& Mode);

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void DrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI) override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

protected:	

	TOptional<ETLLRN_UVEditorUVTransformType> ToolMode;

	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>> Targets;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorUVTransformPropertiesBase> Settings = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorTransformToolDisplayProperties> DisplaySettings = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_UVEditorUVTransformOperatorFactory>> Factories;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolSelectionAPI> TLLRN_UVToolSelectionAPI = nullptr;

	TArray<TArray<FVector2D>> PerTargetPivotLocations;

	//
	// Analytics
	//

	UE::Geometry::TLLRN_UVEditorAnalytics::FTargetAnalytics InputTargetAnalytics;
	FDateTime ToolStartTimeAnalytics;
	void RecordAnalytics();
};
