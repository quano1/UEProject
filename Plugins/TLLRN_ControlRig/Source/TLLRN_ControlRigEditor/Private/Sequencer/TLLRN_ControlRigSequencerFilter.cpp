// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/TLLRN_ControlRigSequencerFilter.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Filters/ISequencerTrackFilters.h"
#include "Filters/SequencerTrackFilterBase.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "MVVM/Extensions/IOutlinerExtension.h"
#include "MVVM/ViewModels/ChannelModel.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"

using namespace UE::Sequencer;

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigSequencerFilter)

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigSequencerTrackFilters"

//////////////////////////////////////////////////////////////////////////
//

class FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControlsCommands
	: public TCommands<FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControlsCommands>
{
public:
	FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControlsCommands()
		: TCommands<FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControlsCommands>(
			TEXT("FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControls"),
			LOCTEXT("FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControls", "TLLRN Control Rig Filters"),
			NAME_None,
			FAppStyle::GetAppStyleSetName())
	{}

	/** Toggle the control rig controls filter */
	TSharedPtr< FUICommandInfo > ToggleFilter_TLLRN_ControlTLLRN_RigControls;

	/** Initialize commands */
	virtual void RegisterCommands() override
	{
		UI_COMMAND(ToggleFilter_TLLRN_ControlTLLRN_RigControls, "TLLRN Control Rig Controls", "Toggle the filter for Control Rig Controls.", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::F9));
	}
};

//////////////////////////////////////////////////////////////////////////
//

class FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControls : public FSequencerTrackFilter
{
public:
	FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControls(ISequencerTrackFilters& InOutFilterInterface, TSharedPtr<FFilterCategory> InCategory = nullptr)
		: FSequencerTrackFilter(InOutFilterInterface, MoveTemp(InCategory))
		, BindingCount(0)
	{
		FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControlsCommands::Register();
	}

	virtual ~FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControls() override
	{
		BindingCount--;
		if (BindingCount < 1)
		{
			FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControlsCommands::Unregister();
		}
	}

	//~ Begin IFilter

	virtual FString GetName() const override { return TEXT("TLLRN_ControlTLLRN_RigControl"); }

	virtual bool PassesFilter(FSequencerTrackFilterType InItem) const override
	{
		FSequencerFilterData& FilterData = FilterInterface.GetFilterData();
		const UMovieSceneTrack* const TrackObject = FilterData.ResolveMovieSceneTrackObject(InItem);
		const UMovieSceneTLLRN_ControlRigParameterTrack* const Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(TrackObject);
		return IsValid(Track);
	}

	//~ End IFilter

	//~ Begin FFilterBase
	virtual FText GetDisplayName() const override { return LOCTEXT("SequenceTrackFilter_TLLRN_ControlTLLRN_RigControl", "TLLRN Control Rig Control"); }
	virtual FSlateIcon GetIcon() const override { return FSlateIconFinder::FindIconForClass(UTLLRN_ControlRigBlueprint::StaticClass()); }
	//~ End FFilterBase

	//~ Begin FSequencerTrackFilter

	virtual FText GetDefaultToolTipText() const override
	{
		return LOCTEXT("SequencerTrackFilter_TLLRN_ControlTLLRN_RigControlsTip", "Show only Control Rig Control tracks");
	}

	virtual TSharedPtr<FUICommandInfo> GetToggleCommand() const override
	{
		return FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControlsCommands::Get().ToggleFilter_TLLRN_ControlTLLRN_RigControls;
	}

	//~ End FSequencerTrackFilter

private:
	mutable uint32 BindingCount;
};

//////////////////////////////////////////////////////////////////////////
//

class FSequencerTrackFilter_TLLRN_ControlRigSelectedControlsCommands
	: public TCommands<FSequencerTrackFilter_TLLRN_ControlRigSelectedControlsCommands>
{
public:
	FSequencerTrackFilter_TLLRN_ControlRigSelectedControlsCommands()
		: TCommands<FSequencerTrackFilter_TLLRN_ControlRigSelectedControlsCommands>(
			TEXT("FSequencerTrackFilter_TLLRN_ControlRigSelectedControls"),
			LOCTEXT("FSequencerTrackFilter_TLLRN_ControlRigSelectedControls", "TLLRN Control Rig Selected Control Filters"),
			NAME_None,
			FAppStyle::GetAppStyleSetName())
	{}

	/** Toggle the control rig selected controls filter */
	TSharedPtr<FUICommandInfo> ToggleFilter_TLLRN_ControlRigSelectedControls;

	/** Initialize commands */
	virtual void RegisterCommands() override
	{
		UI_COMMAND(ToggleFilter_TLLRN_ControlRigSelectedControls, "TLLRN Control Rig Selected Controls", "Toggle the filter for Control Rig Selected Controls.", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::F10));
	}
};

//////////////////////////////////////////////////////////////////////////
//

class FSequencerTrackFilter_TLLRN_ControlRigSelectedControls : public FSequencerTrackFilter
{
public:
	FSequencerTrackFilter_TLLRN_ControlRigSelectedControls(ISequencerTrackFilters& InOutFilterInterface, TSharedPtr<FFilterCategory> InCategory = nullptr)
		:  FSequencerTrackFilter(InOutFilterInterface, MoveTemp(InCategory))
		, BindingCount(0)
	{
		FSequencerTrackFilter_TLLRN_ControlRigSelectedControlsCommands::Register();
	}

