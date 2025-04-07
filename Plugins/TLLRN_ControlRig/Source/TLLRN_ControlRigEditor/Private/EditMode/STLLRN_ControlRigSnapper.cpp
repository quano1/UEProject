// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/STLLRN_ControlRigSnapper.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetRegistry/AssetData.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "ScopedTransaction.h"
#include "TLLRN_ControlRig.h"
#include "UnrealEdGlobals.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "EditorModeManager.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "Selection.h"
#include "Editor.h"
#include "Tools/TLLRN_ControlRigSnapSettings.h"
#include "LevelEditor.h"
#include "PropertyEditorModule.h"
#include "SSocketChooser.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Tools/TLLRN_ControlRigSnapSettings.h"
#include "MovieScene.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigSnapper"

class SComponentPickerPopup : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnComponentChosen, FName);

	SLATE_BEGIN_ARGS(SComponentPickerPopup)
		: _Actor(NULL)
	{}

	/** An actor with components */
	SLATE_ARGUMENT(AActor*, Actor)

		/** Called when the text is chosen. */
		SLATE_EVENT(FOnComponentChosen, OnComponentChosen)

		SLATE_END_ARGS()

		/** Delegate to call when component is selected */
		FOnComponentChosen OnComponentChosen;

	/** List of tag names selected in the tag containers*/
	TArray< TSharedPtr<FName> > ComponentNames;

private:
	TSharedRef<ITableRow> MakeListViewWidget(TSharedPtr<FName> InItem, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow< TSharedPtr<FName> >, OwnerTable)
			[
				SNew(STextBlock).Text(FText::FromName(*InItem.Get()))
			];
	}

	void OnComponentSelected(TSharedPtr<FName> InItem, ESelectInfo::Type InSelectInfo)
	{
		FSlateApplication::Get().DismissAllMenus();

		if (OnComponentChosen.IsBound())
		{
			OnComponentChosen.Execute(*InItem.Get());
		}
	}

public:
	void Construct(const FArguments& InArgs)
	{
		OnComponentChosen = InArgs._OnComponentChosen;
		AActor* Actor = InArgs._Actor;

		TInlineComponentArray<USceneComponent*> Components(Actor);

		ComponentNames.Empty();
		for (USceneComponent* Component : Components)
		{
			if (Component->HasAnySockets())
			{
				ComponentNames.Add(MakeShareable(new FName(Component->GetFName())));
			}
		}

		// Then make widget
		this->ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("Menu.Background")))
			.Padding(5)
			.Content()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 1.0f)
			[
				SNew(STextBlock)
				.Font(FAppStyle::GetFontStyle(TEXT("SocketChooser.TitleFont")))
			.Text(NSLOCTEXT("ComponentChooser", "ChooseComponentLabel", "Choose Component"))
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.MaxHeight(512)
			[
				SNew(SBox)
				.WidthOverride(256)
			.Content()
			[
				SNew(SListView< TSharedPtr<FName> >)
				.ListItemsSource(&ComponentNames)
			.OnGenerateRow(this, &SComponentPickerPopup::MakeListViewWidget)
			.OnSelectionChanged(this, &SComponentPickerPopup::OnComponentSelected)
			]
			]
			]
			];
	}
};

//function to keep framenumber from beign too large do to user error, it can cause a crash if we snap too many frames.
static void KeepFrameInRange(TSharedPtr<ISequencer>& SequencerPtr, FFrameNumber& KeepFrameInRange, UMovieScene* MovieScene)
{
	if (MovieScene && SequencerPtr.IsValid())
	{
		//limit values to 10x times the range from the end points.
		TOptional<TRange<FFrameNumber>> OptionalRange = SequencerPtr->GetSubSequenceRange();
		FFrameNumber StartFrame = OptionalRange.IsSet() ? OptionalRange.GetValue().GetLowerBoundValue() : MovieScene->GetPlaybackRange().GetLowerBoundValue();
		FFrameNumber EndFrame = OptionalRange.IsSet() ? OptionalRange.GetValue().GetUpperBoundValue() : MovieScene->GetPlaybackRange().GetUpperBoundValue();

		if (EndFrame < StartFrame)
		{
			FFrameNumber Temp = StartFrame;
			StartFrame = EndFrame;
			EndFrame = Temp;
		}
		const FFrameNumber Max = (EndFrame - StartFrame) * 10;
		if (KeepFrameInRange < (StartFrame - Max))
		{
			KeepFrameInRange = (StartFrame - Max);
		}
		else if (KeepFrameInRange > (EndFrame + Max))
		{
			KeepFrameInRange = (EndFrame + Max);
		}
	}
}

