// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigEditorController.h"

#include "RigEditor/TLLRN_IKRigAutoCharacterizer.h"
#include "RigEditor/TLLRN_IKRigController.h"
#include "RigEditor/TLLRN_SIKRigHierarchy.h"
#include "RigEditor/TLLRN_SIKRigSolverStack.h"
#include "RigEditor/TLLRN_IKRigAnimInstance.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "RigEditor/TLLRN_SIKRigAssetBrowser.h"
#include "RigEditor/TLLRN_SIKRigOutputLog.h"
#include "Rig/Solvers/TLLRN_IKRig_FBIKSolver.h"
#include "Widgets/Input/SComboBox.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "IPersonaToolkit.h"
#include "SKismetInspector.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Dialog/SCustomDialog.h"
#include "ScopedTransaction.h"
#include "Framework/Notifications/NotificationManager.h"
#include "RigEditor/TLLRN_IKRigAutoFBIK.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRigEditorController)

#if WITH_EDITOR

#include "HAL/PlatformApplicationMisc.h"

#endif

#define LOCTEXT_NAMESPACE "TLLRN_IKRigEditorController"

bool UTLLRN_IKRigBoneDetails::CurrentTransformRelative[3] = {true, true, true};
bool UTLLRN_IKRigBoneDetails::ReferenceTransformRelative[3] = {true, true, true};

TOptional<FTransform> UTLLRN_IKRigBoneDetails::GetTransform(ETLLRN_IKRigTransformType::Type TransformType) const
{
	if(!AnimInstancePtr.IsValid() || !AssetPtr.IsValid())
	{
		return TOptional<FTransform>();
	}
	
	FTransform LocalTransform = FTransform::Identity;
	FTransform GlobalTransform = FTransform::Identity;
	const bool* IsRelative = nullptr;

	const FTLLRN_IKRigSkeleton& Skeleton = AssetPtr->GetSkeleton();
	const int32 BoneIndex = Skeleton.GetBoneIndexFromName(SelectedBone);
	if(BoneIndex == INDEX_NONE)
	{
		return TOptional<FTransform>();
	}

	switch(TransformType)
	{
		case ETLLRN_IKRigTransformType::Current:
		{
			IsRelative = CurrentTransformRelative;
			
			USkeletalMeshComponent* SkeletalMeshComponent = AnimInstancePtr->GetSkelMeshComponent();
			const bool IsSkelMeshValid = SkeletalMeshComponent != nullptr &&
										SkeletalMeshComponent->GetSkeletalMeshAsset() != nullptr;
			if (IsSkelMeshValid)
			{
				GlobalTransform = SkeletalMeshComponent->GetBoneTransform(BoneIndex);
				const TArray<FTransform>& LocalTransforms = SkeletalMeshComponent->GetBoneSpaceTransforms();
				LocalTransform = LocalTransforms.IsValidIndex(BoneIndex) ? LocalTransforms[BoneIndex] : FTransform::Identity;
			}
			else
			{
				GlobalTransform = Skeleton.CurrentPoseGlobal[BoneIndex];
				LocalTransform = Skeleton.CurrentPoseLocal[BoneIndex];
			}
			break;
		}
		case ETLLRN_IKRigTransformType::Reference:
		{
			IsRelative = ReferenceTransformRelative;
			GlobalTransform = Skeleton.RefPoseGlobal[BoneIndex];
			LocalTransform = GlobalTransform;
			const int32 ParentBoneIndex = Skeleton.ParentIndices[BoneIndex];
			if(ParentBoneIndex != INDEX_NONE)
			{
				const FTransform ParentTransform = Skeleton.RefPoseGlobal[ParentBoneIndex];;
				LocalTransform = GlobalTransform.GetRelativeTransform(ParentTransform);
			}
			break;
		}
	}
	checkSlow(IsRelative);

	FTransform Transform = LocalTransform;
	if(!IsRelative[0]) Transform.SetLocation(GlobalTransform.GetLocation());
	if(!IsRelative[1]) Transform.SetRotation(GlobalTransform.GetRotation());
	if(!IsRelative[2]) Transform.SetScale3D(GlobalTransform.GetScale3D());
	return Transform;
}

bool UTLLRN_IKRigBoneDetails::IsComponentRelative(
	ESlateTransformComponent::Type Component,
	ETLLRN_IKRigTransformType::Type TransformType) const
{
	switch(TransformType)
	{
		case ETLLRN_IKRigTransformType::Current:
		{
			return CurrentTransformRelative[(int32)Component]; 
		}
		case ETLLRN_IKRigTransformType::Reference:
		{
			return ReferenceTransformRelative[(int32)Component]; 
		}
	}
	return true;
}

void UTLLRN_IKRigBoneDetails::OnComponentRelativeChanged(
	ESlateTransformComponent::Type Component,
	bool bIsRelative,
	ETLLRN_IKRigTransformType::Type TransformType)
{
	switch(TransformType)
	{
		case ETLLRN_IKRigTransformType::Current:
		{
			CurrentTransformRelative[(int32)Component] = bIsRelative;
			break; 
		}
		case ETLLRN_IKRigTransformType::Reference:
		{
			ReferenceTransformRelative[(int32)Component] = bIsRelative;
			break; 
		}
	}
}

#if WITH_EDITOR

