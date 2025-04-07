// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlTLLRN_RigSpaceChannelEditors.h"
#include "Constraints/MovieSceneConstraintChannelHelper.inl"
#include "MovieSceneSequence.h"
#include "MovieSceneSequenceEditor.h"
#include "MovieSceneEventUtils.h"
#include "Channels/MovieSceneChannelHandle.h"
#include "Sections/MovieSceneEventSectionBase.h"
#include "ISequencerChannelInterface.h"
#include "Widgets/SNullWidget.h"
#include "IKeyArea.h"
#include "ISequencer.h"
#include "SequencerSettings.h"
#include "MovieSceneCommonHelpers.h"
#include "GameFramework/Actor.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "MVVM/Views/KeyDrawParams.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneChannelEditorData.h"
#include "Channels/DoubleChannelCurveModel.h"
#include "Channels/FloatChannelCurveModel.h"
#include "Channels/IntegerChannelCurveModel.h"
#include "Channels/BoolChannelCurveModel.h"
#include "PropertyCustomizationHelpers.h"
#include "MovieSceneObjectBindingIDCustomization.h"
#include "MovieSceneObjectBindingIDPicker.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "SSocketChooser.h"
#include "EntitySystem/MovieSceneDecompositionQuery.h"
#include "EntitySystem/Interrogation/MovieSceneInterrogationLinker.h"
#include "EntitySystem/Interrogation/MovieSceneInterrogatedPropertyInstantiator.h"
#include "Systems/MovieScenePropertyInstantiator.h"
#include "Tracks/MovieScenePropertyTrack.h"
#include "MovieSceneSpawnableAnnotation.h"
#include "ISequencerModule.h"
#include "MovieSceneTracksComponentTypes.h"
#include "Editor/STLLRN_RigSpacePickerWidget.h"
#include "Tools/TLLRN_ControlRigSnapper.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "MovieScene.h"
#include "Sequencer/MovieSceneTLLRN_ControlTLLRN_RigSpaceChannel.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "MovieSceneObjectBindingID.h"
#include "TLLRN_ControlRig.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "TLLRN_ControlTLLRN_RigSpaceChannelCurveModel.h"
#include "ScopedTransaction.h"
#include "ITLLRN_ControlRigEditorModule.h"
#include "TimerManager.h"
#include "SequencerSectionPainter.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "CurveEditorSettings.h"
#include "TimeToPixel.h"
#include "Templates/Tuple.h"
#include "Sequencer/TLLRN_ControlRigSequencerHelpers.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigEditMode"

//flag used when we add or delete a space channel. When we do so we handle the compensation in those functions since we need to 
//compensate based upon the previous spaces, not the new space. So we set that this flag to be true and then
//do not compensate via the FTLLRN_ControlRigParameterTrackEditor setting a key.
static bool bDoNotCompensate = false;

static FKeyHandle SequencerOpenSpaceSwitchDialog(UTLLRN_ControlRig* TLLRN_ControlRig, TArray<FTLLRN_RigElementKey> SelectedControls,ISequencer* Sequencer, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey, FFrameNumber Time)
{
	FKeyHandle Handle = FKeyHandle::Invalid();
	if (TLLRN_ControlRig == nullptr || Sequencer == nullptr)
	{
		return Handle;
	}

	TSharedRef<STLLRN_RigSpacePickerWidget> PickerWidget =
	SNew(STLLRN_RigSpacePickerWidget)
	.Hierarchy(TLLRN_ControlRig->GetHierarchy())
	.Controls(SelectedControls)
	.Title(LOCTEXT("PickSpace", "Pick Space"))
	.AllowDelete(false)
	.AllowReorder(false)
	.AllowAdd(false)
	.GetControlCustomization_Lambda([TLLRN_ControlRig](UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey& InControlKey)
	{
		return TLLRN_ControlRig->GetControlCustomization(InControlKey);
	})
	.OnActiveSpaceChanged_Lambda([&Handle,TLLRN_ControlRig,Sequencer,Channel,SectionToKey,Time,SelectedControls](UTLLRN_RigHierarchy* TLLRN_RigHierarchy, const FTLLRN_RigElementKey& ControlKey, const FTLLRN_RigElementKey& SpaceKey)
	{
			
		Handle = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerKeyTLLRN_ControlTLLRN_RigSpaceChannel(TLLRN_ControlRig, Sequencer, Channel, SectionToKey, Time, TLLRN_RigHierarchy, ControlKey, SpaceKey);

	})
	.OnSpaceListChanged_Lambda([SelectedControls, TLLRN_ControlRig](UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey, const TArray<FTLLRN_RigElementKey>& InSpaceList)
	{
		check(SelectedControls.Contains(InControlKey));
				
		// update the settings in the control element
		if (FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(InControlKey))
		{
			FScopedTransaction Transaction(LOCTEXT("ControlChangeAvailableSpaces", "Edit Available Spaces"));

			InHierarchy->Modify();

			FTLLRN_RigControlElementCustomization ControlCustomization = *TLLRN_ControlRig->GetControlCustomization(InControlKey);
			ControlCustomization.AvailableSpaces = InSpaceList;
			ControlCustomization.RemovedSpaces.Reset();

			// remember  the elements which are in the asset's available list but removed by the user
			for (const FTLLRN_RigElementKey& AvailableSpace : ControlElement->Settings.Customization.AvailableSpaces)
			{
				if (!ControlCustomization.AvailableSpaces.Contains(AvailableSpace))
				{
					ControlCustomization.RemovedSpaces.Add(AvailableSpace);
				}
			}

			TLLRN_ControlRig->SetControlCustomization(InControlKey, ControlCustomization);
			InHierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
		}

	});
	// todo: implement GetAdditionalSpacesDelegate to pull spaces from sequencer

	FReply Reply = PickerWidget->OpenDialog(true);
	if (Reply.IsEventHandled())
	{
		return Handle;
	}
	return FKeyHandle::Invalid();
}

FKeyHandle AddOrUpdateKey(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey, FFrameNumber Time, ISequencer& Sequencer, const FGuid& InObjectBindingID, FTrackInstancePropertyBindings* PropertyBindings)
{
	UE_LOG(LogTLLRN_ControlRigEditor, Log, TEXT("We don't support adding keys to the Space Channel via the + key. Please use the Space Channel Area in the Animation Panel"));

	FKeyHandle Handle = FKeyHandle::Invalid();

	/**
	if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionToKey))
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Section->GetTLLRN_ControlRig())
		{
			FName ControlName = Section->FindControlNameFromSpaceChannel(Channel);
			if (ControlName != NAME_None)
			{
				if (FTLLRN_RigControlElement* Control = TLLRN_ControlRig->FindControl(ControlName))
				{
					TArray<FTLLRN_RigElementKey> Controls;
					FTLLRN_RigElementKey ControlKey = Control->GetKey();
					Controls.Add(ControlKey);
					FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey ExistingValue, Value;
					using namespace UE::MovieScene;
					EvaluateChannel(Channel, Time, ExistingValue);
					Value = ExistingValue;
					UTLLRN_RigHierarchy* TLLRN_RigHierarchy = TLLRN_ControlRig->GetHierarchy();
					Handle = SequencerOpenSpaceSwitchDialog(TLLRN_ControlRig, Controls, &Sequencer, Channel, SectionToKey, Time);
				}
			}
		}
	}
	*/
	return Handle;
}

bool CanCreateKeyEditor(const FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel)
{
	return false; //mz todoo maybe change
}
TSharedRef<SWidget> CreateKeyEditor(const TMovieSceneChannelHandle<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>& Channel, const UE::Sequencer::FCreateKeyEditorParams& Params)
{
	return SNullWidget::NullWidget;
}



/*******************************************************************
*
* FTLLRN_ControlTLLRN_RigSpaceChannelHelpers
*
**********************************************************************/


