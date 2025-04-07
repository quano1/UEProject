// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlTLLRN_RigElementDetails.h"
#include "Widgets/SWidget.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ModularRig.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "SEnumCombo.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_DynamicHierarchy.h"
#include "Widgets/SRigVMGraphPinVariableBinding.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Styling/AppStyle.h"
#include "StructViewerFilter.h"
#include "StructViewerModule.h"
#include "Widgets/SRigVMGraphPinEnumPicker.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlTLLRN_RigElementDetails"

static const FText TLLRN_ControlRigDetailsMultipleValues = LOCTEXT("MultipleValues", "Multiple Values");

struct FTLLRN_RigElementTransformWidgetSettings
{
	FTLLRN_RigElementTransformWidgetSettings()
	: RotationRepresentation(MakeShareable(new ESlateRotationRepresentation::Type(ESlateRotationRepresentation::Rotator)))
	, IsComponentRelative(MakeShareable(new UE::Math::TVector<float>(1.f, 1.f, 1.f)))
	, IsScaleLocked(TSharedPtr<bool>(new bool(false)))
	{
	}

	TSharedPtr<ESlateRotationRepresentation::Type> RotationRepresentation;
	TSharedRef<UE::Math::TVector<float>> IsComponentRelative;
	TSharedPtr<bool> IsScaleLocked;

	static FTLLRN_RigElementTransformWidgetSettings& FindOrAdd(
		ETLLRN_RigControlValueType InValueType,
		ETLLRN_RigTransformElementDetailsTransform::Type InTransformType,
		const SAdvancedTransformInputBox<FEulerTransform>::FArguments& WidgetArgs)
	{
		uint32 Hash = GetTypeHash(WidgetArgs._ConstructLocation);
		Hash = HashCombine(Hash, GetTypeHash(WidgetArgs._ConstructRotation));
		Hash = HashCombine(Hash, GetTypeHash(WidgetArgs._ConstructScale));
		Hash = HashCombine(Hash, GetTypeHash(WidgetArgs._AllowEditRotationRepresentation));
		Hash = HashCombine(Hash, GetTypeHash(WidgetArgs._DisplayScaleLock));
		Hash = HashCombine(Hash, GetTypeHash(InValueType));
		Hash = HashCombine(Hash, GetTypeHash(InTransformType));
		return sSettings.FindOrAdd(Hash);
	}

	static TMap<uint32, FTLLRN_RigElementTransformWidgetSettings> sSettings;
};

TMap<uint32, FTLLRN_RigElementTransformWidgetSettings> FTLLRN_RigElementTransformWidgetSettings::sSettings;


void TLLRN_RigElementKeyDetails_GetCustomizedInfo(TSharedRef<IPropertyHandle> InStructPropertyHandle, UTLLRN_ControlRigBlueprint*& OutBlueprint)
{
	TArray<UObject*> Objects;
	InStructPropertyHandle->GetOuterObjects(Objects);
	for (UObject* Object : Objects)
	{
		if (Object->IsA<UTLLRN_ControlRigBlueprint>())
		{
			OutBlueprint = CastChecked<UTLLRN_ControlRigBlueprint>(Object);
			break;
		}

		OutBlueprint = Object->GetTypedOuter<UTLLRN_ControlRigBlueprint>();
		if(OutBlueprint)
		{
			break;
		}

		if(const UTLLRN_ControlRig* TLLRN_ControlRig = Object->GetTypedOuter<UTLLRN_ControlRig>())
		{
			OutBlueprint = Cast<UTLLRN_ControlRigBlueprint>(TLLRN_ControlRig->GetClass()->ClassGeneratedBy);
			if(OutBlueprint)
			{
				break;
			}
		}
	}

	if (OutBlueprint == nullptr)
	{
		TArray<UPackage*> Packages;
		InStructPropertyHandle->GetOuterPackages(Packages);
		for (UPackage* Package : Packages)
		{
			if (Package == nullptr)
			{
				continue;
			}

			TArray<UObject*> SubObjects;
			Package->GetDefaultSubobjects(SubObjects);
			for (UObject* SubObject : SubObjects)
			{
				if (UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(SubObject))
				{
					UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(Rig->GetClass()->ClassGeneratedBy);
					if (Blueprint)
					{
						if(Blueprint->GetOutermost() == Package)
						{
							OutBlueprint = Blueprint;
							break;
						}
					}
				}
			}

			if (OutBlueprint)
			{
				break;
			}
		}
	}
}

UTLLRN_ControlRigBlueprint* TLLRN_RigElementDetails_GetBlueprintFromHierarchy(UTLLRN_RigHierarchy* InHierarchy)
{
	if(InHierarchy == nullptr)
	{
		return nullptr;
	}

	UTLLRN_ControlRigBlueprint* Blueprint = InHierarchy->GetTypedOuter<UTLLRN_ControlRigBlueprint>();
	if(Blueprint == nullptr)
	{
		UTLLRN_ControlRig* Rig = InHierarchy->GetTypedOuter<UTLLRN_ControlRig>();
		if(Rig)
		{
			Blueprint = Cast<UTLLRN_ControlRigBlueprint>(Rig->GetClass()->ClassGeneratedBy);
        }
	}
	return Blueprint;
}

void STLLRN_RigElementKeyWidget::Construct(const FArguments& InArgs, TSharedPtr<IPropertyHandle> InNameHandle, TSharedPtr<IPropertyHandle> InTypeHandle)
{
	NameHandle = InNameHandle;
	TypeHandle = InTypeHandle;
	Construct(InArgs);
}

void STLLRN_RigElementKeyWidget::Construct(const FArguments& InArgs)
{
	BlueprintBeingCustomized = InArgs._Blueprint;
	OnGetElementType = InArgs._OnGetElementType;
	OnElementNameChanged = InArgs._OnElementNameChanged;
	OnElementTypeChanged = InArgs._OnElementTypeChanged;
	
	UpdateElementNameList();
	
	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			TypeHandle.IsValid() ?
				TypeHandle->CreatePropertyValueWidget()
					:
				SNew(SEnumComboBox, StaticEnum<ETLLRN_RigElementType>())
				.CurrentValue_Lambda([this]()
				{
					if (OnGetElementType.IsBound())
					{
						return (int32)OnGetElementType.Execute();
					}
					return (int32)ETLLRN_RigElementType::None;
				})
				.OnEnumSelectionChanged_Lambda([this](int32 InEnumValue, ESelectInfo::Type SelectInfo)
				{
					ETLLRN_RigElementType EnumValue = static_cast<ETLLRN_RigElementType>(InEnumValue);
					OnElementTypeChanged.ExecuteIfBound(EnumValue);
					UpdateElementNameList();
					SearchableComboBox->ClearSelection();
					OnElementNameChanged.ExecuteIfBound(nullptr, ESelectInfo::Direct);
				})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.f, 0.f, 0.f, 0.f)
		[
			SAssignNew(SearchableComboBox, SSearchableComboBox)
			.OptionsSource(&ElementNameList)
			.OnSelectionChanged(InArgs._OnElementNameChanged)
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
			{
				return SNew(STextBlock)
				.Text(FText::FromString(InItem.IsValid() ? *InItem : FString()))
				.Font(IDetailLayoutBuilder::GetDetailFont());
			})
			.Content()
			[
				SNew(STextBlock)
				.Text_Lambda([InArgs]()
				{
					if (InArgs._OnGetElementNameAsText.IsBound())
					{
						return InArgs._OnGetElementNameAsText.Execute();
					}
					return FText();
				})
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		]
		// Use button
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(1,0)
		.VAlign(VAlign_Center)
		[
			SAssignNew(UseSelectedButton, SButton)
			.ButtonStyle( FAppStyle::Get(), "NoBorder" )
			.ButtonColorAndOpacity_Lambda([this, InArgs]() { return UseSelectedButton.IsValid() && UseSelectedButton->IsHovered() ? InArgs._ActiveBackgroundColor : InArgs._InactiveBackgroundColor; })
			.OnClicked(InArgs._OnGetSelectedClicked)
			.ContentPadding(1.f)
			.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ObjectGraphPin_Use_Tooltip", "Use item selected"))
			[
				SNew(SImage)
				.ColorAndOpacity_Lambda( [this, InArgs]() { return UseSelectedButton.IsValid() && UseSelectedButton->IsHovered() ? InArgs._ActiveForegroundColor : InArgs._InactiveForegroundColor; })
				.Image(FAppStyle::GetBrush("Icons.CircleArrowLeft"))
			]
		]
		// Select in hierarchy button
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(1,0)
		.VAlign(VAlign_Center)
		[
			SAssignNew(SelectElementButton, SButton)
			.ButtonStyle( FAppStyle::Get(), "NoBorder" )
			.ButtonColorAndOpacity_Lambda([this, InArgs]() { return SelectElementButton.IsValid() && SelectElementButton->IsHovered() ? InArgs._ActiveBackgroundColor : InArgs._InactiveBackgroundColor; })
			.OnClicked(InArgs._OnSelectInHierarchyClicked)
			.ContentPadding(0)
			.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ObjectGraphPin_Browse_Tooltip", "Select in hierarchy"))
			[
				SNew(SImage)
				.ColorAndOpacity_Lambda( [this, InArgs]() { return SelectElementButton.IsValid() && SelectElementButton->IsHovered() ? InArgs._ActiveForegroundColor : InArgs._InactiveForegroundColor; })
				.Image(FAppStyle::GetBrush("Icons.Search"))
			]
		]
	];

	if (TypeHandle)
	{
		TypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda(
			[this, InArgs]()
			{
				int32 EnumValue;
				TypeHandle->GetValue(EnumValue);
				OnElementTypeChanged.ExecuteIfBound(static_cast<ETLLRN_RigElementType>(EnumValue));
				UpdateElementNameList();
				SearchableComboBox->ClearSelection();
				OnElementNameChanged.ExecuteIfBound(nullptr, ESelectInfo::Direct);
			}
		));
	}
}

void STLLRN_RigElementKeyWidget::UpdateElementNameList()
{
	ElementNameList.Reset();

	if (BlueprintBeingCustomized)
	{
		for (UEdGraph* Graph : BlueprintBeingCustomized->UbergraphPages)
		{
			if (UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(Graph))
			{
				
				const TArray<TSharedPtr<FRigVMStringWithTag>>* NameList = nullptr;
				if (OnGetElementType.IsBound())
				{
					NameList = RigGraph->GetElementNameList(OnGetElementType.Execute());
				}

				ElementNameList.Reset();
				if (NameList)
				{
					ElementNameList.Reserve(NameList->Num());
					for(const TSharedPtr<FRigVMStringWithTag>& Name : *NameList)
					{
						ElementNameList.Add(MakeShared<FString>(Name->GetString()));
					}
				}
				
				if(SearchableComboBox.IsValid())
				{
					SearchableComboBox->RefreshOptions();
				}
				return;
			}
		}
	}
}

void FTLLRN_RigElementKeyDetails::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	BlueprintBeingCustomized = nullptr;
	TLLRN_RigElementKeyDetails_GetCustomizedInfo(InStructPropertyHandle, BlueprintBeingCustomized);

	UTLLRN_ControlRigGraph* RigGraph = nullptr;
	if(BlueprintBeingCustomized)
	{
		for (UEdGraph* Graph : BlueprintBeingCustomized->UbergraphPages)
		{
			RigGraph = Cast<UTLLRN_ControlRigGraph>(Graph);
			if (RigGraph)
			{
				break;
			}
		}
	}

	// only allow blueprints with at least one rig graph
	if (RigGraph == nullptr)
	{
		BlueprintBeingCustomized = nullptr;
	}

	if (BlueprintBeingCustomized == nullptr)
	{
		HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			InStructPropertyHandle->CreatePropertyValueWidget(false)
		];
	}
	else
	{
		TypeHandle = InStructPropertyHandle->GetChildHandle(TEXT("Type"));
		NameHandle = InStructPropertyHandle->GetChildHandle(TEXT("Name"));

		HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SAssignNew(TLLRN_RigElementKeyWidget, STLLRN_RigElementKeyWidget, NameHandle, TypeHandle)
			.Blueprint(BlueprintBeingCustomized)
			.IsEnabled_Lambda([this](){ return !NameHandle->IsEditConst(); })
			.ActiveBackgroundColor(FSlateColor(FLinearColor(1.f, 1.f, 1.f, FTLLRN_RigElementKeyDetailsDefs::ActivePinBackgroundAlpha)))
			.ActiveForegroundColor(FSlateColor(FLinearColor(1.f, 1.f, 1.f, FTLLRN_RigElementKeyDetailsDefs::ActivePinForegroundAlpha)))
			.InactiveBackgroundColor(FSlateColor(FLinearColor(1.f, 1.f, 1.f, FTLLRN_RigElementKeyDetailsDefs::InactivePinBackgroundAlpha)))
			.InactiveForegroundColor(FSlateColor(FLinearColor(1.f, 1.f, 1.f, FTLLRN_RigElementKeyDetailsDefs::InactivePinForegroundAlpha)))
			.OnElementNameChanged(this, &FTLLRN_RigElementKeyDetails::OnElementNameChanged)
			.OnGetSelectedClicked(this, &FTLLRN_RigElementKeyDetails::OnGetSelectedClicked)
			//.OnGetElementNameWidget(this, &FTLLRN_RigElementKeyDetails::OnGetElementNameWidget)
			.OnSelectInHierarchyClicked(this, &FTLLRN_RigElementKeyDetails::OnSelectInHierarchyClicked)
			.OnGetElementNameAsText_Raw(this, &FTLLRN_RigElementKeyDetails::GetElementNameAsText)
			.OnGetElementType(this, &FTLLRN_RigElementKeyDetails::GetElementType)
		];
	}
}

void FTLLRN_RigElementKeyDetails::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if (InStructPropertyHandle->IsValidHandle())
	{
		// only fill the children if the blueprint cannot be found
		if (BlueprintBeingCustomized == nullptr)
		{
			uint32 NumChildren = 0;
			InStructPropertyHandle->GetNumChildren(NumChildren);

			for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
			{
				StructBuilder.AddProperty(InStructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef());
			}
		}
	}
}

ETLLRN_RigElementType FTLLRN_RigElementKeyDetails::GetElementType() const
{
	ETLLRN_RigElementType ElementType = ETLLRN_RigElementType::None;
	if (TypeHandle.IsValid())
	{
		uint8 Index = 0;
		TypeHandle->GetValue(Index);
		ElementType = (ETLLRN_RigElementType)Index;
	}
	return ElementType;
}

FString FTLLRN_RigElementKeyDetails::GetElementName() const
{
	FString ElementNameStr;
	if (NameHandle.IsValid())
	{
		for(int32 ObjectIndex = 0; ObjectIndex < NameHandle->GetNumPerObjectValues(); ObjectIndex++)
		{
			FString PerObjectValue;
			NameHandle->GetPerObjectValue(ObjectIndex, PerObjectValue);

			if(ObjectIndex == 0)
			{
				ElementNameStr = PerObjectValue;
			}
			else if(ElementNameStr != PerObjectValue)
			{
				return TLLRN_ControlRigDetailsMultipleValues.ToString();
			}
		}
	}
	return ElementNameStr;
}

void FTLLRN_RigElementKeyDetails::SetElementName(FString InName)
{
	if (NameHandle.IsValid())
	{
		NameHandle->SetValue(InName);

		// if this is nested below a connection rule
		const TSharedPtr<IPropertyHandle> KeyHandle = NameHandle->GetParentHandle();
		if(KeyHandle.IsValid())
		{
			const TSharedPtr<IPropertyHandle> ParentHandle = KeyHandle->GetParentHandle();
			if(ParentHandle.IsValid())
			{
				if (const TSharedPtr<IPropertyHandleStruct> StructPropertyHandle = ParentHandle->AsStruct())
				{
					if(const UScriptStruct* RuleStruct = Cast<UScriptStruct>(StructPropertyHandle->GetStructData()->GetStruct()))
					{
						if (RuleStruct->IsChildOf(FTLLRN_RigConnectionRule::StaticStruct()))
						{
							const void* RuleMemory = StructPropertyHandle->GetStructData()->GetStructMemory();
							FString RuleContent;
							RuleStruct->ExportText(RuleContent, RuleMemory, RuleMemory, nullptr, PPF_None, nullptr);

							const TSharedPtr<IPropertyHandle> RuleStashHandle = ParentHandle->GetParentHandle();
							
							FTLLRN_RigConnectionRuleStash Stash;
							Stash.ScriptStructPath = RuleStruct->GetPathName();
							Stash.ExportedText = RuleContent;
							
							FString StashContent;
							FTLLRN_RigConnectionRuleStash::StaticStruct()->ExportText(StashContent, &Stash, &Stash, nullptr, PPF_None, nullptr);

							TArray<UObject*> Objects;
							RuleStashHandle->GetOuterObjects(Objects);
							FString FirstObjectValue;
							for (int32 Index = 0; Index < Objects.Num(); Index++)
							{
								(void)RuleStashHandle->SetPerObjectValue(Index, StashContent, EPropertyValueSetFlags::DefaultFlags);
							}
						}
					}
				}
			}
		}
	}
}

void FTLLRN_RigElementKeyDetails::OnElementNameChanged(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectionInfo)
{
	if (InItem.IsValid())
	{
		SetElementName(*InItem);
	}
	else
	{
		SetElementName(FString());
	}
}

FText FTLLRN_RigElementKeyDetails::GetElementNameAsText() const
{
	return FText::FromString(GetElementName());
}

FSlateColor FTLLRN_RigElementKeyDetails::OnGetWidgetForeground(const TSharedPtr<SButton> Button)
{
	float Alpha = (Button.IsValid() && Button->IsHovered()) ? FTLLRN_RigElementKeyDetailsDefs::ActivePinForegroundAlpha : FTLLRN_RigElementKeyDetailsDefs::InactivePinForegroundAlpha;
	return FSlateColor(FLinearColor(1.f, 1.f, 1.f, Alpha));
}

FSlateColor FTLLRN_RigElementKeyDetails::OnGetWidgetBackground(const TSharedPtr<SButton> Button)
{
	float Alpha = (Button.IsValid() && Button->IsHovered()) ? FTLLRN_RigElementKeyDetailsDefs::ActivePinBackgroundAlpha : FTLLRN_RigElementKeyDetailsDefs::InactivePinBackgroundAlpha;
	return FSlateColor(FLinearColor(1.f, 1.f, 1.f, Alpha));
}

FReply FTLLRN_RigElementKeyDetails::OnGetSelectedClicked()
{
	if (BlueprintBeingCustomized)
	{
		const TArray<FTLLRN_RigElementKey>& Selected = BlueprintBeingCustomized->Hierarchy->GetSelectedKeys();
		if (Selected.Num() > 0)
		{
			if (TypeHandle.IsValid())
			{
				uint8 Index = (uint8) Selected[0].Type;
				TypeHandle->SetValue(Index);
			}
			SetElementName(Selected[0].Name.ToString());
		}
	}
	return FReply::Handled();
}

FReply FTLLRN_RigElementKeyDetails::OnSelectInHierarchyClicked()
{
	if (BlueprintBeingCustomized)
	{
		FTLLRN_RigElementKey Key;
		if (TypeHandle.IsValid())
		{
			uint8 Type;
			TypeHandle->GetValue(Type);
			Key.Type = (ETLLRN_RigElementType) Type;
		}

		if (NameHandle.IsValid())
		{
			NameHandle->GetValue(Key.Name);
		}
				
		if (Key.IsValid())
		{
			BlueprintBeingCustomized->GetHierarchyController()->SetSelection({Key});
		}	
	}
	return FReply::Handled();
}

void FTLLRN_RigComputedTransformDetails::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	BlueprintBeingCustomized = nullptr;
	TLLRN_RigElementKeyDetails_GetCustomizedInfo(InStructPropertyHandle, BlueprintBeingCustomized);
}

void FTLLRN_RigComputedTransformDetails::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	TransformHandle = InStructPropertyHandle->GetChildHandle(TEXT("Transform"));

	StructBuilder
	.AddProperty(TransformHandle.ToSharedRef())
	.DisplayName(InStructPropertyHandle->GetPropertyDisplayName());

    FString PropertyPath = TransformHandle->GeneratePathToProperty();

	if(PropertyPath.StartsWith(TEXT("Struct.")))
	{
		PropertyPath.RightChopInline(7);
	}

	if(PropertyPath.StartsWith(TEXT("Pose.")))
	{
		PropertyPath.RightChopInline(5);
		PropertyChain.AddTail(FTLLRN_RigTransformElement::StaticStruct()->FindPropertyByName(TEXT("Pose")));
	}
	else if(PropertyPath.StartsWith(TEXT("Offset.")))
	{
		PropertyPath.RightChopInline(7);
		PropertyChain.AddTail(FTLLRN_RigControlElement::StaticStruct()->FindPropertyByName(TEXT("Offset")));
	}
	else if(PropertyPath.StartsWith(TEXT("Shape.")))
	{
		PropertyPath.RightChopInline(6);
		PropertyChain.AddTail(FTLLRN_RigControlElement::StaticStruct()->FindPropertyByName(TEXT("Shape")));
	}

	if(PropertyPath.StartsWith(TEXT("Current.")))
	{
		PropertyPath.RightChopInline(8);
		PropertyChain.AddTail(FTLLRN_RigCurrentAndInitialTransform::StaticStruct()->FindPropertyByName(TEXT("Current")));
	}
	else if(PropertyPath.StartsWith(TEXT("Initial.")))
	{
		PropertyPath.RightChopInline(8);
		PropertyChain.AddTail(FTLLRN_RigCurrentAndInitialTransform::StaticStruct()->FindPropertyByName(TEXT("Initial")));
	}

	if(PropertyPath.StartsWith(TEXT("Local.")))
	{
		PropertyPath.RightChopInline(6);
		PropertyChain.AddTail(FTLLRN_RigLocalAndGlobalTransform::StaticStruct()->FindPropertyByName(TEXT("Local")));
	}
	else if(PropertyPath.StartsWith(TEXT("Global.")))
	{
		PropertyPath.RightChopInline(7);
		PropertyChain.AddTail(FTLLRN_RigLocalAndGlobalTransform::StaticStruct()->FindPropertyByName(TEXT("Global")));
	}

	PropertyChain.AddTail(TransformHandle->GetProperty());
	PropertyChain.SetActiveMemberPropertyNode(PropertyChain.GetTail()->GetValue());

	const FSimpleDelegate OnTransformChangedDelegate = FSimpleDelegate::CreateSP(this, &FTLLRN_RigComputedTransformDetails::OnTransformChanged, &PropertyChain);
	TransformHandle->SetOnPropertyValueChanged(OnTransformChangedDelegate);
	TransformHandle->SetOnChildPropertyValueChanged(OnTransformChangedDelegate);
}

void FTLLRN_RigComputedTransformDetails::OnTransformChanged(FEditPropertyChain* InPropertyChain)
{
	if(BlueprintBeingCustomized && InPropertyChain)
	{
		if(InPropertyChain->Num() > 1)
		{
			FPropertyChangedEvent ChangeEvent(InPropertyChain->GetHead()->GetValue(), EPropertyChangeType::ValueSet);
			ChangeEvent.SetActiveMemberProperty(InPropertyChain->GetTail()->GetValue());
			FPropertyChangedChainEvent ChainEvent(*InPropertyChain, ChangeEvent);
			BlueprintBeingCustomized->BroadcastPostEditChangeChainProperty(ChainEvent);
		}
	}
}

void FTLLRN_RigControlTransformChannelDetails::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	Handle = InStructPropertyHandle;

	TArray<int32> VisibleEnumValues;
	const TArray<ETLLRN_RigControlTransformChannel>* VisibleChannels = nullptr;

	// loop for controls to figure out the control type
	TArray<UObject*> Objects;
	InStructPropertyHandle->GetOuterObjects(Objects);
	for (UObject* Object : Objects)
	{
		if (const URigVMDetailsViewWrapperObject* WrapperObject = Cast<URigVMDetailsViewWrapperObject>(Object))
		{
			if(WrapperObject->GetWrappedStruct() == FTLLRN_RigControlElement::StaticStruct())
			{
				const FTLLRN_RigControlElement ControlElement = WrapperObject->GetContent<FTLLRN_RigControlElement>();
				VisibleChannels = GetVisibleChannelsForControlType(ControlElement.Settings.ControlType);
				break;
			}
			if (const URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(WrapperObject->GetOuter()))
			{
				if(UnitNode->GetScriptStruct() && UnitNode->GetScriptStruct()->IsChildOf(FTLLRN_RigUnit_HierarchyAddControlElement::StaticStruct()))
				{
					FStructOnScope StructOnScope(UnitNode->GetScriptStruct());
					WrapperObject->GetContent(StructOnScope.GetStructMemory(), StructOnScope.GetStruct());

					const FTLLRN_RigUnit_HierarchyAddControlElement* TLLRN_RigUnit = (const FTLLRN_RigUnit_HierarchyAddControlElement*)StructOnScope.GetStructMemory();
					VisibleChannels = GetVisibleChannelsForControlType(TLLRN_RigUnit->GetControlTypeToSpawn());
					break;
				}
			}
		}
	}

	if(VisibleChannels)
	{
		VisibleEnumValues.Reserve(VisibleChannels->Num());
		for(int32 Index=0; Index < VisibleChannels->Num(); Index++)
		{
			VisibleEnumValues.Add((int32)(*VisibleChannels)[Index]);
		}
	}
	
	HeaderRow
	.NameContent()
	[
		InStructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SEnumComboBox, StaticEnum<ETLLRN_RigControlTransformChannel>())
		.CurrentValue_Raw(this, &FTLLRN_RigControlTransformChannelDetails::GetChannelAsInt32)
		.OnEnumSelectionChanged_Raw(this, &FTLLRN_RigControlTransformChannelDetails::OnChannelChanged)
		.Font(FAppStyle::GetFontStyle(TEXT("MenuItem.Font")))
		.EnumValueSubset(VisibleEnumValues)
	];
}

void FTLLRN_RigControlTransformChannelDetails::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// nothing to do here
}

ETLLRN_RigControlTransformChannel FTLLRN_RigControlTransformChannelDetails::GetChannel() const
{
	uint8 Value = 0;
	Handle->GetValue(Value);
	return (ETLLRN_RigControlTransformChannel)Value;
}

void FTLLRN_RigControlTransformChannelDetails::OnChannelChanged(int32 NewSelection, ESelectInfo::Type InSelectionInfo)
{
	Handle->SetValue((uint8)NewSelection);
}

const TArray<ETLLRN_RigControlTransformChannel>* FTLLRN_RigControlTransformChannelDetails::GetVisibleChannelsForControlType(ETLLRN_RigControlType InControlType)
{
	switch(InControlType)
	{
		case ETLLRN_RigControlType::Position:
		{
			static const TArray<ETLLRN_RigControlTransformChannel> PositionChannels = {
				ETLLRN_RigControlTransformChannel::TranslationX,
				ETLLRN_RigControlTransformChannel::TranslationY,
				ETLLRN_RigControlTransformChannel::TranslationZ
			};
			return &PositionChannels;
		}
		case ETLLRN_RigControlType::Rotator:
		{
			static const TArray<ETLLRN_RigControlTransformChannel> RotatorChannels = {
				ETLLRN_RigControlTransformChannel::Pitch,
				ETLLRN_RigControlTransformChannel::Yaw,
				ETLLRN_RigControlTransformChannel::Roll
			};
			return &RotatorChannels;
		}
		case ETLLRN_RigControlType::Scale:
		{
			static const TArray<ETLLRN_RigControlTransformChannel> ScaleChannels = {
				ETLLRN_RigControlTransformChannel::ScaleX,
				ETLLRN_RigControlTransformChannel::ScaleY,
				ETLLRN_RigControlTransformChannel::ScaleZ
			};
			return &ScaleChannels;
		}
		case ETLLRN_RigControlType::Vector2D:
		{
			static const TArray<ETLLRN_RigControlTransformChannel> Vector2DChannels = {
				ETLLRN_RigControlTransformChannel::TranslationX,
				ETLLRN_RigControlTransformChannel::TranslationY
			};
			return &Vector2DChannels;
		}
		case ETLLRN_RigControlType::EulerTransform:
		{
			static const TArray<ETLLRN_RigControlTransformChannel> EulerTransformChannels = {
				ETLLRN_RigControlTransformChannel::TranslationX,
				ETLLRN_RigControlTransformChannel::TranslationY,
				ETLLRN_RigControlTransformChannel::TranslationZ,
				ETLLRN_RigControlTransformChannel::Pitch,
				ETLLRN_RigControlTransformChannel::Yaw,
				ETLLRN_RigControlTransformChannel::Roll,
				ETLLRN_RigControlTransformChannel::ScaleX,
				ETLLRN_RigControlTransformChannel::ScaleY,
				ETLLRN_RigControlTransformChannel::ScaleZ
			};
			return &EulerTransformChannels;
		}
		default:
		{
			break;
		}
	}
	return nullptr;
}

