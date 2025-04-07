// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/STLLRN_ModularRigTreeView.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Editor/EditorEngine.h"
#include "HelperUtil.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "TLLRN_ModularRig.h"
#include "TLLRN_ModularRigRuleManager.h"
#include "STLLRN_RigHierarchyTreeView.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Editor/STLLRN_ModularRigModel.h"
#include "Fonts/FontMeasure.h"
#include "Settings/TLLRN_ControlRigSettings.h"
#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "Rigs/AdditiveTLLRN_ControlRig.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SButton.h"
#include "ScopedTransaction.h"
#include "DetailLayoutBuilder.h"
#include "Widgets/SRigVMVariantTagWidget.h"

#define LOCTEXT_NAMESPACE "STLLRN_ModularRigTreeView"

TMap<FSoftObjectPath, TSharedPtr<FSlateBrush>> FTLLRN_ModularRigTreeElement::IconPathToBrush;

//////////////////////////////////////////////////////////////
/// FTLLRN_ModularRigTreeElement
///////////////////////////////////////////////////////////
FTLLRN_ModularRigTreeElement::FTLLRN_ModularRigTreeElement(const FString& InKey, TWeakPtr<STLLRN_ModularRigTreeView> InTreeView, bool bInIsPrimary)
{
	Key = InKey;
	bIsPrimary = bInIsPrimary;

	FString ShortNameStr = Key;
	(void)UTLLRN_RigHierarchy::SplitNameSpace(ShortNameStr, &ModulePath, &ShortNameStr);
	if (bIsPrimary)
	{
		ModulePath = Key;
		if(const UTLLRN_ModularRig* TLLRN_ModularRig = InTreeView.Pin()->GetRigTreeDelegates().GetTLLRN_ModularRig())
		{
			if (const FTLLRN_RigModuleInstance* Module = TLLRN_ModularRig->FindModule(ModulePath))
			{
				if (const UTLLRN_ControlRig* Rig = Module->GetRig())
				{
					if (const FTLLRN_RigModuleConnector* PrimaryConnector = Rig->GetTLLRN_RigModuleSettings().FindPrimaryConnector())
					{
						ConnectorName = PrimaryConnector->Name;
					}
				}
			}
		}
	}
	else
	{
		ConnectorName = ShortNameStr;
	}
	
	ShortName = *ShortNameStr;
	
	if(InTreeView.IsValid())
	{
		if(const UTLLRN_ModularRig* TLLRN_ModularRig = InTreeView.Pin()->GetRigTreeDelegates().GetTLLRN_ModularRig())
		{
			RefreshDisplaySettings(TLLRN_ModularRig);
		}
	}
}

void FTLLRN_ModularRigTreeElement::RefreshDisplaySettings(const UTLLRN_ModularRig* InTLLRN_ModularRig)
{
	const TPair<const FSlateBrush*, FSlateColor> Result = GetBrushAndColor(InTLLRN_ModularRig);

	IconBrush = Result.Key;
	IconColor = Result.Value;
	TextColor = FSlateColor::UseForeground();
}

TSharedRef<ITableRow> FTLLRN_ModularRigTreeElement::MakeTreeRowWidget(const TSharedRef<STableViewBase>& InOwnerTable, TSharedRef<FTLLRN_ModularRigTreeElement> InRigTreeElement, TSharedPtr<STLLRN_ModularRigTreeView> InTreeView, bool bPinned)
{
	return SNew(STLLRN_ModularRigModelItem, InOwnerTable, InRigTreeElement, InTreeView, bPinned);
}

void FTLLRN_ModularRigTreeElement::RequestRename()
{
	OnRenameRequested.ExecuteIfBound();
}

//////////////////////////////////////////////////////////////
/// STLLRN_ModularRigModelItem
///////////////////////////////////////////////////////////
void STLLRN_ModularRigModelItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, TSharedRef<FTLLRN_ModularRigTreeElement> InRigTreeElement, TSharedPtr<STLLRN_ModularRigTreeView> InTreeView, bool bPinned)
{
	WeakRigTreeElement = InRigTreeElement;
	Delegates = InTreeView->GetRigTreeDelegates();

	if (InRigTreeElement->Key.IsEmpty())
	{
		SMultiColumnTableRow<TSharedPtr<FTLLRN_ModularRigTreeElement>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FTLLRN_ModularRigTreeElement>>::FArguments()
			.ShowSelection(false)
			.OnCanAcceptDrop(Delegates.OnCanAcceptDrop)
			.OnAcceptDrop(Delegates.OnAcceptDrop)
			, OwnerTable);
		return;
	}

	const FString& ModulePath = InRigTreeElement->ModulePath;
	FString ConnectorPath = FString::Printf(TEXT("%s:%s"), *ModulePath, *InRigTreeElement->ConnectorName);
	ConnectorKey = FTLLRN_RigElementKey(*ConnectorPath, ETLLRN_RigElementType::Connector);

	SMultiColumnTableRow<TSharedPtr<FTLLRN_ModularRigTreeElement>>::Construct(
		SMultiColumnTableRow<TSharedPtr<FTLLRN_ModularRigTreeElement>>::FArguments()
		.OnDragDetected(Delegates.OnDragDetected)
		.OnCanAcceptDrop(Delegates.OnCanAcceptDrop)
		.OnAcceptDrop(Delegates.OnAcceptDrop)
		.ShowWires(true), OwnerTable);
}


