// Copyright Epic Games, Inc. All Rights Reserved.


#include "TLLRN_TLLRN_AnimDetailsProxy.h"
#include "EditorModeManager.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "TLLRN_ControlRig.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Components/SkeletalMeshComponent.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "MovieSceneCommonHelpers.h"
#include "PropertyHandle.h"
#include "Sequencer/TLLRN_ControlRigSequencerHelpers.h"
#include "ISequencer.h"
#include "CurveEditor.h"
#include "CurveModel.h"
#include "MovieScene.h"
#include "Tracks/MovieScenePropertyTrack.h"
#include "MovieSceneCommonHelpers.h"
#include "UnrealEdGlobals.h"
#include "UnrealEdMisc.h"
#include "Editor/UnrealEdEngine.h"
#include "ScopedTransaction.h"
#include "Channels/MovieSceneBoolChannel.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "Channels/MovieSceneIntegerChannel.h"
#include "Channels/MovieSceneByteChannel.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "SEnumCombo.h"
#include "MVVM/ViewModels/ChannelModel.h"
#include "MVVM/ViewModels/SectionModel.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "MVVM/SectionModelStorageExtension.h"
#include "MVVM/ViewModels/OutlinerViewModel.h"
#include "MVVM/ViewModels/SequenceModel.h"
#include "MVVM/ViewModels/TrackAreaViewModel.h"
#include "MVVM/Extensions/IOutlinerExtension.h"
#include "MVVM/ViewModels/TrackModel.h"
#include "MVVM/CurveEditorExtension.h"
#include "Tree/SCurveEditorTree.h"
#include "IKeyArea.h"
#include "SequencerAddKeyOperation.h"
#include "TransformConstraint.h"
#include "LevelEditorViewport.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "ConstraintsManager.h"
#include "Constraints/MovieSceneConstraintChannelHelper.h"
#include "Constraints/TLLRN_ControlTLLRN_RigTransformableHandle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_TLLRN_AnimDetailsProxy)

static void KeyTrack(TSharedPtr<ISequencer>& Sequencer, UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, UMovieScenePropertyTrack* Track, ETLLRN_ControlRigContextChannelToKey ChannelToKey)
{
	using namespace UE::Sequencer;

	const FFrameNumber Time = Sequencer->GetLocalTime().Time.FloorToFrame();

	float Weight = 0.0;
	UMovieSceneSection* Section = Track->FindOrExtendSection(Time, Weight);
	FScopedTransaction PropertyChangedTransaction(NSLOCTEXT("TLLRN_ControlRig", "KeyProperty", "Key Property"), !GIsTransacting);
	if (!Section || !Section->TryModify())
	{
		PropertyChangedTransaction.Cancel();
		return;
	}

	FSectionModelStorageExtension* SectionModelStorage = Sequencer->GetViewModel()->GetRootModel()->CastDynamic<FSectionModelStorageExtension>();
	check(SectionModelStorage);

	TSharedPtr<FSectionModel> SectionHandle = SectionModelStorage->FindModelForSection(Section);
	if (SectionHandle)
	{
		TArray<TSharedRef<IKeyArea>> KeyAreas;
		TParentFirstChildIterator<FChannelGroupModel> KeyAreaNodes = SectionHandle->GetParentTrackModel().AsModel()->GetDescendantsOfType<FChannelGroupModel>();
		for (const TViewModelPtr<FChannelGroupModel>& KeyAreaNode : KeyAreaNodes)
		{
			for (const TWeakViewModelPtr<FChannelModel>& Channel : KeyAreaNode->GetChannels())
			{
				if (TSharedPtr< FChannelModel> ChannelPtr = Channel.Pin())
				{
					ETLLRN_ControlRigContextChannelToKey ThisChannelToKey = Proxy->GetChannelToKeyFromChannelName(ChannelPtr->GetChannelName().ToString());
					if ((int32)ChannelToKey & (int32)ThisChannelToKey)
					{
						KeyAreas.Add(ChannelPtr->GetKeyArea().ToSharedRef());
					}
				}
			}
		}	
		TSharedPtr<FTrackModel> TrackModel = SectionHandle->FindAncestorOfType<FTrackModel>();
		FAddKeyOperation::FromKeyAreas(TrackModel->GetTrackEditor().Get(), KeyAreas).Commit(Time, *Sequencer);
	}				
}

//////UTLLRN_AnimDetailControlsKeyedProxy////////

static ETLLRN_AnimDetailSelectionState CachePropertySelection(TWeakPtr<FCurveEditor>& InCurveEditor, UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName)
{
	int32 TotalSelected = 0;
	int32 TotalNum = 0; //need to add proxies + items that are valid
	if (InCurveEditor.IsValid())
	{
		TSharedPtr<FCurveEditor> CurveEditor = InCurveEditor.Pin();
		const TMap<FCurveModelID, TUniquePtr<FCurveModel>>& Curves = CurveEditor->GetCurves();
		if (Proxy)
		{
			for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : Proxy->TLLRN_ControlRigItems)
			{
				if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
				{
					for (const FName& CName : Items.Value.ControlElements)
					{
						if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
						{
							++TotalNum;
							ETLLRN_ControlRigContextChannelToKey ChannelToKey = Proxy->GetChannelToKeyFromPropertyName(PropertyName);
							FText ControlNameText = TLLRN_ControlRig->GetHierarchy()->GetDisplayNameForUI(ControlElement);
							FString ControlNameString = ControlNameText.ToString();
							for (const TPair<FCurveModelID, TUniquePtr<FCurveModel>>& Pair : Curves) //horribly slow
							{
								FCurveEditorTreeItemID TreeItemID = CurveEditor->GetTreeIDFromCurveID(Pair.Key);
								if (Pair.Value.IsValid() && CurveEditor->GetTreeSelectionState(TreeItemID) == ECurveEditorTreeSelectionState::Explicit)
								{
									UObject* OwnerObject = Pair.Value->GetOwningObject();
									UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast< UMovieSceneTLLRN_ControlRigParameterSection>(OwnerObject);
									if (!Section || Section->GetTLLRN_ControlRig() != TLLRN_ControlRig)
									{
										continue;
									}
									if (UMovieSceneTLLRN_ControlRigParameterTrack* Track = Section->GetTypedOuter< UMovieSceneTLLRN_ControlRigParameterTrack>())
									{
										if (Track->GetSectionToKey(ControlElement->GetFName()) != Section)
										{
											continue;
										}
									}
									else
									{
										continue;
									}
									UObject* ColorObject = nullptr;
									FString Name;
									Pair.Value->GetCurveColorObjectAndName(&ColorObject, Name);
									TArray<FString> StringArray;
									Name.ParseIntoArray(StringArray, TEXT("."));
									if (StringArray.Num() > 1)
									{
										if (StringArray[0] == ControlNameString ||
											StringArray[1] == ControlNameString) //nested controls will be 2nd
										{
											FString ChannelName;
											if (StringArray.Num() == 3)
											{
												ChannelName = StringArray[1] + "." + StringArray[2];
											}
											else if (StringArray.Num() == 2)
											{
												ChannelName = StringArray[1];
											}

											ETLLRN_ControlRigContextChannelToKey ChannelToKeyFromCurve = Proxy->GetChannelToKeyFromChannelName(ChannelName);
											if (ChannelToKeyFromCurve == ChannelToKey)
											{
												TotalSelected++;
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}
			for (const TPair<TWeakObjectPtr<UObject>, FSequencerProxyItem>& SItems : Proxy->SequencerItems)
			{
				if (UObject* Object = SItems.Key.Get())
				{
					for (const FBindingAndTrack& Element : SItems.Value.Bindings)
					{
						++TotalNum;
						ETLLRN_ControlRigContextChannelToKey ChannelToKey = Proxy->GetChannelToKeyFromPropertyName(PropertyName);
						for (const TPair<FCurveModelID, TUniquePtr<FCurveModel>>& Pair : Curves) //horribly slow
						{
							FCurveEditorTreeItemID TreeItemID = CurveEditor->GetTreeIDFromCurveID(Pair.Key);
							if (Pair.Value.IsValid() && CurveEditor->GetTreeSelectionState(TreeItemID) == ECurveEditorTreeSelectionState::Explicit)
							{
								UObject* OwnerObject = Pair.Value->GetOwningObject();
								UMovieSceneSection* Section = Cast< UMovieSceneSection>(OwnerObject);
								if (!Section || Section->GetTypedOuter<UMovieSceneTrack>() != Element.WeakTrack.Get())
								{
									continue;
								}
								UObject* ColorObject = nullptr;
								FString Name;
								Pair.Value->GetCurveColorObjectAndName(&ColorObject, Name);
								TArray<FString> StringArray;
								Name.ParseIntoArray(StringArray, TEXT("."));
								if (StringArray.Num() > 1)
								{
									FString ChannelName;
									if (StringArray.Num() == 2)
									{
										ChannelName = StringArray[0] + "." + StringArray[1];
									}
									else if (StringArray.Num() == 1)
									{
										ChannelName = StringArray[0];
									}

									ETLLRN_ControlRigContextChannelToKey ChannelToKeyFromCurve = Proxy->GetChannelToKeyFromChannelName(ChannelName);
									if (ChannelToKeyFromCurve == ChannelToKey)
									{
										TotalSelected++;
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if (TotalSelected == TotalNum)
	{
		return ETLLRN_AnimDetailSelectionState::All;
	}
	else if (TotalSelected == 0)
	{
		return ETLLRN_AnimDetailSelectionState::None;
	}
	return ETLLRN_AnimDetailSelectionState::Partial;
}

void UTLLRN_AnimDetailControlsKeyedProxy::SetKey(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& KeyedPropertyHandle)
{
	FTLLRN_RigControlModifiedContext NotifyDrivenContext; //always key ever
	NotifyDrivenContext.SetKey = ETLLRN_ControlRigSetKey::Always;
	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					if (TLLRN_ControlRig->GetHierarchy()->Contains(FTLLRN_RigElementKey(ControlElement->GetKey().Name, ETLLRN_RigElementType::Control)))
					{
						FTLLRN_RigControlModifiedContext Context;
						Context.SetKey = ETLLRN_ControlRigSetKey::Always;
						FName PropertyName = KeyedPropertyHandle.GetProperty()->GetFName();
						Context.KeyMask = (uint32)GetChannelToKeyFromPropertyName(PropertyName);
						SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(TLLRN_ControlRig, ControlElement, Context);
						FTLLRN_ControlRigEditMode::NotifyDrivenControls(TLLRN_ControlRig, ControlElement->GetKey(), NotifyDrivenContext);
					}
				}
			}
		}
	}
	for (const TPair<TWeakObjectPtr<UObject>, FSequencerProxyItem>& SItems : SequencerItems)
	{
		if (SItems.Key.IsValid())
		{
			TArray<UObject*> ObjectsToKey = { SItems.Key.Get()};
			for (const FBindingAndTrack& Element : SItems.Value.Bindings)
			{
				FName PropertyName = KeyedPropertyHandle.GetProperty()->GetFName();
				ETLLRN_ControlRigContextChannelToKey ChannelToKey = GetChannelToKeyFromPropertyName(PropertyName);
				if (UMovieScenePropertyTrack* PropertyTrack = Cast<UMovieScenePropertyTrack>(Element.WeakTrack.Get()))
				{
					KeyTrack(Sequencer, this, PropertyTrack, ChannelToKey);
				}
			}
		}
	}
}
static EPropertyKeyedStatus GetChannelKeyStatus(FMovieSceneChannel* Channel, EPropertyKeyedStatus SectionKeyedStatus, const TRange<FFrameNumber>& Range, int32& EmptyChannelCount)
{
	if (!Channel)
	{
		return SectionKeyedStatus;
	}

	if (Channel->GetNumKeys() == 0)
	{
		++EmptyChannelCount;
		return SectionKeyedStatus;
	}

	SectionKeyedStatus = FMath::Max(SectionKeyedStatus, EPropertyKeyedStatus::KeyedInOtherFrame);

	TArray<FFrameNumber> KeyTimes;
	Channel->GetKeys(Range, &KeyTimes, nullptr);
	if (KeyTimes.IsEmpty())
	{
		++EmptyChannelCount;
	}
	else
	{
		SectionKeyedStatus = FMath::Max(SectionKeyedStatus, EPropertyKeyedStatus::PartiallyKeyed);
	}
	return SectionKeyedStatus;
}

static EPropertyKeyedStatus GetKeyedStatusInSection(const UTLLRN_ControlRig* TLLRN_ControlRig, const FName& ControlName, UMovieSceneTLLRN_ControlRigParameterSection* Section, const TRange<FFrameNumber>& Range, ETLLRN_ControlRigContextChannelToKey ChannelToKey)
{
	int32 EmptyChannelCount = 0;
	EPropertyKeyedStatus SectionKeyedStatus = EPropertyKeyedStatus::NotKeyed;
	const FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig ? TLLRN_ControlRig->FindControl(ControlName) : nullptr;
	if (ControlElement == nullptr)
	{
		return SectionKeyedStatus;
	}
	switch (ControlElement->Settings.ControlType)
	{
	case ETLLRN_RigControlType::Bool:
	{
		TArrayView<FMovieSceneBoolChannel*> BoolChannels = FTLLRN_ControlRigSequencerHelpers::GetBoolChannels(TLLRN_ControlRig, ControlElement->GetKey().Name, Section);
		for (FMovieSceneChannel* Channel : BoolChannels)
		{
			SectionKeyedStatus = GetChannelKeyStatus(Channel, SectionKeyedStatus, Range, EmptyChannelCount);
		}
		break;
	}
	case ETLLRN_RigControlType::Integer:
	{
		TArrayView<FMovieSceneIntegerChannel*> IntegarChannels = FTLLRN_ControlRigSequencerHelpers::GetIntegerChannels(TLLRN_ControlRig, ControlElement->GetKey().Name, Section);
		for (FMovieSceneChannel* Channel : IntegarChannels)
		{
			SectionKeyedStatus = GetChannelKeyStatus(Channel, SectionKeyedStatus, Range, EmptyChannelCount);
		}
		TArrayView<FMovieSceneByteChannel*>  EnumChannels = FTLLRN_ControlRigSequencerHelpers::GetByteChannels(TLLRN_ControlRig, ControlElement->GetKey().Name, Section);
		for (FMovieSceneChannel* Channel : EnumChannels)
		{
			SectionKeyedStatus = GetChannelKeyStatus(Channel, SectionKeyedStatus, Range, EmptyChannelCount);
		}
		break;
	}
	case ETLLRN_RigControlType::Position:
	case ETLLRN_RigControlType::Transform:
	case ETLLRN_RigControlType::TransformNoScale:
	case ETLLRN_RigControlType::EulerTransform:
	case ETLLRN_RigControlType::Float:
	case ETLLRN_RigControlType::ScaleFloat:
	case ETLLRN_RigControlType::Vector2D:
	{
		int32 IChannelToKey = (int32)ChannelToKey;
		TArrayView<FMovieSceneFloatChannel*> FloatChannels = FTLLRN_ControlRigSequencerHelpers::GetFloatChannels(TLLRN_ControlRig, ControlElement->GetKey().Name, Section);
		int32 Num = 0;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::TranslationX))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::TranslationY))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::TranslationZ))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::RotationX))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::RotationY))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::RotationZ))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::ScaleX))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::ScaleY))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::ScaleZ))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		break;
	}
	case ETLLRN_RigControlType::Scale:
	{

		int32 IChannelToKey = (int32)ChannelToKey;
		TArrayView<FMovieSceneFloatChannel*> FloatChannels = FTLLRN_ControlRigSequencerHelpers::GetFloatChannels(TLLRN_ControlRig, ControlElement->GetKey().Name, Section);
		int32 Num = 0;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::ScaleX))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::ScaleY))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::ScaleZ))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		break;
	}
	case ETLLRN_RigControlType::Rotator:
	{
		int32 IChannelToKey = (int32)ChannelToKey;
		TArrayView<FMovieSceneFloatChannel*> FloatChannels = FTLLRN_ControlRigSequencerHelpers::GetFloatChannels(TLLRN_ControlRig, ControlElement->GetKey().Name, Section);
		int32 Num = 0;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::RotationX))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::RotationY))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		++Num;
		if (FloatChannels.Num() > Num)
		{
			if (IChannelToKey & int32(ETLLRN_ControlRigContextChannelToKey::RotationZ))
			{
				SectionKeyedStatus = GetChannelKeyStatus(FloatChannels[Num], SectionKeyedStatus, Range, EmptyChannelCount);
			}
		}
		else
		{
			break;
		}
		break;
	}
	}
	if (EmptyChannelCount == 0 && SectionKeyedStatus == EPropertyKeyedStatus::PartiallyKeyed)
	{
		SectionKeyedStatus = EPropertyKeyedStatus::KeyedInFrame;
	}
	return SectionKeyedStatus;
}

