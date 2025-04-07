// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_SIKRigRetargetChainList.h"

#include "RigEditor/TLLRN_IKRigEditorStyle.h"
#include "RigEditor/TLLRN_IKRigToolkit.h"
#include "RigEditor/TLLRN_IKRigEditorController.h"

#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IPersonaToolkit.h"
#include "SKismetInspector.h"
#include "Dialogs/Dialogs.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SPositiveActionButton.h"
#include "SSearchableComboBox.h"
#include "BoneSelectionWidget.h"
#include "Animation/MirrorDataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Widgets/Input/SSearchBox.h"
#include "ScopedTransaction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_SIKRigRetargetChainList)

#define LOCTEXT_NAMESPACE "TLLRN_SIKRigRetargetChains"

static const FName ColumnId_ChainNameLabel( "Chain Name" );
static const FName ColumnId_ChainStartLabel( "Start Bone" );
static const FName ColumnId_ChainEndLabel( "End Bone" );
static const FName ColumnId_IKGoalLabel( "IK Goal" );
static const FName ColumnId_DeleteChainLabel( "Delete Chain" );

TSharedRef<ITableRow> FRetargetChainElement::MakeListRowWidget(
	const TSharedRef<STableViewBase>& InOwnerTable,
	TSharedRef<FRetargetChainElement> InChainElement,
	TSharedPtr<TLLRN_SIKRigRetargetChainList> InChainList)
{
	return SNew(SIKRigRetargetChainRow, InOwnerTable, InChainElement, InChainList);
}

void SIKRigRetargetChainRow::Construct(
	const FArguments& InArgs,
	const TSharedRef<STableViewBase>& InOwnerTableView,
	TSharedRef<FRetargetChainElement> InChainElement,
	TSharedPtr<TLLRN_SIKRigRetargetChainList> InChainList)
{
	ChainElement = InChainElement;
	ChainList = InChainList;

	// generate list of goals
	// NOTE: cannot just use literal "None" because Combobox considers that a "null" entry and will discard it from the list.
	GoalOptions.Add(MakeShareable(new FString("None")));
	const UTLLRN_IKRigDefinition* IKRigAsset = InChainList->EditorController.Pin()->AssetController->GetAsset();
	const TArray<UTLLRN_IKRigEffectorGoal*>& AssetGoals =  IKRigAsset->GetGoalArray();
	for (const UTLLRN_IKRigEffectorGoal* Goal : AssetGoals)
	{
		GoalOptions.Add(MakeShareable(new FString(Goal->GoalName.ToString())));
	}

	SMultiColumnTableRow< TSharedPtr<FRetargetChainElement> >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
}