void UTLLRN_IKRigBoneDetails::OnCopyToClipboard(ESlateTransformComponent::Type Component, ETLLRN_IKRigTransformType::Type TransformType) const
{
	TOptional<FTransform> Optional = GetTransform(TransformType);
	if(!Optional.IsSet())
	{
		return;
	}

	const FTransform Xfo = Optional.GetValue();
	
	FString Content;
	switch(Component)
	{
	case ESlateTransformComponent::Location:
		{
			GetContentFromData(Xfo.GetLocation(), Content);
			break;
		}
	case ESlateTransformComponent::Rotation:
		{
			GetContentFromData(Xfo.Rotator(), Content);
			break;
		}
	case ESlateTransformComponent::Scale:
		{
			GetContentFromData(Xfo.GetScale3D(), Content);
			break;
		}
	case ESlateTransformComponent::Max:
	default:
		{
			GetContentFromData(Xfo, Content);
			TBaseStructure<FTransform>::Get()->ExportText(Content, &Xfo, &Xfo, nullptr, PPF_None, nullptr);
			break;
		}
	}

	if(!Content.IsEmpty())
	{
		FPlatformApplicationMisc::ClipboardCopy(*Content);
	}
}

void UTLLRN_IKRigBoneDetails::OnPasteFromClipboard(ESlateTransformComponent::Type Component, ETLLRN_IKRigTransformType::Type TransformType)
{
	// paste is not supported yet.
}

#endif

void FTLLRN_RetargetChainAnalyzer::AssignBestGuessName(FTLLRN_BoneChain& Chain, const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton)
{
	// a map of common names used in bone hierarchies, used to derive a best guess name for a retarget chain
	static TMap<FString, TArray<FString>> ChainToBonesMap;
	ChainToBonesMap.Add(FString("Head"), {"head"});
	ChainToBonesMap.Add(FString("Neck"), {"neck"});
	ChainToBonesMap.Add(FString("Leg"), {"leg", "hip", "thigh", "calf", "knee", "foot", "ankle", "toe"});
	ChainToBonesMap.Add(FString("Arm"), {"arm", "clavicle", "shoulder", "elbow", "wrist", "hand"});
	ChainToBonesMap.Add(FString("Spine"), {"spine"});
	
	ChainToBonesMap.Add(FString("Jaw"),{"jaw"});
	ChainToBonesMap.Add(FString("Tail"),{"tail", "tentacle"});
	
	ChainToBonesMap.Add(FString("Thumb"), {"thumb"});
	ChainToBonesMap.Add(FString("Index"), {"index"});
	ChainToBonesMap.Add(FString("Middle"), {"middle"});
	ChainToBonesMap.Add(FString("Ring"), {"ring"});
	ChainToBonesMap.Add(FString("Pinky"), {"pinky"});

	ChainToBonesMap.Add(FString("Root"), {"root"});

	TArray<int32> BonesInChainIndices;
	const bool bIsChainValid = TLLRN_IKRigSkeleton.ValidateChainAndGetBones(Chain, BonesInChainIndices);
	if (!bIsChainValid)
	{
		Chain.ChainName = GetDefaultChainName();
		return;
	}

	// initialize map of chain names with initial scores of 0 for each chain
	TArray<FString> ChainNames;
	ChainToBonesMap.GetKeys(ChainNames);
	TMap<FString, float> ChainScores;
	for (const FString& ChainName : ChainNames)
	{
		ChainScores.Add(*ChainName, 0.f);
	}

	// run text filter on the predefined bone names and record score for each predefined chain name
	for (const int32 ChainBoneIndex : BonesInChainIndices)
	{
		const FName& ChainBoneName = TLLRN_IKRigSkeleton.GetBoneNameFromIndex(ChainBoneIndex);
		const FString ChainBoneNameStr = ChainBoneName.ToString().ToLower();
		for (const FString& ChainName : ChainNames)
		{
			for (const FString& BoneNameToTry : ChainToBonesMap[ChainName])
			{
				TextFilter->SetFilterText(FText::FromString(BoneNameToTry));
				if (TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(ChainBoneNameStr)))
				{
					++ChainScores[ChainName];	
				}
			}
		}
	}

	// find the chain name with the highest score
	float HighestScore = 0.f;
	FString RecommendedChainName = GetDefaultChainName().ToString();
	for (const FString& ChainName : ChainNames)
	{
		if (ChainScores[ChainName] > HighestScore)
		{
			HighestScore = ChainScores[ChainName];
			RecommendedChainName = ChainName;
		}
	}

	// now determine "sidedness" of the chain, and add a suffix if its on the left or right side
	const EChainSide ChainSide = GetSideOfChain(BonesInChainIndices, TLLRN_IKRigSkeleton);
	switch (ChainSide)
	{
	case EChainSide::Left:
		RecommendedChainName = "Left" + RecommendedChainName;
		break;
	case EChainSide::Right:
		RecommendedChainName = "Right" + RecommendedChainName;
		break;
	default:
		break;
	}
	
	Chain.ChainName = FName(RecommendedChainName);
}

FName FTLLRN_RetargetChainAnalyzer::GetDefaultChainName()
{
	static FText NewChainText = LOCTEXT("NewRetargetChainLabel", "NewRetargetChain");
	return FName(*NewChainText.ToString());
}

EChainSide FTLLRN_RetargetChainAnalyzer::GetSideOfChain(
	const TArray<int32>& BoneIndices,
	const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton) const
{
	// check if bones are predominantly named "Left" or "Right"
	auto DoMajorityBonesContainText = [this, &BoneIndices, &TLLRN_IKRigSkeleton](const FString& StrToTest) -> bool
	{
		int32 Score = 0;
		for (const int32 BoneIndex : BoneIndices)
		{
			const FString BoneName = TLLRN_IKRigSkeleton.GetBoneNameFromIndex(BoneIndex).ToString().ToLower();
			if (BoneName.Contains(StrToTest))
			{
				++Score;
			}
		}
		
		return Score > BoneIndices.Num() * 0.5f;
	};
	
	if (DoMajorityBonesContainText("Right"))
	{
		return EChainSide::Right;
	}
	if (DoMajorityBonesContainText("Left"))
	{
		return EChainSide::Left;
	}

	//
	// bones don't have left/right prefix/suffix, so lets fallback on using a spatial test...
	//
	
	// determine "sidedness" of the chain based on the location of the bones (left, right or center of YZ plane)
	float AverageXPositionOfChain = 0.f;
	for (const int32 BoneIndex : BoneIndices)
	{
		AverageXPositionOfChain += TLLRN_IKRigSkeleton.RefPoseGlobal[BoneIndex].GetLocation().X;
	}
	AverageXPositionOfChain /= static_cast<float>(BoneIndices.Num());

	constexpr float CenterThresholdDistance = 1.0f;
	if (FMath::Abs(AverageXPositionOfChain) < CenterThresholdDistance)
	{
		return EChainSide::Center;
	}

	return AverageXPositionOfChain > 0 ? EChainSide::Left :  EChainSide::Right;
}