static EPropertyKeyedStatus GetKeyedStatusInTrack(const UTLLRN_ControlRig* TLLRN_ControlRig, const FName& ControlName, UMovieSceneTLLRN_ControlRigParameterTrack* Track, const TRange<FFrameNumber>& Range, ETLLRN_ControlRigContextChannelToKey ChannelToKey)
{
	EPropertyKeyedStatus SectionKeyedStatus = EPropertyKeyedStatus::NotKeyed;
	int32 EmptyChannelCount = 0;
	const FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig ? TLLRN_ControlRig->FindControl(ControlName) : nullptr;
	if (ControlElement == nullptr)
	{
		return SectionKeyedStatus;
	}

	for (UMovieSceneSection* BaseSection : Track->GetAllSections())
	{
		UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(BaseSection);
		if (!Section)
		{
			continue;
		}
		EPropertyKeyedStatus NewSectionKeyedStatus = GetKeyedStatusInSection(TLLRN_ControlRig, ControlName, Section, Range, ChannelToKey);
		SectionKeyedStatus = FMath::Max(SectionKeyedStatus, NewSectionKeyedStatus);

		// Maximum Status Reached no need to iterate further
		if (SectionKeyedStatus == EPropertyKeyedStatus::KeyedInFrame)
		{
			return SectionKeyedStatus;
		}
	}

	return SectionKeyedStatus;
}

static EPropertyKeyedStatus GetKeyedStatusInSection(const UMovieSceneSection* Section, const TRange<FFrameNumber>& Range,
	ETLLRN_ControlRigContextChannelToKey ChannelToKey, int32 MaxNumIndices)
{
	EPropertyKeyedStatus SectionKeyedStatus = EPropertyKeyedStatus::NotKeyed;

	FMovieSceneChannelProxy& ChannelProxy = Section->GetChannelProxy();

	int32 EmptyChannelCount = 0;
	TArray<int32, TFixedAllocator<3>> ChannelIndices;
	switch (ChannelToKey)
	{
	case ETLLRN_ControlRigContextChannelToKey::Translation:
		ChannelIndices = { 0, 1, 2 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::TranslationX:
		ChannelIndices = { 0 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::TranslationY:
		ChannelIndices = { 1 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::TranslationZ:
		ChannelIndices = { 2 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::Rotation:
		ChannelIndices = { 3, 4, 5 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::RotationX:
		ChannelIndices = { 3 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::RotationY:
		ChannelIndices = { 4 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::RotationZ:
		ChannelIndices = { 5 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::Scale:
		ChannelIndices = { 6, 7, 8 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::ScaleX:
		ChannelIndices = { 6 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::ScaleY:
		ChannelIndices = { 7 };
		break;
	case ETLLRN_ControlRigContextChannelToKey::ScaleZ:
		ChannelIndices = { 8 };
		break;
	}
	for (const FMovieSceneChannelEntry& ChannelEntry : ChannelProxy.GetAllEntries())
	{
		if (ChannelEntry.GetChannelTypeName() != FMovieSceneDoubleChannel::StaticStruct()->GetFName() &&
			ChannelEntry.GetChannelTypeName() != FMovieSceneFloatChannel::StaticStruct()->GetFName() &&
			ChannelEntry.GetChannelTypeName() != FMovieSceneBoolChannel::StaticStruct()->GetFName() &&
			ChannelEntry.GetChannelTypeName() != FMovieSceneIntegerChannel::StaticStruct()->GetFName() &&
			ChannelEntry.GetChannelTypeName() != FMovieSceneByteChannel::StaticStruct()->GetFName())
		{
			continue;
		}

		TConstArrayView<FMovieSceneChannel*> Channels = ChannelEntry.GetChannels();

		int32 ChannelIndex = 0;
		for (FMovieSceneChannel* Channel : Channels)
		{
			if (ChannelIndex >= MaxNumIndices)
			{
				break;
			}
			if (ChannelIndices.Contains(ChannelIndex++) == false)
			{
				continue;
			}
			if (Channel->GetNumKeys() == 0)
			{
				++EmptyChannelCount;
				continue;
			}

			SectionKeyedStatus = FMath::Max(SectionKeyedStatus, EPropertyKeyedStatus::KeyedInOtherFrame);

			TArray<FFrameNumber> KeyTimes;
			Channel->GetKeys(Range, &KeyTimes, nullptr);
			if (KeyTimes.IsEmpty())
			{
				++EmptyChannelCount;
			}
			else
			{
				SectionKeyedStatus = FMath::Max(SectionKeyedStatus, EPropertyKeyedStatus::PartiallyKeyed);
			}
		}
		break; //just do it for one type
	}

	if (EmptyChannelCount == 0 && SectionKeyedStatus == EPropertyKeyedStatus::PartiallyKeyed)
	{
		SectionKeyedStatus = EPropertyKeyedStatus::KeyedInFrame;
	}
	return SectionKeyedStatus;
}

static EPropertyKeyedStatus GetKeyedStatusInTrack(const UMovieScenePropertyTrack* Track, const TRange<FFrameNumber>& Range,
	ETLLRN_ControlRigContextChannelToKey ChannelToKey, int32 MaxNumIndices)
{
	EPropertyKeyedStatus SectionKeyedStatus = EPropertyKeyedStatus::NotKeyed;
	for (UMovieSceneSection* BaseSection : Track->GetAllSections())
	{
		if (!BaseSection)
		{
			continue;
		}
		EPropertyKeyedStatus NewSectionKeyedStatus = GetKeyedStatusInSection(BaseSection, Range, ChannelToKey, MaxNumIndices);
		SectionKeyedStatus = FMath::Max(SectionKeyedStatus, NewSectionKeyedStatus);

		// Maximum Status Reached no need to iterate further
		if (SectionKeyedStatus == EPropertyKeyedStatus::KeyedInFrame)
		{
			return SectionKeyedStatus;
		}
	}

	return SectionKeyedStatus;
}

EPropertyKeyedStatus UTLLRN_AnimDetailControlsKeyedProxy::GetPropertyKeyedStatus(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& PropertyHandle) const
{
	EPropertyKeyedStatus KeyedStatus = EPropertyKeyedStatus::NotKeyed;

	if (Sequencer.IsValid() == false || Sequencer->GetFocusedMovieSceneSequence() == nullptr)
	{
		return KeyedStatus;
	}
	const TRange<FFrameNumber> FrameRange = TRange<FFrameNumber>(Sequencer->GetLocalTime().Time.FrameNumber);
	FName PropertyName = PropertyHandle.GetProperty()->GetFName();
	ETLLRN_ControlRigContextChannelToKey ChannelToKey = GetChannelToKeyFromPropertyName(PropertyName);
	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			UMovieSceneTLLRN_ControlRigParameterTrack* Track = FTLLRN_ControlRigSequencerHelpers::FindTLLRN_ControlRigTrack(Sequencer->GetFocusedMovieSceneSequence(), TLLRN_ControlRig);
			if (Track == nullptr)
			{
				continue;
			}
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					EPropertyKeyedStatus NewKeyedStatus = GetKeyedStatusInTrack(TLLRN_ControlRig, ControlElement->GetKey().Name, Track, FrameRange, ChannelToKey);
					KeyedStatus = FMath::Max(KeyedStatus, NewKeyedStatus);
				}
			}
		}
	}
	int32 MaxNumIndices = 1;
	if (IsA<UTLLRN_AnimDetailControlsProxyTransform>())
	{
		MaxNumIndices = 9;
	}
	else if (IsA<UTLLRN_AnimDetailControlsProxyVector2D>())
	{
		MaxNumIndices = 2;
	}
	for (const TPair<TWeakObjectPtr<UObject>, FSequencerProxyItem>& Items : SequencerItems)
	{
		for (const FBindingAndTrack& Element : Items.Value.Bindings)
		{
			UMovieScenePropertyTrack* Track = Cast<UMovieScenePropertyTrack>(Element.WeakTrack.Get());
			if (Track == nullptr)
			{
				continue;
			}
			EPropertyKeyedStatus NewKeyedStatus = GetKeyedStatusInTrack(Track, FrameRange, ChannelToKey, MaxNumIndices);
			KeyedStatus = FMath::Max(KeyedStatus, NewKeyedStatus);
		}
	}
	return KeyedStatus;
}

#if WITH_EDITOR
void UTLLRN_AnimDetailControlsKeyedProxy::PostEditUndo()
{
	if (UWorld* World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr)
	{
		const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(World);
		Controller.EvaluateAllConstraints();
	}

	static const FTLLRN_RigControlModifiedContext ContextNoKey(ETLLRN_ControlRigSetKey::Never);
	
	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		const FTLLRN_ControlRigProxyItem& ProxyItem = Items.Value;
		if (UTLLRN_ControlRig* TLLRN_ControlRig = ProxyItem.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : ProxyItem.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = ProxyItem.GetControlElement(CName))
				{
					TLLRN_ControlRig->SelectControl(ControlElement->GetKey().Name, bSelected);
					SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(TLLRN_ControlRig, ControlElement, ContextNoKey);
				}
			}
		}
	}
	
	for (TPair <TWeakObjectPtr<UObject>, FSequencerProxyItem>& SItems : SequencerItems)
	{
		//we do this backwards so ValueChanged later is set up correctly since that iterates in the other direction
		TWeakObjectPtr<UObject> WeakObject = SItems.Key;
		if (WeakObject.IsValid())
		{
			UObject* Object = WeakObject.Get();
			for (FBindingAndTrack& Binding : SItems.Value.Bindings)
			{
				SetBindingValueFromCurrent(Object, Binding.Binding, ContextNoKey);
			}
		}
	}
	
	ValueChanged();
}
#endif

//////UTLLRN_AnimDetailControlsProxyTransform////////


static void SetValuesFromContext(const FEulerTransform& EulerTransform, const FTLLRN_RigControlModifiedContext& Context, FVector& TLocation, FRotator& TRotation, FVector& TScale)
{
	ETLLRN_ControlRigContextChannelToKey ChannelsToKey = (ETLLRN_ControlRigContextChannelToKey)Context.KeyMask;
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationX) == false)
	{
		TLocation.X = EulerTransform.Location.X;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationY) == false)
	{
		TLocation.Y = EulerTransform.Location.Y;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationZ) == false)
	{
		TLocation.Z = EulerTransform.Location.Z;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationX) == false)
	{
		TRotation.Roll = EulerTransform.Rotation.Roll;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationZ) == false)
	{
		TRotation.Yaw = EulerTransform.Rotation.Yaw;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationY) == false)
	{
		TRotation.Pitch = EulerTransform.Rotation.Pitch;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleX) == false)
	{
		TScale.X = EulerTransform.Scale.X;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleY) == false)
	{
		TScale.Y = EulerTransform.Scale.Y;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleZ) == false)
	{
		TScale.Z = EulerTransform.Scale.Z;
	}
}

static FEulerTransform GetCurrentValue(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement)
{
	FEulerTransform EulerTransform = FEulerTransform::Identity;

	switch (ControlElement->Settings.ControlType)
	{
		case ETLLRN_RigControlType::Transform:
		{
			const FTransform NewTransform = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
			EulerTransform = FEulerTransform(NewTransform);
			break;

		}
		case ETLLRN_RigControlType::TransformNoScale:
		{
			const FTransformNoScale NewTransform = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FTLLRN_RigControlValue::FTransformNoScale_Float>().ToTransform();
			EulerTransform.Location = NewTransform.Location;
			EulerTransform.Rotation = FRotator(NewTransform.Rotation);
			break;

		}
		case ETLLRN_RigControlType::EulerTransform:
		{
			EulerTransform = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform();
			break;
		}
	};
	if (TLLRN_ControlRig->GetHierarchy()->UsesPreferredEulerAngles())
	{
		EulerTransform.Rotation = TLLRN_ControlRig->GetHierarchy()->GetControlPreferredRotator(ControlElement);
	}
	return EulerTransform;
}

static void GetActorAndSceneComponentFromObject(UObject* Object, AActor*& OutActor, USceneComponent*& OutSceneComponent)
{
	OutActor = Cast<AActor>(Object);
	if (OutActor != nullptr && OutActor->GetRootComponent())
	{
		OutSceneComponent = OutActor->GetRootComponent();
	}
	else
	{
		// If the object wasn't an actor attempt to get it directly as a scene component and then get the actor from there.
		OutSceneComponent = Cast<USceneComponent>(Object);
		if (OutSceneComponent != nullptr)
		{
			OutActor = Cast<AActor>(OutSceneComponent->GetOuter());
		}
	}
}

static FEulerTransform GetCurrentValue(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding)
{
	const FStructProperty* TransformProperty = Binding.IsValid() ? CastField<FStructProperty>(Binding->GetProperty(*InObject)) : nullptr;
	if (TransformProperty)
	{
		if (TransformProperty->Struct == TBaseStructure<FTransform>::Get())
		{
			if (TOptional<FTransform> Transform = Binding->GetOptionalValue<FTransform>(*InObject))
			{
				return FEulerTransform(Transform.GetValue());
			}
		}
		else if (TransformProperty->Struct == TBaseStructure<FEulerTransform>::Get())
		{
			if (TOptional<FEulerTransform> EulerTransform = Binding->GetOptionalValue<FEulerTransform>(*InObject))
			{
				return EulerTransform.GetValue();
			}
		}
	}
	AActor* ActorThatChanged = nullptr;
	USceneComponent* SceneComponentThatChanged = nullptr;
	GetActorAndSceneComponentFromObject(InObject, ActorThatChanged, SceneComponentThatChanged);

	FEulerTransform EulerTransform = FEulerTransform::Identity;
	if (SceneComponentThatChanged)
	{
		EulerTransform.Location = SceneComponentThatChanged->GetRelativeLocation();
		EulerTransform.Rotation = SceneComponentThatChanged->GetRelativeRotation();
		EulerTransform.Scale = SceneComponentThatChanged->GetRelativeScale3D();
	}
	return EulerTransform;
}

