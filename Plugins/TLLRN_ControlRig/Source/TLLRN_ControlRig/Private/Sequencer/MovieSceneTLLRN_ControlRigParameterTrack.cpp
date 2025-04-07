// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Compilation/MovieSceneCompilerRules.h"
#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "MovieSceneCommonHelpers.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTemplate.h"
#include "MovieScene.h"
#include "MovieSceneTimeHelpers.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Rigs/FKTLLRN_ControlRig.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "UObject/Package.h"
#include "Async/Async.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif//WITH_EDITOR

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovieSceneTLLRN_ControlRigParameterTrack)

#define LOCTEXT_NAMESPACE "MovieSceneParameterTLLRN_ControlRigTrack"

FColor UMovieSceneTLLRN_ControlRigParameterTrack::AbsoluteRigTrackColor = FColor(65, 89, 194, 65);
FColor UMovieSceneTLLRN_ControlRigParameterTrack::LayeredRigTrackColor = FColor(173, 151, 114);

FTLLRN_ControlRotationOrder::FTLLRN_ControlRotationOrder()
	:RotationOrder(EEulerRotationOrder::YZX),
	bOverrideSetting(false)

{

}

UMovieSceneTLLRN_ControlRigParameterTrack::UMovieSceneTLLRN_ControlRigParameterTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TLLRN_ControlRig(nullptr)
	, PriorityOrder(INDEX_NONE)
{
#if WITH_EDITORONLY_DATA
	TrackTint = AbsoluteRigTrackColor;
#endif

	SupportedBlendTypes = FMovieSceneBlendTypeField::None();
	SupportedBlendTypes.Add(EMovieSceneBlendType::Additive);
	SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
	SupportedBlendTypes.Add(EMovieSceneBlendType::Override);
}

void UMovieSceneTLLRN_ControlRigParameterTrack::BeginDestroy()
{
	Super::BeginDestroy();
	if (IsValid(TLLRN_ControlRig))
	{
		TLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
	}
}

FMovieSceneEvalTemplatePtr UMovieSceneTLLRN_ControlRigParameterTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	return FMovieSceneTLLRN_ControlRigParameterTemplate(*CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(&InSection), *this);
}

bool UMovieSceneTLLRN_ControlRigParameterTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UMovieSceneTLLRN_ControlRigParameterSection::StaticClass();
}

UMovieSceneSection* UMovieSceneTLLRN_ControlRigParameterTrack::CreateNewSection()
{
	UMovieSceneTLLRN_ControlRigParameterSection* NewSection = NewObject<UMovieSceneTLLRN_ControlRigParameterSection>(this, NAME_None, RF_Transactional);
	NewSection->SetTLLRN_ControlRig(TLLRN_ControlRig);
	bool bSetDefault = false;
	if (Sections.Num() == 0)
	{
		NewSection->SetBlendType(EMovieSceneBlendType::Absolute);
		bSetDefault = true;
	}
	else
	{
		NewSection->SetBlendType(EMovieSceneBlendType::Additive);
	}

	NewSection->SpaceChannelAdded().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnSpaceAdded);
	if (!NewSection->ConstraintChannelAdded().IsBoundToObject(this))
	{
		NewSection->ConstraintChannelAdded().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnConstraintAdded);
	}

	if (TLLRN_ControlRig)
	{
		NewSection->RecreateWithThisTLLRN_ControlRig(TLLRN_ControlRig,bSetDefault);
	}
	return  NewSection;
}

void UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnSpaceAdded(UMovieSceneTLLRN_ControlRigParameterSection* Section, const FName& InControlName, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel)
{
	OnSpaceChannelAdded.Broadcast(Section, InControlName, Channel);

	Channel->OnSpaceNoLongerUsed().RemoveAll(this);
	Channel->OnSpaceNoLongerUsed().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnSpaceNoLongerUsed, InControlName);
}

void UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnSpaceNoLongerUsed(FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* InChannel, const TArray<FTLLRN_RigElementKey>& InSpaces, FName InControlName)
{
	if (InChannel && GetTLLRN_ControlRig())
	{
		if(UTLLRN_RigHierarchy* Hierarchy = GetTLLRN_ControlRig()->GetHierarchy())
		{
			if(UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController())
			{
				const FTLLRN_RigElementKey ControlKey(InControlName, ETLLRN_RigElementType::Control);
				if(Hierarchy->Find<FTLLRN_RigControlElement>(ControlKey))
				{
					FTLLRN_RigElementKey DefaultParent = Hierarchy->GetFirstParent(ControlKey);
					for(const FTLLRN_RigElementKey& ParentToRemove : InSpaces)
					{
						if(DefaultParent != ParentToRemove)
						{
							Controller->RemoveParent(ControlKey, ParentToRemove, false, false, false);
						}
					}
				}
			}
		}
	}
}

void UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnConstraintAdded(
	IMovieSceneConstrainedSection* InSection,
	FMovieSceneConstraintChannel* InChannel) const
{
	OnConstraintChannelAdded.Broadcast(InSection, InChannel);
}

void UMovieSceneTLLRN_ControlRigParameterTrack::RemoveAllAnimationData()
{
	Sections.Empty();
	SectionToKey = nullptr;
}

