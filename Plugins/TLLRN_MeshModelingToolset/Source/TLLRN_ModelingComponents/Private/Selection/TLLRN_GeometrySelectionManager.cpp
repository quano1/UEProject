// Copyright Epic Games, Inc. All Rights Reserved.

#include "Selection/TLLRN_GeometrySelectionManager.h"
#include "CoreGlobals.h" // for GIsTransacting
#include "Engine/Engine.h"
#include "Selection/DynamicMeshSelector.h"
#include "Selection/ToolSelectionUtil.h"
#include "Selection/TLLRN_SelectionEditInteractiveCommand.h"
#include "InteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "ToolDataVisualizer.h"
#include "ToolSetupUtil.h"
#include "Drawing/TLLRN_PreviewGeometryActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Selections/GeometrySelectionUtil.h"
#include "Util/ColorConstants.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_GeometrySelectionManager)

static TAutoConsoleVariable<int32> CVarTLLRN_GeometrySelectionManager_FullSelectionHoverHighlights(
	TEXT("modeling.Selection.FullHoverHighlights"),
	1,
	TEXT("Use full selection hover highlights instead of simplified highlights")
);



using namespace UE::Geometry;


#define LOCTEXT_NAMESPACE "UTLLRN_GeometrySelectionManager"


void UTLLRN_GeometrySelectionManager::Initialize( UInteractiveToolsContext* ToolsContextIn, IToolsContextTransactionsAPI* TransactionsAPIIn )
{
	ToolsContext = ToolsContextIn;
	TransactionsAPI = TransactionsAPIIn;
	TLLRN_PreviewGeometry = NewObject<UTLLRN_PreviewGeometry>(this);
}

void UTLLRN_GeometrySelectionManager::RegisterSelectorFactory(TUniquePtr<IGeometrySelectorFactory> Factory)
{
	Factories.Add(MoveTemp(Factory));
	ResetTargetCache();
}

void UTLLRN_GeometrySelectionManager::Shutdown()
{
	DiscardSavedSelection();
	OnSelectionModified.Clear();
	ToolsContext = nullptr;
	TransactionsAPI = nullptr;

	DisconnectTLLRN_PreviewGeometry();

	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		SleepOrShutdownTarget(Target, false);
	}

	ResetTargetCache();
	ActiveTargetReferences.Reset();
	ActiveTargetMap.Reset();
	UpdateSelectionRenderCacheOnTargetChange();
}

bool UTLLRN_GeometrySelectionManager::HasBeenShutDown() const
{
	return (ToolsContext == nullptr);
}


class FTLLRN_GeometrySelectionManager_SelectionTypeChange : public FToolCommandChange
{
public:
	EGeometryElementType FromElementType = EGeometryElementType::Face;
	EGeometryElementType ToElementType = EGeometryElementType::Face;

	UTLLRN_GeometrySelectionManager::EMeshTopologyMode FromTopologyMode = UTLLRN_GeometrySelectionManager::EMeshTopologyMode::None;
	UTLLRN_GeometrySelectionManager::EMeshTopologyMode ToTopologyMode = UTLLRN_GeometrySelectionManager::EMeshTopologyMode::None;

	/** Makes the change to the object */
	virtual void Apply(UObject* Object) override
	{
		// do the (default) red selectable lines/verts need to be rebuilt?
		// - ex: when moving from object mode to tri/vert/edge mode, or moving between vert and edge/face mode,
		//		 or between triangle and polygroup topology
		bool bRebuildSelectable = false;

		UTLLRN_GeometrySelectionManager* GeoSelectionManager = CastChecked<UTLLRN_GeometrySelectionManager>(Object);

		// removes existing Line/Point/Triangle Sets when moving between vertex and face/edge modes during redo
		if (
			(((ToElementType == EGeometryElementType::Vertex) && (FromElementType == EGeometryElementType::Edge || FromElementType == EGeometryElementType::Face))
			|| ((ToElementType == EGeometryElementType::Edge || ToElementType == EGeometryElementType::Face) && (FromElementType == EGeometryElementType::Vertex)))
			&& (ToTopologyMode != UTLLRN_GeometrySelectionManager::EMeshTopologyMode::None)
		)
		{
			GeoSelectionManager->RemoveAllSets();
			bRebuildSelectable = true;
		}

		if (FromTopologyMode != ToTopologyMode)
		{
			// when changing to Object mode, lines or verts need to be cleared
			if (ToTopologyMode == UTLLRN_GeometrySelectionManager::EMeshTopologyMode::None)
			{
				GeoSelectionManager->RemoveAllSets();
			}
			// in all other cases of changing topology modes, the lines/verts need to be rebuilt
			// uses a flag to preserve order of Removing Sets->Setting Element Type/Topo Mode -> Rebuild (when applicable for each step)
			else
			{
				bRebuildSelectable = true;
			}
			GeoSelectionManager->SetMeshTopologyModeInternal(ToTopologyMode);
		}
		
		if (FromElementType != ToElementType)
		{
			GeoSelectionManager->SetSelectionElementTypeInternal(ToElementType);
		}

		// if applicable, rebuilds lines/verts
		if (bRebuildSelectable)
		{
			GeoSelectionManager->RebuildSelectable();
		}
				
	}

	/** Reverts change to the object */
	virtual void Revert(UObject* Object) override
	{
		// do the (default) red selectable lines/verts need to be rebuilt?
		// - ex: when moving from object mode to tri/vert/edge mode, or moving between vert and edge/face mode
		//		 or between triangle and polygroup topology
		bool bRebuildSelectable = false;

		UTLLRN_GeometrySelectionManager* GeoSelectionManager = CastChecked<UTLLRN_GeometrySelectionManager>(Object);

		// removes existing Line/Point/Triangle Sets when moving between vertex and face/edge modes during undo
		if (
			(((ToElementType == EGeometryElementType::Vertex) && (FromElementType == EGeometryElementType::Edge || FromElementType == EGeometryElementType::Face))
			|| ((ToElementType == EGeometryElementType::Edge || ToElementType == EGeometryElementType::Face) && (FromElementType == EGeometryElementType::Vertex)))
			&& (FromTopologyMode != UTLLRN_GeometrySelectionManager::EMeshTopologyMode::None)
		)
		{
			GeoSelectionManager->RemoveAllSets();
			bRebuildSelectable = true;
			
		}

		if (FromTopologyMode != ToTopologyMode)
		{
			// when changing to Object mode, lines or verts need to be cleared
			if (FromTopologyMode == UTLLRN_GeometrySelectionManager::EMeshTopologyMode::None)
			{
				GeoSelectionManager->RemoveAllSets();
			}
			// in all other cases of changing topology modes, the lines/verts need to be rebuilt
			// uses a flag to preserve order of Removing Sets->Setting Element Type/Topo Mode -> Rebuild (when applicable for each step)
			else
			{
				bRebuildSelectable = true;
			}
			GeoSelectionManager->SetMeshTopologyModeInternal(FromTopologyMode);
		}
		
		if (FromElementType != ToElementType)
		{
			GeoSelectionManager->SetSelectionElementTypeInternal(FromElementType);
		}

		// if applicable, rebuilds lines/verts
		if (bRebuildSelectable)
		{
			GeoSelectionManager->RebuildSelectable();
		}
	}

	/** Describes this change (for debugging) */
	virtual FString ToString() const override { return TEXT("FTLLRN_GeometrySelectionManager_SelectionTypeChange"); }

	virtual bool HasExpired(UObject* Object) const override
	{
		UTLLRN_GeometrySelectionManager* Manager = Cast<UTLLRN_GeometrySelectionManager>(Object);
		return (Manager == nullptr || IsValid(Manager) == false || Manager->HasBeenShutDown());
	}
};



void UTLLRN_GeometrySelectionManager::SetSelectionElementTypeInternal(EGeometryElementType NewElementType)
{
	if (SelectionElementType != NewElementType)
	{
		SelectionElementType = NewElementType;

		for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
		{
			Target->Selection.ElementType = SelectionElementType;
			bool bEnableTopologyFilter = (Target->Selection.TopologyType == EGeometryTopologyType::Polygroup && Target->Selection.ElementType != EGeometryElementType::Vertex);
			Target->SelectionEditor->UpdateQueryConfig(GetCurrentSelectionQueryConfig(), bEnableTopologyFilter);
		}

		MarkRenderCachesDirty(false);
		ClearActivePreview();
	}
}


void UTLLRN_GeometrySelectionManager::SetSelectionElementType(EGeometryElementType NewElementType)
{
	if (SelectionElementType != NewElementType)
	{
		GetTransactionsAPI()->BeginUndoTransaction(LOCTEXT("ChangeElementType", "Selection Type"));

		if (HasSelection())
		{
			ClearSelection();
		}

		// We have to undo/redo the change to the selection type because if we want to 'undo' this later and restore
		// the current selection, we need the active element type to be correct. Note that it goes *after* the Clear
		// so that when we undo, we change to the correct type before we restore
		TUniquePtr<FTLLRN_GeometrySelectionManager_SelectionTypeChange> TypeChange = MakeUnique<FTLLRN_GeometrySelectionManager_SelectionTypeChange>();
		TypeChange->FromElementType = SelectionElementType;
		TypeChange->ToElementType = NewElementType;
		TypeChange->FromTopologyMode = TypeChange->ToTopologyMode = MeshTopologyMode;		// no-op
		GetTransactionsAPI()->AppendChange(this, MoveTemp(TypeChange), LOCTEXT("ChangeElementType", "Selection Type"));

		SetSelectionElementTypeInternal(NewElementType);

		GetTransactionsAPI()->EndUndoTransaction();
	}
}




