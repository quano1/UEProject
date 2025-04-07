// Copyright Epic Games, Inc. All Rights Reserved.

#include "Selection/TLLRN_UVToolSelectionAPI.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "MeshOpPreviewHelpers.h" // UMeshOpPreviewWithBackgroundCompute
#include "Selection/TLLRN_UVEditorMeshSelectionMechanic.h"
#include "Selection/TLLRN_UVToolSelectionHighlightMechanic.h"
#include "TLLRN_UVEditorMechanicAdapterTool.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVToolSelectionAPI)

#define LOCTEXT_NAMESPACE "UUVIslandConformalUnwrapAction"

using namespace UE::Geometry;

namespace TLLRN_UVToolSelectionAPILocals
{
	FText SelectionChangeTransactionName = LOCTEXT("SelectionChangeTransaction", "UV Selection Change");

	auto DoSelectionSetsDiffer(const TArray<FUVToolSelection>& OldSelections, const TArray<FUVToolSelection>& NewSelections)
	{
		if (NewSelections.Num() != OldSelections.Num())
		{
			return true;
		}

		for (const FUVToolSelection& Selection : NewSelections)
		{
			// Find the selection that points to the same target.
			const FUVToolSelection* FoundSelection = OldSelections.FindByPredicate(
				[&Selection](const FUVToolSelection& OldSelection) { return OldSelection.Target == Selection.Target; }
			);
			if (!FoundSelection || *FoundSelection != Selection)
			{
				return true;
			}
		}
		return false;
	};
}

void UTLLRN_UVToolSelectionAPI::Initialize(
	UInteractiveToolManager* ToolManagerIn,
	UWorld* UnwrapWorld, UInputRouter* UnwrapInputRouterIn, 
	UTLLRN_UVToolLivePreviewAPI* LivePreviewAPI,
	UTLLRN_UVToolEmitChangeAPI* EmitChangeAPIIn)
{
	UnwrapInputRouter = UnwrapInputRouterIn;
	EmitChangeAPI = EmitChangeAPIIn;

	MechanicAdapter = NewObject<UTLLRN_UVEditorMechanicAdapterTool>();
	MechanicAdapter->ToolManager = ToolManagerIn;

	HighlightMechanic = NewObject<UTLLRN_UVToolSelectionHighlightMechanic>();
	HighlightMechanic->Setup(MechanicAdapter);
	HighlightMechanic->Initialize(UnwrapWorld, LivePreviewAPI->GetLivePreviewWorld());

	SelectionMechanic = NewObject<UTLLRN_UVEditorMeshSelectionMechanic>();
	SelectionMechanic->Setup(MechanicAdapter);
	SelectionMechanic->Initialize(UnwrapWorld, LivePreviewAPI->GetLivePreviewWorld(), this);

	UnwrapInputRouter->RegisterSource(MechanicAdapter);

	bIsActive = true;
}

void UTLLRN_UVToolSelectionAPI::SetTargets(const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn)
{
	Targets = TargetsIn;
	SelectionMechanic->SetTargets(Targets);

	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->OnCanonicalModified.AddWeakLambda(this, [this](UTLLRN_UVEditorToolMeshInput* Target,
			const UTLLRN_UVEditorToolMeshInput::FCanonicalModifiedInfo ModifiedInfo)
			{
				FUVToolSelection* TargetSelection = CurrentSelections.FindByPredicate(
					[Target](const FUVToolSelection& CandidateSelection) { return CandidateSelection.Target == Target; });
				if (TargetSelection)
				{
					bCachedUnwrapSelectionBoundingBoxCenterValid = false;
					if (TargetSelection->Type == FUVToolSelection::EType::Edge)
					{
						TargetSelection->RestoreFromStableEdgeIdentifiers(*Target->UnwrapCanonical);
					}
				}				
			});
	}
}

