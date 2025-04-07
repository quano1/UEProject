// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/SharedPointer.h"
#include "Containers/Array.h"
#include "Containers/ContainersFwd.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"

#include "MovieSceneKeyStruct.h"
#include "SequencerChannelTraits.h"
#include "Channels/MovieSceneChannelHandle.h"

#include "Sequencer/MovieSceneTLLRN_ControlTLLRN_RigSpaceChannel.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Editor/STLLRN_RigSpacePickerWidget.h"
#include "Containers/SortedMap.h"
#include "KeyBarCurveModel.h"

struct FKeyHandle;
struct FKeyDrawParams;
class UTLLRN_ControlRig;
class ISequencer;
class UMovieSceneSection;
class UTLLRN_RigHierarchy;
struct FTLLRN_RigElementKey;
class SWidget;
class ISequencer;
class FMenuBuilder;


struct FSpaceChannelAndSection
{
	FSpaceChannelAndSection() : SectionToKey(nullptr), SpaceChannel(nullptr) {};
	UMovieSceneSection* SectionToKey;
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* SpaceChannel;
};


/*
* Class that contains helper functions for various space switching activities
*/
struct FTLLRN_ControlTLLRN_RigSpaceChannelHelpers
{
	static FKeyHandle SequencerKeyTLLRN_ControlTLLRN_RigSpaceChannel(UTLLRN_ControlRig* TLLRN_ControlRig, ISequencer* Sequencer, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey, FFrameNumber Time, UTLLRN_RigHierarchy* TLLRN_RigHierarchy, const FTLLRN_RigElementKey& ControlKey, const FTLLRN_RigElementKey& SpaceKey);
	static void SequencerSpaceChannelKeyDeleted(UTLLRN_ControlRig* TLLRN_ControlRig, ISequencer* Sequencer, FName ControlName, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneTLLRN_ControlRigParameterSection* SectionToKey, FFrameNumber TimeOfDeletion);
	static UMovieSceneTLLRN_ControlRigParameterSection* GetTLLRN_ControlRigSection(ISequencer* Sequencer, const UTLLRN_ControlRig* TLLRN_ControlRig);
	static void CompensateIfNeeded(UTLLRN_ControlRig* TLLRN_ControlRig, ISequencer* Sequencer, UMovieSceneTLLRN_ControlRigParameterSection* Section, TOptional<FFrameNumber>& Time, bool bCompPreviousTick);
	static FSpaceChannelAndSection FindSpaceChannelAndSectionForControl(UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, ISequencer* Sequencer, bool bCreateIfNeeded);
	static void SequencerBakeControlInSpace(UTLLRN_ControlRig* TLLRN_ControlRig, ISequencer* Sequencer, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey,
		UTLLRN_RigHierarchy* TLLRN_RigHierarchy, const FTLLRN_RigElementKey& ControlKey, FTLLRN_TLLRN_RigSpacePickerBakeSettings InSettings);
	static void GetFramesInThisSpaceAfterThisTime(UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey CurrentValue,
		FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey,
		FFrameNumber Time, TSortedMap<FFrameNumber,FFrameNumber>& OutMoreFrames);
	static void HandleSpaceKeyTimeChanged(UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName,FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey,
		FFrameNumber CurrentFrame, FFrameNumber NextFrame);
	static void DeleteTransformKeysAtThisTime(UTLLRN_ControlRig* TLLRN_ControlRig, UMovieSceneTLLRN_ControlRigParameterSection* Section, FName ControlName, FFrameNumber Time);
	static FLinearColor GetColor(const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& Key);
	static FReply OpenBakeDialog(ISequencer* Sequencer, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, int32 KeyIndex, UMovieSceneSection* SectionToKey);
	static TArray<FKeyBarCurveModel::FBarRange> FindRanges(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, const UMovieSceneSection* Section);
	// retrieve the control and the channel infos for that TLLRN_ControlRig/Section.
	static TPair<FTLLRN_RigControlElement*, FTLLRN_ChannelMapInfo*> GetControlAndChannelInfo(UTLLRN_ControlRig* TLLRN_ControlRig, UMovieSceneTLLRN_ControlRigParameterSection* Section, FName ControlName);
	// retrieve the number of float channels based on the control type.
	static int32 GetNumFloatChannels(const ETLLRN_RigControlType InControlType);
};

//template specialization
FKeyHandle AddOrUpdateKey(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, UMovieSceneSection* SectionToKey, FFrameNumber Time, ISequencer& Sequencer, const FGuid& ObjectBindingID, FTrackInstancePropertyBindings* PropertyBindings);

/** Key editor overrides */
bool CanCreateKeyEditor(const FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel);
TSharedRef<SWidget> CreateKeyEditor(const TMovieSceneChannelHandle<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>& Channel, const UE::Sequencer::FCreateKeyEditorParams& Params);

/** Key drawing overrides */
void DrawKeys(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, TArrayView<const FKeyHandle> InKeyHandles, const UMovieSceneSection* InOwner, TArrayView<FKeyDrawParams> OutKeyDrawParams);
int32 DrawExtra(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, const UMovieSceneSection* Owner, const FSequencerChannelPaintArgs& PaintArgs, int32 LayerId);

//UMovieSceneKeyStructType* IstanceGeneratedStruct(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel, FSequencerKeyStructGenerator* Generator);

//void PostConstructKeystance(const TMovieSceneChannelHandle<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>& ChannelHandle, FKeyHandle InHandle, FStructOnScope* Struct)

/** Context menu overrides */
//void ExtendSectionMenu(FMenuBuilder& OuterMenuBuilder, TArray<TMovieSceneChannelHandle<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>>&& Channels, TArrayView<UMovieSceneSection* const> Sections, TWeakPtr<ISequencer> InSequencer);
//void ExtendKeyMenu(FMenuBuilder& OuterMenuBuilder, TArray<TExtendKeyMenuParams<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>>&& Channels, TWeakPtr<ISequencer> InSequencer);

/** Curve editor models */
inline bool SupportsCurveEditorModels(const TMovieSceneChannelHandle<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>& Channel) { return true; }
TUniquePtr<FCurveModel> CreateCurveEditorModel(const TMovieSceneChannelHandle<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>& Channel, const UE::Sequencer::FCreateCurveEditorModelParams& Params);