void STLLRN_ControlRigSnapper::Construct(const FArguments& InArgs)
{
	ClearActors();
	SetStartEndFrames();

	//for snapper settings
	UTLLRN_ControlRigSnapSettings* SnapperSettings = GetMutableDefault<UTLLRN_ControlRigSnapSettings>();
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "TLLRN_ControlRigSnapper";

	SnapperDetailsView = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SnapperDetailsView->SetObject(SnapperSettings);


	ChildSlot
		[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(10.f)
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[

							SNew(SBox)
							.Padding(0.0f)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("Children", "Children"))
							]

						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Fill)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.ContentPadding(FMargin(10.0f, 2.0f, 10.0f, 2.0f))
							.OnClicked(this, &STLLRN_ControlRigSnapper::OnActorToSnapClicked)
							[
								SNew(STextBlock)
								.ToolTipText(LOCTEXT("ActorToSnapTooltip","Select child object(s) you want to snap over the interval range"))
								.Text_Lambda([this]()
									{
										return GetActorToSnapText();
									})
							]
						]

					]
					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(10.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[

								SNew(SBox)
								.Padding(0.0f)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("Parent", "Parent"))
								]

							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Fill)
							[
								SNew(SButton)
									.HAlign(HAlign_Center)
									.ContentPadding(FMargin(10.0f, 2.0f, 10.0f, 2.0f))
									.OnClicked(this, &STLLRN_ControlRigSnapper::OnParentToSnapToClicked)
									[
										SNew(STextBlock)
										.ToolTipText(LOCTEXT("ParentToSnapTooltip", "Select parent object you want children to snap to. If one is not selected it will snap to World Location at the start."))
										.Text_Lambda([this]()
											{
												return GetParentToSnapText();
											})
									]
							]

						]
			 ]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(10.f)
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.ContentPadding(FMargin(10.0f, 2.0f, 10.0f, 2.0f))
							.OnClicked(this, &STLLRN_ControlRigSnapper::OnStartFrameClicked)
							[
								SNew(SEditableTextBox)
								.ToolTipText(LOCTEXT("GetStartFrameTooltip", "Set first frame to snap"))
								.OnTextCommitted_Lambda([this](const FText& InText, ETextCommit::Type TextCommitType)
									{
										TSharedPtr<ISequencer> SequencerPtr = Snapper.GetSequencer().Pin();
										if (SequencerPtr.IsValid() && SequencerPtr->GetFocusedMovieSceneSequence())
										{
											TOptional<double> NewFrameTime = SequencerPtr->GetNumericTypeInterface()->FromString(InText.ToString(), 0);
											if (NewFrameTime.IsSet())
											{
												StartFrame = FFrameNumber((int32)NewFrameTime.GetValue());
												KeepFrameInRange(SequencerPtr, StartFrame, SequencerPtr->GetFocusedMovieSceneSequence()->GetMovieScene());
											}
										}
									})
								.Text_Lambda([this]()
									{
										return GetStartFrameToSnapText();
									})
							]

						]
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(10.f)
							.VAlign(VAlign_Center)
						[

							SNew(SButton)
								.HAlign(HAlign_Center)
								.ContentPadding(FMargin(10.0f, 2.0f, 10.0f, 2.0f))
								.OnClicked(this, &STLLRN_ControlRigSnapper::OnEndFrameClicked)
								[
									SNew(SEditableTextBox)
									.ToolTipText(LOCTEXT("GetEndFrameTooltip", "Set end frame to snap"))
									.OnTextCommitted_Lambda([this](const FText& InText, ETextCommit::Type TextCommitType)
									{
										TWeakPtr<ISequencer> Sequencer = Snapper.GetSequencer();
										if (Sequencer.IsValid() && Sequencer.Pin()->GetFocusedMovieSceneSequence())
										{
											TSharedPtr<ISequencer> SequencerPtr = Sequencer.Pin();
											TOptional<double> NewFrameTime = SequencerPtr->GetNumericTypeInterface()->FromString(InText.ToString(), 0);
											if (NewFrameTime.IsSet())
											{
												EndFrame = FFrameNumber((int32)NewFrameTime.GetValue());
												KeepFrameInRange(SequencerPtr, EndFrame, SequencerPtr->GetFocusedMovieSceneSequence()->GetMovieScene());
											}
										}
									})
									.Text_Lambda([this]()
										{
											return GetEndFrameToSnapText();
										})
								]

						]
				 ]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				[
					SnapperDetailsView.ToSharedRef()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Bottom)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(5.f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.HAlign(HAlign_Fill)
						.ContentPadding(FMargin(10.0f, 2.0f, 10.0f, 2.0f))
						.OnClicked(this, &STLLRN_ControlRigSnapper::OnSnapAnimationClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SnapAnimation", "Snap Animation"))
						]
					]
				]
			]
		];

	TWeakPtr<ISequencer> Sequencer = Snapper.GetSequencer();
	if (Sequencer.IsValid())
	{
		Sequencer.Pin()->OnActivateSequence().AddRaw(this, &STLLRN_ControlRigSnapper::OnActivateSequenceChanged);
	}
}

