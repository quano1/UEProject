// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/STLLRN_ControlRigTweenWidget.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Application/SlateUser.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetRegistry/AssetData.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "ScopedTransaction.h"
#include "UnrealEdGlobals.h"
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "ILevelSequenceEditorToolkit.h"
#include "Viewports/InViewportUIDragOperation.h"
#include "EditMode/TLLRN_ControlRigEditModeToolkit.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigTweenWidget"

void STLLRN_ControlRigTweenSlider::Construct(const FArguments& InArgs)
{
	PoseBlendValue = 0.0f;
	bIsBlending = false;
	bSliderStartedTransaction = false;
	AnimSlider = InArgs._InAnimSlider;
	WeakEditMode = InArgs._InWeakEditMode;
	
	ULevelSequence* LevelSequence = ULevelSequenceEditorBlueprintLibrary::GetCurrentLevelSequence();
	IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSequence, false);
	ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
	WeakSequencer = LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[	
					SAssignNew(SpinBox,SSpinBox<double>)
					.PreventThrottling(true)
					.Value(this, &STLLRN_ControlRigTweenSlider::OnGetPoseBlendValue)
					.ToolTipText_Lambda([this]()
						{
							FText TooltipText = AnimSlider->GetTooltipText();
							return TooltipText;
						})
					.MinValue(-2.0)
					.MaxValue(2.0)
					.MinSliderValue(-1.0)
					.MaxSliderValue(1.0)
					.SliderExponent(1)
					.Delta(0.005)
					.MinDesiredWidth(100.0)
					.SupportDynamicSliderMinValue(true)
					.SupportDynamicSliderMaxValue(true)
					.ClearKeyboardFocusOnCommit(true)
					.SelectAllTextOnCommit(false)
					.OnValueChanged(this, &STLLRN_ControlRigTweenSlider::OnPoseBlendChanged)
					.OnValueCommitted(this, &STLLRN_ControlRigTweenSlider::OnPoseBlendCommited)
					.OnBeginSliderMovement(this, &STLLRN_ControlRigTweenSlider::OnBeginSliderMovement)
					.OnEndSliderMovement(this, &STLLRN_ControlRigTweenSlider::OnEndSliderMovement)
					
				]
			]
	];	
}

void STLLRN_ControlRigTweenSlider::OnPoseBlendChanged(double ChangedVal)
{
	if (WeakSequencer.IsValid() && bIsBlending)
	{
		PoseBlendValue = ChangedVal;

		// defer blend function on next tick
		PendingBlendFunction = [this, ChangedVal]()
		{
			AnimSlider->Blend(WeakSequencer, ChangedVal);
		};

		WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
	}

}

void STLLRN_ControlRigTweenSlider::ResetAnimSlider()
{
	if (SpinBox.IsValid())
	{
		PoseBlendValue = 0.0;
	}
}

void STLLRN_ControlRigTweenSlider::Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (PendingBlendFunction)
	{
		PendingBlendFunction();
		PendingBlendFunction.Reset();
	}
	
	SCompoundWidget::Tick(InAllottedGeometry, InCurrentTime, InDeltaTime);
}

void STLLRN_ControlRigTweenSlider::DragAnimSliderTool(double Val)
{
	if (SpinBox.IsValid())
	{
		//if control is down then act like we are overriding the slider
		const bool bCtrlDown = FSlateApplication::Get().GetModifierKeys().IsControlDown();
		const double MinSliderVal = bCtrlDown ? SpinBox->GetMinValue() : SpinBox->GetMinSliderValue();
		const double MaxSliderVal = bCtrlDown ? SpinBox->GetMaxValue() : SpinBox->GetMaxSliderValue();

		double NewVal = Val;
		if (NewVal > MaxSliderVal)
		{
			NewVal = MaxSliderVal;
		}
		else if (NewVal < MinSliderVal)
		{
			NewVal = MinSliderVal;
		}
		Setup();
		bIsBlending = true;
		OnPoseBlendChanged(NewVal);
		bIsBlending = false;
	}
}

void STLLRN_ControlRigTweenSlider::OnBeginSliderMovement()
{
	PendingBlendFunction.Reset();
	if (bSliderStartedTransaction == false)
	{
		bIsBlending = bSliderStartedTransaction = Setup();
		if (bIsBlending)
		{
			GEditor->BeginTransaction(LOCTEXT("AnimSliderBlend", "AnimSlider Blend"));
		}
	}
}

