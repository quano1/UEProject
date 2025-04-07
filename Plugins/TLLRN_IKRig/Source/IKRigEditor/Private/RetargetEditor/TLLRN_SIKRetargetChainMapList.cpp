﻿// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_SIKRetargetChainMapList.h"

#include "ScopedTransaction.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IPersonaToolkit.h"
#include "SKismetInspector.h"
#include "Dialogs/Dialogs.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SPositiveActionButton.h"
#include "Retargeter/TLLRN_IKRetargeter.h"
#include "RigEditor/TLLRN_IKRigEditorStyle.h"
#include "Widgets/Input/SComboBox.h"
#include "SSearchableComboBox.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "Widgets/Input/SSearchBox.h"
#include "Retargeter/TLLRN_IKRetargetSettings.h"
#include "RigEditor/TLLRN_IKRigController.h"

#define LOCTEXT_NAMESPACE "TLLRN_SIKRigRetargetChains"

static const FName ColumnId_TargetChainLabel( "Target Bone Chain" );
static const FName ColumnId_SourceChainLabel( "Source Bone Chain" );
static const FName ColumnId_IKGoalNameLabel( "Target IK Goal" );
static const FName ColumnId_ResetLabel( "Reset" );

TSharedRef<ITableRow> FTLLRN_RetargetChainMapElement::MakeListRowWidget(
	const TSharedRef<STableViewBase>& InOwnerTable,
	TSharedRef<FTLLRN_RetargetChainMapElement> InChainElement,
	TSharedPtr<TLLRN_SIKRetargetChainMapList> InChainList)
{
	return SNew(SIKRetargetChainMapRow, InOwnerTable, InChainElement, InChainList);
}

void SIKRetargetChainMapRow::Construct(
	const FArguments& InArgs,
	const TSharedRef<STableViewBase>& InOwnerTableView,
	TSharedRef<FTLLRN_RetargetChainMapElement> InChainElement,
	TSharedPtr<TLLRN_SIKRetargetChainMapList> InChainList)
{
	ChainMapElement = InChainElement;
	ChainMapList = InChainList;

	// generate list of source chains
	// NOTE: cannot just use FName because "None" is considered a null entry and removed from ComboBox.
	SourceChainOptions.Reset();
	SourceChainOptions.Add(MakeShareable(new FString(TEXT("None"))));
	const UTLLRN_IKRigDefinition* SourceIKRig = ChainMapList.Pin()->EditorController.Pin()->AssetController->GetIKRig(ETLLRN_RetargetSourceOrTarget::Source);
	if (SourceIKRig)
	{
		const TArray<FTLLRN_BoneChain>& Chains = SourceIKRig->GetRetargetChains();
		for (const FTLLRN_BoneChain& BoneChain : Chains)
		{
			SourceChainOptions.Add(MakeShareable(new FString(BoneChain.ChainName.ToString())));
		}
	}

	SMultiColumnTableRow< FTLLRN_RetargetChainMapElementPtr >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
}

TSharedRef< SWidget > SIKRetargetChainMapRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == ColumnId_TargetChainLabel)
	{
		TSharedRef<SWidget> NewWidget =
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(3.0f, 1.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromName(ChainMapElement.Pin()->ChainMap->TargetChain))
				.Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))
			];
		return NewWidget;
	}

	if (ColumnName == ColumnId_SourceChainLabel)
	{
		TSharedRef<SWidget> NewWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(3.0f, 1.0f)
		[
			SNew(SSearchableComboBox)
			.OptionsSource(&SourceChainOptions)
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
			{
				return SNew(STextBlock).Text(FText::FromString(*InItem.Get()));
			})
			.OnSelectionChanged(this, &SIKRetargetChainMapRow::OnSourceChainComboSelectionChanged)
			[
				SNew(STextBlock)
				.Text(this, &SIKRetargetChainMapRow::GetSourceChainName)
			]
		];
		return NewWidget;
	}

	if (ColumnName == ColumnId_IKGoalNameLabel)
	{
		TSharedRef<SWidget> NewWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(3.0f, 1.0f)
		[
			SNew(STextBlock)
			.Text(GetTargetIKGoalName())
			.Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))
		];
		return NewWidget;
	}

	if (ColumnName == ColumnId_ResetLabel)
	{
		TSharedRef<SWidget> NewWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(3.0f, 1.0f)
		[
			SNew(SButton)
			.OnClicked(this, &SIKRetargetChainMapRow::OnResetToDefaultClicked) 
			.Visibility(this, &SIKRetargetChainMapRow::GetResetToDefaultVisibility) 
			.ToolTipText(LOCTEXT("ResetChainToDefaultToolTip", "Reset Chain Settings to Default"))
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.Content()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
			]
		];
		return NewWidget;
	}
	
	checkNoEntry();
	return SNullWidget::NullWidget;
}

