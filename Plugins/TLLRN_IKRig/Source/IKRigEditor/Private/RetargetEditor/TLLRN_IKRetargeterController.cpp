// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_IKRetargeterController.h"

#include "ScopedTransaction.h"
#include "Algo/LevenshteinDistance.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "Retargeter/TLLRN_IKRetargeter.h"
#include "RigEditor/TLLRN_IKRigController.h"
#include "TLLRN_IKRigEditor.h"
#include "RetargetEditor/TLLRN_IKRetargeterPoseGenerator.h"
#include "Retargeter/TLLRN_IKRetargetOps.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRetargeterController)

#define LOCTEXT_NAMESPACE "TLLRN_IKRetargeterController"


UTLLRN_IKRetargeterController* UTLLRN_IKRetargeterController::GetController(const UTLLRN_IKRetargeter* InRetargeterAsset)
{
	if (!InRetargeterAsset)
	{
		return nullptr;
	}

	if (!InRetargeterAsset->Controller)
	{
		UTLLRN_IKRetargeterController* Controller = NewObject<UTLLRN_IKRetargeterController>();
		Controller->Asset = const_cast<UTLLRN_IKRetargeter*>(InRetargeterAsset);
		Controller->Asset->Controller = Controller;
	}

	return Cast<UTLLRN_IKRetargeterController>(InRetargeterAsset->Controller);
}

void UTLLRN_IKRetargeterController::PostInitProperties()
{
	Super::PostInitProperties();
	AutoPoseGenerator = MakeUnique<FTLLRN_RetargetAutoPoseGenerator>(this);
}

UTLLRN_IKRetargeter* UTLLRN_IKRetargeterController::GetAsset() const
{
	return Asset;
}

TObjectPtr<UTLLRN_IKRetargeter>& UTLLRN_IKRetargeterController::GetAssetPtr()
{
	return Asset;
}

void UTLLRN_IKRetargeterController::CleanAsset() const
{
	FScopeLock Lock(&ControllerLock);
	CleanChainMapping();
	CleanPoseList(ETLLRN_RetargetSourceOrTarget::Source);
	CleanPoseList(ETLLRN_RetargetSourceOrTarget::Target);
}

void UTLLRN_IKRetargeterController::SetIKRig(const ETLLRN_RetargetSourceOrTarget SourceOrTarget, UTLLRN_IKRigDefinition* IKRig) const
{
	FScopeLock Lock(&ControllerLock);
	
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	
	if (SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source)
	{
		Asset->SourceIKRigAsset = IKRig;
		Asset->SourcePreviewMesh = IKRig ? IKRig->GetPreviewMesh() : nullptr;
	}
	else
	{
		Asset->TargetIKRigAsset = IKRig;
		Asset->TargetPreviewMesh = IKRig ? IKRig->GetPreviewMesh() : nullptr;
	}
	
	// re-ask to fix root height for this mesh
	if (IKRig)
	{
		SetAskedToFixRootHeightForMesh(GetPreviewMesh(SourceOrTarget), false);
	}

	CleanChainMapping();
	
	constexpr bool bForceRemap = false;
	AutoMapChains(ETLLRN_AutoMapChainType::Exact, bForceRemap);

	// update any editors attached to this asset
	BroadcastIKRigReplaced(SourceOrTarget);
	BroadcastPreviewMeshReplaced(SourceOrTarget);
}

const UTLLRN_IKRigDefinition* UTLLRN_IKRetargeterController::GetIKRig(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	return Asset->GetIKRig(SourceOrTarget);
}

UTLLRN_IKRigDefinition* UTLLRN_IKRetargeterController::GetIKRigWriteable(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	return Asset->GetIKRigWriteable(SourceOrTarget);
}