void UTLLRN_AnimDetailControlsProxyTransform::SetBindingValueFromCurrent(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding, const FTLLRN_RigControlModifiedContext& Context, bool bInteractive)
{
	if (InObject && Binding.IsValid())
	{
		FVector TLocation = Location.ToVector();
		FRotator TRotation = Rotation.ToRotator();
		FVector TScale = Scale.ToVector();
		FEulerTransform EulerTransform = GetCurrentValue(InObject, Binding);
		SetValuesFromContext(EulerTransform, Context, TLocation, TRotation, TScale);
		FTransform RealTransform(TRotation, TLocation, TScale);

		const FStructProperty* TransformProperty = Binding.IsValid() ? CastField<FStructProperty>(Binding->GetProperty(*InObject)) : nullptr;
		if (TransformProperty)
		{
			if (TransformProperty->Struct == TBaseStructure<FEulerTransform>::Get())
			{
				EulerTransform = FEulerTransform(TLocation, TRotation, TScale);
				Binding->SetCurrentValue<FTransform>(*InObject, RealTransform);
			}
		}
		AActor* ActorThatChanged = nullptr;
		USceneComponent* SceneComponentThatChanged = nullptr;
		GetActorAndSceneComponentFromObject(InObject, ActorThatChanged, SceneComponentThatChanged);
		if (SceneComponentThatChanged)
		{
			FProperty* ValueProperty = nullptr;
			FProperty* AxisProperty = nullptr;
			if (Context.SetKey != ETLLRN_ControlRigSetKey::Never)
			{
				ETLLRN_ControlRigContextChannelToKey ChannelsToKey = (ETLLRN_ControlRigContextChannelToKey)Context.KeyMask;
				if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationX) )
				{
					ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeLocationPropertyName());
					AxisProperty = FindFProperty<FDoubleProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, X));
				}
				if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationY))
				{
					ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeLocationPropertyName());
					AxisProperty = FindFProperty<FDoubleProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, Y));
				}
				if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationZ))
				{
					ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeLocationPropertyName());
					AxisProperty = FindFProperty<FDoubleProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, Z));
				}

				if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationX))
				{
					ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeRotationPropertyName());
					AxisProperty = FindFProperty<FDoubleProperty>(TBaseStructure<FRotator>::Get(), GET_MEMBER_NAME_CHECKED(FRotator, Roll));
				}
				if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationY))
				{
					ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeRotationPropertyName());
					AxisProperty = FindFProperty<FDoubleProperty>(TBaseStructure<FRotator>::Get(), GET_MEMBER_NAME_CHECKED(FRotator, Pitch));
				}
				if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationZ))
				{
					ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeRotationPropertyName());
					AxisProperty = FindFProperty<FDoubleProperty>(TBaseStructure<FRotator>::Get(), GET_MEMBER_NAME_CHECKED(FRotator, Yaw));
				}
				if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleX))
				{
					ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeScale3DPropertyName());
					AxisProperty = FindFProperty<FDoubleProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, X));
				}
				if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleY))
				{
					ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeScale3DPropertyName());
					AxisProperty = FindFProperty<FDoubleProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, Y));
				}
				if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleZ))
				{
					ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeScale3DPropertyName());
					AxisProperty = FindFProperty<FDoubleProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, Z));
				}
			}

		
			// Have to downcast here because of function overloading and inheritance not playing nicely
			if (ValueProperty)
			{
				TArray<UObject*> ModifiedObjects = { InObject };
				FPropertyChangedEvent PropertyChangedEvent(ValueProperty, bInteractive ? EPropertyChangeType::Interactive : EPropertyChangeType::ValueSet, MakeArrayView(ModifiedObjects));
				FEditPropertyChain PropertyChain;
				if (AxisProperty)
				{
					PropertyChain.AddHead(AxisProperty);
				}
				PropertyChain.AddHead(ValueProperty);
				FPropertyChangedChainEvent PropertyChangedChainEvent(PropertyChain, PropertyChangedEvent);
				((UObject*)SceneComponentThatChanged)->PreEditChange(PropertyChain);
			
				if (ActorThatChanged && ActorThatChanged->GetRootComponent() == SceneComponentThatChanged)
				{
					((UObject*)ActorThatChanged)->PreEditChange(PropertyChain);
				}
			}
			SceneComponentThatChanged->SetRelativeTransform(RealTransform, false, nullptr, ETeleportType::None);
			// Force the location and rotation values to avoid Rot->Quat->Rot conversions
			SceneComponentThatChanged->SetRelativeLocation_Direct(TLocation);
			SceneComponentThatChanged->SetRelativeRotationExact(TRotation);

			if (ValueProperty)
			{
				TArray<UObject*> ModifiedObjects = { InObject };
				FPropertyChangedEvent PropertyChangedEvent(ValueProperty, bInteractive ? EPropertyChangeType::Interactive : EPropertyChangeType::ValueSet, MakeArrayView(ModifiedObjects));
				FEditPropertyChain PropertyChain;
				if (AxisProperty)
				{
					PropertyChain.AddHead(AxisProperty);
				}
				PropertyChain.AddHead(ValueProperty);
				FPropertyChangedChainEvent PropertyChangedChainEvent(PropertyChain, PropertyChangedEvent);
				((UObject*)SceneComponentThatChanged)->PostEditChangeChainProperty(PropertyChangedChainEvent);
				if (ActorThatChanged && ActorThatChanged->GetRootComponent() == SceneComponentThatChanged)
				{
					((UObject*)ActorThatChanged)->PostEditChangeChainProperty(PropertyChangedChainEvent);
				}
			}
			GUnrealEd->UpdatePivotLocationForSelection();
		}
	}
}

static FTransform GetTLLRN_ControlRigComponentTransform(UTLLRN_ControlRig* TLLRN_ControlRig)
{
	FTransform Transform = FTransform::Identity;
	TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding();
	if (ObjectBinding.IsValid())
	{
		if (USceneComponent* BoundSceneComponent = Cast<USceneComponent>(ObjectBinding->GetBoundObject()))
		{
			return BoundSceneComponent->GetComponentTransform();
		}
	}
	return Transform;
}

static bool SetConstrainedTransform(FTransform LocalTransform, UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& InContext)
{
	const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(TLLRN_ControlRig->GetWorld());
	const uint32 ControlHash = UTLLRN_TransformableControlHandle::ComputeHash(TLLRN_ControlRig, ControlElement->GetFName());
	const TArray< TWeakObjectPtr<UTickableConstraint> > Constraints = Controller.GetParentConstraints(ControlHash, true);
	if (Constraints.IsEmpty())
	{
		return false;
	}
	const int32 LastActiveIndex = FTransformConstraintUtils::GetLastActiveConstraintIndex(Constraints);
	const bool bNeedsConstraintPostProcess = Constraints.IsValidIndex(LastActiveIndex);

	if (!bNeedsConstraintPostProcess)
	{
		return false;
	}
	static constexpr bool bNotify = true, bFixEuler = true, bUndo = true;
	FTLLRN_RigControlModifiedContext Context = InContext;
	Context.EventName = FTLLRN_RigUnit_BeginExecution::EventName;
	Context.bConstraintUpdate = true;
	Context.SetKey = ETLLRN_ControlRigSetKey::Never;

	// set the global space, assumes it's attached to actor
	// no need to compensate for constraints here, this will be done after when setting the control in the constraint space
	{
		TGuardValue<bool> CompensateGuard(FMovieSceneConstraintChannelHelper::bDoNotCompensate, true);
		TLLRN_ControlRig->SetControlLocalTransform(
			ControlElement->GetKey().Name, LocalTransform, bNotify, Context, bUndo, bFixEuler);
	}
	FTransform GlobalTransform = TLLRN_ControlRig->GetControlGlobalTransform(ControlElement->GetKey().Name);

	// switch to constraint space
	FTransform ToWorldTransform = GetTLLRN_ControlRigComponentTransform(TLLRN_ControlRig);
	const FTransform WorldTransform = GlobalTransform * ToWorldTransform;

	const TOptional<FTransform> RelativeTransform =
		FTransformConstraintUtils::GetConstraintsRelativeTransform(Constraints, LocalTransform, WorldTransform);
	if (RelativeTransform)
	{
		LocalTransform = *RelativeTransform;
	}

	Context.bConstraintUpdate = false;
	Context.SetKey = InContext.SetKey;
	TLLRN_ControlRig->SetControlLocalTransform(ControlElement->GetKey().Name, LocalTransform, bNotify, Context, bUndo, bFixEuler);
	TLLRN_ControlRig->Evaluate_AnyThread();
	Controller.EvaluateAllConstraints();

	return true;
}


void UTLLRN_AnimDetailControlsProxyTransform::SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if (ControlElement && TLLRN_ControlRig)
	{
		FVector TLocation = Location.ToVector();
		FRotator TRotation = Rotation.ToRotator();
		FVector TScale = Scale.ToVector();
		FEulerTransform EulerTransform = GetCurrentValue(TLLRN_ControlRig, ControlElement);
		SetValuesFromContext(EulerTransform, Context, TLocation, TRotation, TScale);
		//constraints we just deal with FTransforms unfortunately, need to figure out how to handle rotation orders
		FTransform RealTransform(TRotation, TLocation, TScale);
		if (SetConstrainedTransform(RealTransform, TLLRN_ControlRig, ControlElement,Context))
		{
			ValueChanged();
			return;
		}
		switch (ControlElement->Settings.ControlType)
		{
		case ETLLRN_RigControlType::Transform:
		{
			FVector EulerAngle(TRotation.Roll, TRotation.Pitch, TRotation.Yaw);
			TLLRN_ControlRig->GetHierarchy()->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
			TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransform_Float>(ControlElement->GetKey().Name, RealTransform, true, Context, false);
			TLLRN_ControlRig->GetHierarchy()->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
			break;

		}
		case ETLLRN_RigControlType::TransformNoScale:
		{
			FTransformNoScale NoScale(TLocation, TRotation.Quaternion());
			TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransformNoScale_Float>(ControlElement->GetKey().Name, NoScale, true, Context, false);
			break;

		}
		case ETLLRN_RigControlType::EulerTransform:
		{
			UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();

			if (Hierarchy->UsesPreferredEulerAngles())
			{
				FVector EulerAngle(TRotation.Roll, TRotation.Pitch, TRotation.Yaw);
				FQuat Quat = Hierarchy->GetControlQuaternion(ControlElement, EulerAngle);
				Hierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
				FRotator UERotator(Quat);
				FEulerTransform UETransform(UERotator, TLocation, TScale);
				UETransform.Rotation = UERotator;
				TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FEulerTransform_Float>(ControlElement->GetKey().Name, UETransform, true, Context, false);
				Hierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
			}
			else
			{
				TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FEulerTransform_Float>(ControlElement->GetKey().Name, FEulerTransform(RealTransform), true, Context, false);
			}
			break;
		}
		}
		TLLRN_ControlRig->Evaluate_AnyThread();
	}
}

bool UTLLRN_AnimDetailControlsProxyTransform::PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) 
{
	return ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Location)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Location)) ||
		(Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Rotation)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Rotation)) ||
		(Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Scale)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Scale))
		);
}

void UTLLRN_AnimDetailControlsProxyTransform::ClearMultipleFlags()
{
	Location.State.bXMultiple = false;
	Location.State.bYMultiple = false;
	Location.State.bZMultiple = false;
	Rotation.State.bXMultiple = false;
	Rotation.State.bYMultiple = false;
	Rotation.State.bZMultiple = false;
	Scale.State.bXMultiple = false;
	Scale.State.bYMultiple = false;
	Scale.State.bZMultiple = false;
}

void UTLLRN_AnimDetailControlsProxyTransform::SetMultipleFlags(const FEulerTransform& ValA, const FEulerTransform& ValB)
{
	if (FMath::IsNearlyEqual(ValA.Location.X, ValB.Location.X) == false)
	{
		Location.State.bXMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Location.Y, ValB.Location.Y) == false)
	{
		Location.State.bYMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Location.Z, ValB.Location.Z) == false)
	{
		Location.State.bZMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Rotation.Roll, ValB.Rotation.Roll) == false)
	{
		Rotation.State.bXMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Rotation.Pitch, ValB.Rotation.Pitch) == false)
	{
		Rotation.State.bYMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Rotation.Yaw, ValB.Rotation.Yaw) == false)
	{
		Rotation.State.bZMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Scale.X, ValB.Scale.X) == false)
	{
		Scale.State.bXMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Scale.Y, ValB.Scale.Y) == false)
	{
		Scale.State.bYMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Scale.Z, ValB.Scale.Z) == false)
	{
		Scale.State.bZMultiple = true;
	}
}

void UTLLRN_AnimDetailControlsProxyTransform::ValueChanged()
{
	TOptional<FEulerTransform> LastEulerTransform;
	ClearMultipleFlags();

	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					FEulerTransform EulerTransform = GetCurrentValue(TLLRN_ControlRig, ControlElement);
					if (LastEulerTransform.IsSet())
					{
						const FEulerTransform LastVal = LastEulerTransform.GetValue();
						SetMultipleFlags(LastVal, EulerTransform);
					}
					else
					{
						LastEulerTransform = EulerTransform;
					}
				}
			}
		}
	}
	for (TPair <TWeakObjectPtr<UObject>, FSequencerProxyItem>& SItems : SequencerItems)
	{
		if (SItems.Key.IsValid() == false)
		{
			continue;
		}
		//we do this backwards so ValueChanged later is set up correctly since that iterates in the other direction
		for (int32 Index = SItems.Value.Bindings.Num() - 1; Index >= 0; --Index)
		{
			FBindingAndTrack& Binding = SItems.Value.Bindings[Index];
			if (Binding.Binding.IsValid())
			{
				FEulerTransform EulerTransform = GetCurrentValue(SItems.Key.Get(), Binding.Binding);
				if (LastEulerTransform.IsSet())
				{
					const FEulerTransform LastVal = LastEulerTransform.GetValue();
					SetMultipleFlags(LastVal, EulerTransform);
				}
				else
				{
					LastEulerTransform = EulerTransform;
				}
			}
		}
	}

	const FEulerTransform LastVal = LastEulerTransform.IsSet() ? LastEulerTransform.GetValue() : FEulerTransform::Identity;

	//set from values, note that the fact if they were multiples or not was already set so we need to set them on these values.
	FTLLRN_AnimDetailProxyLocation TLocation(LastVal.Location, Location.State);
	FTLLRN_AnimDetailProxyRotation TRotation(LastVal.Rotation, Rotation.State);
	FTLLRN_AnimDetailProxyScale TScale(LastVal.Scale, Scale.State);

	const FName LocationName("Location");
	FTrackInstancePropertyBindings LocationBinding(LocationName, LocationName.ToString());
	LocationBinding.CallFunction<FTLLRN_AnimDetailProxyLocation>(*this, TLocation);

	const FName RotationName("Rotation");
	FTrackInstancePropertyBindings RotationBinding(RotationName, RotationName.ToString());
	RotationBinding.CallFunction<FTLLRN_AnimDetailProxyRotation>(*this, TRotation);

	const FName ScaleName("Scale");
	FTrackInstancePropertyBindings ScaleBinding(ScaleName, ScaleName.ToString());
	ScaleBinding.CallFunction<FTLLRN_AnimDetailProxyScale>(*this, TScale);
}

bool UTLLRN_AnimDetailControlsProxyTransform::IsMultiple(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX))
	{
		return Location.State.bXMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY))
	{
		return Location.State.bYMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ))
	{
		return Location.State.bZMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX))
	{
		return Rotation.State.bXMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY))
	{
		return Rotation.State.bYMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ))
	{
		return Rotation.State.bZMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX))
	{
		return Scale.State.bXMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY))
	{
		return Scale.State.bYMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ))
	{
		return Scale.State.bZMultiple;
	}

	return false;
}

