// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorLayerEditTool.h"

#include "ContextObjectStore.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshChangeTracker.h"
#include "Parameterization/DynamicMeshUVEditor.h"
#include "InteractiveToolManager.h"
#include "MeshOpPreviewHelpers.h" // UMeshOpPreviewWithBackgroundCompute
#include "ParameterizationOps/UVLayoutOp.h"
#include "Properties/UVLayoutProperties.h"
#include "ToolTargets/TLLRN_UVEditorToolMeshInput.h"
#include "ContextObjects/TLLRN_UVToolContextObjects.h"
#include "ModelingToolTargetUtil.h"
#include "Components.h"
#include "TLLRN_UVEditorToolAnalyticsUtils.h"
#include "EngineAnalytics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_UVEditorLayerEditTool)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UUVChannelEditTool"

namespace UTLLRN_UVEditorChannelEditLocals
{
	const FText UVChannelAddTransactionName = LOCTEXT("UVChannelAddTransactionName", "Add UV Channel");
	const FText UVChannelCopyTransactionName = LOCTEXT("UVChannelCopyTransactionName", "Copy UV Channel");
	const FText UVChannelDeleteTransactionName = LOCTEXT("UVChannelDeleteTransactionName", "Delete UV Channel");

	void DeleteChannel(UTLLRN_UVEditorToolMeshInput* Target, int32 DeletedUVChannelIndex, 
		UTLLRN_UVToolAssetAndChannelAPI* AssetAndChannelAPI, bool bClearInstead)
	{
		FDynamicMeshUVEditor DynamicMeshTLLRN_UVEditor(Target->AppliedCanonical.Get(), DeletedUVChannelIndex, false);

		int32 NewChannelIndex = DeletedUVChannelIndex;
		if (bClearInstead)
		{
			DynamicMeshTLLRN_UVEditor.SetPerTriangleUVs(0.0, nullptr);
		}
		else
		{
			NewChannelIndex = DynamicMeshTLLRN_UVEditor.RemoveUVLayer();
		}

		if (NewChannelIndex != Target->UVLayerIndex)
		{
			// The change of displayed layer will perform the needed update for us
			AssetAndChannelAPI->NotifyOfAssetChannelCountChange(Target->AssetID);
			TArray<int32> ChannelPerAsset = AssetAndChannelAPI->GetCurrentChannelVisibility();
			ChannelPerAsset[Target->AssetID] = NewChannelIndex;
			AssetAndChannelAPI->RequestChannelVisibilityChange(ChannelPerAsset, false);
		}
		else
		{
			// We're showing the same layer index, but it's now the next layer over, actually,
			// so we need to update it.
			Target->UpdateAllFromAppliedCanonical();
		}
	}

	class FInputObjectUVChannelAdd : public FToolCommandChange
	{
	public:
		FInputObjectUVChannelAdd(TObjectPtr<UTLLRN_UVEditorToolMeshInput>& TargetIn, int32 AddedUVChannelIndexIn)
			: Target(TargetIn)
			, AddedUVChannelIndex(AddedUVChannelIndexIn)			
		{
		}

		virtual void Apply(UObject* Object) override
		{
			UInteractiveToolManager* InteractiveToolManager = Cast<UTLLRN_UVEditorChannelEditTool>(Object)->GetToolManager();
			if (!ensure(InteractiveToolManager))
			{
				return;
			}
			UTLLRN_UVToolAssetAndChannelAPI* AssetAndChannelAPI = InteractiveToolManager->GetContextObjectStore()->FindContext<UTLLRN_UVToolAssetAndChannelAPI>();

			FDynamicMeshUVEditor DynamicMeshTLLRN_UVEditor(Target->AppliedCanonical.Get(), AddedUVChannelIndex-1, false);
			int32 NewChannelIndex = DynamicMeshTLLRN_UVEditor.AddUVLayer();
			check(NewChannelIndex == AddedUVChannelIndex);

			if (NewChannelIndex != -1)
			{
				Target->AppliedPreview->PreviewMesh->UpdatePreview(Target->AppliedCanonical.Get());

				AssetAndChannelAPI->NotifyOfAssetChannelCountChange(Target->AssetID);
				Target->OnCanonicalModified.Broadcast(Target.Get(), UTLLRN_UVEditorToolMeshInput::FCanonicalModifiedInfo());
			}

		}

