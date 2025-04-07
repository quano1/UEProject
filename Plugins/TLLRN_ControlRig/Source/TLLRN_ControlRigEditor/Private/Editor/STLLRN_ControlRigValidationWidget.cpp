// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/STLLRN_ControlRigValidationWidget.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/AppStyle.h"
#include "SlateOptMacros.h"
#include "HAL/ConsoleManager.h"
#include "UserInterface/SMessageLogListing.h"
#include "Framework/Application/SlateApplication.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "UObject/UObjectIterator.h"
#include "Rigs/TLLRN_RigHierarchyController.h"

#define LOCTEXT_NAMESPACE "STLLRN_ControlRigValidationWidget"

//////////////////////////////////////////////////////////////
/// STLLRN_ControlRigValidationPassTableRow
///////////////////////////////////////////////////////////

void STLLRN_ControlRigValidationPassTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, STLLRN_ControlRigValidationWidget* InValidationWidget, TSharedRef<FTLLRN_ControlRigValidationPassItem> InPassItem)
{
	STableRow<TSharedPtr<FTLLRN_ControlRigValidationPassItem>>::Construct(
		STableRow<TSharedPtr<FTLLRN_ControlRigValidationPassItem>>::FArguments()
		.Content()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.Padding(4.0f, 4.0f, 0.f, 0.f)
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 12.f, 0.f)
				[
					SNew(STextBlock)
					.Text(InPassItem->DisplayText)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked(InValidationWidget, &STLLRN_ControlRigValidationWidget::IsClassEnabled, InPassItem->Class)
					.OnCheckStateChanged(InValidationWidget, &STLLRN_ControlRigValidationWidget::SetClassEnabled, InPassItem->Class)
				]
			]
	
			+ SVerticalBox::Slot()
			.Padding(24.0f, 4.0f, 0.f, 0.f)
			.AutoHeight()
			[
				SAssignNew(KismetInspector, SKismetInspector)
				.Visibility(InValidationWidget, &STLLRN_ControlRigValidationWidget::IsClassVisible, InPassItem->Class)
				.ShowTitleArea(false)
			]
		]
		, OwnerTable
	);

	RefreshDetails(InValidationWidget->Validator, InPassItem->Class);
}

void STLLRN_ControlRigValidationPassTableRow::RefreshDetails(UTLLRN_ControlRigValidator* InValidator, UClass* InClass)
{
	check(InValidator);

	if (UTLLRN_ControlRigValidationPass* Pass = InValidator->FindPass(InClass))
	{
		KismetInspector->ShowDetailsForSingleObject(Pass);
	}
	else
	{
		KismetInspector->ShowDetailsForSingleObject(nullptr);
	}
}

//////////////////////////////////////////////////////////////
/// STLLRN_ControlRigValidationWidget
///////////////////////////////////////////////////////////

STLLRN_ControlRigValidationWidget::STLLRN_ControlRigValidationWidget()
	: ListingModel(FMessageLogListingModel::Create(TEXT("ValidationLog")))
	, ListingView(FMessageLogListingViewModel::Create(ListingModel, LOCTEXT("ValidationLog", "Validation Log")))
{
	Validator = nullptr;
	ListingView->OnMessageTokenClicked().AddRaw(this, &STLLRN_ControlRigValidationWidget::HandleMessageTokenClicked);
}

STLLRN_ControlRigValidationWidget::~STLLRN_ControlRigValidationWidget()
{
	if(Validator)
	{
		Validator->SetTLLRN_ControlRig(nullptr);
		Validator->OnClear().Unbind();
		Validator->OnReport().Unbind();
	}
}

void STLLRN_ControlRigValidationWidget::Construct(const FArguments& InArgs, UTLLRN_ControlRigValidator* InValidator)
{
	Validator = InValidator;

	ClassItems.Reset();
	for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
	{
		const bool bIsValidationPassChild = ClassIterator->IsChildOf(UTLLRN_ControlRigValidationPass::StaticClass());
		if (bIsValidationPassChild && !ClassIterator->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			ClassItems.Add(MakeShared<FTLLRN_ControlRigValidationPassItem>(*ClassIterator));
		}
	}

	ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.Padding(4.0f, 4.0f, 0.f, 0.f)
			.AutoHeight()
			[
				SNew(SBorder)
				.Visibility(EVisibility::Visible)
				.BorderImage(FAppStyle::GetBrush("Menu.Background"))
				[
					SNew(SListView<TSharedPtr<FTLLRN_ControlRigValidationPassItem>>)
					.ListItemsSource(&ClassItems)
					.OnGenerateRow(this, &STLLRN_ControlRigValidationWidget::GenerateClassListRow)
					.SelectionMode(ESelectionMode::None)
				]
			]

			+ SVerticalBox::Slot()
			.Padding(4.0f, 4.0f, 0.f, 0.f)
			.AutoHeight()
			[
				SNew(SMessageLogListing, ListingView)
			]
		];

	Validator->OnClear().BindRaw(this, &STLLRN_ControlRigValidationWidget::HandleClearMessages);
	Validator->OnReport().BindRaw(this, &STLLRN_ControlRigValidationWidget::HandleMessageReported);
}

