// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/STLLRN_ControlRigEditModeTools.h"
#include "EditMode/TLLRN_ControlTLLRN_RigControlsProxy.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "ISequencer.h"
#include "PropertyHandle.h"
#include "TLLRN_ControlRig.h"
#include "EditMode/TLLRN_ControlRigEditModeSettings.h"
#include "IDetailRootObjectCustomization.h"
#include "Modules/ModuleManager.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditorModeManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "MovieSceneSequence.h"
#include "MovieScene.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Rigs/FKTLLRN_ControlRig.h"
#include "EditMode/STLLRN_ControlRigBaseListWidget.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ITLLRN_ControlRigEditorModule.h"
#include "Framework/Docking/TabManager.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "LevelEditor.h"
#include "EditorModeManager.h"
#include "InteractiveToolManager.h"
#include "EdModeInteractiveToolsContext.h"
#include "TLLRN_ControlTLLRN_RigSpaceChannelEditors.h"
#include "IKeyArea.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/Notifications/NotificationManager.h"
#include "ScopedTransaction.h"
#include "TransformConstraint.h"
#include "EditMode/TLLRN_ControlRigEditModeToolkit.h"
#include "EditMode/STLLRN_ControlRigDetails.h"
#include "Editor/Constraints/SConstraintsWidget.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigEditModeTools"

//statics to reuse in the UI
FTLLRN_TLLRN_RigSpacePickerBakeSettings STLLRN_ControlRigEditModeTools::BakeSpaceSettings;

void STLLRN_ControlRigEditModeTools::Cleanup()
{
	// This is required as these hold a shared pointer to THIS OBJECT and make this class not to be destroyed when the parent class releases the shared pointer of this object
	if(FSlateApplication::IsInitialized())
	{
		if (SettingsDetailsView)
		{
			SettingsDetailsView->SetKeyframeHandler(nullptr);
		}
		if (RigOptionsDetailsView)
		{
			RigOptionsDetailsView->SetKeyframeHandler(nullptr);
		}
	}
	for (TPair<FDelegateHandle, TWeakObjectPtr<UTLLRN_ControlRig>>& Handles : HandlesToClear)
	{
		if (Handles.Value.IsValid())
		{
			Handles.Value->ControlSelected().RemoveAll(this);
		}
		if (Handles.Key.IsValid())
		{
			Handles.Key.Reset();
		}
	}
	HandlesToClear.Reset();
}

void STLLRN_ControlRigEditModeTools::SetTLLRN_ControlRigs(const TArrayView<TWeakObjectPtr<UTLLRN_ControlRig>>& InTLLRN_ControlRigs)
{
	for (TPair<FDelegateHandle, TWeakObjectPtr<UTLLRN_ControlRig>>& Handles : HandlesToClear)
	{
		if (Handles.Value.IsValid())
		{
			Handles.Value->ControlSelected().RemoveAll(this);
		}
		if (Handles.Key.IsValid())
		{
			Handles.Key.Reset();
		}
	}
	HandlesToClear.Reset();
	TLLRN_ControlRigs = InTLLRN_ControlRigs;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& InTLLRN_ControlRig : InTLLRN_ControlRigs)
	{
		if (InTLLRN_ControlRig.IsValid())
		{
			TPair<FDelegateHandle, TWeakObjectPtr<UTLLRN_ControlRig>> Handles;
			Handles.Key = InTLLRN_ControlRig->ControlSelected().AddRaw(this, &STLLRN_ControlRigEditModeTools::OnTLLRN_RigElementSelected);
			Handles.Value = InTLLRN_ControlRig;
			HandlesToClear.Add(Handles);
		}
	}

	//mz todo handle multiple rigs
	TArray<TWeakObjectPtr<>> Objects;
	if (TLLRN_ControlRigs.Num() > 0)
	{
		UTLLRN_ControlRig* Rig = TLLRN_ControlRigs[0].Get();
		Objects.Add(Rig);
	}
	RigOptionsDetailsView->SetObjects(Objects);

#if USE_LOCAL_DETAILS
	HierarchyTreeView->RefreshTreeView(true);
#endif
}

const UTLLRN_RigHierarchy* STLLRN_ControlRigEditModeTools::GetHierarchy() const
{
	//mz todo handle multiple rigs
	UTLLRN_ControlRig* Rig = TLLRN_ControlRigs.Num() > 0 ? TLLRN_ControlRigs[0].Get() : nullptr;
	if (Rig)
	{
		Rig->GetHierarchy();
	}
	return nullptr;
}

void STLLRN_ControlRigEditModeTools::Construct(const FArguments& InArgs, TSharedPtr<FTLLRN_ControlRigEditModeToolkit> InOwningToolkit, FTLLRN_ControlRigEditMode& InEditMode)
{
	bIsChangingTLLRN_RigHierarchy = false;
	OwningToolkit = InOwningToolkit;
	// initialize settings view
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = true;
		DetailsViewArgs.bCustomNameAreaLocation = true;
		DetailsViewArgs.bCustomFilterAreaLocation = true;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bAllowMultipleTopLevelObjects = false;
		DetailsViewArgs.bShowScrollBar = false; // Don't need to show this, as we are putting it in a scroll box
	}
	
	ModeTools = InEditMode.GetModeManager();

	SettingsDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	SettingsDetailsView->SetKeyframeHandler(SharedThis(this));
	SettingsDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	SettingsDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	SettingsDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));