STLLRN_ControlRigSnapper::~STLLRN_ControlRigSnapper()
{
	TWeakPtr<ISequencer> Sequencer = Snapper.GetSequencer();
	if (Sequencer.IsValid())
	{
		Sequencer.Pin()->OnActivateSequence().RemoveAll(this);
	}
}

void STLLRN_ControlRigSnapper::OnActivateSequenceChanged(FMovieSceneSequenceIDRef ID)
{
	if(!GIsTransacting)
	{ 
		ClearActors();
	}
}


FReply STLLRN_ControlRigSnapper::OnActorToSnapClicked()
{
	ActorToSnap = GetSelection(true);
	return FReply::Handled();
}

FReply STLLRN_ControlRigSnapper::OnParentToSnapToClicked()
{
	ParentToSnap = GetSelection(false);
	return FReply::Handled();
}

FText STLLRN_ControlRigSnapper::GetActorToSnapText()
{
	if (ActorToSnap.IsValid())
	{
		return (ActorToSnap.GetName());
	}

	return LOCTEXT("SelectActor", "Select Actor");
}

FText STLLRN_ControlRigSnapper::GetParentToSnapText()
{
	if (ParentToSnap.IsValid())
	{
		return (ParentToSnap.GetName());
	}

	return LOCTEXT("World", "World");
}


FReply STLLRN_ControlRigSnapper::OnStartFrameClicked()
{
	TWeakPtr<ISequencer> Sequencer = Snapper.GetSequencer();
	if (Sequencer.IsValid())
	{
		const FFrameRate TickResolution = Sequencer.Pin()->GetFocusedTickResolution();
		const FFrameTime FrameTime = Sequencer.Pin()->GetLocalTime().ConvertTo(TickResolution);
		StartFrame = FrameTime.GetFrame();
	}
	return FReply::Handled();
}

FReply STLLRN_ControlRigSnapper::OnEndFrameClicked()
{
	TWeakPtr<ISequencer> Sequencer = Snapper.GetSequencer();
	if (Sequencer.IsValid())
	{
		const FFrameRate TickResolution = Sequencer.Pin()->GetFocusedTickResolution();
		const FFrameTime FrameTime = Sequencer.Pin()->GetLocalTime().ConvertTo(TickResolution);
		EndFrame = FrameTime.GetFrame();
	}
	return FReply::Handled();
}