void UTLLRN_UVToolSelectionAPI::Shutdown()
{
	if (UnwrapInputRouter.IsValid())
	{
		// Make sure that we stop any captures that our mechanics may have, then remove them from
		// the input router.
		UnwrapInputRouter->ForceTerminateSource(MechanicAdapter);
		UnwrapInputRouter->DeregisterSource(MechanicAdapter);
		UnwrapInputRouter = nullptr;
	}

	HighlightMechanic->Shutdown();
	SelectionMechanic->Shutdown();

	MechanicAdapter->Shutdown(EToolShutdownType::Completed);
	HighlightMechanic = nullptr;
	SelectionMechanic = nullptr;
	MechanicAdapter = nullptr;

	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->OnCanonicalModified.RemoveAll(this);
	}
	Targets.Empty();

	bIsActive = false;
}

void UTLLRN_UVToolSelectionAPI::ClearSelections(bool bBroadcast, bool bEmitChange)
{
	SetSelections(TArray<FUVToolSelection>(), bBroadcast, bEmitChange);
}

void UTLLRN_UVToolSelectionAPI::OnToolEnded(UInteractiveTool* DeadTool)
{
	if (SelectionMechanic)
	{
		SetSelectionMechanicOptions(FSelectionMechanicOptions());
		SelectionMechanic->SetIsEnabled(false);
	}
	if (HighlightMechanic)
	{
		SetHighlightVisible(false, false);
		SetHighlightOptions(FHighlightOptions());
	}

	OnSelectionChanged.RemoveAll(DeadTool);
	OnPreSelectionChange.RemoveAll(DeadTool);
	OnDragSelectionChanged.RemoveAll(DeadTool);
	
	if (UnwrapInputRouter.IsValid())
	{
		// Make sure that we stop any captures that our mechanics may have.
		UnwrapInputRouter->ForceTerminateSource(MechanicAdapter);
	}
	
}

void UTLLRN_UVToolSelectionAPI::SetLivePreviewSelectionUXSettings(const FLivePreviewSelectionUXSettings& Settings) const
{
	if (Settings.SelectionColor)
	{
		HighlightMechanic->SetColor(Settings.SelectionColor.GetValue());
	}

	if (Settings.LineThickness)
	{
		HighlightMechanic->SetLineThickness(Settings.LineThickness.GetValue());
	}

	if (Settings.PointSize)
	{
		HighlightMechanic->SetPointSize(Settings.PointSize.GetValue());
	}
}