void UTLLRN_GeometrySelectionManager::SetMeshTopologyModeInternal(EMeshTopologyMode NewTopologyMode)
{
	if (MeshTopologyMode != NewTopologyMode)
	{
		MeshTopologyMode = NewTopologyMode;

		for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
		{
			Target->Selection.TopologyType = GetSelectionTopologyType();
			bool bEnableTopologyFilter = (Target->Selection.TopologyType == EGeometryTopologyType::Polygroup && Target->Selection.ElementType != EGeometryElementType::Vertex);
			Target->SelectionEditor->UpdateQueryConfig(GetCurrentSelectionQueryConfig(), bEnableTopologyFilter);
		}

		MarkRenderCachesDirty();
		ClearActivePreview();
	}
}


void UTLLRN_GeometrySelectionManager::SetMeshTopologyMode(EMeshTopologyMode NewTopologyMode)
{
	if (MeshTopologyMode != NewTopologyMode)
	{
		GetTransactionsAPI()->BeginUndoTransaction(LOCTEXT("ChangeSelectionMode", "Selection Mode"));

		if (HasSelection())
		{
			ClearSelection();
		}

		// We have to undo/redo the change to the selection type because if we want to 'undo' this later and restore
		// the current selection, we need the active element type to be correct. Note that it goes *after* the Clear
		// so that when we undo, we change to the correct type before we restore
		TUniquePtr<FTLLRN_GeometrySelectionManager_SelectionTypeChange> TypeChange = MakeUnique<FTLLRN_GeometrySelectionManager_SelectionTypeChange>();
		TypeChange->FromTopologyMode = MeshTopologyMode;
		TypeChange->ToTopologyMode = NewTopologyMode;
		TypeChange->FromElementType = TypeChange->ToElementType = SelectionElementType;		// no-op
		GetTransactionsAPI()->AppendChange(this, MoveTemp(TypeChange),LOCTEXT("ChangeSelectionMode", "Selection Mode"));

		SetMeshTopologyModeInternal(NewTopologyMode);

		GetTransactionsAPI()->EndUndoTransaction();
	}
}

void UTLLRN_GeometrySelectionManager::RebuildSelectable() const
{
	for (int32 k = 0; k < ActiveTargetReferences.Num(); ++k)
	{
		CreateOrUpdateAllSets(CachedSelectableRenderElements[k], UnselectedParams);
	}
}


void UTLLRN_GeometrySelectionManager::SetMeshSelectionTypeAndMode(EGeometryElementType NewElementType, EMeshTopologyMode NewTopologyMode, bool bConvertSelection)
{
	if (MeshTopologyMode != NewTopologyMode || SelectionElementType != NewElementType)
	{
		bool bHasSelection = HasSelection();

		// If we're converting selections, save the old one; we will re-add it after changing the mode
		TArray<FGeometrySelection> OldTypeSelections;
		if (bHasSelection && bConvertSelection)
		{
			for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
			{
				OldTypeSelections.Add(Target->Selection);
			}
		}

		ClearSelection();

		// clear preview geometry sets when in Object selection mode
		if (NewTopologyMode == EMeshTopologyMode::None && TLLRN_PreviewGeometry)
		{
			RemoveAllSets();
		}

		// removes existing Line/Point/Triangle Sets when moving between vertex and face/edge modes
		bool bRebuildSelectable = false;
		if (
			(((NewElementType == EGeometryElementType::Vertex) && (SelectionElementType == EGeometryElementType::Edge || SelectionElementType == EGeometryElementType::Face))
			|| ((NewElementType == EGeometryElementType::Edge || NewElementType == EGeometryElementType::Face) && (SelectionElementType == EGeometryElementType::Vertex)))
			&& (NewTopologyMode != EMeshTopologyMode::None)
		) // ensure lines not rebuilt when changing to object mode
		{
			RemoveAllSets();
			bRebuildSelectable = true;
		}

		GetTransactionsAPI()->BeginUndoTransaction(LOCTEXT("ChangeElementMethod", "Change Selection Method"));

		// We have to undo/redo the change to the selection type because if we want to 'undo' this later and restore
		// the current selection, we need the active element type to be correct. Note that it goes *after* the Clear
		// so that when we undo, we change to the correct type before we restore
		TUniquePtr<FTLLRN_GeometrySelectionManager_SelectionTypeChange> TypeChange = MakeUnique<FTLLRN_GeometrySelectionManager_SelectionTypeChange>();
		TypeChange->FromElementType = SelectionElementType;
		TypeChange->ToElementType = NewElementType;
		TypeChange->FromTopologyMode = MeshTopologyMode;
		TypeChange->ToTopologyMode = NewTopologyMode;
		GetTransactionsAPI()->AppendChange(this, MoveTemp(TypeChange), LOCTEXT("ChangeElementMethod", "Change Selection Method"));

		SetSelectionElementTypeInternal(NewElementType);
		SetMeshTopologyModeInternal(NewTopologyMode);

		if (bRebuildSelectable)
		{
			RebuildSelectable();
		}

		if (bHasSelection && bConvertSelection && ensure(ActiveTargetReferences.Num() == OldTypeSelections.Num()))
		{
			for (int32 TargetIdx = 0; TargetIdx < ActiveTargetReferences.Num(); ++TargetIdx)
			{
				// Add back the old selection, converted to the new mode/type
				TSharedPtr<FGeometrySelectionTarget> Target = ActiveTargetReferences[TargetIdx];
				FGeometrySelection InitialSelection = Target->Selection;
				FGeometrySelectionDelta AfterDelta;
				Target->Selector->UpdateSelectionFromSelection(OldTypeSelections[TargetIdx], true, *Target->SelectionEditor, FGeometrySelectionUpdateConfig{EGeometrySelectionChangeType::Replace}, &AfterDelta);
				if (!AfterDelta.IsEmpty())
				{
					TUniquePtr<FGeometrySelectionReplaceChange> NewSelectionChange = MakeUnique<FGeometrySelectionReplaceChange>();
					NewSelectionChange->Identifier = Target->TargetIdentifier;
					NewSelectionChange->After = Target->Selection;
					NewSelectionChange->Before = InitialSelection;
					GetTransactionsAPI()->AppendChange(this, MoveTemp(NewSelectionChange), LOCTEXT("ConvertSelection", "Convert Selection"));
				}
			}
		}

		GetTransactionsAPI()->EndUndoTransaction();

		OnSelectionModified.Broadcast();
	}
}


EGeometryTopologyType UTLLRN_GeometrySelectionManager::GetSelectionTopologyType() const
{
	if (MeshTopologyMode == EMeshTopologyMode::Polygroup)
	{
		return EGeometryTopologyType::Polygroup;
	}
	else
	{
		return EGeometryTopologyType::Triangle;
	}
}



UE::Geometry::FGeometrySelectionHitQueryConfig UTLLRN_GeometrySelectionManager::GetCurrentSelectionQueryConfig() const
{
	FGeometrySelectionHitQueryConfig HitQueryConfig;
	HitQueryConfig.TopologyType = GetSelectionTopologyType();
	HitQueryConfig.ElementType = GetSelectionElementType();
	HitQueryConfig.bOnlyVisible = true;
	return HitQueryConfig;
}


bool UTLLRN_GeometrySelectionManager::HasSelection() const
{
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		if (Target->Selection.IsEmpty() == false)
		{
			return true;
		}
	}
	return false;
}


void UTLLRN_GeometrySelectionManager::GetActiveSelectionInfo(EGeometryTopologyType& TopologyTypeOut, EGeometryElementType& ElementTypeOut, int& NumTargetsOut, bool& bIsEmpty) const
{
	FGeometrySelectionHitQueryConfig Config = GetCurrentSelectionQueryConfig();
	TopologyTypeOut = Config.TopologyType;
	ElementTypeOut = Config.ElementType;
	NumTargetsOut = ActiveTargetReferences.Num();
	bIsEmpty = (NumTargetsOut == 0) || ActiveTargetReferences[0]->Selection.IsEmpty();
}


class FTLLRN_GeometrySelectionManager_ActiveTargetsChange : public FToolCommandChange
{
public:
	TArray<FGeometryIdentifier> TargetsBefore;
	TArray<FGeometryIdentifier> TargetsAfter;

	virtual void Apply(UObject* Object) override
	{
		CastChecked<UTLLRN_GeometrySelectionManager>(Object)->SetTargetsOnUndoRedo(TargetsAfter);
	}

	virtual void Revert(UObject* Object) override
	{
		CastChecked<UTLLRN_GeometrySelectionManager>(Object)->SetTargetsOnUndoRedo(TargetsBefore);
	}

	virtual FString ToString() const override { return TEXT("FTLLRN_GeometrySelectionManager_ActiveTargetsChange"); }

	virtual bool HasExpired(UObject* Object) const override
	{
		UTLLRN_GeometrySelectionManager* Manager = Cast<UTLLRN_GeometrySelectionManager>(Object);
		return (Manager == nullptr || IsValid(Manager) == false || Manager->HasBeenShutDown());
	}
};



