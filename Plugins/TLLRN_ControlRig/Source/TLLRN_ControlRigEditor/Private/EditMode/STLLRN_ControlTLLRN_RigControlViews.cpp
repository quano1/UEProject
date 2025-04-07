// Copyright Epic Games, Inc. All Rights Reserved.


#include "EditMode/STLLRN_ControlTLLRN_RigControlViews.h"
#include "TLLRN_ControlRig.h"
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/AppStyle.h"
#include "ScopedTransaction.h"
#include "Editor/EditorEngine.h"
#include "AssetViewUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "AssetThumbnail.h"
#include "FileHelpers.h"
#include "Tools/TLLRN_ControlTLLRN_RigPoseMirrorSettings.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditorModeManager.h"
#include "Widgets/Input/SButton.h"
#include "LevelEditor.h"
#include "TLLRN_ControlRigSequencerEditorLibrary.h"
#include "LevelSequence.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "PropertyEditorModule.h"
#include "STLLRN_ControlRigBaseListWidget.h"
#include "UnrealClient.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigBaseListWidget"


void FTLLRN_ControlRigView::CaptureThumbnail(UObject* Asset)
{
	FViewport* Viewport = GEditor->GetActiveViewport();

	if (GCurrentLevelEditingViewportClient && Viewport)
	{
		//have to re-render the requested viewport
		FLevelEditorViewportClient* OldViewportClient = GCurrentLevelEditingViewportClient;
		//remove selection box around client during render
		GCurrentLevelEditingViewportClient = NULL;
		Viewport->Draw();

		TArray<FAssetData> SelectedAssets;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(Asset));
		SelectedAssets.Emplace(AssetData);
		AssetViewUtils::CaptureThumbnailFromViewport(Viewport, SelectedAssets);

		//redraw viewport to have the yellow highlight again
		GCurrentLevelEditingViewportClient = OldViewportClient;
		Viewport->Draw();
	}
	
}

/** Widget wraps an editable text box for editing name of the asset */
class STLLRN_ControlRigAssetEditableTextBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STLLRN_ControlRigAssetEditableTextBox) {}

		SLATE_ARGUMENT(TWeakObjectPtr<UObject>, Asset)

		SLATE_END_ARGS()

		/**
		 * Construct this widget
		 *
		 * @param	InArgs	The declaration data for this widget
		 */
		void Construct(const FArguments& InArgs);


private:

	/** Getter for the Text attribute of the editable text inside this widget */
	FText GetNameText() const;

	/** Getter for the ToolTipText attribute of the editable text inside this widget */
	FText GetNameTooltipText() const;


	/** Getter for the OnTextCommitted event of the editable text inside this widget */
	void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit);

	/** Callback to verify a text change */
	void OnTextChanged(const FText& InLabel);

	/** The list of objects whose names are edited by the widget */
	TWeakObjectPtr<UObject> Asset;

	/** The text box used to edit object names */
	TSharedPtr< STextBlock > TextBox;

};

void STLLRN_ControlRigAssetEditableTextBox::Construct(const FArguments& InArgs)
{
	Asset = InArgs._Asset;
	ChildSlot
		[
			SAssignNew(TextBox, STextBlock)
			.Text(this, &STLLRN_ControlRigAssetEditableTextBox::GetNameText)
			// Current Thinking is to not have this be editable here, so removing it, but leaving in case we change our minds again.
			//.ToolTipText(this, &STLLRN_ControlRigAssetEditableTextBox::GetNameTooltipText)
			//.OnTextCommitted(this, &STLLRN_ControlRigAssetEditableTextBox::OnNameTextCommitted)
			//.OnTextChanged(this, &STLLRN_ControlRigAssetEditableTextBox::OnTextChanged)
			//.RevertTextOnEscape(true)
		];
}

FText STLLRN_ControlRigAssetEditableTextBox::GetNameText() const
{
	if (Asset.IsValid())
	{
		FString Result = Asset.Get()->GetName();
		return FText::FromString(Result);
	}
	return FText();
}

FText STLLRN_ControlRigAssetEditableTextBox::GetNameTooltipText() const
{
	FText Result = FText::Format(LOCTEXT("AssetRenameTooltip", "Rename the selected {0}"), FText::FromString(Asset.Get()->GetClass()->GetName()));
	
	return Result;
}


