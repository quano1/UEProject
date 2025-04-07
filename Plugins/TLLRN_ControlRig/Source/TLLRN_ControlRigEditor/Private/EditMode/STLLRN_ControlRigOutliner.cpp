// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/STLLRN_ControlRigOutliner.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "ScopedTransaction.h"
#include "TLLRN_ControlRig.h"
#include "UnrealEdGlobals.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditorModeManager.h"
#include "ISequencer.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "MVVM/Selection/Selection.h"
#include "LevelSequence.h"
#include "Selection.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "Widgets/Text/STextBlock.h"
#include "EditMode/TLLRN_ControlTLLRN_RigControlsProxy.h"
#include "EditMode/TLLRN_ControlRigEditModeSettings.h"
#include "TimerManager.h"
#include "Editor/STLLRN_RigHierarchyTreeView.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "Settings/TLLRN_ControlRigSettings.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor/EditorEngine.h"
#include "HelperUtil.h"
#include "ScopedTransaction.h"
#include "Rigs/FKTLLRN_ControlRig.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigOutliner"

FRigTreeDisplaySettings FMultiRigTreeDelegates::DefaultDisplaySettings;


//////////////////////////////////////////////////////////////
/// FMultiRigData
///////////////////////////////////////////////////////////

uint32 GetTypeHash(const FMultiRigData& Data)
{
	return GetTypeHash(TTuple<const UTLLRN_ControlRig*, FTLLRN_RigElementKey>(Data.TLLRN_ControlRig.Get(), (Data.Key.IsSet() ? Data.Key.GetValue() : FTLLRN_RigElementKey())));
}

FText FMultiRigData::GetName() const
{
	if (Key.IsSet())
	{
		return FText::FromName(Key.GetValue().Name);
	}
	else if (TLLRN_ControlRig.IsValid())
	{
		FString TLLRN_ControlTLLRN_RigName = TLLRN_ControlRig.Get()->GetName();
		FString BoundObjectName;
		if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig.Get()->GetObjectBinding())
		{
			if (ObjectBinding->GetBoundObject())
			{
				AActor* Actor = ObjectBinding->GetBoundObject()->GetTypedOuter<AActor>();
				if (Actor)
				{
					BoundObjectName = Actor->GetActorLabel();
				}
			}
		}
		FText AreaTitle = FText::Format(LOCTEXT("ControlTitle", "{0}  ({1})"), FText::AsCultureInvariant(TLLRN_ControlTLLRN_RigName), FText::AsCultureInvariant((BoundObjectName)));
		return AreaTitle;
	}
	FName None = NAME_None;
	return FText::FromName(None);
}

FText FMultiRigData::GetDisplayName() const
{
	if (Key.IsSet())
	{
		if(TLLRN_ControlRig.IsValid())
		{
			if(const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
			{
				const FText DisplayNameForUI = Hierarchy->GetDisplayNameForUI(Key.GetValue(), false);
				if(!DisplayNameForUI.IsEmpty())
				{
					return DisplayNameForUI;
				}
				
				if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Key.GetValue()))
				{
					if(!ControlElement->Settings.DisplayName.IsNone())
					{
						return FText::FromName(ControlElement->Settings.DisplayName);
					}
				}
			}
		}
	}
	return GetName();
}

bool FMultiRigData::operator== (const FMultiRigData & Other) const
{
	if (TLLRN_ControlRig.IsValid() == false && Other.TLLRN_ControlRig.IsValid() == false)
	{
		return true;
	}
	else if (TLLRN_ControlRig.IsValid() == true && Other.TLLRN_ControlRig.IsValid() == true)
	{
		if (TLLRN_ControlRig.Get() == Other.TLLRN_ControlRig.Get())
		{
			if (Key.IsSet() == false && Other.Key.IsSet() == false)
			{
				return true;
			}
			else if (Key.IsSet() == true && Other.Key.IsSet() == true)
			{
				return Key.GetValue() == Other.Key.GetValue();
			}
		}
	}
	return false;
}

bool FMultiRigData::IsValid() const
{
	return (TLLRN_ControlRig.IsValid() && (Key.IsSet() == false || Key.GetValue().IsValid()));
}

