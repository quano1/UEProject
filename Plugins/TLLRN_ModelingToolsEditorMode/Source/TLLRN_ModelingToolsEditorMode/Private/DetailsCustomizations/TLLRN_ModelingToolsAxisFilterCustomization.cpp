// Copyright Epic Games, Inc. All Rights Reserved.

#include "DetailsCustomizations/TLLRN_ModelingToolPropertyCustomizations.h"

#include "DetailWidgetRow.h"


#include "PropertySets/AxisFilterPropertyType.h"
#include "Widgets/SBoxPanel.h"


#define LOCTEXT_NAMESPACE "TLLRN_ModelingToolsAxisFilterCustomization"


TSharedRef<IPropertyTypeCustomization> FTLLRN_ModelingToolsAxisFilterCustomization::MakeInstance()
{
	return MakeShareable(new FTLLRN_ModelingToolsAxisFilterCustomization);
}

void FTLLRN_ModelingToolsAxisFilterCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	const float XYZPadding = 10.f;

	TSharedPtr<IPropertyHandle> bAxisX = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FModelingToolsAxisFilter, bAxisX));
	bAxisX->MarkHiddenByCustomization();

	TSharedPtr<IPropertyHandle> bAxisY = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FModelingToolsAxisFilter, bAxisY));
	bAxisY->MarkHiddenByCustomization();

	TSharedPtr<IPropertyHandle> bAxisZ = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FModelingToolsAxisFilter, bAxisZ));
	bAxisZ->MarkHiddenByCustomization();

	HeaderRow.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
	.ValueContent()
		.MinDesiredWidth(125.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(0.f, 0.f, XYZPadding, 0.f)
			.AutoWidth()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					bAxisX->CreatePropertyNameWidget()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					bAxisX->CreatePropertyValueWidget()
				]
			]

			+ SHorizontalBox::Slot()
			.Padding(0.f, 0.f, XYZPadding, 0.f)
			.AutoWidth()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					bAxisY->CreatePropertyNameWidget()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					bAxisY->CreatePropertyValueWidget()
				]
			]

			+ SHorizontalBox::Slot()
			.Padding(0.f, 0.f, XYZPadding, 0.f)
			.AutoWidth()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					bAxisZ->CreatePropertyNameWidget()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					bAxisZ->CreatePropertyValueWidget()
				]
			]
		];
}

void FTLLRN_ModelingToolsAxisFilterCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}


#undef LOCTEXT_NAMESPACE
