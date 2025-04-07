// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "EditorUndoClient.h"

//////////////////////////////////////////////////////////////////////////
// FDisplayedTLLRN_RigCurveInfo

class SInlineEditableTextBlock;
class FTLLRN_ControlRigEditor;
struct FAssetData;
class URigVMBlueprint;
class FRigVMEditor;

class FDisplayedTLLRN_RigCurveInfo
{
public:
	FName CurveName;
	float Value;

	/** Static function for creating a new item, but ensures that you can only have a TSharedRef to one */
	static TSharedRef<FDisplayedTLLRN_RigCurveInfo> Make(const FName& InCurveName)
	{
		return MakeShareable(new FDisplayedTLLRN_RigCurveInfo(InCurveName));
	}

	// editable text
	TSharedPtr<SInlineEditableTextBlock> EditableText;

protected:
	/** Hidden constructor, always use Make above */
	FDisplayedTLLRN_RigCurveInfo(const FName& InCurveName)
		: CurveName(InCurveName)
		, Value( 0 )
	{}
};

typedef TSharedPtr< FDisplayedTLLRN_RigCurveInfo > FDisplayedTLLRN_RigCurveInfoPtr;
typedef SListView< FDisplayedTLLRN_RigCurveInfoPtr > STLLRN_RigCurveListType;

//////////////////////////////////////////////////////////////////////////
// STLLRN_RigCurveListRow

DECLARE_DELEGATE_TwoParams(FSetTLLRN_RigCurveValue, const FName&, float);
DECLARE_DELEGATE_RetVal_OneParam(float, FGetTLLRN_RigCurveValue, const FName&);
DECLARE_DELEGATE_RetVal(FText&, FGetFilterText);

class STLLRN_RigCurveListRow : public SMultiColumnTableRow< FDisplayedTLLRN_RigCurveInfoPtr >
{
public:

	SLATE_BEGIN_ARGS(STLLRN_RigCurveListRow) {}

		/** The item for this row **/
		SLATE_ARGUMENT(FDisplayedTLLRN_RigCurveInfoPtr, Item)

		/** Callback when the text is committed. */
		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)

		/** set the value delegate */
		SLATE_EVENT(FSetTLLRN_RigCurveValue, OnSetTLLRN_RigCurveValue)

		/** set the value delegate */
		SLATE_EVENT(FGetTLLRN_RigCurveValue, OnGetTLLRN_RigCurveValue)

		/** get filter text */
		SLATE_EVENT(FGetFilterText, OnGetFilterText)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView);

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:

	/**
	* Called when the user changes the value of the SSpinBox
	*
	* @param NewValue - The new number the SSpinBox is set to
	* this may not work right if the rig is being evaluated
	* this just overwrite the data when it's written
	*
	*/
	void OnTLLRN_RigCurveValueChanged(float NewValue);

	/**
	* Called when the user types the value and enters
	*
	* @param NewValue - The new number the SSpinBox is set to
	*
	*/
	void OnTLLRN_RigCurveValueValueCommitted(float NewValue, ETextCommit::Type CommitType);

	/** Returns the Value of this curve */
	float GetValue() const;
	/** Returns name of this curve */
	FText GetItemName() const;
	/** Get text we are filtering for */
	FText GetFilterText() const;
	/** Return color for text of item */
	FSlateColor GetItemTextColor() const;

	/** The name and Value of the morph target */
	FDisplayedTLLRN_RigCurveInfoPtr	Item;

	FOnTextCommitted OnTextCommitted;
	FSetTLLRN_RigCurveValue OnSetTLLRN_RigCurveValue;
	FGetTLLRN_RigCurveValue OnGetTLLRN_RigCurveValue;
	FGetFilterText OnGetFilterText;
};

//////////////////////////////////////////////////////////////////////////
// STLLRN_TLLRN_RigCurveContainer

