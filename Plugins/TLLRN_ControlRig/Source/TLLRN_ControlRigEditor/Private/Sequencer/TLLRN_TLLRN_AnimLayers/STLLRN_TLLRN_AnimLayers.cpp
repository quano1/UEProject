// Copyright Epic Games, Inc. All Rights Reserved.
#include "STLLRN_TLLRN_AnimLayers.h"
#include "TLLRN_TLLRN_AnimLayers.h"
#include "Templates/SharedPointer.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STreeView.h"
#include "SPositiveActionButton.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "DetailWidgetRow.h"
#include <DetailsNameWidgetOverrideCustomization.h>
#include "IDetailCustomization.h"
#include "IDetailCustomNodeBuilder.h"
#include "Containers/Set.h"
#include "PropertyPath.h"
#include "Widgets/SWidget.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "Toolkits/IToolkitHost.h"
#include "EditorModeManager.h"
#include "TLLRN_ControlRig.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "MovieScene.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "SceneOutlinerPublicTypes.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "Sequencer/TLLRN_ControlRigParameterTrackEditor.h"
#include "Algo/IndexOf.h"

#define LOCTEXT_NAMESPACE "TLLRN_TLLRN_AnimLayers"

struct FTLLRN_AnimLayerSourceUIEntry;

typedef TSharedPtr<FTLLRN_AnimLayerSourceUIEntry> FTLLRN_AnimLayerSourceUIEntryPtr;

namespace TLLRN_AnimLayerSourceListUI
{
	static const FName LayerColumnName(TEXT("Layer"));
	static const FName StatusColumnName(TEXT("Status"));
	static const FName WeightColumnName(TEXT("Weight"));
	static const FName TypeColumnName(TEXT("Type"));
};

// Structure that defines a single entry in the source UI
struct FTLLRN_AnimLayerSourceUIEntry
{
public:
	FTLLRN_AnimLayerSourceUIEntry(UTLLRN_AnimLayer* InTLLRN_AnimLayer): TLLRN_AnimLayer(InTLLRN_AnimLayer)
	{}

	void SetSelectedInList(bool bInValue);
	bool GetSelectedInList() const;

	void SelectObjects() const;
	void AddSelected() const;
	void RemoveSelected() const;
	void Duplicate() const;
	void DeleteTLLRN_AnimLayer() const;
	void SetPassthroughKey() const;

	ECheckBoxState GetKeyed() const;
	void SetKeyed();
	FReply OnKeyedColor();
	FSlateColor GetKeyedColor() const;

	ECheckBoxState GetSelected() const;
	void SetSelected(bool bInSelected);

	bool GetActive() const;
	void SetActive(bool bInActive);

	bool GetLock() const;
	void SetLock(bool bInLock);

	FText GetName() const;
	void SetName(const FText& InName);

	double GetWeight() const;
	void SetWeight(double InWeight);

	ETLLRN_AnimLayerType GetType() const;
	FText GetTypeToText() const;
	void SetType(ETLLRN_AnimLayerType InType);

	UObject* GetWeightObject() const;

	int32 GetTLLRN_AnimLayerIndex(UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers) const;
	void ClearCaches() const { bSelectionStateValid = false; bKeyedStateIsValid = false; }
private:
	//cached values
	mutable bool bSelectionStateValid = false;
	mutable bool bKeyedStateIsValid = false;
	mutable ECheckBoxState SelectionState = ECheckBoxState::Unchecked;
	mutable ECheckBoxState KeyedState = ECheckBoxState::Unchecked;

	UTLLRN_AnimLayer* TLLRN_AnimLayer = nullptr;
};

int32 FTLLRN_AnimLayerSourceUIEntry::GetTLLRN_AnimLayerIndex(UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers) const
{
	if (TLLRN_TLLRN_AnimLayers && TLLRN_AnimLayer)
	{
		return TLLRN_TLLRN_AnimLayers->TLLRN_TLLRN_AnimLayers.Find(TLLRN_AnimLayer);
	}
	return INDEX_NONE;
}

UObject* FTLLRN_AnimLayerSourceUIEntry::GetWeightObject() const
{
	if (TLLRN_AnimLayer)
	{
		return TLLRN_AnimLayer->WeightProxy.Get();
	}
	return nullptr;
}

void FTLLRN_AnimLayerSourceUIEntry::SelectObjects() const
{
	if (TLLRN_AnimLayer)
	{
		TLLRN_AnimLayer->SetSelected(true, !(FSlateApplication::Get().GetModifierKeys().IsShiftDown()));
		ClearCaches();
	}
}

void FTLLRN_AnimLayerSourceUIEntry::AddSelected() const
{
	if (TLLRN_AnimLayer)
	{
		TLLRN_AnimLayer->AddSelectedInSequencer();
		ClearCaches();
	}
}

void FTLLRN_AnimLayerSourceUIEntry::RemoveSelected() const
{
	if (TLLRN_AnimLayer)
	{
		TLLRN_AnimLayer->RemoveSelectedInSequencer();
		ClearCaches();
	}
}

void FTLLRN_AnimLayerSourceUIEntry::DeleteTLLRN_AnimLayer() const
{
	if (TLLRN_AnimLayer)
	{
		if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
		{
			if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
			{
				int32 Index = TLLRN_TLLRN_AnimLayers->GetTLLRN_AnimLayerIndex(TLLRN_AnimLayer);
				if (Index != INDEX_NONE)
				{
					TLLRN_TLLRN_AnimLayers->DeleteTLLRN_AnimLayer(SequencerPtr.Get(), Index);
				}
			}
		}
		ClearCaches();
	}
}
void FTLLRN_AnimLayerSourceUIEntry::Duplicate() const
{
	if (TLLRN_AnimLayer)
	{
		if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
		{
			if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
			{
				int32 Index = TLLRN_TLLRN_AnimLayers->GetTLLRN_AnimLayerIndex(TLLRN_AnimLayer);
				if (Index != INDEX_NONE)
				{
					TLLRN_TLLRN_AnimLayers->DuplicateTLLRN_AnimLayer(SequencerPtr.Get(), Index);
				}
			}
		}
		ClearCaches();
	}
}

void FTLLRN_AnimLayerSourceUIEntry::SetPassthroughKey() const
{
	if (TLLRN_AnimLayer)
	{
		if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
		{
			if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
			{
				int32 Index = TLLRN_TLLRN_AnimLayers->GetTLLRN_AnimLayerIndex(TLLRN_AnimLayer);
				if (Index != INDEX_NONE)
				{
					TLLRN_TLLRN_AnimLayers->SetPassthroughKey(SequencerPtr.Get(), Index);
				}
			}
		}
		ClearCaches();
	}
}


ECheckBoxState FTLLRN_AnimLayerSourceUIEntry::GetKeyed() const
{
	if (bKeyedStateIsValid == false)
	{
		bKeyedStateIsValid = true;
		if (TLLRN_AnimLayer)
		{
			KeyedState = TLLRN_AnimLayer->GetKeyed();
		}
	}
	return KeyedState;
}

void FTLLRN_AnimLayerSourceUIEntry::SetKeyed()
{
	if (TLLRN_AnimLayer)
	{
		bKeyedStateIsValid = false;
		TLLRN_AnimLayer->SetKeyed();
	}
}

FReply FTLLRN_AnimLayerSourceUIEntry::OnKeyedColor()
{
	if (TLLRN_AnimLayer)
	{
		const FScopedTransaction Transaction(LOCTEXT("SetKeyed_Transaction", "Set Keyed"), !GIsTransacting);
		TLLRN_AnimLayer->SetKeyed();
	}
	return FReply::Handled();
}

