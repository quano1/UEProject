// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_RecomputeUVsTool.h"
#include "InteractiveToolManager.h"
#include "ToolBuilderUtil.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Polygroups/PolygroupUtil.h"
#include "ToolSetupUtil.h"
#include "ModelingToolTargetUtil.h"
#include "ParameterizationOps/TLLRN_RecomputeUVsOp.h"
#include "ToolTargetManager.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RecomputeUVsTool)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UTLLRN_RecomputeUVsTool"


/*
 * ToolBuilder
 */

UTLLRN_SingleSelectionMeshEditingTool* UTLLRN_RecomputeUVsToolBuilder::CreateNewTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_RecomputeUVsTool* NewTool = NewObject<UTLLRN_RecomputeUVsTool>(SceneState.ToolManager);
	return NewTool;
}

bool UTLLRN_RecomputeUVsToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return UTLLRN_SingleSelectionMeshEditingToolBuilder::CanBuildTool(SceneState) && 
		SceneState.TargetManager->CountSelectedAndTargetableWithPredicate(SceneState, GetTargetRequirements(),
			[](UActorComponent& Component) { return ToolBuilderUtil::ComponentTypeCouldHaveUVs(Component); }) > 0;
}

/*
 * Tool
 */


void UTLLRN_RecomputeUVsTool::Setup()
{
	UInteractiveTool::Setup();

	InputMesh = MakeShared<FDynamicMesh3, ESPMode::ThreadSafe>();
	*InputMesh = UE::ToolTarget::GetDynamicMeshCopy(Target);

	FComponentMaterialSet MaterialSet = UE::ToolTarget::GetMaterialSet(Target);
	FTransform TargetTransform = (FTransform)UE::ToolTarget::GetLocalToWorldTransform(Target);

	// initialize our properties

	Settings = NewObject<UTLLRN_RecomputeUVsToolProperties>(this);
	Settings->RestoreProperties(this);
	Settings->bUDIMCVAREnabled = false;
	Settings->bEnableUDIMLayout = false;
	AddToolPropertySource(Settings);

	UVChannelProperties = NewObject<UTLLRN_MeshUVChannelProperties>(this);
	UVChannelProperties->RestoreProperties(this);
	UVChannelProperties->Initialize(InputMesh.Get(), false);
	UVChannelProperties->ValidateSelection(true);
	UVChannelProperties->WatchProperty(UVChannelProperties->UVChannel, [this](const FString& NewValue)
		{
			MaterialSettings->UpdateUVChannels(UVChannelProperties->UVChannelNamesList.IndexOfByKey(UVChannelProperties->UVChannel),
			                                   UVChannelProperties->UVChannelNamesList);
		});
	AddToolPropertySource(UVChannelProperties);

	PolygroupLayerProperties = NewObject<UTLLRN_PolygroupLayersProperties>(this);
	PolygroupLayerProperties->RestoreProperties(this, TEXT("TLLRN_RecomputeUVsTool"));
	PolygroupLayerProperties->InitializeGroupLayers(InputMesh.Get());
	PolygroupLayerProperties->WatchProperty(PolygroupLayerProperties->ActiveGroupLayer, [&](FName) { OnSelectedGroupLayerChanged(); });
	AddToolPropertySource(PolygroupLayerProperties);
	UpdateActiveGroupLayer();

	MaterialSettings = NewObject<UTLLRN_ExistingTLLRN_MeshMaterialProperties>(this);
	MaterialSettings->MaterialMode = ETLLRN_SetMeshMaterialMode::Checkerboard;
	MaterialSettings->RestoreProperties(this, TEXT("ModelingUVTools"));
	AddToolPropertySource(MaterialSettings);

	UE::ToolTarget::HideSourceObject(Target);

	// force update
	MaterialSettings->UpdateMaterials();

	TLLRN_RecomputeUVsOpFactory = NewObject<UTLLRN_RecomputeUVsOpFactory>();
	TLLRN_RecomputeUVsOpFactory->OriginalMesh = InputMesh;
	TLLRN_RecomputeUVsOpFactory->InputGroups = ActiveGroupSet;
	TLLRN_RecomputeUVsOpFactory->Settings = Settings;
	TLLRN_RecomputeUVsOpFactory->TargetTransform = UE::ToolTarget::GetLocalToWorldTransform(Target);
	TLLRN_RecomputeUVsOpFactory->GetSelectedUVChannel = [this]() { return GetSelectedUVChannel(); };

	Preview = NewObject<UTLLRN_MeshOpPreviewWithBackgroundCompute>(this);
	Preview->Setup(GetTargetWorld(), TLLRN_RecomputeUVsOpFactory);
	ToolSetupUtil::ApplyRenderingConfigurationToPreview(Preview->TLLRN_PreviewMesh, Target);
	Preview->TLLRN_PreviewMesh->SetTangentsMode(EDynamicMeshComponentTangentsMode::AutoCalculated);
	Preview->TLLRN_PreviewMesh->ReplaceMesh(*InputMesh);
	Preview->ConfigureMaterials(MaterialSet.Materials, ToolSetupUtil::GetDefaultWorkingMaterial(GetToolManager()));
	Preview->TLLRN_PreviewMesh->SetTransform(TargetTransform);

	Preview->OnMeshUpdated.AddLambda([this](UTLLRN_MeshOpPreviewWithBackgroundCompute* Op)
		{
			OnTLLRN_PreviewMeshUpdated();
		});

	Preview->OverrideMaterial = MaterialSettings->GetActiveOverrideMaterial();

	if (bCreateUVLayoutViewOnSetup)
	{
		UVLayoutView = NewObject<UTLLRN_UVLayoutPreview>(this);
		UVLayoutView->CreateInWorld(GetTargetWorld());
		UVLayoutView->SetSourceMaterials(MaterialSet);
		UVLayoutView->SetSourceWorldPosition(TargetTransform, UE::ToolTarget::GetTargetActor(Target)->GetComponentsBoundingBox());
		UVLayoutView->Settings->bEnabled = false;
		UVLayoutView->Settings->bShowWireframe = false;
		UVLayoutView->Settings->RestoreProperties(this, TEXT("TLLRN_RecomputeUVsTool"));
		AddToolPropertySource(UVLayoutView->Settings);
	}

	Preview->InvalidateResult();    // start compute

	SetToolDisplayName(LOCTEXT("ToolNameLocal", "UV Unwrap"));
	GetToolManager()->DisplayMessage(
		LOCTEXT("OnStartTool_Regions", "Generate UVs for PolyGroups or existing UV islands of the mesh using various strategies."),
		EToolMessageLevel::UserNotification);
}