		virtual void Revert(UObject* Object) override
		{
			UInteractiveToolManager* InteractiveToolManager = Cast<UTLLRN_UVEditorChannelEditTool>(Object)->GetToolManager();
			if (!ensure(InteractiveToolManager))
			{
				return;
			}
			UTLLRN_UVToolAssetAndChannelAPI* AssetAndChannelAPI = InteractiveToolManager->GetContextObjectStore()->FindContext<UTLLRN_UVToolAssetAndChannelAPI>();

			FDynamicMeshUVEditor DynamicMeshTLLRN_UVEditor(Target->AppliedCanonical.Get(), AddedUVChannelIndex, false);
			int32 NewChannelIndex = DynamicMeshTLLRN_UVEditor.RemoveUVLayer();
			Target->AppliedPreview->PreviewMesh->UpdatePreview(Target->AppliedCanonical.Get());
			AssetAndChannelAPI->NotifyOfAssetChannelCountChange(Target->AssetID);
			Target->OnCanonicalModified.Broadcast(Target.Get(), UTLLRN_UVEditorToolMeshInput::FCanonicalModifiedInfo());
		}

		virtual bool HasExpired(UObject* Object) const override
		{
			return !(Target.IsValid() && Target->IsValid());
		}

		virtual FString ToString() const override
		{
			return TEXT("TLLRN_UVEditorModeLocals::FInputObjectUVChannelAdd");
		}

	protected:
		TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput> Target;
		int32 AddedUVChannelIndex;		
	};

	class FInputObjectUVChannelCopy : public FToolCommandChange
	{
	public:
		FInputObjectUVChannelCopy(const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& TargetIn, int32 SourceUVChannelIndexIn, int32 TargetUVChannelIndexIn,
		                          const FDynamicMeshUVOverlay& OriginalUVChannelIn)
			: Target(TargetIn)			
			, SourceUVChannelIndex(SourceUVChannelIndexIn)
			, TargetUVChannelIndex(TargetUVChannelIndexIn)
			, OriginalUVChannel(OriginalUVChannelIn)
		{
		}

		virtual void Apply(UObject* Object) override
		{
			UInteractiveToolManager* InteractiveToolManager = Cast<UTLLRN_UVEditorChannelEditTool>(Object)->GetToolManager();
			if (!ensure(InteractiveToolManager))
			{
				return;
			}

			FDynamicMeshUVEditor DynamicMeshTLLRN_UVEditor(Target->AppliedCanonical.Get(), TargetUVChannelIndex, false);
			DynamicMeshTLLRN_UVEditor.CopyUVLayer(Target->AppliedCanonical->Attributes()->GetUVLayer(SourceUVChannelIndex));
			Target->UpdateAllFromAppliedCanonical();
		}

		virtual void Revert(UObject* Object) override
		{
			UInteractiveToolManager* InteractiveToolManager = Cast<UTLLRN_UVEditorChannelEditTool>(Object)->GetToolManager();
			if (!ensure(InteractiveToolManager))
			{
				return;
			}

			FDynamicMeshUVEditor DynamicMeshTLLRN_UVEditor(Target->AppliedCanonical.Get(), TargetUVChannelIndex, false);
			DynamicMeshTLLRN_UVEditor.CopyUVLayer(&OriginalUVChannel);
			Target->UpdateAllFromAppliedCanonical();
		}

		virtual bool HasExpired(UObject* Object) const override
		{
			return !(Target.IsValid() && Target->IsValid());
		}

		virtual FString ToString() const override
		{
			return TEXT("TLLRN_UVEditorModeLocals::FInputObjectUVChannelCopy");
		}

	protected:
		TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput> Target;
		int32 SourceUVChannelIndex;
		int32 TargetUVChannelIndex;
		FDynamicMeshUVOverlay OriginalUVChannel;
	};