FSlateColor FTLLRN_AnimLayerSourceUIEntry::GetKeyedColor() const
{
	ECheckBoxState State = GetKeyed();
	switch (State)
	{
		case ECheckBoxState::Undetermined:
		{
			return (FLinearColor::Green /2);
		};
		case ECheckBoxState::Checked:
		{
			return FLinearColor::Green;
		};
	}
	return FLinearColor::Transparent;
}

ECheckBoxState FTLLRN_AnimLayerSourceUIEntry::GetSelected() const
{
	if (bSelectionStateValid == false)
	{
		bSelectionStateValid = true;
		if (TLLRN_AnimLayer)
		{
			SelectionState = TLLRN_AnimLayer->GetSelected();
		}
	}
	return SelectionState;
}

void FTLLRN_AnimLayerSourceUIEntry::SetSelected(bool bInSelected)
{
	if (TLLRN_AnimLayer)
	{
		bSelectionStateValid = false;
		TLLRN_AnimLayer->SetSelected(bInSelected, !(FSlateApplication::Get().GetModifierKeys().IsShiftDown()));
	}
}

void FTLLRN_AnimLayerSourceUIEntry::SetSelectedInList(bool bInValue)
{ 
	if (TLLRN_AnimLayer)
	{
		TLLRN_AnimLayer->SetSelectedInList(bInValue);
		if (bInValue)
		{
			TLLRN_AnimLayer->SetKeyed();//selection also sets keyed
		}
	}
}

bool FTLLRN_AnimLayerSourceUIEntry::GetSelectedInList() const 
{ 
	if (TLLRN_AnimLayer)
	{
		return TLLRN_AnimLayer->GetSelectedInList();
	}
	return false;
}

bool FTLLRN_AnimLayerSourceUIEntry::GetActive() const
{
	if (TLLRN_AnimLayer)
	{
		return TLLRN_AnimLayer->GetActive();
	}
	return false;
}

void FTLLRN_AnimLayerSourceUIEntry::SetActive(bool bInActive)
{
	if (TLLRN_AnimLayer)
	{
		TLLRN_AnimLayer->SetActive(bInActive);
	}
}

bool FTLLRN_AnimLayerSourceUIEntry::GetLock() const
{
	if (TLLRN_AnimLayer)
	{
		return TLLRN_AnimLayer->GetLock();
	}
	return false;
}

void FTLLRN_AnimLayerSourceUIEntry::SetLock(bool bInLock)
{
	if (TLLRN_AnimLayer)
	{
		TLLRN_AnimLayer->SetLock(bInLock);
	}
}

FText FTLLRN_AnimLayerSourceUIEntry::GetName() const
{
	if (TLLRN_AnimLayer)
	{
		return TLLRN_AnimLayer->GetName();
	}
	return FText();
}

void FTLLRN_AnimLayerSourceUIEntry::SetName(const FText& InName)
{
	if (TLLRN_AnimLayer)
	{
		TLLRN_AnimLayer->SetName(InName);
	}
}

double FTLLRN_AnimLayerSourceUIEntry::GetWeight() const
{
	if (TLLRN_AnimLayer)
	{
		return TLLRN_AnimLayer->GetWeight();
	}
	return 0.0;
}

void FTLLRN_AnimLayerSourceUIEntry::SetWeight(double InWeight)
{
	if (TLLRN_AnimLayer)
	{
		TLLRN_AnimLayer->SetWeight(InWeight);
	}
}

ETLLRN_AnimLayerType FTLLRN_AnimLayerSourceUIEntry::GetType() const
{
	if (TLLRN_AnimLayer)
	{
		return TLLRN_AnimLayer->GetType();
	}
	return ETLLRN_AnimLayerType::Base;
}


void FTLLRN_AnimLayerSourceUIEntry::SetType(ETLLRN_AnimLayerType InType)
{
	if (TLLRN_AnimLayer)
	{
		TLLRN_AnimLayer->SetType(InType);
	}
}

FText FTLLRN_AnimLayerSourceUIEntry::GetTypeToText() const
{
	if (TLLRN_AnimLayer)
	{
		return TLLRN_AnimLayer->State.TLLRN_AnimLayerTypeToText();
	}
	return FText();
}

namespace UE::TLLRN_TLLRN_AnimLayers
{
	/** Handles deletion */
	template <typename ListType, typename ListElementType>
	class STLLRN_TLLRN_AnimLayersBaseListView : public ListType
	{
	public:
		virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
		{
			if (InKeyEvent.GetKey() == EKeys::Delete || InKeyEvent.GetKey() == EKeys::BackSpace)
			{
				TArray<ListElementType> SelectedItem = ListType::GetSelectedItems();
				for (ListElementType Item : SelectedItem)
				{
					Item->DeleteTLLRN_AnimLayer();
				}
				
				return FReply::Handled();
			}

			return FReply::Unhandled();
		}
	};
}

class STLLRN_AnimLayerListView : public UE::TLLRN_TLLRN_AnimLayers::STLLRN_TLLRN_AnimLayersBaseListView<STreeView<FTLLRN_AnimLayerSourceUIEntryPtr>, FTLLRN_AnimLayerSourceUIEntryPtr>
{
public:
	void Construct(const FArguments& InArgs)
	{
		UE::TLLRN_TLLRN_AnimLayers::STLLRN_TLLRN_AnimLayersBaseListView<STreeView<FTLLRN_AnimLayerSourceUIEntryPtr>, FTLLRN_AnimLayerSourceUIEntryPtr>::Construct(InArgs);
	}
};

class FTLLRN_AnimLayerSourcesView : public TSharedFromThis<FTLLRN_AnimLayerSourcesView>
{
public:
	FTLLRN_AnimLayerSourcesView();

	// Gather information about all sources and update the list view 
	void RefreshSourceData(bool bRefreshUI);
	// Handler that creates a widget row for a given ui entry
	TSharedRef<ITableRow> MakeSourceListViewWidget(FTLLRN_AnimLayerSourceUIEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable) const;
	// Handles constructing a context menu for the sources
	TSharedPtr<SWidget> OnSourceConstructContextMenu();;
	// Handle selection change, triggering the OnSourceSelectionChangedDelegate delegate.
	void OnSourceListSelectionChanged(FTLLRN_AnimLayerSourceUIEntryPtr Entry, ESelectInfo::Type SelectionType) const;
	// Focus on the added layer name
	void FocusRenameLayer(int32 Index)
	{
		FocusOnIndex = Index;
	}

	void AddController(FTLLRN_AnimLayerController* InController);

	void RenameItem(int32 Index) const;
private:
	// Create the sources list view
	void CreateSourcesListView();

	mutable int32  FocusOnIndex = INDEX_NONE;

	void AddSelected();
	void RemoveSelected();
	void SelectObjects();
	void Duplicate();
	void MergeLayers();
	void AdjustmentBlend();
	void SetPassthroughKey();
	void DeleteTLLRN_AnimLayer();
	void Rename();

public:
	// Holds the data that will be displayed in the list view
	TArray<FTLLRN_AnimLayerSourceUIEntryPtr> SourceData;
	// Holds the sources list view widget
	TSharedPtr<STLLRN_AnimLayerListView> SourcesListView;
	//pointer to controller safe to be raw since it's the parent.
	FTLLRN_AnimLayerController* Controller = nullptr;

};

struct  FTLLRN_AnimLayerController : public TSharedFromThis<FTLLRN_AnimLayerController>
{
public:
	FTLLRN_AnimLayerController();
	~FTLLRN_AnimLayerController();
private:
	bool bSelectionFilterActive = false;

private:

	void RebuildSourceList();

