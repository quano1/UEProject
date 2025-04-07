// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/STLLRN_RigHierarchyTreeView.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor/EditorEngine.h"
#include "HelperUtil.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "Editor/STLLRN_RigHierarchy.h"
#include "Settings/TLLRN_ControlRigSettings.h"
#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "Rigs/AdditiveTLLRN_ControlRig.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "Styling/AppStyle.h"
#include "Algo/Sort.h"
#include "TLLRN_ControlRigBlueprint.h"

#define LOCTEXT_NAMESPACE "STLLRN_RigHierarchyTreeView"

//////////////////////////////////////////////////////////////
/// FRigTreeDelegates
///////////////////////////////////////////////////////////
FRigTreeDisplaySettings FRigTreeDelegates::DefaultDisplaySettings;

//////////////////////////////////////////////////////////////
/// FRigTreeElement
///////////////////////////////////////////////////////////
FRigTreeElement::FRigTreeElement(const FTLLRN_RigElementKey& InKey, TWeakPtr<STLLRN_RigHierarchyTreeView> InTreeView, bool InSupportsRename, ERigTreeFilterResult InFilterResult)
{
	Key = InKey;
	ShortName = InKey.Name;
	ChannelName = NAME_None;
	bIsTransient = false;
	bIsAnimationChannel = false;
	bIsProcedural = false;
	bSupportsRename = InSupportsRename;
	FilterResult = InFilterResult;
	bFadedOutDuringDragDrop = false;

	if(InTreeView.IsValid())
	{
		if(const UTLLRN_RigHierarchy* Hierarchy = InTreeView.Pin()->GetRigTreeDelegates().GetHierarchy())
		{
			ShortName = *Hierarchy->GetDisplayNameForUI(InKey, false).ToString();
			
			const FRigTreeDisplaySettings& Settings = InTreeView.Pin()->GetRigTreeDelegates().GetDisplaySettings();
			RefreshDisplaySettings(Hierarchy, Settings);
		}
	}
}


TSharedRef<ITableRow> FRigTreeElement::MakeTreeRowWidget(const TSharedRef<STableViewBase>& InOwnerTable, TSharedRef<FRigTreeElement> InRigTreeElement, TSharedPtr<STLLRN_RigHierarchyTreeView> InTreeView, const FRigTreeDisplaySettings& InSettings, bool bPinned)
{
	return SNew(STLLRN_RigHierarchyItem, InOwnerTable, InRigTreeElement, InTreeView, InSettings, bPinned);
}

void FRigTreeElement::RequestRename()
{
	if(bSupportsRename)
	{
		OnRenameRequested.ExecuteIfBound();
	}
}

void FRigTreeElement::RefreshDisplaySettings(const UTLLRN_RigHierarchy* InHierarchy, const FRigTreeDisplaySettings& InSettings)
{
	const TPair<const FSlateBrush*, FSlateColor> Result = STLLRN_RigHierarchyItem::GetBrushForElementType(InHierarchy, Key);

	if(const FTLLRN_RigBaseElement* Element = InHierarchy->Find(Key))
	{
		bIsProcedural = Element->IsProcedural();
				
		if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
		{
			bIsTransient = ControlElement->Settings.bIsTransientControl;
			bIsAnimationChannel = ControlElement->IsAnimationChannel();
			if(bIsAnimationChannel)
			{
				ChannelName = ControlElement->GetDisplayName();
			}
		}
	}

	IconBrush = Result.Key;
	IconColor = Result.Value;
	if(IconColor.IsColorSpecified() && InSettings.bShowIconColors)
	{
		IconColor = FilterResult == ERigTreeFilterResult::Shown ? Result.Value : FSlateColor(Result.Value.GetSpecifiedColor() * 0.5f);
	}
	else
	{
		IconColor = FilterResult == ERigTreeFilterResult::Shown ? FSlateColor::UseForeground() : FSlateColor(FLinearColor::Gray * 0.5f);
	}

	TextColor = FilterResult == ERigTreeFilterResult::Shown ?
		(InHierarchy->IsProcedural(Key) ? FSlateColor(FLinearColor(0.9f, 0.8f, 0.4f)) : FSlateColor::UseForeground()) :
		(InHierarchy->IsProcedural(Key) ? FSlateColor(FLinearColor(0.9f, 0.8f, 0.4f) * 0.5f) : FSlateColor(FLinearColor::Gray * 0.5f));
}

FSlateColor FRigTreeElement::GetIconColor() const
{
	if(bFadedOutDuringDragDrop)
	{
		if(FSlateApplication::Get().IsDragDropping())
		{
			return IconColor.GetColor(FWidgetStyle()) * 0.3f;
		}
	}
	return IconColor;
}

FSlateColor FRigTreeElement::GetTextColor() const
{
	if(bFadedOutDuringDragDrop)
	{
		if(FSlateApplication::Get().IsDragDropping())
		{
			return TextColor.GetColor(FWidgetStyle()) * 0.3f;
		}
	}
	return TextColor;
}

