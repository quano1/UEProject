// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVLayoutTool.h"
#include "InteractiveToolManager.h"
#include "ToolBuilderUtil.h"

#include "ToolSetupUtil.h"
#include "ModelingToolTargetUtil.h"

#include "DynamicMesh/DynamicMesh3.h"

#include "MeshDescriptionToDynamicMesh.h"
#include "DynamicMeshToMeshDescription.h"

#include "ParameterizationOps/TLLRN_UVLayoutOp.h"
#include "Properties/TLLRN_UVLayoutProperties.h"

#include "ModelingToolTargetUtil.h"
#include "ToolTargetManager.h"

#include "TargetInterfaces/MeshDescriptionProvider.h"
#include "TargetInterfaces/MeshDescriptionCommitter.h"
#include "TargetInterfaces/PrimitiveComponentBackedTarget.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVLayoutTool)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UTLLRN_UVLayoutTool"

/*
 * ToolBuilder
 */

UTLLRN_MultiSelectionMeshEditingTool* UTLLRN_UVLayoutToolBuilder::CreateNewTool(const FToolBuilderState& SceneState) const
{
	return NewObject<UTLLRN_UVLayoutTool>(SceneState.ToolManager);
}

bool UTLLRN_UVLayoutToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return UTLLRN_MultiSelectionMeshEditingToolBuilder::CanBuildTool(SceneState) &&
		SceneState.TargetManager->CountSelectedAndTargetableWithPredicate(SceneState, GetTargetRequirements(),
			[](UActorComponent& Component) { return ToolBuilderUtil::ComponentTypeCouldHaveUVs(Component); }) > 0;
}

const FToolTargetTypeRequirements& UTLLRN_UVLayoutToolBuilder::GetTargetRequirements() const
{
	static FToolTargetTypeRequirements TypeRequirements({
		UMaterialProvider::StaticClass(),
		UMeshDescriptionProvider::StaticClass(),
		UMeshDescriptionCommitter::StaticClass(),
		UPrimitiveComponentBackedTarget::StaticClass()
		});
	return TypeRequirements;
}

/*
 * Tool
 */

UTLLRN_UVLayoutTool::UTLLRN_UVLayoutTool()
{
}

void UTLLRN_UVLayoutTool::Setup()
{
	UInteractiveTool::Setup();

	// hide input StaticMeshComponent
	for (int32 ComponentIdx = 0; ComponentIdx < Targets.Num(); ComponentIdx++)
	{
		UE::ToolTarget::HideSourceObject(Targets[ComponentIdx]);
	}

	// if we only have one object, add ability to set UV channel
	if (Targets.Num() == 1)
	{
		UVChannelProperties = NewObject<UTLLRN_MeshUVChannelProperties>(this);
		UVChannelProperties->RestoreProperties(this);
		UVChannelProperties->Initialize(UE::ToolTarget::GetMeshDescription(Targets[0]), false);
		UVChannelProperties->ValidateSelection(true);
		AddToolPropertySource(UVChannelProperties);
		UVChannelProperties->WatchProperty(UVChannelProperties->UVChannel, [this](const FString& NewValue)
		{
			MaterialSettings->UpdateUVChannels(UVChannelProperties->UVChannelNamesList.IndexOfByKey(UVChannelProperties->UVChannel),
			                                   UVChannelProperties->UVChannelNamesList);
			UpdateVisualization();
		});
	}

	BasicProperties = NewObject<UTLLRN_UVLayoutProperties>(this);
	BasicProperties->RestoreProperties(this);
	BasicProperties->bUDIMCVAREnabled = false;
	BasicProperties->bEnableUDIMLayout = false;
	AddToolPropertySource(BasicProperties);

	MaterialSettings = NewObject<UTLLRN_ExistingTLLRN_MeshMaterialProperties>(this);
	MaterialSettings->RestoreProperties(this);
	AddToolPropertySource(MaterialSettings);

	// if we only have one object, add optional UV layout view
	if (Targets.Num() == 1)
	{
		UVLayoutView = NewObject<UTLLRN_UVLayoutPreview>(this);
		UVLayoutView->CreateInWorld(GetTargetWorld());

		const FComponentMaterialSet MaterialSet = UE::ToolTarget::GetMaterialSet(Targets[0]);
		UVLayoutView->SetSourceMaterials(MaterialSet);

		const AActor* Actor = UE::ToolTarget::GetTargetActor(Targets[0]);
		UVLayoutView->SetSourceWorldPosition(
			Actor->GetTransform(),
			Actor->GetComponentsBoundingBox());

		UVLayoutView->Settings->RestoreProperties(this);
		AddToolPropertySource(UVLayoutView->Settings);
	}

	UpdateVisualization();

	SetToolDisplayName(LOCTEXT("ToolName", "UV Layout"));
	GetToolManager()->DisplayMessage(LOCTEXT("OnStartTLLRN_UVLayoutTool", "Transform/Rotate/Scale existing UV Charts using various strategies"),
		EToolMessageLevel::UserNotification);
}