	class FInputObjectUVChannelDelete : public FToolCommandChange
	{
	public:
		FInputObjectUVChannelDelete(const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& TargetIn, int32 DeletedUVChannelIndexIn, 
			const FDynamicMeshUVOverlay& OriginalUVChannelIn, bool bClearedInsteadIn)
			: Target(TargetIn)			
			, DeletedUVChannelIndex(DeletedUVChannelIndexIn)
			, OriginalUVChannel(OriginalUVChannelIn)
			, bClearedInstead(bClearedInsteadIn)
		{
		}

		virtual void Apply(UObject* Object) override
		{
			UInteractiveToolManager* InteractiveToolManager = Cast<UTLLRN_UVEditorChannelEditTool>(Object)->GetToolManager();
			if (!ensure(InteractiveToolManager))
			{
				return;
			}
			UTLLRN_UVToolAssetAndChannelAPI* AssetAndChannelAPI = InteractiveToolManager->GetContextObjectStore()->FindContext<UTLLRN_UVToolAssetAndChannelAPI>();

			DeleteChannel(Target.Get(), DeletedUVChannelIndex, AssetAndChannelAPI, bClearedInstead);
		}

		virtual void Revert(UObject* Object) override
		{
			UInteractiveToolManager* InteractiveToolManager = Cast<UTLLRN_UVEditorChannelEditTool>(Object)->GetToolManager();
			if (!ensure(InteractiveToolManager))
			{
				return;
			}
			UTLLRN_UVToolAssetAndChannelAPI* AssetAndChannelAPI = InteractiveToolManager->GetContextObjectStore()->FindContext<UTLLRN_UVToolAssetAndChannelAPI>();

			FDynamicMeshUVEditor DynamicMeshTLLRN_UVEditor(Target->AppliedCanonical.Get(), 0, false);
			if (!bClearedInstead)
			{
				int32 NewChannelIndex = DynamicMeshTLLRN_UVEditor.AddUVLayer();

				// Shift the new layer to the correct index.
				// TODO: This is slightly wasteful since we should just be allowed to swap pointers in the underlying indirect array
				for (int32 ChannelIndex = NewChannelIndex; ChannelIndex > DeletedUVChannelIndex; --ChannelIndex)
				{
					DynamicMeshTLLRN_UVEditor.SwitchActiveLayer(ChannelIndex);
					DynamicMeshTLLRN_UVEditor.CopyUVLayer(Target->AppliedCanonical->Attributes()->GetUVLayer(ChannelIndex - 1));
				}
			}

			// Fill the layer
			DynamicMeshTLLRN_UVEditor.SwitchActiveLayer(DeletedUVChannelIndex);
			DynamicMeshTLLRN_UVEditor.CopyUVLayer(&OriginalUVChannel);

			if (DeletedUVChannelIndex != Target->UVLayerIndex)
			{
				// The change of displayed layer will perform the needed update for us
				AssetAndChannelAPI->NotifyOfAssetChannelCountChange(Target->AssetID);
				TArray<int32> ChannelPerAsset = AssetAndChannelAPI->GetCurrentChannelVisibility();
				ChannelPerAsset[Target->AssetID] = DeletedUVChannelIndex;
				AssetAndChannelAPI->RequestChannelVisibilityChange(ChannelPerAsset, false);
			}
			else
			{
				// We're showing the same layer index, but it's actually the previously deleted layer
				Target->UpdateAllFromAppliedCanonical();
			}
		}

		virtual bool HasExpired(UObject* Object) const override
		{
			return !(Target.IsValid() && Target->IsValid());
		}

		virtual FString ToString() const override
		{
			return TEXT("TLLRN_UVEditorModeLocals::FInputObjectUVChannelDelete");
		}

	protected:
		TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput> Target;		
		int32 DeletedUVChannelIndex;
		FDynamicMeshUVOverlay OriginalUVChannel;
		bool bClearedInstead;
	};

}


bool UTLLRN_UVEditorChannelEditToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return Targets && Targets->Num() > 0;
}

UInteractiveTool* UTLLRN_UVEditorChannelEditToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UTLLRN_UVEditorChannelEditTool* NewTool = NewObject<UTLLRN_UVEditorChannelEditTool>(SceneState.ToolManager);
	NewTool->SetTargets(*Targets);

	return NewTool;
}