void STLLRN_ModularRigModelItem::PopulateConnectorTargetList(const FTLLRN_RigElementKey InConnectorKey)
{
	if (!WeakRigTreeElement.IsValid())
	{
		return;
	}

	if (const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig())
	{
		if (UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy())
		{
			if (Cast<FTLLRN_RigConnectorElement>(Hierarchy->Find(InConnectorKey)))
			{
				ConnectorComboBox->GetTreeView()->RefreshTreeView(true);

				// set the focus to the search box so you can start typing right away
				RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateLambda([this](double,float)
				{
					FSlateApplication::Get().ForEachUser([this](FSlateUser& User)
					{
						User.SetFocus(ConnectorComboBox->GetSearchBox());
					});
					return EActiveTimerReturnType::Stop;
				}));
			}
		}
	}
}

void STLLRN_ModularRigModelItem::PopulateConnectorCurrentTarget(TSharedPtr<SVerticalBox> InListBox, const FTLLRN_RigElementKey& InConnectorKey, const FTLLRN_RigElementKey& InTargetKey, const FSlateBrush* InBrush, const FSlateColor& InColor, const FText& InTitle)
{
	static const FSlateBrush* RoundedBoxBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush(TEXT("TLLRN_ControlRig.SpacePicker.RoundedRect"));

	TAttribute<FSlateColor> TextColor = TAttribute<FSlateColor>::CreateLambda([this, InConnectorKey, InTargetKey]() -> FSlateColor
	{
		if(const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig())
		{
			if(const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy())
			{
				if(!Hierarchy->Contains(InTargetKey))
				{
					if(!InTargetKey.IsValid())
					{
						if(const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(InConnectorKey))
						{
							if(Connector->IsOptional())
							{
								return FSlateColor::UseForeground();
							}
						}
					}
					return FLinearColor::Red;
				}
			}
		}
		return FSlateColor::UseForeground();
	});
	
	TSharedPtr<SHorizontalBox> RowBox, ButtonBox;
	InListBox->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Left)
	.Padding(0.0, 0.0, 4.0, 0.0)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.Padding(0, 0, 0, 0)
		[
			SNew( SButton )
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(FMargin(0.0))
			.OnClicked_Lambda([this, InConnectorKey, InTargetKey]()
			{
				//Controller->ConnectConnectorToElement(InConnectorKey, InTargetKey, true, Info.GetTLLRN_ModularRig()->GetTLLRN_ModularRigSettings().bAutoResolve);
				PopulateConnectorTargetList(InConnectorKey);
				return FReply::Handled();
			})
			[
				SAssignNew(RowBox, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
				.Padding(0)
				[
					SNew(SBorder)
					.Padding(FMargin(2.0, 2.0, 5.0, 2.0))
					.BorderImage(RoundedBoxBrush)
					.BorderBackgroundColor(FSlateColor(FLinearColor::Transparent))
					.Content()
					[
						SAssignNew(ButtonBox, SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
						[
							SNew(SImage)
							.Image(InBrush)
							.ColorAndOpacity(InColor)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(0)
						[
							SNew( STextBlock )
							.Text( InTitle )
							.Font( IDetailLayoutBuilder::GetDetailFont() )
							.ColorAndOpacity(TextColor)
						]
					]
				]
			]
		]
	];
}

void STLLRN_ModularRigModelItem::OnConnectorTargetChanged(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo, const FTLLRN_RigElementKey InConnectorKey)
{
	if (!Selection.IsValid() || SelectInfo == ESelectInfo::OnNavigation)
	{
		return;
	}
	
	FScopedTransaction Transaction(LOCTEXT("ModuleHierarchyResolveConnector", "Resolve Connector"));
	TSharedPtr<FRigTreeElement> NewSelection = Selection;
	Delegates.HandleResolveConnector(InConnectorKey, NewSelection->Key);
}

void STLLRN_ModularRigModelItem::OnNameCommitted(const FText& InText, ETextCommit::Type InCommitType) const
{
	// for now only allow enter
	// because it is important to keep the unique names per pose
	if (InCommitType == ETextCommit::OnEnter)
	{
		FString NewName = InText.ToString();
		const FString OldKey = WeakRigTreeElement.Pin()->ModulePath;

		Delegates.HandleRenameElement(OldKey, *NewName);
	}
}

bool STLLRN_ModularRigModelItem::OnVerifyNameChanged(const FText& InText, FText& OutErrorMessage)
{
	const FName NewName = *InText.ToString();
	const FString OldPath = WeakRigTreeElement.Pin()->ModulePath;
	return Delegates.HandleVerifyElementNameChanged(OldPath, NewName, OutErrorMessage);
}

TSharedRef<SWidget> STLLRN_ModularRigModelItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	if(ColumnName == STLLRN_ModularRigTreeView::Column_Module)
	{
		TSharedPtr< SInlineEditableTextBlock > InlineWidget;

		TSharedRef<SWidget> Widget = SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(6, 0, 0, 0)
		.VAlign(VAlign_Fill)
		[
			SNew(SExpanderArrow, SharedThis(this))
			.IndentAmount(12)
			.ShouldDrawWires(true)
		]

		+SHorizontalBox::Slot()
		.MaxWidth(25)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.MaxHeight(25)
			[
				SNew(SImage)
				.Image_Lambda([this]() -> const FSlateBrush*
				{
					if(WeakRigTreeElement.IsValid())
					{
						return WeakRigTreeElement.Pin()->IconBrush;
					}
					return nullptr;
				})
				.ColorAndOpacity_Lambda([this]()
				{
					if(WeakRigTreeElement.IsValid())
					{
						return WeakRigTreeElement.Pin()->IconColor;
					}
					return FSlateColor::UseForeground();
				})
				.DesiredSizeOverride(FVector2D(16, 16))
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew(InlineWidget, SInlineEditableTextBlock)
			.Text(this, &STLLRN_ModularRigModelItem::GetName, true)
			.OnVerifyTextChanged(this, &STLLRN_ModularRigModelItem::OnVerifyNameChanged)
			.OnTextCommitted(this, &STLLRN_ModularRigModelItem::OnNameCommitted)
			.ToolTipText(this, &STLLRN_ModularRigModelItem::GetItemTooltip)
			.MultiLine(false)
			//.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity_Lambda([this]()
			{
				if(WeakRigTreeElement.IsValid())
				{
					return WeakRigTreeElement.Pin()->TextColor;
				}
				return FSlateColor::UseForeground();
			})
		];

		if(WeakRigTreeElement.IsValid())
		{
			WeakRigTreeElement.Pin()->OnRenameRequested.BindSP(InlineWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);
		}

		return Widget;
	}
	if (ColumnName == STLLRN_ModularRigTreeView::Column_Tags)
	{
		TSharedRef<SWidget> Widget = SNew(SImage)
		.Visibility_Lambda([this]() -> EVisibility
		{
			if (WeakRigTreeElement.IsValid())
			{
				if (!WeakRigTreeElement.Pin()->bIsPrimary)
				{
					return EVisibility::Hidden;
				}
			
				if (const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig())
				{
					if (const FTLLRN_RigModuleInstance* Module = TLLRN_ModularRig->FindModule(WeakRigTreeElement.Pin()->ModulePath))
					{
						if (const UTLLRN_ControlRigBlueprint* ModuleBlueprint = Cast<UTLLRN_ControlRigBlueprint>(Module->GetRig()->GetClass()->ClassGeneratedBy))
						{
							for (const FRigVMTag& Tag : ModuleBlueprint->GetAssetVariant().Tags)
							{
								if (Tag.bMarksSubjectAsInvalid)
								{
									return EVisibility::Visible;
								}
							}
						}
					}
				}
			}
			return EVisibility::Hidden;
		})
		.ToolTipText_Lambda([this]() -> FText
		{
			TArray<FString> ToolTip;
			if (WeakRigTreeElement.IsValid())
			{
				if (const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig())
				{
					if (const FTLLRN_RigModuleInstance* Module = TLLRN_ModularRig->FindModule(WeakRigTreeElement.Pin()->ModulePath))
					{
						if (const UTLLRN_ControlRigBlueprint* ModuleBlueprint = Cast<UTLLRN_ControlRigBlueprint>(Module->GetRig()->GetClass()->ClassGeneratedBy))
						{
							for (const FRigVMTag& Tag : ModuleBlueprint->GetAssetVariant().Tags)
							{
								if (Tag.bMarksSubjectAsInvalid)
								{
									ToolTip.Add(FString::Printf(TEXT("%s: %s"), *Tag.Label, *Tag.ToolTip.ToString()));
								}
							}
						}
					}
				}
			}
			return FText::FromString(FString::Join(ToolTip, TEXT("\n")));
		})
		.Image_Lambda([this]() -> const FSlateBrush*
		{
			const FSlateBrush* WarningBrush = FAppStyle::Get().GetBrush("Icons.WarningWithColor");
			return WarningBrush;
		})
		.DesiredSizeOverride(FVector2D(16, 16));

		return Widget;
	}
	if(ColumnName == STLLRN_ModularRigTreeView::Column_Connector)
	{
		TSharedPtr<SVerticalBox> ComboButtonBox;

		FRigTreeDelegates TreeDelegates;
		TreeDelegates.OnGetHierarchy = FOnGetRigTreeHierarchy::CreateLambda([this]()
		{
			return Delegates.GetTLLRN_ModularRig()->GetHierarchy();
		});
		TreeDelegates.OnRigTreeIsItemVisible = FOnRigTreeIsItemVisible::CreateLambda([this](const FTLLRN_RigElementKey& InTarget)
		{
			if(!ConnectorMatches.IsSet() && WeakRigTreeElement.IsValid())
			{
				if (const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig())
				{
					if (const FTLLRN_RigModuleInstance* Module = TLLRN_ModularRig->FindModule(WeakRigTreeElement.Pin()->ModulePath))
					{
						if (UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy())
						{
							if (const FTLLRN_RigConnectorElement* ConnectorElement = Cast<FTLLRN_RigConnectorElement>(Hierarchy->Find(ConnectorKey)))
							{
								const UTLLRN_ModularRigRuleManager* RuleManager = TLLRN_ModularRig->GetHierarchy()->GetRuleManager();
								ConnectorMatches = RuleManager->FindMatches(ConnectorElement, Module, TLLRN_ModularRig->GetElementKeyRedirector());
							}
						}
					}
				}
			}

			if(ConnectorMatches.IsSet())
			{
				return ConnectorMatches.GetValue().ContainsMatch(InTarget); 
			}
			return true;
		});
		TreeDelegates.OnGetSelection.BindLambda([this]() -> TArray<FTLLRN_RigElementKey>
		{
			const FTLLRN_RigElementKeyRedirector Redirector = Delegates.GetTLLRN_ModularRig()->GetElementKeyRedirector();
			if (const FTLLRN_RigElementKey* Key = Redirector.FindExternalKey(ConnectorKey))
			{
				return {*Key};
			}
			return {};
		});
		TreeDelegates.OnSelectionChanged.BindSP(this, &STLLRN_ModularRigModelItem::OnConnectorTargetChanged, ConnectorKey);

		TSharedRef<SWidget> Widget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew( SComboButton )
			.ContentPadding(3)
			.MenuPlacement(MenuPlacement_BelowAnchor)
			.OnComboBoxOpened(this, &STLLRN_ModularRigModelItem::PopulateConnectorTargetList, ConnectorKey)
			.ButtonContent()
			[
				// Wrap in configurable box to restrain height/width of menu
				SNew(SBox)
				.MinDesiredWidth(200.0f)
				.Padding(0, 0, 0, 0)
				.HAlign(HAlign_Left)
				[
					SAssignNew(ComboButtonBox, SVerticalBox)
				]
			]
			.MenuContent()
			[
				SNew(SBorder)
				.Visibility(EVisibility::Visible)
				.BorderImage(FAppStyle::GetBrush("Menu.Background"))
				[
					SAssignNew(ConnectorComboBox, SSearchableTLLRN_RigHierarchyTreeView)
						.RigTreeDelegates(TreeDelegates)
						.MaxHeight(300)
				]
			]
		];

		const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig();
		const FTLLRN_RigElementKeyRedirector& Redirector = TLLRN_ModularRig->GetElementKeyRedirector();
		FTLLRN_RigElementKey CurrentTargetKey;
		if (const FTLLRN_RigElementKey* Key = Redirector.FindExternalKey(ConnectorKey))
		{
			CurrentTargetKey = *Key;
		}
		TPair<const FSlateBrush*, FSlateColor> IconAndColor = STLLRN_RigHierarchyItem::GetBrushForElementType(TLLRN_ModularRig->GetHierarchy(), CurrentTargetKey);

		FText CurrentTargetShortName = FText::FromString(TLLRN_ModularRig->GetShortestDisplayPathForElement(CurrentTargetKey, false));
		if(CurrentTargetShortName.IsEmpty())
		{
			CurrentTargetShortName = FText::FromName(CurrentTargetKey.Name);
		}
		PopulateConnectorCurrentTarget(ComboButtonBox, ConnectorKey, CurrentTargetKey, IconAndColor.Key, IconAndColor.Value, CurrentTargetShortName);

		return Widget;
	}
	if(ColumnName == STLLRN_ModularRigTreeView::Column_Buttons)
	{
		return SNew(SHorizontalBox)

		// Reset button
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 0.f, 0.f)
		[
			SAssignNew(ResetConnectorButton, SButton)
			.ButtonStyle( FAppStyle::Get(), "NoBorder" )
			.ButtonColorAndOpacity_Lambda([this]()
			{
				return ResetConnectorButton.IsValid() && ResetConnectorButton->IsHovered()
					? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
					: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
			})
			.OnClicked_Lambda([this]()
			{
				Delegates.HandleDisconnectConnector(ConnectorKey);
				return FReply::Handled();
			})
			.ContentPadding(1.f)
			.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigModuleDetails", "Reset_Connector", "Reset Connector"))
			[
				SNew(SImage)
				.ColorAndOpacity_Lambda( [this]()
				{
					return ResetConnectorButton.IsValid() && ResetConnectorButton->IsHovered()
					? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
					: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
				})
				.Image(FSlateIcon(FAppStyle::Get().GetStyleSetName(), "PropertyWindow.DiffersFromDefault").GetIcon())
			]
		]

		// Use button
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 0.f, 0.f)
		[
			SAssignNew(UseSelectedButton, SButton)
			.ButtonStyle( FAppStyle::Get(), "NoBorder" )
			.ButtonColorAndOpacity_Lambda([this]()
			{
				return UseSelectedButton.IsValid() && UseSelectedButton->IsHovered()
					? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
					: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
			})
			.OnClicked_Lambda([this]()
			{
				if (const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig())
				{
					const TArray<FTLLRN_RigElementKey>& Selected = TLLRN_ModularRig->GetHierarchy()->GetSelectedKeys();
					if (Selected.Num() > 0)
					{
						Delegates.HandleResolveConnector(ConnectorKey, Selected[0]);
					}
				}
				return FReply::Handled();
			})
			.ContentPadding(1.f)
			.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigModuleDetails", "Use_Selected", "Use Selected"))
			[
				SNew(SImage)
				.ColorAndOpacity_Lambda( [this]()
				{
					return UseSelectedButton.IsValid() && UseSelectedButton->IsHovered()
					? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
					: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
				})
				.Image(FAppStyle::GetBrush("Icons.CircleArrowLeft"))
			]
		]
		
		// Select in hierarchy button
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 0.f, 0.f)
		[
			SAssignNew(SelectElementButton, SButton)
			.ButtonStyle( FAppStyle::Get(), "NoBorder" )
			.ButtonColorAndOpacity_Lambda([this]()
			{
				return SelectElementButton.IsValid() && SelectElementButton->IsHovered()
					? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
					: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
			})
			.OnClicked_Lambda([this]()
			{
				if (const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig())
				{
					const FTLLRN_RigElementKeyRedirector& Redirector = TLLRN_ModularRig->GetElementKeyRedirector();
					if (const FTLLRN_RigElementKey* TargetKey = Redirector.FindExternalKey(ConnectorKey))
					{
						TLLRN_ModularRig->GetHierarchy()->GetController()->SelectElement(*TargetKey, true, true);
					}
				}
				return FReply::Handled();
			})
			.ContentPadding(1.f)
			.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigModuleDetails", "Select_Element", "Select Element"))
			[
				SNew(SImage)
				.ColorAndOpacity_Lambda( [this]()
				{
					return SelectElementButton.IsValid() && SelectElementButton->IsHovered()
					? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
					: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
				})
				.Image(FAppStyle::GetBrush("Icons.Search"))
			]
		];
	}
	return SNullWidget::NullWidget;
}

