// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorMode.h"

#include "Algo/AnyOf.h"
#include "Actions/TLLRN_UnsetUVsAction.h"
#include "Actions/TLLRN_UVMakeIslandAction.h"
#include "Actions/TLLRN_UVSplitAction.h"
#include "Actions/TLLRN_UVSeamSewAction.h"
#include "Actions/TLLRN_TLLRN_UVToolAction.h"
#include "AssetEditorModeManager.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "ContextObjects/TLLRN_UVToolViewportButtonsAPI.h"
#include "ContextObjects/TLLRN_UVToolAssetInputsContext.h"
#include "ContextObjectStore.h"
#include "Selection/TLLRN_UVToolSelectionAPI.h"
#include "Drawing/MeshElementsVisualizer.h"
#include "Editor.h"
#include "EditorModelingObjectsCreationAPI.h"
#include "EditorModes.h"
#include "EditorViewportClient.h"
#include "EdModeInteractiveToolsContext.h" //ToolsContext, EditorInteractiveToolsContext
#include "EngineAnalytics.h"
#include "Framework/Commands/UICommandList.h"
#include "InteractiveTool.h"
#include "MeshOpPreviewHelpers.h" // UMeshOpPreviewWithBackgroundCompute
#include "ModelingToolTargetUtil.h"
#include "PreviewMesh.h"
#include "PreviewScene.h"
#include "TargetInterfaces/AssetBackedTarget.h"
#include "TargetInterfaces/DynamicMeshCommitter.h"
#include "TargetInterfaces/DynamicMeshProvider.h"
#include "TargetInterfaces/MaterialProvider.h"
#include "ToolSetupUtil.h"
#include "ToolTargets/TLLRN_UVEditorToolMeshInput.h"
#include "ToolTargetManager.h"
#include "ToolTargets/TLLRN_UVEditorToolMeshInput.h"
#include "TLLRN_UVEditor3DViewportMode.h"
#include "TLLRN_UVEditorToolBase.h"
#include "TLLRN_UVEditorBrushSelectTool.h"
#include "TLLRN_UVEditorCommands.h"
#include "TLLRN_UVEditorLayoutTool.h"
#include "TLLRN_UVEditorTransformTool.h"
#include "TLLRN_UVEditorParameterizeMeshTool.h"
#include "TLLRN_UVEditorLayerEditTool.h"
#include "TLLRN_UVEditorSeamTool.h"
#include "TLLRN_UVEditorRecomputeUVsTool.h"
#include "TLLRN_UVEditorUVSnapshotTool.h"
#include "TLLRN_UVSelectTool.h"
#include "TLLRN_UVEditorTexelDensityTool.h"
#include "TLLRN_UVEditorInitializationContext.h"
#include "TLLRN_UVEditorModeToolkit.h"
#include "TLLRN_UVEditorSubsystem.h"
#include "ContextObjects/TLLRN_UVToolContextObjects.h"
#include "TLLRN_UVEditorBackgroundPreview.h"
#include "TLLRN_UVEditorDistortionVisualization.h"
#include "TLLRN_UVEditorToolAnalyticsUtils.h"
#include "TLLRN_UVEditorUXSettings.h"
#include "UDIMUtilities.h"
#include "EditorSupportDelegates.h"
#include "Parameterization/MeshUDIMClassifier.h"
#include "TLLRN_UVEditorLogging.h"
#include "UObject/ObjectSaveContext.h"
#include "Materials/Material.h"
#include "TLLRN_UVEditorUXPropertySets.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVEditorMode)

#define LOCTEXT_NAMESPACE "UTLLRN_UVEditorMode"

using namespace UE::Geometry;

const FEditorModeID UTLLRN_UVEditorMode::EM_TLLRN_UVEditorModeId = TEXT("EM_TLLRN_UVEditorMode");

FDateTime UTLLRN_UVEditorMode::AnalyticsLastStartTimestamp;

namespace TLLRN_UVEditorModeLocals
{
	// The layer we open when we first open the UV editor
	const int32 DefaultUVLayerIndex = 0;

	const FText UVLayerChangeTransactionName = LOCTEXT("UVLayerChangeTransactionName", "Change UV Layer");

	void GetCameraState(const FEditorViewportClient& ViewportClientIn, FViewCameraState& CameraStateOut)
	{
		FViewportCameraTransform ViewTransform = ViewportClientIn.GetViewTransform();
		CameraStateOut.bIsOrthographic = false;
		CameraStateOut.bIsVR = false;
		CameraStateOut.Position = ViewTransform.GetLocation();
		CameraStateOut.HorizontalFOVDegrees = ViewportClientIn.ViewFOV;
		CameraStateOut.AspectRatio = ViewportClientIn.AspectRatio;

		// if using Orbit camera, the rotation in the ViewTransform is not the current camera rotation, it
		// is set to a different rotation based on the Orbit. So we have to convert back to camera rotation.
		FRotator ViewRotation = (ViewportClientIn.bUsingOrbitCamera) ?
			ViewTransform.ComputeOrbitMatrix().InverseFast().Rotator() : ViewTransform.GetRotation();
		CameraStateOut.Orientation = ViewRotation.Quaternion();
	}

	/**
	 * Support for undoing a tool start in such a way that we go back to the mode's default
	 * tool on undo.
	 */
	class FTLLRN_UVEditorBeginToolChange : public FToolCommandChange
	{
	public:
		virtual void Apply(UObject* Object) override
		{
			// Do nothing, since we don't allow a re-do back into a tool
		}

		virtual void Revert(UObject* Object) override
		{
			UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(Object);
			// Don't really need the check for default tool since we theoretically shouldn't
			// be issuing this transaction for starting the default tool, but still...
			if (Mode && !Mode->IsDefaultToolActive())
			{
				Mode->GetInteractiveToolsContext()->EndTool(EToolShutdownType::Cancel);
				Mode->ActivateDefaultTool();
			}
		}

		virtual bool HasExpired(UObject* Object) const override
		{
			// To not be expired, we must be active and in some non-default tool.
			UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(Object);
			return !(Mode && Mode->IsActive() && !Mode->IsDefaultToolActive());
		}

		virtual FString ToString() const override
		{
			return TEXT("TLLRN_UVEditorModeLocals::FTLLRN_UVEditorBeginToolChange");
		}
	};

	/** 
	 * Change for undoing/redoing displayed layer changes.
	 */
	class FInputObjectUVLayerChange : public FToolCommandChange
	{
	public:
		FInputObjectUVLayerChange(int32 AssetIDIn, int32 OldUVLayerIndexIn, int32 NewUVLayerIndexIn)
			: AssetID(AssetIDIn)
			, OldUVLayerIndex(OldUVLayerIndexIn)
			, NewUVLayerIndex(NewUVLayerIndexIn)
		{
		}

		virtual void Apply(UObject* Object) override
		{
			UTLLRN_UVEditorMode* TLLRN_UVEditorMode = Cast<UTLLRN_UVEditorMode>(Object);
			TLLRN_UVEditorMode->ChangeInputObjectLayer(AssetID, NewUVLayerIndex);
		}

		virtual void Revert(UObject* Object) override
		{
			UTLLRN_UVEditorMode* TLLRN_UVEditorMode = Cast<UTLLRN_UVEditorMode>(Object);
			TLLRN_UVEditorMode->ChangeInputObjectLayer(AssetID, OldUVLayerIndex);
		}

		virtual bool HasExpired(UObject* Object) const override
		{
			UTLLRN_UVEditorMode* TLLRN_UVEditorMode = Cast<UTLLRN_UVEditorMode>(Object);
			return !(TLLRN_UVEditorMode && TLLRN_UVEditorMode->IsActive());
		}

		virtual FString ToString() const override
		{
			return TEXT("TLLRN_UVEditorModeLocals::FInputObjectUVLayerChange");
		}

	protected:
		int32 AssetID;
		int32 OldUVLayerIndex;
		int32 NewUVLayerIndex;
	};

	void InitializeAssetNames(const TArray<TObjectPtr<UToolTarget>>& ToolTargets, TArray<FString>& AssetNamesOut)
	{
		AssetNamesOut.Reset(ToolTargets.Num());

		for (const TObjectPtr<UToolTarget>& ToolTarget : ToolTargets)
		{
			AssetNamesOut.Add(UE::ToolTarget::GetHumanReadableName(ToolTarget));
		}
	}
}


namespace TLLRN_UVEditorModeChange
{
	/**
	 * Change for undoing/redoing "Apply" of UV editor changes.
	 */
	class FApplyChangesChange : public FToolCommandChange
	{
	public:
		FApplyChangesChange(TSet<int32> OriginalModifiedAssetIDsIn)
			: OriginalModifiedAssetIDs(OriginalModifiedAssetIDsIn)
		{
		}

		virtual void Apply(UObject* Object) override
		{
			UTLLRN_UVEditorMode* TLLRN_UVEditorMode = Cast<UTLLRN_UVEditorMode>(Object);
			TLLRN_UVEditorMode->ModifiedAssetIDs.Reset();
		}

		virtual void Revert(UObject* Object) override
		{
			UTLLRN_UVEditorMode* TLLRN_UVEditorMode = Cast<UTLLRN_UVEditorMode>(Object);
			TLLRN_UVEditorMode->ModifiedAssetIDs = OriginalModifiedAssetIDs;
		}

		virtual bool HasExpired(UObject* Object) const override
		{
			UTLLRN_UVEditorMode* TLLRN_UVEditorMode = Cast<UTLLRN_UVEditorMode>(Object);
			return !(TLLRN_UVEditorMode && TLLRN_UVEditorMode->IsActive());
		}

		virtual FString ToString() const override
		{
			return TEXT("TLLRN_UVEditorModeLocals::FApplyChangesChange");
		}

	protected:
		TSet<int32> OriginalModifiedAssetIDs;
	};
}

const FToolTargetTypeRequirements& UTLLRN_UVEditorMode::GetToolTargetRequirements()
{
	static const FToolTargetTypeRequirements ToolTargetRequirements =
		FToolTargetTypeRequirements({
			UMaterialProvider::StaticClass(),
			UDynamicMeshCommitter::StaticClass(),
			UDynamicMeshProvider::StaticClass()
			});
	return ToolTargetRequirements;
}

void UTLLRN_UVEditorUDIMProperties::PostAction(ETLLRN_UVEditorModeActions Action)
{
	if (Action == ETLLRN_UVEditorModeActions::ConfigureUDIMsFromTexture)
	{
		UpdateActiveUDIMsFromTexture();
	}
	if (Action == ETLLRN_UVEditorModeActions::ConfigureUDIMsFromAsset)
	{
		UpdateActiveUDIMsFromAsset();
	}
}