bool UMovieSceneTLLRN_ControlRigParameterTrack::HasSection(const UMovieSceneSection& Section) const
{
	return Sections.Contains(&Section);
}

void UMovieSceneTLLRN_ControlRigParameterTrack::AddSection(UMovieSceneSection& Section)
{
	Sections.Add(&Section);
	if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(&Section))
	{
		if (CRSection->GetTLLRN_ControlRig() != TLLRN_ControlRig)
		{
			CRSection->SetTLLRN_ControlRig(TLLRN_ControlRig);
		}
		CRSection->ReconstructChannelProxy();
	}

	if (Sections.Num() > 1)
	{
		SetSectionToKey(&Section);
	}
}

void UMovieSceneTLLRN_ControlRigParameterTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
	if (SectionToKey == &Section)
	{
		if (Sections.Num() > 0)
		{
			SectionToKey = Sections[0];
		}
		else
		{
			SectionToKey = nullptr;
		}
	}
}

void UMovieSceneTLLRN_ControlRigParameterTrack::RemoveSectionAt(int32 SectionIndex)
{
	bool bResetSectionToKey = (SectionToKey == Sections[SectionIndex]);

	Sections.RemoveAt(SectionIndex);

	if (bResetSectionToKey)
	{
		SectionToKey = Sections.Num() > 0 ? Sections[0] : nullptr;
	}
}

bool UMovieSceneTLLRN_ControlRigParameterTrack::IsEmpty() const
{
	return Sections.Num() == 0;
}

const TArray<UMovieSceneSection*>& UMovieSceneTLLRN_ControlRigParameterTrack::GetAllSections() const
{
	return Sections;
}


#if WITH_EDITORONLY_DATA
FText UMovieSceneTLLRN_ControlRigParameterTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("DisplayName", "TLLRN Control Rig Parameter");
}
#endif


UMovieSceneSection* UMovieSceneTLLRN_ControlRigParameterTrack::CreateTLLRN_ControlRigSection(FFrameNumber StartTime, UTLLRN_ControlRig* InTLLRN_ControlRig, bool bInOwnsTLLRN_ControlRig)
{
	if (InTLLRN_ControlRig == nullptr)
	{
		return nullptr;
	}
	if (!bInOwnsTLLRN_ControlRig)
	{
		InTLLRN_ControlRig->Rename(nullptr, this);
	}

	if (IsValid(TLLRN_ControlRig))
	{
		TLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
	}

	TLLRN_ControlRig = InTLLRN_ControlRig;

	TLLRN_ControlRig->OnPostConstruction_AnyThread().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnPostConstructed);

	UMovieSceneTLLRN_ControlRigParameterSection* NewSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(CreateNewSection());

	UMovieScene* OuterMovieScene = GetTypedOuter<UMovieScene>();
	NewSection->SetRange(TRange<FFrameNumber>::All());

	//mz todo tbd maybe just set it to animated range? TRange<FFrameNumber> Range = OuterMovieScene->GetPlaybackRange();
	//Range.SetLowerBoundValue(StartTime);
	//NewSection->SetRange(Range);

	AddSection(*NewSection);

	return NewSection;
}

TArray<UMovieSceneSection*, TInlineAllocator<4>> UMovieSceneTLLRN_ControlRigParameterTrack::FindAllSections(FFrameNumber Time)
{
	TArray<UMovieSceneSection*, TInlineAllocator<4>> OverlappingSections;

	for (UMovieSceneSection* Section : Sections)
	{
		if (MovieSceneHelpers::IsSectionKeyable(Section) && Section->GetRange().Contains(Time))
		{
			OverlappingSections.Add(Section);
		}
	}

	Algo::Sort(OverlappingSections, MovieSceneHelpers::SortOverlappingSections);

	return OverlappingSections;
}


UMovieSceneSection* UMovieSceneTLLRN_ControlRigParameterTrack::FindSection(FFrameNumber Time)
{
	TArray<UMovieSceneSection*, TInlineAllocator<4>> OverlappingSections = FindAllSections(Time);

	if (OverlappingSections.Num())
	{
		if (SectionToKey && OverlappingSections.Contains(SectionToKey))
		{
			return SectionToKey;
		}
		else
		{
			return OverlappingSections[0];
		}
	}

	return nullptr;
}