FReply SIKRetargetChainMapRow::OnResetToDefaultClicked()
{
	ChainMapList.Pin()->ResetChainSettings(ChainMapElement.Pin()->ChainMap.Get());
	return FReply::Handled();
}

EVisibility SIKRetargetChainMapRow::GetResetToDefaultVisibility() const
{
	const FTLLRN_TargetChainSettings& DefaultSettings = FTLLRN_TargetChainSettings();
	const FTLLRN_TargetChainSettings& Settings = ChainMapElement.Pin()->ChainMap->Settings;
	return (Settings == DefaultSettings) ? EVisibility::Hidden : EVisibility::Visible;
}

void SIKRetargetChainMapRow::OnSourceChainComboSelectionChanged(TSharedPtr<FString> InName, ESelectInfo::Type SelectInfo)
{
	UTLLRN_IKRetargeterController* RetargeterController = ChainMapList.Pin()->GetRetargetController();
	if (!RetargeterController)
	{
		return; 
	}

	const FName SourceChainName = FName(*InName.Get());
	RetargeterController->SetSourceChain(SourceChainName, ChainMapElement.Pin()->ChainMap->TargetChain);
}

FText SIKRetargetChainMapRow::GetSourceChainName() const
{
	UTLLRN_IKRetargeterController* RetargeterController = ChainMapList.Pin()->GetRetargetController();
	if (!RetargeterController)
	{
		return FText::FromName(NAME_None); 
	}

	return FText::FromName(ChainMapElement.Pin()->ChainMap->SourceChain);
}

FText SIKRetargetChainMapRow::GetTargetIKGoalName() const
{
	UTLLRN_IKRetargeterController* RetargeterController = ChainMapList.Pin()->GetRetargetController();
	if (!RetargeterController)
	{
		return FText(); 
	}

	const UTLLRN_IKRigDefinition* IKRig = RetargeterController->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
	if (!IKRig)
	{
		return FText(); 
	}

	const UTLLRN_IKRigController* RigController = UTLLRN_IKRigController::GetController(IKRig);
	const FTLLRN_BoneChain* Chain = RigController->GetRetargetChainByName(ChainMapElement.Pin()->ChainMap->TargetChain);
	if (!Chain)
	{
		return FText(); 
	}

	if (Chain->IKGoalName == NAME_None)
	{
		return FText::FromString("");
	}
	
	return FText::FromName(Chain->IKGoalName);
}