const TArray<FString>& UTLLRN_UVEditorUDIMProperties::GetAssetNames()
{
	return UVAssetNames;
}

int32 UTLLRN_UVEditorUDIMProperties::AssetByIndex() const
{
	return UVAssetNames.Find(UDIMSourceAsset);
}

void UTLLRN_UVEditorUDIMProperties::InitializeAssets(const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn)
{
	UVAssetNames.Reset(TargetsIn.Num());

	for (int i = 0; i < TargetsIn.Num(); ++i)
	{
		UVAssetNames.Add(UE::ToolTarget::GetHumanReadableName(TargetsIn[i]->SourceTarget));
	}
}

void UTLLRN_UVEditorUDIMProperties::UpdateActiveUDIMsFromTexture()
{
	ActiveUDIMs.Empty();
	if (UDIMSourceTexture && UDIMSourceTexture->IsCurrentlyVirtualTextured() && UDIMSourceTexture->Source.GetNumBlocks() > 1)
	{
		for (int32 Block = 0; Block < UDIMSourceTexture->Source.GetNumBlocks(); ++Block)
		{
			FTextureSourceBlock SourceBlock;
			UDIMSourceTexture->Source.GetBlock(Block, SourceBlock);

			ActiveUDIMs.Add(FTLLRN_UDIMSpecifier({ UE::TextureUtilitiesCommon::GetUDIMIndex(SourceBlock.BlockX, SourceBlock.BlockY),
				                             SourceBlock.BlockX, SourceBlock.BlockY, FMath::Min(SourceBlock.SizeX, SourceBlock.SizeY) }));
		}
	}
	CheckAndUpdateWatched();

	// Make sure we're updating the viewport immediately.
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void UTLLRN_UVEditorUDIMProperties::UpdateActiveUDIMsFromAsset()
{
	ActiveUDIMs.Empty();
	int32 AssetID = AssetByIndex();
	ParentMode->PopulateUDIMsByAsset(AssetID, ActiveUDIMs);
	CheckAndUpdateWatched();

	// Make sure we're updating the viewport immediately.
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

UTLLRN_UVEditorMode::UTLLRN_UVEditorMode()
{
	Info = FEditorModeInfo(
		EM_TLLRN_UVEditorModeId,
		LOCTEXT("TLLRN_UVEditorModeName", "UV"),
		FSlateIcon(),
		false);
}

double UTLLRN_UVEditorMode::GetUVMeshScalingFactor() {
	return FTLLRN_UVEditorUXSettings::UVMeshScalingFactor;
}

void UTLLRN_UVEditorMode::Enter()
{
	Super::Enter();

	bPIEModeActive = false;
	if (GEditor->PlayWorld != NULL || GEditor->bIsSimulatingInEditor)
	{
		bPIEModeActive = true;
		SetSimulationWarning(true);
	}

	BeginPIEDelegateHandle = FEditorDelegates::PreBeginPIE.AddLambda([this](bool bSimulating)
	{
		bPIEModeActive = true;
        SetSimulationWarning(true);
	});

	EndPIEDelegateHandle = FEditorDelegates::EndPIE.AddLambda([this](bool bSimulating)
	{
		bPIEModeActive = false;
		ActivateDefaultTool();
		SetSimulationWarning(false);
	});

	CancelPIEDelegateHandle = FEditorDelegates::CancelPIE.AddLambda([this]()
	{
		bPIEModeActive = false;
		ActivateDefaultTool();
		SetSimulationWarning(false);
	});

	PostSaveWorldDelegateHandle = FEditorDelegates::PostSaveWorldWithContext.AddLambda([this](UWorld*, FObjectPostSaveContext)
	{
		ActivateDefaultTool();
	});

	// Our mode needs to get Render and DrawHUD calls, but doesn't implement the legacy interface that
	// causes those to be called. Instead, we'll hook into the tools context's Render and DrawHUD calls.
	GetInteractiveToolsContext()->OnRender.AddUObject(this, &UTLLRN_UVEditorMode::Render);
	GetInteractiveToolsContext()->OnDrawHUD.AddUObject(this, &UTLLRN_UVEditorMode::DrawHUD);

	// We also want the Render and DrawHUD calls from the 3D viewport
	UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
	UTLLRN_UVEditorInitializationContext* InitContext = ContextStore->FindContext<UTLLRN_UVEditorInitializationContext>();
	UTLLRN_UVToolLivePreviewAPI* LivePreviewAPI = ContextStore->FindContext<UTLLRN_UVToolLivePreviewAPI>();
	if (ensure(InitContext && InitContext->LivePreviewITC.IsValid() && LivePreviewAPI))
	{
		LivePreviewITC = InitContext->LivePreviewITC;
		LivePreviewITC->OnRender.AddWeakLambda(this, [this, LivePreviewAPI](IToolsContextRenderAPI* RenderAPI) 
		{ 
			if (LivePreviewAPI->IsValidLowLevel())
			{
				LivePreviewAPI->OnRender.Broadcast(RenderAPI);
			}

			// This could have been attached to the LivePreviewAPI delegate the way that tools might do it,
			// but might as well do the call ourselves. Same for DrawHUD.
			if (SelectionAPI)
			{
				SelectionAPI->LivePreviewRender(RenderAPI);
			}
		});
		LivePreviewITC->OnDrawHUD.AddWeakLambda(this, [this, LivePreviewAPI](FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI)
		{
			if (LivePreviewAPI->IsValidLowLevel())
			{
				LivePreviewAPI->OnDrawHUD.Broadcast(Canvas, RenderAPI);
			}

			if (SelectionAPI)
			{
				SelectionAPI->LivePreviewDrawHUD(Canvas, RenderAPI);
			}
		});

		LivePreviewToolkitCommands = InitContext->LivePreviewToolkitCommands;
	}

	InitializeModeContexts();
	InitializeTargets();

	// By default combined transform gizmos now hide some of their elements based on what gizmo mode is chosen in
	// the viewport. The UV editor doesn't currently have that selector, so we don't want to try to use it. If we
	// add that selector, we'll need to remove this call.
	GetInteractiveToolsContext()->SetForceCombinedGizmoMode(true);

	BackgroundVisualization = NewObject<UTLLRN_UVEditorBackgroundPreview>(this);
	BackgroundVisualization->CreateInWorld(GetWorld(), FTransform::Identity);

	BackgroundVisualization->Settings->WatchProperty(BackgroundVisualization->Settings->bVisible, 
		[this](bool IsVisible) {
			UpdateTriangleMaterialBasedOnDisplaySettings();			
		});

	BackgroundVisualization->OnBackgroundMaterialChange.AddWeakLambda(this,
		[this](TObjectPtr<UMaterialInstanceDynamic> MaterialInstance) {
			UpdatePreviewMaterialBasedOnBackground();
		});

	PropertyObjectsToTick.Add(BackgroundVisualization->Settings);

	DistortionVisualization = NewObject<UTLLRN_UVEditorDistortionVisualization>(this);
	DistortionVisualization->Targets = ToolInputObjects;
	DistortionVisualization->Initialize();


	DistortionVisualization->Settings->WatchProperty(DistortionVisualization->Settings->bVisible,
		[this](bool IsVisible) {
			UpdateTriangleMaterialBasedOnDisplaySettings();			
		});

	PropertyObjectsToTick.Add(DistortionVisualization->Settings);

	TLLRN_UVEditorGridProperties = NewObject <UTLLRN_UVEditorGridProperties>(this);

	TLLRN_UVEditorGridProperties->WatchProperty(TLLRN_UVEditorGridProperties->bDrawGrid,
		[this](bool bDrawGrid) {
			UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
			UTLLRN_UVTool2DViewportAPI* TLLRN_UVTool2DViewportAPI = ContextStore->FindContext<UTLLRN_UVTool2DViewportAPI>();
			if (TLLRN_UVTool2DViewportAPI)
			{
				TLLRN_UVTool2DViewportAPI->SetDrawGrid(bDrawGrid, true);
			}		
		});

	TLLRN_UVEditorGridProperties->WatchProperty(TLLRN_UVEditorGridProperties->bDrawRulers,
		[this](bool bDrawRulers) {
			UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
			UTLLRN_UVTool2DViewportAPI* TLLRN_UVTool2DViewportAPI = ContextStore->FindContext<UTLLRN_UVTool2DViewportAPI>();
			if (TLLRN_UVTool2DViewportAPI)
			{
				TLLRN_UVTool2DViewportAPI->SetDrawRulers(bDrawRulers, true);
			}
		});

	PropertyObjectsToTick.Add(TLLRN_UVEditorGridProperties);
	
	TLLRN_UVEditorUnwrappedUXProperties = NewObject<UTLLRN_UVEditorUnwrappedUXProperties>(this);
	
	TLLRN_UVEditorUnwrappedUXProperties->BoundaryLineColors.Reserve(ToolInputObjects.Num());
	// initialize display menu properties
	for (int ObjIndex = 0; ObjIndex < ToolInputObjects.Num(); ObjIndex++)
	{
		TLLRN_UVEditorUnwrappedUXProperties->BoundaryLineColors.Add(ToolInputObjects[ObjIndex]->WireframeDisplay->Settings->BoundaryEdgeColor);
	}
	TLLRN_UVEditorUnwrappedUXProperties->BoundaryLineThickness = FTLLRN_UVEditorUXSettings::BoundaryEdgeThickness;
	TLLRN_UVEditorUnwrappedUXProperties->WireframeThickness = FTLLRN_UVEditorUXSettings::WireframeThickness;

	TLLRN_UVEditorUnwrappedUXProperties->WatchProperty(TLLRN_UVEditorUnwrappedUXProperties->BoundaryLineColors,
		[this](const TArray<FColor>& EdgeColors)
		{
			for (int32 AssetID = 0; AssetID < ToolInputObjects.Num(); ++AssetID)
			{
				ToolInputObjects[AssetID]->WireframeDisplay->Settings->BoundaryEdgeColor = EdgeColors[AssetID];
			}
		});
	TLLRN_UVEditorUnwrappedUXProperties->WatchProperty(TLLRN_UVEditorUnwrappedUXProperties->BoundaryLineThickness,
		[this](const float Thickness)
		{
			for (int32 AssetID = 0; AssetID < ToolInputObjects.Num(); ++AssetID)
			{
				ToolInputObjects[AssetID]->WireframeDisplay->WireframeComponent->BoundaryEdgeThickness = Thickness;
				ToolInputObjects[AssetID]->WireframeDisplay->WireframeComponent->UpdateWireframe();
			}
		});
	TLLRN_UVEditorUnwrappedUXProperties->WatchProperty(TLLRN_UVEditorUnwrappedUXProperties->WireframeThickness,
		[this](const float WireframeThickness)
		{
			for (int32 AssetID = 0; AssetID < ToolInputObjects.Num(); ++AssetID)
			{
				ToolInputObjects[AssetID]->WireframeDisplay->WireframeComponent->WireframeThickness = WireframeThickness;
				ToolInputObjects[AssetID]->WireframeDisplay->WireframeComponent->UpdateWireframe();
			}
		});
	PropertyObjectsToTick.Add(TLLRN_UVEditorUnwrappedUXProperties);

	TLLRN_UVEditorLivePreviewUXProperties = NewObject<UTLLRN_UVEditorLivePreviewUXProperties>(this);

	// initialize display menu properties
	TLLRN_UVEditorLivePreviewUXProperties->SelectionColor = FTLLRN_UVEditorUXSettings::SelectionTriangleWireframeColor;
	TLLRN_UVEditorLivePreviewUXProperties->SelectionLineThickness = FTLLRN_UVEditorUXSettings::LivePreviewHighlightThickness;
	TLLRN_UVEditorLivePreviewUXProperties->SelectionPointSize = FTLLRN_UVEditorUXSettings::LivePreviewHighlightPointSize;
	
	TLLRN_UVEditorLivePreviewUXProperties->WatchProperty(TLLRN_UVEditorLivePreviewUXProperties->SelectionColor,
		[this](const FColor InSelectionLineColor)
		{
			UTLLRN_UVToolSelectionAPI::FLivePreviewSelectionUXSettings Settings = UTLLRN_UVToolSelectionAPI::FLivePreviewSelectionUXSettings();
			Settings.SelectionColor = InSelectionLineColor;

			SelectionAPI->SetLivePreviewSelectionUXSettings(Settings);
			SelectionAPI->RebuildAppliedPreviewHighlight();
		});
	TLLRN_UVEditorLivePreviewUXProperties->WatchProperty(TLLRN_UVEditorLivePreviewUXProperties->SelectionLineThickness,
		[this](const float InLineThickness)
		{
			UTLLRN_UVToolSelectionAPI::FLivePreviewSelectionUXSettings Settings = UTLLRN_UVToolSelectionAPI::FLivePreviewSelectionUXSettings();
			Settings.LineThickness = InLineThickness;
			
			SelectionAPI->SetLivePreviewSelectionUXSettings(Settings);
			SelectionAPI->RebuildAppliedPreviewHighlight();
		});
	TLLRN_UVEditorLivePreviewUXProperties->WatchProperty(TLLRN_UVEditorLivePreviewUXProperties->SelectionPointSize,
		[this](const float InPointSize)
		{
			UTLLRN_UVToolSelectionAPI::FLivePreviewSelectionUXSettings Settings = UTLLRN_UVToolSelectionAPI::FLivePreviewSelectionUXSettings();
			Settings.PointSize = InPointSize;
			
			SelectionAPI->SetLivePreviewSelectionUXSettings(Settings);
			SelectionAPI->RebuildAppliedPreviewHighlight();
		});

	PropertyObjectsToTick.Add(TLLRN_UVEditorLivePreviewUXProperties);

	TLLRN_UVEditorUDIMProperties = NewObject< UTLLRN_UVEditorUDIMProperties >(this);
	TLLRN_UVEditorUDIMProperties->Initialize(this);
	TLLRN_UVEditorUDIMProperties->InitializeAssets(ToolInputObjects);
	UDIMsChangedWatcherId = TLLRN_UVEditorUDIMProperties->WatchProperty(TLLRN_UVEditorUDIMProperties->ActiveUDIMs,
		[this](const TArray<FTLLRN_UDIMSpecifier>& ActiveUDIMs) {
			UpdateActiveUDIMs();
		},
		[](const TArray<FTLLRN_UDIMSpecifier>& ActiveUDIMsOld, const TArray<FTLLRN_UDIMSpecifier>& ActiveUDIMsNew) {
			TSet<FTLLRN_UDIMSpecifier> NewCopy(ActiveUDIMsNew);
			TSet<FTLLRN_UDIMSpecifier> OldCopy(ActiveUDIMsOld);

			if (OldCopy.Num() != NewCopy.Num())
			{
				return true;
			}
			TSet<FTLLRN_UDIMSpecifier> Test = OldCopy.Union(NewCopy);
			if (Test.Num() != OldCopy.Num())
			{
				return true;
			}
			return false;
		});
	PropertyObjectsToTick.Add(TLLRN_UVEditorUDIMProperties);

	RegisterTools();
	RegisterActions();
	ActivateDefaultTool();

	// do any toolkit UI initialization that depends on the mode setup above
	if (Toolkit.IsValid())
	{
		FTLLRN_UVEditorModeToolkit* UVModeToolkit = (FTLLRN_UVEditorModeToolkit*)Toolkit.Get();
		UVModeToolkit->InitializeAfterModeSetup();
	}

	if (FEngineAnalytics::IsAvailable())
	{
		AnalyticsLastStartTimestamp = FDateTime::UtcNow();

		TArray<FAnalyticsEventAttribute> Attributes;
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Timestamp"), AnalyticsLastStartTimestamp.ToString()));

		FEngineAnalytics::GetProvider().RecordEvent(TLLRN_UVEditorAnalytics::TLLRN_UVEditorAnalyticsEventName(TEXT("Enter")), Attributes);
	}

	bIsActive = true;
}

void UTLLRN_UVEditorMode::RegisterTools()
{
	const FTLLRN_UVEditorCommands& CommandInfos = FTLLRN_UVEditorCommands::Get();

	// The Select Tool is a bit different because it runs between other tools. It doesn't have a button and is not
	// hotkeyable, hence it doesn't have a command item. Instead we register it directly with the tool manager.
	UTLLRN_UVSelectToolBuilder* TLLRN_UVSelectToolBuilder = NewObject<UTLLRN_UVSelectToolBuilder>();
	TLLRN_UVSelectToolBuilder->Targets = &ToolInputObjects;
	DefaultToolIdentifier = TEXT("BeginSelectTool");
	GetToolManager()->RegisterToolType(DefaultToolIdentifier, TLLRN_UVSelectToolBuilder);
	ToolsThatAllowActions.Add(DefaultToolIdentifier);

	// Note that the identifiers below need to match the command names so that the tool icons can 
	// be easily retrieved from the active tool name in TLLRN_UVEditorModeToolkit::OnToolStarted. Otherwise
	// we would need to keep some other mapping from tool identifier to tool icon.

	UTLLRN_UVEditorLayoutToolBuilder* TLLRN_UVEditorLayoutToolBuilder = NewObject<UTLLRN_UVEditorLayoutToolBuilder>();
	TLLRN_UVEditorLayoutToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginLayoutTool, TEXT("BeginLayoutTool"), TLLRN_UVEditorLayoutToolBuilder);

	UTLLRN_UVEditorTransformToolBuilder* TLLRN_UVEditorTransformToolBuilder = NewObject<UTLLRN_UVEditorTransformToolBuilder>();
	TLLRN_UVEditorTransformToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginTransformTool, TEXT("BeginTransformTool"), TLLRN_UVEditorTransformToolBuilder);

	UTLLRN_UVEditorAlignToolBuilder* TLLRN_UVEditorAlignToolBuilder = NewObject<UTLLRN_UVEditorAlignToolBuilder>();
	TLLRN_UVEditorAlignToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginAlignTool, TEXT("BeginAlignTool"), TLLRN_UVEditorAlignToolBuilder);

	UTLLRN_UVEditorDistributeToolBuilder* TLLRN_UVEditorDistributeToolBuilder = NewObject<UTLLRN_UVEditorDistributeToolBuilder>();
	TLLRN_UVEditorDistributeToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginDistributeTool, TEXT("BeginDistributeTool"), TLLRN_UVEditorDistributeToolBuilder);

	UTLLRN_UVEditorTexelDensityToolBuilder* TLLRN_UVEditorTexelDensityToolBuilder = NewObject<UTLLRN_UVEditorTexelDensityToolBuilder>();
	TLLRN_UVEditorTexelDensityToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginTexelDensityTool, TEXT("BeginTexelDensityTool"), TLLRN_UVEditorTexelDensityToolBuilder);

	UTLLRN_UVEditorParameterizeMeshToolBuilder* TLLRN_UVEditorParameterizeMeshToolBuilder = NewObject<UTLLRN_UVEditorParameterizeMeshToolBuilder>();
	TLLRN_UVEditorParameterizeMeshToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginParameterizeMeshTool, TEXT("BeginParameterizeMeshTool"), TLLRN_UVEditorParameterizeMeshToolBuilder);

	UTLLRN_UVEditorChannelEditToolBuilder* TLLRN_UVEditorChannelEditToolBuilder = NewObject<UTLLRN_UVEditorChannelEditToolBuilder>();
	TLLRN_UVEditorChannelEditToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginChannelEditTool, TEXT("BeginChannelEditTool"), TLLRN_UVEditorChannelEditToolBuilder);

	UTLLRN_UVEditorSeamToolBuilder* TLLRN_UVEditorSeamToolBuilder = NewObject<UTLLRN_UVEditorSeamToolBuilder>();
	TLLRN_UVEditorSeamToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginSeamTool, TEXT("BeginSeamTool"), TLLRN_UVEditorSeamToolBuilder);

	UTLLRN_UVEditorRecomputeUVsToolBuilder* TLLRN_UVEditorRecomputeUVsToolBuilder = NewObject<UTLLRN_UVEditorRecomputeUVsToolBuilder>();
	TLLRN_UVEditorRecomputeUVsToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginRecomputeUVsTool, TEXT("BeginRecomputeUVsTool"), TLLRN_UVEditorRecomputeUVsToolBuilder);

	FString BrushToolIdentifier = TEXT("BeginBrushSelectTool");
	UGenericTLLRN_UVEditorToolBuilder* BrushToolBuilder = NewObject<UGenericTLLRN_UVEditorToolBuilder>();
	BrushToolBuilder->Initialize(ToolInputObjects, UTLLRN_UVEditorBrushSelectTool::StaticClass());
	RegisterTool(CommandInfos.BeginBrushSelectTool, BrushToolIdentifier, BrushToolBuilder);
	ToolsThatAllowActions.Add(BrushToolIdentifier);
	UTLLRN_UVEditorUVSnapshotToolBuilder* TLLRN_UVEditorUVSnapshotToolBuilder = NewObject<UTLLRN_UVEditorUVSnapshotToolBuilder>();
	TLLRN_UVEditorUVSnapshotToolBuilder->Targets = &ToolInputObjects;
	RegisterTool(CommandInfos.BeginUVSnapshotTool, TEXT("BeginUVSnapshotTool"), TLLRN_UVEditorUVSnapshotToolBuilder);
}

