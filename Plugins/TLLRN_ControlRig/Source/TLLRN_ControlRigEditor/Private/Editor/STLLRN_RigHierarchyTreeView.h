// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/STreeView.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "STLLRN_RigHierarchyTagWidget.h"

class SSearchBox;
class STLLRN_RigHierarchyTreeView;
class STLLRN_RigHierarchyItem;
class FRigTreeElement;

struct TLLRN_CONTROLRIGEDITOR_API FRigTreeDisplaySettings
{
	FRigTreeDisplaySettings()
	{
		FilterText = FText();

		bFlattenHierarchyOnFilter = false;
		bHideParentsOnFilter = false;
		bUseShortName = true;
		bShowImportedBones = true;
		bShowBones = true;
		bShowControls = true;
		bShowNulls = true;
		bShowPhysics = true;
		bShowReferences = true;
		bShowSockets = true;
		bShowConnectors = true;
		bShowIconColors = true;
	}
	
	FText FilterText;

	/** Flatten when text filtering is active */
	bool bFlattenHierarchyOnFilter;

	/** Hide parents when text filtering is active */
	bool bHideParentsOnFilter;

	/** When true, the elements will show their short name */
	bool bUseShortName;

	/** Whether or not to show imported bones in the hierarchy */
	bool bShowImportedBones;

	/** Whether or not to show bones in the hierarchy */
	bool bShowBones;

	/** Whether or not to show controls in the hierarchy */
	bool bShowControls;

	/** Whether or not to show spaces in the hierarchy */
	bool bShowNulls;

	/** Whether or not to show physics elements in the hierarchy */
	bool bShowPhysics;

	/** Whether or not to show references in the hierarchy */
	bool bShowReferences;

	/** Whether or not to show sockets in the hierarchy */
	bool bShowSockets;

	/** Whether or not to show connectors in the hierarchy */
	bool bShowConnectors;
	
	/** Whether to tint the icons with the element color */
	bool bShowIconColors;
};

DECLARE_DELEGATE_RetVal(const UTLLRN_RigHierarchy*, FOnGetRigTreeHierarchy);
DECLARE_DELEGATE_RetVal(const FRigTreeDisplaySettings&, FOnGetRigTreeDisplaySettings);
DECLARE_DELEGATE_RetVal(const TArray<FTLLRN_RigElementKey>, FOnRigTreeGetSelection);
DECLARE_DELEGATE_RetVal_TwoParams(FName, FOnRigTreeRenameElement, const FTLLRN_RigElementKey& /*OldKey*/, const FString& /*NewName*/);
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnRigTreeVerifyElementNameChanged, const FTLLRN_RigElementKey& /*OldKey*/, const FString& /*NewName*/, FText& /*OutErrorMessage*/);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnRigTreeCompareKeys, const FTLLRN_RigElementKey& /*A*/, const FTLLRN_RigElementKey& /*B*/);
DECLARE_DELEGATE_RetVal_OneParam(FTLLRN_RigElementKey, FOnRigTreeGetResolvedKey, const FTLLRN_RigElementKey&);
DECLARE_DELEGATE_OneParam(FOnRigTreeRequestDetailsInspection, const FTLLRN_RigElementKey&);
DECLARE_DELEGATE_RetVal_OneParam(TOptional<FText>, FOnRigTreeItemGetToolTip, const FTLLRN_RigElementKey&);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnRigTreeIsItemVisible, const FTLLRN_RigElementKey&);

typedef STableRow<TSharedPtr<FRigTreeElement>>::FOnCanAcceptDrop FOnRigTreeCanAcceptDrop;
typedef STableRow<TSharedPtr<FRigTreeElement>>::FOnAcceptDrop FOnRigTreeAcceptDrop;
typedef STreeView<TSharedPtr<FRigTreeElement>>::FOnSelectionChanged FOnRigTreeSelectionChanged;
typedef STreeView<TSharedPtr<FRigTreeElement>>::FOnMouseButtonClick FOnRigTreeMouseButtonClick;
typedef STreeView<TSharedPtr<FRigTreeElement>>::FOnMouseButtonDoubleClick FOnRigTreeMouseButtonDoubleClick;
typedef STreeView<TSharedPtr<FRigTreeElement>>::FOnSetExpansionRecursive FOnRigTreeSetExpansionRecursive;