void UTLLRN_UVToolSelectionAPI::SetSelections(const TArray<FUVToolSelection>& SelectionsIn, bool bBroadcast, bool bEmitChange)
{
	using namespace TLLRN_UVToolSelectionAPILocals;

	bool bUsingExistingTransaction = bEmitChange;
	if (bEmitChange && ensureMsgf(!PendingSelectionChange && !PendingUnsetSelectionChange,
		TEXT("SetSelections called with bEmitChange while an existing SelectionAPI change transaction is already open. This should be safe but bEmitChange does not need to be explicitly set.")))
	{
		BeginChange();
		bUsingExistingTransaction = false;
	}

	bCachedUnwrapSelectionBoundingBoxCenterValid = false;

	// If we don't match with the current unset selection type, just clear them to keep everything consistent
	if (CurrentUnsetSelections.Num() > 0 && SelectionsIn.Num() > 0 && SelectionsIn[0].Type != CurrentUnsetSelections[0].Type)
	{
		CurrentUnsetSelections.Empty();
	}

	TArray<FUVToolSelection> NewSelections;
	for (const FUVToolSelection& NewSelection : SelectionsIn)
	{
		if (ensure(NewSelection.Target.IsValid() && NewSelection.Target->AreMeshesValid())
			// All of the selections should match type
			&& ensure(NewSelections.Num() == 0 || NewSelection.Type == NewSelections[0].Type)
			// Selection must not be empty, unless it's an edge selection stored as stable identifiers
			&& ensure(!NewSelection.IsEmpty() 
				|| (NewSelection.Type == FUVToolSelection::EType::Edge && NewSelection.HasStableEdgeIdentifiers()))
			// Shouldn't have selection objects pointing to same target
			&& ensure(!NewSelections.FindByPredicate(
				[&NewSelection](const FUVToolSelection& ExistingSelectionElement) {
					return NewSelection.Target == ExistingSelectionElement.Target;
				})))
		{
			NewSelections.Add(NewSelection);

			if (NewSelection.Target.IsValid() && ensure(NewSelection.Target->UnwrapCanonical))
			{
				checkSlow(NewSelection.AreElementsPresentInMesh(*NewSelection.Target->UnwrapCanonical));

				if (NewSelection.Type == FUVToolSelection::EType::Edge)
				{
					NewSelections.Last().SaveStableEdgeIdentifiers(*NewSelection.Target->UnwrapCanonical);
				}
			}
		}
	}

	/*
	* Depending on whether we're calling EndChangeAndEmitIfModified or not, the comparison of selections
	* and the broadcasting of PreSelectionChange and OnSelectionChanged might be done for us. If we're not 
	* calling EndChangeAndEmitIfModified (either because bEmitChange was false, or we're inside an existing
	* transaction that we expect the user to close), then we have to do these things ourselves.
	* 
	* This is a bit messy, but it allows us to not duplicate the change emitting code from EndChangeAndEmitIfModified.
	*/
	bool bWillCallEndChange = bEmitChange && !bUsingExistingTransaction;

	bool bSelectionsDiffer = false;
	if (!bWillCallEndChange)
	{
		bSelectionsDiffer = DoSelectionSetsDiffer(CurrentSelections, NewSelections);

		if (bBroadcast && bSelectionsDiffer)
		{
			OnPreSelectionChange.Broadcast(bEmitChange, (uint32)(ESelectionChangeTypeFlag::SelectionChanged));
		}
	}

	CurrentSelections = MoveTemp(NewSelections);

	if (bWillCallEndChange)
	{
		bSelectionsDiffer = EndChangeAndEmitIfModified(bBroadcast);
	}
	else
	{
		if (bBroadcast && bSelectionsDiffer)
		{
			OnSelectionChanged.Broadcast(bEmitChange, (uint32)(ESelectionChangeTypeFlag::SelectionChanged));
		}
	}

	if (bSelectionsDiffer)
	{

		if (HighlightOptions.bAutoUpdateUnwrap)
		{
			FTransform Transform = FTransform::Identity;
			if (HighlightOptions.bUseCentroidForUnwrapAutoUpdate)
			{
				Transform = FTransform(GetUnwrapSelectionBoundingBoxCenter());
			}
			RebuildUnwrapHighlight(Transform);
		}
		if (HighlightOptions.bAutoUpdateApplied)
		{
			RebuildAppliedPreviewHighlight();
		}
	}
}

void UTLLRN_UVToolSelectionAPI::SetSelectionMechanicOptions(const FSelectionMechanicOptions& Options)
{
	SelectionMechanic->SetShowHoveredElements(Options.bShowHoveredElements);
}

void UTLLRN_UVToolSelectionAPI::SetSelectionMechanicEnabled(bool bIsEnabled)
{
	SelectionMechanic->SetIsEnabled(bIsEnabled);
}

void UTLLRN_UVToolSelectionAPI::SetSelectionMechanicMode(ETLLRN_UVEditorSelectionMode Mode,
	const FSelectionMechanicModeChangeOptions& Options)
{
	SelectionMechanic->SetSelectionMode(Mode, Options);
}

FVector3d UTLLRN_UVToolSelectionAPI::GetUnwrapSelectionBoundingBoxCenter(bool bForceRecalculate)
{
	if (bCachedUnwrapSelectionBoundingBoxCenterValid && !bForceRecalculate)
	{
		return CachedUnwrapSelectionBoundingBoxCenter;
	}

	FVector3d& Center = CachedUnwrapSelectionBoundingBoxCenter;
	
	FAxisAlignedBox3d AllSelectionsBoundingBox;
	double Divisor = 0;
	for (const FUVToolSelection& Selection : GetSelections())
	{
		FDynamicMesh3* Mesh = Selection.Target->UnwrapCanonical.Get();
		AllSelectionsBoundingBox.Contain(Selection.ToBoundingBox(*Mesh));
	}
	Center = AllSelectionsBoundingBox.Center();

	bCachedUnwrapSelectionBoundingBoxCenterValid = true;
	return Center;
}