void UTLLRN_UVEditorMode::RegisterActions()
{
	using namespace TLLRN_UVEditorModeLocals;

	const FTLLRN_UVEditorCommands& CommandInfos = FTLLRN_UVEditorCommands::Get();
	const TSharedRef<FUICommandList>& CommandList = Toolkit->GetToolkitCommands();

	auto PrepAction = [this, CommandList](TSharedPtr<const FUICommandInfo> CommandInfo, UTLLRN_TLLRN_UVToolAction* Action)
	{
		Action->Setup(GetToolManager());
		CommandList->MapAction(CommandInfo,
			FExecuteAction::CreateWeakLambda(Action, [this, Action]() 
			{
				// If we're activating from some other selection tool, go ahead and switch to the regular one
				ActivateDefaultTool();

				Action->ExecuteAction();
				for (const FUVToolSelection& Selection : SelectionAPI->GetSelections())
				{
					// The actions are expected to have left the selection in a valid state. If this is not
					// the case, we'll clear the selection here, which is still not a proper solution because
					// undoing the transaction will still put things into an invalid state (but our Select Tool
					// should be robust to that in its OnSelectionChanged).
					if (!ensure(Selection.Target.IsValid() 
						&& Selection.AreElementsPresentInMesh(*Selection.Target->UnwrapCanonical)))
					{
						SelectionAPI->ClearSelections(true, true);
						SelectionAPI->ClearHighlight();
						break;
					}
				}
			}),
			FCanExecuteAction::CreateWeakLambda(Action, [Action, this]() 
			{
				// We could make some generic system for determining which tools are safe to call one-off actions from,
				//  but it's unclear that it's worth it while we have so few.
				auto DoesCurrentToolAllowActions = [this]()
				{
					if (UInteractiveToolsContext* ToolsContext = GetInteractiveToolsContext())
					{
						return ToolsThatAllowActions.Contains(ToolsContext->GetActiveToolName(EToolSide::Mouse));
					}
					return false;
				};
				return DoesCurrentToolAllowActions() && Action->CanExecuteAction();
			}));
		RegisteredActions.Add(Action);
	};
	PrepAction(CommandInfos.SewAction, NewObject<UTLLRN_UVSeamSewAction>());
	PrepAction(CommandInfos.SplitAction, NewObject<UTLLRN_UVSplitAction>());
	PrepAction(CommandInfos.MakeIslandAction, NewObject<UTLLRN_UVMakeIslandAction>());
	PrepAction(CommandInfos.TLLRN_UnsetUVsAction, NewObject<UTLLRN_UnsetUVsAction>());
}

