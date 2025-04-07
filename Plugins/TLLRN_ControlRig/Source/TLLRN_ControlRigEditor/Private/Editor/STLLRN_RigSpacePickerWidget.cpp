// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/STLLRN_RigSpacePickerWidget.h"
#include "DetailLayoutBuilder.h"
#include "Editor.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Editor/STLLRN_RigHierarchyTreeView.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "PropertyCustomizationHelpers.h"
#include "ISequencer.h"
#include "Widgets/Input/NumericTypeInterface.h"
#include "FrameNumberDetailsCustomization.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Modules/ModuleManager.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "RigVMBlueprintGeneratedClass.h"

#define LOCTEXT_NAMESPACE "STLLRN_RigSpacePickerWidget"

//////////////////////////////////////////////////////////////
/// STLLRN_RigSpacePickerWidget
///////////////////////////////////////////////////////////

FTLLRN_RigElementKey STLLRN_RigSpacePickerWidget::InValidKey;

void STLLRN_RigSpacePickerWidget::Construct(const FArguments& InArgs)
{
	GEditor->RegisterForUndo(this);

	bShowDefaultSpaces = InArgs._ShowDefaultSpaces;
	bShowFavoriteSpaces = InArgs._ShowFavoriteSpaces;
	bShowAdditionalSpaces = InArgs._ShowAdditionalSpaces;
	bAllowReorder = InArgs._AllowReorder;
	bAllowDelete = InArgs._AllowDelete;
	bAllowAdd = InArgs._AllowAdd;
	bShowBakeAndCompensateButton = InArgs._ShowBakeAndCompensateButton;
	GetActiveSpaceDelegate = InArgs._GetActiveSpace;
	GetControlCustomizationDelegate = InArgs._GetControlCustomization;
	GetAdditionalSpacesDelegate = InArgs._GetAdditionalSpaces;
	bRepopulateRequired = false;
	bLaunchingContextMenu = false;

	if(!GetActiveSpaceDelegate.IsBound())
	{
		GetActiveSpaceDelegate = FTLLRN_RigSpacePickerGetActiveSpace::CreateRaw(this, &STLLRN_RigSpacePickerWidget::GetActiveSpace_Private);
	}
	if(!GetAdditionalSpacesDelegate.IsBound())
	{
		GetAdditionalSpacesDelegate = FTLLRN_RigSpacePickerGetAdditionalSpaces::CreateRaw(this, &STLLRN_RigSpacePickerWidget::GetCurrentParents_Private);
	}

	if(InArgs._OnActiveSpaceChanged.IsBound())
	{
		OnActiveSpaceChanged().Add(InArgs._OnActiveSpaceChanged);
	}
	if(InArgs._OnSpaceListChanged.IsBound())
	{
		OnSpaceListChanged().Add(InArgs._OnSpaceListChanged);
	}

	Hierarchy = nullptr;
	ControlKeys.Reset();

	ChildSlot
	[
		SNew(SBorder)
		.Visibility(EVisibility::Visible)
		.BorderImage(InArgs._BackgroundBrush)
		[
			SAssignNew(TopLevelListBox, SVerticalBox)
		]
	];

	if(!InArgs._Title.IsEmpty())
	{
		TopLevelListBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Left)
		.Padding(4.0, 0.0, 4.0, 12.0)
		[
			SNew( STextBlock )
			.Text( InArgs._Title )
			.Font( IDetailLayoutBuilder::GetDetailFontBold() )
		];
	}

	if(InArgs._ShowDefaultSpaces)
	{
		AddSpacePickerRow(
			TopLevelListBox,
			ESpacePickerType_Parent,
			UTLLRN_RigHierarchy::GetDefaultParentKey(),
			FAppStyle::Get().GetBrush("Icons.Transform"),
			FSlateColor::UseForeground(),
			LOCTEXT("Parent", "Parent"),
			FOnClicked::CreateSP(this, &STLLRN_RigSpacePickerWidget::HandleParentSpaceClicked)
		);
		
		AddSpacePickerRow(
			TopLevelListBox,
			ESpacePickerType_World,
			UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey(),
			FAppStyle::GetBrush("EditorViewport.RelativeCoordinateSystem_World"),
			FSlateColor::UseForeground(),
			LOCTEXT("World", "World"),
			FOnClicked::CreateSP(this, &STLLRN_RigSpacePickerWidget::HandleWorldSpaceClicked)
		);
	}

	TopLevelListBox->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Fill)
	.Padding(0.0)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		.Padding(0)
		[
			SAssignNew(ItemSpacesListBox, SVerticalBox)
		]
	];

	if(bAllowAdd || bShowBakeAndCompensateButton)
	{
		TopLevelListBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Fill)
		.Padding(11.f, 8.f, 4.f, 4.f)
		[
			SAssignNew(BottomButtonsListBox, SHorizontalBox)
		];

		if(bAllowAdd)
		{
			BottomButtonsListBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(0.f)
			[
				SNew(SButton)
				.ContentPadding(0.0f)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.OnClicked(this, &STLLRN_RigSpacePickerWidget::HandleAddElementClicked)
				.Cursor(EMouseCursor::Default)
				.ToolTipText(LOCTEXT("AddSpace", "Add Space"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
				]
				.Visibility_Lambda([this]()
				{
					return IsRestricted() ? EVisibility::Collapsed : EVisibility::Visible;
				})
			];
		}

		BottomButtonsListBox->AddSlot()
		.FillWidth(1.f)
		.HAlign(HAlign_Fill)
		[
			SNew(SSpacer)
		];

		if(bShowBakeAndCompensateButton)
		{
			BottomButtonsListBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(0.f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
				.Text(LOCTEXT("CompensateKeyButton", "Comp Key"))
				.OnClicked(InArgs._OnCompensateKeyButtonClicked)
				.IsEnabled_Lambda([this]()
				{
					return (ControlKeys.Num() > 0);
				})
				.ToolTipText(LOCTEXT("CompensateKeyTooltip", "Compensate key at the current time."))
			];
			BottomButtonsListBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(0.f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
				.Text(LOCTEXT("CompensateAllButton", "Comp All"))
				.OnClicked(InArgs._OnCompensateAllButtonClicked)
				.IsEnabled_Lambda([this]()
				{
					return (ControlKeys.Num() > 0);
				})
				.ToolTipText(LOCTEXT("CompensateAllTooltip", "Compensate all space switch keys."))
			];
			BottomButtonsListBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.Padding(0.f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
				.Text(LOCTEXT("BakeButton", "Bake..."))
				.OnClicked(InArgs._OnBakeButtonClicked)
				.IsEnabled_Lambda([this]()
				{
					return (ControlKeys.Num() > 0);
				})
				.ToolTipText(LOCTEXT("BakeButtonToolTip", "Allows to bake the animation of one or more controls to a single space."))
			];
		}
	}

	SetControls(InArgs._Hierarchy, InArgs._Controls);
	SetCanTick(true);
}

