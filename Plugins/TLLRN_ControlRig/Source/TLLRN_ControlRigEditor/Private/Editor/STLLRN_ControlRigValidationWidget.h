// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/SListView.h"
#include "SKismetInspector.h"
#include "Model/MessageLogListingModel.h"
#include "Presentation/MessageLogListingViewModel.h"
#include "TLLRN_ControlRigValidationPass.h"

class STLLRN_ControlRigValidationWidget;

class FTLLRN_ControlRigValidationPassItem : public TSharedFromThis<FTLLRN_ControlRigValidationPassItem>
{
public:
	FTLLRN_ControlRigValidationPassItem(UClass* InClass)
		: Class(InClass)
	{
		static const FName DisplayName(TEXT("DisplayName"));
		DisplayText = FText::FromString(Class->GetMetaData(DisplayName));
	}

	/** The name to display in the UI */
	FText DisplayText;

	/** The struct of the rig unit */
	UClass* Class;
};

class STLLRN_ControlRigValidationPassTableRow : public STableRow<TSharedPtr<FTLLRN_ControlRigValidationPassItem>>
{
	SLATE_BEGIN_ARGS(STLLRN_ControlRigValidationPassTableRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, STLLRN_ControlRigValidationWidget* InValidationWidget, TSharedRef<FTLLRN_ControlRigValidationPassItem> InPassItem);
	void RefreshDetails(UTLLRN_ControlRigValidator* InValidator, UClass* InClass);

private:

	TSharedPtr<SKismetInspector> KismetInspector;
};

class STLLRN_ControlRigValidationWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STLLRN_ControlRigValidationWidget) {}
	SLATE_END_ARGS()

	STLLRN_ControlRigValidationWidget();
	~STLLRN_ControlRigValidationWidget();

	void Construct(const FArguments& InArgs, UTLLRN_ControlRigValidator* InValidator);

	TSharedRef<ITableRow> GenerateClassListRow(TSharedPtr<FTLLRN_ControlRigValidationPassItem> InItem, const TSharedRef<STableViewBase>& InOwningTable);
	ECheckBoxState IsClassEnabled(UClass* InClass) const;
	EVisibility IsClassVisible(UClass* InClass) const;
	void SetClassEnabled(ECheckBoxState NewState, UClass* InClass);

private:

	void HandleClearMessages();
	void HandleMessageReported(EMessageSeverity::Type InSeverity, const FTLLRN_RigElementKey& InKey, float InQuality, const FString& InMessage);
	void HandleMessageTokenClicked(const TSharedRef<class IMessageToken>& InToken);

	UTLLRN_ControlRigValidator* Validator;
	TArray<TSharedPtr<FTLLRN_ControlRigValidationPassItem>> ClassItems;
	TMap<UClass*, TSharedRef<STLLRN_ControlRigValidationPassTableRow>> TableRows;
	TSharedRef<FMessageLogListingModel> ListingModel;
	TSharedRef<FMessageLogListingViewModel> ListingView;

	friend struct FRigValidationTabSummoner;
	friend class STLLRN_ControlRigValidationPassTableRow;
};