void FTLLRN_RigBaseElementDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	PerElementInfos.Reset();

	TArray<TWeakObjectPtr<UObject>> DetailObjects;
	DetailBuilder.GetObjectsBeingCustomized(DetailObjects);
	for(TWeakObjectPtr<UObject> DetailObject : DetailObjects)
	{
		URigVMDetailsViewWrapperObject* WrapperObject = CastChecked<URigVMDetailsViewWrapperObject>(DetailObject.Get());

		const FTLLRN_RigElementKey Key = WrapperObject->GetContent<FTLLRN_RigBaseElement>().GetKey();

		FPerElementInfo Info;
		Info.WrapperObject = WrapperObject;
		if (const UTLLRN_RigHierarchy* Hierarchy = Cast<UTLLRN_RigHierarchy>(WrapperObject->GetSubject()))
		{
			Info.Element = Hierarchy->GetHandle(Key);
		}

		if(!Info.Element.IsValid())
		{
			return;
		}
		if(const UTLLRN_ControlRigBlueprint* Blueprint = Info.GetBlueprint())
		{
			Info.DefaultElement = Blueprint->Hierarchy->GetHandle(Key);
		}

		PerElementInfos.Add(Info);
	}

	IDetailCategoryBuilder& GeneralCategory = DetailBuilder.EditCategory(TEXT("General"), LOCTEXT("General", "General"));

	const bool bIsProcedural = IsAnyElementProcedural();
	if(bIsProcedural)
	{
		GeneralCategory.AddCustomRow(LOCTEXT("ProceduralElement", "ProceduralElement")).WholeRowContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ProceduralElementNote", "This item has been created procedurally."))
			.ToolTipText(LOCTEXT("ProceduralElementTooltip", "You cannot edit the values of the item here.\nPlease change the settings on the node\nthat created the item."))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FLinearColor::Red)
		];
	}

	const bool bAllControls = !IsAnyElementNotOfType(ETLLRN_RigElementType::Control);
	const bool bAllAnimationChannels = bAllControls && !IsAnyControlNotOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel);
	if(bAllControls && bAllAnimationChannels)
	{
		GeneralCategory.AddCustomRow(FText::FromString(TEXT("Parent Control")))
		.NameContent()
		[
			SNew(SInlineEditableTextBlock)
			.Text(FText::FromString(TEXT("Parent Control")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.IsEnabled(false)
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(this, &FTLLRN_RigBaseElementDetails::GetParentElementName)
				.IsEnabled(false)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SAssignNew(SelectParentElementButton, SButton)
				.ButtonStyle( FAppStyle::Get(), "NoBorder" )
				.ButtonColorAndOpacity_Lambda([this]() { return FTLLRN_RigElementKeyDetails::OnGetWidgetBackground(SelectParentElementButton); })
				.OnClicked(this, &FTLLRN_RigBaseElementDetails::OnSelectParentElementInHierarchyClicked)
				.ContentPadding(0)
				.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "SelectParentInHierarchyToolTip", "Select Parent in hierarchy"))
				[
					SNew(SImage)
					.ColorAndOpacity_Lambda( [this]() { return FTLLRN_RigElementKeyDetails::OnGetWidgetForeground(SelectParentElementButton); })
					.Image(FAppStyle::GetBrush("Icons.Search"))
				]
			]
		];
	}

	DetailBuilder.HideCategory(TEXT("TLLRN_RigElement"));

	if(!bAllControls || !bAllAnimationChannels)
	{
		GeneralCategory.AddCustomRow(FText::FromString(TEXT("Name")))
		.IsEnabled(!bIsProcedural)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Name")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(SInlineEditableTextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FTLLRN_RigBaseElementDetails::GetName)
			.OnTextCommitted(this, &FTLLRN_RigBaseElementDetails::SetName)
			.OnVerifyTextChanged(this, &FTLLRN_RigBaseElementDetails::OnVerifyNameChanged)
			.IsEnabled(!bIsProcedural && PerElementInfos.Num() == 1 && !bAllAnimationChannels)
		];
	}

	DetailBuilder.HideCategory(TEXT("TLLRN_RigElement"));

	// if we are not a bone, control or null
	if(!IsAnyElementOfType(ETLLRN_RigElementType::Bone) &&
		!IsAnyElementOfType(ETLLRN_RigElementType::Control) &&
		!IsAnyElementOfType(ETLLRN_RigElementType::Null) &&
		!IsAnyElementOfType(ETLLRN_RigElementType::Connector) &&
		!IsAnyElementOfType(ETLLRN_RigElementType::Socket))
	{
		CustomizeMetadata(DetailBuilder);
	}
}

void FTLLRN_RigBaseElementDetails::PendingDelete()
{
	if (MetadataHandle.IsValid())
	{
		for (const FPerElementInfo& Info : PerElementInfos)
		{
			if (UTLLRN_RigHierarchy* Hierarchy = Info.IsValid() ? Info.GetHierarchy() : nullptr)
			{
				if (Hierarchy->OnMetadataChanged().Remove(MetadataHandle))
				{
					break;
				}
			}
		}
		MetadataHandle.Reset();
	}
	
	IDetailCustomization::PendingDelete();
}

FTLLRN_RigElementKey FTLLRN_RigBaseElementDetails::GetElementKey() const
{
	check(PerElementInfos.Num() == 1);
	if (FTLLRN_RigBaseElement* Element = PerElementInfos[0].GetElement())
	{
		return Element->GetKey();
	}
	return FTLLRN_RigElementKey();
}

FText FTLLRN_RigBaseElementDetails::GetName() const
{
	if(PerElementInfos.Num() > 1)
	{
		return TLLRN_ControlRigDetailsMultipleValues;
	}
	return FText::FromName(GetElementKey().Name);
}

FText FTLLRN_RigBaseElementDetails::GetParentElementName() const
{
	if(PerElementInfos.Num() > 1)
	{
		return TLLRN_ControlRigDetailsMultipleValues;
	}
	return FText::FromName(PerElementInfos[0].GetHierarchy()->GetFirstParent(GetElementKey()).Name);
}

void FTLLRN_RigBaseElementDetails::SetName(const FText& InNewText, ETextCommit::Type InCommitType)
{
	if(InCommitType == ETextCommit::OnCleared)
	{
		return;
	}
	
	if(PerElementInfos.Num() > 1)
	{
		return;
	}

	if(PerElementInfos[0].IsProcedural())
	{
		return;
	}
	
	if (UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetDefaultHierarchy())
	{
		BeginDestroy();
		
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);
		const FTLLRN_RigElementKey NewKey = Controller->RenameElement(GetElementKey(), *InNewText.ToString(), true, true);
		if(NewKey.IsValid())
		{
			Controller->SelectElement(NewKey, true, true);
		}
	}
}

bool FTLLRN_RigBaseElementDetails::OnVerifyNameChanged(const FText& InText, FText& OutErrorMessage)
{
	if(PerElementInfos.Num() > 1)
	{
		return false;
	}

	if(PerElementInfos[0].IsProcedural())
	{
		return false;
	}

	const UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetDefaultHierarchy();
	if (Hierarchy == nullptr)
	{
		return false;
	}

	if (GetElementKey().Name.ToString() == InText.ToString())
	{
		return true;
	}

	FString OutErrorMessageStr;
	if (!Hierarchy->IsNameAvailable(FTLLRN_RigName(InText.ToString()), GetElementKey().Type, &OutErrorMessageStr))
	{
		OutErrorMessage = FText::FromString(OutErrorMessageStr);
		return false;
	}

	return true;
}

void FTLLRN_RigBaseElementDetails::OnStructContentsChanged(FProperty* InProperty, const TSharedRef<IPropertyUtilities> PropertyUtilities)
{
	const FPropertyChangedEvent ChangeEvent(InProperty, EPropertyChangeType::ValueSet);
	PropertyUtilities->NotifyFinishedChangingProperties(ChangeEvent);
}

bool FTLLRN_RigBaseElementDetails::IsConstructionModeEnabled() const
{
	if(PerElementInfos.IsEmpty())
	{
		return false;
	}
	
	if(const UTLLRN_ControlRigBlueprint* Blueprint = PerElementInfos[0].GetBlueprint())
	{
		if (const UTLLRN_ControlRig* DebuggedRig = Cast<UTLLRN_ControlRig>(Blueprint ->GetObjectBeingDebugged()))
		{
			return DebuggedRig->IsConstructionModeEnabled();
		}
	}
	return false;
}

TArray<FTLLRN_RigElementKey> FTLLRN_RigBaseElementDetails::GetElementKeys() const
{
	TArray<FTLLRN_RigElementKey> Keys;
	Algo::Transform(PerElementInfos, Keys, [](const FPerElementInfo& Info)
	{
		return Info.GetElement()->GetKey();
	});
	return Keys;
}

const FTLLRN_RigBaseElementDetails::FPerElementInfo& FTLLRN_RigBaseElementDetails::FindElement(const FTLLRN_RigElementKey& InKey) const
{
	const FPerElementInfo* Info = FindElementByPredicate([InKey](const FPerElementInfo& Info)
	{
		return Info.GetElement()->GetKey() == InKey;
	});

	if(Info)
	{
		return *Info;
	}

	static const FPerElementInfo EmptyInfo;
	return EmptyInfo;
}

bool FTLLRN_RigBaseElementDetails::IsAnyElementOfType(ETLLRN_RigElementType InType) const
{
	return ContainsElementByPredicate([InType](const FPerElementInfo& Info)
	{
		return Info.GetElement()->GetType() == InType;
	});
}

bool FTLLRN_RigBaseElementDetails::IsAnyElementNotOfType(ETLLRN_RigElementType InType) const
{
	return ContainsElementByPredicate([InType](const FPerElementInfo& Info)
	{
		return Info.GetElement()->GetType() != InType;
	});
}

bool FTLLRN_RigBaseElementDetails::IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType InType) const
{
	return ContainsElementByPredicate([InType](const FPerElementInfo& Info)
	{
		if(const FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>())
		{
			return ControlElement->Settings.AnimationType == InType;
		}
		return false;
	});
}

bool FTLLRN_RigBaseElementDetails::IsAnyControlNotOfAnimationType(ETLLRN_RigControlAnimationType InType) const
{
	return ContainsElementByPredicate([InType](const FPerElementInfo& Info)
	{
		if(const FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>())
		{
			return ControlElement->Settings.AnimationType != InType;
		}
		return false;
	});
}

bool FTLLRN_RigBaseElementDetails::IsAnyControlOfValueType(ETLLRN_RigControlType InType) const
{
	return ContainsElementByPredicate([InType](const FPerElementInfo& Info)
	{
		if(const FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>())
		{
			return ControlElement->Settings.ControlType == InType;
		}
		return false;
	});
}

bool FTLLRN_RigBaseElementDetails::IsAnyControlNotOfValueType(ETLLRN_RigControlType InType) const
{
	return ContainsElementByPredicate([InType](const FPerElementInfo& Info)
	{
		if(const FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>())
		{
			return ControlElement->Settings.ControlType != InType;
		}
		return false;
	});
}

bool FTLLRN_RigBaseElementDetails::IsAnyElementProcedural() const
{
	return ContainsElementByPredicate([](const FPerElementInfo& Info)
	{
		return Info.IsProcedural();
	});
}

bool FTLLRN_RigBaseElementDetails::IsAnyConnectorImported() const
{
	return ContainsElementByPredicate([](const FPerElementInfo& Info)
	{
		return Info.Element.GetKey().Name.ToString().Contains(UTLLRN_ModularRig::NamespaceSeparator);
	});
}

bool FTLLRN_RigBaseElementDetails::IsAnyConnectorPrimary() const
{
	return ContainsElementByPredicate([](const FPerElementInfo& Info)
	{
		if(const FTLLRN_RigConnectorElement* Connector = Info.Element.Get<FTLLRN_RigConnectorElement>())
		{
			return Connector->IsPrimary();
		}
		return false;
	});
}

bool FTLLRN_RigBaseElementDetails::GetCommonElementType(ETLLRN_RigElementType& OutElementType) const
{
	OutElementType = ETLLRN_RigElementType::None;

	for(const FPerElementInfo& Info : PerElementInfos)
	{
		const FTLLRN_RigElementKey& Key = Info.Element.GetKey();
		if(Key.IsValid())
		{
			if(OutElementType == ETLLRN_RigElementType::None)
			{
				OutElementType = Key.Type;
			}
			else if(OutElementType != Key.Type)
			{
				OutElementType = ETLLRN_RigElementType::None;
				break;
			}
		}
	}

	return OutElementType != ETLLRN_RigElementType::None;
}

bool FTLLRN_RigBaseElementDetails::GetCommonControlType(ETLLRN_RigControlType& OutControlType) const
{
	OutControlType = ETLLRN_RigControlType::Bool;
	
	ETLLRN_RigElementType ElementType = ETLLRN_RigElementType::None;
	if(GetCommonElementType(ElementType))
	{
		if(ElementType == ETLLRN_RigElementType::Control)
		{
			bool bSuccess = false;
			for(const FPerElementInfo& Info : PerElementInfos)
			{
				if(const FTLLRN_RigControlElement* ControlElement = Info.Element.Get<FTLLRN_RigControlElement>())
				{
					if(!bSuccess)
					{
						OutControlType = ControlElement->Settings.ControlType;
						bSuccess = true;
					}
					else if(OutControlType != ControlElement->Settings.ControlType)
					{
						OutControlType = ETLLRN_RigControlType::Bool;
						bSuccess = false;
						break;
					}
				}
			}
			return bSuccess;
		}
	}
	return false;
}

bool FTLLRN_RigBaseElementDetails::GetCommonAnimationType(ETLLRN_RigControlAnimationType& OutAnimationType) const
{
	OutAnimationType = ETLLRN_RigControlAnimationType::AnimationControl;
	
	ETLLRN_RigElementType ElementType = ETLLRN_RigElementType::None;
	if(GetCommonElementType(ElementType))
	{
		if(ElementType == ETLLRN_RigElementType::Control)
		{
			bool bSuccess = false;
			for(const FPerElementInfo& Info : PerElementInfos)
			{
				if(const FTLLRN_RigControlElement* ControlElement = Info.Element.Get<FTLLRN_RigControlElement>())
				{
					if(!bSuccess)
					{
						OutAnimationType = ControlElement->Settings.AnimationType;
						bSuccess = true;
					}
					else if(OutAnimationType != ControlElement->Settings.AnimationType)
					{
						OutAnimationType = ETLLRN_RigControlAnimationType::AnimationControl;
						bSuccess = false;
						break;
					}
				}
			}
			return bSuccess;
		}
	}
	return false;
}

const FTLLRN_RigBaseElementDetails::FPerElementInfo* FTLLRN_RigBaseElementDetails::FindElementByPredicate(const TFunction<bool(const FPerElementInfo&)>& InPredicate) const
{
	return PerElementInfos.FindByPredicate(InPredicate);
}

bool FTLLRN_RigBaseElementDetails::ContainsElementByPredicate(const TFunction<bool(const FPerElementInfo&)>& InPredicate) const
{
	return PerElementInfos.ContainsByPredicate(InPredicate);
}

void FTLLRN_RigBaseElementDetails::RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule)
{
	const URigVMDetailsViewWrapperObject* CDOWrapper = CastChecked<URigVMDetailsViewWrapperObject>(UTLLRN_ControlRigWrapperObject::StaticClass()->GetDefaultObject());
	FTLLRN_RigBoneElementDetails().RegisterSectionMappings(PropertyEditorModule, CDOWrapper->GetClassForStruct(FTLLRN_RigBoneElement::StaticStruct()));
	FTLLRN_RigNullElementDetails().RegisterSectionMappings(PropertyEditorModule, CDOWrapper->GetClassForStruct(FTLLRN_RigNullElement::StaticStruct()));
	FTLLRN_RigControlElementDetails().RegisterSectionMappings(PropertyEditorModule, CDOWrapper->GetClassForStruct(FTLLRN_RigControlElement::StaticStruct()));
}

void FTLLRN_RigBaseElementDetails::RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule, UClass* InClass)
{
	TSharedRef<FPropertySection> MetadataSection = PropertyEditorModule.FindOrCreateSection(InClass->GetFName(), "Metadata", LOCTEXT("Metadata", "Metadata"));
	MetadataSection->AddCategory("Metadata");
}

FReply FTLLRN_RigBaseElementDetails::OnSelectParentElementInHierarchyClicked()
{
	if (PerElementInfos.Num() == 1)
	{
		FTLLRN_RigElementKey Key = GetElementKey();
		if (Key.IsValid())
		{
			const FTLLRN_RigElementKey ParentKey = PerElementInfos[0].GetHierarchy()->GetFirstParent(GetElementKey());
			if(ParentKey.IsValid())
			{
				return OnSelectElementClicked(ParentKey);
			}
		}	
	}
	return FReply::Handled();
}

FReply FTLLRN_RigBaseElementDetails::OnSelectElementClicked(const FTLLRN_RigElementKey& InKey)
{
	if (PerElementInfos.Num() == 1)
	{
		if (InKey.IsValid())
		{
			 PerElementInfos[0].GetHierarchy()->GetController(true)->SetSelection({InKey});
		}	
	}
	return FReply::Handled();
}

void FTLLRN_RigBaseElementDetails::CustomizeMetadata(IDetailLayoutBuilder& DetailBuilder)
{
	if(PerElementInfos.Num() != 1)
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = nullptr;
	if (!MetadataHandle.IsValid())
	{
		const FPerElementInfo& Info = PerElementInfos[0];
		
		Hierarchy = Info.IsValid() ? Info.GetHierarchy() : nullptr;
		if (!Hierarchy)
		{
			return;
		}
			
		TWeakPtr<IPropertyUtilities> WeakPropertyUtilities =  DetailBuilder.GetPropertyUtilities().ToWeakPtr();
		MetadataHandle = Hierarchy->OnMetadataChanged().AddLambda([this, WeakPropertyUtilities](const FTLLRN_RigElementKey& InKey, const FName&)
		{
			if(WeakPropertyUtilities.IsValid())
			{
				const FTLLRN_RigBaseElement* Element = PerElementInfos.Num() == 1 ? PerElementInfos[0].GetElement() : nullptr;
				if (InKey.Type == ETLLRN_RigElementType::All || (Element && Element->GetKey() == InKey))
				{
					WeakPropertyUtilities.Pin()->ForceRefresh();
				}
			}
		});
	}
	
	FTLLRN_RigBaseElement* Element = PerElementInfos[0].Element.Get();
	TArray<FName> MetadataNames = Element->GetOwner()->GetMetadataNames(Element->GetKey());
	
	if(MetadataNames.IsEmpty())
	{
		return;
	}

	IDetailCategoryBuilder& MetadataCategory = DetailBuilder.EditCategory(TEXT("Metadata"), LOCTEXT("Metadata", "Metadata"));
	for(FName MetadataName: MetadataNames)
	{
		FTLLRN_RigBaseMetadata* Metadata = Element->GetMetadata(MetadataName);
		TSharedPtr<FStructOnScope> StructOnScope = MakeShareable(new FStructOnScope(Metadata->GetMetadataStruct(), reinterpret_cast<uint8*>(Metadata)));

		FAddPropertyParams Params;
		Params.CreateCategoryNodes(false);
		Params.ForceShowProperty();
		
		IDetailPropertyRow* Row = MetadataCategory.AddExternalStructureProperty(StructOnScope, TEXT("Value"), EPropertyLocation::Default, Params);
		if(Row)
		{
			(*Row)
			.DisplayName(FText::FromName(Metadata->GetName()))
			.IsEnabled(false);
		}
	}
}

TSharedPtr<TArray<ETLLRN_RigTransformElementDetailsTransform::Type>> FTLLRN_RigTransformElementDetails::PickedTransforms;

void FTLLRN_RigTransformElementDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	FTLLRN_RigBaseElementDetails::CustomizeDetails(DetailBuilder);
}

void FTLLRN_RigTransformElementDetails::RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule, UClass* InClass)
{
	FTLLRN_RigBaseElementDetails::RegisterSectionMappings(PropertyEditorModule, InClass);
	
	TSharedRef<FPropertySection> TransformSection = PropertyEditorModule.FindOrCreateSection(InClass->GetFName(), "Transform", LOCTEXT("Transform", "Transform"));
	TransformSection->AddCategory("General");
	TransformSection->AddCategory("Value");
	TransformSection->AddCategory("Transform");
}

void FTLLRN_RigTransformElementDetails::CustomizeTransform(IDetailLayoutBuilder& DetailBuilder)
{
	if(PerElementInfos.IsEmpty())
	{
		return;
	}
	
	TArray<FTLLRN_RigElementKey> Keys = GetElementKeys();
	Keys = PerElementInfos[0].GetHierarchy()->SortKeys(Keys);

	const bool bIsProcedural = IsAnyElementProcedural();
	const bool bAllControls = !IsAnyElementNotOfType(ETLLRN_RigElementType::Control) && !IsAnyControlOfValueType(ETLLRN_RigControlType::Bool);;
	const bool bAllAnimationChannels = !IsAnyControlNotOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel);
	if(bAllControls && bAllAnimationChannels)
	{
		return;
	}

	bool bShowLimits = false;
	TArray<ETLLRN_RigTransformElementDetailsTransform::Type> TransformTypes;
	TArray<FText> ButtonLabels;
	TArray<FText> ButtonTooltips;

	if(bAllControls)
	{
		TransformTypes = {
			ETLLRN_RigTransformElementDetailsTransform::Initial,
			ETLLRN_RigTransformElementDetailsTransform::Current,
			ETLLRN_RigTransformElementDetailsTransform::Offset
		};
		ButtonLabels = {
			LOCTEXT("Initial", "Initial"),
			LOCTEXT("Current", "Current"),
			LOCTEXT("Offset", "Offset")
		};
		ButtonTooltips = {
			LOCTEXT("InitialTooltip", "Initial transform in the reference pose"),
			LOCTEXT("CurrentTooltip", "Current animation transform"),
			LOCTEXT("OffsetTooltip", "Offset transform under the control")
		};

		bShowLimits = !IsAnyControlNotOfValueType(ETLLRN_RigControlType::EulerTransform);

		if(bShowLimits)
		{
			TransformTypes.Append({
				ETLLRN_RigTransformElementDetailsTransform::Minimum,
				ETLLRN_RigTransformElementDetailsTransform::Maximum
			});
			ButtonLabels.Append({
				LOCTEXT("Min", "Min"),
				LOCTEXT("Max", "Max")
			});
			ButtonTooltips.Append({
				LOCTEXT("ValueMinimumTooltip", "The minimum limit(s) for the control"),
				LOCTEXT("ValueMaximumTooltip", "The maximum limit(s) for the control")
			});
		}
	}
	else
	{
		TransformTypes = {
			ETLLRN_RigTransformElementDetailsTransform::Initial,
			ETLLRN_RigTransformElementDetailsTransform::Current
		};
		ButtonLabels = {
			LOCTEXT("Initial", "Initial"),
			LOCTEXT("Current", "Current")
		};
		ButtonTooltips = {
			LOCTEXT("InitialTooltip", "Initial transform in the reference pose"),
			LOCTEXT("CurrentTooltip", "Current animation transform")
		};
	}

	TArray<bool> bTransformsEnabled;

	// determine if the transforms are enabled
	for(int32 Index = 0; Index < TransformTypes.Num(); Index++)
	{
		const ETLLRN_RigTransformElementDetailsTransform::Type CurrentTransformType = TransformTypes[Index];

		bool bIsTransformEnabled = true;

		if(bIsProcedural)
		{
			// procedural items only allow editing of the current transform
			bIsTransformEnabled = CurrentTransformType == ETLLRN_RigTransformElementDetailsTransform::Current; 
		}

		if(bIsTransformEnabled)
		{
			if (IsAnyElementOfType(ETLLRN_RigElementType::Control))
			{
				bIsTransformEnabled = IsAnyControlOfValueType(ETLLRN_RigControlType::EulerTransform) ||
					IsAnyControlOfValueType(ETLLRN_RigControlType::Transform) ||
					CurrentTransformType == ETLLRN_RigTransformElementDetailsTransform::Offset;

				if(!bIsTransformEnabled)
				{
					ButtonTooltips[Index] = FText::FromString(
						FString::Printf(TEXT("%s\n%s"),
							*ButtonTooltips[Index].ToString(), 
							TEXT("Only transform controls can be edited here. Refer to the 'Value' section instead.")));
				}
			}
			else if (IsAnyElementOfType(ETLLRN_RigElementType::Bone) && CurrentTransformType == ETLLRN_RigTransformElementDetailsTransform::Initial)
			{
				for(const FPerElementInfo& Info : PerElementInfos)
				{
					if(const FTLLRN_RigBoneElement* BoneElement = Info.GetElement<FTLLRN_RigBoneElement>())
					{
						bIsTransformEnabled = BoneElement->BoneType == ETLLRN_RigBoneType::User;

						if(!bIsTransformEnabled)
						{
							ButtonTooltips[Index] = FText::FromString(
								FString::Printf(TEXT("%s\n%s"),
									*ButtonTooltips[Index].ToString(), 
									TEXT("Imported Bones' initial transform cannot be edited.")));
						}
					}
				}			
			}
		}
		bTransformsEnabled.Add(bIsTransformEnabled);
	}

	if(!PickedTransforms.IsValid())
	{
		PickedTransforms = MakeShareable(new TArray<ETLLRN_RigTransformElementDetailsTransform::Type>({ETLLRN_RigTransformElementDetailsTransform::Current}));
	}

	TSharedPtr<SSegmentedControl<ETLLRN_RigTransformElementDetailsTransform::Type>> TransformChoiceWidget =
		SSegmentedControl<ETLLRN_RigTransformElementDetailsTransform::Type>::Create(
			TransformTypes,
			ButtonLabels,
			ButtonTooltips,
			*PickedTransforms.Get(),
			true,
			SSegmentedControl<ETLLRN_RigTransformElementDetailsTransform::Type>::FOnValuesChanged::CreateLambda(
				[](TArray<ETLLRN_RigTransformElementDetailsTransform::Type> NewSelection)
				{
					(*FTLLRN_RigTransformElementDetails::PickedTransforms.Get()) = NewSelection;
				}
			)
		);

	IDetailCategoryBuilder& TransformCategory = DetailBuilder.EditCategory(TEXT("Transform"), LOCTEXT("Transform", "Transform"));
	AddChoiceWidgetRow(TransformCategory, FText::FromString(TEXT("TransformType")), TransformChoiceWidget.ToSharedRef());

	SAdvancedTransformInputBox<FEulerTransform>::FArguments TransformWidgetArgs = SAdvancedTransformInputBox<FEulerTransform>::FArguments()
	.DisplayToggle(false)
	.DisplayRelativeWorld(true)
	.Font(IDetailLayoutBuilder::GetDetailFont());

	for(int32 Index = 0; Index < ButtonLabels.Num(); Index++)
	{
		const ETLLRN_RigTransformElementDetailsTransform::Type CurrentTransformType = TransformTypes[Index];
		ETLLRN_RigControlValueType CurrentValueType = ETLLRN_RigControlValueType::Current;
		switch(CurrentTransformType)
		{
			case ETLLRN_RigTransformElementDetailsTransform::Initial:
			{
				CurrentValueType = ETLLRN_RigControlValueType::Initial;
				break;
			}
			case ETLLRN_RigTransformElementDetailsTransform::Minimum:
			{
				CurrentValueType = ETLLRN_RigControlValueType::Minimum;
				break;
			}
			case ETLLRN_RigTransformElementDetailsTransform::Maximum:
			{
				CurrentValueType = ETLLRN_RigControlValueType::Maximum;
				break;
			}
		}

		TransformWidgetArgs.Visibility_Lambda([TransformChoiceWidget, Index]() -> EVisibility
		{
			return TransformChoiceWidget->HasValue((ETLLRN_RigTransformElementDetailsTransform::Type)Index) ? EVisibility::Visible : EVisibility::Collapsed;
		});

		TransformWidgetArgs.IsEnabled(bTransformsEnabled[Index]);

		CreateEulerTransformValueWidgetRow(
			Keys,
			TransformWidgetArgs,
			TransformCategory,
			ButtonLabels[Index],
			ButtonTooltips[Index],
			CurrentTransformType,
			CurrentValueType);
	}
}

