// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Editor/STLLRN_RigHierarchy.h"
#include "Widgets/Layout/SBox.h"
#include "Rigs/TLLRN_TLLRN_RigSpaceHierarchy.h"
#include "IStructureDetailsView.h"
#include "Misc/FrameNumber.h"
#include "EditorUndoClient.h"
#include "TLLRN_TLLRN_RigSpacePickerBakeSettings.h"

class IMenu;

DECLARE_DELEGATE_RetVal_TwoParams(FTLLRN_RigElementKey, FTLLRN_RigSpacePickerGetActiveSpace, UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey&);
DECLARE_DELEGATE_RetVal_TwoParams(const FTLLRN_RigControlElementCustomization*, FTLLRN_RigSpacePickerGetControlCustomization, UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey&);
DECLARE_EVENT_ThreeParams(STLLRN_RigSpacePickerWidget, FTLLRN_RigSpacePickerActiveSpaceChanged, UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey&, const FTLLRN_RigElementKey&);
DECLARE_EVENT_ThreeParams(STLLRN_RigSpacePickerWidget, FTLLRN_RigSpacePickerSpaceListChanged, UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey&, const TArray<FTLLRN_RigElementKey>&);
DECLARE_DELEGATE_RetVal_TwoParams(TArray<FTLLRN_RigElementKey>, FTLLRN_RigSpacePickerGetAdditionalSpaces, UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey&);
DECLARE_DELEGATE_RetVal_ThreeParams(FReply, STLLRN_RigSpacePickerOnBake, UTLLRN_RigHierarchy*, TArray<FTLLRN_RigElementKey> /* Controls */, FTLLRN_TLLRN_RigSpacePickerBakeSettings);

/** Widget allowing picking of a space source for space switching */
class TLLRN_CONTROLRIGEDITOR_API STLLRN_RigSpacePickerWidget : public SCompoundWidget, public FEditorUndoClient
{
public:

	SLATE_BEGIN_ARGS(STLLRN_RigSpacePickerWidget)
		: _Hierarchy(nullptr)
		, _Controls()
		, _ShowDefaultSpaces(true)
		, _ShowFavoriteSpaces(true)
		, _ShowAdditionalSpaces(true)
		, _AllowReorder(false)
		, _AllowDelete(false)
		, _AllowAdd(false)
		, _ShowBakeAndCompensateButton(false)
		, _Title()
		, _BackgroundBrush(FAppStyle::GetBrush("Menu.Background"))
		{}
		SLATE_ARGUMENT(UTLLRN_RigHierarchy*, Hierarchy)
		SLATE_ARGUMENT(TArray<FTLLRN_RigElementKey>, Controls)
		SLATE_ARGUMENT(bool, ShowDefaultSpaces)
		SLATE_ARGUMENT(bool, ShowFavoriteSpaces)
		SLATE_ARGUMENT(bool, ShowAdditionalSpaces)
		SLATE_ARGUMENT(bool, AllowReorder)
		SLATE_ARGUMENT(bool, AllowDelete)
		SLATE_ARGUMENT(bool, AllowAdd)
		SLATE_ARGUMENT(bool, ShowBakeAndCompensateButton)
		SLATE_ARGUMENT(FText, Title)
		SLATE_ARGUMENT(const FSlateBrush*, BackgroundBrush)
	
		SLATE_EVENT(FTLLRN_RigSpacePickerGetActiveSpace, GetActiveSpace)
		SLATE_EVENT(FTLLRN_RigSpacePickerGetControlCustomization, GetControlCustomization)
		SLATE_EVENT(FTLLRN_RigSpacePickerActiveSpaceChanged::FDelegate, OnActiveSpaceChanged)
		SLATE_EVENT(FTLLRN_RigSpacePickerSpaceListChanged::FDelegate, OnSpaceListChanged)
		SLATE_ARGUMENT(FTLLRN_RigSpacePickerGetAdditionalSpaces, GetAdditionalSpaces)
		SLATE_EVENT(FOnClicked, OnCompensateKeyButtonClicked)
		SLATE_EVENT(FOnClicked, OnCompensateAllButtonClicked)
		SLATE_EVENT(FOnClicked, OnBakeButtonClicked )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STLLRN_RigSpacePickerWidget() override;

	void SetControls(UTLLRN_RigHierarchy* InHierarchy, const TArray<FTLLRN_RigElementKey>& InControls);

	FReply OpenDialog(bool bModal = true);
	void CloseDialog();
	
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;
	virtual bool SupportsKeyboardFocus() const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	UTLLRN_RigHierarchy* GetHierarchy() const
	{
		if (Hierarchy.IsValid())
		{
			return Hierarchy.Get();
		}
		return nullptr;
	}

	const FRigTreeDisplaySettings& GetHierarchyDisplaySettings() const { return HierarchyDisplaySettings; }
	const UTLLRN_RigHierarchy* GetHierarchyConst() const { return GetHierarchy(); }
	
	const TArray<FTLLRN_RigElementKey>& GetControls() const { return ControlKeys; }
	const TArray<FTLLRN_RigElementKey>& GetActiveSpaces() const;
	TArray<FTLLRN_RigElementKey> GetDefaultSpaces() const;
	TArray<FTLLRN_RigElementKey> GetSpaceList(bool bIncludeDefaultSpaces = false) const;
	FTLLRN_RigSpacePickerActiveSpaceChanged& OnActiveSpaceChanged() { return ActiveSpaceChangedEvent; }
	FTLLRN_RigSpacePickerSpaceListChanged& OnSpaceListChanged() { return SpaceListChangedEvent; }
	void RefreshContents();