UMovieSceneSection* UMovieSceneTLLRN_ControlRigParameterTrack::FindOrExtendSection(FFrameNumber Time, float& Weight)
{
	Weight = 1.0f;
	TArray<UMovieSceneSection*, TInlineAllocator<4>> OverlappingSections = FindAllSections(Time);
	if (SectionToKey && MovieSceneHelpers::IsSectionKeyable(SectionToKey))
	{
		bool bCalculateWeight = false;
		if (!OverlappingSections.Contains(SectionToKey))
		{
			if (SectionToKey->HasEndFrame() && SectionToKey->GetExclusiveEndFrame() <= Time)
			{
				if (SectionToKey->GetExclusiveEndFrame() != Time)
				{
					SectionToKey->SetEndFrame(Time);
				}
			}
			else
			{
				SectionToKey->SetStartFrame(Time);
			}
			if (OverlappingSections.Num() > 0)
			{
				bCalculateWeight = true;
			}
		}
		else
		{
			if (OverlappingSections.Num() > 1)
			{
				bCalculateWeight = true;
			}
		}
		//we need to calculate weight also possibly
		FOptionalMovieSceneBlendType BlendType = SectionToKey->GetBlendType();
		if (bCalculateWeight)
		{
			Weight = MovieSceneHelpers::CalculateWeightForBlending(SectionToKey, Time);
		}
		return SectionToKey;
	}
	else
	{
		if (OverlappingSections.Num() > 0)
		{
			return OverlappingSections[0];
		}
	}

	// Find a spot for the section so that they are sorted by start time
	for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
	{
		UMovieSceneSection* Section = Sections[SectionIndex];

		// Check if there are no more sections that would overlap the time 
		if (!Sections.IsValidIndex(SectionIndex + 1) || (Sections[SectionIndex + 1]->HasEndFrame() && Sections[SectionIndex + 1]->GetExclusiveEndFrame() > Time))
		{
			// No sections overlap the time

			if (SectionIndex > 0)
			{
			// Append and grow the previous section
			UMovieSceneSection* PreviousSection = Sections[SectionIndex ? SectionIndex - 1 : 0];

			PreviousSection->SetEndFrame(Time);
			return PreviousSection;
			}
			else if (Sections.IsValidIndex(SectionIndex + 1))
			{
			// Prepend and grow the next section because there are no sections before this one
			UMovieSceneSection* NextSection = Sections[SectionIndex + 1];
			NextSection->SetStartFrame(Time);
			return NextSection;
			}
			else
			{
			// SectionIndex == 0 
			UMovieSceneSection* PreviousSection = Sections[0];
			if (PreviousSection->HasEndFrame() && PreviousSection->GetExclusiveEndFrame() <= Time)
			{
				// Append and grow the section
				if (PreviousSection->GetExclusiveEndFrame() != Time)
				{
					PreviousSection->SetEndFrame(Time);
				}
			}
			else
			{
				// Prepend and grow the section
				PreviousSection->SetStartFrame(Time);
			}
			return PreviousSection;
			}
		}
	}

	return nullptr;
}

UMovieSceneSection* UMovieSceneTLLRN_ControlRigParameterTrack::FindOrAddSection(FFrameNumber Time, bool& bSectionAdded)
{
	bSectionAdded = false;

	UMovieSceneSection* FoundSection = FindSection(Time);
	if (FoundSection)
	{
		return FoundSection;
	}

	// Add a new section that starts and ends at the same time
	UMovieSceneSection* NewSection = CreateNewSection();
	ensureAlwaysMsgf(NewSection->HasAnyFlags(RF_Transactional), TEXT("CreateNewSection must return an instance with RF_Transactional set! (pass RF_Transactional to NewObject)"));
	NewSection->SetFlags(RF_Transactional);
	NewSection->SetRange(TRange<FFrameNumber>::Inclusive(Time, Time));

	Sections.Add(NewSection);

	bSectionAdded = true;

	return NewSection;
}

TArray<TWeakObjectPtr<UMovieSceneSection>> UMovieSceneTLLRN_ControlRigParameterTrack::GetSectionsToKey() const
{
	TArray<TWeakObjectPtr<UMovieSceneSection>> SectionsToKey;
	if (SectionToKeyPerControl.Num() > 0)
	{
		SectionToKeyPerControl.GenerateValueArray(SectionsToKey);
	}
	else
	{
		SectionsToKey.Add(SectionToKey);
	}
	return SectionsToKey;
}

UMovieSceneSection* UMovieSceneTLLRN_ControlRigParameterTrack::GetSectionToKey(const FName& InControlName) const
{
	if (const TWeakObjectPtr<UMovieSceneSection>* Section = SectionToKeyPerControl.Find(InControlName))
	{
		if (Section->IsValid())
		{
			return Section->Get();
		}
	}
	return GetSectionToKey();
}

void UMovieSceneTLLRN_ControlRigParameterTrack::SetSectionToKey(UMovieSceneSection* InSection, const FName& InControlName)
{
	if (Sections.Num() < 1 || InSection == nullptr)
	{
		return;
	}
	Modify();
	SectionToKeyPerControl.Add(InControlName, InSection);
	SectionToKey = Sections[0];
}

void UMovieSceneTLLRN_ControlRigParameterTrack::SetSectionToKey(UMovieSceneSection* InSection)
{
	if (Sections.Num() < 1 || InSection == nullptr)
	{
		return;
	}
	if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection))
	{
		Modify();
		if (SectionToKeyPerControl.Num() > 0) //we have sections that are in layers so need to respect them
		{
			if (bSetSectionToKeyPerControl)
			{
				for (TPair < FName, TWeakObjectPtr<UMovieSceneSection>>& SectionToKeyItem : SectionToKeyPerControl)
				{
					//only set it as the section to key if it's in that section, otherwise leave it alone
					FTLLRN_ChannelMapInfo* pChannelIndex = CRSection->ControlChannelMap.Find(SectionToKeyItem.Key);
					if (pChannelIndex)
					{
						if (CRSection->GetControlNameMask(SectionToKeyItem.Key))
						{
							SectionToKeyItem.Value = InSection;
						}
					}
				}
			}
			SectionToKey = Sections[0];
		}
		else
		{
			SectionToKey = InSection;
		}
	}
}

