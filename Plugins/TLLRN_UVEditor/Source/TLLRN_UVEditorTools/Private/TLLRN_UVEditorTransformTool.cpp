// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorTransformTool.h"

#include "ContextObjectStore.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshChangeTracker.h"
#include "InteractiveToolManager.h"
#include "MeshOpPreviewHelpers.h" // UMeshOpPreviewWithBackgroundCompute
#include "Operators/TLLRN_UVEditorUVTransformOp.h"
#include "SceneView.h"
#include "ToolTargets/TLLRN_UVEditorToolMeshInput.h"
#include "ContextObjects/TLLRN_UVToolContextObjects.h"
#include "EngineAnalytics.h"
#include "TLLRN_UVEditorToolAnalyticsUtils.h"
#include "TLLRN_UVEditorUXSettings.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVEditorTransformTool)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UTLLRN_UVEditorTransformTool"

namespace TransformToolLocals
{
	FText ToolName(ETLLRN_UVEditorUVTransformType Mode)
	{
		switch (Mode)
		{
		case ETLLRN_UVEditorUVTransformType::Transform:
			return LOCTEXT("ToolNameTransform", "UV Transform");
		case ETLLRN_UVEditorUVTransformType::Align:
			return LOCTEXT("ToolNameAlign", "UV Align");
		case ETLLRN_UVEditorUVTransformType::Distribute:
			return LOCTEXT("ToolNameDistribute", "UV Distribute");
		default:
			ensure(false);
			return FText();
		}
	}

	FText ToolDescription(ETLLRN_UVEditorUVTransformType Mode)
	{
		switch (Mode)
		{
		case ETLLRN_UVEditorUVTransformType::Transform:
			return LOCTEXT("OnStartToolTransform", "Translate, rotate or scale existing UV Charts using various strategies");
		case ETLLRN_UVEditorUVTransformType::Align:
			return LOCTEXT("OnStartToolAlign", "Align UV elements relative to various positions and with various strategies");
		case ETLLRN_UVEditorUVTransformType::Distribute:
			return LOCTEXT("OnStartToolDistribute", "Distribute UV elements spatially with various strategies");
		default:
			ensure(false);
			return FText();
		}
	}

	FText ToolTransaction(ETLLRN_UVEditorUVTransformType Mode)
	{
		switch (Mode)
		{
		case ETLLRN_UVEditorUVTransformType::Transform:
			return LOCTEXT("TransactionNameTransform", "Transform Tool");
		case ETLLRN_UVEditorUVTransformType::Align:
			return LOCTEXT("TransactionNameAlign", "Align Tool");
		case ETLLRN_UVEditorUVTransformType::Distribute:
			return LOCTEXT("TransactionNameDistribute", "Distribute Tool");
		default:
			ensure(false);
			return FText();
		}
	}

	FText ToolConfirmation(ETLLRN_UVEditorUVTransformType Mode)
	{
		switch (Mode)
		{
		case ETLLRN_UVEditorUVTransformType::Transform:
			return LOCTEXT("ApplyToolTransform", "Transform Tool");
		case ETLLRN_UVEditorUVTransformType::Align:
			return LOCTEXT("ApplyToolAlign", "Align Tool");
		case ETLLRN_UVEditorUVTransformType::Distribute:
			return LOCTEXT("ApplyToolDistribute", "Distribute Tool");
		default:
			ensure(false);
			return FText();
		}
	}
}


// Tool builder
// TODO: Could consider sharing some of the tool builder boilerplate for UV editor tools in a common base class.

bool UTLLRN_UVEditorBaseTransformToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return Targets && Targets->Num() > 0;
}

UInteractiveTool* UTLLRN_UVEditorBaseTransformToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_UVEditorTransformTool* NewTool = NewObject<UTLLRN_UVEditorTransformTool>(SceneState.ToolManager);
	ConfigureTool(NewTool);	
	return NewTool;
}

void UTLLRN_UVEditorBaseTransformToolBuilder::ConfigureTool(UTLLRN_UVEditorTransformTool* NewTool) const
{
	NewTool->SetTargets(*Targets);
}