TSharedRef<SWidget> SIKRigRetargetChainRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == ColumnId_ChainNameLabel)
	{
		TSharedRef<SWidget> ChainWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(3.0f, 1.0f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromName(ChainElement.Pin()->ChainName))
			.Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))
			.OnTextCommitted(this, &SIKRigRetargetChainRow::OnRenameChain)
		];
		return ChainWidget;
	}

	if (ColumnName == ColumnId_ChainStartLabel)
	{
		TSharedRef<SWidget> StartWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(3.0f, 1.0f)
		[
			SNew(SBoneSelectionWidget)
			.OnBoneSelectionChanged(this, &SIKRigRetargetChainRow::OnStartBoneComboSelectionChanged)
			.OnGetSelectedBone(this, &SIKRigRetargetChainRow::GetStartBoneName)
			.OnGetReferenceSkeleton(this, &SIKRigRetargetChainRow::GetReferenceSkeleton)
		];
		return StartWidget;
	}

	if (ColumnName == ColumnId_ChainEndLabel)
	{
		TSharedRef<SWidget> EndWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(3.0f, 1.0f)
		[
			SNew(SBoneSelectionWidget)
			.OnBoneSelectionChanged(this, &SIKRigRetargetChainRow::OnEndBoneComboSelectionChanged)
			.OnGetSelectedBone(this, &SIKRigRetargetChainRow::GetEndBoneName)
			.OnGetReferenceSkeleton(this, &SIKRigRetargetChainRow::GetReferenceSkeleton)
		];
		return EndWidget;
	}

	if (ColumnName == ColumnId_IKGoalLabel)
	{
		TSharedRef<SWidget> GoalWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(3.0f, 1.0f)
		[
			SNew(SSearchableComboBox)
			.OptionsSource(&GoalOptions)
			.OnGenerateWidget(this, &SIKRigRetargetChainRow::MakeGoalComboEntryWidget)
			.OnSelectionChanged(this, &SIKRigRetargetChainRow::OnGoalComboSelectionChanged)
			[
				SNew(STextBlock)
				.Text(this, &SIKRigRetargetChainRow::GetGoalName)
			]
		];
		return GoalWidget;
	}

	// ColumnName == ColumnId_DeleteChainLabel
	{
		TSharedRef<SWidget> DeleteWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(3)
		[
			SNew(SButton)
			.ToolTipText(LOCTEXT("DeleteChain", "Remove retarget bone chain from list."))
			.OnClicked_Lambda([this]() -> FReply
			{
				const TSharedPtr<FTLLRN_IKRigEditorController> Controller = ChainList.Pin()->EditorController.Pin();
				if (!Controller.IsValid())
				{
					return FReply::Unhandled();
				}

				UTLLRN_IKRigController* AssetController = Controller->AssetController;
				AssetController->RemoveRetargetChain(ChainElement.Pin()->ChainName);

				ChainList.Pin()->RefreshView();
				return FReply::Handled();
			})
			.Content()
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Delete"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
		return DeleteWidget;
	}
}

TSharedRef<SWidget> SIKRigRetargetChainRow::MakeGoalComboEntryWidget(TSharedPtr<FString> InItem) const
{
	return SNew(STextBlock).Text(FText::FromString(*InItem.Get()));
}

void SIKRigRetargetChainRow::OnStartBoneComboSelectionChanged(FName InName) const
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = ChainList.Pin()->EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}
	
	Controller->AssetController->SetRetargetChainStartBone(ChainElement.Pin()->ChainName, InName);
	ChainList.Pin()->RefreshView();
}

FName SIKRigRetargetChainRow::GetStartBoneName(bool& bMultipleValues) const
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = ChainList.Pin()->EditorController.Pin();
	if (!Controller.IsValid())
	{
		return NAME_None;
	}

	bMultipleValues = false;
	return Controller->AssetController->GetRetargetChainStartBone(ChainElement.Pin()->ChainName);
}

FName SIKRigRetargetChainRow::GetEndBoneName(bool& bMultipleValues) const
{	
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = ChainList.Pin()->EditorController.Pin();
	if (!Controller.IsValid())
	{
		return NAME_None;
	}

	bMultipleValues = false;
	return Controller->AssetController->GetRetargetChainEndBone(ChainElement.Pin()->ChainName);
}

FText SIKRigRetargetChainRow::GetGoalName() const
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = ChainList.Pin()->EditorController.Pin();
	if (!Controller.IsValid())
	{
		return FText::GetEmpty();
	}
	
	const FName GoalName = Controller->AssetController->GetRetargetChainGoal(ChainElement.Pin()->ChainName);
	return FText::FromName(GoalName);
}

void SIKRigRetargetChainRow::OnEndBoneComboSelectionChanged(FName InName) const
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = ChainList.Pin()->EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}
	
	Controller->AssetController->SetRetargetChainEndBone(ChainElement.Pin()->ChainName, InName);
	ChainList.Pin()->RefreshView();
}

void SIKRigRetargetChainRow::OnGoalComboSelectionChanged(TSharedPtr<FString> InGoalName, ESelectInfo::Type SelectInfo)
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = ChainList.Pin()->EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}

	Controller->AssetController->SetRetargetChainGoal(ChainElement.Pin()->ChainName, FName(*InGoalName.Get()));
	ChainList.Pin()->RefreshView();
}