FReply STLLRN_ControlRigSnapper::OnSnapAnimationClicked()
{
	const UTLLRN_ControlRigSnapSettings* SnapSettings = GetDefault<UTLLRN_ControlRigSnapSettings>();
	Snapper.SnapIt(StartFrame, EndFrame, ActorToSnap, ParentToSnap,SnapSettings);
	return FReply::Handled();
}

FText STLLRN_ControlRigSnapper::GetStartFrameToSnapText()
{
	FText Value;
	TWeakPtr<ISequencer> Sequencer = Snapper.GetSequencer();
	if (Sequencer.IsValid() && Sequencer.Pin()->GetFocusedMovieSceneSequence())
	{
		Value = FText::FromString(Sequencer.Pin()->GetNumericTypeInterface()->ToString(StartFrame.Value));
	}
	return Value;
}

FText STLLRN_ControlRigSnapper::GetEndFrameToSnapText()
{
	FText Value;
	TWeakPtr<ISequencer> Sequencer = Snapper.GetSequencer();
	if (Sequencer.IsValid() && Sequencer.Pin()->GetFocusedMovieSceneSequence())
	{
		Value = FText::FromString(Sequencer.Pin()->GetNumericTypeInterface()->ToString(EndFrame.Value));
	}
	return Value;
}

void STLLRN_ControlRigSnapper::ClearActors()
{
	ActorToSnap.Clear();
	ParentToSnap.Clear();
}

void STLLRN_ControlRigSnapper::SetStartEndFrames()
{
	TWeakPtr<ISequencer> Sequencer = Snapper.GetSequencer();
	if (Sequencer.IsValid()  && Sequencer.Pin()->GetFocusedMovieSceneSequence())
	{
		UMovieScene* MovieScene = Sequencer.Pin()->GetFocusedMovieSceneSequence()->GetMovieScene();
		TOptional<TRange<FFrameNumber>> OptionalRange = Sequencer.Pin()->GetSubSequenceRange();
		StartFrame = OptionalRange.IsSet() ? OptionalRange.GetValue().GetLowerBoundValue() : MovieScene->GetPlaybackRange().GetLowerBoundValue();
		EndFrame = OptionalRange.IsSet() ? OptionalRange.GetValue().GetUpperBoundValue() : MovieScene->GetPlaybackRange().GetUpperBoundValue();

	}
}

FTLLRN_ControlRigSnapperSelection STLLRN_ControlRigSnapper::GetSelection(bool bGetAll) 
{
	FTLLRN_ControlRigSnapperSelection Selection;
	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
	GetTLLRN_ControlRigs(TLLRN_ControlRigs);
	for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig)
		{
			if (const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
			{
				TArray<FName> SelectedControls = TLLRN_ControlRig->CurrentControlSelection();
				if (SelectedControls.Num() > 0)
				{
					FTLLRN_ControlRigForWorldTransforms TLLRN_ControlRigAndSelection;
					TLLRN_ControlRigAndSelection.TLLRN_ControlRig = TLLRN_ControlRig;
					if (bGetAll == false)
					{
						//make sure to only use first control that is snappable(that has a shape)
						for (const FName& ControlName : SelectedControls)
						{
							if (const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
							{
								if (ControlElement->Settings.SupportsShape())
								{
									TLLRN_ControlRigAndSelection.ControlNames.Add(ControlName);
									break;
								}
							}
						}
						Selection.TLLRN_ControlRigs.Add(TLLRN_ControlRigAndSelection);
						return Selection;
					}
					else
					{
						for (const FName& ControlName : SelectedControls)
						{
							if (const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
							{
								if (ControlElement->Settings.SupportsShape())
								{
									TLLRN_ControlRigAndSelection.ControlNames.Add(ControlName);
								}
							}
						}
						Selection.TLLRN_ControlRigs.Add(TLLRN_ControlRigAndSelection);

					}
				}
			}
		}
	}
	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			FActorForWorldTransforms ActorSelection;
			ActorSelection.Actor = Actor;
			Selection.Actors.Add(ActorSelection);
			if (bGetAll == false)
			{
				ActorParentPicked(ActorSelection);
				return Selection;
			}
		}
	}
	return Selection;
}

