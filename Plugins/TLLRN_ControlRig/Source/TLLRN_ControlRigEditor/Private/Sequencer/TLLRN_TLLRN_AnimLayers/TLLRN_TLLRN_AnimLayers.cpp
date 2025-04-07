// Copyright Epic Games, Inc. All Rights Reserved.
#include "TLLRN_TLLRN_AnimLayers.h"
#include "TLLRN_ControlRig.h"
#include "ISequencer.h"
#include "MVVM/Selection/Selection.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "MVVM/ViewModels/SequencerOutlinerViewModel.h"
#include "MVVM/ViewModels/SequencerTrackAreaViewModel.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "MVVM/ViewModels/ObjectBindingModel.h"
#include "LevelSequence.h"
#include "TLLRN_ControlRigSequencerEditorLibrary.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieScenePropertyTrack.h"
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequencePlayer.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "MovieScene.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "Editor/EditorEngine.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/Selection.h"
#include "ScopedTransaction.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditorModeManager.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Channels/MovieSceneBoolChannel.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "Channels/MovieSceneIntegerChannel.h"
#include "Channels/MovieSceneByteChannel.h"
#include "ISequencerPropertyKeyedStatus.h"
#include "BakingAnimationKeySettings.h"
#include "Algo/Accumulate.h"
#include "MovieSceneToolHelpers.h"

#define LOCTEXT_NAMESPACE "TLLRN_TLLRN_AnimLayers"


static TArray<FGuid> GetSelectedOutlinerGuids(ISequencer* SequencerPtr)
{
	using namespace UE::Sequencer;
	TArray<FGuid> SelectedObjects;// = SequencerPtr->GetViewModel()->GetSelection()->GetBoundObjectsGuids(); //this is not exposed :(

	for (TViewModelPtr<FObjectBindingModel> ObjectBindingNode : SequencerPtr->GetViewModel()->GetSelection()->Outliner.Filter<FObjectBindingModel>())
	{
		FGuid Guid = ObjectBindingNode->GetObjectGuid();
		SelectedObjects.Add(Guid);
	}
	return SelectedObjects;
}

struct FTLLRN_ControlRigAndControlsAndTrack
{
	UMovieSceneTLLRN_ControlRigParameterTrack* Track;
	UTLLRN_ControlRig* TLLRN_ControlRig;
	TArray<FName> Controls;
};

struct FObjectAndTrack
{
	UMovieScenePropertyTrack* Track;
	UObject* BoundObject;
};

static void GetSelectedTLLRN_ControlRigsAndBoundObjects(ISequencer* SequencerPtr, TArray<FTLLRN_ControlRigAndControlsAndTrack>& OutSelectedCRs, TArray<FObjectAndTrack>& OutBoundObjects)
{
	if (SequencerPtr == nullptr || SequencerPtr->GetViewModel() == nullptr)
	{
		return;
	}
	ULevelSequence* LevelSequence = Cast<ULevelSequence>(SequencerPtr->GetFocusedMovieSceneSequence());
	if (LevelSequence == nullptr)
	{
		return;
	}

	using namespace UE::Sequencer;
	TArray<FGuid> SelectedObjects = GetSelectedOutlinerGuids(SequencerPtr);;// = SequencerPtr->GetViewModel()->GetSelection()->GetBoundObjectsGuids();

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (MovieScene)
	{
		const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();
		for (const FMovieSceneBinding& Binding : Bindings)
		{
			bool bHaveTLLRN_ControlRig = false;
			TArray<UMovieSceneTrack*> CRTracks = MovieScene->FindTracks(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.GetObjectGuid(), NAME_None);
			for (UMovieSceneTrack* AnyOleTrack : CRTracks)
			{
				if (UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(AnyOleTrack))
				{
					if (UTLLRN_ControlRig* TLLRN_ControlRig = Track->GetTLLRN_ControlRig())
					{
						FTLLRN_ControlRigAndControlsAndTrack CRControls;
						CRControls.TLLRN_ControlRig = TLLRN_ControlRig;
						CRControls.Track = Track;
						CRControls.Controls = TLLRN_ControlRig->CurrentControlSelection();
						if (CRControls.Controls.Num() > 0)
						{
							bHaveTLLRN_ControlRig = true;
							OutSelectedCRs.Add(CRControls);
						}
					}
				}
			}
			//if we have control rig controls don't add the base skel mesh for now
			if (bHaveTLLRN_ControlRig == false && SelectedObjects.Contains(Binding.GetObjectGuid()))
			{
				TArray<UMovieSceneTrack*> Tracks = MovieScene->FindTracks(UMovieScenePropertyTrack::StaticClass(), Binding.GetObjectGuid(), NAME_None);
				if (Tracks.Num() < 1)
				{
					continue;
				}
				const FMovieSceneSequenceID& SequenceID = SequencerPtr->State.FindSequenceId(LevelSequence);

				for (UMovieSceneTrack* AnyOleTrack : Tracks)
				{
					if (UMovieScenePropertyTrack* Track = Cast<UMovieScenePropertyTrack>(AnyOleTrack))
					{
						const FMovieSceneBlendTypeField SupportedBlendTypes = Track->GetSupportedBlendTypes();
						if (SupportedBlendTypes.Num() == 0)
						{
							continue;
						}
						const TArrayView<TWeakObjectPtr<UObject>>& BoundObjects = SequencerPtr->FindBoundObjects(Binding.GetObjectGuid(), SequenceID);
						for (const TWeakObjectPtr<UObject> CurrentBoundObject : BoundObjects)
						{
							if (UObject* BoundObject = CurrentBoundObject.Get())
							{
								FObjectAndTrack ObjectAndTrack;
								ObjectAndTrack.BoundObject = BoundObject;
								ObjectAndTrack.Track = Track;
								OutBoundObjects.Add(ObjectAndTrack);
							}
						}
					}
				}
			}
		}
	}
}

bool FTLLRN_AnimLayerSelectionSet::MergeWithAnotherSelection(const FTLLRN_AnimLayerSelectionSet& Selection)
{
	if (BoundObject.IsValid() && (BoundObject.Get() == Selection.BoundObject.Get()))
	{
		for (const TPair<FName, FTLLRN_AnimLayerPropertyAndChannels>& Incoming : Selection.Names)
		{
			FTLLRN_AnimLayerPropertyAndChannels& Channels = Names.FindOrAdd(Incoming.Key);
			Channels.Channels |= (Incoming.Value.Channels);
		}
		return true;
	}
	return false;
}

FTLLRN_AnimLayerState::FTLLRN_AnimLayerState() : bKeyed(ECheckBoxState::Unchecked), bActive(true),  bLock(false), Weight(1.0), Type((uint32)ETLLRN_AnimLayerType::Base)
{
	Name = FText(LOCTEXT("BaseLayer", "Base Layer"));
}

FText FTLLRN_AnimLayerState::TLLRN_AnimLayerTypeToText() const
{
	if ((ETLLRN_AnimLayerType)Type == ETLLRN_AnimLayerType::Additive)
	{
		return FText(LOCTEXT("Additive", "Additive"));
	}
	else if ((ETLLRN_AnimLayerType)Type == ETLLRN_AnimLayerType::Override)
	{
		return FText(LOCTEXT("Override", "Override"));
	}
	return FText(LOCTEXT("Base", "Base"));
}


UTLLRN_AnimLayer::UTLLRN_AnimLayer(class FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WeightProxy = NewObject<UTLLRN_AnimLayerWeightProxy>(this, TEXT("Weight"), RF_Transactional);
}

void UTLLRN_AnimLayer::SetKey(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& KeyedPropertyHandle)
{
	FScopedTransaction PropertyChangedTransaction(LOCTEXT("KeyTLLRN_AnimLayerWeight", "Key Anim Layer Weight"), !GIsTransacting);
	bool bAnythingKeyed = false;

	for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid() && SectionItem.Section.Get()->TryModify())
				{
					FMovieSceneFloatChannel* FloatChannel = nullptr;
					if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionItem.Section.Get()))
					{
						FloatChannel = &CRSection->Weight;
					}
					else if (UMovieScene3DTransformSection* LayerSection = Cast<UMovieScene3DTransformSection>(SectionItem.Section.Get()))
					{
						FloatChannel = LayerSection->GetWeightChannel();
					}
					if (FloatChannel)
					{
						//don't add key if there!
						FFrameTime LocalTime = Sequencer->GetLocalTime().Time;
						const TRange<FFrameNumber> FrameRange = TRange<FFrameNumber>(LocalTime.FrameNumber);
						TArray<FFrameNumber> KeyTimes;
						FloatChannel->GetKeys(FrameRange, &KeyTimes, nullptr);
						if (KeyTimes.Num() == 0)
						{
							float Value = State.Weight;
							FloatChannel->AddCubicKey(LocalTime.FrameNumber, Value, ERichCurveTangentMode::RCTM_SmartAuto);
							bAnythingKeyed = true;
						}
					}
				}
			}
		}
	}

	if(bAnythingKeyed == false)
	{
		PropertyChangedTransaction.Cancel();
	}
}

EPropertyKeyedStatus UTLLRN_AnimLayer::GetPropertyKeyedStatus(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& PropertyHandle)
{
	EPropertyKeyedStatus KeyedStatus = EPropertyKeyedStatus::NotKeyed;

	if (Sequencer.IsValid() == false || Sequencer->GetFocusedMovieSceneSequence() == nullptr)
	{
		return KeyedStatus;
	}
	const TRange<FFrameNumber> FrameRange = TRange<FFrameNumber>(Sequencer->GetLocalTime().Time.FrameNumber);
	int32 NumKeyed = 0;
	int32 NumToCheck = 0;
	for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					FMovieSceneFloatChannel* FloatChannel = nullptr;
					if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionItem.Section.Get()))
					{
						FloatChannel = &CRSection->Weight;
					}
					else if (UMovieScene3DTransformSection* LayerSection = Cast<UMovieScene3DTransformSection>(SectionItem.Section.Get()))
					{
						FloatChannel = LayerSection->GetWeightChannel();
					}
					if (FloatChannel)
					{
						++NumToCheck;
						EPropertyKeyedStatus NewKeyedStatus = EPropertyKeyedStatus::NotKeyed;
						if (FloatChannel->GetNumKeys() > 0)
						{
							TArray<FFrameNumber> KeyTimes;
							FloatChannel->GetKeys(FrameRange, &KeyTimes, nullptr);
							if (KeyTimes.Num() > 0)
							{
								++NumKeyed;
								NewKeyedStatus = EPropertyKeyedStatus::PartiallyKeyed;
							}
							else
							{
								NewKeyedStatus = EPropertyKeyedStatus::KeyedInOtherFrame;
							}
						}
						KeyedStatus = FMath::Max(KeyedStatus, NewKeyedStatus);
					}
				}
			}
		}
	}
	if (KeyedStatus == EPropertyKeyedStatus::PartiallyKeyed && NumToCheck == NumKeyed)
	{
		KeyedStatus = EPropertyKeyedStatus::KeyedInFrame;
	}
	return KeyedStatus;
}

ECheckBoxState UTLLRN_AnimLayer::GetKeyed() const
{
	TOptional<ECheckBoxState> CurrentVal;
	for (const TPair<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					if (UMovieSceneTrack* Track = Cast<UMovieSceneTrack>(SectionItem.Section.Get()->GetTypedOuter< UMovieSceneTrack>()))
					{
						if (UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track))
						{
							for (const TPair<FName, FTLLRN_AnimLayerPropertyAndChannels>& ControlName : SectionItem.TLLRN_AnimLayerSet.Names)
							{
								if (TLLRN_ControlRigTrack->GetSectionToKey(ControlName.Key) == SectionItem.Section.Get())
								{
									if (CurrentVal.IsSet() && CurrentVal.GetValue() != ECheckBoxState::Checked)
									{
										CurrentVal = ECheckBoxState::Undetermined;
									}
									if (CurrentVal.IsSet() == false)
									{
										CurrentVal = ECheckBoxState::Checked;
									}
								}
								else
								{
									if (CurrentVal.IsSet() && CurrentVal.GetValue() != ECheckBoxState::Unchecked)
									{
										CurrentVal = ECheckBoxState::Undetermined;
									}
									if (CurrentVal.IsSet() == false)
									{
										CurrentVal = ECheckBoxState::Unchecked;
									}
								}
							}
						}
						else
						{
							if (Track->GetSectionToKey() == SectionItem.Section.Get() ||
								(Track->GetAllSections().Num() == 1 && (Track->GetAllSections()[0] == SectionItem.Section.Get())))
							{
								if (CurrentVal.IsSet() && CurrentVal.GetValue() != ECheckBoxState::Checked)
								{
									CurrentVal = ECheckBoxState::Undetermined;
								}
								if (CurrentVal.IsSet() == false)
								{
									CurrentVal = ECheckBoxState::Checked;
								}
							}
							else
							{
								if (CurrentVal.IsSet() && CurrentVal.GetValue() != ECheckBoxState::Unchecked)
								{
									CurrentVal = ECheckBoxState::Undetermined;
								}
								if (CurrentVal.IsSet() == false)
								{
									CurrentVal = ECheckBoxState::Unchecked;
								}
							}
						}
					}
				}
			}
		}
	}
	if (CurrentVal.IsSet())
	{
		if (State.bKeyed != CurrentVal.GetValue() && CurrentVal.GetValue() == ECheckBoxState::Checked)
		{
			SetSectionToKey();
		}
		State.bKeyed = CurrentVal.GetValue();
	}
	else
	{
		//has no sections and is base so it's keyed, else it's not
		State.bKeyed = State.Type == ETLLRN_AnimLayerType::Base ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return State.bKeyed;
}