	// Handles source selection changing.
	void OnSourceSelectionChangedHandler(FTLLRN_AnimLayerSourceUIEntryPtr Entry, ESelectInfo::Type SelectionType) const;
public:
	// Handles the source collection changing.
	void RefreshSourceData(bool bRefreshUI);
	void RefreshTimeDependantData();
	void RefreshSelectionData();
	void HandleOnTLLRN_AnimLayerListChanged(UTLLRN_TLLRN_AnimLayers*) { RebuildSourceList(); }
	void FocusRenameLayer(int32 Index) { if (SourcesView.IsValid()) SourcesView->FocusRenameLayer(Index); }
	void SelectItem(int32 Index, bool bClear = true);
	void ToggleSelectionFilterActive();
	bool IsSelectionFilterActive() const;
public:
	// Sources view
	TSharedPtr<FTLLRN_AnimLayerSourcesView> SourcesView;
	// Guard from reentrant selection
	mutable bool bSelectionChangedGuard = false;
};


class STLLRN_AnimLayerSourcesRow : public SMultiColumnTableRow<FTLLRN_AnimLayerSourceUIEntryPtr>
{
public:
	SLATE_BEGIN_ARGS(STLLRN_AnimLayerSourcesRow) {}
	/** The list item for this row */
	SLATE_ARGUMENT(FTLLRN_AnimLayerSourceUIEntryPtr, Entry)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		EntryPtr = Args._Entry;
		LayerTypeTextList.Emplace(new FText(LOCTEXT("Additive", "Additive")));
		LayerTypeTextList.Emplace(new FText(LOCTEXT("Override", "Override")));
		SMultiColumnTableRow<FTLLRN_AnimLayerSourceUIEntryPtr>::Construct(
			FSuperRowType::FArguments()
			.Padding(2.f),
			OwnerTableView
		);
	}

	// Static Source UI FNames
	void BeginEditingName()
	{
		if (LayerNameTextBlock.IsValid())
		{
			LayerNameTextBlock->EnterEditingMode();
		}
	}

	FSlateColor GetKeyedColor() const
	{
		if (EntryPtr)
		{
			return EntryPtr->GetKeyedColor();
		}
		return FLinearColor::Transparent;
	}

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the list view. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == TLLRN_AnimLayerSourceListUI::LayerColumnName)
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.MaxWidth(6)
				[
					SNew(SBox)
						.WidthOverride(6)
						[
							SNew(SButton)
								.ContentPadding(0)
								.VAlign(VAlign_Fill)
								.IsFocusable(true)
								.IsEnabled(true)
								.ButtonStyle(FAppStyle::Get(), "Sequencer.AnimationOutliner.ColorStrip")
								.OnClicked_Lambda([this]()
									{
										return EntryPtr->OnKeyedColor();
									})
								.Content()
								[
									SNew(SImage)
										.Image(FAppStyle::GetBrush("WhiteBrush"))
										.ColorAndOpacity(this, &STLLRN_AnimLayerSourcesRow::GetKeyedColor)
								]
						]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 0)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.IsFocusable(false)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ButtonColorAndOpacity_Lambda([this]()
						{
							return FLinearColor::White;
						})
					.OnClicked_Lambda([this]()
						{
							bool bValue = EntryPtr->GetSelected() == ECheckBoxState::Unchecked ? false : true;
							EntryPtr->SetSelected(!bValue);
							return FReply::Handled();
						})
					.ContentPadding(1.f)
					.ToolTipText(LOCTEXT("TLLRN_AnimLayerSelectionTooltip", "Selection In Layer"))
					[
						SNew(SImage)
							.ColorAndOpacity_Lambda([this]()
								{
									const FColor Selected(38, 187, 255);
									const FColor NotSelected(56, 56, 56);
									//FLinearColor::White;
									bool bValue = EntryPtr->GetSelected() == ECheckBoxState::Unchecked ? false : true;
									FSlateColor SlateColor;
									if (bValue == true)
									{
										if (EntryPtr->GetSelectedInList())
										{
											SlateColor = FLinearColor::White;
										}
										else
										{
											SlateColor = FSlateColor(Selected);
										}										
									}
									else
									{
										SlateColor = FSlateColor(NotSelected);
									}
									return SlateColor;
								})
							.Image(FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_ControlRig.TLLRN_AnimLayerSelected").GetIcon())
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(10.0)
				.Padding(10.0,0)
				[	SNew(SBox)
					.HAlign(EHorizontalAlignment::HAlign_Left)
					[
						SAssignNew(LayerNameTextBlock, SInlineEditableTextBlock)
							.Justification(ETextJustify::Center)
							.Text_Lambda([this]()
								{
									return EntryPtr->GetName();
								})
							.OnTextCommitted(this, &STLLRN_AnimLayerSourcesRow::OnLayerNameCommited)
					]
				];
		}
		else if (ColumnName == TLLRN_AnimLayerSourceListUI::StatusColumnName)
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(1, 0)
					.VAlign(VAlign_Center)
					[
						SAssignNew(MuteButton, SButton)
						.IsFocusable(false)
						.ButtonStyle(FAppStyle::Get(), "NoBorder")
						.ButtonColorAndOpacity_Lambda([this] ()
						{
							const bool bIsActive = !EntryPtr->GetActive();
							const bool bIsHovered = MuteButton->IsHovered();
							return GetStatusImageColorAndOpacity(bIsActive, bIsHovered);		
						})
						.OnClicked_Lambda([this]()
						{
							bool bValue = EntryPtr->GetActive();
							EntryPtr->SetActive(!bValue);
							return FReply::Handled();
						})
						.ContentPadding(1.f)
						.ToolTipText(LOCTEXT("TLLRN_AnimLayerMuteTooltip", "Mute Layer"))
						[
							SNew(SImage)
								.ColorAndOpacity_Lambda([this]() 
								{ 
									const bool bIsActive = !EntryPtr->GetActive();
									const bool bIsHovered = MuteButton->IsHovered();
									return GetStatusImageColorAndOpacity(bIsActive, bIsHovered);
								})
								.Image(FAppStyle::GetBrush("Sequencer.Column.Mute"))
						]
					]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(1, 0)
					.VAlign(VAlign_Center)
					[
						SAssignNew(LockButton, SButton)
						.IsFocusable(false)
						.ButtonStyle(FAppStyle::Get(), "NoBorder")
						.ButtonColorAndOpacity_Lambda([this] ()
						{
							const bool bIsLock = EntryPtr->GetLock();
							const bool bIsHovered = LockButton->IsHovered();
							return GetStatusImageColorAndOpacity(bIsLock, bIsHovered);
						})
						.OnClicked_Lambda([this]()
						{
							bool bValue = EntryPtr->GetLock();
							EntryPtr->SetLock(!bValue);
							return FReply::Handled();
						})
						.ContentPadding(1.f)
						.ToolTipText(LOCTEXT("TLLRN_AnimLayerLockTooltip", "Lock Layer"))
						[
							SNew(SImage)
								.ColorAndOpacity_Lambda([this]() 
								{ 
									const bool bIsLock = EntryPtr->GetLock();
									const bool bIsHovered = LockButton->IsHovered();
									return GetStatusImageColorAndOpacity(bIsLock, bIsHovered);
								})
							.Image(FAppStyle::GetBrush("Sequencer.Column.Locked"))
						]
					];


		}
		else if (ColumnName == TLLRN_AnimLayerSourceListUI::WeightColumnName)
		{
			UObject* WeightObject = nullptr;
			FTLLRN_ControlRigEditMode* EditMode = nullptr;
			if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
			{
				WeightObject = EntryPtr->GetWeightObject();

				TSharedPtr<IToolkitHost> ToolkitHost = SequencerPtr->GetToolkitHost();
				if (ToolkitHost.IsValid())
				{
					FEditorModeTools& EditorModeTools = ToolkitHost->GetEditorModeManager();			
					if (!EditorModeTools.IsModeActive(FTLLRN_ControlRigEditMode::ModeName))
					{
						EditorModeTools.ActivateMode(FTLLRN_ControlRigEditMode::ModeName);

						EditMode = static_cast<FTLLRN_ControlRigEditMode*>(EditorModeTools.GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
						if (EditMode && EditMode->GetToolkit().IsValid() == false)
						{
							EditMode->Enter();
						}
					}
					EditMode = static_cast<FTLLRN_ControlRigEditMode*>(EditorModeTools.GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));	
				}
			}
			return SAssignNew(WeightDetails, SAnimWeightDetails, EditMode, WeightObject);
			
		}
		else if (ColumnName == TLLRN_AnimLayerSourceListUI::TypeColumnName)
		{
			if(EntryPtr->GetType() != (int32)(ETLLRN_AnimLayerType::Base))
			{ 
				return SAssignNew(LayerTypeCombo, SComboBox<TSharedPtr<FText>>)
					.ContentPadding(FMargin(10.0f, 2.0f))
					.OptionsSource(&LayerTypeTextList)
					.HasDownArrow(false)
					.OnGenerateWidget_Lambda([this](TSharedPtr<FText> Item)
					{
						return SNew(SBox)
							.MaxDesiredWidth(100.0f)
							[
								SNew(STextBlock)
								.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
								.Text(*Item)
							];
					})
					.OnSelectionChanged(this, &STLLRN_AnimLayerSourcesRow::OnLayerTypeChanged)
					[
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
						.Text(this, &STLLRN_AnimLayerSourcesRow::GetLayerTypeText)
					];
			}
			else
			{
				return SNullWidget::NullWidget;
			}
		}

		return SNullWidget::NullWidget;
	}