UMovieSceneSection* UMovieSceneTLLRN_ControlRigParameterTrack::GetSectionToKey() const
{
	if (SectionToKey)
	{
		return SectionToKey;
	}
	else if(Sections.Num() >0)
	{
		return Sections[0];
	}
	return nullptr;
}

void UMovieSceneTLLRN_ControlRigParameterTrack::ReconstructTLLRN_ControlRig()
{
	if (TLLRN_ControlRig && !TLLRN_ControlRig->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad | RF_NeedInitialization))
	{
		TLLRN_ControlRig->ConditionalPostLoad();
		TLLRN_ControlRig->Initialize();
		for (UMovieSceneSection* Section : Sections)
		{
			if (Section)
			{
				UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section);
				if (CRSection)
				{
					if (CRSection->SpaceChannelAdded().IsBoundToObject(this) == false)
					{
						CRSection->SpaceChannelAdded().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnSpaceAdded);
					}

					if (!CRSection->ConstraintChannelAdded().IsBoundToObject(this))
					{
						CRSection->ConstraintChannelAdded().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnConstraintAdded);
					}
					
					CRSection->RecreateWithThisTLLRN_ControlRig(TLLRN_ControlRig, CRSection->GetBlendType() == EMovieSceneBlendType::Absolute);
				}
			}
		}
	}
}

void UMovieSceneTLLRN_ControlRigParameterTrack::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	FCoreUObjectDelegates::OnEndLoadPackage.AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandlePackageDone);
	// If we have a control Rig and it's not a native one, register OnEndLoadPackage callback on instance directly
	if (TLLRN_ControlRig && !TLLRN_ControlRig->GetClass()->IsNative())
	{
		TLLRN_ControlRig->OnEndLoadPackage().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleTLLRN_ControlRigPackageDone);
	}
	// Otherwise try and reconstruct the control rig directly (which is fine for native classes)
	else
#endif
	{		
		ReconstructTLLRN_ControlRig();
	}

	if (IsValid(TLLRN_ControlRig))
	{
		TLLRN_ControlRig->OnPostConstruction_AnyThread().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnPostConstructed);
	}
}

//mz todo this is coming from BuildPatchServices in Runtime/Online/BuildPatchServices/Online/Core/AsyncHelper.h
//looking at moving this over
//this is very useful since it properly handles passing in a this pointer to the async task.
namespace MovieSceneTLLRN_ControlRigTrack
{
	/**
	 * Helper functions for wrapping async functionality.
	 */
	namespace AsyncHelpers
	{
		template<typename ResultType, typename... Args>
		static TFunction<void()> MakePromiseKeeper(const TSharedRef<TPromise<ResultType>, ESPMode::ThreadSafe>& Promise, const TFunction<ResultType(Args...)>& Function, Args... FuncArgs)
		{
			return [Promise, Function, FuncArgs...]()
			{
				Promise->SetValue(Function(FuncArgs...));
			};
		}

		template<typename... Args>
		static TFunction<void()> MakePromiseKeeper(const TSharedRef<TPromise<void>, ESPMode::ThreadSafe>& Promise, const TFunction<void(Args...)>& Function, Args... FuncArgs)
		{
			return [Promise, Function, FuncArgs...]()
			{
				Function(FuncArgs...);
				Promise->SetValue();
			};
		}

		template<typename ResultType, typename... Args>
		static TFuture<ResultType> ExecuteOnGameThread(const TFunction<ResultType(Args...)>& Function, Args... FuncArgs)
		{
			TSharedRef<TPromise<ResultType>, ESPMode::ThreadSafe> Promise = MakeShareable(new TPromise<ResultType>());
			TFunction<void()> PromiseKeeper = MakePromiseKeeper(Promise, Function, FuncArgs...);
			if (!IsInGameThread())
			{
				AsyncTask(ENamedThreads::GameThread, MoveTemp(PromiseKeeper));
			}
			else
			{
				PromiseKeeper();
			}
			return Promise->GetFuture();
		}

		template<typename ResultType>
		static TFuture<ResultType> ExecuteOnGameThread(const TFunction<ResultType()>& Function)
		{
			TSharedRef<TPromise<ResultType>, ESPMode::ThreadSafe> Promise = MakeShareable(new TPromise<ResultType>());
			TFunction<void()> PromiseKeeper = MakePromiseKeeper(Promise, Function);
			if (!IsInGameThread())
			{
				AsyncTask(ENamedThreads::GameThread, MoveTemp(PromiseKeeper));
			}
			else
			{
				PromiseKeeper();
			}
			return Promise->GetFuture();
		}
	}
}

void UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnPostConstructed_GameThread()
{
	if (IsValid(TLLRN_ControlRig))
	{
		TArray<FTLLRN_RigControlElement*> SortedControls;
		TLLRN_ControlRig->GetControlsInOrder(SortedControls);
#if WITH_EDITOR
		const FScopedTransaction PostConstructTransation(NSLOCTEXT("TLLRN_ControlRig", "PostConstructTransation", "Post Construct"), !GIsTransacting);
#endif		
		bool bSectionWasDifferent = false;
		for (UMovieSceneSection* BaseSection : GetAllSections())
		{
			if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(BaseSection))
			{
				if (Section->IsDifferentThanLastControlsUsedToReconstruct(SortedControls))
				{
					Section->RecreateWithThisTLLRN_ControlRig(TLLRN_ControlRig, Section->GetBlendType() == EMovieSceneBlendType::Absolute);
					bSectionWasDifferent = true;
				}
			}
		}
		if (bSectionWasDifferent)
		{
			BroadcastChanged();
		}
		if (SortedControls.Num() > 0) //really set up
		{
			TArray<FName> Names = GetControlsWithDifferentRotationOrders();
			ResetControlsToSettingsRotationOrder(Names);
		}
	}
}

void UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnPostConstructed(UTLLRN_ControlRig* Subject, const FName& InEventName)
{
	if(IsInGameThread())
	{
		HandleOnPostConstructed_GameThread();

	}
}

#if WITH_EDITORONLY_DATA
void UMovieSceneTLLRN_ControlRigParameterTrack::DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass)
{
	Super::DeclareConstructClasses(OutConstructClasses, SpecificSubclass);
	OutConstructClasses.Add(FTopLevelAssetPath(UMovieSceneSection::StaticClass()));
	OutConstructClasses.Add(FTopLevelAssetPath(UMovieSceneTLLRN_ControlRigParameterSection::StaticClass()));
}
#endif

#if WITH_EDITOR
void UMovieSceneTLLRN_ControlRigParameterTrack::HandlePackageDone(const FEndLoadPackageContext& Context)
{
	if (!TLLRN_ControlRig || TLLRN_ControlRig->GetClass()->IsNative())
	{
		// EndLoad is never called for native packages, so skip work
		FCoreUObjectDelegates::OnEndLoadPackage.RemoveAll(this);
		return;
	}
	
	// ensure both the track package and the control rig package are fully end-loaded	
	if (!GetPackage()->GetHasBeenEndLoaded())
	{
		return;
	}
	
	if (const UPackage* TLLRN_ControlRigPackage = Cast<UPackage>(TLLRN_ControlRig->GetClass()->GetOutermost()))
	{
		if (!TLLRN_ControlRigPackage->GetHasBeenEndLoaded())
		{
			return;
		}
	}

	// All dependent packages ready, no need to wait/check for any other packages
	// ReconstructTLLRN_ControlRig may trigger loading of packages that we don't care about, so unregister from the delegate
	// before reconstruction to avoid infinite loop
	FCoreUObjectDelegates::OnEndLoadPackage.RemoveAll(this);

	// Only reconstruct in case it is not a native TLLRN_ControlRig class
	ReconstructTLLRN_ControlRig();	
}

void UMovieSceneTLLRN_ControlRigParameterTrack::HandleTLLRN_ControlRigPackageDone(URigVMHost* InTLLRN_ControlRig)
{
	if (ensure(TLLRN_ControlRig == InTLLRN_ControlRig))
	{
		TLLRN_ControlRig->OnEndLoadPackage().RemoveAll(this);
		ReconstructTLLRN_ControlRig();
	}
}
#endif


void UMovieSceneTLLRN_ControlRigParameterTrack::PostEditImport()
{
	Super::PostEditImport();
	if (TLLRN_ControlRig)
	{
		if (TLLRN_ControlRig->OnInitialized_AnyThread().IsBoundToObject(this) == false)
		{
			TLLRN_ControlRig->OnPostConstruction_AnyThread().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnPostConstructed);
		}
		TLLRN_ControlRig->ClearFlags(RF_Transient); //when copied make sure it's no longer transient, sequencer does this for tracks/sections 
											  //but not for all objects in them since the control rig itself has transient objects.
	}
	ReconstructTLLRN_ControlRig();
}

void UMovieSceneTLLRN_ControlRigParameterTrack::RenameParameterName(const FName& OldParameterName, const FName& NewParameterName)
{
	if (OldParameterName != NewParameterName)
	{
		for (UMovieSceneSection* Section : Sections)
		{
			if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section))
			{
				CRSection->RenameParameterName(OldParameterName, NewParameterName);
			}
		}
		if (FTLLRN_ControlRotationOrder* RotationOrder = ControlsRotationOrder.Find(OldParameterName))
		{
			FTLLRN_ControlRotationOrder NewRotationOrder = *RotationOrder;
			ControlsRotationOrder.Remove(OldParameterName);
			ControlsRotationOrder.Add(NewParameterName, NewRotationOrder);
		}
	}
}