void TLLRN_SIKRetargetChainMapList::Construct(
	const FArguments& InArgs,
	TSharedRef<FTLLRN_IKRetargetEditorController> InEditorController)
{
	EditorController = InEditorController;
	EditorController.Pin()->SetChainsView(SharedThis(this));

	TextFilter = MakeShareable(new FTextFilterExpressionEvaluator(ETextFilterExpressionEvaluatorMode::BasicString));
	
	ChildSlot
    [
	    SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TargetRootLabel", "Target Root: "))
				.TextStyle(FAppStyle::Get(), "NormalText")
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &TLLRN_SIKRetargetChainMapList::GetTargetRootBone)
				.IsEnabled(false)
			]
				
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SourceRootLabel", "Source Root: "))
				.TextStyle(FAppStyle::Get(), "NormalText")
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5, 0)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &TLLRN_SIKRetargetChainMapList::GetSourceRootBone)
				.IsEnabled(false)
			]
		]

		+SVerticalBox::Slot()
		.Padding(2.0f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSearchBox)
				.SelectAllTextWhenFocused(true)
				.OnTextChanged( this, &TLLRN_SIKRetargetChainMapList::OnFilterTextChanged )
				.HintText( LOCTEXT( "SearchBoxHint", "Filter Chain List...") )
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(6.f, 0.0))
			.VAlign(VAlign_Center)
			[
				SNew(SComboButton)
				.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButton"))
				.ForegroundColor(FSlateColor::UseStyle())
				.ContentPadding(2.0f)
				.OnGetMenuContent( this, &TLLRN_SIKRetargetChainMapList::CreateFilterMenuWidget )
				.ToolTipText( LOCTEXT("ChainMapFilterToolTip", "Filter list of chain mappings."))
				.HasDownArrow(true)
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Settings"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(6.f, 0.0))
			[
				SNew(SComboButton)
				.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButton"))
				.ForegroundColor(FSlateColor::UseStyle())
				.ContentPadding(2.0f)
				.Visibility(this, &TLLRN_SIKRetargetChainMapList::IsAutoMapButtonVisible)
				.ToolTipText(LOCTEXT("AutoMapButtonToolTip", "Automatically assign source chains based on matching rule."))
				.OnGetMenuContent( this, &TLLRN_SIKRetargetChainMapList::CreateChainMapMenuWidget )
				.HasDownArrow(true)
				.ButtonContent()
				[
					SNew(STextBlock).Text(LOCTEXT("AutoMapButtonLabel", "Auto-Map Chains"))
				]
			]
		]
	    
        +SVerticalBox::Slot()
        [
			SAssignNew(ListView, SRetargetChainMapListViewType )
			.SelectionMode(ESelectionMode::Multi)
			.IsEnabled(this, &TLLRN_SIKRetargetChainMapList::IsChainMapEnabled)
			.ListItemsSource( &ListViewItems )
			.OnGenerateRow( this, &TLLRN_SIKRetargetChainMapList::MakeListRowWidget )
			.OnMouseButtonClick_Lambda([this](TSharedPtr<FTLLRN_RetargetChainMapElement> Item)
			{
				OnItemClicked(Item);
			})
			.HeaderRow
			(
				SNew( SHeaderRow )
				+ SHeaderRow::Column(ColumnId_TargetChainLabel)
				.DefaultLabel(LOCTEXT("TargetColumnLabel", "Target Chain"))
				.DefaultTooltip(LOCTEXT("TargetChainToolTip", "The chain on the target skeleton to copy animation TO."))
				
				+ SHeaderRow::Column(ColumnId_IKGoalNameLabel)
				.DefaultLabel(LOCTEXT("IKColumnLabel", "Target IK Goal"))
				.DefaultTooltip(LOCTEXT("IKGoalToolTip", "The IK Goal assigned to the target chain (if any). Note, this goal should be on the LAST bone in the chain."))

				+ SHeaderRow::Column(ColumnId_SourceChainLabel)
				.DefaultLabel(LOCTEXT("SourceColumnLabel", "Source Chain"))
				.DefaultTooltip(LOCTEXT("SourceChainToolTip", "The chain on the source skeleton to copy animation FROM."))
				
				+ SHeaderRow::Column(ColumnId_ResetLabel)
                .DefaultLabel(LOCTEXT("ResetColumnLabel", "Reset"))
                .DefaultTooltip(LOCTEXT("ResetChainToolTip", "Reset the FK and IK settings for this chain."))
			)
        ]
    ];

	RefreshView();
}

void TLLRN_SIKRetargetChainMapList::ClearSelection() const
{
	ListView->ClearSelection();
}

void TLLRN_SIKRetargetChainMapList::ResetChainSettings(UTLLRN_RetargetChainSettings* ChainMap) const
{
	FScopedTransaction Transaction(LOCTEXT("ResetChainSettings", "Reset Retarget Chain Settings"));
	ChainMap->Modify();
	ChainMap->Settings = FTLLRN_TargetChainSettings();
	EditorController.Pin()->RefreshDetailsView();
}

