// Copyright Epic Games, Inc. All Rights Reserved.
#include "Sequencer/TLLRN_ControlRigSequencerHelpers.h"
#include "TLLRN_ControlRig.h"
#include "Rigs/TLLRN_RigHierarchyElements.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Channels/MovieSceneIntegerChannel.h"
#include "Channels/MovieSceneByteChannel.h"
#include "Channels/MovieSceneBoolChannel.h"
#include "MovieSceneSequence.h"
#include "MovieScene.h"

TPair<const FTLLRN_ChannelMapInfo*, int32> FTLLRN_ControlRigSequencerHelpers::GetInfoAndNumFloatChannels(
	const UTLLRN_ControlRig* InTLLRN_ControlRig,
	const FName& InControlName,
	const UMovieSceneTLLRN_ControlRigParameterSection* InSection)
{
	const FTLLRN_RigControlElement* ControlElement = InTLLRN_ControlRig ? InTLLRN_ControlRig->FindControl(InControlName) : nullptr;
	auto GetNumFloatChannels = [](const ETLLRN_RigControlType& InControlType)
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
		case ETLLRN_RigControlType::Vector2D:
			return 2;
		case ETLLRN_RigControlType::Float:
		case ETLLRN_RigControlType::ScaleFloat:
			return 1;
		default:
			break;
		}
		return 0;
	};

	const int32 NumFloatChannels = ControlElement ? GetNumFloatChannels(ControlElement->Settings.ControlType) : 0;
	const FTLLRN_ChannelMapInfo* ChannelInfo = InSection ? InSection->ControlChannelMap.Find(InControlName) : nullptr;

	return { ChannelInfo, NumFloatChannels };
}


TArrayView<FMovieSceneFloatChannel*>  FTLLRN_ControlRigSequencerHelpers::GetFloatChannels(const UTLLRN_ControlRig* InTLLRN_ControlRig,
	const FName& InControlName, const UMovieSceneSection* InSection)
{
	// no floats for transform sections
	static const TArrayView<FMovieSceneFloatChannel*> EmptyChannelsView;

	const FTLLRN_ChannelMapInfo* ChannelInfo = nullptr;
	int32 NumChannels = 0;
	const UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);
	if (CRSection == nullptr)
	{
		return EmptyChannelsView;
	}

	Tie(ChannelInfo, NumChannels) = FTLLRN_ControlRigSequencerHelpers::GetInfoAndNumFloatChannels(InTLLRN_ControlRig, InControlName, CRSection);

	if (ChannelInfo == nullptr || NumChannels == 0)
	{
		return EmptyChannelsView;
	}

	// return a sub view that just represents the control's channels
	const TArrayView<FMovieSceneFloatChannel*> FloatChannels = InSection->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
	const int32 ChannelStartIndex = ChannelInfo->ChannelIndex;
	return FloatChannels.Slice(ChannelStartIndex, NumChannels);
}


TArrayView<FMovieSceneBoolChannel*> FTLLRN_ControlRigSequencerHelpers::GetBoolChannels(const UTLLRN_ControlRig* InTLLRN_ControlRig,
	const FName& InControlName, const UMovieSceneSection* InSection)
{
	static const TArrayView<FMovieSceneBoolChannel*> EmptyChannelsView;
	const UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);
	if (CRSection == nullptr)
	{
		return EmptyChannelsView;
	}
	const FTLLRN_RigControlElement* ControlElement = InTLLRN_ControlRig ? InTLLRN_ControlRig->FindControl(InControlName) : nullptr;
	if (ControlElement == nullptr)
	{
		return EmptyChannelsView;
	}
	if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool)
	{
		int32 NumChannels = 1;
		const FTLLRN_ChannelMapInfo* ChannelInfo = CRSection ? CRSection->ControlChannelMap.Find(InControlName) : nullptr;
		if (ChannelInfo == nullptr)
		{
			return EmptyChannelsView;
		}
		const TArrayView<FMovieSceneBoolChannel*> Channels = CRSection->GetChannelProxy().GetChannels<FMovieSceneBoolChannel>();
		const int32 ChannelStartIndex = ChannelInfo->ChannelIndex;
		return Channels.Slice(ChannelStartIndex, NumChannels);

	}
	return EmptyChannelsView;
}



