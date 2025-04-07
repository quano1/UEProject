// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_SIKRigSolverStack.h"

#include "RigEditor/TLLRN_IKRigEditorStyle.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "RigEditor/TLLRN_IKRigEditorController.h"
#include "Rig/Solvers/TLLRN_IKRigSolver.h"

#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SKismetInspector.h"
#include "Dialogs/Dialogs.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SPositiveActionButton.h"
#include "Rig/TLLRN_IKRigProcessor.h"

#define LOCTEXT_NAMESPACE "TLLRN_SIKRigSolverStack"

TSharedRef<ITableRow> FTLLRN_SolverStackElement::MakeListRowWidget(
	const TSharedRef<STableViewBase>& InOwnerTable,
	TSharedRef<FTLLRN_SolverStackElement> InStackElement,
	TSharedPtr<TLLRN_SIKRigSolverStack> InSolverStack)
{
	return SNew(TLLRN_SIKRigSolverStackItem, InOwnerTable, InStackElement, InSolverStack);
}

void TLLRN_SIKRigSolverStackItem::Construct(
	const FArguments& InArgs,
	const TSharedRef<STableViewBase>& OwnerTable,
	TSharedRef<FTLLRN_SolverStackElement> InStackElement,
	TSharedPtr<TLLRN_SIKRigSolverStack> InSolverStack)
{
	StackElement = InStackElement;
	SolverStack = InSolverStack;
	
	STableRow<TSharedPtr<FTLLRN_SolverStackElement>>::Construct(
        STableRow<TSharedPtr<FTLLRN_SolverStackElement>>::FArguments()
        .OnDragDetected(InSolverStack.Get(), &TLLRN_SIKRigSolverStack::OnDragDetected)
        .OnCanAcceptDrop(InSolverStack.Get(), &TLLRN_SIKRigSolverStack::OnCanAcceptDrop)
        .OnAcceptDrop(InSolverStack.Get(), &TLLRN_SIKRigSolverStack::OnAcceptDrop)
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
					.IsEnabled_Lambda([this]()
					{
						FText Warning;
						return !GetWarningMessage(Warning);
					})
					.IsChecked_Lambda([InSolverStack, InStackElement]() -> ECheckBoxState
					{
						bool bEnabled = true;
						if (InSolverStack.IsValid() &&
							InSolverStack->EditorController.IsValid() &&
							InSolverStack->EditorController.Pin().IsValid())
						{
							bEnabled = InSolverStack->EditorController.Pin()->AssetController->GetSolverEnabled(InStackElement->IndexInStack);
						}
						return bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([InSolverStack, InStackElement](ECheckBoxState InCheckBoxState)
					{
						if (InSolverStack.IsValid() &&
							InSolverStack->EditorController.IsValid() &&
							InSolverStack->EditorController.Pin().IsValid())
						{
							bool bIsChecked = InCheckBoxState == ECheckBoxState::Checked;
							InSolverStack->EditorController.Pin()->AssetController->SetSolverEnabled(InStackElement->IndexInStack, bIsChecked);
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
						if (!IsSolverEnabled())
						{
							return false;
						}
						FText Warning;
						return !GetWarningMessage(Warning);
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
					.Text(InStackElement->DisplayName)
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
				.ToolTipText(LOCTEXT("DeleteSolver", "Delete solver and remove from stack."))
				.OnClicked_Lambda([InSolverStack, InStackElement]() -> FReply
				{
					InSolverStack.Get()->DeleteSolver(InStackElement);
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

bool TLLRN_SIKRigSolverStackItem::GetWarningMessage(FText& Message) const
{
	constexpr bool bFromAsset = false;
	if (const UTLLRN_IKRigSolver* Solver = GetSolver(bFromAsset))
	{
		return Solver->GetWarningMessage(Message);
	}
	
	return false;
}

bool TLLRN_SIKRigSolverStackItem::IsSolverEnabled() const
{
	constexpr bool bFromAsset = true;
	if (const UTLLRN_IKRigSolver* Solver = GetSolver(bFromAsset))
	{
		return Solver->IsEnabled();
	}
	
	return false;
}

UTLLRN_IKRigSolver* TLLRN_SIKRigSolverStackItem::GetSolver(const bool bFromAsset) const
{
	if (!(StackElement.IsValid() && SolverStack.IsValid()))
	{
		return nullptr;
	}
	if (!SolverStack.Pin()->EditorController.IsValid())
	{
		return nullptr;
	}
	
	const int32 SolverIndex = StackElement.Pin()->IndexInStack;
	const TSharedPtr<FTLLRN_IKRigEditorController> EditorController = SolverStack.Pin()->EditorController.Pin();
	
	if (bFromAsset)
	{
		// get the solver stored in the asset
		return EditorController->AssetController->GetSolverAtIndex(SolverIndex);
	}
	else
	{
		// get the currently running solver instance in the editor's runtime processor
		// this is needed to report solver warnings dependent on being initialized
		if (UTLLRN_IKRigProcessor* Processor = EditorController->GetTLLRN_IKRigProcessor())
		{
			if (Processor->GetSolvers().IsValidIndex(SolverIndex))
			{
				return Processor->GetSolvers()[SolverIndex];
			}
		}
	}

	return nullptr;
}

TSharedRef<FTLLRN_IKRigSolverStackDragDropOp> FTLLRN_IKRigSolverStackDragDropOp::New(TWeakPtr<FTLLRN_SolverStackElement> InElement)
{
	TSharedRef<FTLLRN_IKRigSolverStackDragDropOp> Operation = MakeShared<FTLLRN_IKRigSolverStackDragDropOp>();
	Operation->Element = InElement;
	Operation->Construct();
	return Operation;
}

TSharedPtr<SWidget> FTLLRN_IKRigSolverStackDragDropOp::GetDefaultDecorator() const
{
	return SNew(SBorder)
        .Visibility(EVisibility::Visible)
        .BorderImage(FAppStyle::GetBrush("Menu.Background"))
        [
            SNew(STextBlock)
            .Text(Element.Pin()->DisplayName)
        ];
}

TLLRN_SIKRigSolverStack::~TLLRN_SIKRigSolverStack()
{
	
}

void TLLRN_SIKRigSolverStack::Construct(const FArguments& InArgs, TSharedRef<FTLLRN_IKRigEditorController> InEditorController)
{
	EditorController = InEditorController;
	EditorController.Pin()->SetSolverStackView(SharedThis(this));
	
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
				        .Text(LOCTEXT("AddNewSolverLabel", "Add New Solver"))
				        .ToolTipText(LOCTEXT("AddNewToolTip", "Add a new IK solver to the rig."))
				        .IsEnabled(this, &TLLRN_SIKRigSolverStack::IsAddSolverEnabled)
				        .OnGetMenuContent(this, &TLLRN_SIKRigSolverStack::CreateAddNewMenuWidget)
                    ]
                ]
            ]
        ]

        +SVerticalBox::Slot()
        .Padding(0.0f)
        [
			SAssignNew( ListView, SSolverStackListViewType )
			.SelectionMode(ESelectionMode::Single)
			.IsEnabled(this, &TLLRN_SIKRigSolverStack::IsAddSolverEnabled)
			.ListItemsSource( &ListViewItems )
			.OnGenerateRow( this, &TLLRN_SIKRigSolverStack::MakeListRowWidget )
			.OnSelectionChanged(this, &TLLRN_SIKRigSolverStack::OnSelectionChanged)
			.OnMouseButtonClick(this, &TLLRN_SIKRigSolverStack::OnItemClicked)
        ]
    ];

	RefreshStackView();
}

TSharedRef<SWidget> TLLRN_SIKRigSolverStack::CreateAddNewMenuWidget()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, CommandList);

	BuildAddNewMenu(MenuBuilder);

	return MenuBuilder.MakeWidget();
}

void TLLRN_SIKRigSolverStack::BuildAddNewMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("AddNewSolver", LOCTEXT("AddOperations", "Add New Solver"));
	
	// add menu option to create each solver type
	TArray<UClass*> SolverClasses;
	for(TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;

		if(Class->IsChildOf(UTLLRN_IKRigSolver::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
		{
			UTLLRN_IKRigSolver* SolverCDO = Cast<UTLLRN_IKRigSolver>(Class->GetDefaultObject());
			FUIAction Action = FUIAction( FExecuteAction::CreateSP(this, &TLLRN_SIKRigSolverStack::AddNewSolver, Class));
			MenuBuilder.AddMenuEntry(FText::FromString(SolverCDO->GetNiceName().ToString()), FText::GetEmpty(), FSlateIcon(), Action);
		}
	}
	
	MenuBuilder.EndSection();
}

bool TLLRN_SIKRigSolverStack::IsAddSolverEnabled() const
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return false; 
	}
	
	if (UTLLRN_IKRigController* AssetController = Controller->AssetController)
	{
		if (AssetController->GetTLLRN_IKRigSkeleton().BoneNames.Num() > 0)
		{
			return true;
		}
	}
	
	return false;
}

void TLLRN_SIKRigSolverStack::AddNewSolver(UClass* Class)
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}

	UTLLRN_IKRigController* AssetController = Controller->AssetController;
	if (!AssetController)
	{
		return;
	}
	
	// add the solver
	const int32 NewSolverIndex = AssetController->AddSolver(Class);
	// update stack view
	RefreshStackView();
	Controller->RefreshTreeView(); // updates solver indices in tree items
	// select it
	ListView->SetSelection(ListViewItems[NewSolverIndex]);
	// show details for it
	Controller->ShowDetailsForSolver(NewSolverIndex);
}