void STLLRN_ControlRigSnapper::GetTLLRN_ControlRigs(TArray<UTLLRN_ControlRig*>& OutTLLRN_ControlRigs) const
{
	OutTLLRN_ControlRigs.SetNum(0);
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		TArrayView<TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigPtrs = EditMode->GetTLLRN_ControlRigs();
		for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRigPtr : TLLRN_ControlRigPtrs)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigPtr.Get())
			{
				OutTLLRN_ControlRigs.Add(TLLRN_ControlRig);
			}
		}
	}
}


void STLLRN_ControlRigSnapper::ActorParentSocketPicked(const FName SocketPicked, FActorForWorldTransforms Selection) 
{
	ParentToSnap.Actors.SetNum(0);
	Selection.SocketName = SocketPicked;
	ParentToSnap.Actors.Add(Selection);
	//ParentToSnap
}

void STLLRN_ControlRigSnapper::ActorParentPicked(FActorForWorldTransforms Selection) 
{
	TArray<USceneComponent*> ComponentsWithSockets;
	if (Selection.Actor.IsValid())
	{
		TInlineComponentArray<USceneComponent*> Components(Selection.Actor.Get());

		for (USceneComponent* Component : Components)
		{
			if (Component->HasAnySockets())
			{
				ComponentsWithSockets.Add(Component);
			}
		}
	}

	if (ComponentsWithSockets.Num() == 0)
	{
		FSlateApplication::Get().DismissAllMenus();
		ActorParentSocketPicked(NAME_None, Selection);
		return;
	}

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr< ILevelEditor > LevelEditor = LevelEditorModule.GetFirstLevelEditor();

	TSharedPtr<SWidget> MenuWidget;
	if (ComponentsWithSockets.Num() > 1)
	{
		MenuWidget =
			SNew(SComponentPickerPopup)
			.Actor(Selection.Actor.Get())
			.OnComponentChosen(this, &STLLRN_ControlRigSnapper::ActorParentComponentPicked,Selection);

		// Create as context menu
		FSlateApplication::Get().PushMenu(
			LevelEditor.ToSharedRef(),
			FWidgetPath(),
			MenuWidget.ToSharedRef(),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
		);
	}
	else
	{
		ActorParentComponentPicked(ComponentsWithSockets[0]->GetFName(),Selection);
	}
}


void STLLRN_ControlRigSnapper::ActorParentComponentPicked(FName ComponentName, FActorForWorldTransforms Selection) 
{
	USceneComponent* ComponentWithSockets = nullptr;
	if (Selection.Actor.IsValid())
	{
		AActor* Actor = Selection.Actor.Get();
		TInlineComponentArray<USceneComponent*> Components(Actor);

		for (USceneComponent* Component : Components)
		{
			if (Component->GetFName() == ComponentName)
			{
				ComponentWithSockets = Component;
				break;
			}
		}
	}

	if (ComponentWithSockets == nullptr)
	{
		return;
	}
	Selection.Component = ComponentWithSockets;
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr< ILevelEditor > LevelEditor = LevelEditorModule.GetFirstLevelEditor();

	TSharedPtr<SWidget> MenuWidget;

	MenuWidget =
		SNew(SSocketChooserPopup)
		.SceneComponent(ComponentWithSockets)
		.OnSocketChosen(this, &STLLRN_ControlRigSnapper::ActorParentSocketPicked, Selection);

	// Create as context menu
	FSlateApplication::Get().PushMenu(
		LevelEditor.ToSharedRef(),
		FWidgetPath(),
		MenuWidget.ToSharedRef(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
	);
}



#undef LOCTEXT_NAMESPACE
