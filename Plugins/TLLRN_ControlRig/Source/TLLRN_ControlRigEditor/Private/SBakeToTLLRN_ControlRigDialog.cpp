// Copyright Epic Games, Inc. All Rights Reserved.

#include "SBakeToTLLRN_ControlRigDialog.h"
#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "BakeToTLLRN_ControlRigSettings.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "PropertyEditorModule.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SButton.h"

class SBakeToTLLRN_ControlRigDialog : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SBakeToTLLRN_ControlRigDialog) {}
	SLATE_END_ARGS()
	~SBakeToTLLRN_ControlRigDialog()
	{
	}
	
	void Construct(const FArguments& InArgs)
	{
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bShowPropertyMatrixButton = false;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.ViewIdentifier = "Bake To Control Rig";

		DetailView = PropertyEditor.CreateDetailView(DetailsViewArgs);

		ChildSlot
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
			[
				DetailView.ToSharedRef()
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(5.f)
			[
				SNew(SButton)
				.ContentPadding(FMargin(10, 5))
			.Text(NSLOCTEXT("TLLRN_ControlRig", "BakeToTLLRN_ControlRig", "Bake To Control Rig"))
			.OnClicked(this, &SBakeToTLLRN_ControlRigDialog::OnBakeToTLLRN_ControlRig)
			]

			];

		UBakeToTLLRN_ControlRigSettings* BakeSettings = GetMutableDefault<UBakeToTLLRN_ControlRigSettings>();
		DetailView->SetObject(BakeSettings);
	}

	void SetDelegate(FBakeToControlDelegate& InDelegate)
	{
		Delegate = InDelegate;
	}

private:

	FReply OnBakeToTLLRN_ControlRig()
	{
		UBakeToTLLRN_ControlRigSettings* BakeSettings = GetMutableDefault<UBakeToTLLRN_ControlRigSettings>();
		TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (BakeSettings && Delegate.IsBound())
		{
			Delegate.Execute(BakeSettings->bReduceKeys, BakeSettings->SmartReduce.TolerancePercentage, BakeSettings->SmartReduce.SampleRate, BakeSettings->bResetControls);
		}
		if (Window.IsValid())
		{
			Window->RequestDestroyWindow();
		}
		return FReply::Handled();
	}
	TSharedPtr<IDetailsView> DetailView;
	FBakeToControlDelegate  Delegate;
};


void BakeToTLLRN_ControlRigDialog::GetBakeParams(FBakeToControlDelegate& InDelegate, const FOnWindowClosed& OnClosedDelegate)
{
	const FText TitleText = NSLOCTEXT("TLLRN_ControlRig", "BakeToTLLRN_ControlRig", "Bake To Control Rig");

	// Create the window to choose our options
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(TitleText)
		.HasCloseButton(true)
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(400.0f, 200.0f))
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false);

	TSharedRef<SBakeToTLLRN_ControlRigDialog> DialogWidget = SNew(SBakeToTLLRN_ControlRigDialog);
	DialogWidget->SetDelegate(InDelegate);
	Window->SetContent(DialogWidget);
	Window->SetOnWindowClosed(OnClosedDelegate);

	FSlateApplication::Get().AddWindow(Window);

}

