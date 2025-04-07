// Copyright Epic Games, Inc. All Rights Reserved.

#include "ControlsProxyDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "PropertyHandle.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "IDetailPropertyRow.h"
#include "EditMode/TLLRN_ControlTLLRN_RigControlsProxy.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Layout/SBox.h"
#include "Algo/Transform.h"
#include "SAdvancedTransformInputBox.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

#include "MVVM/ViewModelPtr.h"
#include "MVVM/Selection/Selection.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "MVVM/ViewModels/ViewModelIterators.h"
#include "MVVM/ViewModels/ChannelModel.h"
#include "Rigs/TLLRN_TLLRN_RigControlHierarchy.h"
#include "MVVM/CurveEditorExtension.h"
#include "Tree/SCurveEditorTree.h"

#include "CurveEditor.h"
#include "CurveModel.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FControlsProxyDetailCustomization"

void FPropertySelectionCache::OnCurveModelDisplayChanged(FCurveModel* InCurveModel, bool bDisplayed, const FCurveEditor* InCurveEditor)
{
	bCacheValid = false;
}

ETLLRN_AnimDetailSelectionState FPropertySelectionCache::IsPropertySelected(TSharedPtr<FCurveEditor>& InCurveEditor, UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName)
{
	if (CurveEditor.IsValid() == false || InCurveEditor.Get() != CurveEditor.Pin().Get())
	{
		ClearDelegates();
		CurveEditor = InCurveEditor;
		if (InCurveEditor->OnCurveArrayChanged.IsBoundToObject(this) == false)
		{
			InCurveEditor->OnCurveArrayChanged.AddRaw(this, &FPropertySelectionCache::OnCurveModelDisplayChanged);
		}
		bCacheValid = false;
	}
	if (bCacheValid == false)
	{
		CachePropertySelection(Proxy);
	}
	ETLLRN_AnimDetailSelectionState SelectionState = ETLLRN_AnimDetailSelectionState::None;

	if ( PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, X)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyFloat, Float)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyBool, Bool)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyInteger, Integer)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_ControlRigEnumControlProxyValue, EnumIndex)

		)
	{
		SelectionState = LocationSelectionCache.XSelected;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, Y))
	{
		SelectionState = LocationSelectionCache.YSelected;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ))
	{
		SelectionState = LocationSelectionCache.ZSelected;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyVector2D, Vector2D))
	{
		if (LocationSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::All && LocationSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::All)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::All;
		}
		else if (LocationSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::None && LocationSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::None)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::None;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Location))
	{
		if (LocationSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::All && LocationSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::All
			&& LocationSelectionCache.ZSelected == ETLLRN_AnimDetailSelectionState::All)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::All;
		}
		else if (LocationSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::None && LocationSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::None
			&& LocationSelectionCache.ZSelected == ETLLRN_AnimDetailSelectionState::None)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::None;
		}
		else
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::Partial;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyLocation, Location))
	{
		if (LocationSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::All && LocationSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::All
			&& LocationSelectionCache.ZSelected == ETLLRN_AnimDetailSelectionState::All)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::All;
		}
		else if (LocationSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::None && LocationSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::None
			&& LocationSelectionCache.ZSelected == ETLLRN_AnimDetailSelectionState::None)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::None;
		}
		else
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::Partial;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX))
	{
		SelectionState = RotationSelectionCache.XSelected;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY))
	{
		SelectionState = RotationSelectionCache.YSelected;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ))
	{
		SelectionState = RotationSelectionCache.ZSelected;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Rotation))
	{
		if (RotationSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::All && RotationSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::All
			&& RotationSelectionCache.ZSelected == ETLLRN_AnimDetailSelectionState::All)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::All;
		}
		else if (RotationSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::None && RotationSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::None
			&& RotationSelectionCache.ZSelected == ETLLRN_AnimDetailSelectionState::None)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::None;
		}
		else
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::Partial;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX))
	{
		SelectionState = ScaleSelectionCache.XSelected;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY))
	{
		SelectionState = ScaleSelectionCache.YSelected;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ))
	{
		SelectionState = ScaleSelectionCache.ZSelected;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Scale))
	{
		if (ScaleSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::All && ScaleSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::All
			&& ScaleSelectionCache.ZSelected == ETLLRN_AnimDetailSelectionState::All)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::All;
		}
		else if (ScaleSelectionCache.XSelected == ETLLRN_AnimDetailSelectionState::None && ScaleSelectionCache.YSelected == ETLLRN_AnimDetailSelectionState::None
			&& ScaleSelectionCache.ZSelected == ETLLRN_AnimDetailSelectionState::None)
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::None;
		}
		else
		{
			SelectionState = ETLLRN_AnimDetailSelectionState::Partial;
		}
	}

	return SelectionState;
}

