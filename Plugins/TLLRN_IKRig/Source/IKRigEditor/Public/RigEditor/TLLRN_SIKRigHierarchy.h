// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "TLLRN_SBaseHierarchyTreeView.h"
#include "TLLRN_SIKRigRetargetChainList.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

#include "TLLRN_SIKRigSolverStack.h"

class FTLLRN_IKRigEditorController;
class TLLRN_SIKRigHierarchy;
class FTLLRN_IKRigEditorToolkit;
class USkeletalMesh;
class UTLLRN_IKRigBoneDetails;

enum class IKRigTreeElementType { BONE, GOAL, SOLVERGOAL, BONE_SETTINGS };

class FIKRigTreeElement : public TSharedFromThis<FIKRigTreeElement>
{
public:
	
	FIKRigTreeElement(
		const FText& InKey,
		IKRigTreeElementType InType,
		const TSharedRef<FTLLRN_IKRigEditorController>& InEditorController);

	FText Key;
	IKRigTreeElementType ElementType;

	/** determines if this item is filtered out of the view */
	bool bIsHidden = false;
	TSharedPtr<FIKRigTreeElement> UnFilteredParent;
	
	TSharedPtr<FIKRigTreeElement> Parent;
	TArray<TSharedPtr<FIKRigTreeElement>> Children;

	/** effector meta-data (if it is an effector) */
	FName EffectorGoalName = NAME_None;
	int32 EffectorIndex = INDEX_NONE;

	/** bone setting meta-data (if it is a bone setting) */
	FName BoneSettingBoneName = NAME_None;
	int32 BoneSettingsSolverIndex = INDEX_NONE;

	/** name of bone if it is one */
	FName BoneName = NAME_None;
	
	/** name of goal if it is one */
	FName GoalName = NAME_None;

	/** delegate for when the context menu requests a rename */
	void RequestRename();
	DECLARE_DELEGATE(FOnRenameRequested);
	FOnRenameRequested OnRenameRequested;

	/** get the underlying object */
	TWeakObjectPtr< UObject > GetObject() const;

	/** get the name of the retarget chain this element belongs to (or NAME_None if not in a chain) */
	FName GetChainName() const;

private:
	/** centralized editor controls */
	TWeakPtr<FTLLRN_IKRigEditorController> EditorController;

	/** on demand bone details object */	
	mutable TObjectPtr<UTLLRN_IKRigBoneDetails> OptionalBoneDetails = nullptr;
};

class TLLRN_SIKRigHierarchyItem : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(TLLRN_SIKRigHierarchyItem) {}
	SLATE_ARGUMENT(TWeakPtr<FTLLRN_IKRigEditorController>, EditorController)
	SLATE_ARGUMENT(TWeakPtr<FIKRigTreeElement>, TreeElement)
	SLATE_ARGUMENT(TWeakPtr<TLLRN_SIKRigHierarchy>, HierarchyView)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
private:

	bool OnVerifyNameChanged(const FText& InText, FText& OutErrorMessage) const;
	void OnNameCommitted(const FText& InText, ETextCommit::Type InCommitType) const;
	FText GetName() const;
	
	TWeakPtr<FIKRigTreeElement> WeakTreeElement;
	TWeakPtr<FTLLRN_IKRigEditorController> EditorController;
	TWeakPtr<TLLRN_SIKRigHierarchy> HierarchyView;
};

class FTLLRN_IKRigSkeletonDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FTLLRN_IKRigSkeletonDragDropOp, FDecoratedDragDropOp)
	static TSharedRef<FTLLRN_IKRigSkeletonDragDropOp> New(TWeakPtr<FIKRigTreeElement> InElement);
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
	TWeakPtr<FIKRigTreeElement> Element;
};

class TLLRN_SIKRigSkeletonRow : public SMultiColumnTableRow<TSharedPtr<FIKRigTreeElement>>
{
public:

	SLATE_BEGIN_ARGS(TLLRN_SIKRigSkeletonRow) {}
	SLATE_ARGUMENT(TSharedPtr<FTLLRN_IKRigEditorController>, EditorController)
	SLATE_ARGUMENT(TSharedPtr<FIKRigTreeElement>, TreeElement)
	SLATE_ARGUMENT(TSharedPtr<TLLRN_SIKRigHierarchy>, HierarchyView)
	SLATE_END_ARGS()