void UTLLRN_UVLayoutTool::UpdateNumPreviews()
{
	const int32 CurrentNumPreview = Previews.Num();
	const int32 TargetNumPreview = Targets.Num();
	if (TargetNumPreview < CurrentNumPreview)
	{
		for (int32 PreviewIdx = CurrentNumPreview - 1; PreviewIdx >= TargetNumPreview; PreviewIdx--)
		{
			Previews[PreviewIdx]->Cancel();
		}
		Previews.SetNum(TargetNumPreview);
		OriginalDynamicMeshes.SetNum(TargetNumPreview);
		Factories.SetNum(TargetNumPreview);
	}
	else
	{
		OriginalDynamicMeshes.SetNum(TargetNumPreview);
		Factories.SetNum(TargetNumPreview);
		for (int32 PreviewIdx = CurrentNumPreview; PreviewIdx < TargetNumPreview; PreviewIdx++)
		{
			OriginalDynamicMeshes[PreviewIdx] = MakeShared<FDynamicMesh3, ESPMode::ThreadSafe>();
			*OriginalDynamicMeshes[PreviewIdx] = UE::ToolTarget::GetDynamicMeshCopy(Targets[PreviewIdx]);

			Factories[PreviewIdx]= NewObject<UTLLRN_UVLayoutOperatorFactory>();
			Factories[PreviewIdx]->OriginalMesh = OriginalDynamicMeshes[PreviewIdx];
			Factories[PreviewIdx]->Settings = BasicProperties;
			Factories[PreviewIdx]->TargetTransform = (FTransform) UE::ToolTarget::GetLocalToWorldTransform(Targets[PreviewIdx]);
			Factories[PreviewIdx]->GetSelectedUVChannel = [this]() { return GetSelectedUVChannel(); };

			UTLLRN_MeshOpPreviewWithBackgroundCompute* Preview = Previews.Add_GetRef(NewObject<UTLLRN_MeshOpPreviewWithBackgroundCompute>(Factories[PreviewIdx], "Preview"));
			Preview->Setup(GetTargetWorld(), Factories[PreviewIdx]);
			ToolSetupUtil::ApplyRenderingConfigurationToPreview(Preview->TLLRN_PreviewMesh, Targets[PreviewIdx]); 

			const FComponentMaterialSet MaterialSet = UE::ToolTarget::GetMaterialSet(Targets[PreviewIdx]);
			Preview->ConfigureMaterials(MaterialSet.Materials,
				ToolSetupUtil::GetDefaultWorkingMaterial(GetToolManager())
			);
			Preview->TLLRN_PreviewMesh->UpdatePreview(OriginalDynamicMeshes[PreviewIdx].Get());
			Preview->TLLRN_PreviewMesh->SetTransform((FTransform) UE::ToolTarget::GetLocalToWorldTransform(Targets[PreviewIdx]));

			Preview->OnMeshUpdated.AddLambda([this](UTLLRN_MeshOpPreviewWithBackgroundCompute* Compute)
			{
				OnTLLRN_PreviewMeshUpdated(Compute);
			});

			Preview->SetVisibility(true);
		}
	}
}