bool UTLLRN_UVToolSelectionAPI::HaveUnsetElementAppliedMeshSelections() const
{
	return CurrentUnsetSelections.Num() > 0;
}

void UTLLRN_UVToolSelectionAPI::SetUnsetElementAppliedMeshSelections(const TArray<FUVToolSelection>& UnsetSelectionsIn, bool bBroadcast, bool bEmitChange)
{
	using namespace TLLRN_UVToolSelectionAPILocals;

	bool bUsingExistingTransaction = bEmitChange;
	if (bEmitChange && ensureMsgf(!PendingSelectionChange && !PendingUnsetSelectionChange,
		TEXT("SetUnsetSelections called with bEmitChange while an existing SelectionAPI change transaction is already open. This should be safe but bEmitChange does not need to be explicitly set.")))
	{
		BeginChange();
		bUsingExistingTransaction = false;
	}

	// If we don't match with the current unset selection type, just clear them to keep everything consistent
	if (CurrentSelections.Num() > 0 && UnsetSelectionsIn.Num() > 0 && UnsetSelectionsIn[0].Type != CurrentSelections[0].Type)
	{
		CurrentSelections.Empty();
	}

	TArray<FUVToolSelection> NewUnsetSelections;
	for (const FUVToolSelection& NewUnsetSelection : UnsetSelectionsIn)
	{
		if (ensure(NewUnsetSelection.Target.IsValid() && NewUnsetSelection.Target->AreMeshesValid())
			// All of the unset selections should match type
			&& ensure(NewUnsetSelections.Num() == 0 || NewUnsetSelection.Type == NewUnsetSelections[0].Type)
			// Unset selection must not be empty
			&& ensure(!NewUnsetSelection.IsEmpty())
				// Shouldn't have unset selection objects pointing to same target
			&& ensure(!NewUnsetSelections.FindByPredicate(
				[&NewUnsetSelection](const FUVToolSelection& ExistingSelectionElement) {
					return NewUnsetSelection.Target == ExistingSelectionElement.Target;
				})))
		{
			NewUnsetSelections.Add(NewUnsetSelection);

			if (NewUnsetSelection.Target.IsValid() && ensure(NewUnsetSelection.Target->UnwrapCanonical))
			{
				// These are unset, so they better not be in the Unwrap mesh
				checkSlow(!NewUnsetSelection.AreElementsPresentInMesh(*NewUnsetSelection.Target->UnwrapCanonical));
			}
		}
	}

	/*
	* Depending on whether we're calling EndChangeAndEmitIfModified or not, the comparison of selections
	* and the broadcasting of PreSelectionChange and OnSelectionChanged might be done for us. If we're not 
	* calling EndChangeAndEmitIfModified (either because bEmitChange was false, or we're inside an existing
	* transaction that we expect the user to close), then we have to do these things ourselves.
	* 
	* This is a bit messy, but it allows us to not duplicate the change emitting code from EndChangeAndEmitIfModified.
	*/
	bool bWillCallEndChange = bEmitChange && !bUsingExistingTransaction;

	bool bSelectionsDiffer = false;
	if (!bWillCallEndChange)
	{
		bSelectionsDiffer = DoSelectionSetsDiffer(CurrentUnsetSelections, NewUnsetSelections);

		if (bBroadcast && bSelectionsDiffer)
		{
			OnPreSelectionChange.Broadcast(bEmitChange, (uint32)(ESelectionChangeTypeFlag::UnsetSelectionChanged));
		}
	}

	CurrentUnsetSelections = MoveTemp(NewUnsetSelections);

	if (bWillCallEndChange)
	{
		bSelectionsDiffer = EndChangeAndEmitIfModified(bBroadcast);
	}
	else
	{
		if (bBroadcast && bSelectionsDiffer)
		{
			OnSelectionChanged.Broadcast(bEmitChange, (uint32)(ESelectionChangeTypeFlag::UnsetSelectionChanged));
		}
	}

	if (bSelectionsDiffer)
	{
		if (HighlightOptions.bAutoUpdateApplied)
		{
			RebuildAppliedPreviewHighlight();
		}
	}
}