void SIKRigRetargetChainRow::OnRenameChain(const FText& InText, ETextCommit::Type CommitType) const
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = ChainList.Pin()->EditorController.Pin();
	if (!Controller.IsValid())
	{
		return; 
	}

	// prevent setting name to the same name
	const FName OldName = ChainElement.Pin()->ChainName;
	const FName NewName = FName(*InText.ToString());
	if (OldName == NewName)
	{
		return;
	}

	// prevent multiple commits when pressing enter
	if (CommitType == ETextCommit::OnEnter)
	{
		return;
	}
	
	ChainElement.Pin()->ChainName = Controller->AssetController->RenameRetargetChain(OldName, NewName);
	ChainList.Pin()->RefreshView();
}

const FReferenceSkeleton& SIKRigRetargetChainRow::GetReferenceSkeleton() const
{
	static const FReferenceSkeleton DummySkeleton;
	
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = ChainList.Pin()->EditorController.Pin();
	if (!Controller.IsValid())
	{
		return DummySkeleton; 
	}

	USkeletalMesh* SkeletalMesh = Controller->AssetController->GetSkeletalMesh();
	if (SkeletalMesh == nullptr)
	{
		return DummySkeleton;
	}

	return SkeletalMesh->GetRefSkeleton();
}

void TLLRN_SIKRigRetargetChainList::Construct(const FArguments& InArgs, TSharedRef<FTLLRN_IKRigEditorController> InEditorController)
{
	EditorController = InEditorController;
	EditorController.Pin()->SetRetargetingView(SharedThis(this));

	TextFilter = MakeShareable(new FTextFilterExpressionEvaluator(ETextFilterExpressionEvaluatorMode::BasicString));
	
	CommandList = MakeShared<FUICommandList>();

	ChildSlot
    [
        SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .AutoHeight()
        .VAlign(VAlign_Top)
        [
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(3.0f, 3.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RetargetRootLabel", "Retarget Root:"))
				.TextStyle(FAppStyle::Get(), "NormalText")
			]
				
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(3.0f, 3.0f)
			[
				SAssignNew(RetargetRootTextBox, SEditableTextBox)
				.Text(FText::FromName(InEditorController->AssetController->GetRetargetRoot()))
				.Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))
				.IsReadOnly(true)
			]
        ]

        +SVerticalBox::Slot()
		.Padding(2.0f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(6.f, 0.0))
			[
				SNew(SPositiveActionButton)
				.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
				.Text(LOCTEXT("AddNewChainLabel", "Add New Chain"))
				.ToolTipText(LOCTEXT("AddNewChainToolTip", "Add a new retarget bone chain."))
				.OnClicked_Lambda([this]()
				{
					EditorController.Pin()->CreateNewRetargetChains();
					return FReply::Handled();
				})
			]

			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSearchBox)
				.SelectAllTextWhenFocused(true)
				.OnTextChanged( this, &TLLRN_SIKRigRetargetChainList::OnFilterTextChanged )
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
				.OnGetMenuContent(this, &TLLRN_SIKRigRetargetChainList::CreateFilterMenuWidget)
				.HasDownArrow(true)
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Settings"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		]

        +SVerticalBox::Slot()
		[
			SAssignNew(ListView, SRetargetChainListViewType )
			.SelectionMode(ESelectionMode::Multi)
			.IsEnabled(this, &TLLRN_SIKRigRetargetChainList::IsAddChainEnabled)
			.ListItemsSource( &ListViewItems )
			.OnGenerateRow( this, &TLLRN_SIKRigRetargetChainList::MakeListRowWidget )
			.OnMouseButtonClick(this, &TLLRN_SIKRigRetargetChainList::OnItemClicked)
			.OnContextMenuOpening(this, &TLLRN_SIKRigRetargetChainList::CreateContextMenu)
			.HeaderRow
			(
				SNew( SHeaderRow )
				+ SHeaderRow::Column( ColumnId_ChainNameLabel )
				.DefaultLabel( LOCTEXT( "ChainNameColumnLabel", "Chain Name" ) )

				+ SHeaderRow::Column( ColumnId_ChainStartLabel )
				.DefaultLabel( LOCTEXT( "ChainStartColumnLabel", "Start Bone" ) )

				+ SHeaderRow::Column( ColumnId_ChainEndLabel )
				.DefaultLabel( LOCTEXT( "ChainEndColumnLabel", "End Bone" ) )

				+ SHeaderRow::Column( ColumnId_IKGoalLabel )
				.DefaultLabel( LOCTEXT( "IKGoalColumnLabel", "IK Goal" ) )
				
				+ SHeaderRow::Column( ColumnId_DeleteChainLabel )
				.DefaultLabel( LOCTEXT( "DeleteChainColumnLabel", "Delete Chain" ) )
			)
		]
    ];

	RefreshView();
}