void UTLLRN_UVEditorTransformToolBuilder::ConfigureTool(UTLLRN_UVEditorTransformTool* NewTool) const
{
	UTLLRN_UVEditorBaseTransformToolBuilder::ConfigureTool(NewTool);
	NewTool->SetToolMode(ETLLRN_UVEditorUVTransformType::Transform);
}

void UTLLRN_UVEditorAlignToolBuilder::ConfigureTool(UTLLRN_UVEditorTransformTool* NewTool) const
{
	UTLLRN_UVEditorBaseTransformToolBuilder::ConfigureTool(NewTool);
	NewTool->SetToolMode(ETLLRN_UVEditorUVTransformType::Align);
}

void UTLLRN_UVEditorDistributeToolBuilder::ConfigureTool(UTLLRN_UVEditorTransformTool* NewTool) const
{
	UTLLRN_UVEditorBaseTransformToolBuilder::ConfigureTool(NewTool);
	NewTool->SetToolMode(ETLLRN_UVEditorUVTransformType::Distribute);
}

void UTLLRN_UVEditorTransformTool::SetToolMode(const ETLLRN_UVEditorUVTransformType& Mode)
{
	ToolMode = Mode;
}

void UTLLRN_UVEditorTransformTool::Setup()
{
	check(Targets.Num() > 0);

	ToolStartTimeAnalytics = FDateTime::UtcNow();

	UInteractiveTool::Setup();
	UContextObjectStore* ContextStore = GetToolManager()->GetContextObjectStore();

	switch(ToolMode.Get(ETLLRN_UVEditorUVTransformType::Transform))
	{
		case ETLLRN_UVEditorUVTransformType::Transform:
			Settings = NewObject<UTLLRN_UVEditorUVTransformProperties>(this);
			break;
		case ETLLRN_UVEditorUVTransformType::Align:
			Settings = NewObject<UTLLRN_UVEditorUVAlignProperties>(this);
			break;
		case ETLLRN_UVEditorUVTransformType::Distribute:
			Settings = NewObject<UTLLRN_UVEditorUVDistributeProperties>(this);
			break;
		default:
			ensure(false);
	}
	Settings->RestoreProperties(this);
	//Settings->bUDIMCVAREnabled = (FTLLRN_UVEditorUXSettings::CVarEnablePrototypeUDIMSupport.GetValueOnGameThread() > 0);
	AddToolPropertySource(Settings);

	DisplaySettings = NewObject<UTLLRN_UVEditorTransformToolDisplayProperties>(this);
	DisplaySettings->RestoreProperties(this);
	DisplaySettings->GetOnModified().AddLambda([this](UObject* PropertySetArg, FProperty* PropertyArg)
	{
		OnPropertyModified(PropertySetArg, PropertyArg);
	});
	UTLLRN_UVEditorToolPropertiesAPI* PropertiesAPI = ContextStore->FindContext<UTLLRN_UVEditorToolPropertiesAPI>();
	if (PropertiesAPI)
	{
		PropertiesAPI->SetToolDisplayProperties(DisplaySettings);
	}

	TLLRN_UVToolSelectionAPI = ContextStore->FindContext<UTLLRN_UVToolSelectionAPI>();

	UTLLRN_UVToolSelectionAPI::FHighlightOptions HighlightOptions;
	HighlightOptions.bBaseHighlightOnPreviews = true;
	HighlightOptions.bAutoUpdateUnwrap = true;
	TLLRN_UVToolSelectionAPI->SetHighlightOptions(HighlightOptions);
	TLLRN_UVToolSelectionAPI->SetHighlightVisible(true, false, true);

	PerTargetPivotLocations.SetNum(Targets.Num());

	auto SetupOpFactory = [this](UTLLRN_UVEditorToolMeshInput& Target, const FUVToolSelection* Selection)
	{
		int32 TargetIndex = Targets.Find(&Target);

		TObjectPtr<UTLLRN_UVEditorUVTransformOperatorFactory> Factory = NewObject<UTLLRN_UVEditorUVTransformOperatorFactory>();
		Factory->TargetTransform = Target.UnwrapPreview->PreviewMesh->GetTransform();
		Factory->Settings = Settings;
		Factory->TransformType = ToolMode.Get(ETLLRN_UVEditorUVTransformType::Transform);
		Factory->OriginalMesh = Target.UnwrapCanonical;
		Factory->GetSelectedUVChannel = [&Target]() { return 0; /*Since we're passing in the unwrap mesh, the UV index is always zero.*/ };
		if (Selection)
		{
			// Generate vertex and edge selection sets for the operation. These are needed to differentiate
			// between islands, whether they are composed of triangles, edges or isolated points.
			FUVToolSelection UnwrapVertexSelection;
			FUVToolSelection UnwrapEdgeSelection;
			switch (Selection->Type)
			{
			case FUVToolSelection::EType::Vertex:
				Factory->VertexSelection.Emplace(Selection->SelectedIDs);
				Factory->EdgeSelection.Emplace(Selection->GetConvertedSelection(*Target.UnwrapCanonical, FUVToolSelection::EType::Edge).SelectedIDs);
				break;
			case FUVToolSelection::EType::Edge:
				Factory->VertexSelection.Emplace(Selection->GetConvertedSelection(*Target.UnwrapCanonical, FUVToolSelection::EType::Vertex).SelectedIDs);
				Factory->EdgeSelection.Emplace(Selection->SelectedIDs);
				break;
			case FUVToolSelection::EType::Triangle:
				Factory->VertexSelection.Emplace(Selection->GetConvertedSelection(*Target.UnwrapCanonical, FUVToolSelection::EType::Vertex).SelectedIDs);
				Factory->EdgeSelection.Emplace(Selection->GetConvertedSelection(*Target.UnwrapCanonical, FUVToolSelection::EType::Edge).SelectedIDs);
				break;
			}
		}

		Target.UnwrapPreview->ChangeOpFactory(Factory);
		Target.UnwrapPreview->OnMeshUpdated.AddWeakLambda(this, [this, &Target](UMeshOpPreviewWithBackgroundCompute* Preview) {
			Target.UpdateUnwrapPreviewOverlayFromPositions();
			Target.UpdateAppliedPreviewFromUnwrapPreview();
			

			this->TLLRN_UVToolSelectionAPI->RebuildUnwrapHighlight(Target.UnwrapPreview->PreviewMesh->GetTransform());
			});

		Target.UnwrapPreview->OnOpCompleted.AddWeakLambda(this,
			[this, TargetIndex](const FDynamicMeshOperator* Op)
			{
				const FTLLRN_UVEditorUVTransformBaseOp* TransformBaseOp = (const FTLLRN_UVEditorUVTransformBaseOp*)(Op);
				PerTargetPivotLocations[TargetIndex] = TransformBaseOp->GetPivotLocations();
			}
		);

		Target.UnwrapPreview->InvalidateResult();
		return Factory;
	};

	if (TLLRN_UVToolSelectionAPI->HaveSelections())
	{
		Factories.Reserve(TLLRN_UVToolSelectionAPI->GetSelections().Num());
		for (FUVToolSelection Selection : TLLRN_UVToolSelectionAPI->GetSelections())
		{
			Factories.Add(SetupOpFactory(*Selection.Target, &Selection));
		}
	}
	else
	{
		Factories.Reserve(Targets.Num());
		for (int32 TargetIndex = 0; TargetIndex < Targets.Num(); ++TargetIndex)
		{
			Factories.Add(SetupOpFactory(*Targets[TargetIndex], nullptr));
		}
	}

	SetToolDisplayName(TransformToolLocals::ToolName(ToolMode.Get(ETLLRN_UVEditorUVTransformType::Transform)));
	GetToolManager()->DisplayMessage(TransformToolLocals::ToolDescription(ToolMode.Get(ETLLRN_UVEditorUVTransformType::Transform)),
		EToolMessageLevel::UserNotification);

	// Analytics
	InputTargetAnalytics = TLLRN_UVEditorAnalytics::CollectTargetAnalytics(Targets);
}

