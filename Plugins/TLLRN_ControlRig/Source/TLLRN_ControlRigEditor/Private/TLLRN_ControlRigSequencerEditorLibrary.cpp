// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigSequencerEditorLibrary.h"
#include "LevelSequence.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "UnrealEdGlobals.h"
#include "ISequencer.h"
#include "TLLRN_ControlRigEditorModule.h"
#include "Channels/FloatChannelCurveModel.h"
#include "TransformNoScale.h"
#include "TLLRN_ControlRigComponent.h"
#include "MovieSceneToolHelpers.h"
#include "Rigs/FKTLLRN_ControlRig.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"
#include "TLLRN_ControlRig.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "Engine/SCS_Node.h"
#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "Tools/TLLRN_ControlRigTweener.h"
#include "LevelSequencePlayer.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "Logging/LogMacros.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Exporters/AnimSeqExportOption.h"
#include "MovieSceneTimeHelpers.h"
#include "ScopedTransaction.h"
#include "Sequencer/TLLRN_ControlRigParameterTrackEditor.h"
#include "TLLRN_ControlTLLRN_RigSpaceChannelEditors.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "Tools/ConstraintBaker.h"
#include "TransformConstraint.h"
#include "Rigs/FKTLLRN_ControlRig.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "EditorScriptingHelpers.h"
#include "ConstraintsScripting.h"
#include "Constraints/TLLRN_ControlTLLRN_RigTransformableHandle.h"
#include "Constraints/MovieSceneConstraintChannelHelper.h"
#include "Sections/MovieSceneConstrainedSection.h"
#include "BakingAnimationKeySettings.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "Sequencer/TLLRN_TLLRN_AnimLayers/TLLRN_TLLRN_AnimLayers.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigSequencerEditorLibrary)

#define LOCTEXT_NAMESPACE "ControlrigSequencerEditorLibrary"

TArray<UTLLRN_ControlRig*> UTLLRN_ControlRigSequencerEditorLibrary::GetVisibleTLLRN_ControlRigs()
{
	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
	if (TLLRN_ControlRigEditMode)
	{
		TLLRN_ControlRigs = TLLRN_ControlRigEditMode->GetTLLRN_ControlRigsArray(true /*bIsVisible*/);
	}
	return TLLRN_ControlRigs;
}

TArray<FTLLRN_ControlRigSequencerBindingProxy> UTLLRN_ControlRigSequencerEditorLibrary::GetTLLRN_ControlRigs(ULevelSequence* LevelSequence)
{
	TArray<FTLLRN_ControlRigSequencerBindingProxy> TLLRN_ControlRigBindingProxies;
	if (LevelSequence)
	{
		UMovieScene* MovieScene = LevelSequence->GetMovieScene();
		if (MovieScene)
		{
			const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();
			for (const FMovieSceneBinding& Binding : Bindings)
			{
				TArray<UMovieSceneTrack*> Tracks = MovieScene->FindTracks(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.GetObjectGuid(), NAME_None);
				for (UMovieSceneTrack* AnyOleTrack : Tracks)
				{
					UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(AnyOleTrack);
					if (Track && Track->GetTLLRN_ControlRig())
					{
						FTLLRN_ControlRigSequencerBindingProxy BindingProxy;
						BindingProxy.Track = Track;
						
						BindingProxy.TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(Track->GetTLLRN_ControlRig());
						BindingProxy.Proxy.BindingID = Binding.GetObjectGuid();
						BindingProxy.Proxy.Sequence = LevelSequence;
						TLLRN_ControlRigBindingProxies.Add(BindingProxy);
					}
				}
			}
		}
	}
	return TLLRN_ControlRigBindingProxies;
}

static TArray<UObject*> GetBoundObjects(UWorld* World, ULevelSequence* LevelSequence, const FMovieSceneBindingProxy& Binding, ULevelSequencePlayer** OutPlayer, ALevelSequenceActor** OutActor)
{
	FMovieSceneSequencePlaybackSettings Settings;
	FLevelSequenceCameraSettings CameraSettings;

	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, LevelSequence, Settings, *OutActor);
	*OutPlayer = Player;

	// Evaluation needs to occur in order to obtain spawnables
	Player->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(LevelSequence->GetMovieScene()->GetPlaybackRange().GetLowerBoundValue().Value, EUpdatePositionMethod::Play));

	FMovieSceneSequenceID SequenceId = Player->State.FindSequenceId(LevelSequence);

	FMovieSceneObjectBindingID ObjectBinding = UE::MovieScene::FFixedObjectBindingID(Binding.BindingID, SequenceId);
	TArray<UObject*> BoundObjects = Player->GetBoundObjects(ObjectBinding);
	
	return BoundObjects;
}

static void AcquireSkeletonAndSkelMeshCompFromObject(UObject* BoundObject, USkeleton** OutSkeleton, USkeletalMeshComponent** OutSkeletalMeshComponent)
{
	*OutSkeletalMeshComponent = nullptr;
	*OutSkeleton = nullptr;
	if (AActor* Actor = Cast<AActor>(BoundObject))
	{
		for (UActorComponent* Component : Actor->GetComponents())
		{
			USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Component);
			if (SkeletalMeshComp)
			{
				*OutSkeletalMeshComponent = SkeletalMeshComp;
				if (SkeletalMeshComp->GetSkeletalMeshAsset() && SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton())
				{
					*OutSkeleton = SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton();
				}
				return;
			}
		}

		AActor* ActorCDO = Cast<AActor>(Actor->GetClass()->GetDefaultObject());
		if (ActorCDO)
		{
			for (UActorComponent* Component : ActorCDO->GetComponents())
			{
				USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Component);
				if (SkeletalMeshComp)
				{
					*OutSkeletalMeshComponent = SkeletalMeshComp;
					if (SkeletalMeshComp->GetSkeletalMeshAsset() && SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton())
					{
						*OutSkeleton = SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton();
					}
					return;
				}
			}
		}

		UBlueprintGeneratedClass* ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(Actor->GetClass());
		if (ActorBlueprintGeneratedClass)
		{
			const TArray<USCS_Node*>& ActorBlueprintNodes = ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

			for (USCS_Node* Node : ActorBlueprintNodes)
			{
				if (Node->ComponentClass && Node->ComponentClass->IsChildOf(USkeletalMeshComponent::StaticClass()))
				{
					USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Node->GetActualComponentTemplate(ActorBlueprintGeneratedClass));
					if (SkeletalMeshComp)
					{
						*OutSkeletalMeshComponent = SkeletalMeshComp;
						if (SkeletalMeshComp->GetSkeletalMeshAsset() && SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton())
						{
							*OutSkeleton = SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton();
						}
					}
				}
			}
		}
	}
	else if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(BoundObject))
	{
		*OutSkeletalMeshComponent = SkeletalMeshComponent;
		if (SkeletalMeshComponent->GetSkeletalMeshAsset() && SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton())
		{
			*OutSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton();
		}
	}
}

static TSharedPtr<ISequencer> GetSequencerFromAsset()
{
	//if getting sequencer from level sequence need to use the current(leader), not the focused
	ULevelSequence* LevelSequence = ULevelSequenceEditorBlueprintLibrary::GetCurrentLevelSequence();
	IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSequence, false);
	ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
	TSharedPtr<ISequencer> Sequencer = LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;
	if (Sequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Can not open Sequencer for the LevelSequence %s"), *(LevelSequence->GetPathName()));
	}
	return Sequencer;
}

static UMovieSceneTLLRN_ControlRigParameterTrack* AddTLLRN_ControlRig(ULevelSequence* LevelSequence,const UClass* InClass, FGuid ObjectBinding, UTLLRN_ControlRig* InExistingTLLRN_ControlRig, bool bIsAdditiveTLLRN_ControlRig)
{
	FSlateApplication::Get().DismissAllMenus();
	if (!InClass || !InClass->IsChildOf(UTLLRN_ControlRig::StaticClass()) ||
		!LevelSequence || !LevelSequence->GetMovieScene())
	{
		return nullptr;
	}
	
	UMovieScene* OwnerMovieScene = LevelSequence->GetMovieScene();
	TSharedPtr<ISequencer> SharedSequencer = GetSequencerFromAsset();
	ISequencer* Sequencer = nullptr; // will be valid  if we have a ISequencer AND it's focused.
	if (SharedSequencer.IsValid() && SharedSequencer->GetFocusedMovieSceneSequence() == LevelSequence)
	{
		Sequencer = SharedSequencer.Get();
	}
	LevelSequence->Modify();
	OwnerMovieScene->Modify();
	
	if (bIsAdditiveTLLRN_ControlRig && InClass != UFKTLLRN_ControlRig::StaticClass() && !InClass->GetDefaultObject<UTLLRN_ControlRig>()->SupportsEvent(FTLLRN_RigUnit_InverseExecution::EventName))
	{
		UE_LOG(LogTLLRN_ControlRigEditor, Error, TEXT("Cannot add an additive control rig which does not contain a backwards solve event."));
		return nullptr;
	}
	
	FScopedTransaction AddTLLRN_ControlRigTrackTransaction(LOCTEXT("AddTLLRN_ControlRigTrack", "Add Control Rig Track"));

	UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(OwnerMovieScene->AddTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), ObjectBinding));
	if (Track)
	{
		FString ObjectName = InClass->GetName(); //GetDisplayNameText().ToString();
		ObjectName.RemoveFromEnd(TEXT("_C"));

		bool bSequencerOwnsTLLRN_ControlRig = false;
		UTLLRN_ControlRig* TLLRN_ControlRig = InExistingTLLRN_ControlRig;
		if (TLLRN_ControlRig == nullptr)
		{
			TLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(Track, InClass, FName(*ObjectName), RF_Transactional);
			bSequencerOwnsTLLRN_ControlRig = true;
		}

		TLLRN_ControlRig->Modify();
		if (UFKTLLRN_ControlRig* FKTLLRN_ControlRig = Cast<UFKTLLRN_ControlRig>(Cast<UTLLRN_ControlRig>(TLLRN_ControlRig)))
		{
			if (bIsAdditiveTLLRN_ControlRig)
			{
				FKTLLRN_ControlRig->SetApplyMode(ETLLRN_ControlRigFKRigExecuteMode::Additive);
			}
		}
		else
		{
			TLLRN_ControlRig->SetIsAdditive(bIsAdditiveTLLRN_ControlRig);
		}
		TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
		// Do not re-initialize existing control rig
		if (!InExistingTLLRN_ControlRig)
		{
			TLLRN_ControlRig->Initialize();
		}
		TLLRN_ControlRig->Evaluate_AnyThread();

		if (SharedSequencer.IsValid())
		{
			SharedSequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
		}

		Track->Modify();
		UMovieSceneSection* NewSection = Track->CreateTLLRN_ControlRigSection(0, TLLRN_ControlRig, bSequencerOwnsTLLRN_ControlRig);
		NewSection->Modify();

		Track->SetTrackName(FName(*ObjectName));
		if (bIsAdditiveTLLRN_ControlRig)
		{
			const FString AdditiveObjectName = ObjectName + TEXT(" (Layered)");
			Track->SetDisplayName(FText::FromString(AdditiveObjectName));
		}
		else
		{
			Track->SetDisplayName(FText::FromString(ObjectName));
		}
		UTLLRN_ControlRigSequencerEditorLibrary::MarkLayeredModeOnTrackDisplay(Track);

		if (SharedSequencer.IsValid())
		{
			SharedSequencer->EmptySelection();
			SharedSequencer->SelectSection(NewSection);
			SharedSequencer->ThrobSectionSelection();
			SharedSequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
			SharedSequencer->ObjectImplicitlyAdded(TLLRN_ControlRig);
		}

		FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
		if (!TLLRN_ControlRigEditMode)
		{
			GLevelEditorModeTools().ActivateMode(FTLLRN_ControlRigEditMode::ModeName);
			TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));

		}
		if (TLLRN_ControlRigEditMode)
		{
			TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, SharedSequencer);
		}
		return Track;
	}
	return nullptr;
}