class STLLRN_TLLRN_RigCurveContainer : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS( STLLRN_TLLRN_RigCurveContainer )
	{}
	
	SLATE_END_ARGS()

	/**
	* Slate construction function
	*
	* @param InArgs - Arguments passed from Slate
	*
	*/
	void Construct( const FArguments& InArgs, TSharedRef<FTLLRN_ControlRigEditor> InTLLRN_ControlRigEditor);

	/**
	* Destructor - resets the animation curve
	*
	*/
	virtual ~STLLRN_TLLRN_RigCurveContainer();

	/** SWidget interface */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);

	/**
	* Is registered with TLLRN_ControlRig Editor to handle when its preview mesh is changed.
	*
	* @param NewPreviewMesh - The new preview mesh being used by TLLRN_ControlRig Editor
	*
	*/
	void OnPreviewMeshChanged(class USkeletalMesh* OldPreviewMesh, class USkeletalMesh* NewPreviewMesh);

	/**
	* Is registered with TLLRN_ControlRig Editor to handle when its preview asset is changed.
	*
	* Pose Asset will have to add curve manually
	*/
	void OnPreviewAssetChanged(class UAnimationAsset* NewPreviewAsset);

	/**
	* Filters the SListView when the user changes the search text box (NameFilterBox)
	*
	* @param SearchText - The text the user has typed
	*
	*/
	void OnFilterTextChanged( const FText& SearchText );


	/**
	* Filters the SListView when the user hits enter or clears the search box
	* Simply calls OnFilterTextChanged
	*
	* @param SearchText - The text the user has typed
	* @param CommitInfo - Not used
	*
	*/
	void OnFilterTextCommitted( const FText& SearchText, ETextCommit::Type CommitInfo );

	/**
	* Create a widget for an entry in the tree from an info
	*
	* @param InInfo - Shared pointer to the morph target we're generating a row for
	* @param OwnerTable - The table that owns this row
	*
	* @return A new Slate widget, containing the UI for this row
	*/
	TSharedRef<ITableRow> GenerateTLLRN_RigCurveRow(FDisplayedTLLRN_RigCurveInfoPtr InInfo, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	* Accessor so our rows can grab the filtertext for highlighting
	*
	*/
	FText& GetFilterText() { return FilterText; }

	void RefreshCurveList();

	// When a name is committed after being edited in the list
	virtual void OnNameCommitted(const FText& NewName, ETextCommit::Type CommitType, FDisplayedTLLRN_RigCurveInfoPtr Item);

	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

private:

	void BindCommands();

	/** Handler for context menus */
	TSharedPtr<SWidget> OnGetContextMenuContent() const;

	void OnEditorClose(const FRigVMEditor* InEditor, URigVMBlueprint* InBlueprint);

	/**
	* Clears and rebuilds the table, according to an optional search string
	*
	* @param SearchText - Optional search string
	*
	*/
	void CreateTLLRN_RigCurveList( const FString& SearchText = FString() );

	void OnDeleteNameClicked();
	bool CanDelete();

	void OnRenameClicked();
	bool CanRename();

	void OnAddClicked();

	// Adds a new smartname entry to the skeleton in the container we are managing
	void CreateNewNameEntry(const FText& CommittedText, ETextCommit::Type CommitType);

	// delegate for changing info
	void SetCurveValue(const FName& CurveName, float CurveValue);
	float GetCurveValue(const FName& CurveName);
	void ChangeCurveName(const FName& OldName, const FName& NewName);

	void OnSelectionChanged(FDisplayedTLLRN_RigCurveInfoPtr Selection, ESelectInfo::Type SelectInfo);

	bool bIsChangingTLLRN_RigHierarchy;
	void OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);
	void HandleRefreshEditorFromBlueprint(URigVMBlueprint* InBlueprint);

	// import curve part
	void ImportCurve(const FAssetData& InAssetData);
	void CreateImportMenu(FMenuBuilder& MenuBuilder);
	bool ShouldFilterOnImport(const FAssetData& AssetData) const;

	/** Box to filter to a specific morph target name */
	TSharedPtr<SSearchBox>	NameFilterBox;

	/** A list of animation curve. Used by the TLLRN_RigCurveListView. */
	TArray< FDisplayedTLLRN_RigCurveInfoPtr > TLLRN_RigCurveList;

	/** Widget used to display the list of animation curve */
	TSharedPtr<STLLRN_RigCurveListType> TLLRN_RigCurveListView;

	/** Control Rig Blueprint */
	TWeakObjectPtr<UTLLRN_ControlRigBlueprint> TLLRN_ControlRigBlueprint;
	TWeakPtr<FTLLRN_ControlRigEditor> TLLRN_ControlRigEditor;

	/** Current text typed into NameFilterBox */
	FText FilterText;

	/** Commands that are bound to delegates*/
	TSharedPtr<FUICommandList> UICommandList;

	UTLLRN_RigHierarchy* GetHierarchy() const;
	UTLLRN_RigHierarchy* GetInstanceHierarchy() const;

	friend class STLLRN_RigCurveListRow;
	friend class STLLRN_RigCurveTypeList;
};