void UTLLRN_UVEditorChannelEditTargetProperties::Initialize(
	const TArray<TObjectPtr<UTLLRN_UVEditorToolMeshInput>>& TargetsIn,
	bool bInitializeSelection)
{
	UVAssetNames.Reset(TargetsIn.Num());
	NumUVChannelsPerAsset.Reset(TargetsIn.Num());

	for (int i = 0; i < TargetsIn.Num(); ++i)
	{
		UVAssetNames.Add(UE::ToolTarget::GetHumanReadableName(TargetsIn[i]->SourceTarget));

		int32 NumUVChannels = TargetsIn[i]->AppliedCanonical->HasAttributes() ?
			TargetsIn[i]->AppliedCanonical->Attributes()->NumUVLayers() : 0;

		NumUVChannelsPerAsset.Add(NumUVChannels);
	}

	if (bInitializeSelection)
	{
		Asset = UVAssetNames[0];
		GetUVChannelNames();
		TargetChannel = UVChannelNames.Num() > 0 ? UVChannelNames[0] : TEXT("");
		ReferenceChannel = UVChannelNames.Num() > 0 ? UVChannelNames[0] : TEXT("");
	}
}

const TArray<FString>& UTLLRN_UVEditorChannelEditTargetProperties::GetAssetNames()
{
	return UVAssetNames;
}

const TArray<FString>& UTLLRN_UVEditorChannelEditTargetProperties::GetUVChannelNames()
{
	int32 FoundAssetIndex = UVAssetNames.IndexOfByKey(Asset);

	if (FoundAssetIndex == INDEX_NONE)
	{
		UVChannelNames.Reset();
		return UVChannelNames;
	}

	if (UVChannelNames.Num() != NumUVChannelsPerAsset[FoundAssetIndex])
	{
		UVChannelNames.Reset();
		for (int32 i = 0; i < NumUVChannelsPerAsset[FoundAssetIndex]; ++i)
		{
			UVChannelNames.Add(FString::Printf(TEXT("UV%d"), i));
		}
	}

	return UVChannelNames;
}

bool UTLLRN_UVEditorChannelEditTargetProperties::ValidateUVAssetSelection(bool bUpdateIfInvalid)
{
	int32 FoundIndex = UVAssetNames.IndexOfByKey(Asset);
	if (FoundIndex == INDEX_NONE)
	{
		if (bUpdateIfInvalid)
		{
			Asset = (UVAssetNames.Num() > 0) ? UVAssetNames[0] : TEXT("");
		}
		return false;
	}
	return true;
}

bool UTLLRN_UVEditorChannelEditTargetProperties::ValidateUVChannelSelection(bool bUpdateIfInvalid)
{
	bool bValid = ValidateUVAssetSelection(bUpdateIfInvalid);

	int32 FoundAssetIndex = UVAssetNames.IndexOfByKey(Asset);
	if (FoundAssetIndex == INDEX_NONE)
	{
		if (bUpdateIfInvalid)
		{
			TargetChannel = TEXT("");
			ReferenceChannel = TEXT("");
		}
		return false;
	}

	auto CheckAndUpdateChannel = [this, FoundAssetIndex, bUpdateIfInvalid](FString& UVChannel)
	{
		int32 FoundIndex = UVChannelNames.IndexOfByKey(UVChannel);
		if (FoundIndex >= NumUVChannelsPerAsset[FoundAssetIndex])
		{
			FoundIndex = INDEX_NONE;
		}

		if (FoundIndex == INDEX_NONE)
		{
			if (bUpdateIfInvalid)
			{
				UVChannel = (NumUVChannelsPerAsset[FoundAssetIndex] > 0) ? UVChannelNames[0] : TEXT("");
			}
			return false;
		}
		return true;
	};

	bValid &= CheckAndUpdateChannel(TargetChannel);

	return bValid;
}