UTLLRN_RigHierarchy* FMultiRigData::GetHierarchy() const
{
	if (TLLRN_ControlRig.IsValid())
	{
		return TLLRN_ControlRig.Get()->GetHierarchy();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////
/// FMultiRigTreeElement
///////////////////////////////////////////////////////////

FMultiRigTreeElement::FMultiRigTreeElement(const FMultiRigData& InData, TWeakPtr<SMultiTLLRN_RigHierarchyTreeView> InTreeView, ERigTreeFilterResult InFilterResult)
{
	Data = InData;
	FilterResult = InFilterResult;

	if (InTreeView.IsValid() && Data.TLLRN_ControlRig.IsValid())
	{
		if (const UTLLRN_RigHierarchy* Hierarchy = Data.GetHierarchy())
		{
			const FRigTreeDisplaySettings& Settings = InTreeView.Pin()->GetTreeDelegates().GetDisplaySettings();
			RefreshDisplaySettings(Hierarchy, Settings);
		}
	}
}

TSharedRef<ITableRow> FMultiRigTreeElement::MakeTreeRowWidget(const TSharedRef<STableViewBase>& InOwnerTable, TSharedRef<FMultiRigTreeElement> InRigTreeElement, TSharedPtr<SMultiTLLRN_RigHierarchyTreeView> InTreeView, const FRigTreeDisplaySettings& InSettings, bool bPinned)
{
	return SNew(SMultiTLLRN_RigHierarchyItem, InOwnerTable, InRigTreeElement, InTreeView, InSettings, bPinned);

}

void FMultiRigTreeElement::RefreshDisplaySettings(const UTLLRN_RigHierarchy* InHierarchy, const FRigTreeDisplaySettings& InSettings)
{
	const TPair<const FSlateBrush*, FSlateColor> Result = SMultiTLLRN_RigHierarchyItem::GetBrushForElementType(InHierarchy, Data);

	IconBrush = Result.Key;
	IconColor = Result.Value;
	if (IconColor.IsColorSpecified() && InSettings.bShowIconColors)
	{
		IconColor = FilterResult == ERigTreeFilterResult::Shown ? Result.Value : FSlateColor(Result.Value.GetSpecifiedColor() * 0.5f);
	}
	else
	{
		IconColor = FilterResult == ERigTreeFilterResult::Shown ? FSlateColor::UseForeground() : FSlateColor(FLinearColor::Gray * 0.5f);
	}
	TextColor = FilterResult == ERigTreeFilterResult::Shown ? FSlateColor::UseForeground() : FSlateColor(FLinearColor::Gray * 0.5f);

}
//////////////////////////////////////////////////////////////
/// SMultiTLLRN_RigHierarchyItem
///////////////////////////////////////////////////////////
void SMultiTLLRN_RigHierarchyItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, TSharedRef<FMultiRigTreeElement> InRigTreeElement, TSharedPtr<SMultiTLLRN_RigHierarchyTreeView> InTreeView, const FRigTreeDisplaySettings& InSettings, bool bPinned)
{
	WeakRigTreeElement = InRigTreeElement;
	Delegates = InTreeView->GetTreeDelegates();

	
	if (InRigTreeElement->Data.TLLRN_ControlRig.IsValid() == false || (InRigTreeElement->Data.Key.IsSet() &&
		InRigTreeElement->Data.Key.GetValue().IsValid() == false))
	{
		STableRow<TSharedPtr<FMultiRigTreeElement>>::Construct(
			STableRow<TSharedPtr<FMultiRigTreeElement>>::FArguments()
			.ShowSelection(false)
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

	STableRow<TSharedPtr<FMultiRigTreeElement>>::Construct(
		STableRow<TSharedPtr<FMultiRigTreeElement>>::FArguments()
		.ShowWires(true)
		.Content()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.MaxWidth(24)
			.FillWidth(1.0)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.OnClicked(this, &SMultiTLLRN_RigHierarchyItem::OnGetSelectedClicked)
				[
					SNew(SImage)
					.Image_Lambda([this]() -> const FSlateBrush*
					{
						//if no key is set then it's the control rig so we get that based upon it's state
						if (WeakRigTreeElement.Pin()->Data.Key.IsSet() == false)
						{
							if (UTLLRN_ControlRig* TLLRN_ControlRig = WeakRigTreeElement.Pin()->Data.TLLRN_ControlRig.Get())
							{
								if (TLLRN_ControlRig->GetControlsVisible())
								{
									return (FAppStyle::GetBrush("Level.VisibleIcon16x"));
								}
								else
								{
									return  (FAppStyle::GetBrush("Level.NotVisibleIcon16x"));
								}
							}

						}
						return WeakRigTreeElement.Pin()->IconBrush;
					})
					.ColorAndOpacity_Lambda([this]()
					{
						return WeakRigTreeElement.Pin()->IconColor;
					})
					.DesiredSizeOverride(FVector2D(16, 16))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SAssignNew(InlineWidget, SInlineEditableTextBlock)
				.Text(this, &SMultiTLLRN_RigHierarchyItem::GetDisplayName)
				.MultiLine(false)
				.ColorAndOpacity_Lambda([this]()
				{
					if (WeakRigTreeElement.IsValid())
					{
						return WeakRigTreeElement.Pin()->TextColor;
					}
					return FSlateColor::UseForeground();
				})
			]
		], OwnerTable);

}

FReply SMultiTLLRN_RigHierarchyItem::OnGetSelectedClicked()
{
	if (FMultiRigTreeElement* Element = WeakRigTreeElement.Pin().Get())
	{
		if (Element->Data.Key.IsSet() == false)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = Element->Data.TLLRN_ControlRig.Get())
			{
				FScopedTransaction ScopedTransaction(LOCTEXT("ToggleControlsVisibility", "Toggle Controls Visibility"), !GIsTransacting);
				TLLRN_ControlRig->Modify();
				TLLRN_ControlRig->ToggleControlsVisible();
				return FReply::Handled();
			}
		}
	}
	return FReply::Unhandled();  //should flow to selection instead
}

FText SMultiTLLRN_RigHierarchyItem::GetName() const
{
	return (WeakRigTreeElement.Pin()->Data.GetName());
}

FText SMultiTLLRN_RigHierarchyItem::GetDisplayName() const
{
	return (WeakRigTreeElement.Pin()->Data.GetDisplayName());
}

