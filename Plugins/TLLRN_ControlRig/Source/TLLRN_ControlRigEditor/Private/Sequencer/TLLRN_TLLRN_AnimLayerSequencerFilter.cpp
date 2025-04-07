// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/TLLRN_TLLRN_AnimLayerSequencerFilter.h"
#include "TLLRN_TLLRN_AnimLayers/TLLRN_TLLRN_AnimLayers.h"
#include "Filters/ISequencerTrackFilters.h"
#include "Filters/SequencerTrackFilterBase.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "MVVM/Extensions/IOutlinerExtension.h"
#include "MVVM/ViewModels/SectionModel.h"
#include "MVVM/Extensions/ITrackExtension.h"
#include "MVVM/Extensions/ITrackLaneExtension.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "MVVM/ViewModels/ChannelModel.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_TLLRN_AnimLayerSequencerFilter)

#define LOCTEXT_NAMESPACE "UnimLayerSequencerFilter"

using namespace UE::Sequencer;
//////////////////////////////////////////////////////////////////////////
//

class FSequencerTrackFilter_TLLRN_TLLRN_AnimLayerSequencerFilterCommands
	: public TCommands<FSequencerTrackFilter_TLLRN_TLLRN_AnimLayerSequencerFilterCommands>
{
public:

	FSequencerTrackFilter_TLLRN_TLLRN_AnimLayerSequencerFilterCommands()
		: TCommands<FSequencerTrackFilter_TLLRN_TLLRN_AnimLayerSequencerFilterCommands>
		(
			"FSequencerTrackFilter_TLLRN_TLLRN_AnimLayerSequencerFilter",
			NSLOCTEXT("Contexts", "FSequencerTrackFilter_TLLRN_AnimLayerSequencer", "FSequencerTrackFilter_TLLRN_AnimLayerSequencer"),
			NAME_None,
			FAppStyle::GetAppStyleSetName() // Icon Style Set
			)
	{ }

	/** Toggle the control rig controls filter */
	TSharedPtr< FUICommandInfo > ToggleSelectedTLLRN_AnimLayer;

	/** Initialize commands */
	virtual void RegisterCommands() override
	{
		UI_COMMAND(ToggleSelectedTLLRN_AnimLayer, "Selected Anim layer", "Toggle the filter for elected Anim Layer", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::F8));
	}
};

class FSequencerTrackFilter_ToggleSelectedTLLRN_AnimLayer : public FSequencerTrackFilter
{
public:

	FSequencerTrackFilter_ToggleSelectedTLLRN_AnimLayer(ISequencerTrackFilters& InOutFilterInterface, TSharedPtr<FFilterCategory> InCategory = nullptr)
		: FSequencerTrackFilter(InOutFilterInterface, MoveTemp(InCategory))
		, BindingCount(0)
	{
		FSequencerTrackFilter_TLLRN_TLLRN_AnimLayerSequencerFilterCommands::Register();
	}

	~FSequencerTrackFilter_ToggleSelectedTLLRN_AnimLayer()
	{
		BindingCount--;

		if (BindingCount < 1)
		{
			FSequencerTrackFilter_TLLRN_TLLRN_AnimLayerSequencerFilterCommands::Unregister();
		}
	}

	virtual FString GetName() const override { return TEXT("SelectedTLLRN_TLLRN_AnimLayersFilter"); }
	virtual FText GetDisplayName() const override { return LOCTEXT("SequenceTrackFilter_SelectedTLLRN_TLLRN_AnimLayers", "Selected Anim Layers"); }
	virtual FSlateIcon GetIcon() const { return FSlateIconFinder::FindIconForClass(UTLLRN_ControlRig::StaticClass()); } //UanimLayer

	virtual bool PassesFilter(FSequencerTrackFilterType InItem) const override
	{

		FSequencerFilterData& FilterData = FilterInterface.GetFilterData();

		const TViewModelPtr<IOutlinerExtension> OutlinerExtension = InItem.AsModel()->FindAncestorOfType<IOutlinerExtension>();
		if (!OutlinerExtension.IsValid())
		{
			return false;
		}
		UMovieSceneTrack* Track = nullptr;

		if (const TViewModelPtr<ITrackExtension> AncestorTrackModel = InItem->FindAncestorOfType<ITrackExtension>(true))
		{
			Track = AncestorTrackModel->GetTrack();
		}

		if (Track)
		{
			ULevelSequence* LevelSequence = Track->GetTypedOuter<ULevelSequence>();
			if (LevelSequence)
			{
				if (const UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(LevelSequence))
				{
					TArray<UMovieSceneSection*> Sections = TLLRN_TLLRN_AnimLayers->GetSelectedLayerSections();
					if (TViewModelPtr<FChannelGroupOutlinerModel> ChannelModel = CastViewModel<FChannelGroupOutlinerModel>(InItem))
					{
						for (UMovieSceneSection* Section : Sections)
						{
							if (ChannelModel->GetChannel(Section))
							{
								return true;
							}
						}
					}
				}
			}
		}
		return false;
	}
	virtual FText GetDefaultToolTipText() const override
	{
		return LOCTEXT("SequenceTrackFilter_SelectedTLLRN_TLLRN_AnimLayersTip", "Show Selected Anim Layers");
	}

	virtual TSharedPtr<FUICommandInfo> GetToggleCommand() const override
	{
		return FSequencerTrackFilter_TLLRN_TLLRN_AnimLayerSequencerFilterCommands::Get().ToggleSelectedTLLRN_AnimLayer;
	}


private:
	mutable uint32 BindingCount;
};

//////////////////////////////////////////////////////////////////////////
//


void UTLLRN_TLLRN_AnimLayerSequencerFilter::AddTrackFilterExtensions(ISequencerTrackFilters& InOutFilterInterface
	, const TSharedRef<FFilterCategory>& InPreferredCategory
	, TArray<TSharedRef<FSequencerTrackFilter>>& InOutFilterList) const
{
	InOutFilterList.Add(MakeShared<FSequencerTrackFilter_ToggleSelectedTLLRN_AnimLayer>(InOutFilterInterface, InPreferredCategory));
}
#undef LOCTEXT_NAMESPACE