#if USE_LOCAL_DETAILS
	ControlEulerTransformDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ControlEulerTransformDetailsView->SetKeyframeHandler(SharedThis(this));
	ControlEulerTransformDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	ControlEulerTransformDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	ControlEulerTransformDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));

	ControlTransformDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ControlTransformDetailsView->SetKeyframeHandler(SharedThis(this));
	ControlTransformDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	ControlTransformDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	ControlTransformDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));

	ControlTransformNoScaleDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ControlTransformNoScaleDetailsView->SetKeyframeHandler(SharedThis(this));
	ControlTransformNoScaleDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	ControlTransformNoScaleDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	ControlTransformNoScaleDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));

	ControlFloatDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ControlFloatDetailsView->SetKeyframeHandler(SharedThis(this));
	ControlFloatDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	ControlFloatDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	ControlFloatDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));

	ControlEnumDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ControlEnumDetailsView->SetKeyframeHandler(SharedThis(this));
	ControlEnumDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	ControlEnumDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	ControlEnumDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));

	ControlIntegerDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ControlIntegerDetailsView->SetKeyframeHandler(SharedThis(this));
	ControlIntegerDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	ControlIntegerDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	ControlIntegerDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));

	ControlBoolDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ControlBoolDetailsView->SetKeyframeHandler(SharedThis(this));
	ControlBoolDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	ControlBoolDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	ControlBoolDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));

	ControlVectorDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ControlVectorDetailsView->SetKeyframeHandler(SharedThis(this));
	ControlVectorDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	ControlVectorDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	ControlVectorDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));

	ControlVector2DDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ControlVector2DDetailsView->SetKeyframeHandler(SharedThis(this));
	ControlVector2DDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	ControlVector2DDetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	ControlVector2DDetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));
#endif

	RigOptionsDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	RigOptionsDetailsView->SetKeyframeHandler(SharedThis(this));
	RigOptionsDetailsView->OnFinishedChangingProperties().AddSP(this, &STLLRN_ControlRigEditModeTools::OnRigOptionFinishedChange);

	DisplaySettings.bShowBones = false;
	DisplaySettings.bShowControls = true;
	DisplaySettings.bShowNulls = false;
	DisplaySettings.bShowReferences = false;
	DisplaySettings.bShowSockets = false;
	DisplaySettings.bShowPhysics = false;
	DisplaySettings.bHideParentsOnFilter = true;
	DisplaySettings.bFlattenHierarchyOnFilter = true;
	DisplaySettings.bShowIconColors = true;
#if USE_LOCAL_DETAILS
	FRigTreeDelegates RigTreeDelegates;
	RigTreeDelegates.OnGetHierarchy = FOnGetRigTreeHierarchy::CreateSP(this, &STLLRN_ControlRigEditModeTools::GetHierarchy);
	RigTreeDelegates.OnGetDisplaySettings = FOnGetRigTreeDisplaySettings::CreateSP(this, &STLLRN_ControlRigEditModeTools::GetDisplaySettings);
	RigTreeDelegates.OnSelectionChanged = FOnRigTreeSelectionChanged::CreateSP(this, &STLLRN_ControlRigEditModeTools::HandleSelectionChanged);
#endif


	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
#if USE_LOCAL_DETAILS
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(PickerExpander, SExpandableArea)
				.InitiallyCollapsed(true)
				.AreaTitle(LOCTEXT("Picker_Header", "Controls"))
				.AreaTitleFont(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
				.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f))
				.BodyContent()
				[
					SAssignNew(HierarchyTreeView, STLLRN_RigHierarchyTreeView)
					.RigTreeDelegates(RigTreeDelegates)
				]
			]
#endif
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SettingsDetailsView.ToSharedRef()
			]
#if USE_LOCAL_DETAILS
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ControlEulerTransformDetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ControlTransformDetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ControlTransformNoScaleDetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ControlBoolDetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ControlIntegerDetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ControlEnumDetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ControlVectorDetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ControlVector2DDetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ControlFloatDetailsView.ToSharedRef()
			]