TMap<FName, int32> UTLLRN_AnimDetailControlsProxyTransform::GetPropertyNames() const
{
	TMap<FName, int32> NameToIndex;
	FName PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX);
	int32 Index = 0;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ);
	++Index;
	NameToIndex.Add(PropertyName, Index);

	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ);
	++Index;
	NameToIndex.Add(PropertyName, Index);

	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	return NameToIndex;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyTransform::GetChannelToKeyFromPropertyName(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Location))
	{
		return ETLLRN_ControlRigContextChannelToKey::Translation;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationY;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationZ;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Rotation))
	{
		return ETLLRN_ControlRigContextChannelToKey::Rotation;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationX;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationY;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationZ;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Scale))
	{
		return ETLLRN_ControlRigContextChannelToKey::Scale;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleX;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleY;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleZ;
	}

	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyTransform::GetChannelToKeyFromChannelName(const FString& InChannelName) const
{
	if (InChannelName == TEXT("Location.X"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	else if (InChannelName == TEXT("Location.Y"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationY;
	}
	else if (InChannelName == TEXT("Location.Z"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationZ;
	}
	else if (InChannelName == TEXT("Rotation.X") || InChannelName == TEXT("Rotation.Roll"))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationX;
	}
	else if (InChannelName == TEXT("Rotation.Y") || InChannelName == TEXT("Rotation.Pitch"))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationY;
	}
	else if (InChannelName == TEXT("Rotation.Z") || InChannelName == TEXT("Rotation.Yaw"))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationZ;
	}
	else if (InChannelName == TEXT("Scale.X"))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleX;
	}
	else if (InChannelName == TEXT("Scale.Y"))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleY;
	}
	else if (InChannelName == TEXT("Scale.Z"))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleZ;
	}
	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

void UTLLRN_AnimDetailControlsProxyTransform::GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& LocationSelectionCache, FTLLRN_AnimDetailVectorSelection& RotationSelectionCache,
	FTLLRN_AnimDetailVectorSelection& ScaleSelectionCache)
{
	LocationSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX));
	LocationSelectionCache.YSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY));
	LocationSelectionCache.ZSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ));

	RotationSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX));
	RotationSelectionCache.YSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY));
	RotationSelectionCache.ZSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ));

	ScaleSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX));
	ScaleSelectionCache.YSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY));
	ScaleSelectionCache.ZSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ));
}


//////UTLLRN_AnimDetailControlsProxyLocation////////
static void SetLocationValuesFromContext(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context, FVector3f& TLocation)
{
	FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
	FVector3f Value = ControlValue.Get<FVector3f>();

	ETLLRN_ControlRigContextChannelToKey ChannelsToKey = (ETLLRN_ControlRigContextChannelToKey)Context.KeyMask;
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationX) == false)
	{
		TLocation.X = Value.X;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationY) == false)
	{
		TLocation.Y = Value.Y;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationZ) == false)
	{
		TLocation.Z = Value.Z;
	}
}

void UTLLRN_AnimDetailControlsProxyLocation::SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if (ControlElement && TLLRN_ControlRig && ControlElement->Settings.ControlType == ETLLRN_RigControlType::Position)
	{
		FVector3f TLocation = Location.ToVector3f();
		SetLocationValuesFromContext(TLLRN_ControlRig, ControlElement, Context, TLocation);

		TLLRN_ControlRig->SetControlValue<FVector3f>(ControlElement->GetKey().Name, TLocation, true, Context, false);
		TLLRN_ControlRig->Evaluate_AnyThread();
	}
}

bool UTLLRN_AnimDetailControlsProxyLocation::PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty)
{
	return ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyLocation, Location)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyLocation, Location))
		);
}

void UTLLRN_AnimDetailControlsProxyLocation::ClearMultipleFlags()
{
	Location.State.bXMultiple = false;
	Location.State.bYMultiple = false;
	Location.State.bZMultiple = false;
}

void UTLLRN_AnimDetailControlsProxyLocation::SetMultipleFlags(const FVector3f& ValA, const FVector3f& ValB)
{
	if (FMath::IsNearlyEqual(ValA.X, ValB.X) == false)
	{
		Location.State.bXMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Y, ValB.Y) == false)
	{
		Location.State.bYMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Z, ValB.Z) == false)
	{
		Location.State.bZMultiple = true;
	}
}

void UTLLRN_AnimDetailControlsProxyLocation::ValueChanged()
{
	TOptional<FVector3f> LastValue;
	ClearMultipleFlags();

	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					FVector3f Value = FVector3f::ZeroVector;
					if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Position)
					{
						FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
						Value = ControlValue.Get<FVector3f>();
					}
					if (LastValue.IsSet())
					{
						const FVector3f LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value);
					}
					else
					{
						LastValue = Value;
					}
				}
			}
		}
	}

	const FVector3f LastVal = LastValue.IsSet() ? LastValue.GetValue() : FVector3f::ZeroVector;

	//set from values, note that the fact if they were multiples or not was already set so we need to set them on these values.
	FTLLRN_AnimDetailProxyLocation TLocation(LastVal, Location.State);

	const FName LocationName("Location");
	FTrackInstancePropertyBindings LocationBinding(LocationName, LocationName.ToString());
	LocationBinding.CallFunction<FTLLRN_AnimDetailProxyLocation>(*this, TLocation);

}

bool UTLLRN_AnimDetailControlsProxyLocation::IsMultiple(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX))
	{
		return Location.State.bXMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY))
	{
		return Location.State.bYMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ))
	{
		return Location.State.bZMultiple;
	}

	return false;
}

TMap<FName, int32> UTLLRN_AnimDetailControlsProxyLocation::GetPropertyNames() const
{
	TMap<FName, int32> NameToIndex;
	FName PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX);
	int32 Index = 0;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	return NameToIndex;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyLocation::GetChannelToKeyFromPropertyName(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationY;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationZ;
	}

	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyLocation::GetChannelToKeyFromChannelName(const FString& InChannelName) const
{
	if (InChannelName == TEXT("X"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	else if (InChannelName == TEXT("Y"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationY;
	}
	else if (InChannelName == TEXT("Z"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationZ;
	}
	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

void UTLLRN_AnimDetailControlsProxyLocation::GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& LocationSelectionCache, FTLLRN_AnimDetailVectorSelection& RotationSelectionCache,
	FTLLRN_AnimDetailVectorSelection& ScaleSelectionCache)
{
	LocationSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX));
	LocationSelectionCache.YSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY));
	LocationSelectionCache.ZSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ));
}

//////UTLLRN_AnimDetailControlsProxyRotation////////

static void SetRotationValuesFromContext(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context, FVector3f& Val)
{
	FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
	FVector3f Value = ControlValue.Get<FVector3f>();

	ETLLRN_ControlRigContextChannelToKey ChannelsToKey = (ETLLRN_ControlRigContextChannelToKey)Context.KeyMask;
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationX) == false)
	{
		Val.X = Value.X;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationZ) == false)
	{
		Val.Y = Value.Y;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationY) == false)
	{
		Val.Z = Value.Z;
	}
}

void UTLLRN_AnimDetailControlsProxyRotation::SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if (ControlElement && TLLRN_ControlRig && ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
	{
		FVector3f Val = Rotation.ToVector3f();
		SetRotationValuesFromContext(TLLRN_ControlRig, ControlElement, Context, Val);
		FVector EulerAngle(Rotation.ToRotator().Roll, Rotation.ToRotator().Pitch, Rotation.ToRotator().Yaw);
		TLLRN_ControlRig->GetHierarchy()->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
		TLLRN_ControlRig->SetControlValue<FVector3f>(ControlElement->GetKey().Name, Val, true, Context, false);
		TLLRN_ControlRig->GetHierarchy()->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
		TLLRN_ControlRig->Evaluate_AnyThread();
	}
}

bool UTLLRN_AnimDetailControlsProxyRotation::PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty)
{
	return ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyRotation, Rotation)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyRotation, Rotation))
		);
}

void UTLLRN_AnimDetailControlsProxyRotation::ClearMultipleFlags()
{
	Rotation.State.bXMultiple = false;
	Rotation.State.bYMultiple = false;
	Rotation.State.bZMultiple = false;
}

void UTLLRN_AnimDetailControlsProxyRotation::SetMultipleFlags(const FVector3f& ValA, const FVector3f& ValB)
{
	if (FMath::IsNearlyEqual(ValA.X, ValB.X) == false)
	{
		Rotation.State.bXMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Y, ValB.Y) == false)
	{
		Rotation.State.bYMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Z, ValB.Z) == false)
	{
		Rotation.State.bZMultiple = true;
	}
}

void UTLLRN_AnimDetailControlsProxyRotation::ValueChanged()
{
	TOptional<FVector3f> LastValue;
	ClearMultipleFlags();
	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					FVector3f Value = FVector3f::ZeroVector;
					if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
					{
						FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
						Value = ControlValue.Get<FVector3f>();
					}
					if (LastValue.IsSet())
					{
						const FVector3f LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value);
					}
					else
					{
						LastValue = Value;
					}
				}
			}
		}
	}

	const FVector3f LastVal = LastValue.IsSet() ? LastValue.GetValue() : FVector3f::ZeroVector;

	//set from values, note that the fact if they were multiples or not was already set so we need to set them on these values.
	FTLLRN_AnimDetailProxyRotation Val(LastVal, Rotation.State);

	const FName PropName("Rotation");
	FTrackInstancePropertyBindings Binding(PropName, PropName.ToString());
	Binding.CallFunction<FTLLRN_AnimDetailProxyRotation>(*this, Val);

}

bool UTLLRN_AnimDetailControlsProxyRotation::IsMultiple(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX))
	{
		return Rotation.State.bXMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY))
	{
		return Rotation.State.bYMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ))
	{
		return Rotation.State.bZMultiple;
	}
	return false;
}

TMap<FName, int32> UTLLRN_AnimDetailControlsProxyRotation::GetPropertyNames() const
{
	TMap<FName, int32> NameToIndex;
	FName PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX);
	int32 Index = 0;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	return NameToIndex;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyRotation::GetChannelToKeyFromPropertyName(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationX;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationY;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationZ;
	}

	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyRotation::GetChannelToKeyFromChannelName(const FString& InChannelName) const
{
	if (InChannelName == TEXT("X"))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationX;
	}
	else if (InChannelName == TEXT("Y"))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationY;
	}
	else if (InChannelName == TEXT("Z"))
	{
		return ETLLRN_ControlRigContextChannelToKey::RotationZ;
	}
	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

void UTLLRN_AnimDetailControlsProxyRotation::GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& LocationSelectionCache, FTLLRN_AnimDetailVectorSelection& RotationSelectionCache,
	FTLLRN_AnimDetailVectorSelection& ScaleSelectionCache)
{
	RotationSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX));
	RotationSelectionCache.YSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY));
	RotationSelectionCache.ZSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ));
}

//////UTLLRN_AnimDetailControlsProxyScale////////

static void SetScaleValuesFromContext(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context, FVector3f& TScale)
{
	FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
	FVector3f Value = ControlValue.Get<FVector3f>();

	ETLLRN_ControlRigContextChannelToKey ChannelsToKey = (ETLLRN_ControlRigContextChannelToKey)Context.KeyMask;

	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleX) == false)
	{
		TScale.X = Value.X;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleY) == false)
	{
		TScale.Y = Value.Y;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleZ) == false)
	{
		TScale.Z = Value.Z;
	}
}
void UTLLRN_AnimDetailControlsProxyScale::SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if (ControlElement && TLLRN_ControlRig && ControlElement->Settings.ControlType == ETLLRN_RigControlType::Scale)
	{
		FVector3f Val = Scale.ToVector3f();
		SetScaleValuesFromContext(TLLRN_ControlRig, ControlElement, Context, Val);
		TLLRN_ControlRig->SetControlValue<FVector3f>(ControlElement->GetKey().Name, Val, true, Context, false);
		TLLRN_ControlRig->Evaluate_AnyThread();
	}
}

bool UTLLRN_AnimDetailControlsProxyScale::PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty)
{
	return ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyScale, Scale)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyScale, Scale))
		);
}

void UTLLRN_AnimDetailControlsProxyScale::ClearMultipleFlags()
{
	Scale.State.bXMultiple = false;
	Scale.State.bYMultiple = false;
	Scale.State.bZMultiple = false;
}

void UTLLRN_AnimDetailControlsProxyScale::SetMultipleFlags(const FVector3f& ValA, const FVector3f& ValB)
{
	if (FMath::IsNearlyEqual(ValA.X, ValB.X) == false)
	{
		Scale.State.bXMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Y, ValB.Y) == false)
	{
		Scale.State.bYMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Z, ValB.Z) == false)
	{
		Scale.State.bZMultiple = true;
	}
}

void UTLLRN_AnimDetailControlsProxyScale::ValueChanged()
{
	TOptional<FVector3f> LastValue;
	ClearMultipleFlags();

	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					FVector3f Value = FVector3f::ZeroVector;
					if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Scale)
					{
						FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
						Value = ControlValue.Get<FVector3f>();
					}
					if (LastValue.IsSet())
					{
						const FVector3f LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value);
					}
					else
					{
						LastValue = Value;
					}
				}
			}
		}
	}

	const FVector3f LastVal = LastValue.IsSet() ? LastValue.GetValue() : FVector3f::ZeroVector;
	//set from values, note that the fact if they were multiples or not was already set so we need to set them on these values.
	FTLLRN_AnimDetailProxyScale Value(LastVal, Scale.State);

	const FName PropName("Scale");
	FTrackInstancePropertyBindings Binding(PropName, PropName.ToString());
	Binding.CallFunction<FTLLRN_AnimDetailProxyScale>(*this, Value);

}

bool UTLLRN_AnimDetailControlsProxyScale::IsMultiple(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX))
	{
		return Scale.State.bXMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY))
	{
		return Scale.State.bYMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ))
	{
		return Scale.State.bZMultiple;
	}

	return false;
}

TMap<FName, int32> UTLLRN_AnimDetailControlsProxyScale::GetPropertyNames() const
{
	TMap<FName, int32> NameToIndex;
	FName PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX);
	int32 Index = 0;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	return NameToIndex;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyScale::GetChannelToKeyFromPropertyName(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleX;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleY;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleZ;
	}

	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyScale::GetChannelToKeyFromChannelName(const FString& InChannelName) const
{
	if (InChannelName == TEXT("X"))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleX;
	}
	else if (InChannelName == TEXT("Y"))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleY;
	}
	else if (InChannelName == TEXT("Z"))
	{
		return ETLLRN_ControlRigContextChannelToKey::ScaleZ;
	}
	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

void UTLLRN_AnimDetailControlsProxyScale::GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& LocationSelectionCache, FTLLRN_AnimDetailVectorSelection& RotationSelectionCache,
	FTLLRN_AnimDetailVectorSelection& ScaleSelectionCache)
{
	ScaleSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX));
	ScaleSelectionCache.YSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY));
	ScaleSelectionCache.ZSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ));
}

//////UTLLRN_AnimDetailControlsProxyVector2D////////
static void SetVector2DValuesFromContext(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context, FVector2D& Val)
{
	FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
	FVector3f Value = ControlValue.Get<FVector3f>();

	ETLLRN_ControlRigContextChannelToKey ChannelsToKey = (ETLLRN_ControlRigContextChannelToKey)Context.KeyMask;
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationX) == false)
	{
		Val.X = Value.X;
	}
	if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationY) == false)
	{
		Val.Y = Value.Y;
	}
}
void UTLLRN_AnimDetailControlsProxyVector2D::SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if (ControlElement && TLLRN_ControlRig && ControlElement->Settings.ControlType == ETLLRN_RigControlType::Vector2D)
	{
		FVector2D Val = Vector2D.ToVector2D();
		SetVector2DValuesFromContext(TLLRN_ControlRig, ControlElement, Context, Val);
		TLLRN_ControlRig->SetControlValue<FVector2D>(ControlElement->GetKey().Name, Val, true, Context, false);
		TLLRN_ControlRig->Evaluate_AnyThread();
	}
}

bool UTLLRN_AnimDetailControlsProxyVector2D::PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty)
{
	return ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyVector2D, Vector2D)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyVector2D, Vector2D))
		);
}