struct TLLRN_CONTROLRIGEDITOR_API FRigTreeDelegates
{
	FOnGetRigTreeHierarchy OnGetHierarchy;
	FOnGetRigTreeDisplaySettings OnGetDisplaySettings;
	FOnRigTreeRenameElement OnRenameElement;
	FOnRigTreeVerifyElementNameChanged OnVerifyElementNameChanged;
	FOnDragDetected OnDragDetected;
	FOnRigTreeCanAcceptDrop OnCanAcceptDrop;
	FOnRigTreeAcceptDrop OnAcceptDrop;
	FOnRigTreeGetSelection OnGetSelection;
	FOnRigTreeSelectionChanged OnSelectionChanged;
	FOnContextMenuOpening OnContextMenuOpening;
	FOnRigTreeMouseButtonClick OnMouseButtonClick;
	FOnRigTreeMouseButtonDoubleClick OnMouseButtonDoubleClick;
	FOnRigTreeSetExpansionRecursive OnSetExpansionRecursive;
	FOnRigTreeCompareKeys OnCompareKeys;
	FOnRigTreeGetResolvedKey OnGetResolvedKey;
	FOnRigTreeRequestDetailsInspection OnRequestDetailsInspection;
	FOnRigTreeElementKeyTagDragDetected OnRigTreeElementKeyTagDragDetected;
	FOnRigTreeItemGetToolTip OnRigTreeGetItemToolTip;
	FOnRigTreeIsItemVisible OnRigTreeIsItemVisible;

	FRigTreeDelegates()
	{
		bIsChangingTLLRN_RigHierarchy = false;
	}

	const UTLLRN_RigHierarchy* GetHierarchy() const
	{
		if(OnGetHierarchy.IsBound())
		{
			return OnGetHierarchy.Execute();
		}
		return nullptr;
	}

	const FRigTreeDisplaySettings& GetDisplaySettings() const
	{
		if(OnGetDisplaySettings.IsBound())
		{
			return OnGetDisplaySettings.Execute();
		}
		return DefaultDisplaySettings;
	}

	TArray<FTLLRN_RigElementKey> GetSelection() const
	{
		if (OnGetSelection.IsBound())
		{
			return OnGetSelection.Execute();
		}
		if (const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
		{
			return Hierarchy->GetSelectedKeys();
		}
		return {};
	}
	
	FName HandleRenameElement(const FTLLRN_RigElementKey& OldKey, const FString& NewName) const
	{
		if(OnRenameElement.IsBound())
		{
			return OnRenameElement.Execute(OldKey, NewName);
		}
		return OldKey.Name;
	}
	
	bool HandleVerifyElementNameChanged(const FTLLRN_RigElementKey& OldKey, const FString& NewName, FText& OutErrorMessage) const
	{
		if(OnVerifyElementNameChanged.IsBound())
		{
			return OnVerifyElementNameChanged.Execute(OldKey, NewName, OutErrorMessage);
		}
		return false;
	}

	void HandleSelectionChanged(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo)
	{
		if(bIsChangingTLLRN_RigHierarchy)
		{
			return;
		}
		TGuardValue<bool> Guard(bIsChangingTLLRN_RigHierarchy, true);
		OnSelectionChanged.ExecuteIfBound(Selection, SelectInfo);
	}

	FTLLRN_RigElementKey GetResolvedKey(const FTLLRN_RigElementKey& InKey)
	{
		if(OnGetResolvedKey.IsBound())
		{
			return OnGetResolvedKey.Execute(InKey);
		}
		return InKey;
	}

	void RequestDetailsInspection(const FTLLRN_RigElementKey& InKey)
	{
		if(OnRequestDetailsInspection.IsBound())
		{
			return OnRequestDetailsInspection.Execute(InKey);
		}
	}

	static FRigTreeDisplaySettings DefaultDisplaySettings;
	bool bIsChangingTLLRN_RigHierarchy;
};

/** 
 * Order is important here! 
 * This enum is used internally to the filtering logic and represents an ordering of most filtered (hidden) to 
 * least filtered (highlighted).
 */
enum class ERigTreeFilterResult : int32
{
	/** Hide the item */
	Hidden,

