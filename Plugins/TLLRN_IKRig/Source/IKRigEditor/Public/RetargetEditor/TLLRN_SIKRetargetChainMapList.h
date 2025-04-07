// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "TLLRN_IKRetargetEditorController.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "Framework/Commands/UICommandList.h"

class FTextFilterExpressionEvaluator;
class FTLLRN_IKRigEditorController;
class TLLRN_SIKRetargetChainMapList;
class FTLLRN_IKRetargetEditor;
struct FTLLRN_BoneChain;

class FTLLRN_RetargetChainMapElement
{
public:

	TSharedRef<ITableRow> MakeListRowWidget(
		const TSharedRef<STableViewBase>& InOwnerTable,
        TSharedRef<FTLLRN_RetargetChainMapElement> InStackElement,
        TSharedPtr<TLLRN_SIKRetargetChainMapList> InChainList);

	static TSharedRef<FTLLRN_RetargetChainMapElement> Make(TObjectPtr<UTLLRN_RetargetChainSettings> InChainMap)
	{
		return MakeShareable(new FTLLRN_RetargetChainMapElement(InChainMap));
	}

	TWeakObjectPtr<UTLLRN_RetargetChainSettings> ChainMap;

private:
	
	/** Hidden constructor, always use Make above */
	FTLLRN_RetargetChainMapElement(TObjectPtr<UTLLRN_RetargetChainSettings> InChainMap) : ChainMap(InChainMap) {}

	/** Hidden constructor, always use Make above */
	FTLLRN_RetargetChainMapElement() = default;
};

typedef TSharedPtr< FTLLRN_RetargetChainMapElement > FTLLRN_RetargetChainMapElementPtr;
class SIKRetargetChainMapRow : public SMultiColumnTableRow< FTLLRN_RetargetChainMapElementPtr >
{
public:
	
	void Construct(
        const FArguments& InArgs,
        const TSharedRef<STableViewBase>& InOwnerTableView,
        TSharedRef<FTLLRN_RetargetChainMapElement> InChainElement,
        TSharedPtr<TLLRN_SIKRetargetChainMapList> InChainList);

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the table row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

	void OnSourceChainComboSelectionChanged(TSharedPtr<FString> InName, ESelectInfo::Type SelectInfo);
	
private:

	FReply OnResetToDefaultClicked();

	EVisibility GetResetToDefaultVisibility() const;
	
	FText GetSourceChainName() const;

	FText GetTargetIKGoalName() const;

	TArray<TSharedPtr<FString>> SourceChainOptions;

	TWeakPtr<FTLLRN_RetargetChainMapElement> ChainMapElement;
	
	TWeakPtr<TLLRN_SIKRetargetChainMapList> ChainMapList;

	friend TLLRN_SIKRetargetChainMapList;
};

struct FTLLRN_ChainMapFilterOptions
{
	bool bHideUnmappedChains = false;
	bool bHideMappedChains = false;
	bool bHideChainsWithoutIK = false;
};

typedef SListView< TSharedPtr<FTLLRN_RetargetChainMapElement> > SRetargetChainMapListViewType;

class TLLRN_SIKRetargetChainMapList : public SCompoundWidget, public FEditorUndoClient
{
	
public:
	
	SLATE_BEGIN_ARGS(TLLRN_SIKRetargetChainMapList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTLLRN_IKRetargetEditorController> InEditorController);

	void ClearSelection() const;

	void ResetChainSettings(UTLLRN_RetargetChainSettings* Settings) const;
	
private:
	
	/** menu for adding new solver commands */
	TSharedPtr<FUICommandList> CommandList;
	
	/** editor controller */
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;

	/** list view */
	TSharedPtr<SRetargetChainMapListViewType> ListView;
	TArray< TSharedPtr<FTLLRN_RetargetChainMapElement> > ListViewItems;
	/** END list view */

	UTLLRN_IKRetargeterController* GetRetargetController() const;

	/** callbacks */
	FText GetSourceRootBone() const;
	FText GetTargetRootBone() const;
	bool IsChainMapEnabled() const;
	/** when a chain is clicked on in the table view */
	void OnItemClicked(TSharedPtr<FTLLRN_RetargetChainMapElement> InItem) const;

	/** auto-map chain button*/
	TSharedRef<SWidget> CreateChainMapMenuWidget();
	EVisibility IsAutoMapButtonVisible() const;
	void AutoMapChains(const ETLLRN_AutoMapChainType AutoMapType, const bool bSetUnmatchedToNone);
	/** END auto-map chain button*/

	/** filtering the list with search box */
	TSharedRef<SWidget> CreateFilterMenuWidget();
	void OnFilterTextChanged(const FText& SearchText);
	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;
	FTLLRN_ChainMapFilterOptions ChainFilterOptions;

	/** list view generate row callback */
	TSharedRef<ITableRow> MakeListRowWidget(TSharedPtr<FTLLRN_RetargetChainMapElement> InElement, const TSharedRef<STableViewBase>& OwnerTable);

	/** call to refresh the list view */
	void RefreshView();

	friend SIKRetargetChainMapRow;
	friend FTLLRN_IKRetargetEditorController;
};
