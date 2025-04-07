// Copyright Epic Games, Inc. All Rights Reserved.
/**
* View for holding TLLRN_ControlRig Animation Outliner
*/
#pragma once

#include "CoreMinimal.h"
#include "EditMode/TLLRN_ControlRigBaseDockableView.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "TLLRN_ControlRig.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Editor/STLLRN_RigHierarchyTreeView.h"
#include "Widgets/SBoxPanel.h"


class ISequencer;
class SExpandableArea;
class SSearchableTLLRN_RigHierarchyTreeView;

class SMultiTLLRN_RigHierarchyTreeView;
class SMultiTLLRN_RigHierarchyItem;
class FMultiRigTreeElement;
class FMultiRigData;

DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnMultiRigTreeCompareKeys, const FMultiRigData& /*A*/, const FMultiRigData& /*B*/);

typedef STreeView<TSharedPtr<FMultiRigTreeElement>>::FOnSelectionChanged FOnMultiRigTreeSelectionChanged;
typedef STreeView<TSharedPtr<FMultiRigTreeElement>>::FOnMouseButtonClick FOnMultiRigTreeMouseButtonClick;
typedef STreeView<TSharedPtr<FMultiRigTreeElement>>::FOnMouseButtonDoubleClick FOnMultiRigTreeMouseButtonDoubleClick;
typedef STreeView<TSharedPtr<FMultiRigTreeElement>>::FOnSetExpansionRecursive FOnMultiRigTreeSetExpansionRecursive;

uint32 GetTypeHash(const FMultiRigData& Data);

struct TLLRN_CONTROLRIGEDITOR_API FMultiRigTreeDelegates
{
	FOnGetRigTreeDisplaySettings OnGetDisplaySettings;
	FOnMultiRigTreeSelectionChanged OnSelectionChanged;
	FOnContextMenuOpening OnContextMenuOpening;
	FOnMultiRigTreeMouseButtonClick OnMouseButtonClick;
	FOnMultiRigTreeMouseButtonDoubleClick OnMouseButtonDoubleClick;
	FOnMultiRigTreeSetExpansionRecursive OnSetExpansionRecursive;
	FOnRigTreeCompareKeys OnCompareKeys;

	FMultiRigTreeDelegates()
	{
		bIsChangingTLLRN_RigHierarchy = false;
	}


	const FRigTreeDisplaySettings& GetDisplaySettings() const
	{
		if (OnGetDisplaySettings.IsBound())
		{
			return OnGetDisplaySettings.Execute();
		}
		return DefaultDisplaySettings;
	}

	void HandleSelectionChanged(TSharedPtr<FMultiRigTreeElement> Selection, ESelectInfo::Type SelectInfo)
	{
		if (bIsChangingTLLRN_RigHierarchy)
		{
			return;
		}
		TGuardValue<bool> Guard(bIsChangingTLLRN_RigHierarchy, true);
		OnSelectionChanged.ExecuteIfBound(Selection, SelectInfo);
	}

	static FRigTreeDisplaySettings DefaultDisplaySettings;
	bool bIsChangingTLLRN_RigHierarchy;
};

/** Data for the tree*/
class FMultiRigData
{

public:
	FMultiRigData() {};
	FMultiRigData(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_RigElementKey InKey) : TLLRN_ControlRig(InTLLRN_ControlRig), Key(InKey) {};
	FText GetName() const;
	FText GetDisplayName() const;
	bool operator == (const FMultiRigData & Other) const;
	bool IsValid() const;
	UTLLRN_RigHierarchy* GetHierarchy() const;
public:
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;
	TOptional<FTLLRN_RigElementKey> Key;
};

/** An item in the tree */
class FMultiRigTreeElement : public TSharedFromThis<FMultiRigTreeElement>
{
public:
	FMultiRigTreeElement(const FMultiRigData& InData, TWeakPtr<SMultiTLLRN_RigHierarchyTreeView> InTreeView,ERigTreeFilterResult InFilterResult);
public:
	/** Element Data to display */
	FMultiRigData Data;
	TArray<TSharedPtr<FMultiRigTreeElement>> Children;

