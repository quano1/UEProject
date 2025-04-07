// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "Rigs/TLLRN_RigHierarchyDefines.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Editor/TLLRN_ControlRigWrapperObject.h"
#include "Widgets/SRigVMGraphPinNameListValueWidget.h"
#include "Editor/STLLRN_ControlRigGizmoNameList.h"
#include "Styling/SlateTypes.h"
#include "IPropertyUtilities.h"
#include "SSearchableComboBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "SAdvancedTransformInputBox.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Internationalization/FastDecimalFormat.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Algo/Transform.h"
#include "Editor/STLLRN_RigHierarchyTreeView.h"

#include "TLLRN_ControlTLLRN_RigElementDetails.generated.h"

class IPropertyHandle;

namespace FTLLRN_RigElementKeyDetailsDefs
{
	// Active foreground pin alpha
	static const float ActivePinForegroundAlpha = 1.f;
	// InActive foreground pin alpha
	static const float InactivePinForegroundAlpha = 0.15f;
	// Active background pin alpha
	static const float ActivePinBackgroundAlpha = 0.8f;
	// InActive background pin alpha
	static const float InactivePinBackgroundAlpha = 0.4f;
};

class STLLRN_RigElementKeyWidget : public SCompoundWidget
{
public:

	DECLARE_DELEGATE_RetVal(FText, FGetElementNameAsText);
	DECLARE_DELEGATE_RetVal(ETLLRN_RigElementType, FGetElementType);
	DECLARE_DELEGATE_RetVal(bool, FIsEnabled);
	DECLARE_DELEGATE_OneParam(FOnElementTypeChanged, ETLLRN_RigElementType);
	
	SLATE_BEGIN_ARGS(STLLRN_RigElementKeyWidget)
	{
	}
	SLATE_ARGUMENT(UTLLRN_ControlRigBlueprint*, Blueprint)
	SLATE_ARGUMENT(FSlateColor, ActiveBackgroundColor)
	SLATE_ARGUMENT(FSlateColor, InactiveBackgroundColor)
	SLATE_ARGUMENT(FSlateColor, ActiveForegroundColor)
	SLATE_ARGUMENT(FSlateColor, InactiveForegroundColor)
	SLATE_EVENT(SSearchableComboBox::FOnSelectionChanged, OnElementNameChanged)
	SLATE_EVENT(FOnClicked, OnGetSelectedClicked)
	SLATE_EVENT(FOnClicked, OnSelectInHierarchyClicked)
	SLATE_EVENT(FGetElementNameAsText, OnGetElementNameAsText)
	SLATE_EVENT(FGetElementType, OnGetElementType)
	SLATE_EVENT(FOnElementTypeChanged, OnElementTypeChanged)
	SLATE_EVENT(FIsEnabled, IsEnabled);
	
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<IPropertyHandle> InNameHandle, TSharedPtr<IPropertyHandle> InTypeHandle);
	void Construct(const FArguments& InArgs);

	void UpdateElementNameList();

private:
	TSharedPtr<IPropertyHandle> NameHandle;
	TSharedPtr<IPropertyHandle> TypeHandle;

	/** Helper buttons. */
	TSharedPtr<SButton> UseSelectedButton;
	TSharedPtr<SButton> SelectElementButton;

	FGetElementType OnGetElementType;
	FOnElementTypeChanged OnElementTypeChanged;
	SSearchableComboBox::FOnSelectionChanged OnElementNameChanged;

	UTLLRN_ControlRigBlueprint* BlueprintBeingCustomized;
	TArray<TSharedPtr<FString>> ElementNameList;
	TSharedPtr<SSearchableComboBox> SearchableComboBox;
};

class FTLLRN_RigElementKeyDetails : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigElementKeyDetails);
	}

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:

	ETLLRN_RigElementType GetElementType() const;
	FString GetElementName() const;
	void SetElementName(FString InName);
	void OnElementNameChanged(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectionInfo);
	FText GetElementNameAsText() const;

	/** Helper buttons. */
	TSharedPtr<SButton> UseSelectedButton;
	TSharedPtr<SButton> SelectElementButton;