UMovieSceneTrack* UTLLRN_ControlRigSequencerEditorLibrary::FindOrCreateTLLRN_ControlRigTrack(UWorld* World, ULevelSequence* LevelSequence, const UClass* TLLRN_ControlRigClass, const FMovieSceneBindingProxy& InBinding, bool bIsLayeredTLLRN_ControlRig)
{
	UMovieScene* MovieScene = InBinding.Sequence ? InBinding.Sequence->GetMovieScene() : nullptr;
	UMovieSceneTrack* BaseTrack = nullptr;
	if (LevelSequence && MovieScene && InBinding.BindingID.IsValid())
	{
		if (const FMovieSceneBinding* Binding = MovieScene->FindBinding(InBinding.BindingID))
		{
			TArray<UMovieSceneTrack*> Tracks = MovieScene->FindTracks(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding->GetObjectGuid(), NAME_None);
			for (UMovieSceneTrack* AnyOleTrack : Tracks)
			{
				UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(AnyOleTrack);
				if (Track && Track->GetTLLRN_ControlRig() && Track->GetTLLRN_ControlRig()->GetClass() == TLLRN_ControlRigClass)
				{
					return Track;
				}
			}

			UMovieSceneTLLRN_ControlRigParameterTrack* Track = AddTLLRN_ControlRig(LevelSequence, TLLRN_ControlRigClass,  InBinding.BindingID, nullptr, bIsLayeredTLLRN_ControlRig);

			if (Track)
			{
				BaseTrack = Track;
			}
		}
	}
	return BaseTrack;
}


TArray<UMovieSceneTrack*> UTLLRN_ControlRigSequencerEditorLibrary::FindOrCreateTLLRN_ControlRigComponentTrack(UWorld* World, ULevelSequence* LevelSequence, const FMovieSceneBindingProxy& InBinding)
{
	TArray< UMovieSceneTrack*> Tracks;
	TArray<UObject*, TInlineAllocator<1>> Result;
	if (LevelSequence == nullptr || LevelSequence->GetMovieScene() == nullptr)
	{
		return Tracks;
	}
	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	UObject* Context = nullptr;
	ALevelSequenceActor* OutActor = nullptr;
	ULevelSequencePlayer* OutPlayer = nullptr;
	Result = GetBoundObjects(World, LevelSequence, InBinding, &OutPlayer, &OutActor);
	if (Result.Num() > 0 && Result[0])
	{
		UObject* BoundObject = Result[0];
		if (AActor* BoundActor = Cast<AActor>(BoundObject))
		{
			TArray<UTLLRN_ControlRigComponent*> TLLRN_ControlRigComponents;
			BoundActor->GetComponents<UTLLRN_ControlRigComponent>(TLLRN_ControlRigComponents);
			for (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent : TLLRN_ControlRigComponents)
			{
				if (UTLLRN_ControlRig* CR = Cast<UTLLRN_ControlRig>(TLLRN_ControlRigComponent->GetTLLRN_ControlRig()))
				{
					UMovieSceneTLLRN_ControlRigParameterTrack* GoodTrack = nullptr;
					if (FMovieSceneBinding* Binding = MovieScene->FindBinding(InBinding.BindingID))
					{
						for (UMovieSceneTrack* Track : Binding->GetTracks())
						{
							if (UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigParameterTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track))
							{
								if (TLLRN_ControlRigParameterTrack->GetTLLRN_ControlRig() == CR)
								{
									GoodTrack = TLLRN_ControlRigParameterTrack;
									break;
								}
							}
						}
					}

					if (GoodTrack == nullptr)
					{
						GoodTrack = AddTLLRN_ControlRig(LevelSequence, CR->GetClass(), InBinding.BindingID, CR, false);
					}
					Tracks.Add(GoodTrack);
				}
			}
		}
	}

	if (OutActor)
	{
		World->DestroyActor(OutActor);
	}

	return Tracks;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::TweenTLLRN_ControlRig(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, float TweenValue)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() && WeakSequencer.Pin()->GetFocusedMovieSceneSequence() == LevelSequence
		&& TLLRN_ControlRig && LevelSequence->GetMovieScene())
	{
		FControlsToTween ControlsToTween;
		LevelSequence->GetMovieScene()->Modify();
		TArray<UTLLRN_ControlRig*> SelectedTLLRN_ControlRigs;
		SelectedTLLRN_ControlRigs.Add(TLLRN_ControlRig);
		ControlsToTween.Setup(SelectedTLLRN_ControlRigs, WeakSequencer);
		ControlsToTween.Blend(WeakSequencer, TweenValue);
		return true;
	}
	return false;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::BlendValuesOnSelected(ULevelSequence* LevelSequence, ETLLRN_AnimToolBlendOperation BlendOperation, float BlendValue)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() && WeakSequencer.Pin()->GetFocusedMovieSceneSequence() == LevelSequence
		&& LevelSequence->GetMovieScene())
	{
		if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
		{
			TWeakPtr<FTLLRN_ControlRigEditMode> WeakMode = StaticCastSharedRef<FTLLRN_ControlRigEditMode, FEdMode>(EditMode->AsShared()).ToWeakPtr();
			
			FControlsToTween ControlsToTween;
			LevelSequence->GetMovieScene()->Modify();
			switch(BlendOperation)
			{ 
			case ETLLRN_AnimToolBlendOperation::Tween:
				{
					FControlsToTween BlendTool;
					BlendTool.Setup(WeakSequencer, WeakMode);
					BlendTool.Blend(WeakSequencer, BlendValue);
					return true;
				}
			case ETLLRN_AnimToolBlendOperation::BlendToNeighbor:
				{
					FBlendNeighborSlider BlendTool;
					BlendTool.Setup(WeakSequencer, WeakMode);
					BlendTool.Blend(WeakSequencer, BlendValue);
					return true;
				}
			case ETLLRN_AnimToolBlendOperation::PushPull:
				{
					FPushPullSlider BlendTool;
					BlendTool.Setup(WeakSequencer, WeakMode);
					BlendTool.Blend(WeakSequencer, BlendValue);
					return true;
				}
			case ETLLRN_AnimToolBlendOperation::BlendRelative:
				{
					FBlendRelativeSlider BlendTool;
					BlendTool.Setup(WeakSequencer, WeakMode);
					BlendTool.Blend(WeakSequencer, BlendValue);
					return true;
				}
			case ETLLRN_AnimToolBlendOperation::BlendToEase:
				{
					FBlendToEaseSlider BlendTool;
					BlendTool.Setup(WeakSequencer, WeakMode);
					BlendTool.Blend(WeakSequencer, BlendValue);
					return true;
				}
			case ETLLRN_AnimToolBlendOperation::SmoothRough:
				{
					FSmoothRoughSlider BlendTool;
					BlendTool.Setup(WeakSequencer, WeakMode);
					BlendTool.Blend(WeakSequencer, BlendValue);
					return true;
				}
			}
		}
	}
	return false;
}

UTickableConstraint* UTLLRN_ControlRigSequencerEditorLibrary::AddConstraint(UWorld* World, ETransformConstraintType InType, UTransformableHandle* InChildHandle, UTransformableHandle* InParentHandle, const bool bMaintainOffset)
{
	if (!World)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("AddConstraint: Need Valid World"));
		return nullptr;
	}

	if (!InChildHandle || !InChildHandle->IsValid())
    {
    	UE_LOG(LogTLLRN_ControlRig, Error, TEXT("AddConstraint: Need Valid Child Handle"));
    	return nullptr;
    }

	if (!InParentHandle || !InParentHandle->IsValid())
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("AddConstraint: Need Valid Parent Handle"));
		return nullptr;
	}

	UTickableTransformConstraint* Constraint = FTransformConstraintUtils::CreateFromType(World, InType);
	if (!Constraint)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("AddConstraint: Can not create constraint from type"));
		return nullptr;
	}

	if(FTransformConstraintUtils::AddConstraint(World, InParentHandle, InChildHandle, Constraint, bMaintainOffset) == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("AddConstraint: Constraint not added"));
		Constraint->MarkAsGarbage();
		return nullptr;
	}

	if (ULevelSequence* LevelSequence = ULevelSequenceEditorBlueprintLibrary::GetCurrentLevelSequence())
	{
		//add key
		const TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
		if (WeakSequencer.IsValid())
		{
			FMovieSceneConstraintChannelHelper::SmartConstraintKey(WeakSequencer.Pin(), Constraint, TOptional<bool>(), TOptional<FFrameNumber>());
		}
	}
	else
	{
		FConstraintsManagerController& Controller = FConstraintsManagerController::Get(World);
		Controller.StaticConstraintCreated(World, Constraint);
	}
	return Constraint;
}

TArray <UTickableConstraint*> UTLLRN_ControlRigSequencerEditorLibrary::GetConstraintsForHandle(UWorld* InWorld, const UTransformableHandle* InChild)
{
	TArray <UTickableConstraint*> Constraints;
	const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(InWorld);
	const TArray<TWeakObjectPtr<UTickableConstraint>>& AllConstraints = Controller.GetAllConstraints(false);
	for (const TWeakObjectPtr<UTickableConstraint>& TickConstraint : AllConstraints)
	{
		if (TObjectPtr<UTickableTransformConstraint> Constraint = Cast<UTickableTransformConstraint>(TickConstraint.Get()))
		{
			if (Constraint->ChildTRSHandle == InChild)
			{
				Constraints.Add(Constraint.Get());
			}
		}
	}
	return Constraints;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::Compensate(UTickableConstraint* InConstraint, FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Compensate: Need open Sequencer"));
		return false;
	}
	if (UTickableTransformConstraint* Constraint = Cast<UTickableTransformConstraint>(InConstraint))
	{
		TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
		FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
		FFrameRate DisplayRate = Sequencer->GetFocusedDisplayRate();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			InTime = FFrameRate::TransformTime(FFrameTime(InTime, 0), DisplayRate, TickResolution).RoundToFrame();
		}
		TOptional<FFrameNumber> OptTime(InTime);
		FMovieSceneConstraintChannelHelper::Compensate(Sequencer, Constraint, OptTime, true /*bCompPreviousTick*/);
		return true;
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Compensate: Not Transform Constraint"));
	}
	return false;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::CompensateAll(UTickableConstraint* InConstraint)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("CompensateAll: Need open Sequencer"));
		return false;
	}
	if (UTickableTransformConstraint* Constraint = Cast<UTickableTransformConstraint>(InConstraint))
	{
		FMovieSceneConstraintChannelHelper::Compensate(WeakSequencer.Pin(), Constraint, TOptional<FFrameNumber>(), true /*bCompPreviousTick*/);
		return true;
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("CompensateAll: Not Transform Constraint"));
	}
	return false;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::SetConstraintActiveKey(UTickableConstraint* InConstraint, bool bActive, FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("SetConstraintActiveKey: Need open Sequencer"));
		return false;
	}

	if (UTickableTransformConstraint* Constraint = Cast<UTickableTransformConstraint>(InConstraint))
	{
		TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
		FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
		FFrameRate DisplayRate = Sequencer->GetFocusedDisplayRate();

		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			InTime = FFrameRate::TransformTime(FFrameTime(InTime, 0), DisplayRate, TickResolution).RoundToFrame();
		}
		return FMovieSceneConstraintChannelHelper::SmartConstraintKey(Sequencer, Constraint, TOptional<bool>(bActive), TOptional<FFrameNumber>(InTime));
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("SetConstraintActiveKey: Not Transform Constraint"));
	}
	return false;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::GetConstraintKeys(UTickableConstraint* InConstraint, UMovieSceneSection* ConstraintSection, TArray<bool>& OutBools, TArray<FFrameNumber>& OutFrames, EMovieSceneTimeUnit TimeUnit)
{
	if (InConstraint == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("GetConstraintKeys: Constraint not valid"));
		return false;
	}
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("GetConstraintKeys: Need open Sequencer"));
		return false;
	}

	IMovieSceneConstrainedSection* ConstrainedSection = Cast<IMovieSceneConstrainedSection>(ConstraintSection);
	if (ConstrainedSection == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("GetConstraintKeys: Section doesn't support constraints"));
		return false;
	}
	FConstraintAndActiveChannel* ConstraintAndChannel = ConstrainedSection->GetConstraintChannel(InConstraint->ConstraintID);
	if (ConstraintAndChannel == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("GetConstraintKeys: Constraint not found in section"));
		return false;
	}

	if (UTickableTransformConstraint* Constraint = Cast<UTickableTransformConstraint>(InConstraint))
	{
		TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
		FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
		FFrameRate DisplayRate = Sequencer->GetFocusedDisplayRate();


		TArray<FFrameNumber> OurKeyTimes;
		TArray<FKeyHandle> OurKeyHandles;
		TRange<FFrameNumber> CurrentFrameRange;
		CurrentFrameRange.SetLowerBound(TRangeBound<FFrameNumber>());
		CurrentFrameRange.SetUpperBound(TRangeBound<FFrameNumber>());

		TMovieSceneChannelData<bool> ChannelInterface = ConstraintAndChannel->ActiveChannel.GetData();
		ChannelInterface.GetKeys(CurrentFrameRange, &OurKeyTimes, &OurKeyHandles);
		
		if (OurKeyTimes.Num() > 0)
		{
			OutBools.SetNum(OurKeyTimes.Num());
			OutFrames.SetNum(OurKeyTimes.Num());
			int32 Index = 0;
			for (FFrameNumber Frame : OurKeyTimes)
			{
				ConstraintAndChannel->ActiveChannel.Evaluate(FFrameTime(Frame, 0.0f), OutBools[Index]);
				if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
				{
					Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), TickResolution,DisplayRate).RoundToFrame();
				}
				OutFrames[Index] = Frame;
				++Index;
			}
		}
		else
		{
			OutBools.SetNum(0);
			OutFrames.SetNum(0);
		}
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("GetConstraintKeys: Not Transform Constraint"));
		return false;
	}
	return true;
}


