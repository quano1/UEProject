// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigAnimGraphDetails.h"

#include "TLLRN_ControlRig.h"
#include "Widgets/SWidget.h"
#include "DetailLayoutBuilder.h"
#include "Styling/AppStyle.h"
#include "PropertyCustomizationHelpers.h"
#include "AnimGraphNode_TLLRN_ControlRig.h"
#include "Algo/Transform.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigAnimGraphDetails"
static const FText TLLRN_ControlRigTLLRN_AnimDetailsMultipleValues = LOCTEXT("MultipleValues", "Multiple Values");

void FTLLRN_ControlRigAnimNodeEventNameDetails::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	AnimNodeBeingCustomized = nullptr;

	TArray<UObject*> Objects;
	InStructPropertyHandle->GetOuterObjects(Objects);
	for (UObject* Object : Objects)
	{
		if(UAnimGraphNode_TLLRN_ControlRig* GraphNode = Cast<UAnimGraphNode_TLLRN_ControlRig>(Object))
		{
			AnimNodeBeingCustomized = &GraphNode->Node;
			break;
		}
	}

	if (AnimNodeBeingCustomized == nullptr)
	{
		HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			InStructPropertyHandle->CreatePropertyValueWidget()
		];
	}
	else
	{
		NameHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTLLRN_ControlRigAnimNodeEventName, EventName));
		UpdateEntryNameList();

		HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 0.f, 0.f)
			[
				SAssignNew(SearchableComboBox, SSearchableComboBox)
				.OptionsSource(&EntryNameList)
				.OnSelectionChanged(this, &FTLLRN_ControlRigAnimNodeEventNameDetails::OnEntryNameChanged)
				.OnGenerateWidget(this, &FTLLRN_ControlRigAnimNodeEventNameDetails::OnGetEntryNameWidget)
				.IsEnabled(!NameHandle->IsEditConst())
				.Content()
				[
					SNew(STextBlock)
					.Text(this, &FTLLRN_ControlRigAnimNodeEventNameDetails::GetEntryNameAsText)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
			]
		];
	}
}

void FTLLRN_ControlRigAnimNodeEventNameDetails::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if (InStructPropertyHandle->IsValidHandle())
	{
		// only fill the children if the blueprint cannot be found
		if (AnimNodeBeingCustomized == nullptr)
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

FString FTLLRN_ControlRigAnimNodeEventNameDetails::GetEntryName() const
{
	FString EntryNameStr;
	if (NameHandle.IsValid())
	{
		for(int32 ObjectIndex = 0; ObjectIndex < NameHandle->GetNumPerObjectValues(); ObjectIndex++)
		{
			FString PerObjectValue;
			NameHandle->GetPerObjectValue(ObjectIndex, PerObjectValue);

			if(ObjectIndex == 0)
			{
				EntryNameStr = PerObjectValue;
			}
			else if(EntryNameStr != PerObjectValue)
			{
				return TLLRN_ControlRigTLLRN_AnimDetailsMultipleValues.ToString();
			}
		}
	}
	return EntryNameStr;
}

void FTLLRN_ControlRigAnimNodeEventNameDetails::SetEntryName(FString InName)
{
	if (NameHandle.IsValid())
	{
		NameHandle->SetValue(InName);
	}
}

void FTLLRN_ControlRigAnimNodeEventNameDetails::UpdateEntryNameList()
{
	EntryNameList.Reset();

	if (AnimNodeBeingCustomized)
	{
		if(const UClass* Class = AnimNodeBeingCustomized->GetTLLRN_ControlRigClass())
		{
			if(const UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(Class->GetDefaultObject(true)))
			{
				TArray<FName> SupportedEvents = CDO->GetSupportedEvents();

				// Remove Pre/Post forward solve
				SupportedEvents.Remove(FTLLRN_RigUnit_PreBeginExecution::EventName);
				SupportedEvents.Remove(FTLLRN_RigUnit_PostBeginExecution::EventName);
				
				Algo::Transform(SupportedEvents, EntryNameList,[](const FName& InEntryName)
				{
					return MakeShareable(new FString(InEntryName.ToString()));
				});
				if(SearchableComboBox.IsValid())
				{
					SearchableComboBox->RefreshOptions();
				}
			}
		}
	}
}

void FTLLRN_ControlRigAnimNodeEventNameDetails::OnEntryNameChanged(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectionInfo)
{
	if (InItem.IsValid())
	{
		SetEntryName(*InItem);
	}
	else
	{
		SetEntryName(FString());
	}
}

TSharedRef<SWidget> FTLLRN_ControlRigAnimNodeEventNameDetails::OnGetEntryNameWidget(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(InItem.IsValid() ? *InItem : FString()))
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

FText FTLLRN_ControlRigAnimNodeEventNameDetails::GetEntryNameAsText() const
{
	return FText::FromString(GetEntryName());
}

#undef LOCTEXT_NAMESPACE