void FTLLRN_IKRigEditorController::Initialize(TSharedPtr<FTLLRN_IKRigEditorToolkit> Toolkit, const UTLLRN_IKRigDefinition* IKRigAsset)
{
	EditorToolkit = Toolkit;
	AssetController = UTLLRN_IKRigController::GetController(IKRigAsset);
	BoneDetails = NewObject<UTLLRN_IKRigBoneDetails>();
	
	// register callback to be informed when rig asset is modified by editor
	if (!AssetController->OnIKRigNeedsInitialized().IsBoundToObject(this))
	{
		ReinitializeDelegateHandle = AssetController->OnIKRigNeedsInitialized().AddSP(this, &FTLLRN_IKRigEditorController::HandleIKRigNeedsInitialized);
	}
}

void FTLLRN_IKRigEditorController::Close() const
{
	AssetController->OnIKRigNeedsInitialized().Remove(ReinitializeDelegateHandle);
}

UTLLRN_IKRigProcessor* FTLLRN_IKRigEditorController::GetTLLRN_IKRigProcessor() const
{
	if (AnimInstance)
	{
		return AnimInstance->GetCurrentlyRunningProcessor();
	}

	return nullptr;
}

const FTLLRN_IKRigSkeleton* FTLLRN_IKRigEditorController::GetCurrentTLLRN_IKRigSkeleton() const
{
	if (const UTLLRN_IKRigProcessor* Processor = GetTLLRN_IKRigProcessor())
	{
		if (Processor->IsInitialized())
		{
			return &Processor->GetSkeleton();	
		}
	}

	return nullptr;
}

void FTLLRN_IKRigEditorController::HandleIKRigNeedsInitialized(UTLLRN_IKRigDefinition* ModifiedIKRig) const
{
	if (ModifiedIKRig != AssetController->GetAsset())
	{
		return;
	}

	ClearOutputLog();

	// currently running processor needs reinit
	AnimInstance->SetProcessorNeedsInitialized();
	
	// in case the skeletal mesh was swapped out, we need to ensure the preview scene is up-to-date
	USkeletalMesh* NewMesh = AssetController->GetSkeletalMesh();
	const TSharedRef<IPersonaPreviewScene> PreviewScene = EditorToolkit.Pin()->GetPersonaToolkit()->GetPreviewScene();
	const bool bMeshWasSwapped = PreviewScene->GetPreviewMesh() != NewMesh;
	SkelMeshComponent->SetSkeletalMesh(NewMesh);
	if (bMeshWasSwapped)
	{
		PreviewScene->SetPreviewMeshComponent(SkelMeshComponent);
		PreviewScene->SetPreviewMesh(NewMesh);
		AssetController->ResetInitialGoalTransforms();
	}

	// re-initializes the anim instances running in the viewport
	if (AnimInstance)
	{
		// record what anim was playing so we can restore it after reinit
		UAnimationAsset* AnimThatWasPlaying = AnimInstance->GetCurrentAsset();
		const float TimeWhenReset = AnimInstance->GetCurrentTime();
		const bool bWasPlaying = AnimInstance->IsPlaying();
		
		SkelMeshComponent->PreviewInstance = AnimInstance;
		AnimInstance->InitializeAnimation();
		SkelMeshComponent->EnablePreview(true, nullptr);

		// restore previously playing asset
		if (AnimThatWasPlaying)
		{
			AnimInstance->SetAnimationAsset(AnimThatWasPlaying);
			AnimInstance->SetPlaying(bWasPlaying);
			AnimInstance->SetPosition(TimeWhenReset);
		}
	}

	// update the bone details so it can pull on the current data
	BoneDetails->AnimInstancePtr = AnimInstance;
	BoneDetails->AssetPtr = ModifiedIKRig;

	// refresh all views
	{
		if (SolverStackView.IsValid())
		{
			SolverStackView->RefreshStackView();
		}

		if (SkeletonView.IsValid())
		{
			SkeletonView->RefreshTreeView();
		}

		if (DetailsView.IsValid())
		{
			DetailsView->ForceRefresh();
		}

		if (RetargetingView.IsValid())
		{
			RetargetingView->RefreshView();
		}
		
		if (bMeshWasSwapped && AssetBrowserView.IsValid())
		{
			AssetBrowserView.Get()->RefreshView(); // refresh the asset browser to ensure it shows compatible sequences
		}
	}
}