void UTLLRN_AnimDetailControlsProxyVector2D::ClearMultipleFlags()
{
	Vector2D.State.bXMultiple = false;
	Vector2D.State.bYMultiple = false;
}

void UTLLRN_AnimDetailControlsProxyVector2D::SetMultipleFlags(const FVector2D& ValA, const FVector2D& ValB)
{
	if (FMath::IsNearlyEqual(ValA.X, ValB.X) == false)
	{
		Vector2D.State.bXMultiple = true;
	}
	if (FMath::IsNearlyEqual(ValA.Y, ValB.Y) == false)
	{
		Vector2D.State.bYMultiple = true;
	}
}

void UTLLRN_AnimDetailControlsProxyVector2D::ValueChanged()
{
	TOptional<FVector2D> LastValue;
	ClearMultipleFlags();

	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					FVector2D Value = FVector2D::ZeroVector;
					if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Vector2D)
					{
						FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
						//FVector2D version deleted for some reason so need to convert
						FVector3f Val = ControlValue.Get<FVector3f>();
						Value = FVector2D(Val.X, Val.Y);
					}
					if (LastValue.IsSet())
					{
						const FVector2D LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value);
					}
					else
					{
						LastValue = Value;
					}
				}
			}
		}
	}

	const FVector2D LastVal = LastValue.IsSet() ? LastValue.GetValue() : FVector2D::ZeroVector;

	//set from values, note that the fact if they were multiples or not was already set so we need to set them on these values.
	FTLLRN_AnimDetailProxyVector2D Value(LastVal, Vector2D.State);

	const FName PropName("Vector2D");
	FTrackInstancePropertyBindings Binding(PropName, PropName.ToString());
	Binding.CallFunction<FTLLRN_AnimDetailProxyVector2D>(*this, Value);
}

bool UTLLRN_AnimDetailControlsProxyVector2D::IsMultiple(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, X))
	{
		return Vector2D.State.bXMultiple;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, Y))
	{
		return Vector2D.State.bYMultiple;
	}

	return false;
}

TMap<FName, int32> UTLLRN_AnimDetailControlsProxyVector2D::GetPropertyNames() const
{
	TMap<FName, int32> NameToIndex;
	FName PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, X);
	int32 Index = 0;
	NameToIndex.Add(PropertyName, Index);
	PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, Y);
	++Index;
	NameToIndex.Add(PropertyName, Index);
	return NameToIndex;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyVector2D::GetChannelToKeyFromPropertyName(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, X))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, Y))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationY;
	}

	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyVector2D::GetChannelToKeyFromChannelName(const FString& InChannelName) const
{
	if (InChannelName == TEXT("X"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	else if (InChannelName == TEXT("Y"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationY;
	}
	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

void UTLLRN_AnimDetailControlsProxyVector2D::GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& LocationSelectionCache, FTLLRN_AnimDetailVectorSelection& RotationSelectionCache,
	FTLLRN_AnimDetailVectorSelection& ScaleSelectionCache)
{
	LocationSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, X));
	LocationSelectionCache.YSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, Y));
}

//////UTLLRN_AnimDetailControlsProxyFloat////////

void UTLLRN_AnimDetailControlsProxyFloat::UpdatePropertyNames(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedPtr<IPropertyHandle> ValuePropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyFloat, Float), GetClass());
	if (ValuePropertyHandle)
	{
		ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(GetName()));

		if (GetControlElements().Num() == 0 && GetSequencerItems().Num() == 1)
		{
			TArray<FBindingAndTrack> SItems =  GetSequencerItems();
			const FString PropertyPath = FString::Printf(TEXT("%s.%s"), GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_AnimDetailControlsProxyFloat, Float), GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimDetailProxyFloat, Float));
			ValuePropertyHandle = DetailBuilder.GetProperty(FName(*PropertyPath), GetClass());
			if (ValuePropertyHandle)
			{
				ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(SItems[0].Binding->GetPropertyName()));
			}
		}
		else if (bIsIndividual)
		{
			const FString PropertyPath = FString::Printf(TEXT("%s.%s"), GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_AnimDetailControlsProxyFloat, Float), GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimDetailProxyFloat, Float));
			ValuePropertyHandle = DetailBuilder.GetProperty(FName(*PropertyPath), GetClass());
			if (ValuePropertyHandle)
			{
				ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(GetName()));
			}
		}
	}
}

void UTLLRN_AnimDetailControlsProxyFloat::SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if (ControlElement && TLLRN_ControlRig && (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float ||
		ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat))
	{
		float Val = Float.Float;
		TLLRN_ControlRig->SetControlValue<float>(ControlElement->GetKey().Name, Val, true, Context, false);
		TLLRN_ControlRig->Evaluate_AnyThread();
	}
}

bool UTLLRN_AnimDetailControlsProxyFloat::PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty)
{
	return ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyFloat, Float)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyFloat, Float))
		);
}

void UTLLRN_AnimDetailControlsProxyFloat::ClearMultipleFlags()
{
	State.bMultiple = false;
}

void UTLLRN_AnimDetailControlsProxyFloat::SetMultipleFlags(const float& ValA, const float& ValB)
{
	if (FMath::IsNearlyEqual(ValA, ValB) == false)
	{
		State.bMultiple = true;
	}
}

void UTLLRN_AnimDetailControlsProxyFloat::ValueChanged()
{
	TOptional<float> LastValue;
	ClearMultipleFlags();

	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					float Value = 0.0f;
					if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float ||
						(ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat))
					{
						FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
						Value = ControlValue.Get<float>();
					}
					if (LastValue.IsSet())
					{
						const float LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value);
					}
					else
					{
						LastValue = Value;
					}
				}
			}
		}
	}
	for (TPair <TWeakObjectPtr<UObject>, FSequencerProxyItem>& SItems : SequencerItems)
	{
		if (SItems.Key.IsValid() == false)
		{
			continue;
		}
		for (FBindingAndTrack& Binding: SItems.Value.Bindings)
		{
			if (Binding.Binding.IsValid())
			{
				TOptional<double> Value;
				if (FProperty* Property = Binding.Binding->GetProperty((*(SItems.Value.OwnerObject.Get()))))
				{
					if (Property->IsA(FDoubleProperty::StaticClass()))
					{
						Value = Binding.Binding->GetOptionalValue<double>(*SItems.Key);
					}
					else if (Property->IsA(FFloatProperty::StaticClass()))
					{
						TOptional<float> FVal = Binding.Binding->GetOptionalValue<float>(*SItems.Key);
						if (FVal.IsSet())
						{
							Value = (double)FVal.GetValue();
						}
					}
				}
				if (Value)
				{
					if (LastValue.IsSet())
					{
						const float LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value.GetValue());
					}
					else
					{
						LastValue = Value.GetValue();
					}
				}
				
			}
		}
	}

	const float LastVal = LastValue.IsSet() ? LastValue.GetValue() : 0.0f;

	//set from values, note that the fact if they were multiples or not was already set so we need to set them on these values.

	const FName PropName("Float");
	FTrackInstancePropertyBindings Binding(PropName, PropName.ToString());

	FTLLRN_AnimDetailProxyFloat Value;
	Value.Float = LastVal;
	Binding.CallFunction<FTLLRN_AnimDetailProxyFloat>(*this, Value);
}

bool UTLLRN_AnimDetailControlsProxyFloat::IsMultiple(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyFloat, Float))
	{
		return State.bMultiple;
	}

	return false;
}

TMap<FName, int32> UTLLRN_AnimDetailControlsProxyFloat::GetPropertyNames() const
{
	TMap<FName, int32> NameToIndex;
	FName PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyFloat, Float);
	int32 Index = 0;
	NameToIndex.Add(PropertyName, Index);
	return NameToIndex;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyFloat::GetChannelToKeyFromPropertyName(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyFloat, Float))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}

	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyFloat::GetChannelToKeyFromChannelName(const FString& InChannelName) const
{
	if (InChannelName == TEXT("Float"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	TArray<FTLLRN_RigControlElement*> Elements = GetControlElements();
	for (FTLLRN_RigControlElement* Element : Elements)
	{
		if (Element && Element->GetDisplayName() == InChannelName)
		{
			return ETLLRN_ControlRigContextChannelToKey::TranslationX;
		}
	}
	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

void UTLLRN_AnimDetailControlsProxyFloat::GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& LocationSelectionCache, FTLLRN_AnimDetailVectorSelection& RotationSelectionCache,
	FTLLRN_AnimDetailVectorSelection& ScaleSelectionCache)
{
	LocationSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyFloat, Float));
}

void UTLLRN_AnimDetailControlsProxyFloat::SetBindingValueFromCurrent(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding, const FTLLRN_RigControlModifiedContext& Context, bool bInteractive)
{
	if (InObject && Binding.IsValid())
	{
		if (FProperty* Property = Binding->GetProperty((*(InObject))))
		{
			if (Property->IsA(FDoubleProperty::StaticClass()))
			{
				Binding->SetCurrentValue<double>(*InObject, Float.Float);
			}
			else if (Property->IsA(FFloatProperty::StaticClass()))
			{
				float FVal = (float)Float.Float;
				Binding->SetCurrentValue<float>(*InObject, FVal);
			}
		}
	}
}
//////UTLLRN_AnimDetailControlsProxyBool////////

void UTLLRN_AnimDetailControlsProxyBool::UpdatePropertyNames(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedPtr<IPropertyHandle> ValuePropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyBool, Bool), GetClass());
	if (ValuePropertyHandle)
	{
		ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(GetName()));
	}
	if (GetControlElements().Num() == 0 && GetSequencerItems().Num() == 1)
	{
		TArray<FBindingAndTrack> SItems = GetSequencerItems();
		const FString PropertyPath = FString::Printf(TEXT("%s.%s"), GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_AnimDetailControlsProxyBool, Bool), GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimDetailProxyBool, Bool));
		ValuePropertyHandle = DetailBuilder.GetProperty(FName(*PropertyPath), GetClass());
		if (ValuePropertyHandle)
		{
			ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(SItems[0].Binding->GetPropertyName()));
		}
	}
	else if (bIsIndividual)
	{
		const FString PropertyPath = FString::Printf(TEXT("%s.%s"), GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_AnimDetailControlsProxyBool, Bool), GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimDetailProxyBool, Bool));
		ValuePropertyHandle = DetailBuilder.GetProperty(FName(*PropertyPath), GetClass());
		if (ValuePropertyHandle)
		{
			ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(GetName()));
		}
	}
}

void UTLLRN_AnimDetailControlsProxyBool::SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if (ControlElement && TLLRN_ControlRig && (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool))
	{
		bool Val = Bool.Bool;
		TLLRN_ControlRig->SetControlValue<bool>(ControlElement->GetKey().Name, Val, true, Context, false);
		TLLRN_ControlRig->Evaluate_AnyThread();
	}
}

bool UTLLRN_AnimDetailControlsProxyBool::PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty)
{
	return ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyBool, Bool)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyBool, Bool))
		);
}

void UTLLRN_AnimDetailControlsProxyBool::ClearMultipleFlags()
{
	State.bMultiple = false;
}

void UTLLRN_AnimDetailControlsProxyBool::SetMultipleFlags(const bool& ValA, const bool& ValB)
{
	if (ValA == ValB)
	{
		State.bMultiple = true;
	}
}

void UTLLRN_AnimDetailControlsProxyBool::ValueChanged()
{
	TOptional<bool> LastValue;
	ClearMultipleFlags();

	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					bool Value = false;
					if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool)
					{
						FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
						Value = ControlValue.Get<bool>();
					}
					if (LastValue.IsSet())
					{
						const bool LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value);
					}
					else
					{
						LastValue = Value;
					}
				}
			}
		}
	}
	for (TPair <TWeakObjectPtr<UObject>, FSequencerProxyItem>& SItems : SequencerItems)
	{
		if (SItems.Key.IsValid() == false)
		{
			continue;
		}
		for (FBindingAndTrack& Binding : SItems.Value.Bindings)
		{
			if (Binding.Binding.IsValid())
			{
				if (TOptional<bool> Value = Binding.Binding->GetOptionalValue<bool>(*SItems.Key))
				{
					if (LastValue.IsSet())
					{
						const bool LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value.GetValue());
					}
					else
					{
						LastValue = Value.GetValue();
					}
				}

			}
		}
	}

	const bool LastVal = LastValue.IsSet() ? LastValue.GetValue() : 0.0f;

	//set from values, note that the fact if they were multiples or not was already set so we need to set them on these values.

	const FName PropName("Bool");
	FTrackInstancePropertyBindings Binding(PropName, PropName.ToString());

	FTLLRN_AnimDetailProxyBool Value;
	Value.Bool = LastVal;
	Binding.CallFunction<FTLLRN_AnimDetailProxyBool>(*this, Value);

}

bool UTLLRN_AnimDetailControlsProxyBool::IsMultiple(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyBool, Bool))
	{
		return State.bMultiple;
	}

	return false;
}

TMap<FName, int32> UTLLRN_AnimDetailControlsProxyBool::GetPropertyNames() const
{
	TMap<FName, int32> NameToIndex;
	FName PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyBool, Bool);
	int32 Index = 0;
	NameToIndex.Add(PropertyName, Index);
	return NameToIndex;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyBool::GetChannelToKeyFromPropertyName(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyBool, Bool))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}

	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyBool::GetChannelToKeyFromChannelName(const FString& InChannelName) const
{
	if (InChannelName == TEXT("Bool"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	TArray<FTLLRN_RigControlElement*> Elements = GetControlElements();
	for (FTLLRN_RigControlElement* Element : Elements)
	{
		if (Element && Element->GetDisplayName() == InChannelName)
		{
			return ETLLRN_ControlRigContextChannelToKey::TranslationX;
		}
	}
	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

void UTLLRN_AnimDetailControlsProxyBool::GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& LocationSelectionCache, FTLLRN_AnimDetailVectorSelection& RotationSelectionCache,
	FTLLRN_AnimDetailVectorSelection& ScaleSelectionCache)
{
	LocationSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyBool, Bool));
}

void UTLLRN_AnimDetailControlsProxyBool::SetBindingValueFromCurrent(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding, const FTLLRN_RigControlModifiedContext& Context, bool bInteractive)
{
	if (InObject && Binding.IsValid())
	{
		Binding->SetCurrentValue<bool>(*InObject, Bool.Bool);
	}
}
//////UTLLRN_AnimDetailControlsProxyInteger////////

void UTLLRN_AnimDetailControlsProxyInteger::UpdatePropertyNames(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedPtr<IPropertyHandle> ValuePropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyInteger, Integer), GetClass());
	if (ValuePropertyHandle)
	{
		ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(GetName()));
	}
	if (GetControlElements().Num() == 0 && GetSequencerItems().Num() == 1)
	{
		TArray<FBindingAndTrack> SItems = GetSequencerItems();
		const FString PropertyPath = FString::Printf(TEXT("%s.%s"), GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_AnimDetailControlsProxyInteger, Integer), GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimDetailProxyInteger, Integer));
		ValuePropertyHandle = DetailBuilder.GetProperty(FName(*PropertyPath), GetClass());
		if (ValuePropertyHandle)
		{
			ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(SItems[0].Binding->GetPropertyName()));
		}
	}
	else if (bIsIndividual)
	{
		const FString PropertyPath = FString::Printf(TEXT("%s.%s"), GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_AnimDetailControlsProxyInteger, Integer), GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_AnimDetailProxyInteger, Integer));
		ValuePropertyHandle = DetailBuilder.GetProperty(FName(*PropertyPath), GetClass());
		if (ValuePropertyHandle)
		{
			ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(GetName()));
		}
	}
}