	TSharedRef<ITableRow> MakeTreeRowWidget(const TSharedRef<STableViewBase>& InOwnerTable, TSharedRef<FMultiRigTreeElement> InRigTreeElement, TSharedPtr<SMultiTLLRN_RigHierarchyTreeView> InTreeView, const FRigTreeDisplaySettings& InSettings, bool bPinned);

	void RefreshDisplaySettings(const UTLLRN_RigHierarchy* InHierarchy, const FRigTreeDisplaySettings& InSettings);

	/** The current filter result */
	ERigTreeFilterResult FilterResult;

	/** The brush to use when rendering an icon */
	const FSlateBrush* IconBrush;;

	/** The color to use when rendering an icon */
	FSlateColor IconColor;

	/** The color to use when rendering the label text */
	FSlateColor TextColor;
};

class SMultiTLLRN_RigHierarchyItem : public STableRow<TSharedPtr<FMultiRigTreeElement>>
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, TSharedRef<FMultiRigTreeElement> InRigTreeElement, TSharedPtr<SMultiTLLRN_RigHierarchyTreeView> InTreeView, const FRigTreeDisplaySettings& InSettings, bool bPinned);
	static TPair<const FSlateBrush*, FSlateColor> GetBrushForElementType(const UTLLRN_RigHierarchy* InHierarchy, const FMultiRigData& InData);

private:
	TWeakPtr<FMultiRigTreeElement> WeakRigTreeElement;
	FMultiRigTreeDelegates Delegates;

	FText GetName() const;
	FText GetDisplayName() const;
	FReply OnGetSelectedClicked();

};

class SMultiTLLRN_RigHierarchyTreeView : public STreeView<TSharedPtr<FMultiRigTreeElement>>
{
public:

	SLATE_BEGIN_ARGS(SMultiTLLRN_RigHierarchyTreeView) {}
	SLATE_ARGUMENT(FMultiRigTreeDelegates, RigTreeDelegates)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMultiTLLRN_RigHierarchyTreeView() {}

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override
	{
		FReply Reply = STreeView<TSharedPtr<FMultiRigTreeElement>>::OnFocusReceived(MyGeometry, InFocusEvent);
		return Reply;
	}

	/** Save a snapshot of the internal map that tracks item expansion before tree reconstruction */
	void SaveAndClearSparseItemInfos()
	{
		// Only save the info if there is something to save (do not overwrite info with an empty map)
		if (!SparseItemInfos.IsEmpty())
		{
			OldSparseItemInfos = SparseItemInfos;
		}
		ClearExpandedItems();
	}

	/** Restore the expansion infos map from the saved snapshot after tree reconstruction */
	void RestoreSparseItemInfos(TSharedPtr<FMultiRigTreeElement> ItemPtr)
	{
		for (const auto& Pair : OldSparseItemInfos)
		{
			if (Pair.Key->Data == ItemPtr->Data)
			{
				// the SparseItemInfos now reference the new element, but keep the same expansion state
				SparseItemInfos.Add(ItemPtr, Pair.Value);
				break;
			}
		}
	}

	static TSharedPtr<FMultiRigTreeElement> FindElement(const FMultiRigData& InData, TSharedPtr<FMultiRigTreeElement> CurrentItem);

	bool AddElement(const FMultiRigData& InData, const FMultiRigData& InParentData);
	bool AddElement(UTLLRN_ControlRig* InTLLRN_ControlRig, const FTLLRN_RigBaseElement* InElement);
	bool ReparentElement(const FMultiRigData& InData, const FMultiRigData& InParentData);
	bool RemoveElement(const FMultiRigData& InData);
	void RefreshTreeView(bool bRebuildContent = true);
	void SetExpansionRecursive(TSharedPtr<FMultiRigTreeElement> InElement, bool bTowardsParent, bool bShouldBeExpanded);
	TSharedRef<ITableRow> MakeTableRowWidget(TSharedPtr<FMultiRigTreeElement> InItem, const TSharedRef<STableViewBase>& OwnerTable, bool bPinned);
	void HandleGetChildrenForTree(TSharedPtr<FMultiRigTreeElement> InItem, TArray<TSharedPtr<FMultiRigTreeElement>>& OutChildren);