#endif

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
						.OnClicked(this, &STLLRN_ControlRigEditModeTools::HandleAddSpaceClicked)
						.Cursor(EMouseCursor::Default)
						.ToolTipText(LOCTEXT("AddSpace", "Add Space"))
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
						]
						.Visibility_Lambda([this]()->EVisibility
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
					.GetControlCustomization(this, &STLLRN_ControlRigEditModeTools::HandleGetControlElementCustomization)
					.OnActiveSpaceChanged(this, &STLLRN_ControlRigEditModeTools::HandleActiveSpaceChanged)
					.OnSpaceListChanged(this, &STLLRN_ControlRigEditModeTools::HandleSpaceListChanged)
					.OnCompensateKeyButtonClicked(this, &STLLRN_ControlRigEditModeTools::OnCompensateKeyClicked)
					.OnCompensateAllButtonClicked(this, &STLLRN_ControlRigEditModeTools::OnCompensateAllClicked)
					.OnBakeButtonClicked(this, &STLLRN_ControlRigEditModeTools::OnBakeControlsToNewSpaceButtonClicked)					// todo: implement GetAdditionalSpacesDelegate to pull spaces from sequencer
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(ConstraintPickerExpander, SExpandableArea)
				.InitiallyCollapsed(true)
				.AreaTitle(LOCTEXT("ConstraintsWidget", "Constraints"))
				.AreaTitleFont(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
				.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f))
				.Padding(FMargin(8.f))
				.HeaderContent()
				[
					SNew(SHorizontalBox)

					// "Constraints" label
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ConstraintsWidget", "Constraints"))
						.Font(FCoreStyle::Get().GetFontStyle("ExpandableArea.TitleFont"))
					]

					// Spacer
					+SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(SSpacer)
					]
					// "Selected" button
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(0.f, 2.f, 8.f, 2.f)
					[
						// Combo Button to swap what constraints we show  
						SNew(SComboButton)
						.OnGetMenuContent_Lambda([this]()
						{
							FMenuBuilder MenuBuilder(true, NULL);
							MenuBuilder.BeginSection("Constraints");
							if (ConstraintsEditionWidget.IsValid())
							{
								for (int32 Index = 0; Index < 4; ++Index)
								{
									FUIAction ItemAction(FExecuteAction::CreateSP(this, &STLLRN_ControlRigEditModeTools::OnSelectShowConstraints, Index));
									const TAttribute<FText> Text = ConstraintsEditionWidget->GetShowConstraintsText((FBaseConstraintListWidget::EShowConstraints)(Index));
									const TAttribute<FText> Tooltip = ConstraintsEditionWidget->GetShowConstraintsTooltip((FBaseConstraintListWidget::EShowConstraints)(Index));
									MenuBuilder.AddMenuEntry(Text, Tooltip, FSlateIcon(), ItemAction);
								}
							}
							MenuBuilder.EndSection();

							return MenuBuilder.MakeWidget();
						})
						.ButtonContent()
						[
							SNew(SHorizontalBox)
			
							+SHorizontalBox::Slot()
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
									{
										return GetShowConstraintsName();
									})
								.ToolTipText_Lambda([this]()
								{
									return GetShowConstraintsTooltip();
								})
							]
						]
					]
					// "Plus" icon
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(0.f, 2.f, 8.f, 2.f)
					[
						SNew(SButton)
						.ContentPadding(0.0f)
						.ButtonStyle(FAppStyle::Get(), "NoBorder")
						.IsEnabled_Lambda([this]()
						{
							TArray<AActor*> SelectedActors;
							if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
							{
								const ULevel* CurrentLevel = EditMode->GetWorld()->GetCurrentLevel();
								SelectedActors = ObjectPtrDecay(CurrentLevel->Actors).FilterByPredicate([](const AActor* Actor)
								{
									return Actor && Actor->IsSelected();
								});
							}
							return !SelectedActors.IsEmpty();
						})
						.OnClicked(this, &STLLRN_ControlRigEditModeTools::HandleAddConstraintClicked)
						.Cursor(EMouseCursor::Default)
						.ToolTipText(LOCTEXT("AddConstraint", "Add Constraint"))
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
						]
					]
				]
				.BodyContent()
				[
					SAssignNew(ConstraintsEditionWidget, SConstraintsEditionWidget)
				]
				.OnAreaExpansionChanged_Lambda( [this](bool bIsExpanded)
				{
					if (ConstraintsEditionWidget)
					{
						ConstraintsEditionWidget->RefreshConstraintList();
					}
				})
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(RigOptionExpander, SExpandableArea)
				.InitiallyCollapsed(false)
				.Visibility(this, &STLLRN_ControlRigEditModeTools::GetRigOptionExpanderVisibility)
				.AreaTitle(LOCTEXT("RigOption_Header", "Rig Options"))
				.AreaTitleFont(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
				.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f))
				.BodyContent()
				[
					RigOptionsDetailsView.ToSharedRef()
				]
			]
		]
	];
#if USE_LOCAL_DETAILS
	HierarchyTreeView->RefreshTreeView(true);
#endif
}

void STLLRN_ControlRigEditModeTools::SetSettingsDetailsObject(const TWeakObjectPtr<>& InObject)
{
	if (SettingsDetailsView)
	{
		TArray<TWeakObjectPtr<>> Objects;
		if (InObject.IsValid())
		{
			Objects.Add(InObject);
		}
		SettingsDetailsView->SetObjects(Objects);
	}
}

void STLLRN_ControlRigEditModeTools::OnSelectShowConstraints(int32 Index)
{
	if (ConstraintsEditionWidget.IsValid())
	{
		SConstraintsEditionWidget::EShowConstraints ShowConstraint = (SConstraintsEditionWidget::EShowConstraints)(Index);
		ConstraintsEditionWidget->SetShowConstraints(ShowConstraint);
	}
}

FText STLLRN_ControlRigEditModeTools::GetShowConstraintsName() const
{
	FText Text = FText::GetEmpty();
	if (ConstraintsEditionWidget.IsValid())
	{
		Text = ConstraintsEditionWidget->GetShowConstraintsText(FBaseConstraintListWidget::ShowConstraints);
	}
	return Text;
}

FText STLLRN_ControlRigEditModeTools::GetShowConstraintsTooltip() const
{
	FText Text = FText::GetEmpty();
	if (ConstraintsEditionWidget.IsValid())
	{
		Text = ConstraintsEditionWidget->GetShowConstraintsTooltip(FBaseConstraintListWidget::ShowConstraints);
	}
	return Text;
}

#if USE_LOCAL_DETAILS

void STLLRN_ControlRigEditModeTools::SetEulerTransformDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	if (ControlEulerTransformDetailsView)
	{
		ControlEulerTransformDetailsView->SetObjects(InObjects);
	}
};

void STLLRN_ControlRigEditModeTools::SetTransformDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	if (ControlTransformDetailsView)
	{
		ControlTransformDetailsView->SetObjects(InObjects);
	}
}

void STLLRN_ControlRigEditModeTools::SetTransformNoScaleDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	if (ControlTransformNoScaleDetailsView)
	{
		ControlTransformNoScaleDetailsView->SetObjects(InObjects);
	}
}

void STLLRN_ControlRigEditModeTools::SetFloatDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	if (ControlFloatDetailsView)
	{
		ControlFloatDetailsView->SetObjects(InObjects);
	}
}

void STLLRN_ControlRigEditModeTools::SetBoolDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	if (ControlBoolDetailsView)
	{
		ControlBoolDetailsView->SetObjects(InObjects);
	}
}

void STLLRN_ControlRigEditModeTools::SetIntegerDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	if (ControlIntegerDetailsView)
	{
		ControlIntegerDetailsView->SetObjects(InObjects);
	}
}
void STLLRN_ControlRigEditModeTools::SetEnumDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	if (ControlVectorDetailsView)
	{
		ControlVectorDetailsView->SetObjects(InObjects);
	}
}

void STLLRN_ControlRigEditModeTools::SetVectorDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	if (ControlEnumDetailsView)
	{
		ControlEnumDetailsView->SetObjects(InObjects);
	}
}

