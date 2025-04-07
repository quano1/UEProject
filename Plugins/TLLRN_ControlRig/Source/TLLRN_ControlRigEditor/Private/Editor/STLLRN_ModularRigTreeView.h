// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/STreeView.h"
#include "TLLRN_ModularRig.h"
#include "Editor/STLLRN_RigHierarchyTreeView.h"

class SSearchBox;
class STLLRN_ModularRigTreeView;
class STLLRN_ModularRigModelItem;
class FTLLRN_ModularRigTreeElement;


DECLARE_DELEGATE_RetVal(const UTLLRN_ModularRig*, FOnGetTLLRN_ModularRigTreeRig);
DECLARE_DELEGATE_OneParam(FOnTLLRN_ModularRigTreeRequestDetailsInspection, const FString&);
DECLARE_DELEGATE_RetVal_TwoParams(FName, FOnTLLRN_ModularRigTreeRenameElement, const FString& /*OldPath*/, const FName& /*NewName*/);
DECLARE_DELEGATE_TwoParams(FOnTLLRN_ModularRigTreeResolveConnector, const FTLLRN_RigElementKey& /*Connector*/, const FTLLRN_RigElementKey& /*Target*/);
DECLARE_DELEGATE_OneParam(FOnTLLRN_ModularRigTreeDisconnectConnector, const FTLLRN_RigElementKey& /*Connector*/);
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnTLLRN_ModularRigTreeVerifyElementNameChanged, const FString& /*OldPath*/, const FName& /*NewName*/, FText& /*OutErrorMessage*/);

typedef STreeView<TSharedPtr<FTLLRN_ModularRigTreeElement>>::FOnMouseButtonClick FOnTLLRN_ModularRigTreeMouseButtonClick;
typedef STreeView<TSharedPtr<FTLLRN_ModularRigTreeElement>>::FOnMouseButtonDoubleClick FOnTLLRN_ModularRigTreeMouseButtonDoubleClick;
typedef STableRow<TSharedPtr<FTLLRN_ModularRigTreeElement>>::FOnCanAcceptDrop FOnTLLRN_ModularRigTreeCanAcceptDrop;
typedef STableRow<TSharedPtr<FTLLRN_ModularRigTreeElement>>::FOnAcceptDrop FOnTLLRN_ModularRigTreeAcceptDrop;
typedef STreeView<TSharedPtr<FTLLRN_ModularRigTreeElement>>::FOnSelectionChanged FOnTLLRN_ModularRigTreeSelectionChanged;

struct TLLRN_CONTROLRIGEDITOR_API FTLLRN_ModularRigTreeDelegates
{
public:
	
	FOnGetTLLRN_ModularRigTreeRig OnGetTLLRN_ModularRig;
	FOnTLLRN_ModularRigTreeMouseButtonClick OnMouseButtonClick;
	FOnTLLRN_ModularRigTreeMouseButtonDoubleClick OnMouseButtonDoubleClick;
	FOnDragDetected OnDragDetected;
	FOnTLLRN_ModularRigTreeCanAcceptDrop OnCanAcceptDrop;
	FOnTLLRN_ModularRigTreeAcceptDrop OnAcceptDrop;
	FOnContextMenuOpening OnContextMenuOpening;
	FOnTLLRN_ModularRigTreeRequestDetailsInspection OnRequestDetailsInspection;
	FOnTLLRN_ModularRigTreeRenameElement OnRenameElement;
	FOnTLLRN_ModularRigTreeVerifyElementNameChanged OnVerifyModuleNameChanged;
	FOnTLLRN_ModularRigTreeResolveConnector OnResolveConnector;
	FOnTLLRN_ModularRigTreeDisconnectConnector OnDisconnectConnector;
	FOnTLLRN_ModularRigTreeSelectionChanged OnSelectionChanged;
	
	FTLLRN_ModularRigTreeDelegates()
	{
	}

	const UTLLRN_ModularRig* GetTLLRN_ModularRig() const
	{
		if(OnGetTLLRN_ModularRig.IsBound())
		{
			return OnGetTLLRN_ModularRig.Execute();
		}
		return nullptr;
	}

	FName HandleRenameElement(const FString& OldPath, const FName& NewName) const
	{
		if(OnRenameElement.IsBound())
		{
			return OnRenameElement.Execute(OldPath, NewName);
		}
		return *OldPath;
	}

	bool HandleVerifyElementNameChanged(const FString& OldPath, const FName& NewName, FText& OutErrorMessage) const
	{
		if(OnVerifyModuleNameChanged.IsBound())
		{
			return OnVerifyModuleNameChanged.Execute(OldPath, NewName, OutErrorMessage);
		}
		return false;
	}

