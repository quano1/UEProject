// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ModularRig.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlTLLRN_RigElementDetails.h"
#include "Editor/TLLRN_ControlRigWrapperObject.h"
#include "Styling/SlateTypes.h"
#include "IPropertyUtilities.h"
#include "SSearchableComboBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Internationalization/FastDecimalFormat.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Algo/Transform.h"
#include "IPropertyUtilities.h"
#include "Editor/STLLRN_RigHierarchyTreeView.h"

class IPropertyHandle;

class FTLLRN_RigModuleInstanceDetails : public IDetailCustomization
{
public:

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigModuleInstanceDetails);
	}

	FText GetName() const;
	void SetName(const FText& InValue, ETextCommit::Type InCommitType, const TSharedRef<IPropertyUtilities> PropertyUtilities);
	bool OnVerifyNameChanged(const FText& InText, FText& OutErrorMessage);
	FText GetShortName() const;
	void SetShortName(const FText& InValue, ETextCommit::Type InCommitType, const TSharedRef<IPropertyUtilities> PropertyUtilities);
	bool OnVerifyShortNameChanged(const FText& InText, FText& OutErrorMessage);
	FText GetLongName() const;
	FText GetRigClassPath() const;
	TArray<FTLLRN_RigModuleConnector> GetConnectors() const;
	FTLLRN_RigElementKeyRedirector GetConnections() const;
	void PopulateConnectorTargetList(const FString InConnectorKey);
	void PopulateConnectorCurrentTarget(
		TSharedPtr<SVerticalBox> InListBox,
		const FSlateBrush* InBrush,
		const FSlateColor& InColor,
		const FText& InTitle);

	void OnConfigValueChanged(const FName InVariableName);
	void OnConnectorTargetChanged(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo, const FTLLRN_RigModuleConnector InConnector);
	
	struct FPerModuleInfo
	{
		FPerModuleInfo()
			: Path()
			, Module()
			, DefaultModule()
		{}

		bool IsValid() const { return Module.IsValid(); }
		operator bool() const { return IsValid(); }

		const FString& GetPath() const
		{
			return Path;
		}

		UTLLRN_ModularRig* GetTLLRN_ModularRig() const
		{
			return (UTLLRN_ModularRig*)Module.GetTLLRN_ModularRig();
		}
		
		UTLLRN_ModularRig* GetDefaultRig() const
		{
			if(DefaultModule.IsValid())
			{
				return (UTLLRN_ModularRig*)DefaultModule.GetTLLRN_ModularRig();
			}
			return GetTLLRN_ModularRig();
		}

		UTLLRN_ControlRigBlueprint* GetBlueprint() const
		{
			if(const UTLLRN_ModularRig* TLLRN_ControlRig = GetTLLRN_ModularRig())
			{
				return Cast<UTLLRN_ControlRigBlueprint>(TLLRN_ControlRig->GetClass()->ClassGeneratedBy);
			}
			return nullptr;
		}

		FTLLRN_RigModuleInstance* GetModule() const
		{
			return (FTLLRN_RigModuleInstance*)Module.Get();
		}

		FTLLRN_RigModuleInstance* GetDefaultModule() const
		{
			if(DefaultModule)
			{
				return (FTLLRN_RigModuleInstance*)DefaultModule.Get();
			}
			return GetModule();
		}

		const FTLLRN_RigModuleReference* GetReference() const
		{
			if(const UTLLRN_ControlRigBlueprint* Blueprint = GetBlueprint())
			{
				return Blueprint->TLLRN_ModularRigModel.FindModule(Path);
			}
			return nullptr;
		}

		FString Path;
		FModuleInstanceHandle Module;
		FModuleInstanceHandle DefaultModule;
	};

	const FPerModuleInfo& FindModule(const FString& InKey) const;
	const FPerModuleInfo* FindModuleByPredicate(const TFunction<bool(const FPerModuleInfo&)>& InPredicate) const;
	bool ContainsModuleByPredicate(const TFunction<bool(const FPerModuleInfo&)>& InPredicate) const;

	virtual void RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule, UClass* InClass);

protected:

	FText GetBindingText(const FProperty* InProperty) const;
	const FSlateBrush* GetBindingImage(const FProperty* InProperty) const;
	FLinearColor GetBindingColor(const FProperty* InProperty) const;
	void FillBindingMenu(FMenuBuilder& MenuBuilder, const FProperty* InProperty) const;
	bool CanRemoveBinding(FName InPropertyName) const;
	void HandleRemoveBinding(FName InPropertyName) const;
	void HandleChangeBinding(const FProperty* InProperty, const FString& InNewVariablePath) const;

	TArray<FPerModuleInfo> PerModuleInfos;
	TMap<FString, TSharedPtr<SSearchableTLLRN_RigHierarchyTreeView>> ConnectionListBox;

	/** Helper buttons. */
	TMap<FString, TSharedPtr<SButton>> UseSelectedButton;
	TMap<FString, TSharedPtr<SButton>> SelectElementButton;
	TMap<FString, TSharedPtr<SButton>> ResetConnectorButton;
};