void UMovieSceneTLLRN_ControlRigParameterTrack::ReplaceTLLRN_ControlRig(UTLLRN_ControlRig* NewTLLRN_ControlRig, bool RecreateChannels)
{
	if (IsValid(TLLRN_ControlRig))
	{
		TLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
	}

	TLLRN_ControlRig = NewTLLRN_ControlRig;

	if (IsValid(TLLRN_ControlRig))
	{
		TLLRN_ControlRig->OnPostConstruction_AnyThread().AddUObject(this, &UMovieSceneTLLRN_ControlRigParameterTrack::HandleOnPostConstructed);

		if (TLLRN_ControlRig->GetOuter() != this)
		{
			TLLRN_ControlRig->Rename(nullptr, this);
		}
	}
	for (UMovieSceneSection* Section : Sections)
	{
		if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section))
		{
			if (RecreateChannels)
			{
				CRSection->RecreateWithThisTLLRN_ControlRig(NewTLLRN_ControlRig, CRSection->GetBlendType() == EMovieSceneBlendType::Absolute);
			}
			else
			{
				CRSection->SetTLLRN_ControlRig(NewTLLRN_ControlRig);
			}
		}
	}
}
TOptional<EEulerRotationOrder> UMovieSceneTLLRN_ControlRigParameterTrack::GetTLLRN_ControlRotationOrder(const FTLLRN_RigControlElement* ControlElement,
	bool bCurrent) const
{
	TOptional<EEulerRotationOrder> Order;
	if (bCurrent)
	{
		const FTLLRN_ControlRotationOrder* RotationOrder = ControlsRotationOrder.Find(ControlElement->GetFName());
		if (RotationOrder)
		{
			Order = RotationOrder->RotationOrder;
		}
	}
	else //use setting
	{
		if (TLLRN_ControlRig->GetHierarchy()->GetUsePreferredRotationOrder(ControlElement))
		{
			Order = TLLRN_ControlRig->GetHierarchy()->GetControlPreferredEulerRotationOrder(ControlElement);
		}
	}
	return Order;
}

TArray<FName> UMovieSceneTLLRN_ControlRigParameterTrack::GetControlsWithDifferentRotationOrders() const
{
	TArray<FName> Names;
	if (TLLRN_ControlRig && TLLRN_ControlRig->GetHierarchy())
	{
		UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
		TArray<FTLLRN_RigControlElement*> SortedControls;
		TLLRN_ControlRig->GetControlsInOrder(SortedControls);
		for (const FTLLRN_RigControlElement* ControlElement : SortedControls)
		{
			if (!Hierarchy->IsAnimatable(ControlElement))
			{
				continue;
			}
			TOptional<EEulerRotationOrder> Current = GetTLLRN_ControlRotationOrder(ControlElement, true);
			TOptional<EEulerRotationOrder> Setting = GetTLLRN_ControlRotationOrder(ControlElement, false);
			if (Current != Setting)
			{
				Names.Add(ControlElement->GetFName());
			}

		}
	}
	return Names;
}

void UMovieSceneTLLRN_ControlRigParameterTrack::ResetControlsToSettingsRotationOrder(const TArray<FName>& Names, EMovieSceneKeyInterpolation Interpolation)
{
	if (TLLRN_ControlRig && TLLRN_ControlRig->GetHierarchy())
	{
		for (const FName& Name : Names)
		{
			if (FTLLRN_RigControlElement* ControlElement = GetTLLRN_ControlRig()->FindControl(Name))
			{

				TOptional<EEulerRotationOrder> Current = GetTLLRN_ControlRotationOrder(ControlElement, true);
				TOptional<EEulerRotationOrder> Setting = GetTLLRN_ControlRotationOrder(ControlElement, false);
				if (Current != Setting)
				{
					ChangeTLLRN_ControlRotationOrder(Name, Setting, Interpolation);
				}
			}
		}
	}
}
void UMovieSceneTLLRN_ControlRigParameterTrack::ChangeTLLRN_ControlRotationOrder(const FName& InControlName, const TOptional<EEulerRotationOrder>& NewOrder, EMovieSceneKeyInterpolation Interpolation)
{
	if (TLLRN_ControlRig && TLLRN_ControlRig->GetHierarchy())
	{
		if (FTLLRN_RigControlElement* ControlElement = GetTLLRN_ControlRig()->FindControl(InControlName))
		{
			TOptional<EEulerRotationOrder> Current = GetTLLRN_ControlRotationOrder(ControlElement, true);
			if (Current != NewOrder)
			{
				if (NewOrder.IsSet())
				{
					FTLLRN_ControlRotationOrder& RotationOrder = ControlsRotationOrder.FindOrAdd(InControlName);
					RotationOrder.RotationOrder = NewOrder.GetValue();
					TOptional<EEulerRotationOrder> Setting = GetTLLRN_ControlRotationOrder(ControlElement, false);
					if (Setting != NewOrder)
					{
						RotationOrder.bOverrideSetting = true;
					}
				}
				else //no longer set so just remove
				{
					ControlsRotationOrder.Remove(InControlName);
				}
				for (UMovieSceneSection* Section : Sections)
				{
					if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section))
					{
						CRSection->ChangeTLLRN_ControlRotationOrder(InControlName, Current, NewOrder, Interpolation);
					}
				}
			}
		}
	}	
}

void UMovieSceneTLLRN_ControlRigParameterTrack::GetSelectedNodes(TArray<FName>& SelectedControlNames)
{
	if (GetTLLRN_ControlRig())
	{
		SelectedControlNames = GetTLLRN_ControlRig()->CurrentControlSelection();
	}
}

int32 UMovieSceneTLLRN_ControlRigParameterTrack::GetPriorityOrder() const
{
	return PriorityOrder;
}

