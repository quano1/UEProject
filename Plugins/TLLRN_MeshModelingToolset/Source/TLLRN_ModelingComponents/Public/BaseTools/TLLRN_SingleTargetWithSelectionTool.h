// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SingleSelectionTool.h"
#include "InteractiveToolBuilder.h"
#include "Selections/GeometrySelection.h"
#include "TLLRN_SingleTargetWithSelectionTool.generated.h"

class UTLLRN_PreviewGeometry;
class UTLLRN_GeometrySelectionVisualizationProperties;

PREDECLARE_GEOMETRY(class FDynamicMesh3)
PREDECLARE_GEOMETRY(class FGroupTopology)

UCLASS(Transient, Abstract)
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_SingleTargetWithSelectionToolBuilder : public UInteractiveToolWithToolTargetsBuilder
{
	GENERATED_BODY()
public:
	/** @return true if a single mesh source can be found in the active selection */
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;

	/** @return new Tool instance initialized with selected mesh source */
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	/** @return new Tool instance. Override this in subclasses to build a different Tool class type */
	virtual UTLLRN_SingleTargetWithSelectionTool* CreateNewTool(const FToolBuilderState& SceneState) const PURE_VIRTUAL(UTLLRN_SingleTargetWithSelectionToolBuilder::CreateNewTool, return nullptr; );

	/** Called by BuildTool to configure the Tool with the input MeshSource based on the SceneState */
	virtual void InitializeNewTool(UTLLRN_SingleTargetWithSelectionTool* Tool, const FToolBuilderState& SceneState) const;

	/** @return true if this Tool requires an input selection */
	virtual bool RequiresInputSelection() const { return false; }

protected:
	virtual const FToolTargetTypeRequirements& GetTargetRequirements() const override;
};



UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_SingleTargetWithSelectionTool : public USingleSelectionTool
{
	GENERATED_BODY()
public:
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void OnShutdown(EToolShutdownType ShutdownType);

	virtual void OnTick(float DeltaTime) override;

	virtual void SetTargetWorld(UWorld* World);
	virtual UWorld* GetTargetWorld();

protected:
	UPROPERTY()
	TWeakObjectPtr<UWorld> TargetWorld = nullptr;


public:
	virtual void SetGeometrySelection(const UE::Geometry::FGeometrySelection& SelectionIn);
	virtual void SetGeometrySelection(UE::Geometry::FGeometrySelection&& SelectionIn);

	/** @return true if a Selection is available */
	virtual bool HasGeometrySelection() const;

	/** @return the input Selection */
	virtual const UE::Geometry::FGeometrySelection& GetGeometrySelection() const;

protected:
	UE::Geometry::FGeometrySelection GeometrySelection;
	bool bGeometrySelectionInitialized = false;

	UPROPERTY()
	TObjectPtr<UTLLRN_GeometrySelectionVisualizationProperties> GeometrySelectionVizProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PreviewGeometry> GeometrySelectionViz = nullptr;
};