FText STLLRN_ModularRigModelItem::GetName(bool bUseShortName) const
{
	if(bUseShortName)
	{
		return (FText::FromName(WeakRigTreeElement.Pin()->ShortName));
	}
	return (FText::FromString(WeakRigTreeElement.Pin()->ModulePath));
}

FText STLLRN_ModularRigModelItem::GetItemTooltip() const
{
	const FText FullName = GetName(false);
	const FText ShortName = GetName(true);
	if(FullName.EqualTo(ShortName))
	{
		return FText();
	}
	return FullName;
}

//////////////////////////////////////////////////////////////
/// STLLRN_ModularRigTreeView
///////////////////////////////////////////////////////////

const FName STLLRN_ModularRigTreeView::Column_Module = TEXT("Module");
const FName STLLRN_ModularRigTreeView::Column_Tags = TEXT("Tags");
const FName STLLRN_ModularRigTreeView::Column_Connector = TEXT("Connector");
const FName STLLRN_ModularRigTreeView::Column_Buttons = TEXT("Actions");

void STLLRN_ModularRigTreeView::Construct(const FArguments& InArgs)
{
	Delegates = InArgs._RigTreeDelegates;
	bAutoScrollEnabled = InArgs._AutoScrollEnabled;

	FilterText = InArgs._FilterText;
	ShowSecondaryConnectors = InArgs._ShowSecondaryConnectors;
	ShowOptionalConnectors = InArgs._ShowOptionalConnectors;
	ShowUnresolvedConnectors = InArgs._ShowUnresolvedConnectors;

	STreeView<TSharedPtr<FTLLRN_ModularRigTreeElement>>::FArguments SuperArgs;
	SuperArgs.HeaderRow(InArgs._HeaderRow);
	SuperArgs.TreeItemsSource(&RootElements);
	SuperArgs.SelectionMode(ESelectionMode::Multi);
	SuperArgs.OnGenerateRow(this, &STLLRN_ModularRigTreeView::MakeTableRowWidget, false);
	SuperArgs.OnGetChildren(this, &STLLRN_ModularRigTreeView::HandleGetChildrenForTree);
	SuperArgs.OnSelectionChanged(FOnTLLRN_ModularRigTreeSelectionChanged::CreateRaw(&Delegates, &FTLLRN_ModularRigTreeDelegates::HandleSelectionChanged));
	SuperArgs.OnContextMenuOpening(Delegates.OnContextMenuOpening);
	SuperArgs.HighlightParentNodesForSelection(true);
	SuperArgs.AllowInvisibleItemSelection(true);  //without this we deselect everything when we filter or we collapse
	SuperArgs.OnMouseButtonClick(Delegates.OnMouseButtonClick);
	SuperArgs.OnMouseButtonDoubleClick(Delegates.OnMouseButtonDoubleClick);
	
	SuperArgs.ShouldStackHierarchyHeaders_Lambda([]() -> bool {
		return UTLLRN_ControlRigEditorSettings::Get()->bShowStackedHierarchy;
	});
	SuperArgs.OnGeneratePinnedRow(this, &STLLRN_ModularRigTreeView::MakeTableRowWidget, true);
	SuperArgs.MaxPinnedItems_Lambda([]() -> int32
	{
		return FMath::Max<int32>(1, UTLLRN_ControlRigEditorSettings::Get()->MaxStackSize);
	});

	STreeView<TSharedPtr<FTLLRN_ModularRigTreeElement>>::Construct(SuperArgs);

	LastMousePosition = FVector2D::ZeroVector;
	TimeAtMousePosition = 0.0;
}