TArray<FName> TLLRN_SIKRigRetargetChainList::GetSelectedChains() const
{
	TArray<TSharedPtr<FRetargetChainElement>> SelectedItems = ListView->GetSelectedItems();
	if (SelectedItems.IsEmpty())
	{
		return TArray<FName>();
	}

	TArray<FName> SelectedChainNames;
	for (const TSharedPtr<FRetargetChainElement>& SelectedItem : SelectedItems)
	{
		SelectedChainNames.Add(SelectedItem->ChainName);
	}
	
	return SelectedChainNames;
}

void TLLRN_SIKRigRetargetChainList::OnFilterTextChanged(const FText& SearchText)
{
	TextFilter->SetFilterText(SearchText);
	RefreshView();
}

TSharedRef<SWidget> TLLRN_SIKRigRetargetChainList::CreateFilterMenuWidget()
{
	const FUIAction FilterSingleBoneAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			ChainFilterOptions.bHideSingleBoneChains = !ChainFilterOptions.bHideSingleBoneChains;
			RefreshView();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]()
		{
			return ChainFilterOptions.bHideSingleBoneChains;
		}));

	const FUIAction FilterIKChainAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			ChainFilterOptions.bShowOnlyIKChains = !ChainFilterOptions.bShowOnlyIKChains;
			RefreshView();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]()
		{
			return ChainFilterOptions.bShowOnlyIKChains;
		}));

	const FUIAction MissingBoneChainAction = FUIAction(
		FExecuteAction::CreateLambda([this]
		{
			ChainFilterOptions.bShowOnlyMissingBoneChains = !ChainFilterOptions.bShowOnlyMissingBoneChains;
			RefreshView();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]()
		{
			return ChainFilterOptions.bShowOnlyMissingBoneChains;
		}));
	
	static constexpr bool CloseAfterSelection = true;
	FMenuBuilder MenuBuilder(CloseAfterSelection, CommandList);

	MenuBuilder.BeginSection("Chain Filters", LOCTEXT("ChainFiltersSection", "Filter"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SingleBoneLabel", "Hide Single Bone Chains"),
		LOCTEXT("SingleBoneTooltip", "Show only chains that contain multiple bones."),
		FSlateIcon(),
		FilterSingleBoneAction,
		NAME_None,
		EUserInterfaceActionType::Check);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("HasIKLabel", "Show Only IK Chains"),
		LOCTEXT("HasIKTooltip", "Show only chains that have an IK Goal."),
		FSlateIcon(),
		FilterIKChainAction,
		NAME_None,
		EUserInterfaceActionType::Check);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MissingBoneLabel", "Show Only Chains Missing Bones"),
		LOCTEXT("MissingBoneTooltip", "Show only chains that are missing either a Start or End bone."),
		FSlateIcon(),
		MissingBoneChainAction,
		NAME_None,
		EUserInterfaceActionType::Check);
	
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Clear", LOCTEXT("ClearFiltersSection", "Clear"));
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ClearFilterLabel", "Clear Filters"),
		LOCTEXT("ClearFilterTooltip", "Clear all filters to show all chains."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]
		{
			ChainFilterOptions = FTLLRN_ChainFilterOptions();
			RefreshView();
		})));

	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget();
}

bool TLLRN_SIKRigRetargetChainList::IsAddChainEnabled() const
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return false;
	}
	
	if (UTLLRN_IKRigController* AssetController = Controller->AssetController)
	{
		if (AssetController->GetTLLRN_IKRigSkeleton().BoneNames.Num() > 0)
		{
			return true;
		}
	}
	
	return false;
}