void UTLLRN_UVLayoutTool::OnShutdown(EToolShutdownType ShutdownType)
{
	if (UVLayoutView)
	{
		UVLayoutView->Settings->SaveProperties(this);
		UVLayoutView->Disconnect();
	}

	BasicProperties->SaveProperties(this);
	MaterialSettings->SaveProperties(this);

	// Restore (unhide) the source meshes
	for (int32 ComponentIdx = 0; ComponentIdx < Targets.Num(); ComponentIdx++)
	{
		UE::ToolTarget::ShowSourceObject(Targets[ComponentIdx]);
	}

	TArray<FDynamicMeshOpResult> Results;
	for (UTLLRN_MeshOpPreviewWithBackgroundCompute* Preview : Previews)
	{
		Results.Emplace(Preview->Shutdown());
	}
	if (ShutdownType == EToolShutdownType::Accept)
	{
		GenerateAsset(Results);
	}
	for (int32 TargetIndex = 0; TargetIndex < Targets.Num(); ++TargetIndex)
	{
		Factories[TargetIndex] = nullptr;
	}
}

int32 UTLLRN_UVLayoutTool::GetSelectedUVChannel() const
{
	return UVChannelProperties ? UVChannelProperties->GetSelectedChannelIndex(true) : 0;
}


void UTLLRN_UVLayoutTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	GetToolManager()->GetContextQueriesAPI()->GetCurrentViewState(CameraState);

	if (UVLayoutView)
	{
		UVLayoutView->Render(RenderAPI);
	}

}

void UTLLRN_UVLayoutTool::OnTick(float DeltaTime)
{
	for (UTLLRN_MeshOpPreviewWithBackgroundCompute* Preview : Previews)
	{
		Preview->Tick(DeltaTime);
	}

	if (UVLayoutView)
	{
		UVLayoutView->OnTick(DeltaTime);
	}


}



void UTLLRN_UVLayoutTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	if (PropertySet == BasicProperties || PropertySet == UVChannelProperties)
	{
		UpdateNumPreviews();
		for (UTLLRN_MeshOpPreviewWithBackgroundCompute* Preview : Previews)
		{
			Preview->InvalidateResult();
		}
	}
	else if (PropertySet == MaterialSettings)
	{
		// if we don't know what changed, or we know checker density changed, update checker material
		UpdatePreviewMaterial();
	}
}


void UTLLRN_UVLayoutTool::OnTLLRN_PreviewMeshUpdated(UTLLRN_MeshOpPreviewWithBackgroundCompute* Compute)
{
	if (UVLayoutView)
	{
		FDynamicMesh3 ResultMesh;
		if (Compute->GetCurrentResultCopy(ResultMesh, false) == false)
		{
			return;
		}
		UVLayoutView->UpdateUVMesh(&ResultMesh, GetSelectedUVChannel());
	}

}

void UTLLRN_UVLayoutTool::UpdatePreviewMaterial()
{
	MaterialSettings->UpdateMaterials();
	UpdateNumPreviews();
	for (int PreviewIdx = 0; PreviewIdx < Previews.Num(); PreviewIdx++)
	{
		UTLLRN_MeshOpPreviewWithBackgroundCompute* Preview = Previews[PreviewIdx];
		Preview->OverrideMaterial = MaterialSettings->GetActiveOverrideMaterial();
	}
}

void UTLLRN_UVLayoutTool::UpdateVisualization()
{
	UpdatePreviewMaterial();

	for (UTLLRN_MeshOpPreviewWithBackgroundCompute* Preview : Previews)
	{
		Preview->InvalidateResult();
	}
}

bool UTLLRN_UVLayoutTool::CanAccept() const
{
	for (const UTLLRN_MeshOpPreviewWithBackgroundCompute* Preview : Previews)
	{
		if (!Preview->HaveValidResult())
		{
			return false;
		}
	}
	return Super::CanAccept();
}


void UTLLRN_UVLayoutTool::GenerateAsset(const TArray<FDynamicMeshOpResult>& Results)
{
	GetToolManager()->BeginUndoTransaction(LOCTEXT("TLLRN_UVLayoutToolTransactionName", "UV Layout Tool"));

	check(Results.Num() == Targets.Num());
	
	for (int32 ComponentIdx = 0; ComponentIdx < Targets.Num(); ComponentIdx++)
	{
		const FDynamicMesh3* DynamicMesh = Results[ComponentIdx].Mesh.Get();
		check(DynamicMesh != nullptr);
		UE::ToolTarget::CommitDynamicMeshUVUpdate(Targets[ComponentIdx], DynamicMesh);
	}

	GetToolManager()->EndUndoTransaction();
}




#undef LOCTEXT_NAMESPACE