class FTLLRN_GeometrySelectionManager_TargetLockStateChange : public FToolCommandChange
{
public:
	FGeometryIdentifier TargetIdentifier;
	bool bToState;

	virtual void Apply(UObject* Object) override
	{
		CastChecked<UTLLRN_GeometrySelectionManager>(Object)->SetTargetLockStateOnUndoRedo(TargetIdentifier, bToState);
	}

	virtual void Revert(UObject* Object) override
	{
		CastChecked<UTLLRN_GeometrySelectionManager>(Object)->SetTargetLockStateOnUndoRedo(TargetIdentifier, !bToState);
	}

	virtual FString ToString() const override { return TEXT("FTLLRN_GeometrySelectionManager_TargetLockStateChange"); }

	virtual bool HasExpired(UObject* Object) const override
	{
		UTLLRN_GeometrySelectionManager* Manager = Cast<UTLLRN_GeometrySelectionManager>(Object);
		return (Manager == nullptr || IsValid(Manager) == false || Manager->HasBeenShutDown());
	}
};



bool UTLLRN_GeometrySelectionManager::HasActiveTargets() const
{
	return (ActiveTargetReferences.Num() > 0);
}

bool UTLLRN_GeometrySelectionManager::ValidateSelectionState() const
{
	for (const TSharedPtr<FGeometrySelectionTarget>& Target : ActiveTargetReferences)
	{
		if (!Target.IsValid())
		{
			return false;
		}
		// if we have a stale target/selection object, selection state is not valid
		// Note: it is ok for the object to be explicitly null, just not stale
		if (Target->SelectionIdentifer.TargetObject.IsStale())
		{
			return false;
		}
		if (Target->TargetIdentifier.TargetObject.IsStale())
		{
			return false;
		}
	}
	return true;
}

void UTLLRN_GeometrySelectionManager::ClearActiveTargets()
{
	// generally at this point is it too late to clear the selection, because it will emit an
	// undo that cannot be redone later, because on redo the Targets will not exist yet
	// (one possibility would be to emit separate changes for when the target set is modified?? would that work w/ delete?? )
	ensure(HasSelection() == false);
	DiscardSavedSelection();

	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		SleepOrShutdownTarget(Target, false);
	}

	ActiveTargetReferences.Reset();
	ActiveTargetMap.Reset();

	UpdateSelectionRenderCacheOnTargetChange();

	OnSelectionModified.Broadcast();
}

bool UTLLRN_GeometrySelectionManager::AddActiveTarget(FGeometryIdentifier TargetIdentifier)
{
	if (ActiveTargetMap.Contains(TargetIdentifier))
	{
		return false;
	}

	// need to have a selector factory that can build for this target
	const IGeometrySelectorFactory* UseFactory = nullptr;
	for (const TUniquePtr<IGeometrySelectorFactory>& Factory : Factories)
	{
		if (Factory->CanBuildForTarget(TargetIdentifier))
		{
			UseFactory = Factory.Get();
			break;
		}
	}
	if (UseFactory == nullptr)
	{
		return false;
	}

	TSharedPtr<FGeometrySelectionTarget> SelectionTarget = GetCachedTarget(TargetIdentifier, UseFactory);
	if (SelectionTarget.IsValid() == false)
	{
		return false;
	}

	ActiveTargetMap.Add(TargetIdentifier, SelectionTarget);
	ActiveTargetReferences.Add(SelectionTarget);
	
	SelectionTarget->OnGeometryModifiedHandle = 
		SelectionTarget->Selector->GetOnGeometryModifed().AddUObject(this, &UTLLRN_GeometrySelectionManager::OnTargetGeometryModified);

	UpdateSelectionRenderCacheOnTargetChange();

	return true;
}


void UTLLRN_GeometrySelectionManager::SynchronizeActiveTargets(
	const TArray<FGeometryIdentifier>& DesiredActiveSet,
	TFunctionRef<void()> WillChangeActiveTargetsCallback)
{
	TArray<FGeometryIdentifier> Before = GetCurrentTargetIdentifiers();
	
	// currently only support single selection
	if (DesiredActiveSet.Num() == 1)
	{
		// if we do not already have this target, select it
		if ( ActiveTargetMap.Contains(DesiredActiveSet[0]) == false )
		{
			WillChangeActiveTargetsCallback();
			ClearActiveTargets();
			AddActiveTarget(DesiredActiveSet[0]);
		}
	}
	else
	{
		WillChangeActiveTargetsCallback();
		ClearActiveTargets();
	}

	TArray<FGeometryIdentifier> After = GetCurrentTargetIdentifiers();
	if (Before != After)
	{
		TUniquePtr<FTLLRN_GeometrySelectionManager_ActiveTargetsChange> Change = MakeUnique<FTLLRN_GeometrySelectionManager_ActiveTargetsChange>();
		Change->TargetsBefore = Before;
		Change->TargetsAfter = After;
		GetTransactionsAPI()->AppendChange(this, MoveTemp(Change), LOCTEXT("Change Targets", "Change Targets"));
	}
}


bool UTLLRN_GeometrySelectionManager::GetAnyCurrentTargetsLockable() const
{
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		if (Target->Selector->IsLockable())
		{
			return true;
		}
	}
	return false;
}

bool UTLLRN_GeometrySelectionManager::GetAnyCurrentTargetsLocked() const
{
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		if (Target->Selector->IsLockable() && Target->Selector->IsLocked())
		{
			return true;
		}
	}
	return false;
}

void UTLLRN_GeometrySelectionManager::SetCurrentTargetsLockState(bool bLocked)
{
	bool bInTransaction = false;

	bool bLockStateModified = false;
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		if (Target->Selector->IsLockable() && Target->Selector->IsLocked() != bLocked)
		{
			Target->Selector->SetLockedState(bLocked);
			bLockStateModified = true;

			if (!bInTransaction)
			{
				GetTransactionsAPI()->BeginUndoTransaction(
					(bLocked) ? LOCTEXT("Lock Target", "Lock Target") : LOCTEXT("Unlock Target", "Unlock Target"));
				bInTransaction = true;
			}

			TUniquePtr<FTLLRN_GeometrySelectionManager_TargetLockStateChange> Change = MakeUnique<FTLLRN_GeometrySelectionManager_TargetLockStateChange>();
			Change->TargetIdentifier = Target->TargetIdentifier;
			Change->bToState = bLocked;
			GetTransactionsAPI()->AppendChange(this, MoveTemp(Change), 
				(bLocked) ? LOCTEXT("Lock Target", "Lock Target") : LOCTEXT("Unlock Target", "Unlock Target"));
		}
	}

	if (bLockStateModified)
	{
		ClearSelection();
	}

	if (bInTransaction)
	{
		GetTransactionsAPI()->EndUndoTransaction();
	}
}

void UTLLRN_GeometrySelectionManager::SetTargetLockStateOnUndoRedo(FGeometryIdentifier TargetIdentifier, bool bLocked)
{
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		if (Target->TargetIdentifier == TargetIdentifier)
		{
			Target->Selector->SetLockedState(bLocked);
		}
	}
}


TArray<FGeometryIdentifier> UTLLRN_GeometrySelectionManager::GetCurrentTargetIdentifiers() const
{
	TArray<FGeometryIdentifier> Result;
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		Result.Add(Target->TargetIdentifier);
	}
	return Result;
}

void UTLLRN_GeometrySelectionManager::SetTargetsOnUndoRedo(TArray<FGeometryIdentifier> NewTargets)
{
	ClearActiveTargets();
	for (FGeometryIdentifier Identifier : NewTargets)
	{
		AddActiveTarget(Identifier);
	}
}



void UTLLRN_GeometrySelectionManager::SleepOrShutdownTarget(TSharedPtr<FGeometrySelectionTarget> Target, bool bForceShutdown)
{
	if (Target->Selector->SupportsSleep() && bForceShutdown == false)
	{
		if (Target->Selector->Sleep())
		{
			return;
		}
	}

	// if target cannot sleep or if sleeping failed, make sure it is not in the target
	// cache so that we do not try to restore it later
	TargetCache.Remove(Target->TargetIdentifier);

	Target->Selector->GetOnGeometryModifed().Remove(Target->OnGeometryModifiedHandle);
	Target->Selector->Shutdown();
}