//////////////////////////////////////////////////////////////
/// STLLRN_RigHierarchyItem
///////////////////////////////////////////////////////////
void STLLRN_RigHierarchyItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, TSharedRef<FRigTreeElement> InRigTreeElement, TSharedPtr<STLLRN_RigHierarchyTreeView> InTreeView, const FRigTreeDisplaySettings& InSettings, bool bPinned)
{
	WeakRigTreeElement = InRigTreeElement;
	Delegates = InTreeView->GetRigTreeDelegates();
	FRigTreeDisplaySettings DisplaySettings = Delegates.GetDisplaySettings();

	if (!InRigTreeElement->Key.IsValid())
	{
		STableRow<TSharedPtr<FRigTreeElement>>::Construct(
			STableRow<TSharedPtr<FRigTreeElement>>::FArguments()
			.ShowSelection(false)
			.OnCanAcceptDrop(Delegates.OnCanAcceptDrop)
			.OnAcceptDrop(Delegates.OnAcceptDrop)
			.Content()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(200.f)
				[
					SNew(SSpacer)
				]
			], OwnerTable);
		return;
	}

	TSharedPtr< SInlineEditableTextBlock > InlineWidget;
	TSharedPtr< SHorizontalBox > HorizontalBox;

	STableRow<TSharedPtr<FRigTreeElement>>::Construct(
		STableRow<TSharedPtr<FRigTreeElement>>::FArguments()
		.Padding(FMargin(0, 1, 1, 1))
		.OnDragDetected(Delegates.OnDragDetected)
		.OnCanAcceptDrop(Delegates.OnCanAcceptDrop)
		.OnAcceptDrop(Delegates.OnAcceptDrop)
		.ShowWires(true)
		.Content()
		[
			SAssignNew(HorizontalBox, SHorizontalBox)
			+ SHorizontalBox::Slot()
			.MaxWidth(18)
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
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
						return WeakRigTreeElement.Pin()->GetIconColor();
					}
					return FSlateColor::UseForeground();
				})
				.DesiredSizeOverride(FVector2D(16, 16))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SAssignNew(InlineWidget, SInlineEditableTextBlock)
				.Text(this, &STLLRN_RigHierarchyItem::GetNameForUI)
				.ToolTipText(this, &STLLRN_RigHierarchyItem::GetItemTooltip)
				.OnVerifyTextChanged(this, &STLLRN_RigHierarchyItem::OnVerifyNameChanged)
				.OnTextCommitted(this, &STLLRN_RigHierarchyItem::OnNameCommitted)
				.MultiLine(false)
				.ColorAndOpacity_Lambda([this]()
				{
					if(WeakRigTreeElement.IsValid())
					{
						return WeakRigTreeElement.Pin()->GetTextColor();
					}
					return FSlateColor::UseForeground();
				})
			]
		], OwnerTable);

	if(!InRigTreeElement->Tags.IsEmpty())
	{
		HorizontalBox->AddSlot()
		.FillWidth(1.f)
		[
			SNew(SSpacer)
		];
		
		for(const STLLRN_RigHierarchyTagWidget::FArguments& TagArguments : InRigTreeElement->Tags)
		{
			TSharedRef<STLLRN_RigHierarchyTagWidget> TagWidget = SArgumentNew(TagArguments, STLLRN_RigHierarchyTagWidget);
			TagWidget->OnElementKeyDragDetected().BindSP(InTreeView.Get(), &STLLRN_RigHierarchyTreeView::OnElementKeyTagDragDetected);
			
			HorizontalBox->AddSlot()
			.AutoWidth()
			[
				TagWidget
			];
		}
	}

	InRigTreeElement->OnRenameRequested.BindSP(InlineWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);
}

FText STLLRN_RigHierarchyItem::GetNameForUI() const
{
	return GetName(Delegates.GetDisplaySettings().bUseShortName);
}

FText STLLRN_RigHierarchyItem::GetName(bool bUseShortName) const
{
	if(WeakRigTreeElement.Pin()->bIsTransient)
	{
		static const FText TemporaryControl = FText::FromString(TEXT("Temporary Control"));
		return TemporaryControl;
	}
	if(WeakRigTreeElement.Pin()->bIsAnimationChannel)
	{
		return FText::FromName(WeakRigTreeElement.Pin()->ChannelName);
	}
	if(bUseShortName)
	{
		return (FText::FromName(WeakRigTreeElement.Pin()->ShortName));
	}
	return (FText::FromName(WeakRigTreeElement.Pin()->Key.Name));
}

FText STLLRN_RigHierarchyItem::GetItemTooltip() const
{
	if(Delegates.OnRigTreeGetItemToolTip.IsBound())
	{
		const TOptional<FText> ToolTip = Delegates.OnRigTreeGetItemToolTip.Execute(WeakRigTreeElement.Pin()->Key);
		if(ToolTip.IsSet())
		{
			return ToolTip.GetValue();
		}
	}
	const FText FullName = GetName(false);
	const FText ShortName = GetName(true);
	if(FullName.EqualTo(ShortName))
	{
		return FText();
	}
	return FullName;
}

//////////////////////////////////////////////////////////////
/// STLLRN_RigHierarchyTreeView
///////////////////////////////////////////////////////////

void STLLRN_RigHierarchyTreeView::Construct(const FArguments& InArgs)
{
	Delegates = InArgs._RigTreeDelegates;
	bAutoScrollEnabled = InArgs._AutoScrollEnabled;

	STreeView<TSharedPtr<FRigTreeElement>>::FArguments SuperArgs;
	SuperArgs.TreeItemsSource(&RootElements);
	SuperArgs.SelectionMode(ESelectionMode::Multi);
	SuperArgs.OnGenerateRow(this, &STLLRN_RigHierarchyTreeView::MakeTableRowWidget, false);
	SuperArgs.OnGetChildren(this, &STLLRN_RigHierarchyTreeView::HandleGetChildrenForTree);
	SuperArgs.OnSelectionChanged(FOnRigTreeSelectionChanged::CreateRaw(&Delegates, &FRigTreeDelegates::HandleSelectionChanged));
	SuperArgs.OnContextMenuOpening(Delegates.OnContextMenuOpening);
	SuperArgs.OnMouseButtonClick(Delegates.OnMouseButtonClick);
	SuperArgs.OnMouseButtonDoubleClick(Delegates.OnMouseButtonDoubleClick);
	SuperArgs.OnSetExpansionRecursive(Delegates.OnSetExpansionRecursive);
	SuperArgs.HighlightParentNodesForSelection(true);
	SuperArgs.AllowInvisibleItemSelection(true);  //without this we deselect everything when we filter or we collapse
	
	SuperArgs.ShouldStackHierarchyHeaders_Lambda([]() -> bool {
		return UTLLRN_ControlRigEditorSettings::Get()->bShowStackedHierarchy;
	});
	SuperArgs.OnGeneratePinnedRow(this, &STLLRN_RigHierarchyTreeView::MakeTableRowWidget, true);
	SuperArgs.MaxPinnedItems_Lambda([]() -> int32
	{
		return FMath::Max<int32>(1, UTLLRN_ControlRigEditorSettings::Get()->MaxStackSize);
	});

	STreeView<TSharedPtr<FRigTreeElement>>::Construct(SuperArgs);

	LastMousePosition = FVector2D::ZeroVector;
	TimeAtMousePosition = 0.0;

	if(InArgs._PopulateOnConstruct)
	{
		RefreshTreeView(true);
	}
}