bool UTLLRN_ControlRigSequencerEditorLibrary::MoveConstraintKey(UTickableConstraint* Constraint, UMovieSceneSection* ConstraintSection, FFrameNumber InTime, FFrameNumber InNewTime, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("MoveConstraintKey: Need open Sequencer"));
		return false;
	}
	IMovieSceneConstrainedSection* ConstrainedSection = Cast<IMovieSceneConstrainedSection>(ConstraintSection);
	if (ConstrainedSection == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("MoveConstraintKey: Section doesn't support constraints"));
		return false;
	}
	if (Constraint == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("MoveConstraintKey: Constraint not valid"));
		return false;
	}
	FConstraintAndActiveChannel* ConstraintAndChannel = ConstrainedSection->GetConstraintChannel(Constraint->ConstraintID);
	if (ConstraintAndChannel == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("MoveConstraintKey: Constraint not found in section"));
		return false;
	}
	TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
	FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
	FFrameRate DisplayRate = Sequencer->GetFocusedDisplayRate();

	if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
	{
		InTime = FFrameRate::TransformTime(FFrameTime(InTime, 0), DisplayRate, TickResolution).RoundToFrame();
		InNewTime = FFrameRate::TransformTime(FFrameTime(InNewTime, 0), DisplayRate, TickResolution).RoundToFrame();
	}

	TArray<FFrameNumber> OurKeyTimes;
	TArray<FKeyHandle> OurKeyHandles;
	TRange<FFrameNumber> CurrentFrameRange;
	CurrentFrameRange.SetLowerBound(TRangeBound<FFrameNumber>(InTime));
	CurrentFrameRange.SetUpperBound(TRangeBound<FFrameNumber>(InTime));
	TMovieSceneChannelData<bool> ChannelInterface = ConstraintAndChannel->ActiveChannel.GetData();
	ChannelInterface.GetKeys(CurrentFrameRange, &OurKeyTimes, &OurKeyHandles);
	if (OurKeyHandles.Num() > 0)
	{
		ConstraintSection->TryModify();
		TArray<FFrameNumber> NewKeyTimes;
		NewKeyTimes.Add(InNewTime);
		ConstraintAndChannel->ActiveChannel.SetKeyTimes(OurKeyHandles, NewKeyTimes);
		WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("MoveConstraintKey: No Key To Move At that Time"));
		return false;
	}
	return true;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::DeleteConstraintKey(UTickableConstraint* Constraint, UMovieSceneSection* ConstraintSection, FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("DeleteConstraintKey: Need open Sequencer"));
		return false;
	}
	IMovieSceneConstrainedSection* ConstrainedSection = Cast<IMovieSceneConstrainedSection>(ConstraintSection);
	if (ConstrainedSection == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("DeleteConstraintKey: Section doesn't support constraints"));
		return false;
	}
	if (Constraint == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("DeleteConstraintKey: Constraint not valid"));
		return false;
	}
	FConstraintAndActiveChannel* ConstraintAndChannel = ConstrainedSection->GetConstraintChannel(Constraint->ConstraintID);
	if (ConstraintAndChannel == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("DeleteConstraintKey: Constraint not found in section"));
		return false;
	}
	TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
	FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
	FFrameRate DisplayRate = Sequencer->GetFocusedDisplayRate();

	if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
	{
		InTime = FFrameRate::TransformTime(FFrameTime(InTime, 0), DisplayRate, TickResolution).RoundToFrame();
	}

	TArray<FFrameNumber> OurKeyTimes;
	TArray<FKeyHandle> OurKeyHandles;
	TRange<FFrameNumber> CurrentFrameRange;
	CurrentFrameRange.SetLowerBound(TRangeBound<FFrameNumber>(InTime));
	CurrentFrameRange.SetUpperBound(TRangeBound<FFrameNumber>(InTime));
	TMovieSceneChannelData<bool> ChannelInterface = ConstraintAndChannel->ActiveChannel.GetData();
	ChannelInterface.GetKeys(CurrentFrameRange, &OurKeyTimes, &OurKeyHandles);
	if (OurKeyHandles.Num() > 0)
	{
		ConstraintSection->TryModify();
		ConstraintAndChannel->ActiveChannel.DeleteKeys(OurKeyHandles);
		WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("DeleteConstraintKey: No Key To Delete At that Time"));
		return false;
	}
	return true;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::BakeConstraint(UWorld* World, UTickableConstraint* Constraint, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	if (!World)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("BakeConstraint: Need Valid World"));
		return false;
	}
	TOptional< FBakingAnimationKeySettings> Settings;

	if (UTickableTransformConstraint* TransformConstraint = Cast<UTickableTransformConstraint>(Constraint))
	{
		TSharedPtr<ISequencer> Sequencer = GetSequencerFromAsset();
		if (!Sequencer || !Sequencer->GetFocusedMovieSceneSequence())
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("BakeConstraint: Need loaded level Sequence"));
			return false;
		}
		const UMovieScene* MovieScene = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
		if (!MovieScene)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("BakeConstraint: Need valid Movie Scene"));
			return false;
		}
		
		TOptional<TArray<FFrameNumber>> FramesToBake;
		TArray<FFrameNumber> RealFramesToBake;
		RealFramesToBake.SetNum(Frames.Num());

		int32 Index = 0;
		for (FFrameNumber Frame : Frames) 
		{
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			RealFramesToBake[Index++] = Frame;
		} 
		if (RealFramesToBake.Num() > 0)
		{
			FramesToBake = RealFramesToBake;
		}
		FConstraintBaker::Bake(World, TransformConstraint, Sequencer, Settings, FramesToBake);
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("BakeConstraint: Need Valid Constraint"));
		return false;
	}
	return true;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::BakeConstraints(UWorld* World, TArray<UTickableConstraint*>& InConstraints, const FBakingAnimationKeySettings& InSettings)
{
	if (!World)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("BakeConstraint: Need Valid World"));
		return false;
	}

	TSharedPtr<ISequencer> Sequencer = GetSequencerFromAsset();
	if (!Sequencer || !Sequencer->GetFocusedMovieSceneSequence())
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("BakeConstraint: Need loaded level Sequence"));
		return false;
	}
	const UMovieScene* MovieScene = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
	if (!MovieScene)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("BakeConstraint: Need valid Movie Scene"));
		return false;
	}
	TArray<UTickableTransformConstraint*> TransformConstraints;
	for (UTickableConstraint* Constraint : InConstraints)
	{
		if (UTickableTransformConstraint* TransformConstraint = Cast<UTickableTransformConstraint>(Constraint))
		{
			TransformConstraints.Add(TransformConstraint);
		}
		else
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("BakeConstraint: Need Valid Constraint"));
			return false;
		}
	}

	if(FConstraintBaker::BakeMultiple(World, TransformConstraints, Sequencer, InSettings) == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("BakeMultiple: Failed"));
		return false;
	}
	return true;
}


bool UTLLRN_ControlRigSequencerEditorLibrary::SnapTLLRN_ControlRig(ULevelSequence* LevelSequence, FFrameNumber StartFrame, FFrameNumber EndFrame, const FTLLRN_ControlRigSnapperSelection& ChildrenToSnap,
	const FTLLRN_ControlRigSnapperSelection& ParentToSnap, const UTLLRN_ControlRigSnapSettings* SnapSettings, EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || LevelSequence->GetMovieScene() == nullptr)
	{
		return false;
	}
	FTLLRN_ControlRigSnapper Snapper;
	if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
	{
		StartFrame = FFrameRate::TransformTime(FFrameTime(StartFrame, 0), LevelSequence->GetMovieScene()->GetDisplayRate(), LevelSequence->GetMovieScene()->GetTickResolution()).RoundToFrame();
		EndFrame = FFrameRate::TransformTime(FFrameTime(EndFrame, 0), LevelSequence->GetMovieScene()->GetDisplayRate(), LevelSequence->GetMovieScene()->GetTickResolution()).RoundToFrame();

	}
	return Snapper.SnapIt(StartFrame, EndFrame, ChildrenToSnap, ParentToSnap,SnapSettings);
}

FTransform UTLLRN_ControlRigSequencerEditorLibrary::GetActorWorldTransform(ULevelSequence* LevelSequence, AActor* Actor, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	TArray<FFrameNumber> Frames;
	Frames.Add(Frame);
	TArray<FTransform> Transforms = GetActorWorldTransforms(LevelSequence, Actor, Frames, TimeUnit);
	if (Transforms.Num() == 1)
	{
		return Transforms[0];
	}
	return FTransform::Identity;
}
static void ConvertFramesToTickResolution(UMovieScene* MovieScene, const TArray<FFrameNumber>& InFrames, EMovieSceneTimeUnit TimeUnit, TArray<FFrameNumber>& OutFrames)
{
	OutFrames.SetNum(InFrames.Num());
	int32 Index = 0;
	for (const FFrameNumber& Frame : InFrames)
	{
		FFrameTime FrameTime(Frame);
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			FrameTime = FFrameRate::TransformTime(FrameTime, MovieScene->GetDisplayRate(), MovieScene->GetTickResolution());
		}
		OutFrames[Index++] = FrameTime.RoundToFrame();
	}
}

TArray<FTransform> UTLLRN_ControlRigSequencerEditorLibrary::GetActorWorldTransforms(ULevelSequence* LevelSequence,AActor* Actor, const TArray<FFrameNumber>& InFrames, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	TArray<FTransform> OutWorldTransforms;
	if (WeakSequencer.IsValid() && Actor)
	{
		FActorForWorldTransforms Actors;
		Actors.Actor = Actor;
		TArray<FFrameNumber> Frames;
		ConvertFramesToTickResolution(LevelSequence->GetMovieScene(), InFrames, TimeUnit, Frames);
		MovieSceneToolHelpers::GetActorWorldTransforms(WeakSequencer.Pin().Get(), Actors, Frames, OutWorldTransforms); 
	}
	return OutWorldTransforms;

}

FTransform UTLLRN_ControlRigSequencerEditorLibrary::GetSkeletalMeshComponentWorldTransform(ULevelSequence* LevelSequence,USkeletalMeshComponent* SkeletalMeshComponent, FFrameNumber Frame, 
	EMovieSceneTimeUnit TimeUnit, FName SocketName)
{
	TArray<FFrameNumber> Frames;
	Frames.Add(Frame);
	TArray<FTransform> Transforms = GetSkeletalMeshComponentWorldTransforms(LevelSequence, SkeletalMeshComponent, Frames, TimeUnit,SocketName);
	if (Transforms.Num() == 1)
	{
		return Transforms[0];
	}
	return FTransform::Identity;
}