void STLLRN_ControlRigEditModeTools::SetVector2DDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	if (ControlVector2DDetailsView)
	{
		ControlVector2DDetailsView->SetObjects(InObjects);
	}
}
#endif
void STLLRN_ControlRigEditModeTools::SetSequencer(TWeakPtr<ISequencer> InSequencer)
{
	WeakSequencer = InSequencer.Pin();
	if (ConstraintsEditionWidget)
	{
		ConstraintsEditionWidget->SequencerChanged(InSequencer);
	}
}

bool STLLRN_ControlRigEditModeTools::IsPropertyKeyable(const UClass* InObjectClass, const IPropertyHandle& InPropertyHandle) const
{
	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyTransform::StaticClass()))
	{
		return true;
	}
	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyTransform::StaticClass()) && InPropertyHandle.GetProperty()
		&& (InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Location) ||
		InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Rotation) ||
		InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Scale)))
	{
		return true;
	}

	FCanKeyPropertyParams CanKeyPropertyParams(InObjectClass, InPropertyHandle);
	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	if (Sequencer.IsValid() && Sequencer->CanKeyProperty(CanKeyPropertyParams))
	{
		return true;
	}

	return false;
}

bool STLLRN_ControlRigEditModeTools::IsPropertyKeyingEnabled() const
{
	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	if (Sequencer.IsValid() && Sequencer->GetFocusedMovieSceneSequence())
	{
		return true;
	}

	return false;
}

bool STLLRN_ControlRigEditModeTools::IsPropertyAnimated(const IPropertyHandle& PropertyHandle, UObject *ParentObject) const
{
	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	if (Sequencer.IsValid() && Sequencer->GetFocusedMovieSceneSequence())
	{
		constexpr bool bCreateHandleIfMissing = false;
		FGuid ObjectHandle = Sequencer->GetHandleToObject(ParentObject, bCreateHandleIfMissing);
		if (ObjectHandle.IsValid()) 
		{
			UMovieScene* MovieScene = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
			FProperty* Property = PropertyHandle.GetProperty();
			TSharedRef<FPropertyPath> PropertyPath = FPropertyPath::CreateEmpty();
			PropertyPath->AddProperty(FPropertyInfo(Property));
			FName PropertyName(*PropertyPath->ToString(TEXT(".")));
			TSubclassOf<UMovieSceneTrack> TrackClass; //use empty @todo find way to get the UMovieSceneTrack from the Property type.
			return MovieScene->FindTrack(TrackClass, ObjectHandle, PropertyName) != nullptr;
		}
	}
	return false;
}

void STLLRN_ControlRigEditModeTools::OnKeyPropertyClicked(const IPropertyHandle& KeyedPropertyHandle)
{
	if (WeakSequencer.IsValid() && !WeakSequencer.Pin()->IsAllowedToChange())
	{
		return;
	}

	TArray<UObject*> Objects;
	KeyedPropertyHandle.GetOuterObjects(Objects);
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();

	for (UObject *Object : Objects)
	{
		UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = Cast< UTLLRN_ControlTLLRN_RigControlsProxy>(Object);
		if (Proxy)
	{
			Proxy->SetKey(SequencerPtr,KeyedPropertyHandle);
		}
	}
}

bool STLLRN_ControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization(const FPropertyAndParent& InPropertyAndParent) const
{
	auto ShouldPropertyBeVisible = [](const FProperty& InProperty)
	{
		bool bShow = InProperty.HasAnyPropertyFlags(CPF_Interp) || InProperty.HasMetaData(FRigVMStruct::InputMetaName) || InProperty.HasMetaData(FRigVMStruct::OutputMetaName);

	/*	// Show 'PickerIKTogglePos' properties
		bShow |= (InProperty.GetFName() == GET_MEMBER_NAME_CHECKED(FLimbControl, PickerIKTogglePos));
		bShow |= (InProperty.GetFName() == GET_MEMBER_NAME_CHECKED(FSpineControl, PickerIKTogglePos));
*/

		// Always show settings properties
		const UClass* OwnerClass = InProperty.GetOwner<UClass>();
		bShow |= OwnerClass == UTLLRN_ControlRigEditModeSettings::StaticClass();
		return bShow;
	};

	bool bContainsVisibleProperty = false;
	if (InPropertyAndParent.Property.IsA<FStructProperty>())
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(&InPropertyAndParent.Property);
		for (TFieldIterator<FProperty> PropertyIt(StructProperty->Struct); PropertyIt; ++PropertyIt)
		{
			if (ShouldPropertyBeVisible(**PropertyIt))
			{
				return true;
			}
		}
	}

	return ShouldPropertyBeVisible(InPropertyAndParent.Property) || 
		(InPropertyAndParent.ParentProperties.Num() > 0 && ShouldPropertyBeVisible(*InPropertyAndParent.ParentProperties[0]));
}

bool STLLRN_ControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization(const FPropertyAndParent& InPropertyAndParent) const
{
	auto ShouldPropertyBeEnabled = [](const FProperty& InProperty)
	{
		bool bShow = InProperty.HasAnyPropertyFlags(CPF_Interp) || InProperty.HasMetaData(FRigVMStruct::InputMetaName);

		// Always show settings properties
		const UClass* OwnerClass = InProperty.GetOwner<UClass>();
		bShow |= OwnerClass == UTLLRN_ControlRigEditModeSettings::StaticClass();
		return bShow;
	};

	bool bContainsVisibleProperty = false;
	if (InPropertyAndParent.Property.IsA<FStructProperty>())
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(&InPropertyAndParent.Property);
		for (TFieldIterator<FProperty> PropertyIt(StructProperty->Struct); PropertyIt; ++PropertyIt)
		{
			if (ShouldPropertyBeEnabled(**PropertyIt))
			{
				return false;
			}
		}
	}

	return !(ShouldPropertyBeEnabled(InPropertyAndParent.Property) || 
		(InPropertyAndParent.ParentProperties.Num() > 0 && ShouldPropertyBeEnabled(*InPropertyAndParent.ParentProperties[0])));
}