void UTLLRN_UVToolSelectionAPI::ClearUnsetElementAppliedMeshSelections(bool bBroadcast, bool bEmitChange)
{
	SetUnsetElementAppliedMeshSelections(TArray<FUVToolSelection>(), bBroadcast, bEmitChange);
}

void UTLLRN_UVToolSelectionAPI::SetHighlightVisible(bool bUnwrapHighlightVisible, bool bAppliedHighlightVisible, bool bRebuild)
{
	HighlightMechanic->SetIsVisible(bUnwrapHighlightVisible, bAppliedHighlightVisible);
	if (bRebuild)
	{
		ClearHighlight(!bUnwrapHighlightVisible, !bAppliedHighlightVisible);
		if (bUnwrapHighlightVisible)
		{
			FTransform UnwrapTransform = FTransform::Identity;
			if (HighlightOptions.bUseCentroidForUnwrapAutoUpdate)
			{
				UnwrapTransform = FTransform(GetUnwrapSelectionBoundingBoxCenter());
			}
			RebuildUnwrapHighlight(UnwrapTransform);
		}
		if (bAppliedHighlightVisible)
		{
			RebuildAppliedPreviewHighlight();
		}
	}
}

void UTLLRN_UVToolSelectionAPI::SetHighlightOptions(const FHighlightOptions& Options)
{
	HighlightOptions = Options;

	HighlightMechanic->SetEnablePairedEdgeHighlights(Options.bShowPairedEdgeHighlights);
}

void UTLLRN_UVToolSelectionAPI::ClearHighlight(bool bClearForUnwrap, bool bClearForAppliedPreview)
{
	if (bClearForUnwrap)
	{
		HighlightMechanic->RebuildUnwrapHighlight(TArray<FUVToolSelection>(), FTransform::Identity);
	}
	if (bClearForAppliedPreview)
	{
		HighlightMechanic->RebuildAppliedHighlightFromUnwrapSelection(TArray<FUVToolSelection>());
	}
}

void UTLLRN_UVToolSelectionAPI::RebuildUnwrapHighlight(const FTransform& StartTransform)
{
	// Unfortunately even when tids and vids correspond between unwrap and canonical unwraps, eids
	// may differ, so we need to convert these over to preview unwrap eids if we're using previews.
	if (HighlightOptions.bBaseHighlightOnPreviews 
		&& GetSelectionsType() == FUVToolSelection::EType::Edge)
	{
		// TODO: We should probably have some function like "GetSelectionsInPreview(bool bForceRebuild)" 
		// that is publicly available and does caching.
		TArray<FUVToolSelection> ConvertedSelections = CurrentSelections;
		for (FUVToolSelection& ConvertedSelection : ConvertedSelections)
		{
			ConvertedSelection.SaveStableEdgeIdentifiers(*ConvertedSelection.Target->UnwrapCanonical);
			ConvertedSelection.RestoreFromStableEdgeIdentifiers(*ConvertedSelection.Target->UnwrapPreview->PreviewMesh->GetMesh());
		}
		HighlightMechanic->RebuildUnwrapHighlight(ConvertedSelections, StartTransform,
			HighlightOptions.bBaseHighlightOnPreviews);
	}
	else
	{
		HighlightMechanic->RebuildUnwrapHighlight(CurrentSelections, StartTransform,
			HighlightOptions.bBaseHighlightOnPreviews);
	}
}

void UTLLRN_UVToolSelectionAPI::SetUnwrapHighlightTransform(const FTransform& NewTransform)
{
	HighlightMechanic->SetUnwrapHighlightTransform(NewTransform, 
		HighlightOptions.bShowPairedEdgeHighlights, HighlightOptions.bBaseHighlightOnPreviews);
}

FTransform UTLLRN_UVToolSelectionAPI::GetUnwrapHighlightTransform()
{
	return HighlightMechanic->GetUnwrapHighlightTransform();
}