bool UTLLRN_UVEditorMode::ShouldToolStartBeAllowed(const FString& ToolIdentifier) const
{
	if (bPIEModeActive)
	{
		return false;
	}

	// For now we've decided to disallow switch-away on accept/cancel tools in the UV editor.
	if (GetInteractiveToolsContext()->ActiveToolHasAccept())
	{
		return false;
	}
	
	return Super::ShouldToolStartBeAllowed(ToolIdentifier);
}

void UTLLRN_UVEditorMode::CreateToolkit()
{
	Toolkit = MakeShared<FTLLRN_UVEditorModeToolkit>();
}

void UTLLRN_UVEditorMode::OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	using namespace TLLRN_UVEditorModeLocals;

	FTLLRN_UVEditorToolActionCommands::UpdateToolCommandBinding(Tool, Toolkit->GetToolkitCommands(), false);
	if (LivePreviewToolkitCommands.IsValid())
	{
		FTLLRN_UVEditorToolActionCommands::UpdateToolCommandBinding(Tool, LivePreviewToolkitCommands.Pin(), false);
	}

	FText TransactionName = LOCTEXT("ActivateTool", "Activate Tool");
	
	if (!IsDefaultToolActive())
	{
		GetInteractiveToolsContext()->GetTransactionAPI()->BeginUndoTransaction(TransactionName);
	
		// If a tool doesn't support selection, we can't be certain that it won't put the meshes
		// in a state where the selection refers to invalid elements.
		ITLLRN_UVToolSupportsSelection* SelectionTool = Cast<ITLLRN_UVToolSupportsSelection>(Tool);
		if (!SelectionTool)
		{
			SelectionAPI->BeginChange();
			SelectionAPI->ClearSelections(false, false); 
			SelectionAPI->ClearUnsetElementAppliedMeshSelections(false, false);
			SelectionAPI->EndChangeAndEmitIfModified(true); // broadcast, emit
			SelectionAPI->ClearHighlight();
		}
		else
		{
			if (!SelectionTool->SupportsUnsetElementAppliedMeshSelections())
			{
				SelectionAPI->BeginChange();				
				SelectionAPI->ClearUnsetElementAppliedMeshSelections(false, false);
				SelectionAPI->EndChangeAndEmitIfModified(true); // broadcast, emit
				SelectionAPI->ClearHighlight(false, true); // Clear and rebuild applied highlight to account for clearing unset selections
				SelectionAPI->RebuildAppliedPreviewHighlight();
			}
		}
	
		GetInteractiveToolsContext()->GetTransactionAPI()->AppendChange(
			this, MakeUnique<FTLLRN_UVEditorBeginToolChange>(), TransactionName);

		GetInteractiveToolsContext()->GetTransactionAPI()->EndUndoTransaction();
	}

	UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
	UToolsContextCursorAPI* ToolsContextCursorAPI = ContextStore->FindContext<UToolsContextCursorAPI>();
	if (ToolsContextCursorAPI)
	{
		ToolsContextCursorAPI->ClearCursorOverride();		
	}

}

void UTLLRN_UVEditorMode::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	FTLLRN_UVEditorToolActionCommands::UpdateToolCommandBinding(Tool, Toolkit->GetToolkitCommands(), true);
	if (LivePreviewToolkitCommands.IsValid())
	{
		FTLLRN_UVEditorToolActionCommands::UpdateToolCommandBinding(Tool, LivePreviewToolkitCommands.Pin(), true);
	}

	for (TWeakObjectPtr<UTLLRN_UVToolContextObject> Context : ContextsToUpdateOnToolEnd)
	{
		if (ensure(Context.IsValid()))
		{
			Context->OnToolEnded(Tool);
		}
	}

	UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
	UToolsContextCursorAPI* ToolsContextCursorAPI = ContextStore->FindContext<UToolsContextCursorAPI>();
	if (ToolsContextCursorAPI)
	{
		ToolsContextCursorAPI->ClearCursorOverride();
	}

}

UObject* UTLLRN_UVEditorMode::GetBackgroundSettingsObject()
{
	if (BackgroundVisualization)
	{
		return BackgroundVisualization->Settings;
	}
	return nullptr;
}

UObject* UTLLRN_UVEditorMode::GetDistortionVisualsSettingsObject()
{
	if (DistortionVisualization)
	{
		return DistortionVisualization->Settings;
	}
	return nullptr;
}

UObject* UTLLRN_UVEditorMode::GetGridSettingsObject()
{
	if (TLLRN_UVEditorGridProperties)
	{
		return TLLRN_UVEditorGridProperties;
	}
	return nullptr;
}

UObject* UTLLRN_UVEditorMode::GetUnwrappedUXSettingsObject() const
{
	return TLLRN_UVEditorUnwrappedUXProperties.Get();
}

UObject* UTLLRN_UVEditorMode::GetLivePreviewUXSettingsObject() const
{
	return TLLRN_UVEditorLivePreviewUXProperties.Get();
}

UObject* UTLLRN_UVEditorMode::GetUDIMSettingsObject()
{
	if (TLLRN_UVEditorUDIMProperties)
	{
		return TLLRN_UVEditorUDIMProperties;
	}
	return nullptr;
}

UObject* UTLLRN_UVEditorMode::GetToolDisplaySettingsObject()
{
	UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
	UTLLRN_UVEditorToolPropertiesAPI* ToolPropertiesAPI = ContextStore->FindContext<UTLLRN_UVEditorToolPropertiesAPI>();

	if (ToolPropertiesAPI)
	{
		return ToolPropertiesAPI->GetToolDisplayProperties();
	}
	return nullptr;
}


void UTLLRN_UVEditorMode::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (SelectionAPI)
	{
		SelectionAPI->Render(RenderAPI);
	}
}

void UTLLRN_UVEditorMode::DrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI)
{
	if (SelectionAPI)
	{
		SelectionAPI->DrawHUD(Canvas, RenderAPI);
	}
}

void UTLLRN_UVEditorMode::ActivateDefaultTool()
{
	if (!bPIEModeActive)
	{
		GetInteractiveToolsContext()->StartTool(DefaultToolIdentifier);
	}
}

bool UTLLRN_UVEditorMode::IsDefaultToolActive()
{
	return GetInteractiveToolsContext() && GetInteractiveToolsContext()->IsToolActive(EToolSide::Mouse, DefaultToolIdentifier);
}