#if USE_LOCAL_DETAILS
static bool bPickerChangingSelection = false;

void STLLRN_ControlRigEditModeTools::OnManipulatorsPicked(const TArray<FName>& Manipulators)
{
	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
	if (TLLRN_ControlRigEditMode)
	{
		if (!bPickerChangingSelection)
		{
			TGuardValue<bool> SelectGuard(bPickerChangingSelection, true);
			TLLRN_ControlRigEditMode->ClearTLLRN_RigElementSelection((uint32)ETLLRN_RigElementType::Control);
			TLLRN_ControlRigEditMode->SetTLLRN_RigElementSelection(ETLLRN_RigElementType::Control, Manipulators, true);
		}
	}
}

void STLLRN_ControlRigEditModeTools::HandleModifiedEvent(ERigVMGraphNotifType InNotifType, URigVMGraph* InGraph, UObject* InSubject)
{
	if (bPickerChangingSelection)
	{
		return;
	}

	TGuardValue<bool> SelectGuard(bPickerChangingSelection, true);
	switch (InNotifType)
	{
		case ERigVMGraphNotifType::NodeSelected:
		case ERigVMGraphNotifType::NodeDeselected:
		{
			URigVMNode* Node = Cast<URigVMNode>(InSubject);
			if (Node)
			{
				// those are not yet implemented yet
				// ControlPicker->SelectManipulator(Node->Name, InType == ETLLRN_ControlRigModelNotifType::NodeSelected);
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

void STLLRN_ControlRigEditModeTools::HandleSelectionChanged(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo)
{
	if(bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}
	
	const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if (Hierarchy)
	{
		UTLLRN_RigHierarchyController* Controller = ((UTLLRN_RigHierarchy*)Hierarchy)->GetController(true);
		check(Controller);
		
		TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);

		const TArray<FTLLRN_RigElementKey> NewSelection = HierarchyTreeView->GetSelectedKeys();
		if(!Controller->SetSelection(NewSelection))
		{
			return;
		}
	}
}
#endif

void STLLRN_ControlRigEditModeTools::OnTLLRN_RigElementSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, bool bSelected)
{
#if USE_LOCAL_DETAILS
	const FTLLRN_RigElementKey Key = ControlElement->GetKey();
	for (int32 RootIndex = 0; RootIndex < HierarchyTreeView->GetRootElements().Num(); ++RootIndex)
	{
		TSharedPtr<FRigTreeElement> Found = HierarchyTreeView->FindElement(Key, HierarchyTreeView->GetRootElements()[RootIndex]);
		if (Found.IsValid())
		{
			HierarchyTreeView->SetItemSelection(Found, bSelected, ESelectInfo::OnNavigation);
			
			TArray<TSharedPtr<FRigTreeElement>> SelectedItems = HierarchyTreeView->GetSelectedItems();
			for (TSharedPtr<FRigTreeElement> SelectedItem : SelectedItems)
			{
				HierarchyTreeView->SetExpansionRecursive(SelectedItem, false, true);
			}

			if (SelectedItems.Num() > 0)
			{
				HierarchyTreeView->RequestScrollIntoView(SelectedItems.Last());
			}
		}
	}
#endif

	if (Subject && Subject->GetHierarchy())
	{
		// get the selected controls
		TArray<FTLLRN_RigElementKey> SelectedControls = Subject->GetHierarchy()->GetSelectedKeys(ETLLRN_RigElementType::Control);
		if (SpacePickerWidget)
		{
			SpacePickerWidget->SetControls(Subject->GetHierarchy(), SelectedControls);
		}
		if (ConstraintsEditionWidget)
		{
			ConstraintsEditionWidget->InvalidateConstraintList();
		}
	}
}

const FTLLRN_RigControlElementCustomization* STLLRN_ControlRigEditModeTools::HandleGetControlElementCustomization(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey)
{
	UTLLRN_ControlRig* Rig = TLLRN_ControlRigs.Num() > 0 ? TLLRN_ControlRigs[0].Get() : nullptr;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig.IsValid() && TLLRN_ControlRig->GetHierarchy() == InHierarchy)
		{
			return TLLRN_ControlRig->GetControlCustomization(InControlKey);
		}
	}
	return nullptr;
}

void STLLRN_ControlRigEditModeTools::HandleActiveSpaceChanged(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey,
	const FTLLRN_RigElementKey& InSpaceKey)
{

	if (WeakSequencer.IsValid())
	{
		for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : TLLRN_ControlRigs)
		{
			if (TLLRN_ControlRig.IsValid() && TLLRN_ControlRig->GetHierarchy() == InHierarchy)
			{
				FString FailureReason;
				UTLLRN_RigHierarchy::TElementDependencyMap DependencyMap = InHierarchy->GetDependenciesForVM(TLLRN_ControlRig->GetVM());
				if(!InHierarchy->CanSwitchToParent(InControlKey, InSpaceKey, DependencyMap, &FailureReason))
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
					ISequencer* Sequencer = WeakSequencer.Pin().Get();
					if (Sequencer)
					{
						FScopedTransaction Transaction(LOCTEXT("KeyTLLRN_ControlTLLRN_RigSpace", "Key Control Rig Space"));
						TLLRN_ControlRig->Modify();

						FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig.Get(), InControlKey.Name, Sequencer, true /*bCreateIfNeeded*/);
						if (SpaceChannelAndSection.SpaceChannel)
						{
							const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
							const FFrameTime FrameTime = Sequencer->GetLocalTime().ConvertTo(TickResolution);
							FFrameNumber CurrentTime = FrameTime.GetFrame();
							FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerKeyTLLRN_ControlTLLRN_RigSpaceChannel(TLLRN_ControlRig.Get(), Sequencer, SpaceChannelAndSection.SpaceChannel, SpaceChannelAndSection.SectionToKey, CurrentTime, InHierarchy, InControlKey, InSpaceKey);
						}
					}
				}
			}
		}
	}
}