void FTLLRN_IKRigEditorController::HandleDeleteSelectedElements()
{
	FScopedTransaction Transaction(LOCTEXT("DeleteSelectedIKRigElements_Label", "Delete Selected IK Rig Elements"));
	FTLLRN_ScopedReinitializeIKRig Reinitialize(AssetController);
	
	TArray<TSharedPtr<FIKRigTreeElement>> SelectedItems = SkeletonView->GetSelectedItems();
	for (const TSharedPtr<FIKRigTreeElement>& SelectedItem : SelectedItems)
	{
		switch(SelectedItem->ElementType)
		{
		case IKRigTreeElementType::GOAL:
			AssetController->RemoveGoal(SelectedItem->GoalName);
			break;
		case IKRigTreeElementType::SOLVERGOAL:
			AssetController->DisconnectGoalFromSolver(SelectedItem->EffectorGoalName, SelectedItem->EffectorIndex);
			break;
		case IKRigTreeElementType::BONE_SETTINGS:
			AssetController->RemoveBoneSetting(SelectedItem->BoneSettingBoneName, SelectedItem->BoneSettingsSolverIndex);
			break;
		default:
			break; // can't delete anything else
		}
	}
}

void FTLLRN_IKRigEditorController::Reset() const
{
	SkelMeshComponent->ShowReferencePose(true);
	AssetController->ResetGoalTransforms();
}

void FTLLRN_IKRigEditorController::RefreshTreeView() const
{
	if (SkeletonView.IsValid())
	{
		SkeletonView->RefreshTreeView();
	}
}

void FTLLRN_IKRigEditorController::ClearOutputLog() const
{
	if (OutputLogView.IsValid())
	{
		OutputLogView.Get()->ClearLog();
		if (const UTLLRN_IKRigProcessor* Processor = GetTLLRN_IKRigProcessor())
		{
			Processor->Log.Clear();
		}
	}
}