	// FEditorUndoClient interface
	virtual void PostUndo(bool bSuccess);
	virtual void PostRedo(bool bSuccess);
	// End FEditorUndoClient interface
	
private:

	enum ESpacePickerType
	{
		ESpacePickerType_Parent,
		ESpacePickerType_World,
		ESpacePickerType_Item
	};

	void AddSpacePickerRow(
		TSharedPtr<SVerticalBox> InListBox,
		ESpacePickerType InType,
		const FTLLRN_RigElementKey& InKey,
		const FSlateBrush* InBush,
		const FSlateColor& InColor,
		const FText& InTitle,
		FOnClicked OnClickedDelegate);

	void RepopulateItemSpaces();
	void ClearListBox(TSharedPtr<SVerticalBox> InListBox);
	void UpdateActiveSpaces();
	bool IsValidKey(const FTLLRN_RigElementKey& InKey) const;
	bool IsDefaultSpace(const FTLLRN_RigElementKey& InKey) const;

	FReply HandleParentSpaceClicked();
	FReply HandleWorldSpaceClicked();
	FReply HandleElementSpaceClicked(FTLLRN_RigElementKey InKey);
	FReply HandleSpaceMoveUp(FTLLRN_RigElementKey InKey);
	FReply HandleSpaceMoveDown(FTLLRN_RigElementKey InKey);
	void HandleSpaceDelete(FTLLRN_RigElementKey InKey);

public:
	
	FReply HandleAddElementClicked();
	bool IsRestricted() const;

private:

	bool IsSpaceMoveUpEnabled(FTLLRN_RigElementKey InKey) const;
	bool IsSpaceMoveDownEnabled(FTLLRN_RigElementKey InKey) const;

	void OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);

	FSlateColor GetButtonColor(ESpacePickerType InType, FTLLRN_RigElementKey InKey) const;
	FTLLRN_RigElementKey GetActiveSpace_Private(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey) const;
	TArray<FTLLRN_RigElementKey> GetCurrentParents_Private(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey) const;
	
	FTLLRN_RigSpacePickerActiveSpaceChanged ActiveSpaceChangedEvent;
	FTLLRN_RigSpacePickerSpaceListChanged SpaceListChangedEvent;

	TWeakObjectPtr<UTLLRN_RigHierarchy> Hierarchy;
	TArray<FTLLRN_RigElementKey> ControlKeys;
	TArray<FTLLRN_RigElementKey> CurrentSpaceKeys;
	TArray<FTLLRN_RigElementKey> ActiveSpaceKeys;
	bool bRepopulateRequired;

	bool bShowDefaultSpaces;
	bool bShowFavoriteSpaces;
	bool bShowAdditionalSpaces;
	bool bAllowReorder;
	bool bAllowDelete;
	bool bAllowAdd;
	bool bShowBakeAndCompensateButton;
	bool bLaunchingContextMenu;

	FTLLRN_RigSpacePickerGetControlCustomization GetControlCustomizationDelegate;
	FTLLRN_RigSpacePickerGetActiveSpace GetActiveSpaceDelegate;
	FTLLRN_RigSpacePickerGetAdditionalSpaces GetAdditionalSpacesDelegate; 
	TArray<FTLLRN_RigElementKey> AdditionalSpaces;

	TSharedPtr<SVerticalBox> TopLevelListBox;
	TSharedPtr<SVerticalBox> ItemSpacesListBox;
	TSharedPtr<SHorizontalBox> BottomButtonsListBox;
	TWeakPtr<SWindow> DialogWindow;
	TWeakPtr<IMenu> ContextMenu;
	FDelegateHandle HierarchyModifiedHandle;
	FDelegateHandle ActiveSpaceChangedWindowHandle;
	FRigTreeDisplaySettings HierarchyDisplaySettings;

	static FTLLRN_RigElementKey InValidKey;
};

class ISequencer;

/** Widget allowing baking controls from one space to another */
class TLLRN_CONTROLRIGEDITOR_API STLLRN_RigSpacePickerBakeWidget : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(STLLRN_RigSpacePickerBakeWidget)
	: _Hierarchy(nullptr)
	, _Controls()
	, _Sequencer(nullptr)
	{}
	SLATE_ARGUMENT(UTLLRN_RigHierarchy*, Hierarchy)
	SLATE_ARGUMENT(TArray<FTLLRN_RigElementKey>, Controls)
	SLATE_ARGUMENT(ISequencer*, Sequencer)
	SLATE_EVENT(FTLLRN_RigSpacePickerGetControlCustomization, GetControlCustomization)
	SLATE_ARGUMENT(FTLLRN_TLLRN_RigSpacePickerBakeSettings, Settings)
	SLATE_EVENT(STLLRN_RigSpacePickerOnBake, OnBake)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STLLRN_RigSpacePickerBakeWidget() override {}

	FReply OpenDialog(bool bModal = true);
	void CloseDialog();

private:

	//used for setting up the details
	TSharedPtr<TStructOnScope<FTLLRN_TLLRN_RigSpacePickerBakeSettings>> Settings;

	ISequencer* Sequencer;
	FTLLRN_RigControlElementCustomization Customization;
	
	TWeakPtr<SWindow> DialogWindow;
	TSharedPtr<STLLRN_RigSpacePickerWidget> SpacePickerWidget;
	TSharedPtr<IStructureDetailsView> DetailsView;
};