void FPropertySelectionCache::ClearDelegates()
{
	TSharedPtr<FCurveEditor> SharedCurveEditor = CurveEditor.Pin();

	if (SharedCurveEditor)
	{
		SharedCurveEditor->OnCurveArrayChanged.RemoveAll(this);
	}
}

void FPropertySelectionCache::CachePropertySelection(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy)
{
	if (CurveEditor.IsValid() == false)
	{
		return;
	}
	bCacheValid = true;
	Proxy->GetChannelSelectionState(CurveEditor, LocationSelectionCache, RotationSelectionCache, ScaleSelectionCache);
}

UTLLRN_ControlTLLRN_RigControlsProxy* FTLLRN_AnimDetailValueCustomization::GetProxy(TSharedRef< IPropertyHandle>& PropertyHandle) const
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);
	for (UObject* OuterObject : OuterObjects)
	{
		UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(OuterObject);
		if (Proxy)
		{
			return Proxy;
		}
	}
	return nullptr;
}

UTLLRN_ControlRigDetailPanelControlProxies* FTLLRN_AnimDetailValueCustomization::GetProxyOwner(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy) const
{
	if (Proxy)
	{
		UTLLRN_ControlRigDetailPanelControlProxies* ProxyOwner = Proxy->GetTypedOuter<UTLLRN_ControlRigDetailPanelControlProxies>();
		if (ProxyOwner == nullptr || ProxyOwner->GetSequencer() == nullptr)
		{
			return nullptr;
		}
		return ProxyOwner;
	}
	return nullptr;
}

EVisibility FTLLRN_AnimDetailValueCustomization::IsVisible() const 
{
	if (DetailPropertyRow == nullptr) //need to wait for it to be constructed
	{
		DetailPropertyRow = DetailBuilder->EditPropertyFromRoot(StructPropertyHandlePtr);
	}
	return (DetailPropertyRow && DetailPropertyRow->IsExpanded()) ? EVisibility::Collapsed : EVisibility::Visible;
}

void FTLLRN_AnimDetailValueCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	FOnBooleanValueChanged OnExpansionChanged = FOnBooleanValueChanged::CreateLambda([this](bool bNewValue)
		{
			int a = 3;
		});
	IDetailCategoryBuilder& CatBuilder = StructBuilder.GetParentCategory();
	CatBuilder.OnExpansionChanged(OnExpansionChanged);
	DetailBuilder = &CatBuilder.GetParentLayout();
	StructPropertyHandlePtr = StructPropertyHandle;
	FProperty* Property = StructPropertyHandle->GetProperty(); // Is a FProperty*
	UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = GetProxy(StructPropertyHandle);

	for (int32 ChildIndex = 0; ChildIndex < SortedChildHandles.Num(); ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = SortedChildHandles[ChildIndex];

		const FName PropertyName = ChildHandle->GetProperty()->GetFName();
		const FText PropertyDisplayText = ChildHandle->GetPropertyDisplayName();

		TSharedRef<SWidget> ValueWidget = MakeChildWidget(StructPropertyHandle, ChildHandle);

		// Add the individual properties as children as well so the vector can be expanded for more room
		
		StructBuilder.AddProperty(ChildHandle).CustomWidget()
		.NameContent()
		.HAlign(HAlign_Fill)
		[
			 SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
			.BorderBackgroundColor_Lambda([Proxy, PropertyName, this]()
			{
				ETLLRN_AnimDetailSelectionState SelectionState = IsPropertySelected(Proxy, PropertyName);
				if (SelectionState == ETLLRN_AnimDetailSelectionState::All)
				{
					return  FStyleColors::Select;
				}
				else if (SelectionState == ETLLRN_AnimDetailSelectionState::Partial)
				{
					return  FStyleColors::SelectInactive;
				}
				return FStyleColors::Transparent;
			})
			.OnMouseButtonDown_Lambda([Proxy, PropertyName, this](const FGeometry&, const FPointerEvent& PointerEvent)
			{
				TogglePropertySelection(Proxy, PropertyName);
				if (Proxy)
				{
					UTLLRN_ControlRigDetailPanelControlProxies* ProxyOwner = Proxy->GetTypedOuter<UTLLRN_ControlRigDetailPanelControlProxies>();
				}
				return FReply::Handled();
			})
			.OnMouseButtonUp_Lambda([Proxy](const FGeometry&, const FPointerEvent& PointerEvent)
			{
				if(Proxy)
				{
					UTLLRN_ControlRigDetailPanelControlProxies* ProxyOwner = Proxy->GetTypedOuter<UTLLRN_ControlRigDetailPanelControlProxies>();
				}
				return FReply::Handled();
			})
			.Content()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(0,0,0,0)
				[
					SNew(STextBlock)
					//.Font(InCustomizationUtils.GetRegularFont())
					.Text_Lambda([PropertyDisplayText]()
					{
						return PropertyDisplayText;
					})
					
				]
			]
		]
		.ValueContent()
		.HAlign(HAlign_Fill)
		[
			 SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
			.BorderBackgroundColor_Lambda([Proxy, PropertyName, this]()
			{
				ETLLRN_AnimDetailSelectionState SelectionState = IsPropertySelected(Proxy, PropertyName);
				if (SelectionState == ETLLRN_AnimDetailSelectionState::All)
				{
					return  FStyleColors::Select;
				}
				else if (SelectionState == ETLLRN_AnimDetailSelectionState::Partial)
				{
					return  FStyleColors::SelectInactive;
				}
				return FStyleColors::Transparent;
			})
			.OnMouseButtonDown_Lambda([Proxy, PropertyName, this](const FGeometry&, const FPointerEvent& PointerEvent)
			{
				TogglePropertySelection(Proxy, PropertyName);
				return FReply::Handled();
			})
			.Content()
			[
				ValueWidget
			]
		];
		/*
		.ValueContent()
		[
			ValueWidget
		];
		*/
		
	}
}