TPair<const FSlateBrush*, FSlateColor> SMultiTLLRN_RigHierarchyItem::GetBrushForElementType(const UTLLRN_RigHierarchy* InHierarchy, const FMultiRigData& InData)
{
	const FSlateBrush* Brush = nullptr;
	FSlateColor Color = FSlateColor::UseForeground();
	if (InData.Key.IsSet())
	{
		const FTLLRN_RigElementKey Key = InData.Key.GetValue();
		return STLLRN_RigHierarchyItem::GetBrushForElementType(InHierarchy, Key);
	}
	else
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = InData.TLLRN_ControlRig.Get())
		{
			if (TLLRN_ControlRig->GetControlsVisible())
			{
				Brush = FAppStyle::GetBrush("Level.VisibleIcon16x");
			}
			else
			{
				Brush = FAppStyle::GetBrush("Level.NotVisibleIcon16x");
			}
		}
		else
		{
			Brush = FAppStyle::GetBrush("Level.NotVisibleIcon16x");
		}
	}

	return TPair<const FSlateBrush*, FSlateColor>(Brush, Color);
}

//////////////////////////////////////////////////////////////
/// STLLRN_RigHierarchyTreeView
///////////////////////////////////////////////////////////

void SMultiTLLRN_RigHierarchyTreeView::Construct(const FArguments& InArgs)
{
	Delegates = InArgs._RigTreeDelegates;

	STreeView<TSharedPtr<FMultiRigTreeElement>>::FArguments SuperArgs;
	SuperArgs.TreeItemsSource(&RootElements);
	SuperArgs.SelectionMode(ESelectionMode::Multi);
	SuperArgs.OnGenerateRow(this, &SMultiTLLRN_RigHierarchyTreeView::MakeTableRowWidget, false);
	SuperArgs.OnGetChildren(this, &SMultiTLLRN_RigHierarchyTreeView::HandleGetChildrenForTree);
	SuperArgs.OnSelectionChanged(FOnMultiRigTreeSelectionChanged::CreateRaw(&Delegates, &FMultiRigTreeDelegates::HandleSelectionChanged));
	SuperArgs.OnContextMenuOpening(Delegates.OnContextMenuOpening);
	SuperArgs.OnMouseButtonClick(Delegates.OnMouseButtonClick);
	SuperArgs.OnMouseButtonDoubleClick(Delegates.OnMouseButtonDoubleClick);
	SuperArgs.OnSetExpansionRecursive(Delegates.OnSetExpansionRecursive);
	SuperArgs.HighlightParentNodesForSelection(true);
	SuperArgs.AllowInvisibleItemSelection(true);  //without this we deselect everything when we filter or we collapse

	SuperArgs.ShouldStackHierarchyHeaders_Lambda([]() -> bool {
		return UTLLRN_ControlRigEditorSettings::Get()->bShowStackedHierarchy;
		});
	SuperArgs.OnGeneratePinnedRow(this, &SMultiTLLRN_RigHierarchyTreeView::MakeTableRowWidget, true);
	SuperArgs.MaxPinnedItems_Lambda([]() -> int32
		{
			return FMath::Max<int32>(1, UTLLRN_ControlRigEditorSettings::Get()->MaxStackSize);
		});

	STreeView<TSharedPtr<FMultiRigTreeElement>>::Construct(SuperArgs);
}

TSharedPtr<FMultiRigTreeElement> SMultiTLLRN_RigHierarchyTreeView::FindElement(const FMultiRigData& InElementData, TSharedPtr<FMultiRigTreeElement> CurrentItem)
{
	if (CurrentItem->Data == InElementData)
	{
		return CurrentItem;
	}

	for (int32 ChildIndex = 0; ChildIndex < CurrentItem->Children.Num(); ++ChildIndex)
	{
		TSharedPtr<FMultiRigTreeElement> Found = FindElement(InElementData, CurrentItem->Children[ChildIndex]);
		if (Found.IsValid())
		{
			return Found;
		}
	}

	return TSharedPtr<FMultiRigTreeElement>();
}