FSpaceChannelAndSection FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, ISequencer* Sequencer, bool bCreateIfNeeded)
{
	FSpaceChannelAndSection SpaceChannelAndSection;
	SpaceChannelAndSection.SpaceChannel = nullptr;
	SpaceChannelAndSection.SectionToKey = nullptr;
	if (TLLRN_ControlRig == nullptr || Sequencer == nullptr)
	{
		return SpaceChannelAndSection;
	}
	UMovieScene* MovieScene = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
	if (!MovieScene)
	{
		return SpaceChannelAndSection;
	}

	bool bFoundTrack = false;
	
	const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();
	bool bRecreateCurves = false;
	TArray<TPair<UTLLRN_ControlRig*, FName>> TLLRN_ControlRigPairsToReselect;
	for (const FMovieSceneBinding& Binding : Bindings)
	{
		TArray<UMovieSceneTrack*> Tracks = MovieScene->FindTracks(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.GetObjectGuid(), NAME_None);
		for (UMovieSceneTrack* Track : Tracks)
		{
			UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigParameterTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track);
			if (TLLRN_ControlRigParameterTrack && TLLRN_ControlRigParameterTrack->GetTLLRN_ControlRig() == TLLRN_ControlRig)
			{
				UMovieSceneTLLRN_ControlRigParameterSection* ActiveSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(TLLRN_ControlRigParameterTrack->GetSectionToKey(ControlName));
				if (ActiveSection)
				{
					ActiveSection->Modify();
					TLLRN_ControlRig->Modify();
					SpaceChannelAndSection.SectionToKey = ActiveSection;
					FTLLRN_SpaceControlNameAndChannel* NameAndChannel = ActiveSection->GetSpaceChannel(ControlName);
					if (NameAndChannel)
					{
						SpaceChannelAndSection.SpaceChannel = &NameAndChannel->SpaceCurve;
					}
					else if (bCreateIfNeeded)
					{
						if (TLLRN_ControlRig->IsControlSelected(ControlName))
						{
							TPair<UTLLRN_ControlRig*, FName> Pair;
							Pair.Key = TLLRN_ControlRig;
							Pair.Value = ControlName;
							TLLRN_ControlRigPairsToReselect.Add(Pair);
						}
						ActiveSection->AddSpaceChannel(ControlName, true /*ReconstructChannelProxy*/);
						NameAndChannel = ActiveSection->GetSpaceChannel(ControlName);
						if (NameAndChannel)
						{
							SpaceChannelAndSection.SpaceChannel = &NameAndChannel->SpaceCurve;
							bRecreateCurves = true;
						}
					}
				}
				
				bFoundTrack = true;
				break;
			}
		}

		if (bFoundTrack)
		{
			break;
		}
	}
	if (bRecreateCurves)
	{
		Sequencer->RecreateCurveEditor(); //this will require the curve editor to get recreated so the ordering is correct
		for (TPair<UTLLRN_ControlRig*, FName>& Pair : TLLRN_ControlRigPairsToReselect)
		{

			GEditor->GetTimerManager()->SetTimerForNextTick([Pair]()
				{
					Pair.Key->SelectControl(Pair.Value);
				});

		}
	}
	return SpaceChannelAndSection;
}

static TTuple<FTLLRN_RigControlElement*, FTLLRN_ChannelMapInfo*, int32, int32> GetControlAndChannelInfos(UTLLRN_ControlRig* TLLRN_ControlRig, UMovieSceneTLLRN_ControlRigParameterSection* TLLRN_ControlRigSection, FName ControlName)
{
	FTLLRN_RigControlElement* ControlElement = nullptr;
	FTLLRN_ChannelMapInfo* pChannelIndex = nullptr;
	int32 NumChannels = 0;
	int32 ChannelIndex = 0;
	Tie(ControlElement, pChannelIndex) = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetControlAndChannelInfo(TLLRN_ControlRig, TLLRN_ControlRigSection, ControlName);

	if (pChannelIndex && ControlElement)
	{
		// get the number of float channels to treat
		NumChannels = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetNumFloatChannels(ControlElement->Settings.ControlType);
		if (NumChannels > 0)
		{
			ChannelIndex = pChannelIndex->ChannelIndex;
		}
	}

	return TTuple<FTLLRN_RigControlElement*, FTLLRN_ChannelMapInfo*, int32, int32>(ControlElement, pChannelIndex, NumChannels,ChannelIndex);
}

/*
*  Situations to handle\
*  0) New space sames as current space do nothing
*  1) First space switch, so no existing space keys, if at start frame, just create space key, if not at start frame create current(parent) at start frame then new
*  2) Creating space that was the same as the one just before the current time. In that case we need to delete the current space key and the previous(-1) transform keys
*  3) Creating different space than previous or current, at (-1) curren ttime set space key as current, and set transforms in that space, then at current time set new space and new transforms
*  4) For 1-3 we always compensate any transform keys to the new space
*/