	/** Show the item because child items were shown */
	ShownDescendant,

	/** Show the item */
	Shown,
};

/** An item in the tree */
class FRigTreeElement : public TSharedFromThis<FRigTreeElement>
{
public:
	FRigTreeElement(const FTLLRN_RigElementKey& InKey, TWeakPtr<STLLRN_RigHierarchyTreeView> InTreeView, bool InSupportsRename, ERigTreeFilterResult InFilterResult);
public:
	/** Element Data to display */
	FTLLRN_RigElementKey Key;
	FName ShortName;
	FName ChannelName;
	bool bIsTransient;
	bool bIsAnimationChannel;
	bool bIsProcedural;
	bool bSupportsRename;
	TArray<TSharedPtr<FRigTreeElement>> Children;

	TSharedRef<ITableRow> MakeTreeRowWidget(const TSharedRef<STableViewBase>& InOwnerTable, TSharedRef<FRigTreeElement> InRigTreeElement, TSharedPtr<STLLRN_RigHierarchyTreeView> InTreeView, const FRigTreeDisplaySettings& InSettings, bool bPinned);

	void RequestRename();

	void RefreshDisplaySettings(const UTLLRN_RigHierarchy* InHierarchy, const FRigTreeDisplaySettings& InSettings);

	FSlateColor GetIconColor() const;
	FSlateColor GetTextColor() const;

	/** Delegate for when the context menu requests a rename */
	DECLARE_DELEGATE(FOnRenameRequested);
	FOnRenameRequested OnRenameRequested;

	/** The current filter result */
	ERigTreeFilterResult FilterResult;

	/** The brush to use when rendering an icon */
	const FSlateBrush* IconBrush;;

	/** The color to use when rendering an icon */
	FSlateColor IconColor;

	/** The color to use when rendering the label text */
	FSlateColor TextColor;

	/** If true the item is filtered out during a drag */
	bool bFadedOutDuringDragDrop;

	/** The tag arguments for this element */
	TArray<STLLRN_RigHierarchyTagWidget::FArguments> Tags;
};

class STLLRN_RigHierarchyItem : public STableRow<TSharedPtr<FRigTreeElement>>
{
public:
	
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, TSharedRef<FRigTreeElement> InRigTreeElement, TSharedPtr<STLLRN_RigHierarchyTreeView> InTreeView, const FRigTreeDisplaySettings& InSettings, bool bPinned);
 	void OnNameCommitted(const FText& InText, ETextCommit::Type InCommitType) const;
	bool OnVerifyNameChanged(const FText& InText, FText& OutErrorMessage);
	static TPair<const FSlateBrush*, FSlateColor> GetBrushForElementType(const UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InKey);
	static FLinearColor GetColorForControlType(ETLLRN_RigControlType InControlType, UEnum* InControlEnum);

private:
	TWeakPtr<FRigTreeElement> WeakRigTreeElement;
 	FRigTreeDelegates Delegates;

	FText GetNameForUI() const;
	FText GetName(bool bUseShortName) const;
	FText GetItemTooltip() const;

	friend class STLLRN_RigHierarchyTreeView; 
};

class STLLRN_RigHierarchyTreeView : public STreeView<TSharedPtr<FRigTreeElement>>
{
public:

	SLATE_BEGIN_ARGS(STLLRN_RigHierarchyTreeView)
		: _AutoScrollEnabled(false)
		, _PopulateOnConstruct(false)
	{}
		SLATE_ARGUMENT(FRigTreeDelegates, RigTreeDelegates)
		SLATE_ARGUMENT(bool, AutoScrollEnabled)
		SLATE_ARGUMENT(bool, PopulateOnConstruct)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STLLRN_RigHierarchyTreeView() {}

	/** Performs auto scroll */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override
	{
		FReply Reply = STreeView<TSharedPtr<FRigTreeElement>>::OnFocusReceived(MyGeometry, InFocusEvent);

		LastClickCycles = FPlatformTime::Cycles();

		return Reply;
	}

	uint32 LastClickCycles = 0;

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
	void RestoreSparseItemInfos(TSharedPtr<FRigTreeElement> ItemPtr)
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