bool FTLLRN_RigTransformElementDetails::IsCurrentLocalEnabled() const
{
	return IsAnyElementOfType(ETLLRN_RigElementType::Control);
}

void FTLLRN_RigTransformElementDetails::AddChoiceWidgetRow(IDetailCategoryBuilder& InCategory, const FText& InSearchText, TSharedRef<SWidget> InWidget)
{
	InCategory.AddCustomRow(FText::FromString(TEXT("TransformType")))
	.ValueContent()
	.MinDesiredWidth(375.f)
	.MaxDesiredWidth(375.f)
	.HAlign(HAlign_Left)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			InWidget
		]
	];
}

FDetailWidgetRow& FTLLRN_RigTransformElementDetails::CreateTransformComponentValueWidgetRow(
	ETLLRN_RigControlType InControlType,
	const TArray<FTLLRN_RigElementKey>& Keys,
	SAdvancedTransformInputBox<FEulerTransform>::FArguments TransformWidgetArgs,
	IDetailCategoryBuilder& CategoryBuilder,
	const FText& Label,
	const FText& Tooltip,
	ETLLRN_RigTransformElementDetailsTransform::Type CurrentTransformType,
	ETLLRN_RigControlValueType ValueType,
	TSharedPtr<SWidget> NameContent)
{
	TransformWidgetArgs
	.Font(IDetailLayoutBuilder::GetDetailFont())
	.AllowEditRotationRepresentation(false);

	if(TransformWidgetArgs._DisplayRelativeWorld &&
		!TransformWidgetArgs._OnGetIsComponentRelative.IsBound() &&
		!TransformWidgetArgs._OnIsComponentRelativeChanged.IsBound())
	{
		TSharedRef<UE::Math::TVector<float>> IsComponentRelative = MakeShareable(new UE::Math::TVector<float>(1.f, 1.f, 1.f));
		
		TransformWidgetArgs
		.OnGetIsComponentRelative_Lambda(
			[IsComponentRelative](ESlateTransformComponent::Type InComponent)
			{
				return IsComponentRelative->operator[]((int32)InComponent) > 0.f;
			})
		.OnIsComponentRelativeChanged_Lambda(
			[IsComponentRelative](ESlateTransformComponent::Type InComponent, bool bIsRelative)
			{
				IsComponentRelative->operator[]((int32)InComponent) = bIsRelative ? 1.f : 0.f;
			});
	}

	TransformWidgetArgs.ConstructLocation(InControlType == ETLLRN_RigControlType::Position);
	TransformWidgetArgs.ConstructRotation(InControlType == ETLLRN_RigControlType::Rotator);
	TransformWidgetArgs.ConstructScale(InControlType == ETLLRN_RigControlType::Scale);

	return CreateEulerTransformValueWidgetRow(
		Keys,
		TransformWidgetArgs,
		CategoryBuilder,
		Label,
		Tooltip,
		CurrentTransformType,
		ValueType,
		NameContent);
}

TSharedPtr<TArray<ETLLRN_RigControlValueType>> FTLLRN_RigControlElementDetails::PickedValueTypes;

FDetailWidgetRow& FTLLRN_RigTransformElementDetails::CreateEulerTransformValueWidgetRow(
	const TArray<FTLLRN_RigElementKey>& Keys,
	SAdvancedTransformInputBox<FEulerTransform>::FArguments TransformWidgetArgs,
	IDetailCategoryBuilder& CategoryBuilder,
	const FText& Label,
	const FText& Tooltip,
	ETLLRN_RigTransformElementDetailsTransform::Type CurrentTransformType,
	ETLLRN_RigControlValueType ValueType,
	TSharedPtr<SWidget> NameContent)
{
	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();
	UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();
	if(ValueType == ETLLRN_RigControlValueType::Current)
	{
		HierarchyToChange = Hierarchy;
	}
	
	const FTLLRN_RigElementTransformWidgetSettings& Settings = FTLLRN_RigElementTransformWidgetSettings::FindOrAdd(ValueType, CurrentTransformType, TransformWidgetArgs);

	const bool bDisplayRelativeWorldOnCurrent = TransformWidgetArgs._DisplayRelativeWorld; 
	if(bDisplayRelativeWorldOnCurrent &&
		!TransformWidgetArgs._OnGetIsComponentRelative.IsBound() &&
		!TransformWidgetArgs._OnIsComponentRelativeChanged.IsBound())
	{
		TSharedRef<UE::Math::TVector<float>> IsComponentRelativeStorage = Settings.IsComponentRelative;
		
		TransformWidgetArgs.OnGetIsComponentRelative_Lambda(
			[IsComponentRelativeStorage](ESlateTransformComponent::Type InComponent)
			{
				return IsComponentRelativeStorage->operator[]((int32)InComponent) > 0.f;
			})
		.OnIsComponentRelativeChanged_Lambda(
			[IsComponentRelativeStorage](ESlateTransformComponent::Type InComponent, bool bIsRelative)
			{
				IsComponentRelativeStorage->operator[]((int32)InComponent) = bIsRelative ? 1.f : 0.f;
			});
	}

	const TSharedPtr<ESlateRotationRepresentation::Type> RotationRepresentationStorage = Settings.RotationRepresentation;
	TransformWidgetArgs.RotationRepresentation(RotationRepresentationStorage);

	auto IsComponentRelative = [TransformWidgetArgs](int32 Component) -> bool
	{
		if(TransformWidgetArgs._OnGetIsComponentRelative.IsBound())
		{
			return TransformWidgetArgs._OnGetIsComponentRelative.Execute((ESlateTransformComponent::Type)Component);
		}
		return true;
	};

	auto ConformComponentRelative = [TransformWidgetArgs, IsComponentRelative](int32 Component)
	{
		if(TransformWidgetArgs._OnIsComponentRelativeChanged.IsBound())
		{
			bool bRelative = IsComponentRelative(Component);
			TransformWidgetArgs._OnIsComponentRelativeChanged.Execute(ESlateTransformComponent::Location, bRelative);
			TransformWidgetArgs._OnIsComponentRelativeChanged.Execute(ESlateTransformComponent::Rotation, bRelative);
			TransformWidgetArgs._OnIsComponentRelativeChanged.Execute(ESlateTransformComponent::Scale, bRelative);
		}
	};

	TransformWidgetArgs.IsScaleLocked(Settings.IsScaleLocked);

	switch(CurrentTransformType)
	{
		case ETLLRN_RigTransformElementDetailsTransform::Minimum:
		case ETLLRN_RigTransformElementDetailsTransform::Maximum:
		{
			TransformWidgetArgs.AllowEditRotationRepresentation(false);
			TransformWidgetArgs.DisplayRelativeWorld(false);
			TransformWidgetArgs.DisplayToggle(true);
			TransformWidgetArgs.OnGetToggleChecked_Lambda([Keys, Hierarchy, ValueType]
				(
					ESlateTransformComponent::Type Component,
					ESlateRotationRepresentation::Type RotationRepresentation,
					ESlateTransformSubComponent::Type SubComponent
				) -> ECheckBoxState
				{
					TOptional<bool> FirstValue;

					for(const FTLLRN_RigElementKey& Key : Keys)
					{
						if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Key))
						{
							TOptional<bool> Value;

							switch(ControlElement->Settings.ControlType)
							{
								case ETLLRN_RigControlType::Position:
								case ETLLRN_RigControlType::Rotator:
								case ETLLRN_RigControlType::Scale:
								{
									if(ControlElement->Settings.LimitEnabled.Num() == 3)
									{
										int32 Index = INDEX_NONE;
										if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
										{
											// TRotator is ordered Roll,Pitch,Yaw, while SNumericRotatorInputBox is ordered Pitch,Yaw,Roll
											switch (SubComponent)
											{
												case ESlateTransformSubComponent::Pitch: Index = 1; break;
												case ESlateTransformSubComponent::Yaw: Index = 2; break;
												case ESlateTransformSubComponent::Roll: Index = 0; break;
											}
										}
										else
										{
											Index = int32(SubComponent) - int32(ESlateTransformSubComponent::X);
										}

										if (Index != INDEX_NONE)
										{
											Value = ControlElement->Settings.LimitEnabled[Index].GetForValueType(ValueType);
										}
									}
									break;
								}
								case ETLLRN_RigControlType::EulerTransform:
								{
									if(ControlElement->Settings.LimitEnabled.Num() == 9)
									{
										switch(Component)
										{
											case ESlateTransformComponent::Location:
											{
												switch(SubComponent)
												{
													case ESlateTransformSubComponent::X:
													{
														Value = ControlElement->Settings.LimitEnabled[0].GetForValueType(ValueType);
														break;
													}
													case ESlateTransformSubComponent::Y:
													{
														Value = ControlElement->Settings.LimitEnabled[1].GetForValueType(ValueType);
														break;
													}
													case ESlateTransformSubComponent::Z:
													{
														Value = ControlElement->Settings.LimitEnabled[2].GetForValueType(ValueType);
														break;
													}
													default:
													{
														break;
													}
												}
												break;
											}
											case ESlateTransformComponent::Rotation:
											{
												switch(SubComponent)
												{
													case ESlateTransformSubComponent::Pitch:
													{
														Value = ControlElement->Settings.LimitEnabled[3].GetForValueType(ValueType);
														break;
													}
													case ESlateTransformSubComponent::Yaw:
													{
														Value = ControlElement->Settings.LimitEnabled[4].GetForValueType(ValueType);
														break;
													}
													case ESlateTransformSubComponent::Roll:
													{
														Value = ControlElement->Settings.LimitEnabled[5].GetForValueType(ValueType);
														break;
													}
													default:
													{
														break;
													}
												}
												break;
											}
											case ESlateTransformComponent::Scale:
											{
												switch(SubComponent)
												{
													case ESlateTransformSubComponent::X:
													{
														Value = ControlElement->Settings.LimitEnabled[6].GetForValueType(ValueType);
														break;
													}
													case ESlateTransformSubComponent::Y:
													{
														Value = ControlElement->Settings.LimitEnabled[7].GetForValueType(ValueType);
														break;
													}
													case ESlateTransformSubComponent::Z:
													{
														Value = ControlElement->Settings.LimitEnabled[8].GetForValueType(ValueType);
														break;
													}
													default:
													{
														break;
													}
												}
												break;
											}
										}
									}
									break;
								}
							}

							if(Value.IsSet())
							{
								if(FirstValue.IsSet())
								{
									if(FirstValue.GetValue() != Value.GetValue())
									{
										return ECheckBoxState::Undetermined;
									}
								}
								else
								{
									FirstValue = Value;
								}
							}
						}
					}

					if(!ensure(FirstValue.IsSet()))
					{
						return ECheckBoxState::Undetermined;
					}
					return FirstValue.GetValue() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				});
				
			TransformWidgetArgs.OnToggleChanged_Lambda([ValueType, Keys, this, Hierarchy]
			(
				ESlateTransformComponent::Type Component,
				ESlateRotationRepresentation::Type RotationRepresentation,
				ESlateTransformSubComponent::Type SubComponent,
				ECheckBoxState CheckState
			)
			{
				if(CheckState == ECheckBoxState::Undetermined)
				{
					return;
				}

				const bool Value = CheckState == ECheckBoxState::Checked;

				FScopedTransaction Transaction(LOCTEXT("ChangeLimitToggle", "Change Limit Toggle"));
				Hierarchy->Modify();

				for(const FTLLRN_RigElementKey& Key : Keys)
				{
					if(FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Key))
					{
						switch(ControlElement->Settings.ControlType)
						{
							case ETLLRN_RigControlType::Position:
							case ETLLRN_RigControlType::Rotator:
							case ETLLRN_RigControlType::Scale:
							{
								if(ControlElement->Settings.LimitEnabled.Num() == 3)
								{
									int32 Index = INDEX_NONE;
									if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
									{
										// TRotator is ordered Roll,Pitch,Yaw, while SNumericRotatorInputBox is ordered Pitch,Yaw,Roll
										switch (SubComponent)
										{
											case ESlateTransformSubComponent::Pitch: Index = 1; break;
											case ESlateTransformSubComponent::Yaw: Index = 2; break;
											case ESlateTransformSubComponent::Roll: Index = 0; break;
										}
									}
									else
									{
										Index = int32(SubComponent) - int32(ESlateTransformSubComponent::X);
									}

									if (Index != INDEX_NONE)
									{
										ControlElement->Settings.LimitEnabled[Index].SetForValueType(ValueType, Value);
									}
								}
								break;
							}
							case ETLLRN_RigControlType::EulerTransform:
							{
								if(ControlElement->Settings.LimitEnabled.Num() == 9)
								{
									switch(Component)
									{
										case ESlateTransformComponent::Location:
										{
											switch(SubComponent)
											{
												case ESlateTransformSubComponent::X:
												{
													ControlElement->Settings.LimitEnabled[0].SetForValueType(ValueType, Value);
													break;
												}
												case ESlateTransformSubComponent::Y:
												{
													ControlElement->Settings.LimitEnabled[1].SetForValueType(ValueType, Value);
													break;
												}
												case ESlateTransformSubComponent::Z:
												{
													ControlElement->Settings.LimitEnabled[2].SetForValueType(ValueType, Value);
													break;
												}
												default:
												{
													break;
												}
											}
											break;
										}
										case ESlateTransformComponent::Rotation:
										{
											switch(SubComponent)
											{
												case ESlateTransformSubComponent::Pitch:
												{
													ControlElement->Settings.LimitEnabled[3].SetForValueType(ValueType, Value);
													break;
												}
												case ESlateTransformSubComponent::Yaw:
												{
													ControlElement->Settings.LimitEnabled[4].SetForValueType(ValueType, Value);
													break;
												}
												case ESlateTransformSubComponent::Roll:
												{
													ControlElement->Settings.LimitEnabled[5].SetForValueType(ValueType, Value);
													break;
												}
												default:
												{
													break;
												}
											}
											break;
										}
										case ESlateTransformComponent::Scale:
										{
											switch(SubComponent)
											{
												case ESlateTransformSubComponent::X:
												{
													ControlElement->Settings.LimitEnabled[6].SetForValueType(ValueType, Value);
													break;
												}
												case ESlateTransformSubComponent::Y:
												{
													ControlElement->Settings.LimitEnabled[7].SetForValueType(ValueType, Value);
													break;
												}
												case ESlateTransformSubComponent::Z:
												{
													ControlElement->Settings.LimitEnabled[8].SetForValueType(ValueType, Value);
													break;
												}
												default:
												{
													break;
												}
											}
											break;
										}
									}
								}
								break;
							}
						}
						
						Hierarchy->SetControlSettings(ControlElement, ControlElement->Settings, true, true, true);
					}
				}
			});
			break;
		}
		default:
		{
			TransformWidgetArgs.AllowEditRotationRepresentation(true);
			TransformWidgetArgs.DisplayRelativeWorld(bDisplayRelativeWorldOnCurrent);
			TransformWidgetArgs.DisplayToggle(false);
			TransformWidgetArgs._OnGetToggleChecked.Unbind();
			TransformWidgetArgs._OnToggleChanged.Unbind();
			break;
		}
	}

	auto GetRelativeAbsoluteTransforms = [CurrentTransformType, Keys, Hierarchy](
		const FTLLRN_RigElementKey& Key,
		ETLLRN_RigTransformElementDetailsTransform::Type InTransformType = ETLLRN_RigTransformElementDetailsTransform::Max
		) -> TPair<FEulerTransform, FEulerTransform>
	{
		if(InTransformType == ETLLRN_RigTransformElementDetailsTransform::Max)
		{
			InTransformType = CurrentTransformType;
		}

		FEulerTransform RelativeTransform = FEulerTransform::Identity;
		FEulerTransform AbsoluteTransform = FEulerTransform::Identity;

		const bool bInitial = InTransformType == ETLLRN_RigTransformElementDetailsTransform::Initial; 
		if(bInitial || InTransformType == ETLLRN_RigTransformElementDetailsTransform::Current)
		{
			RelativeTransform.FromFTransform(Hierarchy->GetLocalTransform(Key, bInitial));
			AbsoluteTransform.FromFTransform(Hierarchy->GetGlobalTransform(Key, bInitial));

			if(FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Key))
			{
				switch(ControlElement->Settings.ControlType)
				{
					case ETLLRN_RigControlType::Rotator:
					case ETLLRN_RigControlType::EulerTransform:
					case ETLLRN_RigControlType::Transform:
					case ETLLRN_RigControlType::TransformNoScale:
					{
						FVector Vector;
						if(const UTLLRN_ControlRig* TLLRN_ControlRig = Hierarchy->GetTypedOuter<UTLLRN_ControlRig>())
						{
							Vector = TLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement, bInitial);
						}
						else
						{
							Vector = Hierarchy->GetControlSpecifiedEulerAngle(ControlElement, bInitial);
						}
						RelativeTransform.Rotation =  FRotator(Vector.Y, Vector.Z, Vector.X);
						break;
					}
					default:
					{
						break;
					}
				}
			}
		}
		else
		{
			if(FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Key))
			{
				const ETLLRN_RigControlType ControlType = ControlElement->Settings.ControlType;

				if(InTransformType == ETLLRN_RigTransformElementDetailsTransform::Offset)
				{
					RelativeTransform.FromFTransform(Hierarchy->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::InitialLocal));
					AbsoluteTransform.FromFTransform(Hierarchy->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::InitialGlobal));
				}
				else if(InTransformType == ETLLRN_RigTransformElementDetailsTransform::Minimum)
				{
					switch(ControlType)
					{
						case ETLLRN_RigControlType::Position:
						{
							const FVector Data = 
								(FVector)Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Minimum)
								.Get<FVector3f>();
							AbsoluteTransform = RelativeTransform = FEulerTransform(Data, FRotator::ZeroRotator, FVector::OneVector);
							break;
						}
						case ETLLRN_RigControlType::Rotator:
						{
							const FVector Data = 
								(FVector)Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Minimum)
								.Get<FVector3f>();
							FRotator Rotator = FRotator::MakeFromEuler(Data);
							AbsoluteTransform = RelativeTransform = FEulerTransform(FVector::ZeroVector, Rotator, FVector::OneVector);
							break;
						}
						case ETLLRN_RigControlType::Scale:
						{
							const FVector Data = 
								(FVector)Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Minimum)
								.Get<FVector3f>();
							AbsoluteTransform = RelativeTransform = FEulerTransform(FVector::ZeroVector, FRotator::ZeroRotator, Data);
							break;
						}
						case ETLLRN_RigControlType::EulerTransform:
						{
							const FTLLRN_RigControlValue::FEulerTransform_Float EulerTransform = 
								Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Minimum)
								.Get<FTLLRN_RigControlValue::FEulerTransform_Float>();
							AbsoluteTransform = RelativeTransform = EulerTransform.ToTransform();
							break;
						}
					}
				}
				else if(InTransformType == ETLLRN_RigTransformElementDetailsTransform::Maximum)
				{
					switch(ControlType)
					{
						case ETLLRN_RigControlType::Position:
						{
							const FVector Data = 
								(FVector)Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Maximum)
								.Get<FVector3f>();
							AbsoluteTransform = RelativeTransform = FEulerTransform(Data, FRotator::ZeroRotator, FVector::OneVector);
							break;
						}
						case ETLLRN_RigControlType::Rotator:
						{
							const FVector Data = 
								(FVector)Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Maximum)
								.Get<FVector3f>();
							FRotator Rotator = FRotator::MakeFromEuler(Data);
							AbsoluteTransform = RelativeTransform = FEulerTransform(FVector::ZeroVector, Rotator, FVector::OneVector);
							break;
						}
						case ETLLRN_RigControlType::Scale:
						{
							const FVector Data = 
								(FVector)Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Maximum)
								.Get<FVector3f>();
							AbsoluteTransform = RelativeTransform = FEulerTransform(FVector::ZeroVector, FRotator::ZeroRotator, Data);
							break;
						}
						case ETLLRN_RigControlType::EulerTransform:
						{
							const FTLLRN_RigControlValue::FEulerTransform_Float EulerTransform = 
								Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Maximum)
								.Get<FTLLRN_RigControlValue::FEulerTransform_Float>();
							AbsoluteTransform = RelativeTransform = EulerTransform.ToTransform();
							break;
						}
					}
				}
			}
		}

		return TPair<FEulerTransform, FEulerTransform>(RelativeTransform, AbsoluteTransform);
	};

	
	auto GetCombinedTransform = [IsComponentRelative, GetRelativeAbsoluteTransforms](
		const FTLLRN_RigElementKey& Key,
		ETLLRN_RigTransformElementDetailsTransform::Type InTransformType = ETLLRN_RigTransformElementDetailsTransform::Max
		) -> FEulerTransform
	{
		const TPair<FEulerTransform, FEulerTransform> TransformPair = GetRelativeAbsoluteTransforms(Key, InTransformType);
		const FEulerTransform RelativeTransform = TransformPair.Key;
		const FEulerTransform AbsoluteTransform = TransformPair.Value;

		FEulerTransform Xfo;
		Xfo.SetLocation((IsComponentRelative(0)) ? RelativeTransform.GetLocation() : AbsoluteTransform.GetLocation());
		Xfo.SetRotator((IsComponentRelative(1)) ? RelativeTransform.Rotator() : AbsoluteTransform.Rotator());
		Xfo.SetScale3D((IsComponentRelative(2)) ? RelativeTransform.GetScale3D() : AbsoluteTransform.GetScale3D());

		return Xfo;
	};

	auto GetSingleTransform = [GetRelativeAbsoluteTransforms](
		const FTLLRN_RigElementKey& Key,
		bool bIsRelative,
		ETLLRN_RigTransformElementDetailsTransform::Type InTransformType = ETLLRN_RigTransformElementDetailsTransform::Max
		) -> FEulerTransform
	{
		const TPair<FEulerTransform, FEulerTransform> TransformPair = GetRelativeAbsoluteTransforms(Key, InTransformType);
		const FEulerTransform RelativeTransform = TransformPair.Key;
		const FEulerTransform AbsoluteTransform = TransformPair.Value;
		return bIsRelative ? RelativeTransform : AbsoluteTransform;
	};

	auto SetSingleTransform = [CurrentTransformType, GetRelativeAbsoluteTransforms, this, Hierarchy](
		const FTLLRN_RigElementKey& Key,
		FEulerTransform InTransform,
		bool bIsRelative,
		bool bSetupUndoRedo)
	{
		const bool bCurrent = CurrentTransformType == ETLLRN_RigTransformElementDetailsTransform::Current; 
		const bool bInitial = CurrentTransformType == ETLLRN_RigTransformElementDetailsTransform::Initial; 

		bool bConstructionModeEnabled = false;
		if (UTLLRN_ControlRig* DebuggedRig = Cast<UTLLRN_ControlRig>(PerElementInfos[0].GetBlueprint()->GetObjectBeingDebugged()))
		{
			bConstructionModeEnabled = DebuggedRig->IsConstructionModeEnabled();
		}

		TArray<UTLLRN_RigHierarchy*> HierarchiesToUpdate;
		HierarchiesToUpdate.Add(Hierarchy);
		if(!bCurrent || bConstructionModeEnabled)
		{
			HierarchiesToUpdate.Add(PerElementInfos[0].GetDefaultHierarchy());
		}

		for(UTLLRN_RigHierarchy* HierarchyToUpdate : HierarchiesToUpdate)
		{
			if(bInitial || CurrentTransformType == ETLLRN_RigTransformElementDetailsTransform::Current)
			{
				if(bIsRelative)
				{
					HierarchyToUpdate->SetLocalTransform(Key, InTransform.ToFTransform(), bInitial, true, bSetupUndoRedo);

					if(FTLLRN_RigControlElement* ControlElement = HierarchyToUpdate->Find<FTLLRN_RigControlElement>(Key))
					{
						switch(ControlElement->Settings.ControlType)
						{
							case ETLLRN_RigControlType::Rotator:
							case ETLLRN_RigControlType::EulerTransform:
							case ETLLRN_RigControlType::Transform:
							case ETLLRN_RigControlType::TransformNoScale:
							{
								FVector EulerAngle(InTransform.Rotator().Roll, InTransform.Rotator().Pitch, InTransform.Rotator().Yaw);
								HierarchyToUpdate->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle, bInitial);
								break;
							}
							default:
							{
								break;
							}
						}
					}
				}
				else
				{
					HierarchyToUpdate->SetGlobalTransform(Key, InTransform.ToFTransform(), bInitial, true, bSetupUndoRedo);
				}
			}
			else
			{
				if(FTLLRN_RigControlElement* ControlElement = HierarchyToUpdate->Find<FTLLRN_RigControlElement>(Key))
				{
					const ETLLRN_RigControlType ControlType = ControlElement->Settings.ControlType;

					if(CurrentTransformType == ETLLRN_RigTransformElementDetailsTransform::Offset)
					{
						if(!bIsRelative)
						{
							const FTransform ParentTransform = HierarchyToUpdate->GetParentTransform(Key, bInitial);
							InTransform.FromFTransform(InTransform.ToFTransform().GetRelativeTransform(ParentTransform));
						}
						HierarchyToUpdate->SetControlOffsetTransform(Key, InTransform.ToFTransform(), true, true, bSetupUndoRedo);
					}
					else if(CurrentTransformType == ETLLRN_RigTransformElementDetailsTransform::Minimum)
					{
						switch(ControlType)
						{
							case ETLLRN_RigControlType::Position:
							{
								const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>((FVector3f)InTransform.GetLocation());
								HierarchyToUpdate->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Minimum, bSetupUndoRedo, true);
								break;
							}
							case ETLLRN_RigControlType::Rotator:
							{
								const FVector3f Euler = (FVector3f)InTransform.Rotator().Euler();
								const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>(Euler);
								HierarchyToUpdate->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Minimum, bSetupUndoRedo, true);
								break;
							}
							case ETLLRN_RigControlType::Scale:
							{
								const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>((FVector3f)InTransform.GetScale3D());
								HierarchyToUpdate->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Minimum, bSetupUndoRedo, true);
								break;
							}
							case ETLLRN_RigControlType::EulerTransform:
							{
								const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FEulerTransform_Float>(InTransform);
								HierarchyToUpdate->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Minimum, bSetupUndoRedo, true);
								break;
							}
						}
					}
					else if(CurrentTransformType == ETLLRN_RigTransformElementDetailsTransform::Maximum)
					{
						switch(ControlType)
						{
							case ETLLRN_RigControlType::Position:
							{
								const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>((FVector3f)InTransform.GetLocation());
								HierarchyToUpdate->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Maximum, bSetupUndoRedo, true);
								break;
							}
							case ETLLRN_RigControlType::Rotator:
							{
								const FVector3f Euler = (FVector3f)InTransform.Rotator().Euler();
								const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>(Euler);
								HierarchyToUpdate->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Maximum, bSetupUndoRedo, true);
								break;
							}
							case ETLLRN_RigControlType::Scale:
							{
								const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FVector3f>((FVector3f)InTransform.GetScale3D());
								HierarchyToUpdate->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Maximum, bSetupUndoRedo, true);
								break;
							}
							case ETLLRN_RigControlType::EulerTransform:
							{
								const FTLLRN_RigControlValue Value = FTLLRN_RigControlValue::Make<FTLLRN_RigControlValue::FEulerTransform_Float>(InTransform);
								HierarchyToUpdate->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Maximum, bSetupUndoRedo, true);
								break;
							}
						}
					}
				}
			}
		}
	};

	TransformWidgetArgs.OnGetNumericValue_Lambda([Keys, GetCombinedTransform](
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent) -> TOptional<FVector::FReal>
	{
		TOptional<FVector::FReal> FirstValue;

		for(int32 Index = 0; Index < Keys.Num(); Index++)
		{
			const FTLLRN_RigElementKey& Key = Keys[Index];
			FEulerTransform Xfo = GetCombinedTransform(Key);

			TOptional<FVector::FReal> CurrentValue = SAdvancedTransformInputBox<FEulerTransform>::GetNumericValueFromTransform(Xfo, Component, Representation, SubComponent);
			if(!CurrentValue.IsSet())
			{
				return CurrentValue;
			}

			if(Index == 0)
			{
				FirstValue = CurrentValue;
			}
			else
			{
				if(!FMath::IsNearlyEqual(FirstValue.GetValue(), CurrentValue.GetValue()))
				{
					return TOptional<FVector::FReal>();
				}
			}
		}
		
		return FirstValue;
	});

	TransformWidgetArgs.OnNumericValueChanged_Lambda(
	[
		Keys,
		this,
		IsComponentRelative,
		GetSingleTransform,
		SetSingleTransform,
		HierarchyToChange
	](
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		FVector::FReal InNumericValue)
	{
		const bool bIsRelative = IsComponentRelative((int32)Component);

		for(const FTLLRN_RigElementKey& Key : Keys)
		{
			FEulerTransform Transform = GetSingleTransform(Key, bIsRelative);
			FEulerTransform PreviousTransform = Transform;
			SAdvancedTransformInputBox<FEulerTransform>::ApplyNumericValueChange(Transform, InNumericValue, Component, Representation, SubComponent);

			if(!FTLLRN_RigControlElementDetails::Equals(Transform, PreviousTransform))
			{
				if(!SliderTransaction.IsValid())
				{
					SliderTransaction = MakeShareable(new FScopedTransaction(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ChangeNumericValue", "Change Numeric Value")));
					HierarchyToChange->Modify();
				}
							
				SetSingleTransform(Key, Transform, bIsRelative, false);
			}
		}
	});

	TransformWidgetArgs.OnNumericValueCommitted_Lambda(
	[
		Keys,
		this,
		IsComponentRelative,
		GetSingleTransform,
		SetSingleTransform,
		HierarchyToChange
	](
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		FVector::FReal InNumericValue,
		ETextCommit::Type InCommitType)
	{
		const bool bIsRelative = IsComponentRelative((int32)Component);

		{
			FScopedTransaction Transaction(LOCTEXT("ChangeNumericValue", "Change Numeric Value"));
			if(!SliderTransaction.IsValid())
			{
				HierarchyToChange->Modify();
			}
			
			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				FEulerTransform Transform = GetSingleTransform(Key, bIsRelative);
				SAdvancedTransformInputBox<FEulerTransform>::ApplyNumericValueChange(Transform, InNumericValue, Component, Representation, SubComponent);
				SetSingleTransform(Key, Transform, bIsRelative, true);
			}
		}

		SliderTransaction.Reset();
	});

	TransformWidgetArgs.OnBeginSliderMovement_Lambda(
		[
			this
		](
			ESlateTransformComponent::Type Component,
			ESlateRotationRepresentation::Type Representation,
			ESlateTransformSubComponent::Type SubComponent)
		{
			if (UTLLRN_ControlRig* DebuggedRig = Cast<UTLLRN_ControlRig>(PerElementInfos[0].GetBlueprint()->GetObjectBeingDebugged()))
			{
				ETLLRN_ControlRigInteractionType Type = ETLLRN_ControlRigInteractionType::None;
				switch (Component)
				{
					case ESlateTransformComponent::Location: Type = ETLLRN_ControlRigInteractionType::Translate; break;
					case ESlateTransformComponent::Rotation: Type = ETLLRN_ControlRigInteractionType::Rotate; break;
					case ESlateTransformComponent::Scale: Type = ETLLRN_ControlRigInteractionType::Scale; break;
					default: Type = ETLLRN_ControlRigInteractionType::All;
				}
				DebuggedRig->InteractionType = (uint8)Type;
				DebuggedRig->ElementsBeingInteracted.Reset();
				for (FPerElementInfo& ElementInfo : PerElementInfos)
				{
					DebuggedRig->ElementsBeingInteracted.AddUnique(ElementInfo.Element.GetKey());
				}
				
				FTLLRN_ControlRigInteractionScope* InteractionScope = new FTLLRN_ControlRigInteractionScope(DebuggedRig);
				InteractionScopes.Add(InteractionScope);
			}
		});
	TransformWidgetArgs.OnEndSliderMovement_Lambda(
		[
			this
		](
			ESlateTransformComponent::Type Component,
			ESlateRotationRepresentation::Type Representation,
			ESlateTransformSubComponent::Type SubComponent,
			FVector::FReal InNumericValue)
		{
			if (UTLLRN_ControlRig* DebuggedRig = Cast<UTLLRN_ControlRig>(PerElementInfos[0].GetBlueprint()->GetObjectBeingDebugged()))
			{
				DebuggedRig->InteractionType = (uint8)ETLLRN_ControlRigInteractionType::None;
				DebuggedRig->ElementsBeingInteracted.Reset();
			}
			for (FTLLRN_ControlRigInteractionScope* InteractionScope : InteractionScopes)
			{
				if (InteractionScope)
				{
					delete InteractionScope; 
				}
			}
			InteractionScopes.Reset();
		});

	TransformWidgetArgs.OnCopyToClipboard_Lambda([Keys, IsComponentRelative, ConformComponentRelative, GetSingleTransform](
		ESlateTransformComponent::Type InComponent
		)
	{
		if(Keys.Num() == 0)
		{
			return;
		}

		// make sure that we use the same relative setting on all components when copying
		ConformComponentRelative(0);
		const bool bIsRelative = IsComponentRelative(0); 

		const FTLLRN_RigElementKey& FirstKey = Keys[0];
		FEulerTransform Xfo = GetSingleTransform(FirstKey, bIsRelative);

		FString Content;
		switch(InComponent)
		{
			case ESlateTransformComponent::Location:
			{
				const FVector Data = Xfo.GetLocation();
				TBaseStructure<FVector>::Get()->ExportText(Content, &Data, &Data, nullptr, PPF_None, nullptr);
				break;
			}
			case ESlateTransformComponent::Rotation:
			{
				const FRotator Data = Xfo.Rotator();
				TBaseStructure<FRotator>::Get()->ExportText(Content, &Data, &Data, nullptr, PPF_None, nullptr);
				break;
			}
			case ESlateTransformComponent::Scale:
			{
				const FVector Data = Xfo.GetScale3D();
				TBaseStructure<FVector>::Get()->ExportText(Content, &Data, &Data, nullptr, PPF_None, nullptr);
				break;
			}
			case ESlateTransformComponent::Max:
			default:
			{
				TBaseStructure<FEulerTransform>::Get()->ExportText(Content, &Xfo, &Xfo, nullptr, PPF_None, nullptr);
				break;
			}
		}

		if(!Content.IsEmpty())
		{
			FPlatformApplicationMisc::ClipboardCopy(*Content);
		}
	});

	TransformWidgetArgs.OnPasteFromClipboard_Lambda([this, Keys, IsComponentRelative, ConformComponentRelative, GetSingleTransform, SetSingleTransform, HierarchyToChange](
		ESlateTransformComponent::Type InComponent
		)
	{
		if(Keys.Num() == 0)
		{
			return;
		}
		
		
		// make sure that we use the same relative setting on all components when pasting
		ConformComponentRelative(0);
		const bool bIsRelative = IsComponentRelative(0); 

		FString Content;
		FPlatformApplicationMisc::ClipboardPaste(Content);

		if(Content.IsEmpty())
		{
			return;
		}

		FScopedTransaction Transaction(LOCTEXT("PasteTransform", "Paste Transform"));
		HierarchyToChange->Modify();

		for(const FTLLRN_RigElementKey& Key : Keys)
		{
			FEulerTransform Xfo = GetSingleTransform(Key, bIsRelative);
			{
				class FRigPasteTransformWidgetErrorPipe : public FOutputDevice
				{
				public:

					int32 NumErrors;

					FRigPasteTransformWidgetErrorPipe()
						: FOutputDevice()
						, NumErrors(0)
					{
					}

					virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
					{
						UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Error Pasting to Widget: %s"), V);
						NumErrors++;
					}
				};

				FRigPasteTransformWidgetErrorPipe ErrorPipe;
				
				switch(InComponent)
				{
					case ESlateTransformComponent::Location:
					{
						FVector Data = Xfo.GetLocation();
						TBaseStructure<FVector>::Get()->ImportText(*Content, &Data, nullptr, PPF_None, &ErrorPipe, TBaseStructure<FVector>::Get()->GetName(), true);
						Xfo.SetLocation(Data);
						break;
					}
					case ESlateTransformComponent::Rotation:
					{
						FRotator Data = Xfo.Rotator();
						TBaseStructure<FRotator>::Get()->ImportText(*Content, &Data, nullptr, PPF_None, &ErrorPipe, TBaseStructure<FRotator>::Get()->GetName(), true);
						Xfo.SetRotator(Data);
						break;
					}
					case ESlateTransformComponent::Scale:
					{
						FVector Data = Xfo.GetScale3D();
						TBaseStructure<FVector>::Get()->ImportText(*Content, &Data, nullptr, PPF_None, &ErrorPipe, TBaseStructure<FVector>::Get()->GetName(), true);
						Xfo.SetScale3D(Data);
						break;
					}
					case ESlateTransformComponent::Max:
					default:
					{
						TBaseStructure<FEulerTransform>::Get()->ImportText(*Content, &Xfo, nullptr, PPF_None, &ErrorPipe, TBaseStructure<FEulerTransform>::Get()->GetName(), true);
						break;
					}
				}

				if(ErrorPipe.NumErrors == 0)
				{
					SetSingleTransform(Key, Xfo, bIsRelative, true);
				}
			}
		}
	});

	TransformWidgetArgs.DiffersFromDefault_Lambda([
		CurrentTransformType,
		Keys,
		GetSingleTransform
		
	](
		ESlateTransformComponent::Type InComponent) -> bool
	{
		for(const FTLLRN_RigElementKey& Key : Keys)
		{
			const FEulerTransform CurrentTransform = GetSingleTransform(Key, true);
			FEulerTransform DefaultTransform;

			switch(CurrentTransformType)
			{
				case ETLLRN_RigTransformElementDetailsTransform::Current:
				{
					DefaultTransform = GetSingleTransform(Key, true, ETLLRN_RigTransformElementDetailsTransform::Initial);
					break;
				}
				default:
				{
					DefaultTransform = FEulerTransform::Identity; 
					break;
				}
			}

			switch(InComponent)
			{
				case ESlateTransformComponent::Location:
				{
					if(!DefaultTransform.GetLocation().Equals(CurrentTransform.GetLocation()))
					{
						return true;
					}
					break;
				}
				case ESlateTransformComponent::Rotation:
				{
					if(!DefaultTransform.Rotator().Equals(CurrentTransform.Rotator()))
					{
						return true;
					}
					break;
				}
				case ESlateTransformComponent::Scale:
				{
					if(!DefaultTransform.GetScale3D().Equals(CurrentTransform.GetScale3D()))
					{
						return true;
					}
					break;
				}
				default: // also no component whole transform
				{
					if(!DefaultTransform.GetLocation().Equals(CurrentTransform.GetLocation()) ||
						!DefaultTransform.Rotator().Equals(CurrentTransform.Rotator()) ||
						!DefaultTransform.GetScale3D().Equals(CurrentTransform.GetScale3D()))
					{
						return true;
					}
					break;
				}
			}
		}
		return false;
	});

	TransformWidgetArgs.OnResetToDefault_Lambda([this, CurrentTransformType, Keys, GetSingleTransform, SetSingleTransform, HierarchyToChange](
		ESlateTransformComponent::Type InComponent)
	{
		FScopedTransaction Transaction(LOCTEXT("ResetTransformToDefault", "Reset Transform to Default"));
		HierarchyToChange->Modify();

		for(const FTLLRN_RigElementKey& Key : Keys)
		{
			FEulerTransform CurrentTransform = GetSingleTransform(Key, true);
			FEulerTransform DefaultTransform;

			switch(CurrentTransformType)
			{
				case ETLLRN_RigTransformElementDetailsTransform::Current:
				{
					DefaultTransform = GetSingleTransform(Key, true, ETLLRN_RigTransformElementDetailsTransform::Initial);
					break;
				}
				default:
				{
					DefaultTransform = FEulerTransform::Identity; 
					break;
				}
			}

			switch(InComponent)
			{
				case ESlateTransformComponent::Location:
				{
					CurrentTransform.SetLocation(DefaultTransform.GetLocation());
					break;
				}
				case ESlateTransformComponent::Rotation:
				{
					CurrentTransform.SetRotator(DefaultTransform.Rotator());
					break;
				}
				case ESlateTransformComponent::Scale:
				{
					CurrentTransform.SetScale3D(DefaultTransform.GetScale3D());
					break;
				}
				default: // whole transform / max component
				{
					CurrentTransform = DefaultTransform;
					break;
				}
			}

			SetSingleTransform(Key, CurrentTransform, true, true);
		}
	});

	return SAdvancedTransformInputBox<FEulerTransform>::ConstructGroupedTransformRows(
		CategoryBuilder, 
		Label, 
		Tooltip, 
		TransformWidgetArgs,
		NameContent);
}