private:
	
	FSlateColor GetStatusImageColorAndOpacity(bool bIsActive, bool bIsHovered) const
	{
		FLinearColor OutColor = FLinearColor::White;
		float Opacity = 0.0f;

		if (bIsActive)
		{
			// Directly active, full opacity
			Opacity = 1.0f;
		}
		else if (bIsHovered)
		{
			// Mouse is over widget and it is not directly active.
			Opacity = .8f;
		}
		else 
		{
			// Not active in any way and mouse is not over widget or item.
			Opacity = 0.1f;
		}
		OutColor.A = Opacity;
		return OutColor;
	}


	void OnLayerNameCommited(const FText& InNewText, ETextCommit::Type InTextCommit)
	{
		if (InNewText.IsEmpty())
		{
			return;
		}
		EntryPtr->SetName(InNewText);
	}

	FText GetLayerTypeText() const
	{
		return EntryPtr->GetTypeToText();
	}
	void OnLayerTypeChanged(TSharedPtr<FText> InNewText, ESelectInfo::Type SelectInfo)
	{
		if (InNewText.IsValid() == false || InNewText->IsEmpty())
		{
			return;
		}
		FText Additive(LOCTEXT("Additive", "Additive"));
		FText Override(LOCTEXT("Override", "Override"));
		if (InNewText->IdenticalTo(Additive))
		{
			EntryPtr->SetType(ETLLRN_AnimLayerType::Additive);
		}
		else if (InNewText->IdenticalTo(Override))
		{
			EntryPtr->SetType(ETLLRN_AnimLayerType::Override);
		}
	}


private:
	FTLLRN_AnimLayerSourceUIEntryPtr EntryPtr;
	TSharedPtr<SInlineEditableTextBlock> LayerNameTextBlock;
	mutable TArray<TSharedPtr<FText>> LayerTypeTextList;
	TSharedPtr<SComboBox<TSharedPtr<FText>>> LayerTypeCombo;
	//TSharedPtr<SNumericEntryBox<double>> WeightBox;
	TSharedPtr< SAnimWeightDetails> WeightDetails;
	TSharedPtr<SButton> MuteButton;
	TSharedPtr<SButton> LockButton;

};

FTLLRN_AnimLayerSourcesView::FTLLRN_AnimLayerSourcesView()
{
	CreateSourcesListView();
}

void FTLLRN_AnimLayerSourcesView::AddController(FTLLRN_AnimLayerController* InController)
{
	Controller = InController;
}

TSharedRef<ITableRow> FTLLRN_AnimLayerSourcesView::MakeSourceListViewWidget(FTLLRN_AnimLayerSourceUIEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable) const
{
	if (FocusOnIndex != INDEX_NONE)
	{
		int32 NextIndex = FocusOnIndex;
		FocusOnIndex = INDEX_NONE;
		GEditor->GetTimerManager()->SetTimerForNextTick([this, NextIndex]()
			{
				if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
				{
					if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
					{
						for (int32 Index = 0; Index < SourceData.Num(); ++Index)
						{
							const FTLLRN_AnimLayerSourceUIEntryPtr& Item = SourceData[Index];
							if (Item->GetTLLRN_AnimLayerIndex(TLLRN_TLLRN_AnimLayers) == NextIndex)
							{
								RenameItem(Index);
								FocusOnIndex = INDEX_NONE;
							}
						}
					}
				}
			});
	}
	return SNew(STLLRN_AnimLayerSourcesRow, OwnerTable)
		.Entry(Entry);
}

void FTLLRN_AnimLayerSourcesView::RenameItem(int32 Index) const
{
	if (Index < SourceData.Num())
	{
		if (TSharedPtr<ITableRow> Row = SourcesListView->WidgetFromItem(SourceData[Index]))
		{
			TWeakPtr<STLLRN_AnimLayerSourcesRow> Widget = StaticCastSharedRef<STLLRN_AnimLayerSourcesRow>(Row->AsWidget());
			if (Widget.IsValid())
			{
				Widget.Pin()->BeginEditingName();
			}
		}
	}
}

void FTLLRN_AnimLayerSourcesView::CreateSourcesListView()
{

	SAssignNew(SourcesListView, STLLRN_AnimLayerListView)
	//	.ListItemsSource(&SourceData)
		.TreeItemsSource(&SourceData)
		.OnGetChildren_Lambda([](FTLLRN_AnimLayerSourceUIEntryPtr ChannelItem, TArray<FTLLRN_AnimLayerSourceUIEntryPtr>& OutChildren)
		{
		})
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow_Raw(this, &FTLLRN_AnimLayerSourcesView::MakeSourceListViewWidget)
		.OnContextMenuOpening_Raw(this, &FTLLRN_AnimLayerSourcesView::OnSourceConstructContextMenu)
		.OnSelectionChanged_Raw(this, &FTLLRN_AnimLayerSourcesView::OnSourceListSelectionChanged)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(TLLRN_AnimLayerSourceListUI::LayerColumnName)
			.FillWidth(120.f)
			.DefaultLabel(LOCTEXT("LayerColumnName", "Name"))
			+ SHeaderRow::Column(TLLRN_AnimLayerSourceListUI::StatusColumnName)
			.FillWidth(40.f)
			.DefaultLabel(LOCTEXT("StatusColumnName", "Status"))
			+ SHeaderRow::Column(TLLRN_AnimLayerSourceListUI::WeightColumnName)
			.FillWidth(80.f)
			.DefaultLabel(LOCTEXT("WeightColumnName", "Weight"))
			+ SHeaderRow::Column(TLLRN_AnimLayerSourceListUI::TypeColumnName)
			.FillWidth(80.f)
			.DefaultLabel(LOCTEXT("TypeColumnName", "Type"))
		);
}