void TLLRN_SIKRigSolverStack::DeleteSolver(TSharedPtr<FTLLRN_SolverStackElement> SolverToDelete)
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}
	
	if (!SolverToDelete.IsValid())
	{
		return;
	}

	const UTLLRN_IKRigController* AssetController = Controller->AssetController;
	AssetController->RemoveSolver(SolverToDelete->IndexInStack);
	RefreshStackView();
	Controller->RefreshTreeView();
}

void TLLRN_SIKRigSolverStack::RefreshStackView()
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}

	// record/restore selection
	int32 IndexToSelect = 0; // default to first solver selected
	const TArray<TSharedPtr<FTLLRN_SolverStackElement>> SelectedItems = ListView.Get()->GetSelectedItems();
	if (!SelectedItems.IsEmpty())
	{
		IndexToSelect = SelectedItems[0]->IndexInStack;
	}

	// generate all list items
	static const FText UnknownSolverTxt = FText::FromString("Unknown Solver");
	
	ListViewItems.Reset();
	UTLLRN_IKRigController* AssetController = Controller->AssetController;
	const int32 NumSolvers = AssetController->GetNumSolvers();
	for (int32 i=0; i<NumSolvers; ++i)
	{
		const UTLLRN_IKRigSolver* Solver = AssetController->GetSolverAtIndex(i);
		const FText DisplayName = Solver ? FText::FromString(AssetController->GetSolverUniqueName(i)) : UnknownSolverTxt;
		TSharedPtr<FTLLRN_SolverStackElement> SolverItem = FTLLRN_SolverStackElement::Make(DisplayName, i);
		ListViewItems.Add(SolverItem);
	}

	if (NumSolvers && ListViewItems.IsValidIndex(IndexToSelect))
	{
		// restore selection
		ListView->SetSelection(ListViewItems[IndexToSelect]);
	}
	else
	{
		// clear selection otherwise
		ListView->ClearSelection();
	}

	ListView->RequestListRefresh();
}