ETLLRN_RigTransformElementDetailsTransform::Type FTLLRN_RigTransformElementDetails::GetTransformTypeFromValueType(
	ETLLRN_RigControlValueType InValueType)
{
	ETLLRN_RigTransformElementDetailsTransform::Type TransformType = ETLLRN_RigTransformElementDetailsTransform::Current;
	switch(InValueType)
	{
		case ETLLRN_RigControlValueType::Initial:
		{
			TransformType = ETLLRN_RigTransformElementDetailsTransform::Initial;
			break;
		}
		case ETLLRN_RigControlValueType::Minimum:
		{
			TransformType = ETLLRN_RigTransformElementDetailsTransform::Minimum;
			break;
		}
		case ETLLRN_RigControlValueType::Maximum:
		{
			TransformType = ETLLRN_RigTransformElementDetailsTransform::Maximum;
			break;
		}
		default:
		{
			break;
		}
	}
	return TransformType;
}

void FTLLRN_RigBoneElementDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	FTLLRN_RigTransformElementDetails::CustomizeDetails(DetailBuilder);
	CustomizeTransform(DetailBuilder);
	CustomizeMetadata(DetailBuilder);
}

void FTLLRN_RigControlElementDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	FTLLRN_RigTransformElementDetails::CustomizeDetails(DetailBuilder);

	CustomizeControl(DetailBuilder);
	CustomizeValue(DetailBuilder);
	CustomizeTransform(DetailBuilder);
	CustomizeShape(DetailBuilder);
	CustomizeAvailableSpaces(DetailBuilder);
	CustomizeAnimationChannels(DetailBuilder);
	CustomizeMetadata(DetailBuilder);
}

void FTLLRN_RigControlElementDetails::CustomizeValue(IDetailLayoutBuilder& DetailBuilder)
{
	if(PerElementInfos.IsEmpty())
	{
		return;
	}

	if(IsAnyElementNotOfType(ETLLRN_RigElementType::Control))
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();

	// only show this section if all controls are the same type
	const FTLLRN_RigControlElement* FirstControlElement = PerElementInfos[0].GetElement<FTLLRN_RigControlElement>();
	const ETLLRN_RigControlType ControlType = FirstControlElement->Settings.ControlType;
	bool bAllAnimationChannels = true;
	
	for(const FPerElementInfo& Info : PerElementInfos)
	{
		const FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>();
		if(ControlElement->Settings.ControlType != ControlType)
		{
			return;
		}
		if(ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::AnimationChannel)
		{
			bAllAnimationChannels = false;
		}
	}

	// transforms don't show their value here - instead they are shown in the transform section
	if((ControlType == ETLLRN_RigControlType::EulerTransform ||
		ControlType == ETLLRN_RigControlType::Transform ||
		ControlType == ETLLRN_RigControlType::TransformNoScale) &&
		!bAllAnimationChannels)
	{
		return;
	}
	
	TArray<FText> Labels = {
		LOCTEXT("Initial", "Initial"),
		LOCTEXT("Current", "Current")
	};
	TArray<FText> Tooltips = {
		LOCTEXT("ValueInitialTooltip", "The initial animation value of the control"),
		LOCTEXT("ValueCurrentTooltip", "The current animation value of the control")
	};
	TArray<ETLLRN_RigControlValueType> ValueTypes = {
		ETLLRN_RigControlValueType::Initial,
		ETLLRN_RigControlValueType::Current
	};

	// bool doesn't have limits,
	// transform types already got filtered out earlier.
	// integers with enums don't have limits either
	if(ControlType != ETLLRN_RigControlType::Bool &&
		(ControlType != ETLLRN_RigControlType::Integer || !FirstControlElement->Settings.ControlEnum))
	{
		Labels.Append({
			LOCTEXT("Min", "Min"),
			LOCTEXT("Max", "Max")
		});
		Tooltips.Append({
			LOCTEXT("ValueMinimumTooltip", "The minimum limit(s) for the control"),
			LOCTEXT("ValueMaximumTooltip", "The maximum limit(s) for the control")
		});
		ValueTypes.Append({
			ETLLRN_RigControlValueType::Minimum,
			ETLLRN_RigControlValueType::Maximum
		});
	}
	
	IDetailCategoryBuilder& ValueCategory = DetailBuilder.EditCategory(TEXT("Value"), LOCTEXT("Value", "Value"));

	if(!PickedValueTypes.IsValid())
	{
		PickedValueTypes = MakeShareable(new TArray<ETLLRN_RigControlValueType>({ETLLRN_RigControlValueType::Current}));
	}

	TSharedPtr<SSegmentedControl<ETLLRN_RigControlValueType>> ValueTypeChoiceWidget =
		SSegmentedControl<ETLLRN_RigControlValueType>::Create(
			ValueTypes,
			Labels,
			Tooltips,
			*PickedValueTypes.Get(),
			true,
			SSegmentedControl<ETLLRN_RigControlValueType>::FOnValuesChanged::CreateLambda(
				[](TArray<ETLLRN_RigControlValueType> NewSelection)
				{
					(*FTLLRN_RigControlElementDetails::PickedValueTypes.Get()) = NewSelection;
				}
			)
		);

	AddChoiceWidgetRow(ValueCategory, FText::FromString(TEXT("ValueType")), ValueTypeChoiceWidget.ToSharedRef());

	TArray<FTLLRN_RigElementKey> Keys = GetElementKeys();
	Keys = Hierarchy->SortKeys(Keys);

	for(int32 Index=0; Index < ValueTypes.Num(); Index++)
	{
		const ETLLRN_RigControlValueType ValueType = ValueTypes[Index];

		const TAttribute<EVisibility> VisibilityAttribute =
			TAttribute<EVisibility>::CreateLambda([ValueType, ValueTypeChoiceWidget]()-> EVisibility
			{
				return ValueTypeChoiceWidget->HasValue(ValueType) ? EVisibility::Visible : EVisibility::Collapsed; 
			});
		
		switch(ControlType)
		{
			case ETLLRN_RigControlType::Bool:
			{
				CreateBoolValueWidgetRow(Keys, ValueCategory, Labels[Index], Tooltips[Index], ValueType, VisibilityAttribute);
				break;
			}
			case ETLLRN_RigControlType::Float:
			case ETLLRN_RigControlType::ScaleFloat:
			{
				CreateFloatValueWidgetRow(Keys, ValueCategory, Labels[Index], Tooltips[Index], ValueType, VisibilityAttribute);
				break;
			}
			case ETLLRN_RigControlType::Integer:
			{
				bool bIsEnum = false;
				for(const FTLLRN_RigElementKey& Key : Keys)
				{
					if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Key))
					{
						if(ControlElement->Settings.ControlEnum)
						{
							bIsEnum = true;
							break;
						}
					}
				}

				if(bIsEnum)
				{
					CreateEnumValueWidgetRow(Keys, ValueCategory, Labels[Index], Tooltips[Index], ValueType, VisibilityAttribute);
				}
				else
				{
					CreateIntegerValueWidgetRow(Keys, ValueCategory, Labels[Index], Tooltips[Index], ValueType, VisibilityAttribute);
				}
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				CreateVector2DValueWidgetRow(Keys, ValueCategory, Labels[Index], Tooltips[Index], ValueType, VisibilityAttribute);
				break;
			}
			case ETLLRN_RigControlType::Position:
			case ETLLRN_RigControlType::Rotator:
			case ETLLRN_RigControlType::Scale:
			{
				SAdvancedTransformInputBox<FEulerTransform>::FArguments TransformWidgetArgs = SAdvancedTransformInputBox<FEulerTransform>::FArguments()
				.DisplayToggle(false)
				.DisplayRelativeWorld(true)
				.Visibility(VisibilityAttribute);

				CreateTransformComponentValueWidgetRow(
					ControlType,
					GetElementKeys(),
					TransformWidgetArgs,
					ValueCategory,
					Labels[Index],
					Tooltips[Index],
					GetTransformTypeFromValueType(ValueType),
					ValueType);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

void FTLLRN_RigControlElementDetails::CustomizeControl(IDetailLayoutBuilder& DetailBuilder)
{
	if(PerElementInfos.IsEmpty())
	{
		return;
	}

	if(IsAnyElementNotOfType(ETLLRN_RigElementType::Control))
	{
		return;
	}

	const bool bIsProcedural = IsAnyElementProcedural();
	const bool bIsEnabled = !bIsProcedural;
	
	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();
	UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();

	const TSharedPtr<IPropertyHandle> SettingsHandle = DetailBuilder.GetProperty(TEXT("Settings"));
	DetailBuilder.HideProperty(SettingsHandle);

	IDetailCategoryBuilder& ControlCategory = DetailBuilder.EditCategory(TEXT("Control"), LOCTEXT("Control", "Control"));

	const bool bAllAnimationChannels = !IsAnyControlNotOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel);
	static const FText DisplayNameText = LOCTEXT("DisplayName", "Display Name");
	static const FText ChannelNameText = LOCTEXT("ChannelName", "Channel Name");
	const FText DisplayNameLabelText = bAllAnimationChannels ? ChannelNameText : DisplayNameText;

	const TSharedPtr<IPropertyHandle> DisplayNameHandle = SettingsHandle->GetChildHandle(TEXT("DisplayName"));
	ControlCategory.AddCustomRow(DisplayNameLabelText)
	.IsEnabled(bIsEnabled)
	.NameContent()
	[
		DisplayNameHandle->CreatePropertyNameWidget(DisplayNameLabelText)
	]
	.ValueContent()
	[
		SNew(SInlineEditableTextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(this, &FTLLRN_RigControlElementDetails::GetDisplayName)
		.OnTextCommitted(this, &FTLLRN_RigControlElementDetails::SetDisplayName)
		.OnVerifyTextChanged_Lambda([this](const FText& InText, FText& OutErrorMessage) -> bool
		{
			return OnVerifyDisplayNameChanged(InText, OutErrorMessage, GetElementKey());
		})
		.IsEnabled(bIsEnabled && (PerElementInfos.Num() == 1))
	];

	if(bAllAnimationChannels)
	{
		ControlCategory.AddCustomRow(FText::FromString(TEXT("Script Name")))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Script Name")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.IsEnabled(!bIsProcedural)
		]
		.ValueContent()
		[
			SNew(SInlineEditableTextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FTLLRN_RigBaseElementDetails::GetName)
			.OnTextCommitted(this, &FTLLRN_RigBaseElementDetails::SetName)
			.OnVerifyTextChanged(this, &FTLLRN_RigBaseElementDetails::OnVerifyNameChanged)
			.IsEnabled(!bIsProcedural && PerElementInfos.Num() == 1)
		];
	}

	const TSharedRef<IPropertyUtilities> PropertyUtilities = DetailBuilder.GetPropertyUtilities();

	// when control type changes, we have to refresh detail panel
	const TSharedPtr<IPropertyHandle> AnimationTypeHandle = SettingsHandle->GetChildHandle(TEXT("AnimationType"));
	AnimationTypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda(
		[this, PropertyUtilities, HierarchyToChange, Hierarchy]()
		{
			TArray<FTLLRN_RigControlElement> ControlElementsInView = GetElementsInDetailsView<FTLLRN_RigControlElement>();

			if (HierarchyToChange && ControlElementsInView.Num() == PerElementInfos.Num())
			{
				HierarchyToChange->Modify();
				
				for(int32 ControlIndex = 0; ControlIndex< ControlElementsInView.Num(); ControlIndex++)
				{
					const FTLLRN_RigControlElement& ViewElement = ControlElementsInView[ControlIndex];
					FTLLRN_RigControlElement* ControlElement = PerElementInfos[ControlIndex].GetDefaultElement<FTLLRN_RigControlElement>();
					
					ControlElement->Settings.AnimationType = ViewElement.Settings.AnimationType;

					ControlElement->Settings.bGroupWithParentControl =
						ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool ||
						ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float ||
						ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat ||
						ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer ||
						ControlElement->Settings.ControlType == ETLLRN_RigControlType::Vector2D;

					switch(ControlElement->Settings.AnimationType)
					{
						case ETLLRN_RigControlAnimationType::AnimationControl:
						{
							ControlElement->Settings.ShapeVisibility = ETLLRN_RigControlVisibility::UserDefined;
							ControlElement->Settings.bShapeVisible = true;
							break;
						}
						case ETLLRN_RigControlAnimationType::AnimationChannel:
						{
							ControlElement->Settings.ShapeVisibility = ETLLRN_RigControlVisibility::UserDefined;
							ControlElement->Settings.bShapeVisible = false;
							break;
						}
						case ETLLRN_RigControlAnimationType::ProxyControl:
						{
							ControlElement->Settings.ShapeVisibility = ETLLRN_RigControlVisibility::BasedOnSelection;
							ControlElement->Settings.bShapeVisible = true;
							ControlElement->Settings.bGroupWithParentControl = false;
							break;
						}
						default:
						{
							ControlElement->Settings.ShapeVisibility = ETLLRN_RigControlVisibility::UserDefined;
							ControlElement->Settings.bShapeVisible = true;
							ControlElement->Settings.bGroupWithParentControl = false;
							break;
						}
					}

					HierarchyToChange->SetControlSettings(ControlElement, ControlElement->Settings, true, true, true);
					PerElementInfos[ControlIndex].WrapperObject->SetContent<FTLLRN_RigControlElement>(*ControlElement);

					if (HierarchyToChange != Hierarchy)
					{
						if(FTLLRN_RigControlElement* OtherControlElement = PerElementInfos[0].GetElement<FTLLRN_RigControlElement>())
						{
							OtherControlElement->Settings = ControlElement->Settings;
							Hierarchy->SetControlSettings(OtherControlElement, OtherControlElement->Settings, true, true, true);
						}
					}
				}
				
				PropertyUtilities->ForceRefresh();
			}
		}
	));

	ControlCategory.AddProperty(AnimationTypeHandle.ToSharedRef())
	.IsEnabled(bIsEnabled);

	// when control type changes, we have to refresh detail panel
	const TSharedPtr<IPropertyHandle> ControlTypeHandle = SettingsHandle->GetChildHandle(TEXT("ControlType"));
	ControlTypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda(
		[this, PropertyUtilities]()
		{
			TArray<FTLLRN_RigControlElement> ControlElementsInView = GetElementsInDetailsView<FTLLRN_RigControlElement>();
			HandleControlTypeChanged(ControlElementsInView[0].Settings.ControlType, TArray<FTLLRN_RigElementKey>(), PropertyUtilities);
		}
	));

	ControlCategory.AddProperty(ControlTypeHandle.ToSharedRef())
	.IsEnabled(bIsEnabled);

	const bool bSupportsShape = !IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel) &&
		!IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType::VisualCue);

	if (HierarchyToChange != nullptr)
	{
		bool bEnableGroupWithParentControl = true;
		for(const FPerElementInfo& Info : PerElementInfos)
		{
			if(const FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>())
			{
				bool bSingleEnableGroupWithParentControl = false;
				if(const FTLLRN_RigControlElement* ParentElement =
					Cast<FTLLRN_RigControlElement>(Info.GetHierarchy()->GetFirstParent(ControlElement)))
				{
					if(ControlElement->Settings.IsAnimatable() &&
						Info.GetHierarchy()->GetChildren(ControlElement).IsEmpty())
					{
						bSingleEnableGroupWithParentControl = true;
					}
				}

				if(!bSingleEnableGroupWithParentControl)
				{
					bEnableGroupWithParentControl = false;
					break;
				}
			}
		}
		if(bEnableGroupWithParentControl)
		{
			const TSharedPtr<IPropertyHandle> GroupWithParentControlHandle = SettingsHandle->GetChildHandle(TEXT("bGroupWithParentControl"));
			ControlCategory.AddProperty(GroupWithParentControlHandle.ToSharedRef()).DisplayName(FText::FromString(TEXT("Group Channels")))
			.IsEnabled(bIsEnabled);
		}
	}
	
	if(bSupportsShape &&
		!(IsAnyControlNotOfValueType(ETLLRN_RigControlType::Integer) &&
		IsAnyControlNotOfValueType(ETLLRN_RigControlType::Float) &&
		IsAnyControlNotOfValueType(ETLLRN_RigControlType::ScaleFloat) &&
		IsAnyControlNotOfValueType(ETLLRN_RigControlType::Vector2D)))
	{
		const TSharedPtr<IPropertyHandle> PrimaryAxisHandle = SettingsHandle->GetChildHandle(TEXT("PrimaryAxis"));
		ControlCategory.AddProperty(PrimaryAxisHandle.ToSharedRef()).DisplayName(FText::FromString(TEXT("Primary Axis")))
		.IsEnabled(bIsEnabled);
	}

	if(CVarTLLRN_ControlTLLRN_RigHierarchyEnableRotationOrder.GetValueOnAnyThread())
	{
		if(IsAnyControlOfValueType(ETLLRN_RigControlType::EulerTransform))
		{
			const TSharedPtr<IPropertyHandle> UsePreferredRotationOrderHandle = SettingsHandle->GetChildHandle(TEXT("bUsePreferredRotationOrder"));
			ControlCategory.AddProperty(UsePreferredRotationOrderHandle.ToSharedRef()).DisplayName(FText::FromString(TEXT("Use Preferred Rotation Order")))
				.IsEnabled(bIsEnabled);

			const TSharedPtr<IPropertyHandle> PreferredRotationOrderHandle = SettingsHandle->GetChildHandle(TEXT("PreferredRotationOrder"));
			ControlCategory.AddProperty(PreferredRotationOrderHandle.ToSharedRef()).DisplayName(FText::FromString(TEXT("Preferred Rotation Order")))
			.IsEnabled(bIsEnabled);
		}
	}

	if(IsAnyControlOfValueType(ETLLRN_RigControlType::Integer))
	{		
		FDetailWidgetRow* EnumWidgetRow = &ControlCategory.AddCustomRow(FText::FromString(TEXT("ControlEnum")))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Control Enum")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.IsEnabled(bIsEnabled)
		]
		.ValueContent()
		[
			SNew(SRigVMEnumPicker)
			.OnEnumChanged(this, &FTLLRN_RigControlElementDetails::HandleControlEnumChanged, PropertyUtilities)
			.IsEnabled(bIsEnabled)
			.GetCurrentEnum_Lambda([this]()
			{
				UEnum* CommonControlEnum = nullptr;
				for (int32 ControlIndex=0; ControlIndex < PerElementInfos.Num(); ++ControlIndex)
				{
					FPerElementInfo& Info = PerElementInfos[ControlIndex];
					const FTLLRN_RigControlElement ControlInView = Info.WrapperObject->GetContent<FTLLRN_RigControlElement>();
					FTLLRN_RigControlElement* ControlBeingCustomized = Info.GetDefaultElement<FTLLRN_RigControlElement>();
						
					UEnum* ControlEnum = ControlBeingCustomized->Settings.ControlEnum;
					if (ControlIndex == 0)
					{
						CommonControlEnum = ControlEnum;
					}
					else if(ControlEnum != CommonControlEnum)
					{
						CommonControlEnum = nullptr;
						break;
					}
				}
				return CommonControlEnum;
			})
		];
	}

	if(bSupportsShape)
	{
		const TSharedPtr<IPropertyHandle> RestrictSpaceSwitchingHandle = SettingsHandle->GetChildHandle(TEXT("bRestrictSpaceSwitching"));
		ControlCategory
		.AddProperty(RestrictSpaceSwitchingHandle.ToSharedRef())
		.DisplayName(FText::FromString(TEXT("Restrict Switching")))
		.IsEnabled(bIsEnabled);

		// Available Spaces is now handled by its own category (CustomizeAvailableSpaces)
		//const TSharedPtr<IPropertyHandle> CustomizationHandle = SettingsHandle->GetChildHandle(TEXT("Customization"));
		//const TSharedPtr<IPropertyHandle> AvailableSpacesHandle = CustomizationHandle->GetChildHandle(TEXT("AvailableSpaces"));
		//ControlCategory.AddProperty(AvailableSpacesHandle.ToSharedRef())
		//.IsEnabled(bIsEnabled);
	}

	TArray<FTLLRN_RigElementKey> Keys = GetElementKeys();

	if(bSupportsShape)
	{
		const TSharedPtr<IPropertyHandle> DrawLimitsHandle = SettingsHandle->GetChildHandle(TEXT("bDrawLimits"));
		
		ControlCategory
		.AddProperty(DrawLimitsHandle.ToSharedRef()).DisplayName(FText::FromString(TEXT("Draw Limits")))
		.IsEnabled(TAttribute<bool>::CreateLambda([Keys, Hierarchy, bIsEnabled]() -> bool
		{
			if(!bIsEnabled)
			{
				return false;
			}
			
			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Key))
				{
					if(ControlElement->Settings.LimitEnabled.Contains(FTLLRN_RigControlLimitEnabled(true, true)))
					{
						return true;
					}
				}
			}
			return false;
		}));
	}

	ETLLRN_RigControlType CommonControlType = ETLLRN_RigControlType::Bool;
	if(GetCommonControlType(CommonControlType))
	{
		if(FTLLRN_RigControlTransformChannelDetails::GetVisibleChannelsForControlType(CommonControlType) != nullptr)
		{
			const TSharedPtr<IPropertyHandle> FilteredChannelsHandle = SettingsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTLLRN_RigControlSettings, FilteredChannels));
			ControlCategory.AddProperty(FilteredChannelsHandle.ToSharedRef())
			.IsEnabled(bIsEnabled);
		}
	}
	
	if(IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType::ProxyControl) || IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType::AnimationControl))
	{
		ControlCategory.AddProperty(SettingsHandle->GetChildHandle(TEXT("DrivenControls")).ToSharedRef())
		.IsEnabled(bIsEnabled);
	}
}