	void Construct(
		const FArguments& InArgs,
		const TSharedRef<STableViewBase>& InOwnerTable)
	{
		OwnerTable = InOwnerTable;
		WeakTreeElement = InArgs._TreeElement;
		EditorController = InArgs._EditorController;
		HierarchyView = InArgs._HierarchyView;
		
		SMultiColumnTableRow<TSharedPtr<FIKRigTreeElement>>::Construct(
			FSuperRowType::FArguments()
			.OnDragDetected(this, &TLLRN_SIKRigSkeletonRow::HandleDragDetected)
			.OnCanAcceptDrop(this, &TLLRN_SIKRigSkeletonRow::HandleCanAcceptDrop)
			.OnAcceptDrop(this, &TLLRN_SIKRigSkeletonRow::HandleAcceptDrop)
			,InOwnerTable);
	}

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the list view. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	/** End SMultiColumnTableRow */

	/** drag and drop */
	FReply HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	TOptional<EItemDropZone> HandleCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FIKRigTreeElement> TargetItem);
	FReply HandleAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FIKRigTreeElement> TargetItem);
	/** END drag and drop */

private:

	TSharedPtr<STableViewBase> OwnerTable;
	TWeakPtr<FIKRigTreeElement> WeakTreeElement;
	TWeakPtr<FTLLRN_IKRigEditorController> EditorController;
	TWeakPtr<TLLRN_SIKRigHierarchy> HierarchyView;
};

struct FTLLRN_IKRigHierarchyFilterOptions
{
	bool bShowAll = true;
	bool bShowOnlyBones = false;
	bool bShowOnlyGoals = false;
	bool bShowOnlyBoneSettings = false;
	bool bHideUnaffectedBones = false;

	void ResetShowOptions()
	{
		bShowAll = false;
		bShowOnlyBones = false;
		bShowOnlyGoals = false;
		bShowOnlyBoneSettings = false;
	}
};

typedef TLLRN_SBaseHierarchyTreeView<FIKRigTreeElement> TLLRN_SIKRigSkeletonTreeView;

class TLLRN_SIKRigHierarchy : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(TLLRN_SIKRigHierarchy) {}
	SLATE_END_ARGS()

    ~TLLRN_SIKRigHierarchy() {};

	void Construct(const FArguments& InArgs, TSharedRef<FTLLRN_IKRigEditorController> InEditorController);

	/** selection state queries */
	static bool IsBoneInSelection(TArray<TSharedPtr<FIKRigTreeElement>>& SelectedBoneItems, const FName& BoneName);
	void GetSelectedBones(TArray<TSharedPtr<FIKRigTreeElement>>& OutBoneItems) const;
	void GetSelectedBoneNames(TArray<FName>& OutSelectedBoneNames) const;
	void GetSelectedGoals(TArray<TSharedPtr<FIKRigTreeElement>>& OutSelectedGoals) const;
	int32 GetNumSelectedGoals();
	void GetSelectedGoalNames(TArray<FName>& OutSelectedGoalNames) const;
	bool IsGoalSelected(const FName& GoalName);
	void AddSelectedItemFromViewport(
		const FName& ItemName,
		IKRigTreeElementType ItemType,
		const bool bReplace);
	void GetSelectedBoneChains(TArray<FTLLRN_BoneChain>& OutChains) const;
	TArray<TSharedPtr<FIKRigTreeElement>> GetSelectedItems() const;
	bool HasSelectedItems() const;
	/** END selection state queries */

	/** determine if the element is connected to the selected solver */
	bool IsElementConnectedToSolver(TWeakPtr<FIKRigTreeElement> TreeElement, int32 SolverIndex);
	/** determine if the element is connected to ANY solver */
	bool IsElementConnectedToAnySolver(TWeakPtr<FIKRigTreeElement> TreeElement);
	/** determine if the element is an excluded bone*/
	bool IsElementExcludedBone(TWeakPtr<FIKRigTreeElement> TreeElement);