void STLLRN_ControlRigEditModeTools::HandleSpaceListChanged(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey,
	const TArray<FTLLRN_RigElementKey>& InSpaceList)
{
	FScopedTransaction Transaction(LOCTEXT("ChangeTLLRN_ControlTLLRN_RigSpace", "Change Control Rig Space"));

	for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig.IsValid() && TLLRN_ControlRig->GetHierarchy() == InHierarchy)
		{
			TLLRN_ControlRig->Modify();

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
					const TGuardValue<bool> SuspendGuard(EditMode->bSuspendHierarchyNotifs, true);
					InHierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
				}
				else
				{
					InHierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
				}

				SpacePickerWidget->RefreshContents();
			}
		}
	}
}

FReply STLLRN_ControlRigEditModeTools::HandleAddSpaceClicked()
{
	return SpacePickerWidget->HandleAddElementClicked();
}

EVisibility STLLRN_ControlRigEditModeTools::GetAddSpaceButtonVisibility() const
{
	return IsSpaceSwitchingRestricted() ? EVisibility::Hidden : EVisibility::Visible;
}

bool STLLRN_ControlRigEditModeTools::IsSpaceSwitchingRestricted() const
{
	return SpacePickerWidget->IsRestricted();
}

bool STLLRN_ControlRigEditModeTools::ReadyForBakeOrCompensation() const
{
	if (SpacePickerWidget->GetHierarchy() == nullptr)
	{
		return false;
	}
	if (SpacePickerWidget->GetControls().Num() == 0)
	{
		return false;
	}

	bool bNoValidTLLRN_ControlRig = true;
	for (const TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig.IsValid() && SpacePickerWidget->GetHierarchy() == TLLRN_ControlRig->GetHierarchy())
		{
			bNoValidTLLRN_ControlRig = false;
			break;
		}
	}

	if (bNoValidTLLRN_ControlRig)
	{
		return false;
	}
	ISequencer* Sequencer = WeakSequencer.Pin().Get();
	if (Sequencer == nullptr || Sequencer->GetFocusedMovieSceneSequence() == nullptr || Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene() == nullptr)
	{
		return false;
	}
	return true;
}

FReply STLLRN_ControlRigEditModeTools::OnCompensateKeyClicked()
{
	if (ReadyForBakeOrCompensation() == false)
	{
		return FReply::Unhandled();
	}
	ISequencer* Sequencer = WeakSequencer.Pin().Get();
	const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
	const FFrameTime FrameTime = Sequencer->GetLocalTime().ConvertTo(TickResolution);
	const TOptional<FFrameNumber> OptionalKeyTime = FrameTime.GetFrame();
	const bool bSetPreviousKey = true;
	Compensate(OptionalKeyTime, bSetPreviousKey);
	return FReply::Handled();
}

FReply STLLRN_ControlRigEditModeTools::OnCompensateAllClicked()
{
	if (ReadyForBakeOrCompensation() == false)
	{
		return FReply::Unhandled();
	}
	const TOptional<FFrameNumber> OptionalKeyTime;
	ISequencer* Sequencer = WeakSequencer.Pin().Get();
	const bool bSetPreviousKey = true;
	Compensate(OptionalKeyTime, bSetPreviousKey);
	return FReply::Handled();
}

void STLLRN_ControlRigEditModeTools::Compensate(TOptional<FFrameNumber> OptionalKeyTime, bool bSetPreviousTick)
{
	if (ReadyForBakeOrCompensation() == false)
	{
		return;
	}
	ISequencer* Sequencer = WeakSequencer.Pin().Get();
	for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig.IsValid() && SpacePickerWidget->GetHierarchy() == TLLRN_ControlRig->GetHierarchy())
		{
			// compensate spaces
			if(UMovieSceneTLLRN_ControlRigParameterSection* CRSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::GetTLLRN_ControlRigSection(Sequencer, TLLRN_ControlRig.Get()))
			{	
				// compensate spaces
				FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::CompensateIfNeeded(
					TLLRN_ControlRig.Get(), Sequencer, CRSection,
				OptionalKeyTime, bSetPreviousTick);
			}
		}
	}
}

