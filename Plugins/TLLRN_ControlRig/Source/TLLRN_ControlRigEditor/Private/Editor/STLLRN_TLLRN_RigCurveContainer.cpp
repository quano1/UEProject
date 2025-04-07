// Copyright Epic Games, Inc. All Rights Reserved.


#include "Editor/STLLRN_TLLRN_RigCurveContainer.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Input/STextEntryPopup.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/Commands/GenericCommands.h"
#include "Editor/TLLRN_TLLRN_RigCurveContainerCommands.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "ScopedTransaction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Rigs/TLLRN_RigHierarchyController.h"

#define LOCTEXT_NAMESPACE "STLLRN_TLLRN_RigCurveContainer"

static const FName ColumnId_TLLRN_RigCurveNameLabel( "Curve" );
static const FName ColumnID_TLLRN_RigCurveValueLabel( "Value" );

//////////////////////////////////////////////////////////////////////////
// STLLRN_RigCurveListRow

void STLLRN_RigCurveListRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	Item = InArgs._Item;
	OnTextCommitted = InArgs._OnTextCommitted;
	OnSetTLLRN_RigCurveValue = InArgs._OnSetTLLRN_RigCurveValue;
	OnGetTLLRN_RigCurveValue = InArgs._OnGetTLLRN_RigCurveValue;
	OnGetFilterText = InArgs._OnGetFilterText;

	check( Item.IsValid() );

	SMultiColumnTableRow< FDisplayedTLLRN_RigCurveInfoPtr >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
}

TSharedRef< SWidget > STLLRN_RigCurveListRow::GenerateWidgetForColumn( const FName& ColumnName )
{
	if ( ColumnName == ColumnId_TLLRN_RigCurveNameLabel )
	{
		return
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4)
			.VAlign(VAlign_Center)
			[
				SAssignNew(Item->EditableText, SInlineEditableTextBlock)
				.OnTextCommitted(OnTextCommitted)
				.ColorAndOpacity(this, &STLLRN_RigCurveListRow::GetItemTextColor)
				.IsSelected(this, &STLLRN_RigCurveListRow::IsSelected)
				.Text(this, &STLLRN_RigCurveListRow::GetItemName)
				.HighlightText(this, &STLLRN_RigCurveListRow::GetFilterText)
			];
	}
	else if ( ColumnName == ColumnID_TLLRN_RigCurveValueLabel )
	{
		// Encase the SSpinbox in an SVertical box so we can apply padding. Setting ItemHeight on the containing SListView has no effect :-(
		return
			SNew( SVerticalBox )

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 0.0f, 1.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SSpinBox<float> )
				.Value( this, &STLLRN_RigCurveListRow::GetValue )
				.OnValueChanged( this, &STLLRN_RigCurveListRow::OnTLLRN_RigCurveValueChanged )
				.OnValueCommitted( this, &STLLRN_RigCurveListRow::OnTLLRN_RigCurveValueValueCommitted )
				.IsEnabled(false)
			];
	}

	return SNullWidget::NullWidget;
}

void STLLRN_RigCurveListRow::OnTLLRN_RigCurveValueChanged( float NewValue )
{
	Item->Value = NewValue;

	OnSetTLLRN_RigCurveValue.ExecuteIfBound(Item->CurveName, NewValue);
}

void STLLRN_RigCurveListRow::OnTLLRN_RigCurveValueValueCommitted( float NewValue, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
	{
		OnTLLRN_RigCurveValueChanged(NewValue);
	}
}


FText STLLRN_RigCurveListRow::GetItemName() const
{
	return FText::FromName(Item->CurveName);
}

FText STLLRN_RigCurveListRow::GetFilterText() const
{
	if (OnGetFilterText.IsBound())
	{
		return OnGetFilterText.Execute();
	}
	else
	{
		return FText::GetEmpty();
	}
}

FSlateColor STLLRN_RigCurveListRow::GetItemTextColor() const
{
	// If row is selected, show text as black to make it easier to read
	if (IsSelected())
	{
		return FLinearColor(0, 0, 0);
	}

	return FLinearColor(1, 1, 1);
}

float STLLRN_RigCurveListRow::GetValue() const 
{ 
	if (OnGetTLLRN_RigCurveValue.IsBound())
	{
		return OnGetTLLRN_RigCurveValue.Execute(Item->CurveName);
	}

	return 0.f;
}

