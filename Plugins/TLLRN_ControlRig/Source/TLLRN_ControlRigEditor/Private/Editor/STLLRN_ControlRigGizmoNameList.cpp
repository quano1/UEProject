// Copyright Epic Games, Inc. All Rights Reserved.


#include "Editor/STLLRN_ControlRigGizmoNameList.h"
#include "Widgets/SRigVMGraphPinNameListValueWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "EdGraphSchema_K2.h"
#include "ScopedTransaction.h"
#include "DetailLayoutBuilder.h"
#include "TLLRN_ControlRigBlueprint.h"

void STLLRN_ControlRigShapeNameList::Construct(const FArguments& InArgs, FTLLRN_RigControlElement* ControlElement, UTLLRN_ControlRigBlueprint* InBlueprint)
{
	TArray<FTLLRN_RigControlElement*> ControlElements;
	ControlElements.Add(ControlElement);
	return Construct(InArgs, ControlElements, InBlueprint);
}

void STLLRN_ControlRigShapeNameList::Construct(const FArguments& InArgs, TArray<FTLLRN_RigControlElement*> ControlElements, UTLLRN_ControlRigBlueprint* InBlueprint)
{
	this->OnGetNameListContent = InArgs._OnGetNameListContent;
	this->ControlKeys.Reset();

	for(FTLLRN_RigControlElement* ControlElement : ControlElements)
	{
		this->ControlKeys.Add(ControlElement->GetKey());
	}
	this->Blueprint = InBlueprint;

	ConstructCommon();
}

void STLLRN_ControlRigShapeNameList::Construct(const FArguments& InArgs, TArray<FTLLRN_RigControlElement> ControlElements, UTLLRN_ControlRigBlueprint* InBlueprint)
{
	this->OnGetNameListContent = InArgs._OnGetNameListContent;
	this->ControlKeys.Reset();

	for(const FTLLRN_RigControlElement& ControlElement : ControlElements)
	{
		this->ControlKeys.Add(ControlElement.GetKey());
	}
	this->Blueprint = InBlueprint;

	ConstructCommon();
}

void STLLRN_ControlRigShapeNameList::BeginDestroy()
{
	if(NameListComboBox.IsValid())
	{
		NameListComboBox->SetOptionsSource(&GetEmptyList());
	}
}

void STLLRN_ControlRigShapeNameList::ConstructCommon()
{
	SBox::Construct(SBox::FArguments());

	TSharedPtr<FRigVMStringWithTag> InitialSelected;
	for (TSharedPtr<FRigVMStringWithTag> Item : GetNameList())
	{
		if (Item->Equals(GetNameListText().ToString()))
		{
			InitialSelected = Item;
		}
	}

	SetContent(
		SNew(SBox)
		.MinDesiredWidth(150)
		.MaxDesiredWidth(400)
		[
			SAssignNew(NameListComboBox, SRigVMGraphPinNameListValueWidget)
			.OptionsSource(&GetNameList())
			.OnGenerateWidget(this, &STLLRN_ControlRigShapeNameList::MakeNameListItemWidget)
			.OnSelectionChanged(this, &STLLRN_ControlRigShapeNameList::OnNameListChanged)
			.OnComboBoxOpening(this, &STLLRN_ControlRigShapeNameList::OnNameListComboBox)
			.InitiallySelectedItem(InitialSelected)
			.Content()
			[
				SNew(STextBlock)
				.Text(this, &STLLRN_ControlRigShapeNameList::GetNameListText)
			]
		]
	);
}

const TArray<TSharedPtr<FRigVMStringWithTag>>& STLLRN_ControlRigShapeNameList::GetNameList() const
{
	if (OnGetNameListContent.IsBound())
	{
		return OnGetNameListContent.Execute();
	}
	return GetEmptyList();
}

FText STLLRN_ControlRigShapeNameList::GetNameListText() const
{
	FName FirstName = NAME_None;
	FText Text;
	for(int32 KeyIndex = 0; KeyIndex < ControlKeys.Num(); KeyIndex++)
	{
		const int32 ControlIndex = Blueprint->Hierarchy->GetIndex(ControlKeys[KeyIndex]);
		if (ControlIndex != INDEX_NONE)
		{
			const FName ShapeName = Blueprint->Hierarchy->GetChecked<FTLLRN_RigControlElement>(ControlIndex)->Settings.ShapeName; 
			if(KeyIndex == 0)
			{
				Text = FText::FromName(ShapeName);
				FirstName = ShapeName;
			}
			else if(FirstName != ShapeName)
			{
				static const FString MultipleValues = TEXT("Multiple Values");
				Text = FText::FromString(MultipleValues);
				break;
			}
		}
	}
	return Text;
}

void STLLRN_ControlRigShapeNameList::SetNameListText(const FText& NewTypeInValue, ETextCommit::Type /*CommitInfo*/)
{
	const FScopedTransaction Transaction(NSLOCTEXT("TLLRN_ControlRigEditor", "ChangeShapeName", "Change Shape Name"));

	for(int32 KeyIndex = 0; KeyIndex < ControlKeys.Num(); KeyIndex++)
	{
		const int32 ControlIndex = Blueprint->Hierarchy->GetIndex(ControlKeys[KeyIndex]);
		if (ControlIndex != INDEX_NONE)
		{
			const FName NewName = *NewTypeInValue.ToString();
			UTLLRN_RigHierarchy* Hierarchy = Blueprint->Hierarchy;

			FTLLRN_RigControlElement* ControlElement = Hierarchy->Get<FTLLRN_RigControlElement>(ControlIndex);
			if ((ControlElement != nullptr) && (ControlElement->Settings.ShapeName != NewName))
			{
				Hierarchy->Modify();

				FTLLRN_RigControlSettings Settings = ControlElement->Settings;
				Settings.ShapeName = NewName;
				Hierarchy->SetControlSettings(ControlElement, Settings, true, true, true);
			}
		}
	}
}

TSharedRef<SWidget> STLLRN_ControlRigShapeNameList::MakeNameListItemWidget(TSharedPtr<FRigVMStringWithTag> InItem)
{
	return 	SNew(STextBlock).Text(FText::FromString(InItem->GetString()));// .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

void STLLRN_ControlRigShapeNameList::OnNameListChanged(TSharedPtr<FRigVMStringWithTag> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		if (NewSelection.IsValid())
		{
			const FString& NewValue = NewSelection->GetString();
			SetNameListText(FText::FromString(NewValue), ETextCommit::OnEnter);
		}
		else
		{
			SetNameListText(FText(), ETextCommit::OnEnter);
		}
	}
}

void STLLRN_ControlRigShapeNameList::OnNameListComboBox()
{
	TSharedPtr<FRigVMStringWithTag> CurrentlySelected;
	for (TSharedPtr<FRigVMStringWithTag> Item : GetNameList())
	{
		if (Item->Equals(GetNameListText().ToString()))
		{
			CurrentlySelected = Item;
		}
	}
	NameListComboBox->SetSelectedItem(CurrentlySelected);
}