FKeyHandle FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerKeyTLLRN_ControlTLLRN_RigSpaceChannel(UTLLRN_ControlRig* TLLRN_ControlRig, ISequencer* Sequencer, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey, FFrameNumber Time, UTLLRN_RigHierarchy* TLLRN_RigHierarchy, const FTLLRN_RigElementKey& ControlKey, const FTLLRN_RigElementKey& SpaceKey)
{
	FKeyHandle Handle = FKeyHandle::Invalid();
	if (bDoNotCompensate == true || TLLRN_ControlRig == nullptr || Sequencer == nullptr || Sequencer->GetFocusedMovieSceneSequence() == nullptr || Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene() == nullptr)
	{
		return Handle;
	}

	TGuardValue<bool> CompensateGuard(bDoNotCompensate, true);

	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey ExistingValue, PreviousValue;
	using namespace UE::MovieScene;
	EvaluateChannel(Channel, Time, ExistingValue);
	EvaluateChannel(Channel, Time -1, PreviousValue);

	TArray<FTLLRN_RigElementKey> BeforeKeys, AfterKeys;
	Channel->GetUniqueSpaceList(&BeforeKeys);

	FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value = ExistingValue;

	if (SpaceKey == TLLRN_RigHierarchy->GetWorldSpaceReferenceKey())
	{
		Value.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World;
		Value.TLLRN_ControlTLLRN_RigElement = UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey();
	}
	else if (SpaceKey == TLLRN_RigHierarchy->GetDefaultParentKey())
	{
		Value.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent;
		Value.TLLRN_ControlTLLRN_RigElement = TLLRN_RigHierarchy->GetFirstParent(ControlKey);
	}
	else
	{
		FTLLRN_RigElementKey DefaultParent = TLLRN_RigHierarchy->GetFirstParent(ControlKey);
		if (DefaultParent == SpaceKey)
		{
			Value.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent;
			Value.TLLRN_ControlTLLRN_RigElement = DefaultParent;
		}
		else  //support all types
		{
			Value.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig;
			Value.TLLRN_ControlTLLRN_RigElement = SpaceKey;
		}
	}

	//we only key if the value is different.
	if (Value != ExistingValue)
	{
		FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
		FMovieSceneSequenceTransform RootToLocalTransform = Sequencer->GetFocusedMovieSceneSequenceTransform();

		//make sure to evaluate first frame
		FFrameTime CurrentTime(Time);
		CurrentTime = RootToLocalTransform.Inverse().TryTransformTime(CurrentTime).Get(CurrentTime);

		FMovieSceneContext SceneContext = FMovieSceneContext(FMovieSceneEvaluationRange(CurrentTime, TickResolution), Sequencer->GetPlaybackStatus()).SetHasJumped(true);
		Sequencer->GetEvaluationTemplate().EvaluateSynchronousBlocking(SceneContext);
		TLLRN_ControlRig->Evaluate_AnyThread();

		TArray<FFrameNumber> Frames;
		Frames.Add(Time);
		
		TMovieSceneChannelData<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> ChannelInterface = Channel->GetData();
		bool bSetPreviousKey = true;
		//find all of the times in the space after this time we now need to compensate for
		TSortedMap<FFrameNumber, FFrameNumber> ExtraFrames;
		FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetFramesInThisSpaceAfterThisTime(TLLRN_ControlRig, ControlKey.Name, ExistingValue,
			Channel, SectionToKey, Time, ExtraFrames);
		if (ExtraFrames.Num() > 0)
		{
			for (const TPair<FFrameNumber, FFrameNumber>& Frame : ExtraFrames)
			{
				Frames.Add(Frame.Value);
			}
		}
		SectionToKey->Modify();
		const FFrameNumber StartFrame = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange().GetLowerBoundValue();
		if (StartFrame == Time)
		{
			bSetPreviousKey = false;
		}
		//if we have no keys need to set key for current space at start frame, unless setting key at start time, where then don't do previous compensation
		if (Channel->GetNumKeys() == 0)
		{
			if (StartFrame != Time)
			{
				FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Original = ExistingValue;
				ChannelInterface.AddKey(StartFrame, Forward<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey>(Original));
			}
		}

		TArray<FTransform> TLLRN_ControlRigParentWorldTransforms;
		TLLRN_ControlRigParentWorldTransforms.SetNum(Frames.Num());
		for (FTransform& WorldTransform : TLLRN_ControlRigParentWorldTransforms)
		{
			WorldTransform  = FTransform::Identity;
		}
		TArray<FTransform> ControlWorldTransforms;
		FTLLRN_ControlRigSnapper Snapper;
		Snapper.GetTLLRN_ControlTLLRN_RigControlTransforms(Sequencer, TLLRN_ControlRig, ControlKey.Name, Frames, TLLRN_ControlRigParentWorldTransforms, ControlWorldTransforms);

		// store tangents to keep the animation when switching 
		TArray<FMovieSceneTangentData> Tangents;
		UMovieSceneTLLRN_ControlRigParameterSection* TLLRN_ControlRigSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionToKey);
		if (TLLRN_ControlRigSection)
		{
			FTLLRN_ChannelMapInfo* pChannelIndex = nullptr;
			FTLLRN_RigControlElement* ControlElement = nullptr;
			int32 NumChannels = 0;
			int32 ChannelIndex = 0;
			Tie(ControlElement, pChannelIndex,NumChannels, ChannelIndex) = GetControlAndChannelInfos(TLLRN_ControlRig, TLLRN_ControlRigSection, ControlKey.Name);

			if (pChannelIndex && ControlElement && NumChannels > 0)
			{
				EvaluateTangentAtThisTime<FMovieSceneFloatChannel>(ChannelIndex, NumChannels,TLLRN_ControlRigSection, Time, Tangents);
			}
		}
		else
		{
			return Handle;
		}
		
		// if current Value not the same as the previous add new space key
		if (PreviousValue != Value)
		{
			SectionToKey->Modify();
			int32 ExistingIndex = ChannelInterface.FindKey(Time);
			if (ExistingIndex != INDEX_NONE)
			{
				Handle = ChannelInterface.GetHandle(ExistingIndex);
				using namespace UE::MovieScene;
				AssignValue(Channel, Handle, Forward<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey>(Value));
			}
			else
			{
				ExistingIndex = ChannelInterface.AddKey(Time, Forward<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey>(Value));
				Handle = ChannelInterface.GetHandle(ExistingIndex);
			}
		}
		else //same as previous
		{
			if (TLLRN_ControlRigSection)
			{
				TLLRN_ControlRigSection->Modify();
				TArray<FFrameNumber> OurKeyTimes;
				TArray<FKeyHandle> OurKeyHandles;
				TRange<FFrameNumber> CurrentFrameRange;
				CurrentFrameRange.SetLowerBound(TRangeBound<FFrameNumber>(Time));
				CurrentFrameRange.SetUpperBound(TRangeBound<FFrameNumber>(Time));
				ChannelInterface.GetKeys(CurrentFrameRange, &OurKeyTimes, &OurKeyHandles);
				if (OurKeyHandles.Num() > 0)
				{
					ChannelInterface.DeleteKeys(OurKeyHandles);
				}
				//now delete any extra TimeOfDelete -1
				// NOTE do we need to update the tangents here?
				FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::DeleteTransformKeysAtThisTime(TLLRN_ControlRig, TLLRN_ControlRigSection, ControlKey.Name, Time - 1);
				bSetPreviousKey = false; //also don't set previous
			}
		}

		FMovieSceneInverseSequenceTransform LocalToRootTransform = RootToLocalTransform.Inverse();

		//do any compensation or previous key adding
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;

		// set previous key if we're going to switch space
		if (bSetPreviousKey)
		{
			FFrameTime GlobalTime(Time - 1);
			GlobalTime = LocalToRootTransform.TryTransformTime(GlobalTime).Get(GlobalTime);

			SceneContext = FMovieSceneContext(FMovieSceneEvaluationRange(GlobalTime, TickResolution), Sequencer->GetPlaybackStatus()).SetHasJumped(true);
			Sequencer->GetEvaluationTemplate().EvaluateSynchronousBlocking(SceneContext);

			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Time - 1));
			TLLRN_ControlRig->SetControlGlobalTransform(ControlKey.Name, ControlWorldTransforms[0], true, Context, false /*undo*/, false /*bPrintPython*/, true/* bFixEulerFlips*/);
			if (TLLRN_ControlRig->IsAdditive())
			{
				// We need to evaluate in order to trigger a notification
				TLLRN_ControlRig->Evaluate_AnyThread();
			}

			//need to do this after eval
			FTLLRN_ChannelMapInfo* pChannelIndex = nullptr;
			FTLLRN_RigControlElement* ControlElement = nullptr;
			int32 NumChannels = 0;
			int32 ChannelIndex = 0;
			Tie(ControlElement, pChannelIndex, NumChannels, ChannelIndex) = GetControlAndChannelInfos(TLLRN_ControlRig, TLLRN_ControlRigSection, ControlKey.Name);

			if (pChannelIndex && ControlElement && NumChannels > 0)
			{
				// need to update the tangents here to keep the arriving animation
				SetTangentsAtThisTime<FMovieSceneFloatChannel>(ChannelIndex, NumChannels, TLLRN_ControlRigSection, GlobalTime.GetFrame(), Tangents);
			}
		}

		// effectively switch to new space
		TLLRN_ControlRig->SwitchToParent(ControlKey, SpaceKey, false, true);
		// add new keys in the new space context
		int32 FramesIndex = 0;
		for (const FFrameNumber& Frame : Frames)
		{
			FFrameTime GlobalTime(Frame);
			GlobalTime = LocalToRootTransform.TryTransformTime(GlobalTime).Get(GlobalTime);

			SceneContext = FMovieSceneContext(FMovieSceneEvaluationRange(GlobalTime, TickResolution), Sequencer->GetPlaybackStatus()).SetHasJumped(true);
			Sequencer->GetEvaluationTemplate().EvaluateSynchronousBlocking(SceneContext);

			TLLRN_ControlRig->Evaluate_AnyThread();
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			TLLRN_ControlRig->SetControlGlobalTransform(ControlKey.Name, ControlWorldTransforms[FramesIndex], true, Context, false /*undo*/, false /*bPrintPython*/, true/* bFixEulerFlips*/);
			if (TLLRN_ControlRig->IsAdditive())
			{
				// We need to evaluate in order to trigger a notification
				TLLRN_ControlRig->Evaluate_AnyThread();
			}
			
			//need to do this after eval
			FTLLRN_ChannelMapInfo* pChannelIndex = nullptr;
			FTLLRN_RigControlElement* ControlElement = nullptr;
			int32 NumChannels = 0;
			int32 ChannelIndex = 0;
			Tie(ControlElement, pChannelIndex, NumChannels, ChannelIndex) = GetControlAndChannelInfos(TLLRN_ControlRig, TLLRN_ControlRigSection, ControlKey.Name);

			// need to update the tangents here to keep the leaving animation
			if (NumChannels > 0 && bSetPreviousKey && FramesIndex == 0)
			{
				if (TLLRN_ControlRigSection && NumChannels > 0)
				{
					SetTangentsAtThisTime<FMovieSceneFloatChannel>(ChannelIndex, NumChannels, TLLRN_ControlRigSection, GlobalTime.GetFrame(), Tangents);
				}
			}

			FramesIndex++;
		}
		Channel->FindSpaceIntervals();
		Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);

		Channel->GetUniqueSpaceList(&AfterKeys);
		Channel->BroadcastSpaceNoLongerUsed(BeforeKeys, AfterKeys);
	}

	return Handle;

}

//get the mask of the transform keys at the specified time
static ETLLRN_ControlRigContextChannelToKey GetCurrentTransformKeysAtThisTime(UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, UMovieSceneSection* SectionToKey, FFrameNumber Time)
{
	ETLLRN_ControlRigContextChannelToKey ChannelToKey = ETLLRN_ControlRigContextChannelToKey::None;
	if (TLLRN_ControlRig && SectionToKey)
	{
		if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionToKey))
		{
			TArrayView<FMovieSceneFloatChannel*> FloatChannels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
			FTLLRN_ChannelMapInfo* pChannelIndex = Section->ControlChannelMap.Find(ControlName);
			if (pChannelIndex != nullptr)
			{
				int32 ChannelIndex = pChannelIndex->ChannelIndex;

				if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlName))
				{
					FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
					switch (ControlElement->Settings.ControlType)
					{
					case ETLLRN_RigControlType::Position:
					case ETLLRN_RigControlType::Scale:
					case ETLLRN_RigControlType::Rotator:
					case ETLLRN_RigControlType::Transform:
					case ETLLRN_RigControlType::TransformNoScale:
					case ETLLRN_RigControlType::EulerTransform:
					{
						if (FloatChannels[ChannelIndex]->GetData().FindKey(Time) != INDEX_NONE)
						{
							if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::RotationX;
							}
							else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Scale)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::ScaleX;
							}
							else
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::TranslationX;
							}
						}
						if (FloatChannels[ChannelIndex + 1]->GetData().FindKey(Time) != INDEX_NONE)
						{
							if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::RotationY;
							}
							else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Scale)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::ScaleY;
							}
							else
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::TranslationY;
							}
						}
						if (FloatChannels[ChannelIndex + 2]->GetData().FindKey(Time) != INDEX_NONE)
						{
							if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::RotationZ;
							}
							else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Scale)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::ScaleZ;
							}
							else
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::TranslationZ;
							}
						}
						if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform ||
							ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform ||
							ControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale
							)
						{
							if (FloatChannels[ChannelIndex + 3]->GetData().FindKey(Time) != INDEX_NONE)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::RotationX;
							}
							if (FloatChannels[ChannelIndex + 4]->GetData().FindKey(Time) != INDEX_NONE)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::RotationY;
							}
							if (FloatChannels[ChannelIndex + 5]->GetData().FindKey(Time) != INDEX_NONE)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::RotationZ;
							}

						}
						if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform ||
							ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
						{
							if (FloatChannels[ChannelIndex + 6]->GetData().FindKey(Time) != INDEX_NONE)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::ScaleX;
							}
							if (FloatChannels[ChannelIndex + 7]->GetData().FindKey(Time) != INDEX_NONE)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::ScaleY;
							}
							if (FloatChannels[ChannelIndex + 8]->GetData().FindKey(Time) != INDEX_NONE)
							{
								ChannelToKey |= ETLLRN_ControlRigContextChannelToKey::ScaleZ;
							}
						}
						break;

					};

					}
				}
			}
		
		}

	}
	return ChannelToKey;
}