STLLRN_RigSpacePickerWidget::~STLLRN_RigSpacePickerWidget()
{
	GEditor->UnregisterForUndo(this);

	if(HierarchyModifiedHandle.IsValid())
	{
		if(Hierarchy.IsValid())
		{
			Hierarchy->OnModified().Remove(HierarchyModifiedHandle);
			HierarchyModifiedHandle.Reset();
		}
	}
}

void STLLRN_RigSpacePickerWidget::SetControls(
	UTLLRN_RigHierarchy* InHierarchy,
	const TArray<FTLLRN_RigElementKey>& InControls)
{
	if(Hierarchy.IsValid())
	{
		UTLLRN_RigHierarchy* StrongHierarchy = Hierarchy.Get();
		if (StrongHierarchy != InHierarchy)
		{
			if(HierarchyModifiedHandle.IsValid())
			{
				StrongHierarchy->OnModified().Remove(HierarchyModifiedHandle);
				HierarchyModifiedHandle.Reset();
			}
		}
	}
	
	Hierarchy = InHierarchy;
	ControlKeys.SetNum(0);
	for (const FTLLRN_RigElementKey& Key : InControls)
	{
		if (const FTLLRN_RigControlElement* ControlElement = Hierarchy->FindChecked<FTLLRN_RigControlElement>(Key))
		{
			//if it has no shape or not animatable then bail
			if (ControlElement->Settings.SupportsShape() == false || Hierarchy->IsAnimatable(ControlElement) == false)
			{
				continue;
			}
			if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool ||
				ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float ||
				ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat ||
				ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer)
			{
				//if it has a channel and has a parent bail
				if (const FTLLRN_RigControlElement* ParentControlElement = Cast<FTLLRN_RigControlElement>(Hierarchy->GetFirstParent(ControlElement)))
				{
					continue;
				}
			}
		}
		ControlKeys.Add(Key);
	}
	if (Hierarchy.IsValid() && HierarchyModifiedHandle.IsValid() == false)
	{
		HierarchyModifiedHandle = InHierarchy->OnModified().AddSP(this, &STLLRN_RigSpacePickerWidget::OnHierarchyModified);
	}
	UpdateActiveSpaces();
	RepopulateItemSpaces();
}

class STLLRN_RigSpaceDialogWindow : public SWindow
{
}; 

FReply STLLRN_RigSpacePickerWidget::OpenDialog(bool bModal)
{
	check(!DialogWindow.IsValid());
		
	const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();

	TSharedRef<STLLRN_RigSpaceDialogWindow> Window = SNew(STLLRN_RigSpaceDialogWindow)
	.Title( LOCTEXT("STLLRN_RigSpacePickerWidgetPickSpace", "Pick a new space") )
	.CreateTitleBar(false)
	.Type(EWindowType::Menu)
	.IsPopupWindow(true) // the window automatically closes when user clicks outside of it
	.SizingRule( ESizingRule::Autosized )
	.ScreenPosition(CursorPos)
	.FocusWhenFirstShown(true)
	.ActivationPolicy(EWindowActivationPolicy::FirstShown)
	[
		AsShared()
	];
	
	Window->SetWidgetToFocusOnActivate(AsShared());
	if (!Window->GetOnWindowDeactivatedEvent().IsBoundToObject(this))
	{
		Window->GetOnWindowDeactivatedEvent().AddLambda([this]()
		{
			// Do not reset if we lost focus because of opening the context menu
			if (!ContextMenu.IsValid())
			{
				SetControls(nullptr, {});
			}
		});
	}
	
	DialogWindow = Window;

	Window->MoveWindowTo(CursorPos);

	if(bModal)
	{
		GEditor->EditorAddModalWindow(Window);
	}
	else
	{
		FSlateApplication::Get().AddWindow( Window );
	}

	return FReply::Handled();
}