TArray<FTransform> UTLLRN_ControlRigSequencerEditorLibrary::GetSkeletalMeshComponentWorldTransforms(ULevelSequence* LevelSequence,USkeletalMeshComponent* SkeletalMeshComponent, const TArray<FFrameNumber>& InFrames,
	EMovieSceneTimeUnit TimeUnit, FName SocketName)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	TArray<FTransform> OutWorldTransforms;
	if (WeakSequencer.IsValid() && SkeletalMeshComponent)
	{
		FActorForWorldTransforms Actors;
		AActor* Actor = SkeletalMeshComponent->GetTypedOuter<AActor>();
		if (Actor)
		{
			Actors.Actor = Actor;
			Actors.Component = SkeletalMeshComponent;
			Actors.SocketName = SocketName;
			TArray<FFrameNumber> Frames;
			ConvertFramesToTickResolution(LevelSequence->GetMovieScene(), InFrames, TimeUnit, Frames);
			MovieSceneToolHelpers::GetActorWorldTransforms(WeakSequencer.Pin().Get(), Actors, Frames, OutWorldTransforms);
		}
	}
	return OutWorldTransforms;
}

FTransform UTLLRN_ControlRigSequencerEditorLibrary::GetTLLRN_ControlRigWorldTransform(ULevelSequence* LevelSequence,UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
	EMovieSceneTimeUnit TimeUnit)
{
	TArray<FFrameNumber> Frames;
	Frames.Add(Frame);
	TArray<FTransform> Transforms = GetTLLRN_ControlRigWorldTransforms(LevelSequence, TLLRN_ControlRig, ControlName, Frames, TimeUnit);
	if (Transforms.Num() == 1)
	{
		return Transforms[0];
	}
	return FTransform::Identity;
}

TArray<FTransform> UTLLRN_ControlRigSequencerEditorLibrary::GetTLLRN_ControlRigWorldTransforms(ULevelSequence* LevelSequence,UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& InFrames,
	EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	TArray<FTransform> OutWorldTransforms;
	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
		{
			USceneComponent* Component = Cast<USceneComponent>(ObjectBinding->GetBoundObject());
			if (Component)
			{
				AActor* Actor = Component->GetTypedOuter< AActor >();
				if (Actor)
				{
					TLLRN_ControlRig->Modify();
					TArray<FTransform> TLLRN_ControlRigParentWorldTransforms;
					FActorForWorldTransforms TLLRN_ControlRigActorSelection;
					TLLRN_ControlRigActorSelection.Actor = Actor;
					TArray<FFrameNumber> Frames;
					ConvertFramesToTickResolution(LevelSequence->GetMovieScene(), InFrames, TimeUnit, Frames);
					MovieSceneToolHelpers::GetActorWorldTransforms(WeakSequencer.Pin().Get(), TLLRN_ControlRigActorSelection, Frames, TLLRN_ControlRigParentWorldTransforms);
					FTLLRN_ControlRigSnapper Snapper;
					Snapper.GetTLLRN_ControlTLLRN_RigControlTransforms(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, Frames, TLLRN_ControlRigParentWorldTransforms, OutWorldTransforms);	
				}
			}
		}
	}
	return OutWorldTransforms;
}


static void LocalSetTLLRN_ControlRigWorldTransforms(ULevelSequence* LevelSequence,UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, ETLLRN_ControlRigSetKey SetKey, const TArray<FFrameNumber>& InFrames, 
	const TArray<FTransform>& WorldTransforms, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
		{
			USceneComponent* Component = Cast<USceneComponent>(ObjectBinding->GetBoundObject());
			if (Component)
			{
				AActor* Actor = Component->GetTypedOuter< AActor >();
				if (Actor)
				{
					TArray<FFrameNumber> Frames;
					ConvertFramesToTickResolution(LevelSequence->GetMovieScene(), InFrames, TimeUnit, Frames);

					UMovieScene* MovieScene = WeakSequencer.Pin().Get()->GetFocusedMovieSceneSequence()->GetMovieScene();
					MovieScene->Modify();
					FFrameRate TickResolution = MovieScene->GetTickResolution();
					FTLLRN_RigControlModifiedContext Context;
					Context.SetKey = SetKey;

					TLLRN_ControlRig->Modify();
					TArray<FTransform> TLLRN_ControlRigParentWorldTransforms;
					FActorForWorldTransforms TLLRN_ControlRigActorSelection;
					TLLRN_ControlRigActorSelection.Actor = Actor;
					MovieSceneToolHelpers::GetActorWorldTransforms(WeakSequencer.Pin().Get(), TLLRN_ControlRigActorSelection, Frames, TLLRN_ControlRigParentWorldTransforms);
					FTLLRN_ControlRigSnapper Snapper;
					
					TArray<FFrameNumber> OneFrame;
					OneFrame.SetNum(1);
					TArray<FTransform> CurrentTLLRN_ControlTLLRN_RigTransform, CurrentParentWorldTransform;
					CurrentTLLRN_ControlTLLRN_RigTransform.SetNum(1);
					CurrentParentWorldTransform.SetNum(1);
					
					for (int32 Index = 0; Index < WorldTransforms.Num(); ++Index)
					{
						OneFrame[0] = Frames[Index];
						CurrentParentWorldTransform[0] = TLLRN_ControlRigParentWorldTransforms[Index];
						//this will evaluate at the current frame which we want
						Snapper.GetTLLRN_ControlTLLRN_RigControlTransforms(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, OneFrame, CurrentParentWorldTransform, CurrentTLLRN_ControlTLLRN_RigTransform);
							
						const FFrameNumber& FrameNumber = Frames[Index];
						Context.LocalTime = TickResolution.AsSeconds(FFrameTime(FrameNumber));
						FTransform GlobalTransform = WorldTransforms[Index].GetRelativeTransform(TLLRN_ControlRigParentWorldTransforms[Index]);
						TLLRN_ControlRig->SetControlGlobalTransform(ControlName, GlobalTransform, true, Context, false /*undo*/, false /*bPrintPython*/, true/* bFixEulerFlips*/);
					}
				}
			}
		}
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetTLLRN_ControlRigWorldTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, const FTransform& WorldTransform,
	EMovieSceneTimeUnit TimeUnit, bool bSetKey)
{
	ETLLRN_ControlRigSetKey SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
	TArray<FFrameNumber> Frames;
	Frames.Add(Frame);
	TArray<FTransform> WorldTransforms;
	WorldTransforms.Add(WorldTransform);

	LocalSetTLLRN_ControlRigWorldTransforms(LevelSequence, TLLRN_ControlRig, ControlName, SetKey, Frames, WorldTransforms, TimeUnit);

}

void UTLLRN_ControlRigSequencerEditorLibrary::SetTLLRN_ControlRigWorldTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<FTransform>& WorldTransforms,
	EMovieSceneTimeUnit TimeUnit)
{
	LocalSetTLLRN_ControlRigWorldTransforms(LevelSequence, TLLRN_ControlRig, ControlName, ETLLRN_ControlRigSetKey::Always, Frames, WorldTransforms,TimeUnit);
}

bool UTLLRN_ControlRigSequencerEditorLibrary::SmartReduce(FSmartReduceParams& ReduceParams, UMovieSceneSection* MovieSceneSection)
{
	//get level sequence if one exists...
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Smart Reduce: No open level sequence"));
		return false;
	}
	if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(MovieSceneSection))
	{
		TSharedPtr<ISequencer>  SequencerPtr = WeakSequencer.Pin();
		FTLLRN_ControlRigParameterTrackEditor::SmartReduce(SequencerPtr, ReduceParams, Section);
		return true;
	}
	UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Smart Reduce: Section is not Control Rig Section"));
	return false;
}