void FTLLRN_AnimLayerSourcesView::AddSelected()
{
	TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
	SourcesListView->GetSelectedItems(Selected);
	if (Selected.Num() > 0)
	{
		const FScopedTransaction Transaction(LOCTEXT("AddSelectedTLLRN_AnimLayer_Transaction", "Add Selected"), !GIsTransacting);
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Ptr : Selected)
		{
			Ptr->AddSelected();
		}
	}
}
void FTLLRN_AnimLayerSourcesView::RemoveSelected()
{
	TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
	SourcesListView->GetSelectedItems(Selected);
	if (Selected.Num() > 0)
	{
		const FScopedTransaction Transaction(LOCTEXT("RemoveSelected_Transaction", "Remove Selected"), !GIsTransacting);
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Ptr : Selected)
		{
			Ptr->RemoveSelected();
		}
	}
}

void FTLLRN_AnimLayerSourcesView::SelectObjects()
{
	TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
	SourcesListView->GetSelectedItems(Selected);
	if (Selected.Num() > 0)
	{
		const FScopedTransaction Transaction(LOCTEXT("SetSelected_Transaction", "Set Selection"), !GIsTransacting);
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Ptr : Selected)
		{
			Ptr->SelectObjects();
		}
	}
}

void FTLLRN_AnimLayerSourcesView::DeleteTLLRN_AnimLayer()
{
	TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
	SourcesListView->GetSelectedItems(Selected);
	if (Selected.Num() > 0)
	{
		const FScopedTransaction Transaction(LOCTEXT("DeleteTLLRN_AnimLayer_Transaction", "Delete Anim Layer"), !GIsTransacting);
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Ptr : Selected)
		{
			Ptr->DeleteTLLRN_AnimLayer();
		}
	}
}
void FTLLRN_AnimLayerSourcesView::Duplicate()
{
	TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
	SourcesListView->GetSelectedItems(Selected);
	if (Selected.Num() > 0)
	{
		const FScopedTransaction Transaction(LOCTEXT("DuplicateTLLRN_AnimLayer_Transaction", "Duplicate Anim Layer"), !GIsTransacting);
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Ptr : Selected)
		{
			Ptr->Duplicate();
		}
	}
}

void FTLLRN_AnimLayerSourcesView::AdjustmentBlend()
{
	if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
		{
			TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
			SourcesListView->GetSelectedItems(Selected);
			if (Selected.Num() != 1)
			{
				return;
			}
			int32 Index = Selected[0]->GetTLLRN_AnimLayerIndex(TLLRN_TLLRN_AnimLayers);
			if (Index != INDEX_NONE && Index != 0) //not base
			{
				TLLRN_TLLRN_AnimLayers->AdjustmentBlendLayers(SequencerPtr.Get(), Index);
			}
		}
	}
}

void FTLLRN_AnimLayerSourcesView::MergeLayers()
{
	if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
		{
			TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
			SourcesListView->GetSelectedItems(Selected);
			if (Selected.Num() < 2)
			{
				return;
			}
			TArray<int32> LayersToMerge;
			for (const FTLLRN_AnimLayerSourceUIEntryPtr& Ptr : Selected)
			{
				int32 Index = Ptr->GetTLLRN_AnimLayerIndex(TLLRN_TLLRN_AnimLayers);
				if (Index != INDEX_NONE)
				{
					LayersToMerge.Add(Index);
				}
			}
			if (LayersToMerge.Num() > 1)
			{
				TLLRN_TLLRN_AnimLayers->MergeTLLRN_TLLRN_AnimLayers(SequencerPtr.Get(), LayersToMerge, nullptr);
				//for now we don't use the dialog since it's not doing proper per key frame reductions
				/*
				FCollapseControlsCB CollapseCB = FCollapseControlsCB::CreateLambda([TLLRN_TLLRN_AnimLayers,LayersToMerge](TSharedPtr<ISequencer>& InSequencer,const FBakingAnimationKeySettings& InSettings)
				{
					TLLRN_TLLRN_AnimLayers->MergeTLLRN_TLLRN_AnimLayers(InSequencer.Get(), LayersToMerge, InSettings);

				});

				TSharedRef<SCollapseControlsWidget> BakeWidget =
					SNew(SCollapseControlsWidget)
					.Sequencer(SequencerPtr);

				BakeWidget->SetCollapseCB(CollapseCB);
				BakeWidget->OpenDialog(false);
				*/
			}
		}
	}
}

void FTLLRN_AnimLayerSourcesView::SetPassthroughKey()
{
	TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
	SourcesListView->GetSelectedItems(Selected);
	if (Selected.Num() > 0)
	{
		const FScopedTransaction Transaction(LOCTEXT("SetPassthroughKey_Transaction", "Set Passthrough Key"), !GIsTransacting);
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Ptr : Selected)
		{
			Ptr->SetPassthroughKey();
		}
	}
}

void FTLLRN_AnimLayerSourcesView::Rename()
{
	TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
	SourcesListView->GetSelectedItems(Selected);
	if (Selected.Num() == 1)
	{
		int32 Index = INDEX_NONE;
		if (SourceData.Find(Selected[0], Index))
		{
			if (Index != INDEX_NONE)
			{
				RenameItem(Index);
			}
		}
	}
}

TSharedPtr<SWidget> FTLLRN_AnimLayerSourcesView::OnSourceConstructContextMenu()
{
	if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
		{
			const bool bShouldCloseWindowAfterMenuSelection = true;
			FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr);

			TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
			SourcesListView->GetSelectedItems(Selected);
			int32 BaseLayerIndex = Algo::IndexOfByPredicate(Selected, [TLLRN_TLLRN_AnimLayers](const FTLLRN_AnimLayerSourceUIEntryPtr& Key)
				{
					return (Key.IsValid() && Key->GetTLLRN_AnimLayerIndex(TLLRN_TLLRN_AnimLayers) == 0);
				});
			//if we have a base layer selected only show Merge 
			if (BaseLayerIndex != INDEX_NONE)
			{
				if (Selected.Num() > 1)
				{
					MenuBuilder.BeginSection("TLLRN_AnimLayerContextMenuLayer", LOCTEXT("TLLRN_AnimLayerContextMenuLayer", "Layer"));

					FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::MergeLayers));
					const FText Label = LOCTEXT("MergeLayers", "Merge Layers");
					const FText ToolTipText = LOCTEXT("MergeLayerstooltip", "Merge selected layers");
					MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
				}
				MenuBuilder.AddMenuSeparator();
				return MenuBuilder.MakeWidget();

			}
			else if (Selected.Num() > 0)
			{
				MenuBuilder.BeginSection("TLLRN_AnimLayerContextMenuLayer", LOCTEXT("TLLRN_AnimLayerContextMenuLayer", "Layer"));
				{
					FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::AddSelected));
					const FText Label = LOCTEXT("AddSelected", "Add Selected");
					const FText ToolTipText = LOCTEXT("AddSelectedTooltip", "Add selection to objects to selected layers");
					MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
				}
				{
					FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::RemoveSelected));
					const FText Label = LOCTEXT("RemoveSelected", "Remove Selected");
					const FText ToolTipText = LOCTEXT("RemoveSelectedtooltip", "Remove selection from selected layers");
					MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
				}
				{
					FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::SelectObjects));
					const FText Label = LOCTEXT("SelectObjects", "Select Objects");
					const FText ToolTipText = LOCTEXT("SelectObjectsTooltip", "Select all objects in this layer");
					MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
				}

				MenuBuilder.AddMenuSeparator();

				{
					FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::Duplicate));
					const FText Label = LOCTEXT("Duplicate", "Duplicate");
					const FText ToolTipText = LOCTEXT("Duplicatetooltip", "Duplicate to new layer");
					MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
				}
				if (Selected.Num() > 1)
				{
					FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::MergeLayers));
					const FText Label = LOCTEXT("MergeLayers", "Merge Layers");
					const FText ToolTipText = LOCTEXT("MergeLayerstooltip", "Merge selected layers");
					MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
				}
				{
					FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::SetPassthroughKey));
					const FText Label = LOCTEXT("SetPassthroughKey", "Passthrough Key");
					const FText ToolTipText = LOCTEXT("SetPassthroughKeytooltip", "Set zero key(Additive) or previous value(Override)");
					MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
				}
				/* adjustment blending not in yet
				if (Selected.Num() == 1 && Selected[0]->GetTLLRN_AnimLayerIndex(TLLRN_TLLRN_AnimLayers) != 0)
				{
					FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::AdjustmentBlend));
					const FText Label = LOCTEXT("AdjustmentBlend", "Adjustment blend");
					const FText ToolTipText = LOCTEXT("AdjustmentBlendtooltip", "AdustmentBlend");
					MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
				}
				*/
				MenuBuilder.AddMenuSeparator();
				if (Selected.Num() == 1)
				{
					{
						FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::Rename));
						const FText Label = LOCTEXT("Rename", "Rename");
						const FText ToolTipText = LOCTEXT("RenameLayerTooltip", "Rename selected layer");
						MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
					}

				}
				{
					FUIAction Action = FUIAction(FExecuteAction::CreateRaw((this), &FTLLRN_AnimLayerSourcesView::DeleteTLLRN_AnimLayer));
					const FText Label = LOCTEXT("DeletaLayer", "Delete Layer");
					const FText ToolTipText = LOCTEXT("DeleteLayertooltip", "Delete selected layers");
					MenuBuilder.AddMenuEntry(Label, ToolTipText, FSlateIcon(), Action);
				}

				MenuBuilder.EndSection();

				return MenuBuilder.MakeWidget();
			}
		}
	}
	return nullptr;
}