void UMovieSceneTLLRN_ControlRigParameterTrack::SetPriorityOrder(int32 InPriorityIndex)
{
	if (InPriorityIndex >= 0)
	{
		PriorityOrder = InPriorityIndex;
	}
	else
	{
		PriorityOrder = 0;
	}
}

#if WITH_EDITOR
bool UMovieSceneTLLRN_ControlRigParameterTrack::GetFbxCurveDataFromChannelMetadata(const FMovieSceneChannelMetaData& MetaData, FControlRigFbxCurveData& OutCurveData)
{
	const FString ChannelName = MetaData.Name.ToString();
	TArray<FString> ChannelParts;

	// The channel has an attribute
	if (ChannelName.ParseIntoArray(ChannelParts, TEXT(".")) > 1)
	{
		// Retrieve the attribute
		OutCurveData.AttributeName = ChannelParts.Last();
		
		// The control name (left part) will be used as the node name
		OutCurveData.NodeName = ChannelParts[0];
		OutCurveData.ControlName = *OutCurveData.NodeName;

		// The channel has 3 parts, the middle one (i.e. Location) will be treated as the property name
		if (ChannelParts.Num() > 2)
		{
			OutCurveData.AttributePropertyName = ChannelParts[1];
		}
	}
	// The channel does not have an attribute
	else
	{
		// Thus no property above the attribute
		OutCurveData.AttributePropertyName.Empty();
		
		// The channel group will be used as the node name (name of the control this channel is grouped under - i.e. for animation channels)
		OutCurveData.NodeName = MetaData.Group.ToString();

		// The channel name will be used as the control name and attribute name (i.e. Weight)
		OutCurveData.ControlName = *ChannelName;
		OutCurveData.AttributeName = OutCurveData.ControlName.ToString();
	}

	if (OutCurveData.NodeName.IsEmpty() || OutCurveData.AttributeName.IsEmpty())
	{
		return false;
	}
	
	// Retrieve the control type
	if (GetTLLRN_ControlRig())
	{
		if (FTLLRN_RigControlElement* Control = GetTLLRN_ControlRig()->FindControl(OutCurveData.ControlName))
		{
			OutCurveData.ControlType = (FFBXControlRigTypeProxyEnum)(uint8)Control->Settings.ControlType;
			return true;
		}
	}
	return false;
}
#endif

UTLLRN_ControlRig* UMovieSceneTLLRN_ControlRigParameterTrack::GetGameWorldTLLRN_ControlRig(UWorld* InWorld) 
{
	if (GameWorldTLLRN_ControlRigs.Find(InWorld) == nullptr && TLLRN_ControlRig)
	{
		UTLLRN_ControlRig* NewGameWorldTLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(this, TLLRN_ControlRig->GetClass(), NAME_None, RF_Transient);
		NewGameWorldTLLRN_ControlRig->Initialize();
		if (UFKTLLRN_ControlRig* FKTLLRN_ControlRig = Cast<UFKTLLRN_ControlRig>(Cast<UTLLRN_ControlRig>(TLLRN_ControlRig)))
		{
			if (UFKTLLRN_ControlRig* NewFKTLLRN_ControlRig = Cast<UFKTLLRN_ControlRig>(Cast<UTLLRN_ControlRig>(NewGameWorldTLLRN_ControlRig)))
			{
				NewFKTLLRN_ControlRig->SetApplyMode(FKTLLRN_ControlRig->GetApplyMode());
			}
		}
		else
		{
			NewGameWorldTLLRN_ControlRig->SetIsAdditive(TLLRN_ControlRig->IsAdditive());
		}
		GameWorldTLLRN_ControlRigs.Add(InWorld, NewGameWorldTLLRN_ControlRig);
	}
	TObjectPtr<UTLLRN_ControlRig> * GameWorldTLLRN_ControlRig = GameWorldTLLRN_ControlRigs.Find(InWorld);
	if (GameWorldTLLRN_ControlRig != nullptr)
	{
		return GameWorldTLLRN_ControlRig->Get();
	}
	return nullptr;
}

bool UMovieSceneTLLRN_ControlRigParameterTrack::IsAGameInstance(const UTLLRN_ControlRig* InTLLRN_ControlRig, const bool bCheckValidWorld) const
{
	if (!InTLLRN_ControlRig || GameWorldTLLRN_ControlRigs.IsEmpty())
	{
		return false;
	}

	for (const TPair<TWeakObjectPtr<UWorld>, TObjectPtr<UTLLRN_ControlRig>>& WorldAndTLLRN_ControlRig: GameWorldTLLRN_ControlRigs)
	{
		if (WorldAndTLLRN_ControlRig.Value == InTLLRN_ControlRig)
		{
			return bCheckValidWorld ? WorldAndTLLRN_ControlRig.Key.IsValid() : true;
		}
	}
	
	return false;
}