void UTLLRN_AnimDetailControlsProxyInteger::SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if (ControlElement && TLLRN_ControlRig && (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer &&
		ControlElement->Settings.ControlEnum == nullptr))
	{
		int32 Val = (int32)Integer.Integer;
		TLLRN_ControlRig->SetControlValue<int32>(ControlElement->GetKey().Name, Val, true, Context, false);
		TLLRN_ControlRig->Evaluate_AnyThread();
	}
}

bool UTLLRN_AnimDetailControlsProxyInteger::PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty)
{
	return ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyInteger, Integer)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyInteger, Integer))
		);
}

void UTLLRN_AnimDetailControlsProxyInteger::ClearMultipleFlags()
{
	State.bMultiple = false;
}

void UTLLRN_AnimDetailControlsProxyInteger::SetMultipleFlags(const int64& ValA, const int64& ValB)
{
	if (ValA != ValB)
	{
		State.bMultiple = true;
	}
}

void UTLLRN_AnimDetailControlsProxyInteger::ValueChanged()
{
	TOptional<int64> LastValue;
	ClearMultipleFlags();

	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					int64 Value = 0;
					if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer)
					{
						FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
						Value = ControlValue.Get<int32>();
					}
					if (LastValue.IsSet())
					{
						const int64 LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value);
					}
					else
					{
						LastValue = Value;
					}
				}
			}
		}
		for (TPair <TWeakObjectPtr<UObject>, FSequencerProxyItem>& SItems : SequencerItems)
		{
			if (SItems.Key.IsValid() == false)
			{
				continue;
			}
			for (FBindingAndTrack& Binding : SItems.Value.Bindings)
			{
				if (Binding.Binding.IsValid())
				{
					if (TOptional<int64> Value = Binding.Binding->GetOptionalValue<int64>(*SItems.Key))
					{
						if (LastValue.IsSet())
						{
							const float LastVal = LastValue.GetValue();
							SetMultipleFlags(LastVal, Value.GetValue());
						}
						else
						{
							LastValue = Value.GetValue();
						}
					}

				}
			}
		}
	}

	const int64 LastVal = LastValue.IsSet() ? LastValue.GetValue() : 0;

	//set from values, note that the fact if they were multiples or not was already set so we need to set them on these values.

	const FName PropName("Integer");
	FTrackInstancePropertyBindings Binding(PropName, PropName.ToString());
	FTLLRN_AnimDetailProxyInteger Value;
	Value.Integer = LastVal;
	Binding.CallFunction<FTLLRN_AnimDetailProxyInteger>(*this, Value);
}

bool UTLLRN_AnimDetailControlsProxyInteger::IsMultiple(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyInteger, Integer))
	{
		return State.bMultiple;
	}
	return false;
}

TMap<FName, int32> UTLLRN_AnimDetailControlsProxyInteger::GetPropertyNames() const
{
	TMap<FName, int32> NameToIndex;
	FName PropertyName = GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyInteger, Integer);
	int32 Index = 0;
	NameToIndex.Add(PropertyName, Index);
	return NameToIndex;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyInteger::GetChannelToKeyFromPropertyName(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyInteger, Integer))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}

	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyInteger::GetChannelToKeyFromChannelName(const FString& InChannelName) const
{
	if (InChannelName == TEXT("Integer"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	TArray<FTLLRN_RigControlElement*> Elements = GetControlElements();
	for (FTLLRN_RigControlElement* Element : Elements)
	{
		if (Element && Element->GetDisplayName() == InChannelName)
		{
			return ETLLRN_ControlRigContextChannelToKey::TranslationX;
		}
	}
	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

void UTLLRN_AnimDetailControlsProxyInteger::GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& LocationSelectionCache, FTLLRN_AnimDetailVectorSelection& RotationSelectionCache,
	FTLLRN_AnimDetailVectorSelection& ScaleSelectionCache)
{
	LocationSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyInteger, Integer));
}

void UTLLRN_AnimDetailControlsProxyInteger::SetBindingValueFromCurrent(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding, const FTLLRN_RigControlModifiedContext& Context, bool bInteractive)
{
	if (InObject && Binding.IsValid())
	{
		Binding->SetCurrentValue<int64>(*InObject, Integer.Integer);
	}
}
//////UTLLRN_AnimDetailControlsProxyEnum////////

void UTLLRN_AnimDetailControlsProxyEnum::UpdatePropertyNames(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedPtr<IPropertyHandle> ValuePropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyEnum, Enum), GetClass());
	if (ValuePropertyHandle)
	{
		ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(GetName()));
	}
	if (GetControlElements().Num() == 0 && GetSequencerItems().Num() == 1)
	{
		TArray<FBindingAndTrack> SItems = GetSequencerItems();
		const FString PropertyPath = FString::Printf(TEXT("%s.%s"), GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_AnimDetailControlsProxyEnum, Enum), GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_ControlRigEnumControlProxyValue, EnumIndex));
		ValuePropertyHandle = DetailBuilder.GetProperty(FName(*PropertyPath), GetClass());
		if (ValuePropertyHandle)
		{
			ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(SItems[0].Binding->GetPropertyName()));
		}
	}
	else if (bIsIndividual)
	{
		const FString PropertyPath = FString::Printf(TEXT("%s.%s"), GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_AnimDetailControlsProxyEnum, Enum), GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_ControlRigEnumControlProxyValue, EnumIndex));
		ValuePropertyHandle = DetailBuilder.GetProperty(FName(*PropertyPath), GetClass());
		if (ValuePropertyHandle)
		{
			ValuePropertyHandle->SetPropertyDisplayName(FText::FromName(GetName()));
		}
	}
}

void UTLLRN_AnimDetailControlsProxyEnum::SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if (ControlElement && TLLRN_ControlRig && (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer &&
		ControlElement->Settings.ControlEnum != nullptr))
	{
		int32 Val = Enum.EnumIndex;
		TLLRN_ControlRig->SetControlValue<int32>(ControlElement->GetKey().Name, Val, true, Context, false);
		TLLRN_ControlRig->Evaluate_AnyThread();
	}
}

bool UTLLRN_AnimDetailControlsProxyEnum::PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty)
{
	return ((Property && Property->GetFName() == GET_MEMBER_NAME_CHECKED(FTLLRN_ControlRigEnumControlProxyValue, EnumIndex)) ||
		(MemberProperty && MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyEnum, Enum))
	);
}

void UTLLRN_AnimDetailControlsProxyEnum::ClearMultipleFlags()
{
	State.bMultiple = false;
}

void UTLLRN_AnimDetailControlsProxyEnum::SetMultipleFlags(const int32& ValA, const int32& ValB)
{
	if (ValA != ValB)
	{
		State.bMultiple = true;
	}
}

void UTLLRN_AnimDetailControlsProxyEnum::ValueChanged()
{
	TOptional<int32> LastValue;
	ClearMultipleFlags();
	TObjectPtr<UEnum> EnumType = nullptr;
	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					int32 Value = 0;
					if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer &&
						ControlElement->Settings.ControlEnum)
					{
						EnumType = ControlElement->Settings.ControlEnum;
						FTLLRN_RigControlValue ControlValue = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
						Value = ControlValue.Get<int32>();
					}

					if (LastValue.IsSet())
					{
						const int32 LastVal = LastValue.GetValue();
						SetMultipleFlags(LastVal, Value);
					}
					else
					{
						LastValue = Value;
					}
				}
			}
		}
	}

	const int32 LastVal = LastValue.IsSet() ? LastValue.GetValue() : 0;

	//set from values, note that the fact if they were multiples or not was already set so we need to set them on these values.
	int32 Value(LastVal);

	if (EnumType)
	{
		const FName PropertyName("Enum");
		FTrackInstancePropertyBindings Binding(PropertyName, PropertyName.ToString());
		FTLLRN_ControlRigEnumControlProxyValue Val;
		Val.EnumType = EnumType;
		Val.EnumIndex = Value;
		Binding.CallFunction<FTLLRN_ControlRigEnumControlProxyValue>(*this, Val);
	}

}

bool UTLLRN_AnimDetailControlsProxyEnum::IsMultiple(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_ControlRigEnumControlProxyValue, EnumIndex))
	{
		return State.bMultiple;
	}

	return false;
}

TMap<FName, int32> UTLLRN_AnimDetailControlsProxyEnum::GetPropertyNames() const
{
	TMap<FName, int32> NameToIndex;
	FName PropertyName = GET_MEMBER_NAME_CHECKED(FTLLRN_ControlRigEnumControlProxyValue, EnumIndex);
	int32 Index = 0;
	NameToIndex.Add(PropertyName, Index);
	return NameToIndex;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyEnum::GetChannelToKeyFromPropertyName(const FName& PropertyName) const
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_ControlRigEnumControlProxyValue, EnumIndex))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}

	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

ETLLRN_ControlRigContextChannelToKey UTLLRN_AnimDetailControlsProxyEnum::GetChannelToKeyFromChannelName(const FString& InChannelName) const
{
	if (InChannelName == TEXT("EnumIndex"))
	{
		return ETLLRN_ControlRigContextChannelToKey::TranslationX;
	}
	TArray<FTLLRN_RigControlElement*> Elements = GetControlElements();
	for (FTLLRN_RigControlElement* Element : Elements)
	{
		if (Element && Element->GetDisplayName() == InChannelName)
		{
			return ETLLRN_ControlRigContextChannelToKey::TranslationX;
		}
	}
	return ETLLRN_ControlRigContextChannelToKey::AllTransform;
}

void UTLLRN_AnimDetailControlsProxyEnum::GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& LocationSelectionCache, FTLLRN_AnimDetailVectorSelection& RotationSelectionCache,
	FTLLRN_AnimDetailVectorSelection& ScaleSelectionCache)
{
	LocationSelectionCache.XSelected = CachePropertySelection(CurveEditor, this, GET_MEMBER_NAME_CHECKED(FTLLRN_ControlRigEnumControlProxyValue, EnumIndex));
}

//////UControlDetailPanelControlProxies////////
UTLLRN_ControlRigDetailPanelControlProxies::UTLLRN_ControlRigDetailPanelControlProxies()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UTLLRN_ControlRigDetailPanelControlProxies::OnPostPropertyChanged);
}

UTLLRN_ControlRigDetailPanelControlProxies::~UTLLRN_ControlRigDetailPanelControlProxies()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

UTLLRN_ControlTLLRN_RigControlsProxy* UTLLRN_ControlRigDetailPanelControlProxies::FindProxy(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement) const
{
	const FTLLRN_NameToProxyMap* TLLRN_ControlRigProxies = TLLRN_ControlRigOnlyProxies.Find(TLLRN_ControlRig);
	if (TLLRN_ControlRigProxies)
	{
		TObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy> const* Proxy = TLLRN_ControlRigProxies->NameToProxy.Find(ControlElement->GetFName());
		if (Proxy && Proxy[0])
		{
			return Proxy[0];
		}
	}
	return nullptr;
}

UTLLRN_ControlTLLRN_RigControlsProxy* UTLLRN_ControlRigDetailPanelControlProxies::FindProxy(UObject* InObject, FName PropertyName) const
{
	const FTLLRN_NameToProxyMap* SequencerProxies = SequencerOnlyProxies.Find(InObject);
	if (SequencerProxies)
	{
		TObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy> const* Proxy = SequencerProxies->NameToProxy.Find(PropertyName);
		if (Proxy && Proxy[0])
		{
			return Proxy[0];
		}
	}
	return nullptr;
}

void UTLLRN_ControlRigDetailPanelControlProxies::OnPostPropertyChanged(UObject* InObject, FPropertyChangedEvent& InPropertyChangedEvent)
{
	const FName MemberPropertyName = InPropertyChangedEvent.MemberProperty != nullptr ? InPropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	/*
	const bool bTransformationChanged =
		(MemberPropertyName == USceneComponent::GetRelativeLocationPropertyName() ||
			MemberPropertyName == USceneComponent::GetRelativeRotationPropertyName() ||
			MemberPropertyName == USceneComponent::GetRelativeScale3DPropertyName());

	if (InObject && bTransformationChanged)
	{
		OnTransformChanged(*InObject);
	}
	*/
}

void UTLLRN_ControlRigDetailPanelControlProxies::ResetSequencerProxies(TMap<ETLLRN_RigControlType, FSequencerProxyPerType>& ProxyPerType)
{
	//Remove
	SelectedSequencerProxies.SetNum(0);
	for (TPair<ETLLRN_RigControlType, FSequencerProxyPerType>& Pair : ProxyPerType)
	{
		for (TPair<UObject*, TArray<FBindingAndTrack>>& PerTypePair : Pair.Value.Bindings)
		{
			for (FBindingAndTrack& Binding : PerTypePair.Value)
			{
				if (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = AddProxy(PerTypePair.Key, Pair.Key, Binding.WeakTrack, Binding.Binding))
				{
					SelectedSequencerProxies.Add(Proxy);
				}
			}
		}
	}
}

const TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> UTLLRN_ControlRigDetailPanelControlProxies::GetAllSelectedProxies()
{
	TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> SelectedProxies(SelectedTLLRN_ControlRigProxies);
	TArray<USceneComponent*> BoundCRObjects;
	TArray<AActor*> BoundCRActors;
	for (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy : SelectedProxies)
	{
		if (Proxy && Proxy->OwnerTLLRN_ControlRig.IsValid() && Proxy->OwnerTLLRN_ControlRig->GetObjectBinding().IsValid())
		{
			if (USceneComponent* SceneComponent = Cast<USceneComponent>(Proxy->OwnerTLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
			{
				BoundCRObjects.Add(SceneComponent);
				if (AActor* ParentActor = SceneComponent->GetOwner())
				{
					BoundCRActors.Add(ParentActor);
				}
			}
		}
	}
	if (SelectedSequencerProxies.Num() > 0)
	{
		if (SelectedProxies.Num() > 0)
		{
			for (int32 Index = SelectedSequencerProxies.Num() - 1; Index >= 0; --Index)
			{
				if (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = SelectedSequencerProxies[Index])
				{
					if (Proxy->OwnerObject.IsValid())
					{
						if (USceneComponent* SceneComponent = Cast<USceneComponent>(Proxy->OwnerObject.Get()))
						{
							if (BoundCRObjects.Contains(SceneComponent))
							{
								SelectedSequencerProxies.RemoveAt(Index);
								continue;
							}
						}
						if (AActor* Actor = Cast<AActor>(Proxy->OwnerObject.Get()))
						{
							if (BoundCRActors.Contains(Actor))
							{
								SelectedSequencerProxies.RemoveAt(Index);
								continue;
							}
						}
					}
					SelectedProxies.Add(Proxy);
				}
			}
		}
		else //no CR selected so just add them all
		{
			SelectedProxies.Append(SelectedSequencerProxies);
		}
	}
	return SelectedProxies;
}

void UTLLRN_ControlRigDetailPanelControlProxies::ValuesChanged()
{
	if (SelectedTLLRN_ControlRigProxies.IsEmpty() && SelectedSequencerProxies.IsEmpty())
	{
		return;
	}
	
	if (UWorld* World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr)
	{
		const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(World);
		Controller.EvaluateAllConstraints();
	}

	for (UTLLRN_ControlTLLRN_RigControlsProxy* P1 : SelectedTLLRN_ControlRigProxies)
	{
		P1->ValueChanged();
	}
	for (UTLLRN_ControlTLLRN_RigControlsProxy* P2 : SelectedSequencerProxies)
	{
		P2->ValueChanged();
	}
}

UTLLRN_ControlTLLRN_RigControlsProxy* UTLLRN_ControlRigDetailPanelControlProxies::AddProxy(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement)
{
	if (TLLRN_ControlRig == nullptr || ControlElement == nullptr)
	{
		return nullptr;
	}
	//check if forced to be individual
	bool bIsIndividual = (ControlElement->IsAnimationChannel()) ||
		(ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl);

	UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = FindProxy(TLLRN_ControlRig, ControlElement);
	if (!Proxy && ControlElement != nullptr)
	{
		Proxy = NewProxyFromType(ControlElement->Settings.ControlType, ControlElement->Settings.ControlEnum);
		//proxy was created so add it
		if (Proxy)
		{
			FTLLRN_NameToProxyMap* TLLRN_ControlRigProxies = TLLRN_ControlRigOnlyProxies.Find(TLLRN_ControlRig);
			if (TLLRN_ControlRigProxies)
			{
				TLLRN_ControlRigProxies->NameToProxy.Add(ControlElement->GetFName(), Proxy);
			}
			else
			{
				FTLLRN_NameToProxyMap NewTLLRN_ControlRigProxies;
				NewTLLRN_ControlRigProxies.NameToProxy.Add(ControlElement->GetFName(), Proxy);
				TLLRN_ControlRigOnlyProxies.Add(TLLRN_ControlRig, NewTLLRN_ControlRigProxies);
			}
		}
		if (Proxy)
		{
			Proxy->Type = ControlElement->Settings.ControlType;;
			Proxy->OwnerControlElement.UpdateCache(ControlElement->GetKey(), TLLRN_ControlRig->GetHierarchy());
			Proxy->OwnerTLLRN_ControlRig = TLLRN_ControlRig;
			Proxy->AddTLLRN_ControlTLLRN_RigControl(TLLRN_ControlRig, ControlElement->GetKey().Name);
			Proxy->bIsIndividual = bIsIndividual;
			Proxy->SetFlags(RF_Transactional);
			Proxy->Modify();
			UWorld* World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr;
			const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(World);
			Controller.EvaluateAllConstraints();
			Proxy->ValueChanged();
		}
	}
	return Proxy;
}

UTLLRN_ControlTLLRN_RigControlsProxy* UTLLRN_ControlRigDetailPanelControlProxies::AddProxy(UObject* InObject, ETLLRN_RigControlType Type, TWeakObjectPtr<UMovieSceneTrack>& Track, TSharedPtr<FTrackInstancePropertyBindings>& Binding)
{
	FName PropertyName = Binding->GetPropertyName();
	UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = FindProxy(InObject, PropertyName);
	if (!Proxy)
	{
		TObjectPtr<UEnum> Enum = nullptr;
		Proxy = NewProxyFromType(Type, Enum);
		//proxy was created so add it
		if (Proxy)
		{
			FTLLRN_NameToProxyMap* SequencerProxies = SequencerOnlyProxies.Find(InObject);
			if (SequencerProxies)
			{
				SequencerProxies->NameToProxy.Add(PropertyName, Proxy);
			}
			else
			{
				FTLLRN_NameToProxyMap NewProxies;
				NewProxies.NameToProxy.Add(PropertyName, Proxy);
				SequencerOnlyProxies.Add(InObject, NewProxies);
			}
		}
		if (!Proxy)
		{
			return Proxy;
		}
		Proxy->Type = Type;
		Proxy->OwnerObject = InObject;
		Proxy->OwnerBindingAndTrack.WeakTrack = Track;
		Proxy->OwnerBindingAndTrack.Binding = Binding;
		Proxy->AddSequencerProxyItem(InObject, Proxy->OwnerBindingAndTrack.WeakTrack, Binding);
		if (Type == ETLLRN_RigControlType::Transform || Type == ETLLRN_RigControlType::TransformNoScale || Type == ETLLRN_RigControlType::EulerTransform)
		{
			Proxy->bIsIndividual = false;
		}
		else
		{
			Proxy->bIsIndividual = true;
		}			
		Proxy->SetFlags(RF_Transactional);
		Proxy->Modify();
		UWorld* World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr;
		const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(World);
		Controller.EvaluateAllConstraints();
		Proxy->ValueChanged();
	}
	return Proxy;
}


UTLLRN_ControlTLLRN_RigControlsProxy* UTLLRN_ControlRigDetailPanelControlProxies::NewProxyFromType(ETLLRN_RigControlType ControlType, TObjectPtr<UEnum>& EnumPtr)
{
	UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = nullptr;
	switch (ControlType)
	{
	case ETLLRN_RigControlType::Transform:
	case ETLLRN_RigControlType::TransformNoScale:
	case ETLLRN_RigControlType::EulerTransform:
	{
		Proxy = NewObject<UTLLRN_AnimDetailControlsProxyTransform>(this, NAME_None);
		break;
	}
	case ETLLRN_RigControlType::Float:
	case ETLLRN_RigControlType::ScaleFloat:
	{
		Proxy = NewObject<UTLLRN_AnimDetailControlsProxyFloat>(this, NAME_None);
		break;
	}
	case ETLLRN_RigControlType::Integer:
	{
		if (EnumPtr == nullptr)
		{
			Proxy = NewObject<UTLLRN_AnimDetailControlsProxyInteger>(this, NAME_None); //was GetTransientPackage....? whay
		}
		else
		{
			UTLLRN_AnimDetailControlsProxyEnum* EnumProxy = NewObject<UTLLRN_AnimDetailControlsProxyEnum>(this, NAME_None);
			EnumProxy->Enum.EnumType = EnumPtr;
			Proxy = EnumProxy;
		}
		break;

	}
	case ETLLRN_RigControlType::Position:
	{
		Proxy = NewObject<UTLLRN_AnimDetailControlsProxyLocation>(this, NAME_None);
		break;
	}
	case ETLLRN_RigControlType::Rotator:
	{
		Proxy = NewObject<UTLLRN_AnimDetailControlsProxyRotation>(this, NAME_None);
		break;
	}
	case ETLLRN_RigControlType::Scale:
	{
		Proxy = NewObject<UTLLRN_AnimDetailControlsProxyScale>(this, NAME_None);
		break;
	}
	case ETLLRN_RigControlType::Vector2D:
	{
		Proxy = NewObject<UTLLRN_AnimDetailControlsProxyVector2D>(this, NAME_None);
		break;
	}
	case ETLLRN_RigControlType::Bool:
	{
		Proxy = NewObject<UTLLRN_AnimDetailControlsProxyBool>(this, NAME_None);
		break;
	}
	default:
		break;
	}

	return Proxy;
}

void UTLLRN_ControlRigDetailPanelControlProxies::RemoveAllProxies()
{
	RemoveTLLRN_ControlRigProxies(nullptr);
	RemoveSequencerProxies(nullptr);
}

void UTLLRN_ControlRigDetailPanelControlProxies::RemoveSequencerProxies(UObject* InObject)
{
	//no control rig remove all
	if (InObject == nullptr)
	{
		for (TPair<TObjectPtr<UObject>, FTLLRN_NameToProxyMap>& Proxies : SequencerOnlyProxies)
		{
			for (TPair<FName, TObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy> >& Pair : Proxies.Value.NameToProxy)
			{
				UTLLRN_ControlTLLRN_RigControlsProxy* ExistingProxy = Pair.Value;
				if (ExistingProxy)
				{
					ExistingProxy->Rename(nullptr, GetTransientPackage());
					ExistingProxy->MarkAsGarbage();
				}
			}
		}
		SequencerOnlyProxies.Empty();
		SelectedSequencerProxies.SetNum(0);
	}
	else
	{
		FTLLRN_NameToProxyMap* Proxies = SequencerOnlyProxies.Find(InObject);
		if (Proxies)
		{
			for (TPair<FName, TObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy>>& Pair : Proxies->NameToProxy)
			{
				UTLLRN_ControlTLLRN_RigControlsProxy* ExistingProxy = Pair.Value;
				if (ExistingProxy)
				{
					SelectedTLLRN_ControlRigProxies.Remove(ExistingProxy);
					ClearSelectedProperty(ExistingProxy);
					ExistingProxy->Rename(nullptr, GetTransientPackage());
					ExistingProxy->MarkAsGarbage();
				}
			}
			SequencerOnlyProxies.Remove(InObject);
		}
	}
}

void UTLLRN_ControlRigDetailPanelControlProxies::RemoveTLLRN_ControlRigProxies(UTLLRN_ControlRig* TLLRN_ControlRig)
{
	//no control rig remove all
	if (TLLRN_ControlRig == nullptr)
	{
		for (TPair<TObjectPtr<UTLLRN_ControlRig>, FTLLRN_NameToProxyMap>& TLLRN_ControlRigProxies : TLLRN_ControlRigOnlyProxies)
		{
			for (TPair<FName, TObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy> >& Pair : TLLRN_ControlRigProxies.Value.NameToProxy)
			{
				UTLLRN_ControlTLLRN_RigControlsProxy* ExistingProxy = Pair.Value;
				if (ExistingProxy)
				{
					ExistingProxy->Rename(nullptr, GetTransientPackage());
					ExistingProxy->MarkAsGarbage();
				}
			}
		}
		TLLRN_ControlRigOnlyProxies.Empty();
		SelectedTLLRN_ControlRigProxies.SetNum(0);
		ClearSelectedProperty();
	}
	else
	{
		FTLLRN_NameToProxyMap* TLLRN_ControlRigProxies = TLLRN_ControlRigOnlyProxies.Find(TLLRN_ControlRig);
		if (TLLRN_ControlRigProxies)
		{
			for (TPair<FName, TObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy>>& Pair : TLLRN_ControlRigProxies->NameToProxy)
			{
				UTLLRN_ControlTLLRN_RigControlsProxy* ExistingProxy = Pair.Value;
				if (ExistingProxy)
				{
					SelectedTLLRN_ControlRigProxies.Remove(ExistingProxy);
					ClearSelectedProperty(ExistingProxy);
					ExistingProxy->Rename(nullptr, GetTransientPackage());
					ExistingProxy->MarkAsGarbage();
				}
			}

			TLLRN_ControlRigOnlyProxies.Remove(TLLRN_ControlRig);
		}
	}
}

void UTLLRN_ControlRigDetailPanelControlProxies::RecreateAllProxies(UTLLRN_ControlRig* TLLRN_ControlRig)
{
	RemoveTLLRN_ControlRigProxies(TLLRN_ControlRig);
	TArray<FTLLRN_RigControlElement*> Controls = TLLRN_ControlRig->AvailableControls();
	for (FTLLRN_RigControlElement* ControlElement : Controls)
	{
		if (ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue)
		{
			AddProxy(TLLRN_ControlRig, ControlElement);
		}
	}
}

void UTLLRN_ControlRigDetailPanelControlProxies::ProxyChanged(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, bool bModify)
{
	if (IsInGameThread())
	{
		UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = FindProxy(TLLRN_ControlRig, ControlElement);
		if (Proxy)
		{
			if (bModify)
			{
				Modify();
				Proxy->Modify();
			}
			Proxy->ValueChanged();
		}
	}
}

void UTLLRN_ControlRigDetailPanelControlProxies::SelectProxy(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* TLLRN_RigElement, bool bSelected)
{
	UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = FindProxy(TLLRN_ControlRig, TLLRN_RigElement);
	if (Proxy)
	{
		Modify();
		if (bSelected)
		{
			if (!SelectedTLLRN_ControlRigProxies.Contains(Proxy))
			{
				SelectedTLLRN_ControlRigProxies.Add(Proxy);
			}
		}
		else
		{
			ClearSelectedProperty(Proxy);
			SelectedTLLRN_ControlRigProxies.Remove(Proxy);
		}
		Proxy->SelectionChanged(bSelected);
	}
}

bool UTLLRN_ControlRigDetailPanelControlProxies::IsSelected(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_RigControlElement* TLLRN_RigElement) const
{
	if (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = FindProxy(InTLLRN_ControlRig, TLLRN_RigElement))
	{
		return SelectedTLLRN_ControlRigProxies.Contains(Proxy);
	}
	return false;
}

void UTLLRN_ControlRigDetailPanelControlProxies::ClearSelectedProperty(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy)
{
	if (Proxy == nullptr || LastSelection.Key == Proxy)
	{
		LastSelection.Key.Reset();
		LastSelection.Value = NAME_None;
	}
}

static TArray<FName> GetPropertyNames(TMap<FName, int32>& NameToIndex, const TOptional<FName>& OptionalPropertyNameFirst, const TOptional<FName>& OptionalPropertyNameSecond,
	ETLLRN_AnimDetailRangeDirection Direction = ETLLRN_AnimDetailRangeDirection::Down)
{
	const int32 MaxNumProperties = NameToIndex.Num();
	TArray<FName> PropertyNames;
	//if we have a first name then we either go to the second name or go in direction
	if (OptionalPropertyNameFirst.IsSet())
	{
		int32* FirstLocation = NameToIndex.Find(OptionalPropertyNameFirst.GetValue());
		if (FirstLocation != nullptr && (*FirstLocation >= 0 && *FirstLocation < MaxNumProperties))
		{
			int32 FirstIndex = INDEX_NONE, SecondIndex = INDEX_NONE;
			if (OptionalPropertyNameSecond.IsSet())
			{
				int32* SecondLocation = NameToIndex.Find(OptionalPropertyNameSecond.GetValue());
				if (SecondLocation != nullptr && (*SecondLocation >= 0 && *SecondLocation < MaxNumProperties))
				{
					if (*FirstLocation < *SecondLocation)
					{
						FirstIndex = *FirstLocation;
						SecondIndex = *SecondLocation;
					}
					else
					{
						FirstIndex = *SecondLocation;
						SecondIndex = *FirstLocation;
					}
				}
			}
			else if (Direction == ETLLRN_AnimDetailRangeDirection::Down)
			{
				FirstIndex = *FirstLocation;
				SecondIndex = MaxNumProperties - 1;
			}
			else if (Direction == ETLLRN_AnimDetailRangeDirection::Up)
			{
				FirstIndex = 0;
				SecondIndex = *FirstLocation;
			}
			if (FirstIndex != INDEX_NONE && SecondIndex != INDEX_NONE)
			{
				TArray<FName> AllPropertyNames;
				NameToIndex.GenerateKeyArray(AllPropertyNames);
				for (int32 Index = FirstIndex; Index <= SecondIndex; ++Index)
				{
					PropertyNames.Add(AllPropertyNames[Index]);
				}
			}
		}
	}
	else //no first name we just get all
	{
		NameToIndex.GenerateKeyArray(PropertyNames);
	}
	return PropertyNames;
}

TArray<TPair< UTLLRN_ControlTLLRN_RigControlsProxy*, FName>> UTLLRN_ControlRigDetailPanelControlProxies::GetPropertiesFromLastSelection(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName) const
{
	TArray<TPair<UTLLRN_ControlTLLRN_RigControlsProxy*, FName>> PropertiesToSelect;

	auto GetPropertiesToSelect = ([&PropertiesToSelect](UTLLRN_ControlTLLRN_RigControlsProxy* ProxyToGet, TOptional <FName>& FirstName, TOptional <FName>& SecondName, ETLLRN_AnimDetailRangeDirection Direction)
	{
		TMap<FName, int32> NameToIndex = ProxyToGet->GetPropertyNames();
		TArray<FName> PropertyNames = GetPropertyNames(NameToIndex, FirstName, SecondName, Direction);
		for (FName& PropName : PropertyNames)
		{
			TPair<UTLLRN_ControlTLLRN_RigControlsProxy*, FName> Selected(ProxyToGet, PropName);
			PropertiesToSelect.Add(Selected);
		}
	});

	auto SelectChildProxiesFromIndex = ([&GetPropertiesToSelect,&PropertiesToSelect](UTLLRN_ControlTLLRN_RigControlsProxy *ParentProxy, const FName& InPropretyName,int32 StartIndex, int32 ChildIndex)
	{
			//now we need to select all of the child proxies before this and including this
			for (int32 Index = StartIndex; Index <= ChildIndex; ++Index)
			{
				UTLLRN_ControlTLLRN_RigControlsProxy* ChildProxy = ParentProxy->ChildProxies[Index];

				TMap<FName, int32> NameToIndex = ChildProxy->GetPropertyNames();
				TArray<FName> PropertyNames;
				NameToIndex.GenerateKeyArray(PropertyNames);
				TOptional <FName> SecondName; //empty
				ETLLRN_AnimDetailRangeDirection Direction = ETLLRN_AnimDetailRangeDirection::Down;
				if (PropertyNames.Num() > 0)
				{
					TOptional <FName> FirstName = PropertyNames[0];
					if (Index != ChildIndex) //not the current one need to select everything
					{
						GetPropertiesToSelect(ChildProxy, FirstName, SecondName, Direction);
					}
					else  //select from the name up
					{
						SecondName = InPropretyName;
						GetPropertiesToSelect(ChildProxy, FirstName, SecondName, Direction);
					}
				}

			}
	});
	if (LastSelection.Key.IsValid())
	{
		//same proxy so just get names to match.
		if (Proxy == LastSelection.Key.Get())
		{
			TMap<FName, int32> NameToIndex = Proxy->GetPropertyNames();
			TOptional <FName> FirstName = LastSelection.Value;
			TOptional <FName> SecondName = PropertyName;
			ETLLRN_AnimDetailRangeDirection Direction = ETLLRN_AnimDetailRangeDirection::Down;
			GetPropertiesToSelect(Proxy, FirstName, SecondName, Direction);
		}
		else if (Proxy->bIsIndividual) //check to see if last selection is another channel with same parent or parent proxy
		{
			if (LastSelection.Key.Get()->bIsIndividual)
			{
				bool bFoundSibling = false;
				UTLLRN_ControlRigDetailPanelControlProxies* ProxyOwner = Proxy->GetTypedOuter<UTLLRN_ControlRigDetailPanelControlProxies>();
				if (ProxyOwner)
				{
					TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> AllProxies = ProxyOwner->GetAllSelectedProxies();
					for (UTLLRN_ControlTLLRN_RigControlsProxy* ParentProxy : AllProxies)
					{
						if (ParentProxy->ChildProxies.Num() > 1)
						{
							int32 OneIndex = ParentProxy->ChildProxies.Find(Proxy);
							if (OneIndex != INDEX_NONE)
							{
								int32 TwoIndex = ParentProxy->ChildProxies.Find(LastSelection.Key.Get());
								if(TwoIndex != INDEX_NONE)
								{
									bFoundSibling = true;
									if (OneIndex > TwoIndex)
									{
										int32 Swap = TwoIndex;
										TwoIndex = OneIndex;
										OneIndex = Swap;
									}
									SelectChildProxiesFromIndex(ParentProxy, PropertyName, OneIndex, TwoIndex);
								}
							}
						}
					}
				}
				if (bFoundSibling == false)
				{
					TPair<UTLLRN_ControlTLLRN_RigControlsProxy*, FName> Selected(Proxy, PropertyName);
					PropertiesToSelect.Add(Selected);
				}
			}
			else //may be it's parent
			{
				UTLLRN_ControlTLLRN_RigControlsProxy* ParentProxy = LastSelection.Key.Get();
				int32 ChildIndex = ParentProxy->ChildProxies.Find(Proxy);
				if (ChildIndex != INDEX_NONE)
				{
					//found child so we need to select all of our properties down to the last one
					TOptional <FName> FirstName = LastSelection.Value;
					TOptional <FName> SecondName; //empty
					ETLLRN_AnimDetailRangeDirection Direction = ETLLRN_AnimDetailRangeDirection::Down;
					GetPropertiesToSelect(LastSelection.Key.Get(), FirstName, SecondName, Direction);
					//now we need to select all of the child proxies before this and including this
					SelectChildProxiesFromIndex(ParentProxy, PropertyName, 0, ChildIndex);
				}
				else
				{
					TPair<UTLLRN_ControlTLLRN_RigControlsProxy*, FName> Selected(Proxy, PropertyName);
					PropertiesToSelect.Add(Selected);
				}
			}	
		}
		else if (Proxy->bIsIndividual == false)
		{
			if (LastSelection.Key->bIsIndividual) //last may be a child
			{
				//select this proxy down
				TOptional <FName> FirstName = PropertyName;
				TOptional <FName> SecondName; //empty
				ETLLRN_AnimDetailRangeDirection Direction = ETLLRN_AnimDetailRangeDirection::Down;
				GetPropertiesToSelect(Proxy, FirstName, SecondName, Direction);
				int32 ChildIndex = Proxy->ChildProxies.Find(LastSelection.Key.Get());
				if (ChildIndex != INDEX_NONE)
				{
					SelectChildProxiesFromIndex(Proxy, LastSelection.Value, 0, ChildIndex);
				}
			}
			else
			{
				TPair<UTLLRN_ControlTLLRN_RigControlsProxy*, FName> Selected(Proxy, PropertyName);
				PropertiesToSelect.Add(Selected);
			}
		}
	}
	else //no range set so 
	{
		TPair<UTLLRN_ControlTLLRN_RigControlsProxy*, FName> Selected(Proxy, PropertyName);
		PropertiesToSelect.Add(Selected);
	}

	return PropertiesToSelect;
}

bool UTLLRN_ControlRigDetailPanelControlProxies::IsPropertyEditingEnabled() const
{
	return true;
}

void UTLLRN_ControlRigDetailPanelControlProxies::SelectProperty(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName, ETLLRN_AnimDetailPropertySelectionType SelectionType)
{
	if (SelectionType == ETLLRN_AnimDetailPropertySelectionType::SelectRange && LastSelection.Key.IsValid())
	{
		TArray<TPair<UTLLRN_ControlTLLRN_RigControlsProxy*, FName>> PropertiesToSelect = GetPropertiesFromLastSelection(Proxy, PropertyName);
		for (TPair<UTLLRN_ControlTLLRN_RigControlsProxy*, FName>& Selected : PropertiesToSelect)
		{
			SelectPropertyInternal(Selected.Key, Selected.Value, SelectionType);
		}
		LastSelection.Key = Proxy;
		LastSelection.Value = PropertyName;
	}
	else
	{
		if (SelectPropertyInternal(Proxy, PropertyName, SelectionType))
		{
			LastSelection.Key = Proxy;
			LastSelection.Value = PropertyName;
		}
	}
}

//may want this to be under each proxy? need to think about this.
static bool GetChannelNameForCurve(const TArray<FString>& CurveString, const FTLLRN_RigControlElement* ControlElement, FString& OutChannelName)
{
	//if single channel expect one item and the name will match
	if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat ||
		ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float ||
		ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool ||
		ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer)
	{
		if (CurveString[0] == ControlElement->GetKey().Name)
		{
			if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat ||
				ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float)
			{
				OutChannelName = FString("Float");
				return true;
			}
			if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool)
			{
				OutChannelName = FString("Bool");
				return true;
			}
			if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer)
			{
				OutChannelName = FString("Integer");
				return true;
			}
		}
	}
	else if (CurveString.Num() > 1)
	{
		if (CurveString[0] == ControlElement->GetKey().Name)
		{
			if (CurveString.Num() == 3)
			{
				OutChannelName = CurveString[1] + "." + CurveString[2];
				return true;
			}
			else if (CurveString.Num() == 2)
			{
				OutChannelName = CurveString[1];
				return true;
			}
		}
	}
	return false;
}