void UTLLRN_AnimLayer::SetSectionToKey() const
{

	for (const TPair<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					if (UMovieSceneTrack* Track = Cast<UMovieSceneTrack>(SectionItem.Section.Get()->GetTypedOuter< UMovieSceneTrack>()))
					{
						Track->Modify();
						if (UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track))
						{
							for (const TPair<FName, FTLLRN_AnimLayerPropertyAndChannels>& ControlName : SectionItem.TLLRN_AnimLayerSet.Names)
							{
								TLLRN_ControlRigTrack->SetSectionToKey(SectionItem.Section.Get(), ControlName.Key);
							}
						}
						else
						{
							Track->SetSectionToKey(SectionItem.Section.Get());
						}
					}
				}
			}
		}
	}
}

void UTLLRN_AnimLayer::SetKeyed()
{
	State.bKeyed = ECheckBoxState::Checked;
	SetSectionToKey();

}

bool UTLLRN_AnimLayer::GetActive() const
{
	TOptional<bool> CurrentVal;
	for (const TPair<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					const bool bActive = SectionItem.Section.Get()->IsActive();
					if (CurrentVal.IsSet() && CurrentVal.GetValue() != bActive)
					{
						SectionItem.Section.Get()->SetIsActive(CurrentVal.GetValue());
					}
					if (CurrentVal.IsSet() == false)
					{
						CurrentVal = bActive;
					}
				}
			}
		}
	}
	if (CurrentVal.IsSet())
	{
		State.bActive = CurrentVal.GetValue();
	}
	return State.bActive;
}

void UTLLRN_AnimLayer::SetActive(bool bInActive)
{
	const FScopedTransaction Transaction(LOCTEXT("SetActive_Transaction", "Set Active"), !GIsTransacting);
	Modify();
	State.bActive = bInActive;
	for (const TPair<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					SectionItem.Section->Modify();
					SectionItem.Section.Get()->SetIsActive(State.bActive);
				}
			}
		}
	}
}

bool UTLLRN_AnimLayer::AddSelectedInSequencer() 
{
	TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset();
	if (SequencerPtr.IsValid() == false)
	{
		return false;
	}
	TArray<FTLLRN_ControlRigAndControlsAndTrack> SelectedCRs;
	TArray<FObjectAndTrack> SelectedBoundObjects;
	GetSelectedTLLRN_ControlRigsAndBoundObjects(SequencerPtr.Get(), SelectedCRs, SelectedBoundObjects);
	if (SelectedCRs.Num() <= 0 && SelectedBoundObjects.Num() <= 0)
	{
		return false;
	}
	bool bAddedSomething = false;
	const FScopedTransaction Transaction(LOCTEXT("AddSelectedTLLRN_AnimLayer_Transaction", "Add Selected"),!GIsTransacting);
	Modify();
	for (FTLLRN_ControlRigAndControlsAndTrack& CRControls : SelectedCRs)
	{
		if (FTLLRN_AnimLayerItem* ExistingTLLRN_AnimLayerItem = TLLRN_AnimLayerItems.Find(CRControls.TLLRN_ControlRig))
		{
			for (FTLLRN_AnimLayerSectionItem& SectionItem : ExistingTLLRN_AnimLayerItem->SectionItems)
			{
				if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionItem.Section))
				{
					for (FName& ControlName : CRControls.Controls)
					{
						if (SectionItem.TLLRN_AnimLayerSet.Names.Contains(ControlName) == false)
						{
							FTLLRN_AnimLayerPropertyAndChannels Channels;
							Channels.Name = ControlName;
							Channels.Channels = (uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform;
							SectionItem.TLLRN_AnimLayerSet.Names.Add(ControlName, Channels);
						}
					}
					TArray<FName> AllControls;
					SectionItem.TLLRN_AnimLayerSet.Names.GenerateKeyArray(AllControls);
					UTLLRN_TLLRN_AnimLayers::SetUpTLLRN_ControlRigSection(CRSection, AllControls);
					bAddedSomething = true;
				}
			}
		}
		else
		{
			//add new section
			FTLLRN_AnimLayerItem TLLRN_AnimLayerItem;
			FTLLRN_AnimLayerSectionItem SectionItem;
			SectionItem.TLLRN_AnimLayerSet.BoundObject = CRControls.TLLRN_ControlRig;
			for (FName& ControlName : CRControls.Controls)
			{
				FTLLRN_AnimLayerPropertyAndChannels Channels;
				Channels.Name = ControlName;
				Channels.Channels = (uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform;
				SectionItem.TLLRN_AnimLayerSet.Names.Add(ControlName, Channels);
			}
			// Add a new section that starts and ends at the same time
			TGuardValue<bool> GuardSetSection(CRControls.Track->bSetSectionToKeyPerControl, false);
			if (UMovieSceneTLLRN_ControlRigParameterSection* NewSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(CRControls.Track->CreateNewSection()))
			{
				ensureAlwaysMsgf(NewSection->HasAnyFlags(RF_Transactional), TEXT("CreateNewSection must return an instance with RF_Transactional set! (pass RF_Transactional to NewObject)"));
				NewSection->SetFlags(RF_Transactional);
				NewSection->SetTransformMask(FMovieSceneTransformMask{ EMovieSceneTransformChannel::All });
				FMovieSceneFloatChannel* FloatChannel = &NewSection->Weight;
				SectionItem.Section = NewSection;
				TLLRN_AnimLayerItem.SectionItems.Add(SectionItem);
				TLLRN_AnimLayerItems.Add(CRControls.TLLRN_ControlRig, TLLRN_AnimLayerItem);
				UTLLRN_TLLRN_AnimLayers::SetUpSectionDefaults(SequencerPtr.Get(), this, CRControls.Track, NewSection,FloatChannel);
				UTLLRN_TLLRN_AnimLayers::SetUpTLLRN_ControlRigSection(NewSection, CRControls.Controls);
				bAddedSomething = true;
			}
		}
	}

	for (FObjectAndTrack& ObjectAndTrack : SelectedBoundObjects)
	{
		FTLLRN_AnimLayerItem& TLLRN_AnimLayerItem = TLLRN_AnimLayerItems.FindOrAdd(ObjectAndTrack.BoundObject);
		FTLLRN_AnimLayerSectionItem SectionItem;
		SectionItem.TLLRN_AnimLayerSet.BoundObject = ObjectAndTrack.BoundObject;
		/* if we ever support channels
		for (FName& ControlName : SelectedControls)
		{
			FTLLRN_AnimLayerPropertyAndChannels Channels;
			Channels.Name = ControlName;
			Channels.Channels = (uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform;
			TLLRN_AnimLayerItem.TLLRN_AnimLayerSet.Names.Add(ControlName, Channels);

		}*/
		// Add a new section that starts and ends at the same time
		ObjectAndTrack.Track->Modify();
		if (UMovieSceneSection* NewSection = ObjectAndTrack.Track->CreateNewSection())
		{
			ensureAlwaysMsgf(NewSection->HasAnyFlags(RF_Transactional), TEXT("CreateNewSection must return an instance with RF_Transactional set! (pass RF_Transactional to NewObject)"));
			NewSection->SetFlags(RF_Transactional);
			FMovieSceneFloatChannel* FloatChannel = nullptr;
			if (UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(NewSection))
			{
				TransformSection->SetMask(FMovieSceneTransformMask{ EMovieSceneTransformChannel::All });
				FloatChannel = TransformSection->GetWeightChannel();
			}
			SectionItem.Section = NewSection;
			TLLRN_AnimLayerItem.SectionItems.Add(SectionItem);
			UTLLRN_TLLRN_AnimLayers::SetUpSectionDefaults(SequencerPtr.Get(), this, ObjectAndTrack.Track, NewSection, FloatChannel);
			bAddedSomething = true;
		}
	}
	if (bAddedSomething)
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
		{
			TLLRN_TLLRN_AnimLayers->SetUpBaseLayerSections();
		}
		SetKeyed();
	}
	SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	return true;
}

bool UTLLRN_AnimLayer::RemoveTLLRN_AnimLayerItem(UObject* InObject)
{
	if (FTLLRN_AnimLayerItem* Item =  TLLRN_AnimLayerItems.Find(InObject))
	{
		for (const FTLLRN_AnimLayerSectionItem& SectionItem : Item->SectionItems)
		{
			if (SectionItem.Section.IsValid())
			{
				if (UMovieSceneTrack* Track = SectionItem.Section->GetTypedOuter<UMovieSceneTrack>())
				{
					if (Track->GetAllSections().Find(SectionItem.Section.Get()) != 0)
					{
						Track->Modify();
						Track->RemoveSection(*(SectionItem.Section.Get()));
					}
				}
			}
		}
		TLLRN_AnimLayerItems.Remove(InObject);
		return true;
	}
	return false;
	
}
bool UTLLRN_AnimLayer::RemoveSelectedInSequencer() 
{
	TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset();
	if (SequencerPtr.IsValid() == false)
	{
		return false;
	}

	if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
	{
		if (TLLRN_TLLRN_AnimLayers->TLLRN_TLLRN_AnimLayers[0] == this)
		{
			return false;
		}
	}

	TArray<FTLLRN_ControlRigAndControlsAndTrack> SelectedCRs;
	TArray<FObjectAndTrack> SelectedBoundObjects;
	GetSelectedTLLRN_ControlRigsAndBoundObjects(SequencerPtr.Get(), SelectedCRs, SelectedBoundObjects);
	if (SelectedCRs.Num() <= 0 && SelectedBoundObjects.Num() < 0)
	{
		return false;
	}
	bool bRemovedSomething = false;
	UTLLRN_AnimLayer& TLLRN_AnimLayer = *this;
	const FScopedTransaction Transaction(LOCTEXT("RemoveSelected_Transaction", "Remove Selected"), !GIsTransacting);
	Modify();
	for (FTLLRN_ControlRigAndControlsAndTrack& CRControls : SelectedCRs)
	{
		if (FTLLRN_AnimLayerItem* ExistingTLLRN_AnimLayerItem = TLLRN_AnimLayer.TLLRN_AnimLayerItems.Find(CRControls.TLLRN_ControlRig))
		{
			for (FTLLRN_AnimLayerSectionItem& SectionItem : ExistingTLLRN_AnimLayerItem->SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionItem.Section))
					{
						for (FName& ControlName : CRControls.Controls)
						{
							if (SectionItem.TLLRN_AnimLayerSet.Names.Contains(ControlName) == true)
							{
								SectionItem.TLLRN_AnimLayerSet.Names.Remove(ControlName);
							}
							TArray<FName> ControlNames;
							SectionItem.TLLRN_AnimLayerSet.Names.GetKeys(ControlNames);
							UTLLRN_TLLRN_AnimLayers::SetUpTLLRN_ControlRigSection(CRSection, ControlNames);
							bRemovedSomething = true;
						}
					}
					if (SectionItem.TLLRN_AnimLayerSet.Names.Num() == 0)
					{
						TLLRN_AnimLayer.RemoveTLLRN_AnimLayerItem(CRControls.TLLRN_ControlRig);
						bRemovedSomething = true;
						break;
					}
				}
			}
		}
	}
	for (FObjectAndTrack& ObjectAndTrack : SelectedBoundObjects)
	{
		if (TLLRN_AnimLayer.TLLRN_AnimLayerItems.Find(ObjectAndTrack.BoundObject))
		{
			TLLRN_AnimLayer.RemoveTLLRN_AnimLayerItem(ObjectAndTrack.BoundObject);
			bRemovedSomething = true;
		}
	}
	if (bRemovedSomething)
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
		{
			TLLRN_TLLRN_AnimLayers->SetUpBaseLayerSections();
		}
	}
	if (TLLRN_AnimLayer.TLLRN_AnimLayerItems.Num() == 0)
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
		{
			int32 Index = TLLRN_TLLRN_AnimLayers->GetTLLRN_AnimLayerIndex(this);
			if (Index != INDEX_NONE)
			{
				TLLRN_TLLRN_AnimLayers->DeleteTLLRN_AnimLayer(SequencerPtr.Get(), Index);
			}
		}
	}
	
	SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemRemoved);
	return true;

}

void UTLLRN_AnimLayer::GetTLLRN_AnimLayerObjects(FTLLRN_AnimLayerObjects& InLayerObjects) const
{
	for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					if (Pair.Key->IsA<UTLLRN_ControlRig>())
					{
						if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionItem.Section.Get()))
						{
							FTLLRN_AnimLayerTLLRN_ControlRigObject TLLRN_ControlRigObject;
							TLLRN_ControlRigObject.TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(Pair.Key.Get());
							for (const TPair<FName, FTLLRN_AnimLayerPropertyAndChannels>& ControlName : SectionItem.TLLRN_AnimLayerSet.Names)
							{
								TLLRN_ControlRigObject.ControlNames.Add(ControlName.Key);
							}
							InLayerObjects.TLLRN_ControlRigObjects.Add(TLLRN_ControlRigObject);
						}
					}
					else if (IsAccepableNonTLLRN_ControlRigObject(Pair.Key.Get()))
					{
						FTLLRN_AnimLayerSceneObject SceneObject;
						SceneObject.SceneObjectOrComponent = Pair.Key;
						InLayerObjects.SceneObjects.Add(SceneObject);
					}
				}
			}
		}
	}
}

bool UTLLRN_AnimLayer::IsAccepableNonTLLRN_ControlRigObject(UObject* InObject)
{
	return (InObject->IsA<AActor>() || InObject->IsA<USceneComponent>());
}