void UTLLRN_UVEditorTransformTool::Shutdown(EToolShutdownType ShutdownType)
{
	Settings->SaveProperties(this);
	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->UnwrapPreview->OnMeshUpdated.RemoveAll(this);
		Target->UnwrapPreview->OnOpCompleted.RemoveAll(this);
	}

	if (ShutdownType == EToolShutdownType::Accept)
	{
		UTLLRN_UVToolEmitChangeAPI* ChangeAPI = GetToolManager()->GetContextObjectStore()->FindContext<UTLLRN_UVToolEmitChangeAPI>();
		const FText TransactionName(TransformToolLocals::ToolTransaction(ToolMode.Get(ETLLRN_UVEditorUVTransformType::Transform)));
		ChangeAPI->BeginUndoTransaction(TransactionName);

		for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
		{
			// Set things up for undo. 
			// TODO: It's not entirely clear whether it would be safe to use a FMeshVertexChange instead... It seems like
			// when bAllowFlips is true, we would end up with changes to the tris of the unwrap. Also, if we stick to saving
			// all the tris and verts, should we consider using the new dynamic mesh serialization?
			FDynamicMeshChangeTracker ChangeTracker(Target->UnwrapCanonical.Get());
			ChangeTracker.BeginChange();

			for (int32 Tid : Target->UnwrapCanonical->TriangleIndicesItr())
			{
				ChangeTracker.SaveTriangle(Tid, true);
			}

			// TODO: Again, it's not clear whether we need to update the entire triangle topology...
			Target->UpdateCanonicalFromPreviews();

			ChangeAPI->EmitToolIndependentUnwrapCanonicalChange(Target, ChangeTracker.EndChange(),
				       TransformToolLocals::ToolConfirmation(ToolMode.Get(ETLLRN_UVEditorUVTransformType::Transform)));
		}

		ChangeAPI->EndUndoTransaction();

		// Analytics
		RecordAnalytics();
	}
	else
	{
		// Reset the inputs
		for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
		{
			Target->UpdatePreviewsFromCanonical();
		}
	}


	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->UnwrapPreview->ClearOpFactory();
	}

	for (int32 FactoryIndex = 0; FactoryIndex < Factories.Num(); ++FactoryIndex)
	{
		Factories[FactoryIndex] = nullptr;
	}

	Settings = nullptr;
	DisplaySettings = nullptr;
	Targets.Empty();
}

