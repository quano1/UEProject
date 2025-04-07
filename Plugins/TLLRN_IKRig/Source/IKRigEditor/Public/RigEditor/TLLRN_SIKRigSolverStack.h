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

class UTLLRN_IKRigSolver;
class FTLLRN_IKRigEditorController;
class TLLRN_SIKRigSolverStack;
class FTLLRN_IKRigEditorToolkit;


class FTLLRN_SolverStackElement
{
public:

	TSharedRef<ITableRow> MakeListRowWidget(
		const TSharedRef<STableViewBase>& InOwnerTable,
        TSharedRef<FTLLRN_SolverStackElement> InStackElement,
        TSharedPtr<TLLRN_SIKRigSolverStack> InSolverStack);

	static TSharedRef<FTLLRN_SolverStackElement> Make(FText DisplayName, int32 SolverIndex)
	{
		return MakeShareable(new FTLLRN_SolverStackElement(DisplayName, SolverIndex));
	}
	
	FText DisplayName;
	int32 IndexInStack;

private:
	/** Hidden constructor, always use Make above */
	FTLLRN_SolverStackElement(FText InDisplayName, int32 SolverIndex)
        : DisplayName(InDisplayName), IndexInStack(SolverIndex)
	{
	}

	/** Hidden constructor, always use Make above */
	FTLLRN_SolverStackElement() {}
};

class TLLRN_SIKRigSolverStackItem : public STableRow<TSharedPtr<FTLLRN_SolverStackElement>>
{
public:
	
	void Construct(
        const FArguments& InArgs,
        const TSharedRef<STableViewBase>& OwnerTable,
        TSharedRef<FTLLRN_SolverStackElement> InStackElement,
        TSharedPtr<TLLRN_SIKRigSolverStack> InSolverStack);

	bool GetWarningMessage(FText& Message) const;

	bool IsSolverEnabled() const;

private:
	UTLLRN_IKRigSolver* GetSolver(const bool bFromAsset) const;
	TWeakPtr<FTLLRN_SolverStackElement> StackElement;
	TWeakPtr<TLLRN_SIKRigSolverStack> SolverStack;
};

class FTLLRN_IKRigSolverStackDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FTLLRN_IKRigSolverStackDragDropOp, FDecoratedDragDropOp)
    static TSharedRef<FTLLRN_IKRigSolverStackDragDropOp> New(TWeakPtr<FTLLRN_SolverStackElement> InElement);
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
	TWeakPtr<FTLLRN_SolverStackElement> Element;
};


typedef SListView< TSharedPtr<FTLLRN_SolverStackElement> > SSolverStackListViewType;

class TLLRN_SIKRigSolverStack : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(TLLRN_SIKRigSolverStack) {}
	SLATE_END_ARGS()

    ~TLLRN_SIKRigSolverStack();

	void Construct(const FArguments& InArgs, TSharedRef<FTLLRN_IKRigEditorController> InEditorController);

private:
	/** menu for adding new solver commands */
	TSharedPtr<FUICommandList> CommandList;
	
	/** editor controller */
	TWeakPtr<FTLLRN_IKRigEditorController> EditorController;
	
	/** the solver stack list view */
	TSharedPtr<SSolverStackListViewType> ListView;
	TArray< TSharedPtr<FTLLRN_SolverStackElement> > ListViewItems;

	/** solver stack menu */
	TSharedRef<SWidget> CreateAddNewMenuWidget();
	void BuildAddNewMenu(FMenuBuilder& MenuBuilder);
	bool IsAddSolverEnabled() const;
	/** END solver stack menu */
	
	/** menu command callback for adding a new solver */
	void AddNewSolver(UClass* Class);
	/** delete solver from stack */
	void DeleteSolver(TSharedPtr<FTLLRN_SolverStackElement> SolverToDelete);
	/** when a solver is selected on in the stack view */
	void OnSelectionChanged(TSharedPtr<FTLLRN_SolverStackElement> InItem, ESelectInfo::Type SelectInfo);
	/** when a solver is clicked on in the stack view */
	void OnItemClicked(TSharedPtr<FTLLRN_SolverStackElement> InItem);
	void ShowDetailsForItem(TSharedPtr<FTLLRN_SolverStackElement> InItem); 

	/** list view generate row callback */
	TSharedRef<ITableRow> MakeListRowWidget(TSharedPtr<FTLLRN_SolverStackElement> InElement, const TSharedRef<STableViewBase>& OwnerTable);

	/** call to refresh the stack view */
	void RefreshStackView();

	/** SWidget interface */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	/** END SWidget interface */
	
	/** drag and drop operations */
    FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
    TOptional<EItemDropZone> OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FTLLRN_SolverStackElement> TargetItem);
    FReply OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FTLLRN_SolverStackElement> TargetItem);
	/** END drag and drop operations */

	friend TLLRN_SIKRigSolverStackItem;
	friend FTLLRN_IKRigEditorController;
};