void STLLRN_ModularRigTreeView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	STreeView<TSharedPtr<FTLLRN_ModularRigTreeElement, ESPMode::ThreadSafe>>::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	const FGeometry PaintGeometry = GetPaintSpaceGeometry();
	const FVector2D MousePosition = FSlateApplication::Get().GetCursorPos();

	if(PaintGeometry.IsUnderLocation(MousePosition))
	{
		const FVector2D WidgetPosition = PaintGeometry.AbsoluteToLocal(MousePosition);

		static constexpr float SteadyMousePositionTolerance = 5.f;

		if(LastMousePosition.Equals(MousePosition, SteadyMousePositionTolerance))
		{
			TimeAtMousePosition += InDeltaTime;
		}
		else
		{
			LastMousePosition = MousePosition;
			TimeAtMousePosition = 0.0;
		}

		static constexpr float AutoScrollStartDuration = 0.5f; // in seconds
		static constexpr float AutoScrollDistance = 24.f; // in pixels
		static constexpr float AutoScrollSpeed = 150.f;

		if(TimeAtMousePosition > AutoScrollStartDuration && FSlateApplication::Get().IsDragDropping())
		{
			if((WidgetPosition.Y < AutoScrollDistance) || (WidgetPosition.Y > PaintGeometry.Size.Y - AutoScrollDistance))
			{
				if(bAutoScrollEnabled)
				{
					const bool bScrollUp = (WidgetPosition.Y < AutoScrollDistance);

					const float DeltaInSlateUnits = (bScrollUp ? -InDeltaTime : InDeltaTime) * AutoScrollSpeed; 
					ScrollBy(GetCachedGeometry(), DeltaInSlateUnits, EAllowOverscroll::No);
				}
			}
			else
			{
				const TSharedPtr<FTLLRN_ModularRigTreeElement>* Item = FindItemAtPosition(MousePosition);
				if(Item && Item->IsValid())
				{
					if(!IsItemExpanded(*Item))
					{
						SetItemExpansion(*Item, true);
					}
				}
			}
		}
	}

	if (bRequestRenameSelected)
	{
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateLambda([this](double, float) {
			TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedItems = GetSelectedItems();
			if (SelectedItems.Num() == 1)
			{
				SelectedItems[0]->RequestRename();
			}
			return EActiveTimerReturnType::Stop;
		}));
		bRequestRenameSelected = false;
	}
}