void UTLLRN_UVEditorMode::BindCommands()
{
	const FTLLRN_UVEditorCommands& CommandInfos = FTLLRN_UVEditorCommands::Get();
	const TSharedRef<FUICommandList>& CommandList = Toolkit->GetToolkitCommands();

	// Hookup Background toggle command
	CommandList->MapAction(CommandInfos.ToggleBackground, FExecuteAction::CreateLambda([this]() {
		BackgroundVisualization->Settings->bVisible = !BackgroundVisualization->Settings->bVisible;
	}));

	// Hook up to Enter/Esc key presses
	CommandList->MapAction(
		CommandInfos.AcceptOrCompleteActiveTool,
		FExecuteAction::CreateLambda([this]() {
			// Give the tool a chance to take the nested accept first
			if (GetToolManager()->HasAnyActiveTool())
			{
				UInteractiveTool* Tool = GetToolManager()->GetActiveTool(EToolSide::Mouse);
				IInteractiveToolNestedAcceptCancelAPI* AcceptAPI = Cast<IInteractiveToolNestedAcceptCancelAPI>(Tool);
				if (AcceptAPI && AcceptAPI->SupportsNestedAcceptCommand() && AcceptAPI->CanCurrentlyNestedAccept())
				{
					if (AcceptAPI->ExecuteNestedAcceptCommand())
					{
						return;
					}
				}
			}
			GetInteractiveToolsContext()->EndTool(EToolShutdownType::Accept); 
			ActivateDefaultTool();
			}),
		FCanExecuteAction::CreateLambda([this]() {
			if (GetInteractiveToolsContext()->CanAcceptActiveTool() || GetInteractiveToolsContext()->CanCompleteActiveTool())
			{
				return true;
			}
			// If we can't currently accept, may still be able to pass down to tool
			UInteractiveTool* Tool = GetToolManager()->GetActiveTool(EToolSide::Mouse);
			IInteractiveToolNestedAcceptCancelAPI* AcceptAPI = Cast<IInteractiveToolNestedAcceptCancelAPI>(Tool);
			return AcceptAPI && AcceptAPI->SupportsNestedAcceptCommand() && AcceptAPI->CanCurrentlyNestedAccept(); 
			}),
		FGetActionCheckState(),
		FIsActionButtonVisible::CreateLambda([this]() {return GetInteractiveToolsContext()->ActiveToolHasAccept(); }),
		EUIActionRepeatMode::RepeatDisabled
	);

	CommandList->MapAction(
		CommandInfos.CancelOrCompleteActiveTool,
		FExecuteAction::CreateLambda([this]() { 
			// Give the tool a chance to take the nested cancel first
			if (GetToolManager()->HasAnyActiveTool())
			{
				UInteractiveTool* Tool = GetToolManager()->GetActiveTool(EToolSide::Mouse);
				IInteractiveToolNestedAcceptCancelAPI* CancelAPI = Cast<IInteractiveToolNestedAcceptCancelAPI>(Tool);
				if (CancelAPI && CancelAPI->SupportsNestedCancelCommand() && CancelAPI->CanCurrentlyNestedCancel())
				{
					if (CancelAPI->ExecuteNestedCancelCommand())
					{
						return;
					}
				}
			}

			GetInteractiveToolsContext()->EndTool(EToolShutdownType::Cancel);
			ActivateDefaultTool();
			}),
		FCanExecuteAction::CreateLambda([this]() { 
			if (GetInteractiveToolsContext()->CanCancelActiveTool() || GetInteractiveToolsContext()->CanCompleteActiveTool())
			{
				return true;
			}
			// If we can't currently cancel, may still be able to pass down to tool
			UInteractiveTool* Tool = GetToolManager()->GetActiveTool(EToolSide::Mouse);
			IInteractiveToolNestedAcceptCancelAPI* CancelAPI = Cast<IInteractiveToolNestedAcceptCancelAPI>(Tool);
			return CancelAPI && CancelAPI->SupportsNestedCancelCommand() && CancelAPI->CanCurrentlyNestedCancel();
			}),
		FGetActionCheckState(),
		FIsActionButtonVisible::CreateLambda([this]() {return GetInteractiveToolsContext()->ActiveToolHasAccept(); }),
		EUIActionRepeatMode::RepeatDisabled
	);

	CommandList->MapAction(
		CommandInfos.SelectAll,
		FExecuteAction::CreateLambda([this]() {

			if (IsDefaultToolActive())
			{
				UInteractiveTool* Tool = GetToolManager()->GetActiveTool(EToolSide::Mouse);
				UTLLRN_UVSelectTool* SelectTool = Cast<UTLLRN_UVSelectTool>(Tool);
				check(SelectTool);
				SelectTool->SelectAll();
			}
		}),
		FCanExecuteAction::CreateLambda([this]() { 
			return IsDefaultToolActive();
		}));

	UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
	UTLLRN_UVToolViewportButtonsAPI* TLLRN_UVToolViewportButtonsAPI = ContextStore->FindContext<UTLLRN_UVToolViewportButtonsAPI>();
	if (TLLRN_UVToolViewportButtonsAPI)
	{
		TLLRN_UVToolViewportButtonsAPI->OnInitiateFocusCameraOnSelection.AddUObject(this, &UTLLRN_UVEditorMode::FocusLivePreviewCameraOnSelection);
	}
}

void UTLLRN_UVEditorMode::Exit()
{
	if (FEngineAnalytics::IsAvailable())
	{
		const FDateTime Now = FDateTime::UtcNow();
		const FTimespan ModeUsageDuration = Now - AnalyticsLastStartTimestamp;

		TArray<FAnalyticsEventAttribute> Attributes;
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Timestamp"), Now.ToString()));
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Duration.Seconds"), static_cast<float>(ModeUsageDuration.GetTotalSeconds())));

		FEngineAnalytics::GetProvider().RecordEvent(TLLRN_UVEditorAnalytics::TLLRN_UVEditorAnalyticsEventName(TEXT("Exit")), Attributes);
	}

	// ToolsContext->EndTool only shuts the tool on the next tick, and ToolsContext->DeactivateActiveTool is
	// inaccessible, so we end up having to do this to force the shutdown right now.
	GetToolManager()->DeactivateTool(EToolSide::Mouse, EToolShutdownType::Cancel);

	for (UTLLRN_TLLRN_UVToolAction* Action : RegisteredActions)
	{
		Action->Shutdown();
	}
	RegisteredActions.Reset();

	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& ToolInput : ToolInputObjects)
	{
		ToolInput->Shutdown();
	}
	ToolInputObjects.Reset();
	WireframesToTick.Reset();
	OriginalObjectsToEdit.Reset();
	
	for (const TObjectPtr<UMeshOpPreviewWithBackgroundCompute>& Preview : AppliedPreviews)
	{
		Preview->Shutdown();
	}
	AppliedPreviews.Reset();
	AppliedCanonicalMeshes.Reset();
	ToolTargets.Reset();

	if (BackgroundVisualization)
	{
		BackgroundVisualization->Disconnect();
		BackgroundVisualization = nullptr;
	}

	DistortionVisualization = nullptr;

	PropertyObjectsToTick.Empty();
	LivePreviewWorld = nullptr;

	bIsActive = false;

	UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
	for (TWeakObjectPtr<UTLLRN_UVToolContextObject> Context : ContextsToShutdown)
	{
		if (ensure(Context.IsValid()))
		{
			Context->Shutdown();
			ContextStore->RemoveContextObject(Context.Get());
		}
	}

	GetInteractiveToolsContext()->OnRender.RemoveAll(this);
	GetInteractiveToolsContext()->OnDrawHUD.RemoveAll(this);
	if (LivePreviewITC.IsValid())
	{
		LivePreviewITC->OnRender.RemoveAll(this);
		LivePreviewITC->OnDrawHUD.RemoveAll(this);
	}

	FEditorDelegates::PreBeginPIE.Remove(BeginPIEDelegateHandle);
	FEditorDelegates::EndPIE.Remove(EndPIEDelegateHandle);
	FEditorDelegates::CancelPIE.Remove(CancelPIEDelegateHandle);
	FEditorDelegates::PostSaveWorldWithContext.Remove(PostSaveWorldDelegateHandle);

	Super::Exit();
}

void UTLLRN_UVEditorMode::InitializeAssetEditorContexts(UContextObjectStore& ContextStore,
	const TArray<TObjectPtr<UObject>>& AssetsIn, const TArray<FTransform>& TransformsIn,
	FEditorViewportClient& LivePreviewViewportClient, FAssetEditorModeManager& LivePreviewModeManager,
	UTLLRN_UVToolViewportButtonsAPI& ViewportButtonsAPI, UTLLRN_UVTool2DViewportAPI& TLLRN_UVTool2DViewportAPI)
{
	using namespace TLLRN_UVEditorModeLocals;

	UTLLRN_UVToolAssetInputsContext* AssetInputsContext = ContextStore.FindContext<UTLLRN_UVToolAssetInputsContext>();
	if (!AssetInputsContext)
	{
		AssetInputsContext = NewObject<UTLLRN_UVToolAssetInputsContext>();
		AssetInputsContext->Initialize(AssetsIn, TransformsIn);
		ContextStore.AddContextObject(AssetInputsContext);
	}

	UEditorModelingObjectsCreationAPI* ModelingObjectsCreationAPI = ContextStore.FindContext<UEditorModelingObjectsCreationAPI>();
	if (!ModelingObjectsCreationAPI)
	{
		ModelingObjectsCreationAPI = NewObject<UEditorModelingObjectsCreationAPI>();
		ContextStore.AddContextObject(ModelingObjectsCreationAPI);
	}
	
	UToolsContextCursorAPI* LivePreviewToolsContextCursorAPI = LivePreviewModeManager.GetInteractiveToolsContext()->ContextObjectStore->FindContext<UToolsContextCursorAPI>();
	UTLLRN_UVToolLivePreviewAPI* LivePreviewAPI = ContextStore.FindContext<UTLLRN_UVToolLivePreviewAPI>();
	if (!LivePreviewAPI)
	{
		LivePreviewAPI = NewObject<UTLLRN_UVToolLivePreviewAPI>();
		LivePreviewAPI->Initialize(
			LivePreviewModeManager.GetPreviewScene()->GetWorld(),
			LivePreviewModeManager.GetInteractiveToolsContext()->InputRouter,
			[LivePreviewViewportClientPtr = &LivePreviewViewportClient](FViewCameraState& CameraStateOut) {
				GetCameraState(*LivePreviewViewportClientPtr, CameraStateOut);
			},
			[LivePreviewViewportClientPtr = &LivePreviewViewportClient](const FAxisAlignedBox3d& BoundingBox) {
				// We check for the Viewport here because it might not be open at the time this
				// method is called, e.g. during startup with an initially closed tab. And since
				// the FocusViewportOnBox method doesn't check internally that the Viewport is
				// available, this can crash.
				if (LivePreviewViewportClientPtr && LivePreviewViewportClientPtr->Viewport)
				{
					LivePreviewViewportClientPtr->FocusViewportOnBox((FBox)BoundingBox, true);
				}
			},
			[LivePreviewToolsContextCursorAPI](const EMouseCursor::Type Cursor, bool bEnableOverride)
			{
				if(bEnableOverride)
				{
					LivePreviewToolsContextCursorAPI->SetCursorOverride(Cursor);
				}
				else
				{
					LivePreviewToolsContextCursorAPI->ClearCursorOverride();
				}
			},
			LivePreviewModeManager.GetInteractiveToolsContext()->GizmoManager);
		ContextStore.AddContextObject(LivePreviewAPI);
	}

	// Prep the editor-only context that we use to pass things to the mode.
	if (!ContextStore.FindContext<UTLLRN_UVEditorInitializationContext>())
	{
		UTLLRN_UVEditorInitializationContext* InitContext = NewObject<UTLLRN_UVEditorInitializationContext>();
		InitContext->LivePreviewITC = Cast<UEditorInteractiveToolsContext>(LivePreviewModeManager.GetInteractiveToolsContext());

		LivePreviewModeManager.ActivateMode(UTLLRN_UVEditor3DViewportMode::EM_ModeID);
		UEdMode* LivePreviewDefaultMode = LivePreviewModeManager.GetActiveScriptableMode(UTLLRN_UVEditor3DViewportMode::EM_ModeID);
		if (ensure(LivePreviewDefaultMode))
		{
			TSharedPtr<FModeToolkit> Toolkit = LivePreviewDefaultMode->GetToolkit().Pin();
			if (ensure(Toolkit.IsValid()))
			{
				InitContext->LivePreviewToolkitCommands = Toolkit->GetToolkitCommands();
			}
		}

		ContextStore.AddContextObject(InitContext);
	}

	if (!ContextStore.FindContext<UTLLRN_UVToolViewportButtonsAPI>())
	{
		ContextStore.AddContextObject(&ViewportButtonsAPI);
	}

	if (!ContextStore.FindContext<UTLLRN_UVTool2DViewportAPI>())
	{
		ContextStore.AddContextObject(&TLLRN_UVTool2DViewportAPI);
	}
}