TSharedPtr<UTLLRN_GeometrySelectionManager::FGeometrySelectionTarget> UTLLRN_GeometrySelectionManager::GetCachedTarget(FGeometryIdentifier TargetIdentifier, const IGeometrySelectorFactory* UseFactory)
{
	if (TargetCache.Contains(TargetIdentifier))
	{
		TSharedPtr<FGeometrySelectionTarget> FoundTarget = TargetCache[TargetIdentifier];
		FoundTarget->Selection.Reset();
		bool bRestored = FoundTarget->Selector->Restore();
		if (bRestored)
		{
			// ensure these are current, as they may have changed while Target was asleep
			FoundTarget->Selection.ElementType = GetSelectionElementType();
			FoundTarget->Selection.TopologyType = GetSelectionTopologyType();
			bool bEnableTopologyFilter = (FoundTarget->Selection.TopologyType == EGeometryTopologyType::Polygroup && FoundTarget->Selection.ElementType != EGeometryElementType::Vertex);
			FoundTarget->SelectionEditor->UpdateQueryConfig(GetCurrentSelectionQueryConfig(), bEnableTopologyFilter);

			return FoundTarget;
		}
		else
		{
			// if restore failed, something is wrong w/ TargetCache, remove this Target
			TargetCache.Remove(TargetIdentifier);
		}
	}

	// if we are in a situation where we don't have a cache, currently we need the Factory to exist?
	if (UseFactory == nullptr)
	{
		return nullptr;
	}

	// selector has to be built properly
	TUniquePtr<IGeometrySelector> Selector = UseFactory->BuildForTarget(TargetIdentifier);
	if (Selector.IsValid() == false)
	{
		return nullptr;
	}

	TSharedPtr<FGeometrySelectionTarget> SelectionTarget = MakeShared<FGeometrySelectionTarget>();
	SelectionTarget->Selector = MoveTemp(Selector);
	SelectionTarget->TargetIdentifier = TargetIdentifier;
	SelectionTarget->SelectionIdentifer = SelectionTarget->Selector->GetIdentifier();
	SelectionTarget->Selection.ElementType = GetSelectionElementType();
	SelectionTarget->Selection.TopologyType = GetSelectionTopologyType();

	SelectionTarget->SelectionEditor = MakeUnique<FGeometrySelectionEditor>();
	FGeometrySelectionHitQueryConfig HitQueryConfig = GetCurrentSelectionQueryConfig();
	bool bEnableTopologyFilter = (HitQueryConfig.TopologyType == EGeometryTopologyType::Polygroup && HitQueryConfig.ElementType != EGeometryElementType::Vertex);
	SelectionTarget->SelectionEditor->Initialize(&SelectionTarget->Selection, HitQueryConfig, bEnableTopologyFilter);

	if (SelectionTarget->Selector->SupportsSleep())
	{
		TargetCache.Add(TargetIdentifier, SelectionTarget);
	}
	
	return SelectionTarget;
}

void UTLLRN_GeometrySelectionManager::ResetTargetCache()
{
	// SleepOrShutdownTarget may modify TargetCache
	TArray<TSharedPtr<FGeometrySelectionTarget>> ToShutdown;
	TargetCache.GenerateValueArray(ToShutdown);
	for (TSharedPtr<FGeometrySelectionTarget> Target : ToShutdown)
	{
		SleepOrShutdownTarget(Target, true);
	}
	TargetCache.Reset();
}



bool UTLLRN_GeometrySelectionManager::RayHitTest(
	const FRay3d& WorldRay,
	FInputRayHit& HitResultOut )
{
	HitResultOut = FInputRayHit();
	if (ActiveTargetReferences.Num() == 0)
	{
		return false;
	}

	IGeometrySelector::FWorldRayQueryInfo RayQueryInfo;
	RayQueryInfo.WorldRay = WorldRay;
	IToolsContextQueriesAPI* QueryAPI = this->ToolsContext->ToolManager->GetContextQueriesAPI();
	QueryAPI->GetCurrentViewState(RayQueryInfo.CameraState);

	// currently only going to support one object, not sure how to support more yet...
	FGeometrySelectionTarget* Target = ActiveTargetReferences[0].Get();

	FGeometrySelectionHitQueryConfig HitQueryConfig = GetCurrentSelectionQueryConfig();
	bool bHit = Target->Selector->RayHitTest(RayQueryInfo, HitQueryConfig, HitResultOut);
	if (bHit)
	{
		HitResultOut.HitOwner = ActiveTargetReferences[0].Get();
		HitResultOut.HitObject = (ActiveTargetReferences[0]->TargetIdentifier.TargetType == FGeometryIdentifier::ETargetType::PrimitiveComponent) ?
			ActiveTargetReferences[0]->TargetIdentifier.TargetObject : nullptr;
	}

	// currently only going to support one object, not sure how to support more yet...
	return bHit;
}


void UTLLRN_GeometrySelectionManager::ClearSelection(bool bSaveSelectionBeforeClear)
{
	if (!HasSelection())
	{
		return;
	}

	if (bSaveSelectionBeforeClear)
	{
		SaveCurrentSelection();
	}

	GetTransactionsAPI()->BeginUndoTransaction(LOCTEXT("ClearSelection", "Clear Selection"));

	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		FGeometrySelectionDelta ClearDelta;
		Target->SelectionEditor->ClearSelection(ClearDelta);
		if (ClearDelta.IsEmpty() == false)
		{
			TUniquePtr<FGeometrySelectionDeltaChange> ClearChange = MakeUnique<FGeometrySelectionDeltaChange>();
			ClearChange->Identifier = Target->TargetIdentifier;
			ClearChange->Delta = MoveTemp(ClearDelta);
			GetTransactionsAPI()->AppendChange(this, MoveTemp(ClearChange), LOCTEXT("ClearSelection", "Clear Selection"));
		}
	}

	GetTransactionsAPI()->EndUndoTransaction();

	MarkRenderCachesDirty(false);
	OnSelectionModified.Broadcast();
}


void UTLLRN_GeometrySelectionManager::UpdateSelectionViaRaycast(
	const FRay3d& WorldRay,
	const FGeometrySelectionUpdateConfig& UpdateConfig,
	FGeometrySelectionUpdateResult& ResultOut
)
{
	ResultOut.bSelectionModified = false;

	if (ActiveTargetReferences.Num() == 0)
	{
		return;
	}

	// currently only going to support one object, not sure how to support more yet...
	FGeometrySelectionTarget* Target = ActiveTargetReferences[0].Get();

	IGeometrySelector::FWorldRayQueryInfo RayQueryInfo;
	RayQueryInfo.WorldRay = WorldRay;
	IToolsContextQueriesAPI* QueryAPI = this->ToolsContext->ToolManager->GetContextQueriesAPI();
	QueryAPI->GetCurrentViewState(RayQueryInfo.CameraState);

	Target->Selector->UpdateSelectionViaRaycast(
		RayQueryInfo, *Target->SelectionEditor, UpdateConfig, ResultOut
	);

	if (ResultOut.bSelectionModified)
	{
		TUniquePtr<FGeometrySelectionDeltaChange> DeltaChange = MakeUnique<FGeometrySelectionDeltaChange>();
		DeltaChange->Identifier = Target->TargetIdentifier;
		DeltaChange->Delta = ResultOut.SelectionDelta;

		GetTransactionsAPI()->BeginUndoTransaction(LOCTEXT("UpdateSelectionViaRaycast", "Change Selection"));
		GetTransactionsAPI()->AppendChange(this, MoveTemp(DeltaChange), LOCTEXT("UpdateSelectionViaRaycast", "Change Selection"));
		GetTransactionsAPI()->EndUndoTransaction();

		MarkRenderCachesDirty(false);
		OnSelectionModified.Broadcast();
	}
	else if (ResultOut.bSelectionMissed && UpdateConfig.ChangeType == EGeometrySelectionChangeType::Replace)
	{
		ClearSelection();
	}
}


void UTLLRN_GeometrySelectionManager::UpdateSelectionViaConvex(
	const FConvexVolume& ConvexVolume,
	const FGeometrySelectionUpdateConfig& UpdateConfig,
	FGeometrySelectionUpdateResult& ResultOut )
{
	ResultOut.bSelectionModified = false;

	if (ActiveTargetReferences.Num() == 0)
	{
		return;
	}

	// currently only going to support one object, not sure how to support more yet...
	FGeometrySelectionTarget* Target = ActiveTargetReferences[0].Get();

	IGeometrySelector::FWorldShapeQueryInfo ShapeQueryInfo;
	ShapeQueryInfo.Convex = ConvexVolume;
	IToolsContextQueriesAPI* QueryAPI = this->ToolsContext->ToolManager->GetContextQueriesAPI();
	QueryAPI->GetCurrentViewState(ShapeQueryInfo.CameraState);

	Target->Selector->UpdateSelectionViaShape(
		ShapeQueryInfo, *Target->SelectionEditor, UpdateConfig, ResultOut );

	if (ResultOut.bSelectionModified)
	{
		TUniquePtr<FGeometrySelectionDeltaChange> DeltaChange = MakeUnique<FGeometrySelectionDeltaChange>();
		DeltaChange->Identifier = Target->TargetIdentifier;
		DeltaChange->Delta = ResultOut.SelectionDelta;

		GetTransactionsAPI()->BeginUndoTransaction(LOCTEXT("UpdateSelectionViaConvex", "Change Selection"));
		GetTransactionsAPI()->AppendChange(this, MoveTemp(DeltaChange), LOCTEXT("UpdateSelectionViaConvex", "Change Selection"));
		GetTransactionsAPI()->EndUndoTransaction();

		MarkRenderCachesDirty(false);
		OnSelectionModified.Broadcast();
	}
	else if (ResultOut.bSelectionMissed && UpdateConfig.ChangeType == EGeometrySelectionChangeType::Replace)
	{
		ClearSelection();
	}
}



bool UTLLRN_GeometrySelectionManager::CanBeginTrackedSelectionChange() const
{
	return ActiveTargetReferences.Num() > 0 && bInTrackedSelectionChange == false;
}