TSharedPtr<FTLLRN_ModularRigTreeElement> STLLRN_ModularRigTreeView::FindElement(const FString& InElementKey)
{
	for (TSharedPtr<FTLLRN_ModularRigTreeElement> Root : RootElements)
	{
		if (TSharedPtr<FTLLRN_ModularRigTreeElement> Found = FindElement(InElementKey, Root))
		{
			return Found;
		}
	}

	return TSharedPtr<FTLLRN_ModularRigTreeElement>();
}

TSharedPtr<FTLLRN_ModularRigTreeElement> STLLRN_ModularRigTreeView::FindElement(const FString& InElementKey, TSharedPtr<FTLLRN_ModularRigTreeElement> CurrentItem)
{
	if (CurrentItem->Key == InElementKey)
	{
		return CurrentItem;
	}

	for (int32 ChildIndex = 0; ChildIndex < CurrentItem->Children.Num(); ++ChildIndex)
	{
		TSharedPtr<FTLLRN_ModularRigTreeElement> Found = FindElement(InElementKey, CurrentItem->Children[ChildIndex]);
		if (Found.IsValid())
		{
			return Found;
		}
	}

	return TSharedPtr<FTLLRN_ModularRigTreeElement>();
}

bool STLLRN_ModularRigTreeView::AddElement(FString InKey, FString InParentKey, bool bApplyFilterText)
{
	if(ElementMap.Contains(InKey))
	{
		return false;
	}

	if (!InKey.IsEmpty())
	{
		const FString ModulePath = InKey;

		bool bFilteredOutElement = false;
		const FString FilterTextString = FilterText.Get().ToString();
		if(!FilterTextString.IsEmpty())
		{
			FString StringToSearch = InKey;
			(void)UTLLRN_RigHierarchy::SplitNameSpace(StringToSearch, nullptr, &StringToSearch);
			
			if(!StringToSearch.Contains(FilterTextString, ESearchCase::IgnoreCase))
			{
				bFilteredOutElement = true;
			}
		}

		TArray<FString> FilteredConnectors;
		if (const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig())
		{
			const FTLLRN_ModularRigModel& Model = TLLRN_ModularRig->GetTLLRN_ModularRigModel();
			
			if (const FTLLRN_RigModuleInstance* Module = TLLRN_ModularRig->FindModule(ModulePath))
			{
				if (const UTLLRN_ControlRig* ModuleRig = Module->GetRig())
				{
					const UTLLRN_ControlRig* CDO = ModuleRig->GetClass()->GetDefaultObject<UTLLRN_ControlRig>();
					const TArray<FTLLRN_RigModuleConnector>& Connectors = CDO->GetTLLRN_RigModuleSettings().ExposedConnectors;

					for (const FTLLRN_RigModuleConnector& Connector : Connectors)
					{
						if (Connector.IsPrimary())
						{
							continue;
						}

						const FString Key = FString::Printf(TEXT("%s:%s"), *ModulePath, *Connector.Name);
						bool bShouldFilterByTLLRN_ConnectorType = true;

						if(!FilterTextString.IsEmpty())
						{
							const bool bMatchesFilter = Connector.Name.Contains(FilterTextString, ESearchCase::IgnoreCase);
							if(bFilteredOutElement)
							{
								if(!bMatchesFilter)
								{
									continue;
								}
							}
							bShouldFilterByTLLRN_ConnectorType = !bMatchesFilter;
						}

						if(bShouldFilterByTLLRN_ConnectorType)
						{
							const bool bIsConnected = Model.Connections.HasConnection(FTLLRN_RigElementKey(*Key, ETLLRN_RigElementType::Connector));
							if(bIsConnected || !ShowUnresolvedConnectors.Get())
							{
								if(Connector.IsOptional())
								{
									if(ShowOptionalConnectors.Get() == false)
									{
										continue;
									}
								}
								else if(Connector.IsSecondary())
								{
									if(ShowSecondaryConnectors.Get() == false)
									{
										continue;
									}
								}
							}
						}

						FilteredConnectors.Add(Key);
					}
				}
			}
		}
		
		if(bFilteredOutElement && bApplyFilterText && FilteredConnectors.IsEmpty())
		{
			return false;
		}
		
		TSharedPtr<FTLLRN_ModularRigTreeElement> NewItem = MakeShared<FTLLRN_ModularRigTreeElement>(InKey, SharedThis(this), true);

		ElementMap.Add(InKey, NewItem);
		if (!InParentKey.IsEmpty())
		{
			ParentMap.Add(InKey, InParentKey);

			TSharedPtr<FTLLRN_ModularRigTreeElement>* FoundItem = ElementMap.Find(InParentKey);
			check(FoundItem);
			FoundItem->Get()->Children.Add(NewItem);
		}
		else
		{
			RootElements.Add(NewItem);
		}

		SetItemExpansion(NewItem, true);

		for (const FString& Key : FilteredConnectors)
		{
			TSharedPtr<FTLLRN_ModularRigTreeElement> ConnectorItem = MakeShared<FTLLRN_ModularRigTreeElement>(Key, SharedThis(this), false);
			NewItem.Get()->Children.Add(ConnectorItem);
			ElementMap.Add(Key, ConnectorItem);
			ParentMap.Add(Key, InKey);
		}
	}

	return true;
}