void STLLRN_RigSpacePickerWidget::CloseDialog()
{
	if(bLaunchingContextMenu)
	{
		return;
	}
	
	if(ContextMenu.IsValid())
	{
		return;
	}
	
	if ( DialogWindow.IsValid() )
	{
		DialogWindow.Pin()->GetOnWindowDeactivatedEvent().RemoveAll(this);
		DialogWindow.Pin()->RequestDestroyWindow();
		DialogWindow.Reset();
	}
}

FReply STLLRN_RigSpacePickerWidget::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if(DialogWindow.IsValid())
		{
			CloseDialog();
		}
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

bool STLLRN_RigSpacePickerWidget::SupportsKeyboardFocus() const
{
	return true;
}

void STLLRN_RigSpacePickerWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if(bRepopulateRequired)
	{
		UpdateActiveSpaces();
		RepopulateItemSpaces();
		bRepopulateRequired = false;
	}
	else if(GetAdditionalSpacesDelegate.IsBound())
	{
		if (Hierarchy.IsValid())
		{
			UTLLRN_RigHierarchy* StrongHierarchy = Hierarchy.Get();
			TArray<FTLLRN_RigElementKey> CurrentAdditionalSpaces;
			for(const FTLLRN_RigElementKey& ControlKey: ControlKeys)
			{
				CurrentAdditionalSpaces.Append(GetAdditionalSpacesDelegate.Execute(StrongHierarchy, ControlKey));
			}
		
			if(CurrentAdditionalSpaces != AdditionalSpaces)
			{
				RepopulateItemSpaces();
			}
		}
	}
}

const TArray<FTLLRN_RigElementKey>& STLLRN_RigSpacePickerWidget::GetActiveSpaces() const
{
	return ActiveSpaceKeys;
}

TArray<FTLLRN_RigElementKey> STLLRN_RigSpacePickerWidget::GetDefaultSpaces() const
{
	TArray<FTLLRN_RigElementKey> DefaultSpaces;
	DefaultSpaces.Add(UTLLRN_RigHierarchy::GetDefaultParentKey());
	DefaultSpaces.Add(UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey());
	return DefaultSpaces;
}

TArray<FTLLRN_RigElementKey> STLLRN_RigSpacePickerWidget::GetSpaceList(bool bIncludeDefaultSpaces) const
{
	if(bIncludeDefaultSpaces && bShowDefaultSpaces)
	{
		TArray<FTLLRN_RigElementKey> Spaces;
		Spaces.Append(GetDefaultSpaces());
		Spaces.Append(CurrentSpaceKeys);
		return Spaces;
	}
	return CurrentSpaceKeys;
}

void STLLRN_RigSpacePickerWidget::RefreshContents()
{
	UpdateActiveSpaces();
	RepopulateItemSpaces();
}

void STLLRN_RigSpacePickerWidget::AddSpacePickerRow(
	TSharedPtr<SVerticalBox> InListBox,
	ESpacePickerType InType,
	const FTLLRN_RigElementKey& InKey,
	const FSlateBrush* InBush,
	const FSlateColor& InColor,
	const FText& InTitle,
    FOnClicked OnClickedDelegate)
{
	static const FSlateBrush* RoundedBoxBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush(TEXT("TLLRN_ControlRig.SpacePicker.RoundedRect"));

	TSharedPtr<SHorizontalBox> RowBox, ButtonBox;
	InListBox->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Fill)
	.Padding(4.0, 0.0, 4.0, 0.0)
	[
		SNew( SButton )
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(0.0))
		.OnClicked(OnClickedDelegate)
		[
			SAssignNew(RowBox, SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)
			.Padding(0)
			[
				SNew(SBorder)
				.Padding(FMargin(5.0, 2.0, 5.0, 2.0))
				.BorderImage(RoundedBoxBrush)
				.BorderBackgroundColor(this, &STLLRN_RigSpacePickerWidget::GetButtonColor, InType, InKey)
				.Content()
				[
					SAssignNew(ButtonBox, SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Left)
					.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
					[
						SNew(SImage)
						.Image(InBush)
						.ColorAndOpacity(InColor)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Left)
					.Padding(0)
					[
						SNew( STextBlock )
						.Text( InTitle )
						.Font( IDetailLayoutBuilder::GetDetailFont() )
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(SSpacer)
					]
				]
			]
		]
	];

	if(!IsDefaultSpace(InKey))
	{
		const TAttribute<EVisibility> RestrictedVisibility = TAttribute<EVisibility>::CreateLambda([this]
		{
			return IsRestricted() ? EVisibility::Collapsed : EVisibility::Visible;
		});
		
		if(bAllowDelete || bAllowReorder)
		{
			RowBox->AddSlot()
			.FillWidth(1.f)
			[
				SNew(SSpacer)
				.Visibility(RestrictedVisibility)
			];
		}
		
		if(bAllowReorder)
		{
			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(0)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
				.ContentPadding(0)
				.OnClicked(this, &STLLRN_RigSpacePickerWidget::HandleSpaceMoveUp, InKey)
				.IsEnabled(this, &STLLRN_RigSpacePickerWidget::IsSpaceMoveUpEnabled, InKey)
				.ToolTipText(LOCTEXT("MoveSpaceDown", "Move this space down in the list."))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.ChevronUp"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
				.Visibility(RestrictedVisibility)
			];

			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(0)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
				.ContentPadding(0)
				.OnClicked(this, &STLLRN_RigSpacePickerWidget::HandleSpaceMoveDown, InKey)
				.IsEnabled(this, &STLLRN_RigSpacePickerWidget::IsSpaceMoveDownEnabled, InKey)
				.ToolTipText(LOCTEXT("MoveSpaceUp", "Move this space up in the list."))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.ChevronDown"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
				.Visibility(RestrictedVisibility)
			];
		}

		if(bAllowDelete)
		{
			const TSharedRef<SWidget> ClearButton = PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &STLLRN_RigSpacePickerWidget::HandleSpaceDelete, InKey), LOCTEXT("DeleteSpace", "Remove this space."), true);
			ClearButton->SetVisibility(RestrictedVisibility);

			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(0)
			[
				ClearButton
			];
		}
	}
}