UTLLRN_IKRetargeterController* TLLRN_SIKRetargetChainMapList::GetRetargetController() const
{
	const TSharedPtr<FTLLRN_IKRetargetEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return nullptr;
	}

	return Controller->AssetController;
}

FText TLLRN_SIKRetargetChainMapList::GetSourceRootBone() const
{
	UTLLRN_IKRetargeterController* RetargeterController = GetRetargetController();
	if (!RetargeterController)
	{
		return FText::FromName(NAME_None); 
	}
	
	return FText::FromName(RetargeterController->GetRetargetRootBone(ETLLRN_RetargetSourceOrTarget::Source));
}

FText TLLRN_SIKRetargetChainMapList::GetTargetRootBone() const
{
	UTLLRN_IKRetargeterController* RetargeterController = GetRetargetController();
	if (!RetargeterController)
	{
		return FText::FromName(NAME_None); 
	}
	
	return FText::FromName(RetargeterController->GetRetargetRootBone(ETLLRN_RetargetSourceOrTarget::Target));
}

bool TLLRN_SIKRetargetChainMapList::IsChainMapEnabled() const
{
	UTLLRN_IKRetargeterController* RetargeterController = GetRetargetController();
	if (!RetargeterController)
	{
		return false; 
	}
	
	if (const UTLLRN_IKRigDefinition* IKRig = RetargeterController->GetAsset()->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target))
	{
		const TArray<FTLLRN_BoneChain>& Chains = IKRig->GetRetargetChains();
		return Chains.Num() > 0;
	}

	return false;
}

void TLLRN_SIKRetargetChainMapList::RefreshView()
{
	UTLLRN_IKRetargeterController* AssetController = GetRetargetController();
	if (!AssetController)
	{
		return; 
	}

	auto FilterString = [this](const FString& StringToTest) ->bool
	{
		return TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(StringToTest));
	};

	auto DoesChainHaveIK = [AssetController](const FName TargetChainName) ->bool
	{
		const UTLLRN_IKRigDefinition* IKRig = AssetController->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
		if (!IKRig)
		{
			return false;
		}

		const UTLLRN_IKRigController* RigController = UTLLRN_IKRigController::GetController(IKRig);
		const FTLLRN_BoneChain* Chain = RigController->GetRetargetChainByName(TargetChainName);
		if (!Chain)
		{
			return false;
		}
		
		return Chain->IKGoalName != NAME_None;
	};

	const TArray<FName>& SelectedChains = EditorController.Pin()->GetSelectedChains();
	const FName LiteralNone = FName("None");

	// refresh items
	ListViewItems.Reset();
	const TArray<UTLLRN_RetargetChainSettings*>& ChainMappings = AssetController->GetAllChainSettings();
	for (const TObjectPtr<UTLLRN_RetargetChainSettings> ChainMap : ChainMappings)
	{
		// apply text filter to items
		if (!(TextFilter->GetFilterText().IsEmpty() ||
			FilterString(ChainMap->SourceChain.ToString()) ||
			FilterString(ChainMap->TargetChain.ToString())))
		{
			continue;
		}
		
		// apply "only IK" filter
		if (ChainFilterOptions.bHideChainsWithoutIK && !DoesChainHaveIK(ChainMap->TargetChain))
		{
			continue;
		}

		// apply "hide mapped chains" filter
		if (ChainFilterOptions.bHideMappedChains && ChainMap->SourceChain != LiteralNone)
		{
			continue;
		}
		
		// apply "hide un-mapped chains" filter
		if (ChainFilterOptions.bHideUnmappedChains && ChainMap->SourceChain == LiteralNone)
		{
			continue;
		}
		
		// create an item for this chain
		TSharedPtr<FTLLRN_RetargetChainMapElement> ChainItem = FTLLRN_RetargetChainMapElement::Make(ChainMap);
		ListViewItems.Add(ChainItem);

		// select/deselect it
		const bool bIsSelected = SelectedChains.Contains(ChainMap->TargetChain);
		ListView->SetItemSelection(ChainItem, bIsSelected, ESelectInfo::Direct);
	}
	
	ListView->RequestListRefresh();
}