ECheckBoxState UTLLRN_AnimLayer::GetSelected() const
{
	FTLLRN_AnimLayerObjects LayerObjects;
	GetTLLRN_AnimLayerObjects(LayerObjects);
	TOptional<ECheckBoxState>  SelectionState;
	for (const FTLLRN_AnimLayerTLLRN_ControlRigObject& TLLRN_ControlRigObject : LayerObjects.TLLRN_ControlRigObjects)
	{
		if (TLLRN_ControlRigObject.TLLRN_ControlRig.IsValid())
		{
			TArray<FName> SelectedControls = TLLRN_ControlRigObject.TLLRN_ControlRig->CurrentControlSelection();
			for (const FName& ControlName : TLLRN_ControlRigObject.ControlNames)
			{
				if (SelectedControls.Contains(ControlName))
				{
					if (SelectionState.IsSet() == false)
					{
						SelectionState = ECheckBoxState::Checked;
					}
					else if (SelectionState.GetValue() != ECheckBoxState::Checked)
					{
						SelectionState = ECheckBoxState::Undetermined;
					}
				}
				else
				{
					if (SelectionState.IsSet() == false)
					{
						SelectionState = ECheckBoxState::Unchecked;
					}
					else if (SelectionState.GetValue() != ECheckBoxState::Unchecked)
					{
						SelectionState = ECheckBoxState::Undetermined;
					}
				}
			}
		}
		else
		{
			if (SelectionState.IsSet())
			{
				if (SelectionState.GetValue() == ECheckBoxState::Checked)
				{
					SelectionState = ECheckBoxState::Undetermined;
				}
			}
			else
			{
				SelectionState = ECheckBoxState::Unchecked;
			}
		}
	}
	USelection* ComponentSelection = GEditor->GetSelectedComponents();
	TArray<TWeakObjectPtr<UObject>> SelectedComponents;
	ComponentSelection->GetSelectedObjects(SelectedComponents);
	USelection* ActorSelection = GEditor->GetSelectedActors();
	TArray<TWeakObjectPtr<UObject>> SelectedActors;
	ActorSelection->GetSelectedObjects(SelectedActors);

	for (const FTLLRN_AnimLayerSceneObject& SceneObject : LayerObjects.SceneObjects)
	{
		if (SceneObject.SceneObjectOrComponent.IsValid() &&
			SceneObject.SceneObjectOrComponent->IsA<AActor>())
		{
			if (SelectedActors.Contains(SceneObject.SceneObjectOrComponent))
			{
				if (SelectionState.IsSet() == false)
				{
					SelectionState = ECheckBoxState::Checked;
				}
				else if (SelectionState.GetValue() != ECheckBoxState::Checked)
				{
					SelectionState = ECheckBoxState::Undetermined;
				}
			}
			else
			{
				if (SelectionState.IsSet() == false)
				{
					SelectionState = ECheckBoxState::Unchecked;
				}
				else if (SelectionState.GetValue() != ECheckBoxState::Unchecked)
				{
					SelectionState = ECheckBoxState::Undetermined;
				}
			}
		}
		else if (SceneObject.SceneObjectOrComponent.IsValid() &&
			SceneObject.SceneObjectOrComponent->IsA<USceneComponent>())
		{
			if (SelectedComponents.Contains(SceneObject.SceneObjectOrComponent))
			{
				if (SelectionState.IsSet() == false)
				{
					SelectionState = ECheckBoxState::Checked;
				}
				else if (SelectionState.GetValue() != ECheckBoxState::Checked)
				{
					SelectionState = ECheckBoxState::Undetermined;
				}
			}
			else
			{
				if (SelectionState.IsSet() == false)
				{
					SelectionState = ECheckBoxState::Unchecked;
				}
				else if (SelectionState.GetValue() != ECheckBoxState::Unchecked)
				{
					SelectionState = ECheckBoxState::Undetermined;
				}
			}

		}
	}
	if (SelectionState.IsSet())
	{
		return SelectionState.GetValue();
	}
	return ECheckBoxState::Unchecked;
}

void UTLLRN_AnimLayer::SetSelected(bool bInSelected, bool bClearSelection)
{
	if (!GEditor)
	{
		return;
	}
	FTLLRN_AnimLayerObjects LayerObjects;
	GetTLLRN_AnimLayerObjects(LayerObjects);
	if (LayerObjects.TLLRN_ControlRigObjects.Num() <= 0 && LayerObjects.SceneObjects.Num() <= 0)
	{
		return;
	}
	const FScopedTransaction Transaction(LOCTEXT("SetSelected_Transaction", "Set Selection"), !GIsTransacting);
	Modify();
	State.bSelected = bInSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	if (bClearSelection)
	{
		if ((GEditor->GetSelectedActorCount() || GEditor->GetSelectedComponentCount()))
		{
			GEditor->SelectNone(false, true);
			GEditor->NoteSelectionChange();
		}

		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
		FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
		if (TLLRN_ControlRigEditMode)
		{
			TLLRN_ControlRigs = TLLRN_ControlRigEditMode->GetTLLRN_ControlRigsArray(false /*bIsVisible*/);
			for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
			{
				TLLRN_ControlRig->ClearControlSelection();
			}
		}
		if (bInSelected == false)
		{
			return; //clearing and not selecting so we are done.
		}
	}

	for (const FTLLRN_AnimLayerSceneObject& SceneObject : LayerObjects.SceneObjects)
	{
		if (AActor* Actor = Cast<AActor>(SceneObject.SceneObjectOrComponent.Get()))
		{
			GEditor->SelectActor(Actor, bInSelected, true);
		}
		else if (UActorComponent* Component = Cast<UActorComponent>(SceneObject.SceneObjectOrComponent.Get()))
		{
			GEditor->SelectComponent(Component, bInSelected, true);
		}
	}

	for (const FTLLRN_AnimLayerTLLRN_ControlRigObject& TLLRN_ControlRigObject : LayerObjects.TLLRN_ControlRigObjects)
	{
		if (TLLRN_ControlRigObject.TLLRN_ControlRig.IsValid())
		{
			for (const FName& ControlName : TLLRN_ControlRigObject.ControlNames)
			{
				TLLRN_ControlRigObject.TLLRN_ControlRig->SelectControl(ControlName, bInSelected);
			}
		}
	}	
}

bool UTLLRN_AnimLayer::GetLock() const
{
	TOptional<bool> CurrentVal;
	for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					const bool bIsLocked = SectionItem.Section.Get()->IsLocked();
					if (CurrentVal.IsSet() && CurrentVal.GetValue() != bIsLocked)
					{
						SectionItem.Section.Get()->SetIsLocked(CurrentVal.GetValue());
					}
					if (CurrentVal.IsSet() == false)
					{
						CurrentVal = bIsLocked;
					}
				}
			}
		}
	}
	if (CurrentVal.IsSet())
	{
		State.bLock = CurrentVal.GetValue();
	}
	return State.bLock;
}

void UTLLRN_AnimLayer::SetLock(bool bInLock)
{
	const FScopedTransaction Transaction(LOCTEXT("SetLock_Transaction", "Set Lock"), !GIsTransacting);
	Modify();
	State.bLock = bInLock;
	for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{

					SectionItem.Section->Modify();
					SectionItem.Section.Get()->SetIsLocked(State.bLock);
				}
			}
		}
	}
}

FText UTLLRN_AnimLayer::GetName() const
{
	return State.Name;
}

void UTLLRN_AnimLayer::SetName(const FText& InName)
{
	const FScopedTransaction Transaction(LOCTEXT("SetName_Transaction", "Set Name"), !GIsTransacting);
	Modify();
	State.Name = InName;

	for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					if (UMovieSceneNameableTrack* NameableTrack = Cast<UMovieSceneNameableTrack>(SectionItem.Section.Get()->GetTypedOuter< UMovieSceneNameableTrack>()))
					{
						NameableTrack->Modify();
						NameableTrack->SetTrackRowDisplayName(State.Name, SectionItem.Section.Get()->GetRowIndex());
					}
				}
			}
		}
	}
}

double UTLLRN_AnimLayer::GetWeight() const
{
	TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset();
	if (SequencerPtr)
	{
		TOptional<float> DifferentWeightValue;
		for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
		{
			if (Pair.Key != nullptr)
			{
				for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
				{
					if (SectionItem.Section.IsValid())
					{
						FMovieSceneFloatChannel* FloatChannel = nullptr;
						if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionItem.Section.Get()))
						{
							FloatChannel = &CRSection->Weight;
						}
						else if (UMovieScene3DTransformSection* LayerSection = Cast<UMovieScene3DTransformSection>(SectionItem.Section.Get()))
						{
							FloatChannel = LayerSection->GetWeightChannel();
						}
						if (FloatChannel)
						{
							const FFrameNumber CurrentTime = SequencerPtr->GetLocalTime().Time.FloorToFrame();
							float Value = 0.f;
							FloatChannel->Evaluate(CurrentTime, Value);
							if (State.Weight != Value || WeightProxy->Weight != Value)
							{
								DifferentWeightValue = Value;
							}

						}
					}
				}
			}
		}
		if (DifferentWeightValue.IsSet())
		{
			State.Weight = DifferentWeightValue.GetValue();
			WeightProxy->Weight = State.Weight;
		}
	}
	return State.Weight;
}

static void SetFloatWeightValue(float InValue, ISequencer * Sequencer, UMovieSceneSection* OwningSection,FMovieSceneFloatChannel* Channel)
{
	using namespace UE::MovieScene;
	using namespace UE::Sequencer;


	if (!OwningSection)
	{
		return;
	}
	OwningSection->SetFlags(RF_Transactional);;

	if (!OwningSection->TryModify() || !Channel || !Sequencer)
	{
		return;
	}

	const bool  bAutoSetTrackDefaults = Sequencer->GetAutoSetTrackDefaults();	
	const FFrameNumber CurrentTime = Sequencer->GetLocalTime().Time.FloorToFrame();

	EMovieSceneKeyInterpolation Interpolation = GetInterpolationMode(Channel, CurrentTime, Sequencer->GetKeyInterpolation());

	TArray<FKeyHandle> KeysAtCurrentTime;
	Channel->GetKeys(TRange<FFrameNumber>(CurrentTime), nullptr, &KeysAtCurrentTime);

	if (KeysAtCurrentTime.Num() > 0)
	{
		AssignValue(Channel, KeysAtCurrentTime[0], InValue);
	}
	else
	{
		bool bHasAnyKeys = Channel->GetNumKeys() != 0;

		if (bHasAnyKeys || bAutoSetTrackDefaults == false)
		{
			// When auto setting track defaults are disabled, add a key even when it's empty so that the changed
			// value is saved and is propagated to the property.
			AddKeyToChannel(Channel, CurrentTime, InValue, Interpolation);
			bHasAnyKeys = Channel->GetNumKeys() != 0;
		}

		if (bHasAnyKeys)
		{
			TRange<FFrameNumber> KeyRange = TRange<FFrameNumber>(CurrentTime);
			TRange<FFrameNumber> SectionRange = OwningSection->GetRange();

			if (!SectionRange.Contains(KeyRange))
			{
				OwningSection->SetRange(TRange<FFrameNumber>::Hull(KeyRange, SectionRange));
			}
		}
	}

	// Always update the default value when auto-set default values is enabled so that the last changes
	// are always saved to the track.
	if (bAutoSetTrackDefaults)
	{
		SetChannelDefault(Channel, InValue);
	}

	Channel->AutoSetTangents();
	Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
}

void UTLLRN_AnimLayer::SetWeight(double InWeight)
{
	State.Weight = InWeight;
	TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset();
	if (SequencerPtr)
	{
		for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
		{
			if (Pair.Key != nullptr)
			{
				for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
				{
					if (SectionItem.Section.IsValid())
					{
						if (SectionItem.Section.Get()->TryModify())
						{
							FMovieSceneFloatChannel* FloatChannel = nullptr;
							if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionItem.Section.Get()))
							{
								FloatChannel = &CRSection->Weight;
							}
							else if (UMovieScene3DTransformSection* LayerSection = Cast<UMovieScene3DTransformSection>(SectionItem.Section.Get()))
							{
								FloatChannel = LayerSection->GetWeightChannel();
							}
							if (FloatChannel)
							{
								float WeightValue = (float)InWeight;
								SetFloatWeightValue(WeightValue, SequencerPtr.Get(), SectionItem.Section.Get(), FloatChannel);
							}
						}
					}
				}
			}
		}
	}
}

void UTLLRN_AnimLayerWeightProxy::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
#if WITH_EDITOR
	if (UTLLRN_AnimLayer* TLLRN_AnimLayer = GetTypedOuter<UTLLRN_AnimLayer>())
	{
		if (PropertyChangedEvent.Property && (
			PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet ||
			PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive ||
			PropertyChangedEvent.ChangeType == EPropertyChangeType::Unspecified)
			)
		{
			//set values
			FProperty* Property = PropertyChangedEvent.Property;
			FProperty* MemberProperty = nullptr;
			if ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimLayerWeightProxy, Weight)) ||
				(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimLayerWeightProxy, Weight)))
			{
				Modify();
				TLLRN_AnimLayer->SetWeight(Weight);
			}
		}
	}
#endif
}