FReply STLLRN_ControlRigEditModeTools::OnBakeControlsToNewSpaceButtonClicked()
{
	if (ReadyForBakeOrCompensation() == false)
	{
		return FReply::Unhandled();
	}
	ISequencer* Sequencer = WeakSequencer.Pin().Get();

	for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig.IsValid() && SpacePickerWidget->GetHierarchy() == TLLRN_ControlRig->GetHierarchy())
		{

			TArray<FTLLRN_RigElementKey> ControlKeys = SpacePickerWidget->GetControls();
			FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig.Get(), ControlKeys[0].Name, Sequencer, false /*bCreateIfNeeded*/);
			if (SpaceChannelAndSection.SpaceChannel != nullptr)
			{
				const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
				const FFrameTime FrameTime = Sequencer->GetLocalTime().ConvertTo(TickResolution);
				FFrameNumber CurrentTime = FrameTime.GetFrame();
				FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
				using namespace UE::MovieScene;
				//set up settings if not setup
				if (BakeSpaceSettings.TargetSpace == FTLLRN_RigElementKey())
				{
					BakeSpaceSettings.TargetSpace = UTLLRN_RigHierarchy::GetDefaultParentKey();
					TRange<FFrameNumber> Range = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange();
					BakeSpaceSettings.Settings.StartFrame = Range.GetLowerBoundValue();
					BakeSpaceSettings.Settings.EndFrame = Range.GetUpperBoundValue();
				}

				TSharedRef<STLLRN_RigSpacePickerBakeWidget> BakeWidget =
					SNew(STLLRN_RigSpacePickerBakeWidget)
					.Settings(BakeSpaceSettings)
					.Hierarchy(SpacePickerWidget->GetHierarchy())
					.Controls(ControlKeys) // use the cached controls here since the selection is not recovered until next tick.
					.Sequencer(Sequencer)
					.GetControlCustomization(this, &STLLRN_ControlRigEditModeTools::HandleGetControlElementCustomization)
					.OnBake_Lambda([Sequencer, TLLRN_ControlRig, TickResolution](UTLLRN_RigHierarchy* InHierarchy, TArray<FTLLRN_RigElementKey> InControls, const FTLLRN_TLLRN_RigSpacePickerBakeSettings& InSettings)
						{		
							FScopedTransaction Transaction(LOCTEXT("BakeControlToSpace", "Bake Control In Space"));
							for (const FTLLRN_RigElementKey& ControlKey : InControls)
							{
								//when baking we will now create a channel if one doesn't exist, was causing confusion
								FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(TLLRN_ControlRig.Get(), ControlKey.Name, Sequencer, true /*bCreateIfNeeded*/);
								if (SpaceChannelAndSection.SpaceChannel)
								{
									FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerBakeControlInSpace(TLLRN_ControlRig.Get(), Sequencer, SpaceChannelAndSection.SpaceChannel, SpaceChannelAndSection.SectionToKey,
										InHierarchy, ControlKey, InSettings);
								}
								STLLRN_ControlRigEditModeTools::BakeSpaceSettings = InSettings;
							}
							return FReply::Handled();
						});

				return BakeWidget->OpenDialog(true);
			}
			break; //mz todo need baketo handle more than one
		}

	}
	return FReply::Unhandled();
}

FReply STLLRN_ControlRigEditModeTools::HandleAddConstraintClicked()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	auto AddConstraintWidget = [&](ETransformConstraintType InConstraintType)
	{
		const TSharedRef<SConstraintMenuEntry> Entry =
			SNew(SConstraintMenuEntry, InConstraintType)
		.OnConstraintCreated_Lambda([this]()
		{
			// magic number to auto expand the widget when creating a new constraint. We keep that number below a reasonable
			// threshold to avoid automatically creating a large number of items (this can be style done by the user) 
			static constexpr int32 NumAutoExpand = 20;
			const int32 NumItems = ConstraintsEditionWidget ? ConstraintsEditionWidget->RefreshConstraintList() : 0;	
			if (ConstraintPickerExpander && NumItems < NumAutoExpand)
			{
				ConstraintPickerExpander->SetExpanded(true);
			}
		});
		MenuBuilder.AddWidget(Entry, FText::GetEmpty(), true);
	};
	
	MenuBuilder.BeginSection("CreateConstraint", LOCTEXT("CreateConstraintHeader", "Create New..."));
	{
		AddConstraintWidget(ETransformConstraintType::Translation);
		AddConstraintWidget(ETransformConstraintType::Rotation);
		AddConstraintWidget(ETransformConstraintType::Scale);
		AddConstraintWidget(ETransformConstraintType::Parent);
		AddConstraintWidget(ETransformConstraintType::LookAt);
	}
	MenuBuilder.EndSection();
	
	FSlateApplication::Get().PushMenu(
		AsShared(),
		FWidgetPath(),
		MenuBuilder.MakeWidget(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
	
	return FReply::Handled();
}

EVisibility STLLRN_ControlRigEditModeTools::GetRigOptionExpanderVisibility() const
{
	for (const TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig.IsValid())
		{
			if (Cast<UFKTLLRN_ControlRig>(TLLRN_ControlRig))
			{
				return EVisibility::Visible;
			}
		}
	}
	return EVisibility::Hidden;
}

void STLLRN_ControlRigEditModeTools::OnRigOptionFinishedChange(const FPropertyChangedEvent& PropertyChangedEvent)
{
	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigsCopy = TLLRN_ControlRigs;
	SetTLLRN_ControlRigs(TLLRN_ControlRigsCopy);

	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		EditMode->SetObjects_Internal();
	}
}