private:
	/** SWidget interface */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	/** END SWidget interface */
	
	/** Bind commands that this widget handles */
	void BindCommands();

	/** directly, programmatically add item to selection */
	void AddItemToSelection(const TSharedPtr<FIKRigTreeElement>& InItem);
	/** directly, programmatically add item to selection */
	void RemoveItemFromSelection(const TSharedPtr<FIKRigTreeElement>& InItem);
	/** directly, programmatically replace item in selection */
	void ReplaceItemInSelection(const FText& OldName, const FText& NewName);

	/** creating / renaming / deleting goals */
	void HandleNewGoal() const;
	bool CanAddNewGoal() const;
	void HandleRenameGoal() const;
	bool CanRenameGoal() const;
	void HandleDeleteElements();
	bool CanDeleteElement() const;
	/** END creating / renaming / deleting goals */

	/** connecting/disconnecting goals to solvers */
	void HandleConnectGoalToSolver();
	void HandleDisconnectGoalFromSolver();
	bool CanConnectGoalToSolvers() const;
	bool CanDisconnectGoalFromSolvers() const;
	void ConnectSelectedGoalsToSelectedSolvers(bool bConnect);
	int32 GetNumSelectedGoalToSolverConnections(bool bCountOnlyConnected) const;
	/** END connecting/disconnecting goals to solvers */

	/** setting root bone */
	void HandleSetRootBoneOnSolvers();
	bool CanSetRootBoneOnSolvers();
	/** END setting root bone */

	/** setting end bone */
	void HandleSetEndBoneOnSolvers();
	bool CanSetEndBoneOnSolvers() const;
	bool HasEndBoneCompatibleSolverSelected() const;
	/** END setting end bone */

	/** per-bone settings */
	void HandleAddBoneSettings();
	bool CanAddBoneSettings();
	void HandleRemoveBoneSettings();
	bool CanRemoveBoneSettings();
	/** END per-bone settings */
	
	/** exclude/include bones */
	void HandleExcludeBone();
	bool CanExcludeBone();
	void HandleIncludeBone();
	bool CanIncludeBone();
	/** END exclude/include bones */

	/** retarget chains */
	void HandleNewRetargetChain();
	bool CanAddNewRetargetChain();
	void HandleSetRetargetRoot();
	bool CanSetRetargetRoot();
	void HandleClearRetargetRoot();
	bool CanClearRetargetRoot();
	/** END retarget chains */

	/** centralized editor controls (facilitate cross-communication between multiple UI elements)*/
	TWeakPtr<FTLLRN_IKRigEditorController> EditorController;
	
	/** command list we bind to */
	TSharedPtr<FUICommandList> CommandList;
	
	/** tree view widget */
	TSharedPtr<TLLRN_SIKRigSkeletonTreeView> TreeView;
	TArray<TSharedPtr<FIKRigTreeElement>> RootElements;
	TArray<TSharedPtr<FIKRigTreeElement>> AllElements;

	/** filtering the tree with search box */
	TSharedRef<SWidget> CreateFilterMenuWidget();
	void OnFilterTextChanged(const FText& SearchText);
	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;
	FTLLRN_IKRigHierarchyFilterOptions FilterOptions;
	
	/** tree view callbacks */
	void RefreshTreeView(bool IsInitialSetup=false);
	void HandleGetChildrenForTree(TSharedPtr<FIKRigTreeElement> InItem, TArray<TSharedPtr<FIKRigTreeElement>>& OutChildren);
	void OnSelectionChanged(TSharedPtr<FIKRigTreeElement> Selection, ESelectInfo::Type SelectInfo);
	TSharedRef< SWidget > CreateAddNewMenu();
	TSharedPtr< SWidget > CreateContextMenu();
	void OnItemClicked(TSharedPtr<FIKRigTreeElement> InItem);
	void OnItemDoubleClicked(TSharedPtr<FIKRigTreeElement> InItem);
	void OnSetExpansionRecursive(TSharedPtr<FIKRigTreeElement> InItem, bool bShouldBeExpanded);
	void SetExpansionRecursive(TSharedPtr<FIKRigTreeElement> InElement, bool bTowardsParent, bool bShouldBeExpanded);
	void FillContextMenu(FMenuBuilder& MenuBuilder);
	/** END tree view callbacks */

	friend TLLRN_SIKRigHierarchyItem;
	friend FTLLRN_IKRigEditorController;
	friend TLLRN_SIKRigSolverStack;
	friend TLLRN_SIKRigRetargetChainList;
};