void FTLLRN_IKRigEditorController::AutoGenerateRetargetChains() const
{
	USkeletalMesh* Mesh = AssetController->GetSkeletalMesh();
	if (!Mesh)
	{
		FNotificationInfo Info(LOCTEXT("NoMeshToAutoCharacterize", "No mesh to auto-characterize. Operation cancelled."));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// auto generate a retarget definition
	FTLLRN_AutoCharacterizeResults Results;
	AssetController->AutoGenerateRetargetDefinition(Results);
	
	// notify user of the results of the auto characterization
	if (Results.bUsedTemplate)
	{
		// actually apply the auto-generated retarget definition
		// TODO move this outside the condition once procedural retarget definitions are supported
		AssetController->SetRetargetDefinition(Results.AutoRetargetDefinition.RetargetDefinition);
		
		// notify user of which skeleton was detected
		const FText ScoreAsText = FText::AsPercent(Results.BestPercentageOfTemplateScore);
		const FText NameAsText = FText::FromName(Results.BestTemplateName);
		const FText Message = FText::Format(
			LOCTEXT("AutoCharacterizeResults", "Using {0} template. Skeletal structure matches with {1} accuracy."),
			NameAsText, ScoreAsText);
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info);

		// log all the differences between the template the skeleton
		if (UTLLRN_IKRigProcessor* Processor = GetTLLRN_IKRigProcessor())
		{
			FTLLRN_IKRigLogger& Log = Processor->Log;

			// missing bones
			for (const FName& MissingBone : Results.MissingBones)
			{
				Log.LogWarning(FText::Format(
				LOCTEXT("MissingTemplateBone", "{0} was not found in this skeleton, but is used by {1}."),
				FText::FromName(MissingBone),
				FText::FromName(Results.BestTemplateName)));
			}

			// different parents
			for (const FName& MissingParent : Results.BonesWithMissingParent)
			{
				Log.LogWarning(FText::Format(
				LOCTEXT("DifferentParentTemplateBone", "{0} has a different parent in the template: {1}."),
				FText::FromName(MissingParent),
				FText::FromName(Results.BestTemplateName)));
			}

			// inform user if any chains were expanded beyond what the template defined
			for (TPair<FName, int32> ExpandedChain : Results.ExpandedChains)
			{
				if (ExpandedChain.Value > 0)
				{
					Log.LogWarning(FText::Format(
						LOCTEXT("ExpandedChain", "The '{0}' chain was expanded beyond the template by {1} bones."),
						FText::FromName(ExpandedChain.Key),
						FText::AsNumber(ExpandedChain.Value)));
				}
			}
		}
	}
	else
	{
		// notify user that no skeleton template was used
		// TODO change this message once procedurally generated retarget definitions are provided
		FNotificationInfo Info(LOCTEXT("MissingTemplateCharacterizeSkipped", "No matching skeletal template found. Characterization skipped."));
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void FTLLRN_IKRigEditorController::AutoGenerateFBIK() const
{
	FTLLRN_AutoFBIKResults Results;
	AssetController->AutoGenerateFBIK(Results);

	switch (Results.Outcome)
	{
	case EAutoFBIKResult::AllOk:
		{
			FNotificationInfo Info(LOCTEXT("AutoIKSuccess", "Auto FBIK Successfully Setup."));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		break;
	case EAutoFBIKResult::MissingMesh:
		{
			FNotificationInfo Info(LOCTEXT("AutoIKNoMesh", "No mesh to create IK for. Auto FBIK skipped."));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		break;
	case EAutoFBIKResult::MissingChains:
		{
			FNotificationInfo Info(LOCTEXT("AutoIKMissingChains", "Missing retarget chains. Auto FBIK did not find all expected chains. See output."));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		break;
	case EAutoFBIKResult::UnknownSkeletonType:
		{
			FNotificationInfo Info(LOCTEXT("AutoIKUnknownSkeleton", "Unknown skeleton type. Auto FBIK skipped."));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
	case EAutoFBIKResult::MissingRootBone:
		{
			FNotificationInfo Info(LOCTEXT("AutoIKMissingRootBone", "Auto FBIK setup, but root bone was missing. Please assign a root bone manually."));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
		break;
	default:
		checkNoEntry();
	}
}

void FTLLRN_IKRigEditorController::AddNewGoals(const TArray<FName>& GoalNames, const TArray<FName>& BoneNames)
{
	check(GoalNames.Num() == BoneNames.Num());

	FScopedTransaction Transaction(LOCTEXT("AddNewGoals_Label", "Add New Goals"));
	FTLLRN_ScopedReinitializeIKRig Reinitialize(AssetController);
	AssetController->GetAsset()->Modify();

	// add a default solver if there isn't one already
	const bool bCancelled = PromptToAddDefaultSolver();
	if (bCancelled)
	{
		return;
	}

	// create goals
	FName LastCreatedGoalName = NAME_None;
	for (int32 I=0; I<GoalNames.Num(); ++I)
	{
		const FName& GoalName = GoalNames[I];
		const FName& BoneName = BoneNames[I];

		// create a new goal
		const FName NewGoalName = AssetController->AddNewGoal(GoalName, BoneName);
		if (NewGoalName == NAME_None)
		{
			continue; // already exists
		}

		// ask user if they want to assign this goal to a chain (if there is one on this bone)
		PromptToAssignGoalToChain(NewGoalName);

		// connect the new goal to selected solvers
		TArray<TSharedPtr<FTLLRN_SolverStackElement>> SelectedSolvers;
		GetSelectedSolvers(SelectedSolvers);
		for (TSharedPtr<FTLLRN_SolverStackElement>& Solver : SelectedSolvers)
		{
			AssetController->ConnectGoalToSolver(NewGoalName, Solver.Get()->IndexInStack);
		}

		LastCreatedGoalName = GoalName;
	}
	
	// show last created goal in details view
	if (LastCreatedGoalName != NAME_None)
	{
		ShowDetailsForGoal(LastCreatedGoalName);
	}
}

void FTLLRN_IKRigEditorController::ClearSelection() const
{
	if (SkeletonView.IsValid())
	{
		SkeletonView->TreeView->ClearSelection();	
	}
	
	ShowAssetDetails();
}

void FTLLRN_IKRigEditorController::HandleGoalSelectedInViewport(const FName& GoalName, bool bReplace) const
{
	if (SkeletonView.IsValid())
	{
		SkeletonView->AddSelectedItemFromViewport(GoalName, IKRigTreeElementType::GOAL, bReplace);
		ShowDetailsForElements(SkeletonView->GetSelectedItems());
		return;
	}

	ShowDetailsForGoal(GoalName);
}

void FTLLRN_IKRigEditorController::HandleBoneSelectedInViewport(const FName& BoneName, bool bReplace) const
{
	if (SkeletonView.IsValid())
	{
		SkeletonView->AddSelectedItemFromViewport(BoneName, IKRigTreeElementType::BONE, bReplace);
		ShowDetailsForElements(SkeletonView->GetSelectedItems());
		return;
	}
	
	ShowDetailsForBone(BoneName);
}

void FTLLRN_IKRigEditorController::GetSelectedSolvers(TArray<TSharedPtr<FTLLRN_SolverStackElement>>& OutSelectedSolvers) const
{
	if (SolverStackView.IsValid())
	{
		OutSelectedSolvers.Reset();
		OutSelectedSolvers.Append(SolverStackView->ListView->GetSelectedItems());
	}
}

int32 FTLLRN_IKRigEditorController::GetSelectedSolverIndex()
{
	if (!SolverStackView.IsValid())
	{
		return INDEX_NONE;
	}
	
	TArray<TSharedPtr<FTLLRN_SolverStackElement>> SelectedSolvers = SolverStackView->ListView->GetSelectedItems();
	if (SelectedSolvers.IsEmpty())
	{
		return INDEX_NONE;
	}

	return SelectedSolvers[0]->IndexInStack;
}

void FTLLRN_IKRigEditorController::GetSelectedGoalNames(TArray<FName>& OutGoalNames) const
{
	if (!SkeletonView.IsValid())
	{
		return;
	}

	SkeletonView->GetSelectedGoalNames(OutGoalNames);
}

int32 FTLLRN_IKRigEditorController::GetNumSelectedGoals() const
{
	if (!SkeletonView.IsValid())
	{
		return 0;
	}

	return SkeletonView->GetNumSelectedGoals();
}

void FTLLRN_IKRigEditorController::GetSelectedBoneNames(TArray<FName>& OutBoneNames) const
{
	if (!SkeletonView.IsValid())
	{
		return;
	}

	SkeletonView->GetSelectedBoneNames(OutBoneNames);
}

void FTLLRN_IKRigEditorController::GetSelectedBones(TArray<TSharedPtr<FIKRigTreeElement>>& OutBoneItems) const
{
	if (!SkeletonView.IsValid())
	{
		return;
	}

	SkeletonView->GetSelectedBones(OutBoneItems);
}

bool FTLLRN_IKRigEditorController::IsGoalSelected(const FName& GoalName) const
{
	if (!SkeletonView.IsValid())
	{
		return false;
	}

	return SkeletonView->IsGoalSelected(GoalName);
}

TArray<FName> FTLLRN_IKRigEditorController::GetSelectedChains() const
{
	if (!RetargetingView.IsValid())
	{
		return TArray<FName>();
	}

	return RetargetingView->GetSelectedChains();
}

bool FTLLRN_IKRigEditorController::DoesSkeletonHaveSelectedItems() const
{
	if (!SkeletonView.IsValid())
	{
		return false;
	}
	return SkeletonView->HasSelectedItems();
}

void FTLLRN_IKRigEditorController::GetChainsSelectedInSkeletonView(TArray<FTLLRN_BoneChain>& InOutChains)
{
	if (!SkeletonView.IsValid())
	{
		return;
	}

	SkeletonView->GetSelectedBoneChains(InOutChains);
}

void FTLLRN_IKRigEditorController::CreateNewRetargetChains()
{
	// get selected chains from hierarchy view
	TArray<FTLLRN_BoneChain> SelectedBoneChains;
	if (SkeletonView.IsValid())
	{
		SkeletonView->GetSelectedBoneChains(SelectedBoneChains);
	}
	
	FScopedTransaction Transaction(LOCTEXT("AddMultipleRetargetChains_Label", "Add Retarget Chain(s)"));
	FTLLRN_ScopedReinitializeIKRig Reinitialize(AssetController);
	
	if (!SelectedBoneChains.IsEmpty())
	{
		// create a chain for each selected chain in hierarchy
		for (FTLLRN_BoneChain& BoneChain : SelectedBoneChains)
		{
			ChainAnalyzer.AssignBestGuessName(BoneChain, AssetController->GetTLLRN_IKRigSkeleton());
			PromptToAddNewRetargetChain(BoneChain);
		}
	}
	else
	{
		// create an empty chain
		FTLLRN_BoneChain Chain(FTLLRN_RetargetChainAnalyzer::GetDefaultChainName(), NAME_None, NAME_None, NAME_None);
		PromptToAddNewRetargetChain(Chain);
	}
}

bool FTLLRN_IKRigEditorController::PromptToAddDefaultSolver() const
{
	if (AssetController->GetNumSolvers() > 0)
	{
		return false;
	}

	TArray<TSharedPtr<FTLLRN_IKRigSolverTypeAndName>> SolverTypes;
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;
		if (!Class->IsNative())
		{
			continue;
		}

		if (!ClassIt->IsChildOf(UTLLRN_IKRigSolver::StaticClass()))
		{
			continue;
		}

		if (Class == UTLLRN_IKRigSolver::StaticClass())
		{
			continue; // skip base class
		}

		const UTLLRN_IKRigSolver* SolverCDO = CastChecked<UTLLRN_IKRigSolver>(Class->ClassDefaultObject);
		TSharedPtr<FTLLRN_IKRigSolverTypeAndName> SolverType = MakeShared<FTLLRN_IKRigSolverTypeAndName>();
		SolverType->NiceName = SolverCDO->GetNiceName();
		SolverType->SolverType = TSubclassOf<UTLLRN_IKRigSolver>(Class);
		SolverTypes.Add(SolverType);
	}

	// select "full body IK" by default
	TSharedPtr<FTLLRN_IKRigSolverTypeAndName> SelectedSolver = SolverTypes[0];
	for (TSharedPtr<FTLLRN_IKRigSolverTypeAndName>& SolverType :SolverTypes)
	{
		if (SolverType->SolverType == UTLLRN_IKRigFBIKSolver::StaticClass())
		{
			SelectedSolver = SolverType;
			break;
		}
	}
	
	const TSharedRef<SComboBox<TSharedPtr<FTLLRN_IKRigSolverTypeAndName>>> SolverOptionBox = SNew(SComboBox<TSharedPtr<FTLLRN_IKRigSolverTypeAndName>>)
	.OptionsSource(&SolverTypes)
	.OnGenerateWidget_Lambda([](TSharedPtr<FTLLRN_IKRigSolverTypeAndName> Item)
	{
		return SNew(STextBlock).Text(Item->NiceName);
	})
	.OnSelectionChanged_Lambda([&SelectedSolver](TSharedPtr<FTLLRN_IKRigSolverTypeAndName> Item, ESelectInfo::Type)
	{
		SelectedSolver = Item;
	})
	.Content()
	[
		SNew(STextBlock)
		.MinDesiredWidth(200)
		.Text_Lambda([&SelectedSolver]()
		{
			return SelectedSolver->NiceName;
		})
	];
	
	TSharedRef<SCustomDialog> AddSolverDialog = SNew(SCustomDialog)
		.Title(FText(LOCTEXT("EditorController_IKRigFirstSolver", "Add Default Solver")))
		.Content()
		[
			SolverOptionBox
		]
		.Buttons({
			SCustomDialog::FButton(LOCTEXT("AddSolver", "Add Solver")),
			SCustomDialog::FButton(LOCTEXT("Skip", "Skip"))
	});

	// show window and get result
	const int32 Result = AddSolverDialog->ShowModal();
	const bool bWindowClosed = Result < 0;
	const bool bSkipped = Result == 1;
	
	if (bWindowClosed)
	{
		return true; // window closed
	}

	if (bSkipped)
	{
		return false; // user opted NOT to add a solver
	}

	// add a solver
	if (SelectedSolver->SolverType != nullptr && SolverStackView.IsValid())
	{
		AssetController->AddSolver(SelectedSolver->SolverType);
	}

	// must refresh the view so that subsequent goal operations see a selected solver to connect to
	SolverStackView->RefreshStackView();
	
	return false;
}

void FTLLRN_IKRigEditorController::ShowDetailsForBone(const FName BoneName) const
{
	BoneDetails->SetBone(BoneName);
	DetailsView->SetObject(BoneDetails);
}

void FTLLRN_IKRigEditorController::ShowDetailsForBoneSettings(const FName& BoneName, int32 SolverIndex) const
{
	if (UObject* BoneSettings = AssetController->GetBoneSettings(BoneName, SolverIndex))
	{
		DetailsView->SetObject(BoneSettings);
	}
}

void FTLLRN_IKRigEditorController::ShowDetailsForGoal(const FName& GoalName) const
{
	DetailsView->SetObject(AssetController->GetGoal(GoalName));
}

void FTLLRN_IKRigEditorController::ShowDetailsForGoalSettings(const FName GoalName, const int32 SolverIndex) const
{
	// get solver that owns this effector
	if (const UTLLRN_IKRigSolver* SolverWithEffector = AssetController->GetSolverAtIndex(SolverIndex))
	{
		if (UObject* EffectorSettings = SolverWithEffector->GetGoalSettings(GoalName))
		{
			DetailsView->SetObject(EffectorSettings);
		}
	}
}

void FTLLRN_IKRigEditorController::ShowDetailsForSolver(const int32 SolverIndex) const
{
	DetailsView->SetObject(AssetController->GetSolverAtIndex(SolverIndex));
}

void FTLLRN_IKRigEditorController::ShowAssetDetails() const
{
	DetailsView->SetObject(AssetController->GetAsset());
}

void FTLLRN_IKRigEditorController::ShowDetailsForElements(const TArray<TSharedPtr<FIKRigTreeElement>>& InItems) const
{
	if (!InItems.Num())
	{
		ShowAssetDetails();
		return;
	}

	const TSharedPtr<FIKRigTreeElement>& LastItem = InItems.Last();

	// check is the items are all of the same type
	const bool bContainsSeveralTypes = InItems.ContainsByPredicate( [LastItem](const TSharedPtr<FIKRigTreeElement>& Item)
	{
		return Item->ElementType != LastItem->ElementType;
	});

	// if all elements are similar then treat them once
	if (!bContainsSeveralTypes)
	{
		TArray<TWeakObjectPtr<>> Objects;
		for (const TSharedPtr<FIKRigTreeElement>& Item: InItems)
		{
			TWeakObjectPtr<> Object = Item->GetObject();
			if (Object.IsValid())
			{
				Objects.Add(Object);
			}
		}
		DetailsView->SetObjects(Objects);
		return;
	}

	// fallback to the last selected element
	switch (LastItem->ElementType)
	{
	case IKRigTreeElementType::BONE:
		ShowDetailsForBone(LastItem->BoneName);
		break;
		
	case IKRigTreeElementType::GOAL:
		ShowDetailsForGoal(LastItem->GoalName);
		break;
		
	case IKRigTreeElementType::SOLVERGOAL:
		ShowDetailsForGoalSettings(LastItem->EffectorGoalName, LastItem->EffectorIndex);
		break;
		
	case IKRigTreeElementType::BONE_SETTINGS:
		ShowDetailsForBoneSettings(LastItem->BoneSettingBoneName, LastItem->BoneSettingsSolverIndex);
		break;
		
	default:
		ensure(false);
		break;
	}
}

void FTLLRN_IKRigEditorController::OnFinishedChangingDetails(const FPropertyChangedEvent& PropertyChangedEvent) const
{
	const bool bPreviewMeshChanged = PropertyChangedEvent.GetPropertyName() == UTLLRN_IKRigDefinition::GetPreviewMeshPropertyName();
	if (bPreviewMeshChanged)
	{
		USkeletalMesh* Mesh = AssetController->GetSkeletalMesh();
		AssetController->SetSkeletalMesh(Mesh);
	}
}

bool FTLLRN_IKRigEditorController::IsObjectInDetailsView(const UObject* Object) const
{
	if (!DetailsView.IsValid())
	{
		return false;
	}

	if (!Object)
	{
		return false;
	}
	
	TArray<TWeakObjectPtr<UObject>> SelectedObjects = DetailsView->GetSelectedObjects();
	for (TWeakObjectPtr<UObject> SelectedObject : SelectedObjects)
	{
		if (SelectedObject.Get() == Object)
		{
			return true;
		}
	}

	return false;
}

void FTLLRN_IKRigEditorController::SetDetailsView(const TSharedPtr<IDetailsView>& InDetailsView)
{
	DetailsView = InDetailsView;
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FTLLRN_IKRigEditorController::OnFinishedChangingDetails);
	ShowAssetDetails();
}

void FTLLRN_IKRigEditorController::PromptToAssignGoalToChain(const FName NewGoalName) const
{
	const UTLLRN_IKRigEffectorGoal* NewGoal = AssetController->GetGoal(NewGoalName);
	check(NewGoal);
	const TArray<FTLLRN_BoneChain>& AllRetargetChains = AssetController->GetRetargetChains();
	FName ChainToAddGoalTo = NAME_None;
	for (const FTLLRN_BoneChain& Chain : AllRetargetChains)
	{
		if (Chain.EndBone == NewGoal->BoneName)
		{
			ChainToAddGoalTo = Chain.ChainName;
		}
	}

	if (ChainToAddGoalTo == NAME_None)
	{
		return;
	}

	TSharedRef<SCustomDialog> AddGoalToChainDialog = SNew(SCustomDialog)
		.Title(FText(LOCTEXT("AssignGoalToNewChainTitle", "Assign Existing Goal to Retarget Chain")))
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("AssignGoalToChainLabel", "Assign goal, {0} to retarget chain, {1}?"), FText::FromName(NewGoal->GoalName), FText::FromName(ChainToAddGoalTo)))
		]
		.Buttons({
			SCustomDialog::FButton(LOCTEXT("AssignGoal", "Assign Goal")),
			SCustomDialog::FButton(LOCTEXT("SkipGoal", "Skip Goal"))
	});

	if (AddGoalToChainDialog->ShowModal() != 0)
	{
		return; // cancel button pressed, or window closed
	}
	
	AssetController->SetRetargetChainGoal(ChainToAddGoalTo, NewGoal->GoalName);
}

FName FTLLRN_IKRigEditorController::PromptToAddNewRetargetChain(FTLLRN_BoneChain& BoneChain) const
{
	TArray<SCustomDialog::FButton> Buttons;
	Buttons.Add(SCustomDialog::FButton(LOCTEXT("AddChain", "Add Chain")));

	const bool bHasExistingGoal = BoneChain.IKGoalName != NAME_None;
	if (bHasExistingGoal)
	{
		Buttons.Add(SCustomDialog::FButton(LOCTEXT("AddChainUsingGoal", "Add Chain using Goal")));
	}
	else
	{
		Buttons.Add(SCustomDialog::FButton(LOCTEXT("AddChainAndGoal", "Add Chain and Goal")));
	}
	
	Buttons.Add(SCustomDialog::FButton(LOCTEXT("Cancel", "Cancel")));

	// ask user if they want to add a goal to the end of this chain
	const TSharedRef<SCustomDialog> AddNewRetargetChainDialog =
		SNew(SCustomDialog)
		.Title(FText(LOCTEXT("AddNewChainTitleLabel", "Add New Retarget Chain")))
		.HAlignContent(HAlign_Center)
		.Content()
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2, 1)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					[
						SNew(STextBlock).Text(LOCTEXT("ChainNameLabel", "Chain Name"))
					]

					+SHorizontalBox::Slot()
					[
						SNew(SEditableTextBox)
						.MinDesiredWidth(200.0f)
						.OnTextChanged_Lambda([&BoneChain](const FText& InText)
						{
							BoneChain.ChainName = FName(InText.ToString());
						})
						.Text(FText::FromName(BoneChain.ChainName))
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2, 1)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					[
						SNew(STextBlock).Text(LOCTEXT("StartBoneLabel", "Start Bone"))
					]

					+SHorizontalBox::Slot()
					[
						SNew(SEditableTextBox)
						.Text(FText::FromName(BoneChain.StartBone.BoneName))
						.IsReadOnly(true)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2, 1)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					[
						SNew(STextBlock).Text(LOCTEXT("EndBoneLabel", "End Bone"))
					]

					+SHorizontalBox::Slot()
					[
						SNew(SEditableTextBox)
						.Text(FText::FromName(BoneChain.EndBone.BoneName))
						.IsReadOnly(true)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2, 1)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					[
						SNew(STextBlock).Text(LOCTEXT("GoalLabel", "Goal"))
					]

					+SHorizontalBox::Slot()
					[
						SNew(SEditableTextBox)
						.Text(FText::FromName(BoneChain.IKGoalName))
						.IsReadOnly(true)
					]
				]
			]
			
		]
		.Buttons(Buttons);

	// show the dialog and handle user choice
	const int32 UserChoice = AddNewRetargetChainDialog->ShowModal();
	const bool bUserClosedWindow = UserChoice < 0;
	const bool bUserAddedChainNoGoal = UserChoice == 0;
	const bool bUserAddedChainWithGoal = UserChoice == 1;
	const bool bUserPressedCancel = UserChoice == 2;

	// cancel button pressed, or window closed ?
	if (bUserClosedWindow || bUserPressedCancel)
	{
		return NAME_None;  
	}

	// user opted not to add a goal, so remove it in case one was in the bone chain
	if (bUserAddedChainNoGoal)
	{
		BoneChain.IKGoalName = NAME_None;
	}

	// add the retarget chain
	const FName NewChainName = AssetController->AddRetargetChainInternal(BoneChain);
	
	// did user choose to assign a goal
	if (bUserAddedChainWithGoal)
	{
		FName GoalName;
		if (bHasExistingGoal)
		{
			// use the existing goal
			GoalName = BoneChain.IKGoalName;
		}
		else
		{
			// add a default solver if there isn't one already
			const bool bCancelled = PromptToAddDefaultSolver();
			if (bCancelled)
			{
				// user cancelled creating a goal
				return NewChainName;
			}
			
			// create new goal
			const FName NewGoalName = FName(FText::Format(LOCTEXT("GoalOnNewChainName", "{0}_Goal"), FText::FromName(BoneChain.ChainName)).ToString());
			GoalName = AssetController->AddNewGoal(NewGoalName, BoneChain.EndBone.BoneName);

			// connect the new goal to selected solvers
			TArray<TSharedPtr<FTLLRN_SolverStackElement>> SelectedSolvers;
			GetSelectedSolvers(SelectedSolvers);
			for (TSharedPtr<FTLLRN_SolverStackElement>& Solver : SelectedSolvers)
			{
				AssetController->ConnectGoalToSolver(GoalName, Solver.Get()->IndexInStack);
			}
		}
		
		// assign the existing goal to it
		AssetController->SetRetargetChainGoal(NewChainName, GoalName);
	}
	
	return NewChainName;
}