	TSharedPtr<FRigTreeElement> FindElement(const FTLLRN_RigElementKey& InElementKey) const;
	static TSharedPtr<FRigTreeElement> FindElement(const FTLLRN_RigElementKey& InElementKey, TSharedPtr<FRigTreeElement> CurrentItem);
	bool AddElement(FTLLRN_RigElementKey InKey, FTLLRN_RigElementKey InParentKey = FTLLRN_RigElementKey());
	bool AddElement(const FTLLRN_RigBaseElement* InElement);
	void AddSpacerElement();
	bool ReparentElement(FTLLRN_RigElementKey InKey, FTLLRN_RigElementKey InParentKey);
	bool RemoveElement(FTLLRN_RigElementKey InKey);
	void RefreshTreeView(bool bRebuildContent = true);
	void SetExpansionRecursive(TSharedPtr<FRigTreeElement> InElement, bool bTowardsParent, bool bShouldBeExpanded);
	TSharedRef<ITableRow> MakeTableRowWidget(TSharedPtr<FRigTreeElement> InItem, const TSharedRef<STableViewBase>& OwnerTable, bool bPinned);
	void HandleGetChildrenForTree(TSharedPtr<FRigTreeElement> InItem, TArray<TSharedPtr<FRigTreeElement>>& OutChildren);
	void OnElementKeyTagDragDetected(const FTLLRN_RigElementKey& InDraggedTag);

	TArray<FTLLRN_RigElementKey> GetSelectedKeys() const;
	const TArray<TSharedPtr<FRigTreeElement>>& GetRootElements() const { return RootElements; }
	FRigTreeDelegates& GetRigTreeDelegates() { return Delegates; }

	/** Given a position, return the item under that position. If nothing is there, return null. */
	const TSharedPtr<FRigTreeElement>* FindItemAtPosition(FVector2D InScreenSpacePosition) const;

private:

	void AddConnectorResolveWarningTag(TSharedPtr<FRigTreeElement> InTreeElement, const FTLLRN_RigBaseElement* InTLLRN_RigElement, const UTLLRN_RigHierarchy* InHierarchy);
	FText GetConnectorWarningMessage(TSharedPtr<FRigTreeElement> InTreeElement, TWeakObjectPtr<UTLLRN_ControlRig> InTLLRN_ControlRigPtr, const FTLLRN_RigElementKey InConnectorKey) const;

	/** A temporary snapshot of the SparseItemInfos in STreeView, used during RefreshTreeView() */
	TSparseItemMap OldSparseItemInfos;

	/** Backing array for tree view */
	TArray<TSharedPtr<FRigTreeElement>> RootElements;
	
	/** A map for looking up items based on their key */
	TMap<FTLLRN_RigElementKey, TSharedPtr<FRigTreeElement>> ElementMap;

	/** A map for looking up a parent based on their key */
	TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> ParentMap;

	FRigTreeDelegates Delegates;

	bool bAutoScrollEnabled;
	FVector2D LastMousePosition;
	double TimeAtMousePosition;

	friend class STLLRN_RigHierarchy;
};

class SSearchableTLLRN_RigHierarchyTreeView : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSearchableTLLRN_RigHierarchyTreeView)
		:_MaxHeight(0.f)
	{}
		SLATE_ARGUMENT(FRigTreeDelegates, RigTreeDelegates)
		SLATE_ARGUMENT(FText, InitialFilterText)
		SLATE_ARGUMENT(float, MaxHeight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SSearchableTLLRN_RigHierarchyTreeView() {}
	TSharedRef<SSearchBox> GetSearchBox() const { return SearchBox.ToSharedRef(); }
	TSharedRef<STLLRN_RigHierarchyTreeView> GetTreeView() const { return TreeView.ToSharedRef(); }
	const FRigTreeDisplaySettings& GetDisplaySettings();

private:

	void OnFilterTextChanged(const FText& SearchText);

	FOnGetRigTreeDisplaySettings SuperGetRigTreeDisplaySettings;
	FText FilterText;
	FRigTreeDisplaySettings Settings;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<STLLRN_RigHierarchyTreeView> TreeView;
	float MaxHeight = 0.f;
};
