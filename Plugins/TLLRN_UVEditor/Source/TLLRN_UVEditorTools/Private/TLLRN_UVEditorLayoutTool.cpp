// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorLayoutTool.h"

#include "ContextObjectStore.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshChangeTracker.h"
#include "InteractiveToolManager.h"
#include "MeshOpPreviewHelpers.h" // UMeshOpPreviewWithBackgroundCompute
#include "ParameterizationOps/UVLayoutOp.h"
#include "Properties/UVLayoutProperties.h"
#include "ToolTargets/TLLRN_UVEditorToolMeshInput.h"
#include "ContextObjects/TLLRN_UVToolContextObjects.h"
#include "EngineAnalytics.h"
#include "TLLRN_UVEditorToolAnalyticsUtils.h"
#include "TLLRN_UVEditorUXSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVEditorLayoutTool)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UTLLRN_UVEditorLayoutTool"

// Tool builder
// TODO: Could consider sharing some of the tool builder boilerplate for UV editor tools in a common base class.

bool UTLLRN_UVEditorLayoutToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return Targets && Targets->Num() > 0;
}

UInteractiveTool* UTLLRN_UVEditorLayoutToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_UVEditorLayoutTool* NewTool = NewObject<UTLLRN_UVEditorLayoutTool>(SceneState.ToolManager);
	NewTool->SetTargets(*Targets);

	return NewTool;
}


void UTLLRN_UVEditorLayoutTool::Setup()
{
	check(Targets.Num() > 0);
	
	ToolStartTimeAnalytics = FDateTime::UtcNow();

	UInteractiveTool::Setup();

	Settings = NewObject<UUVLayoutProperties>(this);
	Settings->RestoreProperties(this);
	Settings->bUDIMCVAREnabled = (FTLLRN_UVEditorUXSettings::CVarEnablePrototypeUDIMSupport.GetValueOnGameThread() > 0);
	AddToolPropertySource(Settings);

	UContextObjectStore* ContextStore = GetToolManager()->GetContextObjectStore();
	TLLRN_UVToolSelectionAPI = ContextStore->FindContext<UTLLRN_UVToolSelectionAPI>();

	TLLRN_UVTool2DViewportAPI = ContextStore->FindContext<UTLLRN_UVTool2DViewportAPI>();

	UTLLRN_UVToolSelectionAPI::FHighlightOptions HighlightOptions;
	HighlightOptions.bBaseHighlightOnPreviews = true;
	HighlightOptions.bAutoUpdateUnwrap = true;
	TLLRN_UVToolSelectionAPI->SetHighlightOptions(HighlightOptions);
	TLLRN_UVToolSelectionAPI->SetHighlightVisible(true, false, true);

	auto SetupOpFactory = [this](UTLLRN_UVEditorToolMeshInput& Target, const FUVToolSelection* Selection)
	{
		TObjectPtr<UUVLayoutOperatorFactory> Factory = NewObject<UUVLayoutOperatorFactory>();
		Factory->TargetTransform = Target.AppliedPreview->PreviewMesh->GetTransform();
		Factory->Settings = Settings;
		Factory->OriginalMesh = Target.AppliedCanonical;
		Factory->GetSelectedUVChannel = [&Target]() { return Target.UVLayerIndex; };
		if (Selection)
		{
			Factory->Selection.Emplace(Selection->GetConvertedSelection(*Selection->Target->UnwrapCanonical,
				                                                        UE::Geometry::FUVToolSelection::EType::Triangle).SelectedIDs);
		}

		if (TLLRN_UVTool2DViewportAPI)
		{
			TMap<int32, int32> TextureResolutionPerUDIM;
			for (const FTLLRN_UDIMBlock& UDIM : TLLRN_UVTool2DViewportAPI->GetTLLRN_UDIMBlocks())
			{
				TextureResolutionPerUDIM.Add(UDIM.UDIM, UDIM.TextureResolution);
			}
			Factory->TextureResolutionPerUDIM = TextureResolutionPerUDIM;
		}

		Target.AppliedPreview->ChangeOpFactory(Factory);
		Target.AppliedPreview->OnMeshUpdated.AddWeakLambda(this, [this, &Target](UMeshOpPreviewWithBackgroundCompute* Preview) {
			Target.UpdateUnwrapPreviewFromAppliedPreview();

			this->TLLRN_UVToolSelectionAPI->RebuildUnwrapHighlight(Preview->PreviewMesh->GetTransform());
			});

		Target.AppliedPreview->InvalidateResult();
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

	SetToolDisplayName(LOCTEXT("ToolName", "UV Layout"));
	GetToolManager()->DisplayMessage(LOCTEXT("OnStartUVLayoutTool", "Translate, rotate or scale existing UV Charts using various strategies"),
		EToolMessageLevel::UserNotification);
	
	// Analytics
	InputTargetAnalytics = TLLRN_UVEditorAnalytics::CollectTargetAnalytics(Targets);
}

void UTLLRN_UVEditorLayoutTool::Shutdown(EToolShutdownType ShutdownType)
{
	Settings->SaveProperties(this);
	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->AppliedPreview->OnMeshUpdated.RemoveAll(this);
	}

	if (ShutdownType == EToolShutdownType::Accept)
	{
		UTLLRN_UVToolEmitChangeAPI* ChangeAPI = GetToolManager()->GetContextObjectStore()->FindContext<UTLLRN_UVToolEmitChangeAPI>();
		const FText TransactionName(LOCTEXT("LayoutTransactionName", "Layout Tool"));
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

			ChangeAPI->EmitToolIndependentUnwrapCanonicalChange(Target, ChangeTracker.EndChange(), LOCTEXT("ApplyLayoutTool", "Layout Tool"));
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
		Target->AppliedPreview->ClearOpFactory();
	}

	for (int32 FactoryIndex = 0; FactoryIndex < Factories.Num(); ++FactoryIndex)
	{
		Factories[FactoryIndex] = nullptr;
	}

	Settings = nullptr;
	Targets.Empty();
}