TArray<FRigControlFBXNodeAndChannels>* UMovieSceneTLLRN_ControlRigParameterTrack::GetNodeAndChannelMappings(UMovieSceneSection* InSection )
{
#if WITH_EDITOR
	if (GetTLLRN_ControlRig() == nullptr)
	{
		return nullptr;
	}
	bool bSectionAdded;
	//use passed in section if available, else section to key if available, else first section or create one.
	UMovieSceneTLLRN_ControlRigParameterSection* CurrentSectionToKey = InSection ? Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection) : Cast<UMovieSceneTLLRN_ControlRigParameterSection>(GetSectionToKey());
	if (CurrentSectionToKey == nullptr)
	{
		CurrentSectionToKey = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(FindOrAddSection(0, bSectionAdded));
	} 
	if (!CurrentSectionToKey)
	{
		return nullptr;
	}

	const FName DoubleChannelTypeName = FMovieSceneDoubleChannel::StaticStruct()->GetFName();
	const FName FloatChannelTypeName = FMovieSceneFloatChannel::StaticStruct()->GetFName();
	const FName BoolChannelTypeName = FMovieSceneBoolChannel::StaticStruct()->GetFName();
	const FName EnumChannelTypeName = FMovieSceneByteChannel::StaticStruct()->GetFName();
	const FName IntegerChannelTypeName = FMovieSceneIntegerChannel::StaticStruct()->GetFName();

	// Our resulting mapping containing FBX node & UE channels data for each control
	TArray<FRigControlFBXNodeAndChannels>* NodeAndChannels = new TArray<FRigControlFBXNodeAndChannels>();
	
	FMovieSceneChannelProxy& ChannelProxy = CurrentSectionToKey->GetChannelProxy();
	for (const FMovieSceneChannelEntry& Entry : CurrentSectionToKey->GetChannelProxy().GetAllEntries())
	{
		const FName ChannelTypeName = Entry.GetChannelTypeName();
		if (ChannelTypeName != DoubleChannelTypeName && ChannelTypeName != FloatChannelTypeName && ChannelTypeName != BoolChannelTypeName
			&& ChannelTypeName != EnumChannelTypeName && ChannelTypeName != IntegerChannelTypeName)
		{
			continue;
		}

		TArrayView<FMovieSceneChannel* const> Channels = Entry.GetChannels();
		TArrayView<const FMovieSceneChannelMetaData> AllMetaData = Entry.GetMetaData();

		for (int32 Index = 0; Index < Channels.Num(); ++Index)
		{
			FMovieSceneChannelHandle Channel = ChannelProxy.MakeHandle(ChannelTypeName, Index);
			const FMovieSceneChannelMetaData& MetaData = AllMetaData[Index];

			FControlRigFbxCurveData FbxCurveData;
			if (!GetFbxCurveDataFromChannelMetadata(MetaData, FbxCurveData))
			{
				continue;
			}

			// Retrieve the current control node, usually the last one but not given
			FRigControlFBXNodeAndChannels* CurrentNodeAndChannel;

			const int i = NodeAndChannels->FindLastByPredicate([FbxCurveData](const FRigControlFBXNodeAndChannels& A)
				{ return A.NodeName == FbxCurveData.NodeName && A.ControlName == FbxCurveData.ControlName; }
			);
			if (i != INDEX_NONE)
			{
				CurrentNodeAndChannel = &(*NodeAndChannels)[i];
			}
			// Create the node if not created yet
			else
			{
				NodeAndChannels->Add(FRigControlFBXNodeAndChannels());
				
				CurrentNodeAndChannel = &NodeAndChannels->Last();

				CurrentNodeAndChannel->MovieSceneTrack = this;
				CurrentNodeAndChannel->ControlType = FbxCurveData.ControlType;
				CurrentNodeAndChannel->NodeName = FbxCurveData.NodeName;
				CurrentNodeAndChannel->ControlName = FbxCurveData.ControlName;
			}

			if (ChannelTypeName == DoubleChannelTypeName)
			{
				FMovieSceneDoubleChannel* DoubleChannel = Channel.Cast<FMovieSceneDoubleChannel>().Get();
				CurrentNodeAndChannel->DoubleChannels.Add(DoubleChannel);
			}
			else if (ChannelTypeName == FloatChannelTypeName)
			{
				FMovieSceneFloatChannel* FloatChannel = Channel.Cast<FMovieSceneFloatChannel>().Get();
				CurrentNodeAndChannel->FloatChannels.Add(FloatChannel);
			}
			else if (ChannelTypeName == BoolChannelTypeName)
			{
				FMovieSceneBoolChannel* BoolChannel = Channel.Cast<FMovieSceneBoolChannel>().Get();
				CurrentNodeAndChannel->BoolChannels.Add(BoolChannel);
			}
			else if (ChannelTypeName == EnumChannelTypeName)
			{
				FMovieSceneByteChannel* EnumChannel = Channel.Cast<FMovieSceneByteChannel>().Get();
				CurrentNodeAndChannel->EnumChannels.Add(EnumChannel);
			}
			else if (ChannelTypeName == IntegerChannelTypeName)
			{
				FMovieSceneIntegerChannel* IntegerChannel = Channel.Cast<FMovieSceneIntegerChannel>().Get();
				CurrentNodeAndChannel->IntegerChannels.Add(IntegerChannel);
			}
		}
	}

	return NodeAndChannels;
#else
	return nullptr;
#endif
}


#undef LOCTEXT_NAMESPACE