bool UTLLRN_ControlRigDetailPanelControlProxies::SelectPropertyInternal(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName, ETLLRN_AnimDetailPropertySelectionType SelectionType)
{
	using namespace UE::Sequencer;

	const TSharedPtr<FSequencerEditorViewModel> SequencerViewModel = GetSequencer()->GetViewModel();
	const FCurveEditorExtension* CurveEditorExtension = SequencerViewModel->CastDynamic<FCurveEditorExtension>();
	check(CurveEditorExtension);
	TSharedPtr<FCurveEditor> CurveEditor = CurveEditorExtension->GetCurveEditor();
	TSharedPtr<SCurveEditorTree>  CurveEditorTreeView = CurveEditorExtension->GetCurveEditorTreeView();
	TSharedPtr<FOutlinerViewModel> OutlinerViewModel = SequencerViewModel->GetOutliner();
	bool bIsSelected = false;
	if (Proxy)
	{
		for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : Proxy->TLLRN_ControlRigItems)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
			{
				for (const FName& CName : Items.Value.ControlElements)
				{
					if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
					{
						ETLLRN_ControlRigContextChannelToKey ChannelToKey = Proxy->GetChannelToKeyFromPropertyName(PropertyName);
						TParentFirstChildIterator<IOutlinerExtension> OutlinerExtenstionIt = OutlinerViewModel->GetRootItem()->GetDescendantsOfType<IOutlinerExtension>();
						for (; OutlinerExtenstionIt; ++OutlinerExtenstionIt)
						{
							if (TSharedPtr<FTrackModel> TrackModel = OutlinerExtenstionIt.GetCurrentItem()->FindAncestorOfType<FTrackModel>())
							{
								if (UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(TrackModel->GetTrack()))
								{
									if (Track->GetTLLRN_ControlRig() != TLLRN_ControlRig || Track->GetAllSections().Num() < 1)
									{
										continue;
									}
									UMovieSceneSection* SectionToKey = Track->GetSectionToKey(ControlElement->GetFName()) ? Track->GetSectionToKey(ControlElement->GetFName()) : Track->GetAllSections()[0];
									if (TViewModelPtr<FChannelGroupOutlinerModel> ChannelModel = CastViewModel<FChannelGroupOutlinerModel>(OutlinerExtenstionIt.GetCurrentItem()))
									{
										if (ChannelModel->GetChannel(SectionToKey) == nullptr) //if not section to key we also don't select it.
										{
											continue;
										}
									}
									else
									{
										continue;
									}
									FName ID = OutlinerExtenstionIt->GetIdentifier();
									FString Name = ID.ToString();
									TArray<FString> StringArray;
									Name.ParseIntoArray(StringArray, TEXT("."));

									FString ChannelName;
									if (GetChannelNameForCurve(StringArray, ControlElement, ChannelName))
									{
										ETLLRN_ControlRigContextChannelToKey ChannelToKeyFromCurve = Proxy->GetChannelToKeyFromChannelName(ChannelName);
										if (ChannelToKey == ChannelToKeyFromCurve)
										{
											if (TViewModelPtr<ICurveEditorTreeItemExtension> CurveEditorItem = OutlinerExtenstionIt.GetCurrentItem().ImplicitCast())
											{
												FCurveEditorTreeItemID CurveEditorTreeItem = CurveEditorItem->GetCurveEditorItemID();
												if (CurveEditorTreeItem != FCurveEditorTreeItemID::Invalid())
												{
													bIsSelected = (SelectionType == ETLLRN_AnimDetailPropertySelectionType::Toggle) ? !CurveEditorTreeView->IsItemSelected(CurveEditorTreeItem) : true;
													CurveEditorTreeView->SetItemSelection(CurveEditorTreeItem, bIsSelected);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		for (const TPair<TWeakObjectPtr<UObject>, FSequencerProxyItem>& SItems : Proxy->SequencerItems)
		{
			if (UObject* Object = SItems.Key.Get())
			{
				for (const FBindingAndTrack& Element : SItems.Value.Bindings)
				{
					ETLLRN_ControlRigContextChannelToKey ChannelToKey = Proxy->GetChannelToKeyFromPropertyName(PropertyName);
					TParentFirstChildIterator<IOutlinerExtension> OutlinerExtenstionIt = OutlinerViewModel->GetRootItem()->GetDescendantsOfType<IOutlinerExtension>();
					for (; OutlinerExtenstionIt; ++OutlinerExtenstionIt)
					{
						if (TSharedPtr<FTrackModel> TrackModel = OutlinerExtenstionIt.GetCurrentItem()->FindAncestorOfType<FTrackModel>())
						{
							if (TrackModel->GetTrack() == Element.WeakTrack.Get())
							{
								if (TrackModel->GetTrack()->GetAllSections().Num() < 1)
								{
									continue;
								}
								UMovieSceneSection* SectionToKey = TrackModel->GetTrack()->GetSectionToKey() ? TrackModel->GetTrack()->GetSectionToKey() : TrackModel->GetTrack()->GetAllSections()[0];
								if (TViewModelPtr<FChannelGroupOutlinerModel> ChannelModel = CastViewModel<FChannelGroupOutlinerModel>(OutlinerExtenstionIt.GetCurrentItem()))
								{
									if (ChannelModel->GetChannel(SectionToKey) == nullptr) //if not section to key we also don't select it.
									{
										continue;
									}
								}
								else
								{
									continue;
								}
								FName ID = OutlinerExtenstionIt->GetIdentifier();
								FString Name = ID.ToString();
								TArray<FString> StringArray;
								Name.ParseIntoArray(StringArray, TEXT("."));

								FString ChannelName;
								if (StringArray.Num() == 2)
								{
									ChannelName = StringArray[0] + "." + StringArray[1];
								}
								else if (StringArray.Num() == 1)
								{
									ChannelName = StringArray[0];
								}

								ETLLRN_ControlRigContextChannelToKey ChannelToKeyFromCurve = Proxy->GetChannelToKeyFromChannelName(ChannelName);
								if (ChannelToKey == ChannelToKeyFromCurve)
								{
									if (TViewModelPtr<ICurveEditorTreeItemExtension> CurveEditorItem = OutlinerExtenstionIt.GetCurrentItem().ImplicitCast())
									{
										FCurveEditorTreeItemID CurveEditorTreeItem = CurveEditorItem->GetCurveEditorItemID();
										if (CurveEditorTreeItem != FCurveEditorTreeItemID::Invalid())
										{
											bIsSelected = (SelectionType == ETLLRN_AnimDetailPropertySelectionType::Toggle) ? !CurveEditorTreeView->IsItemSelected(CurveEditorTreeItem) : true;
											CurveEditorTreeView->SetItemSelection(CurveEditorTreeItem, bIsSelected);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return bIsSelected;
}

void FTLLRN_ControlRigEnumControlProxyValueDetails::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	TArray<UObject*> Objects;
	InStructPropertyHandle->GetOuterObjects(Objects);
	ensure(Objects.Num() == 1); // This is in here to ensure we are only showing the modifier details in the blueprint editor

	for (UObject* Object : Objects)
	{
		if (Object->IsA<UTLLRN_AnimDetailControlsProxyEnum>())
		{
			ProxyBeingCustomized = Cast<UTLLRN_AnimDetailControlsProxyEnum>(Object);
		}
	}

	check(ProxyBeingCustomized);

	HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
	.ValueContent()
		[
			SNew(SEnumComboBox, ProxyBeingCustomized->Enum.EnumType)
			.OnEnumSelectionChanged(SEnumComboBox::FOnEnumSelectionChanged::CreateSP(this, &FTLLRN_ControlRigEnumControlProxyValueDetails::OnEnumValueChanged, InStructPropertyHandle))
		.CurrentValue(this, &FTLLRN_ControlRigEnumControlProxyValueDetails::GetEnumValue)
		.Font(FAppStyle::GetFontStyle(TEXT("MenuItem.Font")))
		];
}

void FTLLRN_ControlRigEnumControlProxyValueDetails::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

int32 FTLLRN_ControlRigEnumControlProxyValueDetails::GetEnumValue() const
{
	if (ProxyBeingCustomized)
	{
		return ProxyBeingCustomized->Enum.EnumIndex;
	}
	return 0;
}

void FTLLRN_ControlRigEnumControlProxyValueDetails::OnEnumValueChanged(int32 InValue, ESelectInfo::Type InSelectInfo, TSharedRef<IPropertyHandle> InStructHandle)
{
	if (ProxyBeingCustomized)
	{
		ProxyBeingCustomized->Enum.EnumIndex = InValue;
		InStructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}
