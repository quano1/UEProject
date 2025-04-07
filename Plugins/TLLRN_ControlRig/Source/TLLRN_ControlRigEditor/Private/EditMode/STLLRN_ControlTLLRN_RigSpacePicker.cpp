// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/STLLRN_ControlTLLRN_RigSpacePicker.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Layout/SSpacer.h"
#include "Styling/AppStyle.h"
#include "ISequencer.h"
#include "ScopedTransaction.h"
#include "TLLRN_ControlRig.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "MovieSceneSequence.h"
#include "MovieScene.h"
#include "TLLRN_ControlTLLRN_RigSpaceChannelEditors.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlTLLRN_RigSpacePicker"

void STLLRN_ControlTLLRN_RigSpacePicker::Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode& InEditMode)
{
	
	ChildSlot
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(PickerExpander, SExpandableArea)
					.InitiallyCollapsed(true)
					.AreaTitle(LOCTEXT("Picker_SpaceWidget", "Spaces"))
					.AreaTitleFont(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
					.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f))
					.Padding(FMargin(8.f))
					.HeaderContent()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.Padding(0.f, 0.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Picker_SpaceWidget", "Spaces"))
							.Font(FCoreStyle::Get().GetFontStyle("ExpandableArea.TitleFont"))
						]
					
						+SHorizontalBox::Slot()
						.FillWidth(1.f)
						[
							SNew(SSpacer)
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Center)
						.Padding(0.f, 2.f, 8.f, 2.f)
						[
							SNew(SButton)
							.ContentPadding(0.0f)
							.ButtonStyle(FAppStyle::Get(), "NoBorder")
							.OnClicked(this, &STLLRN_ControlTLLRN_RigSpacePicker::HandleAddSpaceClicked)
							.Cursor(EMouseCursor::Default)
							.ToolTipText(LOCTEXT("AddSpace", "Add Space"))
							[
								SNew(SImage)
								.Image(FAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
							]
							.Visibility_Lambda([this]()
							{
								return GetAddSpaceButtonVisibility();
							})
						]
					]
					.BodyContent()
					[
						SAssignNew(SpacePickerWidget, STLLRN_RigSpacePickerWidget)
						.AllowDelete(true)
						.AllowReorder(true)
						.AllowAdd(false)
						.ShowBakeAndCompensateButton(true)
						.GetControlCustomization(this, &STLLRN_ControlTLLRN_RigSpacePicker::HandleGetControlElementCustomization)
						.OnActiveSpaceChanged(this, &STLLRN_ControlTLLRN_RigSpacePicker::HandleActiveSpaceChanged)
						.OnSpaceListChanged(this, &STLLRN_ControlTLLRN_RigSpacePicker::HandleSpaceListChanged)
						.OnCompensateKeyButtonClicked(this, &STLLRN_ControlTLLRN_RigSpacePicker::OnCompensateKeyClicked)
						.OnCompensateAllButtonClicked(this, &STLLRN_ControlTLLRN_RigSpacePicker::OnCompensateAllClicked)
						.OnBakeButtonClicked(this, &STLLRN_ControlTLLRN_RigSpacePicker::OnBakeControlsToNewSpaceButtonClicked)
						// todo: implement GetAdditionalSpacesDelegate to pull spaces from sequencer
					]
				]

			]
		];

	SetEditMode(InEditMode);
}

STLLRN_ControlTLLRN_RigSpacePicker::~STLLRN_ControlTLLRN_RigSpacePicker()
{
	//base class handles control rig related cleanup
}

UTLLRN_ControlRig* STLLRN_ControlTLLRN_RigSpacePicker::GetTLLRN_ControlRig() const
{
	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs = GetTLLRN_ControlRigs();
	for(UTLLRN_ControlRig* TLLRN_ControlRig: TLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig)
		{
			TArray<FTLLRN_RigElementKey> SelectedControls = TLLRN_ControlRig->GetHierarchy()->GetSelectedKeys(ETLLRN_RigElementType::Control);
			if(SelectedControls.Num() >0)
			{
				return TLLRN_ControlRig;
			}
		}
	}
	return nullptr;
}

void STLLRN_ControlTLLRN_RigSpacePicker::HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, bool bSelected)
{
	FTLLRN_ControlRigBaseDockableView::HandleControlSelected(Subject, ControlElement, bSelected);
	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		// get the selected controls
		TArray<FTLLRN_RigElementKey> SelectedControls = TLLRN_ControlRig->GetHierarchy()->GetSelectedKeys(ETLLRN_RigElementType::Control);
		SpacePickerWidget->SetControls(TLLRN_ControlRig->GetHierarchy(), SelectedControls);
	}
	else //set nothing
	{
		TArray<FTLLRN_RigElementKey> SelectedControls;
		SpacePickerWidget->SetControls(nullptr, SelectedControls);
	}
}

