// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ParameterizeMeshTool.h"
#include "InteractiveToolManager.h"
#include "ToolBuilderUtil.h"
#include "ToolSetupUtil.h"
#include "ModelingToolTargetUtil.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "ParameterizationOps/TLLRN_ParameterizeMeshOp.h"
#include "Properties/TLLRN_ParameterizeMeshProperties.h"
#include "Polygroups/PolygroupUtil.h"
#include "Polygroups/PolygroupSet.h"
#include "ToolTargetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ParameterizeMeshTool)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UTLLRN_ParameterizeMeshTool"

/*
 * ToolBuilder
 */

UTLLRN_SingleSelectionMeshEditingTool* UTLLRN_ParameterizeMeshToolBuilder::CreateNewTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_ParameterizeMeshTool* NewTool = NewObject<UTLLRN_ParameterizeMeshTool>(SceneState.ToolManager);
	return NewTool;
}

bool UTLLRN_ParameterizeMeshToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return UTLLRN_SingleSelectionMeshEditingToolBuilder::CanBuildTool(SceneState) &&
		SceneState.TargetManager->CountSelectedAndTargetableWithPredicate(SceneState, GetTargetRequirements(),
			[](UActorComponent& Component) { return ToolBuilderUtil::ComponentTypeCouldHaveUVs(Component); }) == 1;
}

/*
 * Tool
 */