void UTLLRN_UVToolSelectionAPI::RebuildAppliedPreviewHighlight()
{
	// Unfortunately even when tids and vids correspond between unwrap and canonical unwraps, eids
	// may differ, so we need to convert these over to preview unwrap eids if we're using previews.
	if (HighlightOptions.bBaseHighlightOnPreviews
		&& GetSelectionsType() == FUVToolSelection::EType::Edge)
	{
		TArray<FUVToolSelection> ConvertedSelections = CurrentSelections;
		for (FUVToolSelection& ConvertedSelection : ConvertedSelections)
		{
			ConvertedSelection.SaveStableEdgeIdentifiers(*ConvertedSelection.Target->UnwrapCanonical);
			ConvertedSelection.RestoreFromStableEdgeIdentifiers(*ConvertedSelection.Target->UnwrapPreview->PreviewMesh->GetMesh());
		}
		HighlightMechanic->RebuildAppliedHighlightFromUnwrapSelection(ConvertedSelections,
			HighlightOptions.bBaseHighlightOnPreviews);
	}
	else
	{
		HighlightMechanic->RebuildAppliedHighlightFromUnwrapSelection(CurrentSelections,
			HighlightOptions.bBaseHighlightOnPreviews);
	}
	HighlightMechanic->AppendAppliedHighlight(CurrentUnsetSelections,
		HighlightOptions.bBaseHighlightOnPreviews);
}

void UTLLRN_UVToolSelectionAPI::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (SelectionMechanic)
	{
		SelectionMechanic->Render(RenderAPI);
	}
}
void UTLLRN_UVToolSelectionAPI::DrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI)
{
	if (SelectionMechanic)
	{
		SelectionMechanic->DrawHUD(Canvas, RenderAPI);
	}
}
void UTLLRN_UVToolSelectionAPI::LivePreviewRender(IToolsContextRenderAPI* RenderAPI)
{
	if (SelectionMechanic)
	{
		SelectionMechanic->LivePreviewRender(RenderAPI);
	}
}
void UTLLRN_UVToolSelectionAPI::LivePreviewDrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI)
{
	if (SelectionMechanic)
	{
		SelectionMechanic->LivePreviewDrawHUD(Canvas, RenderAPI);
	}
}

void UTLLRN_UVToolSelectionAPI::BeginChange()
{
	PendingSelectionChange = MakeUnique<FSelectionChange>();
	PendingUnsetSelectionChange = MakeUnique<FUnsetSelectionChange>();
	FSelectionChange* CastSelectionChange = static_cast<FSelectionChange*>(PendingSelectionChange.Get());
	CastSelectionChange->SetBefore(GetSelections());
	FUnsetSelectionChange* CastUnsetSelectionChange = static_cast<FUnsetSelectionChange*>(PendingUnsetSelectionChange.Get());
	CastUnsetSelectionChange->SetBefore(GetUnsetElementAppliedMeshSelections());

}