void UTLLRN_UVEditorMode::InitializeModeContexts()
{
	UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();

	UTLLRN_UVToolAssetInputsContext* AssetInputsContext = ContextStore->FindContext<UTLLRN_UVToolAssetInputsContext>();
	check(AssetInputsContext);
	AssetInputsContext->GetAssetInputs(OriginalObjectsToEdit, Transforms);
	ContextsToUpdateOnToolEnd.Add(AssetInputsContext);

	UTLLRN_UVToolLivePreviewAPI* LivePreviewAPI = ContextStore->FindContext<UTLLRN_UVToolLivePreviewAPI>();
	check(LivePreviewAPI);
	LivePreviewWorld = LivePreviewAPI->GetLivePreviewWorld();
	ContextsToUpdateOnToolEnd.Add(LivePreviewAPI);

	UTLLRN_UVToolViewportButtonsAPI* ViewportButtonsAPI = ContextStore->FindContext<UTLLRN_UVToolViewportButtonsAPI>();
	check(ViewportButtonsAPI);
	ContextsToUpdateOnToolEnd.Add(ViewportButtonsAPI);

	UTLLRN_UVTool2DViewportAPI* TLLRN_UVTool2DViewportAPI = ContextStore->FindContext<UTLLRN_UVTool2DViewportAPI>();
	check(TLLRN_UVTool2DViewportAPI);
	ContextsToUpdateOnToolEnd.Add(TLLRN_UVTool2DViewportAPI);

	// Helper function for adding contexts that our mode creates itself, rather than getting from
	// the asset editor.
	auto AddContextObject = [this, ContextStore](UTLLRN_UVToolContextObject* Object)
	{
		if (ensure(ContextStore->AddContextObject(Object)))
		{
			ContextsToShutdown.Add(Object);
		}
		ContextsToUpdateOnToolEnd.Add(Object);
	};

	UTLLRN_UVToolEmitChangeAPI* EmitChangeAPI = NewObject<UTLLRN_UVToolEmitChangeAPI>();
	EmitChangeAPI = NewObject<UTLLRN_UVToolEmitChangeAPI>();
	EmitChangeAPI->Initialize(GetInteractiveToolsContext()->ToolManager);
	AddContextObject(EmitChangeAPI);

	UTLLRN_UVToolAssetAndChannelAPI* AssetAndLayerAPI = NewObject<UTLLRN_UVToolAssetAndChannelAPI>();
	AssetAndLayerAPI->RequestChannelVisibilityChangeFunc = [this](const TArray<int32>& LayerPerAsset, bool bEmitUndoTransaction) {
		SetDisplayedUVChannels(LayerPerAsset, bEmitUndoTransaction);
	};
	AssetAndLayerAPI->NotifyOfAssetChannelCountChangeFunc = [this](int32 AssetID) {
		// Don't currently need to do anything because the layer selection menu gets populated
		// from scratch each time that it's opened.
	};
	AssetAndLayerAPI->GetCurrentChannelVisibilityFunc = [this]() {
		TArray<int32> VisibleLayers;
		VisibleLayers.SetNum(ToolTargets.Num());
		for (int32 AssetID = 0; AssetID < ToolTargets.Num(); ++AssetID)
		{
			VisibleLayers[AssetID] = GetDisplayedChannel(AssetID);
		}
		return VisibleLayers;
	};
	AddContextObject(AssetAndLayerAPI);

	SelectionAPI = NewObject<UTLLRN_UVToolSelectionAPI>();
	SelectionAPI->Initialize(GetToolManager(), GetWorld(), 
		GetInteractiveToolsContext()->InputRouter, LivePreviewAPI, EmitChangeAPI);
	AddContextObject(SelectionAPI);

	UTLLRN_UVEditorToolPropertiesAPI* TLLRN_UVEditorToolPropertiesAPI = NewObject<UTLLRN_UVEditorToolPropertiesAPI>();
	AddContextObject(TLLRN_UVEditorToolPropertiesAPI);

	// Register gizmo helper
	UE::TransformGizmoUtil::RegisterTransformGizmoContextObject(GetInteractiveToolsContext());
}

void UTLLRN_UVEditorMode::InitializeTargets()
{
	using namespace TLLRN_UVEditorModeLocals;

	// InitializeModeContexts needs to have been called first so that we have the 3d preview world ready.
	check(LivePreviewWorld);

	// Build the tool targets that provide us with 3d dynamic meshes
	UTLLRN_UVEditorSubsystem* UVSubsystem = GEditor->GetEditorSubsystem<UTLLRN_UVEditorSubsystem>();
	UVSubsystem->BuildTargets(OriginalObjectsToEdit, GetToolTargetRequirements(), ToolTargets);

	// Collect the 3d dynamic meshes from targets. There will always be one for each asset, and the AssetID
	// of each asset will be the index into these arrays. Individual input objects (representing a specific
	// UV layer), will point to these existing 3d meshes.
	for (UToolTarget* Target : ToolTargets)
	{
		// The applied canonical mesh is the 3d mesh with all of the layer changes applied. If we switch
		// to a different layer, the changes persist in the applied canonical.
		TSharedPtr<FDynamicMesh3> AppliedCanonical = MakeShared<FDynamicMesh3>(UE::ToolTarget::GetDynamicMeshCopy(Target));
		AppliedCanonicalMeshes.Add(AppliedCanonical);

		// Make a preview version of the applied canonical to show. Tools can attach computes to this, though
		// they would have to take care if we ever allow multiple layers to be displayed for one asset, to
		// avoid trying to attach two computes to the same preview object (in which case one would be thrown out)
		UMeshOpPreviewWithBackgroundCompute* AppliedPreview = NewObject<UMeshOpPreviewWithBackgroundCompute>();
		AppliedPreview->Setup(LivePreviewWorld);
		AppliedPreview->PreviewMesh->UpdatePreview(AppliedCanonical.Get());

		FComponentMaterialSet MaterialSet = UE::ToolTarget::GetMaterialSet(Target);
		AppliedPreview->ConfigureMaterials(MaterialSet.Materials,
			ToolSetupUtil::GetDefaultWorkingMaterial(GetToolManager()));
		AppliedPreviews.Add(AppliedPreview);
	}

	UMaterialInterface* Material = LoadObject<UMaterial>(nullptr, TEXT("/TLLRN_UVEditor/Materials/TLLRN_UVEditor_UnwrapMaterial"));

	// When creating UV unwraps, these functions will determine the mapping between UV values and the
	// resulting unwrap mesh vertex positions. 
	// If we're looking down on the unwrapped mesh, with the Z axis towards us, we want U's to be right, and
	// V's to be up. In Unreal's left-handed coordinate system, this means that we map U's to world Y
	// and V's to world X.
	// Also, Unreal changes the V coordinates of imported meshes to 1-V internally, and we undo this
	// while displaying the UV's because the users likely expect to see the original UV's (it would
	// be particularly confusing for users working with UDIM assets, where internally stored V's 
	// frequently end up negative).
	// The ScaleFactor just scales the mesh up. Scaling the mesh up makes it easier to zoom in
	// further into the display before getting issues with the camera near plane distance.

	// Construct the full input objects that the tools actually operate on.
	for (int32 AssetID = 0; AssetID < ToolTargets.Num(); ++AssetID)
	{
		UTLLRN_UVEditorToolMeshInput* ToolInputObject = NewObject<UTLLRN_UVEditorToolMeshInput>();

		if (!ToolInputObject->InitializeMeshes(ToolTargets[AssetID], AppliedCanonicalMeshes[AssetID],
			AppliedPreviews[AssetID], AssetID, DefaultUVLayerIndex,
			GetWorld(), LivePreviewWorld, ToolSetupUtil::GetDefaultWorkingMaterial(GetToolManager()),
			FTLLRN_UVEditorUXSettings::UVToVertPosition, FTLLRN_UVEditorUXSettings::VertPositionToUV))
		{
			return;
		}

		if (Transforms.Num() == ToolTargets.Num())
		{
			ToolInputObject->AppliedPreview->PreviewMesh->SetTransform(Transforms[AssetID]);
		}

		UMaterialInstanceDynamic* MatInstance = UMaterialInstanceDynamic::Create(Material, GetToolManager());
		MatInstance->SetVectorParameterValue(TEXT("Color"), FTLLRN_UVEditorUXSettings::GetTriangleColorByTargetIndex(AssetID));
		MatInstance->SetScalarParameterValue(TEXT("DepthBias"), FTLLRN_UVEditorUXSettings::UnwrapTriangleDepthOffset);
		MatInstance->SetScalarParameterValue(TEXT("Opacity"), FTLLRN_UVEditorUXSettings::UnwrapTriangleOpacity);
		MatInstance->SetScalarParameterValue(TEXT("UseVertexColors"), false);

		ToolInputObject->bEnableTriangleVertexColors = false;
		ToolInputObject->UnwrapPreview->PreviewMesh->SetMaterial(
			0, MatInstance);

		// Set up the wireframe display of the unwrapped mesh.
		UMeshElementsVisualizer* WireframeDisplay = NewObject<UMeshElementsVisualizer>(this);
		WireframeDisplay->CreateInWorld(GetWorld(), FTransform::Identity);

		WireframeDisplay->Settings->DepthBias = FTLLRN_UVEditorUXSettings::WireframeDepthOffset;
		WireframeDisplay->Settings->bAdjustDepthBiasUsingMeshSize = false;
		WireframeDisplay->Settings->bShowWireframe = true;
		WireframeDisplay->Settings->bShowBorders = true;
		WireframeDisplay->Settings->WireframeColor = FTLLRN_UVEditorUXSettings::GetWireframeColorByTargetIndex(AssetID).ToFColorSRGB();
		WireframeDisplay->Settings->BoundaryEdgeColor = FTLLRN_UVEditorUXSettings::GetBoundaryColorByTargetIndex(AssetID).ToFColorSRGB(); ;
		WireframeDisplay->Settings->bShowUVSeams = false;
		WireframeDisplay->Settings->bShowNormalSeams = false;
		// These are not exposed at the visualizer level yet
		// TODO: Should they be?
		WireframeDisplay->WireframeComponent->WireframeThickness = FTLLRN_UVEditorUXSettings::WireframeThickness;
		WireframeDisplay->WireframeComponent->BoundaryEdgeThickness = FTLLRN_UVEditorUXSettings::BoundaryEdgeThickness;

		// The wireframe will track the unwrap preview mesh
		WireframeDisplay->SetMeshAccessFunction([ToolInputObject](UMeshElementsVisualizer::ProcessDynamicMeshFunc ProcessFunc) {
			ToolInputObject->UnwrapPreview->ProcessCurrentMesh(ProcessFunc);
			});

		// The settings object and wireframe are not part of a tool, so they won't get ticked like they
		// are supposed to (to enable property watching), unless we add this here.
		PropertyObjectsToTick.Add(WireframeDisplay->Settings);
		WireframesToTick.Add(WireframeDisplay);

		// The tool input object will hold on to the wireframe for the purposes of updating it and cleaning it up
		ToolInputObject->WireframeDisplay = WireframeDisplay;

		// Bind to delegate so that we can detect changes
		ToolInputObject->OnCanonicalModified.AddWeakLambda(this, [this]
		(UTLLRN_UVEditorToolMeshInput* InputObject, const UTLLRN_UVEditorToolMeshInput::FCanonicalModifiedInfo&) {
				ModifiedAssetIDs.Add(InputObject->AssetID);
			});

		ToolInputObjects.Add(ToolInputObject);
	}

	// Prep things for layer/channel selection
	InitializeAssetNames(ToolTargets, AssetNames);
	PendingUVLayerIndex.SetNumZeroed(ToolTargets.Num());


	// Finish initializing the selection api
	SelectionAPI->SetTargets(ToolInputObjects);
}