	bool HandleResolveConnector(const FTLLRN_RigElementKey& InConnector, const FTLLRN_RigElementKey& InTarget)
	{
		if(OnResolveConnector.IsBound())
		{
			OnResolveConnector.Execute(InConnector, InTarget);
			return true;
		}
		return false;
	}

	bool HandleDisconnectConnector(const FTLLRN_RigElementKey& InConnector)
	{
		if(OnDisconnectConnector.IsBound())
		{
			OnDisconnectConnector.Execute(InConnector);
			return true;
		}
		return false;
	}

	void HandleSelectionChanged(TSharedPtr<FTLLRN_ModularRigTreeElement> Selection, ESelectInfo::Type SelectInfo)
	{
		if(bSuspendSelectionDelegate)
		{
			return;
		}
		TGuardValue<bool> Guard(bSuspendSelectionDelegate, true);
		(void)OnSelectionChanged.ExecuteIfBound(Selection, SelectInfo);
	}

private:

	bool bSuspendSelectionDelegate = false;

	friend class STLLRN_ModularRigTreeView;
};


/** An item in the tree */
class FTLLRN_ModularRigTreeElement : public TSharedFromThis<FTLLRN_ModularRigTreeElement>
{
public:
	FTLLRN_ModularRigTreeElement(const FString& InKey, TWeakPtr<STLLRN_ModularRigTreeView> InTreeView, bool bInIsPrimary);

public:
	/** Element Data to display */
	FString Key;
	bool bIsPrimary;
	FString ModulePath;
	FString ConnectorName;
	FName ShortName;
	TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> Children;

	TSharedRef<ITableRow> MakeTreeRowWidget(const TSharedRef<STableViewBase>& InOwnerTable, TSharedRef<FTLLRN_ModularRigTreeElement> InRigTreeElement, TSharedPtr<STLLRN_ModularRigTreeView> InTreeView, bool bPinned);

	void RequestRename();

	void RefreshDisplaySettings(const UTLLRN_ModularRig* InTLLRN_ModularRig);

	TPair<const FSlateBrush*, FSlateColor> GetBrushAndColor(const UTLLRN_ModularRig* InTLLRN_ModularRig);

	/** Delegate for when the context menu requests a rename */
	DECLARE_DELEGATE(FOnRenameRequested);
	FOnRenameRequested OnRenameRequested;

	static TMap<FSoftObjectPath, TSharedPtr<FSlateBrush>> IconPathToBrush;

	/** The brush to use when rendering an icon */
	const FSlateBrush* IconBrush;

	/** The color to use when rendering an icon */
	FSlateColor IconColor;

	/** The color to use when rendering the label text */
	FSlateColor TextColor;
};

class STLLRN_ModularRigModelItem : public SMultiColumnTableRow<TSharedPtr<FTLLRN_ModularRigTreeElement>>
{
public:
	
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, TSharedRef<FTLLRN_ModularRigTreeElement> InRigTreeElement, TSharedPtr<STLLRN_ModularRigTreeView> InTreeView, bool bPinned);
	void PopulateConnectorTargetList(FTLLRN_RigElementKey InConnectorKey);
	void PopulateConnectorCurrentTarget(
		TSharedPtr<SVerticalBox> InListBox, 
		const FTLLRN_RigElementKey& InConnectorKey,
		const FTLLRN_RigElementKey& InTargetKey,
		const FSlateBrush* InBrush,
		const FSlateColor& InColor,
		const FText& InTitle);
	void OnConnectorTargetChanged(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo, const FTLLRN_RigElementKey InConnectorKey);

	void OnNameCommitted(const FText& InText, ETextCommit::Type InCommitType) const;
	bool OnVerifyNameChanged(const FText& InText, FText& OutErrorMessage);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TWeakPtr<FTLLRN_ModularRigTreeElement> WeakRigTreeElement;
 	FTLLRN_ModularRigTreeDelegates Delegates;
	TSharedPtr<SSearchableTLLRN_RigHierarchyTreeView> ConnectorComboBox;
	TSharedPtr<SButton> ResetConnectorButton;
	TSharedPtr<SButton> UseSelectedButton;
	TSharedPtr<SButton> SelectElementButton;
	FTLLRN_RigElementKey ConnectorKey;
	TOptional<FTLLRN_ModularRigResolveResult> ConnectorMatches;

	FText GetName(bool bUseShortName) const;
	FText GetItemTooltip() const;

	friend class STLLRN_ModularRigTreeView; 
};

class STLLRN_ModularRigTreeView : public STreeView<TSharedPtr<FTLLRN_ModularRigTreeElement>>
{
public:

	static const FName Column_Module;
	static const FName Column_Tags;
	static const FName Column_Connector;
	static const FName Column_Buttons;

	SLATE_BEGIN_ARGS(STLLRN_ModularRigTreeView)
		: _AutoScrollEnabled(false)
		, _FilterText()
		, _ShowSecondaryConnectors(false)
		, _ShowOptionalConnectors(false)
		, _ShowUnresolvedConnectors(true)
	{}
		SLATE_ARGUMENT( TSharedPtr<SHeaderRow>, HeaderRow )
		SLATE_ARGUMENT(FTLLRN_ModularRigTreeDelegates, RigTreeDelegates)
		SLATE_ARGUMENT(bool, AutoScrollEnabled)
		SLATE_ATTRIBUTE(FText, FilterText)
		SLATE_ATTRIBUTE(bool, ShowSecondaryConnectors)
		SLATE_ATTRIBUTE(bool, ShowOptionalConnectors)
		SLATE_ATTRIBUTE(bool, ShowUnresolvedConnectors)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STLLRN_ModularRigTreeView() {}

	/** Performs auto scroll */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	bool bRequestRenameSelected = false;

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
	void RestoreSparseItemInfos(TSharedPtr<FTLLRN_ModularRigTreeElement> ItemPtr)
	{
		for (const auto& Pair : OldSparseItemInfos)
		{
			if (Pair.Key->Key == ItemPtr->Key)
			{
				// the SparseItemInfos now reference the new element, but keep the same expansion state
				SparseItemInfos.Add(ItemPtr, Pair.Value);
				break;
			}
		}
	}


	TSharedPtr<FTLLRN_ModularRigTreeElement> FindElement(const FString& InElementKey);
	static TSharedPtr<FTLLRN_ModularRigTreeElement> FindElement(const FString& InElementKey, TSharedPtr<FTLLRN_ModularRigTreeElement> CurrentItem);
	bool AddElement(FString InKey, FString InParentKey = FString(), bool bApplyFilterText = true);
	bool AddElement(const FTLLRN_RigModuleInstance* InElement, bool bApplyFilterText);
	void AddSpacerElement();
	bool ReparentElement(const FString InKey, const FString InParentKey);
	void RefreshTreeView(bool bRebuildContent = true);
	TSharedRef<ITableRow> MakeTableRowWidget(TSharedPtr<FTLLRN_ModularRigTreeElement> InItem, const TSharedRef<STableViewBase>& OwnerTable, bool bPinned);
	void HandleGetChildrenForTree(TSharedPtr<FTLLRN_ModularRigTreeElement> InItem, TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>>& OutChildren);

	TArray<FString> GetSelectedKeys() const;
	void SetSelection(const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>>& InSelection);
	const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>>& GetRootElements() const { return RootElements; }
	FTLLRN_ModularRigTreeDelegates& GetRigTreeDelegates() { return Delegates; }

	/** Given a position, return the item under that position. If nothing is there, return null. */
	const TSharedPtr<FTLLRN_ModularRigTreeElement>* FindItemAtPosition(FVector2D InScreenSpacePosition) const;

private:

	/** A temporary snapshot of the SparseItemInfos in STreeView, used during RefreshTreeView() */
	TSparseItemMap OldSparseItemInfos;

	/** Backing array for tree view */
	TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> RootElements;
	
	/** A map for looking up items based on their key */
	TMap<FString, TSharedPtr<FTLLRN_ModularRigTreeElement>> ElementMap;

	/** A map for looking up a parent based on their key */
	TMap<FString, FString> ParentMap;

	FTLLRN_ModularRigTreeDelegates Delegates;

	bool bAutoScrollEnabled;
	FVector2D LastMousePosition;
	double TimeAtMousePosition;

	TAttribute<FText> FilterText;
	TAttribute<bool> ShowSecondaryConnectors;
	TAttribute<bool> ShowOptionalConnectors;
	TAttribute<bool> ShowUnresolvedConnectors;

	friend class STLLRN_ModularRigModel;
};

class SSearchableTLLRN_ModularRigTreeView : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSearchableTLLRN_ModularRigTreeView) {}
		SLATE_ARGUMENT(FTLLRN_ModularRigTreeDelegates, RigTreeDelegates)
		SLATE_ARGUMENT(FText, InitialFilterText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SSearchableTLLRN_ModularRigTreeView() {}
	TSharedRef<STLLRN_ModularRigTreeView> GetTreeView() const { return TreeView.ToSharedRef(); }

private:

	TSharedPtr<STLLRN_ModularRigTreeView> TreeView;
};
