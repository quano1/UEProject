// Copyright Epic Games, Inc. All Rights Reserved.

#include "ControlConstraintChannelInterface.h"

#include "TLLRN_ControlTLLRN_RigSpaceChannelEditors.h"
#include "ISequencer.h"

#include "Constraints/TLLRN_ControlTLLRN_RigTransformableHandle.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "LevelSequence.h"
#include "MovieSceneToolHelpers.h"

#include "TransformConstraint.h"
#include "Algo/Copy.h"
#include "ConstraintsManager.h"
#include "ScopedTransaction.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Constraints/MovieSceneConstraintChannelHelper.inl"

#define LOCTEXT_NAMESPACE "Constraints"

namespace
{
	UMovieScene* GetMovieScene(const TSharedPtr<ISequencer>& InSequencer)
	{
		const UMovieSceneSequence* MovieSceneSequence = InSequencer ? InSequencer->GetFocusedMovieSceneSequence() : nullptr;
		return MovieSceneSequence ? MovieSceneSequence->GetMovieScene() : nullptr;
	}
}

UMovieSceneSection* FControlConstraintChannelInterface::GetHandleSection(
	const UTransformableHandle* InHandle,
	const TSharedPtr<ISequencer>& InSequencer)
{
	if (!IsValid(InHandle) || !InHandle->IsValid())
	{
		return nullptr;
	}
	
	const UTLLRN_TransformableControlHandle* ControlHandle = static_cast<const UTLLRN_TransformableControlHandle*>(InHandle);
	static constexpr bool bConstraintSection = false;
	return GetControlSection(ControlHandle, InSequencer, bConstraintSection);
}

UMovieSceneSection* FControlConstraintChannelInterface::GetHandleConstraintSection(const UTransformableHandle* InHandle, const TSharedPtr<ISequencer>& InSequencer)
{
	if (!IsValid(InHandle) || !InHandle->IsValid())
	{
		return nullptr;
	}

	const UTLLRN_TransformableControlHandle* ControlHandle = static_cast<const UTLLRN_TransformableControlHandle*>(InHandle);
	static constexpr bool bConstraintSection = true;
	return GetControlSection(ControlHandle, InSequencer, bConstraintSection);
}

UWorld* FControlConstraintChannelInterface::GetHandleWorld(UTransformableHandle* InHandle)
{
	if (!IsValid(InHandle) || !InHandle->IsValid())
	{
		return nullptr;
	}
	
	const UTLLRN_TransformableControlHandle* ControlHandle = static_cast<UTLLRN_TransformableControlHandle*>(InHandle);
	const UTLLRN_ControlRig* TLLRN_ControlRig = ControlHandle->TLLRN_ControlRig.LoadSynchronous();

	return TLLRN_ControlRig ? TLLRN_ControlRig->GetWorld() : nullptr;
}

bool FControlConstraintChannelInterface::SmartConstraintKey(
	UTickableTransformConstraint* InConstraint, const TOptional<bool>& InOptActive,
	const FFrameNumber& InTime, const TSharedPtr<ISequencer>& InSequencer)
{
	const UTLLRN_TransformableControlHandle* ControlHandle = Cast<UTLLRN_TransformableControlHandle>(InConstraint->ChildTRSHandle);
	if (!ControlHandle)
	{
		return false;
	}

	UMovieSceneTLLRN_ControlRigParameterSection* ConstraintSection = GetControlSection(ControlHandle, InSequencer, true);
	UMovieSceneTLLRN_ControlRigParameterSection* TransformSection = GetControlSection(ControlHandle, InSequencer, false);
	if ((!ConstraintSection) || (!TransformSection))
	{
		return false;
	}
	
	FScopedTransaction Transaction(LOCTEXT("KeyConstraintaKehy", "Key Constraint Key"));
	ConstraintSection->Modify();
	TransformSection->Modify();

	// set constraint as dynamic
	InConstraint->bDynamicOffset = true;
	
	//check if static if so we need to delete it from world, will get added later again
	if (UConstraintsManager* Manager = InConstraint->GetTypedOuter<UConstraintsManager>())
	{
		Manager->RemoveStaticConstraint(InConstraint);
	}

	// add the channel
	ConstraintSection->AddConstraintChannel(InConstraint);

	// add key if needed
	if (FConstraintAndActiveChannel* Channel = ConstraintSection->GetConstraintChannel(InConstraint->ConstraintID))
	{
		bool ActiveValueToBeSet = false;
		//add key if we can and make sure the key we are setting is what we want
		if (CanAddKey(Channel->ActiveChannel, InTime, ActiveValueToBeSet) && (InOptActive.IsSet() == false || InOptActive.GetValue() == ActiveValueToBeSet))
		{
			const FFrameRate TickResolution = InSequencer->GetFocusedTickResolution();

			const bool bNeedsCompensation = InConstraint->NeedsCompensation();
				
			TGuardValue<bool> CompensateGuard(FMovieSceneConstraintChannelHelper::bDoNotCompensate, true);
			TGuardValue<bool> RemoveConstraintGuard(FConstraintsManagerController::bDoNotRemoveConstraint, true);

			UTLLRN_ControlRig* TLLRN_ControlRig = ControlHandle->TLLRN_ControlRig.Get();
			const FName& ControlName = ControlHandle->ControlName;
				
			// store the frames to compensate
			const TArrayView<FMovieSceneFloatChannel*> Channels = ControlHandle->GetFloatChannels(TransformSection);
			TArray<FFrameNumber> FramesToCompensate;
			if (bNeedsCompensation)
			{
				FMovieSceneConstraintChannelHelper::GetFramesToCompensate<FMovieSceneFloatChannel>(Channel->ActiveChannel, ActiveValueToBeSet, InTime, Channels, FramesToCompensate);
			}
			else
			{
				FramesToCompensate.Add(InTime);
			}


			// store child and space transforms for these frames
			FCompensationEvaluator Evaluator(InConstraint);
			Evaluator.ComputeLocalTransforms(TLLRN_ControlRig->GetWorld(), InSequencer, FramesToCompensate, ActiveValueToBeSet);
			TArray<FTransform>& ChildLocals = Evaluator.ChildLocals;
			
			// store tangents at this time
			TArray<FMovieSceneTangentData> Tangents;
			int32 ChannelIndex = 0, NumChannels = 0;

			FTLLRN_ChannelMapInfo* pChannelIndex = nullptr;
			FTLLRN_RigControlElement* ControlElement = nullptr;
			Tie(ControlElement, pChannelIndex) = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetControlAndChannelInfo(TLLRN_ControlRig, TransformSection, ControlName);

			if (pChannelIndex && ControlElement)
			{
				// get the number of float channels to treat
				NumChannels = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetNumFloatChannels(ControlElement->Settings.ControlType);
				if (bNeedsCompensation && NumChannels > 0)
				{
					ChannelIndex = pChannelIndex->ChannelIndex;
					EvaluateTangentAtThisTime<FMovieSceneFloatChannel>(ChannelIndex, NumChannels, TransformSection, InTime, Tangents);
				}
			}
		
			const EMovieSceneTransformChannel ChannelsToKey =InConstraint->GetChannelsToKey();
			
			// add child's transform key at Time-1 to keep animation
			if (bNeedsCompensation)
			{
				const FFrameNumber TimeMinusOne(InTime - 1);

				ControlHandle->AddTransformKeys({ TimeMinusOne },
					{ ChildLocals[0] }, ChannelsToKey, TickResolution, nullptr,true);

				// set tangents at Time-1
				if (NumChannels > 0) //-V547
				{
					SetTangentsAtThisTime<FMovieSceneFloatChannel>(ChannelIndex, NumChannels, TransformSection, TimeMinusOne, Tangents);
				}
			}

			// add active key
			{
				TMovieSceneChannelData<bool> ChannelData = Channel->ActiveChannel.GetData();
				ChannelData.AddKey(InTime, ActiveValueToBeSet);
			}

			// compensate
			{
				// we need to remove the first transforms as we store NumFrames+1 transforms
				ChildLocals.RemoveAt(0);

				// add keys
				ControlHandle->AddTransformKeys(FramesToCompensate,
					ChildLocals, ChannelsToKey, TickResolution, nullptr,true);

				// set tangents at Time
				if (bNeedsCompensation && NumChannels > 0)
				{
					SetTangentsAtThisTime<FMovieSceneFloatChannel>(ChannelIndex, NumChannels, TransformSection, InTime, Tangents);
				}
			}
			return true;
		}
	}
	return false;
}