void UTLLRN_UVEditorMode::EmitToolIndependentObjectChange(UObject* TargetObject, TUniquePtr<FToolCommandChange> Change, const FText& Description)
{
	GetInteractiveToolsContext()->GetTransactionAPI()->AppendChange(TargetObject, MoveTemp(Change), Description);
}

bool UTLLRN_UVEditorMode::HaveUnappliedChanges() const
{
	return ModifiedAssetIDs.Num() > 0;
}

bool UTLLRN_UVEditorMode::CanApplyChanges() const
{
	return !bPIEModeActive && HaveUnappliedChanges();
}


void UTLLRN_UVEditorMode::GetAssetsWithUnappliedChanges(TArray<TObjectPtr<UObject>> UnappliedAssetsOut)
{
	for (int32 AssetID : ModifiedAssetIDs)
	{
		// The asset ID corresponds to the index into OriginalObjectsToEdit
		UnappliedAssetsOut.Add(OriginalObjectsToEdit[AssetID]);
	}
}

void UTLLRN_UVEditorMode::ApplyChanges()
{
	using namespace TLLRN_UVEditorModeLocals;

	FText ApplyChangesText = LOCTEXT("TLLRN_UVEditorApplyChangesTransaction", "UV Editor Apply Changes");
	GetToolManager()->BeginUndoTransaction(ApplyChangesText);

	for (int32 AssetID : ModifiedAssetIDs)
	{
		// The asset ID corresponds to the index into ToolTargets and AppliedCanonicalMeshes
		UE::ToolTarget::CommitDynamicMeshUVUpdate(ToolTargets[AssetID], AppliedCanonicalMeshes[AssetID].Get());
	}

	GetInteractiveToolsContext()->GetTransactionAPI()->AppendChange(
		this, MakeUnique<TLLRN_UVEditorModeChange::FApplyChangesChange>(ModifiedAssetIDs), ApplyChangesText);
	ModifiedAssetIDs.Reset();
	GetInteractiveToolsContext()->GetTransactionAPI()->EndUndoTransaction();

	GetToolManager()->EndUndoTransaction();
}


void UTLLRN_UVEditorMode::UpdateActiveUDIMs()
{
	bool bEnableUDIMSupport = (FTLLRN_UVEditorUXSettings::CVarEnablePrototypeUDIMSupport.GetValueOnGameThread() > 0);
	if (!bEnableUDIMSupport)
	{
		return;
	}

	if (!TLLRN_UVEditorUDIMProperties)
	{
		return;
	}

	TSet<FTLLRN_UDIMSpecifier> ActiveUDIMs(TLLRN_UVEditorUDIMProperties->ActiveUDIMs);
	UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
	UTLLRN_UVTool2DViewportAPI* TLLRN_UVTool2DViewportAPI = ContextStore->FindContext<UTLLRN_UVTool2DViewportAPI>();
	if (TLLRN_UVTool2DViewportAPI)
	{
		TArray<FTLLRN_UDIMBlock> Blocks;
		if (ActiveUDIMs.Num() > 0)
		{
			Blocks.Reserve(ActiveUDIMs.Num());
			for (FTLLRN_UDIMSpecifier& TLLRN_UDIMSpecifier : ActiveUDIMs)
			{
				Blocks.Add({ TLLRN_UDIMSpecifier.UDIM, TLLRN_UDIMSpecifier.TextureResolution });
				TLLRN_UDIMSpecifier.UCoord = Blocks.Last().BlockU();
				TLLRN_UDIMSpecifier.VCoord = Blocks.Last().BlockV();				
			}
		}
		TLLRN_UVTool2DViewportAPI->SetTLLRN_UDIMBlocks(Blocks, true);
	}
	TLLRN_UVEditorUDIMProperties->ActiveUDIMs = ActiveUDIMs.Array();
	TLLRN_UVEditorUDIMProperties->SilentUpdateWatcherAtIndex(UDIMsChangedWatcherId);

	if (BackgroundVisualization)
	{
		BackgroundVisualization->Settings->TLLRN_UDIMBlocks.Empty();
		BackgroundVisualization->Settings->TLLRN_UDIMBlocks.Reserve(ActiveUDIMs.Num());
		for (FTLLRN_UDIMSpecifier& TLLRN_UDIMSpecifier : ActiveUDIMs)
		{
			BackgroundVisualization->Settings->TLLRN_UDIMBlocks.Add(TLLRN_UDIMSpecifier.UDIM);
		}
		BackgroundVisualization->Settings->CheckAndUpdateWatched();
	}

	if (DistortionVisualization)
	{
		DistortionVisualization->Settings->PerUDIMTextureResolution.Empty();
		DistortionVisualization->Settings->PerUDIMTextureResolution.Reserve(ActiveUDIMs.Num());
		for (FTLLRN_UDIMSpecifier& TLLRN_UDIMSpecifier : ActiveUDIMs)
		{
			DistortionVisualization->Settings->PerUDIMTextureResolution.Add(TLLRN_UDIMSpecifier.UDIM, TLLRN_UDIMSpecifier.TextureResolution);
		}
		DistortionVisualization->Settings->CheckAndUpdateWatched();
	}
}

void UTLLRN_UVEditorMode::PopulateUDIMsByAsset(int32 AssetId, TArray<FTLLRN_UDIMSpecifier>& UDIMsOut) const
{
	UDIMsOut.Empty();
	if (AssetId < 0 || AssetId >= ToolInputObjects.Num())
	{
		return;
	}

	if (ToolInputObjects[AssetId]->AppliedCanonical->HasAttributes() && ToolInputObjects[AssetId]->UVLayerIndex >= 0)
	{
		FDynamicMeshUVOverlay* UVLayer = ToolInputObjects[AssetId]->AppliedCanonical->Attributes()->GetUVLayer(ToolInputObjects[AssetId]->UVLayerIndex);
		FDynamicMeshUDIMClassifier TileClassifier(UVLayer);
		TArray<FVector2i> Tiles = TileClassifier.ActiveTiles();
		for (FVector2i Tile : Tiles)
		{
			if (Tile.X >= 10 || Tile.X < 0 || Tile.Y < 0)
			{
				UE_LOG(LogTLLRN_UVEditor, Warning, TEXT("Tile <%d,%d> is out of bounds of the UDIM10 convention, skipping..."), Tile.X, Tile.Y);
			}
			else
			{
				FTLLRN_UDIMSpecifier TLLRN_UDIMSpecifier;
				TLLRN_UDIMSpecifier.UCoord = Tile.X;
				TLLRN_UDIMSpecifier.VCoord = Tile.Y;
				TLLRN_UDIMSpecifier.UDIM = UE::TextureUtilitiesCommon::GetUDIMIndex(Tile.X, Tile.Y);
				UDIMsOut.AddUnique(TLLRN_UDIMSpecifier);
			}
		}
	}
}

void UTLLRN_UVEditorMode::UpdateTriangleMaterialBasedOnBackground(bool IsBackgroundVisible)
{
	UpdateTriangleMaterialBasedOnDisplaySettings();
}

void UTLLRN_UVEditorMode::UpdateTriangleMaterialBasedOnDisplaySettings()
{
	float TriangleOpacity;

	// We adjust the mesh opacity depending on whether we're layered over the background or not.
	if (BackgroundVisualization && BackgroundVisualization->Settings && BackgroundVisualization->Settings->bVisible)
	{
		TriangleOpacity = FTLLRN_UVEditorUXSettings::UnwrapTriangleOpacityWithBackground;
	}
	else
	{
		TriangleOpacity = FTLLRN_UVEditorUXSettings::UnwrapTriangleOpacity;
	}

	bool bUseVertexColors = DistortionVisualization && DistortionVisualization->Settings && DistortionVisualization->Settings->bVisible;

	// Modify the material of the unwrapped mesh to account for the presence/absence of the background, 
	// changing the opacity as set just above.
	UMaterialInterface* Material = LoadObject<UMaterial>(nullptr, TEXT("/TLLRN_UVEditor/Materials/TLLRN_UVEditor_UnwrapMaterial"));
	if (Material != nullptr)
	{
		for (int32 AssetID = 0; AssetID < ToolInputObjects.Num(); ++AssetID) {
			UMaterialInstanceDynamic* MatInstance = UMaterialInstanceDynamic::Create(Material, GetToolManager());
			MatInstance->SetVectorParameterValue(TEXT("Color"), FTLLRN_UVEditorUXSettings::GetTriangleColorByTargetIndex(AssetID));
			MatInstance->SetScalarParameterValue(TEXT("DepthBias"), FTLLRN_UVEditorUXSettings::UnwrapTriangleDepthOffset);
			MatInstance->SetScalarParameterValue(TEXT("Opacity"), TriangleOpacity);
			MatInstance->SetScalarParameterValue(TEXT("UseVertexColors"), bUseVertexColors);

			ToolInputObjects[AssetID]->bEnableTriangleVertexColors = bUseVertexColors;
			ToolInputObjects[AssetID]->UnwrapPreview->PreviewMesh->SetMaterial(
				0, MatInstance);
		}
	}
}