bool UTLLRN_ControlRigSequencerEditorLibrary::BakeToTLLRN_ControlRig(UWorld* World, ULevelSequence* LevelSequence, UClass* InClass, UAnimSeqExportOption* ExportOptions, bool bReduceKeys, float Tolerance,
	const FMovieSceneBindingProxy& Binding, bool bResetControls)
{
	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (Binding.Sequence != LevelSequence)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Baking: Binding.Sequence different"));
		return false;
	}
	//get level sequence if one exists...
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	ALevelSequenceActor* OutActor = nullptr;
	FMovieSceneSequencePlaybackSettings Settings;
	FLevelSequenceCameraSettings CameraSettings;
	FMovieSceneSequenceIDRef Template = MovieSceneSequenceID::Root;
	FMovieSceneSequenceTransform RootToLocalTransform;
	IMovieScenePlayer* Player = nullptr;
	ULevelSequencePlayer* LevelPlayer = nullptr;
	if (WeakSequencer.IsValid())
	{
		Player = WeakSequencer.Pin().Get();
	}
	else
	{
		Player = LevelPlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(World, LevelSequence, Settings, OutActor);
	}
	if (Player == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Baking: Problem Setting up Player"));
		return false;
	}
	bool bResult = false;
	const FScopedTransaction Transaction(LOCTEXT("BakeToTLLRN_ControlRig_Transaction", "Bake To Control Rig"));
	{
		FSpawnableRestoreState SpawnableRestoreState(MovieScene, Player->GetSharedPlaybackState().ToSharedPtr());

		if (LevelPlayer && SpawnableRestoreState.bWasChanged)
		{
			// Evaluate at the beginning of the subscene time to ensure that spawnables are created before export
			FFrameTime StartTime = FFrameRate::TransformTime(UE::MovieScene::DiscreteInclusiveLower(MovieScene->GetPlaybackRange()).Value, MovieScene->GetTickResolution(), MovieScene->GetDisplayRate());
			LevelPlayer->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(StartTime, EUpdatePositionMethod::Play));
		}
		TArrayView<TWeakObjectPtr<>>  Result = Player->FindBoundObjects(Binding.BindingID, Template);
	
		if (Result.Num() > 0 && Result[0].IsValid())
		{
			UObject* BoundObject = Result[0].Get();
			USkeleton* Skeleton = nullptr;
			USkeletalMeshComponent* SkeletalMeshComp = nullptr;
			AcquireSkeletonAndSkelMeshCompFromObject(BoundObject, &Skeleton, &SkeletalMeshComp);
			if (SkeletalMeshComp && SkeletalMeshComp->GetSkeletalMeshAsset() && SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton())
			{
				UAnimSequence* TempAnimSequence = NewObject<UAnimSequence>(GetTransientPackage(), NAME_None);
				TempAnimSequence->SetSkeleton(Skeleton);
				FAnimExportSequenceParameters AESP;
				AESP.Player = Player;
				AESP.RootToLocalTransform = RootToLocalTransform;
				AESP.MovieSceneSequence = LevelSequence;
				AESP.RootMovieSceneSequence = LevelSequence;
				bResult = MovieSceneToolHelpers::ExportToAnimSequence(TempAnimSequence, ExportOptions, AESP, SkeletalMeshComp);
				if (bResult == false)
				{
					TempAnimSequence->MarkAsGarbage();
					if (OutActor)
					{
						World->DestroyActor(OutActor);
					}
					return false;
				}

				MovieScene->Modify();
				TArray<UMovieSceneTrack*> Tracks = MovieScene->FindTracks(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.BindingID, NAME_None);
				UMovieSceneTLLRN_ControlRigParameterTrack* Track = nullptr;
				for (UMovieSceneTrack* AnyOleTrack : Tracks)
				{
					UMovieSceneTLLRN_ControlRigParameterTrack* ValidTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(AnyOleTrack);
					if (ValidTrack)
					{
						Track = ValidTrack;
						Track->Modify();
						for (UMovieSceneSection* Section : Track->GetAllSections())
						{
							Section->SetIsActive(false);
						}
					}
				}
				if(Track == nullptr)
				{
					Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(MovieScene->AddTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.BindingID));
					{
						Track->Modify();
					}
				}

				if (Track)
				{
					FString ObjectName = InClass->GetName();
					ObjectName.RemoveFromEnd(TEXT("_C"));
					UTLLRN_ControlRig* TLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(Track, InClass, FName(*ObjectName), RF_Transactional);
					FName OldEventString = FName(FString(TEXT("Inverse")));

					if (InClass != UFKTLLRN_ControlRig::StaticClass() && !(TLLRN_ControlRig->SupportsEvent(FTLLRN_RigUnit_InverseExecution::EventName) || TLLRN_ControlRig->SupportsEvent(OldEventString)))
					{
						TempAnimSequence->MarkAsGarbage();
						MovieScene->RemoveTrack(*Track);
						if (OutActor)
						{
							World->DestroyActor(OutActor);
						}
						return false;
					}
					FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = nullptr;
					if (WeakSequencer.IsValid())
					{
						TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
						if (!TLLRN_ControlRigEditMode)
						{
							GLevelEditorModeTools().ActivateMode(FTLLRN_ControlRigEditMode::ModeName);
							TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));

						}
						else
						{
							/* mz todo we don't unbind  will test more
							UTLLRN_ControlRig* OldTLLRN_ControlRig = TLLRN_ControlRigEditMode->GetTLLRN_ControlRig(false);
							if (OldTLLRN_ControlRig)
							{
								WeakSequencer.Pin()->ObjectImplicitlyRemoved(OldTLLRN_ControlRig);
							}
							*/
						}
					}

					TLLRN_ControlRig->Modify();
					TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
					TLLRN_ControlRig->GetObjectBinding()->BindToObject(SkeletalMeshComp);
					TLLRN_ControlRig->GetDataSourceRegistry()->RegisterDataSource(UTLLRN_ControlRig::OwnerComponent, TLLRN_ControlRig->GetObjectBinding()->GetBoundObject());
					TLLRN_ControlRig->Initialize();
					TLLRN_ControlRig->RequestInit();
					TLLRN_ControlRig->SetBoneInitialTransformsFromSkeletalMeshComponent(SkeletalMeshComp, true);
					TLLRN_ControlRig->Evaluate_AnyThread();

					bool bSequencerOwnsTLLRN_ControlRig = true;
					UMovieSceneSection* NewSection = Track->CreateTLLRN_ControlRigSection(0, TLLRN_ControlRig, bSequencerOwnsTLLRN_ControlRig);
					UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(NewSection);

					//mz todo need to have multiple rigs with same class
					Track->SetTrackName(FName(*ObjectName));
					Track->SetDisplayName(FText::FromString(ObjectName));

					EMovieSceneKeyInterpolation DefaultInterpolation = EMovieSceneKeyInterpolation::SmartAuto;
					if (WeakSequencer.IsValid())
					{
						WeakSequencer.Pin()->EmptySelection();
						WeakSequencer.Pin()->SelectSection(NewSection);
						WeakSequencer.Pin()->ThrobSectionSelection();
						WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
						DefaultInterpolation = WeakSequencer.Pin()->GetKeyInterpolation();
					}
					FFrameNumber StartFrame = MovieScene->GetPlaybackRange().GetLowerBoundValue();
					ParamSection->LoadAnimSequenceIntoThisSection(TempAnimSequence, StartFrame, MovieScene, SkeletalMeshComp,
						bReduceKeys, Tolerance, bResetControls, FFrameNumber(0), DefaultInterpolation);

					//Turn Off Any Skeletal Animation Tracks
					UMovieSceneSkeletalAnimationTrack* SkelTrack = Cast<UMovieSceneSkeletalAnimationTrack>(MovieScene->FindTrack(UMovieSceneSkeletalAnimationTrack::StaticClass(), Binding.BindingID, NAME_None));
					if (SkelTrack)
					{
						SkelTrack->Modify();
						//can't just turn off the track so need to mute the sections
						const TArray<UMovieSceneSection*>& Sections = SkelTrack->GetAllSections();
						for (UMovieSceneSection* Section : Sections)
						{
							if (Section)
							{
								Section->TryModify();
								Section->SetIsActive(false);
							}
						}
					}
					//Finish Setup
					if (TLLRN_ControlRigEditMode)
					{
						TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, WeakSequencer.Pin());
					}

					TempAnimSequence->MarkAsGarbage();
					if (WeakSequencer.IsValid())
					{
						WeakSequencer.Pin()->ObjectImplicitlyAdded(TLLRN_ControlRig);
						WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
					}
					bResult = true;
				}
			}
		}
	}

	if (LevelPlayer)
	{
		LevelPlayer->Stop();
	}
	if (OutActor)
	{
		World->DestroyActor(OutActor);
	}
	return bResult;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::LoadAnimSequenceIntoTLLRN_ControlRigSection(UMovieSceneSection* MovieSceneSection, UAnimSequence* AnimSequence, USkeletalMeshComponent* SkelMeshComp,
	FFrameNumber InStartFrame, EMovieSceneTimeUnit TimeUnit,bool bKeyReduce, float Tolerance, EMovieSceneKeyInterpolation Interpolation, bool bResetControls)
{
	if (MovieSceneSection == nullptr || AnimSequence == nullptr || SkelMeshComp == nullptr)
	{
		return false;
	}
	UMovieScene* MovieScene = MovieSceneSection->GetTypedOuter<UMovieScene>();
	if (MovieScene == nullptr)
	{
		return false;
	}
	if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(MovieSceneSection))
	{
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			InStartFrame = FFrameRate::TransformTime(FFrameTime(InStartFrame, 0), MovieScene->GetDisplayRate(),MovieScene->GetTickResolution()).RoundToFrame();
		}
		FFrameNumber StartFrame  = MovieScene->GetPlaybackRange().GetLowerBoundValue();
		return Section->LoadAnimSequenceIntoThisSection(AnimSequence, StartFrame,  MovieScene, SkelMeshComp, bKeyReduce, Tolerance, bResetControls, InStartFrame, Interpolation);
	}
	return false;
}

static bool LocalGetTLLRN_ControlTLLRN_RigControlValues(IMovieScenePlayer* Player, UMovieSceneSequence* MovieSceneSequence, FMovieSceneSequenceIDRef Template, FMovieSceneSequenceTransform& RootToLocalTransform,
	UTLLRN_ControlRig* TLLRN_ControlRig, const FName& ControlName, EMovieSceneTimeUnit TimeUnit,
	const TArray<FFrameNumber>& InFrames, TArray<FTLLRN_RigControlValue>& OutValues)
{
	if (Player == nullptr || MovieSceneSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return false;
	}
	if (TLLRN_ControlRig->FindControl(ControlName) == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Can not find Control %s"), *(ControlName.ToString()));
		return false;
	}
	if (UMovieScene* MovieScene = MovieSceneSequence->GetMovieScene())
	{

		FFrameRate TickResolution = MovieScene->GetTickResolution();
		OutValues.SetNum(InFrames.Num());
		for (int32 Index = 0; Index < InFrames.Num(); ++Index)
		{
			const FFrameNumber& FrameNumber = InFrames[Index];
			FFrameTime GlobalTime(FrameNumber);
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				GlobalTime = FFrameRate::TransformTime(GlobalTime, MovieScene->GetDisplayRate(), MovieScene->GetTickResolution());
			}
			GlobalTime = RootToLocalTransform.Inverse().TryTransformTime(GlobalTime).Get(GlobalTime);
			FMovieSceneContext Context = FMovieSceneContext(FMovieSceneEvaluationRange(GlobalTime, TickResolution), Player->GetPlaybackStatus()).SetHasJumped(true);

			Player->GetEvaluationTemplate().EvaluateSynchronousBlocking(Context);
			TLLRN_ControlRig->Evaluate_AnyThread();
			OutValues[Index] = TLLRN_ControlRig->GetControlValue(ControlName);
		}
	}
	return true;
}

static bool GetTLLRN_ControlRigValues(ISequencer* Sequencer, UTLLRN_ControlRig* TLLRN_ControlRig, const FName& ControlName, EMovieSceneTimeUnit TimeUnit,
	const TArray<FFrameNumber>& Frames,  TArray<FTLLRN_RigControlValue>& OutValues)
{
	if (Sequencer->GetFocusedMovieSceneSequence())
	{
		FMovieSceneSequenceIDRef Template = Sequencer->GetFocusedTemplateID();
		FMovieSceneSequenceTransform RootToLocalTransform = Sequencer->GetFocusedMovieSceneSequenceTransform();
		bool bGood = LocalGetTLLRN_ControlTLLRN_RigControlValues(Sequencer, Sequencer->GetFocusedMovieSceneSequence(), Template, RootToLocalTransform,
			TLLRN_ControlRig, ControlName,TimeUnit, Frames, OutValues);
		Sequencer->RequestEvaluate();
		return bGood;
	}
	return false;
}

static bool GetTLLRN_ControlRigValue(ISequencer* Sequencer, UTLLRN_ControlRig* TLLRN_ControlRig, const FName& ControlName, EMovieSceneTimeUnit TimeUnit,
	const FFrameNumber Frame, FTLLRN_RigControlValue& OutValue)
{
	if (Sequencer->GetFocusedMovieSceneSequence())
	{
		TArray<FFrameNumber> Frames;
		Frames.Add(Frame);
		TArray<FTLLRN_RigControlValue> OutValues;
		FMovieSceneSequenceIDRef Template = Sequencer->GetFocusedTemplateID();
		FMovieSceneSequenceTransform RootToLocalTransform = Sequencer->GetFocusedMovieSceneSequenceTransform();
		bool bVal = LocalGetTLLRN_ControlTLLRN_RigControlValues(Sequencer, Sequencer->GetFocusedMovieSceneSequence(), Template, RootToLocalTransform,
			TLLRN_ControlRig, ControlName,TimeUnit, Frames, OutValues);
		if (bVal)
		{
			OutValue = OutValues[0];
		}
		return bVal;
	}
	return false;
}

static bool GetTLLRN_ControlRigValues(UWorld* World, ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, const FName& ControlName, EMovieSceneTimeUnit TimeUnit,
	const TArray<FFrameNumber>& Frames, TArray<FTLLRN_RigControlValue>& OutValues)
{
	if (LevelSequence)
	{
		ALevelSequenceActor* OutActor = nullptr;
		FMovieSceneSequencePlaybackSettings Settings;
		FLevelSequenceCameraSettings CameraSettings;
		FMovieSceneSequenceIDRef Template = MovieSceneSequenceID::Root;
		FMovieSceneSequenceTransform RootToLocalTransform;
		ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, LevelSequence, Settings, OutActor);
		return LocalGetTLLRN_ControlTLLRN_RigControlValues(Player, LevelSequence, Template, RootToLocalTransform,
			TLLRN_ControlRig, ControlName,TimeUnit, Frames, OutValues);

	}
	return false;
}


float UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigFloat(ULevelSequence* LevelSequence,UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	float Value = 0.0f;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Float && Element->Settings.ControlType != ETLLRN_RigControlType::ScaleFloat)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frame, RigValue))
		{
			Value = RigValue.Get<float>();
		}
	}
	return Value;
}

TArray<float> UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigFloats(ULevelSequence* LevelSequence,UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<float> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Float && Element->Settings.ControlType != ETLLRN_RigControlType::ScaleFloat)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}

		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName,TimeUnit, Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				float Value = RigValue.Get<float>();
				Values.Add(Value);
			}
		}
	}
	return Values;
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigFloat(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, float Value, EMovieSceneTimeUnit TimeUnit, bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Float && Element->Settings.ControlType != ETLLRN_RigControlType::ScaleFloat)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		TLLRN_ControlRig->SetControlValue<float>(ControlName, Value, true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigFloats(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<float> Values,
	EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Float && Element->Settings.ControlType != ETLLRN_RigControlType::ScaleFloat)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			float  Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlValue<float>(ControlName, Value, true, Context);
		}
	}
}


bool UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigBool(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	bool Value = true;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Bool)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName,TimeUnit, Frame, RigValue))
		{
			Value = RigValue.Get<bool>();
		}
	}
	return Value;
}

TArray<bool> UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigBools(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<bool> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Bool)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}
		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName,TimeUnit, Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				bool Value = RigValue.Get<bool>();
				Values.Add(Value);
			}
		}
	}
	return Values;
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigBool(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, bool Value, EMovieSceneTimeUnit TimeUnit, bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Bool)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return;
			}
		}
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		TLLRN_ControlRig->SetControlValue<bool>(ControlName, Value, true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigBools(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
	const TArray<bool> Values, EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Bool)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return;
			}
		}
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			bool Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlValue<bool>(ControlName, Value, true, Context);
		}
	}
}