//////////////////////////////////////////////////////////////////////////
// STLLRN_TLLRN_RigCurveContainer

void STLLRN_TLLRN_RigCurveContainer::Construct(const FArguments& InArgs, TSharedRef<FTLLRN_ControlRigEditor> InTLLRN_ControlRigEditor)
{
	TLLRN_ControlRigEditor = InTLLRN_ControlRigEditor;
	TLLRN_ControlRigBlueprint = InTLLRN_ControlRigEditor.Get().GetTLLRN_ControlRigBlueprint();
	bIsChangingTLLRN_RigHierarchy = false;

	TLLRN_ControlRigBlueprint->Hierarchy->OnModified().AddRaw(this, &STLLRN_TLLRN_RigCurveContainer::OnHierarchyModified);
	TLLRN_ControlRigBlueprint->OnRefreshEditor().AddRaw(this, &STLLRN_TLLRN_RigCurveContainer::HandleRefreshEditorFromBlueprint);

	UEditorEngine* Editor = Cast<UEditorEngine>(GEngine);
	if (Editor != nullptr)
	{
		Editor->RegisterForUndo(this);
	}


	// Register and bind all our menu commands
	FCurveContainerCommands::Register();
	BindCommands();

	ChildSlot
	[
		SNew( SVerticalBox )
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0,2)
		[
			SNew(SHorizontalBox)
			// Filter entry
			+SHorizontalBox::Slot()
			.FillWidth( 1 )
			[
				SAssignNew( NameFilterBox, SSearchBox )
				.SelectAllTextWhenFocused( true )
				.OnTextChanged( this, &STLLRN_TLLRN_RigCurveContainer::OnFilterTextChanged )
				.OnTextCommitted( this, &STLLRN_TLLRN_RigCurveContainer::OnFilterTextCommitted )
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight( 1.0f )		// This is required to make the scrollbar work, as content overflows Slate containers by default
		[
			SAssignNew( TLLRN_RigCurveListView, STLLRN_RigCurveListType )
			.ListItemsSource( &TLLRN_RigCurveList )
			.OnGenerateRow( this, &STLLRN_TLLRN_RigCurveContainer::GenerateTLLRN_RigCurveRow )
			.OnContextMenuOpening( this, &STLLRN_TLLRN_RigCurveContainer::OnGetContextMenuContent )
			.SelectionMode(ESelectionMode::Multi)
			.OnSelectionChanged( this, &STLLRN_TLLRN_RigCurveContainer::OnSelectionChanged )
			.HeaderRow
			(
				SNew( SHeaderRow )
				+ SHeaderRow::Column( ColumnId_TLLRN_RigCurveNameLabel )
				.FillWidth(1.f)
				.DefaultLabel( LOCTEXT( "TLLRN_RigCurveNameLabel", "Curve" ) )

				+ SHeaderRow::Column( ColumnID_TLLRN_RigCurveValueLabel )
				.FillWidth(1.f)
				.DefaultLabel( LOCTEXT( "TLLRN_RigCurveValueLabel", "Value" ) )
			)
		]
	];

	CreateTLLRN_RigCurveList();

	if(TLLRN_ControlRigEditor.IsValid())
	{
		TLLRN_ControlRigEditor.Pin()->OnEditorClosed().AddSP(this, &STLLRN_TLLRN_RigCurveContainer::OnEditorClose);
	}
}

STLLRN_TLLRN_RigCurveContainer::~STLLRN_TLLRN_RigCurveContainer()
{
	const FTLLRN_ControlRigEditor* Editor = TLLRN_ControlRigEditor.IsValid() ? TLLRN_ControlRigEditor.Pin().Get() : nullptr;
	OnEditorClose(Editor, TLLRN_ControlRigBlueprint.Get());
}

FReply STLLRN_TLLRN_RigCurveContainer::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (UICommandList.IsValid() && UICommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void STLLRN_TLLRN_RigCurveContainer::BindCommands()
{
	// This should not be called twice on the same instance
	check(!UICommandList.IsValid());

	UICommandList = MakeShareable(new FUICommandList);

	FUICommandList& CommandList = *UICommandList;

	// Grab the list of menu commands to bind...
	const FCurveContainerCommands& MenuActions = FCurveContainerCommands::Get();

	// ...and bind them all

	CommandList.MapAction(
		FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &STLLRN_TLLRN_RigCurveContainer::OnRenameClicked),
		FCanExecuteAction::CreateSP(this, &STLLRN_TLLRN_RigCurveContainer::CanRename));

	CommandList.MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &STLLRN_TLLRN_RigCurveContainer::OnDeleteNameClicked),
		FCanExecuteAction::CreateSP(this, &STLLRN_TLLRN_RigCurveContainer::CanDelete));

	CommandList.MapAction(
		MenuActions.AddCurve,
		FExecuteAction::CreateSP(this, &STLLRN_TLLRN_RigCurveContainer::OnAddClicked),
		FCanExecuteAction());
}

