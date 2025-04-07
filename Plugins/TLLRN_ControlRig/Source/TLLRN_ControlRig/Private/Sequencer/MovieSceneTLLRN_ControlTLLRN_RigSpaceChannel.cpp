// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/MovieSceneTLLRN_ControlTLLRN_RigSpaceChannel.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "MovieSceneObjectBindingID.h"
#include "MovieSceneSequenceID.h"
#include "Evaluation/MovieSceneSequenceHierarchy.h"
#include "IMovieScenePlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovieSceneTLLRN_ControlTLLRN_RigSpaceChannel)

bool FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::Evaluate(FFrameTime InTime, FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& OutValue) const
{
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey DefaultValue; //this will be in parent space
	if (KeyTimes.Num())
	{
		const int32 Index = FMath::Max(0, Algo::UpperBound(KeyTimes, InTime.FrameNumber) - 1);
		OutValue = KeyValues[Index];
		return true;
	}
	OutValue = DefaultValue;
	return true;
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::GetKeys(const TRange<FFrameNumber>& WithinRange, TArray<FFrameNumber>* OutKeyTimes, TArray<FKeyHandle>* DeleteKeyHandles)
{
	GetData().GetKeys(WithinRange, OutKeyTimes, DeleteKeyHandles);
}

FKeyHandle FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::GetHandle(int32 Index)
{
	return GetData().GetHandle(Index);
}

int32 FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::GetIndex(FKeyHandle Handle)
{
	return GetData().GetIndex(Handle);
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::GetKeyTimes(TArrayView<const FKeyHandle> InHandles, TArrayView<FFrameNumber> OutKeyTimes)
{
	GetData().GetKeyTimes(InHandles, OutKeyTimes);
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::SetKeyTimes(TArrayView<const FKeyHandle> InHandles, TArrayView<const FFrameNumber> InKeyTimes)
{
	GetData().SetKeyTimes(InHandles, InKeyTimes);
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::DuplicateKeys(TArrayView<const FKeyHandle> InHandles, TArrayView<FKeyHandle> OutNewHandles)
{
	GetData().DuplicateKeys(InHandles, OutNewHandles);
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::DeleteKeys(TArrayView<const FKeyHandle> InHandles)
{
	TArray<FTLLRN_RigElementKey> BeforeKeys, AfterKeys;
	GetUniqueSpaceList(&BeforeKeys);
	
	GetData().DeleteKeys(InHandles);

	GetUniqueSpaceList(&AfterKeys);
	BroadcastSpaceNoLongerUsed(BeforeKeys, AfterKeys);
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::DeleteKeysFrom(FFrameNumber InTime, bool bDeleteKeysBefore)
{
	TArray<FTLLRN_RigElementKey> BeforeKeys, AfterKeys;
	GetUniqueSpaceList(&BeforeKeys);
	
	// Insert a key at the current time to maintain evaluation
	if (GetData().GetTimes().Num() > 0)
	{
		FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
		Evaluate(InTime, Value);
		GetData().UpdateOrAddKey(InTime, Value);
	}

	GetData().DeleteKeysFrom(InTime, bDeleteKeysBefore);

	GetUniqueSpaceList(&AfterKeys);
	BroadcastSpaceNoLongerUsed(BeforeKeys, AfterKeys);
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::ChangeFrameResolution(FFrameRate SourceRate, FFrameRate DestinationRate)
{
	GetData().ChangeFrameResolution(SourceRate, DestinationRate);
}

TRange<FFrameNumber> FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::ComputeEffectiveRange() const
{
	return GetData().GetTotalRange();
}

int32 FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::GetNumKeys() const
{
	return KeyTimes.Num();
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::Reset()
{
	KeyTimes.Reset();
	KeyValues.Reset();
	KeyHandles.Reset();
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::Offset(FFrameNumber DeltaPosition)
{
	GetData().Offset(DeltaPosition);
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::GetUniqueSpaceList(TArray<FTLLRN_RigElementKey>* OutList)
{
	if(OutList)
	{
		OutList->Reset();
		
		for(const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& KeyValue : KeyValues)
		{
			OutList->AddUnique(KeyValue.TLLRN_ControlTLLRN_RigElement);
		}
	}
}

//this will also delete any duplicated keys.
TArray <FSpaceRange> FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::FindSpaceIntervals()
{
	TArray<FSpaceRange> Ranges;
	TArray<int32> ToDelete;
	for (int32 Index = 0; Index < KeyTimes.Num(); ++Index)
	{
		FSpaceRange Range;
		Range.Key = KeyValues[Index];
		if (Index == KeyTimes.Num() - 1)
		{
			Range.Range.SetLowerBound(TRangeBound<FFrameNumber>(FFrameNumber(KeyTimes[Index])));
			Range.Range.SetUpperBound(TRangeBound<FFrameNumber>(FFrameNumber(KeyTimes[Index])));
		}
		else
		{
			int32 NextIndex = Index;
			FFrameNumber LowerBound = KeyTimes[Index],UpperBound = KeyTimes[Index];
			while (NextIndex < KeyTimes.Num() -1)
			{
				NextIndex = Index + 1;
				if (Range.Key == KeyValues[NextIndex])
				{
					Index++;
					ToDelete.Add(NextIndex);
				}
				else
				{
					UpperBound = KeyTimes[NextIndex];
					Index = NextIndex - 1;
					NextIndex = KeyTimes.Num();
				}
			}
			Range.Range.SetLowerBound(TRangeBound<FFrameNumber>(LowerBound));
			Range.Range.SetUpperBound(TRangeBound<FFrameNumber>(UpperBound));
		}
		Ranges.Add(Range);
	}
	//now delete duplicate values
	if (ToDelete.Num() > 0)
	{
		TArray<FKeyHandle> DeleteKeyHandles;
		for (int32 Index : ToDelete)
		{
			FKeyHandle KeyHandle = GetData().GetHandle(Index);
			DeleteKeyHandles.Add(KeyHandle);
		}

		GetData().DeleteKeys(DeleteKeyHandles);

	}
	return Ranges;
}

void FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel::BroadcastSpaceNoLongerUsed(const TArray<FTLLRN_RigElementKey>& BeforeKeys, const TArray<FTLLRN_RigElementKey>& AfterKeys)
{
	if(BeforeKeys == AfterKeys)
	{
		return;
	}

	if(!SpaceNoLongerUsedEvent.IsBound())
	{
		return;
	}

	TArray<FTLLRN_RigElementKey> SpacesNoLongerUsed;
	for(const FTLLRN_RigElementKey& BeforeKey : BeforeKeys)
	{
		if(!AfterKeys.Contains(BeforeKey))
		{
			SpacesNoLongerUsed.Add(BeforeKey);
		}
	}

	if(!SpacesNoLongerUsed.IsEmpty())
	{
		SpaceNoLongerUsedEvent.Broadcast(this, SpacesNoLongerUsed);
	}
}

FName FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey::GetName() const
{
	switch (SpaceType)
	{
	case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent:
		return FName(TEXT("Parent"));
		break;

	case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World:
		return FName(TEXT("World"));
		break;

	case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig:
		return TLLRN_ControlTLLRN_RigElement.Name;
		break;
	};
	return NAME_None;
}

/* Is this needed if not remove mz todoo
TSharedPtr<FStructOnScope> GetKeyStruct(TMovieSceneChannelHandle<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel> Channel, FKeyHandle InHandle)
{
	int32 KeyValueIndex = Channel.Get()->GetData().GetIndex(InHandle);
	if (KeyValueIndex != INDEX_NONE)
	{
		FNiagaraTypeDefinition KeyType = Channel.Get()->GetData().GetValues()[KeyValueIndex].Value.GetType();
		uint8* KeyData = Channel.Get()->GetData().GetValues()[KeyValueIndex].Value.GetData();
		return MakeShared<FStructOnScope>(KeyType.GetStruct(), KeyData);
		
	}
	return TSharedPtr<FStructOnScope>();
}
*/