TSharedRef<IPropertyTypeCustomization> FTLLRN_AnimDetailValueCustomization::MakeInstance()
{
	return MakeShareable(new FTLLRN_AnimDetailValueCustomization);
}

FTLLRN_AnimDetailValueCustomization::FTLLRN_AnimDetailValueCustomization() = default;

FTLLRN_AnimDetailValueCustomization::~FTLLRN_AnimDetailValueCustomization() = default;

void FTLLRN_AnimDetailValueCustomization::MakeHeaderRow(TSharedRef<class IPropertyHandle>& StructPropertyHandle, FDetailWidgetRow& Row)
{
	if (SortedChildHandles.Num() == 1)
	{
		return;
	}

	FProperty* Property = StructPropertyHandle->GetProperty(); // Is a FProperty*
	TWeakPtr<IPropertyHandle> StructWeakHandlePtr = StructPropertyHandle;
	TSharedPtr<SHorizontalBox> HorizontalBox;
	Row.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
	.PasteAction(FUIAction(
		FExecuteAction::CreateLambda([]()
			{
			}),
		FCanExecuteAction::CreateLambda([]() { return false; }))
	)
	.ValueContent()
		// Make enough space for each child handle
		.MinDesiredWidth(125.f * SortedChildHandles.Num())
		.MaxDesiredWidth(125.f * SortedChildHandles.Num())
		[
				SAssignNew(HorizontalBox, SHorizontalBox)
				.Visibility(this, &FTLLRN_AnimDetailValueCustomization::IsVisible)
				.IsEnabled(this, &FMathStructCustomization::IsValueEnabled, StructWeakHandlePtr)
		];


	for (int32 ChildIndex = 0; ChildIndex < SortedChildHandles.Num(); ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = SortedChildHandles[ChildIndex];
		ChildHandle->GetProperty()->SetPropertyFlags(CPF_TextExportTransient); //hack to turn off shift copy/paste
		// Propagate metadata to child properties so that it's reflected in the nested, individual spin boxes
		ChildHandle->SetInstanceMetaData(TEXT("UIMin"), StructPropertyHandle->GetMetaData(TEXT("UIMin")));
		ChildHandle->SetInstanceMetaData(TEXT("UIMax"), StructPropertyHandle->GetMetaData(TEXT("UIMax")));
		ChildHandle->SetInstanceMetaData(TEXT("SliderExponent"), StructPropertyHandle->GetMetaData(TEXT("SliderExponent")));
		ChildHandle->SetInstanceMetaData(TEXT("Delta"), StructPropertyHandle->GetMetaData(TEXT("Delta")));
		ChildHandle->SetInstanceMetaData(TEXT("LinearDeltaSensitivity"), StructPropertyHandle->GetMetaData(TEXT("LinearDeltaSensitivity")));
		ChildHandle->SetInstanceMetaData(TEXT("ShiftMultiplier"), StructPropertyHandle->GetMetaData(TEXT("ShiftMultiplier")));
		ChildHandle->SetInstanceMetaData(TEXT("CtrlMultiplier"), StructPropertyHandle->GetMetaData(TEXT("CtrlMultiplier")));
		ChildHandle->SetInstanceMetaData(TEXT("SupportDynamicSliderMaxValue"), StructPropertyHandle->GetMetaData(TEXT("SupportDynamicSliderMaxValue")));
		ChildHandle->SetInstanceMetaData(TEXT("SupportDynamicSliderMinValue"), StructPropertyHandle->GetMetaData(TEXT("SupportDynamicSliderMinValue")));
		ChildHandle->SetInstanceMetaData(TEXT("ClampMin"), StructPropertyHandle->GetMetaData(TEXT("ClampMin")));
		ChildHandle->SetInstanceMetaData(TEXT("ClampMax"), StructPropertyHandle->GetMetaData(TEXT("ClampMax")));

		const bool bLastChild = SortedChildHandles.Num() - 1 == ChildIndex;

		TSharedRef<SWidget> ChildWidget = MakeChildWidget(StructPropertyHandle, ChildHandle);
		if (ChildHandle->GetPropertyClass() == FBoolProperty::StaticClass())
		{
			HorizontalBox->AddSlot()
				.Padding(FMargin(0.f, 2.f, bLastChild ? 0.f : 3.f, 2.f))
				.AutoWidth()  // keep the check box slots small
				[
					ChildWidget
				];
		}
		else
		{
			if (ChildHandle->GetPropertyClass() == FDoubleProperty::StaticClass())
			{
				NumericEntryBoxWidgetList.Add(ChildWidget);
			}

			HorizontalBox->AddSlot()
				.Padding(FMargin(0.f, 2.f, bLastChild ? 0.f : 3.f, 2.f))
				[
					ChildWidget
				];
		}
	}
}