bool UTLLRN_GeometrySelectionManager::BeginTrackedSelectionChange(FGeometrySelectionUpdateConfig UpdateConfig, bool bClearOnBegin)
{
	if (ensureMsgf(CanBeginTrackedSelectionChange(), TEXT("Cannot begin Selection Change - validate CanBeginTrackedSelectionChange() before calling BeginTrackedSelectionChange()")) == false)
	{
		return false;
	}

	GetTransactionsAPI()->BeginUndoTransaction(LOCTEXT("ChangeSelection", "Change Selection"));
	bInTrackedSelectionChange = true;

	// currently only going to support one object, not sure how to support more yet...
	FGeometrySelectionTarget* Target = ActiveTargetReferences[0].Get();

	ActiveTrackedUpdateConfig = UpdateConfig;
	bSelectionModifiedDuringTrackedChange = false;

	// if we are doing a Replace selection, we want to clear on initialization...
	InitialTrackedDelta = FGeometrySelectionDelta();
	if (bClearOnBegin)
	{
		Target->SelectionEditor->ClearSelection(InitialTrackedDelta);
		bSelectionModifiedDuringTrackedChange = true;
	}

	ActiveTrackedSelection = Target->Selection;
	ActiveTrackedDelta = FGeometrySelectionDelta();

	if (bClearOnBegin && InitialTrackedDelta.IsEmpty() == false)
	{
		MarkRenderCachesDirty(false);
		OnSelectionModified.Broadcast();
	}

	return true;
}

void UTLLRN_GeometrySelectionManager::AccumulateSelectionUpdate_Raycast(
	const FRay3d& WorldRay,
	FGeometrySelectionUpdateResult& ResultOut )
{
	if (!ensure(bInTrackedSelectionChange)) return;

	// currently only going to support one object, not sure how to support more yet...
	FGeometrySelectionTarget* Target = ActiveTargetReferences[0].Get();

	IGeometrySelector::FWorldRayQueryInfo RayQueryInfo;
	RayQueryInfo.WorldRay = WorldRay;
	IToolsContextQueriesAPI* QueryAPI = this->ToolsContext->ToolManager->GetContextQueriesAPI();
	QueryAPI->GetCurrentViewState(RayQueryInfo.CameraState);

	Target->Selector->UpdateSelectionViaRaycast(
		RayQueryInfo, *Target->SelectionEditor, ActiveTrackedUpdateConfig, ResultOut );

	if (ResultOut.bSelectionModified)
	{
		bSelectionModifiedDuringTrackedChange = true;
		ActiveTrackedDelta.Added.Append( ResultOut.SelectionDelta.Added );
		ActiveTrackedDelta.Removed.Append( ResultOut.SelectionDelta.Removed );

		MarkRenderCachesDirty(false);
		OnSelectionModified.Broadcast();
	}
}

void UTLLRN_GeometrySelectionManager::EndTrackedSelectionChange()
{
	if ( ensure(bInTrackedSelectionChange) )
	{
		if (bSelectionModifiedDuringTrackedChange)
		{
			FGeometrySelectionTarget* Target = ActiveTargetReferences[0].Get();

			if (InitialTrackedDelta.IsEmpty() == false)
			{
				TUniquePtr<FGeometrySelectionDeltaChange> InitialDeltaChange = MakeUnique<FGeometrySelectionDeltaChange>();
				InitialDeltaChange->Identifier = Target->TargetIdentifier;
				InitialDeltaChange->Delta = MoveTemp(InitialTrackedDelta);
				GetTransactionsAPI()->AppendChange(this, MoveTemp(InitialDeltaChange), LOCTEXT("ChangeSelection", "Change Selection"));
			}

			if (ActiveTrackedDelta.IsEmpty() == false)
			{
				TUniquePtr<FGeometrySelectionDeltaChange> AccumDeltaChange = MakeUnique<FGeometrySelectionDeltaChange>();
				AccumDeltaChange->Identifier = Target->TargetIdentifier;
				AccumDeltaChange->Delta = MoveTemp(ActiveTrackedDelta);
				GetTransactionsAPI()->AppendChange(this, MoveTemp(AccumDeltaChange), LOCTEXT("ChangeSelection", "Change Selection"));
			}
		}

		GetTransactionsAPI()->EndUndoTransaction();
		bInTrackedSelectionChange = false;
	}
}


bool UTLLRN_GeometrySelectionManager::SetSelectionForComponent(UPrimitiveComponent* Component, const FGeometrySelection& NewSelection)
{
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		if (Target->TargetIdentifier.TargetObject == Component)
		{
			FGeometrySelection InitialSelection = Target->Selection;
			FGeometrySelectionDelta AfterDelta;
			Target->Selector->UpdateSelectionFromSelection(
				NewSelection, true, *Target->SelectionEditor, 
				FGeometrySelectionUpdateConfig{EGeometrySelectionChangeType::Replace}, &AfterDelta);
			if ( AfterDelta.IsEmpty() == false )
			{
				TUniquePtr<FGeometrySelectionReplaceChange> NewSelectionChange = MakeUnique<FGeometrySelectionReplaceChange>();
				NewSelectionChange->Identifier = Target->TargetIdentifier; //Target->Selector->GetIdentifier();
				NewSelectionChange->After = Target->Selection;
				NewSelectionChange->Before = InitialSelection;
				GetTransactionsAPI()->AppendChange(this, MoveTemp(NewSelectionChange), LOCTEXT("NewSelection", "New Selection"));

				MarkRenderCachesDirty(false);
				OnSelectionModified.Broadcast();
			}
			return true;
		}
	}
	return false;
}

void UTLLRN_GeometrySelectionManager::SaveCurrentSelection()
{
	SavedSelection.Reset();
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		SavedSelection.Targets.Add(Target->TargetIdentifier.TargetObject);
		SavedSelection.Selections.Add(Target->Selection);
	}
}

bool UTLLRN_GeometrySelectionManager::RestoreSavedSelection()
{
#if WITH_EDITORONLY_DATA
	// Cannot update the selection if we're already in a transaction (can happen e.g. when we undo out of a tool)
	if (GIsTransacting)
	{
		DiscardSavedSelection();
		return false;
	}
#endif

	check(SavedSelection.Targets.Num() == SavedSelection.Selections.Num());
	GetTransactionsAPI()->BeginUndoTransaction(LOCTEXT("RestoreSelection", "Restore Selection"));

	bool bSuccess = true;
	for (int32 TargetIdx = 0; TargetIdx < SavedSelection.Targets.Num(); ++TargetIdx)
	{
		if (!SavedSelection.Targets[TargetIdx].IsValid())
		{
			bSuccess = false;
			continue;
		}
		const FGeometrySelection& NewSelection = SavedSelection.Selections[TargetIdx];
		bool bFound = false;
		for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
		{
			if (SavedSelection.Targets[TargetIdx] == Target->TargetIdentifier.TargetObject)
			{
				FGeometrySelection InitialSelection = Target->Selection;
				FGeometrySelectionDelta AfterDelta;
				Target->Selector->UpdateSelectionFromSelection(
					NewSelection, true, *Target->SelectionEditor,
					FGeometrySelectionUpdateConfig{ EGeometrySelectionChangeType::Replace }, &AfterDelta);
				if (AfterDelta.IsEmpty() == false)
				{
					TUniquePtr<FGeometrySelectionReplaceChange> NewSelectionChange = MakeUnique<FGeometrySelectionReplaceChange>();
					NewSelectionChange->Identifier = Target->TargetIdentifier;
					NewSelectionChange->After = Target->Selection;
					NewSelectionChange->Before = InitialSelection;
					GetTransactionsAPI()->AppendChange(this, MoveTemp(NewSelectionChange), LOCTEXT("RestoreSelection", "Restore Selection"));

					MarkRenderCachesDirty(false);
					OnSelectionModified.Broadcast();
				}
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			bSuccess = false;
		}
	}

	GetTransactionsAPI()->EndUndoTransaction();

	DiscardSavedSelection();

	return bSuccess;
}

void UTLLRN_GeometrySelectionManager::DiscardSavedSelection()
{
	SavedSelection.Empty();
}

bool UTLLRN_GeometrySelectionManager::HasSavedSelection()
{
	return !SavedSelection.Selections.IsEmpty();
}


bool UTLLRN_GeometrySelectionManager::UpdateSelectionPreviewViaRaycast(
	const FRay3d& WorldRay )
{
	if (ActiveTargetReferences.Num() == 0)
	{
		return false;
	}
	
	// currently only going to support one object, not sure how to support more yet...

	FGeometrySelectionTarget* Target = ActiveTargetReferences[0].Get();

	IGeometrySelector::FWorldRayQueryInfo RayQueryInfo;
	RayQueryInfo.WorldRay = WorldRay;
	IToolsContextQueriesAPI* QueryAPI = this->ToolsContext->ToolManager->GetContextQueriesAPI();
	QueryAPI->GetCurrentViewState(RayQueryInfo.CameraState);

	FGeometrySelectionPreview NewPreview( *(Target->SelectionEditor) );
	Target->Selector->GetSelectionPreviewForRaycast( RayQueryInfo, NewPreview );
	if ( ! UE::Geometry::AreSelectionsIdentical(NewPreview.PreviewSelection, ActivePreviewSelection) )
	{
		ActivePreviewSelection = MoveTemp(NewPreview.PreviewSelection);
		
		// Initialize [Un]SelectedActivePreviewSelection(s) so that they are of the correct Topology and Geometry type, then clear them
		SelectedActivePreviewSelection = MoveTemp(NewPreview.PreviewSelection);
		UnselectedActivePreviewSelection = MoveTemp(NewPreview.PreviewSelection);
		SelectedActivePreviewSelection.Reset();
		UnselectedActivePreviewSelection.Reset();

		if (MeshTopologyMode == EMeshTopologyMode::Polygroup)
		{
			// Get all polygroup IDs in current preview selection
			TSet<uint32> SelectedGroupIDs;
			for (const uint64 ID : Target->Selection.Selection)
			{
				SelectedGroupIDs.Add(FGeoSelectionID(ID).TopologyID);
			}

			// Get GroupID of active preview selection (hovered items)
			for (const uint64 ID : ActivePreviewSelection.Selection)
			{
				const uint32 TopoID = FGeoSelectionID(ID).TopologyID;

				// add to selection according to if an element with the GroupID is already selected
				if (SelectedGroupIDs.Contains(TopoID))
				{
					SelectedActivePreviewSelection.Selection.Add(ID);
				}
				else
				{
					UnselectedActivePreviewSelection.Selection.Add(ID);
				}
			}
		}
		// Triangle Topology mode is more straightforward
		else if (MeshTopologyMode == EMeshTopologyMode::Triangle)
		{
			for (const uint64 ID : ActivePreviewSelection.Selection)
			{
				if (Target->Selection.Selection.Contains(ID))
				{
					SelectedActivePreviewSelection.Selection.Add(ID);
				}
				else
				{
					UnselectedActivePreviewSelection.Selection.Add(ID);
				}
			}
		}
		CachedSelectedPreviewRenderElements.Reset();
		CachedUnselectedPreviewRenderElements.Reset();

		RenderCachesDirtyFlags |= EEnumerateRenderCachesDirtyFlags::PreviewCachesDirty;

		RemoveSets(HoverOverSelectedParams.Identifiers);
		RemoveSets(HoverOverUnselectedParams.Identifiers);
	}

	return (ActivePreviewSelection.IsEmpty() == false);
}

void UTLLRN_GeometrySelectionManager::ClearSelectionPreview()
{
	ClearActivePreview();
}

bool UTLLRN_GeometrySelectionManager::GetSelectionBounds(FGeometrySelectionBounds& BoundsOut) const
{
	BoundsOut = FGeometrySelectionBounds();

	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		Target->Selector->AccumulateSelectionBounds(Target->Selection, BoundsOut, true);
	}

	return (BoundsOut.WorldBounds.IsEmpty() == false);
}