TSharedRef<SWidget> TLLRN_SIKRetargetChainMapList::CreateFilterMenuWidget()
{
	const FUIAction FilterHideMappedAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			ChainFilterOptions.bHideMappedChains = !ChainFilterOptions.bHideMappedChains;
			RefreshView();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]()
		{
			return ChainFilterOptions.bHideMappedChains;
		}));

	const FUIAction FilterOnlyUnMappedAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			ChainFilterOptions.bHideUnmappedChains = !ChainFilterOptions.bHideUnmappedChains;
			RefreshView();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]()
		{
			return ChainFilterOptions.bHideUnmappedChains;
		}));

	const FUIAction FilterIKChainAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			ChainFilterOptions.bHideChainsWithoutIK = !ChainFilterOptions.bHideChainsWithoutIK;
			RefreshView();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]()
		{
			return ChainFilterOptions.bHideChainsWithoutIK;
		}));
	
	static constexpr bool CloseAfterSelection = true;
	FMenuBuilder MenuBuilder(CloseAfterSelection, CommandList);

	MenuBuilder.BeginSection("Chain Map Filters", LOCTEXT("ChainMapFiltersSection", "Filter Chain Mappings"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("HideMappedLabel", "Hide Mapped Chains"),
		LOCTEXT("HideMappedTooltip", "Hide chains mapped to a source chain."),
		FSlateIcon(),
		FilterHideMappedAction,
		NAME_None,
		EUserInterfaceActionType::Check);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("HideUnMappedLabel", "Hide Unmapped Chains"),
		LOCTEXT("HideUnMappedTooltip", "Hide chains not mapped to a source chain."),
		FSlateIcon(),
		FilterOnlyUnMappedAction,
		NAME_None,
		EUserInterfaceActionType::Check);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("HideNonIKLabel", "Hide Chains Without IK"),
		LOCTEXT("HideNonIKTooltip", "Hide chains not using IK."),
		FSlateIcon(),
		FilterIKChainAction,
		NAME_None,
		EUserInterfaceActionType::Check);
	
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Clear", LOCTEXT("ClearMapFiltersSection", "Clear"));
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ClearMapFilterLabel", "Clear Filters"),
		LOCTEXT("ClearMapFilterTooltip", "Clear all filters to show all chain mappings."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]
		{
			ChainFilterOptions = FTLLRN_ChainMapFilterOptions();
			RefreshView();
		})));

	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget();
}

void TLLRN_SIKRetargetChainMapList::OnFilterTextChanged(const FText& SearchText)
{
	TextFilter->SetFilterText(SearchText);
	RefreshView();
}

TSharedRef<ITableRow> TLLRN_SIKRetargetChainMapList::MakeListRowWidget(
	TSharedPtr<FTLLRN_RetargetChainMapElement> InElement,
    const TSharedRef<STableViewBase>& OwnerTable)
{
	return InElement->MakeListRowWidget(
		OwnerTable,
		InElement.ToSharedRef(),
		SharedThis(this));
}

void TLLRN_SIKRetargetChainMapList::OnItemClicked(TSharedPtr<FTLLRN_RetargetChainMapElement> InItem) const 
{
	// get list of selected chains
	TArray<FName> SelectedChains;
	TArray<TSharedPtr<FTLLRN_RetargetChainMapElement>> SelectedItems = ListView.Get()->GetSelectedItems();
	for (const TSharedPtr<FTLLRN_RetargetChainMapElement>& Item : SelectedItems)
	{
		SelectedChains.Add(Item.Get()->ChainMap->TargetChain);
	}
	
	// replace the chain selection
	constexpr bool bEditFromChainsView = true;
	EditorController.Pin()->EditChainSelection(SelectedChains, ETLLRN_SelectionEdit::Replace, bEditFromChainsView);
}

