// Copyright Epic Games, Inc. All Rights Reserved.

#include "BaseTools/TLLRN_MultiTargetWithSelectionTool.h"

#include "ModelingToolTargetUtil.h"
#include "Engine/World.h"
#include "UDynamicMesh.h"

#include "Drawing/TLLRN_PreviewGeometryActor.h"
#include "TargetInterfaces/TLLRN_DynamicMeshSource.h"
#include "TargetInterfaces/MaterialProvider.h"
#include "TargetInterfaces/MeshDescriptionCommitter.h"
#include "TargetInterfaces/MeshDescriptionProvider.h"
#include "TargetInterfaces/PrimitiveComponentBackedTarget.h"
#include "ToolTargetManager.h"
#include "Selection/StoredMeshSelectionUtil.h"
#include "Selection/GeometrySelectionVisualization.h"
#include "Selections/GeometrySelection.h"
#include "PropertySets/TLLRN_GeometrySelectionVisualizationProperties.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_MultiTargetWithSelectionTool)


/*
 * ToolBuilder
 */
const FToolTargetTypeRequirements& UTLLRN_MultiTargetWithSelectionToolBuilder::GetTargetRequirements() const
{
	static FToolTargetTypeRequirements TypeRequirements({
		UMaterialProvider::StaticClass(),
		UMeshDescriptionCommitter::StaticClass(),
		UMeshDescriptionProvider::StaticClass(),
		UPrimitiveComponentBackedTarget::StaticClass()
		});
	return TypeRequirements;
}

bool UTLLRN_MultiTargetWithSelectionToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	if (RequiresInputSelection() && UE::Geometry::HaveAvailableGeometrySelection(SceneState) == false )
	{
		return false;
	}

	return SceneState.TargetManager->CountSelectedAndTargetable(SceneState, GetTargetRequirements()) > 0;
}

UInteractiveTool* UTLLRN_MultiTargetWithSelectionToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_MultiTargetWithSelectionTool* NewTool = CreateNewTool(SceneState);
	InitializeNewTool(NewTool, SceneState);
	return NewTool;
}

void UTLLRN_MultiTargetWithSelectionToolBuilder::InitializeNewTool(UTLLRN_MultiTargetWithSelectionTool* NewTool, const FToolBuilderState& SceneState) const
{

	const TArray<TObjectPtr<UToolTarget>> Targets = SceneState.TargetManager->BuildAllSelectedTargetable(SceneState, GetTargetRequirements());
	const int NumTargets = Targets.Num();
	
	NewTool->InitializeGeometrySelectionArrays(NumTargets);
	NewTool->SetTargets(Targets);
	NewTool->SetTargetWorld(SceneState.World);

	TArray<UE::Geometry::FGeometrySelection> Selections;
	Selections.SetNum(NumTargets);
	for (int TargetIndex = 0; TargetIndex < NumTargets; TargetIndex++)
	{
		bool bHaveSelection = UE::Geometry::GetCurrentGeometrySelectionForTarget(SceneState, Targets[TargetIndex], Selections[TargetIndex]);

		if (bHaveSelection)
		{
			NewTool->SetGeometrySelection(MoveTemp(Selections[TargetIndex]), TargetIndex);
		}
	}

}

void UTLLRN_MultiTargetWithSelectionTool::OnTick(float DeltaTime)
{
	Super::OnTick(DeltaTime);

	if (GeometrySelectionViz)
	{
		UE::Geometry::UpdateGeometrySelectionVisualization(GeometrySelectionViz, GeometrySelectionVizProperties);
	}
}


void UTLLRN_MultiTargetWithSelectionTool::Shutdown(EToolShutdownType ShutdownType)
{
	OnShutdown(ShutdownType);
	TargetWorld = nullptr;

	Super::Shutdown(ShutdownType);
}

void UTLLRN_MultiTargetWithSelectionTool::OnShutdown(EToolShutdownType ShutdownType)
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

void UTLLRN_MultiTargetWithSelectionTool::SetTargetWorld(UWorld* World)
{
	TargetWorld = World;
}

UWorld* UTLLRN_MultiTargetWithSelectionTool::GetTargetWorld()
{
	return TargetWorld.Get();
}

void UTLLRN_MultiTargetWithSelectionTool::InitializeGeometrySelectionArrays(const int NumTargets)
{
	GeometrySelectionArray.SetNum(NumTargets);
	GeometrySelectionBoolArray.SetNum(NumTargets);
	for (int BoolArrIndex = 0; BoolArrIndex < NumTargets; BoolArrIndex++)
	{
		GeometrySelectionBoolArray[BoolArrIndex] = false;
	}
}



void UTLLRN_MultiTargetWithSelectionTool::SetGeometrySelection(const UE::Geometry::FGeometrySelection& SelectionIn, const int TargetIndex)
{
	GeometrySelectionArray[TargetIndex] = SelectionIn;
	GeometrySelectionBoolArray[TargetIndex] = true;
}

void UTLLRN_MultiTargetWithSelectionTool::SetGeometrySelection(UE::Geometry::FGeometrySelection&& SelectionIn, const int TargetIndex)
{
	GeometrySelectionArray[TargetIndex] = MoveTemp(SelectionIn);
	GeometrySelectionBoolArray[TargetIndex] = true;
}

bool UTLLRN_MultiTargetWithSelectionTool::HasGeometrySelection(const int TargetIndex) const
{
	return GeometrySelectionBoolArray[TargetIndex];
}

const UE::Geometry::FGeometrySelection& UTLLRN_MultiTargetWithSelectionTool::GetGeometrySelection(const int TargetIndex) const
{
	return GeometrySelectionArray[TargetIndex];
}

bool UTLLRN_MultiTargetWithSelectionTool::HasAnyGeometrySelection() const
{
	bool bHasGeometrySelectedAcrossAllTargets = false;
	for (const bool bGeoSelectedPerTarget : GeometrySelectionBoolArray)
	{
		bHasGeometrySelectedAcrossAllTargets = bHasGeometrySelectedAcrossAllTargets || bGeoSelectedPerTarget;
	}
	return bHasGeometrySelectedAcrossAllTargets;
}