void STLLRN_ControlRigAssetEditableTextBox::OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{

	if (InTextCommit != ETextCommit::OnCleared)
	{
		FText TrimmedText = FText::TrimPrecedingAndTrailing(NewText);

		if (!TrimmedText.IsEmpty())
		{

			IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
			FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(Asset.Get()));
			const FString PackagePath = FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());

			//Need to save asset before renaming else may lose snapshot
			// save existing play list asset
			TArray<UPackage*> PackagesToSave;
			PackagesToSave.Add(Asset->GetPackage());
			FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false /*bCheckDirty*/, false /*bPromptToSave*/);

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

			TArray<FAssetRenameData> AssetsAndNames;
			AssetsAndNames.Emplace(FAssetRenameData(Asset, PackagePath, TrimmedText.ToString()));
			AssetToolsModule.Get().RenameAssetsWithDialog(AssetsAndNames);

		}
			
		// Remove ourselves from the window focus so we don't get automatically reselected when scrolling around the context menu.
		TSharedPtr< SWindow > ParentWindow = FSlateApplication::Get().FindWidgetWindow(SharedThis(this));
		if (ParentWindow.IsValid())
		{
			ParentWindow->SetWidgetToFocusOnActivate(NULL);
		}
	}

	// Clear Error 
	//TextBox->SetError(FText::GetEmpty());
}

void STLLRN_ControlRigAssetEditableTextBox::OnTextChanged(const FText& InLabel)
{
	const FString PackagePath = FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	const FString PackageName = PackagePath / InLabel.ToString();
	const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *(InLabel.ToString()));

	FText OutErrorMessage;
	if (!AssetViewUtils::IsValidObjectPathForCreate(ObjectPath, OutErrorMessage))
	{
		//TextBox->SetError(OutErrorMessage);
	}
	else
	{
		//TextBox->SetError(FText::GetEmpty());
	}
}

bool STLLRN_ControlTLLRN_RigPoseView::bIsKey = false;
bool STLLRN_ControlTLLRN_RigPoseView::bIsMirror = false;