bool SMultiTLLRN_RigHierarchyTreeView::AddElement(const FMultiRigData& InData, const FMultiRigData& InParentData)
{
	if (ElementMap.Contains(InData))
	{
		return false;
	}

	const FRigTreeDisplaySettings& Settings = Delegates.GetDisplaySettings();

	const FString FilteredString = Settings.FilterText.ToString();
	if (FilteredString.IsEmpty() || !InData.IsValid())
	{
		TSharedPtr<FMultiRigTreeElement> NewItem = MakeShared<FMultiRigTreeElement>(InData, SharedThis(this), ERigTreeFilterResult::Shown);

		if (InData.IsValid())
		{
			ElementMap.Add(InData, NewItem);
			if (InParentData.IsValid())
			{
				ParentMap.Add(InData, InParentData);
				TSharedPtr<FMultiRigTreeElement>* FoundItem = ElementMap.Find(InParentData);
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
		FString FilteredStringUnderScores = FilteredString.Replace(TEXT(" "), TEXT("_"));
		if (InData.GetName().ToString().Contains(FilteredString) || InData.GetName().ToString().Contains(FilteredStringUnderScores))
		{
			TSharedPtr<FMultiRigTreeElement> NewItem = MakeShared<FMultiRigTreeElement>(InData, SharedThis(this), ERigTreeFilterResult::Shown);
			ElementMap.Add(InData, NewItem);
			RootElements.Add(NewItem);

			if (!Settings.bFlattenHierarchyOnFilter && !Settings.bHideParentsOnFilter)
			{
				if (const UTLLRN_RigHierarchy* Hierarchy = InData.GetHierarchy())
				{
					if (InData.Key.IsSet())
					{
						TSharedPtr<FMultiRigTreeElement> ChildItem = NewItem;
						FTLLRN_RigElementKey ParentKey = Hierarchy->GetFirstParent(InData.Key.GetValue());
						while (ParentKey.IsValid())
						{
							FMultiRigData ParentData(InData.TLLRN_ControlRig.Get(), ParentKey);
							if (!ElementMap.Contains(ParentData))
							{
								TSharedPtr<FMultiRigTreeElement> ParentItem = MakeShared<FMultiRigTreeElement>(ParentData, SharedThis(this), ERigTreeFilterResult::ShownDescendant);
								ElementMap.Add(ParentData, ParentItem);
								RootElements.Add(ParentItem);

								ReparentElement(ChildItem->Data, ParentData);

								ChildItem = ParentItem;
								ParentKey = Hierarchy->GetFirstParent(ParentKey);
							}
							else
							{
								ReparentElement(ChildItem->Data, ParentData);
								break;
							}
						}
					}
				}
			}
		}
	}

	return true;
}

bool SMultiTLLRN_RigHierarchyTreeView::AddElement(UTLLRN_ControlRig* InTLLRN_ControlRig, const FTLLRN_RigBaseElement* InElement)
{
	check(InTLLRN_ControlRig);
	check(InElement);
	
	FMultiRigData Data(InTLLRN_ControlRig, InElement->GetKey());

	if (ElementMap.Contains(Data))
	{
		return false;
	}

	const FRigTreeDisplaySettings& Settings = Delegates.GetDisplaySettings();

	auto IsElementShown = [Settings](const FTLLRN_RigBaseElement* InElement) -> bool
	{
		switch (InElement->GetType())
		{
			case ETLLRN_RigElementType::Bone:
			{
				if (!Settings.bShowBones)
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
				if (!Settings.bShowNulls)
				{
					return false;
				}
				break;
			}
			case ETLLRN_RigElementType::Control:
			{
				const FTLLRN_RigControlElement* ControlElement = CastChecked<FTLLRN_RigControlElement>(InElement);
				if (!Settings.bShowControls || ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::VisualCue)
				{
					return false;
				}
				break;
			}
			case ETLLRN_RigElementType::Physics:
			{
				if (!Settings.bShowPhysics)
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
				if (!Settings.bShowReferences)
				{
					return false;
				}
				break;
			}
			case ETLLRN_RigElementType::Socket:
			{
				if (!Settings.bShowSockets)
				{
					return false;
				}
				break;
			}
			case ETLLRN_RigElementType::Connector:
			{
				if (!Settings.bShowConnectors)
				{
					return false;
				}
				break;
			}
			case ETLLRN_RigElementType::Curve:
			{
				return false;
			}
			default:
			{
				break;
			}
		}
		return true;
	};

	if(!IsElementShown(InElement))
	{
		return false;
	}

	FMultiRigData ParentData;
	ParentData.TLLRN_ControlRig = InTLLRN_ControlRig;

	if (!AddElement(Data,ParentData))
	{
		return false;
	}

	UFKTLLRN_ControlRig* FKTLLRN_ControlRig = Cast<UFKTLLRN_ControlRig>(InTLLRN_ControlRig);

	if (ElementMap.Contains(Data))
	{
		if (const UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig->GetHierarchy())
		{
			FTLLRN_RigElementKey ParentKey = Hierarchy->GetFirstParent(InElement->GetKey());

			TArray<FTLLRN_RigElementWeight> ParentWeights = Hierarchy->GetParentWeightArray(InElement->GetKey());
			if (ParentWeights.Num() > 0)
			{
				TArray<FTLLRN_RigElementKey> ParentKeys = Hierarchy->GetParents(InElement->GetKey());
				check(ParentKeys.Num() == ParentWeights.Num());
				for (int32 ParentIndex = 0; ParentIndex < ParentKeys.Num(); ParentIndex++)
				{
					if (ParentWeights[ParentIndex].IsAlmostZero())
					{
						continue;
					}
					ParentKey = ParentKeys[ParentIndex];
					break;
				}
			}

			if (ParentKey.IsValid())
			{
				if(FKTLLRN_ControlRig && ParentKey != UTLLRN_RigHierarchy::GetWorldSpaceReferenceKey())
				{
					if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InElement))
					{
						if(ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::AnimationControl)
						{
							const FTLLRN_RigElementKey& ElementKey = InElement->GetKey();
							const FName BoneName = FKTLLRN_ControlRig->GetControlTargetName(ElementKey.Name, ParentKey.Type);
							const FTLLRN_RigElementKey ParentBoneKey = Hierarchy->GetFirstParent(FTLLRN_RigElementKey(BoneName, ETLLRN_RigElementType::Bone));
							if(ParentBoneKey.IsValid())
							{
								ParentKey = FTLLRN_RigElementKey(FKTLLRN_ControlRig->GetControlName(ParentBoneKey.Name, ParentKey.Type), ElementKey.Type);
							}
						}
					}
				}

				if (const FTLLRN_RigBaseElement* ParentElement = Hierarchy->Find(ParentKey))
				{
					if(ParentElement != nullptr)
					{
						AddElement(InTLLRN_ControlRig,ParentElement);

						FMultiRigData NewParentData(InTLLRN_ControlRig, ParentKey);

						if (ElementMap.Contains(NewParentData))
						{
							ReparentElement(Data, NewParentData);
						}
					}
				}
			}
		}
	}

	return true;
}