public:
	
	static FSlateColor OnGetWidgetForeground(const TSharedPtr<SButton> Button);
	static FSlateColor OnGetWidgetBackground(const TSharedPtr<SButton> Button);
	
protected:
	
	FReply OnGetSelectedClicked();
	FReply OnSelectInHierarchyClicked();
	
	TSharedPtr<IPropertyHandle> TypeHandle;
	TSharedPtr<IPropertyHandle> NameHandle;
	UTLLRN_ControlRigBlueprint* BlueprintBeingCustomized;
	TSharedPtr<STLLRN_RigElementKeyWidget> TLLRN_RigElementKeyWidget;
};

UENUM()
enum class ETLLRN_RigElementDetailsTransformComponent : uint8
{
	TranslationX,
	TranslationY,
	TranslationZ,
	RotationRoll,
	RotationPitch,
	RotationYaw,
	ScaleX,
	ScaleY,
	ScaleZ
};

class FTLLRN_RigComputedTransformDetails : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigComputedTransformDetails);
	}

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:

	TSharedPtr<IPropertyHandle> TransformHandle;
	FEditPropertyChain PropertyChain;
	UTLLRN_ControlRigBlueprint* BlueprintBeingCustomized;

	void OnTransformChanged(FEditPropertyChain* InPropertyChain);
};

class FTLLRN_RigControlTransformChannelDetails : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigControlTransformChannelDetails);
	}

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:
	ETLLRN_RigControlTransformChannel GetChannel() const;
	int32 GetChannelAsInt32() const { return (int32)GetChannel(); }
	void OnChannelChanged(int32 NewSelection, ESelectInfo::Type);

public:
	static const TArray<ETLLRN_RigControlTransformChannel>* GetVisibleChannelsForControlType(ETLLRN_RigControlType InControlType);

private:
	TSharedPtr<IPropertyHandle> Handle;
};

class FTLLRN_RigBaseElementDetails : public IDetailCustomization
{
public:

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void BeginDestroy() {};
	virtual void PendingDelete() override;

	FTLLRN_RigElementKey GetElementKey() const;
	FText GetName() const;
	void SetName(const FText& InNewText, ETextCommit::Type InCommitType);
	bool OnVerifyNameChanged(const FText& InText, FText& OutErrorMessage);

	void OnStructContentsChanged(FProperty* InProperty, const TSharedRef<IPropertyUtilities> PropertyUtilities);
	bool IsConstructionModeEnabled() const;

	FText GetParentElementName() const;

	TArray<FTLLRN_RigElementKey> GetElementKeys() const;

	template<typename T>
	TArray<T> GetElementsInDetailsView(const TArray<FTLLRN_RigElementKey>& InFilter = TArray<FTLLRN_RigElementKey>()) const
	{
		TArray<T> Elements;
		for(const FPerElementInfo& Info : PerElementInfos)
		{
			T Content = Info.WrapperObject->GetContent<T>();
			if(!InFilter.IsEmpty() && !InFilter.Contains(Content.GetKey()))
			{
				continue;
			}
			Elements.Add(Content);
		}
		return Elements;
	}

	struct FPerElementInfo
	{
		FPerElementInfo()
			: WrapperObject()
			, Element()
			, DefaultElement()
		{}

		bool IsValid() const { return Element.IsValid(); }
		bool IsProcedural() const { return Element.IsValid() && Element.Get()->IsProcedural(); }
		operator bool() const { return IsValid(); }

		UTLLRN_RigHierarchy* GetHierarchy() const { return (UTLLRN_RigHierarchy*)Element.GetHierarchy(); }
		UTLLRN_RigHierarchy* GetDefaultHierarchy() const
		{
			if(DefaultElement.IsValid())
			{
				return (UTLLRN_RigHierarchy*)DefaultElement.GetHierarchy();
			}
			return GetHierarchy();
		}