#if WITH_EDITOR
void UTLLRN_AnimLayerWeightProxy::PostEditUndo()
{
	if (UTLLRN_AnimLayer* TLLRN_AnimLayer = GetTypedOuter<UTLLRN_AnimLayer>())
	{
		TLLRN_AnimLayer->SetWeight(Weight);
	}
}
#endif
ETLLRN_AnimLayerType UTLLRN_AnimLayer::GetType() const
{
	TOptional<EMovieSceneBlendType> CurrentVal;
	for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					const EMovieSceneBlendType BlendType = SectionItem.Section.Get()->GetBlendType().BlendType;
					if (CurrentVal.IsSet() && CurrentVal.GetValue() != BlendType)
					{
						SectionItem.Section.Get()->SetBlendType(CurrentVal.GetValue());
					}
					if (CurrentVal.IsSet() == false)
					{
						CurrentVal = BlendType;
					}
				}
			}
		}
	}
	if (CurrentVal.IsSet())
	{
		switch (CurrentVal.GetValue())
		{
		case EMovieSceneBlendType::Additive:
			State.Type = (int32)ETLLRN_AnimLayerType::Additive;
			break;
		case EMovieSceneBlendType::Override:
			State.Type = (int32)ETLLRN_AnimLayerType::Override;
			break;
		case EMovieSceneBlendType::Absolute:
			State.Type = (int32)ETLLRN_AnimLayerType::Base;
			break;
		}
	}

	return (ETLLRN_AnimLayerType)(State.Type);
}

static void SetDefaultsForOverride(UMovieSceneSection* InSection)
{
	if (InSection->IsA<UMovieSceneTLLRN_ControlRigParameterSection>())
	{
		return; //control rig sections already handle this
	}
	TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset();
	if (SequencerPtr.IsValid() == false)
	{
		return;
	}
	FFrameNumber FrameNumber = SequencerPtr->GetLocalTime().Time.GetFrame();
	if (UMovieSceneTrack* OwnerTrack = InSection->GetTypedOuter<UMovieSceneTrack>())
	{
		TArray<UMovieSceneSection*> TrackSections = OwnerTrack->GetAllSections();
		int32 SectionIndex = TrackSections.Find((InSection));
		if (SectionIndex != INDEX_NONE)
		{
			InSection->Modify();
			TrackSections.SetNum(SectionIndex); //this will gives us up to the section 
			TArray<UMovieSceneSection*> Sections;
			TArray<UMovieSceneSection*> AbsoluteSections;
			MovieSceneToolHelpers::SplitSectionsByBlendType(EMovieSceneBlendType::Absolute, TrackSections, Sections, AbsoluteSections);
			TArrayView<FMovieSceneFloatChannel*> BaseFloatChannels = InSection->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
			TArrayView<FMovieSceneDoubleChannel*> BaseDoubleChannels = InSection->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
			if (BaseDoubleChannels.Num() > 0)
			{
				int32 NumChannels = BaseDoubleChannels.Num();
				const int32 StartIndex = 0;
				const int32 EndIndex = NumChannels - 1;
				TArray<double> ChannelValues = MovieSceneToolHelpers::GetChannelValues<FMovieSceneDoubleChannel,
					double>(StartIndex,EndIndex, Sections, AbsoluteSections, FrameNumber);
				for (int32 Index = 0; Index < NumChannels; ++Index)
				{
					FMovieSceneDoubleChannel* DoubleChannel = BaseDoubleChannels[Index];
					const double Value = ChannelValues[Index];
					DoubleChannel->SetDefault(Value);
				}
			}
			else if (BaseFloatChannels.Num() > 0)
			{
				int32 NumChannels = BaseFloatChannels.Num();
				const int32 StartIndex = 0;
				const int32 EndIndex = NumChannels - 1;
				TArray<float> ChannelValues = MovieSceneToolHelpers::GetChannelValues<FMovieSceneFloatChannel,
					float>(StartIndex, EndIndex, Sections, AbsoluteSections, FrameNumber);
				for (int32 Index = 0; Index < NumChannels; ++Index)
				{
					FMovieSceneFloatChannel* FloatChannel = BaseFloatChannels[Index];
					const float Value = ChannelValues[Index];
					FloatChannel->SetDefault(Value);
				}
			}
			SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
		}
	}
}

void UTLLRN_AnimLayer::SetType(ETLLRN_AnimLayerType LayerType)
{
	const FScopedTransaction Transaction(LOCTEXT("SetType_Transaction", "Set Type"), !GIsTransacting);
	Modify();

	State.Type = (uint32)(LayerType);
	for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					switch (LayerType)
					{
					case ETLLRN_AnimLayerType::Additive:
						SectionItem.Section.Get()->SetBlendType(EMovieSceneBlendType::Additive);
						break;
					case ETLLRN_AnimLayerType::Override:
						SectionItem.Section.Get()->SetBlendType(EMovieSceneBlendType::Override);
						SetDefaultsForOverride(SectionItem.Section.Get());
						break;
					case ETLLRN_AnimLayerType::Base:
						SectionItem.Section.Get()->SetBlendType(EMovieSceneBlendType::Absolute);
						break;
					}
				}
			}
		}
	}
}

UTLLRN_TLLRN_AnimLayers::UTLLRN_TLLRN_AnimLayers(class FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

#if WITH_EDITOR
void UTLLRN_TLLRN_AnimLayers::PostEditUndo()
{
	TLLRN_AnimLayerListChangedBroadcast();
}
#endif

TSharedPtr<ISequencer> UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset()
{
	ULevelSequence* LevelSequence = ULevelSequenceEditorBlueprintLibrary::GetCurrentLevelSequence();
	IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSequence, false);
	ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
	TSharedPtr<ISequencer> SequencerPtr = LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;
	return SequencerPtr;
}

void UTLLRN_TLLRN_AnimLayers::AddBaseLayer()
{
	UTLLRN_AnimLayer* TLLRN_AnimLayer = NewObject<UTLLRN_AnimLayer>(this, TEXT("BaseLayer"), RF_Transactional);
	TLLRN_AnimLayer->State.Type = ETLLRN_AnimLayerType::Base;
	TLLRN_AnimLayer->State.bKeyed = ECheckBoxState::Checked;
	TLLRN_TLLRN_AnimLayers.Add(TLLRN_AnimLayer);
}

UTLLRN_TLLRN_AnimLayers* UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(ISequencer* SequencerPtr)
{
	if (!SequencerPtr)
	{
		return nullptr;
	}
	ULevelSequence* LevelSequence = Cast<ULevelSequence>(SequencerPtr->GetFocusedMovieSceneSequence());
	return UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(LevelSequence);
}

UTLLRN_TLLRN_AnimLayers* UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(ULevelSequence* LevelSequence)
{
	if(!LevelSequence)
	{
		return nullptr;
	}
	if (LevelSequence && LevelSequence->GetClass()->ImplementsInterface(UInterface_AssetUserData::StaticClass()))
	{
		if (IInterface_AssetUserData* AssetUserDataInterface = Cast< IInterface_AssetUserData >(LevelSequence))
		{
			UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = AssetUserDataInterface->GetAssetUserData< UTLLRN_TLLRN_AnimLayers >();
			if (!TLLRN_TLLRN_AnimLayers)
			{
				TLLRN_TLLRN_AnimLayers = NewObject<UTLLRN_TLLRN_AnimLayers>(LevelSequence, NAME_None, RF_Public | RF_Transactional);
				AssetUserDataInterface->AddAssetUserData(TLLRN_TLLRN_AnimLayers);
			}
			return TLLRN_TLLRN_AnimLayers;
		}
	}
	return nullptr;
}

int32 UTLLRN_TLLRN_AnimLayers::GetTLLRN_AnimLayerIndex(UTLLRN_AnimLayer* InTLLRN_AnimLayer) const
{
	if (InTLLRN_AnimLayer != nullptr)
	{
		return TLLRN_TLLRN_AnimLayers.Find(InTLLRN_AnimLayer);
	}
	return INDEX_NONE;
}

bool UTLLRN_TLLRN_AnimLayers::DeleteTLLRN_AnimLayer(ISequencer* SequencerPtr, int32 Index)
{
	if (Index >= 1 && Index < TLLRN_TLLRN_AnimLayers.Num()) //don't delete base
	{
		if (UTLLRN_AnimLayer* TLLRN_AnimLayer = TLLRN_TLLRN_AnimLayers[Index])
		{
			const FScopedTransaction Transaction(LOCTEXT("DeleteTLLRN_AnimLayer_Transaction", "Delete Anim Layer"), !GIsTransacting);
			Modify();
			for (const TPair<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayer->TLLRN_AnimLayerItems)
			{
				if (Pair.Key != nullptr)
				{
					for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
					{
						if (SectionItem.Section.IsValid())
						{
							if (UMovieSceneTrack* Track = SectionItem.Section->GetTypedOuter<UMovieSceneTrack>())
							{
								if (Track->GetAllSections().Find(SectionItem.Section.Get()) != 0)
								{
									Track->Modify();
									Track->RemoveSection(*(SectionItem.Section.Get()));
								}
							}
						}
					}
				}
			}
			TLLRN_TLLRN_AnimLayers.RemoveAt(Index);
			SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemRemoved);
		}
		if (UTLLRN_AnimLayer* BaseTLLRN_AnimLayer = TLLRN_TLLRN_AnimLayers[0]) //set base as keyed
		{
			BaseTLLRN_AnimLayer->SetKeyed();
		}
		TLLRN_AnimLayerListChangedBroadcast();
	}
	else
	{
		return false;
	}
	return true;
}

static void CopySectionIntoAnother(UMovieSceneSection* ToSection, UMovieSceneSection* FromSection)
{
	const FFrameNumber Min = TNumericLimits<FFrameNumber>::Lowest();
	const FFrameNumber Max = TNumericLimits<FFrameNumber>::Max();
	TRange<FFrameNumber> Range(Min, Max);
	TArray<UMovieSceneSection*> AbsoluteSections;
	TArray<UMovieSceneSection*> AdditiveSections;

	AdditiveSections.Add(ToSection);
	AdditiveSections.Add(FromSection);
	

	FMovieSceneChannelProxy& ChannelProxy = ToSection->GetChannelProxy();
	for (const FMovieSceneChannelEntry& Entry : ToSection->GetChannelProxy().GetAllEntries())
	{
		const FName ChannelTypeName = Entry.GetChannelTypeName();

		if (ChannelTypeName == FMovieSceneFloatChannel::StaticStruct()->GetFName())
		{
			TArrayView<FMovieSceneFloatChannel*> BaseFloatChannels = ToSection->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
			const int StartIndex = 0;
			const int EndIndex = BaseFloatChannels.Num() - 1;
			MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(ToSection, AbsoluteSections, AdditiveSections, 
				StartIndex, EndIndex, Range);
		}
		else if (ChannelTypeName == FMovieSceneDoubleChannel::StaticStruct()->GetFName())
		{
			TArrayView<FMovieSceneDoubleChannel*> BaseDoubleChannels = ToSection->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
			const int StartIndex = 0;
			const int EndIndex = BaseDoubleChannels.Num() - 1;
			MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(ToSection, AbsoluteSections, AdditiveSections, 
				StartIndex, EndIndex, Range);
		}
	}
}