int32 UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlRigInt(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	int32 Value = 0;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Integer)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName,TimeUnit, Frame, RigValue))
		{
			Value = RigValue.Get<int32>();
		}
	}
	return Value;
}

TArray<int32> UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlRigInts(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<int32> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Integer)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}
		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit,Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				int32 Value = RigValue.Get<int32>();
				Values.Add(Value);
			}
		}
	}
	return Values;
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlRigInt(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, int32 Value, EMovieSceneTimeUnit TimeUnit,bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Integer)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		TLLRN_ControlRig->SetControlValue<int32>(ControlName, Value, true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlRigInts(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
	const TArray<int32> Values , EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Integer)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			int32 Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlValue<int32>(ControlName, Value, true, Context);
		}
	}
}


FVector2D UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigVector2D(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	FVector2D Value = FVector2D::ZeroVector;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Vector2D)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frame, RigValue))
		{
			const FVector3f TempValue = RigValue.Get<FVector3f>(); 
			Value = FVector2D(TempValue.X, TempValue.Y);
		}
	}
	return Value;
}

TArray<FVector2D> UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigVector2Ds(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<FVector2D> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Vector2D)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}
		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				const FVector3f TempValue = RigValue.Get<FVector3f>(); 
				Values.Add(FVector2D(TempValue.X, TempValue.Y));
			}
		}
	}
	return Values;

}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigVector2D(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FVector2D Value, EMovieSceneTimeUnit TimeUnit,bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Vector2D)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		TLLRN_ControlRig->SetControlValue<FVector3f>(ControlName, FVector3f(Value.X, Value.Y, 0.f), true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigVector2Ds(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, 
	const TArray<FVector2D> Values, EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Vector2D)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			FVector2D Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlValue<FVector3f>(ControlName, FVector3f(Value.X, Value.Y, 0.f), true, Context);
		}
	}
}


FVector UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlRigPosition(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	FVector Value = FVector::ZeroVector;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Position)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frame, RigValue))
		{
			Value = (FVector)RigValue.Get<FVector3f>();
		}
	}
	return Value;
}

TArray<FVector> UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlRigPositions(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<FVector> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Position)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}
		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				FVector Value = (FVector)RigValue.Get<FVector3f>();
				Values.Add(Value);
			}
		}
	}
	return Values;
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlRigPosition(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FVector Value, EMovieSceneTimeUnit TimeUnit,bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Position)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		TLLRN_ControlRig->SetControlValue<FVector3f>(ControlName, (FVector3f)Value, true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlRigPositions(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, 
	const TArray<FVector> Values, EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Position)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			FVector Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlValue<FVector3f>(ControlName, (FVector3f)Value, true, Context);
		}
	}
}


FRotator UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigRotator(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	FRotator Value = FRotator::ZeroRotator;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Rotator)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName,TimeUnit, Frame, RigValue))
		{
			Value = FRotator::MakeFromEuler((FVector)RigValue.Get<FVector3f>());
		}
	}
	return Value;
}

TArray<FRotator> UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigRotators(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<FRotator> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Rotator)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}
		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				FRotator Value = FRotator::MakeFromEuler((FVector)RigValue.Get<FVector3f>());
				Values.Add(Value);
			}
		}
	}
	return Values;
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigRotator(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FRotator Value, EMovieSceneTimeUnit TimeUnit, bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Rotator)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		TLLRN_ControlRig->SetControlValue<FVector3f>(ControlName, (FVector3f)Value.Euler(), true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigRotators(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
	const TArray<FRotator> Values, EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Rotator)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			FRotator Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlValue<FVector3f>(ControlName, (FVector3f)Value.Euler(), true, Context);
		}
	}
}


FVector UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlRigScale(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,  EMovieSceneTimeUnit TimeUnit)
{
	FVector Value = FVector::ZeroVector;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Scale)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName,TimeUnit, Frame, RigValue))
		{
			Value = (FVector)RigValue.Get<FVector3f>();
		}
	}
	return Value;
}

TArray<FVector>UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlRigScales(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<FVector> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Scale)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}
		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName,TimeUnit, Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				FVector Value = (FVector)RigValue.Get<FVector3f>();
				Values.Add(Value);
			}
		}
	}
	return Values;
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlRigScale(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FVector Value, EMovieSceneTimeUnit TimeUnit, bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Scale)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		TLLRN_ControlRig->SetControlValue<FVector3f>(ControlName, (FVector3f)Value, true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlRigScales(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, 
	const TArray<FVector> Values, EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Scale)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			FVector Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlValue<FVector3f>(ControlName, (FVector3f)Value, true, Context);
		}
	}
}


FEulerTransform UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlRigEulerTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	FEulerTransform Value = FEulerTransform::Identity;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::EulerTransform)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frame, RigValue))
		{
			Value = RigValue.Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform();
		}
	}
	return Value;
}

TArray<FEulerTransform> UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlRigEulerTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<FEulerTransform> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::EulerTransform)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}
		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				FEulerTransform Value = RigValue.Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform();
				Values.Add(Value);
			}
		}
	}
	return Values;
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlRigEulerTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FEulerTransform Value,
	EMovieSceneTimeUnit TimeUnit, bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::EulerTransform)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlName))
		{
			FVector EulerAngle(Value.Rotation.Roll, Value.Rotation.Pitch, Value.Rotation.Yaw);
			TLLRN_ControlRig->GetHierarchy()->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
		}
		TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FEulerTransform_Float>(ControlName, Value, true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlRigEulerTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
	const TArray<FEulerTransform> Values, EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::EulerTransform)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			FEulerTransform Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));

			if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlName))
			{
				FVector EulerAngle(Value.Rotation.Roll, Value.Rotation.Pitch, Value.Rotation.Yaw);
				TLLRN_ControlRig->GetHierarchy()->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
			}
			TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FEulerTransform_Float>(ControlName, Value, true, Context);
		}
	}
}


FTransformNoScale UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigTransformNoScale(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	FTransformNoScale Value = FTransformNoScale::Identity;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::TransformNoScale)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName,TimeUnit, Frame, RigValue))
		{
			Value = RigValue.Get<FTLLRN_RigControlValue::FTransformNoScale_Float>().ToTransform();
		}
	}
	return Value;
}

TArray<FTransformNoScale> UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigTransformNoScales(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<FTransformNoScale> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::TransformNoScale)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}
		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName,TimeUnit, Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				FTransformNoScale Value = RigValue.Get<FTLLRN_RigControlValue::FTransformNoScale_Float>().ToTransform();
				Values.Add(Value);
			}
		}
	}
	return Values;
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigTransformNoScale(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FTransformNoScale Value,
	EMovieSceneTimeUnit TimeUnit,bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::TransformNoScale)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransformNoScale_Float>(ControlName, Value, true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigTransformNoScales(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, 
	const TArray<FTransformNoScale> Values, EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::TransformNoScale)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			FTransformNoScale Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransformNoScale_Float>(ControlName, Value, true, Context);
		}
	}
}


FTransform UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit)
{
	FTransform Value = FTransform::Identity;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Transform)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Value;
			}
		}
		
		FTLLRN_RigControlValue RigValue;
		if (GetTLLRN_ControlRigValue(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frame, RigValue))
		{
			Value = RigValue.Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
		}
	}
	return Value;
}

TArray<FTransform> UTLLRN_ControlRigSequencerEditorLibrary::GetLocalTLLRN_ControlTLLRN_RigTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit)
{
	TArray<FTransform> Values;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();

	if (WeakSequencer.IsValid() && TLLRN_ControlRig)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			if (Element->Settings.ControlType != ETLLRN_RigControlType::Transform)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
				return Values;
			}
		}
		TArray<FTLLRN_RigControlValue> RigValues;
		if (GetTLLRN_ControlRigValues(WeakSequencer.Pin().Get(), TLLRN_ControlRig, ControlName, TimeUnit, Frames, RigValues))
		{
			Values.Reserve(RigValues.Num());
			for (const FTLLRN_RigControlValue& RigValue : RigValues)
			{
				FTransform Value = RigValue.Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
				Values.Add(Value);
			}
		}
	}
	return Values;
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FTransform Value,
	EMovieSceneTimeUnit TimeUnit,bool bSetKey)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Transform)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
		}
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = bSetKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
		Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
		TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransform_Float>(ControlName, Value, true, Context);
	}
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetLocalTLLRN_ControlTLLRN_RigTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, 
	const TArray<FTransform> Values, EMovieSceneTimeUnit TimeUnit)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr || (Frames.Num() != Values.Num()))
	{
		return;
	}
	if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
	{
		if (Element->Settings.ControlType != ETLLRN_RigControlType::Transform)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig Wrong Type"));
			return;
		}
	}
	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		FFrameRate TickResolution = MovieScene->GetTickResolution();
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			FFrameNumber Frame = Frames[Index];
			if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
			{
				Frame = FFrameRate::TransformTime(FFrameTime(Frame, 0), MovieScene->GetDisplayRate(), MovieScene->GetTickResolution()).RoundToFrame();
			}
			FTransform Value = Values[Index];
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransform_Float>(ControlName, Value, true, Context);
		}
	}
}

bool UTLLRN_ControlRigSequencerEditorLibrary::ImportFBXToTLLRN_ControlRigTrack(UWorld* World, ULevelSequence* Sequence, UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, UMovieSceneTLLRN_ControlRigParameterSection* InSection,
	const TArray<FString>& TLLRN_ControlTLLRN_RigNames,
	UMovieSceneUserImportFBXControlRigSettings* ImportFBXTLLRN_ControlRigSettings,
	const FString& ImportFilename)
{

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene || MovieScene->IsReadOnly() || !InTrack)
	{
		return false;
	}

	bool bValid = false;
	ALevelSequenceActor* OutActor;
	FMovieSceneSequencePlaybackSettings Settings;
	FLevelSequenceCameraSettings CameraSettings;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Sequence, Settings, OutActor);

	INodeAndChannelMappings* ChannelMapping = Cast<INodeAndChannelMappings>(InTrack);
	if (ChannelMapping)
	{
		TArray<FRigControlFBXNodeAndChannels>* NodeAndChannels = ChannelMapping->GetNodeAndChannelMappings(InSection);
		TArray<FName> SelectedControls;
		for (const FString& StringName : TLLRN_ControlTLLRN_RigNames)
		{
			FName Name(*StringName);
			SelectedControls.Add(Name);
		}

		bValid = MovieSceneToolHelpers::ImportFBXIntoControlRigChannels(MovieScene, ImportFilename, ImportFBXTLLRN_ControlRigSettings,
			NodeAndChannels, SelectedControls, MovieScene->GetTickResolution());
		if (NodeAndChannels)
		{
			delete NodeAndChannels;
		}
	}
	return bValid;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::ExportFBXFromTLLRN_ControlRigSection(ULevelSequence* Sequence, const UMovieSceneTLLRN_ControlRigParameterSection* Section, const UMovieSceneUserExportFBXControlRigSettings* ExportFBXTLLRN_ControlRigSettings)
{
	if (!Sequence || !ExportFBXTLLRN_ControlRigSettings ||
		!Section || !Section->GetTLLRN_ControlRig() ||
		!Sequence->GetMovieScene() || Sequence->GetMovieScene()->IsReadOnly())
	{
		return false;
	}

	TArray<FName> SelectedControls;
	if (UMovieSceneTLLRN_ControlRigParameterTrack* Track = Section->GetTypedOuter<UMovieSceneTLLRN_ControlRigParameterTrack>())
	{
		Track->GetSelectedNodes(SelectedControls);
	}

	FMovieSceneSequenceTransform RootToLocalTransform;
	if (IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Sequence, false))
	{
		if (const ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor))
		{
			if (const TSharedPtr<ISequencer> Sequencer = LevelSequenceEditor->GetSequencer())
			{
				RootToLocalTransform = Sequencer->GetFocusedMovieSceneSequenceTransform();
			}
		}
	}
	
	return  MovieSceneToolHelpers::ExportFBXFromControlRigChannels(Section, ExportFBXTLLRN_ControlRigSettings, SelectedControls, RootToLocalTransform);
}

