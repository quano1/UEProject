// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SRigVMGraphPinNameListValueWidget.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "TLLRN_ControlRigBlueprint.h"

class STLLRN_ControlRigShapeNameList : public SBox
{
public:

	DECLARE_DELEGATE_RetVal( const TArray<TSharedPtr<FRigVMStringWithTag>>&, FOnGetNameListContent );

	SLATE_BEGIN_ARGS(STLLRN_ControlRigShapeNameList){}

		SLATE_EVENT(FOnGetNameListContent, OnGetNameListContent)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FTLLRN_RigControlElement* ControlElement, UTLLRN_ControlRigBlueprint* InBlueprint);
	void Construct(const FArguments& InArgs, TArray<FTLLRN_RigControlElement*> ControlElements, UTLLRN_ControlRigBlueprint* InBlueprint);
	void Construct(const FArguments& InArgs, TArray<FTLLRN_RigControlElement> ControlElements, UTLLRN_ControlRigBlueprint* InBlueprint);
	void BeginDestroy();

protected:

	void ConstructCommon();

	const TArray<TSharedPtr<FRigVMStringWithTag>>& GetNameList() const;
	FText GetNameListText() const;
	virtual void SetNameListText(const FText& NewTypeInValue, ETextCommit::Type CommitInfo);

	TSharedRef<SWidget> MakeNameListItemWidget(TSharedPtr<FRigVMStringWithTag> InItem);
	void OnNameListChanged(TSharedPtr<FRigVMStringWithTag> NewSelection, ESelectInfo::Type SelectInfo);
	void OnNameListComboBox();

	static const TArray< TSharedPtr<FRigVMStringWithTag> >& GetEmptyList()
	{
		static const TArray< TSharedPtr<FRigVMStringWithTag> > EmptyList;
		return EmptyList;
	}

	FOnGetNameListContent OnGetNameListContent;
	TSharedPtr<SRigVMGraphPinNameListValueWidget> NameListComboBox;

	TArray<FTLLRN_RigElementKey> ControlKeys;
	UTLLRN_ControlRigBlueprint* Blueprint;
};