FLinearColor FTLLRN_AnimDetailValueCustomization::GetColorFromProperty(const FName& PropertyName) const
{
	FLinearColor Color = FLinearColor::White;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector3, X) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, X) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LX) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RX) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SX))
	{
		Color = SNumericEntryBox<double>::RedLabelBackgroundColor;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector3, Y) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector2D, Y) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LY) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RY) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SY))
	{
		Color = SNumericEntryBox<double>::GreenLabelBackgroundColor;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyVector3, Z) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyLocation, LZ) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyRotation, RZ) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FTLLRN_AnimDetailProxyScale, SZ))
	{
		Color = SNumericEntryBox<double>::BlueLabelBackgroundColor;
	}
	return Color;
}

ETLLRN_AnimDetailSelectionState FTLLRN_AnimDetailValueCustomization::IsPropertySelected(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy,const FName& PropertyName)
{
	using namespace UE::Sequencer;

	UTLLRN_ControlRigDetailPanelControlProxies* ProxyOwner = GetProxyOwner(Proxy);
	if (ProxyOwner == nullptr)
	{
		return ETLLRN_AnimDetailSelectionState::None;
	}
	const TSharedPtr<FSequencerEditorViewModel> SequencerViewModel = ProxyOwner->GetSequencer()->GetViewModel();
	const FCurveEditorExtension* CurveEditorExtension = SequencerViewModel->CastDynamic<FCurveEditorExtension>();
	check(CurveEditorExtension);
	TSharedPtr<FCurveEditor> CurveEditor = CurveEditorExtension->GetCurveEditor();
	return SelectionCache.IsPropertySelected(CurveEditor,Proxy, PropertyName);
	
}

void FTLLRN_AnimDetailValueCustomization::TogglePropertySelection(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName) const
{
	using namespace UE::Sequencer;
	UTLLRN_ControlRigDetailPanelControlProxies* ProxyOwner = GetProxyOwner(Proxy);
	if (ProxyOwner == nullptr)
	{
		return;
	}

	const TSharedPtr<FSequencerEditorViewModel> SequencerViewModel = ProxyOwner->GetSequencer()->GetViewModel();
	const FCurveEditorExtension* CurveEditorExtension = SequencerViewModel->CastDynamic<FCurveEditorExtension>();
	check(CurveEditorExtension);
	TSharedPtr<FCurveEditor> CurveEditor = CurveEditorExtension->GetCurveEditor();
	TSharedPtr<SCurveEditorTree>  CurveEditorTreeView = CurveEditorExtension->GetCurveEditorTreeView();

	const bool bIsShiftDown = FSlateApplication::Get().GetModifierKeys().IsShiftDown();
	const bool bIsCtrlDown = FSlateApplication::Get().GetModifierKeys().IsControlDown();
	if (bIsShiftDown == false && bIsCtrlDown == false)
	{
		CurveEditorTreeView->ClearSelection();
	}
	ETLLRN_AnimDetailPropertySelectionType SelectionType = bIsShiftDown ? ETLLRN_AnimDetailPropertySelectionType::SelectRange : (bIsCtrlDown ? ETLLRN_AnimDetailPropertySelectionType::Toggle : ETLLRN_AnimDetailPropertySelectionType::Select);
	ProxyOwner->SelectProperty(Proxy, PropertyName, SelectionType);
}

bool FTLLRN_AnimDetailValueCustomization::IsMultiple(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy,const FName& InPropertyName) const
{
	if (Proxy)
	{
		if (Proxy->IsMultiple(InPropertyName))
		{
			return true;
		}
	}
	return false;
}