void FTLLRN_AnimLayerSourcesView::OnSourceListSelectionChanged(FTLLRN_AnimLayerSourceUIEntryPtr Entry, ESelectInfo::Type SelectionType) const
{
	TArray<FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
	SourcesListView->GetSelectedItems(Selected);
	//first turn off all
	if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
		{
			for (UTLLRN_AnimLayer* TLLRN_AnimLayer : TLLRN_TLLRN_AnimLayers->TLLRN_TLLRN_AnimLayers)
			{
				TLLRN_AnimLayer->SetSelectedInList(false);
			}
		}
	}
	for (const FTLLRN_AnimLayerSourceUIEntryPtr& Item : Selected)
	{
		Item->SetSelectedInList(true);
	}	
	//refresh tree to rerun the filter
	if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
	{
		SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::RefreshTree);
	}
}

void FTLLRN_AnimLayerSourcesView::RefreshSourceData(bool bRefreshUI)
{
	SourceData.Reset();
	FocusOnIndex = INDEX_NONE;
	if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
		{
			for (UTLLRN_AnimLayer* TLLRN_AnimLayer : TLLRN_TLLRN_AnimLayers->TLLRN_TLLRN_AnimLayers)
			{
				if (Controller == nullptr || Controller->IsSelectionFilterActive() == false
					|| TLLRN_AnimLayer->GetSelected() != ECheckBoxState::Unchecked)
				{
					SourceData.Add(MakeShared<FTLLRN_AnimLayerSourceUIEntry>(TLLRN_AnimLayer));
				}
			}
			/*
			for (FGuid SourceGuid : Client->GetDisplayableSources())
			{
				SourceData.Add(MakeShared<FTLLRN_AnimLayerSourceUIEntry>(SourceGuid, Client));
			}
			*/
		}
	}
	if (bRefreshUI)
	{
		SourcesListView->RequestListRefresh();
		TArray< FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Item : SourceData) 
		{
			if (Item->GetSelectedInList())
			{
				Selected.Add(Item);
			}
		}
		if (Selected.Num())
		{
			SourcesListView->SetItemSelection(Selected, true);
		}
	}
}

//////////////////////////////////////////////////////////////
/// FTLLRN_AnimLayerController
///////////////////////////////////////////////////////////

FTLLRN_AnimLayerController::FTLLRN_AnimLayerController()
{

	SourcesView = MakeShared<FTLLRN_AnimLayerSourcesView>();
	RebuildSourceList();
}

FTLLRN_AnimLayerController::~FTLLRN_AnimLayerController()
{
	/*
	if (Client)
	{
		Client->OnLiveLinkSourcesChanged().Remove(OnSourcesChangedHandle);
		OnSourcesChangedHandle.Reset();
	}
	*/
}

void FTLLRN_AnimLayerController::RefreshTimeDependantData()
{
	if (SourcesView.IsValid())
	{
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Item : SourcesView->SourceData)
		{
			Item->GetWeight();
		}
	}
}
void FTLLRN_AnimLayerController::SelectItem(int32 Index, bool bClear)
{
	if (SourcesView.IsValid() && SourcesView->SourcesListView.IsValid())
	{
		if (bClear)
		{
			SourcesView->SourcesListView->ClearSelection();
		}
		TArray< FTLLRN_AnimLayerSourceUIEntryPtr> Selected;
		int32 Count = 0;
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Item : SourcesView->SourceData)
		{
			if (Count == Index)
			{
				Item->SetSelectedInList(true);
				Selected.Add(Item);
			}
			else if (bClear && Item->GetSelectedInList())
			{
				Item->SetSelectedInList(false);
			}
			++Count;
		}
		if (Selected.Num())
		{
			SourcesView->SourcesListView->SetItemSelection(Selected, true);
		}
	}


}

void FTLLRN_AnimLayerController::RefreshSelectionData()
{
	if (SourcesView.IsValid())
	{
		for (const FTLLRN_AnimLayerSourceUIEntryPtr& Item : SourcesView->SourceData)
		{
			Item->ClearCaches();
		}
	}
}

void FTLLRN_AnimLayerController::RefreshSourceData(bool bRefreshUI)
{
	if (SourcesView.IsValid())
	{
		SourcesView->RefreshSourceData(bRefreshUI);
	}
}
void FTLLRN_AnimLayerController::ToggleSelectionFilterActive()
{
	bSelectionFilterActive = !bSelectionFilterActive;
	RebuildSourceList();
}

bool FTLLRN_AnimLayerController::IsSelectionFilterActive() const
{
	return bSelectionFilterActive;
}

void FTLLRN_AnimLayerController::OnSourceSelectionChangedHandler(FTLLRN_AnimLayerSourceUIEntryPtr Entry, ESelectInfo::Type SelectionType) const
{
	if (bSelectionChangedGuard)
	{
		return;
	}
}

void FTLLRN_AnimLayerController::RebuildSourceList()
{
	SourcesView->RefreshSourceData(true);
}

//////////////////////////////////////////////////////////////
/// STLLRN_TLLRN_AnimLayers
///////////////////////////////////////////////////////////