void STLLRN_RigHierarchyTreeView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	STreeView<TSharedPtr<FRigTreeElement, ESPMode::ThreadSafe>>::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

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
				const TSharedPtr<FRigTreeElement>* Item = FindItemAtPosition(MousePosition);
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
}

TSharedPtr<FRigTreeElement> STLLRN_RigHierarchyTreeView::FindElement(const FTLLRN_RigElementKey& InElementKey) const
{
	for (int32 RootIndex = 0; RootIndex < RootElements.Num(); ++RootIndex)
	{
		TSharedPtr<FRigTreeElement> Found = FindElement(InElementKey, RootElements[RootIndex]);
		if (Found.IsValid())
		{
			return Found;
		}
	}
	return TSharedPtr<FRigTreeElement>();
}

TSharedPtr<FRigTreeElement> STLLRN_RigHierarchyTreeView::FindElement(const FTLLRN_RigElementKey& InElementKey, TSharedPtr<FRigTreeElement> CurrentItem)
{
	if (CurrentItem->Key == InElementKey)
	{
		return CurrentItem;
	}

	for (int32 ChildIndex = 0; ChildIndex < CurrentItem->Children.Num(); ++ChildIndex)
	{
		TSharedPtr<FRigTreeElement> Found = FindElement(InElementKey, CurrentItem->Children[ChildIndex]);
		if (Found.IsValid())
		{
			return Found;
		}
	}

	return TSharedPtr<FRigTreeElement>();
}

bool STLLRN_RigHierarchyTreeView::AddElement(FTLLRN_RigElementKey InKey, FTLLRN_RigElementKey InParentKey)
{
	if(ElementMap.Contains(InKey))
	{
		return false;
	}

	// skip transient controls
	if(const UTLLRN_RigHierarchy* Hierarchy = Delegates.GetHierarchy())
	{
		if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(InKey))
		{
			if(ControlElement->Settings.bIsTransientControl)
			{
				return false;
			}
		}
	}

	const FRigTreeDisplaySettings& Settings = Delegates.GetDisplaySettings();
	const bool bSupportsRename = Delegates.OnRenameElement.IsBound();

	const FString FilteredString = Settings.FilterText.ToString();
	bool bAnyFilteredOut = Delegates.OnRigTreeIsItemVisible.IsBound();
	if (!bAnyFilteredOut)
	{
		bAnyFilteredOut = !FilteredString.IsEmpty() && InKey.IsValid();
	}

	if (!bAnyFilteredOut)
	{
		TSharedPtr<FRigTreeElement> NewItem = MakeShared<FRigTreeElement>(InKey, SharedThis(this), bSupportsRename, ERigTreeFilterResult::Shown);

		if (InKey.IsValid())
		{
			ElementMap.Add(InKey, NewItem);
			if (InParentKey)
			{
				ParentMap.Add(InKey, InParentKey);
			}

			if (InParentKey)
			{
				TSharedPtr<FRigTreeElement>* FoundItem = ElementMap.Find(InParentKey);
				check(FoundItem);
				FoundItem->Get()->Children.Add(NewItem);
			}
			else
			{
				RootElements.Add(NewItem);
			}
		}
		else
		{
			RootElements.Add(NewItem);
		}
	}
	else
	{
		bool bIsFilteredOut = false;
		if (Delegates.OnRigTreeIsItemVisible.IsBound())
		{
			bIsFilteredOut = !Delegates.OnRigTreeIsItemVisible.Execute(InKey);
		}
		
		FString FilteredStringUnderScores = FilteredString.Replace(TEXT(" "), TEXT("_"));
		if (!bIsFilteredOut && (InKey.Name.ToString().Contains(FilteredString) || InKey.Name.ToString().Contains(FilteredStringUnderScores)))
		{
			TSharedPtr<FRigTreeElement> NewItem = MakeShared<FRigTreeElement>(InKey, SharedThis(this), bSupportsRename, ERigTreeFilterResult::Shown);
			ElementMap.Add(InKey, NewItem);
			RootElements.Add(NewItem);

			if (!Settings.bFlattenHierarchyOnFilter && !Settings.bHideParentsOnFilter)
			{
				if(const UTLLRN_RigHierarchy* Hierarchy = Delegates.GetHierarchy())
				{
					TSharedPtr<FRigTreeElement> ChildItem = NewItem;
					FTLLRN_RigElementKey ParentKey = Hierarchy->GetFirstParent(InKey);
					while (ParentKey.IsValid())
					{
						if (!ElementMap.Contains(ParentKey))
						{
							TSharedPtr<FRigTreeElement> ParentItem = MakeShared<FRigTreeElement>(ParentKey, SharedThis(this), bSupportsRename, ERigTreeFilterResult::ShownDescendant);							
							ElementMap.Add(ParentKey, ParentItem);
							RootElements.Add(ParentItem);

							ReparentElement(ChildItem->Key, ParentKey);

							ChildItem = ParentItem;
							ParentKey = Hierarchy->GetFirstParent(ParentKey);
						}
						else
						{
							ReparentElement(ChildItem->Key, ParentKey);
							break;
						}						
					}
				}
			}
		}
	}

	return true;
}