void FTLLRN_RigControlElementDetails::HandleControlEnumChanged(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectionInfo, const TSharedRef<IPropertyUtilities> PropertyUtilities)
{
	PropertyUtilities->ForceRefresh();
	UEnum* ControlEnum = FindObject<UEnum>(nullptr, **InItem.Get(), false);

	for(int32 ControlIndex = 0; ControlIndex < PerElementInfos.Num(); ControlIndex++)
	{
		FPerElementInfo& Info = PerElementInfos[ControlIndex];
		const FTLLRN_RigControlElement ControlInView = Info.WrapperObject->GetContent<FTLLRN_RigControlElement>();
		FTLLRN_RigControlElement* ControlBeingCustomized = Info.GetDefaultElement<FTLLRN_RigControlElement>();
		
		ControlBeingCustomized->Settings.ControlEnum = ControlEnum;
		if (ControlEnum != nullptr)
		{
			int32 Maximum = (int32)ControlEnum->GetMaxEnumValue() - 1;
			ControlBeingCustomized->Settings.MinimumValue.Set<int32>(0);
			ControlBeingCustomized->Settings.MaximumValue.Set<int32>(Maximum);
			ControlBeingCustomized->Settings.LimitEnabled.Reset();
			ControlBeingCustomized->Settings.LimitEnabled.Add(true);
			Info.GetDefaultHierarchy()->SetControlSettings(ControlBeingCustomized, ControlBeingCustomized->Settings, true, true, true);

			FTLLRN_RigControlValue InitialValue = Info.GetDefaultHierarchy()->GetControlValue(ControlBeingCustomized, ETLLRN_RigControlValueType::Initial);
			FTLLRN_RigControlValue CurrentValue = Info.GetDefaultHierarchy()->GetControlValue(ControlBeingCustomized, ETLLRN_RigControlValueType::Current);

			ControlBeingCustomized->Settings.ApplyLimits(InitialValue);
			ControlBeingCustomized->Settings.ApplyLimits(CurrentValue);
			Info.GetDefaultHierarchy()->SetControlValue(ControlBeingCustomized, InitialValue, ETLLRN_RigControlValueType::Initial, false, false, true);
			Info.GetDefaultHierarchy()->SetControlValue(ControlBeingCustomized, CurrentValue, ETLLRN_RigControlValueType::Current, false, false, true);

			if (UTLLRN_ControlRig* DebuggedRig = Cast<UTLLRN_ControlRig>(Info.GetBlueprint()->GetObjectBeingDebugged()))
			{
				UTLLRN_RigHierarchy* DebuggedHierarchy = DebuggedRig->GetHierarchy();
				if(FTLLRN_RigControlElement* DebuggedControlElement = DebuggedHierarchy->Find<FTLLRN_RigControlElement>(ControlBeingCustomized->GetKey()))
				{
					DebuggedControlElement->Settings.MinimumValue.Set<int32>(0);
					DebuggedControlElement->Settings.MaximumValue.Set<int32>(Maximum);
					DebuggedHierarchy->SetControlSettings(DebuggedControlElement, DebuggedControlElement->Settings, true, true, true);

					DebuggedHierarchy->SetControlValue(DebuggedControlElement, InitialValue, ETLLRN_RigControlValueType::Initial);
					DebuggedHierarchy->SetControlValue(DebuggedControlElement, CurrentValue, ETLLRN_RigControlValueType::Current);
				}
			}
		}

		Info.WrapperObject->SetContent<FTLLRN_RigControlElement>(*ControlBeingCustomized);
	}
}

void FTLLRN_RigControlElementDetails::CustomizeAnimationChannels(IDetailLayoutBuilder& DetailBuilder)
{
	// We only show this section for parents of animation channels
	if(!IsAnyControlNotOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel))
	{
		// If all controls are animation channels, just return 
		return;
	}

	// only show this if only one control is selected
	if(PerElementInfos.Num() != 1)
	{
		return;
	}

	const FTLLRN_RigControlElement* ControlElement = PerElementInfos[0].GetElement<FTLLRN_RigControlElement>();
	if(ControlElement == nullptr)
	{
		return;
	}

	const bool bIsProcedural = IsAnyElementProcedural();
	const bool bIsEnabled = !bIsProcedural;

	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();
	UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("AnimationChannels"), LOCTEXT("AnimationChannels", "Animation Channels"));
	
	const TSharedRef<IPropertyUtilities> PropertyUtilities = DetailBuilder.GetPropertyUtilities();
	
	const TSharedRef<SHorizontalBox> HeaderContentWidget = SNew(SHorizontalBox);
	HeaderContentWidget->AddSlot()
	.HAlign(HAlign_Right)
	[
		SNew(SButton)
		.IsEnabled(bIsEnabled)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(1, 0))
		.OnClicked(this, &FTLLRN_RigControlElementDetails::OnAddAnimationChannelClicked)
		.HAlign(HAlign_Right)
		.ToolTipText(LOCTEXT("AddAnimationChannelToolTip", "Add a new animation channel"))
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(FAppStyle::Get().GetBrush("Icons.PlusCircle"))
			.ColorAndOpacity(FSlateColor::UseForeground())
		]
	];
	Category.HeaderContent(HeaderContentWidget);

	const TArray<FTLLRN_RigControlElement*> AnimationChannels = Hierarchy->GetAnimationChannels(ControlElement, false);
	const bool bHasAnimationChannels = !AnimationChannels.IsEmpty();
	const FTLLRN_RigElementKey ControlElementKey = ControlElement->GetKey();

	for(const FTLLRN_RigControlElement* AssignedAnimationChannel : AnimationChannels)
	{
		const FTLLRN_RigElementKey ChildElementKey = AssignedAnimationChannel->GetKey();
		const bool bIsDirectlyParentedAnimationChannel = HierarchyToChange->GetDefaultParent(ChildElementKey) == ControlElementKey;
		
		const TPair<const FSlateBrush*, FSlateColor> BrushAndColor = STLLRN_RigHierarchyItem::GetBrushForElementType(Hierarchy, ChildElementKey);

		static TArray<TSharedPtr<ETLLRN_RigControlType>> ControlValueTypes;
		if(ControlValueTypes.IsEmpty())
		{
			const UEnum* ValueTypeEnum = StaticEnum<ETLLRN_RigControlType>();
			for(int64 EnumValue = 0; EnumValue < ValueTypeEnum->GetMaxEnumValue(); EnumValue++)
			{
				if(ValueTypeEnum->HasMetaData(TEXT("Hidden"), (int32)EnumValue))
				{
					continue;
				}
				ControlValueTypes.Add(MakeShareable(new ETLLRN_RigControlType((ETLLRN_RigControlType)EnumValue)));
			}
		}

		TSharedPtr<SButton> SelectAnimationChannelButton;
		TSharedPtr<SImage> SelectAnimationChannelImage;
		TSharedPtr<SWidget> NameContent;

		SAssignNew(NameContent, SHorizontalBox)
		.IsEnabled(bIsEnabled)

		+ SHorizontalBox::Slot()
		.MaxWidth(32)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
		[
			SNew(SComboButton)
			.ContentPadding(0)
			.HasDownArrow(false)
			.ButtonContent()
			[
				SNew(SImage)
				.Image(BrushAndColor.Key)
				.ColorAndOpacity(BrushAndColor.Value)
			]
			.MenuContent()
			[
				SNew(SListView<TSharedPtr<ETLLRN_RigControlType>>)
				.ListItemsSource( &ControlValueTypes )
				.OnGenerateRow(this, &FTLLRN_RigControlElementDetails::HandleGenerateAnimationChannelTypeRow, ChildElementKey)
				.OnSelectionChanged(this, &FTLLRN_RigControlElementDetails::HandleControlTypeChanged, ChildElementKey, PropertyUtilities)
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
		[
			SNew(SInlineEditableTextBlock)
			.Font(bIsDirectlyParentedAnimationChannel ? IDetailLayoutBuilder::GetDetailFont() : IDetailLayoutBuilder::GetDetailFontItalic())
			.Text_Lambda([this, ChildElementKey]() -> FText
			{
				return GetDisplayNameForElement(ChildElementKey);
			})
			.OnTextCommitted_Lambda([this, ChildElementKey](const FText& InNewText, ETextCommit::Type InCommitType)
			{
				SetDisplayNameForElement(InNewText, InCommitType, ChildElementKey);
			})
			.OnVerifyTextChanged_Lambda([this, ChildElementKey](const FText& InText, FText& OutErrorMessage) -> bool
			{
				return OnVerifyDisplayNameChanged(InText, OutErrorMessage, ChildElementKey);
			})
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 0.f, 0.f))
		[
			SAssignNew(SelectAnimationChannelButton, SButton)
			.ButtonStyle( FAppStyle::Get(), "NoBorder" )
			.OnClicked_Lambda([this, ChildElementKey]() -> FReply
			{
				return OnSelectElementClicked(ChildElementKey);
			})
			.ContentPadding(0)
			.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "SelectAnimationChannelInHierarchyToolTip", "Select Animation Channel"))
			[
				SAssignNew(SelectAnimationChannelImage, SImage)
				.Image(FAppStyle::GetBrush("Icons.Search"))
			]
		];

		SelectAnimationChannelImage->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this, SelectAnimationChannelButton]() { return FTLLRN_RigElementKeyDetails::OnGetWidgetForeground(SelectAnimationChannelButton); }));

		const FText Label = FText::FromString(FString::Printf(TEXT("Channel%s"), *AssignedAnimationChannel->GetDisplayName().ToString()));
		const TArray<FTLLRN_RigElementKey> ChildElementKeys = {ChildElementKey};
		TAttribute<EVisibility> Visibility = EVisibility::Visible;

		FDetailWidgetRow* WidgetRow = nullptr; 
		switch(AssignedAnimationChannel->Settings.ControlType)
		{
			case ETLLRN_RigControlType::Bool:
			{
				WidgetRow = &CreateBoolValueWidgetRow(ChildElementKeys, Category, Label, FText(), ETLLRN_RigControlValueType::Current, Visibility, NameContent);
				break;
			}
			case ETLLRN_RigControlType::Float:
			case ETLLRN_RigControlType::ScaleFloat:
			{
				WidgetRow = &CreateFloatValueWidgetRow(ChildElementKeys, Category, Label, FText(), ETLLRN_RigControlValueType::Current, Visibility, NameContent);
				break;
			}
			case ETLLRN_RigControlType::Integer:
			{
				if(AssignedAnimationChannel->Settings.ControlEnum)
				{
					WidgetRow = &CreateEnumValueWidgetRow(ChildElementKeys, Category, Label, FText(), ETLLRN_RigControlValueType::Current, Visibility, NameContent);
				}
				else
				{
					WidgetRow = &CreateIntegerValueWidgetRow(ChildElementKeys, Category, Label, FText(), ETLLRN_RigControlValueType::Current, Visibility, NameContent);
				}
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				WidgetRow = &CreateVector2DValueWidgetRow(ChildElementKeys, Category, Label, FText(), ETLLRN_RigControlValueType::Current, Visibility, NameContent);
				break;
			}
			case ETLLRN_RigControlType::Position:
			case ETLLRN_RigControlType::Rotator:
			case ETLLRN_RigControlType::Scale:
			{
				SAdvancedTransformInputBox<FEulerTransform>::FArguments TransformWidgetArgs =
					SAdvancedTransformInputBox<FEulerTransform>::FArguments()
				.DisplayToggle(false)
				.DisplayRelativeWorld(false)
				.Visibility(EVisibility::Visible);

				WidgetRow = &CreateTransformComponentValueWidgetRow(
					AssignedAnimationChannel->Settings.ControlType,
					ChildElementKeys,
					TransformWidgetArgs,
					Category,
					Label,
					FText(),
					GetTransformTypeFromValueType(ETLLRN_RigControlValueType::Current),
					ETLLRN_RigControlValueType::Current,
					NameContent);
				break;
			}
			case ETLLRN_RigControlType::Transform:
			case ETLLRN_RigControlType::EulerTransform:
			{
				SAdvancedTransformInputBox<FEulerTransform>::FArguments TransformWidgetArgs =
					SAdvancedTransformInputBox<FEulerTransform>::FArguments()
				.DisplayToggle(false)
				.DisplayRelativeWorld(false)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Visibility(EVisibility::Visible);

				WidgetRow = &CreateEulerTransformValueWidgetRow(
					ChildElementKeys,
					TransformWidgetArgs,
					Category,
					Label,
					FText(),
					ETLLRN_RigTransformElementDetailsTransform::Current,
					ETLLRN_RigControlValueType::Current,
					NameContent);
				break;
			}
			default:
			{
				WidgetRow = &Category.AddCustomRow(Label)
				.NameContent()
				[
					NameContent.ToSharedRef()
				];
				break;
			}
		}

		if(WidgetRow)
		{
			if(bIsDirectlyParentedAnimationChannel)
			{
				WidgetRow->AddCustomContextMenuAction(FUIAction(
					FExecuteAction::CreateLambda([this, ChildElementKeys, HierarchyToChange]()
					{
						if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
						{
							FScopedTransaction Transaction(LOCTEXT("DeleteAnimationChannels", "Delete Animation Channels"));
							HierarchyToChange->Modify();
							
							for(const FTLLRN_RigElementKey& KeyToRemove : ChildElementKeys)
							{
								Controller->RemoveElement(KeyToRemove, true, true);
							}
						}
					})),
					LOCTEXT("DeleteAnimationChannel", "Delete"),
					LOCTEXT("DeleteAnimationChannelTooltip", "Deletes this animation channel"),
					FSlateIcon());
			}
			else
			{
				WidgetRow->AddCustomContextMenuAction(FUIAction(
					FExecuteAction::CreateLambda([this, ControlElementKey, ChildElementKeys, HierarchyToChange, PropertyUtilities]()
					{
						if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
						{
							FScopedTransaction Transaction(LOCTEXT("RemoveAnimationChannelHosts", "Remove Animation Channel Hosts"));
							HierarchyToChange->Modify();
											
							for(const FTLLRN_RigElementKey& KeyToRemove : ChildElementKeys)
							{
								Controller->RemoveChannelHost(KeyToRemove, ControlElementKey, true, true);
							}
							PropertyUtilities->ForceRefresh();
						}
					})),
					LOCTEXT("RemoveAnimationChannelHost", "Remove from this host"),
					LOCTEXT("RemoveAnimationChannelHostTooltip", "Remove the animation channel from this host"),
					FSlateIcon());
			}
			
			// move up or down
			WidgetRow->AddCustomContextMenuAction(FUIAction(
				FExecuteAction::CreateLambda([this, ControlElementKey, ChildElementKeys, HierarchyToChange]()
				{
					if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
					{
						FScopedTransaction Transaction(LOCTEXT("MoveAnimationChannelUpTransaction", "Move Animation Channel Up"));
						HierarchyToChange->Modify();
						
						for(const FTLLRN_RigElementKey& KeyToMove : ChildElementKeys)
						{
							const int32 LocalIndex = HierarchyToChange->GetLocalIndex(KeyToMove);
							Controller->ReorderElement(KeyToMove, LocalIndex - 1, true);
						}
						Controller->SelectElement(ControlElementKey, true, true);
					}
				})),
				LOCTEXT("MoveAnimationChannelUp", "Move Up"),
				LOCTEXT("MoveAnimationChannelUpTooltip", "Reorders this animation channel to show up one higher"),
				FSlateIcon());
			WidgetRow->AddCustomContextMenuAction(FUIAction(
				FExecuteAction::CreateLambda([this, ControlElementKey, ChildElementKeys, HierarchyToChange]()
				{
					if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
					{
						FScopedTransaction Transaction(LOCTEXT("MoveAnimationChannelDownTransaction", "Move Animation Channel Down"));
						HierarchyToChange->Modify();
						
						for(const FTLLRN_RigElementKey& KeyToMove : ChildElementKeys)
						{
							const int32 LocalIndex = HierarchyToChange->GetLocalIndex(KeyToMove);
							Controller->ReorderElement(KeyToMove, LocalIndex + 1, true);
						}
						Controller->SelectElement(ControlElementKey, true, true);
					}
				})),
				LOCTEXT("MoveAnimationChannelDown", "Move Down"),
				LOCTEXT("MoveAnimationChannelDownTooltip", "Reorders this animation channel to show up one lower"),
				FSlateIcon());
		}
	}

	Category.InitiallyCollapsed(!bHasAnimationChannels);
	if(!bHasAnimationChannels)
	{
		Category.AddCustomRow(FText()).WholeRowContent()
		[
			SNew(SHorizontalBox)
	
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.IsEnabled(bIsEnabled)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(LOCTEXT("NoAnimationChannels", "No animation channels"))
			]
		];
	}
}