void UTLLRN_RecomputeUVsTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	bool bForceMaterialUpdate = false;
	if (PropertySet == Settings || PropertySet == UVChannelProperties)
	{
		// One of the UV generation properties must have changed.  Dirty the result to force a recompute
		Preview->InvalidateResult();
		bForceMaterialUpdate = true;
	}

	if (PropertySet == MaterialSettings || bForceMaterialUpdate)
	{
		MaterialSettings->UpdateMaterials();
		Preview->OverrideMaterial = MaterialSettings->GetActiveOverrideMaterial();
	}
}


void UTLLRN_RecomputeUVsTool::OnShutdown(EToolShutdownType ShutdownType)
{
	if (UVLayoutView)
	{
		UVLayoutView->Settings->SaveProperties(this, TEXT("TLLRN_RecomputeUVsTool"));
		UVLayoutView->Disconnect();
	}

	UVChannelProperties->SaveProperties(this);
	Settings->SaveProperties(this);
	PolygroupLayerProperties->RestoreProperties(this, TEXT("TLLRN_RecomputeUVsTool"));
	MaterialSettings->SaveProperties(this, TEXT("ModelingUVTools"));

	UE::ToolTarget::ShowSourceObject(Target);

	FDynamicMeshOpResult Result = Preview->Shutdown();
	if (ShutdownType == EToolShutdownType::Accept)
	{
		GetToolManager()->BeginUndoTransaction(LOCTEXT("RecomputeUVs", "Recompute UVs"));
		FDynamicMesh3* NewDynamicMesh = Result.Mesh.Get();
		if (ensure(NewDynamicMesh))
		{
			UE::ToolTarget::CommitDynamicMeshUVUpdate(Target, NewDynamicMesh);
		}
		GetToolManager()->EndUndoTransaction();
	}

	if (TLLRN_RecomputeUVsOpFactory)
	{
		TLLRN_RecomputeUVsOpFactory = nullptr;
	}

}

void UTLLRN_RecomputeUVsTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (UVLayoutView)
	{
		UVLayoutView->Render(RenderAPI);
	}
}


void UTLLRN_RecomputeUVsTool::OnTick(float DeltaTime)
{
	Preview->Tick(DeltaTime);

	if (UVLayoutView)
	{
		UVLayoutView->OnTick(DeltaTime);
	}
}

bool UTLLRN_RecomputeUVsTool::CanAccept() const
{
	return Super::CanAccept() && Preview->HaveValidResult();
}


int32 UTLLRN_RecomputeUVsTool::GetSelectedUVChannel() const
{
	return UVChannelProperties ? UVChannelProperties->GetSelectedChannelIndex(true) : 0;
}

void UTLLRN_RecomputeUVsTool::OnSelectedGroupLayerChanged()
{
	UpdateActiveGroupLayer();
	Preview->InvalidateResult();
}


void UTLLRN_RecomputeUVsTool::UpdateActiveGroupLayer()
{
	if (PolygroupLayerProperties->HasSelectedPolygroup() == false)
	{
		ActiveGroupSet = MakeShared<UE::Geometry::FPolygroupSet, ESPMode::ThreadSafe>(InputMesh.Get());
	}
	else
	{
		FName SelectedName = PolygroupLayerProperties->ActiveGroupLayer;
		FDynamicMeshPolygroupAttribute* FoundAttrib = UE::Geometry::FindPolygroupLayerByName(*InputMesh, SelectedName);
		ensureMsgf(FoundAttrib, TEXT("Selected Attribute Not Found! Falling back to Default group layer."));
		ActiveGroupSet = MakeShared<UE::Geometry::FPolygroupSet, ESPMode::ThreadSafe>(InputMesh.Get(), FoundAttrib);
	}
	if (TLLRN_RecomputeUVsOpFactory)
	{
		TLLRN_RecomputeUVsOpFactory->InputGroups = ActiveGroupSet;
	}
}


void UTLLRN_RecomputeUVsTool::OnTLLRN_PreviewMeshUpdated()
{
	if (UVLayoutView)
	{
		int32 UVChannel = UVChannelProperties ? UVChannelProperties->GetSelectedChannelIndex(true) : 0;
		Preview->TLLRN_PreviewMesh->ProcessMesh([&](const FDynamicMesh3& NewMesh)
		{
			UVLayoutView->UpdateUVMesh(&NewMesh, UVChannel);
		});
	}

	if (MaterialSettings)
	{
		MaterialSettings->UpdateMaterials();
	}

}



#undef LOCTEXT_NAMESPACE

