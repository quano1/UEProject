// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "InteractiveToolBuilder.h"
#include "TLLRN_MultiSelectionMeshEditingTool.h"
#include "Selections/GeometrySelection.h"
#include "TLLRN_MultiTargetWithSelectionTool.generated.h"

class UTLLRN_MultiTargetWithSelectionTool;
class UTLLRN_PreviewGeometry;
class UTLLRN_GeometrySelectionVisualizationProperties;

PREDECLARE_GEOMETRY(class FDynamicMesh3)
PREDECLARE_GEOMETRY(class FGroupTopology)


/**
 * UTLLRN_MultiTargetWithSelectionToolBuilder is a base tool builder for multi
 * selection tools with selections.
 * Currently, geometry selection across multiple meshes is not supported, restricting the effectiveness
 * of this class. If that support is built in the future, this will become more useful, and likely need to be expanded
 */
UCLASS(Transient, Abstract)
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_MultiTargetWithSelectionToolBuilder : public UInteractiveToolWithToolTargetsBuilder
{
	GENERATED_BODY()
public:
	/** @return true if mesh sources can be found in the active selection */
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;

	/** @return new Tool instance initialized with selected mesh source(s) */
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	/** @return new Tool instance. Override this in subclasses to build a different Tool class type */
	virtual UTLLRN_MultiTargetWithSelectionTool* CreateNewTool(const FToolBuilderState& SceneState) const PURE_VIRTUAL(UTLLRN_MultiTargetWithSelectionToolBuilder::CreateNewTool, return nullptr; );

	/** Called by BuildTool to configure the Tool with the input mesh source(s) based on the SceneState */
	virtual void InitializeNewTool(UTLLRN_MultiTargetWithSelectionTool* NewTool, const FToolBuilderState& SceneState) const;

	/** @return true if this Tool requires an input selection */
	virtual bool RequiresInputSelection() const { return false; }

protected:
	virtual const FToolTargetTypeRequirements& GetTargetRequirements() const override;
};

/**
 * Multi Target with Selection tool base class.
 */

UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_MultiTargetWithSelectionTool : public UMultiSelectionTool
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
	virtual void SetGeometrySelection(const UE::Geometry::FGeometrySelection& SelectionIn, const int TargetIndex);
	virtual void SetGeometrySelection(UE::Geometry::FGeometrySelection&& SelectionIn, const int TargetIndex);

	/** @return true if a Selection is available for the Target at the given index*/
	virtual bool HasGeometrySelection(const int TargetIndex) const;

	/** @return the input Selection for the Target at the given index*/
	virtual const UE::Geometry::FGeometrySelection& GetGeometrySelection(const int TargetIndex) const;

	/** @return if a Selection is available for ANY of the Targets */
	virtual bool HasAnyGeometrySelection() const;

	/** initialize the Geometry Selection array and the boolean arrays according to the number of targets */
	virtual void InitializeGeometrySelectionArrays(const int NumTargets);

protected:
	TArray<UE::Geometry::FGeometrySelection> GeometrySelectionArray;
	TArray<bool> GeometrySelectionBoolArray;

	UPROPERTY()
	TObjectPtr<UTLLRN_GeometrySelectionVisualizationProperties> GeometrySelectionVizProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UTLLRN_PreviewGeometry> GeometrySelectionViz = nullptr;
};