void UTLLRN_ParameterizeMeshTool::Setup()
{
	UInteractiveTool::Setup();

	InputMesh = MakeShared<FDynamicMesh3, ESPMode::ThreadSafe>();
	*InputMesh = UE::ToolTarget::GetDynamicMeshCopy(Target);
	FTransform InputTransform = (FTransform)UE::ToolTarget::GetLocalToWorldTransform(Target);
	FComponentMaterialSet MaterialSet = UE::ToolTarget::GetMaterialSet(Target);

	UE::ToolTarget::HideSourceObject(Target);

	// initialize our properties

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

	Settings = NewObject<UTLLRN_ParameterizeMeshToolProperties>(this);
	Settings->RestoreProperties(this);
	AddToolPropertySource(Settings);
	Settings->WatchProperty(Settings->Method, [&](ETLLRN_ParameterizeMeshUVMethod) { OnMethodTypeChanged(); });


	UVAtlasProperties = NewObject<UTLLRN_ParameterizeMeshToolUVAtlasProperties>(this);
	UVAtlasProperties->RestoreProperties(this);
	AddToolPropertySource(UVAtlasProperties);
	SetToolPropertySourceEnabled(UVAtlasProperties, false);

	XAtlasProperties = NewObject<UTLLRN_ParameterizeMeshToolXAtlasProperties>(this);
	XAtlasProperties->RestoreProperties(this);
	AddToolPropertySource(XAtlasProperties);
	SetToolPropertySourceEnabled(XAtlasProperties, false);

	PatchBuilderProperties = NewObject<UTLLRN_ParameterizeMeshToolPatchBuilderProperties>(this);
	PatchBuilderProperties->RestoreProperties(this);
	AddToolPropertySource(PatchBuilderProperties);
	SetToolPropertySourceEnabled(PatchBuilderProperties, false);

	PolygroupLayerProperties = NewObject<UTLLRN_PolygroupLayersProperties>(this);
	PolygroupLayerProperties->RestoreProperties(this, TEXT("ModelingUVTools"));
	PolygroupLayerProperties->InitializeGroupLayers(InputMesh.Get());
	AddToolPropertySource(PolygroupLayerProperties);
	PatchBuilderProperties->bPolygroupsEnabled = true;
	UVAtlasProperties->bPolygroupsEnabled = true;
	SetToolPropertySourceEnabled(PolygroupLayerProperties, (Settings->Method == ETLLRN_ParameterizeMeshUVMethod::PatchBuilder && 
		                                                    PatchBuilderProperties->bUsePolygroups)
														   || (Settings->Method == ETLLRN_ParameterizeMeshUVMethod::UVAtlas &&
															   UVAtlasProperties->bUsePolygroups));

	MaterialSettings = NewObject<UTLLRN_ExistingTLLRN_MeshMaterialProperties>(this);
	MaterialSettings->MaterialMode = ETLLRN_SetMeshMaterialMode::Checkerboard;
	MaterialSettings->RestoreProperties(this, TEXT("ModelingUVTools"));
	AddToolPropertySource(MaterialSettings);

	if (bCreateUVLayoutViewOnSetup)
	{
		UVLayoutView = NewObject<UTLLRN_UVLayoutPreview>(this);
		UVLayoutView->CreateInWorld(GetTargetWorld());
		UVLayoutView->SetSourceMaterials(MaterialSet);
		FBox Bounds = UE::ToolTarget::GetTargetActor(Target)->GetComponentsBoundingBox(true /*bNonColliding*/);
		if (!Bounds.IsValid) // If component did not have valid bounds ...
		{
			// Try getting bounds from mesh
			Bounds = (FBox)InputMesh->GetBounds();
			Bounds = Bounds.TransformBy(InputTransform);
			// If mesh is also empty, just create some small valid Bounds
			if (!Bounds.IsValid)
			{
				UE_LOG(LogGeometry, Warning, TEXT("Auto UV Tool started on mesh with empty bounding box"));
				constexpr double SmallSize = FMathd::ZeroTolerance;
				Bounds = FBox(FVector(-1, -1, -1) * SmallSize, FVector(1, 1, 1) * SmallSize);
			}
		}
		UVLayoutView->SetSourceWorldPosition(InputTransform, Bounds);
		UVLayoutView->Settings->bEnabled = false;
		UVLayoutView->Settings->bShowWireframe = false;
		UVLayoutView->Settings->RestoreProperties(this, TEXT("TLLRN_ParameterizeMeshTool"));
		AddToolPropertySource(UVLayoutView->Settings);
	}

	Factory = NewObject<UTLLRN_ParameterizeMeshOperatorFactory>();
	Factory->TargetTransform = InputTransform;
	Factory->Settings = Settings;
	Factory->GetPolygroups = [this]() {
		if (PolygroupLayerProperties->HasSelectedPolygroup() == false)
		{
			return MakeShared<UE::Geometry::FPolygroupSet, ESPMode::ThreadSafe>(InputMesh.Get());
		}
		else
		{
			FName SelectedName = PolygroupLayerProperties->ActiveGroupLayer;
			FDynamicMeshPolygroupAttribute* FoundAttrib = UE::Geometry::FindPolygroupLayerByName(*InputMesh, SelectedName);
			ensureMsgf(FoundAttrib, TEXT("Selected attribute not found! Falling back to Default group layer."));
			return MakeShared<UE::Geometry::FPolygroupSet, ESPMode::ThreadSafe>(InputMesh.Get(), FoundAttrib);
		}
	};
	Factory->UVAtlasProperties = UVAtlasProperties;
	Factory->XAtlasProperties = XAtlasProperties;
	Factory->PatchBuilderProperties = PatchBuilderProperties;
	Factory->OriginalMesh = InputMesh;
	Factory->GetSelectedUVChannel = [this]() { return UVChannelProperties->UVChannelNamesList.IndexOfByKey(UVChannelProperties->UVChannel); };

	Preview = NewObject<UTLLRN_MeshOpPreviewWithBackgroundCompute>(this);
	Preview->Setup(GetTargetWorld(), Factory);
	ToolSetupUtil::ApplyRenderingConfigurationToPreview(Preview->TLLRN_PreviewMesh, nullptr);
	Preview->TLLRN_PreviewMesh->SetTangentsMode(EDynamicMeshComponentTangentsMode::AutoCalculated);
	Preview->TLLRN_PreviewMesh->ReplaceMesh(*InputMesh);
	Preview->ConfigureMaterials(MaterialSet.Materials, ToolSetupUtil::GetDefaultWorkingMaterial(GetToolManager()));
	Preview->TLLRN_PreviewMesh->SetTransform(InputTransform);

	Preview->OnMeshUpdated.AddLambda([this](UTLLRN_MeshOpPreviewWithBackgroundCompute* Op)
		{
			OnTLLRN_PreviewMeshUpdated();
		});
	// force update
	MaterialSettings->UpdateMaterials();
	Preview->OverrideMaterial = MaterialSettings->GetActiveOverrideMaterial();
	Preview->InvalidateResult();    // start compute

	SetToolDisplayName(LOCTEXT("ToolNameGlobal", "AutoUV"));
	GetToolManager()->DisplayMessage(
		LOCTEXT("OnStartTool_Global", "Automatically partition the selected Mesh into UV islands, flatten, and pack into a single UV chart"),
		EToolMessageLevel::UserNotification);
}