bool UTLLRN_UVToolSelectionAPI::EndChangeAndEmitIfModified(bool bBroadcast)
{
	using namespace TLLRN_UVToolSelectionAPILocals;

	if (!PendingSelectionChange && !PendingUnsetSelectionChange)
	{
		return false;
	}

	FSelectionChange* CastSelectionChange = static_cast<FSelectionChange*>(PendingSelectionChange.Get());
	FUnsetSelectionChange* CastUnsetSelectionChange = static_cast<FUnsetSelectionChange*>(PendingUnsetSelectionChange.Get());

	bool bSelectionDiffer = true;
	bool bUnsetSelectionDiffer = true;
	// See if the selection has changed
	if (!DoSelectionSetsDiffer(CastSelectionChange->GetBefore(), GetSelections()))
	{
		PendingSelectionChange.Reset();		
		bSelectionDiffer = false;
	}
	if (!DoSelectionSetsDiffer(CastUnsetSelectionChange->GetBefore(), GetUnsetElementAppliedMeshSelections()))
	{
		PendingUnsetSelectionChange.Reset();
		bUnsetSelectionDiffer = false;
	}
	if (!bSelectionDiffer && !bUnsetSelectionDiffer)
	{
		return false;
	}

	EmitChangeAPI->BeginUndoTransaction(SelectionChangeTransactionName);
	if (bBroadcast && (bSelectionDiffer || bUnsetSelectionDiffer))
	{
		OnPreSelectionChange.Broadcast(true, (uint32)((bSelectionDiffer ? ESelectionChangeTypeFlag::SelectionChanged : ESelectionChangeTypeFlag::None) |
			                                          (bUnsetSelectionDiffer ? ESelectionChangeTypeFlag::UnsetSelectionChanged : ESelectionChangeTypeFlag::None)));
	}
	if (bSelectionDiffer)
	{
		CastSelectionChange->SetAfter(GetSelections());
		EmitChangeAPI->EmitToolIndependentChange(this, MoveTemp(PendingSelectionChange),
			SelectionChangeTransactionName);
		PendingSelectionChange.Reset();
	}
	if (bUnsetSelectionDiffer)
	{
		CastUnsetSelectionChange->SetAfter(GetUnsetElementAppliedMeshSelections());
		EmitChangeAPI->EmitToolIndependentChange(this, MoveTemp(PendingUnsetSelectionChange),
			SelectionChangeTransactionName);
		PendingUnsetSelectionChange.Reset();
	}

	if (bBroadcast && (bSelectionDiffer || bUnsetSelectionDiffer))
	{
		OnSelectionChanged.Broadcast(true, (uint32)((bSelectionDiffer ? ESelectionChangeTypeFlag::SelectionChanged : ESelectionChangeTypeFlag::None) |
													(bUnsetSelectionDiffer ? ESelectionChangeTypeFlag::UnsetSelectionChanged : ESelectionChangeTypeFlag::None)));
	}
	EmitChangeAPI->EndUndoTransaction();

	return true;
}

void UTLLRN_UVToolSelectionAPI::FSelectionChange::SetBefore(TArray<FUVToolSelection> SelectionsIn)
{
	Before = MoveTemp(SelectionsIn);
	if (bUseStableUnwrapCanonicalIDsForEdges)
	{
		for (FUVToolSelection& Selection : Before)
		{
			if (Selection.Type == FUVToolSelection::EType::Edge
				&& Selection.Target.IsValid()
				&& Selection.Target->UnwrapCanonical)
			{
				Selection.SaveStableEdgeIdentifiers(*Selection.Target->UnwrapCanonical);
				Selection.SelectedIDs.Empty(); // Don't need to store since we'll always restore
			}
		}
	}
}

void UTLLRN_UVToolSelectionAPI::FSelectionChange::SetAfter(TArray<FUVToolSelection> SelectionsIn)
{
	After = MoveTemp(SelectionsIn);
	if (bUseStableUnwrapCanonicalIDsForEdges)
	{
		for (FUVToolSelection& Selection : After)
		{
			if (Selection.Type == FUVToolSelection::EType::Edge
				&& Selection.Target.IsValid()
				&& Selection.Target->UnwrapCanonical)
			{
				Selection.SaveStableEdgeIdentifiers(*Selection.Target->UnwrapCanonical);
				Selection.SelectedIDs.Empty(); // Don't need to store since we'll always restore
			}
		}
	}
}

TArray<FUVToolSelection> UTLLRN_UVToolSelectionAPI::FSelectionChange::GetBefore() const
{
	TArray<FUVToolSelection> SelectionsOut;
	for (const FUVToolSelection& Selection : Before)
	{
		if (bUseStableUnwrapCanonicalIDsForEdges
			&& Selection.Type == FUVToolSelection::EType::Edge
			&& Selection.Target.IsValid()
			&& Selection.Target->UnwrapCanonical)
		{
			FUVToolSelection UnpackedSelection = Selection;
			UnpackedSelection.RestoreFromStableEdgeIdentifiers(*Selection.Target->UnwrapCanonical);
			SelectionsOut.Add(UnpackedSelection);
		}
		else
		{
			SelectionsOut.Add(Selection);
		}
	}
	return SelectionsOut;
}