bool STLLRN_ModularRigTreeView::AddElement(const FTLLRN_RigModuleInstance* InElement, bool bApplyFilterText)
{
	check(InElement);
	
	if (ElementMap.Contains(InElement->GetPath()))
	{
		return false;
	}

	const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig();

	FString ElementPath = InElement->GetPath();
	if(!AddElement(ElementPath, FString(), bApplyFilterText))
	{
		return false;
	}

	if (ElementMap.Contains(InElement->GetPath()))
	{
		if(TLLRN_ModularRig)
		{
			FString ParentPath = TLLRN_ModularRig->GetParentPath(ElementPath);
			if (!ParentPath.IsEmpty())
			{
				if(const FTLLRN_RigModuleInstance* ParentElement = TLLRN_ModularRig->FindModule(ParentPath))
				{
					AddElement(ParentElement, false);

					if(ElementMap.Contains(ParentPath))
					{
						ReparentElement(ElementPath, ParentPath);
					}
				}
			}
		}
	}

	return true;
}

void STLLRN_ModularRigTreeView::AddSpacerElement()
{
	AddElement(FString(), FString());
}

bool STLLRN_ModularRigTreeView::ReparentElement(const FString InKey, const FString InParentKey)
{
	if (InKey.IsEmpty() || InKey == InParentKey)
	{
		return false;
	}

	TSharedPtr<FTLLRN_ModularRigTreeElement>* FoundItem = ElementMap.Find(InKey);
	if (FoundItem == nullptr)
	{
		return false;
	}

	if (const FString* ExistingParentKey = ParentMap.Find(InKey))
	{
		if (*ExistingParentKey == InParentKey)
		{
			return false;
		}

		if (TSharedPtr<FTLLRN_ModularRigTreeElement>* ExistingParent = ElementMap.Find(*ExistingParentKey))
		{
			(*ExistingParent)->Children.Remove(*FoundItem);
		}

		ParentMap.Remove(InKey);
	}
	else
	{
		if (InParentKey.IsEmpty())
		{
			return false;
		}

		RootElements.Remove(*FoundItem);
	}

	if (!InParentKey.IsEmpty())
	{
		ParentMap.Add(InKey, InParentKey);

		TSharedPtr<FTLLRN_ModularRigTreeElement>* FoundParent = ElementMap.Find(InParentKey);
		if(FoundParent)
		{
			FoundParent->Get()->Children.Add(*FoundItem);
		}
	}
	else
	{
		RootElements.Add(*FoundItem);
	}

	return true;
}