void UTLLRN_IKRetargeterController::SetPreviewMesh(
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
	USkeletalMesh* PreviewMesh) const
{
	FScopeLock Lock(&ControllerLock);

	FScopedTransaction Transaction(LOCTEXT("SetPreviewMesh_Transaction", "Set Preview Mesh"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	
	if (SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source)
	{
		Asset->SourcePreviewMesh = PreviewMesh;
	}
	else
	{
		Asset->TargetPreviewMesh = PreviewMesh;
	}
	
	// re-ask to fix root height for this mesh
	SetAskedToFixRootHeightForMesh(PreviewMesh, false);
	
	// update any editors attached to this asset
	BroadcastPreviewMeshReplaced(SourceOrTarget);
}

USkeletalMesh* UTLLRN_IKRetargeterController::GetPreviewMesh(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	
	// can't preview anything if target IK Rig is null
	const UTLLRN_IKRigDefinition* IKRig = GetIKRig(SourceOrTarget);
	if (!IKRig)
	{
		return nullptr;
	}

	// optionally prefer override if one is provided
	const TSoftObjectPtr<USkeletalMesh> PreviewMesh = SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? Asset->SourcePreviewMesh : Asset->TargetPreviewMesh;
	if (!PreviewMesh.IsNull())
	{
		return PreviewMesh.LoadSynchronous();
	}

	// fallback to preview mesh from IK Rig asset
	return IKRig->GetPreviewMesh();
}

FTLLRN_TargetRootSettings UTLLRN_IKRetargeterController::GetRootSettings() const
{
	FScopeLock Lock(&ControllerLock);
	return GetAsset()->GetRootSettingsUObject()->Settings;
}

void UTLLRN_IKRetargeterController::SetRootSettings(const FTLLRN_TargetRootSettings& RootSettings) const
{
	FScopeLock Lock(&ControllerLock);
	FScopedTransaction Transaction(LOCTEXT("SetRootSettings_Transaction", "Set Root Settings"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	GetAsset()->Modify();
	GetAsset()->GetRootSettingsUObject()->Settings = RootSettings;
}

FTLLRN_RetargetGlobalSettings UTLLRN_IKRetargeterController::GetGlobalSettings() const
{
	FScopeLock Lock(&ControllerLock);
	return GetAsset()->GetGlobalSettingsUObject()->Settings;
}

void UTLLRN_IKRetargeterController::SetGlobalSettings(const FTLLRN_RetargetGlobalSettings& GlobalSettings) const
{
	FScopeLock Lock(&ControllerLock);
	FScopedTransaction Transaction(LOCTEXT("SetGlobalSettings_Transaction", "Set Global Settings"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	GetAsset()->Modify();
	GetAsset()->GetGlobalSettingsUObject()->Settings = GlobalSettings;
}

FTLLRN_TargetChainSettings UTLLRN_IKRetargeterController::GetRetargetChainSettings(const FName& TargetChainName) const
{
	FScopeLock Lock(&ControllerLock);
	
	const UTLLRN_RetargetChainSettings* ChainSettings = GetChainSettings(TargetChainName);
	if (!ChainSettings)
	{
		return FTLLRN_TargetChainSettings();
	}

	return ChainSettings->Settings;
}

bool UTLLRN_IKRetargeterController::SetRetargetChainSettings(const FName& TargetChainName, const FTLLRN_TargetChainSettings& Settings) const
{
	FScopeLock Lock(&ControllerLock);

	FScopedTransaction Transaction(LOCTEXT("SetChainSettings_Transaction", "Set Chain Settings"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	
	if (UTLLRN_RetargetChainSettings* ChainSettings = GetChainSettings(TargetChainName))
	{
		ChainSettings->Modify();
		ChainSettings->Settings = Settings;
		return true;
	}

	return false;
}

int32 UTLLRN_IKRetargeterController::AddRetargetOp(TSubclassOf<UTLLRN_RetargetOpBase> InOpClass) const
{
	check(Asset)

	if (!InOpClass)
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Could not add Op to stack. Invalid Op class specified."));
		return INDEX_NONE;
	}

	FScopedTransaction Transaction(LOCTEXT("AddRetargetOp_Label", "Add Retarget Op"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	Asset->OpStack->Modify();
	UTLLRN_RetargetOpBase* NewOp = NewObject<UTLLRN_RetargetOpBase>(Asset, InOpClass, NAME_None, RF_Transactional);
	NewOp->OnAddedToStack(GetAsset());
	return Asset->OpStack->RetargetOps.Add(NewOp);
}

bool UTLLRN_IKRetargeterController::RemoveRetargetOp(const int32 OpIndex) const
{
	check(Asset)
	
	if (!Asset->OpStack->RetargetOps.IsValidIndex(OpIndex))
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Retarget Op not removed. Invalid index, %d."), OpIndex);
		return false;
	}

	FScopedTransaction Transaction(LOCTEXT("RemoveRetargetOp_Label", "Remove Retarget Op"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	Asset->OpStack->Modify();
	Asset->OpStack->RetargetOps.RemoveAt(OpIndex);
	return true;
}

bool UTLLRN_IKRetargeterController::RemoveAllOps() const
{
	check(Asset)

	FScopedTransaction Transaction(LOCTEXT("RemoveAllRetargetOps_Label", "Remove All Retarget Ops"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	Asset->OpStack->Modify();
	Asset->OpStack->RetargetOps.Empty();
	return true;
}

UTLLRN_RetargetOpBase* UTLLRN_IKRetargeterController::GetRetargetOpAtIndex(int32 Index) const
{
	check(Asset)

	if (Asset->OpStack->RetargetOps.IsValidIndex(Index))
	{
		return Asset->OpStack->RetargetOps[Index];
	}
	
	return nullptr;
}

int32 UTLLRN_IKRetargeterController::GetIndexOfRetargetOp(UTLLRN_RetargetOpBase* RetargetOp) const
{
	check(Asset)
	return Asset->OpStack->RetargetOps.Find(RetargetOp);
}

int32 UTLLRN_IKRetargeterController::GetNumRetargetOps() const
{
	check(Asset)
	return Asset->OpStack->RetargetOps.Num();
}

bool UTLLRN_IKRetargeterController::MoveRetargetOpInStack(int32 OpToMoveIndex, int32 TargetIndex) const
{
	TArray<TObjectPtr<UTLLRN_RetargetOpBase>>& RetargetOps = Asset->OpStack->RetargetOps;
	
	if (!RetargetOps.IsValidIndex(OpToMoveIndex))
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Retarget Op not moved. Invalid source index, %d."), OpToMoveIndex);
		return false;
	}

	if (!RetargetOps.IsValidIndex(TargetIndex))
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Retarget Op not moved. Invalid target index, %d."), TargetIndex);
		return false;
	}

	if (OpToMoveIndex == TargetIndex)
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Retarget Op not moved. Source and target index cannot be the same."));
		return false;
	}

	FScopedTransaction Transaction(LOCTEXT("ReorderRetargetOps_Label", "Reorder Retarget Ops"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	Asset->OpStack->Modify();
	UTLLRN_RetargetOpBase* OpToMove = RetargetOps[OpToMoveIndex];
	RetargetOps.Insert(OpToMove, TargetIndex + 1);
	const int32 OpToRemove = TargetIndex > OpToMoveIndex ? OpToMoveIndex : OpToMoveIndex + 1;
	RetargetOps.RemoveAt(OpToRemove);
	return true;
}

bool UTLLRN_IKRetargeterController::SetRetargetOpEnabled(int32 RetargetOpIndex, bool bIsEnabled) const
{
	if (!Asset->OpStack->RetargetOps.IsValidIndex(RetargetOpIndex))
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Retarget op not found. Invalid index, %d."), RetargetOpIndex);
		return false;
	}
	
	FScopedTransaction Transaction(LOCTEXT("SetRetargetOpEnabled_Label", "Enable/Disable Op"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	UTLLRN_RetargetOpBase* OpToMove = Asset->OpStack->RetargetOps[RetargetOpIndex];
	OpToMove->Modify();
	OpToMove->bIsEnabled = bIsEnabled;
	return true;
}

bool UTLLRN_IKRetargeterController::GetRetargetOpEnabled(int32 RetargetOpIndex) const
{
	if (!Asset->OpStack->RetargetOps.IsValidIndex(RetargetOpIndex))
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Invalid retarget op index, %d."), RetargetOpIndex);
		return false;
	}

	return Asset->OpStack->RetargetOps[RetargetOpIndex]->bIsEnabled;
}

bool UTLLRN_IKRetargeterController::GetAskedToFixRootHeightForMesh(USkeletalMesh* Mesh) const
{
	return GetAsset()->MeshesAskedToFixRootHeightFor.Contains(Mesh);
}

void UTLLRN_IKRetargeterController::SetAskedToFixRootHeightForMesh(USkeletalMesh* Mesh, bool InAsked) const
{
	FScopeLock Lock(&ControllerLock);
	if (InAsked)
	{
		GetAsset()->MeshesAskedToFixRootHeightFor.Add(Mesh);
	}
	else
	{
		GetAsset()->MeshesAskedToFixRootHeightFor.Remove(Mesh);
	}
}

FName UTLLRN_IKRetargeterController::GetRetargetRootBone(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	const UTLLRN_IKRigDefinition* IKRig = GetIKRig(SourceOrTarget);
	return IKRig ? IKRig->GetRetargetRoot() : FName("None");
}

void UTLLRN_IKRetargeterController::GetChainNames(const ETLLRN_RetargetSourceOrTarget SourceOrTarget, TArray<FName>& OutNames) const
{
	if (const UTLLRN_IKRigDefinition* IKRig = GetIKRig(SourceOrTarget))
	{
		const TArray<FTLLRN_BoneChain>& Chains = IKRig->GetRetargetChains();
		for (const FTLLRN_BoneChain& Chain : Chains)
		{
			OutNames.Add(Chain.ChainName);
		}
	}
}

void UTLLRN_IKRetargeterController::CleanChainMapping() const
{
	if (IsValid(Asset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target)))
	{
		TArray<FName> TargetChainNames;
		GetChainNames(ETLLRN_RetargetSourceOrTarget::Target, TargetChainNames);

		// remove all target chains that are no longer in the target IK rig asset
		TArray<FName> TargetChainsToRemove;
		for (const UTLLRN_RetargetChainSettings* ChainMap : Asset->ChainSettings)
		{
			if (!TargetChainNames.Contains(ChainMap->TargetChain))
			{
				TargetChainsToRemove.Add(ChainMap->TargetChain);
			}
		}
		for (FName TargetChainToRemove : TargetChainsToRemove)
		{
			Asset->ChainSettings.RemoveAll([&TargetChainToRemove](const UTLLRN_RetargetChainSettings* Element)
			{
				return Element->TargetChain == TargetChainToRemove;
			});
		}

		// add a mapping for each chain that is in the target IK rig (if it doesn't have one already)
		for (FName TargetChainName : TargetChainNames)
		{
			const bool HasChain = Asset->ChainSettings.ContainsByPredicate([&TargetChainName](const UTLLRN_RetargetChainSettings* Element)
			{
				return Element->TargetChain == TargetChainName;
			});
		
			if (!HasChain)
			{
				TObjectPtr<UTLLRN_RetargetChainSettings> ChainMap = NewObject<UTLLRN_RetargetChainSettings>(Asset, UTLLRN_RetargetChainSettings::StaticClass(), NAME_None, RF_Transactional);
				ChainMap->TargetChain = TargetChainName;
				Asset->ChainSettings.Add(ChainMap);
			}
		}
	}

	if (IsValid(Asset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Source)))
	{
		TArray<FName> SourceChainNames;
		GetChainNames(ETLLRN_RetargetSourceOrTarget::Source,SourceChainNames);
	
		// reset any sources that are no longer present to "None"
		for (UTLLRN_RetargetChainSettings* ChainMap : Asset->ChainSettings)
		{
			if (!SourceChainNames.Contains(ChainMap->SourceChain))
			{
				ChainMap->SourceChain = NAME_None;
			}
		}
	}

	// enforce the chain order based on the StartBone index
	SortChainMapping();
}

void UTLLRN_IKRetargeterController::CleanPoseList(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	const UTLLRN_IKRigDefinition* IKRig = GetIKRig(SourceOrTarget);
	if (!IKRig)
	{
		return;
	}
	
	// remove all bone offsets that are no longer part of the skeleton
	const TArray<FName> AllowedBoneNames = IKRig->GetSkeleton().BoneNames;
	TMap<FName, FTLLRN_IKRetargetPose>& RetargetPoses = GetRetargetPoses(SourceOrTarget);
	for (TTuple<FName, FTLLRN_IKRetargetPose>& Pose : RetargetPoses)
	{
		// find bone offsets no longer in target skeleton
		TArray<FName> BonesToRemove;
		for (TTuple<FName, FQuat>& BoneOffset : Pose.Value.BoneRotationOffsets)
		{
			if (!AllowedBoneNames.Contains(BoneOffset.Key))
			{
				BonesToRemove.Add(BoneOffset.Key);
			}
		}
		
		// remove bone offsets
		for (const FName& BoneToRemove : BonesToRemove)
		{
			Pose.Value.BoneRotationOffsets.Remove(BoneToRemove);
		}

		// sort the pose offset from leaf to root
		Pose.Value.SortHierarchically(IKRig->GetSkeleton());
	}
}

void UTLLRN_IKRetargeterController::AutoMapChains(const ETLLRN_AutoMapChainType AutoMapType, const bool bForceRemap) const
{
	FScopeLock Lock(&ControllerLock);
	FScopedTransaction Transaction(LOCTEXT("AutoMapRetargetChains", "Auto-Map Retarget Chains"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	
	CleanChainMapping();

	// get names of all the potential chains we could map to on the source
	TArray<FName> SourceChainNames;
	GetChainNames(ETLLRN_RetargetSourceOrTarget::Source, SourceChainNames);
	
	// iterate over all the chain mappings and find matching source chain
	for (UTLLRN_RetargetChainSettings* ChainMap : Asset->ChainSettings)
	{
		const bool bIsMappedAlready = ChainMap->SourceChain != NAME_None;
		if (bIsMappedAlready && !bForceRemap)
		{
			continue; // already set by user
		}

		ChainMap->Modify();

		// find a source chain to map to
		int32 SourceChainIndexToMapTo = -1;

		switch (AutoMapType)
		{
		case ETLLRN_AutoMapChainType::Fuzzy:
			{
				// auto-map chains using a fuzzy string comparison
				FString TargetNameLowerCase = ChainMap->TargetChain.ToString().ToLower();
				float HighestScore = 0.2f;
				for (int32 ChainIndex=0; ChainIndex<SourceChainNames.Num(); ++ChainIndex)
				{
					FString SourceNameLowerCase = SourceChainNames[ChainIndex].ToString().ToLower();
					float WorstCase = TargetNameLowerCase.Len() + SourceNameLowerCase.Len();
					WorstCase = WorstCase < 1.0f ? 1.0f : WorstCase;
					const float Score = 1.0f - (Algo::LevenshteinDistance(TargetNameLowerCase, SourceNameLowerCase) / WorstCase);
					if (Score > HighestScore)
					{
						HighestScore = Score;
						SourceChainIndexToMapTo = ChainIndex;
					}
				}
				break;
			}
		case ETLLRN_AutoMapChainType::Exact:
			{
				// if no exact match is found, then set to None
				ChainMap->SourceChain = NAME_None;
				
				// auto-map chains with EXACT same name
				for (int32 ChainIndex=0; ChainIndex<SourceChainNames.Num(); ++ChainIndex)
				{
					if (SourceChainNames[ChainIndex] == ChainMap->TargetChain)
					{
						SourceChainIndexToMapTo = ChainIndex;
						break;
					}
				}
				break;
			}
		case ETLLRN_AutoMapChainType::Clear:
			{
				ChainMap->SourceChain = NAME_None;
				break;
			}
		default:
			checkNoEntry();
		}

		// apply source if any decent matches were found
		if (SourceChainNames.IsValidIndex(SourceChainIndexToMapTo))
		{
			ChainMap->SourceChain = SourceChainNames[SourceChainIndexToMapTo];
		}
	}

	// sort them
	SortChainMapping();
}

void UTLLRN_IKRetargeterController::HandleRetargetChainAdded(UTLLRN_IKRigDefinition* IKRig) const
{
	const bool bIsTargetRig = IKRig == Asset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
	if (!bIsTargetRig)
	{
		// if a source chain is added, it will simply be available as a new option, no need to reinitialize until it's used
		return;
	}

	// add the new chain to the mapping data
	CleanChainMapping();
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
}

void UTLLRN_IKRetargeterController::HandleRetargetChainRenamed(UTLLRN_IKRigDefinition* IKRig, FName OldChainName, FName NewChainName) const
{
	const bool bIsSourceRig = IKRig == Asset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Source);
	const bool bIsTargetRig = IKRig == Asset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
	check(bIsSourceRig || bIsTargetRig)
	for (UTLLRN_RetargetChainSettings* ChainMap : Asset->ChainSettings)
	{
		FName& ChainNameToUpdate = bIsSourceRig ? ChainMap->SourceChain : ChainMap->TargetChain;
		if (ChainNameToUpdate == OldChainName)
		{
			FScopedTransaction Transaction(LOCTEXT("RetargetChainRenamed_Label", "Retarget Chain Renamed"));
			ChainMap->Modify();
			ChainNameToUpdate = NewChainName;
			FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
			return;
		}
	}
}

void UTLLRN_IKRetargeterController::HandleRetargetChainRemoved(UTLLRN_IKRigDefinition* IKRig, const FName& InChainRemoved) const
{
	FScopedTransaction Transaction(LOCTEXT("RetargetChainRemoved_Label", "Retarget Chain Removed"));
	Asset->Modify();
	
	const bool bIsSourceRig = IKRig == Asset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Source);
	const bool bIsTargetRig = IKRig == Asset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
	check(bIsSourceRig || bIsTargetRig)
	
	// set source chain name to NONE if it has been deleted 
	if (bIsSourceRig)
	{
		for (UTLLRN_RetargetChainSettings* ChainMap : Asset->ChainSettings)
		{
			if (ChainMap->SourceChain == InChainRemoved)
			{
				ChainMap->SourceChain = NAME_None;
				FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
				return;
			}
		}
		return;
	}
	
	// remove target mapping if the target chain has been removed
	const int32 ChainIndex = Asset->ChainSettings.IndexOfByPredicate([&InChainRemoved](const UTLLRN_RetargetChainSettings* ChainMap)
	{
		return ChainMap->TargetChain == InChainRemoved;
	});
	
	if (ChainIndex != INDEX_NONE)
	{
		Asset->ChainSettings.RemoveAt(ChainIndex);
		FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	}
}

bool UTLLRN_IKRetargeterController::SetSourceChain(FName SourceChainName, FName TargetChainName) const
{
	FScopeLock Lock(&ControllerLock);
	UTLLRN_RetargetChainSettings* ChainSettings = GetChainSettings(TargetChainName);
	if (!ChainSettings)
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Could not map target chain. Target chain not found, %s."), *TargetChainName.ToString());
		return false;
	}
	
	FScopedTransaction Transaction(LOCTEXT("SetRetargetChainSource", "Set Retarget Chain Source"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	ChainSettings->Modify();
	ChainSettings->SourceChain = SourceChainName;
	return true;
}

FName UTLLRN_IKRetargeterController::GetSourceChain(const FName& TargetChainName) const
{
	const UTLLRN_RetargetChainSettings* ChainSettings = GetChainSettings(TargetChainName);
	if (!ChainSettings)
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Could not map target chain. Target chain not found, %s."), *TargetChainName.ToString());
		return NAME_None;
	}
	
	return ChainSettings->SourceChain;
}

const TArray<UTLLRN_RetargetChainSettings*>& UTLLRN_IKRetargeterController::GetAllChainSettings() const
{
	return Asset->ChainSettings;
}

FName UTLLRN_IKRetargeterController::GetChainGoal(const TObjectPtr<UTLLRN_RetargetChainSettings> ChainSettings) const
{
	if (!ChainSettings)
	{
		return NAME_None;
	}
	
	const UTLLRN_IKRigDefinition* TargetIKRig = GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
	if (!TargetIKRig)
	{
		return NAME_None;
	}

	const UTLLRN_IKRigController* RigController = UTLLRN_IKRigController::GetController(TargetIKRig);
	return RigController->GetRetargetChainGoal(ChainSettings->TargetChain);
}

bool UTLLRN_IKRetargeterController::IsChainGoalConnectedToASolver(const FName& GoalName) const
{
	const UTLLRN_IKRigDefinition* TargetIKRig = GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
	if (!TargetIKRig)
	{
		return false;
	}

	const UTLLRN_IKRigController* RigController = UTLLRN_IKRigController::GetController(TargetIKRig);
	return RigController->IsGoalConnectedToAnySolver(GoalName);
}

FName UTLLRN_IKRetargeterController::CreateRetargetPose(const FName& NewPoseName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	FScopedTransaction Transaction(LOCTEXT("CreateRetargetPose", "Create Retarget Pose"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	Asset->Modify();

	// create a new pose with a unique name 
	const FName UniqueNewPoseName = MakePoseNameUnique(NewPoseName.ToString(), SourceOrTarget);
	GetRetargetPoses(SourceOrTarget).Add(UniqueNewPoseName);

	// set new pose as the current pose
	FName& CurrentRetargetPoseName = SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? Asset->CurrentSourceRetargetPose : Asset->CurrentTargetRetargetPose;
	CurrentRetargetPoseName = UniqueNewPoseName;

	return UniqueNewPoseName;
}

bool UTLLRN_IKRetargeterController::RemoveRetargetPose(const FName& PoseToRemove, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	if (PoseToRemove == Asset->GetDefaultPoseName())
	{
		return false; // cannot remove default pose
	}

	TMap<FName, FTLLRN_IKRetargetPose>& Poses = GetRetargetPoses(SourceOrTarget);
	if (!Poses.Contains(PoseToRemove))
	{
		return false; // cannot remove pose that doesn't exist
	}

	FScopeLock Lock(&ControllerLock);
	FScopedTransaction Transaction(LOCTEXT("RemoveRetargetPose", "Remove Retarget Pose"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	Asset->Modify();

	Poses.Remove(PoseToRemove);

	// did we remove the currently used pose?
	if (GetCurrentRetargetPoseName(SourceOrTarget) == PoseToRemove)
	{
		SetCurrentRetargetPose(UTLLRN_IKRetargeter::GetDefaultPoseName(), SourceOrTarget);
	}
	
	return true;
}

FName UTLLRN_IKRetargeterController::DuplicateRetargetPose( const FName PoseToDuplicate, const FName NewPoseName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	TMap<FName, FTLLRN_IKRetargetPose>& Poses = GetRetargetPoses(SourceOrTarget);
	if (!Poses.Contains(PoseToDuplicate))
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Trying to duplicate pose that does not exist, %s."), *PoseToDuplicate.ToString());
		return NAME_None; // cannot duplicate pose that doesn't exist
	}

	FScopeLock Lock(&ControllerLock);
	FScopedTransaction Transaction(LOCTEXT("DuplicateRetargetPose", "Duplicate Retarget Pose"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	Asset->Modify();

	// create a new pose with a unique name
	const FName UniqueNewPoseName = MakePoseNameUnique(NewPoseName.ToString(), SourceOrTarget);
	FTLLRN_IKRetargetPose& NewPose = Poses.Add(UniqueNewPoseName);
	// duplicate the pose data
	NewPose.RootTranslationOffset = Poses[PoseToDuplicate].RootTranslationOffset;
	NewPose.BoneRotationOffsets = Poses[PoseToDuplicate].BoneRotationOffsets;

	// set duplicate to be the current pose
	FName& CurrentRetargetPoseName = SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? Asset->CurrentSourceRetargetPose : Asset->CurrentTargetRetargetPose;
	CurrentRetargetPoseName = UniqueNewPoseName;
	return UniqueNewPoseName;
}

bool UTLLRN_IKRetargeterController::RenameRetargetPose(const FName OldPoseName, const FName NewPoseName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	
	// does the old pose exist?
	if (!GetRetargetPoses(SourceOrTarget).Contains(OldPoseName))
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Trying to rename pose that does not exist, %s."), *OldPoseName.ToString());
		return false;
	}

	// do not allow renaming the default pose (this is disallowed from the UI, but must be done here as well for API usage)
	if (OldPoseName == UTLLRN_IKRetargeter::GetDefaultPoseName())
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Trying to rename the default pose. This is not allowed."));
    	return false;
	}

	// check if we're renaming the current pose
	const bool bWasCurrentPose = GetCurrentRetargetPoseName(SourceOrTarget) == OldPoseName;
	
	FScopedTransaction Transaction(LOCTEXT("RenameRetargetPose", "Rename Retarget Pose"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	Asset->Modify();

	// make sure new name is unique
	const FName UniqueNewPoseName = MakePoseNameUnique(NewPoseName.ToString(), SourceOrTarget);

	// replace key in the map
	TMap<FName, FTLLRN_IKRetargetPose>& Poses = GetRetargetPoses(SourceOrTarget);
	const FTLLRN_IKRetargetPose OldPoseData = Poses[OldPoseName];
	Poses.Remove(OldPoseName);
	Poses.Shrink();
	Poses.Add(UniqueNewPoseName, OldPoseData);

	// make this the current retarget pose, iff the old one was
	if (bWasCurrentPose)
	{
		SetCurrentRetargetPose(UniqueNewPoseName, SourceOrTarget);
	}
	return true;
}

void UTLLRN_IKRetargeterController::ResetRetargetPose(
	const FName& PoseToReset,
	const TArray<FName>& BonesToReset,
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	
	TMap<FName, FTLLRN_IKRetargetPose>& Poses = GetRetargetPoses(SourceOrTarget);
	if (!Poses.Contains(PoseToReset))
	{
		return; // cannot reset pose that doesn't exist
	}
	
	FTLLRN_IKRetargetPose& PoseToEdit = Poses[PoseToReset];

	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	
	if (BonesToReset.IsEmpty())
	{
		FScopedTransaction Transaction(LOCTEXT("ResetRetargetPose", "Reset Retarget Pose"));
		Asset->Modify();

		PoseToEdit.BoneRotationOffsets.Reset();
		PoseToEdit.RootTranslationOffset = FVector::ZeroVector;
	}
	else
	{
		FScopedTransaction Transaction(LOCTEXT("ResetRetargetBonePose", "Reset Bone Pose"));
		Asset->Modify();
		
		const FName RootBoneName = GetRetargetRootBone(SourceOrTarget);
		for (const FName& BoneToReset : BonesToReset)
		{
			if (PoseToEdit.BoneRotationOffsets.Contains(BoneToReset))
			{
				PoseToEdit.BoneRotationOffsets.Remove(BoneToReset);
			}

			if (BoneToReset == RootBoneName)
			{
				PoseToEdit.RootTranslationOffset = FVector::ZeroVector;	
			}
		}
	}
}

FName UTLLRN_IKRetargeterController::GetCurrentRetargetPoseName(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	return SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? GetAsset()->CurrentSourceRetargetPose : GetAsset()->CurrentTargetRetargetPose;
}

bool UTLLRN_IKRetargeterController::SetCurrentRetargetPose(FName NewCurrentPose, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	
	const TMap<FName, FTLLRN_IKRetargetPose>& Poses = GetRetargetPoses(SourceOrTarget);
	if (!Poses.Contains(NewCurrentPose))
	{
		UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Trying to set current pose to a pose that does not exist, %s."), *NewCurrentPose.ToString());
		return false;
	}
	
	FScopedTransaction Transaction(LOCTEXT("SetCurrentPose", "Set Current Pose"));
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	Asset->Modify();
	FName& CurrentPose = SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? Asset->CurrentSourceRetargetPose : Asset->CurrentTargetRetargetPose;
	CurrentPose = NewCurrentPose;
	return true;
}

TMap<FName, FTLLRN_IKRetargetPose>& UTLLRN_IKRetargeterController::GetRetargetPoses(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	return SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? GetAsset()->SourceRetargetPoses : GetAsset()->TargetRetargetPoses;
}

FTLLRN_IKRetargetPose& UTLLRN_IKRetargeterController::GetCurrentRetargetPose(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	return GetRetargetPoses(SourceOrTarget)[GetCurrentRetargetPoseName(SourceOrTarget)];
}

void UTLLRN_IKRetargeterController::SetRotationOffsetForRetargetPoseBone(
	const FName& BoneName,
	const FQuat& RotationOffset,
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	FTLLRN_IKRetargetPose& Pose = GetCurrentRetargetPose(SourceOrTarget);
	Pose.SetDeltaRotationForBone(BoneName, RotationOffset);
	const UTLLRN_IKRigDefinition* IKRig = GetAsset()->GetIKRig(SourceOrTarget);
	Pose.SortHierarchically(IKRig->GetSkeleton());
}

FQuat UTLLRN_IKRetargeterController::GetRotationOffsetForRetargetPoseBone(
	const FName& BoneName,
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	
	TMap<FName, FQuat>& BoneOffsets = GetCurrentRetargetPose(SourceOrTarget).BoneRotationOffsets;
	if (!BoneOffsets.Contains(BoneName))
	{
		return FQuat::Identity;
	}
	
	return BoneOffsets[BoneName];
}

void UTLLRN_IKRetargeterController::SetRootOffsetInRetargetPose(
	const FVector& TranslationOffset,
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	GetCurrentRetargetPose(SourceOrTarget).AddToRootTranslationDelta(TranslationOffset);
}

FVector UTLLRN_IKRetargeterController::GetRootOffsetInRetargetPose(
	const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FScopeLock Lock(&ControllerLock);
	return GetCurrentRetargetPose(SourceOrTarget).GetRootTranslationDelta();
}

void UTLLRN_IKRetargeterController::AutoAlignAllBones(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	// undo transaction
	constexpr bool bShouldTransact = true;
	FScopedTransaction Transaction(LOCTEXT("AutoAlignAllBones", "Auto Align All Bones"), bShouldTransact);
	Asset->Modify();
	
	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);

	// first reset the entire retarget pose
	ResetRetargetPose(GetCurrentRetargetPoseName(SourceOrTarget), TArray<FName>(), SourceOrTarget);
	
	// suppress warnings about bones that cannot be aligned when aligning ALL bones
	constexpr bool bSuppressWarnings = true;
	AutoPoseGenerator.Get()->AlignAllBones(SourceOrTarget, bSuppressWarnings);
}

void UTLLRN_IKRetargeterController::AutoAlignBones(
	const TArray<FName>& BonesToAlign,
	const ETLLRN_RetargetAutoAlignMethod Method,
	ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	// undo transaction
	constexpr bool bShouldTransact = true;
	FScopedTransaction Transaction(LOCTEXT("AutoAlignSelectedBones", "Auto Align Selected Bones"), bShouldTransact);
	Asset->Modify();

	FTLLRN_ScopedReinitializeTLLRN_IKRetargeter Reinitialize(this);
	
	// allow warnings about bones that cannot be aligned when bones are explicitly specified by user
	constexpr bool bSuppressWarnings = false;
	AutoPoseGenerator.Get()->AlignBones(
		BonesToAlign,
		Method,
		SourceOrTarget,
		bSuppressWarnings);
}

void UTLLRN_IKRetargeterController::SnapBoneToGround(FName ReferenceBone, ETLLRN_RetargetSourceOrTarget SourceOrTarget)
{
	// undo transaction
	constexpr bool bShouldTransact = true;
	FScopedTransaction Transaction(LOCTEXT("SnapBoneToGround", "Snap Bone to Ground"), bShouldTransact);
	Asset->Modify();

	AutoPoseGenerator.Get()->SnapToGround(ReferenceBone, SourceOrTarget);
}

FName UTLLRN_IKRetargeterController::MakePoseNameUnique(const FString& PoseName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	FString UniqueName = PoseName;
	
	if (UniqueName.IsEmpty())
	{
		UniqueName = Asset->GetDefaultPoseName().ToString();
	}
	
	int32 Suffix = 1;
	const TMap<FName, FTLLRN_IKRetargetPose>& Poses = GetRetargetPoses(SourceOrTarget);
	while (Poses.Contains(FName(UniqueName)))
	{
		UniqueName = PoseName + "_" + FString::FromInt(Suffix);
		++Suffix;
	}
	return FName(UniqueName);
}

UTLLRN_RetargetChainSettings* UTLLRN_IKRetargeterController::GetChainSettings(const FName& TargetChainName) const
{
	for (UTLLRN_RetargetChainSettings* ChainMap : Asset->ChainSettings)
	{
		if (ChainMap->TargetChain == TargetChainName)
		{
			return ChainMap;
		}
	}

	return nullptr;
}

void UTLLRN_IKRetargeterController::SortChainMapping() const
{
	const UTLLRN_IKRigDefinition* TargetIKRig = Asset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
	if (!IsValid(TargetIKRig))
	{
		return;
	}
	
	Asset->ChainSettings.Sort([TargetIKRig](const UTLLRN_RetargetChainSettings& A, const UTLLRN_RetargetChainSettings& B)
	{
		const TArray<FTLLRN_BoneChain>& BoneChains = TargetIKRig->GetRetargetChains();
		const FTLLRN_IKRigSkeleton& TargetSkeleton = TargetIKRig->GetSkeleton();

		// look for chains
		const int32 IndexA = BoneChains.IndexOfByPredicate([&A](const FTLLRN_BoneChain& Chain)
		{
			return A.TargetChain == Chain.ChainName;
		});

		const int32 IndexB = BoneChains.IndexOfByPredicate([&B](const FTLLRN_BoneChain& Chain)
		{
			return B.TargetChain == Chain.ChainName;
		});

		// compare their StartBone Index 
		if (IndexA > INDEX_NONE && IndexB > INDEX_NONE)
		{
			const int32 StartBoneIndexA = TargetSkeleton.GetBoneIndexFromName(BoneChains[IndexA].StartBone.BoneName);
			const int32 StartBoneIndexB = TargetSkeleton.GetBoneIndexFromName(BoneChains[IndexB].StartBone.BoneName);

			if (StartBoneIndexA == StartBoneIndexB)
			{
				// fallback to sorting alphabetically
				return BoneChains[IndexA].ChainName.LexicalLess(BoneChains[IndexB].ChainName);
			}
				
			return StartBoneIndexA < StartBoneIndexB;	
		}

		// sort them according to the target ik rig if previously failed 
		return IndexA < IndexB;
	});
}

#undef LOCTEXT_NAMESPACE