void UTLLRN_UVEditorLayoutTool::OnTick(float DeltaTime)
{
	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->AppliedPreview->Tick(DeltaTime);
	}
}



void UTLLRN_UVEditorLayoutTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->AppliedPreview->InvalidateResult();
	}
}

bool UTLLRN_UVEditorLayoutTool::CanAccept() const
{
	if (TLLRN_UVToolSelectionAPI->HaveSelections())
	{
		for (FUVToolSelection Selection : TLLRN_UVToolSelectionAPI->GetSelections())
		{
			if (!Selection.Target->AppliedPreview->HaveValidResult())
			{
				return false;
			}
		}
	}
	else
	{
		for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
		{
			if (!Target->AppliedPreview->HaveValidResult())
			{
				return false;
			}
		}
	}
	return true;
}

void UTLLRN_UVEditorLayoutTool::RecordAnalytics()
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
			PerAssetValidResultComputeTimes.Add(Target->AppliedPreview->GetValidResultComputeTime());
		}
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Stats.PerAsset.ComputeTimeSeconds"), PerAssetValidResultComputeTimes));
	}
	Attributes.Add(FAnalyticsEventAttribute(TEXT("Stats.ToolActiveDuration"), (FDateTime::UtcNow() - ToolStartTimeAnalytics).ToString()));

	// Tool settings chosen by the user
	Attributes.Add(AnalyticsEventAttributeEnum(TEXT("Settings.LayoutType"), Settings->LayoutType));
	Attributes.Add(FAnalyticsEventAttribute(TEXT("Settings.TextureResolution"), Settings->TextureResolution));
	Attributes.Add(FAnalyticsEventAttribute(TEXT("Settings.Scale"), Settings->Scale));
	const TArray<FVector2D::FReal> TranslationArray({Settings->Translation.X, Settings->Translation.Y});
	Attributes.Add(FAnalyticsEventAttribute(TEXT("Settings.Translation"), TranslationArray));
	Attributes.Add(FAnalyticsEventAttribute(TEXT("Settings.AllowFlips"), Settings->bAllowFlips));

	FEngineAnalytics::GetProvider().RecordEvent(TLLRN_UVEditorAnalyticsEventName(TEXT("LayoutTool")), Attributes);

	if constexpr (false)
	{
		for (const FAnalyticsEventAttribute& Attr : Attributes)
		{
			UE_LOG(LogGeometry, Log, TEXT("Debug %s.%s = %s"), *TLLRN_UVEditorAnalyticsEventName(TEXT("LayoutTool")), *Attr.GetName(), *Attr.GetValue());
		}
	}
}

#undef LOCTEXT_NAMESPACE

