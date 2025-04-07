// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_SRetargetOpStack.h"

#include "UObject/UObjectIterator.h"
#include "SPositiveActionButton.h"
#include "RetargetEditor/TLLRN_IKRetargetEditorController.h"
#include "Retargeter/TLLRN_IKRetargetOps.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"

#define LOCTEXT_NAMESPACE "TLLRN_SRetargetOpStack"

TSharedRef<ITableRow> FRetargetOpStackElement::MakeListRowWidget(
	const TSharedRef<STableViewBase>& InOwnerTable,
	TSharedRef<FRetargetOpStackElement> InStackElement,
	TSharedPtr<TLLRN_SRetargetOpStack> InOpStackWidget)
{
	return SNew(TLLRN_SRetargetOpStackItem, InOwnerTable, InStackElement, InOpStackWidget);
}

void TLLRN_SRetargetOpStackItem::Construct(
	const FArguments& InArgs,
	const TSharedRef<STableViewBase>& OwnerTable,
	TSharedRef<FRetargetOpStackElement> InStackElement,
	TSharedPtr<TLLRN_SRetargetOpStack> InOpStackWidget)
{
	StackElement = InStackElement;
	OpStackWidget = InOpStackWidget;
	
	STableRow<TSharedPtr<FRetargetOpStackElement>>::Construct(
        STableRow<TSharedPtr<FRetargetOpStackElement>>::FArguments()
        .OnDragDetected(InOpStackWidget.Get(), &TLLRN_SRetargetOpStack::OnDragDetected)
        .OnCanAcceptDrop(InOpStackWidget.Get(), &TLLRN_SRetargetOpStack::OnCanAcceptDrop)
        .OnAcceptDrop(InOpStackWidget.Get(), &TLLRN_SRetargetOpStack::OnAcceptDrop)
        .Content()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .HAlign(HAlign_Left)
            .Padding(3)
            [
            	SNew(SHorizontalBox)
	            + SHorizontalBox::Slot()
	            .MaxWidth(18)
				.FillWidth(1.0)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
	            [
	                SNew(SImage)
	                .Image(FTLLRN_IKRigEditorStyle::Get().GetBrush("IKRig.DragSolver"))
	            ]

	            + SHorizontalBox::Slot()
	            .AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(3.0f, 1.0f)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([InOpStackWidget, InStackElement]() -> ECheckBoxState
					{
						bool bEnabled = true;
						if (InOpStackWidget.IsValid() &&
							InOpStackWidget->EditorController.IsValid() &&
							InOpStackWidget->EditorController.Pin().IsValid())
						{
							const TObjectPtr<UTLLRN_IKRetargeterController> AssetController = InOpStackWidget->EditorController.Pin()->AssetController;
							bEnabled = AssetController->GetRetargetOpEnabled(InStackElement->IndexInStack);
						}
						return bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([InOpStackWidget, InStackElement](ECheckBoxState InCheckBoxState)
					{
						if (InOpStackWidget.IsValid() &&
							InOpStackWidget->EditorController.IsValid() &&
							InOpStackWidget->EditorController.Pin().IsValid())
						{
							const bool bIsChecked = InCheckBoxState == ECheckBoxState::Checked;
							InOpStackWidget->EditorController.Pin()->AssetController->SetRetargetOpEnabled(InStackElement->IndexInStack, bIsChecked);
							InOpStackWidget->EditorController.Pin()->ReinitializeRetargeterNoUIRefresh();
						}
					})
				]
	     
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(3.0f, 1.0f)
	            [
					SNew(STextBlock)
					.Text(InStackElement->DisplayName)
					.IsEnabled_Lambda([this]()
					{
						return IsOpEnabled();
					})
					.TextStyle(FAppStyle::Get(), "NormalText.Important")
	            ]

	            + SHorizontalBox::Slot()
	            .AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(3.0f, 1.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						FText Message;
						GetWarningMessage(Message);
						return Message;
					})
					.TextStyle(FAppStyle::Get(), "NormalText.Subdued")
				]
            ]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(3)
			[
				SNew(SButton)
				.ToolTipText(LOCTEXT("DeleteOp", "Delete retarget op and remove from stack."))
				.OnClicked_Lambda([InOpStackWidget, InStackElement]() -> FReply
				{
					InOpStackWidget.Get()->DeleteRetargetOp(InStackElement);
					return FReply::Handled();
				})
				.Content()
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Delete"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
        ], OwnerTable);	
}

bool TLLRN_SRetargetOpStackItem::GetWarningMessage(FText& Message) const
{
	const UTLLRN_IKRetargetProcessor* Processor = OpStackWidget.Pin()->EditorController.Pin()->GetRetargetProcessor();
	if (!(Processor && Processor->IsInitialized()))
	{
		return false;
	}

	const TArray<TObjectPtr<UTLLRN_RetargetOpBase>>& RetargetOps = Processor->GetRetargetOps();
	const int32 OpIndex = StackElement.Pin()->IndexInStack;
	if (RetargetOps.IsValidIndex(OpIndex))
	{
		Message = RetargetOps[OpIndex]->WarningMessage();
		return true;
	}
	
	return false;
}

