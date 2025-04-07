// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "TLLRN_IKRetargetEditorController.h"
#include "TLLRN_SBaseHierarchyTreeView.h"
#include "TLLRN_SIKRetargetPoseEditor.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/SCompoundWidget.h"

class FTextFilterExpressionEvaluator;
class TLLRN_SIKRetargetPoseEditor;

class FIKRetargetHierarchyElement : public TSharedFromThis<FIKRetargetHierarchyElement>
{
public:
	
	FIKRetargetHierarchyElement(
		const FName& InName,
		const FName& InChainName,
		const TSharedRef<FTLLRN_IKRetargetEditorController>& InEditorController);

	FText Key;
	TSharedPtr<FIKRetargetHierarchyElement> Parent;
	TArray<TSharedPtr<FIKRetargetHierarchyElement>> Children;
	FName Name;
	FName ChainName;
	bool bIsHidden = false;

private:
	
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
};

class TLLRN_SIKRetargetHierarchyRow : public SMultiColumnTableRow<TSharedPtr<FIKRetargetHierarchyElement>>
{
	
public:

	SLATE_BEGIN_ARGS(TLLRN_SIKRetargetHierarchyRow) {}
	SLATE_ARGUMENT(TSharedPtr<FTLLRN_IKRetargetEditorController>, EditorController)
	SLATE_ARGUMENT(TSharedPtr<FIKRetargetHierarchyElement>, TreeElement)
	SLATE_END_ARGS()
	
	void Construct(
		const FArguments& InArgs,
		const TSharedRef<STableViewBase>& InOwnerTable)
	{
		OwnerTable = InOwnerTable;
		WeakTreeElement = InArgs._TreeElement;
		EditorController = InArgs._EditorController;
		
		SMultiColumnTableRow<TSharedPtr<FIKRetargetHierarchyElement>>::Construct(FSuperRowType::FArguments(), InOwnerTable);
	}

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the list view. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:

	TSharedPtr<STableViewBase> OwnerTable;
	TWeakPtr<FIKRetargetHierarchyElement> WeakTreeElement;
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
};

struct FTLLRN_RetargetHierarchyFilterOptions
{
	bool bHideBonesNotInChain;
	bool bHideNotRetargetedBones;
	bool bHideRetargetedBones;
};

typedef TLLRN_SBaseHierarchyTreeView<FIKRetargetHierarchyElement> TLLRN_SIKRetargetHierarchyTreeView;

class TLLRN_SIKRetargetHierarchy : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(TLLRN_SIKRetargetHierarchy) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTLLRN_IKRetargetEditorController> InEditorController);

	void ShowItemAfterSelection(FName ItemKey);

	void RefreshPoseList() const { EditPoseView.Get()->Refresh(); };

private:
	
	/** centralized editor controls (facilitate cross-communication between multiple UI elements)*/
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
	
	/** command list we bind to */
	TSharedPtr<FUICommandList> CommandList;

	/** edit pose widget */
	TSharedPtr<TLLRN_SIKRetargetPoseEditor> EditPoseView;
	
	/** tree view widget */
	TSharedPtr<TLLRN_SIKRetargetHierarchyTreeView> TreeView;
	TArray<TSharedPtr<FIKRetargetHierarchyElement>> RootElements;
	TArray<TSharedPtr<FIKRetargetHierarchyElement>> AllElements;

	/** filtering the tree with search box */
	TSharedRef<SWidget> CreateFilterMenuWidget();
	void OnFilterTextChanged(const FText& SearchText);
	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;
	FTLLRN_RetargetHierarchyFilterOptions FilterOptions;
	
	/** tree view callbacks */
	void RefreshTreeView(bool IsInitialSetup=false);
	void HandleGetChildrenForTree(TSharedPtr<FIKRetargetHierarchyElement> InElement, TArray<TSharedPtr<FIKRetargetHierarchyElement>>& OutChildren);
	void OnSelectionChanged(TSharedPtr<FIKRetargetHierarchyElement> Selection, ESelectInfo::Type SelectInfo);
	void OnItemDoubleClicked(TSharedPtr<FIKRetargetHierarchyElement> InElement);
	void OnSetExpansionRecursive(TSharedPtr<FIKRetargetHierarchyElement> InElement, bool bShouldBeExpanded);
	void SetExpansionRecursive(TSharedPtr<FIKRetargetHierarchyElement> InElement, bool bTowardsParent, bool bShouldBeExpanded);
	/** END tree view callbacks */

	friend TLLRN_SIKRetargetHierarchyRow;
	friend FTLLRN_IKRetargetEditorController;
};