bool STLLRN_ControlRigTweenSlider::Setup()
{
	//if getting sequencer from level sequence need to use the current(leader), not the focused
	return AnimSlider->Setup(WeakSequencer, WeakEditMode);
	
}

void STLLRN_ControlRigTweenSlider::OnEndSliderMovement(double NewValue)
{
	if (PendingBlendFunction)
	{
		PendingBlendFunction();
		PendingBlendFunction.Reset();
	}
	
	if (bSliderStartedTransaction)
	{
		GEditor->EndTransaction();
		bSliderStartedTransaction = false;
		bIsBlending = false;
		PoseBlendValue = 0.0f;
	}
	// Set focus back to the parent widget for users focusing the slider
	TSharedRef<SWidget> ThisRef = AsShared();
	FSlateApplication::Get().ForEachUser([&ThisRef](FSlateUser& User) {
		if (User.HasFocusedDescendants(ThisRef) && ThisRef->IsParentValid())
		{
			User.SetFocus(ThisRef->GetParentWidget().ToSharedRef());
		}
	});
}


FReply STLLRN_ControlRigTweenWidget::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Need to remember where within a tab we grabbed
	const FVector2D TabGrabScreenSpaceOffset = MouseEvent.GetScreenSpacePosition() - MyGeometry.GetAbsolutePosition();

	FOnInViewportUIDropped OnUIDropped = FOnInViewportUIDropped::CreateSP(this, &STLLRN_ControlRigTweenWidget::FinishDraggingWidget);
	// Start dragging.
	TSharedRef<FInViewportUIDragOperation> DragDropOperation =
		FInViewportUIDragOperation::New(
			SharedThis(this),
			TabGrabScreenSpaceOffset,
			GetDesiredSize(),
			OnUIDropped
		);

	if (OwningToolkit.IsValid())
	{
		OwningToolkit.Pin()->TryRemoveTweenOverlay();
	}
	return FReply::Handled().BeginDragDrop(DragDropOperation);
}

void STLLRN_ControlRigTweenWidget::FinishDraggingWidget(const FVector2D InLocation)
{
	if (OwningToolkit.IsValid())
	{
		OwningToolkit.Pin()->UpdateTweenWidgetLocation(InLocation);
		OwningToolkit.Pin()->TryShowTweenOverlay();
	}
}

void STLLRN_ControlRigTweenSlider::OnPoseBlendCommited(double ChangedVal, ETextCommit::Type Type)
{
	if (SpinBox.IsValid() && SpinBox->HasKeyboardFocus())
	{
		if (bIsBlending == false)
		{
			bIsBlending = Setup();

		}
		if (bIsBlending)
		{
			FScopedTransaction ScopedTransaction(LOCTEXT("TweenTransaction", "Tween"));
			PoseBlendValue = ChangedVal;
			OnPoseBlendChanged(ChangedVal);
			bIsBlending = false;
			PoseBlendValue = 0.0f;
		}
	}
}


void STLLRN_ControlRigTweenWidget::OnSelectSliderTool(int32 Index)
{
	ActiveSlider = Index;
	if (ActiveSlider >= 0 && ActiveSlider < AnimBlendTools.GetAnimSliders().Num())
	{
		SliderWidget->SetAnimSlider(AnimBlendTools.GetAnimSliders()[ActiveSlider]);
	}
	
}
FText STLLRN_ControlRigTweenWidget::GetActiveSliderName() const
{
	if (ActiveSlider >= 0 && ActiveSlider < AnimBlendTools.GetAnimSliders().Num())
	{
		return AnimBlendTools.GetAnimSliders()[ActiveSlider].Get()->GetText();
	}
	return FText();
}

FText STLLRN_ControlRigTweenWidget::GetActiveSliderTooltip() const
{
	if (ActiveSlider >= 0 && ActiveSlider < AnimBlendTools.GetAnimSliders().Num())
	{
		return AnimBlendTools.GetAnimSliders()[ActiveSlider].Get()->GetTooltipText();
	}
	return FText();
}

int32 STLLRN_ControlRigTweenWidget::ActiveSlider = 0;