void  FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerSpaceChannelKeyDeleted(UTLLRN_ControlRig* TLLRN_ControlRig, ISequencer* Sequencer, FName ControlName, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneTLLRN_ControlRigParameterSection* SectionToKey,
	FFrameNumber TimeOfDeletion)
{
	if (bDoNotCompensate == true)
	{
		return;
	}
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey ExistingValue, PreviousValue;
	using namespace UE::MovieScene;
	EvaluateChannel(Channel, TimeOfDeletion - 1, PreviousValue);
	EvaluateChannel(Channel, TimeOfDeletion, ExistingValue);
	if (ExistingValue != PreviousValue) //if they are the same no need to do anything
	{
		TGuardValue<bool> CompensateGuard(bDoNotCompensate, true);

		//find all key frames we need to compensate
		TArray<FFrameNumber> Frames;
		Frames.Add(TimeOfDeletion);
		TSortedMap<FFrameNumber, FFrameNumber> ExtraFrames;
		FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetFramesInThisSpaceAfterThisTime(TLLRN_ControlRig, ControlName, ExistingValue,
			Channel, SectionToKey, TimeOfDeletion, ExtraFrames);
		if (ExtraFrames.Num() > 0)
		{
			for (const TPair<FFrameNumber, FFrameNumber>& Frame : ExtraFrames)
			{
				Frames.Add(Frame.Value);
			}
		}
		TArray<FTransform> TLLRN_ControlRigParentWorldTransforms;
		TLLRN_ControlRigParentWorldTransforms.SetNum(Frames.Num());
		for (FTransform& WorldTransform : TLLRN_ControlRigParentWorldTransforms)
		{
			WorldTransform = FTransform::Identity;
		}
		TArray<FTransform> ControlWorldTransforms;
		FTLLRN_ControlRigSnapper Snapper;
		Snapper.GetTLLRN_ControlTLLRN_RigControlTransforms(Sequencer, TLLRN_ControlRig, ControlName, Frames, TLLRN_ControlRigParentWorldTransforms, ControlWorldTransforms);
		FTLLRN_RigElementKey ControlKey;
		ControlKey.Name = ControlName;
		ControlKey.Type = ETLLRN_RigElementType::Control;
		UTLLRN_RigHierarchy* TLLRN_RigHierarchy = TLLRN_ControlRig->GetHierarchy();

		int32 FramesIndex = 0;
		FTLLRN_RigControlModifiedContext Context;
		Context.SetKey = ETLLRN_ControlRigSetKey::Always;
		FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
		FMovieSceneSequenceTransform RootToLocalTransform = Sequencer->GetFocusedMovieSceneSequenceTransform();
		FMovieSceneInverseSequenceTransform LocalToRootTransform = RootToLocalTransform.Inverse();

		for (const FFrameNumber& Frame : Frames)
		{
			//evaluate sequencer
			FFrameTime GlobalTime(Frame);
			GlobalTime = LocalToRootTransform.TryTransformTime(GlobalTime).Get(GlobalTime);

			FMovieSceneContext SceneContext = FMovieSceneContext(FMovieSceneEvaluationRange(GlobalTime, TickResolution), Sequencer->GetPlaybackStatus()).SetHasJumped(true);
			Sequencer->GetEvaluationTemplate().EvaluateSynchronousBlocking(SceneContext);
			//make sure to set rig hierarchy correct since key is not deleted yet
			switch (PreviousValue.SpaceType)
			{
			case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent:
				TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetDefaultParent(ControlKey), false, true);
				break;
			case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World:
				TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetWorldSpaceReferenceKey(), false, true);
				break;
			case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig:
				TLLRN_ControlRig->SwitchToParent(ControlKey, PreviousValue.TLLRN_ControlTLLRN_RigElement, false, true);
				break;
			}
			TLLRN_ControlRig->Evaluate_AnyThread();
			Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
			//make sure we only add keys to those that exist since they may be getting deleted also.
			Context.KeyMask = (uint32)GetCurrentTransformKeysAtThisTime(TLLRN_ControlRig, ControlKey.Name, SectionToKey, Frame);
			TLLRN_ControlRig->SetControlGlobalTransform(ControlKey.Name, ControlWorldTransforms[FramesIndex++], true, Context, false /*undo*/, false /*bPrintPython*/, true/* bFixEulerFlips*/);
		}
		//now delete any extra TimeOfDelete -1
		FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::DeleteTransformKeysAtThisTime(TLLRN_ControlRig, SectionToKey, ControlName, TimeOfDeletion - 1);
		
	}
}

void FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::DeleteTransformKeysAtThisTime(UTLLRN_ControlRig* TLLRN_ControlRig, UMovieSceneTLLRN_ControlRigParameterSection* Section, FName ControlName, FFrameNumber Time)
{
	if (Section && TLLRN_ControlRig)
	{
		TArrayView<FMovieSceneFloatChannel*> FloatChannels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
		FTLLRN_ChannelMapInfo* pChannelIndex = Section->ControlChannelMap.Find(ControlName);
		if (pChannelIndex != nullptr)
		{
			int32 ChannelIndex = pChannelIndex->ChannelIndex;

			if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlName))
			{
				FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
				switch (ControlElement->Settings.ControlType)
				{
					case ETLLRN_RigControlType::Position:
					case ETLLRN_RigControlType::Scale:
					case ETLLRN_RigControlType::Rotator:
					case ETLLRN_RigControlType::Transform:
					case ETLLRN_RigControlType::TransformNoScale:
					case ETLLRN_RigControlType::EulerTransform:
					{
						int NumChannels = 0;
						if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform ||
							ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
						{
							NumChannels = 9;
						}
						else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale)
						{
							NumChannels = 6;
						}
						else //vectors
						{
							NumChannels = 3;
						}
						for (int32 Index = 0; Index < NumChannels; ++Index)
						{
							int32 KeyIndex = 0;
							for (FFrameNumber Frame : FloatChannels[ChannelIndex]->GetData().GetTimes())
							{
								if (Frame == Time)
								{
									FloatChannels[ChannelIndex]->GetData().RemoveKey(KeyIndex);
									break;
								}
								else if (Frame > Time)
								{
									break;
								}
								++KeyIndex;
							}
							++ChannelIndex;
						}
						break;
					}
					default:
						break;

				}
			}
		}
	}
}

void FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetFramesInThisSpaceAfterThisTime(
	UTLLRN_ControlRig* TLLRN_ControlRig,
	FName ControlName,
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey CurrentValue,
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel,
	UMovieSceneSection* SectionToKey,
	FFrameNumber Time,
	TSortedMap<FFrameNumber,FFrameNumber>& OutMoreFrames)
{
	OutMoreFrames.Reset();
	if (TLLRN_ControlRig && Channel && SectionToKey)
	{
		if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionToKey))
		{
			TArrayView<FMovieSceneFloatChannel*> FloatChannels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
			FTLLRN_ChannelMapInfo* pChannelIndex = Section->ControlChannelMap.Find(ControlName);
			if (pChannelIndex != nullptr)
			{
				int32 ChannelIndex = pChannelIndex->ChannelIndex;

				if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlName))
				{
					FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
					switch (ControlElement->Settings.ControlType)
					{
						case ETLLRN_RigControlType::Position:
						case ETLLRN_RigControlType::Scale:
						case ETLLRN_RigControlType::Rotator:
						case ETLLRN_RigControlType::Transform:
						case ETLLRN_RigControlType::TransformNoScale:
						case ETLLRN_RigControlType::EulerTransform:
						{
							int NumChannels = 0;
							if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform ||
								ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
							{
								NumChannels = 9;
							}
							else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale)
							{
								NumChannels = 6;
							}
							else //vectors
							{
								NumChannels = 3;
							}
							for (int32 Index = 0; Index < NumChannels; ++Index)
							{
								for (FFrameNumber Frame : FloatChannels[ChannelIndex++]->GetData().GetTimes())
								{
									if (Frame > Time)
									{
										using namespace UE::MovieScene;
										EvaluateChannel(Channel, Frame, Value);
										if (CurrentValue == Value)
										{
											if (OutMoreFrames.Find(Frame) == nullptr)
											{
												OutMoreFrames.Add(Frame, Frame);
											}
										}
										else
										{
											break;
										}
									}
								}
							}
							break;
						}

					}
				}
			}
		}
	}
}

void FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerBakeControlInSpace(UTLLRN_ControlRig* TLLRN_ControlRig, ISequencer* Sequencer, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey,
	UTLLRN_RigHierarchy* TLLRN_RigHierarchy, const FTLLRN_RigElementKey& ControlKey, FTLLRN_TLLRN_RigSpacePickerBakeSettings Settings)
{
	if (bDoNotCompensate == true)
	{
		return;
	}
	if (TLLRN_ControlRig && Sequencer && Channel && SectionToKey && (Settings.Settings.StartFrame != Settings.Settings.EndFrame) && TLLRN_RigHierarchy
		&& Sequencer->GetFocusedMovieSceneSequence() && Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene())
	{
	
		if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionToKey))
		{
			//get the setting as a rig element key to use for comparisons for smart baking, 
			FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
			if (Settings.TargetSpace == TLLRN_RigHierarchy->GetWorldSpaceReferenceKey())
			{
				Value.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World;
			}
			else if (Settings.TargetSpace == TLLRN_RigHierarchy->GetDefaultParentKey())
			{
				Value.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent;
			}
			else
			{
				FTLLRN_RigElementKey DefaultParent = TLLRN_RigHierarchy->GetFirstParent(ControlKey);
				if (DefaultParent == Settings.TargetSpace)
				{
					Value.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent;
				}
				else
				{
					Value.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig;
					Value.TLLRN_ControlTLLRN_RigElement = Settings.TargetSpace;
				}
			}
			TArrayView<FMovieSceneFloatChannel*> Channels = FTLLRN_ControlRigSequencerHelpers::GetFloatChannels(TLLRN_ControlRig,
				ControlKey.Name, Section);
			//if smart baking we 1) Don't do any baking over times where the Settings.TargetSpace is active, 
			//and 2) we just bake over key frames
			//need to store these times first since we will still delete all of these space keys

			TArray<FFrameNumber> Frames;
			const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
			const FFrameRate  FrameRate = Sequencer->GetFocusedDisplayRate();
			//get space keys so we can delete them and use them for smart baking at them
			TArray<FFrameNumber> Keys;
			TArray < FKeyHandle> KeyHandles;
			FFrameNumber StartFrame = Settings.Settings.StartFrame;
			FFrameNumber EndFrame = Settings.Settings.EndFrame;
			TRange<FFrameNumber> Range(StartFrame, EndFrame);
			Channel->GetKeys(Range, &Keys, &KeyHandles);
			TArray<FFrameNumber> MinusOneFrames; //frames we need to delete
			TArray<TPair<FFrameNumber, TArray<FMovieSceneTangentData>>> StoredTangents; //need to set tangents at the space switch locations
			TSet<FFrameNumber> ParentFrames; // these are parent frames we keep track of so we can 
			if (Settings.Settings.BakingKeySettings == EBakingKeySettings::KeysOnly)
			{
				TSet<FFrameNumber> MinusOneKeys; 
				TSortedMap<FFrameNumber,FFrameNumber> FrameMap;
				//add space keys to bake
				{
					for (FFrameNumber& Frame : Keys)
					{
						FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey SpaceValue;
						using namespace UE::MovieScene;
						EvaluateChannel(Channel, Frame, SpaceValue);
						if (SpaceValue != Value)
						{
							FTLLRN_ChannelMapInfo* pChannelIndex = nullptr;
							FTLLRN_RigControlElement* ControlElement = nullptr;
							int32 NumChannels = 0;
							int32 ChannelIndex = 0;
							Tie(ControlElement, pChannelIndex, NumChannels, ChannelIndex) = GetControlAndChannelInfos(TLLRN_ControlRig, Section, ControlKey.Name);

							if (pChannelIndex && ControlElement && NumChannels > 0)
							{
								TPair<FFrameNumber, TArray<FMovieSceneTangentData>> FrameAndTangent;
								FrameAndTangent.Key = Frame;
								EvaluateTangentAtThisTime<FMovieSceneFloatChannel>(ChannelIndex, NumChannels, Section, Frame, FrameAndTangent.Value);
								StoredTangents.Add(FrameAndTangent);
							}
							FrameMap.Add(Frame,Frame);
						}
						MinusOneKeys.Add(Frame - 1);
					}
				}

				TArray<FFrameNumber> TransformFrameTimes = FMovieSceneConstraintChannelHelper::GetTransformTimes(
					Channels, Settings.Settings.StartFrame, Settings.Settings.EndFrame);
				//add transforms keys to bake
				{
					for (FFrameNumber& Frame : TransformFrameTimes)
					{
						if (MinusOneKeys.Contains(Frame) == false)
						{
							FrameMap.Add(Frame);
						}
					}
				}
				//also need to add transform keys for the parent control rig intervals over the time we bake
				TArray<FSpaceRange> SpaceRanges = Channel->FindSpaceIntervals();
				for (const FSpaceRange& SpaceRange : SpaceRanges)
				{
					if (SpaceRange.Key.SpaceType == EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig && SpaceRange.Key.TLLRN_ControlTLLRN_RigElement.IsValid())
					{
						TRange<FFrameNumber> Overlap = TRange<FFrameNumber>::Intersection(Range, SpaceRange.Range);
						if (Overlap.IsEmpty() == false)
						{
							TArrayView<FMovieSceneFloatChannel*> ParentChannels = FTLLRN_ControlRigSequencerHelpers::GetFloatChannels(TLLRN_ControlRig,
								SpaceRange.Key.TLLRN_ControlTLLRN_RigElement.Name, Section);

							TArray<FFrameNumber> ParentFrameTimes = FMovieSceneConstraintChannelHelper::GetTransformTimes(
								ParentChannels, StartFrame, EndFrame);
							for (FFrameNumber& Frame : ParentFrameTimes)
							{
								//if we don't have a key at the parent time, add the frame AND store it's tangent.
								if (FrameMap.Contains(Frame) == false)
								{
									FrameMap.Add(Frame);
									ParentFrames.Add(Frame);

									FTLLRN_ChannelMapInfo* pChannelIndex = nullptr;
									FTLLRN_RigControlElement* ControlElement = nullptr;
									int32 NumChannels = 0;
									int32 ChannelIndex = 0;
									Tie(ControlElement, pChannelIndex, NumChannels, ChannelIndex) = GetControlAndChannelInfos(TLLRN_ControlRig, Section, SpaceRange.Key.TLLRN_ControlTLLRN_RigElement.Name);
									TPair<FFrameNumber, TArray<FMovieSceneTangentData>> FrameAndTangent;
									FrameAndTangent.Key = Frame;
									EvaluateTangentAtThisTime<FMovieSceneFloatChannel>(ChannelIndex, NumChannels, Section, Frame, FrameAndTangent.Value);
									StoredTangents.Add(FrameAndTangent);
								}
							}
						}
					}
				
				}
				FrameMap.GenerateKeyArray(Frames);

			}
			else
			{
				FFrameNumber FrameRateInFrameNumber = TickResolution.AsFrameNumber(FrameRate.AsInterval());
				if (Settings.Settings.FrameIncrement > 1) //increment per frame by increment
				{
					FrameRateInFrameNumber.Value *= Settings.Settings.FrameIncrement;
				}
				for (FFrameNumber& Frame = Settings.Settings.StartFrame; Frame <= Settings.Settings.EndFrame; Frame += FrameRateInFrameNumber)
				{
					Frames.Add(Frame);
				}

			}
			if (Frames.Num() == 0)
			{
				return;
			}
			TGuardValue<bool> CompensateGuard(bDoNotCompensate, true);
			TArray<FTransform> TLLRN_ControlRigParentWorldTransforms;
			TLLRN_ControlRigParentWorldTransforms.SetNum(Frames.Num());
			for (FTransform& Transform : TLLRN_ControlRigParentWorldTransforms)
			{
				Transform = FTransform::Identity;
			}
			//Store transforms
			FTLLRN_ControlRigSnapper Snapper;
			TArray<FTransform> ControlWorldTransforms;
			Snapper.GetTLLRN_ControlTLLRN_RigControlTransforms(Sequencer, TLLRN_ControlRig, ControlKey.Name, Frames, TLLRN_ControlRigParentWorldTransforms, ControlWorldTransforms);

			//Need to delete keys, if smart baking we only delete the -1 keys,
			//if not, we need to delete all transform
			//and always delete space keys
			Section->Modify();
			Channel->DeleteKeys(KeyHandles);
			if (Settings.Settings.BakingKeySettings == EBakingKeySettings::KeysOnly)
			{
				for (const FFrameNumber& DeleteFrame : Keys)
				{
					FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::DeleteTransformKeysAtThisTime(TLLRN_ControlRig, Section, ControlKey.Name, DeleteFrame - 1);
				}
			}
			else
			{
				FMovieSceneConstraintChannelHelper::DeleteTransformTimes(Channels, StartFrame, EndFrame);
			}
			

			UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();

			//now find space at start and end see if different than the new space if so we need to compensate
			FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey StartFrameValue, EndFrameValue;
			using namespace UE::MovieScene;
			EvaluateChannel(Channel, StartFrame, StartFrameValue);
			EvaluateChannel(Channel, EndFrame, EndFrameValue);
			
			const bool bCompensateStart = StartFrameValue != Value;
			TRange<FFrameNumber> PlaybackRange = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange();
			const bool bCompensateEnd = (EndFrameValue != Value && PlaybackRange.GetUpperBoundValue() != EndFrame);

			//if compensate at the start we need to set the channel key as the new value
			if (bCompensateStart)
			{
				TMovieSceneChannelData<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> ChannelInterface = Channel->GetData();
				ChannelInterface.AddKey(StartFrame, Forward<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey>(Value));

			}
			//if we compensate at the end we change the last frame to frame -1(tick), and then later set the space to the other one and 
			if (bCompensateEnd)
			{
				Frames[Frames.Num() - 1] = Frames[Frames.Num() - 1] - 1;
			}
			//now set all of the key values
			FTLLRN_RigControlModifiedContext Context;
			Context.SetKey = ETLLRN_ControlRigSetKey::Always;
			Context.KeyMask = (uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform;
			TLLRN_ControlRig->SwitchToParent(ControlKey, Settings.TargetSpace, false, true);
			TLLRN_ControlRig->Evaluate_AnyThread();

			FMovieSceneSequenceTransform RootToLocalTransform = Sequencer->GetFocusedMovieSceneSequenceTransform();
			FMovieSceneInverseSequenceTransform LocalToRootTransform = RootToLocalTransform.Inverse();

			for (int32 Index = 0; Index < Frames.Num(); ++Index)
			{
				const FTransform GlobalTransform = ControlWorldTransforms[Index];
				const FFrameNumber Frame = Frames[Index];

				//evaluate sequencer
				FFrameTime GlobalTime(Frame);
				GlobalTime = LocalToRootTransform.TryTransformTime(GlobalTime).Get(GlobalTime);

				FMovieSceneContext SceneContext = FMovieSceneContext(FMovieSceneEvaluationRange(GlobalTime, TickResolution), Sequencer->GetPlaybackStatus()).SetHasJumped(true);
				Sequencer->GetEvaluationTemplate().EvaluateSynchronousBlocking(SceneContext);

				//evaluate control rig
				TLLRN_ControlRig->Evaluate_AnyThread();
				Context.LocalTime = TickResolution.AsSeconds(FFrameTime(Frame));
				
				//if doing smart baking only set keys on those that exist IF not a key from the parent, in that case we need to key the  whole transform.
				if (Settings.Settings.BakingKeySettings == EBakingKeySettings::KeysOnly && ParentFrames.Contains(Frame) == false)
				{
					Context.KeyMask = (uint32)GetCurrentTransformKeysAtThisTime(TLLRN_ControlRig, ControlKey.Name, SectionToKey, Frame);
				}
				else
				{
					Context.KeyMask = (uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform;
				}
				TLLRN_ControlRig->SetControlGlobalTransform(ControlKey.Name, GlobalTransform, true, Context, false /*undo*/, false /*bPrintPython*/, true/* bFixEulerFlips*/);
			}

			//if end compensated set the space that was active previously and set the compensated global value
			if (bCompensateEnd)
			{
				//EndFrameValue to SpaceKey todoo move to function
				switch (EndFrameValue.SpaceType)
				{
				case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent:
					TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetDefaultParent(ControlKey), false, true);
					break;
				case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World:
					TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetWorldSpaceReferenceKey(), false, true);
					break;
				case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig:
					TLLRN_ControlRig->SwitchToParent(ControlKey, EndFrameValue.TLLRN_ControlTLLRN_RigElement, false, true);
					break;
				}

				//evaluate sequencer
				FFrameTime GlobalTime(EndFrame);
				GlobalTime = LocalToRootTransform.TryTransformTime(GlobalTime).Get(GlobalTime);

				FMovieSceneContext SceneContext = FMovieSceneContext(FMovieSceneEvaluationRange(GlobalTime, TickResolution), Sequencer->GetPlaybackStatus()).SetHasJumped(true);
				Sequencer->GetEvaluationTemplate().EvaluateSynchronousBlocking(SceneContext);
		
				//evaluate control rig
				TLLRN_ControlRig->Evaluate_AnyThread();

				TMovieSceneChannelData<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> ChannelInterface = Channel->GetData();
				ChannelInterface.AddKey(EndFrame, Forward<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey>(EndFrameValue));
				const FTransform GlobalTransform = ControlWorldTransforms[Frames.Num() - 1];
				Context.LocalTime = TickResolution.AsSeconds(FFrameTime(EndFrame));
				Context.KeyMask = (uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform;
				TLLRN_ControlRig->SetControlGlobalTransform(ControlKey.Name, GlobalTransform, true, Context, false /*undo*/, false /*bPrintPython*/, true/* bFixEulerFlips*/);
			}
			//Fix tangents at removed space switches if needed
			for (TPair<FFrameNumber, TArray<FMovieSceneTangentData>>& StoredTangent : StoredTangents)
			{
				FTLLRN_ChannelMapInfo* pChannelIndex = nullptr;
				FTLLRN_RigControlElement* ControlElement = nullptr;
				int32 NumChannels = 0;
				int32 ChannelIndex = 0;
				Tie(ControlElement, pChannelIndex, NumChannels, ChannelIndex) = GetControlAndChannelInfos(TLLRN_ControlRig, Section, ControlKey.Name);

				if (pChannelIndex && ControlElement && NumChannels > 0)
				{
					SetTangentsAtThisTime<FMovieSceneFloatChannel>(ChannelIndex, NumChannels, Section, StoredTangent.Key, StoredTangent.Value);
				}
			}
			// Fix any Rotation Channels
			Section->FixRotationWinding(ControlKey.Name, Frames[0], Frames[Frames.Num() - 1]);
			// Then optimize
			if (Settings.Settings.BakingKeySettings == EBakingKeySettings::AllFrames && Settings.Settings.bReduceKeys)
			{
				FKeyDataOptimizationParams Params;
				Params.bAutoSetInterpolation = true;
				Params.Tolerance = Settings.Settings.Tolerance;
				Params.Range = TRange <FFrameNumber>(Frames[0], Frames[Frames.Num() - 1]);
				Section->OptimizeSection(ControlKey.Name, Params);
			}
			else  //need to auto set tangents, the above section will do that also
			{
				Section->AutoSetTangents(ControlKey.Name);
			}	
		}
		Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded); //may have added channel
	}
}

void FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::HandleSpaceKeyTimeChanged(UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey,
	FFrameNumber CurrentFrame, FFrameNumber NextFrame)
{
	if (TLLRN_ControlRig && Channel && SectionToKey && (CurrentFrame != NextFrame))
	{
		if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionToKey))
		{
			TArrayView<FMovieSceneFloatChannel*> FloatChannels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
			FTLLRN_ChannelMapInfo* pChannelIndex = Section->ControlChannelMap.Find(ControlName);
			if (pChannelIndex != nullptr)
			{
				int32 ChannelIndex = pChannelIndex->ChannelIndex;
				FFrameNumber Delta = NextFrame - CurrentFrame;
				if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlName))
				{
					FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
					switch (ControlElement->Settings.ControlType)
					{
						case ETLLRN_RigControlType::Position:
						case ETLLRN_RigControlType::Scale:
						case ETLLRN_RigControlType::Rotator:
						case ETLLRN_RigControlType::Transform:
						case ETLLRN_RigControlType::TransformNoScale:
						case ETLLRN_RigControlType::EulerTransform:
						{
							int NumChannels = 0;
							if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform ||
								ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
							{
								NumChannels = 9;
							}
							else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale)
							{
								NumChannels = 6;
							}
							else //vectors
							{
								NumChannels = 3;
							}
							for (int32 Index = 0; Index < NumChannels; ++Index)
							{
								FMovieSceneFloatChannel* FloatChannel = FloatChannels[ChannelIndex++];
								if (Delta > 0) //if we are moving keys positively in time we start from end frames and move them so we can use indices
								{
									for (int32 KeyIndex = FloatChannel->GetData().GetTimes().Num() - 1; KeyIndex >= 0; --KeyIndex)
									{
										const FFrameNumber Frame = FloatChannel->GetData().GetTimes()[KeyIndex];
										FFrameNumber Diff = Frame - CurrentFrame;
										FFrameNumber AbsDiff = Diff < 0 ? -Diff : Diff;
										if (AbsDiff <= 1)
										{
											FFrameNumber NewKeyTime = Frame + Delta;
											FloatChannel->GetData().MoveKey(KeyIndex, NewKeyTime);
										}
									}
								}
								else
								{
									for (int32 KeyIndex = 0; KeyIndex < FloatChannel->GetData().GetTimes().Num(); ++KeyIndex)
									{
										const FFrameNumber Frame = FloatChannel->GetData().GetTimes()[KeyIndex];
										FFrameNumber Diff = Frame - CurrentFrame;
										FFrameNumber AbsDiff = Diff < 0 ? -Diff : Diff;
										if (AbsDiff <= 1)
										{
											FFrameNumber NewKeyTime = Frame + Delta;
											FloatChannel->GetData().MoveKey(KeyIndex, NewKeyTime);
										}
									}
								}
							}
							break;
						}
						default:
							break;
					}
				}
			}
		}
	}
}