TSharedRef<ITableRow> TLLRN_SIKRigSolverStack::MakeListRowWidget(
	TSharedPtr<FTLLRN_SolverStackElement> InElement,
    const TSharedRef<STableViewBase>& OwnerTable)
{
	return InElement->MakeListRowWidget(
		OwnerTable,
		InElement.ToSharedRef(),
		SharedThis(this));
}

FReply TLLRN_SIKRigSolverStack::OnDragDetected(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	const TArray<TSharedPtr<FTLLRN_SolverStackElement>> SelectedItems = ListView.Get()->GetSelectedItems();
	if (SelectedItems.Num() != 1)
	{
		return FReply::Unhandled();
	}
	
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		const TSharedPtr<FTLLRN_SolverStackElement> DraggedElement = SelectedItems[0];
		const TSharedRef<FTLLRN_IKRigSolverStackDragDropOp> DragDropOp = FTLLRN_IKRigSolverStackDragDropOp::New(DraggedElement);
		return FReply::Handled().BeginDragDrop(DragDropOp);
	}

	return FReply::Unhandled();
}

void TLLRN_SIKRigSolverStack::OnSelectionChanged(TSharedPtr<FTLLRN_SolverStackElement> InItem, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		ShowDetailsForItem(InItem);	
	}
}