		UTLLRN_ControlRigBlueprint* GetBlueprint() const
		{
			if(const UTLLRN_ControlRig* TLLRN_ControlRig = GetHierarchy()->GetTypedOuter<UTLLRN_ControlRig>())
			{
				return Cast<UTLLRN_ControlRigBlueprint>(TLLRN_ControlRig->GetClass()->ClassGeneratedBy);
			}
			return GetDefaultHierarchy()->GetTypedOuter<UTLLRN_ControlRigBlueprint>();
		}

		template<typename T = FTLLRN_RigBaseElement>
		T* GetElement() const
		{
			return (T*)Element.Get<T>();
		}

		template<typename T = FTLLRN_RigBaseElement>
		T* GetDefaultElement() const
		{
			if(DefaultElement)
			{
				return (T*)DefaultElement.Get<T>();
			}
			return GetElement<T>();
		}

		TWeakObjectPtr<URigVMDetailsViewWrapperObject> WrapperObject;
		FTLLRN_RigElementHandle Element;
		FTLLRN_RigElementHandle DefaultElement;
	};

	const FPerElementInfo& FindElement(const FTLLRN_RigElementKey& InKey) const;
	bool IsAnyElementOfType(ETLLRN_RigElementType InType) const;
	bool IsAnyElementNotOfType(ETLLRN_RigElementType InType) const;
	bool IsAnyControlOfAnimationType(ETLLRN_RigControlAnimationType InType) const;
	bool IsAnyControlNotOfAnimationType(ETLLRN_RigControlAnimationType InType) const;
	bool IsAnyControlOfValueType(ETLLRN_RigControlType InType) const;
	bool IsAnyControlNotOfValueType(ETLLRN_RigControlType InType) const;
	bool IsAnyElementProcedural() const;
	bool IsAnyConnectorImported() const;
	bool IsAnyConnectorPrimary() const;
	bool GetCommonElementType(ETLLRN_RigElementType& OutElementType) const;
	bool GetCommonControlType(ETLLRN_RigControlType& OutControlType) const;
	bool GetCommonAnimationType(ETLLRN_RigControlAnimationType& OutAnimationType) const;
	const FPerElementInfo* FindElementByPredicate(const TFunction<bool(const FPerElementInfo&)>& InPredicate) const;
	bool ContainsElementByPredicate(const TFunction<bool(const FPerElementInfo&)>& InPredicate) const;

	static void RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule);
	virtual void RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule, UClass* InClass);

	FReply OnSelectParentElementInHierarchyClicked();
	FReply OnSelectElementClicked(const FTLLRN_RigElementKey& InKey);

protected:

	void CustomizeMetadata(IDetailLayoutBuilder& DetailBuilder);

	TArray<FPerElementInfo> PerElementInfos;
	FDelegateHandle MetadataHandle;
	TArray<FTLLRN_ControlRigInteractionScope*> InteractionScopes;
	
	TSharedPtr<SButton> SelectParentElementButton;
};

namespace ETLLRN_RigTransformElementDetailsTransform
{
	enum Type
	{
		Initial,
		Current,
		Offset,
		Minimum,
		Maximum,
		Max
	};
}

class FTLLRN_RigTransformElementDetails : public FTLLRN_RigBaseElementDetails
{
public:

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	/** FTLLRN_RigBaseElementDetails interface */
	virtual void RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule, UClass* InClass) override;

	virtual void CustomizeTransform(IDetailLayoutBuilder& DetailBuilder);

protected:

	bool IsCurrentLocalEnabled() const;

	void AddChoiceWidgetRow(IDetailCategoryBuilder& InCategory, const FText& InSearchText, TSharedRef<SWidget> InWidget);

