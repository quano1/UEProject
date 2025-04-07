// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/STLLRN_ControlRigUpdatePose.h"
#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SButton.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "EditorModeManager.h"
#include "ScopedTransaction.h"

class STLLRN_ControlRigUpdatePoseDialog : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STLLRN_ControlRigUpdatePoseDialog) {}
	SLATE_END_ARGS()
	~STLLRN_ControlRigUpdatePoseDialog()
	{
	}
	
	void Construct(const FArguments& InArgs)
	{

		ChildSlot
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(5.f)
				[
				SNew(STextBlock)
				.Text(NSLOCTEXT("TLLRN_ControlRig", "UpdatePoseWithSelectedControls", "Update Pose With Selected Controls?"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(5.f)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.Padding(10)
					[
						SNew(SButton)
						.ContentPadding(FMargin(10, 5))
						.Text(NSLOCTEXT("TLLRN_ControlRig", "Yes", "Yes"))
						.OnClicked(this, &STLLRN_ControlRigUpdatePoseDialog::UpdatePose)
						.IsEnabled(this, &STLLRN_ControlRigUpdatePoseDialog::CanUpdatePose)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.Padding(10)
					[
						SNew(SButton)
						.ContentPadding(FMargin(10, 5))
						.Text(NSLOCTEXT("TLLRN_ControlRig", "No", "No"))
						.OnClicked(this, &STLLRN_ControlRigUpdatePoseDialog::CancelUpdatePose)
					]
				]
			];
	}

	void SetPose(UTLLRN_ControlTLLRN_RigPoseAsset* InPoseAsset)
	{
		PoseAsset = InPoseAsset;
	}

private:

	bool CanUpdatePose() const
	{
		FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
		if (PoseAsset.IsValid() && TLLRN_ControlRigEditMode)
		{
			TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> AllSelectedControls;
			TLLRN_ControlRigEditMode->GetAllSelectedControls(AllSelectedControls);
			return (AllSelectedControls.Num() > 0);
		}
		return false;

	}
	FReply UpdatePose()
	{
		TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared());

		FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
		if (PoseAsset.IsValid() && TLLRN_ControlRigEditMode)
		{
			TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> AllSelectedControls;
			TLLRN_ControlRigEditMode->GetAllSelectedControls(AllSelectedControls);
			if (AllSelectedControls.Num() == 1)
			{
				TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
				AllSelectedControls.GenerateKeyArray(TLLRN_ControlRigs);
				const FScopedTransaction Transaction(NSLOCTEXT("TLLRN_ControlRig", "UpdatePose", "Update Pose"));
				PoseAsset->Modify();
				PoseAsset->SavePose(TLLRN_ControlRigs[0], false);
			}
			else
			{

			}
		}
		if (Window.IsValid())
		{
			Window->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	FReply CancelUpdatePose()
	{
		TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (Window.IsValid())
		{
			Window->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	TSharedPtr<IDetailsView> DetailView;
	TWeakObjectPtr<UTLLRN_ControlTLLRN_RigPoseAsset> PoseAsset;
};


void FTLLRN_ControlRigUpdatePoseDialog::UpdatePose(UTLLRN_ControlTLLRN_RigPoseAsset* PoseAsset)
{
	const FText TitleText = NSLOCTEXT("TLLRN_ControlRig", "UpdateTLLRN_ControlTLLRN_RigPose", "Update Control Rig Pose");

	// Create the window to choose our options
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(TitleText)
		.HasCloseButton(true)
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(400.0f, 100.0f))
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false);

	TSharedRef<STLLRN_ControlRigUpdatePoseDialog> DialogWidget = SNew(STLLRN_ControlRigUpdatePoseDialog);
	DialogWidget->SetPose(PoseAsset);
	Window->SetContent(DialogWidget);

	FSlateApplication::Get().AddWindow(Window);
}