void TLLRN_SIKRigRetargetChainList::RefreshView()
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return;
	}

	auto FilterString = [this](const FString& StringToTest) ->bool
	{
		return TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(StringToTest));
	};
	
	// refresh retarget root
	RetargetRootTextBox.Get()->SetText(FText::FromName(Controller->AssetController->GetRetargetRoot()));

	// refresh list of chains
	ListViewItems.Reset();
	const TArray<FTLLRN_BoneChain>& Chains = Controller->AssetController->GetRetargetChains();
	for (const FTLLRN_BoneChain& Chain : Chains)
	{
		// apply text filter to items
		if (!(TextFilter->GetFilterText().IsEmpty() ||
			FilterString(Chain.ChainName.ToString()) ||
			FilterString(Chain.StartBone.BoneName.ToString()) ||
			FilterString(Chain.EndBone.BoneName.ToString()) ||
			FilterString(Chain.IKGoalName.ToString())))
		{
			continue;
		}

		// apply single-bone filter
		if (ChainFilterOptions.bHideSingleBoneChains &&
			Chain.StartBone == Chain.EndBone)
		{
			continue;
		}

		// apply missing-bone filter
		if (ChainFilterOptions.bShowOnlyMissingBoneChains &&
			(Chain.StartBone.BoneName != NAME_None && Chain.EndBone.BoneName != NAME_None))
		{
			continue;
		}
		
		// apply IK filter
		if (ChainFilterOptions.bShowOnlyIKChains &&
			Chain.IKGoalName == NAME_None)
		{
			continue;
		}
		
		TSharedPtr<FRetargetChainElement> ChainItem = FRetargetChainElement::Make(Chain.ChainName);
		ListViewItems.Add(ChainItem);
	}

	// select first item if none others selected
	if (ListViewItems.Num() > 0 && ListView->GetNumItemsSelected() == 0)
	{
		ListView->SetSelection(ListViewItems[0]);
	}

	ListView->RequestListRefresh();

	// refresh the tree to show updated chain column info
	Controller->RefreshTreeView();
}

TSharedRef<ITableRow> TLLRN_SIKRigRetargetChainList::MakeListRowWidget(
	TSharedPtr<FRetargetChainElement> InElement,
    const TSharedRef<STableViewBase>& OwnerTable)
{
	return InElement->MakeListRowWidget(
		OwnerTable,
		InElement.ToSharedRef(),
		SharedThis(this));
}

void TLLRN_SIKRigRetargetChainList::OnItemClicked(TSharedPtr<FRetargetChainElement> InItem)
{
	EditorController.Pin()->SetLastSelectedType(EIKRigSelectionType::RetargetChains);
}