bool SMultiTLLRN_RigHierarchyTreeView::ReparentElement(const FMultiRigData& InData, const FMultiRigData& InParentData)
{
	if (!InData.IsValid() || InData == InParentData)
	{
		return false;
	}

	const FRigTreeDisplaySettings& Settings = Delegates.GetDisplaySettings();

	TSharedPtr<FMultiRigTreeElement>* FoundItem = ElementMap.Find(InData);
	if (FoundItem == nullptr)
	{
		return false;
	}

	if (!Settings.FilterText.IsEmpty() && Settings.bFlattenHierarchyOnFilter)
	{
		return false;
	}

	if (const FMultiRigData* ExistingParentKey = ParentMap.Find(InData))
	{
		if (*ExistingParentKey == InParentData)
		{
			return false;
		}

		if (TSharedPtr<FMultiRigTreeElement>* ExistingParent = ElementMap.Find(*ExistingParentKey))
		{
			(*ExistingParent)->Children.Remove(*FoundItem);
		}

		ParentMap.Remove(InData);
	}
	else
	{
		if (!InParentData.IsValid())
		{
			return false;
		}

		RootElements.Remove(*FoundItem);
	}

	if (InParentData.IsValid())
	{
		ParentMap.Add(InData, InParentData);

		TSharedPtr<FMultiRigTreeElement>* FoundParent = ElementMap.Find(InParentData);
		check(FoundParent);
		FoundParent->Get()->Children.Add(*FoundItem);
	}
	else
	{
		RootElements.Add(*FoundItem);
	}

	return true;
}

bool SMultiTLLRN_RigHierarchyTreeView::RemoveElement(const FMultiRigData& InData)
{
	TSharedPtr<FMultiRigTreeElement>* FoundItem = ElementMap.Find(InData);
	if (FoundItem == nullptr)
	{
		return false;
	}

	FMultiRigData EmptyParent(nullptr, FTLLRN_RigElementKey());
	ReparentElement(InData, EmptyParent);

	RootElements.Remove(*FoundItem);
	return ElementMap.Remove(InData) > 0;
}

void SMultiTLLRN_RigHierarchyTreeView::RefreshTreeView(bool bRebuildContent)
{
	TMap<FMultiRigData, bool> ExpansionState;

	if (bRebuildContent)
	{
		for (TPair<FMultiRigData, TSharedPtr<FMultiRigTreeElement>> Pair : ElementMap)
		{
			ExpansionState.FindOrAdd(Pair.Key) = IsItemExpanded(Pair.Value);
		}

		// internally save expansion states before rebuilding the tree, so the states can be restored later
		SaveAndClearSparseItemInfos();

		RootElements.Reset();
		ElementMap.Reset();
		ParentMap.Reset();
	}

	if (bRebuildContent)
	{
		for (const TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRigPtr : TLLRN_ControlRigs)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig  = TLLRN_ControlRigPtr.Get())
			{
				FMultiRigData Empty(nullptr, FTLLRN_RigElementKey());
				FMultiRigData CRData;
				CRData.TLLRN_ControlRig = TLLRN_ControlRig; //leave key unset so it's valid

				AddElement(CRData, Empty);
				if (const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
				{
					Hierarchy->Traverse([&](FTLLRN_RigBaseElement* Element, bool& bContinue)
						{
							AddElement(TLLRN_ControlRig,Element);
							bContinue = true;
						});

					// expand all elements upon the initial construction of the tree
					if (ExpansionState.Num() == 0)
					{
						for (TSharedPtr<FMultiRigTreeElement> RootElement : RootElements)
						{
							SetExpansionRecursive(RootElement, false, true);
						}
					}
					else if (ExpansionState.Num() < ElementMap.Num())
					{
						for (const TPair<FMultiRigData, TSharedPtr<FMultiRigTreeElement>>& Element : ElementMap)
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

				}
			}
		}
	}
	else
	{
		if (RootElements.Num() > 0)
		{
			// elements may be added at the end of the list after a spacer element
			// we need to remove the spacer element and re-add it at the end
			RootElements.RemoveAll([](TSharedPtr<FMultiRigTreeElement> InElement)
				{
					return (InElement.Get()->Data.TLLRN_ControlRig == nullptr && InElement.Get()->Data.Key == FTLLRN_RigElementKey());
				});
		}
	}

	RequestTreeRefresh();
	{
		ClearSelection();

		for (const TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRigPtr : TLLRN_ControlRigs)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigPtr.Get())
			{
				if (const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
				{

					TArray<FTLLRN_RigElementKey> Selection = Hierarchy->GetSelectedKeys();
					for (const FTLLRN_RigElementKey& Key : Selection)
					{
						for (int32 RootIndex = 0; RootIndex < RootElements.Num(); ++RootIndex)
						{
							FMultiRigData Data(TLLRN_ControlRig, Key);
							TSharedPtr<FMultiRigTreeElement> Found = FindElement(Data, RootElements[RootIndex]);
							if (Found.IsValid())
							{
								SetItemSelection(Found, true, ESelectInfo::OnNavigation);
							}
						}
					}
				}
			}
		}
	}
}

