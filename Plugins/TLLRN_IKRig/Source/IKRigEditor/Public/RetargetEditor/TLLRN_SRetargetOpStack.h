// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "Framework/Commands/UICommandList.h"

class UTLLRN_RetargetOpBase;
class FTLLRN_IKRetargetEditorController;
class TLLRN_SRetargetOpStack;

class FRetargetOpStackElement
{
	
public:

	TSharedRef<ITableRow> MakeListRowWidget(
		const TSharedRef<STableViewBase>& InOwnerTable,
        TSharedRef<FRetargetOpStackElement> InStackElement,
        TSharedPtr<TLLRN_SRetargetOpStack> InSolverStack);

	static TSharedRef<FRetargetOpStackElement> Make(FText DisplayName, int32 SolverIndex)
	{
		return MakeShareable(new FRetargetOpStackElement(DisplayName, SolverIndex));
	}
	
	FText DisplayName;
	int32 IndexInStack;

private:
	// hidden constructors, always use Make above
	FRetargetOpStackElement() {}
	FRetargetOpStackElement(FText InDisplayName, int32 StackIndex) :
		DisplayName(InDisplayName), IndexInStack(StackIndex){}
};

class TLLRN_SRetargetOpStackItem : public STableRow<TSharedPtr<FRetargetOpStackElement>>
{
public:
	
	void Construct(
        const FArguments& InArgs,
        const TSharedRef<STableViewBase>& OwnerTable,
        TSharedRef<FRetargetOpStackElement> InStackElement,
        TSharedPtr<TLLRN_SRetargetOpStack> InOpStackWidget);

	bool GetWarningMessage(FText& Message) const;

	bool IsOpEnabled() const;

private:
	UTLLRN_RetargetOpBase* GetRetargetOp() const;
	TWeakPtr<FRetargetOpStackElement> StackElement;
	TWeakPtr<TLLRN_SRetargetOpStack> OpStackWidget;
};

class FRetargetOpStackDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FRetargetOpStackDragDropOp, FDecoratedDragDropOp)
    static TSharedRef<FRetargetOpStackDragDropOp> New(TWeakPtr<FRetargetOpStackElement> InElement);
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
	TWeakPtr<FRetargetOpStackElement> Element;
};

typedef SListView< TSharedPtr<FRetargetOpStackElement> > SOpStackViewListType;
class TLLRN_SRetargetOpStack : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(TLLRN_SRetargetOpStack) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TWeakPtr<FTLLRN_IKRetargetEditorController>& InEditorController);

	int32 GetSelectedItemIndex() const;

private:
	// menu for adding new commands
	TSharedPtr<FUICommandList> CommandList;
	
	// editor controller
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
	
	// the op stack list view
	TSharedPtr<SOpStackViewListType> ListView;
	TArray< TSharedPtr<FRetargetOpStackElement> > ListViewItems;

	// add new op menu
	TSharedRef<SWidget> CreateAddNewMenuWidget();
	bool IsAddOpEnabled() const;
	// END op stack menu
	
	// menu command callback for adding a new op
	void AddNewRetargetOp(UClass* Class);
	// delete op from stack
	void DeleteRetargetOp(TSharedPtr<FRetargetOpStackElement> OpToDelete);
	// when an op is selected on in the stack view
	void OnSelectionChanged(TSharedPtr<FRetargetOpStackElement> InItem, ESelectInfo::Type SelectInfo);

	// list view generate row callback
	TSharedRef<ITableRow> MakeListRowWidget(TSharedPtr<FRetargetOpStackElement> InElement, const TSharedRef<STableViewBase>& OwnerTable);

	// call to refresh the stack view
	void RefreshStackView();

	// SWidget interface
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	// END SWidget interface
	
	// drag and drop operations
    FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
    TOptional<EItemDropZone> OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FRetargetOpStackElement> TargetItem);
    FReply OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FRetargetOpStackElement> TargetItem);
	// END drag and drop operations
	
	friend TLLRN_SRetargetOpStackItem;
	friend FTLLRN_IKRetargetEditorController;
};