void TLLRN_SIKRigSolverStack::OnItemClicked(TSharedPtr<FTLLRN_SolverStackElement> InItem)
{
	ShowDetailsForItem(InItem);
	EditorController.Pin()->SetLastSelectedType(EIKRigSelectionType::SolverStack);
}

void TLLRN_SIKRigSolverStack::ShowDetailsForItem(TSharedPtr<FTLLRN_SolverStackElement> InItem)
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}

	// update bones greyed out when not affected
	Controller->RefreshTreeView();

	// the solver selection must be done after we rebuilt the skeleton tree has it keeps the selection
	if (!InItem.IsValid())
	{
		// clear the details panel only if there's nothing selected in the skeleton view
		if (Controller->DoesSkeletonHaveSelectedItems())
		{
			return;
		}
		
		Controller->ShowAssetDetails();
	}
	else
	{
		Controller->ShowDetailsForSolver(InItem.Get()->IndexInStack);
	}
}

TOptional<EItemDropZone> TLLRN_SIKRigSolverStack::OnCanAcceptDrop(
	const FDragDropEvent& DragDropEvent,
	EItemDropZone DropZone,
	TSharedPtr<FTLLRN_SolverStackElement> TargetItem)
{
	TOptional<EItemDropZone> ReturnedDropZone;
	
	const TSharedPtr<FTLLRN_IKRigSolverStackDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FTLLRN_IKRigSolverStackDragDropOp>();
	if (DragDropOp.IsValid())
	{
		ReturnedDropZone = DropZone == EItemDropZone::BelowItem ? EItemDropZone::BelowItem : EItemDropZone::AboveItem;
	}
	
	return ReturnedDropZone;
}

FReply TLLRN_SIKRigSolverStack::OnAcceptDrop(
	const FDragDropEvent& DragDropEvent,
	EItemDropZone DropZone,
	TSharedPtr<FTLLRN_SolverStackElement> TargetItem)
{
	const TSharedPtr<FTLLRN_IKRigSolverStackDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FTLLRN_IKRigSolverStackDragDropOp>();
	if (!DragDropOp.IsValid())
	{
		return FReply::Unhandled();
	}
	
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return FReply::Handled();
	}

	const FTLLRN_SolverStackElement& DraggedElement = *DragDropOp.Get()->Element.Pin().Get();
	if (DraggedElement.IndexInStack == TargetItem->IndexInStack)
	{
		return FReply::Handled();
	}
	
	UTLLRN_IKRigController* AssetController = Controller->AssetController;
	int32 TargetIndex = TargetItem.Get()->IndexInStack;
	if ( DropZone == EItemDropZone::AboveItem)
	{
		TargetIndex = FMath::Max(0, TargetIndex-1);
	}
	const bool bWasMoved = AssetController->MoveSolverInStack(DraggedElement.IndexInStack, TargetIndex);
	if (bWasMoved)
	{
		RefreshStackView();
		Controller->RefreshTreeView(); // update solver indices in effector items
	}
	
	return FReply::Handled();
}

FReply TLLRN_SIKRigSolverStack::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{	
	// handle deleting selected solver
	FKey KeyPressed = InKeyEvent.GetKey();
	if (KeyPressed == EKeys::Delete || KeyPressed == EKeys::BackSpace)
	{
		TArray<TSharedPtr<FTLLRN_SolverStackElement>> SelectedItems = ListView->GetSelectedItems();
		if (!SelectedItems.IsEmpty())
		{
			// only delete 1 at a time to avoid messing up indices
			DeleteSolver(SelectedItems[0]);
		}
		
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