void SMultiTLLRN_RigHierarchyTreeView::SetExpansionRecursive(TSharedPtr<FMultiRigTreeElement> InElement, bool bTowardsParent,
	bool bShouldBeExpanded)
{
	SetItemExpansion(InElement, bShouldBeExpanded);

	if (bTowardsParent)
	{
		if (const FMultiRigData* ParentKey = ParentMap.Find(InElement->Data))
		{
			if (TSharedPtr<FMultiRigTreeElement>* ParentItem = ElementMap.Find(*ParentKey))
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

TSharedRef<ITableRow> SMultiTLLRN_RigHierarchyTreeView::MakeTableRowWidget(TSharedPtr<FMultiRigTreeElement> InItem,
	const TSharedRef<STableViewBase>& OwnerTable, bool bPinned)
{
	const FRigTreeDisplaySettings& Settings = Delegates.GetDisplaySettings();
	return InItem->MakeTreeRowWidget(OwnerTable, InItem.ToSharedRef(), SharedThis(this), Settings, bPinned);
}

void SMultiTLLRN_RigHierarchyTreeView::HandleGetChildrenForTree(TSharedPtr<FMultiRigTreeElement> InItem,
	TArray<TSharedPtr<FMultiRigTreeElement>>& OutChildren)
{
	OutChildren = InItem.Get()->Children;
}

TArray<FMultiRigData> SMultiTLLRN_RigHierarchyTreeView::GetSelectedData() const
{
	TArray<FMultiRigData> Keys;
	TArray<TSharedPtr<FMultiRigTreeElement>> SelectedElements = GetSelectedItems();
	for (const TSharedPtr<FMultiRigTreeElement>& SelectedElement : SelectedElements)
	{
		Keys.Add(SelectedElement->Data);
	}
	return Keys;
}

TArray<UTLLRN_RigHierarchy*> SMultiTLLRN_RigHierarchyTreeView::GetHierarchy() const
{
	TArray<UTLLRN_RigHierarchy*> TLLRN_RigHierarchy;
	for (const TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig.IsValid())
		{
			TLLRN_RigHierarchy.Add(TLLRN_ControlRig.Get()->GetHierarchy());
		}
	}
	return TLLRN_RigHierarchy;
}

void SMultiTLLRN_RigHierarchyTreeView::SetTLLRN_ControlRigs(TArrayView < TWeakObjectPtr<UTLLRN_ControlRig>>& InTLLRN_ControlRigs)
{
	TLLRN_ControlRigs.SetNum(0);
	TArray < TPair<UTLLRN_ControlRig*, TArray<FName>>> SelectedControls;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : InTLLRN_ControlRigs)
	{
		if (TLLRN_ControlRig.IsValid())
		{
			TLLRN_ControlRigs.Add(TLLRN_ControlRig.Get());
			SelectedControls.Add(TPair<UTLLRN_ControlRig*, TArray<FName>>(TLLRN_ControlRig.Get(), TLLRN_ControlRig->CurrentControlSelection()));
		}
	}
	
	RefreshTreeView(true);
	//reselect controls that will have gotten cleared by the refresh, this situation can happen on save
	for (TPair<UTLLRN_ControlRig*, TArray<FName>>& CRS : SelectedControls)
	{
		for (const FName& Name : CRS.Value)
		{
			CRS.Key->SelectControl(Name, true);
		}
	}
}

//////////////////////////////////////////////////////////////
/// SSearchableMultiTLLRN_RigHierarchyTreeView
///////////////////////////////////////////////////////////

void SSearchableMultiTLLRN_RigHierarchyTreeView::Construct(const FArguments& InArgs)
{
	FMultiRigTreeDelegates TreeDelegates = InArgs._RigTreeDelegates;
	SuperGetRigTreeDisplaySettings = TreeDelegates.OnGetDisplaySettings;

	TreeDelegates.OnGetDisplaySettings.BindSP(this, &SSearchableMultiTLLRN_RigHierarchyTreeView::GetDisplaySettings);

	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Fill)
				.Padding(0.0f)
				[
					SNew(SSearchBox)
					.InitialText(InArgs._InitialFilterText)
				.OnTextChanged(this, &SSearchableMultiTLLRN_RigHierarchyTreeView::OnFilterTextChanged)
				]

			+ SVerticalBox::Slot()
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
							SAssignNew(TreeView, SMultiTLLRN_RigHierarchyTreeView)
							.RigTreeDelegates(TreeDelegates)
						]
					]
				]
		];
}

const FRigTreeDisplaySettings& SSearchableMultiTLLRN_RigHierarchyTreeView::GetDisplaySettings()
{
	if (SuperGetRigTreeDisplaySettings.IsBound())
	{
		Settings = SuperGetRigTreeDisplaySettings.Execute();
	}
	Settings.FilterText = FilterText;
	return Settings;
}

void SSearchableMultiTLLRN_RigHierarchyTreeView::OnFilterTextChanged(const FText& SearchText)
{
	FilterText = SearchText;
	GetTreeView()->RefreshTreeView();
}

//////////////////////////////////////////////////////////////
/// STLLRN_ControlRigOutliner
///////////////////////////////////////////////////////////