protected:

	FDetailWidgetRow& CreateTransformComponentValueWidgetRow(
		ETLLRN_RigControlType InControlType,
		const TArray<FTLLRN_RigElementKey>& Keys,
		SAdvancedTransformInputBox<FEulerTransform>::FArguments TransformWidgetArgs,
		IDetailCategoryBuilder& CategoryBuilder, 
		const FText& Label, 
		const FText& Tooltip,
		ETLLRN_RigTransformElementDetailsTransform::Type CurrentTransformType,
		ETLLRN_RigControlValueType ValueType,
		TSharedPtr<SWidget> NameContent = TSharedPtr<SWidget>());

	FDetailWidgetRow& CreateEulerTransformValueWidgetRow(
		const TArray<FTLLRN_RigElementKey>& Keys,
		SAdvancedTransformInputBox<FEulerTransform>::FArguments TransformWidgetArgs,
		IDetailCategoryBuilder& CategoryBuilder, 
		const FText& Label, 
		const FText& Tooltip,
		ETLLRN_RigTransformElementDetailsTransform::Type CurrentTransformType,
		ETLLRN_RigControlValueType ValueType,
		TSharedPtr<SWidget> NameContent = TSharedPtr<SWidget>());

	static ETLLRN_RigTransformElementDetailsTransform::Type GetTransformTypeFromValueType(ETLLRN_RigControlValueType InValueType);

private:

	static TSharedPtr<TArray<ETLLRN_RigTransformElementDetailsTransform::Type>> PickedTransforms;

protected:

	TSharedPtr<FScopedTransaction> SliderTransaction;
};

class FTLLRN_RigBoneElementDetails : public FTLLRN_RigTransformElementDetails
{
public:

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigBoneElementDetails);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

class FTLLRN_RigControlElementDetails : public FTLLRN_RigTransformElementDetails
{
public:

	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigControlElementDetails);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	void CustomizeValue(IDetailLayoutBuilder& DetailBuilder);
	void CustomizeControl(IDetailLayoutBuilder& DetailBuilder);
	void CustomizeAnimationChannels(IDetailLayoutBuilder& DetailBuilder);
	void CustomizeAvailableSpaces(IDetailLayoutBuilder& DetailBuilder);
	void CustomizeShape(IDetailLayoutBuilder& DetailBuilder);
	virtual void BeginDestroy() override;

	/** FTLLRN_RigBaseElementDetails interface */
	virtual void RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule, UClass* InClass) override;

	bool IsShapeEnabled() const;

	const TArray<TSharedPtr<FRigVMStringWithTag>>& GetShapeNameList() const;

	FText GetDisplayName() const;
	void SetDisplayName(const FText& InNewText, ETextCommit::Type InCommitType);
	FText GetDisplayNameForElement(const FTLLRN_RigElementKey& InKey) const;
	void SetDisplayNameForElement(const FText& InNewText, ETextCommit::Type InCommitType, const FTLLRN_RigElementKey& InKeyToRename);
	bool OnVerifyDisplayNameChanged(const FText& InText, FText& OutErrorMessage, const FTLLRN_RigElementKey& InKeyToRename);

	void OnCopyShapeProperties();
	void OnPasteShapeProperties();

	FDetailWidgetRow& CreateBoolValueWidgetRow(
		const TArray<FTLLRN_RigElementKey>& Keys,
		IDetailCategoryBuilder& CategoryBuilder, 
		const FText& Label, 
		const FText& Tooltip, 
		ETLLRN_RigControlValueType ValueType,
		TAttribute<EVisibility> Visibility = EVisibility::Visible,
		TSharedPtr<SWidget> NameContent = TSharedPtr<SWidget>());

	FDetailWidgetRow& CreateFloatValueWidgetRow(
		const TArray<FTLLRN_RigElementKey>& Keys,
		IDetailCategoryBuilder& CategoryBuilder, 
		const FText& Label, 
		const FText& Tooltip, 
		ETLLRN_RigControlValueType ValueType,
		TAttribute<EVisibility> Visibility = EVisibility::Visible,
		TSharedPtr<SWidget> NameContent = TSharedPtr<SWidget>());

	FDetailWidgetRow& CreateIntegerValueWidgetRow(
		const TArray<FTLLRN_RigElementKey>& Keys,
		IDetailCategoryBuilder& CategoryBuilder, 
		const FText& Label, 
		const FText& Tooltip, 
		ETLLRN_RigControlValueType ValueType,
		TAttribute<EVisibility> Visibility = EVisibility::Visible,
		TSharedPtr<SWidget> NameContent = TSharedPtr<SWidget>());

	FDetailWidgetRow& CreateEnumValueWidgetRow(
		const TArray<FTLLRN_RigElementKey>& Keys,
		IDetailCategoryBuilder& CategoryBuilder, 
		const FText& Label, 
		const FText& Tooltip, 
		ETLLRN_RigControlValueType ValueType,
		TAttribute<EVisibility> Visibility = EVisibility::Visible,
		TSharedPtr<SWidget> NameContent = TSharedPtr<SWidget>());

	FDetailWidgetRow& CreateVector2DValueWidgetRow(
		const TArray<FTLLRN_RigElementKey>& Keys,
		IDetailCategoryBuilder& CategoryBuilder, 
		const FText& Label, 
		const FText& Tooltip, 
		ETLLRN_RigControlValueType ValueType,
		TAttribute<EVisibility> Visibility = EVisibility::Visible,
		TSharedPtr<SWidget> NameContent = TSharedPtr<SWidget>());


	// this is a template since we specialize it further down for
	// a 'nearly equals' implementation for floats and math types.
	template<typename T>
	static bool Equals(const T& A, const T& B)
	{
		return A == B;
	}