TSharedRef<SWidget> FTLLRN_AnimDetailValueCustomization::MakeChildWidget(
	TSharedRef<IPropertyHandle>& StructurePropertyHandle,
	TSharedRef<IPropertyHandle>& PropertyHandle)
{
	const FFieldClass* PropertyClass = PropertyHandle->GetPropertyClass();
	FName PropertyName = PropertyHandle->GetProperty()->GetFName();
	FLinearColor LinearColor = GetColorFromProperty(PropertyName);
	
	if (PropertyClass == FDoubleProperty::StaticClass())
	{
		return MakeDoubleWidget(StructurePropertyHandle, PropertyHandle, LinearColor);
	}
	else if (PropertyClass == FBoolProperty::StaticClass())
	{
		return MakeBoolWidget(StructurePropertyHandle, PropertyHandle, LinearColor);
	}
	else if (PropertyClass == FInt64Property::StaticClass())
	{
		return MakeIntegerWidget(StructurePropertyHandle, PropertyHandle, LinearColor);

	}

	checkf(false, TEXT("Unsupported property class for the Anim Detail Values customization."));
	return SNullWidget::NullWidget;
}


TSharedRef<SWidget> FTLLRN_AnimDetailValueCustomization::MakeIntegerWidget(
	TSharedRef<IPropertyHandle>& StructurePropertyHandle,
	TSharedRef<IPropertyHandle>& PropertyHandle,
	const FLinearColor& LabelColor)
{

	UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = GetProxy(StructurePropertyHandle);

	TWeakPtr<IPropertyHandle> WeakHandlePtr = PropertyHandle;

	return SNew(SNumericEntryBox<int64>)
		.IsEnabled(this, &FMathStructCustomization::IsValueEnabled, WeakHandlePtr)
		.EditableTextBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
		.Value_Lambda([WeakHandlePtr, this, Proxy]()
			{
				int64 Value = 0;
				bool bIsMultiple = IsMultiple(Proxy, WeakHandlePtr.Pin()->GetProperty()->GetFName());
				return (bIsMultiple == false && WeakHandlePtr.Pin()->GetValue(Value) == FPropertyAccess::Success) ?
					TOptional<int64>(Value) :
					TOptional<int64>();  // Value couldn't be accessed, return an unset value
			})
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.UndeterminedString(NSLOCTEXT("PropertyEditor", "MultipleValues", "Multiple Values"))
		.OnValueCommitted_Lambda([WeakHandlePtr](int64 Value, ETextCommit::Type)
			{
				WeakHandlePtr.Pin()->SetValue(Value, EPropertyValueSetFlags::DefaultFlags);
			})
		.OnValueChanged_Lambda([this, WeakHandlePtr](int64 Value)
			{
				if (bIsUsingSlider)
				{
					WeakHandlePtr.Pin()->SetValue(Value, EPropertyValueSetFlags::InteractiveChange);
				}
			})
		.OnBeginSliderMovement_Lambda([this]()
			{
				bIsUsingSlider = true;
				GEditor->BeginTransaction(LOCTEXT("SetVectorProperty", "Set Property"));
			})
		.OnEndSliderMovement_Lambda([this](int64 Value)
			{
				bIsUsingSlider = false;
				GEditor->EndTransaction();
			})
			// Only allow spin on handles with one object.  Otherwise it is not clear what value to spin
			.AllowSpin(PropertyHandle->GetNumOuterObjects() < 2)
			.LabelPadding(FMargin(3))
			.LabelLocation(SNumericEntryBox<int64>::ELabelLocation::Inside)
			.Label()
			[
				SNumericEntryBox<int64>::BuildNarrowColorLabel(LabelColor)
			];
}

TSharedRef<SWidget> FTLLRN_AnimDetailValueCustomization::MakeBoolWidget(
	TSharedRef<IPropertyHandle>& StructurePropertyHandle,
	TSharedRef<IPropertyHandle>& PropertyHandle,
	const FLinearColor& LabelColor)
{
	UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = GetProxy(StructurePropertyHandle);
	TWeakPtr<IPropertyHandle> WeakHandlePtr = PropertyHandle;
	return
		SNew(SCheckBox)
		.Type(ESlateCheckBoxType::CheckBox)
		.IsChecked_Lambda([WeakHandlePtr, this, Proxy]()->ECheckBoxState
			{
				bool bIsMultiple = IsMultiple(Proxy, WeakHandlePtr.Pin()->GetProperty()->GetFName());
				if (bIsMultiple)
				{
					return ECheckBoxState::Undetermined;
				}

				bool bValue = false;
				WeakHandlePtr.Pin()->GetValue(bValue);
				return bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

			})
		.OnCheckStateChanged_Lambda([WeakHandlePtr](ECheckBoxState CheckBoxState)
			{
				WeakHandlePtr.Pin()->SetValue(CheckBoxState == ECheckBoxState::Checked, EPropertyValueSetFlags::DefaultFlags);
			});
}