int32 UTLLRN_TLLRN_AnimLayers::DuplicateTLLRN_AnimLayer(ISequencer* SequencerPtr, int32 Index)
{
	int32 NewIndex = INDEX_NONE;
	if (Index >= 1 && Index < TLLRN_TLLRN_AnimLayers.Num()) //don't duplicate base
	{
		if (UTLLRN_AnimLayer* ExistingTLLRN_AnimLayer = TLLRN_TLLRN_AnimLayers[Index])
		{
			if (ExistingTLLRN_AnimLayer->TLLRN_AnimLayerItems.Num() == 0)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Anim Layers: Can not duplicate empty layer"));
				return INDEX_NONE;
			}
			const FScopedTransaction Transaction(LOCTEXT("DuplicateTLLRN_AnimLayer_Transaction", "Duplicate Anim Layer"), !GIsTransacting);
			Modify();
			UTLLRN_AnimLayer* NewTLLRN_AnimLayer = NewObject<UTLLRN_AnimLayer>(this, NAME_None, RF_Transactional);
			NewTLLRN_AnimLayer->SetType(ExistingTLLRN_AnimLayer->GetType());
			bool bItemAdded = false;
			for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : ExistingTLLRN_AnimLayer->TLLRN_AnimLayerItems)
			{
				if (Pair.Key != nullptr)
				{
					for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
					{
						if (SectionItem.Section.IsValid())
						{
							if (UMovieSceneTrack* Track = SectionItem.Section->GetTypedOuter<UMovieSceneTrack>())
							{
								Track->Modify();
								if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionItem.Section.Get()))
								{
									if (UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(Pair.Key))
									{
										FTLLRN_AnimLayerItem TLLRN_AnimLayerItem;
										FTLLRN_AnimLayerSectionItem NewSectionItem;
										NewSectionItem.TLLRN_AnimLayerSet.BoundObject = TLLRN_ControlRig;
										NewSectionItem.TLLRN_AnimLayerSet = SectionItem.TLLRN_AnimLayerSet;
										// Add a new section that starts and ends at the same time
										if (UMovieSceneTLLRN_ControlRigParameterTrack* CRTack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track))
										{
											TGuardValue<bool> GuardSetSection(CRTack->bSetSectionToKeyPerControl, false);
											if (UMovieSceneTLLRN_ControlRigParameterSection* NewSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Track->CreateNewSection()))
											{
												if (bItemAdded == false)
												{
													NewTLLRN_AnimLayer->State.Weight = 1.0;
													NewTLLRN_AnimLayer->State.Type = (ETLLRN_AnimLayerType::Additive);
													bItemAdded = true;
												}
												ensureAlwaysMsgf(NewSection->HasAnyFlags(RF_Transactional), TEXT("CreateNewSection must return an instance with RF_Transactional set! (pass RF_Transactional to NewObject)"));
												NewSection->SetFlags(RF_Transactional);
												NewSection->SetTransformMask(FMovieSceneTransformMask{ EMovieSceneTransformChannel::All });
												FMovieSceneFloatChannel* FloatChannel = &NewSection->Weight;
												NewSectionItem.Section = NewSection;
												TLLRN_AnimLayerItem.SectionItems.Add(NewSectionItem);
												NewTLLRN_AnimLayer->TLLRN_AnimLayerItems.Add(TLLRN_ControlRig, TLLRN_AnimLayerItem);
												SetUpSectionDefaults(SequencerPtr, NewTLLRN_AnimLayer, Track, NewSection, FloatChannel);
												NewSection->SetBlendType(Section->GetBlendType().Get());
												TArray<FName> ControlNames;
												NewSectionItem.TLLRN_AnimLayerSet.Names.GetKeys(ControlNames);
												SetUpTLLRN_ControlRigSection(NewSection, ControlNames);
												//current copy keys
												CopySectionIntoAnother(NewSection, Section);
											}
										}
									}
								}
								else
								{
									FTLLRN_AnimLayerItem TLLRN_AnimLayerItem;
									FTLLRN_AnimLayerSectionItem NewSectionItem;
									NewSectionItem.TLLRN_AnimLayerSet.BoundObject = Pair.Key;
									if (UMovieSceneSection* NewSection = Track->CreateNewSection())
									{
										if (bItemAdded == false)
										{
											NewTLLRN_AnimLayer->State.Weight = 1.0;
											NewTLLRN_AnimLayer->State.Type = (ETLLRN_AnimLayerType::Additive);
											bItemAdded = true;
										}

										ensureAlwaysMsgf(NewSection->HasAnyFlags(RF_Transactional), TEXT("CreateNewSection must return an instance with RF_Transactional set! (pass RF_Transactional to NewObject)"));
										NewSection->SetFlags(RF_Transactional);
										FMovieSceneFloatChannel* FloatChannel = nullptr;
										if (UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(NewSection))
										{
											TransformSection->SetMask(FMovieSceneTransformMask{ EMovieSceneTransformChannel::All });
											FloatChannel = TransformSection->GetWeightChannel();
										}
										NewSectionItem.Section = NewSection;
										TLLRN_AnimLayerItem.SectionItems.Add(NewSectionItem);
										NewTLLRN_AnimLayer->TLLRN_AnimLayerItems.Add(Pair.Key, TLLRN_AnimLayerItem);
										SetUpSectionDefaults(SequencerPtr, NewTLLRN_AnimLayer, Track, NewSection, FloatChannel);
										NewSection->SetBlendType(SectionItem.Section->GetBlendType().Get());
										//current copy keys
										CopySectionIntoAnother(NewSection, SectionItem.Section.Get());
									}
								}
							}
						}
					}
				}
			}
			if (bItemAdded)
			{
				FString ExistingName = ExistingTLLRN_AnimLayer->GetName().ToString();
				FString NewLayerName = FString::Printf(TEXT("%s_Duplicate"), *ExistingName);

				FText LayerText;
				LayerText = LayerText.FromString(NewLayerName);
				NewTLLRN_AnimLayer->SetName(LayerText); //need items/sections to be added so we can change their track row names
				NewIndex = TLLRN_TLLRN_AnimLayers.Add(NewTLLRN_AnimLayer);
				NewTLLRN_AnimLayer->SetKeyed();
				TLLRN_AnimLayerListChangedBroadcast();
				// no need to since it's a dup SetUpBaseLayerSections();
			}
		}
	}
	SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	return NewIndex;
}

static void AddNamesToMask(FTLLRN_AnimLayerSectionItem* Owner, UMovieSceneTLLRN_ControlRigParameterSection* CRSection,const FTLLRN_AnimLayerSelectionSet& NewSet)
{
	bool bNameAdded = false;
	for (TPair<FName, FTLLRN_AnimLayerPropertyAndChannels> NameAndChannels : NewSet.Names)
	{
		if (Owner->TLLRN_AnimLayerSet.Names.Contains(NameAndChannels.Key) == false)
		{
			bNameAdded = true;
			Owner->TLLRN_AnimLayerSet.Names.Add(NameAndChannels);
		}
	}
	if (bNameAdded)
	{
		TArray<FName> AllControls;
		Owner->TLLRN_AnimLayerSet.Names.GenerateKeyArray(AllControls);
		UTLLRN_TLLRN_AnimLayers::SetUpTLLRN_ControlRigSection(CRSection, AllControls);
	}
}

FTLLRN_AnimLayerSectionItem* FTLLRN_AnimLayerItem::FindMatchingSectionItem(UMovieSceneSection* InMovieSceneSection)
{
	if (InMovieSceneSection)
	{
		for (FTLLRN_AnimLayerSectionItem& CurrentItem : SectionItems)
		{
			if (CurrentItem.Section.IsValid())
			{
				UMovieSceneTrack* InTrack = InMovieSceneSection->GetTypedOuter<UMovieSceneTrack>();
				UMovieSceneTrack* CurrentTrack = CurrentItem.Section->GetTypedOuter<UMovieSceneTrack>();
				if (CurrentTrack && InTrack && CurrentTrack == InTrack)
				{
					return &CurrentItem;
				}
			}
		}
	}
	return nullptr;
}

template<typename ChannelType>
void AddKeyToChannel(ChannelType* Channel, EMovieSceneKeyInterpolation DefaultInterpolation, const FFrameNumber& FrameNumber, double Value)
{
	switch (DefaultInterpolation)
	{
	case EMovieSceneKeyInterpolation::Linear:
		Channel->AddLinearKey(FrameNumber, Value);
		break;
	case EMovieSceneKeyInterpolation::Constant:
		Channel->AddConstantKey(FrameNumber, Value);
		break;
	case  EMovieSceneKeyInterpolation::Auto:
		Channel->AddCubicKey(FrameNumber, Value, ERichCurveTangentMode::RCTM_Auto);
		break;
	case  EMovieSceneKeyInterpolation::SmartAuto:
	default:
		Channel->AddCubicKey(FrameNumber, Value, ERichCurveTangentMode::RCTM_SmartAuto);
		break;
	}
}

bool UTLLRN_TLLRN_AnimLayers::SetPassthroughKey(ISequencer* InSequencer, int32 InIndex)
{
	if (InSequencer == nullptr || InIndex <= 0 || InIndex >= TLLRN_TLLRN_AnimLayers.Num())
	{
		return false;
	}
	FFrameNumber FrameNumber = InSequencer->GetLocalTime().Time.GetFrame();
	EMovieSceneKeyInterpolation DefaultInterpolation = InSequencer->GetKeyInterpolation();
	if (UTLLRN_AnimLayer* TLLRN_AnimLayer = TLLRN_TLLRN_AnimLayers[InIndex].Get())
	{
		for (TPair<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayer->TLLRN_AnimLayerItems)
		{
			if (Pair.Key != nullptr)
			{
				for (FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
				{
					if (SectionItem.Section.IsValid())
					{
						if (UMovieSceneTrack* OwnerTrack = SectionItem.Section->GetTypedOuter<UMovieSceneTrack>())
						{
							TArray<UMovieSceneSection*> TrackSections = OwnerTrack->GetAllSections();
							int32 SectionIndex = TrackSections.Find((SectionItem.Section.Get()));
							if (SectionIndex != INDEX_NONE)
							{
								const FScopedTransaction Transaction(LOCTEXT("SetPassthroughKey_Transaction", "Set Passthrough Key"), !GIsTransacting);
								SectionItem.Section->Modify();
								TrackSections.SetNum(SectionIndex); //this will gives us up to the section 
								TArray<UMovieSceneSection*> Sections;
								TArray<UMovieSceneSection*> AbsoluteSections;
								MovieSceneToolHelpers::SplitSectionsByBlendType(EMovieSceneBlendType::Absolute, TrackSections, Sections, AbsoluteSections);
								TArrayView<FMovieSceneFloatChannel*> BaseFloatChannels = SectionItem.Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
								TArrayView<FMovieSceneDoubleChannel*> BaseDoubleChannels = SectionItem.Section->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
								if (BaseDoubleChannels.Num() > 0)
								{
									if (SectionItem.Section->GetBlendType().IsValid() &&
										SectionItem.Section->GetBlendType() == EMovieSceneBlendType::Additive)
									{
										const double Value = 0.0;
										for (FMovieSceneDoubleChannel* DoubleChannel : BaseDoubleChannels)
										{
											AddKeyToChannel(DoubleChannel, FrameNumber, Value, DefaultInterpolation);
										}
									}
									else if (SectionItem.Section->GetBlendType().IsValid() &&
										SectionItem.Section->GetBlendType() == EMovieSceneBlendType::Override)
									{
										int32 NumChannels = BaseDoubleChannels.Num();
										const int32 StartIndex = 0;
										const int32 EndIndex = NumChannels - 1;
										TArray<double> ChannelValues = MovieSceneToolHelpers::GetChannelValues<FMovieSceneDoubleChannel,
											double>(StartIndex, EndIndex, Sections,AbsoluteSections,FrameNumber);
										for (int32 Index = 0; Index < NumChannels; ++Index)
										{
											FMovieSceneDoubleChannel* DoubleChannel = BaseDoubleChannels[Index];
											const double Value = ChannelValues[Index];
											AddKeyToChannel(DoubleChannel, FrameNumber, Value, DefaultInterpolation);
										}
									}
								}
								else if (BaseFloatChannels.Num() > 0)
								{
									int32 NumChannels = SectionItem.Section->IsA<UMovieSceneTLLRN_ControlRigParameterSection>() ?
										BaseFloatChannels.Num() - 1 : BaseFloatChannels.Num();// skip weight if CR section
									const int32 StartIndex = 0;
									const int32 EndIndex = NumChannels - 1;
									if (SectionItem.Section->GetBlendType().IsValid() &&
										SectionItem.Section->GetBlendType() == EMovieSceneBlendType::Additive)
									{
										const float Value = 0.0;
										for (int32 Index = 0; Index < NumChannels; ++Index)
										{
											FMovieSceneFloatChannel* FloatChannel = BaseFloatChannels[Index];
											AddKeyToChannel(FloatChannel, FrameNumber, Value, DefaultInterpolation);
										}
									}
									else if (SectionItem.Section->GetBlendType().IsValid() &&
										SectionItem.Section->GetBlendType() == EMovieSceneBlendType::Override)
									{
										TArray<float> ChannelValues = MovieSceneToolHelpers::GetChannelValues<FMovieSceneFloatChannel,
											float>(StartIndex,EndIndex, Sections,AbsoluteSections, FrameNumber);
										for (int32 Index = 0; Index < NumChannels; ++Index)
										{
											FMovieSceneFloatChannel* FloatChannel = BaseFloatChannels[Index];
											const float Value = ChannelValues[Index];
											AddKeyToChannel(FloatChannel, FrameNumber, Value, DefaultInterpolation);
										}
									}
								}
								InSequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
							}
						}
						else
						{
							return false;
						}
					}
				}
			}
		}
	}
	else
	{
		return false;
	}
	return true;
}


static void MergeTLLRN_ControlRigSections(UMovieSceneTLLRN_ControlRigParameterSection* BaseSection, UMovieSceneTLLRN_ControlRigParameterSection* Section, const TRange<FFrameNumber>& Range)
{
	//disable section if no control in the section and override, need to make sure we skip it's eval or it will incorrectly
	//override when merging
	auto ShouldDisableSection = [](UMovieSceneTLLRN_ControlRigParameterSection* CRSection, FTLLRN_RigControlElement* ControlElement)
	{
		return (CRSection->GetControlNameMask(ControlElement->GetFName()) == false &&
				CRSection->GetBlendType().IsValid() && CRSection->GetBlendType() == EMovieSceneBlendType::Override);
	};
	TArrayView<FMovieSceneFloatChannel*> BaseFloatChannels = BaseSection->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
	if (BaseFloatChannels.Num() > 0)
	{
		//need to go through each control and merge that
		TArray<FTLLRN_RigControlElement*> Controls;
		UTLLRN_ControlRig* TLLRN_ControlRig = BaseSection->GetTLLRN_ControlRig();
		if (TLLRN_ControlRig == nullptr)
		{
			return;
		}
		TLLRN_ControlRig->GetControlsInOrder(Controls);
		UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
		if (Hierarchy == nullptr)
		{
			return;
		}

		for (int32 LocalControlIndex = 0; LocalControlIndex < Controls.Num(); ++LocalControlIndex)
		{
			FTLLRN_RigControlElement* ControlElement = Controls[LocalControlIndex];
			check(ControlElement);
			if (!Hierarchy->IsAnimatable(ControlElement))
			{
				continue;
			}

			if (FTLLRN_ChannelMapInfo* pChannelIndex = BaseSection->ControlChannelMap.Find(ControlElement->GetFName()))
			{
				const int32 ChannelIndex = pChannelIndex->ChannelIndex;

				//if section is additive an control not there, skip it. we need to keep overrides since we may be blending over
				//an additive and we want the full override value (with prpoer scale)
				const bool bMaskKeyOut = (Section->GetBlendType().IsValid() && Section->GetBlendType() == EMovieSceneBlendType::Additive 
					&& Section->GetControlNameMask(ControlElement->GetFName()) == false);
				if (bMaskKeyOut)
				{
					continue;
				}
				TOptional<bool> bBaseSectionResetActive;
				TOptional<bool> bSectionResetActive;

				if (ShouldDisableSection(BaseSection, ControlElement))
				{
					bBaseSectionResetActive = BaseSection->IsActive();
					BaseSection->SetIsActive(false);
				}

				if (ShouldDisableSection(Section, ControlElement))
				{
					bSectionResetActive = Section->IsActive();
					Section->SetIsActive(false);
				}

				switch (ControlElement->Settings.ControlType)
				{

					case ETLLRN_RigControlType::Float:
					case ETLLRN_RigControlType::ScaleFloat:
					{
						int32 StartIndex = ChannelIndex;
						int32 EndIndex = ChannelIndex;
						MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
							Section, StartIndex, EndIndex, Range);
						break;
					}
					case ETLLRN_RigControlType::Vector2D:
					{
						int32 StartIndex = ChannelIndex;
						int32 EndIndex = ChannelIndex + 1;
						MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
							Section, StartIndex, EndIndex, Range);
						break;
					}
					case ETLLRN_RigControlType::Position:
					case ETLLRN_RigControlType::Scale:
					case ETLLRN_RigControlType::Rotator:
					{
						int32 StartIndex = ChannelIndex;
						int32 EndIndex = ChannelIndex + 2;
						MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
							Section, StartIndex, EndIndex, Range);
						break;
					}

					case ETLLRN_RigControlType::Transform:
					case ETLLRN_RigControlType::TransformNoScale:
					case ETLLRN_RigControlType::EulerTransform:
					{
						EMovieSceneTransformChannel BaseChannelMask = BaseSection->GetTransformMask().GetChannels();
						EMovieSceneTransformChannel ChannelMask = Section->GetTransformMask().GetChannels();

						const bool bDoAllTransform = EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::AllTransform);
						if (bDoAllTransform)
						{
							if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale)
							{
								int32 StartIndex = ChannelIndex;
								int32 EndIndex = ChannelIndex + 5;
								MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
									Section, StartIndex, EndIndex, Range);
							}
							else
							{
								int32 StartIndex = ChannelIndex;
								int32 EndIndex = ChannelIndex + 8;
								MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
									Section, StartIndex, EndIndex, Range);
							}
						}
						else
						{
							if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::TranslationX)
								 && EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::TranslationX))
							{
								int32 StartIndex = ChannelIndex;
								int32 EndIndex = StartIndex;
								MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
									Section, StartIndex, EndIndex, Range);
							}
							if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::TranslationY)
								&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::TranslationY))
							{
								int32 StartIndex = ChannelIndex + 1;
								int32 EndIndex = StartIndex;
								MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
									Section, StartIndex, EndIndex, Range);
							}
							if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::TranslationZ)
								&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::TranslationZ))
							{
								int32 StartIndex = ChannelIndex + 2;
								int32 EndIndex = StartIndex;
								MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
									Section, StartIndex, EndIndex, Range);
							}
							if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::RotationX)
								&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::RotationX))
							{
								int32 StartIndex = ChannelIndex + 3;
								int32 EndIndex = StartIndex;
								MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
									Section, StartIndex, EndIndex, Range);
							}
							if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::RotationY)
								&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::RotationY))
							{
								int32 StartIndex = ChannelIndex + 4;
								int32 EndIndex = StartIndex;
								MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
									Section, StartIndex, EndIndex, Range);
							}
							if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::RotationZ)
								&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::RotationZ))
							{
								int32 StartIndex = ChannelIndex + 5;
								int32 EndIndex = StartIndex;
								MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
									Section, StartIndex, EndIndex, Range);
							}
							if (ControlElement->Settings.ControlType != ETLLRN_RigControlType::TransformNoScale)
							{
								if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::ScaleX)
									&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::ScaleX))
								{
									int32 StartIndex = ChannelIndex + 6;
									int32 EndIndex = StartIndex;
									MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
										Section, StartIndex, EndIndex, Range);
								}
								if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::ScaleY)
									&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::ScaleY))
								{
									int32 StartIndex = ChannelIndex + 7;
									int32 EndIndex = StartIndex;
									MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
										Section, StartIndex, EndIndex, Range);
								}
								if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::ScaleZ)
									&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::ScaleZ))
								{
									int32 StartIndex = ChannelIndex + 8;
									int32 EndIndex = StartIndex;
									MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
										Section, StartIndex, EndIndex, Range);
								}
							}
						}
						break;
					}
					default:
						break;
				}
				if (bBaseSectionResetActive.IsSet())
				{
					BaseSection->SetIsActive(bBaseSectionResetActive.GetValue());
				}
				if (bSectionResetActive.IsSet())
				{
					Section->SetIsActive(bSectionResetActive.GetValue());
				}
			}
		}
	}
}