void FTLLRN_RigControlElementDetails::CustomizeAvailableSpaces(IDetailLayoutBuilder& DetailBuilder)
{
	// only show this if only one control / animation channel is selected
	if(PerElementInfos.Num() != 1)
	{
		return;
	}

	const FTLLRN_RigControlElement* ControlElement = PerElementInfos[0].GetElement<FTLLRN_RigControlElement>();
	if(ControlElement == nullptr)
	{
		return;
	}

	const bool bIsAnimationChannel = IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel);
	const bool bIsProcedural = IsAnyElementProcedural();
	const bool bIsEnabled = !bIsProcedural;

	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();
	UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();

	static const FText ControlSpaces = LOCTEXT("AvailableSpaces", "Available Spaces"); 
	static const FText ChannelHosts = LOCTEXT("ChannelHosts", "Channel Hosts"); 
	static const FText ControlSpacesToolTip = LOCTEXT("AvailableSpacesToolTip", "Spaces available for this Control"); 
	static const FText ChannelHostsToolTip = LOCTEXT("ChannelHostsToolTip", "A list of controls this channel is listed under"); 
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("MultiParents"), bIsAnimationChannel ? ChannelHosts : ControlSpaces);
	Category.SetToolTip(bIsAnimationChannel ? ChannelHostsToolTip : ControlSpaces);
	
	const TSharedRef<IPropertyUtilities> PropertyUtilities = DetailBuilder.GetPropertyUtilities();

	DisplaySettings.bShowBones = true;
	DisplaySettings.bShowControls = true;
	DisplaySettings.bShowNulls = true;
	DisplaySettings.bShowReferences = false;
	DisplaySettings.bShowSockets = false;
	DisplaySettings.bShowPhysics = false;
	DisplaySettings.bHideParentsOnFilter = true;
	DisplaySettings.bFlattenHierarchyOnFilter = true;
	DisplaySettings.bShowIconColors = true;

	const TSharedRef<SHorizontalBox> HeaderContentWidget = SNew(SHorizontalBox);
	HeaderContentWidget->AddSlot()
	.HAlign(HAlign_Right)
	[
		SAssignNew(AddSpaceMenuAnchor, SMenuAnchor)
		.Placement( MenuPlacement_BelowAnchor )
		.OnGetMenuContent( this, &FTLLRN_RigControlElementDetails::GetAddSpaceContent, PropertyUtilities)
		.Content()
		[
			SNew(SImage)
			.OnMouseButtonDown(this, &FTLLRN_RigControlElementDetails::OnAddSpaceMouseDown, PropertyUtilities)
			.Image(FAppStyle::Get().GetBrush("Icons.PlusCircle"))
			.ColorAndOpacity(FSlateColor::UseForeground())
		]	
	];
	Category.HeaderContent(HeaderContentWidget);

	TArray<FTLLRN_RigElementKey> AvailableSpaces = { Hierarchy->GetDefaultParent(ControlElement->GetKey()) };
	for(const FTLLRN_RigElementKey& AvailableSpace : ControlElement->Settings.Customization.AvailableSpaces)
	{
		AvailableSpaces.AddUnique(AvailableSpace);
	}

	static const FText RemoveSpaceText = LOCTEXT("RemoveSpace", "Remove Space");
	static const FText RemoveChannelHostText = LOCTEXT("RemoveChannelHost", "Remove Channel Host");
	static const FText RemoveSpaceToolTipText = LOCTEXT("RemoveSpaceToolTip", "Removes this space from the list of available spaces");
	static const FText RemoveChannelHostToolTipText = LOCTEXT("RemoveChannelHostToolTip", "Remove the channel from this hosting control");

	for(int32 Index = 0; Index < AvailableSpaces.Num(); Index++)
	{
		const FTLLRN_RigElementKey ControlKey = ControlElement->GetKey();
		const FTLLRN_RigElementKey AvailableSpace = AvailableSpaces[Index];
		const bool bIsParentSpace = Index == 0;
		const TPair<const FSlateBrush*, FSlateColor> BrushAndColor = STLLRN_RigHierarchyItem::GetBrushForElementType(Hierarchy, AvailableSpace);

		TSharedPtr<SButton> SelectSpaceButton, RemoveSpaceButton, MoveSpaceUpButton, MoveSpaceDownButton;
		TSharedPtr<SImage> SelectSpaceImage, RemoveSpaceImage, MoveSpaceUpImage, MoveSpaceDownImage;

		FDetailWidgetRow& WidgetRow = Category.AddCustomRow(FText::FromString(AvailableSpace.ToString()))
		.NameContent()
		.MinDesiredWidth(200.f)
		.MaxDesiredWidth(800.f)
		[
			SNew(SHorizontalBox)
			.IsEnabled(bIsEnabled)

			+ SHorizontalBox::Slot()
			.MaxWidth(32)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
			[
				SNew(SImage)
				.Image(BrushAndColor.Key)
				.ColorAndOpacity(BrushAndColor.Value)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text_Lambda([this, AvailableSpace]() -> FText
				{
					return GetDisplayNameForElement(AvailableSpace);
				})
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 0.f, 0.f))
			[
				SAssignNew(SelectSpaceButton, SButton)
				.ButtonStyle( FAppStyle::Get(), "NoBorder" )
				.OnClicked_Lambda([this, AvailableSpace]() -> FReply
				{
					return OnSelectElementClicked(AvailableSpace);
				})
				.ContentPadding(0)
				.ToolTipText(LOCTEXT("SelectElementInHierarchy", "Select Element in hierarchy"))
				[
					SAssignNew(SelectSpaceImage, SImage)
					.Image(FAppStyle::GetBrush("Icons.Search"))
				]
			]
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			.IsEnabled(bIsEnabled)
			
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(MoveSpaceUpButton, SButton)
				.Visibility((Index > 0 && !bIsAnimationChannel) ? EVisibility::Visible : EVisibility::Collapsed)
				.ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
				.ContentPadding(0)
				.IsEnabled(Index > 1)
				.OnClicked_Lambda([this, ControlKey, AvailableSpace, HierarchyToChange, Index, PropertyUtilities]
				{
					if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
					{
						FScopedTransaction Transaction(LOCTEXT("MoveAvailableSpaceUp", "Move Available Space Up"));
						HierarchyToChange->Modify();
						Controller->SetAvailableSpaceIndex(ControlKey, AvailableSpace, Index - 2);
						PropertyUtilities->ForceRefresh();
					}
					return FReply::Handled();
				})
				.ToolTipText(LOCTEXT("MoveUp", "Move Up"))
				[
					SAssignNew(MoveSpaceUpImage, SImage)
					.Image(FAppStyle::GetBrush("Icons.ChevronUp"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(MoveSpaceDownButton, SButton)
				.Visibility((Index > 0 && !bIsAnimationChannel) ? EVisibility::Visible : EVisibility::Collapsed)
				.ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
				.ContentPadding(0)
				.IsEnabled(Index > 0 && Index < AvailableSpaces.Num() - 1)
				.OnClicked_Lambda([this, ControlKey, AvailableSpace, HierarchyToChange, Index, PropertyUtilities]
				{
					if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
					{
						FScopedTransaction Transaction(LOCTEXT("MoveAvailableSpaceDown", "Move Available Space Down"));
						HierarchyToChange->Modify();
						Controller->SetAvailableSpaceIndex(ControlKey, AvailableSpace, Index);
						PropertyUtilities->ForceRefresh();
					}
					return FReply::Handled();
				})
				.ToolTipText(LOCTEXT("MoveDown", "Move Down"))
				[
					SAssignNew(MoveSpaceDownImage, SImage)
					.Image(FAppStyle::GetBrush("Icons.ChevronDown"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(RemoveSpaceButton, SButton)
				.Visibility(Index > 0 ? EVisibility::Visible : EVisibility::Collapsed)
				.ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
				.ContentPadding(0)
				.IsEnabled(Index > 0)
				.OnClicked_Lambda([this, ControlKey, AvailableSpace, HierarchyToChange, Index, PropertyUtilities]
				{
					if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
					{
						const bool bIsAnimationChannel = IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel);
						FScopedTransaction Transaction(bIsAnimationChannel ? RemoveChannelHostText : RemoveSpaceText);
						HierarchyToChange->Modify();
						if(bIsAnimationChannel)
						{
							Controller->RemoveChannelHost(ControlKey, AvailableSpace);
						}
						else
						{
							Controller->RemoveAvailableSpace(ControlKey, AvailableSpace);
						}
						PropertyUtilities->ForceRefresh();
					}
					return FReply::Handled();
				})
				.ToolTipText(LOCTEXT("Remove", "Remove"))
				[
					SAssignNew(RemoveSpaceImage, SImage)
					.Image(FAppStyle::GetBrush("Icons.Delete"))
				]
			]
		];

		SelectSpaceImage->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this, SelectSpaceButton]() { return FTLLRN_RigElementKeyDetails::OnGetWidgetForeground(SelectSpaceButton); }));
		MoveSpaceUpImage->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this, MoveSpaceUpButton]() { return FTLLRN_RigElementKeyDetails::OnGetWidgetForeground(MoveSpaceUpButton); }));
		MoveSpaceDownImage->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this, MoveSpaceDownButton]() { return FTLLRN_RigElementKeyDetails::OnGetWidgetForeground(MoveSpaceDownButton); }));
		RemoveSpaceImage->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this, RemoveSpaceButton]() { return FTLLRN_RigElementKeyDetails::OnGetWidgetForeground(RemoveSpaceButton); }));

		if(!bIsProcedural)
		{
			if(!bIsAnimationChannel)
			{
				WidgetRow.AddCustomContextMenuAction(FUIAction(
					FExecuteAction::CreateLambda([this, ControlKey, AvailableSpace, HierarchyToChange, Index, PropertyUtilities]()
					{
						if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
						{
							FScopedTransaction Transaction(LOCTEXT("MoveAvailableSpaceUp", "Move Available Space Up"));
							HierarchyToChange->Modify();
							Controller->SetAvailableSpaceIndex(ControlKey, AvailableSpace, Index - 2);
							PropertyUtilities->ForceRefresh();
						}
					}),
					FCanExecuteAction::CreateLambda([Index](){ return Index > 1; })),
					LOCTEXT("MoveUp", "Move Up"),
					LOCTEXT("MoveAvailableSpaceUpToolTip", "Moves this available space up in the list of spaces"),
					FSlateIcon());

				const int32 NumSpaces = AvailableSpaces.Num();
				WidgetRow.AddCustomContextMenuAction(FUIAction(
					FExecuteAction::CreateLambda([this, ControlKey, AvailableSpace, HierarchyToChange, Index, PropertyUtilities]()
					{
						if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
						{
							FScopedTransaction Transaction(LOCTEXT("MoveAvailableSpaceDown", "Move Available Space Down"));
							HierarchyToChange->Modify();
							Controller->SetAvailableSpaceIndex(ControlKey, AvailableSpace, Index);
							PropertyUtilities->ForceRefresh();
						}
					}),
					FCanExecuteAction::CreateLambda([Index, NumSpaces](){ return Index > 0 && Index < NumSpaces - 1; })),
					LOCTEXT("MoveDown", "Move Down"),
					LOCTEXT("MoveAvailableSpaceDownToolTip", "Moves this available space down in the list of spaces"),
					FSlateIcon());
			}

			WidgetRow.AddCustomContextMenuAction(FUIAction(
				FExecuteAction::CreateLambda([this, ControlKey, AvailableSpace, HierarchyToChange, PropertyUtilities]()
				{
					if(UTLLRN_RigHierarchyController* Controller = HierarchyToChange->GetController(true))
					{
						const bool bIsAnimationChannel = IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel);
						FScopedTransaction Transaction(bIsAnimationChannel ? RemoveChannelHostText : RemoveSpaceText);
						HierarchyToChange->Modify();
						if(bIsAnimationChannel)
						{
							Controller->RemoveChannelHost(ControlKey, AvailableSpace);
						}
						else
						{
							Controller->RemoveAvailableSpace(ControlKey, AvailableSpace);
						}
						PropertyUtilities->ForceRefresh();
					}
				}),
				FCanExecuteAction::CreateLambda([bIsParentSpace](){ return !bIsParentSpace; })),
				bIsAnimationChannel ? RemoveChannelHostText : RemoveSpaceText,
				bIsAnimationChannel ? RemoveChannelHostToolTipText : RemoveSpaceToolTipText,
				FSlateIcon());
		}
	}

	Category.InitiallyCollapsed(AvailableSpaces.Num() < 2);
	if(AvailableSpaces.IsEmpty())
	{
		static const FText NoSpacesText = LOCTEXT("NoSpacesSet", "No Available Spaces set");
		static const FText NoChannelHostsText = LOCTEXT("NoChannelHostsSet", "No Channel Hosts set");

		Category.AddCustomRow(FText()).WholeRowContent()
		[
			SNew(SHorizontalBox)
	
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.IsEnabled(bIsEnabled)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(bIsAnimationChannel ? NoChannelHostsText : NoSpacesText)
			]
		];
	}
}

FReply FTLLRN_RigControlElementDetails::OnAddAnimationChannelClicked()
{
	if(IsAnyElementNotOfType(ETLLRN_RigElementType::Control) || IsAnyElementProcedural())
	{
		return FReply::Handled();
	}

	const FTLLRN_RigElementKey Key = PerElementInfos[0].GetElement()->GetKey();
	UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();

	static const FName ChannelName = TEXT("Channel");
	FTLLRN_RigControlSettings Settings;
	Settings.AnimationType = ETLLRN_RigControlAnimationType::AnimationChannel;
	Settings.ControlType = ETLLRN_RigControlType::Float;
	Settings.MinimumValue = FTLLRN_RigControlValue::Make<float>(0.f);
	Settings.MaximumValue = FTLLRN_RigControlValue::Make<float>(1.f);
	Settings.DisplayName = HierarchyToChange->GetSafeNewDisplayName(Key, ChannelName);
	HierarchyToChange->GetController(true)->AddAnimationChannel(ChannelName, Key, Settings, true, true);
	HierarchyToChange->GetController(true)->SelectElement(Key);
	return FReply::Handled();
}

TSharedRef<ITableRow> FTLLRN_RigControlElementDetails::HandleGenerateAnimationChannelTypeRow(TSharedPtr<ETLLRN_RigControlType> ControlType, const TSharedRef<STableViewBase>& OwnerTable, FTLLRN_RigElementKey ControlKey)
{
	const UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();

	TPair<const FSlateBrush*, FSlateColor> BrushAndColor = STLLRN_RigHierarchyItem::GetBrushForElementType(HierarchyToChange, ControlKey);
	BrushAndColor.Value = STLLRN_RigHierarchyItem::GetColorForControlType(*ControlType.Get(), nullptr);

	return SNew(STableRow<TSharedPtr<ETLLRN_RigControlType>>, OwnerTable)
	.Content()
	[
		SNew(SHorizontalBox)
	
		+ SHorizontalBox::Slot()
		.MaxWidth(18)
		.FillWidth(1.0)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
		[
			SNew(SImage)
			.Image(BrushAndColor.Key)
			.ColorAndOpacity(BrushAndColor.Value)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(StaticEnum<ETLLRN_RigControlType>()->GetDisplayNameTextByValue((int64)*ControlType.Get()))
		]
	];
}

TSharedRef<SWidget> FTLLRN_RigControlElementDetails::GetAddSpaceContent(const TSharedRef<IPropertyUtilities> PropertyUtilities)
{
	if(PerElementInfos.IsEmpty())
	{
		return SNullWidget::NullWidget;
	}

	FRigTreeDelegates RigTreeDelegates;
	RigTreeDelegates.OnGetHierarchy = FOnGetRigTreeHierarchy::CreateLambda([this]()
	{
		return PerElementInfos[0].GetHierarchy();
	});
	RigTreeDelegates.OnGetDisplaySettings = FOnGetRigTreeDisplaySettings::CreateSP(this, &FTLLRN_RigControlElementDetails::GetDisplaySettings);
	RigTreeDelegates.OnGetSelection = FOnRigTreeGetSelection::CreateLambda([]() -> TArray<FTLLRN_RigElementKey> { return {}; });
	RigTreeDelegates.OnSelectionChanged = FOnRigTreeSelectionChanged::CreateSP(this, &FTLLRN_RigControlElementDetails::OnAddSpaceSelection, PropertyUtilities);

	return SNew(SBox)
		.Padding(2.5)
		.MinDesiredWidth(200)
		.MinDesiredHeight(300)
		[
			SNew(STLLRN_RigHierarchyTreeView)
			.RigTreeDelegates(RigTreeDelegates)
			.PopulateOnConstruct(true)
		];
}

FReply FTLLRN_RigControlElementDetails::OnAddSpaceMouseDown(const FGeometry& InGeometry, const FPointerEvent& InPointerEvent, const TSharedRef<IPropertyUtilities> PropertyUtilities)
{
	AddSpaceMenuAnchor->SetIsOpen(true);
	return FReply::Handled();
}

void FTLLRN_RigControlElementDetails::OnAddSpaceSelection(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo, const TSharedRef<IPropertyUtilities> PropertyUtilities)
{
	if(Selection)
	{
		const FTLLRN_RigElementKey ChildKey = PerElementInfos[0].GetElement()->GetKey();
		const FTLLRN_RigElementKey NewParentKey = Selection->Key;
		UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();
		AddSpaceMenuAnchor->SetIsOpen(false);

		const bool bIsAnimationChannel = IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel);
		static const FText AddSpaceText = LOCTEXT("AddSpace", "Add Space");
		static const FText AddChannelHostText = LOCTEXT("AddChannelHost", "Add Channel Host");
		FScopedTransaction Transaction(bIsAnimationChannel ? AddChannelHostText : AddSpaceText);
		HierarchyToChange->Modify();
		if(bIsAnimationChannel)
		{
			HierarchyToChange->GetController(true)->AddChannelHost(ChildKey, NewParentKey);
		}
		else
		{
			HierarchyToChange->GetController(true)->AddAvailableSpace(ChildKey, NewParentKey);
		}
		PropertyUtilities->ForceRefresh();
	}
}

void FTLLRN_RigControlElementDetails::HandleControlTypeChanged(TSharedPtr<ETLLRN_RigControlType> ControlType, ESelectInfo::Type SelectInfo, FTLLRN_RigElementKey ControlKey, const TSharedRef<IPropertyUtilities> PropertyUtilities)
{
	HandleControlTypeChanged(*ControlType.Get(), {ControlKey}, PropertyUtilities);
}

void FTLLRN_RigControlElementDetails::HandleControlTypeChanged(ETLLRN_RigControlType ControlType, TArray<FTLLRN_RigElementKey> ControlKeys, const TSharedRef<IPropertyUtilities> PropertyUtilities)
{
	if(PerElementInfos.IsEmpty())
	{
		return;
	}
	
	if(ControlKeys.IsEmpty())
	{
		for(const FPerElementInfo& Info : PerElementInfos)
		{
			ControlKeys.Add(Info.GetDefaultElement<FTLLRN_RigControlElement>()->GetKey());
		}
	}

	for(const FTLLRN_RigElementKey& ControlKey : ControlKeys)
	{
		UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();
		UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();
		HierarchyToChange->Modify();
		
		FTLLRN_RigControlElement* ControlElement = HierarchyToChange->FindChecked<FTLLRN_RigControlElement>(ControlKey);
		
		FTLLRN_RigControlValue ValueToSet;

		ControlElement->Settings.ControlType = ControlType;
		ControlElement->Settings.LimitEnabled.Reset();
		ControlElement->Settings.bGroupWithParentControl = false;
		ControlElement->Settings.FilteredChannels.Reset();

		switch (ControlElement->Settings.ControlType)
		{
			case ETLLRN_RigControlType::Bool:
			{
				ControlElement->Settings.AnimationType = ETLLRN_RigControlAnimationType::AnimationChannel;
				ValueToSet = FTLLRN_RigControlValue::Make<bool>(false);
				ControlElement->Settings.bGroupWithParentControl = ControlElement->Settings.IsAnimatable();
				break;
			}
			case ETLLRN_RigControlType::Float:
			{
				ValueToSet = FTLLRN_RigControlValue::Make<float>(0.f);
				ControlElement->Settings.SetupLimitArrayForType(true);
				ControlElement->Settings.MinimumValue = FTLLRN_RigControlValue::Make<float>(0.f);
				ControlElement->Settings.MaximumValue = FTLLRN_RigControlValue::Make<float>(100.f);
				ControlElement->Settings.bGroupWithParentControl = ControlElement->Settings.IsAnimatable();
				break;
			}
			case ETLLRN_RigControlType::ScaleFloat:
			{
				ValueToSet = FTLLRN_RigControlValue::Make<float>(1.f);
				ControlElement->Settings.SetupLimitArrayForType(false);
				ControlElement->Settings.MinimumValue = FTLLRN_RigControlValue::Make<float>(0.f);
				ControlElement->Settings.MaximumValue = FTLLRN_RigControlValue::Make<float>(10.f);
				ControlElement->Settings.bGroupWithParentControl = ControlElement->Settings.IsAnimatable();
				break;
			}
			case ETLLRN_RigControlType::Integer:
			{
				ValueToSet = FTLLRN_RigControlValue::Make<int32>(0);
				ControlElement->Settings.SetupLimitArrayForType(true);
				ControlElement->Settings.MinimumValue = FTLLRN_RigControlValue::Make<int32>(0);
				ControlElement->Settings.MaximumValue = FTLLRN_RigControlValue::Make<int32>(100);
				ControlElement->Settings.bGroupWithParentControl = ControlElement->Settings.IsAnimatable();
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				ValueToSet = FTLLRN_RigControlValue::Make<FVector2D>(FVector2D::ZeroVector);
				ControlElement->Settings.SetupLimitArrayForType(true);
				ControlElement->Settings.MinimumValue = FTLLRN_RigControlValue::Make<FVector2D>(FVector2D::ZeroVector);
				ControlElement->Settings.MaximumValue = FTLLRN_RigControlValue::Make<FVector2D>(FVector2D(100.f, 100.f));
				ControlElement->Settings.bGroupWithParentControl = ControlElement->Settings.IsAnimatable();
				break;
			}
			case ETLLRN_RigControlType::Position:
			{
				ValueToSet = FTLLRN_RigControlValue::Make<FVector>(FVector::ZeroVector);
				ControlElement->Settings.SetupLimitArrayForType(false);
				ControlElement->Settings.MinimumValue = FTLLRN_RigControlValue::Make<FVector>(-FVector::OneVector);
				ControlElement->Settings.MaximumValue = FTLLRN_RigControlValue::Make<FVector>(FVector::OneVector);
				break;
			}
			case ETLLRN_RigControlType::Scale:
			{
				ValueToSet = FTLLRN_RigControlValue::Make<FVector>(FVector::OneVector);
				ControlElement->Settings.SetupLimitArrayForType(false);
				ControlElement->Settings.MinimumValue = FTLLRN_RigControlValue::Make<FVector>(FVector::ZeroVector);
				ControlElement->Settings.MaximumValue = FTLLRN_RigControlValue::Make<FVector>(FVector::OneVector);
				break;
			}
			case ETLLRN_RigControlType::Rotator:
			{
				ValueToSet = FTLLRN_RigControlValue::Make<FRotator>(FRotator::ZeroRotator);
				ControlElement->Settings.SetupLimitArrayForType(false, false);
				ControlElement->Settings.MinimumValue = FTLLRN_RigControlValue::Make<FRotator>(FRotator::ZeroRotator);
				ControlElement->Settings.MaximumValue = FTLLRN_RigControlValue::Make<FRotator>(FRotator(180.f, 180.f, 180.f));
				break;
			}
			case ETLLRN_RigControlType::Transform:
			{
				ValueToSet = FTLLRN_RigControlValue::Make<FTransform>(FTransform::Identity);
				ControlElement->Settings.SetupLimitArrayForType(false, false, false);
				ControlElement->Settings.MinimumValue = ValueToSet;
				ControlElement->Settings.MaximumValue = ValueToSet;
				break;
			}
			case ETLLRN_RigControlType::TransformNoScale:
			{
				FTransformNoScale Identity = FTransform::Identity;
				ValueToSet = FTLLRN_RigControlValue::Make<FTransformNoScale>(Identity);
				ControlElement->Settings.SetupLimitArrayForType(false, false, false);
				ControlElement->Settings.MinimumValue = ValueToSet;
				ControlElement->Settings.MaximumValue = ValueToSet;
				break;
			}
			case ETLLRN_RigControlType::EulerTransform:
			{
				FEulerTransform Identity = FEulerTransform::Identity;
				ValueToSet = FTLLRN_RigControlValue::Make<FEulerTransform>(Identity);
				ControlElement->Settings.SetupLimitArrayForType(false, false, false);
				ControlElement->Settings.MinimumValue = ValueToSet;
				ControlElement->Settings.MaximumValue = ValueToSet;
				break;
			}
			default:
			{
				ensure(false);
				break;
			}
		}

		HierarchyToChange->SetControlSettings(ControlElement, ControlElement->Settings, true, true, true);
		HierarchyToChange->SetControlValue(ControlElement, ValueToSet, ETLLRN_RigControlValueType::Initial, true, false, true);
		HierarchyToChange->SetControlValue(ControlElement, ValueToSet, ETLLRN_RigControlValueType::Current, true, false, true);

		for(const FPerElementInfo& Info : PerElementInfos)
		{
			if(Info.Element.Get()->GetKey() == ControlKey)
			{
				Info.WrapperObject->SetContent<FTLLRN_RigControlElement>(*ControlElement);
			}
		}

		if (HierarchyToChange != Hierarchy)
		{
			if(FTLLRN_RigControlElement* OtherControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(ControlKey))
			{
				OtherControlElement->Settings = ControlElement->Settings;
				Hierarchy->SetControlSettings(OtherControlElement, OtherControlElement->Settings, true, true, true);
				Hierarchy->SetControlValue(OtherControlElement, ValueToSet, ETLLRN_RigControlValueType::Initial, true);
				Hierarchy->SetControlValue(OtherControlElement, ValueToSet, ETLLRN_RigControlValueType::Current, true);
			}
		}
		else
		{
			PerElementInfos[0].GetBlueprint()->PropagateHierarchyFromBPToInstances();
		}
	}
	
	PropertyUtilities->ForceRefresh();
}