void STLLRN_ControlRigEditModeTools::CustomizeToolBarPalette(FToolBarBuilder& ToolBarBuilder)
{
	//TOGGLE SELECTED RIG CONTROLS
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([this] {
			FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
			if (TLLRN_ControlRigEditMode)
			{
				TLLRN_ControlRigEditMode->SetOnlySelectTLLRN_RigControls(!TLLRN_ControlRigEditMode->GetOnlySelectTLLRN_RigControls());
			}
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this] {
			FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
			if (TLLRN_ControlRigEditMode)
			{
				return TLLRN_ControlRigEditMode->GetOnlySelectTLLRN_RigControls();
			}
			return false;
			})

		),
		NAME_None,
		LOCTEXT("OnlySelectControls", "Select"),
		LOCTEXT("OnlySelectControlsTooltip", "Only Select Control Rig Controls"),
		FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.OnlySelectControls")),
		EUserInterfaceActionType::ToggleButton
		);

	ToolBarBuilder.AddSeparator();

	//POSES
	ToolBarBuilder.AddToolBarButton(
		FExecuteAction::CreateRaw(OwningToolkit.Pin().Get(), &FTLLRN_ControlRigEditModeToolkit::TryInvokeToolkitUI, FTLLRN_ControlRigEditModeToolkit::PoseTabName),
		NAME_None,
		LOCTEXT("Poses", "Poses"),
		LOCTEXT("PosesTooltip", "Show Poses"),
		FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.PoseTool")),
		EUserInterfaceActionType::Button
	);
	ToolBarBuilder.AddSeparator();

	// Tweens
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
		FExecuteAction::CreateRaw(OwningToolkit.Pin().Get(), &FTLLRN_ControlRigEditModeToolkit::TryInvokeToolkitUI, FTLLRN_ControlRigEditModeToolkit::TweenOverlayName),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(OwningToolkit.Pin().Get(), &FTLLRN_ControlRigEditModeToolkit::IsToolkitUIActive, FTLLRN_ControlRigEditModeToolkit::TweenOverlayName)
		),		
		NAME_None,
		LOCTEXT("Tweens", "Tweens"),
		LOCTEXT("TweensTooltip", "Create Tweens"),
		FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.TweenTool")),
		EUserInterfaceActionType::ToggleButton
	);

	// Snap
	ToolBarBuilder.AddToolBarButton(
		FExecuteAction::CreateRaw(OwningToolkit.Pin().Get(), &FTLLRN_ControlRigEditModeToolkit::TryInvokeToolkitUI, FTLLRN_ControlRigEditModeToolkit::SnapperTabName),
		NAME_None,
		LOCTEXT("Snapper", "Snapper"),
		LOCTEXT("SnapperTooltip", "Snap child objects to a parent object over a set of frames"),
		FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.SnapperTool")),
		EUserInterfaceActionType::Button
	);

	// Motion Trail
	ToolBarBuilder.AddToolBarButton(
		FExecuteAction::CreateRaw(OwningToolkit.Pin().Get(), &FTLLRN_ControlRigEditModeToolkit::TryInvokeToolkitUI, FTLLRN_ControlRigEditModeToolkit::MotionTrailTabName),
		NAME_None,
		LOCTEXT("MotionTrails", "Trails"),
		LOCTEXT("MotionTrailsTooltip", "Display motion trails for animated objects"),
		FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.EditableMotionTrails")),
		EUserInterfaceActionType::Button
	);

	// Anim Layer
	ToolBarBuilder.AddToolBarButton(
		FExecuteAction::CreateRaw(OwningToolkit.Pin().Get(), &FTLLRN_ControlRigEditModeToolkit::TryInvokeToolkitUI, FTLLRN_ControlRigEditModeToolkit::TLLRN_AnimLayerTabName),
		NAME_None,
		LOCTEXT("Layers", "Layers"),
		LOCTEXT("TLLRN_TLLRN_AnimLayersTooltip", "Display animation layers"),
		FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.TLLRN_TLLRN_AnimLayers")),
		EUserInterfaceActionType::Button
	);

	//Pivot
	ToolBarBuilder.AddToolBarButton(
		FUIAction(
		FExecuteAction::CreateSP(this, &STLLRN_ControlRigEditModeTools::ToggleEditPivotMode),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this] {
				if (TSharedPtr<IToolkit> Toolkit = OwningToolkit.Pin())
				{
					if (Toolkit.IsValid())
					{
						const FString ActiveToolName = Toolkit->GetToolkitHost()->GetEditorModeManager().GetInteractiveToolsContext()->ToolManager->GetActiveToolName(EToolSide::Left);
						if (ActiveToolName == TEXT("SequencerPivotTool"))
						{
							return true;
						}
					}
				}
				return false;

			})
		),
		NAME_None,
		LOCTEXT("TempPivot", "Pivot"),
		LOCTEXT("TempPivotTooltip", "Create a temporary pivot to rotate the selected Control"),
		FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.TemporaryPivot")),
		EUserInterfaceActionType::ToggleButton
		);
}

void STLLRN_ControlRigEditModeTools::ToggleEditPivotMode()
{
	FEditorModeID ModeID = TEXT("SequencerToolsEditMode");
	if (const TSharedPtr<IToolkit> Toolkit = OwningToolkit.Pin())
	{
		if (Toolkit.IsValid())
		{
			const FEditorModeTools& Tools =Toolkit->GetToolkitHost()->GetEditorModeManager();
			const FString ActiveToolName = Tools.GetInteractiveToolsContext()->ToolManager->GetActiveToolName(EToolSide::Left);
			if (ActiveToolName == TEXT("SequencerPivotTool"))
			{
				Tools.GetInteractiveToolsContext()->ToolManager->DeactivateTool(EToolSide::Left, EToolShutdownType::Completed);
			}
			else
			{
				Tools.GetInteractiveToolsContext()->ToolManager->SelectActiveToolType(EToolSide::Left, TEXT("SequencerPivotTool"));
				Tools.GetInteractiveToolsContext()->ToolManager->ActivateTool(EToolSide::Left);
			}
		}
	}
}


/* MZ TODO
void STLLRN_ControlRigEditModeTools::MakeSelectionSetDialog()
{

	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
	if (TLLRN_ControlRigEditMode)
	{
		TSharedPtr<SWindow> ExistingWindow = SelectionSetWindow.Pin();
		if (ExistingWindow.IsValid())
		{
			ExistingWindow->BringToFront();
		}
		else
		{
			ExistingWindow = SNew(SWindow)
				.Title(LOCTEXT("SelectionSets", "Selection Set"))
				.HasCloseButton(true)
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.ClientSize(FVector2D(165, 200));
			TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
			if (RootWindow.IsValid())
			{
				FSlateApplication::Get().AddWindowAsNativeChild(ExistingWindow.ToSharedRef(), RootWindow.ToSharedRef());
			}
			else
			{
				FSlateApplication::Get().AddWindow(ExistingWindow.ToSharedRef());
			}

		}

		ExistingWindow->SetContent(
			SNew(STLLRN_ControlRigBaseListWidget)
		);
		SelectionSetWindow = ExistingWindow;
	}
}
*/
FText STLLRN_ControlRigEditModeTools::GetActiveToolName() const
{
	return FText::FromString(TEXT("TLLRN Control Rig Editing"));
}

FText STLLRN_ControlRigEditModeTools::GetActiveToolMessage() const
{
	return  FText();
}

#undef LOCTEXT_NAMESPACE