void FTLLRN_IKRigEditorController::PlayAnimationAsset(UAnimationAsset* AssetToPlay)
{
	if (AssetToPlay && AnimInstance)
	{
		AnimInstance->SetAnimationAsset(AssetToPlay);
	}
}

EIKRigSelectionType FTLLRN_IKRigEditorController::GetLastSelectedType() const
{
	return LastSelectedType;
}

void FTLLRN_IKRigEditorController::SetLastSelectedType(EIKRigSelectionType SelectionType)
{
	LastSelectedType = SelectionType;
}

TObjectPtr<UTLLRN_IKRigBoneDetails> FTLLRN_IKRigEditorController::CreateBoneDetails(const TSharedPtr<FIKRigTreeElement const>& InBoneItem) const
{
	// ensure that the element is related to a bone
	if (InBoneItem->ElementType != IKRigTreeElementType::BONE)
	{
		return nullptr;
	}
	
	// create and store a new one
	UTLLRN_IKRigBoneDetails* NewBoneDetails = NewObject<UTLLRN_IKRigBoneDetails>(AssetController->GetAsset(), FName(InBoneItem->BoneName), RF_Standalone | RF_Transient );
	NewBoneDetails->SelectedBone = InBoneItem->BoneName;
	NewBoneDetails->AnimInstancePtr = AnimInstance;
	NewBoneDetails->AssetPtr = AssetController->GetAsset();
	
	return NewBoneDetails;
}

#undef LOCTEXT_NAMESPACE