void FTLLRN_RigControlElementDetails::CustomizeShape(IDetailLayoutBuilder& DetailBuilder)
{
	if(PerElementInfos.IsEmpty())
	{
		return;
	}

	if(ContainsElementByPredicate([](const FPerElementInfo& Info)
	{
		if(const FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>())
		{
			return !ControlElement->Settings.SupportsShape();
		}
		return true;
	}))
	{
		return;
	}

	const bool bIsProcedural = IsAnyElementProcedural();
	const bool bIsEnabled = !bIsProcedural;

	ShapeNameList.Reset();
	
	if (UTLLRN_ControlRigBlueprint* Blueprint = PerElementInfos[0].GetBlueprint())
	{
		if(UEdGraph* RootEdGraph = Blueprint->GetEdGraph(Blueprint->GetModel()))
		{
			if(UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(RootEdGraph))
			{
				UTLLRN_RigHierarchy* Hierarchy = Blueprint->Hierarchy;
				if(UTLLRN_ControlRig* RigBeingDebugged = Cast<UTLLRN_ControlRig>(Blueprint->GetObjectBeingDebugged()))
				{
					Hierarchy = RigBeingDebugged->GetHierarchy();
				}

				const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>* ShapeLibraries = &Blueprint->ShapeLibraries;
				if(const UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Hierarchy->GetTypedOuter<UTLLRN_ControlRig>())
				{
					ShapeLibraries = &DebuggedTLLRN_ControlRig->GetShapeLibraries();
				}
				RigGraph->CacheNameLists(Hierarchy, &Blueprint->DrawContainer, *ShapeLibraries);
				
				if(const TArray<TSharedPtr<FRigVMStringWithTag>>* GraphShapeNameListPtr = RigGraph->GetShapeNameList())
				{
					ShapeNameList = *GraphShapeNameListPtr;
				}
			}
		}

		if(ShapeNameList.IsEmpty())
		{
			const bool bUseNameSpace = Blueprint->ShapeLibraries.Num() > 1;
			for(TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>& ShapeLibrary : Blueprint->ShapeLibraries)
			{
				if (!ShapeLibrary.IsValid())
				{
					ShapeLibrary.LoadSynchronous();
				}
				if (ShapeLibrary.IsValid())
				{
					const FString NameSpace = bUseNameSpace ? ShapeLibrary->GetName() + TEXT(".") : FString();
					ShapeNameList.Add(MakeShared<FRigVMStringWithTag>(NameSpace + ShapeLibrary->DefaultShape.ShapeName.ToString()));
					for (const FTLLRN_ControlRigShapeDefinition& Shape : ShapeLibrary->Shapes)
					{
						ShapeNameList.Add(MakeShared<FRigVMStringWithTag>(NameSpace + Shape.ShapeName.ToString()));
					}
				}
			}
		}
	}

	IDetailCategoryBuilder& ShapeCategory = DetailBuilder.EditCategory(TEXT("Shape"), LOCTEXT("Shape", "Shape"));

	const TSharedPtr<IPropertyHandle> SettingsHandle = DetailBuilder.GetProperty(TEXT("Settings"));

	if(!IsAnyControlNotOfAnimationType(ETLLRN_RigControlAnimationType::ProxyControl))
	{
		ShapeCategory.AddProperty(SettingsHandle->GetChildHandle(TEXT("ShapeVisibility")).ToSharedRef())
		.IsEnabled(bIsEnabled)
		.DisplayName(FText::FromString(TEXT("Visibility Mode")));
	}

	ShapeCategory.AddProperty(SettingsHandle->GetChildHandle(TEXT("bShapeVisible")).ToSharedRef())
	.IsEnabled(bIsEnabled)
	.DisplayName(FText::FromString(TEXT("Visible")));

	IDetailGroup& ShapePropertiesGroup = ShapeCategory.AddGroup(TEXT("Shape Properties"), LOCTEXT("ShapeProperties", "Shape Properties"));
	ShapePropertiesGroup.HeaderRow()
	.IsEnabled(bIsEnabled)
	.NameContent()
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(LOCTEXT("ShapeProperties", "Shape Properties"))
		.ToolTipText(LOCTEXT("ShapePropertiesTooltip", "Customize the properties of the shape"))
	]
	.CopyAction(FUIAction(
		FExecuteAction::CreateSP(this, &FTLLRN_RigControlElementDetails::OnCopyShapeProperties)))
	.PasteAction(FUIAction(
		FExecuteAction::CreateSP(this, &FTLLRN_RigControlElementDetails::OnPasteShapeProperties),
		FCanExecuteAction::CreateLambda([bIsEnabled]() { return bIsEnabled; })));
	
	// setup shape transform
	SAdvancedTransformInputBox<FEulerTransform>::FArguments TransformWidgetArgs = SAdvancedTransformInputBox<FEulerTransform>::FArguments()
	.IsEnabled(bIsEnabled)
	.DisplayToggle(false)
	.DisplayRelativeWorld(false)
	.Font(IDetailLayoutBuilder::GetDetailFont());

	TArray<FTLLRN_RigElementKey> Keys = GetElementKeys();
	Keys = PerElementInfos[0].GetHierarchy()->SortKeys(Keys);

	auto GetShapeTransform = [this](
		const FTLLRN_RigElementKey& Key
		) -> FEulerTransform
	{
		if(const FPerElementInfo& Info = FindElement(Key))
		{
			if(FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>())
			{
				return FEulerTransform(Info.GetHierarchy()->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::InitialLocal));
			}
		}
		return FEulerTransform::Identity;
	};

	auto SetShapeTransform = [this](
		const FTLLRN_RigElementKey& Key,
		const FEulerTransform& InTransform,
		bool bSetupUndo
		)
	{
		if(const FPerElementInfo& Info = FindElement(Key))
		{
			if(const FTLLRN_RigControlElement* ControlElement = Info.GetDefaultElement<FTLLRN_RigControlElement>())
			{
				Info.GetDefaultHierarchy()->SetControlShapeTransform((FTLLRN_RigControlElement*)ControlElement, InTransform.ToFTransform(), ETLLRN_RigTransformType::InitialLocal, bSetupUndo, true, bSetupUndo);
			}
		}
	};

	TransformWidgetArgs.OnGetNumericValue_Lambda([Keys, GetShapeTransform](
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent) -> TOptional<FVector::FReal>
	{
		TOptional<FVector::FReal> FirstValue;

		for(int32 Index = 0; Index < Keys.Num(); Index++)
		{
			const FTLLRN_RigElementKey& Key = Keys[Index];
			FEulerTransform Xfo = GetShapeTransform(Key);

			TOptional<FVector::FReal> CurrentValue = SAdvancedTransformInputBox<FEulerTransform>::GetNumericValueFromTransform(Xfo, Component, Representation, SubComponent);
			if(!CurrentValue.IsSet())
			{
				return CurrentValue;
			}

			if(Index == 0)
			{
				FirstValue = CurrentValue;
			}
			else
			{
				if(!FMath::IsNearlyEqual(FirstValue.GetValue(), CurrentValue.GetValue()))
				{
					return TOptional<FVector::FReal>();
				}
			}
		}
		
		return FirstValue;
	});

	TransformWidgetArgs.OnNumericValueChanged_Lambda(
	[
		Keys,
		this,
		GetShapeTransform,
		SetShapeTransform
	](
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		FVector::FReal InNumericValue)
	{
		for(const FTLLRN_RigElementKey& Key : Keys)
		{
			FEulerTransform Transform = GetShapeTransform(Key);
			FEulerTransform PreviousTransform = Transform;
			SAdvancedTransformInputBox<FEulerTransform>::ApplyNumericValueChange(Transform, InNumericValue, Component, Representation, SubComponent);

			if(!FTLLRN_RigControlElementDetails::Equals(Transform, PreviousTransform))
			{
				if(!SliderTransaction.IsValid())
				{
					SliderTransaction = MakeShareable(new FScopedTransaction(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ChangeNumericValue", "Change Numeric Value")));
					PerElementInfos[0].GetDefaultHierarchy()->Modify();
				}
				SetShapeTransform(Key, Transform, false);
			}
		}
	});

	TransformWidgetArgs.OnNumericValueCommitted_Lambda(
	[
		Keys,
		this,
		GetShapeTransform,
		SetShapeTransform
	](
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		FVector::FReal InNumericValue,
		ETextCommit::Type InCommitType)
	{
		{
			FScopedTransaction Transaction(LOCTEXT("ChangeNumericValue", "Change Numeric Value"));
			PerElementInfos[0].GetDefaultHierarchy()->Modify();

			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				FEulerTransform Transform = GetShapeTransform(Key);
				FEulerTransform PreviousTransform = Transform;
				SAdvancedTransformInputBox<FEulerTransform>::ApplyNumericValueChange(Transform, InNumericValue, Component, Representation, SubComponent);
				if(!FTLLRN_RigControlElementDetails::Equals(Transform, PreviousTransform))
				{
					SetShapeTransform(Key, Transform, true);
				}
			}
		}
		SliderTransaction.Reset();
	});

	TransformWidgetArgs.OnCopyToClipboard_Lambda([Keys, GetShapeTransform](
		ESlateTransformComponent::Type InComponent
		)
	{
		if(Keys.Num() == 0)
		{
			return;
		}

		const FTLLRN_RigElementKey& FirstKey = Keys[0];
		FEulerTransform Xfo = GetShapeTransform(FirstKey);

		FString Content;
		switch(InComponent)
		{
			case ESlateTransformComponent::Location:
			{
				const FVector Data = Xfo.GetLocation();
				TBaseStructure<FVector>::Get()->ExportText(Content, &Data, &Data, nullptr, PPF_None, nullptr);
				break;
			}
			case ESlateTransformComponent::Rotation:
			{
				const FRotator Data = Xfo.Rotator();
				TBaseStructure<FRotator>::Get()->ExportText(Content, &Data, &Data, nullptr, PPF_None, nullptr);
				break;
			}
			case ESlateTransformComponent::Scale:
			{
				const FVector Data = Xfo.GetScale3D();
				TBaseStructure<FVector>::Get()->ExportText(Content, &Data, &Data, nullptr, PPF_None, nullptr);
				break;
			}
			case ESlateTransformComponent::Max:
			default:
			{
				TBaseStructure<FEulerTransform>::Get()->ExportText(Content, &Xfo, &Xfo, nullptr, PPF_None, nullptr);
				break;
			}
		}

		if(!Content.IsEmpty())
		{
			FPlatformApplicationMisc::ClipboardCopy(*Content);
		}
	});

	TransformWidgetArgs.OnPasteFromClipboard_Lambda([Keys, GetShapeTransform, SetShapeTransform, this](
		ESlateTransformComponent::Type InComponent
		)
	{
		if(Keys.Num() == 0)
		{
			return;
		}

		FString Content;
		FPlatformApplicationMisc::ClipboardPaste(Content);

		if(Content.IsEmpty())
		{
			return;
		}

		FScopedTransaction Transaction(LOCTEXT("PasteTransform", "Paste Transform"));
		PerElementInfos[0].GetDefaultHierarchy()->Modify();

		for(const FTLLRN_RigElementKey& Key : Keys)
		{
			FEulerTransform Xfo = GetShapeTransform(Key);
			{
				class FRigPasteTransformWidgetErrorPipe : public FOutputDevice
				{
				public:

					int32 NumErrors;

					FRigPasteTransformWidgetErrorPipe()
						: FOutputDevice()
						, NumErrors(0)
					{
					}

					virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
					{
						UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Error Pasting to Widget: %s"), V);
						NumErrors++;
					}
				};

				FRigPasteTransformWidgetErrorPipe ErrorPipe;
				
				switch(InComponent)
				{
					case ESlateTransformComponent::Location:
					{
						FVector Data = Xfo.GetLocation();
						TBaseStructure<FVector>::Get()->ImportText(*Content, &Data, nullptr, PPF_None, &ErrorPipe, TBaseStructure<FVector>::Get()->GetName(), true);
						Xfo.SetLocation(Data);
						break;
					}
					case ESlateTransformComponent::Rotation:
					{
						FRotator Data = Xfo.Rotator();
						TBaseStructure<FRotator>::Get()->ImportText(*Content, &Data, nullptr, PPF_None, &ErrorPipe, TBaseStructure<FRotator>::Get()->GetName(), true);
						Xfo.SetRotator(Data);
						break;
					}
					case ESlateTransformComponent::Scale:
					{
						FVector Data = Xfo.GetScale3D();
						TBaseStructure<FVector>::Get()->ImportText(*Content, &Data, nullptr, PPF_None, &ErrorPipe, TBaseStructure<FVector>::Get()->GetName(), true);
						Xfo.SetScale3D(Data);
						break;
					}
					case ESlateTransformComponent::Max:
					default:
					{
						TBaseStructure<FEulerTransform>::Get()->ImportText(*Content, &Xfo, nullptr, PPF_None, &ErrorPipe, TBaseStructure<FEulerTransform>::Get()->GetName(), true);
						break;
					}
				}

				if(ErrorPipe.NumErrors == 0)
				{
					SetShapeTransform(Key, Xfo, true);
				}
			}
		}
	});

	TransformWidgetArgs.DiffersFromDefault_Lambda([
		Keys,
		GetShapeTransform
	](
		ESlateTransformComponent::Type InComponent) -> bool
	{
		for(const FTLLRN_RigElementKey& Key : Keys)
		{
			const FEulerTransform CurrentTransform = GetShapeTransform(Key);
			static const FEulerTransform DefaultTransform = FEulerTransform::Identity;

			switch(InComponent)
			{
				case ESlateTransformComponent::Location:
				{
					if(!DefaultTransform.GetLocation().Equals(CurrentTransform.GetLocation()))
					{
						return true;
					}
					break;
				}
				case ESlateTransformComponent::Rotation:
				{
					if(!DefaultTransform.Rotator().Equals(CurrentTransform.Rotator()))
					{
						return true;
					}
					break;
				}
				case ESlateTransformComponent::Scale:
				{
					if(!DefaultTransform.GetScale3D().Equals(CurrentTransform.GetScale3D()))
					{
						return true;
					}
					break;
				}
				default: // also no component whole transform
				{
					if(!DefaultTransform.GetLocation().Equals(CurrentTransform.GetLocation()) ||
						!DefaultTransform.Rotator().Equals(CurrentTransform.Rotator()) ||
						!DefaultTransform.GetScale3D().Equals(CurrentTransform.GetScale3D()))
					{
						return true;
					}
					break;
				}
			}
		}
		return false;
	});

	TransformWidgetArgs.OnResetToDefault_Lambda([Keys, GetShapeTransform, SetShapeTransform, this](
		ESlateTransformComponent::Type InComponent)
	{
		FScopedTransaction Transaction(LOCTEXT("ResetTransformToDefault", "Reset Transform to Default"));
		PerElementInfos[0].GetDefaultHierarchy()->Modify();

		for(const FTLLRN_RigElementKey& Key : Keys)
		{
			FEulerTransform CurrentTransform = GetShapeTransform(Key);
			static const FEulerTransform DefaultTransform = FEulerTransform::Identity; 

			switch(InComponent)
			{
				case ESlateTransformComponent::Location:
				{
					CurrentTransform.SetLocation(DefaultTransform.GetLocation());
					break;
				}
				case ESlateTransformComponent::Rotation:
				{
					CurrentTransform.SetRotator(DefaultTransform.Rotator());
					break;
				}
				case ESlateTransformComponent::Scale:
				{
					CurrentTransform.SetScale3D(DefaultTransform.GetScale3D());
					break;
				}
				default: // whole transform / max component
				{
					CurrentTransform = DefaultTransform;
					break;
				}
			}

			SetShapeTransform(Key, CurrentTransform, true);
		}
	});

	TArray<FTLLRN_RigControlElement*> ControlElements;
	Algo::Transform(PerElementInfos, ControlElements, [](const FPerElementInfo& Info)
	{
		return Info.GetElement<FTLLRN_RigControlElement>();
	});

	SAdvancedTransformInputBox<FEulerTransform>::ConstructGroupedTransformRows(
		ShapeCategory, 
		LOCTEXT("ShapeTransform", "Shape Transform"), 
		LOCTEXT("ShapeTransformTooltip", "The relative transform of the shape under the control"),
		TransformWidgetArgs);

	ShapeNameHandle = SettingsHandle->GetChildHandle(TEXT("ShapeName"));
	ShapePropertiesGroup.AddPropertyRow(ShapeNameHandle.ToSharedRef()).CustomWidget()
	.NameContent()
	[
		SNew(STextBlock)
		.IsEnabled(bIsEnabled)
		.Text(FText::FromString(TEXT("Shape")))
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.IsEnabled(this, &FTLLRN_RigControlElementDetails::IsShapeEnabled)
	]
	.ValueContent()
	[
		SAssignNew(ShapeNameListWidget, STLLRN_ControlRigShapeNameList, ControlElements, PerElementInfos[0].GetBlueprint())
		.OnGetNameListContent(this, &FTLLRN_RigControlElementDetails::GetShapeNameList)
		.IsEnabled(this, &FTLLRN_RigControlElementDetails::IsShapeEnabled)
	];

	ShapeColorHandle = SettingsHandle->GetChildHandle(TEXT("ShapeColor"));
	ShapePropertiesGroup.AddPropertyRow(ShapeColorHandle.ToSharedRef())
	.IsEnabled(bIsEnabled)
	.DisplayName(FText::FromString(TEXT("Color")));
}

void FTLLRN_RigControlElementDetails::BeginDestroy()
{
	FTLLRN_RigTransformElementDetails::BeginDestroy();

	if(ShapeNameListWidget.IsValid())
	{
		ShapeNameListWidget->BeginDestroy();
	}
}

void FTLLRN_RigControlElementDetails::RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule, UClass* InClass)
{
	FTLLRN_RigTransformElementDetails::RegisterSectionMappings(PropertyEditorModule, InClass);

	TSharedRef<FPropertySection> ControlSection = PropertyEditorModule.FindOrCreateSection(InClass->GetFName(), "Control", LOCTEXT("Control", "Control"));
	ControlSection->AddCategory("General");
	ControlSection->AddCategory("Control");
	ControlSection->AddCategory("Value");
	ControlSection->AddCategory("AnimationChannels");

	TSharedRef<FPropertySection> ShapeSection = PropertyEditorModule.FindOrCreateSection(InClass->GetFName(), "Shape", LOCTEXT("Shape", "Shape"));
	ShapeSection->AddCategory("General");
	ShapeSection->AddCategory("Shape");

	TSharedRef<FPropertySection> ChannelsSection = PropertyEditorModule.FindOrCreateSection(InClass->GetFName(), "Channels", LOCTEXT("Channels", "Channels"));
	ChannelsSection->AddCategory("AnimationChannels");
}

bool FTLLRN_RigControlElementDetails::IsShapeEnabled() const
{
	if(IsAnyElementProcedural())
	{
		 return false;
	}
	
	return ContainsElementByPredicate([](const FPerElementInfo& Info)
	{
		if(const FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>())
		{
			return ControlElement->Settings.SupportsShape();
		}
		return false;
	});
}

const TArray<TSharedPtr<FRigVMStringWithTag>>& FTLLRN_RigControlElementDetails::GetShapeNameList() const
{
	return ShapeNameList;
}

FText FTLLRN_RigControlElementDetails::GetDisplayName() const
{
	FName DisplayName(NAME_None);

	for(int32 ObjectIndex = 0; ObjectIndex < PerElementInfos.Num(); ObjectIndex++)
	{
		const FPerElementInfo& Info = PerElementInfos[ObjectIndex];
		if(const FTLLRN_RigControlElement* ControlElement = Info.GetDefaultElement<FTLLRN_RigControlElement>())
		{
 			const FName ThisDisplayName =
 				(ControlElement->IsAnimationChannel()) ?
 				ControlElement->GetDisplayName() :
				ControlElement->Settings.DisplayName;

			if(ObjectIndex == 0)
			{
				DisplayName = ThisDisplayName;
			}
			else if(DisplayName != ThisDisplayName)
			{
				return TLLRN_ControlRigDetailsMultipleValues;
			}
		}
	}

	if(!DisplayName.IsNone())
	{
		return FText::FromName(DisplayName);
	}
	return FText();
}

void FTLLRN_RigControlElementDetails::SetDisplayName(const FText& InNewText, ETextCommit::Type InCommitType)
{
	for(int32 ObjectIndex = 0; ObjectIndex < PerElementInfos.Num(); ObjectIndex++)
	{
		const FPerElementInfo& Info = PerElementInfos[ObjectIndex];
		if(const FTLLRN_RigControlElement* ControlElement = Info.GetDefaultElement<FTLLRN_RigControlElement>())
		{
			SetDisplayNameForElement(InNewText, InCommitType, ControlElement->GetKey());
		}
	}
}

FText FTLLRN_RigControlElementDetails::GetDisplayNameForElement(const FTLLRN_RigElementKey& InKey) const
{
	if(PerElementInfos.IsEmpty())
	{
		return FText();
	}

	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetDefaultHierarchy();
	const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(InKey);
	if(ControlElement == nullptr)
	{
		return FText::FromName(InKey.Name);
	}

	return FText::FromName(ControlElement->GetDisplayName());
}

void FTLLRN_RigControlElementDetails::SetDisplayNameForElement(const FText& InNewText, ETextCommit::Type InCommitType, const FTLLRN_RigElementKey& InKeyToRename)
{
	if(InCommitType == ETextCommit::OnCleared)
	{
		return;
	}

	if(PerElementInfos.IsEmpty())
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetDefaultHierarchy();
	const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(InKeyToRename);
	if(ControlElement == nullptr)
	{
		return;
	}
	if(ControlElement->IsProcedural())
	{
		return;
	}

	const FName DisplayName = InNewText.IsEmpty() ? FName(NAME_None) : FName(*InNewText.ToString());
	const bool bRename = IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType::AnimationChannel);
	Hierarchy->GetController(true)->SetDisplayName(InKeyToRename, DisplayName, bRename, true, true);
}

bool FTLLRN_RigControlElementDetails::OnVerifyDisplayNameChanged(const FText& InText, FText& OutErrorMessage, const FTLLRN_RigElementKey& InKeyToRename)
{
	const FString NewName = InText.ToString();
	if (NewName.IsEmpty())
	{
		OutErrorMessage = FText::FromString(TEXT("Name is empty."));
		return false;
	}

	if(PerElementInfos.IsEmpty())
	{
		return false;
	}

	const UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetDefaultHierarchy();
	const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(InKeyToRename);
	if(ControlElement == nullptr)
	{
		return false;
	}
	if(ControlElement->IsProcedural())
	{
		return false;
	}

	// make sure there is no duplicate
	if(const FTLLRN_RigBaseElement* ParentElement = Hierarchy->GetFirstParent(ControlElement))
	{
		FString OutErrorString;
		if (!Hierarchy->IsDisplayNameAvailable(ParentElement->GetKey(), FTLLRN_RigName(NewName), &OutErrorString))
		{
			OutErrorMessage = FText::FromString(OutErrorString);
			return false;
		}
	}
	return true;
}

void FTLLRN_RigControlElementDetails::OnCopyShapeProperties()
{
	FString Value;

	if (!PerElementInfos.IsEmpty())
	{
		if(const FTLLRN_RigControlElement* ControlElement = PerElementInfos[0].GetElement<FTLLRN_RigControlElement>())
		{
			Value = FString::Printf(TEXT("(ShapeName=\"%s\",ShapeColor=%s)"),
				*ControlElement->Settings.ShapeName.ToString(),
				*ControlElement->Settings.ShapeColor.ToString());
		}
	}
		
	if (!Value.IsEmpty())
	{
		// Copy.
		FPlatformApplicationMisc::ClipboardCopy(*Value);
	}
}

void FTLLRN_RigControlElementDetails::OnPasteShapeProperties()
{
	FString PastedText;
	FPlatformApplicationMisc::ClipboardPaste(PastedText);
	
	FString TrimmedText = PastedText.LeftChop(1).RightChop(1);
	FString ShapeName;
	FString ShapeColorStr;
	bool bSuccessful = FParse::Value(*TrimmedText, TEXT("ShapeName="), ShapeName) &&
					   FParse::Value(*TrimmedText, TEXT("ShapeColor="), ShapeColorStr, false);

	if (bSuccessful)
	{
		FScopedTransaction Transaction(LOCTEXT("PasteShape", "Paste Shape"));
		
		// Name
		{
			ShapeNameHandle->NotifyPreChange();
			ShapeNameHandle->SetValue(ShapeName);
			ShapeNameHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
		}
		
		// Color
		{
			ShapeColorHandle->NotifyPreChange();
			TArray<void*> RawDataPtrs;
			ShapeColorHandle->AccessRawData(RawDataPtrs);
			for (void* RawPtr: RawDataPtrs)
			{
				bSuccessful &= static_cast<FLinearColor*>(RawPtr)->InitFromString(ShapeColorStr);
				if (!bSuccessful)
				{
					Transaction.Cancel();
					return;
				}
			}		
			ShapeColorHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
		}
	}
}

FDetailWidgetRow& FTLLRN_RigControlElementDetails::CreateBoolValueWidgetRow(
	const TArray<FTLLRN_RigElementKey>& Keys,
	IDetailCategoryBuilder& CategoryBuilder,
	const FText& Label,
	const FText& Tooltip,
	ETLLRN_RigControlValueType ValueType,
	TAttribute<EVisibility> Visibility,
	TSharedPtr<SWidget> NameContent)
{
	const static TCHAR* TrueText = TEXT("True");
	const static TCHAR* FalseText = TEXT("False");
	
	const bool bIsProcedural = IsAnyElementProcedural();
	const bool bIsEnabled = !bIsProcedural || ValueType == ETLLRN_RigControlValueType::Current;

	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();
	UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();
	if(ValueType == ETLLRN_RigControlValueType::Current)
	{
		HierarchyToChange = Hierarchy;
	}

	if(!NameContent.IsValid())
	{
		SAssignNew(NameContent, STextBlock)
		.Text(Label)
		.ToolTipText(Tooltip)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.IsEnabled(bIsEnabled);
	}

	FDetailWidgetRow& WidgetRow = CategoryBuilder.AddCustomRow(Label)
	.Visibility(Visibility)
	.NameContent()
	.MinDesiredWidth(200.f)
	.MaxDesiredWidth(800.f)
	[
		NameContent.ToSharedRef()
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([ValueType, Keys, Hierarchy]() -> ECheckBoxState
		{
			const bool FirstValue = Hierarchy->GetControlValue<bool>(Keys[0], ValueType);
			for(int32 Index = 1; Index < Keys.Num(); Index++)
			{
				const bool SecondValue = Hierarchy->GetControlValue<bool>(Keys[Index], ValueType);
				if(FirstValue != SecondValue)
				{
					return ECheckBoxState::Undetermined;
				}
			}
			return FirstValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([ValueType, Keys, HierarchyToChange](ECheckBoxState NewState)
		{
			if(NewState == ECheckBoxState::Undetermined)
			{
				return;
			}

			const bool Value = NewState == ECheckBoxState::Checked;
			FScopedTransaction Transaction(LOCTEXT("ChangeValue", "Change Value"));
			HierarchyToChange->Modify();
			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<bool>(Value), ValueType, true, true); 
			}
		})
		.IsEnabled(bIsEnabled)
	]
	.CopyAction(FUIAction(
	FExecuteAction::CreateLambda([ValueType, Keys, Hierarchy]()
		{
			const bool FirstValue = Hierarchy->GetControlValue<bool>(Keys[0], ValueType);
			FPlatformApplicationMisc::ClipboardCopy(FirstValue ? TrueText : FalseText);
		}),
		FCanExecuteAction())
	)
	.PasteAction(FUIAction(
		FExecuteAction::CreateLambda([ValueType, Keys, HierarchyToChange]()
		{
			FString Content;
			FPlatformApplicationMisc::ClipboardPaste(Content);

			const bool Value = FToBoolHelper::FromCStringWide(*Content);
			FScopedTransaction Transaction(LOCTEXT("ChangeValue", "Change Value"));
			HierarchyToChange->Modify();
			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<bool>(Value), ValueType, true, true); 
			}
		}),
		FCanExecuteAction::CreateLambda([bIsEnabled]() { return bIsEnabled; }))
	)
	.OverrideResetToDefault(FResetToDefaultOverride::Create(
		TAttribute<bool>::CreateLambda([ValueType, Keys, Hierarchy, bIsEnabled]() -> bool
		{
			if(!bIsEnabled)
			{
				return false;
			}
			
			const bool FirstValue = Hierarchy->GetControlValue<bool>(Keys[0], ValueType);
			const bool ReferenceValue = ValueType == ETLLRN_RigControlValueType::Initial ? false :
				Hierarchy->GetControlValue<bool>(Keys[0], ETLLRN_RigControlValueType::Initial);

			return FirstValue != ReferenceValue;
		}),
		FSimpleDelegate::CreateLambda([ValueType, Keys, HierarchyToChange]()
		{
			FScopedTransaction Transaction(LOCTEXT("ResetValueToDefault", "Reset Value To Default"));
			HierarchyToChange->Modify();
			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				const bool ReferenceValue = ValueType == ETLLRN_RigControlValueType::Initial ? false :
					HierarchyToChange->GetControlValue<bool>(Keys[0], ETLLRN_RigControlValueType::Initial);
				HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<bool>(ReferenceValue), ValueType, true, true); 
			}
		})
	));

	return WidgetRow;
}

FDetailWidgetRow& FTLLRN_RigControlElementDetails::CreateFloatValueWidgetRow(
	const TArray<FTLLRN_RigElementKey>& Keys,
	IDetailCategoryBuilder& CategoryBuilder,
	const FText& Label,
	const FText& Tooltip,
	ETLLRN_RigControlValueType ValueType,
	TAttribute<EVisibility> Visibility,
	TSharedPtr<SWidget> NameContent)
{
	return CreateNumericValueWidgetRow<float>(Keys, CategoryBuilder, Label, Tooltip, ValueType, Visibility, NameContent);
}

FDetailWidgetRow& FTLLRN_RigControlElementDetails::CreateIntegerValueWidgetRow(
	const TArray<FTLLRN_RigElementKey>& Keys,
	IDetailCategoryBuilder& CategoryBuilder,
	const FText& Label,
	const FText& Tooltip,
	ETLLRN_RigControlValueType ValueType,
	TAttribute<EVisibility> Visibility,
	TSharedPtr<SWidget> NameContent)
{
	return CreateNumericValueWidgetRow<int32>(Keys, CategoryBuilder, Label, Tooltip, ValueType, Visibility, NameContent);
}

FDetailWidgetRow& FTLLRN_RigControlElementDetails::CreateEnumValueWidgetRow(
	const TArray<FTLLRN_RigElementKey>& Keys,
	IDetailCategoryBuilder& CategoryBuilder,
	const FText& Label,
	const FText& Tooltip,
	ETLLRN_RigControlValueType ValueType,
	TAttribute<EVisibility> Visibility,
	TSharedPtr<SWidget> NameContent)
{
	const bool bIsProcedural = IsAnyElementProcedural();
	const bool bIsEnabled = !bIsProcedural || ValueType == ETLLRN_RigControlValueType::Current;

	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();
	UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();
	if(ValueType == ETLLRN_RigControlValueType::Current)
	{
		HierarchyToChange = Hierarchy;
	}

	UEnum* Enum = nullptr;
	for(const FTLLRN_RigElementKey& Key : Keys)
	{
		if(const FPerElementInfo& Info = FindElement(Key))
		{
			if(const FTLLRN_RigControlElement* ControlElement = Info.GetElement<FTLLRN_RigControlElement>())
			{
				Enum = ControlElement->Settings.ControlEnum.Get();
				if(Enum)
				{
					break;
				}
			}
		}
		else
		{
			// If the key was not found for selected elements, it might be a child channel of one of the elements
			for (const FPerElementInfo ElementInfo : PerElementInfos)
			{
				if (const FTLLRN_RigControlElement* ControlElement = ElementInfo.GetElement<FTLLRN_RigControlElement>())
				{
					const FTLLRN_RigBaseElementChildrenArray Children = Hierarchy->GetChildren(ControlElement, false);
					if (FTLLRN_RigBaseElement* const* Child = Children.FindByPredicate([Key](const FTLLRN_RigBaseElement* Info)
					{
						return Info->GetKey() == Key;
					}))
					{
						if (const FTLLRN_RigControlElement* ChildElement = Cast<FTLLRN_RigControlElement>(*Child))
						{
							Enum = ChildElement->Settings.ControlEnum.Get();
							if(Enum)
							{
								break;
							}
						}
					}
				}
			}
			if(Enum)
			{
				break;
			}
		}
	}

	check(Enum != nullptr);

	if(!NameContent.IsValid())
	{
		SAssignNew(NameContent, STextBlock)
		.Text(Label)
		.ToolTipText(Tooltip)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.IsEnabled(bIsEnabled);
	}

	FDetailWidgetRow& WidgetRow = CategoryBuilder.AddCustomRow(Label)
	.Visibility(Visibility)
	.NameContent()
	.MinDesiredWidth(200.f)
	.MaxDesiredWidth(800.f)
	[
		NameContent.ToSharedRef()
	]
	.ValueContent()
	[
		SNew(SEnumComboBox, Enum)
		.CurrentValue_Lambda([ValueType, Keys, Hierarchy]() -> int32
		{
			const int32 FirstValue = Hierarchy->GetControlValue<int32>(Keys[0], ValueType);
			for(int32 Index = 1; Index < Keys.Num(); Index++)
			{
				const int32 SecondValue = Hierarchy->GetControlValue<int32>(Keys[Index], ValueType);
				if(FirstValue != SecondValue)
				{
					return INDEX_NONE;
				}
			}
			return FirstValue;
		})
		.OnEnumSelectionChanged_Lambda([ValueType, Keys, HierarchyToChange](int32 NewSelection, ESelectInfo::Type)
		{
			if(NewSelection == INDEX_NONE)
			{
				return;
			}

			FScopedTransaction Transaction(LOCTEXT("ChangeValue", "Change Value"));
			HierarchyToChange->Modify();
			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<int32>(NewSelection), ValueType, true, true); 
			}
		})
		.Font(FAppStyle::GetFontStyle(TEXT("MenuItem.Font")))
		.IsEnabled(bIsEnabled)
	]
	.CopyAction(FUIAction(
	FExecuteAction::CreateLambda([ValueType, Keys, Hierarchy]()
		{
			const int32 FirstValue = Hierarchy->GetControlValue<int32>(Keys[0], ValueType);
			FPlatformApplicationMisc::ClipboardCopy(*FString::FromInt(FirstValue));
		}),
		FCanExecuteAction())
	)
	.PasteAction(FUIAction(
		FExecuteAction::CreateLambda([ValueType, Keys, HierarchyToChange]()
		{
			FString Content;
			FPlatformApplicationMisc::ClipboardPaste(Content);
			if(!Content.IsNumeric())
			{
				return;
			}

			const int32 Value = FCString::Atoi(*Content);
			FScopedTransaction Transaction(LOCTEXT("ChangeValue", "Change Value"));
			HierarchyToChange->Modify();

			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<int32>(Value), ValueType, true, true); 
			}
		}),
		FCanExecuteAction::CreateLambda([bIsEnabled]() { return bIsEnabled; }))
	)
	.OverrideResetToDefault(FResetToDefaultOverride::Create(
		TAttribute<bool>::CreateLambda([ValueType, Keys, Hierarchy, bIsEnabled]() -> bool
		{
			if(!bIsEnabled)
			{
				return false;
			}
			
			const int32 FirstValue = Hierarchy->GetControlValue<int32>(Keys[0], ValueType);
			const int32 ReferenceValue = ValueType == ETLLRN_RigControlValueType::Initial ? 0 :
				Hierarchy->GetControlValue<int32>(Keys[0], ETLLRN_RigControlValueType::Initial);

			return FirstValue != ReferenceValue;
		}),
		FSimpleDelegate::CreateLambda([ValueType, Keys, HierarchyToChange]()
		{
			FScopedTransaction Transaction(LOCTEXT("ResetValueToDefault", "Reset Value To Default"));
			HierarchyToChange->Modify();
			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				const int32 ReferenceValue = ValueType == ETLLRN_RigControlValueType::Initial ? 0 :
					HierarchyToChange->GetControlValue<int32>(Keys[0], ETLLRN_RigControlValueType::Initial);
				HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<int32>(ReferenceValue), ValueType, true, true); 
			}
		})
	));

	return WidgetRow;
}