void FControlConstraintChannelInterface::AddHandleTransformKeys(
	const TSharedPtr<ISequencer>& InSequencer,
	const UTransformableHandle* InHandle,
	const TArray<FFrameNumber>& InFrames,
	const TArray<FTransform>& InLocalTransforms,
	const EMovieSceneTransformChannel& InChannels)
{
	ensure(InLocalTransforms.Num());

	if (!IsValid(InHandle) || !InHandle->IsValid())
	{
		return;
	}

	const UTLLRN_TransformableControlHandle* Handle = static_cast<const UTLLRN_TransformableControlHandle*>(InHandle);
	const UMovieScene* MovieScene = InSequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
	UMovieSceneSection* Section = nullptr; //control rig doesn't need section it instead
	Handle->AddTransformKeys(InFrames, InLocalTransforms, InChannels, MovieScene->GetTickResolution(), Section, true);
}

UMovieSceneTLLRN_ControlRigParameterSection* FControlConstraintChannelInterface::GetControlSection(
	const UTLLRN_TransformableControlHandle* InHandle,
	const TSharedPtr<ISequencer>& InSequencer,
	const bool bIsConstraint)
{
	if (!IsValid(InHandle) || !InHandle->IsValid())
	{
		return nullptr;
	}
	
	const UTLLRN_ControlRig* TLLRN_ControlRig = InHandle->TLLRN_ControlRig.LoadSynchronous();
	if (!TLLRN_ControlRig)
	{
		return nullptr;
	}
	
	const UMovieScene* MovieScene = GetMovieScene(InSequencer);
	if (!MovieScene)
	{
		return nullptr;
	}

	auto GetTLLRN_ControlRigTrack = [InHandle, MovieScene]()->UMovieSceneTLLRN_ControlRigParameterTrack*
	{
		const TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig = InHandle->TLLRN_ControlRig.LoadSynchronous();
		if (TLLRN_ControlRig.IsValid())
		{	
			const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();
			for (const FMovieSceneBinding& Binding : Bindings)
			{
				for (UMovieSceneTrack* Track : MovieScene->FindTracks(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.GetObjectGuid()))
				{
					UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track);
					if (TLLRN_ControlRigTrack && TLLRN_ControlRigTrack->GetTLLRN_ControlRig() == TLLRN_ControlRig)
					{
						return TLLRN_ControlRigTrack;
					}
				}
			}
		}
		return nullptr;
	};

	UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigTrack = GetTLLRN_ControlRigTrack();
	if (!TLLRN_ControlRigTrack)
	{
		return nullptr;
	}

	UMovieSceneSection* Section = TLLRN_ControlRigTrack->FindSection(0);
	if (bIsConstraint)
	{
		const TArray<UMovieSceneSection*>& AllSections = TLLRN_ControlRigTrack->GetAllSections();
		if (!AllSections.IsEmpty())
		{
			Section = AllSections[0]; 
		}
	}

	return Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section);
}

#undef LOCTEXT_NAMESPACE