UMovieSceneTLLRN_ControlRigParameterSection* FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetTLLRN_ControlRigSection(ISequencer* Sequencer, const UTLLRN_ControlRig* TLLRN_ControlRig)
{

	if (TLLRN_ControlRig == nullptr || Sequencer == nullptr)
	{
		return nullptr;
	}
	UMovieScene* MovieScene = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
	if (!MovieScene)
	{
		return nullptr;
	}
	const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();
	for (const FMovieSceneBinding& Binding : Bindings)
	{
		UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigParameterTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(MovieScene->FindTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.GetObjectGuid(), NAME_None));
		if (TLLRN_ControlRigParameterTrack && TLLRN_ControlRigParameterTrack->GetTLLRN_ControlRig() == TLLRN_ControlRig)
		{
			UMovieSceneTLLRN_ControlRigParameterSection* ActiveSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(TLLRN_ControlRigParameterTrack->GetSectionToKey());
			if (ActiveSection)
			{
				return ActiveSection;
			}
		}
	}
	return nullptr;
}

void FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::CompensateIfNeeded(
	UTLLRN_ControlRig* TLLRN_ControlRig,
	ISequencer* Sequencer,
	UMovieSceneTLLRN_ControlRigParameterSection* Section,
	TOptional<FFrameNumber>& OptionalTime, 
	bool bCompPreviousTick)
{
	if (bDoNotCompensate == true)
	{
		return;
	}

	// Frames to compensate
	TArray<FFrameNumber> OptionalTimeArray;
	if(OptionalTime.IsSet())
	{
		OptionalTimeArray.Add(OptionalTime.GetValue());
	}
	
	auto GetSpaceTimesToCompensate = [&OptionalTimeArray](const FTLLRN_SpaceControlNameAndChannel* Channel)->TArrayView<const FFrameNumber>
	{
		if (OptionalTimeArray.IsEmpty())
		{
			return Channel->SpaceCurve.GetData().GetTimes();
		}
		return OptionalTimeArray;
	};

	// keyframe context
	FTLLRN_RigControlModifiedContext KeyframeContext;
	KeyframeContext.SetKey = ETLLRN_ControlRigSetKey::Always;
	const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
	
	//we need to check all controls for 1) space and 2) previous frame and if so we automatically compensate.
	bool bDidIt = false;

	// compensate spaces
	UTLLRN_RigHierarchy* TLLRN_RigHierarchy = TLLRN_ControlRig->GetHierarchy();
	const TArray<FTLLRN_RigControlElement*> Controls = TLLRN_RigHierarchy->GetControls();
	for (const FTLLRN_RigControlElement* Control: Controls)
	{ 
		if(Control)
		{ 
			//only if we have a channel
			if (FTLLRN_SpaceControlNameAndChannel* Channel = Section->GetSpaceChannel(Control->GetFName()))
			{
				const TArrayView<const FFrameNumber> FramesToCompensate = GetSpaceTimesToCompensate(Channel);
				if (FramesToCompensate.Num() > 0)
				{
					for (const FFrameNumber& Time : FramesToCompensate)
					{
						const FFrameNumber TimeToCompensate = bCompPreviousTick ? (Time - 1) : Time;
						const FFrameNumber TimeToCompare = bCompPreviousTick ? Time : (Time - 1);
						FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey ExistingValue, PreviousValue;
						using namespace UE::MovieScene;
						EvaluateChannel(&(Channel->SpaceCurve), TimeToCompensate, PreviousValue);
						EvaluateChannel(&(Channel->SpaceCurve), TimeToCompare, ExistingValue);

						if (ExistingValue != PreviousValue) //if they are the same no need to do anything
						{
							TGuardValue<bool> CompensateGuard(bDoNotCompensate, true);
							
							//find global value at current time
							TArray<FTransform> TLLRN_ControlRigParentWorldTransforms({FTransform::Identity});
							TArray<FTransform> ControlWorldTransforms;
							FTLLRN_ControlRigSnapper Snapper;
							Snapper.GetTLLRN_ControlTLLRN_RigControlTransforms(
								Sequencer, TLLRN_ControlRig, Control->GetFName(),
								{ TimeToCompare },
								TLLRN_ControlRigParentWorldTransforms, ControlWorldTransforms);

							//set space to previous space value that's different.
							const FTLLRN_RigElementKey ControlKey = Control->GetKey();
							switch (PreviousValue.SpaceType)
							{
								case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent:
									TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetDefaultParent(ControlKey), false, true);
									break;
								case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World:
									TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetWorldSpaceReferenceKey(), false, true);
									break;
								case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig:
									TLLRN_ControlRig->SwitchToParent(ControlKey, PreviousValue.TLLRN_ControlTLLRN_RigElement, false, true);
									break;
							}
							
							//now set time -1 frame value
							TLLRN_ControlRig->Evaluate_AnyThread();
							KeyframeContext.LocalTime = TickResolution.AsSeconds(FFrameTime(TimeToCompensate));
							TLLRN_ControlRig->SetControlGlobalTransform(Control->GetFName(), ControlWorldTransforms[0], true, KeyframeContext, false /*undo*/, false /*bPrintPython*/, true/* bFixEulerFlips*/);
							
							bDidIt = true;
						}
					}
				}
			}
		}
	}
	
	if (bDidIt)
	{
		Sequencer->ForceEvaluate();
	}
}

FLinearColor FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetColor(const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& Key)
{
	const UCurveEditorSettings* Settings = GetDefault<UCurveEditorSettings>();
	static TMap<FName, FLinearColor> ColorsForSpaces;
	switch (Key.SpaceType)
	{
		case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent:
		{
			TOptional<FLinearColor> OptColor = Settings->GetSpaceSwitchColor(FString(TEXT("Parent")));
			if (OptColor.IsSet())
			{
				return OptColor.GetValue();
			}

			return  FLinearColor(.93, .31, .19); //pastel orange

		}
		case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World:
		{
			TOptional<FLinearColor> OptColor = Settings->GetSpaceSwitchColor(FString(TEXT("World")));
			if (OptColor.IsSet())
			{
				return OptColor.GetValue();
			}

			return  FLinearColor(.198, .610, .558); //pastel teal
		}
		case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig:
		{
			TOptional<FLinearColor> OptColor = Settings->GetSpaceSwitchColor(Key.TLLRN_ControlTLLRN_RigElement.Name.ToString());
			if (OptColor.IsSet())
			{
				return OptColor.GetValue();
			}
			if (FLinearColor* Color = ColorsForSpaces.Find(Key.TLLRN_ControlTLLRN_RigElement.Name))
			{
				return *Color;
			}
			else
			{
				FLinearColor RandomColor = UCurveEditorSettings::GetNextRandomColor();
				ColorsForSpaces.Add(Key.TLLRN_ControlTLLRN_RigElement.Name, RandomColor);
				return RandomColor;
			}
		}
	};
	return  FLinearColor(FColor::White);

}

FReply FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::OpenBakeDialog(ISequencer* Sequencer, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel,int32 KeyIndex, UMovieSceneSection* SectionToKey )
{

	if (!Sequencer || !Sequencer->GetFocusedMovieSceneSequence()|| !Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene())
	{
		return FReply::Unhandled();
	}
	if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionToKey))
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Section->GetTLLRN_ControlRig())
		{
			FName ControlName = Section->FindControlNameFromSpaceChannel(Channel);

			if (ControlName == NAME_None)
			{
				return FReply::Unhandled();
			}
			FTLLRN_TLLRN_RigSpacePickerBakeSettings Settings;

			const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
			FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;

			UTLLRN_RigHierarchy* TLLRN_RigHierarchy = TLLRN_ControlRig->GetHierarchy();
			Settings.TargetSpace = UTLLRN_RigHierarchy::GetDefaultParentKey();

			TRange<FFrameNumber> Range = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange();
			Settings.Settings.StartFrame = Range.GetLowerBoundValue();
			Settings.Settings.EndFrame = Range.GetUpperBoundValue();
			TMovieSceneChannelData<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> ChannelData = Channel->GetData();
			TArrayView<const FFrameNumber> Times = ChannelData.GetTimes();
			if (KeyIndex >= 0 && KeyIndex < Times.Num())
			{
				Settings.Settings.StartFrame = Times[KeyIndex];
				if (KeyIndex + 1 < Times.Num())
				{
					Settings.Settings.EndFrame = Times[KeyIndex + 1];
				}
			}
			TArray<FTLLRN_RigElementKey> Controls;
			FTLLRN_RigElementKey Key;
			Key.Name = ControlName;
			Key.Type = ETLLRN_RigElementType::Control;
			Controls.Add(Key);
			TSharedRef<STLLRN_RigSpacePickerBakeWidget> BakeWidget =
				SNew(STLLRN_RigSpacePickerBakeWidget)
				.Settings(Settings)
				.Hierarchy(TLLRN_ControlRig->GetHierarchy())
				.Controls(Controls)
				.Sequencer(Sequencer)
				.GetControlCustomization_Lambda([TLLRN_ControlRig](UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey& InControlKey)
				{
					return TLLRN_ControlRig->GetControlCustomization(InControlKey);
				})
				.OnBake_Lambda([Sequencer, TLLRN_ControlRig, TickResolution](UTLLRN_RigHierarchy* InHierarchy, TArray<FTLLRN_RigElementKey> InControls, FTLLRN_TLLRN_RigSpacePickerBakeSettings InSettings)
				{
					
					FScopedTransaction Transaction(LOCTEXT("BakeControlToSpace", "Bake Control In Space"));
					for (const FTLLRN_RigElementKey& ControlKey : InControls)
					{
						FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig, ControlKey.Name, Sequencer, false /*bCreateIfNeeded*/);
						if (SpaceChannelAndSection.SpaceChannel)
						{
							FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerBakeControlInSpace(TLLRN_ControlRig, Sequencer, SpaceChannelAndSection.SpaceChannel, SpaceChannelAndSection.SectionToKey,
								InHierarchy, ControlKey, InSettings);
						}
					}
					return FReply::Handled();
				});

			return BakeWidget->OpenDialog(true);
			
		}
	}
	return FReply::Unhandled();
}