private:

	template<typename T>
	FDetailWidgetRow& CreateNumericValueWidgetRow(
		const TArray<FTLLRN_RigElementKey>& Keys,
		IDetailCategoryBuilder& CategoryBuilder, 
		const FText& Label, 
		const FText& Tooltip, 
		ETLLRN_RigControlValueType ValueType,
		TAttribute<EVisibility> Visibility = EVisibility::Visible,
		TSharedPtr<SWidget> NameContent = TSharedPtr<SWidget>())
	{
		const bool bShowToggle = (ValueType == ETLLRN_RigControlValueType::Minimum) || (ValueType == ETLLRN_RigControlValueType::Maximum);
		const bool bIsProcedural = IsAnyElementProcedural();
		const bool bIsEnabled = !bIsProcedural || ValueType == ETLLRN_RigControlValueType::Current;

		UTLLRN_RigHierarchy* Hierarchy = PerElementInfos[0].GetHierarchy();
		UTLLRN_RigHierarchy* HierarchyToChange = PerElementInfos[0].GetDefaultHierarchy();
		if(ValueType == ETLLRN_RigControlValueType::Current)
		{
			HierarchyToChange = Hierarchy;
		}

		TSharedPtr<SNumericEntryBox<T>> NumericEntryBox;
		
		FDetailWidgetRow& WidgetRow = CategoryBuilder.AddCustomRow(Label);
		TAttribute<ECheckBoxState> ToggleChecked;
		FOnCheckStateChanged OnToggleChanged;

		if(bShowToggle)
		{
			ToggleChecked = TAttribute<ECheckBoxState>::CreateLambda(
				[ValueType, Keys, Hierarchy]() -> ECheckBoxState
				{
					TOptional<bool> FirstValue;

					for(const FTLLRN_RigElementKey& Key : Keys)
					{
						if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Key))
						{
							if(ControlElement->Settings.LimitEnabled.Num() == 1)
							{
								const bool Value = ControlElement->Settings.LimitEnabled[0].GetForValueType(ValueType);
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
				});

			OnToggleChanged = FOnCheckStateChanged::CreateLambda(
				[ValueType, Keys, HierarchyToChange](ECheckBoxState InValue)
				{
					if(InValue == ECheckBoxState::Undetermined)
					{
						return;
					}

					FScopedTransaction Transaction(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ChangeLimitToggle", "Change Limit Toggle"));
					for(const FTLLRN_RigElementKey& Key : Keys)
					{
						if(FTLLRN_RigControlElement* ControlElement = HierarchyToChange->Find<FTLLRN_RigControlElement>(Key))
						{
							if(ControlElement->Settings.LimitEnabled.Num() == 1)
							{
								HierarchyToChange->Modify();
								ControlElement->Settings.LimitEnabled[0].SetForValueType(ValueType, InValue == ECheckBoxState::Checked);
								HierarchyToChange->SetControlSettings(ControlElement, ControlElement->Settings, true, true, true);
							}
						}
					}
				});
		}

		auto OnValueChanged = [ValueType, Keys, HierarchyToChange, this]
			(TOptional<T> InValue, ETextCommit::Type InCommitType, bool bSetupUndo)
			{
				if(!InValue.IsSet())
				{
					return;
				}

				const T Value = InValue.GetValue();
				
				for(const FTLLRN_RigElementKey& Key : Keys)
				{
					const T PreviousValue = HierarchyToChange->GetControlValue(Key, ValueType).Get<T>();
					if(!Equals(PreviousValue, Value))
					{
						if(!SliderTransaction.IsValid())
						{
							SliderTransaction = MakeShareable(new FScopedTransaction(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ChangeValue", "Change Value")));
							HierarchyToChange->Modify();
						}
						HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<T>(Value), ValueType, bSetupUndo, bSetupUndo);
					}
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
			SAssignNew(NumericEntryBox, SNumericEntryBox<T>)
	        .Font(FAppStyle::GetFontStyle(TEXT("MenuItem.Font")))
	        .AllowSpin(ValueType == ETLLRN_RigControlValueType::Current || ValueType == ETLLRN_RigControlValueType::Initial)
	        .LinearDeltaSensitivity(1)
			.Delta(0.01f)
	        .Value_Lambda([ValueType, Keys, Hierarchy]() -> TOptional<T>
	        {
		        const T FirstValue = Hierarchy->GetControlValue<T>(Keys[0], ValueType);
				for(int32 Index = 1; Index < Keys.Num(); Index++)
				{
					const T SecondValue = Hierarchy->GetControlValue<T>(Keys[Index], ValueType);
					if(FirstValue != SecondValue)
					{
						return TOptional<T>();
					}
				}
				return FirstValue;
	        })
	        .OnValueChanged_Lambda([ValueType, Keys, HierarchyToChange, OnValueChanged](TOptional<T> InValue)
	        {
        		OnValueChanged(InValue, ETextCommit::Default, false);
	        })
	        .OnValueCommitted_Lambda([ValueType, Keys, HierarchyToChange, OnValueChanged](TOptional<T> InValue, ETextCommit::Type InCommitType)
	        {
        		OnValueChanged(InValue, InCommitType, true);
	        })
	        .MinSliderValue_Lambda([ValueType, Keys, Hierarchy]() -> TOptional<T>
			 {
				 if(ValueType == ETLLRN_RigControlValueType::Current || ValueType == ETLLRN_RigControlValueType::Initial)
				 {
			 		return Hierarchy->GetControlValue<T>(Keys[0], ETLLRN_RigControlValueType::Minimum);
				 }
				 return TOptional<T>();
			 })
			 .MaxSliderValue_Lambda([ValueType, Keys, Hierarchy]() -> TOptional<T>
			 {
		 		if(ValueType == ETLLRN_RigControlValueType::Current || ValueType == ETLLRN_RigControlValueType::Initial)
		 		{
					 return Hierarchy->GetControlValue<T>(Keys[0], ETLLRN_RigControlValueType::Maximum);
				 }
				 return TOptional<T>();
			 })
			 .DisplayToggle(bShowToggle)
			 .ToggleChecked(ToggleChecked)
			 .OnToggleChanged(OnToggleChanged)
			 .UndeterminedString(NSLOCTEXT("FTLLRN_RigControlElementDetails", "MultipleValues", "Multiple Values"))
			 .IsEnabled(bIsEnabled)
		]
		.CopyAction(FUIAction(
		FExecuteAction::CreateLambda([ValueType, Keys, Hierarchy]()
			{
				const T FirstValue = Hierarchy->GetControlValue<T>(Keys[0], ValueType);
				const FString Content = FastDecimalFormat::NumberToString(
					FirstValue,
					FastDecimalFormat::GetCultureAgnosticFormattingRules(),
					FNumberFormattingOptions());
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

				T Value = T(0);
				if(FastDecimalFormat::StringToNumber(
					*Content,
					Content.Len(),
					FastDecimalFormat::GetCultureAgnosticFormattingRules(),
					FNumberParsingOptions(),
					Value))
				{
					FScopedTransaction Transaction(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ChangeValue", "Change Value"));
					for(const FTLLRN_RigElementKey& Key : Keys)
					{
						HierarchyToChange->Modify();
						HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<T>(Value), ValueType, true, true); 
					}
				}
			}),
			FCanExecuteAction::CreateLambda([bIsEnabled]() { return bIsEnabled; }))
		);

		if((ValueType == ETLLRN_RigControlValueType::Current || ValueType == ETLLRN_RigControlValueType::Initial) && bIsEnabled)
		{
			WidgetRow.OverrideResetToDefault(FResetToDefaultOverride::Create(
				TAttribute<bool>::CreateLambda([ValueType, Keys, Hierarchy]() -> bool
				{
					const T FirstValue = Hierarchy->GetControlValue<T>(Keys[0], ValueType);
					const T ReferenceValue = ValueType == ETLLRN_RigControlValueType::Initial ? T(0) :
						Hierarchy->GetControlValue<T>(Keys[0], ETLLRN_RigControlValueType::Initial);

					return !FTLLRN_RigControlElementDetails::Equals(FirstValue, ReferenceValue);
				}),
				FSimpleDelegate::CreateLambda([ValueType, Keys, HierarchyToChange]()
				{
					FScopedTransaction Transaction(NSLOCTEXT("TLLRN_ControlTLLRN_RigElementDetails", "ResetValueToDefault", "Reset Value To Default"));
					for(const FTLLRN_RigElementKey& Key : Keys)
					{
						const T ReferenceValue = ValueType == ETLLRN_RigControlValueType::Initial ? T(0) :
							HierarchyToChange->GetControlValue<T>(Keys[0], ETLLRN_RigControlValueType::Initial);
						HierarchyToChange->Modify();
						HierarchyToChange->SetControlValue(Key, FTLLRN_RigControlValue::Make<T>(ReferenceValue), ValueType, true, true); 
					}
				})
			));
		}

		return WidgetRow;
	}

	// animation channel related callbacks
	FReply OnAddAnimationChannelClicked();
	TSharedRef<ITableRow> HandleGenerateAnimationChannelTypeRow(TSharedPtr<ETLLRN_RigControlType> ControlType, const TSharedRef<STableViewBase>& OwnerTable, FTLLRN_RigElementKey ControlKey);

	// multi parent related callbacks
	const FRigTreeDisplaySettings& GetDisplaySettings() const { return DisplaySettings; }
	TSharedRef<SWidget> GetAddSpaceContent(const TSharedRef<IPropertyUtilities> PropertyUtilities);
	FReply OnAddSpaceMouseDown(const FGeometry& InGeometry, const FPointerEvent& InPointerEvent, const TSharedRef<IPropertyUtilities> PropertyUtilities);
	void OnAddSpaceSelection(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo, const TSharedRef<IPropertyUtilities> PropertyUtilities);

	void HandleControlTypeChanged(TSharedPtr<ETLLRN_RigControlType> ControlType, ESelectInfo::Type SelectInfo, FTLLRN_RigElementKey ControlKey, const TSharedRef<IPropertyUtilities> PropertyUtilities);
	void HandleControlTypeChanged(ETLLRN_RigControlType ControlType, TArray<FTLLRN_RigElementKey> ControlKeys, const TSharedRef<IPropertyUtilities> PropertyUtilities);
	void HandleControlEnumChanged(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectionInfo, const TSharedRef<IPropertyUtilities> PropertyUtilities);

	TArray<TSharedPtr<FRigVMStringWithTag>> ShapeNameList;
	TSharedPtr<FTLLRN_RigInfluenceEntryModifier> InfluenceModifier;
	TSharedPtr<FStructOnScope> InfluenceModifierStruct;

	TSharedPtr<IPropertyHandle> ShapeNameHandle;
	TSharedPtr<IPropertyHandle> ShapeColorHandle;

	TSharedPtr<STLLRN_ControlRigShapeNameList> ShapeNameListWidget; 
	static TSharedPtr<TArray<ETLLRN_RigControlValueType>> PickedValueTypes;

	FRigTreeDisplaySettings DisplaySettings;
	TSharedPtr<SMenuAnchor> AddSpaceMenuAnchor;
};

template<>
inline bool FTLLRN_RigControlElementDetails::Equals<float>(const float& A, const float& B)
{
	return FMath::IsNearlyEqual(A, B);
}

template<>
inline bool FTLLRN_RigControlElementDetails::Equals<double>(const double& A, const double& B)
{
	return FMath::IsNearlyEqual(A, B);
}

template<>
inline bool FTLLRN_RigControlElementDetails::Equals<FVector>(const FVector& A, const FVector& B)
{
	return (A - B).IsNearlyZero();
}

template<>
inline bool FTLLRN_RigControlElementDetails::Equals<FRotator>(const FRotator& A, const FRotator& B)
{
	return (A - B).IsNearlyZero();
}

template<>
inline bool FTLLRN_RigControlElementDetails::Equals<FEulerTransform>(const FEulerTransform& A, const FEulerTransform& B)
{
	return Equals(A.Location, B.Location) && Equals(A.Rotation, B.Rotation) && Equals(A.Scale, B.Scale);
}

class FTLLRN_RigNullElementDetails : public FTLLRN_RigTransformElementDetails
{
public:

	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigNullElementDetails);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

class FTLLRN_RigConnectorElementDetails : public FTLLRN_RigTransformElementDetails
{
public:

	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigConnectorElementDetails);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	void CustomizeSettings(IDetailLayoutBuilder& DetailBuilder);
};

class FTLLRN_RigSocketElementDetails : public FTLLRN_RigNullElementDetails
{
public:

	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigSocketElementDetails);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	void CustomizeSettings(IDetailLayoutBuilder& DetailBuilder);

private:

	FReply SetSocketColor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FLinearColor GetSocketColor() const; 
	void OnSocketColorPicked(FLinearColor NewColor);
	void SetSocketDescription(const FText& InDescription, ETextCommit::Type InCommitType);
	FText GetSocketDescription() const; 
};

class FTLLRN_RigConnectionRuleDetails : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigConnectionRuleDetails);
	}

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:
	
	TSharedRef<SWidget> GenerateStructPicker();
	void OnPickedStruct(const UScriptStruct* ChosenStruct);
	FText OnGetStructTextValue() const;
	void OnRuleContentChanged();
	
	FTLLRN_RigConnectionRuleStash RuleStash;
	TSharedPtr<FStructOnScope> Storage;
	UTLLRN_ControlRigBlueprint* BlueprintBeingCustomized;
	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyUtilities> PropertyUtilities;
	TAttribute<bool> EnabledAttribute;
};

class FTLLRN_RigPhysicsElementDetails : public FTLLRN_RigTransformElementDetails
{
public:

	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigPhysicsElementDetails);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	void CustomizeSettings(IDetailLayoutBuilder& DetailBuilder);

private:

	FText GetSolverNameText() const;
};