void UTLLRN_GeometrySelectionManager::GetSelectionWorldFrame(FFrame3d& SelectionFrame) const
{
	SelectionFrame = FFrame3d();
	if (HasSelection())
	{
		// only handling this case for now
		//if (ActiveTargetReferences.Num() == 1)
		TSharedPtr<FGeometrySelectionTarget> Target = ActiveTargetReferences[0];
		Target->Selector->GetSelectionFrame(Target->Selection, SelectionFrame, true);
	}
}

void UTLLRN_GeometrySelectionManager::GetTargetWorldFrame(FFrame3d& SelectionFrame) const
{
	SelectionFrame = FFrame3d();
	if (HasSelection())
	{
		// only handling one target for now
		//if (ActiveTargetReferences.Num() == 1)
		TSharedPtr<FGeometrySelectionTarget> Target = ActiveTargetReferences[0];
		Target->Selector->GetTargetFrame(Target->Selection, SelectionFrame);
	}
}

bool UTLLRN_GeometrySelectionManager::HasSelectionForComponent(UPrimitiveComponent* Component) const
{
	if (HasSelection())
	{
		for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
		{
			if (Target->TargetIdentifier.TargetObject == Component)
			{
				return Target->Selection.IsEmpty();
			}
		}
	}
	return false;
}

bool UTLLRN_GeometrySelectionManager::GetSelectionForComponent(UPrimitiveComponent* Component, FGeometrySelection& SelectionOut) const
{
	if (HasSelection())
	{
		for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
		{
			if (Target->TargetIdentifier.TargetObject == Component)
			{
				SelectionOut = Target->Selection;
				return ! Target->Selection.IsEmpty();
			}
		}
	}
	return false;
}



bool UTLLRN_GeometrySelectionManager::BeginTransformation()
{
	if (!ensure(IsInActiveTransformation() == false))
	{
		return false;
	}
	if (HasSelection() == false)
	{
		return false;
	}

	bool bHaveTransformers = false;
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		IGeometrySelectionTransformer* Transformer = Target->Selector->InitializeTransformation(Target->Selection);
		if (Transformer != nullptr)
		{
			Transformer->BeginTransform(Target->Selection);
			ActiveTransformations.Add(Transformer);
			bHaveTransformers = true;
		}
		else
		{
			ActiveTransformations.Add(nullptr);
		}
	}

	if (!bHaveTransformers)
	{
		ActiveTransformations.Reset();
		return false;
	}

	return true;
}

void UTLLRN_GeometrySelectionManager::UpdateTransformation( 
	TFunctionRef<FVector3d(int32 VertexID, const FVector3d&, const FTransform&)> PositionTransformFunc )
{
	if ( ! ensure(IsInActiveTransformation()) ) return;

	for (int32 k = 0; k < ActiveTargetReferences.Num(); ++k)
	{
		if (ActiveTransformations[k] != nullptr)
		{
			ActiveTransformations[k]->UpdateTransform(PositionTransformFunc);
		}
	}

	RenderCachesDirtyFlags |= EEnumerateRenderCachesDirtyFlags::SelectionCachesDirty;
}

void UTLLRN_GeometrySelectionManager::EndTransformation()
{
	if ( ! ensure(IsInActiveTransformation()) ) return;

	GetTransactionsAPI()->BeginUndoTransaction(LOCTEXT("EndTransformation", "Transform Selection"));

	for (int32 k = 0; k < ActiveTargetReferences.Num(); ++k)
	{
		if (ActiveTransformations[k] != nullptr)
		{
			ActiveTransformations[k]->EndTransform(GetTransactionsAPI());

			ActiveTargetReferences[k]->Selector->ShutdownTransformation(ActiveTransformations[k]);
		}
	}
	ActiveTransformations.Reset();

	GetTransactionsAPI()->EndUndoTransaction();

	RenderCachesDirtyFlags |= EEnumerateRenderCachesDirtyFlags::SelectionCachesDirty;
}





bool UTLLRN_GeometrySelectionManager::CanExecuteSelectionCommand(UTLLRN_GeometrySelectionEditCommand* Command)
{
	if (SelectionArguments == nullptr)
	{
		SelectionArguments = NewObject<UTLLRN_GeometrySelectionEditCommandArguments>();
	}

	bool bCanExecute = true;
	bool bHaveSelections = false;
	ProcessActiveSelections([&](FGeometrySelectionHandle Handle)
	{
		SelectionArguments->SelectionHandle = Handle;
		SelectionArguments->SetTransactionsAPI(TransactionsAPI);
		bCanExecute = bCanExecute && Command->CanExecuteCommand(SelectionArguments);
		bHaveSelections = true;
	});

	return ( bHaveSelections || (Command->AllowEmptySelection() && HasActiveTargets() && MeshTopologyMode != EMeshTopologyMode::None ) ) && bCanExecute;
}

void UTLLRN_GeometrySelectionManager::ExecuteSelectionCommand(UTLLRN_GeometrySelectionEditCommand* Command)
{
	if (SelectionArguments == nullptr)
	{
		SelectionArguments = NewObject<UTLLRN_GeometrySelectionEditCommandArguments>();
	}

	// open transaction to wrap the entire set of Commands and selection changes
	FText CommandText = Command->GetCommandShortString();
	GetTransactionsAPI()->BeginUndoTransaction(CommandText);

	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		if (Target->Selection.IsEmpty() && Command->AllowEmptySelection() == false) continue;

		// TODO: can use Command->IsModifySelectionCommand() to check if this is a command that only affects selection
		// and not geometry. In that case we can skip the intermediate clear-selection and emit a single change.

		// When initially executing the command, we do not clear the selection, because we pass it to the command.
		// However, when we later *undo* any changes emitted by the command, we need to restore the selection aftewards.
		// So we emit a clearing change here, so that undo un-clears. 
		// When we later Redo, it is also necessary to Clear as otherwise an invalid Selection might hang around.
		// Note that this must happen *before* the Command. The Command will not be re-executed, only its emitted Changes,
		// so it will not be holding onto the active Selection on Redo later
		// (if that becomes necessary, this sequence of changes will need to become more complicated....)
		TUniquePtr<FGeometrySelectionReplaceChange> ClearChange = MakeUnique<FGeometrySelectionReplaceChange>();
		ClearChange->Identifier = Target->TargetIdentifier; //Target->Selector->GetIdentifier();
		ClearChange->Before = Target->Selection;
		ClearChange->After.InitializeTypes(ClearChange->Before);
		GetTransactionsAPI()->AppendChange(this, MoveTemp(ClearChange), LOCTEXT("ClearSelection", "Clear Selection"));

		// q: we could clear the selection here, and pass the Handle a copy. Perhaps safer?
		UInteractiveCommandResult* ResultPtr = nullptr;
		SelectionArguments->SelectionHandle = FGeometrySelectionHandle{ Target->Selector->GetIdentifier(), &Target->Selection, Target->Selector.Get() };
		SelectionArguments->ElementType = SelectionElementType;
		SelectionArguments->TopologyMode = GetSelectionTopologyType();
		SelectionArguments->SetTransactionsAPI(TransactionsAPI);
		Command->ExecuteCommand(SelectionArguments, &ResultPtr);

		// actually clear selection after executing command. 
		FGeometrySelectionDelta ClearDelta;
		Target->SelectionEditor->ClearSelection(ClearDelta);

		// if selection returned a result, and it was a non-empty selection, select it
		if ( UTLLRN_GeometrySelectionEditCommandResult* SelectionResult = Cast<UTLLRN_GeometrySelectionEditCommandResult>(ResultPtr) )
		{
			if (SelectionResult->OutputSelection.IsEmpty() == false )
			{
				FGeometrySelectionDelta AfterDelta;
				Target->Selector->UpdateSelectionFromSelection(
					SelectionResult->OutputSelection, true, *Target->SelectionEditor, 
					FGeometrySelectionUpdateConfig{EGeometrySelectionChangeType::Add}, &AfterDelta);
				if (Target->Selection.IsEmpty() == false)
				{
					TUniquePtr<FGeometrySelectionReplaceChange> NewSelectionChange = MakeUnique<FGeometrySelectionReplaceChange>();
					NewSelectionChange->Identifier = Target->TargetIdentifier; //Target->Selector->GetIdentifier();
					NewSelectionChange->After = Target->Selection;
					NewSelectionChange->Before.InitializeTypes(Target->Selection);
					GetTransactionsAPI()->AppendChange(this, MoveTemp(NewSelectionChange), LOCTEXT("NewSelection", "New Selection"));
				}
			}
		}
	}

	GetTransactionsAPI()->EndUndoTransaction();

	// assume marking render caches (except selectable) is true for now
	MarkRenderCachesDirty(false);
	OnSelectionModified.Broadcast();
}