FReply STLLRN_RigSpacePickerWidget::HandleParentSpaceClicked()
{
	return HandleElementSpaceClicked(UTLLRN_RigHierarchy::GetDefaultParentKey());
}

FReply STLLRN_RigSpacePickerWidget::HandleWorldSpaceClicked()
{
	return HandleElementSpaceClicked(UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey());
}

FReply STLLRN_RigSpacePickerWidget::HandleElementSpaceClicked(FTLLRN_RigElementKey InKey)
{
	if (Hierarchy.IsValid())
	{
		UTLLRN_RigHierarchy* StrongHierarchy = Hierarchy.Get();
		
		//need to make copy since array may get shrunk during the event broadcast
		TArray<FTLLRN_RigElementKey> ControlKeysCopy = ControlKeys;
		for (const FTLLRN_RigElementKey& ControlKey : ControlKeysCopy)
		{
			ActiveSpaceChangedEvent.Broadcast(StrongHierarchy, ControlKey, InKey);
		}
	}

	if(DialogWindow.IsValid())
	{
		CloseDialog();
	}
	
	return FReply::Handled();
}

FReply STLLRN_RigSpacePickerWidget::HandleSpaceMoveUp(FTLLRN_RigElementKey InKey)
{
	if(CurrentSpaceKeys.Num() > 1)
	{
		const int32 Index = CurrentSpaceKeys.Find(InKey);
		if(CurrentSpaceKeys.IsValidIndex(Index))
		{
			if(Index > 0)
			{
				TArray<FTLLRN_RigElementKey> ChangedSpaceKeys = CurrentSpaceKeys;
				ChangedSpaceKeys.Swap(Index, Index - 1);

				if (Hierarchy.IsValid())
				{
					UTLLRN_RigHierarchy* StrongHierarchy = Hierarchy.Get();
					for(const FTLLRN_RigElementKey& ControlKey : ControlKeys)
					{
						SpaceListChangedEvent.Broadcast(StrongHierarchy, ControlKey, ChangedSpaceKeys);
					}
				}

				return FReply::Handled();
			}
		}
	}
	return FReply::Unhandled();
}

FReply STLLRN_RigSpacePickerWidget::HandleSpaceMoveDown(FTLLRN_RigElementKey InKey)
{
	if(CurrentSpaceKeys.Num() > 1)
	{
		const int32 Index = CurrentSpaceKeys.Find(InKey);
		if(CurrentSpaceKeys.IsValidIndex(Index))
		{
			if(Index < CurrentSpaceKeys.Num() - 1)
			{
				TArray<FTLLRN_RigElementKey> ChangedSpaceKeys = CurrentSpaceKeys;
				ChangedSpaceKeys.Swap(Index, Index + 1);

				if (Hierarchy.IsValid())
				{
					UTLLRN_RigHierarchy* StrongHierarchy = Hierarchy.Get();
					for(const FTLLRN_RigElementKey& ControlKey : ControlKeys)
					{
						SpaceListChangedEvent.Broadcast(StrongHierarchy, ControlKey, ChangedSpaceKeys);
					}
				}
				
				return FReply::Handled();
			}
		}
	}
	return FReply::Unhandled();
}

void STLLRN_RigSpacePickerWidget::HandleSpaceDelete(FTLLRN_RigElementKey InKey)
{
	TArray<FTLLRN_RigElementKey> ChangedSpaceKeys = CurrentSpaceKeys;
	if(ChangedSpaceKeys.Remove(InKey) > 0)
	{
		if (Hierarchy.IsValid())
		{
			UTLLRN_RigHierarchy* StrongHierarchy = Hierarchy.Get();
			for(const FTLLRN_RigElementKey& ControlKey : ControlKeys)
			{
				SpaceListChangedEvent.Broadcast(StrongHierarchy, ControlKey, ChangedSpaceKeys);
			}
		}
	}
}