TSharedRef<SWidget> TLLRN_SIKRetargetChainMapList::CreateChainMapMenuWidget()
{
	const FUIAction MapAllByFuzzyNameAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			constexpr bool bForceRemap = true;
			AutoMapChains(ETLLRN_AutoMapChainType::Fuzzy, bForceRemap);
		}),
		FCanExecuteAction(), FIsActionChecked());
	
	const FUIAction MapAllByExactNameAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			constexpr bool bForceRemap = true;
			AutoMapChains(ETLLRN_AutoMapChainType::Exact, bForceRemap);
		}),
		FCanExecuteAction(), FIsActionChecked());

	const FUIAction MapUnmappedByExactNameAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			constexpr bool bForceRemap = false;
			AutoMapChains(ETLLRN_AutoMapChainType::Exact, bForceRemap);
		}),
		FCanExecuteAction(), FIsActionChecked());

	const FUIAction MapUnmappedByFuzzyNameAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			constexpr bool bForceRemap = false;
			AutoMapChains(ETLLRN_AutoMapChainType::Fuzzy, bForceRemap);
		}),
		FCanExecuteAction(), FIsActionChecked());

	const FUIAction ClearAllMappingsAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			constexpr bool bForceRemap = true;
			AutoMapChains(ETLLRN_AutoMapChainType::Clear, bForceRemap);
		}),
		FCanExecuteAction(), FIsActionChecked());
	
	static constexpr bool CloseAfterSelection = true;
	FMenuBuilder MenuBuilder(CloseAfterSelection, CommandList);

	MenuBuilder.BeginSection("Auto-Map Chains Fuzzy", LOCTEXT("FuzzyNameSection", "Fuzzy Name Matching"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MapAllByNameFuzzyLabel", "Map All (Fuzzy)"),
		LOCTEXT("MapAllByNameFuzzyTooltip", "Map all chains to the source chain with the closest name (not necessarily exact)."),
		FSlateIcon(),
		MapAllByFuzzyNameAction,
		NAME_None,
		EUserInterfaceActionType::Button);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MapMissingByNameFuzzyLabel", "Map Only Empty (Fuzzy)"),
		LOCTEXT("MapMissingByNameFuzzyTooltip", "Map all unmapped chains to the source chain with the closest name (not necessarily exact)."),
		FSlateIcon(),
		MapUnmappedByFuzzyNameAction,
		NAME_None,
		EUserInterfaceActionType::Button);

	MenuBuilder.EndSection();
	MenuBuilder.BeginSection("Auto-Map Chains Exact", LOCTEXT("ExactNameSection", "Exact Name Matching"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MapAllByNameExactLabel", "Map All (Exact)"),
		LOCTEXT("MapAllByNameExactTooltip", "Map all chains with identical name. If no match found, does not change mapping."),
		FSlateIcon(),
		MapAllByExactNameAction,
		NAME_None,
		EUserInterfaceActionType::Button);
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("MapMissingByNameExactLabel", "Map Only Empty (Exact)"),
		LOCTEXT("MapMissingByNameExactTooltip", "Map unmapped chains using identical name. If no match found, does not change mapping."),
		FSlateIcon(),
		MapUnmappedByExactNameAction,
		NAME_None,
		EUserInterfaceActionType::Button);

	MenuBuilder.EndSection();
	MenuBuilder.BeginSection("Clear", LOCTEXT("ClearMapSection", "Clear All"));
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ClearMapLabel", "Clear All Mappings"),
		LOCTEXT("ClearMapTooltip", "Map all chains to None."),
		FSlateIcon(),
		ClearAllMappingsAction,
		NAME_None,
		EUserInterfaceActionType::Button);

	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget();
}

EVisibility TLLRN_SIKRetargetChainMapList::IsAutoMapButtonVisible() const
{
	return IsChainMapEnabled() ? EVisibility::Visible : EVisibility::Hidden;
}

void TLLRN_SIKRetargetChainMapList::AutoMapChains(const ETLLRN_AutoMapChainType AutoMapType, const bool bForceRemap)
{
	const TSharedPtr<FTLLRN_IKRetargetEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return;
	}
	
	UTLLRN_IKRetargeterController* RetargeterController = GetRetargetController();
	if (!RetargeterController)
	{
		return;
	}

	Controller->ClearOutputLog();
	RetargeterController->AutoMapChains(AutoMapType, bForceRemap);
	Controller->HandleRetargeterNeedsInitialized();
}

#undef LOCTEXT_NAMESPACE

