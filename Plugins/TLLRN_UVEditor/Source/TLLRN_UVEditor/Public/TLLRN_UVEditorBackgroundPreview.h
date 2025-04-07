// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Drawing/PreviewGeometryActor.h"
#include "Drawing/TriangleSetComponent.h"
#include "InteractiveTool.h"
#include "GeometryBase.h"
#include "TLLRN_UVEditorBackgroundPreview.generated.h"


/**
 * Enum to control the background visualiztion mode
 */
UENUM()
enum class ETLLRN_UVEditorBackgroundSourceType
{
	Checkerboard,
	Texture,
	Material
};

/**
 * Visualization settings for the UTLLRN_UVEditorBackgroundPreview
 */
UCLASS()
class TLLRN_UVEDITOR_API UTLLRN_UVEditorBackgroundPreviewProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Should the background be shown (Alt+B)*/
	UPROPERTY(EditAnywhere, Category = Background, meta = (DisplayName = "Display Background"))
	bool bVisible = false;

	/** Source of background visuals */
	UPROPERTY(EditAnywhere, Category = Background, meta = (EditCondition = "bVisible", DisplayName = "Background Source"))
	ETLLRN_UVEditorBackgroundSourceType SourceType = ETLLRN_UVEditorBackgroundSourceType::Checkerboard;

	/** Display a background based on the selected texture */
	UPROPERTY(EditAnywhere, Category = Background, meta = (EditCondition = "SourceType==ETLLRN_UVEditorBackgroundSourceType::Texture && bVisible", EditConditionHides = true))
	TObjectPtr<UTexture2D> SourceTexture;

	/** Display a background based on the selected material */
	UPROPERTY(EditAnywhere, Category = Background, meta = (EditCondition = "SourceType==ETLLRN_UVEditorBackgroundSourceType::Material && bVisible", EditConditionHides = true))
	TObjectPtr<UMaterial> SourceMaterial;

	UPROPERTY()
	TArray<int32> TLLRN_UDIMBlocks;

	UPROPERTY()
	bool bUDIMsEnabled = false;
};

/**
  Serves as a container for the background texture/material display in the TLLRN_UVEditor. This class is responsible for managing the quad
  drawn behind the grid, as well as maintaining the texture and material choices from the user to display.
 */
UCLASS(Transient)
class TLLRN_UVEDITOR_API UTLLRN_UVEditorBackgroundPreview : public UPreviewGeometry
{
	GENERATED_BODY()

public:

	/**
	 * Client must call this every frame for changes to .Settings to be reflected in rendered result.
	 */
	void OnTick(float DeltaTime);
	
public:
	/** Visualization settings */
	UPROPERTY()
	TObjectPtr<UTLLRN_UVEditorBackgroundPreviewProperties> Settings;

	/** The component containing the quad visualization */
	UPROPERTY()
	TObjectPtr<UTriangleSetComponent> BackgroundComponent;

	/** The active material being displayed for the background */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BackgroundMaterial;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBackgroundMaterialChange, TObjectPtr<UMaterialInstanceDynamic> MaterialInstance);
	FOnBackgroundMaterialChange OnBackgroundMaterialChange;

protected:
	virtual void OnCreated() override;

	bool bSettingsModified = false;

	void UpdateVisibility();
	void UpdateBackground();
};

