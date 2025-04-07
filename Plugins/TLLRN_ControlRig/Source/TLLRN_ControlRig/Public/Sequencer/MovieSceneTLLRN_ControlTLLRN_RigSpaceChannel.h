// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Curves/KeyHandle.h"
#include "MovieSceneObjectBindingID.h"
#include "MovieSceneSection.h"
#include "Curves/IntegralCurve.h"
#include "Channels/MovieSceneChannel.h"
#include "Channels/MovieSceneChannelData.h"
#include "Channels/MovieSceneChannelTraits.h"
#include "MovieSceneClipboard.h"
#include "Rigs/TLLRN_RigHierarchyDefines.h"
#include "MovieSceneTLLRN_ControlTLLRN_RigSpaceChannel.generated.h"

class UTLLRN_ControlRig;
struct FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel;

DECLARE_MULTICAST_DELEGATE_TwoParams(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannelSpaceNoLongerUsedEvent, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel*, const TArray<FTLLRN_RigElementKey>&);

UENUM(Blueprintable)
enum class EMovieSceneTLLRN_ControlTLLRN_RigSpaceType : uint8
{
	Parent = 0,
	World,
	TLLRN_ControlRig
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey
{
	GENERATED_BODY()
	
	friend bool operator==(const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& A, const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& B)
	{
		return A.SpaceType == B.SpaceType && (A.SpaceType != EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig || A.TLLRN_ControlTLLRN_RigElement == B.TLLRN_ControlTLLRN_RigElement);
	}

	friend bool operator!=(const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& A, const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& B)
	{
		return A.SpaceType != B.SpaceType || (A.SpaceType == EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig && A.TLLRN_ControlTLLRN_RigElement != B.TLLRN_ControlTLLRN_RigElement);
	}

	FName GetName() const;

	UPROPERTY(EditAnywhere, Category = "Key")
	EMovieSceneTLLRN_ControlTLLRN_RigSpaceType SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent;
	UPROPERTY(EditAnywhere, Category = "Key")
	FTLLRN_RigElementKey TLLRN_ControlTLLRN_RigElement;

};

struct FSpaceRange
{
	TRange<FFrameNumber> Range;
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Key;
};

/** A curve of spaces */
USTRUCT()
struct TLLRN_CONTROLRIG_API FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel : public FMovieSceneChannel
{
	GENERATED_BODY()

	FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel()
	{}

	/**
	* Access a mutable interface for this channel's data
	*
	* @return An object that is able to manipulate this channel's data
	*/
	TMovieSceneChannelData<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> GetData()
	{
		return TMovieSceneChannelData<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey>(&KeyTimes, &KeyValues, this, &KeyHandles);
	}

	/**
	* Access a constant interface for this channel's data
	*
	* @return An object that is able to interrogate this channel's data
	*/
	TMovieSceneChannelData<const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> GetData() const
	{
		return TMovieSceneChannelData<const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey>(&KeyTimes, &KeyValues);
	}

	/**
	* Evaluate this channel
	*
	* @param InTime     The time to evaluate at
	* @param OutValue   A value to receive the result
	* @return true if the channel was evaluated successfully, false otherwise
	*/
	bool Evaluate(FFrameTime InTime, FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& OutValue) const;

public:

	// ~ FMovieSceneChannel Interface
	virtual void GetKeys(const TRange<FFrameNumber>& WithinRange, TArray<FFrameNumber>* OutKeyTimes, TArray<FKeyHandle>* OutKeyHandles) override;
	virtual void GetKeyTimes(TArrayView<const FKeyHandle> InHandles, TArrayView<FFrameNumber> OutKeyTimes) override;
	virtual void SetKeyTimes(TArrayView<const FKeyHandle> InHandles, TArrayView<const FFrameNumber> InKeyTimes) override;
	virtual void DuplicateKeys(TArrayView<const FKeyHandle> InHandles, TArrayView<FKeyHandle> OutNewHandles) override;
	virtual void DeleteKeys(TArrayView<const FKeyHandle> InHandles) override;
	virtual void DeleteKeysFrom(FFrameNumber InTime, bool bDeleteKeysBefore) override;
	virtual void ChangeFrameResolution(FFrameRate SourceRate, FFrameRate DestinationRate) override;
	virtual TRange<FFrameNumber> ComputeEffectiveRange() const override;
	virtual int32 GetNumKeys() const override;
	virtual void Reset() override;
	virtual void Offset(FFrameNumber DeltaPosition) override;
	virtual FKeyHandle GetHandle(int32 Index) override;
	virtual int32 GetIndex(FKeyHandle Handle) override;

	void GetUniqueSpaceList(TArray<FTLLRN_RigElementKey>* OutList);
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannelSpaceNoLongerUsedEvent& OnSpaceNoLongerUsed() { return SpaceNoLongerUsedEvent; }

	TArray <FSpaceRange> FindSpaceIntervals();

private:

	void BroadcastSpaceNoLongerUsed(const TArray<FTLLRN_RigElementKey>& BeforeKeys, const TArray<FTLLRN_RigElementKey>& AfterKeys);

	/** Sorted array of key times */
	UPROPERTY(meta = (KeyTimes))
	TArray<FFrameNumber> KeyTimes;

	/** Array of values that correspond to each key time */
	UPROPERTY(meta = (KeyValues))
	TArray<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> KeyValues;

	/** This needs to be a UPROPERTY so it gets saved into editor transactions but transient so it doesn't get saved into assets. */
	UPROPERTY(Transient)
	FMovieSceneKeyHandleMap KeyHandles;
	
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannelSpaceNoLongerUsedEvent SpaceNoLongerUsedEvent;

	friend struct FTLLRN_ControlTLLRN_RigSpaceChannelHelpers;
};


template<>
struct TMovieSceneChannelTraits<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel> : TMovieSceneChannelTraitsBase<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>
{
	enum { SupportsDefaults = false };

};

inline bool EvaluateChannel(const FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* InChannel, FFrameTime InTime, FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& OutValue)
{
	return InChannel->Evaluate(InTime, OutValue);
}

#if WITH_EDITOR
namespace MovieSceneClipboard
{
	template<> inline FName GetKeyTypeName<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey>()
	{
		return "FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey";
	}
}
#endif

//mz todoo TSharedPtr<FStructOnScope> GetKeyStruct(TMovieSceneChannelHandle<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel> Channel, FKeyHandle InHandle);