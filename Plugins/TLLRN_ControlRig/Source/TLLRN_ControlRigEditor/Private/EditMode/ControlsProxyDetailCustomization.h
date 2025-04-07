// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "TLLRN_TLLRN_AnimDetailsProxy.h"
#include "Customizations/MathStructCustomizations.h"

class IPropertyHandle;
class FCurveModel;
class FCurveEditor;
class IDetailPropertyRow;

struct FPropertySelectionCache
{
	~FPropertySelectionCache() { ClearDelegates(); }

	ETLLRN_AnimDetailSelectionState IsPropertySelected(TSharedPtr<FCurveEditor>& InCurveEditor, UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName);
	void OnCurveModelDisplayChanged(FCurveModel* InCurveModel, bool bDisplayed, const FCurveEditor* InCurveEditor);
	void ClearDelegates();
	void CachePropertySelection(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy);
	ETLLRN_AnimDetailSelectionState CachePropertySelection(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName);

	FTLLRN_AnimDetailVectorSelection LocationSelectionCache;
	FTLLRN_AnimDetailVectorSelection RotationSelectionCache;
	FTLLRN_AnimDetailVectorSelection ScaleSelectionCache;

	bool bCacheValid = false;
	TWeakPtr<FCurveEditor> CurveEditor;
};

class FTLLRN_AnimDetailValueCustomization
		: public FMathStructCustomization
	{
	public:
		static TSharedRef<IPropertyTypeCustomization> MakeInstance();

		FTLLRN_AnimDetailValueCustomization();
		virtual ~FTLLRN_AnimDetailValueCustomization() override;

	protected:
		virtual void MakeHeaderRow(TSharedRef<IPropertyHandle>& StructPropertyHandle, FDetailWidgetRow& Row) override;
		virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
		virtual TSharedRef<SWidget> MakeChildWidget(TSharedRef<IPropertyHandle>& StructurePropertyHandle, TSharedRef<IPropertyHandle>& PropertyHandle) override;

	private:

		UTLLRN_ControlTLLRN_RigControlsProxy* GetProxy(TSharedRef< IPropertyHandle>& PropertyHandle) const;
		FLinearColor GetColorFromProperty(const FName& PropertyName) const;
		ETLLRN_AnimDetailSelectionState IsPropertySelected(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName);
		bool IsMultiple(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& InPropertyName) const;
		
		void TogglePropertySelection(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName) const;
		static void ExtractDoubleMetadata(
			TSharedRef<IPropertyHandle>& PropertyHandle,
			TOptional<double>& MinValue,
			TOptional<double>& MaxValue,
			TOptional<double>& SliderMinValue,
			TOptional<double>& SliderMaxValue,
			double& SliderExponent,
			double& Delta,
			float& ShiftMultiplier,
			float& CtrlMultiplier,
			bool& SupportDynamicSliderMaxValue,
			bool& SupportDynamicSliderMinValue);
		void OnDynamicSliderMaxValueChanged(double NewMaxSliderValue, TWeakPtr<SWidget> InValueChangedSourceWidget, bool IsOriginator, bool UpdateOnlyIfHigher);
		void OnDynamicSliderMinValueChanged(double NewMinSliderValue, TWeakPtr<SWidget> InValueChangedSourceWidget, bool IsOriginator, bool UpdateOnlyIfLower);

		//widgets
		TSharedRef<SWidget> MakeDoubleWidget(TSharedRef<IPropertyHandle>& StructurePropertyHandle, TSharedRef<IPropertyHandle>& PropertyHandle, const FLinearColor& LinearColor);
		TSharedRef<SWidget> MakeBoolWidget(TSharedRef<IPropertyHandle>& StructurePropertyHandle, TSharedRef<IPropertyHandle>& PropertyHandle, const FLinearColor& LinearColor);
		TSharedRef<SWidget> MakeIntegerWidget(TSharedRef<IPropertyHandle>& StructurePropertyHandle, TSharedRef<IPropertyHandle>& PropertyHandle, const FLinearColor& LinearColor);

		UTLLRN_ControlRigDetailPanelControlProxies* GetProxyOwner(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy) const;

		EVisibility IsVisible() const;
	private:
		//current selection cache
		FPropertySelectionCache SelectionCache;
		//the scrubbed value when doing multiples, is basicually used a as a delta to add onto the multiple values
		double MultipleValue = 0.0;
		// use the property row to get expansion state to hide/show widgets, and use the builder/structhandler to get it
		mutable IDetailPropertyRow* DetailPropertyRow = nullptr;
		IDetailLayoutBuilder* DetailBuilder = nullptr;
		TSharedPtr<IPropertyHandle> StructPropertyHandlePtr;

};

class FTLLRN_AnimDetailProxyDetails : public IDetailCustomization
{
public:
	FTLLRN_AnimDetailProxyDetails(const FName& InCategoryName);
	/** Makes a new instance of this detail layout class for a specific detail view requesting it, with the specified category
	name*/
	static TSharedRef<IDetailCustomization> MakeInstance(const FName& InCategoryName);

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	FName CategoryName;
};