FDetailWidgetRow& FTLLRN_RigControlElementDetails::CreateVector2DValueWidgetRow(
	const TArray<FTLLRN_RigElementKey>& Keys,
	IDetailCategoryBuilder& CategoryBuilder, 
	const FText& Label, 
	const FText& Tooltip, 
	ETLLRN_RigControlValueType ValueType,
	TAttribute<EVisibility> Visibility,
	TSharedPtr<SWidget> NameContent)
{
	const bool bIsProcedural = IsAnyElementProcedural();
	const bool bIsEnabled = !bIsProcedural || ValueType == ETLLRN_RigControlValueType::Current;
	const bool bShowToggle = (ValueType == ETLLRN_RigControlValueType::Minimum) || (ValueType == ETLLRN_RigControlValueType::Maximum);
	
	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();
	UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();
	if(ValueType == ETLLRN_RigControlValueType::Current)
	{
		HierarchyToChange = Hierarchy;
	}

	using SNumericVector2DInputBox = SNumericVectorInputBox<float, FVector2f, 2>;
	TSharedPtr<SNumericVector2DInputBox> VectorInputBox;
	
	FDetailWidgetRow& WidgetRow = CategoryBuilder.AddCustomRow(Label);
	TAttribute<ECheckBoxState> ToggleXChecked, ToggleYChecked;
	FOnCheckStateChanged OnToggleXChanged, OnToggleYChanged;

	if(bShowToggle)
	{
		auto ToggleChecked = [ValueType, Keys, Hierarchy](int32 Index) -> ECheckBoxState
		{
			TOptional<bool> FirstValue;

			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Key))
				{
					if(ControlElement->Settings.LimitEnabled.Num() == 2)
					{
						const bool Value = ControlElement->Settings.LimitEnabled[Index].GetForValueType(ValueType);
						if(FirstValue.IsSet())
						{
							if(FirstValue.GetValue() != Value)
							{
								return ECheckBoxState::Undetermined;
							}
						}
						else
						{
							FirstValue = Value;
						}
					}
				}
			}

			if(!ensure(FirstValue.IsSet()))
			{
				return ECheckBoxState::Undetermined;
			}

			return FirstValue.GetValue() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		};
		
		ToggleXChecked = TAttribute<ECheckBoxState>::CreateLambda([ToggleChecked]() -> ECheckBoxState
		{
			return ToggleChecked(0);
		});

		ToggleYChecked = TAttribute<ECheckBoxState>::CreateLambda([ToggleChecked]() -> ECheckBoxState
		{
			return ToggleChecked(1);
		});

		auto OnToggleChanged = [ValueType, Keys, HierarchyToChange](ECheckBoxState InValue, int32 Index)
		{
			if(InValue == ECheckBoxState::Undetermined)
			{
				return;
			}
					
			FScopedTransaction Transaction(LOCTEXT("ChangeLimitToggle", "Change Limit Toggle"));
			HierarchyToChange->Modify();

			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				if(FTLLRN_RigControlElement* ControlElement = HierarchyToChange->Find<FTLLRN_RigControlElement>(Key))
				{
					if(ControlElement->Settings.LimitEnabled.Num() == 2)
					{
						ControlElement->Settings.LimitEnabled[Index].SetForValueType(ValueType, InValue == ECheckBoxState::Checked);
						HierarchyToChange->SetControlSettings(ControlElement, ControlElement->Settings, true, true, true);
					}
				}
			}
		};

		OnToggleXChanged = FOnCheckStateChanged::CreateLambda([OnToggleChanged](ECheckBoxState InValue)
		{
			OnToggleChanged(InValue, 0);
		});

		OnToggleYChanged = FOnCheckStateChanged::CreateLambda([OnToggleChanged](ECheckBoxState InValue)
		{
			OnToggleChanged(InValue, 1);
		});
	}

	auto GetValue = [ValueType, Keys, Hierarchy](int32 Component) -> TOptional<float>
	{
		const float FirstValue = Hierarchy->GetControlValue<FVector3f>(Keys[0], ValueType).Component(Component);
		for(int32 Index = 1; Index < Keys.Num(); Index++)
		{
			const float SecondValue = Hierarchy->GetControlValue<FVector3f>(Keys[Index], ValueType).Component(Component);
			if(FirstValue != SecondValue)
			{
				return TOptional<float>();
			}
		}
		return FirstValue;
	};

	auto OnValueChanged = [ValueType, Keys, Hierarchy, HierarchyToChange, this]
		(TOptional<float> InValue, ETextCommit::Type InCommitType, bool bSetupUndo, int32 Component)
		{
			if(!InValue.IsSet())
			{
				return;
			}

			const float Value = InValue.GetValue();
		
			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				FVector3f Vector = Hierarchy->GetControlValue<FVector3f>(Key, ValueType);
				if(!FMath::IsNearlyEqual(Vector.Component(Component), Value))
				{
					if(!SliderTransaction.IsValid())
					{
						SliderTransaction = MakeShareable(new FScopedTransaction(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ChangeValue", "Change Value")));
						HierarchyToChange->Modify();
					}
					Vector.Component(Component) = Value;
					HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<FVector3f>(Vector), ValueType, bSetupUndo, bSetupUndo);
				};
			}

			if(bSetupUndo)
			{
				SliderTransaction.Reset();
			}
	};

	if(!NameContent.IsValid())
	{
		SAssignNew(NameContent, STextBlock)
		.Text(Label)
		.ToolTipText(Tooltip)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.IsEnabled(bIsEnabled);
	}

	WidgetRow
	.Visibility(Visibility)
	.NameContent()
	.MinDesiredWidth(200.f)
	.MaxDesiredWidth(800.f)
	[
		NameContent.ToSharedRef()
	]
	.ValueContent()
	[
		SAssignNew(VectorInputBox, SNumericVector2DInputBox)
        .Font(FAppStyle::GetFontStyle(TEXT("MenuItem.Font")))
        .AllowSpin(ValueType == ETLLRN_RigControlValueType::Current || ValueType == ETLLRN_RigControlValueType::Initial)
		.SpinDelta(0.01f)
		.X_Lambda([GetValue]() -> TOptional<float>
        {
			return GetValue(0);
        })
        .Y_Lambda([GetValue]() -> TOptional<float>
		{
			return GetValue(1);
		})
		.OnXChanged_Lambda([OnValueChanged](TOptional<float> InValue)
		{
			OnValueChanged(InValue, ETextCommit::Default, false, 0);
		})
		.OnYChanged_Lambda([OnValueChanged](TOptional<float> InValue)
		{
			OnValueChanged(InValue, ETextCommit::Default, false, 1);
		})
		.OnXCommitted_Lambda([OnValueChanged](TOptional<float> InValue, ETextCommit::Type InCommitType)
		{
			OnValueChanged(InValue, InCommitType, true, 0);
		})
		.OnYCommitted_Lambda([OnValueChanged](TOptional<float> InValue, ETextCommit::Type InCommitType)
		{
			OnValueChanged(InValue, InCommitType, true, 1);
		})
		 .DisplayToggle(bShowToggle)
		 .ToggleXChecked(ToggleXChecked)
		 .ToggleYChecked(ToggleYChecked)
		 .OnToggleXChanged(OnToggleXChanged)
		 .OnToggleYChanged(OnToggleYChanged)
		 .IsEnabled(bIsEnabled)
	]
	.CopyAction(FUIAction(
	FExecuteAction::CreateLambda([ValueType, Keys, Hierarchy]()
		{
			const FVector3f Data3 = Hierarchy->GetControlValue<FVector3f>(Keys[0], ValueType);
			const FVector2f Data(Data3.X, Data3.Y);
			FString Content = Data.ToString();
			FPlatformApplicationMisc::ClipboardCopy(*Content);
		}),
		FCanExecuteAction())
	)
	.PasteAction(FUIAction(
		FExecuteAction::CreateLambda([ValueType, Keys, HierarchyToChange]()
		{
			FString Content;
			FPlatformApplicationMisc::ClipboardPaste(Content);
			if(Content.IsEmpty())
			{
				return;
			}

			FVector2f Data = FVector2f::ZeroVector;
			Data.InitFromString(Content);

			FVector3f Data3(Data.X, Data.Y, 0);

			FScopedTransaction Transaction(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ChangeValue", "Change Value"));
			HierarchyToChange->Modify();
			
			for(const FTLLRN_RigElementKey& Key : Keys)
			{
				HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<FVector3f>(Data3), ValueType, true, true); 
			}
		}),
		FCanExecuteAction::CreateLambda([bIsEnabled]() { return bIsEnabled; }))
	);

	if((ValueType == ETLLRN_RigControlValueType::Current || ValueType == ETLLRN_RigControlValueType::Initial) && bIsEnabled)
	{
		WidgetRow.OverrideResetToDefault(FResetToDefaultOverride::Create(
			TAttribute<bool>::CreateLambda([ValueType, Keys, Hierarchy]() -> bool
			{
				const FVector3f FirstValue = Hierarchy->GetControlValue<FVector3f>(Keys[0], ValueType);
				const FVector3f ReferenceValue = ValueType == ETLLRN_RigControlValueType::Initial ? FVector3f::ZeroVector :
					Hierarchy->GetControlValue<FVector3f>(Keys[0], ETLLRN_RigControlValueType::Initial);

				return !(FirstValue - ReferenceValue).IsNearlyZero();
			}),
			FSimpleDelegate::CreateLambda([ValueType, Keys, HierarchyToChange]()
			{
				FScopedTransaction Transaction(LOCTEXT("ResetValueToDefault", "Reset Value To Default"));
				HierarchyToChange->Modify();
				
				for(const FTLLRN_RigElementKey& Key : Keys)
				{
					const FVector3f ReferenceValue = ValueType == ETLLRN_RigControlValueType::Initial ? FVector3f::ZeroVector :
						HierarchyToChange->GetControlValue<FVector3f>(Keys[0], ETLLRN_RigControlValueType::Initial);
					HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<FVector3f>(ReferenceValue), ValueType, true, true); 
				}
			})
		));
	}

	return WidgetRow;
}

void FTLLRN_RigNullElementDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	FTLLRN_RigTransformElementDetails::CustomizeDetails(DetailBuilder);
	CustomizeTransform(DetailBuilder);
	CustomizeMetadata(DetailBuilder);
}

void FTLLRN_RigConnectorElementDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	FTLLRN_RigTransformElementDetails::CustomizeDetails(DetailBuilder);
	CustomizeSettings(DetailBuilder);
	//CustomizeTransform(DetailBuilder);
	//CustomizeMetadata(DetailBuilder);
}

void FTLLRN_RigConnectorElementDetails::CustomizeSettings(IDetailLayoutBuilder& DetailBuilder)
{
	if(PerElementInfos.IsEmpty())
	{
		return;
	}

	if(IsAnyElementNotOfType(ETLLRN_RigElementType::Connector))
	{
		return;
	}

	const TSharedPtr<IPropertyHandle> SettingsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(FTLLRN_RigConnectorElement, Settings));
	DetailBuilder.HideProperty(SettingsHandle);

	IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory(TEXT("Settings"), LOCTEXT("Settings", "Settings"));

	SettingsCategory
		.AddProperty(SettingsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTLLRN_RigConnectorSettings, Type)))
		.IsEnabled(false);

	SettingsCategory
		.AddProperty(SettingsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTLLRN_RigConnectorSettings, bOptional)))
		.Visibility(IsAnyConnectorPrimary() ? EVisibility::Collapsed : EVisibility::Visible)
		.IsEnabled(!IsAnyConnectorImported());

	bool bHideRules = false;
	uint32 FirstHash = UINT32_MAX;
	for (const FPerElementInfo& Info : PerElementInfos)
	{
		if (UTLLRN_RigHierarchy* Hierarchy = Info.IsValid() ? Info.GetHierarchy() : nullptr)
		{
			if(const FTLLRN_RigConnectorElement* Connector = Info.GetElement<FTLLRN_RigConnectorElement>())
			{
				const uint32 Hash = Connector->Settings.GetRulesHash();
				if(FirstHash == UINT32_MAX)
				{
					FirstHash = Hash;
				}
				else if(FirstHash != Hash)
				{
					bHideRules = true;
					break;
				}
			}
			else
			{
				bHideRules = true;
			}
		}
	}

	if(!bHideRules)
	{
		SettingsCategory
			.AddProperty(SettingsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTLLRN_RigConnectorSettings, Rules)))
			.IsEnabled(!IsAnyConnectorImported());
	}
}

void FTLLRN_RigSocketElementDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	FTLLRN_RigTransformElementDetails::CustomizeDetails(DetailBuilder);
	CustomizeSettings(DetailBuilder);
	CustomizeTransform(DetailBuilder);
	CustomizeMetadata(DetailBuilder);
}

void FTLLRN_RigSocketElementDetails::CustomizeSettings(IDetailLayoutBuilder& DetailBuilder)
{
	if(PerElementInfos.IsEmpty())
	{
		return;
	}

	if(IsAnyElementNotOfType(ETLLRN_RigElementType::Socket))
	{
		return;
	}

	const bool bIsProcedural = IsAnyElementProcedural();
	
	IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory(TEXT("Settings"), LOCTEXT("Settings", "Settings"));

	SettingsCategory.AddCustomRow(FText::FromString(TEXT("Color")))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("Color", "Color"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	[
		SNew(SColorBlock)
		.IsEnabled(!bIsProcedural)
		//.Size(FVector2D(6.0, 38.0))
		.Color(this, &FTLLRN_RigSocketElementDetails::GetSocketColor) 
		.OnMouseButtonDown(this, &FTLLRN_RigSocketElementDetails::SetSocketColor)
	];

	SettingsCategory.AddCustomRow(FText::FromString(TEXT("Description")))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("Description", "Description"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	[
		SNew(SEditableText)
		.IsEnabled(!bIsProcedural)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(this, &FTLLRN_RigSocketElementDetails::GetSocketDescription)
		.OnTextCommitted(this, &FTLLRN_RigSocketElementDetails::SetSocketDescription)
	];
}

FReply FTLLRN_RigSocketElementDetails::SetSocketColor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FColorPickerArgs PickerArgs;
	PickerArgs.bUseAlpha = false;
	PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
	PickerArgs.InitialColor = GetSocketColor();
	PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &FTLLRN_RigSocketElementDetails::OnSocketColorPicked);
	OpenColorPicker(PickerArgs);
	return FReply::Handled();
}

FLinearColor FTLLRN_RigSocketElementDetails::GetSocketColor() const 
{
	if(PerElementInfos.Num() > 1)
	{
		return FTLLRN_RigSocketElement::SocketDefaultColor;
	}
	UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetDefaultHierarchy();
	const FTLLRN_RigSocketElement* Socket = PerElementInfos[0].GetDefaultElement<FTLLRN_RigSocketElement>();
	return Socket->GetColor(Hierarchy);
}

void FTLLRN_RigSocketElementDetails::OnSocketColorPicked(FLinearColor NewColor)
{
	FScopedTransaction Transaction(LOCTEXT("SocketColorChanged", "Socket Color Changed"));
	for(FPerElementInfo& Info : PerElementInfos)
	{
		UTLLRN_RigHierarchy* Hierarchy = Info.GetDefaultHierarchy();
		Hierarchy->Modify();
		FTLLRN_RigSocketElement* Socket = Info.GetDefaultElement<FTLLRN_RigSocketElement>();
		Socket->SetColor(NewColor, Hierarchy);
	}
}

void FTLLRN_RigSocketElementDetails::SetSocketDescription(const FText& InDescription, ETextCommit::Type InCommitType)
{
	const FString Description = InDescription.ToString();
	for(FPerElementInfo& Info : PerElementInfos)
	{
		UTLLRN_RigHierarchy* Hierarchy = Info.GetDefaultHierarchy();
		Hierarchy->Modify();
		FTLLRN_RigSocketElement* Socket = Info.GetDefaultElement<FTLLRN_RigSocketElement>();
		Socket->SetDescription(Description, Hierarchy);
	}
}

FText FTLLRN_RigSocketElementDetails::GetSocketDescription() const
{
	FString FirstValue;
	for(int32 Index = 0; Index < PerElementInfos.Num(); Index++)
	{
		UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[Index].GetDefaultHierarchy();
		const FTLLRN_RigSocketElement* Socket = PerElementInfos[Index].GetDefaultElement<FTLLRN_RigSocketElement>();
		const FString Description = Socket->GetDescription(Hierarchy);
		if(Index == 0)
		{
			FirstValue = Description;
		}
		else if(!FirstValue.Equals(Description, ESearchCase::CaseSensitive))
		{
			return TLLRN_ControlRigDetailsMultipleValues;
		}
	}
	return FText::FromString(FirstValue);
}

void FTLLRN_RigConnectionRuleDetails::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle.ToSharedPtr();
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();
	BlueprintBeingCustomized = nullptr;
	EnabledAttribute = false;
	TLLRN_RigElementKeyDetails_GetCustomizedInfo(InStructPropertyHandle, BlueprintBeingCustomized);

	TArray<UObject*> Objects;
	StructPropertyHandle->GetOuterObjects(Objects);
	FString FirstObjectValue;
	for (int32 Index = 0; Index < Objects.Num(); Index++)
	{
		FString ObjectValue;
		if(InStructPropertyHandle->GetPerObjectValue(Index, ObjectValue) == FPropertyAccess::Result::Success)
		{
			if(FirstObjectValue.IsEmpty())
			{
				FirstObjectValue = ObjectValue;
			}
			else
			{
				if(!FirstObjectValue.Equals(ObjectValue, ESearchCase::CaseSensitive))
				{
					FirstObjectValue.Reset();
					break;
				}
			}
		}

		// only enable editing of the rule if the widget is nested under a wrapper object (a rig element)
		if(Objects[Index]->IsA<URigVMDetailsViewWrapperObject>())
		{
			EnabledAttribute = true;
		}
	}

	if(!FirstObjectValue.IsEmpty())
	{
		FTLLRN_RigConnectionRuleStash::StaticStruct()->ImportText(*FirstObjectValue, &RuleStash, nullptr, EPropertyPortFlags::PPF_None, nullptr, FTLLRN_RigConnectionRuleStash::StaticStruct()->GetName(), true);
	}

	if (BlueprintBeingCustomized == nullptr || FirstObjectValue.IsEmpty())
	{
		HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			StructPropertyHandle->CreatePropertyValueWidget()
		];
	}
	else
	{
		HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.ContentPadding(FMargin(2,2,2,1))
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FTLLRN_RigConnectionRuleDetails::OnGetStructTextValue)
			]
			.OnGetMenuContent(this, &FTLLRN_RigConnectionRuleDetails::GenerateStructPicker)
			.IsEnabled(EnabledAttribute)
		];
	}
}

void FTLLRN_RigConnectionRuleDetails::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	const UScriptStruct* ScriptStruct = RuleStash.GetScriptStruct();
	if(ScriptStruct == nullptr)
	{
		return;
	}

	(void)RuleStash.Get(Storage);
	const TSharedRef<FStructOnScope> StorageRef = Storage.ToSharedRef();
	const FSimpleDelegate OnPropertyChanged = FSimpleDelegate::CreateSP(this, &FTLLRN_RigConnectionRuleDetails::OnRuleContentChanged);

	TArray<TSharedPtr<IPropertyHandle>> ChildProperties = InStructPropertyHandle->AddChildStructure(StorageRef);
	for (TSharedPtr<IPropertyHandle> ChildHandle : ChildProperties)
	{
		ChildHandle->SetOnPropertyValueChanged(OnPropertyChanged);
		IDetailPropertyRow& ChildRow = StructBuilder.AddProperty(ChildHandle.ToSharedRef());
		ChildRow.IsEnabled(EnabledAttribute);
	}
}

TSharedRef<SWidget> FTLLRN_RigConnectionRuleDetails::GenerateStructPicker()
{
	FStructViewerModule& StructViewerModule = FModuleManager::LoadModuleChecked<FStructViewerModule>("StructViewer");

	class FTLLRN_RigConnectionRuleFilter : public IStructViewerFilter
	{
	public:
		FTLLRN_RigConnectionRuleFilter()
		{
		}

		virtual bool IsStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const UScriptStruct* InStruct, TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override
		{
			static const UScriptStruct* BaseStruct = FTLLRN_RigConnectionRule::StaticStruct();
			return InStruct != BaseStruct && InStruct->IsChildOf(BaseStruct);
		}

		virtual bool IsUnloadedStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const FSoftObjectPath& InStructPath, TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override
		{
			return false;
		}
	};
		
	static TSharedPtr<FTLLRN_RigConnectionRuleFilter> Filter = MakeShared<FTLLRN_RigConnectionRuleFilter>();
	FStructViewerInitializationOptions Options;
	{
		Options.StructFilter = Filter;
		Options.Mode = EStructViewerMode::StructPicker;
		Options.DisplayMode = EStructViewerDisplayMode::ListView;
		Options.NameTypeToDisplay = EStructViewerNameTypeToDisplay::DisplayName;
		Options.bShowNoneOption = false;
		Options.bShowUnloadedStructs = false;
		Options.bAllowViewOptions = false;
	}

	return
		SNew(SBox)
		.WidthOverride(330.0f)
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.MaxHeight(500)
			[
				SNew(SBorder)
				.Padding(4)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					StructViewerModule.CreateStructViewer(Options, FOnStructPicked::CreateSP(this, &FTLLRN_RigConnectionRuleDetails::OnPickedStruct))
				]
			]
		];
}

void FTLLRN_RigConnectionRuleDetails::OnPickedStruct(const UScriptStruct* ChosenStruct)
{
	if(ChosenStruct == nullptr)
	{
		RuleStash = FTLLRN_RigConnectionRuleStash();
	}
	else
	{
		RuleStash.ScriptStructPath = ChosenStruct->GetPathName();
		RuleStash.ExportedText = TEXT("()");
		Storage.Reset();
		RuleStash.Get(Storage);
	}
	OnRuleContentChanged();
}

FText FTLLRN_RigConnectionRuleDetails::OnGetStructTextValue() const
{
	const UScriptStruct* ScriptStruct = RuleStash.GetScriptStruct();
	return ScriptStruct
		? FText::AsCultureInvariant(ScriptStruct->GetDisplayNameText())
		: LOCTEXT("None", "None");
}

void FTLLRN_RigConnectionRuleDetails::OnRuleContentChanged()
{
	const UScriptStruct* ScriptStruct = RuleStash.GetScriptStruct();
	if(Storage && Storage->GetStruct() == ScriptStruct)
	{
		RuleStash.ExportedText.Reset();
		const uint8* StructMemory = Storage->GetStructMemory();
		ScriptStruct->ExportText(RuleStash.ExportedText, StructMemory, StructMemory, nullptr, PPF_None, nullptr);
	}
	
	FString Content;
	FTLLRN_RigConnectionRuleStash::StaticStruct()->ExportText(Content, &RuleStash, &RuleStash, nullptr, PPF_None, nullptr);

	TArray<UObject*> Objects;
	StructPropertyHandle->GetOuterObjects(Objects);
	FString FirstObjectValue;
	for (int32 Index = 0; Index < Objects.Num(); Index++)
	{
		(void)StructPropertyHandle->SetPerObjectValue(Index, Content, EPropertyValueSetFlags::DefaultFlags);
	}
	StructPropertyHandle->GetParentHandle()->NotifyPostChange(EPropertyChangeType::ValueSet);
}

void FTLLRN_RigPhysicsElementDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	FTLLRN_RigTransformElementDetails::CustomizeDetails(DetailBuilder);
	CustomizeSettings(DetailBuilder);
	CustomizeTransform(DetailBuilder);
	CustomizeMetadata(DetailBuilder);
}

void FTLLRN_RigPhysicsElementDetails::CustomizeSettings(IDetailLayoutBuilder& DetailBuilder)
{
	if(PerElementInfos.IsEmpty())
	{
		return;
	}

	if(IsAnyElementNotOfType(ETLLRN_RigElementType::Physics))
	{
		return;
	}

	const TSharedPtr<IPropertyHandle> SolverHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(FTLLRN_RigPhysicsElement, Solver));
	DetailBuilder.HideProperty(SolverHandle);

	const TSharedPtr<IPropertyHandle> SettingsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(FTLLRN_RigPhysicsElement, Settings));
	DetailBuilder.HideProperty(SettingsHandle);

	IDetailCategoryBuilder& PhysicsCategory = DetailBuilder.EditCategory(TEXT("Physics"), LOCTEXT("Physics", "Physics"));

	FDetailWidgetRow& SolverNameRow = PhysicsCategory.AddCustomRow(LOCTEXT("SolverName", "Solver Name"));
	SolverNameRow.IsEnabled(false);
	SolverNameRow.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("SolverName", "Solver Name"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];
	SolverNameRow.ValueContent()
	[
		SNew(SEditableText)
		.Text(this, &FTLLRN_RigPhysicsElementDetails::GetSolverNameText)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	PhysicsCategory
	.AddProperty(SolverHandle)
	.DisplayName(LOCTEXT("SolverGuid", "Solver Guid"))
	.IsEnabled(false);

	const bool bIsProcedural = IsAnyElementProcedural();

	PhysicsCategory
		.AddProperty(SettingsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTLLRN_RigPhysicsSettings, Mass)))
		.IsEnabled(!bIsProcedural);
}

FText FTLLRN_RigPhysicsElementDetails::GetSolverNameText() const
{
	if(PerElementInfos.Num() > 0)
	{
		if(const UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy())
		{
			TOptional<FText> FirstValue;
			
			for(const FPerElementInfo& Info : PerElementInfos)
			{
				if(const FTLLRN_RigPhysicsElement* PhysicsElement = Info.Element.Get<FTLLRN_RigPhysicsElement>())
				{
					if(const FTLLRN_RigPhysicsSolverDescription* Solver = Hierarchy->FindPhysicsSolver(PhysicsElement->Solver))
					{
						const FText SolverName = FText::FromName(Solver->Name);
						if(FirstValue.IsSet())
						{
							if(!FirstValue.GetValue().EqualTo(SolverName))
							{
								return TLLRN_ControlRigDetailsMultipleValues;	
							}
						}
						else
						{
							FirstValue = SolverName;
						}
					}
				}
			}

			if(FirstValue.IsSet())
			{
				return FirstValue.GetValue();
			}
		}
	}
	return FText();
}
#undef LOCTEXT_NAMESPACE