void STLLRN_TLLRN_RigCurveContainer::OnPreviewMeshChanged(class USkeletalMesh* OldPreviewMesh, class USkeletalMesh* NewPreviewMesh)
{
	RefreshCurveList();
}

void STLLRN_TLLRN_RigCurveContainer::OnFilterTextChanged( const FText& SearchText )
{
	FilterText = SearchText;

	RefreshCurveList();
}

void STLLRN_TLLRN_RigCurveContainer::OnFilterTextCommitted( const FText& SearchText, ETextCommit::Type CommitInfo )
{
	// Just do the same as if the user typed in the box
	OnFilterTextChanged( SearchText );
}

TSharedRef<ITableRow> STLLRN_TLLRN_RigCurveContainer::GenerateTLLRN_RigCurveRow(FDisplayedTLLRN_RigCurveInfoPtr InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check( InInfo.IsValid() );

	return
		SNew( STLLRN_RigCurveListRow, OwnerTable)
		.Item( InInfo )
		.OnTextCommitted(this, &STLLRN_TLLRN_RigCurveContainer::OnNameCommitted, InInfo)
		.OnSetTLLRN_RigCurveValue(this, &STLLRN_TLLRN_RigCurveContainer::SetCurveValue)
		.OnGetTLLRN_RigCurveValue(this, &STLLRN_TLLRN_RigCurveContainer::GetCurveValue)
		.OnGetFilterText(this, &STLLRN_TLLRN_RigCurveContainer::GetFilterText);
}

TSharedPtr<SWidget> STLLRN_TLLRN_RigCurveContainer::OnGetContextMenuContent() const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, UICommandList);

	const FCurveContainerCommands& Actions = FCurveContainerCommands::Get();

	MenuBuilder.BeginSection("TLLRN_RigCurveAction", LOCTEXT( "CurveAction", "Curve Actions" ) );

	MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename, NAME_None, LOCTEXT("RenameSmartNameLabel", "Rename Curve"), LOCTEXT("RenameSmartNameToolTip", "Rename the selected curve"));
	MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete, NAME_None, LOCTEXT("DeleteSmartNameLabel", "Delete Curve"), LOCTEXT("DeleteSmartNameToolTip", "Delete the selected curve"));
	MenuBuilder.AddMenuEntry(Actions.AddCurve);
	MenuBuilder.AddMenuSeparator();
	MenuBuilder.AddSubMenu(
		LOCTEXT("ImportSubMenu", "Import"),
		LOCTEXT("ImportSubMenu_ToolTip", "Import curves to the current rig. This only imports non-existing curve."),
		FNewMenuDelegate::CreateSP(const_cast<STLLRN_TLLRN_RigCurveContainer*>(this), &STLLRN_TLLRN_RigCurveContainer::CreateImportMenu)
	);


	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void STLLRN_TLLRN_RigCurveContainer::OnEditorClose(const FRigVMEditor* InEditor, URigVMBlueprint* InBlueprint)
{
	if (UTLLRN_ControlRigBlueprint* BP = Cast<UTLLRN_ControlRigBlueprint>(InBlueprint))
	{
		BP->Hierarchy->OnModified().RemoveAll(this);
		BP->OnRefreshEditor().RemoveAll(this);
	}

	TLLRN_ControlRigBlueprint.Reset();
	TLLRN_ControlRigEditor.Reset();
}

void STLLRN_TLLRN_RigCurveContainer::OnRenameClicked()
{
	TArray< FDisplayedTLLRN_RigCurveInfoPtr > SelectedItems = TLLRN_RigCurveListView->GetSelectedItems();

	SelectedItems[0]->EditableText->EnterEditingMode();
}