void UTLLRN_ParameterizeMeshTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	SetToolPropertySourceEnabled(PolygroupLayerProperties, (Settings->Method == ETLLRN_ParameterizeMeshUVMethod::PatchBuilder && 
		                                                    PatchBuilderProperties->bUsePolygroups)
														   || (Settings->Method == ETLLRN_ParameterizeMeshUVMethod::UVAtlas &&
															   UVAtlasProperties->bUsePolygroups));

	if (PropertySet == UVChannelProperties 
		|| PropertySet == Settings 
		|| PropertySet == UVAtlasProperties  
		|| PropertySet == XAtlasProperties  
		|| PropertySet == PatchBuilderProperties
		|| PropertySet == PolygroupLayerProperties)
	{
		Preview->InvalidateResult();
	}

	MaterialSettings->UpdateMaterials();
	Preview->OverrideMaterial = MaterialSettings->GetActiveOverrideMaterial();
}


void UTLLRN_ParameterizeMeshTool::OnMethodTypeChanged()
{
	SetToolPropertySourceEnabled(UVAtlasProperties, Settings->Method == ETLLRN_ParameterizeMeshUVMethod::UVAtlas);
	SetToolPropertySourceEnabled(XAtlasProperties, Settings->Method == ETLLRN_ParameterizeMeshUVMethod::XAtlas);
	SetToolPropertySourceEnabled(PatchBuilderProperties, Settings->Method == ETLLRN_ParameterizeMeshUVMethod::PatchBuilder);

	Preview->InvalidateResult();
}


void UTLLRN_ParameterizeMeshTool::OnShutdown(EToolShutdownType ShutdownType)
{
	if (UVLayoutView)
	{
		UVLayoutView->Settings->SaveProperties(this, TEXT("TLLRN_ParameterizeMeshTool"));
		UVLayoutView->Disconnect();
	}

	UVChannelProperties->SaveProperties(this);
	Settings->SaveProperties(this);
	MaterialSettings->SaveProperties(this, TEXT("ModelingUVTools"));
	UVAtlasProperties->SaveProperties(this);
	XAtlasProperties->SaveProperties(this);
	PatchBuilderProperties->SaveProperties(this);
	PolygroupLayerProperties->SaveProperties(this, TEXT("ModelingUVTools"));

	FDynamicMeshOpResult Result = Preview->Shutdown();
	
	// Restore (unhide) the source meshes
	UE::ToolTarget::ShowSourceObject(Target);

	if (ShutdownType == EToolShutdownType::Accept)
	{
		GetToolManager()->BeginUndoTransaction(LOCTEXT("ParameterizeMesh", "Auto UVs"));
		FDynamicMesh3* NewDynamicMesh = Result.Mesh.Get();
		if (ensure(NewDynamicMesh))
		{
			UE::ToolTarget::CommitDynamicMeshUVUpdate(Target, NewDynamicMesh);
		}
		GetToolManager()->EndUndoTransaction();
	}

	Factory = nullptr;
}

void UTLLRN_ParameterizeMeshTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (UVLayoutView)
	{
		UVLayoutView->Render(RenderAPI);
	}
}

void UTLLRN_ParameterizeMeshTool::OnTick(float DeltaTime)
{
	Preview->Tick(DeltaTime);

	if (UVLayoutView)
	{
		UVLayoutView->OnTick(DeltaTime);
	}
}

bool UTLLRN_ParameterizeMeshTool::CanAccept() const
{
	return Super::CanAccept() && Preview->HaveValidResult();
}

void UTLLRN_ParameterizeMeshTool::OnTLLRN_PreviewMeshUpdated()
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