void STLLRN_ModularRigTreeView::RefreshTreeView(bool bRebuildContent)
{
	TMap<FString, bool> ExpansionState;
	TArray<FString> Selection;

	if(bRebuildContent)
	{
		for (TPair<FString, TSharedPtr<FTLLRN_ModularRigTreeElement>> Pair : ElementMap)
		{
			ExpansionState.FindOrAdd(Pair.Key) = IsItemExpanded(Pair.Value);
		}

		// internally save expansion states before rebuilding the tree, so the states can be restored later
		SaveAndClearSparseItemInfos();

		RootElements.Reset();
		ElementMap.Reset();
		ParentMap.Reset();

		Selection = GetSelectedKeys();
	}

	if(bRebuildContent)
	{
		const UTLLRN_ModularRig* TLLRN_ModularRig = Delegates.GetTLLRN_ModularRig();
		if(TLLRN_ModularRig)
		{
			TLLRN_ModularRig->ForEachModule([&](const FTLLRN_RigModuleInstance* Element)
			{
				AddElement(Element, true);
				return true;
			});

			// expand all elements upon the initial construction of the tree
			if (ExpansionState.Num() < ElementMap.Num())
			{
				for (const TPair<FString, TSharedPtr<FTLLRN_ModularRigTreeElement>>& Element : ElementMap)
				{
					if (!ExpansionState.Contains(Element.Key))
					{
						SetItemExpansion(Element.Value, true);
					}
				}
			}

			for (const auto& Pair : ElementMap)
			{
				RestoreSparseItemInfos(Pair.Value);
			}

			if (RootElements.Num() > 0)
			{
				AddSpacerElement();
			}
		}
	}
	else
	{
		if (RootElements.Num()> 0)
		{
			// elements may be added at the end of the list after a spacer element
			// we need to remove the spacer element and re-add it at the end
			RootElements.RemoveAll([](TSharedPtr<FTLLRN_ModularRigTreeElement> InElement)
			{
				return InElement.Get()->Key == FString();
			});
			AddSpacerElement();
		}
	}

	RequestTreeRefresh();
	{
		TGuardValue<bool> Guard(Delegates.bSuspendSelectionDelegate, true);
		ClearSelection();

		if(!Selection.IsEmpty())
		{
			TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedElements;
			for(const FString& SelectedPath : Selection)
			{
				if(const TSharedPtr<FTLLRN_ModularRigTreeElement> ElementToSelect = FindElement(SelectedPath))
				{
					SelectedElements.Add(ElementToSelect);
				}
			}
			if(!SelectedElements.IsEmpty())
			{
				SetSelection(SelectedElements);
			}
		}
	}
}

TSharedRef<ITableRow> STLLRN_ModularRigTreeView::MakeTableRowWidget(TSharedPtr<FTLLRN_ModularRigTreeElement> InItem,
	const TSharedRef<STableViewBase>& OwnerTable, bool bPinned)
{
	return InItem->MakeTreeRowWidget(OwnerTable, InItem.ToSharedRef(), SharedThis(this), bPinned);
}

void STLLRN_ModularRigTreeView::HandleGetChildrenForTree(TSharedPtr<FTLLRN_ModularRigTreeElement> InItem,
	TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>>& OutChildren)
{
	OutChildren = InItem.Get()->Children;
}