bool STLLRN_TLLRN_RigCurveContainer::CanRename()
{
	return TLLRN_RigCurveListView->GetNumItemsSelected() == 1;
}

void STLLRN_TLLRN_RigCurveContainer::OnAddClicked()
{
	TSharedRef<STextEntryPopup> TextEntry =
		SNew(STextEntryPopup)
		.Label(LOCTEXT("NewSmartnameLabel", "New Name"))
		.OnTextCommitted(this, &STLLRN_TLLRN_RigCurveContainer::CreateNewNameEntry);

	FSlateApplication& SlateApp = FSlateApplication::Get();
	SlateApp.PushMenu(
		AsShared(),
		FWidgetPath(),
		TextEntry,
		SlateApp.GetCursorPos(),
		FPopupTransitionEffect::TypeInPopup
		);
}


void STLLRN_TLLRN_RigCurveContainer::CreateNewNameEntry(const FText& CommittedText, ETextCommit::Type CommitType)
{
	FSlateApplication::Get().DismissAllMenus();
	if (!CommittedText.IsEmpty() && CommitType == ETextCommit::OnEnter)
	{
		UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
		if (Hierarchy)
		{
			TGuardValue<bool> GuardReentry(bIsChangingTLLRN_RigHierarchy, true);

			const FName NewName = FName(*CommittedText.ToString());

			if(UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController())
			{
				const FTLLRN_RigElementKey CurveKey = Controller->AddCurve(NewName, 0.f, true);
				Controller->ClearSelection();
				Controller->SelectElement(CurveKey);
			}
		}

		FSlateApplication::Get().DismissAllMenus();
		RefreshCurveList();
	}
}

void STLLRN_TLLRN_RigCurveContainer::CreateTLLRN_RigCurveList( const FString& SearchText )
{
	if(bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}
	TGuardValue<bool> GuardReentry(bIsChangingTLLRN_RigHierarchy, true);

	if(!TLLRN_ControlRigBlueprint.IsValid())
	{
		return;
	}
	
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if (Hierarchy)
	{
		TLLRN_RigCurveList.Reset();

		// Iterate through all curves..
		Hierarchy->ForEach<FTLLRN_RigCurveElement>([this](FTLLRN_RigCurveElement* CurveElement) -> bool
		{
			const FString CurveString = CurveElement->GetName();
			
			// See if we pass the search filter
            if (!FilterText.IsEmpty() && !CurveString.Contains(*FilterText.ToString()))
            {
                return true;
            }

            TSharedRef<FDisplayedTLLRN_RigCurveInfo> NewItem = FDisplayedTLLRN_RigCurveInfo::Make(CurveElement->GetFName());
            TLLRN_RigCurveList.Add(NewItem);

			return true;
		});

		// Sort final list
		struct FSortNamesAlphabetically
		{
			bool operator()(const FDisplayedTLLRN_RigCurveInfoPtr& A, const FDisplayedTLLRN_RigCurveInfoPtr& B) const
			{
				return (A.Get()->CurveName.Compare(B.Get()->CurveName) < 0);
			}
		};

		TLLRN_RigCurveList.Sort(FSortNamesAlphabetically());
	}
	TLLRN_RigCurveListView->RequestListRefresh();

	if (Hierarchy)
	{
		TArray<FTLLRN_RigElementKey> SelectedCurveKeys = Hierarchy->GetSelectedKeys(ETLLRN_RigElementType::Curve);
		for (const FTLLRN_RigElementKey& SelectedCurveKey : SelectedCurveKeys)
		{
			if(FTLLRN_RigCurveElement* CurveElement = Hierarchy->Find<FTLLRN_RigCurveElement>(SelectedCurveKey))
			{
				OnHierarchyModified(ETLLRN_RigHierarchyNotification::ElementSelected, Hierarchy, CurveElement);
			}
		}
	}

}

void STLLRN_TLLRN_RigCurveContainer::RefreshCurveList()
{
	CreateTLLRN_RigCurveList(FilterText.ToString());
}

void STLLRN_TLLRN_RigCurveContainer::OnNameCommitted(const FText& InNewName, ETextCommit::Type CommitType, FDisplayedTLLRN_RigCurveInfoPtr Item)
{
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if (Hierarchy)
	{
		if (CommitType == ETextCommit::OnEnter)
		{
			if(UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController())
			{
				FName NewName = FName(*InNewName.ToString());
				FName OldName = Item->CurveName;
				Controller->RenameElement(FTLLRN_RigElementKey(OldName, ETLLRN_RigElementType::Curve), NewName, true, true);
			}
		}
	}
}