TUniquePtr<FCurveModel> CreateCurveEditorModel(const TMovieSceneChannelHandle<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>& ChannelHandle, const UE::Sequencer::FCreateCurveEditorModelParams& Params)
{
	if (FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel = ChannelHandle.Get())
	{
		if (Channel->GetNumKeys() > 0)
		{
			const UCurveEditorSettings* Settings = GetDefault<UCurveEditorSettings>();
			if (Settings == nullptr || Settings->GetShowBars())
			{
				return MakeUnique<FTLLRN_ControlTLLRN_RigSpaceChannelCurveModel>(ChannelHandle, Params.OwningSection, Params.Sequencer);
			}
		}
	}
	return nullptr;
}

TArray<FKeyBarCurveModel::FBarRange> FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindRanges(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, const UMovieSceneSection* Section)
{
	TArray<FKeyBarCurveModel::FBarRange> Range;
	if (Channel && Section)
	{
		FFrameRate TickResolution = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();
		TArray<FSpaceRange> SpaceRanges = Channel->FindSpaceIntervals();
		for (const FSpaceRange& SpaceRange : SpaceRanges)
		{
			FKeyBarCurveModel::FBarRange BarRange;
			double LowerValue = SpaceRange.Range.GetLowerBoundValue() / TickResolution;
			double UpperValue = SpaceRange.Range.GetUpperBoundValue() / TickResolution;

			BarRange.Range.SetLowerBound(TRangeBound<double>(LowerValue));
			BarRange.Range.SetUpperBound(TRangeBound<double>(UpperValue));

			BarRange.Name = SpaceRange.Key.GetName();
			BarRange.Color = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetColor(SpaceRange.Key);

			Range.Add(BarRange);
		}
	}
	return  Range;
}

// NOTE use this function in HandleSpaceKeyTimeChanged, DeleteTransformKeysAtThisTime, ...
TPair<FTLLRN_RigControlElement*, FTLLRN_ChannelMapInfo*> FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetControlAndChannelInfo(UTLLRN_ControlRig* TLLRN_ControlRig, UMovieSceneTLLRN_ControlRigParameterSection* Section, FName ControlName)
{
	FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig ? TLLRN_ControlRig->FindControl(ControlName) : nullptr;
	FTLLRN_ChannelMapInfo* pChannelIndex = Section ? Section->ControlChannelMap.Find(ControlName) : nullptr;

	return TPair<FTLLRN_RigControlElement*, FTLLRN_ChannelMapInfo*>(ControlElement, pChannelIndex);
}

// NOTE use this function in HandleSpaceKeyTimeChanged, DeleteTransformKeysAtThisTime, ...
int32 FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetNumFloatChannels(const ETLLRN_RigControlType InControlType)
{
	switch (InControlType)
	{
	case ETLLRN_RigControlType::Position:
	case ETLLRN_RigControlType::Scale:
	case ETLLRN_RigControlType::Rotator:
		return 3;
	case ETLLRN_RigControlType::TransformNoScale:
		return 6;
	case ETLLRN_RigControlType::Transform:
	case ETLLRN_RigControlType::EulerTransform:
		return 9;
	default:
		break;
	}
	return 0;
}

int32 DrawExtra(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, const UMovieSceneSection* Owner, const FSequencerChannelPaintArgs& PaintArgs, int32 LayerId)
{
	using namespace UE::Sequencer;

	if (const UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Owner))
	{
		TArray<FKeyBarCurveModel::FBarRange> Ranges = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindRanges(Channel, Owner);
		const FSlateBrush* WhiteBrush = FAppStyle::GetBrush("WhiteBrush");
		const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;

		const FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle("ToolTip.LargerFont");
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		const FVector2D LocalSize = PaintArgs.Geometry.GetLocalSize();
		const float LaneTop = 0;

		const double InputMin = PaintArgs.TimeToPixel.PixelToSeconds(0.f);
		const double InputMax = PaintArgs.TimeToPixel.PixelToSeconds(LocalSize.X);

		int32 LastLabelPos = -1;
		for (int32 Index = 0; Index < Ranges.Num(); ++Index)
		{
			const FKeyBarCurveModel::FBarRange& Range = Ranges[Index];
			const double LowerSeconds = Range.Range.GetLowerBoundValue();
			const double UpperSeconds = Range.Range.GetUpperBoundValue();
			const bool bOutsideUpper = (Index != Ranges.Num() - 1) && UpperSeconds < InputMin;
			if (LowerSeconds > InputMax || bOutsideUpper) //out of range
			{
				continue;
			}

			FLinearColor CurveColor = Range.Color;
	
			static FLinearColor ZebraTint = FLinearColor::White.CopyWithNewOpacity(0.01f);
			if (CurveColor == FLinearColor::White)
			{
				CurveColor = ZebraTint;
			}
			else
			{
				CurveColor = CurveColor * (1.f - ZebraTint.A) + ZebraTint * ZebraTint.A;
			}
			

			if (CurveColor != FLinearColor::White)
			{
				const double LowerSecondsForBox = (Index == 0 && LowerSeconds > InputMin) ? InputMin : LowerSeconds;
				const double BoxPos = PaintArgs.TimeToPixel.SecondsToPixel(LowerSecondsForBox);

				const FPaintGeometry BoxGeometry = PaintArgs.Geometry.ToPaintGeometry(
					FVector2D(PaintArgs.Geometry.GetLocalSize().X, PaintArgs.Geometry.GetLocalSize().Y),
					FSlateLayoutTransform(FVector2D(BoxPos, LaneTop))
				);

				FSlateDrawElement::MakeBox(PaintArgs.DrawElements, LayerId, BoxGeometry, WhiteBrush, DrawEffects, CurveColor);
			}
			const double LowerSecondsForLabel = (InputMin > LowerSeconds) ? InputMin : LowerSeconds;
			double LabelPos = PaintArgs.TimeToPixel.SecondsToPixel(LowerSecondsForLabel) + 10;

			const FText Label = FText::FromName(Range.Name);
			const FVector2D TextSize = FontMeasure->Measure(Label, FontInfo);
			if (Index > 0)
			{
				LabelPos = (LabelPos < LastLabelPos) ? LastLabelPos + 5 : LabelPos;
			}
			LastLabelPos = LabelPos + TextSize.X + 15;
			const FVector2D Position(LabelPos, LaneTop + (PaintArgs.Geometry.GetLocalSize().Y - TextSize.Y) * .5f);

			const FPaintGeometry LabelGeometry = PaintArgs.Geometry.ToPaintGeometry(
				FSlateLayoutTransform(Position)
			);

			FSlateDrawElement::MakeText(PaintArgs.DrawElements, LayerId, LabelGeometry, Label, FontInfo, DrawEffects, FLinearColor::White);
		}
	}

	return LayerId + 1;
}

void DrawKeys(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, TArrayView<const FKeyHandle> InKeyHandles, const UMovieSceneSection* InOwner, TArrayView<FKeyDrawParams> OutKeyDrawParams)
{
	if (const UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InOwner))
	{
		TMovieSceneChannelData<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> ChannelInterface = Channel->GetData();

		for (int32 Index = 0; Index < InKeyHandles.Num(); ++Index)
		{
			FKeyHandle Handle = InKeyHandles[Index];

			FKeyDrawParams Params;
			static const FName SquareKeyBrushName("Sequencer.KeySquare");
			const FSlateBrush* SquareKeyBrush = FAppStyle::GetBrush(SquareKeyBrushName);
			Params.FillBrush = FAppStyle::Get().GetBrush("FilledBorder");
			Params.BorderBrush = SquareKeyBrush;
			const int32 KeyIndex = ChannelInterface.GetIndex(Handle);

			OutKeyDrawParams[Index] = Params;
		}
	}
}

#undef LOCTEXT_NAMESPACE