TArray<FString> STLLRN_ModularRigTreeView::GetSelectedKeys() const
{
	TArray<FString> Keys;
	TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedElements = GetSelectedItems();
	for(const TSharedPtr<FTLLRN_ModularRigTreeElement>& SelectedElement : SelectedElements)
	{
		if (SelectedElement.IsValid())
		{
			Keys.AddUnique(SelectedElement->ModulePath);
		}
	}
	return Keys;
}

void STLLRN_ModularRigTreeView::SetSelection(const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>>& InSelection) 
{
	ClearSelection();
	SetItemSelection(InSelection, true, ESelectInfo::Direct);
}

const TSharedPtr<FTLLRN_ModularRigTreeElement>* STLLRN_ModularRigTreeView::FindItemAtPosition(FVector2D InScreenSpacePosition) const
{
	if (ItemsPanel.IsValid() && SListView<TSharedPtr<FTLLRN_ModularRigTreeElement>>::HasValidItemsSource())
	{
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		const int32 Index = FindChildUnderPosition(ArrangedChildren, InScreenSpacePosition);
		if (ArrangedChildren.IsValidIndex(Index))
		{
			TSharedRef<STLLRN_ModularRigModelItem> ItemWidget = StaticCastSharedRef<STLLRN_ModularRigModelItem>(ArrangedChildren[Index].Widget);
			if (ItemWidget->WeakRigTreeElement.IsValid())
			{
				const FString Key = ItemWidget->WeakRigTreeElement.Pin()->Key;
				const TSharedPtr<FTLLRN_ModularRigTreeElement>* ResultPtr = SListView<TSharedPtr<FTLLRN_ModularRigTreeElement>>::GetItems().FindByPredicate([Key](const TSharedPtr<FTLLRN_ModularRigTreeElement>& Item) -> bool
					{
						return Item->Key == Key;
					});

				if (ResultPtr)
				{
					return ResultPtr;
				}
			}
		}
	}
	return nullptr;
}

TPair<const FSlateBrush*, FSlateColor> FTLLRN_ModularRigTreeElement::GetBrushAndColor(const UTLLRN_ModularRig* InTLLRN_ModularRig)
{
	const FSlateBrush* Brush = nullptr;
	FLinearColor Color = FSlateColor(EStyleColor::Foreground).GetColor(FWidgetStyle());
	float Opacity = 1.f;

	if (const FTLLRN_RigModuleInstance* ConnectorModule = InTLLRN_ModularRig->FindModule(ModulePath))
	{
		const FTLLRN_ModularRigModel& Model = InTLLRN_ModularRig->GetTLLRN_ModularRigModel();
		const FString ConnectorPath = FString::Printf(TEXT("%s:%s"), *ModulePath, *ConnectorName);
		bool bIsConnected = Model.Connections.HasConnection(FTLLRN_RigElementKey(*ConnectorPath, ETLLRN_RigElementType::Connector));
		bool bConnectionWarning = !bIsConnected;
		
		if (const UTLLRN_ControlRig* ModuleRig = ConnectorModule->GetRig())
		{
			const FTLLRN_RigModuleConnector* Connector = ModuleRig->GetTLLRN_RigModuleSettings().ExposedConnectors.FindByPredicate([this](FTLLRN_RigModuleConnector& Connector)
			{
				return Connector.Name == ConnectorName;
			});
			if (Connector)
			{
				if (Connector->IsPrimary())
				{
					if (bIsConnected)
					{
						const FSoftObjectPath IconPath = ModuleRig->GetTLLRN_RigModuleSettings().Icon;
						const TSharedPtr<FSlateBrush>* ExistingBrush = IconPathToBrush.Find(IconPath);
						if(ExistingBrush && ExistingBrush->IsValid())
						{
							Brush = ExistingBrush->Get();
						}
						else
						{
							if(UTexture2D* Icon = Cast<UTexture2D>(IconPath.TryLoad()))
							{
								const TSharedPtr<FSlateBrush> NewBrush = MakeShareable(new FSlateBrush(UWidgetBlueprintLibrary::MakeBrushFromTexture(Icon, 16.0f, 16.0f)));
								IconPathToBrush.FindOrAdd(IconPath) = NewBrush;
								Brush = NewBrush.Get();
							}
						}
					}
					else
					{
						Brush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorWarning");
					}
				}
				else if (Connector->Settings.bOptional)
				{
					bConnectionWarning = false;
					if (!bIsConnected)
					{
						Opacity = 0.7;
						Color = FSlateColor(EStyleColor::Hover2).GetColor(FWidgetStyle());
					}
					Brush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorOptional");
				}
				else
				{
					Brush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorSecondary");
				}
			}
		}

		if (bConnectionWarning)
		{
			Color = FSlateColor(EStyleColor::Warning).GetColor(FWidgetStyle());
		}
	}
	if (!Brush)
	{
		Brush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Tree.RigidBody");
	}

	// Apply opacity
	Color = Color.CopyWithNewOpacity(Opacity);
	
	return TPair<const FSlateBrush*, FSlateColor>(Brush, Color);
}

//////////////////////////////////////////////////////////////
/// SSearchableTLLRN_ModularRigTreeView
///////////////////////////////////////////////////////////

void SSearchableTLLRN_ModularRigTreeView::Construct(const FArguments& InArgs)
{
	FTLLRN_ModularRigTreeDelegates TreeDelegates = InArgs._RigTreeDelegates;
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Fill)
		.Padding(0.0f, 0.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SBorder)
				.Padding(2.0f)
				.BorderImage(FAppStyle::GetBrush("SCSEditor.TreePanel"))
				[
					SAssignNew(TreeView, STLLRN_ModularRigTreeView)
					.RigTreeDelegates(TreeDelegates)
				]
			]
		]
	];
}


#undef LOCTEXT_NAMESPACE