bool STLLRN_RigHierarchyTreeView::AddElement(const FTLLRN_RigBaseElement* InElement)
{
	check(InElement);
	
	if (ElementMap.Contains(InElement->GetKey()))
	{
		return false;
	}

	const FRigTreeDisplaySettings& Settings = Delegates.GetDisplaySettings();
	const UTLLRN_RigHierarchy* Hierarchy = Delegates.GetHierarchy();

	switch(InElement->GetType())
	{
		case ETLLRN_RigElementType::Bone:
		{
			if(!Settings.bShowBones)
			{
				return false;
			}

			const FTLLRN_RigBoneElement* BoneElement = CastChecked<FTLLRN_RigBoneElement>(InElement);
			if (!Settings.bShowImportedBones && BoneElement->BoneType == ETLLRN_RigBoneType::Imported)
			{
				return false;
			}
			break;
		}
		case ETLLRN_RigElementType::Null:
		{
			if(!Settings.bShowNulls)
			{
				return false;
			}
			break;
		}
		case ETLLRN_RigElementType::Control:
		{
			if(!Settings.bShowControls)
			{
				return false;
			}
			break;
		}
		case ETLLRN_RigElementType::Physics:
		{
			if(!Settings.bShowPhysics)
			{
				return false;
			}
			if(CVarTLLRN_ControlTLLRN_RigHierarchyEnablePhysics.GetValueOnAnyThread() == false)
			{
				return false;
			}
			break;
		}
		case ETLLRN_RigElementType::Reference:
		{
			if(!Settings.bShowReferences)
			{
				return false;
			}
			break;
		}
		case ETLLRN_RigElementType::Curve:
		{
			return false;
		}
		case ETLLRN_RigElementType::Connector:
		{
			if(Hierarchy)
			{
				// add the connector as a tag rather than its own element in the tree
				if(UTLLRN_ControlRig* TLLRN_ControlRig = Hierarchy->GetTypedOuter<UTLLRN_ControlRig>())
				{
					FTLLRN_RigElementKeyRedirector& Redirector = TLLRN_ControlRig->GetElementKeyRedirector();
					if(const FTLLRN_CachedTLLRN_RigElement* Cache = Redirector.Find(InElement->GetKey()))
					{
						if(const_cast<FTLLRN_CachedTLLRN_RigElement*>(Cache)->UpdateCache(Hierarchy))
						{
							if(const TSharedPtr<FRigTreeElement>* TargetElementPtr = ElementMap.Find(Cache->GetKey()))
							{
								const FTLLRN_RigElementKey ConnectorKey = InElement->GetKey();

								STLLRN_RigHierarchyTagWidget::FArguments TagArguments;

								static const FLinearColor BackgroundColor = FColor::FromHex(TEXT("#26BBFF"));
								static const FLinearColor TextColor = FColor::FromHex(TEXT("#0F0F0F"));
								static const FLinearColor IconColor = FColor::FromHex(TEXT("#1A1A1A"));

								static const FSlateBrush* PrimaryBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorPrimary");
								static const FSlateBrush* SecondaryBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorSecondary");
								static const FSlateBrush* OptionalBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorOptional");

								const FSlateBrush* IconBrush = PrimaryBrush;
								if(const FTLLRN_RigConnectorElement* ConnectorElement = Cast<FTLLRN_RigConnectorElement>(InElement))
								{
									if(ConnectorElement->Settings.Type == ETLLRN_ConnectorType::Secondary)
									{
										IconBrush = ConnectorElement->Settings.bOptional ? OptionalBrush : SecondaryBrush;
									}
								}

								FName Name = ConnectorKey.Name;
								if (GetRigTreeDelegates().GetDisplaySettings().bUseShortName)
								{
									Name = *Hierarchy->GetDisplayNameForUI(ConnectorKey).ToString();
								}
								TagArguments.Text(FText::FromName(Name));
								TagArguments.TooltipText(FText::FromName(ConnectorKey.Name));
								TagArguments.Color(BackgroundColor);
								TagArguments.IconColor(IconColor);
								TagArguments.TextColor(TextColor);
								TagArguments.Icon(IconBrush);
								TagArguments.IconSize(FVector2d(16.f, 16.f));
								TagArguments.AllowDragDrop(true);
								FString Identifier;
								FTLLRN_RigElementKey::StaticStruct()->ExportText(Identifier, &ConnectorKey, nullptr, nullptr, PPF_None, nullptr);
								TagArguments.Identifier(Identifier);

								TagArguments.OnClicked_Lambda([ConnectorKey, this]()
								{
									Delegates.RequestDetailsInspection(ConnectorKey);
								});

								if (!TLLRN_ControlRig->IsTLLRN_ModularRig())
								{
									TagArguments.OnRenamed_Lambda([ConnectorKey, this](const FText& InNewName, ETextCommit::Type InCommitType)
									{
										Delegates.HandleRenameElement(ConnectorKey, InNewName.ToString());
									});
									TagArguments.OnVerifyRename_Lambda([ConnectorKey, this](const FText& InText, FText& OutError)
									{
										return Delegates.HandleVerifyElementNameChanged(ConnectorKey, InText.ToString(), OutError);
									});
								}

								TargetElementPtr->Get()->Tags.Add(TagArguments);
								return true;
							}
						}
					}
				}
			}
			break;
		}
		case ETLLRN_RigElementType::Socket:
		{
			if(!Settings.bShowSockets)
			{
				return false;
			}
			break;
		}
		default:
		{
			break;
		}
	}

	if(!AddElement(InElement->GetKey(), FTLLRN_RigElementKey()))
	{
		return false;
	}

	if (ElementMap.Contains(InElement->GetKey()))
	{
		if(Hierarchy)
		{
			if(InElement->GetType() == ETLLRN_RigElementType::Connector)
			{
				AddConnectorResolveWarningTag(ElementMap.FindChecked(InElement->GetKey()), InElement, Hierarchy);
			}
			
			FTLLRN_RigElementKey ParentKey = Hierarchy->GetFirstParent(InElement->GetKey());
			if(InElement->GetType() == ETLLRN_RigElementType::Connector)
			{
				ParentKey = Delegates.GetResolvedKey(InElement->GetKey());
				if(ParentKey == InElement->GetKey())
				{
					ParentKey.Reset();
				}
			}

			TArray<FTLLRN_RigElementWeight> ParentWeights = Hierarchy->GetParentWeightArray(InElement->GetKey());
			if(ParentWeights.Num() > 0)
			{
				TArray<FTLLRN_RigElementKey> ParentKeys = Hierarchy->GetParents(InElement->GetKey());
				check(ParentKeys.Num() == ParentWeights.Num());
				for(int32 ParentIndex=0;ParentIndex<ParentKeys.Num();ParentIndex++)
				{
					if(ParentWeights[ParentIndex].IsAlmostZero())
					{
						continue;
					}
					ParentKey = ParentKeys[ParentIndex];
					break;
				}
			}

			if (ParentKey.IsValid())
			{
				if(const FTLLRN_RigBaseElement* ParentElement = Hierarchy->Find(ParentKey))
				{
					AddElement(ParentElement);

					if(ElementMap.Contains(ParentKey))
					{
						ReparentElement(InElement->GetKey(), ParentKey);
					}
				}
			}
		}
	}

	return true;
}