int32 UTLLRN_UVEditorChannelEditTargetProperties::GetSelectedAssetID()
{
	int32 FoundIndex = UVAssetNames.IndexOfByKey(Asset);
	if (FoundIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	return FoundIndex;
}

int32 UTLLRN_UVEditorChannelEditTargetProperties::GetSelectedChannelIndex(bool bForceToZeroOnFailure, bool bUseReference)
{
	int32 FoundAssetIndex = UVAssetNames.IndexOfByKey(Asset);
	if (FoundAssetIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	FString UVChannel = bUseReference ? ReferenceChannel : TargetChannel;

	int32 FoundUVIndex = UVChannelNames.IndexOfByKey(UVChannel);
	if (!ensure(FoundUVIndex < NumUVChannelsPerAsset[FoundAssetIndex]))
	{
		return INDEX_NONE;
	}
	return FoundUVIndex;
}

void UTLLRN_UVEditorChannelEditTargetProperties::SetUsageFlags(ETLLRN_ChannelEditToolAction Action) {
	switch (Action)
	{
		case ETLLRN_ChannelEditToolAction::Add:
			bActionNeedsAsset = true;
			bActionNeedsReference = false;
			bActionNeedsTarget = false;
			break;
		case ETLLRN_ChannelEditToolAction::Copy:
			bActionNeedsAsset = true;
			bActionNeedsReference = true;
			bActionNeedsTarget = true;
			break;
		case ETLLRN_ChannelEditToolAction::Delete:
			bActionNeedsAsset = true;
			bActionNeedsReference = false;
			bActionNeedsTarget = true;
			break;
		default:
			bActionNeedsAsset = false;
			bActionNeedsReference = false;
			bActionNeedsTarget = false;
			break;
	}
}




// Tool property functions

void  UTLLRN_UVEditorTLLRN_ChannelEditToolActionPropertySet::Apply()
{
	if (ParentTool.IsValid())
	{
		PostAction(ParentTool->ActiveAction());
		return;
	}	
	PostAction(ETLLRN_ChannelEditToolAction::NoAction);
}


void UTLLRN_UVEditorTLLRN_ChannelEditToolActionPropertySet::PostAction(ETLLRN_ChannelEditToolAction Action)
{
	if (ParentTool.IsValid())
	{
		ParentTool->RequestAction(Action);
	}
}



void UTLLRN_UVEditorChannelEditTool::Setup()
{
	check(Targets.Num() > 0);
	
	ToolStartTimeAnalytics = FDateTime::UtcNow();

	UInteractiveTool::Setup();

	EmitChangeAPI = GetToolManager()->GetContextObjectStore()->FindContext<UTLLRN_UVToolEmitChangeAPI>();

	ActionSelectionProperties = NewObject<UTLLRN_UVEditorChannelEditSettings>(this);
	ActionSelectionProperties->WatchProperty(ActionSelectionProperties->Action, [this](ETLLRN_ChannelEditToolAction Action) {
		if (SourceChannelProperties)
		{
			SourceChannelProperties->SetUsageFlags(Action);
			NotifyOfPropertyChangeByTool(SourceChannelProperties);
		}
	});
	AddToolPropertySource(ActionSelectionProperties);

	SourceChannelProperties = NewObject<UTLLRN_UVEditorChannelEditTargetProperties>(this);
	SourceChannelProperties->Initialize(Targets,true);
	SourceChannelProperties->WatchProperty(SourceChannelProperties->Asset, [this](FString UVAsset) {ApplyVisbleChannelChange(); });
	SourceChannelProperties->WatchProperty(SourceChannelProperties->TargetChannel, [this](FString UVChannel) {ApplyVisbleChannelChange(); });


	AddToolPropertySource(SourceChannelProperties);

	AddActionProperties = NewObject<UTLLRN_UVEditorChannelEditAddProperties>(this);
	AddToolPropertySource(AddActionProperties);

	CopyActionProperties = NewObject<UTLLRN_UVEditorChannelEditCopyProperties>(this);
	AddToolPropertySource(CopyActionProperties);

	DeleteActionProperties = NewObject<UTLLRN_UVEditorChannelEditDeleteProperties>(this);
	AddToolPropertySource(DeleteActionProperties);

	ToolActions = NewObject<UTLLRN_UVEditorTLLRN_ChannelEditToolActionPropertySet>(this);
	ToolActions->Initialize(this);
	AddToolPropertySource(ToolActions);

	SetToolDisplayName(LOCTEXT("ToolName", "UV Channel Edit"));
	GetToolManager()->DisplayMessage(LOCTEXT("OnStartUVChannelEditTool", "Add, copy or delete UV channels"),
		EToolMessageLevel::UserNotification);

	for (int32 i = 0; i < Targets.Num(); ++i)
	{
		Targets[i]->OnCanonicalModified.AddWeakLambda(this, [this, i]
		(UTLLRN_UVEditorToolMeshInput* InputObject, const UTLLRN_UVEditorToolMeshInput::FCanonicalModifiedInfo&) {
			UpdateChannelSelectionProperties(i);
		});
	}

	// Analytics
	InputTargetAnalytics = TLLRN_UVEditorAnalytics::CollectTargetAnalytics(Targets);
}

void UTLLRN_UVEditorChannelEditTool::Shutdown(EToolShutdownType ShutdownType)
{
	for (const TObjectPtr<UTLLRN_UVEditorToolMeshInput>& Target : Targets)
	{
		Target->OnCanonicalModified.RemoveAll(this);
	}

	Targets.Empty();

	// Analytics
	RecordAnalytics();
}

void UTLLRN_UVEditorChannelEditTool::UpdateChannelSelectionProperties(int32 ChangingAsset)
{
	SourceChannelProperties->Initialize(Targets, false);

	UTLLRN_UVToolAssetAndChannelAPI* AssetAndChannelAPI = GetToolManager()->GetContextObjectStore()->FindContext<UTLLRN_UVToolAssetAndChannelAPI>();
	if (AssetAndChannelAPI)
	{
		ActiveChannel = AssetAndChannelAPI->GetCurrentChannelVisibility()[ChangingAsset];

		SourceChannelProperties->Asset = SourceChannelProperties->UVAssetNames[ChangingAsset];
		SourceChannelProperties->TargetChannel = SourceChannelProperties->GetUVChannelNames()[ActiveChannel];
		SourceChannelProperties->ValidateUVChannelSelection(true);
		SourceChannelProperties->SilentUpdateWatched();
	}
}

void UTLLRN_UVEditorChannelEditTool::OnTick(float DeltaTime)
{
	if (PendingAction != ETLLRN_ChannelEditToolAction::NoAction)
	{
		ActiveAsset = SourceChannelProperties->GetSelectedAssetID();
		ActiveChannel = SourceChannelProperties->GetSelectedChannelIndex(true, false);
		ReferenceChannel = SourceChannelProperties->GetSelectedChannelIndex(true, true);

		switch (PendingAction)
		{
		case ETLLRN_ChannelEditToolAction::Add:
		{
			AddChannel();
		}
		break;

		case ETLLRN_ChannelEditToolAction::Copy:
		{
			CopyChannel();
		}
		break;

		case ETLLRN_ChannelEditToolAction::Delete:
		{
			DeleteChannel();
		}
		break;

		default:
			checkSlow(false);
		}

		PendingAction = ETLLRN_ChannelEditToolAction::NoAction;
	}
}

void UTLLRN_UVEditorChannelEditTool::ApplyVisbleChannelChange()
{
	SourceChannelProperties->ValidateUVAssetSelection(true);
	SourceChannelProperties->ValidateUVChannelSelection(true);

	ActiveAsset = SourceChannelProperties->GetSelectedAssetID();
	ActiveChannel = SourceChannelProperties->GetSelectedChannelIndex(true, false);

	UTLLRN_UVToolAssetAndChannelAPI* AssetAndChannelAPI = GetToolManager()->GetContextObjectStore()->FindContext<UTLLRN_UVToolAssetAndChannelAPI>();
	if (AssetAndChannelAPI)
	{
		TArray<int32> ChannelPerAsset = AssetAndChannelAPI->GetCurrentChannelVisibility();
		ChannelPerAsset[ActiveAsset] = ActiveChannel;
		AssetAndChannelAPI->RequestChannelVisibilityChange(ChannelPerAsset, true);
	}
}

void UTLLRN_UVEditorChannelEditTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	GetToolManager()->DisplayMessage(LOCTEXT("OnStartUVChannelEditTool", "Add, copy or delete UV channels"),
		EToolMessageLevel::UserNotification);
}

ETLLRN_ChannelEditToolAction UTLLRN_UVEditorChannelEditTool::ActiveAction() const
{
	if (ActionSelectionProperties)
	{
		return ActionSelectionProperties->Action;
	}
	return ETLLRN_ChannelEditToolAction::NoAction;
}

void UTLLRN_UVEditorChannelEditTool::RequestAction(ETLLRN_ChannelEditToolAction ActionType)
{
	PendingAction = ActionType;
}

void UTLLRN_UVEditorChannelEditTool::AddChannel()
{
	UTLLRN_UVToolAssetAndChannelAPI* AssetAndChannelAPI = GetToolManager()->GetContextObjectStore()->FindContext<UTLLRN_UVToolAssetAndChannelAPI>();
	FDynamicMeshUVEditor DynamicMeshTLLRN_UVEditor(Targets[ActiveAsset]->AppliedCanonical.Get(), ActiveChannel, false);
	int32 NewChannelIndex = DynamicMeshTLLRN_UVEditor.AddUVLayer();

	if (NewChannelIndex != -1)
	{
		// Analytics
		FActionHistoryItem Item;
		Item.ActionType = ETLLRN_ChannelEditToolAction::Add;
		Item.FirstOperandIndex = NewChannelIndex;
		AnalyticsActionHistory.Add(Item);

		Targets[ActiveAsset]->AppliedPreview->PreviewMesh->UpdatePreview(Targets[ActiveAsset]->AppliedCanonical.Get());

		EmitChangeAPI->BeginUndoTransaction(UTLLRN_UVEditorChannelEditLocals::UVChannelAddTransactionName);
		EmitChangeAPI->EmitToolIndependentChange(this,
			MakeUnique<UTLLRN_UVEditorChannelEditLocals::FInputObjectUVChannelAdd>(Targets[ActiveAsset], NewChannelIndex),
			UTLLRN_UVEditorChannelEditLocals::UVChannelAddTransactionName);

		AssetAndChannelAPI->NotifyOfAssetChannelCountChange(ActiveAsset);
		TArray<int32> ChannelPerAsset = AssetAndChannelAPI->GetCurrentChannelVisibility();
		ChannelPerAsset[ActiveAsset] = NewChannelIndex;
		AssetAndChannelAPI->RequestChannelVisibilityChange(ChannelPerAsset, true);

		EmitChangeAPI->EndUndoTransaction();

		SourceChannelProperties->Initialize(Targets, false);
		SourceChannelProperties->TargetChannel = SourceChannelProperties->GetUVChannelNames()[NewChannelIndex];
	}

	GetToolManager()->DisplayMessage(LOCTEXT("AddChannelNotification", "New UV Channel added."), EToolMessageLevel::UserNotification);
}

void UTLLRN_UVEditorChannelEditTool::CopyChannel()
{
	// Analytics
	FActionHistoryItem Item;
	Item.ActionType = ETLLRN_ChannelEditToolAction::Copy;
	Item.FirstOperandIndex = ReferenceChannel;
	Item.SecondOperandIndex = ActiveChannel;
	AnalyticsActionHistory.Add(Item);

	EmitChangeAPI->EmitToolIndependentChange(this,
		MakeUnique<UTLLRN_UVEditorChannelEditLocals::FInputObjectUVChannelCopy>(Targets[ActiveAsset], ReferenceChannel, ActiveChannel, 
			*Targets[ActiveAsset]->AppliedCanonical->Attributes()->GetUVLayer(ActiveChannel)),
		UTLLRN_UVEditorChannelEditLocals::UVChannelCopyTransactionName);

	FDynamicMeshUVEditor DynamicMeshTLLRN_UVEditor(Targets[ActiveAsset]->AppliedCanonical.Get(), ActiveChannel, false);
	DynamicMeshTLLRN_UVEditor.CopyUVLayer(Targets[ActiveAsset]->AppliedCanonical->Attributes()->GetUVLayer(ReferenceChannel));
	Targets[ActiveAsset]->UpdateAllFromAppliedCanonical();

	SourceChannelProperties->Initialize(Targets, false);

	GetToolManager()->DisplayMessage(LOCTEXT("CopyChannelNotification", "UV Channel copied."), EToolMessageLevel::UserNotification);
}

void UTLLRN_UVEditorChannelEditTool::DeleteChannel()
{
	UTLLRN_UVToolAssetAndChannelAPI* AssetAndChannelAPI = GetToolManager()->GetContextObjectStore()->FindContext<UTLLRN_UVToolAssetAndChannelAPI>();

	int32 TotalLayerCount = Targets[ActiveAsset]->AppliedCanonical->Attributes()->NumUVLayers();
	bool bClearInstead = TotalLayerCount == 1;
	
	// Analytics
	FActionHistoryItem Item;
	Item.ActionType = ETLLRN_ChannelEditToolAction::Delete;
	Item.FirstOperandIndex = ActiveChannel;
	Item.bDeleteActionWasActuallyClear = bClearInstead;
	AnalyticsActionHistory.Add(Item);

	EmitChangeAPI->EmitToolIndependentChange(this,
		MakeUnique<UTLLRN_UVEditorChannelEditLocals::FInputObjectUVChannelDelete>(Targets[ActiveAsset], ActiveChannel, 
			*Targets[ActiveAsset]->AppliedCanonical->Attributes()->GetUVLayer(ActiveChannel), bClearInstead),
		UTLLRN_UVEditorChannelEditLocals::UVChannelDeleteTransactionName);

	UTLLRN_UVEditorChannelEditLocals::DeleteChannel(Targets[ActiveAsset], ActiveChannel, AssetAndChannelAPI, bClearInstead);

	SourceChannelProperties->Initialize(Targets, false);
	SourceChannelProperties->TargetChannel = SourceChannelProperties->GetUVChannelNames()[Targets[ActiveAsset]->UVLayerIndex];

	GetToolManager()->DisplayMessage(LOCTEXT("DeleteChannelNotification", "UV Channel deleted."), EToolMessageLevel::UserNotification);
}

void UTLLRN_UVEditorChannelEditTool::RecordAnalytics()
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
	auto MaybeAppendStatsToAttributes = [this, &Attributes](ETLLRN_ChannelEditToolAction ActionType)
	{
		// For now we only care about the number of times actions were issued
		int32 NumActions = 0;
		
		for (const FActionHistoryItem& Item : AnalyticsActionHistory)
		{
			if (Item.ActionType == ActionType) {
				NumActions += 1;
			}
		}
		
		if (NumActions > 0)
		{
			const FString ActionName = StaticEnum<ETLLRN_ChannelEditToolAction>()->GetNameStringByIndex(static_cast<int>(ActionType));
			Attributes.Add(FAnalyticsEventAttribute(FString::Printf(TEXT("Stats.%sAction.NumActions"), *ActionName), NumActions));
		}
	};
	MaybeAppendStatsToAttributes(ETLLRN_ChannelEditToolAction::Add);
	MaybeAppendStatsToAttributes(ETLLRN_ChannelEditToolAction::Delete);
	MaybeAppendStatsToAttributes(ETLLRN_ChannelEditToolAction::Copy);
	Attributes.Add(FAnalyticsEventAttribute(TEXT("Stats.ToolActiveDuration"), (FDateTime::UtcNow() - ToolStartTimeAnalytics).ToString()));
	
	FEngineAnalytics::GetProvider().RecordEvent(TLLRN_UVEditorAnalyticsEventName(TEXT("ChannelEditTool")), Attributes);

	if constexpr (false)
	{
		for (const FAnalyticsEventAttribute& Attr : Attributes)
		{
			UE_LOG(LogGeometry, Log, TEXT("Debug %s.%s = %s"), *TLLRN_UVEditorAnalyticsEventName(TEXT("ChannelEditTool")), *Attr.GetName(), *Attr.GetValue());
		}
	}
}

#undef LOCTEXT_NAMESPACE