void UTLLRN_UVEditorMode::UpdatePreviewMaterialBasedOnBackground()
{
	for (int32 AssetID = 0; AssetID < ToolInputObjects.Num(); ++AssetID) {
		ToolInputObjects[AssetID]->AppliedPreview->OverrideMaterial = nullptr;
	}
	if (BackgroundVisualization->Settings->bVisible)
	{
		for (int32 AssetID = 0; AssetID < ToolInputObjects.Num(); ++AssetID) {			
			TObjectPtr<UMaterialInstanceDynamic> BackgroundMaterialOverride = UMaterialInstanceDynamic::Create(BackgroundVisualization->BackgroundMaterial->Parent, this);
			BackgroundMaterialOverride->CopyInterpParameters(BackgroundVisualization->BackgroundMaterial);
			BackgroundMaterialOverride->SetScalarParameterValue(TEXT("UVChannel"), static_cast<float>(GetDisplayedChannel(AssetID)));

			ToolInputObjects[AssetID]->AppliedPreview->OverrideMaterial = BackgroundMaterialOverride;
		}
	}
}

void UTLLRN_UVEditorMode::FocusLivePreviewCameraOnSelection()
{
	UContextObjectStore* ContextStore = GetInteractiveToolsContext()->ToolManager->GetContextObjectStore();
	UTLLRN_UVToolLivePreviewAPI* LivePreviewAPI = ContextStore->FindContext<UTLLRN_UVToolLivePreviewAPI>();
	if (!LivePreviewAPI)
	{
		return;
	}

	FAxisAlignedBox3d SelectionBoundingBox;

	const TArray<FUVToolSelection>& CurrentSelections = SelectionAPI->GetSelections();
	const TArray<FUVToolSelection>& CurrentUnsetSelections = SelectionAPI->GetUnsetElementAppliedMeshSelections();

	for (const FUVToolSelection& Selection : CurrentSelections)
	{
		FTransform3d Transform = Selection.Target->AppliedPreview->PreviewMesh->GetTransform();
		SelectionBoundingBox.Contain(Selection.GetConvertedSelectionForAppliedMesh().ToBoundingBox(*Selection.Target->AppliedCanonical, Transform));
	}
	for (const FUVToolSelection& Selection : CurrentUnsetSelections)
	{
		FTransform3d Transform = Selection.Target->AppliedPreview->PreviewMesh->GetTransform();
		SelectionBoundingBox.Contain(Selection.ToBoundingBox(*Selection.Target->AppliedCanonical, Transform));
	}
	if (CurrentSelections.Num() == 0 && CurrentUnsetSelections.Num() == 0)
	{
		for (int32 AssetID = 0; AssetID < ToolInputObjects.Num(); ++AssetID)
		{
			FTransform3d Transform = ToolInputObjects[AssetID]->AppliedPreview->PreviewMesh->GetTransform();
			FAxisAlignedBox3d ObjectBoundingBox = ToolInputObjects[AssetID]->AppliedCanonical->GetBounds();
			ObjectBoundingBox.Max = Transform.TransformPosition(ObjectBoundingBox.Max);
			ObjectBoundingBox.Min = Transform.TransformPosition(ObjectBoundingBox.Min);
			SelectionBoundingBox.Contain(ObjectBoundingBox);
		}
	}
	
	LivePreviewAPI->SetLivePreviewCameraToLookAtVolume(SelectionBoundingBox);
}

void UTLLRN_UVEditorMode::ModeTick(float DeltaTime)
{
	using namespace TLLRN_UVEditorModeLocals;

	Super::ModeTick(DeltaTime);

	bool bSwitchingLayers = false;
	for (int32 AssetID = 0; AssetID < ToolInputObjects.Num(); ++AssetID)
	{
		if (ToolInputObjects[AssetID]->UVLayerIndex != PendingUVLayerIndex[AssetID])
		{
			bSwitchingLayers = true;
			break;
		}
	}

	if (bSwitchingLayers)
	{
		// TODO: Perhaps we need our own interactive tools context that allows this kind of "end tool now"
		// call. We can't do the normal GetInteractiveToolsContext()->EndTool() call because we cannot defer
		// shutdown.
		GetInteractiveToolsContext()->ToolManager->DeactivateTool(EToolSide::Mouse, EToolShutdownType::Cancel);

		// We open the transaction here instead of before the tool close above because otherwise we seem
		// to get some kind of spurious items in the transaction that, while they don't seem to do anything,
		// cause the transaction to not be fully expired on editor close.
		GetToolManager()->BeginUndoTransaction(UVLayerChangeTransactionName);

		SetDisplayedUVChannels(PendingUVLayerIndex, true);

		ActivateDefaultTool();

		GetToolManager()->EndUndoTransaction();
	}
	
	for (TObjectPtr<UInteractiveToolPropertySet>& Propset : PropertyObjectsToTick)
	{
		if (Propset)
		{
			if (Propset->IsPropertySetEnabled())
			{
				Propset->CheckAndUpdateWatched();
			}
			else
			{
				Propset->SilentUpdateWatched();
			}
		}
	}

	for (TWeakObjectPtr<UMeshElementsVisualizer> WireframeDisplay : WireframesToTick)
	{
		if (WireframeDisplay.IsValid())
		{
			WireframeDisplay->OnTick(DeltaTime);
		}
	}

	if (BackgroundVisualization)
	{
		BackgroundVisualization->OnTick(DeltaTime);
	}

	if (DistortionVisualization)
	{
		DistortionVisualization->OnTick(DeltaTime);
	}

	for (int i = 0; i < ToolInputObjects.Num(); ++i)
	{
		TObjectPtr<UTLLRN_UVEditorToolMeshInput> ToolInput = ToolInputObjects[i];
		ToolInput->AppliedPreview->Tick(DeltaTime);
		ToolInput->UnwrapPreview->Tick(DeltaTime);
	}

	// If nothing is running at this point, restart the default tool, since it needs to be running to handle some toolbar UX
	if (GetInteractiveToolsContext() && !GetInteractiveToolsContext()->HasActiveTool())
	{
		ActivateDefaultTool();
	}

}

void UTLLRN_UVEditorMode::SetSimulationWarning(bool bEnabled)
{
	if (Toolkit.IsValid())
	{
		FTLLRN_UVEditorModeToolkit* TLLRN_UVEditorModeToolkit = StaticCast<FTLLRN_UVEditorModeToolkit*>(Toolkit.Get());
		TLLRN_UVEditorModeToolkit->EnableShowPIEWarning(bEnabled);
	}
}

int32 UTLLRN_UVEditorMode::GetNumUVChannels(int32 AssetID) const
{
	if (AssetID < 0 || AssetID >= AppliedCanonicalMeshes.Num())
	{
		return IndexConstants::InvalidID;
	}

	return AppliedCanonicalMeshes[AssetID]->HasAttributes() ?
		AppliedCanonicalMeshes[AssetID]->Attributes()->NumUVLayers() : 0;
}

int32 UTLLRN_UVEditorMode::GetDisplayedChannel(int32 AssetID) const
{
	if (!ensure(AssetID >= 0 && AssetID < ToolInputObjects.Num()))
	{
		return IndexConstants::InvalidID;
	}

	return ToolInputObjects[AssetID]->UVLayerIndex;;
}

void UTLLRN_UVEditorMode::RequestUVChannelChange(int32 AssetID, int32 Channel)
{
	if (!ensure(
		AssetID >= 0 && AssetID < ToolTargets.Num())
		&& Channel >= 0 && Channel < GetNumUVChannels(AssetID))
	{
		return;
	}

	PendingUVLayerIndex[AssetID] = Channel;
}

void UTLLRN_UVEditorMode::ChangeInputObjectLayer(int32 AssetID, int32 NewLayerIndex)
{
	using namespace TLLRN_UVEditorModeLocals;

	if (!ensure(AssetID >= 0 && AssetID < ToolInputObjects.Num() 
		&& ToolInputObjects[AssetID]->AssetID == AssetID))
	{
		return;
	}

	UTLLRN_UVEditorToolMeshInput* InputObject = ToolInputObjects[AssetID];
	if (InputObject->UVLayerIndex != NewLayerIndex)
	{
		InputObject->UVLayerIndex = NewLayerIndex;
		InputObject->UpdateAllFromAppliedCanonical();
	}

	PendingUVLayerIndex[AssetID] = InputObject->UVLayerIndex;
	UpdatePreviewMaterialBasedOnBackground();
}

void UTLLRN_UVEditorMode::SetDisplayedUVChannels(const TArray<int32>& LayerPerAsset, bool bEmitUndoTransaction)
{
	// Deal with any existing selections first.
	// TODO: We could consider keeping triangle selections, since these can be valid across different layers,
	// or converting the selections to elements in the applied mesh and back. The gain for this would probably
	// be minor, so for now we'll just clear selection here.
	if (SelectionAPI)
	{
		SelectionAPI->ClearSelections(true, bEmitUndoTransaction); // broadcast, maybe emit
	}

	// Now actually swap out the layers
	for (int32 AssetID = 0; AssetID < ToolInputObjects.Num(); ++AssetID)
	{
		if (ToolInputObjects[AssetID]->UVLayerIndex != LayerPerAsset[AssetID])
		{			
			if (bEmitUndoTransaction)
			{
				GetInteractiveToolsContext()->GetTransactionAPI()->AppendChange(this,
					MakeUnique<TLLRN_UVEditorModeLocals::FInputObjectUVLayerChange>(AssetID, ToolInputObjects[AssetID]->UVLayerIndex, LayerPerAsset[AssetID]),
					TLLRN_UVEditorModeLocals::UVLayerChangeTransactionName);
			}

			ChangeInputObjectLayer(AssetID, LayerPerAsset[AssetID]);
		}
	}
}

#undef LOCTEXT_NAMESPACE