const FTLLRN_RigControlElementCustomization* STLLRN_ControlTLLRN_RigSpacePicker::HandleGetControlElementCustomization(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey)
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		return TLLRN_ControlRig->GetControlCustomization(InControlKey);
	}
	return nullptr;
}

void STLLRN_ControlTLLRN_RigSpacePicker::HandleActiveSpaceChanged(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey,
	const FTLLRN_RigElementKey& InSpaceKey)
{

	if (ISequencer* Sequencer = GetSequencer())
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
		{
			FString FailureReason;
			UTLLRN_RigHierarchy::TElementDependencyMap DependencyMap = InHierarchy->GetDependenciesForVM(TLLRN_ControlRig->GetVM());
			if (!InHierarchy->CanSwitchToParent(InControlKey, InSpaceKey, DependencyMap, &FailureReason))
			{
				// notification
				FNotificationInfo Info(FText::FromString(FailureReason));
				Info.bFireAndForget = true;
				Info.FadeOutDuration = 2.0f;
				Info.ExpireDuration = 8.0f;

				const TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
				NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);
				return;
			}

			if (const FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(InControlKey))
			{
				FScopedTransaction Transaction(LOCTEXT("KeyTLLRN_ControlTLLRN_RigSpace", "Key Control Rig Space"));
				TLLRN_ControlRig->Modify();

				FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig, InControlKey.Name, Sequencer, true /*bCreateIfNeeded*/);
				if (SpaceChannelAndSection.SpaceChannel)
				{
					const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
					const FFrameTime FrameTime = Sequencer->GetLocalTime().ConvertTo(TickResolution);
					FFrameNumber CurrentTime = FrameTime.GetFrame();
					FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerKeyTLLRN_ControlTLLRN_RigSpaceChannel(TLLRN_ControlRig, Sequencer, SpaceChannelAndSection.SpaceChannel, SpaceChannelAndSection.SectionToKey, CurrentTime, InHierarchy, InControlKey, InSpaceKey);
				}
			}
		}
	}
}

void STLLRN_ControlTLLRN_RigSpacePicker::HandleSpaceListChanged(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey,
	const TArray<FTLLRN_RigElementKey>& InSpaceList)
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		if (const FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(InControlKey))
		{
			FTLLRN_RigControlElementCustomization ControlCustomization = *TLLRN_ControlRig->GetControlCustomization(InControlKey);
			ControlCustomization.AvailableSpaces = InSpaceList;
			ControlCustomization.RemovedSpaces.Reset();

			// remember  the elements which are in the asset's available list but removed by the user
			for (const FTLLRN_RigElementKey& AvailableSpace : ControlElement->Settings.Customization.AvailableSpaces)
			{
				if (!ControlCustomization.AvailableSpaces.Contains(AvailableSpace))
				{
					ControlCustomization.RemovedSpaces.Add(AvailableSpace);
				}
			}

			TLLRN_ControlRig->SetControlCustomization(InControlKey, ControlCustomization);

			if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
			{
				EditMode->SuspendHierarchyNotifs(true);
				InHierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
				EditMode->SuspendHierarchyNotifs(false);
			}
			else
			{
				InHierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
			}

			SpacePickerWidget->RefreshContents();
		}
	}
}

FReply STLLRN_ControlTLLRN_RigSpacePicker::HandleAddSpaceClicked()
{
	return SpacePickerWidget->HandleAddElementClicked();
}

bool STLLRN_ControlTLLRN_RigSpacePicker::ReadyForBakeOrCompensation() const
{
	if (SpacePickerWidget->GetHierarchy() == nullptr)
	{
		return false;
	}
	if (SpacePickerWidget->GetControls().Num() == 0)
	{
		return false;
	}
	if (!GetTLLRN_ControlRig())
	{
		return false;
	}
	ISequencer* Sequencer = GetSequencer();
	if (Sequencer == nullptr || Sequencer->GetFocusedMovieSceneSequence() == nullptr || Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene() == nullptr)
	{
		return false;
	}
	return true;
}

FReply STLLRN_ControlTLLRN_RigSpacePicker::OnCompensateKeyClicked()
{
	if (ReadyForBakeOrCompensation() == false)
	{
		return FReply::Unhandled();
	}
	ISequencer* Sequencer = GetSequencer();
	const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
	const FFrameTime FrameTime = Sequencer->GetLocalTime().ConvertTo(TickResolution);
	const TOptional<FFrameNumber> OptionalKeyTime = FrameTime.GetFrame();
	const bool bSetPreviousKey = true;
	Compensate(OptionalKeyTime, bSetPreviousKey);
	return FReply::Handled();
}

FReply STLLRN_ControlTLLRN_RigSpacePicker::OnCompensateAllClicked()
{
	if (ReadyForBakeOrCompensation() == false)
	{
		return FReply::Unhandled();
	}
	const TOptional<FFrameNumber> OptionalKeyTime;
	ISequencer* Sequencer = GetSequencer();
	const bool bSetPreviousKey = true;
	Compensate(OptionalKeyTime, bSetPreviousKey);
	return FReply::Handled();
}