	virtual ~FSequencerTrackFilter_TLLRN_ControlRigSelectedControls() override
	{
		BindingCount--;
		if (BindingCount < 1)
		{
			FSequencerTrackFilter_TLLRN_ControlRigSelectedControlsCommands::Unregister();
		}
	}

	//~ Begin IFilter

	virtual FString GetName() const override { return TEXT("SelectedTLLRN_ControlTLLRN_RigControl"); }

	virtual bool PassesFilter(FSequencerTrackFilterType InItem) const override
	{
		FSequencerFilterData& FilterData = FilterInterface.GetFilterData();

		UMovieSceneTrack* const TrackObject = FilterData.ResolveMovieSceneTrackObject(InItem);
		UTLLRN_RigHierarchy* const TLLRN_ControlTLLRN_RigHierarchy = GetTLLRN_ControlTLLRN_RigHierarchyFromTrackObject(TrackObject);

		if (!IsValid(TLLRN_ControlTLLRN_RigHierarchy))
		{
			return false;
		}

		const TViewModelPtr<IOutlinerExtension> OutlinerExtension = InItem.AsModel()->FindAncestorOfType<IOutlinerExtension>();
		if (!OutlinerExtension.IsValid())
		{
			return false;
		}

		const FString ControlTrackLabel = OutlinerExtension->GetLabel().ToString();

		const TArray<const FTLLRN_RigBaseElement*> BaseTLLRN_RigElements = TLLRN_ControlTLLRN_RigHierarchy->GetSelectedElements(ETLLRN_RigElementType::Control);
		for (const FTLLRN_RigBaseElement* const BaseTLLRN_RigElement : BaseTLLRN_RigElements)
		{
			const FString ElementDisplayName = TLLRN_ControlTLLRN_RigHierarchy->GetDisplayNameForUI(BaseTLLRN_RigElement).ToString();
			if (ElementDisplayName.Equals(ControlTrackLabel))
			{
				return true;
			}

			if (const FTLLRN_RigControlElement* const ControlElement = Cast<FTLLRN_RigControlElement>(BaseTLLRN_RigElement))
			{
				if (ControlElement->CanDriveControls())
				{
					const TArray<FTLLRN_RigElementKey>& DrivenControls = ControlElement->Settings.DrivenControls;
					for (const FTLLRN_RigElementKey& DrivenKey : DrivenControls)
					{
						if (const FTLLRN_RigBaseElement* const DrivenKeyElement = TLLRN_ControlTLLRN_RigHierarchy->Find(DrivenKey))
						{
							const FString DrivenKeyElementDisplayName = TLLRN_ControlTLLRN_RigHierarchy->GetDisplayNameForUI(DrivenKeyElement).ToString();
							if (DrivenKeyElementDisplayName.Equals(ControlTrackLabel))
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

	//~ End IFilter

	//~ Begin FFilterBase
	virtual FText GetDisplayName() const override { return LOCTEXT("SequenceTrackFilter_TLLRN_ControlRigSelectedControl", "Selected Control Rig Control"); }
	virtual FSlateIcon GetIcon() const override { return FSlateIconFinder::FindIconForClass(UTLLRN_ControlRigBlueprint::StaticClass()); }
	//~ End FFilterBase

	//~ Begin FSequencerTrackFilter

	virtual FText GetDefaultToolTipText() const override
	{
		return LOCTEXT("SequencerTrackFilter_TLLRN_ControlRigSelectedControlsTip", "Show Only Selected Control Rig Controls.");
	}

	virtual TSharedPtr<FUICommandInfo> GetToggleCommand() const override
	{
		return FSequencerTrackFilter_TLLRN_ControlRigSelectedControlsCommands::Get().ToggleFilter_TLLRN_ControlRigSelectedControls;
	}

	//~ End FSequencerTrackFilter

	static UTLLRN_RigHierarchy* GetTLLRN_ControlTLLRN_RigHierarchyFromTrackObject(UMovieSceneTrack* const InTrackObject)
	{
		const UMovieSceneTLLRN_ControlRigParameterTrack* const Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(InTrackObject);
		if (!IsValid(Track))
		{
			return nullptr;
		}

		UTLLRN_ControlRig* const TLLRN_ControlRig = Track->GetTLLRN_ControlRig();
		if (!IsValid(TLLRN_ControlRig))
		{
			return nullptr;
		}

		return TLLRN_ControlRig->GetHierarchy();
	}

private:
	mutable uint32 BindingCount;
};

/*

	Bool,
	Float,
	Vector2D,
	Position,
	Scale,
	Rotator,
	Transform
*/

//////////////////////////////////////////////////////////////////////////
//

void UTLLRN_ControlRigTrackFilter::AddTrackFilterExtensions(ISequencerTrackFilters& InOutFilterInterface
	, const TSharedRef<FFilterCategory>& InPreferredCategory
	, TArray<TSharedRef<FSequencerTrackFilter>>& InOutFilterList) const
{
	InOutFilterList.Add(MakeShared<FSequencerTrackFilter_TLLRN_ControlTLLRN_RigControls>(InOutFilterInterface, InPreferredCategory));
	InOutFilterList.Add(MakeShared<FSequencerTrackFilter_TLLRN_ControlRigSelectedControls>(InOutFilterInterface, InPreferredCategory));
}

#undef LOCTEXT_NAMESPACE