void STLLRN_TLLRN_AnimLayers::Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode& InEditMode)
{
	TLLRN_AnimLayerController = MakeShared<FTLLRN_AnimLayerController>();
	TLLRN_AnimLayerController->SourcesView->AddController(TLLRN_AnimLayerController.Get());
	LastMovieSceneSig = FGuid();
	if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
	{
		SequencerPtr->OnActivateSequence().AddRaw(this, &STLLRN_TLLRN_AnimLayers::OnActivateSequence);
		SequencerPtr->OnMovieSceneDataChanged().AddRaw(this, &STLLRN_TLLRN_AnimLayers::OnMovieSceneDataChanged);
		SequencerPtr->OnGlobalTimeChanged().AddRaw(this, &STLLRN_TLLRN_AnimLayers::OnGlobalTimeChanged);
		SequencerPtr->OnEndScrubbingEvent().AddRaw(this, &STLLRN_TLLRN_AnimLayers::OnGlobalTimeChanged);
		SequencerPtr->OnStopEvent().AddRaw(this, &STLLRN_TLLRN_AnimLayers::OnGlobalTimeChanged);

		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayersPtr = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr.Get()))
		{
			TLLRN_TLLRN_AnimLayersPtr->TLLRN_AnimLayerListChanged().AddRaw(TLLRN_AnimLayerController.Get(), &FTLLRN_AnimLayerController::HandleOnTLLRN_AnimLayerListChanged);
			TLLRN_TLLRN_AnimLayers = TLLRN_TLLRN_AnimLayersPtr;
		}
	}
	ChildSlot
	[
		SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.Padding(FMargin(4.0f, 0.f,0.f,0.f))
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(.0f)
					.HAlign(HAlign_Fill)
					.FillWidth(1.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(EHorizontalAlignment::HAlign_Right)
						.Padding(.0f)
						[
							SNew(SPositiveActionButton)
							.OnClicked(this, &STLLRN_TLLRN_AnimLayers::OnAddClicked)
							.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
							.Text(LOCTEXT("TLLRN_AnimLayer", "Layer"))
							.ToolTipText(LOCTEXT("TLLRN_AnimLayerTooltip", "Add a new Animation Layer"))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(10.f)
						[
							SNew(SSpacer)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(EHorizontalAlignment::HAlign_Right)
						.Padding(5.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "NoBorder")
								.ButtonColorAndOpacity_Lambda([this]()
									{
										return FLinearColor::White;
									})
								.OnClicked_Lambda([this]()
									{
										TLLRN_AnimLayerController->ToggleSelectionFilterActive();
										return FReply::Handled();
									})
								.ContentPadding(1.f)
								.ToolTipText(LOCTEXT("TLLRN_AnimLayerSelectionFilerTooltip", "Only show Anim Layers with selected objects"))
								[
									SNew(SImage)
										.ColorAndOpacity_Lambda([this]()
											{

												const FLinearColor Selected = FLinearColor(FColor::FromHex(TEXT("#EBA30A")));
												const FColor NotSelected(56, 56, 56);
												FSlateColor SlateColor;
												if (TLLRN_AnimLayerController->IsSelectionFilterActive() == true)
												{
													SlateColor = FSlateColor(Selected);
												}
												else
												{
													SlateColor = FSlateColor(NotSelected);
												}
												return SlateColor;
											})
										.Image(FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_ControlRig.FilterTLLRN_AnimLayerSelected").GetIcon())
								]
						]	
					]
				]
			]
			+SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
			[
				TLLRN_AnimLayerController->SourcesView->SourcesListView.ToSharedRef()
			]
	];
	SetEditMode(InEditMode);
	RegisterSelectionChanged();
	SetCanTick(true);

}

void STLLRN_TLLRN_AnimLayers::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (ISequencer* Sequencer = GetSequencer())
	{
		FGuid CurrentMovieSceneSig = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetSignature();
		if (LastMovieSceneSig != CurrentMovieSceneSig)
		{
			LastMovieSceneSig = CurrentMovieSceneSig;
			TLLRN_AnimLayerController->RefreshSelectionData();
		}
	}
}


FReply STLLRN_TLLRN_AnimLayers::OnSelectionFilterClicked()
{
	TLLRN_AnimLayerController->ToggleSelectionFilterActive();
	return FReply::Handled();
}

bool STLLRN_TLLRN_AnimLayers::IsSelectionFilterActive() const
{
	return TLLRN_AnimLayerController->IsSelectionFilterActive();
}

//mz todo, if in layers with control rig's need to replace them.
void STLLRN_TLLRN_AnimLayers::OnObjectsReplaced(const TMap<UObject*, UObject*>& OldToNewInstanceMap)
{
	//if there's a control rig recreate the tree, controls may have changed
	bool bNewTLLRN_ControlRig = false;
	for (const TPair<UObject*, UObject*>& Pair : OldToNewInstanceMap)
	{
		if (Pair.Key && Pair.Value)
		{
			if (Pair.Key->IsA<UTLLRN_ControlRig>() && Pair.Value->IsA<UTLLRN_ControlRig>())
			{
				bNewTLLRN_ControlRig = false;
				break;
			}
		}
	}
}

STLLRN_TLLRN_AnimLayers::STLLRN_TLLRN_AnimLayers()
{
	FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &STLLRN_TLLRN_AnimLayers::OnObjectsReplaced);
}

STLLRN_TLLRN_AnimLayers::~STLLRN_TLLRN_AnimLayers()
{
	if (TLLRN_TLLRN_AnimLayers.IsValid())
	{
		TLLRN_TLLRN_AnimLayers.Get()->TLLRN_AnimLayerListChanged().RemoveAll(TLLRN_AnimLayerController.Get());
	}

	if (TSharedPtr<ISequencer> SequencerPtr = UTLLRN_TLLRN_AnimLayers::GetSequencerFromAsset())
	{
		SequencerPtr->OnActivateSequence().RemoveAll(this);
		SequencerPtr->OnMovieSceneDataChanged().RemoveAll(this);
		SequencerPtr->OnGlobalTimeChanged().RemoveAll(this);
		SequencerPtr->OnEndScrubbingEvent().RemoveAll(this);
		SequencerPtr->OnStopEvent().RemoveAll(this);

	}
	
	// unregister previous one
	if (OnSelectionChangedHandle.IsValid())
	{
		FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		FLevelEditorModule::FActorSelectionChangedEvent& ActorSelectionChangedEvent = LevelEditor.OnActorSelectionChanged();
		ActorSelectionChangedEvent.Remove(OnSelectionChangedHandle);
		OnSelectionChangedHandle.Reset();
	}	
	
	for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : BoundTLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig.IsValid())
		{
			TLLRN_ControlRig.Get()->TLLRN_ControlRigBound().RemoveAll(this);
			const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig.Get()->GetObjectBinding();
			if (Binding)
			{
				Binding->OnTLLRN_ControlRigBind().RemoveAll(this);
			}
		}
	}
	BoundTLLRN_ControlRigs.SetNum(0);
}

FReply STLLRN_TLLRN_AnimLayers::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		TWeakPtr<ISequencer> Sequencer = EditMode->GetWeakSequencer();
		if (Sequencer.IsValid())
		{

			if (Sequencer.Pin()->GetCommandBindings(ESequencerCommandBindings::CurveEditor).Get()->ProcessCommandBindings(InKeyEvent))
			{
				return FReply::Handled();
			}
		}
	}
	return FReply::Unhandled();
}

void STLLRN_TLLRN_AnimLayers::HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, bool bSelected)
{
	FTLLRN_ControlRigBaseDockableView::HandleControlSelected(Subject, ControlElement, bSelected);
	if (TLLRN_AnimLayerController)
	{
		if (TLLRN_AnimLayerController->IsSelectionFilterActive())
		{
			TLLRN_AnimLayerController->RefreshSourceData(true);
		}
		TLLRN_AnimLayerController->RefreshSelectionData();
	}
}