bool UTLLRN_ControlRigSequencerEditorLibrary::CollapseTLLRN_ControlRigTLLRN_TLLRN_AnimLayersWithSettings(ULevelSequence* InSequence, UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, const FBakingAnimationKeySettings& InSettings)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	bool bValid = false;
	if (WeakSequencer.IsValid() && InTrack)
	{
		TSharedPtr<ISequencer>  SequencerPtr = WeakSequencer.Pin();
		bValid = FTLLRN_ControlRigParameterTrackEditor::CollapseAllLayers(SequencerPtr, InTrack, InSettings);
		
	}
	return bValid;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::CollapseTLLRN_ControlRigTLLRN_TLLRN_AnimLayers(ULevelSequence* LevelSequence, UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, bool bKeyReduce, float Tolerance)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	bool bValid = false;

	if (WeakSequencer.IsValid() && InTrack)
	{
		TSharedPtr<ISequencer>  SequencerPtr = WeakSequencer.Pin();
		FBakingAnimationKeySettings CollapseControlsSettings;
		const FFrameRate TickResolution = SequencerPtr->GetFocusedTickResolution();
		const FFrameTime FrameTime = SequencerPtr->GetLocalTime().ConvertTo(TickResolution);
		FFrameNumber CurrentTime = FrameTime.GetFrame();

		TRange<FFrameNumber> Range = SequencerPtr->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange();

		CollapseControlsSettings.StartFrame = Range.GetLowerBoundValue();
		CollapseControlsSettings.EndFrame = Range.GetUpperBoundValue();
		bValid = FTLLRN_ControlRigParameterTrackEditor::CollapseAllLayers(SequencerPtr,InTrack, CollapseControlsSettings);	
	}
	return bValid;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::SetTLLRN_ControlTLLRN_RigSpace(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const FTLLRN_RigElementKey& InSpaceKey, FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	bool bValid = false;

	if (WeakSequencer.IsValid() && TLLRN_ControlRig && ControlName != NAME_None)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
			FScopedTransaction Transaction(LOCTEXT("KeyTLLRN_ControlTLLRN_RigSpace", "Key Control Rig Space"));
			FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig, ControlName, Sequencer.Get(), true /*bCreateIfNeeded*/);
			if (SpaceChannelAndSection.SpaceChannel)
			{
				if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
				{
					InTime = FFrameRate::TransformTime(FFrameTime(InTime, 0), LevelSequence->GetMovieScene()->GetDisplayRate(), LevelSequence->GetMovieScene()->GetTickResolution()).RoundToFrame();
				}
				FKeyHandle Handle = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerKeyTLLRN_ControlTLLRN_RigSpaceChannel(TLLRN_ControlRig, Sequencer.Get(), SpaceChannelAndSection.SpaceChannel, SpaceChannelAndSection.SectionToKey, InTime, TLLRN_ControlRig->GetHierarchy(), Element->GetKey(), InSpaceKey);
				bValid = Handle != FKeyHandle::Invalid();
			}
			else
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Can not find Space Channel"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Can not find Control with that Name"));
			return false;
		}
	}
	return bValid;
}


bool UTLLRN_ControlRigSequencerEditorLibrary::SpaceCompensate(UTLLRN_ControlRig* InTLLRN_ControlRig, FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() && InTLLRN_ControlRig)
	{
		TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
		TOptional<FFrameNumber> OptionalTime;
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
			const FFrameRate& FrameRate = Sequencer->GetFocusedDisplayRate();
			OptionalTime = FFrameRate::TransformTime(FFrameTime(InTime, 0), FrameRate, TickResolution).RoundToFrame();
		}
		else
		{
			OptionalTime = InTime;
		}
		FScopedTransaction Transaction(LOCTEXT("SpacecCompensate", "Space Compensate"));
		// compensate spaces
		if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetTLLRN_ControlRigSection(Sequencer.Get(), InTLLRN_ControlRig))
		{
			// compensate spaces
			FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::CompensateIfNeeded(
				InTLLRN_ControlRig, Sequencer.Get(), CRSection,
				OptionalTime, true /*comp previous*/);
		}
		return true;
	}
	return false;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::SpaceCompensateAll(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	bool bValid = false;
	TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
	if (WeakSequencer.IsValid() && InTLLRN_ControlRig)
	{
		TOptional<FFrameNumber> OptionalTime;
		FScopedTransaction Transaction(LOCTEXT("SpacecCompensateAll", "Space Compensate All"));
		// compensate spaces
		if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetTLLRN_ControlRigSection(Sequencer.Get(), InTLLRN_ControlRig))
		{
			// compensate spaces
			FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::CompensateIfNeeded(
				InTLLRN_ControlRig, Sequencer.Get(), CRSection,
				OptionalTime, true /*comp previous*/);
		}
		return true;
	}
	return false;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::BakeTLLRN_ControlTLLRN_RigSpace(ULevelSequence* InSequence, UTLLRN_ControlRig* InTLLRN_ControlRig, const TArray<FName>& InControlNames, FTLLRN_TLLRN_RigSpacePickerBakeSettings InSettings, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	bool bValid = false;

	if (WeakSequencer.IsValid() && InTLLRN_ControlRig && InControlNames.Num() > 0)
	{
		TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
		const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
		const FFrameRate& FrameRate = Sequencer->GetFocusedDisplayRate();
		FFrameNumber FrameRateInFrameNumber = TickResolution.AsFrameNumber(FrameRate.AsInterval());
		if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
		{
			InSettings.Settings.StartFrame = FFrameRate::TransformTime(FFrameTime(InSettings.Settings.StartFrame, 0), FrameRate, TickResolution).RoundToFrame();
			InSettings.Settings.EndFrame = FFrameRate::TransformTime(FFrameTime(InSettings.Settings.EndFrame, 0), FrameRate, TickResolution).RoundToFrame();
		}

		FScopedTransaction Transaction(LOCTEXT("BakeControlToSpace", "Bake Control In Space"));
		for (const FName& ControlName : InControlNames)
		{
			if (FTLLRN_RigControlElement* Element = InTLLRN_ControlRig->FindControl(ControlName))
			{
				FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(InTLLRN_ControlRig, ControlName, Sequencer.Get(), false /*bCreateIfNeeded*/);
				if (SpaceChannelAndSection.SpaceChannel)
				{
					FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerBakeControlInSpace(InTLLRN_ControlRig, Sequencer.Get(), SpaceChannelAndSection.SpaceChannel, SpaceChannelAndSection.SectionToKey,
						InTLLRN_ControlRig->GetHierarchy(), Element->GetKey(), InSettings);
				}
			}
		}
		bValid = true;
	}
	return bValid;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::DeleteTLLRN_ControlTLLRN_RigSpace(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	bool bValid = false;

	if (WeakSequencer.IsValid() && TLLRN_ControlRig && ControlName != NAME_None)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
			FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig, ControlName, Sequencer.Get(), false /*bCreateIfNeeded*/);
			if (SpaceChannelAndSection.SpaceChannel)
			{

				if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
				{
					InTime = FFrameRate::TransformTime(FFrameTime(InTime, 0), LevelSequence->GetMovieScene()->GetDisplayRate(), LevelSequence->GetMovieScene()->GetTickResolution()).RoundToFrame();
				}
				UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SpaceChannelAndSection.SectionToKey);


				TArray<FFrameNumber> OurKeyTimes;
				TArray<FKeyHandle> OurKeyHandles;
				TRange<FFrameNumber> CurrentFrameRange;
				CurrentFrameRange.SetLowerBound(TRangeBound<FFrameNumber>(InTime));
				CurrentFrameRange.SetUpperBound(TRangeBound<FFrameNumber>(InTime));
				TMovieSceneChannelData<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> ChannelInterface = SpaceChannelAndSection.SpaceChannel->GetData();
				ChannelInterface.GetKeys(CurrentFrameRange, &OurKeyTimes, &OurKeyHandles);
				if (OurKeyHandles.Num() > 0)
				{
					FScopedTransaction DeleteKeysTransaction(LOCTEXT("DeleteSpaceChannelKeys_Transaction", "Delete Space Channel Keys"));
					ParamSection->TryModify();
					SpaceChannelAndSection.SpaceChannel->DeleteKeys(OurKeyHandles);
					WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
					bValid = true;
				}
				else
				{
					UE_LOG(LogTLLRN_ControlRig, Error, TEXT("No Keys To Delete At that Time"));
					return false;
				}
			}
			else
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Can not find Space Channel"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Can not find Control with that Name"));
			return false;
		}
	}
	return bValid;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::MoveTLLRN_ControlTLLRN_RigSpace(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber InTime, FFrameNumber InNewTime, EMovieSceneTimeUnit TimeUnit)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	bool bValid = false;

	if (WeakSequencer.IsValid() && TLLRN_ControlRig && ControlName != NAME_None)
	{
		if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(ControlName))
		{
			TSharedPtr<ISequencer>  Sequencer = WeakSequencer.Pin();
			FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig, ControlName, Sequencer.Get(), false /*bCreateIfNeeded*/);
			if (SpaceChannelAndSection.SpaceChannel)
			{

				if (TimeUnit == EMovieSceneTimeUnit::DisplayRate)
				{
					InTime = FFrameRate::TransformTime(FFrameTime(InTime, 0), LevelSequence->GetMovieScene()->GetDisplayRate(), LevelSequence->GetMovieScene()->GetTickResolution()).RoundToFrame();
					InNewTime = FFrameRate::TransformTime(FFrameTime(InNewTime, 0), LevelSequence->GetMovieScene()->GetDisplayRate(), LevelSequence->GetMovieScene()->GetTickResolution()).RoundToFrame();
				}
				UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SpaceChannelAndSection.SectionToKey);

				TArray<FFrameNumber> OurKeyTimes;
				TArray<FKeyHandle> OurKeyHandles;
				TRange<FFrameNumber> CurrentFrameRange;
				CurrentFrameRange.SetLowerBound(TRangeBound<FFrameNumber>(InTime));
				CurrentFrameRange.SetUpperBound(TRangeBound<FFrameNumber>(InTime));
				TMovieSceneChannelData<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> ChannelInterface = SpaceChannelAndSection.SpaceChannel->GetData();
				ChannelInterface.GetKeys(CurrentFrameRange, &OurKeyTimes, &OurKeyHandles);
				if (OurKeyHandles.Num() > 0)
				{
					FScopedTransaction MoveKeys(LOCTEXT("MoveSpaceChannelKeys_Transaction", "Move Space Channel Keys"));
					ParamSection->TryModify();
					TArray<FFrameNumber> NewKeyTimes;
					NewKeyTimes.Add(InNewTime);
					SpaceChannelAndSection.SpaceChannel->SetKeyTimes(OurKeyHandles, NewKeyTimes);
					WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
					bValid = true;
				}
				else
				{
					UE_LOG(LogTLLRN_ControlRig, Error, TEXT("No Key To Move At that Time"));
					return false;
				}
			}
			else
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Can not find Space Channel"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Can not find Control with that Name"));
			return false;
		}
	}
	return bValid;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::RenameTLLRN_ControlTLLRN_RigControlChannels(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, const TArray<FName>& InOldControlNames, const TArray<FName>& InNewControlNames)
{
	if (LevelSequence == nullptr || TLLRN_ControlRig == nullptr)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("LevelSequence and Control Rig must be valid"));
		return false;
	}
	if (InOldControlNames.Num() != InNewControlNames.Num() || InOldControlNames.Num() < 1)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Old and New Control Name arrays don't match in length"));
		return false;
	}
	for (const FName& NewName : InNewControlNames)
	{
		if (TLLRN_ControlRig->FindControl(NewName) == nullptr)
		{
			const FString StringName = NewName.ToString();
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Missing Control Name %s"), *(StringName));
			return false;
		}
	}
	bool bValid = false;


	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (MovieScene)
	{
		const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();
		for (const FMovieSceneBinding& Binding : Bindings)
		{
			TArray<UMovieSceneTrack*> Tracks = MovieScene->FindTracks(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.GetObjectGuid(), NAME_None);
			for (UMovieSceneTrack* AnyOleTrack : Tracks)
			{
				UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(AnyOleTrack);
				if (Track && Track->GetTLLRN_ControlRig() == TLLRN_ControlRig)
				{
					Track->Modify();
					bValid = true;
					for (int32 Index = 0; Index < InOldControlNames.Num(); ++Index)
					{
						Track->RenameParameterName(InOldControlNames[Index], InNewControlNames[Index]);
					}
				}
			}
		}
	}
	
	return bValid;
}