TSharedRef<SWidget> FTLLRN_AnimDetailValueCustomization::MakeDoubleWidget(
	TSharedRef<IPropertyHandle>& StructurePropertyHandle,
	TSharedRef<IPropertyHandle>& PropertyHandle,
	const FLinearColor &LabelColor)
{
	TOptional<double> MinValue, MaxValue, SliderMinValue, SliderMaxValue;
	double SliderExponent, Delta;
	float ShiftMultiplier = 10.f;
	float CtrlMultiplier = 0.1f;
	bool SupportDynamicSliderMaxValue = false;
	bool SupportDynamicSliderMinValue = false;

	UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = GetProxy(StructurePropertyHandle);

	ExtractDoubleMetadata(PropertyHandle, MinValue, MaxValue, SliderMinValue, SliderMaxValue, SliderExponent, Delta, ShiftMultiplier, CtrlMultiplier, SupportDynamicSliderMaxValue, SupportDynamicSliderMinValue);

	TWeakPtr<IPropertyHandle> WeakHandlePtr = PropertyHandle;

	return SNew(SNumericEntryBox<double>)
		.IsEnabled(this, &FMathStructCustomization::IsValueEnabled, WeakHandlePtr)
		.EditableTextBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
		.Value_Lambda([WeakHandlePtr,this,Proxy ]()
			{
				bool bIsMultiple = IsMultiple(Proxy, WeakHandlePtr.Pin()->GetProperty()->GetFName());
				
				/* wip attempt to try to get sliders working with multiple values,
				one issue is that we need a bIsUsingSliderOnThisProxy, not a global one for al
				if (bIsMultiple && bIsUsingSlider)
				{
					return TOptional<double>(MultipleValue);
				}*/
				double Value = 0.;
				return (bIsMultiple == false && WeakHandlePtr.Pin()->GetValue(Value) == FPropertyAccess::Success) ?
					TOptional<double>(Value) :
					TOptional<double>();  // Value couldn't be accessed, return an unset value
			})
		.Font(IDetailLayoutBuilder::GetDetailFont())
				.UndeterminedString(NSLOCTEXT("PropertyEditor", "MultipleValues", "Multiple Values"))
				.OnValueCommitted_Lambda([WeakHandlePtr](double Value, ETextCommit::Type)
					{
						WeakHandlePtr.Pin()->SetValue(Value, EPropertyValueSetFlags::DefaultFlags);
					})
				.OnValueChanged_Lambda([this, WeakHandlePtr](double Value)
					{
						if (bIsUsingSlider)
						{
							MultipleValue = Value;
							WeakHandlePtr.Pin()->SetValue(Value, EPropertyValueSetFlags::InteractiveChange);
						}
					})
		.OnBeginSliderMovement_Lambda([WeakHandlePtr, this, Proxy]()
			{
				bIsUsingSlider = true;
				GEditor->BeginTransaction(LOCTEXT("SetVectorProperty", "Set Property"));
				bool bIsMultiple = IsMultiple(Proxy, WeakHandlePtr.Pin()->GetProperty()->GetFName());
				if (bIsMultiple)
				{
					MultipleValue = 0.0;
					WeakHandlePtr.Pin()->SetValue(MultipleValue, EPropertyValueSetFlags::DefaultFlags);
				}

			})
		.OnEndSliderMovement_Lambda([this](double Value)
			{
				bIsUsingSlider = false;
				GEditor->EndTransaction();
			})
				// Only allow spin on handles with one object.  Otherwise it is not clear what value to spin
				.AllowSpin(PropertyHandle->GetNumOuterObjects() < 2)
				.ShiftMultiplier(ShiftMultiplier)
				.CtrlMultiplier(CtrlMultiplier)
				.SupportDynamicSliderMaxValue(SupportDynamicSliderMaxValue)
				.SupportDynamicSliderMinValue(SupportDynamicSliderMinValue)
				.OnDynamicSliderMaxValueChanged(this, &FTLLRN_AnimDetailValueCustomization::OnDynamicSliderMaxValueChanged)
				.OnDynamicSliderMinValueChanged(this, &FTLLRN_AnimDetailValueCustomization::OnDynamicSliderMinValueChanged)
				.MinValue(MinValue)
				.MaxValue(MaxValue)
				.MinSliderValue(SliderMinValue)
				.MaxSliderValue(SliderMaxValue)
				.SliderExponent(SliderExponent)
				.LinearDeltaSensitivity(1.0)
				.Delta(Delta)
				.LabelPadding(FMargin(3))
				.LabelLocation(SNumericEntryBox<double>::ELabelLocation::Inside)
				.Label()
				[
					SNumericEntryBox<double>::BuildNarrowColorLabel(LabelColor)
				];
}