FReply STLLRN_RigSpacePickerWidget::HandleAddElementClicked()
{
	HierarchyDisplaySettings.bShowConnectors = false;
	HierarchyDisplaySettings.bShowSockets = false;
	HierarchyDisplaySettings.bShowPhysics = false;
	
	FRigTreeDelegates TreeDelegates;
	TreeDelegates.OnGetDisplaySettings = FOnGetRigTreeDisplaySettings::CreateSP(this, &STLLRN_RigSpacePickerWidget::GetHierarchyDisplaySettings); 
	TreeDelegates.OnGetHierarchy = FOnGetRigTreeHierarchy::CreateSP(this, &STLLRN_RigSpacePickerWidget::GetHierarchyConst);
	TreeDelegates.OnMouseButtonClick = FOnRigTreeMouseButtonClick::CreateLambda([this](TSharedPtr<FRigTreeElement> InItem)
	{
		if(InItem.IsValid())
		{
			const FTLLRN_RigElementKey Key = InItem->Key;
			if(!IsDefaultSpace(Key) && IsValidKey(Key))
			{
				if (Hierarchy.IsValid())
				{
					UTLLRN_RigHierarchy* StrongHierarchy = Hierarchy.Get();
					for(const FTLLRN_RigElementKey& ControlKey : ControlKeys)
					{
						UTLLRN_RigHierarchy::TElementDependencyMap DependencyMap;
						FString FailureReason;

						if(UTLLRN_ControlRig* TLLRN_ControlRig = StrongHierarchy->GetTypedOuter<UTLLRN_ControlRig>())
						{
							DependencyMap = StrongHierarchy->GetDependenciesForVM(TLLRN_ControlRig->GetVM()); 
						}
						else if(UTLLRN_ControlRigBlueprint* RigBlueprint = StrongHierarchy->GetTypedOuter<UTLLRN_ControlRigBlueprint>())
						{
							if(UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigBlueprint->GetRigVMBlueprintGeneratedClass()->GetDefaultObject()))
							{
								DependencyMap = StrongHierarchy->GetDependenciesForVM(CDO->GetVM()); 
							}
						}
						
						if(!StrongHierarchy->CanSwitchToParent(ControlKey, Key, DependencyMap, &FailureReason))
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
					}
					
					TArray<FTLLRN_RigElementKey> ChangedSpaceKeys = CurrentSpaceKeys;
					ChangedSpaceKeys.AddUnique(Key);

					for(const FTLLRN_RigElementKey& ControlKey : ControlKeys)
					{
						SpaceListChangedEvent.Broadcast(StrongHierarchy, ControlKey, ChangedSpaceKeys);
					}
				}
			}
		}

		if(ContextMenu.IsValid())
		{
			ContextMenu.Pin()->Dismiss();
			ContextMenu.Reset();
		}
	});

	TreeDelegates.OnCompareKeys = FOnRigTreeCompareKeys::CreateLambda([](const FTLLRN_RigElementKey& A, const FTLLRN_RigElementKey& B) -> bool
	{
		// controls should always show up first - so we'll sort them to the start of the list
		if(A.Type == ETLLRN_RigElementType::Control && B.Type != ETLLRN_RigElementType::Control)
		{
			return true;
		}
		if(B.Type == ETLLRN_RigElementType::Control && A.Type != ETLLRN_RigElementType::Control)
		{
			return false;
		}
		return A < B;
	});

	TSharedPtr<SSearchableTLLRN_RigHierarchyTreeView> SearchableTreeView = SNew(SSearchableTLLRN_RigHierarchyTreeView)
	.RigTreeDelegates(TreeDelegates);
	SearchableTreeView->GetTreeView()->RefreshTreeView(true);

	const bool bFocusImmediately = false;
	// Create as context menu
	TGuardValue<bool> AboutToShowMenu(bLaunchingContextMenu, true);
	ContextMenu = FSlateApplication::Get().PushMenu(
		AsShared(),
		FWidgetPath(),
		SearchableTreeView.ToSharedRef(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu),
		bFocusImmediately
	);

	if(!ContextMenu.IsValid())
	{
		return FReply::Unhandled();
	}
	
	ContextMenu.Pin()->GetOnMenuDismissed().AddLambda([this](TSharedRef<IMenu> InMenu)
	{
		ContextMenu.Reset();

		if(DialogWindow.IsValid())
		{
			DialogWindow.Pin()->BringToFront(true);

			TSharedRef<SWidget> ThisRef = AsShared();
			FSlateApplication::Get().ForEachUser([&ThisRef](FSlateUser& User) {
				User.SetFocus(ThisRef, EFocusCause::SetDirectly);
			});
		}
	});

	return FReply::Handled().SetUserFocus(SearchableTreeView->GetSearchBox(), EFocusCause::SetDirectly);
}