TArrayView<FMovieSceneByteChannel*> FTLLRN_ControlRigSequencerHelpers::GetByteChannels(const UTLLRN_ControlRig* InTLLRN_ControlRig,
	const FName& InControlName, const UMovieSceneSection* InSection)
{
	static const TArrayView<FMovieSceneByteChannel*> EmptyChannelsView;
	const UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);
	if (CRSection == nullptr)
	{
		return EmptyChannelsView;
	}
	const FTLLRN_RigControlElement* ControlElement = InTLLRN_ControlRig ? InTLLRN_ControlRig->FindControl(InControlName) : nullptr;
	if (ControlElement == nullptr)
	{
		return EmptyChannelsView;
	}
	if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer && ControlElement->Settings.ControlEnum)
	{
		int32 NumChannels = 1;
		const FTLLRN_ChannelMapInfo* ChannelInfo = CRSection ? CRSection->ControlChannelMap.Find(InControlName) : nullptr;
		if (ChannelInfo == nullptr)
		{
			return EmptyChannelsView;
		}
		const TArrayView<FMovieSceneByteChannel*> Channels = CRSection->GetChannelProxy().GetChannels<FMovieSceneByteChannel>();
		const int32 ChannelStartIndex = ChannelInfo->ChannelIndex;
		return Channels.Slice(ChannelStartIndex, NumChannels);

	}
	return EmptyChannelsView;
}

TArrayView<FMovieSceneIntegerChannel*> FTLLRN_ControlRigSequencerHelpers::GetIntegerChannels(const UTLLRN_ControlRig* InTLLRN_ControlRig,
	const FName& InControlName, const UMovieSceneSection* InSection)
{
	static const TArrayView<FMovieSceneIntegerChannel*> EmptyChannelsView;
	const UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);
	if (CRSection == nullptr)
	{
		return EmptyChannelsView;
	}
	const FTLLRN_RigControlElement* ControlElement = InTLLRN_ControlRig ? InTLLRN_ControlRig->FindControl(InControlName) : nullptr;
	if (ControlElement == nullptr)
	{
		return EmptyChannelsView;
	}
	if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer && ControlElement->Settings.ControlEnum == nullptr)
	{
		int32 NumChannels = 1;
		const FTLLRN_ChannelMapInfo* ChannelInfo = CRSection ? CRSection->ControlChannelMap.Find(InControlName) : nullptr;
		if (ChannelInfo == nullptr)
		{
			return EmptyChannelsView;
		}
		const TArrayView<FMovieSceneIntegerChannel*> Channels = CRSection->GetChannelProxy().GetChannels<FMovieSceneIntegerChannel>();
		const int32 ChannelStartIndex = ChannelInfo->ChannelIndex;
		return Channels.Slice(ChannelStartIndex, NumChannels);

	}
	return EmptyChannelsView;
}

UMovieSceneTLLRN_ControlRigParameterTrack* FTLLRN_ControlRigSequencerHelpers::FindTLLRN_ControlRigTrack(UMovieSceneSequence* MovieSceneSequence, const UTLLRN_ControlRig* TLLRN_ControlRig)
{
	UMovieSceneTLLRN_ControlRigParameterTrack* Track = nullptr;
	if (MovieSceneSequence == nullptr)
	{
		return Track;
	}
	UMovieScene* MovieScene = MovieSceneSequence->GetMovieScene();
	const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();
	bool bRecreateCurves = false;
	TArray<TPair<UTLLRN_ControlRig*, FName>> TLLRN_ControlRigPairsToReselect;
	for (const FMovieSceneBinding& Binding : Bindings)
	{
		UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigParameterTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(MovieScene->FindTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.GetObjectGuid(), NAME_None));
		if (TLLRN_ControlRigParameterTrack && TLLRN_ControlRigParameterTrack->GetTLLRN_ControlRig() == TLLRN_ControlRig)
		{
			Track = TLLRN_ControlRigParameterTrack;
			break;
		}
	}
	return Track;
}