	TArray<FMultiRigData> GetSelectedData() const;
	const TArray<TSharedPtr<FMultiRigTreeElement>>& GetRootElements() const { return RootElements; }
	FMultiRigTreeDelegates& GetTreeDelegates() { return Delegates; }

	TArray<UTLLRN_RigHierarchy*> GetHierarchy() const;
	void SetTLLRN_ControlRigs(TArrayView < TWeakObjectPtr<UTLLRN_ControlRig>>& InTLLRN_ControlRigs);

private:

	/** A temporary snapshot of the SparseItemInfos in STreeView, used during RefreshTreeView() */
	TSparseItemMap OldSparseItemInfos;

	/** Backing array for tree view */
	TArray<TSharedPtr<FMultiRigTreeElement>> RootElements;

	/** A map for looking up items based on their key */
	TMap<FMultiRigData, TSharedPtr<FMultiRigTreeElement>> ElementMap;

	/** A map for looking up a parent based on their key */
	TMap<FMultiRigData, FMultiRigData> ParentMap;

	FMultiRigTreeDelegates Delegates;

	friend class STLLRN_RigHierarchy;

	TArray <TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigs;
};

class SSearchableMultiTLLRN_RigHierarchyTreeView : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSearchableMultiTLLRN_RigHierarchyTreeView) {}
	SLATE_ARGUMENT(FMultiRigTreeDelegates, RigTreeDelegates)
		SLATE_ARGUMENT(FText, InitialFilterText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SSearchableMultiTLLRN_RigHierarchyTreeView() {}
	TSharedRef<SMultiTLLRN_RigHierarchyTreeView> GetTreeView() const { return TreeView.ToSharedRef(); }
	const FRigTreeDisplaySettings& GetDisplaySettings();

private:

	void OnFilterTextChanged(const FText& SearchText);

	FOnGetRigTreeDisplaySettings SuperGetRigTreeDisplaySettings;
	FText FilterText;
	FRigTreeDisplaySettings Settings;
	TSharedPtr<SMultiTLLRN_RigHierarchyTreeView> TreeView;
};

class STLLRN_ControlRigOutliner : public FTLLRN_ControlRigBaseDockableView, public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STLLRN_ControlRigOutliner){}
	SLATE_END_ARGS()
	STLLRN_ControlRigOutliner();
	~STLLRN_ControlRigOutliner();

	void Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode& InEditMode);

	//FTLLRN_ControlRigBaseDockableView overrides
	virtual void SetEditMode(FTLLRN_ControlRigEditMode& InEditMode) override;
private:
	virtual void HandleControlAdded(UTLLRN_ControlRig* TLLRN_ControlRig, bool bIsAdded) override;
	virtual void HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControl, bool bSelected) override;

	//control rig delegates
	void HandleOnTLLRN_ControlRigBound(UTLLRN_ControlRig* InTLLRN_ControlRig);
	void HandleOnObjectBoundToTLLRN_ControlRig(UObject* InObject);

	void OnObjectsReplaced(const TMap<UObject*, UObject*>& OldToNewInstanceMap);

	void HandleSelectionChanged(TSharedPtr<FMultiRigTreeElement> Selection, ESelectInfo::Type SelectInfo);

	/** Hierarchy picker for controls*/
	TSharedPtr<SSearchableMultiTLLRN_RigHierarchyTreeView> HierarchyTreeView;
	FRigTreeDisplaySettings DisplaySettings;
	const FRigTreeDisplaySettings& GetDisplaySettings() const { return DisplaySettings; }
	bool bIsChangingTLLRN_RigHierarchy = false;
	TSharedPtr<SExpandableArea> PickerExpander;

	//set of control rigs we are bound too and need to clear delegates from
	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> BoundTLLRN_ControlRigs;
};


