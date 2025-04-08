// Copyright Epic Games, Inc. All Rights Reserved.

#include "BaseTools/TLLRN_SingleTargetWithSelectionTool.h"

#include "ModelingToolTargetUtil.h"
#include "Engine/World.h"
#include "UDynamicMesh.h"

#include "Drawing/TLLRN_PreviewGeometryActor.h"
#include "TargetInterfaces/TLLRN_DynamicMeshSource.h"
#include "TargetInterfaces/MaterialProvider.h"
#include "TargetInterfaces/TLLRN_DynamicMeshCommitter.h"
#include "TargetInterfaces/TLLRN_DynamicMeshProvider.h"
#include "TargetInterfaces/PrimitiveComponentBackedTarget.h"
#include "ToolTargetManager.h"
#include "Selection/StoredMeshSelectionUtil.h"
#include "Selection/GeometrySelectionVisualization.h"
#include "Selections/GeometrySelection.h"
#include "PropertySets/TLLRN_GeometrySelectionVisualizationProperties.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_SingleTargetWithSelectionTool)


/*
 * ToolBuilder
 */
const FToolTargetTypeRequirements& UTLLRN_SingleTargetWithSelectionToolBuilder::GetTargetRequirements() const
{
	static FToolTargetTypeRequirements TypeRequirements({
		UMaterialProvider::StaticClass(),
		UTLLRN_DynamicMeshCommitter::StaticClass(),
		UTLLRN_DynamicMeshProvider::StaticClass(),
		UPrimitiveComponentBackedTarget::StaticClass()
		});
	return TypeRequirements;
}

bool UTLLRN_SingleTargetWithSelectionToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	if (RequiresInputSelection() && UE::Geometry::HaveAvailableGeometrySelection(SceneState) == false )
	{
		return false;
	}

	return SceneState.TargetManager->CountSelectedAndTargetable(SceneState, GetTargetRequirements()) == 1;
}

UInteractiveTool* UTLLRN_SingleTargetWithSelectionToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_SingleTargetWithSelectionTool* NewTool = CreateNewTool(SceneState);
	InitializeNewTool(NewTool, SceneState);
	return NewTool;
}

void UTLLRN_SingleTargetWithSelectionToolBuilder::InitializeNewTool(UTLLRN_SingleTargetWithSelectionTool* NewTool, const FToolBuilderState& SceneState) const
{
	UToolTarget* Target = SceneState.TargetManager->BuildFirstSelectedTargetable(SceneState, GetTargetRequirements());
	check(Target);
	NewTool->SetTarget(Target);
	NewTool->SetTargetWorld(SceneState.World);

	UE::Geometry::FGeometrySelection Selection;
	bool bHaveSelection = UE::Geometry::GetCurrentGeometrySelectionForTarget(SceneState, Target, Selection);
	if (bHaveSelection)
	{
		NewTool->SetGeometrySelection(MoveTemp(Selection));
	}

}

void UTLLRN_SingleTargetWithSelectionTool::OnTick(float DeltaTime)
{
	Super::OnTick(DeltaTime);

	if (GeometrySelectionViz)
	{
		UE::Geometry::UpdateGeometrySelectionVisualization(GeometrySelectionViz, GeometrySelectionVizProperties);
	}
}


void UTLLRN_SingleTargetWithSelectionTool::Shutdown(EToolShutdownType ShutdownType)
{
	OnShutdown(ShutdownType);
	TargetWorld = nullptr;

	Super::Shutdown(ShutdownType);
}

void UTLLRN_SingleTargetWithSelectionTool::OnShutdown(EToolShutdownType ShutdownType)
{
	if (GeometrySelectionViz)
	{
		GeometrySelectionViz->Disconnect();
	}

	if (GeometrySelectionVizProperties)
	{
		GeometrySelectionVizProperties->SaveProperties(this);
	}
}

void UTLLRN_SingleTargetWithSelectionTool::SetTargetWorld(UWorld* World)
{
	TargetWorld = World;
}

UWorld* UTLLRN_SingleTargetWithSelectionTool::GetTargetWorld()
{
	return TargetWorld.Get();
}


void UTLLRN_SingleTargetWithSelectionTool::SetGeometrySelection(const UE::Geometry::FGeometrySelection& SelectionIn)
{
	GeometrySelection = SelectionIn;
	bGeometrySelectionInitialized = true;
}

void UTLLRN_SingleTargetWithSelectionTool::SetGeometrySelection(UE::Geometry::FGeometrySelection&& SelectionIn)
{
	GeometrySelection = MoveTemp(SelectionIn);
	bGeometrySelectionInitialized = true;
}

bool UTLLRN_SingleTargetWithSelectionTool::HasGeometrySelection() const
{
	return bGeometrySelectionInitialized;
}

const UE::Geometry::FGeometrySelection& UTLLRN_SingleTargetWithSelectionTool::GetGeometrySelection() const
{
	return GeometrySelection;
}