void STLLRN_TLLRN_AnimLayers::RegisterSelectionChanged()
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	FLevelEditorModule::FActorSelectionChangedEvent& ActorSelectionChangedEvent = LevelEditor.OnActorSelectionChanged();

	// unregister previous one
	if (OnSelectionChangedHandle.IsValid())
	{
		ActorSelectionChangedEvent.Remove(OnSelectionChangedHandle);
		OnSelectionChangedHandle.Reset();
	}

	// register
	OnSelectionChangedHandle = ActorSelectionChangedEvent.AddRaw(this, &STLLRN_TLLRN_AnimLayers::OnActorSelectionChanged);
}

void STLLRN_TLLRN_AnimLayers::OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh)
{
	if (TLLRN_AnimLayerController)
	{
		if (TLLRN_AnimLayerController->IsSelectionFilterActive())
		{
			TLLRN_AnimLayerController->RefreshSourceData(true);
		}
		TLLRN_AnimLayerController->RefreshSelectionData();
	}
}

void STLLRN_TLLRN_AnimLayers::OnActivateSequence(FMovieSceneSequenceIDRef ID)
{
	if (TLLRN_AnimLayerController)
	{
		TLLRN_AnimLayerController->RefreshSourceData(true);
		TLLRN_AnimLayerController->RefreshSelectionData();
	}
}

void STLLRN_TLLRN_AnimLayers::OnGlobalTimeChanged()
{
	if (TLLRN_AnimLayerController)
	{
		TLLRN_AnimLayerController->RefreshTimeDependantData();
	}
}

void STLLRN_TLLRN_AnimLayers::OnMovieSceneDataChanged(EMovieSceneDataChangeType)
{
	if (TLLRN_AnimLayerController)
	{
		TLLRN_AnimLayerController->RefreshTimeDependantData();
		TLLRN_AnimLayerController->RefreshSelectionData();
	}
}

FReply STLLRN_TLLRN_AnimLayers::OnAddClicked()
{
	if (ISequencer* SequencerPtr = GetSequencer())
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayersPtr = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(SequencerPtr))
		{
			int32 Index = TLLRN_TLLRN_AnimLayersPtr->AddTLLRN_AnimLayerFromSelection(SequencerPtr);
			if (Index != INDEX_NONE)
			{
				if (TLLRN_AnimLayerController.IsValid())
				{
					TLLRN_AnimLayerController->FocusRenameLayer(Index);
					TLLRN_AnimLayerController->SelectItem(Index, true);
				}
			}
		}
	}

	return FReply::Handled();
}

void STLLRN_TLLRN_AnimLayers::SetEditMode(FTLLRN_ControlRigEditMode& InEditMode)
{
	FTLLRN_ControlRigBaseDockableView::SetEditMode(InEditMode);
	ModeTools = InEditMode.GetModeManager();
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		TArrayView<TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigs = EditMode->GetTLLRN_ControlRigs();
		for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : TLLRN_ControlRigs)
		{
			if (TLLRN_ControlRig.IsValid())
			{
				if (!TLLRN_ControlRig.Get()->TLLRN_ControlRigBound().IsBoundToObject(this))
				{
					TLLRN_ControlRig.Get()->TLLRN_ControlRigBound().AddRaw(this, &STLLRN_TLLRN_AnimLayers::HandleOnTLLRN_ControlRigBound);
					BoundTLLRN_ControlRigs.Add(TLLRN_ControlRig);
				}
				const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig.Get()->GetObjectBinding();
				if (Binding && !Binding->OnTLLRN_ControlRigBind().IsBoundToObject(this))
				{
					Binding->OnTLLRN_ControlRigBind().AddRaw(this, &STLLRN_TLLRN_AnimLayers::HandleOnObjectBoundToTLLRN_ControlRig);
				}
			}
		}
	}
}

void STLLRN_TLLRN_AnimLayers::HandleControlAdded(UTLLRN_ControlRig* TLLRN_ControlRig, bool bIsAdded)
{
	FTLLRN_ControlRigBaseDockableView::HandleControlAdded(TLLRN_ControlRig, bIsAdded);
	if (TLLRN_ControlRig)
	{
		if (bIsAdded == true)
		{
			if (!TLLRN_ControlRig->TLLRN_ControlRigBound().IsBoundToObject(this))
			{
				TLLRN_ControlRig->TLLRN_ControlRigBound().AddRaw(this, &STLLRN_TLLRN_AnimLayers::HandleOnTLLRN_ControlRigBound);
				BoundTLLRN_ControlRigs.Add(TLLRN_ControlRig);
			}
			const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig->GetObjectBinding();
			if (Binding && !Binding->OnTLLRN_ControlRigBind().IsBoundToObject(this))
			{
				Binding->OnTLLRN_ControlRigBind().AddRaw(this, &STLLRN_TLLRN_AnimLayers::HandleOnObjectBoundToTLLRN_ControlRig);
			}
		}
		else
		{
			BoundTLLRN_ControlRigs.Remove(TLLRN_ControlRig);
			TLLRN_ControlRig->TLLRN_ControlRigBound().RemoveAll(this);
			const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig->GetObjectBinding();
			if (Binding)
			{
				Binding->OnTLLRN_ControlRigBind().RemoveAll(this);
			}
		}
	}
}

void STLLRN_TLLRN_AnimLayers::HandleOnTLLRN_ControlRigBound(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if (!InTLLRN_ControlRig)
	{
		return;
	}

	const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = InTLLRN_ControlRig->GetObjectBinding();

	if (Binding && !Binding->OnTLLRN_ControlRigBind().IsBoundToObject(this))
	{
		Binding->OnTLLRN_ControlRigBind().AddRaw(this, &STLLRN_TLLRN_AnimLayers::HandleOnObjectBoundToTLLRN_ControlRig);
	}
}

//mz todo need to test recompiling
void STLLRN_TLLRN_AnimLayers::HandleOnObjectBoundToTLLRN_ControlRig(UObject* InObject)
{
	
}

class SInvalidWeightNameDetailWidget : public SSpacer
{
	SLATE_BEGIN_ARGS(SInvalidWeightNameDetailWidget)
		{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SetVisibility(EVisibility::Collapsed);
	}

};

class  FWeightNameOverride : public FDetailsNameWidgetOverrideCustomization
{
public:
	FWeightNameOverride() {};
	virtual ~FWeightNameOverride() override = default;
	virtual TSharedRef<SWidget> CustomizeName(TSharedRef<SWidget> InnerNameContent, FPropertyPath& Path) override 
	{
		const TSharedRef<SWidget> NameContent = SNew(SInvalidWeightNameDetailWidget);
		return NameContent;
	}
};

void SAnimWeightDetails::Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode* InEditMode, UObject* InWeightObject)
{
	if (InEditMode == nullptr || InWeightObject == nullptr)
	{
		return;
	}
	using namespace UE::Sequencer;

	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = false;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
		DetailsViewArgs.bCustomNameAreaLocation = false;
		DetailsViewArgs.bCustomFilterAreaLocation = false;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
		DetailsViewArgs.bShowScrollBar = false; // Don't need to show this, as we are putting it in a scroll box
		DetailsViewArgs.DetailsNameWidgetOverrideCustomization = MakeShared<FWeightNameOverride>();

	}

	WeightView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	WeightView->SetKeyframeHandler(InEditMode->DetailKeyFrameCache);


	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				WeightView.ToSharedRef()
			]
		]
	];
	TArray<UObject*> Objects;
	Objects.Add(InWeightObject);
	WeightView->SetObjects(Objects, true);
}

SAnimWeightDetails::~SAnimWeightDetails()
{
}



#undef LOCTEXT_NAMESPACE