bool TLLRN_SRetargetOpStackItem::IsOpEnabled() const
{
	if (const UTLLRN_RetargetOpBase* Op = GetRetargetOp())
	{
		return Op->bIsEnabled;
	}
	
	return false;
}

UTLLRN_RetargetOpBase* TLLRN_SRetargetOpStackItem::GetRetargetOp() const
{
	if (!(StackElement.IsValid() && OpStackWidget.IsValid()))
	{
		return nullptr;
	}
	if (!OpStackWidget.Pin()->EditorController.IsValid())
	{
		return nullptr;
	}
	const int32 OpIndex = StackElement.Pin()->IndexInStack;
	return OpStackWidget.Pin()->EditorController.Pin()->AssetController->GetRetargetOpAtIndex(OpIndex);
}

void TLLRN_SRetargetOpStack::Construct(
	const FArguments& InArgs,
	const TWeakPtr<FTLLRN_IKRetargetEditorController>& InEditorController)
{
	EditorController = InEditorController;
	EditorController.Pin()->SetOpStackView(SharedThis(this));
	
	CommandList = MakeShared<FUICommandList>();

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.Padding(0.0f)
		[
			SNew(SBorder)
			.Padding(0.0f)
			.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
			.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Top)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Left)
					.FillWidth(1.f)
					.Padding(3.0f, 1.0f)
					[
						SNew(SPositiveActionButton)
						.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
						.Text(LOCTEXT("AddNewRetargetOpLabel", "Add New Retarget Op"))
						.ToolTipText(LOCTEXT("AddNewToolTip", "Add a new operation to run as part of the retargeter."))
						.IsEnabled(this, &TLLRN_SRetargetOpStack::IsAddOpEnabled)
						.OnGetMenuContent(this, &TLLRN_SRetargetOpStack::CreateAddNewMenuWidget)
					]
				]
			]
		]

		+SVerticalBox::Slot()
		.Padding(0.0f)
		[
			SAssignNew( ListView, SOpStackViewListType )
			.SelectionMode(ESelectionMode::Single)
			.IsEnabled(this, &TLLRN_SRetargetOpStack::IsAddOpEnabled)
			.ListItemsSource( &ListViewItems )
			.OnGenerateRow( this, &TLLRN_SRetargetOpStack::MakeListRowWidget )
			.OnSelectionChanged(this, &TLLRN_SRetargetOpStack::OnSelectionChanged)
		]
	];

	RefreshStackView();
}

int32 TLLRN_SRetargetOpStack::GetSelectedItemIndex() const
{
	if (!ListView.IsValid())
	{
		return INDEX_NONE;
	}
	
	const TArray<TSharedPtr<FRetargetOpStackElement>>& SelectedItems = ListView.Get()->GetSelectedItems();
	if (SelectedItems.IsEmpty())
	{
		return INDEX_NONE;
	}

	return SelectedItems[0]->IndexInStack;
}