void STLLRN_ControlTLLRN_RigPoseView::Construct(const FArguments& InArgs)
{
	PoseAsset = InArgs._PoseAsset;
	OwningWidget = InArgs._OwningWidget;

	PoseBlendValue = 0.0f;
	bIsBlending = false;
	bSliderStartedTransaction = false;

	TSharedRef<SWidget> ThumbnailWidget = GetThumbnailWidget();
	TSharedRef <STLLRN_ControlRigAssetEditableTextBox> ObjectNameBox = SNew(STLLRN_ControlRigAssetEditableTextBox).Asset(PoseAsset);

	//Not used currently CreateControlList();

	//for mirror settings
	UTLLRN_ControlTLLRN_RigPoseMirrorSettings* MirrorSettings = GetMutableDefault<UTLLRN_ControlTLLRN_RigPoseMirrorSettings>();
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.ViewIdentifier = "Create Control Asset";

	MirrorDetailsView = PropertyEditor.CreateDetailView(DetailsViewArgs);
	MirrorDetailsView->SetObject(MirrorSettings);
			
	ChildSlot
	[

		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		
			.FillHeight(1)
			.Padding(0, 0, 0, 4)
			[
			SNew(SSplitter)

				+ SSplitter::Slot()
				.Value(0.33f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(5.f)
						[
							SNew(SBox)
							.VAlign(VAlign_Center)
						[
							ObjectNameBox
						]
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(5.f)
						[
							SNew(SBox)
							.VAlign(VAlign_Center)
						[
							ThumbnailWidget
						]
						]
					+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(5.f)

						[
							SNew(SButton)
							.ContentPadding(FMargin(10, 5))
						.Text(LOCTEXT("CaptureThmbnail", "Capture Thumbnail"))
						.ToolTipText(LOCTEXT("CaptureThmbnailTooltip", "Captures a thumbnail from the active viewport"))
						.OnClicked(this, &STLLRN_ControlTLLRN_RigPoseView::OnCaptureThumbnail)
						]
					]
				]

			+ SSplitter::Slot()
				.Value(0.33f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(5.f)

							[
							SNew(SButton)
								.ContentPadding(FMargin(10, 5))
							.Text(LOCTEXT("PastePose", "Paste Pose"))
							.OnClicked(this, &STLLRN_ControlTLLRN_RigPoseView::OnPastePose)
							]
						+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(2.5f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.Padding(2.5f)
							[
								SNew(SCheckBox)
								.IsChecked(this, &STLLRN_ControlTLLRN_RigPoseView::IsKeyPoseChecked)
							.OnCheckStateChanged(this, &STLLRN_ControlTLLRN_RigPoseView::OnKeyPoseChecked)
							.Padding(2.5f)

							[
								SNew(STextBlock).Text(LOCTEXT("Key", "Key"))

							]
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()

							.HAlign(HAlign_Center)

							.Padding(2.5f)
							[
								SNew(SCheckBox)
								.IsChecked(this, &STLLRN_ControlTLLRN_RigPoseView::IsMirrorPoseChecked)
							.OnCheckStateChanged(this, &STLLRN_ControlTLLRN_RigPoseView::OnMirrorPoseChecked)
							.IsEnabled(this, &STLLRN_ControlTLLRN_RigPoseView::IsMirrorEnabled)
							.Padding(1.0f)
							[
								SNew(STextBlock).Text(LOCTEXT("Mirror", "Mirror"))
								.IsEnabled(this, &STLLRN_ControlTLLRN_RigPoseView::IsMirrorEnabled)
							]
							]
							]
						+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(2.5f)
							[

								SNew(SSpinBox<float>)
								// Only allow spinning if we have a single value
								.PreventThrottling(true)
								.Value(this, &STLLRN_ControlTLLRN_RigPoseView::OnGetPoseBlendValue)
								.ToolTipText(LOCTEXT("BlendTooltip", "Blend between current pose and pose asset. Use Ctrl drag for under and over shoot."))
								.MinValue(0.0f)
								.MaxValue(1.0f)
								.MinSliderValue(0.0f)
								.MaxSliderValue(1.0f)
								.SliderExponent(1)
								.Delta(0.005f)
								.MinDesiredWidth(100.0f)
								.SupportDynamicSliderMinValue(true)
								.SupportDynamicSliderMaxValue(true)
								.OnValueChanged(this, &STLLRN_ControlTLLRN_RigPoseView::OnPoseBlendChanged)
								.OnValueCommitted(this, &STLLRN_ControlTLLRN_RigPoseView::OnPoseBlendCommited)
								.OnBeginSliderMovement(this,&STLLRN_ControlTLLRN_RigPoseView::OnBeginSliderMovement)
								.OnEndSliderMovement(this,&STLLRN_ControlTLLRN_RigPoseView::OnEndSliderMovement)

							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(15.f)
							[
								SNew(SButton)
								.ContentPadding(FMargin(10, 5))
								.Text(LOCTEXT("SelectControls", "Select Controls"))
								.OnClicked(this, &STLLRN_ControlTLLRN_RigPoseView::OnSelectControls)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(3.f)
							[
								SNew(SBorder)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								.Padding(FMargin(3.0f, 2.0f))
								.Visibility(EVisibility::HitTestInvisible)
								[
									SAssignNew(TextStatusBlock1, STextBlock)
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(1.0f)
								[
									SNew(SBorder)
									.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								.Padding(FMargin(3.0f, 0.0f))
								.Visibility(EVisibility::HitTestInvisible)
								[
									SAssignNew(TextStatusBlock2, STextBlock)
								]
								]
					]
				]
			+ SSplitter::Slot()
				.Value(0.33f)
				[
					MirrorDetailsView.ToSharedRef()
				]
			/*  todo may want to put this back, it let's you see the controls...
			+ SSplitter::Slot()
				.Value(0.33f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(2.f)
							[

							SNew(SButton)
								.ContentPadding(FMargin(5, 5))
								.Text(LOCTEXT("SelectControls", "Select Controls"))
								.ToolTipText(LOCTEXT("SelectControlsTooltip", "Select controls from this asset"))
								.OnClicked(this, &STLLRN_ControlTLLRN_RigPoseView::OnSelectControls)
				
							]
						+ SVerticalBox::Slot()
							.VAlign(VAlign_Fill)
							.HAlign(HAlign_Center)
							.Padding(5.f)

							[
							SNew(SListView< TSharedPtr<FString> >)
								.ItemHeight(24)
								.ListItemsSource(&ControlList)
								.SelectionMode(ESelectionMode::None)
								.OnGenerateRow(this, &STLLRN_ControlTLLRN_RigPoseView::OnGenerateWidgetForList)
							]
					]
				]
				*/
			]
	];
	if (OwningWidget.IsValid())
	{
		if (FTLLRN_ControlRigEditMode* EditMode = OwningWidget.Pin()->GetEditMode())
		{
			EditMode->OnTLLRN_ControlRigAddedOrRemoved().AddRaw(this, &STLLRN_ControlTLLRN_RigPoseView::HandleControlAdded);
			TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs = GetTLLRN_ControlRigs();
			for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
			{
				HandleControlAdded(TLLRN_ControlRig, true);
			}
		}
	}
}

STLLRN_ControlTLLRN_RigPoseView::~STLLRN_ControlTLLRN_RigPoseView()
{
	if(OwningWidget.IsValid() && OwningWidget.Pin()->GetEditMode())
	{
		FTLLRN_ControlRigEditMode* EditMode = OwningWidget.Pin()->GetEditMode();
		EditMode->OnTLLRN_ControlRigAddedOrRemoved().RemoveAll(this);
		TArray<UTLLRN_ControlRig*> EditModeRigs = EditMode->GetTLLRN_ControlRigsArray(false /*bIsVisible*/);
		for (UTLLRN_ControlRig* TLLRN_ControlRig : EditModeRigs)
		{
			if (TLLRN_ControlRig)
			{
				TLLRN_ControlRig->ControlSelected().RemoveAll(this);
			}
		}
	}
	else
	{
		for (TWeakObjectPtr<UTLLRN_ControlRig>& CurrentTLLRN_ControlRig: CurrentTLLRN_ControlRigs)
		{
			if (CurrentTLLRN_ControlRig.IsValid())
			{
				(CurrentTLLRN_ControlRig.Get())->ControlSelected().RemoveAll(this);
			}
		}
	}
}

void STLLRN_ControlTLLRN_RigPoseView::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	UpdateStatusBlocks();
}

ECheckBoxState STLLRN_ControlTLLRN_RigPoseView::IsKeyPoseChecked() const
{
	if (bIsKey)
	{
		return ECheckBoxState::Checked;
	}
	return ECheckBoxState::Unchecked;
}

void STLLRN_ControlTLLRN_RigPoseView::OnKeyPoseChecked(ECheckBoxState NewState)
{
	if (NewState == ECheckBoxState::Checked)
	{
		bIsKey = true;
	}
	else
	{
		bIsKey = false;
	}
}

ECheckBoxState STLLRN_ControlTLLRN_RigPoseView::IsMirrorPoseChecked() const
{
	if (bIsMirror)
	{
		return ECheckBoxState::Checked;
	}
	return ECheckBoxState::Unchecked;
}

void STLLRN_ControlTLLRN_RigPoseView::OnMirrorPoseChecked(ECheckBoxState NewState)
{
	if (NewState == ECheckBoxState::Checked)
	{
		bIsMirror = true;
	}
	else
	{
		bIsMirror = false;
	}
	UpdateStatusBlocks();
}

bool STLLRN_ControlTLLRN_RigPoseView::IsMirrorEnabled() const
{
	return true;
}


FReply STLLRN_ControlTLLRN_RigPoseView::OnPastePose()
{
	if (PoseAsset.IsValid())
	{
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs = GetTLLRN_ControlRigs();
		for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
		{
			if (TLLRN_ControlRig)
			{
				PoseAsset->PastePose(TLLRN_ControlRig, bIsKey, bIsMirror);
			}
		}
	}
	return FReply::Handled();
}

FReply STLLRN_ControlTLLRN_RigPoseView::OnSelectControls()
{	
	if (PoseAsset.IsValid())
	{
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs = GetTLLRN_ControlRigs();
		for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
		{
			if (TLLRN_ControlRig)
			{
				PoseAsset->SelectControls(TLLRN_ControlRig, bIsMirror);
			}
		}
	}
	return FReply::Handled();
}

void STLLRN_ControlTLLRN_RigPoseView::OnPoseBlendChanged(float ChangedVal)
{
	auto TLLRN_ControlRigContainsControls = [this] (const UTLLRN_ControlRig* InTLLRN_ControlRig)
	{
		TArray<FName> ControlNames = PoseAsset->GetControlNames();
		for (FName ControlName : ControlNames)
		{
			if (InTLLRN_ControlRig->FindControl(ControlName))
			{
				return true;
			}
		}
		return false;
	};
	
	if (PoseAsset.IsValid())
	{
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs = GetTLLRN_ControlRigs();
		for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
		{
			if (TLLRN_ControlRig && TLLRN_ControlRigContainsControls(TLLRN_ControlRig))
			{
				PoseBlendValue = ChangedVal;
				if (!bIsBlending)
				{
					bIsBlending = true;
					PoseAsset->GetCurrentPose(TLLRN_ControlRig, TempPose);
				}

				PoseAsset->BlendWithInitialPoses(TempPose, TLLRN_ControlRig, false, bIsMirror, PoseBlendValue);
				PoseAsset->BlendWithInitialPoses(TempPose, TLLRN_ControlRig, false, bIsMirror, PoseBlendValue);
			}
		}
	}
}
void STLLRN_ControlTLLRN_RigPoseView::OnBeginSliderMovement()
{
	if (bSliderStartedTransaction == false)
	{
		bSliderStartedTransaction = true;
		GEditor->BeginTransaction(LOCTEXT("PastePoseTransation", "Paste Pose"));
	}
}
void STLLRN_ControlTLLRN_RigPoseView::OnEndSliderMovement(float NewValue)
{
	if (bSliderStartedTransaction)
	{
		GEditor->EndTransaction();
		bSliderStartedTransaction = false;

	}
}

void STLLRN_ControlTLLRN_RigPoseView::OnPoseBlendCommited(float ChangedVal, ETextCommit::Type Type)
{
	if (PoseAsset.IsValid())
	{
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs = GetTLLRN_ControlRigs();
		if (TLLRN_ControlRigs.Num() > 0)
		{
			FScopedTransaction ScopedTransaction(LOCTEXT("PastePoseTransaction", "Paste Pose"));
			for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
			{
				if (TLLRN_ControlRig)
				{
					PoseBlendValue = ChangedVal;
					PoseAsset->BlendWithInitialPoses(TempPose, TLLRN_ControlRig, bIsKey, bIsMirror, PoseBlendValue);
					PoseAsset->BlendWithInitialPoses(TempPose, TLLRN_ControlRig, bIsKey, bIsMirror, PoseBlendValue);
					bIsBlending = false;
					PoseBlendValue = 0.0f;

				}
			}
		}
	}
}

FReply STLLRN_ControlTLLRN_RigPoseView::OnCaptureThumbnail()
{
	FTLLRN_ControlRigView::CaptureThumbnail(PoseAsset.Get());
	return FReply::Handled();
}

TSharedRef<SWidget> STLLRN_ControlTLLRN_RigPoseView::GetThumbnailWidget()
{
	const int32 ThumbnailSize = 128;
	Thumbnail = MakeShareable(new FAssetThumbnail(PoseAsset.Get(), ThumbnailSize, ThumbnailSize, UThumbnailManager::Get().GetSharedThumbnailPool()));
	FAssetThumbnailConfig ThumbnailConfig;
	ThumbnailConfig.bAllowFadeIn = false;
	ThumbnailConfig.bAllowHintText = false;
	ThumbnailConfig.bAllowRealTimeOnHovered = false; // we use our own OnMouseEnter/Leave for logical asset item
	ThumbnailConfig.bForceGenericThumbnail = false;

	TSharedRef<SOverlay> ItemContentsOverlay = SNew(SOverlay);
	ItemContentsOverlay->AddSlot()
		[
			Thumbnail->MakeThumbnailWidget(ThumbnailConfig)
		];

	return SNew(SBox)
		.Padding(0)
		.WidthOverride(ThumbnailSize)
		.HeightOverride(ThumbnailSize)
		[
			ItemContentsOverlay

		];
}

TArray<UTLLRN_ControlRig*> STLLRN_ControlTLLRN_RigPoseView::GetTLLRN_ControlRigs()
{
	TArray<UTLLRN_ControlRig*> NewTLLRN_ControlRigs;
	if (OwningWidget.IsValid())
	{
		FTLLRN_ControlRigEditMode* EditMode = OwningWidget.Pin()->GetEditMode();
		if (EditMode)
		{
			NewTLLRN_ControlRigs =  EditMode->GetTLLRN_ControlRigsArray(false /*bIsVisible*/);
		}
		for (TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRigPtr : CurrentTLLRN_ControlRigs)
		{
			if (TLLRN_ControlRigPtr.IsValid())
			{
				if (NewTLLRN_ControlRigs.Contains(TLLRN_ControlRigPtr.Get()) == false)
				{
					(TLLRN_ControlRigPtr.Get())->ControlSelected().RemoveAll(this);
				}
			}
		}
		if (EditMode)
		{
			CurrentTLLRN_ControlRigs = EditMode->GetTLLRN_ControlRigs();
		}
	}
	
	return NewTLLRN_ControlRigs;
}

/* We may want to list the Controls in it (design said no but animators said yes)
void STLLRN_ControlTLLRN_RigPoseView::CreateControlList()
{
	if (PoseAsset.IsValid())
	{
		const TArray<FName>& Controls = PoseAsset.Get()->GetControlNames();
		for (const FName& ControlName : Controls)
		{
			ControlList.Add(MakeShared<FString>(ControlName.ToString()));
		}
	}
}
*/
void STLLRN_ControlTLLRN_RigPoseView::HandleControlAdded(UTLLRN_ControlRig* TLLRN_ControlRig, bool bIsAdded)
{
	if (TLLRN_ControlRig)
	{
		if (bIsAdded)
		{
			(TLLRN_ControlRig)->ControlSelected().RemoveAll(this);
			(TLLRN_ControlRig)->ControlSelected().AddRaw(this, &STLLRN_ControlTLLRN_RigPoseView::HandleControlSelected);
		}
		else
		{
			(TLLRN_ControlRig)->ControlSelected().RemoveAll(this);
		}
	}
	UpdateStatusBlocks();
}

void STLLRN_ControlTLLRN_RigPoseView::HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControl, bool bSelected)
{
	UpdateStatusBlocks();
}

void STLLRN_ControlTLLRN_RigPoseView::UpdateStatusBlocks()
{
	FText StatusText1;
	FText StatusText2;
	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs = GetTLLRN_ControlRigs();
	if (PoseAsset.IsValid() && TLLRN_ControlRigs.Num() > 0)
	{

		FFormatNamedArguments NamedArgs;
		TArray<FName> ControlNames = PoseAsset->GetControlNames();
		NamedArgs.Add("Total", ControlNames.Num());
		int32 TotalSelected = 0;
		uint32 Matching = 0;
		uint32 MirrorMatching = 0;
		for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
		{

			TArray<FName> SelectedNames = TLLRN_ControlRig->CurrentControlSelection();
			TotalSelected += SelectedNames.Num();
			for (const FName& ControlName : ControlNames)
			{
				for (const FName& SelectedName : SelectedNames)
				{
					if (SelectedName == ControlName)
					{
						++Matching;
						if (STLLRN_ControlTLLRN_RigPoseView::bIsMirror)
						{
							if (PoseAsset->DoesMirrorMatch(TLLRN_ControlRig, ControlName))
							{
								++MirrorMatching;
							}
						}
					}
				}
			}
		}

		NamedArgs.Add("Selected", TotalSelected);//SelectedNames.Num());
		NamedArgs.Add("Matching", Matching);
		NamedArgs.Add("MirrorMatching", MirrorMatching);

		if (STLLRN_ControlTLLRN_RigPoseView::bIsMirror)
		{
			StatusText1 = FText::Format(LOCTEXT("NumberControlsAndMatch", "{Total} Controls Matching {Matching} of {Selected} Selected"), NamedArgs);
			StatusText2 = FText::Format(LOCTEXT("NumberMirroredMatch", " {MirrorMatching} Mirror String Matches"), NamedArgs);
		}
		else
		{
			StatusText1 = FText::Format(LOCTEXT("NumberControlsAndMatch", "{Total} Controls Matching {Matching} of {Selected} Selected"), NamedArgs);
			StatusText2 = FText::GetEmpty();
		}
	}
	else
	{
		StatusText1 = FText::GetEmpty();
		StatusText2 = FText::GetEmpty();
	}
	if (TextStatusBlock1.IsValid())
	{
		TextStatusBlock1->SetText(StatusText1);
	}
	if (TextStatusBlock2.IsValid())
	{
		TextStatusBlock2->SetText(StatusText2);
	}
}

#undef LOCTEXT_NAMESPACE