FTLLRN_RigElementKey UTLLRN_ControlRigSequencerEditorLibrary::GetDefaultParentKey()
{
	return UTLLRN_RigHierarchy::GetDefaultParentKey();
}

FTLLRN_RigElementKey UTLLRN_ControlRigSequencerEditorLibrary::GetWorldSpaceReferenceKey()
{
	return UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey();
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetTLLRN_ControlRigPriorityOrder(UMovieSceneTrack* InTrack, int32 PriorityOrder)
{
	UMovieSceneTLLRN_ControlRigParameterTrack* ParameterTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(InTrack);
	if (!ParameterTrack)
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot call SetTLLRN_ControlRigPriorityOrder without a UMovieSceneTLLRN_ControlRigParameterTrack"), ELogVerbosity::Error);
		return;
	}

	ParameterTrack->SetPriorityOrder(PriorityOrder);
}

int32 UTLLRN_ControlRigSequencerEditorLibrary::GetTLLRN_ControlRigPriorityOrder(UMovieSceneTrack* InTrack)
{
	UMovieSceneTLLRN_ControlRigParameterTrack* ParameterTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(InTrack);
	if (!ParameterTrack)
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot call SetTLLRN_ControlRigPriorityOrder without a UMovieSceneTLLRN_ControlRigParameterTrack"), ELogVerbosity::Error);
		return INDEX_NONE;
	}
	return 	ParameterTrack->GetPriorityOrder();

}

bool UTLLRN_ControlRigSequencerEditorLibrary::GetControlsMask(UMovieSceneSection* InSection, FName ControlName)
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);
	if (!ParameterSection)
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot call GetControlsMask without a UMovieSceneTLLRN_ControlRigParameterSection"), ELogVerbosity::Error);
		return false;
	}
	
	UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ParameterSection->GetTLLRN_ControlRig());
	if (!TLLRN_ControlRig)
	{
		FFrame::KismetExecutionMessage(TEXT("Section does not have a control rig"), ELogVerbosity::Error);
		return false;
	}
	return ParameterSection->GetControlNameMask(ControlName);
}

void UTLLRN_ControlRigSequencerEditorLibrary::SetControlsMask(UMovieSceneSection* InSection, const TArray<FName>& ControlNames, bool bVisible)
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);
	if (!ParameterSection)
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot call SetControlsMask without a UMovieSceneTLLRN_ControlRigParameterSection"), ELogVerbosity::Error);
		return;
	}

	UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ParameterSection->GetTLLRN_ControlRig());
	if (!TLLRN_ControlRig)
	{
		FFrame::KismetExecutionMessage(TEXT("Section does not have a control rig"), ELogVerbosity::Error);
		return;
	}

	ParameterSection->Modify();
	for (const FName& ControlName : ControlNames)
	{
		ParameterSection->SetControlNameMask(ControlName, bVisible);
	}
	
}

void UTLLRN_ControlRigSequencerEditorLibrary::ShowAllControls(UMovieSceneSection* InSection)
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);
	if (!ParameterSection)
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot call ShowAllControls without a UMovieSceneTLLRN_ControlRigParameterSection"), ELogVerbosity::Error);
		return;
	}

	ParameterSection->Modify();
	ParameterSection->FillControlNameMask(true);
}

void UTLLRN_ControlRigSequencerEditorLibrary::HideAllControls(UMovieSceneSection* InSection)
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);
	if (!ParameterSection)
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot call HideAllControls without a UMovieSceneTLLRN_ControlRigParameterSection"), ELogVerbosity::Error);
		return;
	}

	ParameterSection->Modify();
	ParameterSection->FillControlNameMask(false);
}

bool UTLLRN_ControlRigSequencerEditorLibrary::IsFKTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	return (InTLLRN_ControlRig && InTLLRN_ControlRig->IsA<UFKTLLRN_ControlRig>());
}

bool UTLLRN_ControlRigSequencerEditorLibrary::IsLayeredTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	return (InTLLRN_ControlRig && InTLLRN_ControlRig->IsAdditive());
}

bool UTLLRN_ControlRigSequencerEditorLibrary::MarkLayeredModeOnTrackDisplay(UMovieSceneTLLRN_ControlRigParameterTrack* InTrack)
{
	if (!InTrack)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid track"), ELogVerbosity::Error);
		return false;
	}

	UTLLRN_ControlRig* TLLRN_ControlRig = InTrack->GetTLLRN_ControlRig();
	if (!TLLRN_ControlRig)
	{
		FFrame::KismetExecutionMessage(TEXT("Track does not have a control rig"), ELogVerbosity::Error);
		return false;
	}

	if (TLLRN_ControlRig->IsAdditive())
	{
		InTrack->SetColorTint(UMovieSceneTLLRN_ControlRigParameterTrack::LayeredRigTrackColor);
	}
	else
	{
		InTrack->SetColorTint(UMovieSceneTLLRN_ControlRigParameterTrack::AbsoluteRigTrackColor);
	}

	return true;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::SetTLLRN_ControlRigLayeredMode(UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, bool bSetIsLayered)
{
	if (!InTrack)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid track"), ELogVerbosity::Error);
		return false;
	}

	UTLLRN_ControlRig* TLLRN_ControlRig = InTrack->GetTLLRN_ControlRig();
	if (!TLLRN_ControlRig)
	{
		FFrame::KismetExecutionMessage(TEXT("Track does not have a control rig"), ELogVerbosity::Error);
		return false;
	}

	if (TLLRN_ControlRig->IsAdditive() == bSetIsLayered)
	{
		if(bSetIsLayered)
		{
			FFrame::KismetExecutionMessage(TEXT("Control rig is already in layered mode"), ELogVerbosity::Error);
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Control rig is already in absolute mode"), ELogVerbosity::Error);
		}
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("ConvertToLayeredTLLRN_ControlRig_Transaction", "Convert to Layered Control Rig"));
	InTrack->Modify();
	TLLRN_ControlRig->Modify();

	if (UFKTLLRN_ControlRig* FKRig = Cast<UFKTLLRN_ControlRig>(TLLRN_ControlRig))
	{
		FKRig->SetApplyMode(bSetIsLayered ? ETLLRN_ControlRigFKRigExecuteMode::Additive : ETLLRN_ControlRigFKRigExecuteMode::Replace);
	}
	else
	{
		TLLRN_ControlRig->ClearPoseBeforeBackwardsSolve();
		TLLRN_ControlRig->ResetControlValues();
		TLLRN_ControlRig->SetIsAdditive(bSetIsLayered);

		TLLRN_ControlRig->Evaluate_AnyThread();
	}

	FString ObjectName = TLLRN_ControlRig->GetClass()->GetName(); //GetDisplayNameText().ToString();
	ObjectName.RemoveFromEnd(TEXT("_C"));
	InTrack->SetTrackName(FName(*ObjectName));
	
	MarkLayeredModeOnTrackDisplay(InTrack);

	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
	if (TLLRN_ControlRigEditMode)
	{
		TLLRN_ControlRigEditMode->ZeroTransforms(false);
	}

	for (UMovieSceneSection* Section : InTrack->GetAllSections())
	{
		if (Section)
		{
			UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section);
			if (CRSection)
			{
				Section->Modify();
				CRSection->ClearAllParameters();
				CRSection->RecreateWithThisTLLRN_ControlRig(CRSection->GetTLLRN_ControlRig(), true);
			}
		}
	}

	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid())
	{
		WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
	}

	return true;
}

ETLLRN_ControlRigFKRigExecuteMode UTLLRN_ControlRigSequencerEditorLibrary::GetFKTLLRN_ControlRigApplyMode(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	ETLLRN_ControlRigFKRigExecuteMode ApplyMode = ETLLRN_ControlRigFKRigExecuteMode::Direct;
	if (UFKTLLRN_ControlRig* FKRig = Cast<UFKTLLRN_ControlRig>(Cast<UTLLRN_ControlRig>(InTLLRN_ControlRig)))
	{
		ApplyMode = FKRig->GetApplyMode();
	}
	return ApplyMode;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::SetTLLRN_ControlRigApplyMode(UTLLRN_ControlRig* InTLLRN_ControlRig, ETLLRN_ControlRigFKRigExecuteMode InApplyMode)
{
	if (UFKTLLRN_ControlRig* FKRig = Cast<UFKTLLRN_ControlRig>(Cast<UTLLRN_ControlRig>(InTLLRN_ControlRig)))
	{
		FKRig->SetApplyMode(InApplyMode);
		return true;
	}
	return false;
}

bool UTLLRN_ControlRigSequencerEditorLibrary::DeleteTLLRN_AnimLayer(int32 Index)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("DeleteTLLRN_AnimLayer: Need open Sequencer"));
		return false;
	}
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
	{
		if (Index != INDEX_NONE)
		{
			return TLLRN_TLLRN_AnimLayers->DeleteTLLRN_AnimLayer(SequencerPtr.Get(), Index);
		}
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("DeleteTLLRN_AnimLayer: No Anim Layers on Level Sequence"));
	}
	return false;
}

int32 UTLLRN_ControlRigSequencerEditorLibrary::DuplicateTLLRN_AnimLayer(int32 Index)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("DuplicateTLLRN_AnimLayer: Need open Sequencer"));
		return INDEX_NONE;
	}
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
	{
		if (Index != INDEX_NONE)
		{
			return TLLRN_TLLRN_AnimLayers->DuplicateTLLRN_AnimLayer(SequencerPtr.Get(), Index);
		}
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("DuplicateTLLRN_AnimLayer: No Anim Layers on Level Sequence"));
	}
	return INDEX_NONE;
}

int32 UTLLRN_ControlRigSequencerEditorLibrary::AddTLLRN_AnimLayerFromSelection()
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("AddTLLRN_AnimLayerFromSelection: Need open Sequencer"));
		return INDEX_NONE;
	}
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
	{
		return TLLRN_TLLRN_AnimLayers->AddTLLRN_AnimLayerFromSelection(SequencerPtr.Get());
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("AddTLLRN_AnimLayerFromSelection: No Anim Layers on Level Sequence"));
	}
	return INDEX_NONE;

}

bool UTLLRN_ControlRigSequencerEditorLibrary::MergeTLLRN_TLLRN_AnimLayers(const TArray<int32>& Indices)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("MergeTLLRN_TLLRN_AnimLayers: Need open Sequencer"));
		return false;
	}
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
	{
		return TLLRN_TLLRN_AnimLayers->MergeTLLRN_TLLRN_AnimLayers(SequencerPtr.Get(), Indices, nullptr);
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("MergeTLLRN_TLLRN_AnimLayers: No Anim Layers on Level Sequence"));
	}
	return false;
}

TArray<UTLLRN_AnimLayer*> UTLLRN_ControlRigSequencerEditorLibrary::GetTLLRN_TLLRN_AnimLayers()
{
	TArray<UTLLRN_AnimLayer*> TLLRN_TLLRN_AnimLayersArray;
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("GetTLLRN_TLLRN_AnimLayers: Need open Sequencer"));
		return TLLRN_TLLRN_AnimLayersArray;
	}
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
	{
		return TLLRN_TLLRN_AnimLayers->TLLRN_TLLRN_AnimLayers;
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("GetTLLRN_TLLRN_AnimLayers: No Anim Layers on Level Sequence"));
	}
	return TLLRN_TLLRN_AnimLayersArray;
}

int32 UTLLRN_ControlRigSequencerEditorLibrary::GetTLLRN_AnimLayerIndex(UTLLRN_AnimLayer* TLLRN_AnimLayer)
{
	TWeakPtr<ISequencer> WeakSequencer = GetSequencerFromAsset();
	if (WeakSequencer.IsValid() == false)
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("GetTLLRN_AnimLayerIndex: Need open Sequencer"));
		return INDEX_NONE;
	}
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
	{
		return TLLRN_TLLRN_AnimLayers->GetTLLRN_AnimLayerIndex(TLLRN_AnimLayer);
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("GetTLLRN_AnimLayerIndex: No Anim Layers on Level Sequence"));
	}
	return INDEX_NONE;
}

#undef LOCTEXT_NAMESPACE

