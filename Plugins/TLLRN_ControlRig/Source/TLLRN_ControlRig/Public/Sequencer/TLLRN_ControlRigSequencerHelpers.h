// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FTLLRN_ChannelMapInfo;
class UTLLRN_ControlRig;
class UMovieSceneTLLRN_ControlRigParameterSection;
struct FMovieSceneFloatChannel;
struct FMovieSceneBoolChannel;
struct FMovieSceneIntegerChannel;
struct FMovieSceneByteChannel;
class UMovieSceneSection;
class UMovieSceneTLLRN_ControlRigParameterTrack;
class UMovieSceneSequence;

struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigSequencerHelpers
{
	static TPair<const FTLLRN_ChannelMapInfo*, int32> GetInfoAndNumFloatChannels(
		const UTLLRN_ControlRig* InTLLRN_ControlRig,
		const FName& InControlName,
		const UMovieSceneTLLRN_ControlRigParameterSection* InSection);

	static TArrayView<FMovieSceneFloatChannel*> GetFloatChannels(const UTLLRN_ControlRig* InTLLRN_ControlRig,
		const FName& InControlName, const UMovieSceneSection* InSection);

	static TArrayView<FMovieSceneBoolChannel*> GetBoolChannels(const UTLLRN_ControlRig* InTLLRN_ControlRig,
		const FName& InControlName, const UMovieSceneSection* InSection);

	static TArrayView<FMovieSceneByteChannel*> GetByteChannels(const UTLLRN_ControlRig* InTLLRN_ControlRig,
		const FName& InControlName, const UMovieSceneSection* InSection);

	static TArrayView<FMovieSceneIntegerChannel*> GetIntegerChannels(const UTLLRN_ControlRig* InTLLRN_ControlRig,
		const FName& InControlName, const UMovieSceneSection* InSection);

	static UMovieSceneTLLRN_ControlRigParameterTrack*  FindTLLRN_ControlRigTrack(UMovieSceneSequence* InSequencer, const UTLLRN_ControlRig* InTLLRN_ControlRig);

};