void STLLRN_TLLRN_RigCurveContainer::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		RefreshCurveList();
	}
}

void STLLRN_TLLRN_RigCurveContainer::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
		RefreshCurveList();
	}
}

void STLLRN_TLLRN_RigCurveContainer::OnDeleteNameClicked()
{
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if (Hierarchy)
	{
		TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);

		TArray< FDisplayedTLLRN_RigCurveInfoPtr > SelectedItems = TLLRN_RigCurveListView->GetSelectedItems();
		for (auto Item : SelectedItems)
		{
			if(UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController())
			{
				Controller->RemoveElement(FTLLRN_RigElementKey(Item->CurveName, ETLLRN_RigElementType::Curve), true, true);
			}
		}
	}

	TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
	RefreshCurveList();
}

bool STLLRN_TLLRN_RigCurveContainer::CanDelete()
{
	return TLLRN_RigCurveListView->GetNumItemsSelected() > 0;
}

void STLLRN_TLLRN_RigCurveContainer::SetCurveValue(const FName& CurveName, float CurveValue)
{
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if (Hierarchy)
	{
		Hierarchy->SetCurveValue(FTLLRN_RigElementKey(CurveName, ETLLRN_RigElementType::Curve), CurveValue, true);
	}
}

float STLLRN_TLLRN_RigCurveContainer::GetCurveValue(const FName& CurveName)
{
	UTLLRN_RigHierarchy* Hierarchy = GetInstanceHierarchy();
	if (Hierarchy)
	{
		return Hierarchy->GetCurveValue(FTLLRN_RigElementKey(CurveName, ETLLRN_RigElementType::Curve));
	}
	return 0.f;
}

void STLLRN_TLLRN_RigCurveContainer::ChangeCurveName(const FName& OldName, const FName& NewName)
{
	TGuardValue<bool> GuardReentry(bIsChangingTLLRN_RigHierarchy, true);

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if (Hierarchy)
	{
		if(UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController())
		{
			Controller->RenameElement(FTLLRN_RigElementKey(OldName, ETLLRN_RigElementType::Curve), NewName, true, true);
		}
	}
}

void STLLRN_TLLRN_RigCurveContainer::OnSelectionChanged(FDisplayedTLLRN_RigCurveInfoPtr Selection, ESelectInfo::Type SelectInfo)
{
	if (bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if (Hierarchy)
	{
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController();
		if(Controller == nullptr)
		{
			return;
		}

		FScopedTransaction ScopedTransaction(LOCTEXT("SelectCurveTransaction", "Select Curve"), !GIsTransacting);

		TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);

		TArray<FTLLRN_RigElementKey> OldSelection = Hierarchy->GetSelectedKeys(ETLLRN_RigElementType::Curve);
		TArray<FTLLRN_RigElementKey> NewSelection;

		TArray<FDisplayedTLLRN_RigCurveInfoPtr> SelectedItems = TLLRN_RigCurveListView->GetSelectedItems();
		for (const FDisplayedTLLRN_RigCurveInfoPtr& SelectedItem : SelectedItems)
		{
			NewSelection.Add(FTLLRN_RigElementKey(SelectedItem->CurveName, ETLLRN_RigElementType::Curve));
		}

		for (const FTLLRN_RigElementKey& PreviouslySelected : OldSelection)
		{
			if (NewSelection.Contains(PreviouslySelected))
			{
				continue;
			}
			Controller->DeselectElement(PreviouslySelected);
		}

		for (const FTLLRN_RigElementKey& NewlySelected : NewSelection)
		{
			Controller->SelectElement(NewlySelected);
		}
	}
}