TSharedRef<ITableRow> STLLRN_ControlRigValidationWidget::GenerateClassListRow(TSharedPtr<FTLLRN_ControlRigValidationPassItem> InItem, const TSharedRef<STableViewBase>& InOwningTable)
{
	TSharedRef<STLLRN_ControlRigValidationPassTableRow> TableRow = SNew(STLLRN_ControlRigValidationPassTableRow, InOwningTable, this, InItem.ToSharedRef());
	TableRows.Add(InItem->Class, TableRow);
	return TableRow;
}

ECheckBoxState STLLRN_ControlRigValidationWidget::IsClassEnabled(UClass* InClass) const
{
	if (Validator)
	{
		return Validator->FindPass(InClass) != nullptr ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Unchecked;
}

EVisibility STLLRN_ControlRigValidationWidget::IsClassVisible(UClass* InClass) const
{
	if (IsClassEnabled(InClass) == ECheckBoxState::Checked)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

void STLLRN_ControlRigValidationWidget::SetClassEnabled(ECheckBoxState NewState, UClass* InClass)
{
	if (Validator)
	{
		if (NewState == ECheckBoxState::Checked)
		{
			Validator->AddPass(InClass);
		}
		else
		{
			Validator->RemovePass(InClass);
		}

		if (TSharedRef<STLLRN_ControlRigValidationPassTableRow>* TableRowPtr = TableRows.Find(InClass))
		{
			TSharedRef<STLLRN_ControlRigValidationPassTableRow>& TableRow = *TableRowPtr;
			TableRow->RefreshDetails(Validator, InClass);
		}
	}
}

void STLLRN_ControlRigValidationWidget::HandleClearMessages()
{
	ListingModel->ClearMessages();
}

void STLLRN_ControlRigValidationWidget::HandleMessageReported(EMessageSeverity::Type InSeverity, const FTLLRN_RigElementKey& InKey, float InQuality, const FString& InMessage)
{
	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(InSeverity);

	if (InKey.IsValid())
	{
		FString TypeString = StaticEnum<ETLLRN_RigElementType>()->GetDisplayNameTextByValue((int64)InKey.Type).ToString();
		FString KeyString = FString::Printf(TEXT("%s %s"), *TypeString, *InKey.Name.ToString());
		Message->AddToken(FAssetNameToken::Create(KeyString));
	}

	/*
	if (InQuality != FLT_MAX)
	{
		FString QualityString = FString::Printf(TEXT("%.02f"), InQuality);
		Message->AddToken(FTextToken::Create(FText::FromString(QualityString)));
	}
	*/

	if (!InMessage.IsEmpty())
	{
		Message->AddToken(FTextToken::Create(FText::FromString(InMessage)));
	}

	ListingModel->AddMessage(Message, false, true);
}

void STLLRN_ControlRigValidationWidget::HandleMessageTokenClicked(const TSharedRef<class IMessageToken>& InToken)
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = Validator->GetTLLRN_ControlRig())
	{
		if (InToken->GetType() == EMessageToken::AssetName)
		{
			FString Content = StaticCastSharedRef<FAssetNameToken>(InToken)->GetAssetName();

			FString Left, Right;
			if (Content.Split(TEXT(" "), &Left, &Right))
			{
				int32 TypeIndex = StaticEnum<ETLLRN_RigElementType>()->GetIndexByNameString(Left);
				if (TypeIndex != INDEX_NONE)
				{
					FTLLRN_RigElementKey Key(*Right, (ETLLRN_RigElementType)StaticEnum<ETLLRN_RigElementType>()->GetValueByIndex(TypeIndex));

					UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(TLLRN_ControlRig->GetClass());
					if (BlueprintClass)
					{
						UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(BlueprintClass->ClassGeneratedBy);
						RigBlueprint->GetHierarchyController()->ClearSelection();
						RigBlueprint->GetHierarchyController()->SelectElement(Key, true);

						if(UTLLRN_RigHierarchyController* Controller = TLLRN_ControlRig->GetHierarchy()->GetController())
						{
							Controller->ClearSelection();
							Controller->SelectElement(Key, true);
						}
					}
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