void STLLRN_ControlRigOutliner::Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode& InEditMode)
{
	bIsChangingTLLRN_RigHierarchy = false;

	DisplaySettings.bShowBones = false;
	DisplaySettings.bShowControls = true;
	DisplaySettings.bShowNulls = false;
	DisplaySettings.bShowReferences = false;
	DisplaySettings.bShowSockets = false;
	DisplaySettings.bShowPhysics = false;
	DisplaySettings.bHideParentsOnFilter = true;
	DisplaySettings.bFlattenHierarchyOnFilter = true;
	DisplaySettings.bShowConnectors = false;

	FMultiRigTreeDelegates RigTreeDelegates;
	RigTreeDelegates.OnGetDisplaySettings = FOnGetRigTreeDisplaySettings::CreateSP(this, &STLLRN_ControlRigOutliner::GetDisplaySettings);
	RigTreeDelegates.OnSelectionChanged = FOnMultiRigTreeSelectionChanged::CreateSP(this, &STLLRN_ControlRigOutliner::HandleSelectionChanged);

	
	ChildSlot
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(PickerExpander, SExpandableArea)
					.InitiallyCollapsed(false)
					//.AreaTitle(AreaTitle)
					//.AreaTitleFont(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
					.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f))
					.BodyContent()
					[
						SAssignNew(HierarchyTreeView, SSearchableMultiTLLRN_RigHierarchyTreeView)
						.RigTreeDelegates(RigTreeDelegates)
					]
					
				]
		
			]
		];
	SetEditMode(InEditMode);
}

void STLLRN_ControlRigOutliner::OnObjectsReplaced(const TMap<UObject*, UObject*>& OldToNewInstanceMap)
{
	//if there's a control rig recreate the tree, controls may have changed
	bool bNewTLLRN_ControlRig = false;
	for (const TPair<UObject*, UObject*>& Pair : OldToNewInstanceMap)
	{
		if(Pair.Key && Pair.Value)
		{
			if (Pair.Key->IsA<UTLLRN_ControlRig>() && Pair.Value->IsA<UTLLRN_ControlRig>())
			{
				bNewTLLRN_ControlRig = false;
				break;
			}
		}
	}
	if (bNewTLLRN_ControlRig)
	{
		HierarchyTreeView->GetTreeView()->RefreshTreeView(true);
	}
}

STLLRN_ControlRigOutliner::STLLRN_ControlRigOutliner()
{
	FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &STLLRN_ControlRigOutliner::OnObjectsReplaced);
}

STLLRN_ControlRigOutliner::~STLLRN_ControlRigOutliner()
{
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
	for(TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig: BoundTLLRN_ControlRigs)
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

void STLLRN_ControlRigOutliner::HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, bool bSelected)
{
	FTLLRN_ControlRigBaseDockableView::HandleControlSelected(Subject, ControlElement, bSelected);
	const FTLLRN_RigElementKey Key = ControlElement->GetKey();
	FMultiRigData Data(Subject, Key);
	for (int32 RootIndex = 0; RootIndex < HierarchyTreeView->GetTreeView()->GetRootElements().Num(); ++RootIndex)
	{
		TSharedPtr<FMultiRigTreeElement> Found = HierarchyTreeView->GetTreeView()->FindElement(Data, HierarchyTreeView->GetTreeView()->GetRootElements()[RootIndex]);
		if (Found.IsValid())
		{
			TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);

			HierarchyTreeView->GetTreeView()->SetItemSelection(Found, bSelected, ESelectInfo::OnNavigation);

			TArray<TSharedPtr<FMultiRigTreeElement>> SelectedItems = HierarchyTreeView->GetTreeView()->GetSelectedItems();
			for (TSharedPtr<FMultiRigTreeElement> SelectedItem : SelectedItems)
			{
				HierarchyTreeView->GetTreeView()->SetExpansionRecursive(SelectedItem, false, true);
			}

			if (SelectedItems.Num() > 0)
			{
				HierarchyTreeView->GetTreeView()->RequestScrollIntoView(SelectedItems.Last());
			}
		}
	}
}