void STLLRN_TLLRN_RigCurveContainer::OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	if (bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}

	if(InElement)
	{
		if(!InElement->IsTypeOf(ETLLRN_RigElementType::Curve))
		{
			return;
		}
	}

	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		if (TLLRN_ControlRigBlueprint->bSuspendAllNotifications)
		{
			return;
		}
	}

	switch(InNotif)
	{
		case ETLLRN_RigHierarchyNotification::ElementAdded:
		case ETLLRN_RigHierarchyNotification::ElementRemoved:
		case ETLLRN_RigHierarchyNotification::ElementRenamed:
		case ETLLRN_RigHierarchyNotification::ElementReordered:
		case ETLLRN_RigHierarchyNotification::HierarchyReset:
		{
			RefreshCurveList();
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementSelected:
    	case ETLLRN_RigHierarchyNotification::ElementDeselected:
		{
			if(InElement)
			{
				const bool bSelected = InNotif == ETLLRN_RigHierarchyNotification::ElementSelected;
				for(const FDisplayedTLLRN_RigCurveInfoPtr& Item : TLLRN_RigCurveList)
				{
					if (Item->CurveName == InElement->GetFName())
					{
						TLLRN_RigCurveListView->SetItemSelection(Item, bSelected);
						break;
					}
				}
			}
			break;
		}
		default:
		{
			break;
		}
    }
}

void STLLRN_TLLRN_RigCurveContainer::HandleRefreshEditorFromBlueprint(URigVMBlueprint* InBlueprint)
{
	RefreshCurveList();
}

void STLLRN_TLLRN_RigCurveContainer::CreateImportMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddWidget(
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3)
		[
			SNew(STextBlock)
			.Font(FAppStyle::GetFontStyle("TLLRN_ControlRig.Curve.Menu"))
			.Text(LOCTEXT("ImportMesh_Title", "Select Mesh"))
			.ToolTipText(LOCTEXT("ImportMesh_Tooltip", "Select Mesh to import Curve from... It will only import if the node doesn't exist in the current Curve."))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3)
		[
			SNew(SObjectPropertyEntryBox)
			//.ObjectPath_UObject(this, &STLLRN_TLLRN_RigCurveContainer::GetCurrentHierarchy)
			.OnShouldFilterAsset(this, &STLLRN_TLLRN_RigCurveContainer::ShouldFilterOnImport)
			.OnObjectChanged(this, &STLLRN_TLLRN_RigCurveContainer::ImportCurve)
		]
		,
		FText()
		);
}

bool STLLRN_TLLRN_RigCurveContainer::ShouldFilterOnImport(const FAssetData& AssetData) const
{
	return (AssetData.AssetClassPath != USkeletalMesh::StaticClass()->GetClassPathName() &&
		AssetData.AssetClassPath != USkeleton::StaticClass()->GetClassPathName());
}

void STLLRN_TLLRN_RigCurveContainer::ImportCurve(const FAssetData& InAssetData)
{
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if (Hierarchy)
	{
		TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);

		USkeleton* Skeleton = nullptr;
		USkeletalMesh* Mesh = Cast<USkeletalMesh>(InAssetData.GetAsset());
		if (Mesh)
		{
			Skeleton = Mesh->GetSkeleton();
			TLLRN_ControlRigBlueprint->SourceCurveImport = Skeleton;
		}
		else 
		{
			Skeleton = Cast<USkeleton>(InAssetData.GetAsset());
			TLLRN_ControlRigBlueprint->SourceCurveImport = Skeleton;
		}

		if (Skeleton)
		{
			TGuardValue<bool> GuardReentry(bIsChangingTLLRN_RigHierarchy, true);

			FScopedTransaction Transaction(LOCTEXT("CurveImport", "Import Curve"));
			TLLRN_ControlRigBlueprint->Modify();
			
			if(UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController())
			{
				Controller->ClearSelection();
				if(Mesh)
				{
					Controller->ImportCurvesFromSkeletalMesh(Mesh, NAME_None, false, true, true);
				}
				else
				{
					Controller->ImportCurves(Skeleton, NAME_None, false, true, true);
				}
			}

			FSlateApplication::Get().DismissAllMenus();
		}
	}

	RefreshCurveList();
	TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
}

UTLLRN_RigHierarchy* STLLRN_TLLRN_RigCurveContainer::GetHierarchy() const
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		return TLLRN_ControlRigBlueprint->Hierarchy;
	}
	return nullptr;
}

UTLLRN_RigHierarchy* STLLRN_TLLRN_RigCurveContainer::GetInstanceHierarchy() const
{
	if (TLLRN_ControlRigEditor.IsValid())
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigEditor.Pin()->GetInstanceRig();
		if (TLLRN_ControlRig)
		{
			return TLLRN_ControlRig->GetHierarchy();
		}
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
