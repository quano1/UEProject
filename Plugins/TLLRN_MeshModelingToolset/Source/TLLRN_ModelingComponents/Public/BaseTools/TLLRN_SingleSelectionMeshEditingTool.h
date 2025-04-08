// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SingleSelectionTool.h"
#include "InteractiveToolBuilder.h"
#include "TLLRN_SingleSelectionMeshEditingTool.generated.h"

class UTLLRN_SingleSelectionMeshEditingTool;

/**
 * UTLLRN_SingleSelectionMeshEditingToolBuilder is a base tool builder for single
 * selection tools that define a common set of ToolTarget interfaces required
 * for editing meshes.
 */
UCLASS(Transient, Abstract)
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_SingleSelectionMeshEditingToolBuilder : public UInteractiveToolWithToolTargetsBuilder
{
	GENERATED_BODY()

public:
	/** @return true if a single mesh source can be found in the active selection */
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;

	/** @return new Tool instance initialized with selected mesh source */
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	/** @return new Tool instance. Override this in subclasses to build a different Tool class type */
	virtual UTLLRN_SingleSelectionMeshEditingTool* CreateNewTool(const FToolBuilderState& SceneState) const PURE_VIRTUAL(UTLLRN_SingleSelectionMeshEditingToolBuilder::CreateNewTool, return nullptr; );

	/** Called by BuildTool to configure the Tool with the input MeshSource based on the SceneState */
	virtual void InitializeNewTool(UTLLRN_SingleSelectionMeshEditingTool* Tool, const FToolBuilderState& SceneState) const;

protected:
	virtual const FToolTargetTypeRequirements& GetTargetRequirements() const override;
};


/**
 * Single Selection Mesh Editing tool base class.
 */
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_SingleSelectionMeshEditingTool : public USingleSelectionTool
{
	GENERATED_BODY()
public:
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void OnShutdown(EToolShutdownType ShutdownType);

	virtual void SetWorld(UWorld* World);
	virtual UWorld* GetTargetWorld();

protected:
	UPROPERTY()
	TWeakObjectPtr<UWorld> TargetWorld = nullptr;
};