TSharedRef<SWidget> TLLRN_SRetargetOpStack::CreateAddNewMenuWidget()
{
	constexpr bool bCloseMenuAfterSelection = true;
	FMenuBuilder MenuBuilder(bCloseMenuAfterSelection, nullptr);

	MenuBuilder.BeginSection("AddNewRetargetOp", LOCTEXT("AddOperations", "Add New Retarget Op"));

	// add menu option to create each retarget op type
	for(TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;
		if(Class->IsChildOf(UTLLRN_RetargetOpBase::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
		{
			const UTLLRN_RetargetOpBase* OpCDO = Cast<UTLLRN_RetargetOpBase>(Class->GetDefaultObject());
			FUIAction Action = FUIAction( FExecuteAction::CreateSP(this, &TLLRN_SRetargetOpStack::AddNewRetargetOp, Class));
			MenuBuilder.AddMenuEntry(FText::FromString(OpCDO->GetNiceName().ToString()), FText::GetEmpty(), FSlateIcon(), Action);
		}
	}

	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

bool TLLRN_SRetargetOpStack::IsAddOpEnabled() const
{
	if (!EditorController.IsValid())
	{
		return false; 
	}

	if (const UTLLRN_IKRetargetProcessor* Processor = EditorController.Pin()->GetRetargetProcessor())
	{
		return Processor->IsInitialized();
	}
	
	return false;
}

void TLLRN_SRetargetOpStack::AddNewRetargetOp(UClass* Class)
{
	if (!EditorController.IsValid())
	{
		return; 
	}

	const UTLLRN_IKRetargeterController* AssetController = EditorController.Pin()->AssetController;
	if (!AssetController)
	{
		return;
	}
	
	AssetController->AddRetargetOp(Class);
}

void TLLRN_SRetargetOpStack::DeleteRetargetOp(TSharedPtr<FRetargetOpStackElement> OpToDelete)
{
	const TSharedPtr<FTLLRN_IKRetargetEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}
	
	if (!OpToDelete.IsValid())
	{
		return;
	}
	
	Controller->AssetController->RemoveRetargetOp(OpToDelete->IndexInStack);
	RefreshStackView();
}

void TLLRN_SRetargetOpStack::OnSelectionChanged(
	TSharedPtr<FRetargetOpStackElement> InItem,
	ESelectInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::Direct && InItem.IsValid())
	{
		EditorController.Pin()->LastSelectedOpIndex = InItem->IndexInStack;
	}
}

TSharedRef<ITableRow> TLLRN_SRetargetOpStack::MakeListRowWidget(
	TSharedPtr<FRetargetOpStackElement> InElement,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return InElement->MakeListRowWidget(
		OwnerTable,
		InElement.ToSharedRef(),
		SharedThis(this));
}

void TLLRN_SRetargetOpStack::RefreshStackView()
{
	const TSharedPtr<FTLLRN_IKRetargetEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}

	// generate all list items
	ListViewItems.Reset();
	UTLLRN_IKRetargeterController* AssetController = Controller->AssetController;
	const int32 NumOps = AssetController->GetNumRetargetOps();
	for (int32 i=0; i<NumOps; ++i)
	{
		const UTLLRN_RetargetOpBase* Op = AssetController->GetRetargetOpAtIndex(i);
		FString UniqueName =  FString::FromInt(i+1) + " - " + Op->GetNiceName().ToString();
		TSharedPtr<FRetargetOpStackElement> StackElement = FRetargetOpStackElement::Make(FText::FromString(UniqueName), i);
		ListViewItems.Add(StackElement);
	}

	if (NumOps && ListViewItems.IsValidIndex(EditorController.Pin()->LastSelectedOpIndex))
	{
		// restore selection
		ListView->SetSelection(ListViewItems[EditorController.Pin()->LastSelectedOpIndex]);
	}
	else
	{
		// clear selection otherwise
		ListView->ClearSelection();
	}

	ListView->RequestListRefresh();
}

FReply TLLRN_SRetargetOpStack::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// TODO delete here
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

TSharedRef<FRetargetOpStackDragDropOp> FRetargetOpStackDragDropOp::New(TWeakPtr<FRetargetOpStackElement> InElement)
{
	TSharedRef<FRetargetOpStackDragDropOp> Operation = MakeShared<FRetargetOpStackDragDropOp>();
	Operation->Element = InElement;
	Operation->Construct();
	return Operation;
}

TSharedPtr<SWidget> FRetargetOpStackDragDropOp::GetDefaultDecorator() const
{
	return SNew(SBorder)
		.Visibility(EVisibility::Visible)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			SNew(STextBlock)
			.Text(Element.Pin()->DisplayName)
		];
}

FReply TLLRN_SRetargetOpStack::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const TArray<TSharedPtr<FRetargetOpStackElement>> SelectedItems = ListView.Get()->GetSelectedItems();
	if (SelectedItems.Num() != 1)
	{
		return FReply::Unhandled();
	}
	
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		const TSharedPtr<FRetargetOpStackElement> DraggedElement = SelectedItems[0];
		const TSharedRef<FRetargetOpStackDragDropOp> DragDropOp = FRetargetOpStackDragDropOp::New(DraggedElement);
		return FReply::Handled().BeginDragDrop(DragDropOp);
	}

	return FReply::Unhandled();
}

TOptional<EItemDropZone> TLLRN_SRetargetOpStack::OnCanAcceptDrop(
	const FDragDropEvent& DragDropEvent,
	EItemDropZone DropZone,
	TSharedPtr<FRetargetOpStackElement> TargetItem)
{
	TOptional<EItemDropZone> ReturnedDropZone;
	
	const TSharedPtr<FRetargetOpStackDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FRetargetOpStackDragDropOp>();
	if (DragDropOp.IsValid())
	{
		ReturnedDropZone = EItemDropZone::BelowItem;	
	}
	
	return ReturnedDropZone;
}

FReply TLLRN_SRetargetOpStack::OnAcceptDrop(
	const FDragDropEvent& DragDropEvent,
	EItemDropZone DropZone,
	TSharedPtr<FRetargetOpStackElement> TargetItem)
{
	const TSharedPtr<FRetargetOpStackDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FRetargetOpStackDragDropOp>();
	if (!DragDropOp.IsValid())
	{
		return FReply::Unhandled();
	}
	
	const TSharedPtr<FTLLRN_IKRetargetEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return FReply::Handled();
	}

	const FRetargetOpStackElement& DraggedElement = *DragDropOp.Get()->Element.Pin().Get();
	const UTLLRN_IKRetargeterController* AssetController = Controller->AssetController;
	const bool bWasReparented = AssetController->MoveRetargetOpInStack(DraggedElement.IndexInStack, TargetItem.Get()->IndexInStack);
	if (bWasReparented)
	{
		RefreshStackView();
	}
	
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