bool UTLLRN_UVToolSelectionAPI::FSelectionChange::HasExpired(UObject* Object) const
{
	UTLLRN_UVToolSelectionAPI* SelectionAPI = Cast<UTLLRN_UVToolSelectionAPI>(Object);
	return !(SelectionAPI && SelectionAPI->IsActive());
}

void UTLLRN_UVToolSelectionAPI::FSelectionChange::Apply(UObject* Object) 
{
	UTLLRN_UVToolSelectionAPI* SelectionAPI = Cast<UTLLRN_UVToolSelectionAPI>(Object);
	if (SelectionAPI)
	{
		if (bUseStableUnwrapCanonicalIDsForEdges)
		{
			for (FUVToolSelection& Selection : After)
			{
				if (Selection.Type == FUVToolSelection::EType::Edge
					&& Selection.Target.IsValid()
					&& Selection.Target->UnwrapCanonical)
				{
					Selection.RestoreFromStableEdgeIdentifiers(*Selection.Target->UnwrapCanonical);
				}
			}
		}
		SelectionAPI->SetSelections(After, true, false);
	}
}

void UTLLRN_UVToolSelectionAPI::FSelectionChange::Revert(UObject* Object)
{
	UTLLRN_UVToolSelectionAPI* SelectionAPI = Cast<UTLLRN_UVToolSelectionAPI>(Object);
	if (SelectionAPI)
	{
		if (bUseStableUnwrapCanonicalIDsForEdges)
		{
			for (FUVToolSelection& Selection : Before)
			{
				if (Selection.Type == FUVToolSelection::EType::Edge
					&& Selection.Target.IsValid()
					&& Selection.Target->UnwrapCanonical)
				{
					Selection.RestoreFromStableEdgeIdentifiers(*Selection.Target->UnwrapCanonical);
				}
			}
		}
		SelectionAPI->SetSelections(Before, true, false);
	}
}

FString UTLLRN_UVToolSelectionAPI::FSelectionChange::ToString() const
{
	return TEXT("UTLLRN_UVToolSelectionAPI::FSelectionChange");
}

void UTLLRN_UVToolSelectionAPI::FUnsetSelectionChange::SetBefore(TArray<FUVToolSelection> SelectionsIn)
{
	Before = MoveTemp(SelectionsIn);
}

void UTLLRN_UVToolSelectionAPI::FUnsetSelectionChange::SetAfter(TArray<FUVToolSelection> SelectionsIn)
{
	After = MoveTemp(SelectionsIn);
}

const TArray<FUVToolSelection>& UTLLRN_UVToolSelectionAPI::FUnsetSelectionChange::GetBefore() const
{
	return Before;
}

bool UTLLRN_UVToolSelectionAPI::FUnsetSelectionChange::HasExpired(UObject* Object) const
{
	UTLLRN_UVToolSelectionAPI* SelectionAPI = Cast<UTLLRN_UVToolSelectionAPI>(Object);
	return !(SelectionAPI && SelectionAPI->IsActive());
}

void UTLLRN_UVToolSelectionAPI::FUnsetSelectionChange::Apply(UObject* Object)
{
	UTLLRN_UVToolSelectionAPI* SelectionAPI = Cast<UTLLRN_UVToolSelectionAPI>(Object);
	if (SelectionAPI)
	{
		SelectionAPI->SetUnsetElementAppliedMeshSelections(After, true, false);
	}
}

void UTLLRN_UVToolSelectionAPI::FUnsetSelectionChange::Revert(UObject* Object)
{
	UTLLRN_UVToolSelectionAPI* SelectionAPI = Cast<UTLLRN_UVToolSelectionAPI>(Object);
	if (SelectionAPI)
	{
		SelectionAPI->SetUnsetElementAppliedMeshSelections(Before, true, false);
	}
}

FString UTLLRN_UVToolSelectionAPI::FUnsetSelectionChange::ToString() const
{
	return TEXT("UTLLRN_UVToolSelectionAPI::FUnsetSelectionChange");
}

#undef LOCTEXT_NAMESPACE