void UTLLRN_GeometrySelectionManager::ProcessActiveSelections(TFunctionRef<void(FGeometrySelectionHandle)> ProcessFunc)
{
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		if (Target->Selection.IsEmpty() == false)
		{
			FGeometrySelectionHandle Handle;
			Handle.Selection = & Target->Selection;
			Handle.Identifier = Target->Selector->GetIdentifier();
			Handle.Selector = Target->Selector.Get();
			ProcessFunc(Handle);
		}
	}
}









void UTLLRN_GeometrySelectionManager::ApplyChange(IGeometrySelectionChange* Change)
{
	// We should not get here because selection changes should have been expired.
	if ( ! ensure(HasBeenShutDown() == false) )
	{
		return;
	}

	FGeometryIdentifier Identifer = Change->GetIdentifier();

	for (int32 k = 0; k < ActiveTargetReferences.Num(); ++k)
	{
		if (ActiveTargetReferences[k]->TargetIdentifier == Identifer)
		{
			FGeometrySelectionDelta ApplyDelta;
			Change->ApplyChange( ActiveTargetReferences[k]->SelectionEditor.Get(), ApplyDelta);

			if (ApplyDelta.IsEmpty() == false)
			{
				MarkRenderCachesDirty(false);
				OnSelectionModified.Broadcast();
			}

			break;
		}
	}
}


void UTLLRN_GeometrySelectionManager::RevertChange(IGeometrySelectionChange* Change)
{
	// We should not get here because selection changes should have been expired.
	if ( ! ensure(HasBeenShutDown() == false) )
	{
		return;
	}

	FGeometryIdentifier Identifer = Change->GetIdentifier();

	for (int32 k = 0; k < ActiveTargetReferences.Num(); ++k)
	{
		if (ActiveTargetReferences[k]->TargetIdentifier == Identifer) 
		{
			FGeometrySelectionDelta RevertDelta;
			Change->RevertChange( ActiveTargetReferences[k]->SelectionEditor.Get(), RevertDelta);

			if (RevertDelta.IsEmpty() == false)
			{
				MarkRenderCachesDirty(false);
				OnSelectionModified.Broadcast();
			}

			break;
		}
	}
}


void UTLLRN_GeometrySelectionManager::OnTargetGeometryModified(IGeometrySelector* Selector)
{
	CachedSelectableRenderElements.Reset();
	
	CachedSelectableRenderElements.SetNum(ActiveTargetReferences.Num());

	RemoveSets(UnselectedParams.Identifiers);
	
	MarkRenderCachesDirty();
	ClearActivePreview();
}


void UTLLRN_GeometrySelectionManager::UpdateSelectionRenderCacheOnTargetChange()
{
	CachedSelectionRenderElements.Reset();
	CachedSelectableRenderElements.Reset();
	
	CachedSelectionRenderElements.SetNum(ActiveTargetReferences.Num());
	CachedSelectableRenderElements.SetNum(ActiveTargetReferences.Num());

	RemoveSets(SelectedParams.Identifiers);
	RemoveSets(UnselectedParams.Identifiers);
	
	MarkRenderCachesDirty();
	ClearActivePreview();
}

void UTLLRN_GeometrySelectionManager::MarkRenderCachesDirty(bool bMarkSelectableDirty)
{
	if (bMarkSelectableDirty)
	{
		RenderCachesDirtyFlags |= EEnumerateRenderCachesDirtyFlags::UnselectedCachesDirty;
	}
	RenderCachesDirtyFlags |= EEnumerateRenderCachesDirtyFlags::PreviewCachesDirty | EEnumerateRenderCachesDirtyFlags::SelectionCachesDirty;
}

void UTLLRN_GeometrySelectionManager::RebuildSelectionRenderCaches()
{
	RebuildSelectionRenderCache();
	RebuildSelectableRenderCache();
	RebuildPreviewRenderCache();
}

void UTLLRN_GeometrySelectionManager::ClearActivePreview()
{
	ActivePreviewSelection.Reset();
	SelectedActivePreviewSelection.Reset();
	UnselectedActivePreviewSelection.Reset();
	CachedSelectedPreviewRenderElements.Reset();
	CachedUnselectedPreviewRenderElements.Reset();
	
	RenderCachesDirtyFlags |= EEnumerateRenderCachesDirtyFlags::PreviewCachesDirty;

	RemoveSets(HoverOverSelectedParams.Identifiers);
	RemoveSets(HoverOverUnselectedParams.Identifiers);
}


void UTLLRN_GeometrySelectionManager::RemoveAllSets() const
{
	if(!ensure(TLLRN_PreviewGeometry))
	{
		return;
	}
	TLLRN_PreviewGeometry->RemoveAllLineSets();
	TLLRN_PreviewGeometry->RemoveAllPointSets();
	TLLRN_PreviewGeometry->RemoveAllTriangleSets();
}

void UTLLRN_GeometrySelectionManager::RemoveSets(const TArrayView<FString>& SetIdentifiers) const
{
	
	if (TLLRN_PreviewGeometry)
	{
		TLLRN_PreviewGeometry->RemoveLineSet(SetIdentifiers[1]);
		TLLRN_PreviewGeometry->RemovePointSet(SetIdentifiers[0]);
		TLLRN_PreviewGeometry->RemoveTriangleSet(SetIdentifiers[2]);
	}
}

void UTLLRN_GeometrySelectionManager::CreateOrUpdateAllSets(const FGeometrySelectionElements& Elements, const FTLLRN_MeshElementSelectionParams SelectionParams) const
{

	UWorld* World = GetWorld();
	if (World && !TLLRN_PreviewGeometry->ParentActor)
	{
		TLLRN_PreviewGeometry->CreateInWorld(World, FTransform::Identity);
	}

	if(SelectionElementType == EGeometryElementType::Edge || SelectionElementType == EGeometryElementType::Face)
	{
		TLLRN_PreviewGeometry->CreateOrUpdateLineSet(SelectionParams.Identifiers[1], Elements.Segments.Num(), [&](int32 j, TArray<FRenderableLine>& LinesOut)
		{
		const FSegment3d Seg = Elements.Segments[j];
		LinesOut.Add(FRenderableLine(Seg.StartPoint(), Seg.EndPoint(),  SelectionParams.Color,  SelectionParams.LineThickness, SelectionParams.DepthBias));
		}, 1);
	
		TLLRN_PreviewGeometry->CreateOrUpdateTriangleSet(SelectionParams.Identifiers[2], Elements.Triangles.Num(), [&](int32 k, TArray<FTLLRN_RenderableTriangle>& TrianglesOut)
			{
			const FTriangle3d Triangle = Elements.Triangles[k];
			const FVector3d Normal = Triangle.Normal();
			FTLLRN_RenderableTriangleVertex A(Triangle.V[0], FVector2D(0,0), Normal, SelectionParams.Color);
			FTLLRN_RenderableTriangleVertex B(Triangle.V[1], FVector2D(1,0), Normal, SelectionParams.Color);
			FTLLRN_RenderableTriangleVertex C(Triangle.V[2], FVector2D(1,1), Normal, SelectionParams.Color);
			TrianglesOut.Add(FTLLRN_RenderableTriangle(SelectionParams.SelectionFillColor, A, B, C));
			},1 );
	}
	else if (SelectionElementType == EGeometryElementType::Vertex)
	{
		TLLRN_PreviewGeometry->CreateOrUpdatePointSet(SelectionParams.Identifiers[0], Elements.Points.Num(), [&](int32 k, TArray<FRenderablePoint>& PointsOut)
		{
		const FVector3d Point = Elements.Points[k];
		PointsOut.Add(FRenderablePoint(Point, SelectionParams.Color, SelectionParams.PointSize, SelectionParams.DepthBias));
		});
	}
}