// The following code is just a plain copy of FMathStructCustomization which
// would need changes to be able to serve as a base class for this customization.
void FTLLRN_AnimDetailValueCustomization::ExtractDoubleMetadata(TSharedRef<IPropertyHandle>& PropertyHandle, TOptional<double>& MinValue, TOptional<double>& MaxValue, TOptional<double>& SliderMinValue, TOptional<double>& SliderMaxValue, double& SliderExponent, double& Delta, float& ShiftMultiplier, float& CtrlMultiplier, bool& SupportDynamicSliderMaxValue, bool& SupportDynamicSliderMinValue)
{
	FProperty* Property = PropertyHandle->GetProperty();

	const FString& MetaUIMinString = Property->GetMetaData(TEXT("UIMin"));
	const FString& MetaUIMaxString = Property->GetMetaData(TEXT("UIMax"));
	const FString& SliderExponentString = Property->GetMetaData(TEXT("SliderExponent"));
	const FString& DeltaString = Property->GetMetaData(TEXT("Delta"));
	const FString& ShiftMultiplierString = Property->GetMetaData(TEXT("ShiftMultiplier"));
	const FString& CtrlMultiplierString = Property->GetMetaData(TEXT("CtrlMultiplier"));
	const FString& SupportDynamicSliderMaxValueString = Property->GetMetaData(TEXT("SupportDynamicSliderMaxValue"));
	const FString& SupportDynamicSliderMinValueString = Property->GetMetaData(TEXT("SupportDynamicSliderMinValue"));
	const FString& ClampMinString = Property->GetMetaData(TEXT("ClampMin"));
	const FString& ClampMaxString = Property->GetMetaData(TEXT("ClampMax"));

	// If no UIMin/Max was specified then use the clamp string
	const FString& UIMinString = MetaUIMinString.Len() ? MetaUIMinString : ClampMinString;
	const FString& UIMaxString = MetaUIMaxString.Len() ? MetaUIMaxString : ClampMaxString;

	double ClampMin = TNumericLimits<double>::Lowest();
	double ClampMax = TNumericLimits<double>::Max();

	if (!ClampMinString.IsEmpty())
	{
		TTypeFromString<double>::FromString(ClampMin, *ClampMinString);
	}

	if (!ClampMaxString.IsEmpty())
	{
		TTypeFromString<double>::FromString(ClampMax, *ClampMaxString);
	}

	double UIMin = TNumericLimits<double>::Lowest();
	double UIMax = TNumericLimits<double>::Max();
	TTypeFromString<double>::FromString(UIMin, *UIMinString);
	TTypeFromString<double>::FromString(UIMax, *UIMaxString);

	SliderExponent = double(1);

	if (SliderExponentString.Len())
	{
		TTypeFromString<double>::FromString(SliderExponent, *SliderExponentString);
	}

	Delta = double(0);

	if (DeltaString.Len())
	{
		TTypeFromString<double>::FromString(Delta, *DeltaString);
	}

	ShiftMultiplier = 10.f;
	if (ShiftMultiplierString.Len())
	{
		TTypeFromString<float>::FromString(ShiftMultiplier, *ShiftMultiplierString);
	}

	CtrlMultiplier = 0.1f;
	if (CtrlMultiplierString.Len())
	{
		TTypeFromString<float>::FromString(CtrlMultiplier, *CtrlMultiplierString);
	}

	const double ActualUIMin = FMath::Max(UIMin, ClampMin);
	const double ActualUIMax = FMath::Min(UIMax, ClampMax);

	MinValue = ClampMinString.Len() ? ClampMin : TOptional<double>();
	MaxValue = ClampMaxString.Len() ? ClampMax : TOptional<double>();
	SliderMinValue = (UIMinString.Len()) ? ActualUIMin : TOptional<double>();
	SliderMaxValue = (UIMaxString.Len()) ? ActualUIMax : TOptional<double>();

	SupportDynamicSliderMaxValue = SupportDynamicSliderMaxValueString.Len() > 0 && SupportDynamicSliderMaxValueString.ToBool();
	SupportDynamicSliderMinValue = SupportDynamicSliderMinValueString.Len() > 0 && SupportDynamicSliderMinValueString.ToBool();
}