void STLLRN_ControlTLLRN_RigSpacePicker::Compensate(TOptional<FFrameNumber> OptionalKeyTime, bool bSetPreviousTick)
{
	if (ReadyForBakeOrCompensation() == false)
	{
		return;
	}
	ISequencer* Sequencer = GetSequencer();
	UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig(); //!!!!! THIS SHOULD SUPPORT MULTIPLE!

	if (TLLRN_ControlRig && SpacePickerWidget->GetHierarchy() == TLLRN_ControlRig->GetHierarchy())
	{
		// compensate spaces
		if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetTLLRN_ControlRigSection(Sequencer, TLLRN_ControlRig))
		{
			// compensate spaces
			FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::CompensateIfNeeded(
				TLLRN_ControlRig, Sequencer, CRSection,
				OptionalKeyTime, bSetPreviousTick);
		}
	}
}

FReply STLLRN_ControlTLLRN_RigSpacePicker::OnBakeControlsToNewSpaceButtonClicked()
{
	if (ReadyForBakeOrCompensation() == false)
	{
		return FReply::Unhandled();
	}

	ISequencer* Sequencer = GetSequencer();
	UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig();

	FTLLRN_TLLRN_RigSpacePickerBakeSettings Settings;
	//Find default target space, just use first control and find space at current sequencer time
	//Then Find range

	// FindSpaceChannelAndSectionForControl() will trigger RecreateCurveEditor(), which will deselect the controls
	// but in theory the selection will be recovered in the next tick, so here we just cache the selected controls
	// and use it throughout this function. If this deselection is causing other problems, this part could use a revisit.
	TArray<FTLLRN_RigElementKey> ControlKeys = SpacePickerWidget->GetControls();

	FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig, ControlKeys[0].Name, Sequencer, true /*bCreateIfNeeded*/);
	if (SpaceChannelAndSection.SpaceChannel != nullptr)
	{
		const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
		const FFrameTime FrameTime = Sequencer->GetLocalTime().ConvertTo(TickResolution);
		FFrameNumber CurrentTime = FrameTime.GetFrame();
		FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
		using namespace UE::MovieScene;
		UTLLRN_RigHierarchy* TLLRN_RigHierarchy = SpacePickerWidget->GetHierarchy();
		Settings.TargetSpace = UTLLRN_RigHierarchy::GetDefaultParentKey();

		TRange<FFrameNumber> Range = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange();

		Settings.Settings.StartFrame = Range.GetLowerBoundValue();
		Settings.Settings.EndFrame = Range.GetUpperBoundValue();

		TSharedRef<STLLRN_RigSpacePickerBakeWidget> BakeWidget =
			SNew(STLLRN_RigSpacePickerBakeWidget)
			.Settings(Settings)
			.Hierarchy(SpacePickerWidget->GetHierarchy())
			.Controls(ControlKeys) // use the cached controls here since the selection is not recovered until next tick.
			.Sequencer(Sequencer)
			.GetControlCustomization(this, &STLLRN_ControlTLLRN_RigSpacePicker::HandleGetControlElementCustomization)
			.OnBake_Lambda([Sequencer, TLLRN_ControlRig, TickResolution](UTLLRN_RigHierarchy* InHierarchy, TArray<FTLLRN_RigElementKey> InControls, FTLLRN_TLLRN_RigSpacePickerBakeSettings InSettings)
		{
	
			FScopedTransaction Transaction(LOCTEXT("BakeControlToSpace", "Bake Control In Space"));
			for (const FTLLRN_RigElementKey& ControlKey : InControls)
			{
				//when baking we will now create a channel if one doesn't exist, was causing confusion
				FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig, ControlKey.Name, Sequencer, true /*bCreateIfNeeded*/);
				if (SpaceChannelAndSection.SpaceChannel)
				{
					FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerBakeControlInSpace(TLLRN_ControlRig, Sequencer, SpaceChannelAndSection.SpaceChannel, SpaceChannelAndSection.SectionToKey,
						InHierarchy, ControlKey, InSettings);
				}
			}
			return FReply::Handled();
		});

		return BakeWidget->OpenDialog(true);
	}
	return FReply::Unhandled();
}

EVisibility STLLRN_ControlTLLRN_RigSpacePicker::GetAddSpaceButtonVisibility() const
{
	if(const UTLLRN_RigHierarchy* Hierarchy = SpacePickerWidget->GetHierarchy())
	{
		for(const FTLLRN_RigElementKey& Control : SpacePickerWidget->GetControls())
		{
			if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Control))
			{
				if(ControlElement->Settings.bRestrictSpaceSwitching)
				{
					return EVisibility::Collapsed;
				}
			}
		}
	}

	return EVisibility::Visible;
}

#undef LOCTEXT_NAMESPACE