void UTLLRN_UVEditorTransformTool::OnTick(float DeltaTime)
{
	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->UnwrapPreview->Tick(DeltaTime);
	}
}

void UTLLRN_UVEditorTransformTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	if (PropertySet == DisplaySettings)
	{
		return;
	}

	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->UnwrapPreview->InvalidateResult();
	}
}

bool UTLLRN_UVEditorTransformTool::CanAccept() const
{

	if (TLLRN_UVToolSelectionAPI->HaveSelections())
	{
		for (FUVToolSelection Selection : TLLRN_UVToolSelectionAPI->GetSelections())
		{
			if (!Selection.Target->UnwrapPreview->HaveValidResult())
			{
				return false;
			}
		}
	}
	else
	{
		for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
		{
			if (!Target->UnwrapPreview->HaveValidResult())
			{
				return false;
			}
		}
	}
	return true;
}

void UTLLRN_UVEditorTransformTool::DrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI)
{
	// TODO: Add support here for highlighting first selected item for alignment visualization	

	auto ConvertUVToPixel = [RenderAPI](const FVector2D& UVIn, FVector2D& PixelOut)
	{
		FVector WorldPoint = FTLLRN_UVEditorUXSettings::ExternalUVToUnwrapWorldPosition((FVector2f)UVIn);
		FVector4 TestProjectedHomogenous = RenderAPI->GetSceneView()->WorldToScreen(WorldPoint);
		bool bValid = RenderAPI->GetSceneView()->ScreenToPixel(TestProjectedHomogenous, PixelOut);
		return bValid;
	};

	if (DisplaySettings->bDrawPivots)
	{
		for (int32 TargetIndex = 0; TargetIndex < Targets.Num(); ++TargetIndex)
		{
			for (const FVector2D& PivotLocation : PerTargetPivotLocations[TargetIndex])
			{
				FVector2D PivotLocationPixel;
				ConvertUVToPixel(PivotLocation, PivotLocationPixel);

				const int32 NumSides = FTLLRN_UVEditorUXSettings::PivotCircleNumSides;
				const float Radius = FTLLRN_UVEditorUXSettings::PivotCircleRadius;
				const float LineThickness = FTLLRN_UVEditorUXSettings::PivotLineThickness;
				const FColor LineColor = FTLLRN_UVEditorUXSettings::PivotLineColor;

				const float	AngleDelta = 2.0f * UE_PI / NumSides;
				FVector2D AxisX(1.f, 0.f);
				FVector2D AxisY(0.f, -1.f);
				FVector2D LastVertex = PivotLocationPixel + AxisX * Radius;

				for (int32 SideIndex = 0; SideIndex < NumSides; SideIndex++)
				{
					const FVector2D Vertex = PivotLocationPixel + (AxisX * FMath::Cos(AngleDelta * (SideIndex + 1)) + AxisY * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
					FCanvasLineItem LineItem(LastVertex, Vertex);
					LineItem.LineThickness = LineThickness;
					LineItem.SetColor(LineColor);
					Canvas->DrawItem(LineItem);
					LastVertex = Vertex;
				}
			}
		}
	}
}


void UTLLRN_UVEditorTransformTool::RecordAnalytics()
{
	using namespace TLLRN_UVEditorAnalytics;

	if (!FEngineAnalytics::IsAvailable())
	{
		return;
	}

	TArray<FAnalyticsEventAttribute> Attributes;
	Attributes.Add(FAnalyticsEventAttribute(TEXT("Timestamp"), FDateTime::UtcNow().ToString()));

	// Tool inputs
	InputTargetAnalytics.AppendToAttributes(Attributes, "Input");

	// Tool stats
	if (CanAccept())
	{
		TArray<double> PerAssetValidResultComputeTimes;
		for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
		{
			// Note: This would log -1 if the result was invalid, but checking CanAccept above ensures results are valid
			PerAssetValidResultComputeTimes.Add(Target->UnwrapPreview->GetValidResultComputeTime());
		}
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Stats.PerAsset.ComputeTimeSeconds"), PerAssetValidResultComputeTimes));
	}
	Attributes.Add(FAnalyticsEventAttribute(TEXT("Stats.ToolActiveDuration"), (FDateTime::UtcNow() - ToolStartTimeAnalytics).ToString()));

	// Tool settings chosen by the user
	//Attributes.Add(AnalyticsEventAttributeEnum(TEXT("Settings.LayoutType"), Settings->LayoutType));
	//Attributes.Add(FAnalyticsEventAttribute(TEXT("Settings.TextureResolution"), Settings->TextureResolution));
	//Attributes.Add(FAnalyticsEventAttribute(TEXT("Settings.Scale"), Settings->Scale));
	//const TArray<FVector2D::FReal> TranslationArray({ Settings->Translation.X, Settings->Translation.Y });
	//Attributes.Add(FAnalyticsEventAttribute(TEXT("Settings.Translation"), TranslationArray));
	//Attributes.Add(FAnalyticsEventAttribute(TEXT("Settings.AllowFlips"), Settings->bAllowFlips));

	FEngineAnalytics::GetProvider().RecordEvent(TLLRN_UVEditorAnalyticsEventName(TEXT("TransformTool")), Attributes);

	if constexpr (false)
	{
		for (const FAnalyticsEventAttribute& Attr : Attributes)
		{
			UE_LOG(LogGeometry, Log, TEXT("Debug %s.%s = %s"), *TLLRN_UVEditorAnalyticsEventName(TEXT("TransformTool")), *Attr.GetName(), *Attr.GetValue());
		}
	}
}

#undef LOCTEXT_NAMESPACE