void STLLRN_ControlRigTweenWidget::Construct(const FArguments& InArgs)
{

	TSharedPtr<FBaseAnimSlider> BlendNeighborPtr = MakeShareable(new FBlendNeighborSlider());
	AnimBlendTools.RegisterAnimSlider(BlendNeighborPtr);
	TSharedPtr<FBaseAnimSlider> PushPullPtr = MakeShareable(new FPushPullSlider());
	AnimBlendTools.RegisterAnimSlider(PushPullPtr);
	OwningToolkit = InArgs._InOwningToolkit;
	OwningEditMode = InArgs._InOwningEditMode;
	
	TSharedPtr<FBaseAnimSlider> TweenPtr = MakeShareable(new FControlsToTween());
	AnimBlendTools.RegisterAnimSlider(TweenPtr);
	TSharedPtr<FBaseAnimSlider> BlendRelativePtr = MakeShareable(new FBlendRelativeSlider());
	AnimBlendTools.RegisterAnimSlider(BlendRelativePtr);
	TSharedPtr<FBaseAnimSlider> BlendToEasePtr = MakeShareable(new FBlendToEaseSlider());
	AnimBlendTools.RegisterAnimSlider(BlendToEasePtr);
	TSharedPtr<FBaseAnimSlider> SmoothRoughPtr = MakeShareable(new FSmoothRoughSlider());
	AnimBlendTools.RegisterAnimSlider(SmoothRoughPtr);

	// Combo Button to swap sliders 
	TSharedRef<SComboButton> SliderComoboBtn = SNew(SComboButton)
		.OnGetMenuContent_Lambda([this]()
			{

				FMenuBuilder MenuBuilder(true, NULL); //maybe todo look at settting these up with commands

				MenuBuilder.BeginSection("Anim Sliders");

				int32 Index = 0;
				for (const TSharedPtr<FBaseAnimSlider>& SliderPtr : AnimBlendTools.GetAnimSliders())
				{
					FUIAction ItemAction(FExecuteAction::CreateSP(this, &STLLRN_ControlRigTweenWidget::OnSelectSliderTool, Index));
					MenuBuilder.AddMenuEntry(SliderPtr.Get()->GetText(), TAttribute<FText>(), FSlateIcon(), ItemAction);
					++Index;
				}

				MenuBuilder.EndSection();

				return MenuBuilder.MakeWidget();
			})
			.ButtonContent()
				[
					SNew(SHorizontalBox)
					/*  todo add an icon
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
					[
						SNew(SBox)
						.WidthOverride(16.f)
						.HeightOverride(16.f)
						[
							SNew(SImage)
							.Image_Static(&FLevelEditorToolBar::GetActiveModeIcon, LevelEditorPtr)
						]

					]
					*/
				+SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
						{
							return GetActiveSliderName();
						})
					.ToolTipText_Lambda([this]()
					{
						return GetActiveSliderTooltip();
					})
				]

			];

		TSharedRef<SHorizontalBox> MainBox = SNew(SHorizontalBox);
		TSharedPtr<FBaseAnimSlider> SliderPtr = AnimBlendTools.GetAnimSliders()[ActiveSlider];
		MainBox->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.0f, 2.0f, 2.0f, 2.0f)
		[
			SliderComoboBtn
		];
		MainBox->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.0f, 2.0f, 2.0f, 2.0f)
		[
			SAssignNew(SliderWidget,STLLRN_ControlRigTweenSlider)
			.InAnimSlider(SliderPtr)
			.InWeakEditMode(OwningEditMode)
		];
	
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("EditorViewport.OverlayBrush"))
		[
			MainBox
		]
	];
}

void STLLRN_ControlRigTweenWidget::GetToNextActiveSlider()
{
	int32 Index = ActiveSlider < (AnimBlendTools.GetAnimSliders().Num() - 1) ? ActiveSlider + 1 : 0;
	OnSelectSliderTool(Index);
}

void STLLRN_ControlRigTweenWidget::DragAnimSliderTool(double Val)
{
	if (SliderWidget.IsValid())
	{
		SliderWidget->DragAnimSliderTool(Val);
	}
}

void STLLRN_ControlRigTweenWidget::ResetAnimSlider()
{
	if (SliderWidget.IsValid())
	{
		SliderWidget->ResetAnimSlider();
	}
}

void STLLRN_ControlRigTweenWidget::StartAnimSliderTool()
{
	if (SliderWidget.IsValid())
	{
		SliderWidget->Setup();
	}
}

#undef LOCTEXT_NAMESPACE