FReply TLLRN_SIKRigRetargetChainList::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// handle deleting selected chain
	if (Key == EKeys::Delete)
	{
		TArray<TSharedPtr<FRetargetChainElement>> SelectedItems = ListView->GetSelectedItems();
		if (SelectedItems.IsEmpty())
		{
			return FReply::Unhandled();
		}
		
		const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
		if (!Controller.IsValid())
		{
			return FReply::Unhandled();
		}

		const UTLLRN_IKRigController* AssetController = Controller->AssetController;
		AssetController->RemoveRetargetChain(SelectedItems[0]->ChainName);

		RefreshView();
		
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

TSharedPtr<SWidget> TLLRN_SIKRigRetargetChainList::CreateContextMenu()
{
	static constexpr bool CloseAfterSelection = true;
	FMenuBuilder MenuBuilder(CloseAfterSelection, CommandList);

	MenuBuilder.BeginSection("Chains", LOCTEXT("ChainsSection", "Chains"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MirrorChainsLabel", "Mirror Chain"),
		LOCTEXT("MirrorChainsTooltip", "Create a new, duplicate chain on the opposite side of the skeleton."),
		FSlateIcon(),
		FUIAction( FExecuteAction::CreateSP(this, &TLLRN_SIKRigRetargetChainList::MirrorSelectedChains)));

	MenuBuilder.AddSeparator();
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SortChainsLabel", "Sort Chains"),
		LOCTEXT("SortChainsTooltip", "Sort chain list in hierarchical order. This does not affect the retargeting behavior."),
		FSlateIcon(),
		FUIAction( FExecuteAction::CreateSP(this, &TLLRN_SIKRigRetargetChainList::SortChainList)));

	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget();
}

void TLLRN_SIKRigRetargetChainList::SortChainList()
{
	const TSharedPtr<FTLLRN_IKRigEditorController> Controller = EditorController.Pin();
	if (!Controller.IsValid())
	{
		return;
	}
	
	if (const UTLLRN_IKRigController* AssetController = Controller->AssetController)
	{
		AssetController->SortRetargetChains();
		RefreshView();
	}
}

void TLLRN_SIKRigRetargetChainList::MirrorSelectedChains() const
{
	const TArray<FName> SelectedChainNames = GetSelectedChains();
	if (SelectedChainNames.IsEmpty())
	{
		return;
	}

	const FTLLRN_IKRigEditorController& Controller = *EditorController.Pin();
	const UTLLRN_IKRigController* AssetController = Controller.AssetController;
	const FTLLRN_IKRigSkeleton& TLLRN_IKRigSkeleton = AssetController->GetTLLRN_IKRigSkeleton();

	FScopedTransaction Transaction(LOCTEXT("MirrorRetargetChain_Label", "Mirror Retarget Chains"));
	FTLLRN_ScopedReinitializeIKRig Reinitialize(AssetController, true /*bGoalsChanged*/);
	AssetController->GetAsset()->Modify();

	for (const FName& SelectedChainName : SelectedChainNames)
	{
		const FTLLRN_BoneChain* Chain = AssetController->GetRetargetChainByName(SelectedChainName);
		if (!Chain)
		{
			continue;
		}
		
		TArray<int32> BonesInChainIndices;
		const bool bIsChainValid = TLLRN_IKRigSkeleton.ValidateChainAndGetBones(*Chain, BonesInChainIndices);
		if (!bIsChainValid)
		{
			continue;
		}

		const EChainSide ChainSide = Controller.ChainAnalyzer.GetSideOfChain(BonesInChainIndices, TLLRN_IKRigSkeleton);
		if (ChainSide == EChainSide::Center)
		{
			continue;
		}

		TArray<int32> MirroredIndices;
		if (!TLLRN_IKRigSkeleton.GetMirroredBoneIndices(BonesInChainIndices, MirroredIndices))
		{
			continue;
		}

		FTLLRN_BoneChain MirroredChain = *Chain;
		MirroredChain.ChainName = UMirrorDataTable::GetSettingsMirrorName(MirroredChain.ChainName);
		MirroredChain.StartBone = TLLRN_IKRigSkeleton.BoneNames[MirroredIndices[0]];
		MirroredChain.EndBone = TLLRN_IKRigSkeleton.BoneNames[MirroredIndices.Last()];
		MirroredChain.IKGoalName = AssetController->GetGoalNameForBone(MirroredChain.EndBone.BoneName);

		const FName NewChainName = Controller.PromptToAddNewRetargetChain(MirroredChain);
		const FTLLRN_BoneChain* NewChain = AssetController->GetRetargetChainByName(NewChainName);
		if (!NewChain)
		{
			// user cancelled mirroring the chain
			continue;
		}
		
		// check old bone has a goal, and the new bone also has a goal
		// so we can connect the new goal to the same solver(s)
		const FName GoalOnOldChainName = AssetController->GetGoalNameForBone(Chain->EndBone.BoneName);
		const UTLLRN_IKRigEffectorGoal* GoalOnNewChain = AssetController->GetGoal(NewChain->IKGoalName);
		if (GoalOnOldChainName != NAME_None && GoalOnNewChain)
		{
			// connect to the same solvers
			const TArray<UTLLRN_IKRigSolver*>& AllSolvers =  AssetController->GetSolverArray();
			for (int32 SolverIndex=0; SolverIndex<AllSolvers.Num(); ++SolverIndex)
			{
				if (AssetController->IsGoalConnectedToSolver(GoalOnOldChainName, SolverIndex))
				{
					AssetController->ConnectGoalToSolver(GoalOnNewChain->GoalName, SolverIndex);
				}
				else
				{
					AssetController->DisconnectGoalFromSolver(GoalOnNewChain->GoalName, SolverIndex);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