void STLLRN_ControlRigOutliner::HandleSelectionChanged(TSharedPtr<FMultiRigTreeElement> Selection, ESelectInfo::Type SelectInfo)
{
	if (bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}
	const TArray<FMultiRigData> NewSelection = HierarchyTreeView->GetTreeView()->GetSelectedData();
	TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> SelectedRigAndKeys;
	for (const FMultiRigData& Data : NewSelection)
	{
		if (Data.TLLRN_ControlRig.IsValid() && (Data.Key.IsSet() && Data.Key.GetValue() != FTLLRN_RigElementKey()))
		{
			if (SelectedRigAndKeys.Find(Data.TLLRN_ControlRig.Get()) == nullptr)
			{
				SelectedRigAndKeys.Add(Data.TLLRN_ControlRig.Get());
			}
			SelectedRigAndKeys[Data.TLLRN_ControlRig.Get()].Add(Data.Key.GetValue());
		}
	}
	TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
	FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
	bool bEndTransaction = false;
	if (GEditor && !GIsTransacting && EditMode && EditMode->IsInLevelEditor())
	{
		GEditor->BeginTransaction(LOCTEXT("SelectControl", "Select Control"));
		bEndTransaction = true;
	}
	//due to how Sequencer Tree View will redo selection on next tick if we aren't keeping or toggling selection we need to clear it out
	if (FSlateApplication::Get().GetModifierKeys().IsShiftDown() == false || FSlateApplication::Get().GetModifierKeys().IsControlDown() == false)
	{
		if (EditMode)
		{		
			TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> SelectedControls;
			EditMode->GetAllSelectedControls(SelectedControls);
			for (TPair<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>>& CurrentSelection : SelectedControls)
			{
				if (CurrentSelection.Key)
				{
					CurrentSelection.Key->ClearControlSelection();
				}
			}
			if (GEditor)
			{
				// Replicating the UEditorEngine::HandleSelectCommand, without the transaction to avoid ensure(!GIsTransacting)
				GEditor->SelectNone(true, true);
				GEditor->RedrawLevelEditingViewports();
			}
			const TWeakPtr<ISequencer>& WeakSequencer = EditMode->GetWeakSequencer();
			//also need to clear explicitly in sequencer
			if (WeakSequencer.IsValid())
			{
				if (ISequencer* SequencerPtr = WeakSequencer.Pin().Get())
				{
					SequencerPtr->GetViewModel()->GetSelection()->Empty();
				}
			}
		}
	}

	for(TPair<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>>& RigAndKeys: SelectedRigAndKeys)
	{ 
		const UTLLRN_RigHierarchy* Hierarchy = RigAndKeys.Key->GetHierarchy();
		if (Hierarchy)
		{
			UTLLRN_RigHierarchyController* Controller = ((UTLLRN_RigHierarchy*)Hierarchy)->GetController(true);
			check(Controller);
			if (!Controller->SetSelection(RigAndKeys.Value))
			{
				if (bEndTransaction)
				{
					GEditor->EndTransaction();
				}
				return;
			}
		}
	}
	if (bEndTransaction)
	{
		GEditor->EndTransaction();
	}
}

void STLLRN_ControlRigOutliner::SetEditMode(FTLLRN_ControlRigEditMode& InEditMode)
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
					TLLRN_ControlRig.Get()->TLLRN_ControlRigBound().AddRaw(this, &STLLRN_ControlRigOutliner::HandleOnTLLRN_ControlRigBound);
					BoundTLLRN_ControlRigs.Add(TLLRN_ControlRig);
				}
				const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig.Get()->GetObjectBinding();
				if (Binding && !Binding->OnTLLRN_ControlRigBind().IsBoundToObject(this))
				{
					Binding->OnTLLRN_ControlRigBind().AddRaw(this, &STLLRN_ControlRigOutliner::HandleOnObjectBoundToTLLRN_ControlRig);
				}
			}
		}
		HierarchyTreeView->GetTreeView()->SetTLLRN_ControlRigs(TLLRN_ControlRigs); //will refresh tree
	}
}

void STLLRN_ControlRigOutliner::HandleControlAdded(UTLLRN_ControlRig* TLLRN_ControlRig, bool bIsAdded)
{
	FTLLRN_ControlRigBaseDockableView::HandleControlAdded(TLLRN_ControlRig, bIsAdded);
	if (TLLRN_ControlRig)
	{
		if (bIsAdded == true )
		{
			if (!TLLRN_ControlRig->TLLRN_ControlRigBound().IsBoundToObject(this))
			{
				TLLRN_ControlRig->TLLRN_ControlRigBound().AddRaw(this, &STLLRN_ControlRigOutliner::HandleOnTLLRN_ControlRigBound);
				BoundTLLRN_ControlRigs.Add(TLLRN_ControlRig);
			}
			const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig->GetObjectBinding();
			if (Binding && !Binding->OnTLLRN_ControlRigBind().IsBoundToObject(this))
			{
				Binding->OnTLLRN_ControlRigBind().AddRaw(this, &STLLRN_ControlRigOutliner::HandleOnObjectBoundToTLLRN_ControlRig);
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
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		TArrayView<TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigs = EditMode->GetTLLRN_ControlRigs();
		HierarchyTreeView->GetTreeView()->SetTLLRN_ControlRigs(TLLRN_ControlRigs); //will refresh tree
	}
}

void STLLRN_ControlRigOutliner::HandleOnTLLRN_ControlRigBound(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if (!InTLLRN_ControlRig)
	{
		return;
	}

	const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = InTLLRN_ControlRig->GetObjectBinding();

	if (Binding && !Binding->OnTLLRN_ControlRigBind().IsBoundToObject(this))
	{
		Binding->OnTLLRN_ControlRigBind().AddRaw(this, &STLLRN_ControlRigOutliner::HandleOnObjectBoundToTLLRN_ControlRig);
	}
}


void STLLRN_ControlRigOutliner::HandleOnObjectBoundToTLLRN_ControlRig(UObject* InObject)
{
	//just refresh the views, but do so on next tick since with FK control rig's the controls aren't set up
	//until AFTER we are bound.
	TWeakPtr<STLLRN_ControlRigOutliner> WeakPtr = StaticCastSharedRef<STLLRN_ControlRigOutliner>(AsShared()).ToWeakPtr();
	GEditor->GetTimerManager()->SetTimerForNextTick([WeakPtr]()
	{
		if (WeakPtr.IsValid())
		{
			TSharedPtr<STLLRN_ControlRigOutliner> StrongThis = WeakPtr.Pin();
			if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(StrongThis->ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
			{
				TArrayView<TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigs = EditMode->GetTLLRN_ControlRigs();
				StrongThis->HierarchyTreeView->GetTreeView()->SetTLLRN_ControlRigs(TLLRN_ControlRigs); //will refresh tree
			}
		}
	});
}

#undef LOCTEXT_NAMESPACE