static void MergeTransformSections(UMovieScene3DTransformSection* BaseSection, UMovieScene3DTransformSection* Section, const TRange<FFrameNumber>& Range)
{
	if (!BaseSection || !Section)
	{
		return;
	}
	TArrayView<FMovieSceneDoubleChannel*> BaseDoubleChannels = BaseSection->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
	if (BaseDoubleChannels.Num() > 0)
	{
		EMovieSceneTransformChannel BaseChannelMask = BaseSection->GetMask().GetChannels();
		EMovieSceneTransformChannel ChannelMask = Section->GetMask().GetChannels();

		const bool bDoAllTransform = EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::AllTransform);
		if (bDoAllTransform)
		{
			int32 StartIndex = 0;
			int32 EndIndex = BaseDoubleChannels.Num() - 1;
			MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(BaseSection,
				Section, StartIndex, EndIndex, Range);
		}
		else
		{
			const int32 ChannelIndex = 0;
			if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::TranslationX)
				&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::TranslationX))
			{
				int32 StartIndex = ChannelIndex;
				int32 EndIndex = StartIndex;
				MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(BaseSection,
					Section, StartIndex, EndIndex, Range);
			}
			if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::TranslationY)
				&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::TranslationY))
			{
				int32 StartIndex = ChannelIndex + 1;
				int32 EndIndex = StartIndex;
				MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(BaseSection,
					Section, StartIndex, EndIndex, Range);
			}
			if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::TranslationZ)
				&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::TranslationZ))
			{
				int32 StartIndex = ChannelIndex + 2;
				int32 EndIndex = StartIndex;
				MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(BaseSection,
					Section, StartIndex, EndIndex, Range);
			}
			if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::RotationX)
				&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::RotationX))
			{
				int32 StartIndex = ChannelIndex + 3;
				int32 EndIndex = StartIndex;
				MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(BaseSection,
					Section, StartIndex, EndIndex, Range);
			}
			if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::RotationY)
				&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::RotationY))
			{
				int32 StartIndex = ChannelIndex + 4;
				int32 EndIndex = StartIndex;
				MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(BaseSection,
					Section, StartIndex, EndIndex, Range);
			}
			if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::RotationZ)
				&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::RotationZ))
			{
				int32 StartIndex = ChannelIndex +  5;
				int32 EndIndex = StartIndex;
				MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(BaseSection,
					Section, StartIndex, EndIndex, Range);
			}
			if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::ScaleX)
				&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::ScaleX))
			{
				int32 StartIndex = ChannelIndex + 6;
				int32 EndIndex = StartIndex;
				MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
					Section, StartIndex, EndIndex, Range);
			}
			if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::ScaleY)
				&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::ScaleY))
			{
				int32 StartIndex = ChannelIndex + 7;
				int32 EndIndex = StartIndex;
				MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(BaseSection,
					Section, StartIndex, EndIndex, Range);
			}
			if (EnumHasAllFlags(BaseChannelMask, EMovieSceneTransformChannel::ScaleZ)
				&& EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::ScaleZ))
			{
				int32 StartIndex = ChannelIndex + 8;
				int32 EndIndex = StartIndex;
				MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSection,
					Section, StartIndex, EndIndex, Range);
			}
		}
	}
}

bool UTLLRN_TLLRN_AnimLayers::MergeTLLRN_TLLRN_AnimLayers(ISequencer* InSequencer, const TArray<int32>& Indices, const FBakingAnimationKeySettings* InSettings)
{
	if (InSequencer == nullptr)
	{
		return false;
	}
	TArray<UTLLRN_AnimLayer*> LayersToMerge;
	const FFrameNumber Min = InSettings ? InSettings->StartFrame : TNumericLimits<FFrameNumber>::Lowest();
	const FFrameNumber Max = InSettings ? InSettings->EndFrame : TNumericLimits<FFrameNumber>::Max();
	TRange<FFrameNumber> Range(Min, Max);
	FScopedTransaction Transaction(LOCTEXT("Merge Anim Layers", "Merge Anim Layers"), !GIsTransacting);

	TArray<int32> SortedIndices = Indices;
	//we go backwards to the first one
	SortedIndices.Sort([](const int32& Index1, const int32& Index2) {
		return  Index1 > Index2;
		});

	for (int32 Index : SortedIndices)
	{
		if (Index >= 0 && Index < TLLRN_TLLRN_AnimLayers.Num())
		{
			if (UTLLRN_AnimLayer* TLLRN_AnimLayer = TLLRN_TLLRN_AnimLayers[Index].Get())
			{
				LayersToMerge.Add(TLLRN_AnimLayer);
			}
		}
	}
	if (LayersToMerge.Num() < 1)
	{
		return false;
	}
	Modify();

	for (int32 Index = 0; Index < LayersToMerge.Num() -1; ++Index)
	{
		UTLLRN_AnimLayer* BaseLayer = LayersToMerge[Index + 1];
		UTLLRN_AnimLayer* TLLRN_AnimLayer = LayersToMerge[Index];
		BaseLayer->Modify();
		TLLRN_AnimLayer->Modify();
		for (TPair<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayer->TLLRN_AnimLayerItems)
		{
			if (Pair.Key != nullptr)
			{
				for (FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
				{
					if (SectionItem.Section.IsValid())
					{
						FTLLRN_AnimLayerSectionItem* BaseSectionItem = nullptr;
						if (FTLLRN_AnimLayerItem* Owner = BaseLayer->TLLRN_AnimLayerItems.Find(Pair.Key))
						{
							BaseSectionItem = Owner->FindMatchingSectionItem(SectionItem.Section.Get());

						}
						if (BaseSectionItem && BaseSectionItem->Section.IsValid())
						{
							if (SectionItem.Section->IsActive())//active sections merge them
							{
								//if transform or control rig section we need to handle masking
								if (UMovieSceneTLLRN_ControlRigParameterSection* BaseCRSection = Cast< UMovieSceneTLLRN_ControlRigParameterSection>(BaseSectionItem->Section.Get()))
								{
									UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast< UMovieSceneTLLRN_ControlRigParameterSection>(SectionItem.Section.Get());
									MergeTLLRN_ControlRigSections(BaseCRSection, CRSection, Range);
								}
								else if (UMovieScene3DTransformSection* BaseTRSection = Cast< UMovieScene3DTransformSection>(BaseSectionItem->Section.Get()))
								{
									UMovieScene3DTransformSection* TRSection = Cast< UMovieScene3DTransformSection>(SectionItem.Section.Get());
									MergeTransformSections(BaseTRSection, TRSection, Range);
								}
								else
								{
									TArrayView<FMovieSceneFloatChannel*> BaseFloatChannels = BaseSectionItem->Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
									TArrayView<FMovieSceneDoubleChannel*> BaseDoubleChannels = BaseSectionItem->Section->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
									if (BaseDoubleChannels.Num() > 0)
									{
										int32 StartIndex = 0;
										int32 EndIndex = BaseDoubleChannels.Num() - 1;
										MovieSceneToolHelpers::MergeSections<FMovieSceneDoubleChannel>(BaseSectionItem->Section.Get(),
											SectionItem.Section.Get(), StartIndex, EndIndex, Range);

									}
									else if (BaseFloatChannels.Num() > 0)
									{
										int32 StartIndex = 0;
										int32 EndIndex = BaseFloatChannels.Num() - 1;
										MovieSceneToolHelpers::MergeSections<FMovieSceneFloatChannel>(BaseSectionItem->Section.Get(),
											SectionItem.Section.Get(), StartIndex, EndIndex, Range);
									}
								}
							}
							if (BaseLayer != TLLRN_TLLRN_AnimLayers[0]) //if not base layer
							{
								if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(BaseSectionItem->Section))
								{
									if (SectionItem.TLLRN_AnimLayerSet.Names.Num() > 0)
									{
										AddNamesToMask(BaseSectionItem, CRSection, SectionItem.TLLRN_AnimLayerSet);
										if (SortedIndices[0] != 0) //if not base then make sure mask is set up
										{
											TArray<FName> AllControls;
											BaseSectionItem->TLLRN_AnimLayerSet.Names.GenerateKeyArray(AllControls);
											UTLLRN_TLLRN_AnimLayers::SetUpTLLRN_ControlRigSection(CRSection, AllControls);
										}
									}
								}
							}
						}
						else //okay this object doesn't exist in the first layer we are merging into so we need to move it to the other one 
						{
							FTLLRN_AnimLayerItem TLLRN_AnimLayerItem;
							FTLLRN_AnimLayerSectionItem NewSectionItem;
							NewSectionItem.TLLRN_AnimLayerSet.BoundObject = Pair.Key;
							NewSectionItem.TLLRN_AnimLayerSet.Names = SectionItem.TLLRN_AnimLayerSet.Names;
							NewSectionItem.Section = SectionItem.Section;
							if (UMovieSceneNameableTrack* NameableTrack = Cast<UMovieSceneNameableTrack>(SectionItem.Section.Get()->GetTypedOuter< UMovieSceneNameableTrack>()))
							{
								NameableTrack->Modify();
								NameableTrack->SetTrackRowDisplayName(BaseLayer->State.Name, SectionItem.Section.Get()->GetRowIndex());
							}
							TLLRN_AnimLayerItem.SectionItems.Add(NewSectionItem);
							BaseLayer->TLLRN_AnimLayerItems.Add(Pair.Key, TLLRN_AnimLayerItem);
							//since we moved the section over we reset it on the merged so we don't delete it when we remove the layer
							if (FTLLRN_AnimLayerItem* OldTLLRN_AnimLayeritem = TLLRN_AnimLayer->TLLRN_AnimLayerItems.Find(Pair.Key))
							{
								SectionItem.Section.Reset();
							}
						}
					}
				}
			}
		}
		if (TLLRN_AnimLayer->GetType() == ETLLRN_AnimLayerType::Override &&
			(BaseLayer->GetType() == ETLLRN_AnimLayerType::Additive))
		{
			BaseLayer->SetType(ETLLRN_AnimLayerType::Override);
		}
		int32 LayerIndex = GetTLLRN_AnimLayerIndex(TLLRN_AnimLayer);
		if (LayerIndex != INDEX_NONE)
		{
			DeleteTLLRN_AnimLayer(InSequencer, LayerIndex);
		}
	}
	UTLLRN_AnimLayer* BaseLayer = LayersToMerge[LayersToMerge.Num() - 1];
	if (BaseLayer != TLLRN_TLLRN_AnimLayers[0]) //if not base layer
	{
		FString Merged(TEXT("Merged"));
		FString ExistingName = BaseLayer->GetName().ToString();
		if (ExistingName.Contains(Merged) == false)
		{
			FString NewLayerName = FString::Printf(TEXT("%s_Merged"), *ExistingName);
			FText LayerText;
			LayerText = LayerText.FromString(NewLayerName);
			BaseLayer->SetName(LayerText); //need items/sections to be added so we can change their track row names
		}
	}
	else
	{
		SetUpBaseLayerSections(); //if it is the base reset it
	}
	InSequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	return true;
}