void STLLRN_RigHierarchyTreeView::AddSpacerElement()
{
	AddElement(FTLLRN_RigElementKey(), FTLLRN_RigElementKey());
}

bool STLLRN_RigHierarchyTreeView::ReparentElement(FTLLRN_RigElementKey InKey, FTLLRN_RigElementKey InParentKey)
{
	if (!InKey.IsValid() || InKey == InParentKey)
	{
		return false;
	}

	if(InKey.Type == ETLLRN_RigElementType::Connector)
	{
		return false;
	}

	const FRigTreeDisplaySettings& Settings = Delegates.GetDisplaySettings();

	TSharedPtr<FRigTreeElement>* FoundItem = ElementMap.Find(InKey);
	if (FoundItem == nullptr)
	{
		return false;
	}

	if (!Settings.FilterText.IsEmpty() && Settings.bFlattenHierarchyOnFilter)
	{
		return false;
	}

	if (const FTLLRN_RigElementKey* ExistingParentKey = ParentMap.Find(InKey))
	{
		if (*ExistingParentKey == InParentKey)
		{
			return false;
		}

		if (TSharedPtr<FRigTreeElement>* ExistingParent = ElementMap.Find(*ExistingParentKey))
		{
			(*ExistingParent)->Children.Remove(*FoundItem);
		}

		ParentMap.Remove(InKey);
	}
	else
	{
		if (!InParentKey.IsValid())
		{
			return false;
		}

		RootElements.Remove(*FoundItem);
	}

	if (InParentKey)
	{
		ParentMap.Add(InKey, InParentKey);

		TSharedPtr<FRigTreeElement>* FoundParent = ElementMap.Find(InParentKey);
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

bool STLLRN_RigHierarchyTreeView::RemoveElement(FTLLRN_RigElementKey InKey)
{
	TSharedPtr<FRigTreeElement>* FoundItem = ElementMap.Find(InKey);
	if (FoundItem == nullptr)
	{
		return false;
	}

	ReparentElement(InKey, FTLLRN_RigElementKey());

	RootElements.Remove(*FoundItem);
	return ElementMap.Remove(InKey) > 0;
}

void STLLRN_RigHierarchyTreeView::RefreshTreeView(bool bRebuildContent)
{
		TMap<FTLLRN_RigElementKey, bool> ExpansionState;

	if(bRebuildContent)
	{
		for (TPair<FTLLRN_RigElementKey, TSharedPtr<FRigTreeElement>> Pair : ElementMap)
		{
			ExpansionState.FindOrAdd(Pair.Key) = IsItemExpanded(Pair.Value);
		}

		// internally save expansion states before rebuilding the tree, so the states can be restored later
		SaveAndClearSparseItemInfos();

		RootElements.Reset();
		ElementMap.Reset();
		ParentMap.Reset();
	}

	if(bRebuildContent)
	{
		const UTLLRN_RigHierarchy* Hierarchy = Delegates.GetHierarchy();
		if(Hierarchy)
		{
			TArray<const FTLLRN_RigSocketElement*> Sockets;
			TArray<const FTLLRN_RigConnectorElement*> Connectors;
			TArray<const FTLLRN_RigBaseElement*> EverythingElse;
			TMap<const FTLLRN_RigBaseElement*, int32> ElementDepth;
			Sockets.Reserve(Hierarchy->Num(ETLLRN_RigElementType::Socket));
			Connectors.Reserve(Hierarchy->Num(ETLLRN_RigElementType::Connector));
			EverythingElse.Reserve(Hierarchy->Num() - Hierarchy->Num(ETLLRN_RigElementType::Socket) - Hierarchy->Num(ETLLRN_RigElementType::Connector));
			
			Hierarchy->Traverse([&](FTLLRN_RigBaseElement* Element, bool& bContinue)
			{
				int32& Depth = ElementDepth.Add(Element, 0);
				if(const FTLLRN_RigBaseElement* ParentElement = Hierarchy->GetFirstParent(Element))
				{
					if (int32* ParentDepth = ElementDepth.Find(ParentElement))
					{
						Depth = *ParentDepth + 1;
					}
				}
				
				if(const FTLLRN_RigSocketElement* Socket = Cast<FTLLRN_RigSocketElement>(Element))
				{
					Sockets.Add(Socket);
				}
				else if(const FTLLRN_RigConnectorElement* Connector = Cast<FTLLRN_RigConnectorElement>(Element))
				{
					Connectors.Add(Connector);
				}
				else
				{
					EverythingElse.Add(Element);
				}
				bContinue = true;
			});

			// sort the sockets by depth
			Algo::SortBy(Sockets, [ElementDepth](const FTLLRN_RigSocketElement* Socket) -> int32
			{
				return ElementDepth.FindChecked(Socket);
			});
			for(const FTLLRN_RigSocketElement* Socket : Sockets)
			{
				AddElement(Socket);
			}

			// add everything but connectors and sockets
			for(const FTLLRN_RigBaseElement* Element : EverythingElse)
			{
				AddElement(Element);
			}

			// add all of the connectors. their parent relationship in the tree represents resolve
			for(const FTLLRN_RigConnectorElement* Connector : Connectors)
			{
				AddElement(Connector);
			}

			// expand all elements upon the initial construction of the tree
			if (ExpansionState.Num() == 0)
			{
				for (TSharedPtr<FRigTreeElement> RootElement : RootElements)
				{
					SetExpansionRecursive(RootElement, false, true);
				}
			}
			else if (ExpansionState.Num() < ElementMap.Num())
			{
				for (const TPair<FTLLRN_RigElementKey, TSharedPtr<FRigTreeElement>>& Element : ElementMap)
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

			if(Delegates.OnCompareKeys.IsBound())
			{
				Algo::Sort(RootElements, [&](const TSharedPtr<FRigTreeElement>& A, const TSharedPtr<FRigTreeElement>& B)
				{
					return Delegates.OnCompareKeys.Execute(A->Key, B->Key);
				});
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
			RootElements.RemoveAll([](TSharedPtr<FRigTreeElement> InElement)
			{
				return InElement.Get()->Key == FTLLRN_RigElementKey();
			});
			AddSpacerElement();
		}
	}

	RequestTreeRefresh();
	{
		ClearSelection();

		if(const UTLLRN_RigHierarchy* Hierarchy = Delegates.GetHierarchy())
		{
			TArray<FTLLRN_RigElementKey> Selection = Delegates.GetSelection();
			for (const FTLLRN_RigElementKey& Key : Selection)
			{
				for (int32 RootIndex = 0; RootIndex < RootElements.Num(); ++RootIndex)
				{
					TSharedPtr<FRigTreeElement> Found = FindElement(Key, RootElements[RootIndex]);
					if (Found.IsValid())
					{
						SetItemSelection(Found, true, ESelectInfo::OnNavigation);
					}
				}
			}
		}
	}
}

void STLLRN_RigHierarchyTreeView::SetExpansionRecursive(TSharedPtr<FRigTreeElement> InElement, bool bTowardsParent,
	bool bShouldBeExpanded)
{
	SetItemExpansion(InElement, bShouldBeExpanded);

	if (bTowardsParent)
	{
		if (const FTLLRN_RigElementKey* ParentKey = ParentMap.Find(InElement->Key))
		{
			if (TSharedPtr<FRigTreeElement>* ParentItem = ElementMap.Find(*ParentKey))
			{
				SetExpansionRecursive(*ParentItem, bTowardsParent, bShouldBeExpanded);
			}
		}
	}
	else
	{
		for (int32 ChildIndex = 0; ChildIndex < InElement->Children.Num(); ++ChildIndex)
		{
			SetExpansionRecursive(InElement->Children[ChildIndex], bTowardsParent, bShouldBeExpanded);
		}
	}
}

TSharedRef<ITableRow> STLLRN_RigHierarchyTreeView::MakeTableRowWidget(TSharedPtr<FRigTreeElement> InItem,
	const TSharedRef<STableViewBase>& OwnerTable, bool bPinned)
{
	const FRigTreeDisplaySettings& Settings = Delegates.GetDisplaySettings();
	return InItem->MakeTreeRowWidget(OwnerTable, InItem.ToSharedRef(), SharedThis(this), Settings, bPinned);
}

void STLLRN_RigHierarchyTreeView::HandleGetChildrenForTree(TSharedPtr<FRigTreeElement> InItem,
	TArray<TSharedPtr<FRigTreeElement>>& OutChildren)
{
	OutChildren = InItem.Get()->Children;
}

void STLLRN_RigHierarchyTreeView::OnElementKeyTagDragDetected(const FTLLRN_RigElementKey& InDraggedTag)
{
	(void)Delegates.OnRigTreeElementKeyTagDragDetected.ExecuteIfBound(InDraggedTag);
}

TArray<FTLLRN_RigElementKey> STLLRN_RigHierarchyTreeView::GetSelectedKeys() const
{
	TArray<FTLLRN_RigElementKey> Keys;
	TArray<TSharedPtr<FRigTreeElement>> SelectedElements = GetSelectedItems();
	for(const TSharedPtr<FRigTreeElement>& SelectedElement : SelectedElements)
	{
		Keys.Add(SelectedElement->Key);
	}
	return Keys;
}

const TSharedPtr<FRigTreeElement>* STLLRN_RigHierarchyTreeView::FindItemAtPosition(FVector2D InScreenSpacePosition) const
{
	if (ItemsPanel.IsValid() && SListView<TSharedPtr<FRigTreeElement>>::HasValidItemsSource())
	{
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		const int32 Index = FindChildUnderPosition(ArrangedChildren, InScreenSpacePosition);
		if (ArrangedChildren.IsValidIndex(Index))
		{
			TSharedRef<STLLRN_RigHierarchyItem> ItemWidget = StaticCastSharedRef<STLLRN_RigHierarchyItem>(ArrangedChildren[Index].Widget);
			if (ItemWidget->WeakRigTreeElement.IsValid())
			{
				const FTLLRN_RigElementKey Key = ItemWidget->WeakRigTreeElement.Pin()->Key;
				const TSharedPtr<FRigTreeElement>* ResultPtr = SListView<TSharedPtr<FRigTreeElement>>::GetItems().FindByPredicate([Key](const TSharedPtr<FRigTreeElement>& Item) -> bool
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

void STLLRN_RigHierarchyTreeView::AddConnectorResolveWarningTag(TSharedPtr<FRigTreeElement> InTreeElement,
	const FTLLRN_RigBaseElement* InTLLRN_RigElement, const UTLLRN_RigHierarchy* InHierarchy)
{
	check(InTreeElement.IsValid());
	check(InTLLRN_RigElement);
	check(InTLLRN_RigElement->GetType() == ETLLRN_RigElementType::Connector);

	if(const FTLLRN_RigConnectorElement* ConnectorElement = Cast<FTLLRN_RigConnectorElement>(InTLLRN_RigElement))
	{
		if(ConnectorElement->IsOptional())
		{
			return;
		}
	}

	if(UTLLRN_ControlRig* TLLRN_ControlRig = InHierarchy->GetTypedOuter<UTLLRN_ControlRig>())
	{
		TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRigPtr(TLLRN_ControlRig);
		const FTLLRN_RigElementKey ConnectorKey = InTLLRN_RigElement->GetKey();
		
		TAttribute<FText> GetTooltipText = TAttribute<FText>::CreateSP(this,
			&STLLRN_RigHierarchyTreeView::GetConnectorWarningMessage, InTreeElement, TLLRN_ControlRigPtr, ConnectorKey);

		static const FLinearColor BackgroundColor = FColor::FromHex(TEXT("#FFB800"));
		static const FLinearColor TextColor = FColor::FromHex(TEXT("#0F0F0F"));
		static const FLinearColor IconColor = FColor::FromHex(TEXT("#1A1A1A"));
		static const FSlateBrush* WarningBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorWarning");

		STLLRN_RigHierarchyTagWidget::FArguments TagArguments;
		TagArguments.Visibility_Lambda([GetTooltipText]() -> EVisibility
		{
			return GetTooltipText.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
		});
		TagArguments.Text(LOCTEXT("ConnectorWarningTagLabel", "Warning"));
		TagArguments.ToolTipText(GetTooltipText);
		TagArguments.Color(BackgroundColor);
		TagArguments.IconColor(IconColor);
		TagArguments.TextColor(TextColor);
		TagArguments.Icon(WarningBrush);
		TagArguments.IconSize(FVector2d(16.f, 16.f));
		InTreeElement->Tags.Add(TagArguments);
	}
}

FText STLLRN_RigHierarchyTreeView::GetConnectorWarningMessage(TSharedPtr<FRigTreeElement> InTreeElement,
	TWeakObjectPtr<UTLLRN_ControlRig> InTLLRN_ControlRigPtr, const FTLLRN_RigElementKey InConnectorKey) const
{
	if(UTLLRN_ControlRig* TLLRN_ControlRig = InTLLRN_ControlRigPtr.Get())
	{
		if(const UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(TLLRN_ControlRig->GetClass()->ClassGeneratedBy))
		{
			const FTLLRN_RigElementKey TargetKey = TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections.FindTargetFromConnector(InConnectorKey);
			if(TargetKey.IsValid())
			{
				const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
				if(Hierarchy->Contains(TargetKey))
				{
					return FText();
				}
			}
		}
	}

	static const FText NotResolvedWarning = LOCTEXT("ConnectorWarningConnectorNotResolved", "Connector is not resolved.");
	return NotResolvedWarning;
}

bool STLLRN_RigHierarchyItem::OnVerifyNameChanged(const FText& InText, FText& OutErrorMessage)
{
	const FString NewName = InText.ToString();
	const FTLLRN_RigElementKey OldKey = WeakRigTreeElement.Pin()->Key;
	return Delegates.HandleVerifyElementNameChanged(OldKey, NewName, OutErrorMessage);
}

TPair<const FSlateBrush*, FSlateColor> STLLRN_RigHierarchyItem::GetBrushForElementType(const UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InKey)
{
	static const FSlateBrush* ProxyControlBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Tree.ProxyControl"); 
	static const FSlateBrush* ControlBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Tree.Control");
	static const FSlateBrush* NullBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Tree.Null");
	static const FSlateBrush* BoneImportedBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Tree.BoneImported");
	static const FSlateBrush* BoneUserBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Tree.BoneUser");
	static const FSlateBrush* PhysicsBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Tree.RigidBody");
	static const FSlateBrush* SocketOpenBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Tree.Socket_Open");
	static const FSlateBrush* SocketClosedBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Tree.Socket_Closed");
	static const FSlateBrush* PrimaryConnectorBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorPrimary");
	static const FSlateBrush* SecondaryConnectorBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorSecondary");
	static const FSlateBrush* OptionalConnectorBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorOptional");

	const FSlateBrush* Brush = nullptr;
	FSlateColor Color = FSlateColor::UseForeground();
	switch (InKey.Type)
	{
		case ETLLRN_RigElementType::Control:
		{
			if(const FTLLRN_RigControlElement* Control = InHierarchy->Find<FTLLRN_RigControlElement>(InKey))
			{
				FLinearColor ShapeColor = FLinearColor::White;
				
				if(Control->Settings.SupportsShape())
				{
					if(Control->Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl)
					{
						Brush = ProxyControlBrush;
					}
					else
					{
						Brush = ControlBrush;
					}
					ShapeColor = Control->Settings.ShapeColor;
				}
				else
				{
					static const FLazyName TypeIcon(TEXT("Kismet.VariableList.TypeIcon"));
					Brush = FAppStyle::GetBrush(TypeIcon);
					ShapeColor = GetColorForControlType(Control->Settings.ControlType, Control->Settings.ControlEnum);
				}
				
				// ensure the alpha is always visible
				ShapeColor.A = 1.f;
				Color = FSlateColor(ShapeColor);
			}
			else
			{
				Brush = ControlBrush;
			}
			break;
		}
		case ETLLRN_RigElementType::Null:
		{
			Brush = NullBrush;
			break;
		}
		case ETLLRN_RigElementType::Bone:
		{
			ETLLRN_RigBoneType BoneType = ETLLRN_RigBoneType::User;

			if(InHierarchy)
			{
				const FTLLRN_RigBoneElement* BoneElement = InHierarchy->Find<FTLLRN_RigBoneElement>(InKey);
				if(BoneElement)
				{
					BoneType = BoneElement->BoneType;
				}
			}

			switch (BoneType)
			{
				case ETLLRN_RigBoneType::Imported:
				{
					Brush = BoneImportedBrush;
					break;
				}
				case ETLLRN_RigBoneType::User:
				default:
				{
					Brush = BoneUserBrush;
					break;
				}
			}

			break;
		}
		case ETLLRN_RigElementType::Physics:
		{
			Brush = PhysicsBrush;
			break;
		}
		case ETLLRN_RigElementType::Reference:
		case ETLLRN_RigElementType::Socket:
		{
			Brush = SocketOpenBrush;

			if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InHierarchy->GetOuter()))
			{
				if(const FTLLRN_RigElementKey* ConnectorKey = TLLRN_ControlRig->GetElementKeyRedirector().FindReverse(InKey))
				{
					if(ConnectorKey->Type == ETLLRN_RigElementType::Connector)
					{
						Brush = SocketClosedBrush;
					}
				}
			}

			if(const FTLLRN_RigSocketElement* Socket = InHierarchy->Find<FTLLRN_RigSocketElement>(InKey))
			{
				Color = Socket->GetColor(InHierarchy);
			}
			break;
		}
		case ETLLRN_RigElementType::Connector:
		{
			Brush = PrimaryConnectorBrush;
			if(const FTLLRN_RigConnectorElement* Connector = InHierarchy->Find<FTLLRN_RigConnectorElement>(InKey))
			{
				if(!Connector->IsPrimary())
				{
					Brush = Connector->IsOptional() ? OptionalConnectorBrush : SecondaryConnectorBrush;
				}
			}
			break;
		}
		default:
		{
			break;
		}
	}

	return TPair<const FSlateBrush*, FSlateColor>(Brush, Color);
}

FLinearColor STLLRN_RigHierarchyItem::GetColorForControlType(ETLLRN_RigControlType InControlType, UEnum* InControlEnum)
{
	FEdGraphPinType PinType;
	switch(InControlType)
	{
		case ETLLRN_RigControlType::Bool:
		{
			PinType = RigVMTypeUtils::PinTypeFromCPPType(RigVMTypeUtils::BoolTypeName, nullptr);
			break;
		}
		case ETLLRN_RigControlType::Float:
		case ETLLRN_RigControlType::ScaleFloat:
		{
			PinType = RigVMTypeUtils::PinTypeFromCPPType(RigVMTypeUtils::FloatTypeName, nullptr);
			break;
		}
		case ETLLRN_RigControlType::Integer:
		{
			if(InControlEnum)
			{
				PinType = RigVMTypeUtils::PinTypeFromCPPType(NAME_None, InControlEnum);
			}
			else
			{
				PinType = RigVMTypeUtils::PinTypeFromCPPType(RigVMTypeUtils::Int32TypeName, nullptr);
			}
			break;
		}
		case ETLLRN_RigControlType::Vector2D:
		{
			UScriptStruct* Struct = TBaseStructure<FVector2D>::Get(); 
			PinType = RigVMTypeUtils::PinTypeFromCPPType(*RigVMTypeUtils::GetUniqueStructTypeName(Struct), Struct);
			break;
		}
		case ETLLRN_RigControlType::Position:
		case ETLLRN_RigControlType::Scale:
		{
			UScriptStruct* Struct = TBaseStructure<FVector>::Get(); 
			PinType = RigVMTypeUtils::PinTypeFromCPPType(*RigVMTypeUtils::GetUniqueStructTypeName(Struct), Struct);
			break;
		}
		case ETLLRN_RigControlType::Rotator:
		{
			UScriptStruct* Struct = TBaseStructure<FRotator>::Get(); 
			PinType = RigVMTypeUtils::PinTypeFromCPPType(*RigVMTypeUtils::GetUniqueStructTypeName(Struct), Struct);
			break;
		}
		case ETLLRN_RigControlType::Transform:
		case ETLLRN_RigControlType::TransformNoScale:
		case ETLLRN_RigControlType::EulerTransform:
		default:
		{
			UScriptStruct* Struct = TBaseStructure<FTransform>::Get(); 
			PinType = RigVMTypeUtils::PinTypeFromCPPType(*RigVMTypeUtils::GetUniqueStructTypeName(Struct), Struct);
			break;
		}
	}
	const UTLLRN_ControlRigGraphSchema* Schema = GetDefault<UTLLRN_ControlRigGraphSchema>();
	return Schema->GetPinTypeColor(PinType);
}

void STLLRN_RigHierarchyItem::OnNameCommitted(const FText& InText, ETextCommit::Type InCommitType) const
{
	// for now only allow enter
	// because it is important to keep the unique names per pose
	if (InCommitType == ETextCommit::OnEnter)
	{
		FString NewName = InText.ToString();
		const FTLLRN_RigElementKey OldKey = WeakRigTreeElement.Pin()->Key;

		const FName NewSanitizedName = Delegates.HandleRenameElement(OldKey, NewName);
		if (NewSanitizedName.IsNone())
		{
			return;
		}
		NewName = NewSanitizedName.ToString();

		if (WeakRigTreeElement.IsValid())
		{
			WeakRigTreeElement.Pin()->Key.Name = *NewName;
		}
	}
}

//////////////////////////////////////////////////////////////
/// SSearchableTLLRN_RigHierarchyTreeView
///////////////////////////////////////////////////////////

void SSearchableTLLRN_RigHierarchyTreeView::Construct(const FArguments& InArgs)
{
	FRigTreeDelegates TreeDelegates = InArgs._RigTreeDelegates;
	SuperGetRigTreeDisplaySettings = TreeDelegates.OnGetDisplaySettings;

	MaxHeight = InArgs._MaxHeight;

	TreeDelegates.OnGetDisplaySettings.BindSP(this, &SSearchableTLLRN_RigHierarchyTreeView::GetDisplaySettings);

	TSharedPtr<SVerticalBox> VerticalBox;
	ChildSlot
	[
		SAssignNew(VerticalBox, SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Fill)
		.Padding(0.0f)
		[
			SAssignNew(SearchBox, SSearchBox)
			.InitialText(InArgs._InitialFilterText)
			.OnTextChanged(this, &SSearchableTLLRN_RigHierarchyTreeView::OnFilterTextChanged)
		]

		+SVerticalBox::Slot()
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
					SAssignNew(TreeView, STLLRN_RigHierarchyTreeView)
					.RigTreeDelegates(TreeDelegates)
				]
			]
		]
	];

	if (MaxHeight > SMALL_NUMBER)
	{
		VerticalBox->GetSlot(1).SetMaxHeight(MaxHeight);
	}
	else
	{
		VerticalBox->GetSlot(1).SetAutoHeight();
	}
}

const FRigTreeDisplaySettings& SSearchableTLLRN_RigHierarchyTreeView::GetDisplaySettings()
{
	if(SuperGetRigTreeDisplaySettings.IsBound())
	{
		Settings = SuperGetRigTreeDisplaySettings.Execute();
	}
	Settings.FilterText = FilterText;
	return Settings;
}

void SSearchableTLLRN_RigHierarchyTreeView::OnFilterTextChanged(const FText& SearchText)
{
	FilterText = SearchText;
	GetTreeView()->RefreshTreeView();
}


#undef LOCTEXT_NAMESPACE