bool STLLRN_RigSpacePickerWidget::IsRestricted() const
{
	if(UTLLRN_RigHierarchy* CurrentHierarchy = GetHierarchy())
	{
		for(const FTLLRN_RigElementKey& Control : GetControls())
		{
			if(const FTLLRN_RigControlElement* ControlElement = CurrentHierarchy->Find<FTLLRN_RigControlElement>(Control))
			{
				if(ControlElement->Settings.bRestrictSpaceSwitching)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool STLLRN_RigSpacePickerWidget::IsSpaceMoveUpEnabled(FTLLRN_RigElementKey InKey) const
{
	if(CurrentSpaceKeys.IsEmpty())
	{
		return false;
	}
	return CurrentSpaceKeys[0] != InKey;
}

bool STLLRN_RigSpacePickerWidget::IsSpaceMoveDownEnabled(FTLLRN_RigElementKey InKey) const
{
	if(CurrentSpaceKeys.IsEmpty())
	{
		return false;
	}
	return CurrentSpaceKeys.Last( )!= InKey;
}

void STLLRN_RigSpacePickerWidget::OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy,
                                                const FTLLRN_RigBaseElement* InElement)
{
	if(InElement == nullptr)
	{
		return;
	}

	if(!ControlKeys.Contains(InElement->GetKey()))
	{
		return;
	}
	
	switch(InNotif)
	{
		case ETLLRN_RigHierarchyNotification::ParentChanged:
		case ETLLRN_RigHierarchyNotification::ParentWeightsChanged:
		case ETLLRN_RigHierarchyNotification::ControlSettingChanged:
		{
			bRepopulateRequired = true;
			break;
		}
		default:
		{
			break;
		}
	}
}

FSlateColor STLLRN_RigSpacePickerWidget::GetButtonColor(ESpacePickerType InType, FTLLRN_RigElementKey InKey) const
{
	static const FSlateColor ActiveColor = FTLLRN_ControlRigEditorStyle::Get().SpacePickerSelectColor;

	switch(InType)
	{
		case ESpacePickerType_Parent:
		{
			// this is also true if the object has no parent
			if(ActiveSpaceKeys.Contains(UTLLRN_RigHierarchy::GetDefaultParentKey()))
			{
				return ActiveColor;
			}
			break;
		}
		case ESpacePickerType_World:
		{
			if(ActiveSpaceKeys.Contains(UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey()))
			{
				return ActiveColor;
			}
			break;
		}
		case ESpacePickerType_Item:
		default:
		{
			if(ActiveSpaceKeys.Contains(InKey) && InKey.IsValid())
			{
				return ActiveColor;
			}
			break;
		}
	}
	return FStyleColors::Transparent;
}

FTLLRN_RigElementKey STLLRN_RigSpacePickerWidget::GetActiveSpace_Private(UTLLRN_RigHierarchy* InHierarchy,
	const FTLLRN_RigElementKey& InControlKey) const
{
	if(InHierarchy)
	{
		return InHierarchy->GetActiveParent(InControlKey);
	}
	return UTLLRN_RigHierarchy::GetDefaultParentKey();
}

TArray<FTLLRN_RigElementKey> STLLRN_RigSpacePickerWidget::GetCurrentParents_Private(UTLLRN_RigHierarchy* InHierarchy,
                                                                const FTLLRN_RigElementKey& InControlKey) const
{
	if(!InControlKey.IsValid() || InHierarchy == nullptr)
	{
		return TArray<FTLLRN_RigElementKey>();
	}

	check(ControlKeys.Contains(InControlKey));
	TArray<FTLLRN_RigElementKey> Parents = InHierarchy->GetParents(InControlKey);
	if(Parents.Num() > 0)
	{
		if(!IsDefaultSpace(Parents[0]))
		{
			Parents[0] = UTLLRN_RigHierarchy::GetDefaultParentKey();
		}
	}
	return Parents;
}

void STLLRN_RigSpacePickerWidget::RepopulateItemSpaces()
{
	if(!ItemSpacesListBox.IsValid())
	{
		return;
	}
	if(!Hierarchy.IsValid())
	{
		return;
	}

	UTLLRN_RigHierarchy* StrongHierarchy = Hierarchy.Get();
	
	TArray<FTLLRN_RigElementKey> FavoriteKeys, SpacesFromDelegate;

	if(bShowFavoriteSpaces)
	{
		for(const FTLLRN_RigElementKey& ControlKey : ControlKeys)
		{
			const FTLLRN_RigControlElementCustomization* Customization = nullptr;
			if(GetControlCustomizationDelegate.IsBound())
			{
				Customization = GetControlCustomizationDelegate.Execute(StrongHierarchy, ControlKey);
			}

			if(Customization)
			{
				for(const FTLLRN_RigElementKey& Key : Customization->AvailableSpaces)
				{
					if(IsDefaultSpace(Key) || !IsValidKey(Key))
					{
						continue;
					}
					FavoriteKeys.AddUnique(Key);
				}
			}
			
			// check if the customization is different from the base one in the asset
			if(const FTLLRN_RigControlElement* ControlElement = StrongHierarchy->Find<FTLLRN_RigControlElement>(ControlKey))
			{
				if(Customization != &ControlElement->Settings.Customization)
				{
					for(const FTLLRN_RigElementKey& Key : ControlElement->Settings.Customization.AvailableSpaces)
					{
						if(IsDefaultSpace(Key) || !IsValidKey(Key))
						{
							continue;
						}

						if(Customization)
						{
							if(Customization->AvailableSpaces.Contains(Key))
							{
								continue;
							}
							if(Customization->RemovedSpaces.Contains(Key))
							{
								continue;
							}
						}
						FavoriteKeys.AddUnique(Key);
					}
				}
			}
		}
	}
	
	// now gather all of the spaces using the get additional spaces delegate
	if(GetAdditionalSpacesDelegate.IsBound() && bShowAdditionalSpaces)
	{
		AdditionalSpaces.Reset();
		for(const FTLLRN_RigElementKey& ControlKey: ControlKeys)
		{
			AdditionalSpaces.Append(GetAdditionalSpacesDelegate.Execute(StrongHierarchy, ControlKey));
		}
		
		for(const FTLLRN_RigElementKey& Key : AdditionalSpaces)
		{
			if(IsDefaultSpace(Key)  || !IsValidKey(Key))
			{
				continue;
			}
			SpacesFromDelegate.AddUnique(Key);
		}
	}

	/*
	struct FKeySortPredicate
	{
		bool operator()(const FTLLRN_RigElementKey& A, const FTLLRN_RigElementKey& B) const
		{
			static TMap<ETLLRN_RigElementType, int32> TypeOrder;
			if(TypeOrder.IsEmpty())
			{
				TypeOrder.Add(ETLLRN_RigElementType::Control, 0);
				TypeOrder.Add(ETLLRN_RigElementType::Reference, 1);
				TypeOrder.Add(ETLLRN_RigElementType::Null, 2);
				TypeOrder.Add(ETLLRN_RigElementType::Bone, 3);
				TypeOrder.Add(ETLLRN_RigElementType::RigidBody, 4);
			}

			const int32 TypeIndexA = TypeOrder.FindChecked(A.Type);
			const int32 TypeIndexB = TypeOrder.FindChecked(B.Type);
			if(TypeIndexA != TypeIndexB)
			{
				return TypeIndexA < TypeIndexB;
			}

			return A.Name.Compare(B.Name) < 0; 
		}
	};
	SpacesFromDelegate.Sort(FKeySortPredicate());
	*/

	TArray<FTLLRN_RigElementKey> Keys = FavoriteKeys;
	for(const FTLLRN_RigElementKey& Key : SpacesFromDelegate)
	{
		Keys.AddUnique(Key);
	}

	if(Keys == CurrentSpaceKeys)
	{
		return;
	}

	ClearListBox(ItemSpacesListBox);

	for(const FTLLRN_RigElementKey& Key : Keys)
	{
		TPair<const FSlateBrush*, FSlateColor> IconAndColor = STLLRN_RigHierarchyItem::GetBrushForElementType(StrongHierarchy, Key);
		
		AddSpacePickerRow(
			ItemSpacesListBox,
			ESpacePickerType_Item,
			Key,
			IconAndColor.Key,
			IconAndColor.Value,
			FText::FromName(Key.Name),
			FOnClicked::CreateSP(this, &STLLRN_RigSpacePickerWidget::HandleElementSpaceClicked, Key)
		);
	}

	CurrentSpaceKeys = Keys;
}

void STLLRN_RigSpacePickerWidget::ClearListBox(TSharedPtr<SVerticalBox> InListBox)
{
	InListBox->ClearChildren();
}

void STLLRN_RigSpacePickerWidget::UpdateActiveSpaces()
{
	ActiveSpaceKeys.Reset();

	if(!Hierarchy.IsValid())
	{
		return;
	}

	UTLLRN_RigHierarchy* StrongHierarchy = Hierarchy.Get();	
	for(int32 ControlIndex=0;ControlIndex<ControlKeys.Num();ControlIndex++)
	{
		ActiveSpaceKeys.Add(UTLLRN_RigHierarchy::GetDefaultParentKey());

		if(GetActiveSpaceDelegate.IsBound())
		{
			ActiveSpaceKeys[ControlIndex] = GetActiveSpaceDelegate.Execute(StrongHierarchy, ControlKeys[ControlIndex]);
		}
	}
}

bool STLLRN_RigSpacePickerWidget::IsValidKey(const FTLLRN_RigElementKey& InKey) const
{
	if(!InKey.IsValid())
	{
		return false;
	}
	if(Hierarchy == nullptr)
	{
		return false;
	}
	return Hierarchy->Contains(InKey);
}

bool STLLRN_RigSpacePickerWidget::IsDefaultSpace(const FTLLRN_RigElementKey& InKey) const
{
	if(bShowDefaultSpaces)
	{
		return InKey == UTLLRN_RigHierarchy::GetDefaultParentKey() || InKey == UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey();
	}
	return false;
}

void STLLRN_RigSpacePickerWidget::PostUndo(bool bSuccess)
{
	RefreshContents();
}
void STLLRN_RigSpacePickerWidget::PostRedo(bool bSuccess)
{
	RefreshContents();
}

//////////////////////////////////////////////////////////////
/// STLLRN_RigSpacePickerBakeWidget
///////////////////////////////////////////////////////////

void STLLRN_RigSpacePickerBakeWidget::Construct(const FArguments& InArgs)
{
	check(InArgs._Hierarchy);
	check(InArgs._Controls.Num() > 0);
	check(InArgs._Sequencer);
	check(InArgs._OnBake.IsBound());
	
	Settings = MakeShared<TStructOnScope<FTLLRN_TLLRN_RigSpacePickerBakeSettings>>();
	Settings->InitializeAs<FTLLRN_TLLRN_RigSpacePickerBakeSettings>();
	*Settings = InArgs._Settings;
	//always setting space to be parent as default, since stored space may not be available.
	Settings->Get()->TargetSpace = UTLLRN_RigHierarchy::GetDefaultParentKey();
	Sequencer = InArgs._Sequencer;

	FStructureDetailsViewArgs StructureViewArgs;
	StructureViewArgs.bShowObjects = true;
	StructureViewArgs.bShowAssets = true;
	StructureViewArgs.bShowClasses = true;
	StructureViewArgs.bShowInterfaces = true;

	FDetailsViewArgs ViewArgs;
	ViewArgs.bAllowSearch = false;
	ViewArgs.bHideSelectionTip = false;
	ViewArgs.bShowObjectLabel = false;

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	DetailsView = PropertyEditor.CreateStructureDetailView(ViewArgs, StructureViewArgs, TSharedPtr<FStructOnScope>());
	TSharedPtr<INumericTypeInterface<double>> NumericTypeInterface = Sequencer->GetNumericTypeInterface();
	DetailsView->GetDetailsView()->RegisterInstancedCustomPropertyTypeLayout("FrameNumber",
		FOnGetPropertyTypeCustomizationInstance::CreateLambda([=]() {return MakeShared<FFrameNumberDetailsCustomization>(NumericTypeInterface); }));
	DetailsView->SetStructureData(Settings);

	ChildSlot
	[
		SNew(SBorder)
		.Visibility(EVisibility::Visible)
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(SpacePickerWidget, STLLRN_RigSpacePickerWidget)
				.Hierarchy(InArgs._Hierarchy)
				.Controls(InArgs._Controls)
				.AllowDelete(false)
				.AllowReorder(false)
				.AllowAdd(true)
				.ShowBakeAndCompensateButton(false)
				.GetControlCustomization_Lambda([this] (UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey)
				{
					return &Customization;
				})
				.OnSpaceListChanged_Lambda([this](UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey&, const TArray<FTLLRN_RigElementKey>& InSpaceList)
				{
					if(Customization.AvailableSpaces != InSpaceList)
					{
						Customization.AvailableSpaces = InSpaceList;
						SpacePickerWidget->RefreshContents();
					}
				})
				.GetActiveSpace_Lambda([this](UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey&)
				{
					return Settings->Get()->TargetSpace;
				})
				.OnActiveSpaceChanged_Lambda([this] (UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey&, const FTLLRN_RigElementKey InSpaceKey)
				{
					if(Settings->Get()->TargetSpace != InSpaceKey)
					{
						Settings->Get()->TargetSpace = InSpaceKey;
						SpacePickerWidget->RefreshContents();
					}
				})
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 0.f)
			[
				DetailsView->GetWidget().ToSharedRef()
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 16.f, 0.f, 16.f)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SSpacer)
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
					.Text(LOCTEXT("OK", "OK"))
					.OnClicked_Lambda([this, InArgs]()
					{
						FReply Reply =  InArgs._OnBake.Execute(SpacePickerWidget->GetHierarchy(), SpacePickerWidget->GetControls(),*(Settings->Get()));
						CloseDialog();
						return Reply;

					})
					.IsEnabled_Lambda([this]()
					{
						return Settings->Get()->TargetSpace.IsValid();
					})
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(8.f, 0.f, 16.f, 0.f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
					.Text(LOCTEXT("Cancel", "Cancel"))
					.OnClicked_Lambda([this]()
					{
						CloseDialog();
						return FReply::Handled();
					})
				]
			]
		]
	];
}