void UTLLRN_GeometrySelectionManager::RebuildSelectionRenderCache()
{
	if ((RenderCachesDirtyFlags & EEnumerateRenderCachesDirtyFlags::SelectionCachesDirty) == EEnumerateRenderCachesDirtyFlags::None)
	{
		return;
	}
	check(ActiveTargetReferences.Num() == CachedSelectionRenderElements.Num());
	for (int32 k = 0; k < ActiveTargetReferences.Num(); ++k)
	{
		TSharedPtr<FGeometrySelectionTarget> Target = ActiveTargetReferences[k];
		
		FGeometrySelectionElements& SelectionElements = CachedSelectionRenderElements[k];
		SelectionElements.Reset();
		Target->Selector->AccumulateSelectionElements(Target->Selection, SelectionElements, true, EEnumerateSelectionMapping::Default);
		CreateOrUpdateAllSets(SelectionElements, SelectedParams);
	}

	RenderCachesDirtyFlags &= ~EEnumerateRenderCachesDirtyFlags::SelectionCachesDirty;
}

void UTLLRN_GeometrySelectionManager::RebuildSelectableRenderCache()
{
	if ((RenderCachesDirtyFlags & EEnumerateRenderCachesDirtyFlags::UnselectedCachesDirty) == EEnumerateRenderCachesDirtyFlags::None || MeshTopologyMode == EMeshTopologyMode::None)
	{
		return;
	}

	check(ActiveTargetReferences.Num() == CachedSelectionRenderElements.Num());
	for (int32 k = 0; k < ActiveTargetReferences.Num(); ++k)
	{
		TSharedPtr<FGeometrySelectionTarget> Target = ActiveTargetReferences[k];
		FGeometrySelectionElements& AllElements = CachedSelectableRenderElements[k];
		
		AllElements.Reset();
		
		Target->Selector->AccumulateElementsFromPredicate(AllElements, true, false, MeshTopologyMode == EMeshTopologyMode::Polygroup, [Target, this](EGeometryElementType Type, FGeoSelectionID ID)
		{
			// Selectable faces are not displayed directly, just implicitly via displayed edges.
			if (Type == EGeometryElementType::Face)
			{
				return false;
			}
			
			return true;
		});
		CreateOrUpdateAllSets(AllElements, UnselectedParams);
	}

	RenderCachesDirtyFlags &= ~EEnumerateRenderCachesDirtyFlags::UnselectedCachesDirty;
}

void UTLLRN_GeometrySelectionManager::RebuildPreviewRenderCache()
{
	if ((RenderCachesDirtyFlags & EEnumerateRenderCachesDirtyFlags::PreviewCachesDirty) == EEnumerateRenderCachesDirtyFlags::None || ActiveTargetReferences.Num() == 0)
	{
		return;
	}

	// defaults to off/false; when off, will show outlines and fill color when hovering. When on/true, will only show outlines
	const TSharedPtr<FGeometrySelectionTarget> Target = ActiveTargetReferences[0];
	
	if (ActivePreviewSelection.IsEmpty() == false)
	{
		EEnumerateSelectionMapping MappingFlags = EEnumerateSelectionMapping::Default | EEnumerateSelectionMapping::FacesToEdges;
		if (CVarTLLRN_GeometrySelectionManager_FullSelectionHoverHighlights.GetValueOnGameThread() == 0)
		{
			// Unset FacesToFaces flag if full hover highlights are disabled
			MappingFlags &= ~EEnumerateSelectionMapping::FacesToFaces;
		}
		
		Target->Selector->AccumulateSelectionElements(SelectedActivePreviewSelection, CachedSelectedPreviewRenderElements, true, MappingFlags);
		Target->Selector->AccumulateSelectionElements(UnselectedActivePreviewSelection, CachedUnselectedPreviewRenderElements, true, MappingFlags);
		CreateOrUpdateAllSets(CachedSelectedPreviewRenderElements, HoverOverSelectedParams);
		CreateOrUpdateAllSets(CachedUnselectedPreviewRenderElements, HoverOverUnselectedParams);
	}
	RenderCachesDirtyFlags &= ~EEnumerateRenderCachesDirtyFlags::PreviewCachesDirty;
}


void UTLLRN_GeometrySelectionManager::DebugPrintSelection()
{
	if (ActiveTargetReferences.Num() == 0)
	{
		UE_LOG(LogGeometry, Warning, TEXT("[SelectionManager] No Active Selection"));
		return;
	}
	
	int32 NumSelected = 0;
	for (TSharedPtr<FGeometrySelectionTarget> Target : ActiveTargetReferences)
	{
		NumSelected += Target->Selection.Num();
	}
	UE_LOG(LogGeometry, Warning, TEXT("[SelectionManager] %d selected items in %d active targets"), NumSelected, ActiveTargetReferences.Num());
}


void UTLLRN_GeometrySelectionManager::DebugRender(IToolsContextRenderAPI* RenderAPI)
{
	// disable selection during xform to avoid overhead
	if (IsInActiveTransformation())
	{
		for (int32 k = 0; k < ActiveTargetReferences.Num(); ++k)
		{
			if (ActiveTransformations[k] != nullptr)
			{
				ActiveTransformations[k]->PreviewRender(RenderAPI);
			}
		}

		return;
	}
	if (!this->ToolsContext->ToolManager->HasAnyActiveTool())
	{
		RebuildSelectionRenderCaches();
	}
	else
	{
		// disables TLLRN_PreviewGeometry when in a tool
		RemoveAllSets();
		RenderCachesDirtyFlags |= EEnumerateRenderCachesDirtyFlags::UnselectedCachesDirty;
	}
}

void UTLLRN_GeometrySelectionManager::SetSelectionColors(const FLinearColor UnselectedCol, const FLinearColor HoverOverSelectedCol, const FLinearColor HoverOverUnselectedCol, const FLinearColor GeometrySelectedCol)
{
	UnselectedParams.Color = UnselectedCol.ToFColor(true);
	HoverOverSelectedParams.Color = HoverOverSelectedCol.ToFColor(true);
	HoverOverUnselectedParams.Color = HoverOverUnselectedCol.ToFColor(true);
	SelectedParams.Color = GeometrySelectedCol.ToFColor(true);

	// on initial set up of Materials used for selection colors material (typically when entering modeling mode)
	if (UnselectedParams.SelectionFillColor == nullptr) // if one is null, they all will be
	{
		auto SetMaterial = [this] (FTLLRN_MeshElementSelectionParams& Params, const FLinearColor Color)
		{
			Params.SelectionFillColor =
				ToolSetupUtil::GetCustomTwoSidedDepthOffsetMaterial(this->ToolsContext->ToolManager, Color, Params.DepthBias, Color.A);
		};

		SetMaterial(UnselectedParams, UnselectedCol);
		SetMaterial(HoverOverUnselectedParams, HoverOverUnselectedCol);
		SetMaterial(SelectedParams, GeometrySelectedCol);

		// to avoid flickering, this version of GetCustomTwoSidedDeptOffsetMaterial (without opacity parameter) must be called for HoverOverSelected
		HoverOverSelectedParams.SelectionFillColor =
			ToolSetupUtil::GetCustomTwoSidedDepthOffsetMaterial(this->ToolsContext->ToolManager, HoverOverSelectedCol, HoverOverSelectedParams.DepthBias);
	}
	// setting colors after initialization of Materials (typically using color customization in editor preferences)
	else
	{
		auto SetColorAndOpacity = [](const FTLLRN_MeshElementSelectionParams& Params, const FLinearColor Color)
		{
			Params.SelectionFillColor->SetScalarParameterValue("Opacity", Color.A); // no effect for HoverOverSelected
			Params.SelectionFillColor->SetVectorParameterValue("Color", Color);
		};

		SetColorAndOpacity(UnselectedParams, UnselectedCol);
		SetColorAndOpacity(HoverOverSelectedParams, HoverOverSelectedCol);
		SetColorAndOpacity(HoverOverUnselectedParams, HoverOverUnselectedCol);
		SetColorAndOpacity(SelectedParams, GeometrySelectedCol);
	}

	// ensures that when color is changed in Editor Preferences, colors are immediately updated in the UI
	auto UpdateAllSetsColor = [this](const FTLLRN_MeshElementSelectionParams& Params)
	{
		UTLLRN_PointSetComponent* PointSet = TLLRN_PreviewGeometry->FindPointSet(Params.Identifiers[0]);
		UTLLRN_LineSetComponent* LineSet = TLLRN_PreviewGeometry->FindLineSet(Params.Identifiers[1]);
		UTLLRN_TriangleSetComponent* TriSet = TLLRN_PreviewGeometry->FindTriangleSet(Params.Identifiers[2]);
		if (PointSet) {	PointSet->SetAllPointsColor(Params.Color); }
		if (LineSet) { LineSet->SetAllLinesColor(Params.Color); }
		if (TriSet) { TriSet->SetAllTrianglesColor(Params.Color); }
	};

	UpdateAllSetsColor(UnselectedParams);
	UpdateAllSetsColor(HoverOverSelectedParams);
	UpdateAllSetsColor(HoverOverUnselectedParams);
	UpdateAllSetsColor(SelectedParams);
}

void UTLLRN_GeometrySelectionManager::DisconnectTLLRN_PreviewGeometry()
{
	if (TLLRN_PreviewGeometry)
	{
		RemoveAllSets();
		TLLRN_PreviewGeometry->Disconnect();
		TLLRN_PreviewGeometry = nullptr;
	}
}


#undef LOCTEXT_NAMESPACE
