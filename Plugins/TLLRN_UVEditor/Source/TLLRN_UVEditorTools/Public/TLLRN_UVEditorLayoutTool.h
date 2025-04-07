// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "InteractiveTool.h"
#include "InteractiveToolBuilder.h"
#include "TLLRN_UVEditorToolAnalyticsUtils.h"
#include "Selection/TLLRN_UVToolSelectionAPI.h"

#include "TLLRN_UVEditorLayoutTool.generated.h"

class UTLLRN_UVEditorToolMeshInput;
class UUVLayoutProperties;
class UUVLayoutOperatorFactory;
class UTLLRN_UVTool2DViewportAPI;

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorLayoutToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	// This is a pointer so that it can be updated under the builder without
	// having to set it in the mode after initializing targets.
	const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>* Targets = nullptr;
};

/**
 * 
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVEditorLayoutTool : public UInteractiveTool, public ITLLRN_UVToolSupportsSelection
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

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;
protected:

	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>> Targets;

	UPROPERTY()
	TObjectPtr<UUVLayoutProperties> Settings = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UUVLayoutOperatorFactory>> Factories;

	UPROPERTY()
	TObjectPtr<UTLLRN_UVToolSelectionAPI> TLLRN_UVToolSelectionAPI = nullptr;

	//~ For UDIM information access
	UPROPERTY()
	TObjectPtr< UTLLRN_UVTool2DViewportAPI> TLLRN_UVTool2DViewportAPI = nullptr; 

	//
	// Analytics
	//

	UE::Geometry::TLLRN_UVEditorAnalytics::FTargetAnalytics InputTargetAnalytics;
	FDateTime ToolStartTimeAnalytics;
	void RecordAnalytics();
};