FReply STLLRN_RigSpacePickerBakeWidget::OpenDialog(bool bModal)
{
	check(!DialogWindow.IsValid());
		
	const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();

	TSharedRef<STLLRN_RigSpaceDialogWindow> Window = SNew(STLLRN_RigSpaceDialogWindow)
	.Title( LOCTEXT("STLLRN_RigSpacePickerBakeWidgetTitle", "Bake Controls To Specified Space") )
	.CreateTitleBar(true)
	.Type(EWindowType::Normal)
	.SizingRule( ESizingRule::Autosized )
	.ScreenPosition(CursorPos)
	.FocusWhenFirstShown(true)
	.ActivationPolicy(EWindowActivationPolicy::FirstShown)
	[
		AsShared()
	];
	
	Window->SetWidgetToFocusOnActivate(AsShared());
	
	DialogWindow = Window;

	Window->MoveWindowTo(CursorPos);

	if(bModal)
	{
		GEditor->EditorAddModalWindow(Window);
	}
	else
	{
		FSlateApplication::Get().AddWindow( Window );
	}

	return FReply::Handled();
}

void STLLRN_RigSpacePickerBakeWidget::CloseDialog()
{
	if ( DialogWindow.IsValid() )
	{
		DialogWindow.Pin()->RequestDestroyWindow();
		DialogWindow.Reset();
	}
}

#undef LOCTEXT_NAMESPACE