void UTLLRN_TLLRN_AnimLayers::TLLRN_AnimLayerListChangedBroadcast()
{
	OnTLLRN_AnimLayerListChanged.Broadcast(this);
}

TArray<UMovieSceneSection*> UTLLRN_TLLRN_AnimLayers::GetSelectedLayerSections() const
{
	TArray<UMovieSceneSection*> Sections;
	for (const UTLLRN_AnimLayer* TLLRN_AnimLayer : TLLRN_TLLRN_AnimLayers)
	{
		if (TLLRN_AnimLayer && TLLRN_AnimLayer->bIsSelectedInList)
		{
			for (const TPair<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayer->TLLRN_AnimLayerItems)
			{
				if (Pair.Key != nullptr)
				{
					for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
					{
						if (SectionItem.Section.IsValid() )
						{
							Sections.Add(SectionItem.Section.Get());
						}
					}
				}
			}
		}
	}
	return Sections;
}

bool UTLLRN_TLLRN_AnimLayers::IsTrackOnSelectedLayer(const UMovieSceneTrack* InTrack)const
{
	TSet<UMovieSceneTrack*> CurrentTracks;
	for (const UTLLRN_AnimLayer* TLLRN_AnimLayer : TLLRN_TLLRN_AnimLayers)
	{
		if (TLLRN_AnimLayer && TLLRN_AnimLayer->bIsSelectedInList)
		{
			for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayer->TLLRN_AnimLayerItems)
			{
				if (Pair.Key != nullptr)
				{
					for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
					{
						if (SectionItem.Section.IsValid())
						{
							UMovieSceneTrack* OwnerTrack = SectionItem.Section->GetTypedOuter<UMovieSceneTrack>();
							if (OwnerTrack == InTrack)
							{
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}

int32 UTLLRN_TLLRN_AnimLayers::AddTLLRN_AnimLayerFromSelection(ISequencer* SequencerPtr)
{
	int32 NewIndex = INDEX_NONE;
	//wrap scoped transaction since it can deselect control rigs
	TArray<FTLLRN_ControlRigAndControlsAndTrack> SelectedCRs;
	TArray<FObjectAndTrack> SelectedBoundObjects; 
	{
		const FScopedTransaction Transaction(LOCTEXT("AddTLLRN_AnimLayer_Transaction", "Add Anim Layer"), !GIsTransacting);
		Modify();
		if (TLLRN_TLLRN_AnimLayers.Num() == 0)
		{
			AddBaseLayer();
		}
		UTLLRN_AnimLayer* TLLRN_AnimLayer = NewObject<UTLLRN_AnimLayer>(this, NAME_None, RF_Transactional);

		GetSelectedTLLRN_ControlRigsAndBoundObjects(SequencerPtr, SelectedCRs, SelectedBoundObjects);

		if (SelectedCRs.Num() <= 0 && SelectedBoundObjects.Num() <= 0)
		{
			FString LayerName = FString::Printf(TEXT("Empty Layer %d"), TLLRN_TLLRN_AnimLayers.Num());
			FText LayerText;
			LayerText = LayerText.FromString(LayerName);
			TLLRN_AnimLayer->SetName(LayerText); //need items/sections to be added so we can change their track row names
			TLLRN_AnimLayer->State.Weight = 1.0;
			TLLRN_AnimLayer->State.Type = (ETLLRN_AnimLayerType::Additive);
			int32 Index = TLLRN_TLLRN_AnimLayers.Add(TLLRN_AnimLayer);
			TLLRN_AnimLayerListChangedBroadcast();

			return Index;
		}

		bool bItemAdded = false;

		for (FTLLRN_ControlRigAndControlsAndTrack& CRControls : SelectedCRs)
		{
			Modify();
			FTLLRN_AnimLayerItem TLLRN_AnimLayerItem;
			FTLLRN_AnimLayerSectionItem SectionItem;
			SectionItem.TLLRN_AnimLayerSet.BoundObject = CRControls.TLLRN_ControlRig;
			for (FName& ControlName : CRControls.Controls)
			{
				FTLLRN_AnimLayerPropertyAndChannels Channels;
				Channels.Name = ControlName;
				Channels.Channels = (uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform;
				SectionItem.TLLRN_AnimLayerSet.Names.Add(ControlName, Channels);

			}
			CRControls.Track->Modify();
			// Add a new section that starts and ends at the same time
			TGuardValue<bool> GuardSetSection(CRControls.Track->bSetSectionToKeyPerControl, false);
			if (UMovieSceneTLLRN_ControlRigParameterSection* NewSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(CRControls.Track->CreateNewSection()))
			{
				if (bItemAdded == false)
				{
					TLLRN_AnimLayer->State.Weight = 1.0;
					TLLRN_AnimLayer->State.Type = (ETLLRN_AnimLayerType::Additive);
					bItemAdded = true;
				}

				ensureAlwaysMsgf(NewSection->HasAnyFlags(RF_Transactional), TEXT("CreateNewSection must return an instance with RF_Transactional set! (pass RF_Transactional to NewObject)"));
				NewSection->SetFlags(RF_Transactional);
				NewSection->SetTransformMask(FMovieSceneTransformMask{ EMovieSceneTransformChannel::All });
				FMovieSceneFloatChannel* FloatChannel = &NewSection->Weight;
				SectionItem.Section = NewSection;
				TLLRN_AnimLayerItem.SectionItems.Add(SectionItem);
				TLLRN_AnimLayer->TLLRN_AnimLayerItems.Add(CRControls.TLLRN_ControlRig, TLLRN_AnimLayerItem);
				SetUpSectionDefaults(SequencerPtr, TLLRN_AnimLayer, CRControls.Track, NewSection, FloatChannel);
				SetUpTLLRN_ControlRigSection(NewSection, CRControls.Controls);

			}
		}
		for (FObjectAndTrack& ObjectAndTrack : SelectedBoundObjects)
		{
			Modify();

			FTLLRN_AnimLayerItem& TLLRN_AnimLayerItem = TLLRN_AnimLayer->TLLRN_AnimLayerItems.FindOrAdd(ObjectAndTrack.BoundObject);
			FTLLRN_AnimLayerSectionItem SectionItem;
			SectionItem.TLLRN_AnimLayerSet.BoundObject = ObjectAndTrack.BoundObject;
			/*
			for (FName& ControlName : SelectedControls)
			{
				FTLLRN_AnimLayerPropertyAndChannels Channels;
				Channels.Name = ControlName;
				Channels.Channels = (uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform;
				TLLRN_AnimLayerItem.TLLRN_AnimLayerSet.Names.Add(ControlName, Channels);

			}*/

			// Add a new section that starts and ends at the same time
			ObjectAndTrack.Track->Modify();
			if (UMovieSceneSection* NewSection = ObjectAndTrack.Track->CreateNewSection())
			{
				if (bItemAdded == false)
				{
					TLLRN_AnimLayer->State.Weight = 1.0;
					TLLRN_AnimLayer->State.Type = (ETLLRN_AnimLayerType::Additive);
					bItemAdded = true;
				}

				ensureAlwaysMsgf(NewSection->HasAnyFlags(RF_Transactional), TEXT("CreateNewSection must return an instance with RF_Transactional set! (pass RF_Transactional to NewObject)"));
				NewSection->SetFlags(RF_Transactional);
				SectionItem.Section = NewSection;
				FMovieSceneFloatChannel* FloatChannel = nullptr;
				if (UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(NewSection))
				{
					TransformSection->SetMask(FMovieSceneTransformMask{ EMovieSceneTransformChannel::All });
					FloatChannel = TransformSection->GetWeightChannel();
				}
				TLLRN_AnimLayerItem.SectionItems.Add(SectionItem);
				SetUpSectionDefaults(SequencerPtr, TLLRN_AnimLayer, ObjectAndTrack.Track, NewSection, FloatChannel);
			}
		}

		if (bItemAdded)
		{
			FString LayerName = FString::Printf(TEXT("Anim Layer %d"), TLLRN_TLLRN_AnimLayers.Num());
			FText LayerText;
			LayerText = LayerText.FromString(LayerName);
			TLLRN_AnimLayer->SetName(LayerText); //need items/sections to be added so we can change their track row names
			int32 Index = TLLRN_TLLRN_AnimLayers.Add(TLLRN_AnimLayer);
			SetUpBaseLayerSections();
			TLLRN_AnimLayer->SetKeyed();
			SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);

			NewIndex = Index;;
			TLLRN_AnimLayerListChangedBroadcast();
		}
	}
	//may need to reselect controls here
	for (FTLLRN_ControlRigAndControlsAndTrack& CRControls : SelectedCRs)
	{
		for (const FName& Control : CRControls.Controls)
		{
			if (CRControls.TLLRN_ControlRig->IsControlSelected(Control) == false)
			{
				CRControls.TLLRN_ControlRig->SelectControl(Control, true);
			}
		}
	}
	return NewIndex;
}

static void AddSectionToTLLRN_AnimLayerItem(const FTLLRN_AnimLayerSelectionSet& CurrentTLLRN_AnimLayerSet, FTLLRN_AnimLayerItem* TLLRN_AnimLayerItem, UObject* BoundObject, UMovieSceneSection* InSection)
{
	FTLLRN_AnimLayerSectionItem NewSectionItem;
	NewSectionItem.TLLRN_AnimLayerSet.BoundObject = BoundObject;
	NewSectionItem.Section = InSection;
	if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(NewSectionItem.Section))
	{
		FMovieSceneFloatChannel* FloatChannel = &CRSection->Weight;
		FloatChannel->SetDefault(1.0f);
		CRSection->SetTransformMask(CRSection->GetTransformMask().GetChannels() | EMovieSceneTransformChannel::Weight);
	}
	else if (UMovieScene3DTransformSection* Section = Cast<UMovieScene3DTransformSection>(NewSectionItem.Section))
	{
		FMovieSceneFloatChannel* FloatChannel = Section->GetWeightChannel();
		FloatChannel->SetDefault(1.0f);
		Section->SetMask(Section->GetMask().GetChannels() | EMovieSceneTransformChannel::Weight);
	}
	for (const TPair<FName, FTLLRN_AnimLayerPropertyAndChannels>& Set : CurrentTLLRN_AnimLayerSet.Names)
	{
		FTLLRN_AnimLayerPropertyAndChannels Channels;
		Channels.Name = Set.Value.Name;
		Channels.Channels = Set.Value.Channels;
		NewSectionItem.TLLRN_AnimLayerSet.Names.Add(Set.Key, Channels);
	}
	TLLRN_AnimLayerItem->SectionItems.Add(NewSectionItem);
}

void UTLLRN_TLLRN_AnimLayers::SetUpBaseLayerSections()
{
	if (TLLRN_TLLRN_AnimLayers.Num() > 0)
	{
		if (UTLLRN_AnimLayer* BaseTLLRN_AnimLayer = TLLRN_TLLRN_AnimLayers[0])
		{
			BaseTLLRN_AnimLayer->Modify();
			BaseTLLRN_AnimLayer->TLLRN_AnimLayerItems.Reset(); //clear it out
			for (int32 Index = 1; Index < TLLRN_TLLRN_AnimLayers.Num(); ++Index)
			{
				UTLLRN_AnimLayer* TLLRN_AnimLayer = TLLRN_TLLRN_AnimLayers[Index];
				TLLRN_AnimLayer->Modify();
				for (const TPair<TWeakObjectPtr<UObject>,FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayer->TLLRN_AnimLayerItems)
				{
					if (Pair.Key != nullptr)
					{
						for (const FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
						{
							if (SectionItem.Section.IsValid())
							{
								if (UMovieSceneTrack* Track = SectionItem.Section->GetTypedOuter<UMovieSceneTrack>())
								{
									const TArray<UMovieSceneSection*>& Sections = Track->GetAllSections();
									if (Sections.Num() > 1 && Sections[0]->GetBlendType().IsValid() && Sections[0]->GetBlendType() == EMovieSceneBlendType::Absolute)
									{
										if (FTLLRN_AnimLayerItem* Existing = BaseTLLRN_AnimLayer->TLLRN_AnimLayerItems.Find(Pair.Key))
										{
											if (Pair.Key->IsA<UTLLRN_ControlRig>()) //if control rig just merge over control names
											{
												for (const TPair<FName, FTLLRN_AnimLayerPropertyAndChannels>& Set : SectionItem.TLLRN_AnimLayerSet.Names)
												{
													for (FTLLRN_AnimLayerSectionItem& ExistingSectionItem : Existing->SectionItems)
													{
														if (ExistingSectionItem.TLLRN_AnimLayerSet.Names.Find(Set.Key) == nullptr)
														{
															FTLLRN_AnimLayerPropertyAndChannels Channels;
															Channels.Name = Set.Value.Name;
															Channels.Channels = Set.Value.Channels;
															ExistingSectionItem.TLLRN_AnimLayerSet.Names.Add(Set.Key, Channels);
														}
													}
												}
											}
											else
											{
												AddSectionToTLLRN_AnimLayerItem(SectionItem.TLLRN_AnimLayerSet,Existing, Pair.Key.Get(), Sections[0]);
											}
										}
										else
										{
											FTLLRN_AnimLayerItem TLLRN_AnimLayerItem;
											AddSectionToTLLRN_AnimLayerItem(SectionItem.TLLRN_AnimLayerSet, &TLLRN_AnimLayerItem, Pair.Key.Get(), Sections[0]);
											BaseTLLRN_AnimLayer->TLLRN_AnimLayerItems.Add(Pair.Key, TLLRN_AnimLayerItem);
										}
									}
								}
							}
						}
					}
				}
			}
			//set the name will set it on all base sections
			FText LayerText = BaseTLLRN_AnimLayer->State.Name;
			BaseTLLRN_AnimLayer->SetName(LayerText);
		}
	}
	else
	{
		AddBaseLayer();
	}
}

void UTLLRN_TLLRN_AnimLayers::GetTLLRN_AnimLayerStates(TArray<FTLLRN_AnimLayerState>& OutStates)
{
	OutStates.Reset();
	for (UTLLRN_AnimLayer* TLLRN_AnimLayer : TLLRN_TLLRN_AnimLayers)
	{
		if (TLLRN_AnimLayer)
		{
			OutStates.Emplace(TLLRN_AnimLayer->State);
		}
	}
}

void UTLLRN_TLLRN_AnimLayers::SetUpSectionDefaults(ISequencer* SequencerPtr, UTLLRN_AnimLayer* Layer, UMovieSceneTrack* Track, UMovieSceneSection* NewSection, FMovieSceneFloatChannel* WeightChannel)
{
	int32 OverlapPriority = 0;
	TMap<int32, int32> NewToOldRowIndices;
	int32 RowIndex = Track->GetMaxRowIndex() + 1;
	for (UMovieSceneSection* Section : Track->GetAllSections())
	{
		OverlapPriority = FMath::Max(Section->GetOverlapPriority() + 1, OverlapPriority);

		// Move existing sections on the same row or beyond so that they don't overlap with the new section
		if (Section != NewSection && Section->GetRowIndex() >= RowIndex)
		{
			int32 OldRowIndex = Section->GetRowIndex();
			int32 NewRowIndex = Section->GetRowIndex() + 1;
			NewToOldRowIndices.FindOrAdd(NewRowIndex, OldRowIndex);
			Section->Modify();
			Section->SetRowIndex(NewRowIndex);
		}
	}

	Track->Modify();

	Track->OnRowIndicesChanged(NewToOldRowIndices);
	NewSection->SetRange(TRange<FFrameNumber>::All());
	
	NewSection->SetOverlapPriority(OverlapPriority);
	NewSection->SetRowIndex(RowIndex);

	Track->AddSection(*NewSection);
	Track->UpdateEasing();

	if (UMovieSceneNameableTrack* NameableTrack = Cast<UMovieSceneNameableTrack>(Track))
	{
		NameableTrack->SetTrackRowDisplayName(Layer->GetName(), RowIndex);
	}

	switch (Layer->State.Type)
	{
	case ETLLRN_AnimLayerType::Additive:
		NewSection->SetBlendType(EMovieSceneBlendType::Additive);
		break;
	case ETLLRN_AnimLayerType::Override:
		NewSection->SetBlendType(EMovieSceneBlendType::Override);
		SetDefaultsForOverride(NewSection);
		break;
	case ETLLRN_AnimLayerType::Base:
		NewSection->SetBlendType(EMovieSceneBlendType::Absolute);
		break;
	}
	if (WeightChannel)
	{
		WeightChannel->SetDefault(1.0f);
	}
}

void UTLLRN_TLLRN_AnimLayers::SetUpTLLRN_ControlRigSection(UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection, TArray<FName>& ControlNames)
{
	if (ParameterSection)
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = ParameterSection ? ParameterSection->GetTLLRN_ControlRig() : nullptr;

		if (ParameterSection && TLLRN_ControlRig)
		{
			ParameterSection->Modify();
			ParameterSection->FillControlNameMask(false);

			TArray<FTLLRN_RigControlElement*> Controls;
			TLLRN_ControlRig->GetControlsInOrder(Controls);
			for (const FName& TLLRN_RigName : ControlNames)
			{
				ParameterSection->SetControlNameMask(TLLRN_RigName, true);
			}
		}
	}
}

struct FKeyInterval
{
	FFrameNumber StartFrame;
	double StartValue;
	FFrameNumber EndFrame;
	double EndValue;
};

template<typename ChannelType>
void GetPairs(ChannelType* Channel, TArray<FKeyInterval>& OutKeyIntervals)
{
	using ChannelValueType = typename ChannelType::ChannelValueType;
	TMovieSceneChannelData<ChannelValueType> ChannelData = Channel->GetData();

	TArrayView<FFrameNumber> Times = ChannelData.GetTimes();
	TArrayView <ChannelValueType> Values = ChannelData.GetValues();
	OutKeyIntervals.SetNum(0);
	for (int32 Index = 0; Index < Times.Num() -1; ++Index)
	{
		FKeyInterval KeyInterval;
		KeyInterval.StartFrame = Times[Index];
		KeyInterval.StartValue = Values[Index].Value;
		KeyInterval.EndFrame = Times[Index + 1];
		KeyInterval.EndValue = Values[Index + 1].Value;
		OutKeyIntervals.Add(KeyInterval);
	}
}

template<typename ChannelType>
void  EvaluateCurveOverRange(ChannelType* Channel, const FFrameNumber& StartTime, const FFrameNumber& EndTime,
	const FFrameNumber& Interval, TArray<TPair<FFrameNumber, double>>& OutKeys)
{
	using CurveValueType = typename ChannelType::CurveValueType;
	CurveValueType Value;
	FFrameNumber CurrentTime = StartTime;
	OutKeys.SetNum(0);
	while (CurrentTime < EndTime)
	{
		Channel->Evaluate(CurrentTime, Value);
		CurrentTime += Interval;
		TPair<FFrameNumber, double> OutValue;
		OutValue.Key = CurrentTime;
		OutValue.Value = (double) Value;
		OutKeys.Add(OutValue);
	}
}

void GetPercentageOfChange(const TArray<TPair<FFrameNumber, double>>& InKeys, TArray<double>& ValueDifferences,
TArray<TPair<FFrameNumber, double>>& PercentageDifferences)
{
	ValueDifferences.SetNum(InKeys.Num());
	for (int32 Index = 0; Index < InKeys.Num() -1 ; ++Index)
	{
		const double ValueDifference = InKeys[Index + 1].Value - InKeys[Index].Value;
		ValueDifferences[Index] = ValueDifference;
	}
	const double TotalChange = Algo::Accumulate(ValueDifferences, 0.);
	if (FMath::IsNearlyZero(TotalChange) == false)
	{
		const double TotalChangePercentage = (100.0 / TotalChange);
		PercentageDifferences.SetNum(InKeys.Num());
		for (int32 Index = 0; Index < InKeys.Num(); ++Index)
		{
			PercentageDifferences[Index].Key = InKeys[Index].Key;
			PercentageDifferences[Index].Value = TotalChangePercentage * ValueDifferences[Index];
		}
	}
	else
	{
		PercentageDifferences.SetNum(0);
	}
}

template<typename ChannelType>
void AdjustmentBlend(UMovieSceneSection* Section, TArrayView<ChannelType*>& BaseChannels, 
	TArrayView<ChannelType*>& LayerChannels,ISequencer* InSequencer)
{
	if (BaseChannels.Num() != LayerChannels.Num())
	{
		return;
	}
	const FFrameRate& FrameRate = InSequencer->GetFocusedDisplayRate();
	const FFrameRate& TickResolution = InSequencer->GetFocusedTickResolution();
	const FFrameNumber Interval = TickResolution.AsFrameNumber(FrameRate.AsInterval());
	EMovieSceneKeyInterpolation DefaultInterpolation = InSequencer->GetKeyInterpolation();

	TArray<FKeyInterval> KeyIntervals;
	TArray <TPair<FFrameNumber, double>> Keys;
	TArray<double> ValueDifferences;
	TArray<TPair<FFrameNumber, double>> PercentageDifferences;
	for (int32 Index = 0; Index < BaseChannels.Num(); ++Index)
	{
		ChannelType* BaseChannel = BaseChannels[Index];
		ChannelType* LayerChannel = LayerChannels[Index];
		KeyIntervals.SetNum(0);
		Keys.SetNum(0);
		GetPairs(LayerChannel, KeyIntervals);
		for (FKeyInterval& KeyInterval : KeyIntervals)
		{
			EvaluateCurveOverRange(BaseChannel, KeyInterval.StartFrame, KeyInterval.EndFrame,
				Interval, Keys);
			GetPercentageOfChange(Keys, ValueDifferences, PercentageDifferences);
			const double TotalPoseLayerChange = FMath::Abs(KeyInterval.EndValue - KeyInterval.StartValue);
			double PreviousValue = KeyInterval.StartValue;
			for (TPair<FFrameNumber, double>& TimeValue : PercentageDifferences)
			{
				const double ValueDelta = (TotalPoseLayerChange / 100.0) * TimeValue.Value;
				const double CurrentValue = (KeyInterval.EndValue > KeyInterval.StartValue) ?
					PreviousValue + ValueDelta : PreviousValue - ValueDelta;
				AddKeyToChannel(LayerChannel, DefaultInterpolation, TimeValue.Key, CurrentValue);
			}
		}
	}
}

bool UTLLRN_TLLRN_AnimLayers::AdjustmentBlendLayers(ISequencer* InSequencer, int32 LayerIndex)
{
	if (InSequencer == nullptr)
	{
		return false;
	}
	if (LayerIndex < 1 || LayerIndex >= TLLRN_TLLRN_AnimLayers.Num())
	{
		return false;
	}
	UTLLRN_AnimLayer* BaseLayer = TLLRN_TLLRN_AnimLayers[0];
	UTLLRN_AnimLayer* TLLRN_AnimLayer = TLLRN_TLLRN_AnimLayers[LayerIndex];
	const FFrameNumber Min =  TNumericLimits<FFrameNumber>::Lowest();
	const FFrameNumber Max =  TNumericLimits<FFrameNumber>::Max();
	TRange<FFrameNumber> Range(Min, Max);
	FScopedTransaction Transaction(LOCTEXT("AdjustmentBlendLayer", "Adjustment Blend layer"), !GIsTransacting);
	
	Modify();
	TLLRN_AnimLayer->Modify();


	for (TPair<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem>& Pair : TLLRN_AnimLayer->TLLRN_AnimLayerItems)
	{
		if (Pair.Key != nullptr)
		{
			for (FTLLRN_AnimLayerSectionItem& SectionItem : Pair.Value.SectionItems)
			{
				if (SectionItem.Section.IsValid())
				{
					FTLLRN_AnimLayerSectionItem* BaseSectionItem = nullptr;
					if (FTLLRN_AnimLayerItem* Owner = BaseLayer->TLLRN_AnimLayerItems.Find(Pair.Key))
					{
						BaseSectionItem = Owner->FindMatchingSectionItem(SectionItem.Section.Get());

					}
					if (BaseSectionItem && BaseSectionItem->Section.IsValid())
					{
						if (SectionItem.Section->IsActive())//active sections merge them
						{
							TArrayView<FMovieSceneFloatChannel*> BaseFloatChannels = BaseSectionItem->Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
							TArrayView<FMovieSceneDoubleChannel*> BaseDoubleChannels = BaseSectionItem->Section->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
							TArrayView<FMovieSceneFloatChannel*> LayerFloatChannels = SectionItem.Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
							TArrayView<FMovieSceneDoubleChannel*> LayerDoubleChannels = SectionItem.Section->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();

							
							if (BaseDoubleChannels.Num() > 0)
							{
								AdjustmentBlend<FMovieSceneDoubleChannel>(SectionItem.Section.Get(), BaseDoubleChannels,LayerDoubleChannels,  InSequencer);
							}
							else if (BaseFloatChannels.Num() > 0)
							{
								AdjustmentBlend<FMovieSceneFloatChannel>(SectionItem.Section.Get(), BaseFloatChannels,LayerFloatChannels, InSequencer);
							}
						}	
					}
				}
			}
		}
	}

	InSequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
	return true;
}

#undef LOCTEXT_NAMESPACE