void FTLLRN_AnimDetailValueCustomization::OnDynamicSliderMaxValueChanged(double NewMaxSliderValue, TWeakPtr<SWidget> InValueChangedSourceWidget, bool IsOriginator, bool UpdateOnlyIfHigher)
{
	for (TWeakPtr<SWidget>& Widget : NumericEntryBoxWidgetList)
	{
		TSharedPtr<SNumericEntryBox<double>> NumericBox = StaticCastSharedPtr<SNumericEntryBox<double>>(Widget.Pin());

		if (NumericBox.IsValid())
		{
			TSharedPtr<SSpinBox<double>> SpinBox = StaticCastSharedPtr<SSpinBox<double>>(NumericBox->GetSpinBox());

			if (SpinBox.IsValid())
			{
				if (SpinBox != InValueChangedSourceWidget)
				{
					if ((NewMaxSliderValue > SpinBox->GetMaxSliderValue() && UpdateOnlyIfHigher) || !UpdateOnlyIfHigher)
					{
						// Make sure the max slider value is not a getter otherwise we will break the link!
						verifySlow(!SpinBox->IsMaxSliderValueBound());
						SpinBox->SetMaxSliderValue(NewMaxSliderValue);
					}
				}
			}
		}
	}

	if (IsOriginator)
	{
		OnNumericEntryBoxDynamicSliderMaxValueChanged.Broadcast((double)NewMaxSliderValue, InValueChangedSourceWidget, false, UpdateOnlyIfHigher);
	}
}

void FTLLRN_AnimDetailValueCustomization::OnDynamicSliderMinValueChanged(double NewMinSliderValue, TWeakPtr<SWidget> InValueChangedSourceWidget, bool IsOriginator, bool UpdateOnlyIfLower)
{
	for (TWeakPtr<SWidget>& Widget : NumericEntryBoxWidgetList)
	{
		TSharedPtr<SNumericEntryBox<double>> NumericBox = StaticCastSharedPtr<SNumericEntryBox<double>>(Widget.Pin());

		if (NumericBox.IsValid())
		{
			TSharedPtr<SSpinBox<double>> SpinBox = StaticCastSharedPtr<SSpinBox<double>>(NumericBox->GetSpinBox());

			if (SpinBox.IsValid())
			{
				if (SpinBox != InValueChangedSourceWidget)
				{
					if ((NewMinSliderValue < SpinBox->GetMinSliderValue() && UpdateOnlyIfLower) || !UpdateOnlyIfLower)
					{
						// Make sure the min slider value is not a getter otherwise we will break the link!
						verifySlow(!SpinBox->IsMinSliderValueBound());
						SpinBox->SetMinSliderValue(NewMinSliderValue);
					}
				}
			}
		}
	}

	if (IsOriginator)
	{
		OnNumericEntryBoxDynamicSliderMinValueChanged.Broadcast((double)NewMinSliderValue, InValueChangedSourceWidget, false, UpdateOnlyIfLower);
	}
}

FTLLRN_AnimDetailProxyDetails::FTLLRN_AnimDetailProxyDetails(const FName& InCategoryName)
{
	CategoryName = InCategoryName;
}

TSharedRef<IDetailCustomization> FTLLRN_AnimDetailProxyDetails::MakeInstance(const FName& InCategoryName)
{
	return MakeShareable(new FTLLRN_AnimDetailProxyDetails(InCategoryName));
}

void FTLLRN_AnimDetailProxyDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> ControlProxies;

	TArray<TWeakObjectPtr<UObject> > EditedObjects;
	DetailBuilder.GetObjectsBeingCustomized(EditedObjects);
	for (int32 i = 0; i < EditedObjects.Num(); i++)
	{
		UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(EditedObjects[i].Get());
		if (Proxy)
		{
			ControlProxies.Add(Proxy);
		}
	}
	FText Name;
	if (ControlProxies.Num() == 1)
	{
		Name = FText::FromName(ControlProxies[0]->GetName());
		ControlProxies[0]->UpdatePropertyNames(DetailBuilder);
	}
	else
	{
		FString DisplayString = TEXT("Multiple");
		Name = FText::FromString(*DisplayString);
		
	}
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(CategoryName, Name, ECategoryPriority::Important);
		
	//add custom attributes
	TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> NestedProxies;
	for (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy : ControlProxies)
	{
		if (Proxy)
		{
			for (UTLLRN_ControlTLLRN_RigControlsProxy* ChildProxy : Proxy->ChildProxies)
			{
				if (ChildProxy)
				{
					NestedProxies.Add(ChildProxy);
				}
			}
		}
	}
	IDetailCategoryBuilder& AttributesCategory = DetailBuilder.EditCategory("Attributes");

	if (NestedProxies.Num() > 0)
	{
		AttributesCategory.SetCategoryVisibility(true);
		for (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy: NestedProxies)
		{
			if (Proxy)
			{
				TArray<UObject*> ExternalObjects;
				ExternalObjects.Add(Proxy);

				FAddPropertyParams Params;
				Params.CreateCategoryNodes(false);
				Params.HideRootObjectNode(true);
				IDetailPropertyRow* NestedRow = AttributesCategory.AddExternalObjects(
					ExternalObjects,
					EPropertyLocation::Default,
					Params);
			}
		}
	}
	else
	{
		AttributesCategory.SetCategoryVisibility(false);
	}
}

#undef LOCTEXT_NAMESPACE
