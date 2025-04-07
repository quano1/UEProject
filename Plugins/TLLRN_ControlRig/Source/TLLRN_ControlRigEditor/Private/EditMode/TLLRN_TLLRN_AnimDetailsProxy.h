// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Engine/EngineBaseTypes.h"
#include "TLLRN_ControlTLLRN_RigControlsProxy.h"
#include "TLLRN_TLLRN_AnimDetailsProxy.generated.h"


/**
* State for details, including if they are selected(shown in curve editor) or have multiple different values.
*/
struct FTLLRN_AnimDetailPropertyState
{
	bool bMultiple = false;
	bool bSelected = false;
};

struct FTLLRN_AnimDetailVectorState
{
	bool bXMultiple = false;
	bool bYMultiple = false;
	bool bZMultiple = false;

	bool bXSelected = false;
	bool bYSelected = false;
	bool bZSelected = false;
};

//note control rig uses 'float' controls so we call this float though it's a 
//double internally so we can use same for non-control rig parameters
USTRUCT(BlueprintType)
struct FTLLRN_AnimDetailProxyFloat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Interp, Category = "Float", meta = (SliderExponent = "1.0"))
	double Float = 0.0;

};

USTRUCT(BlueprintType)
struct FTLLRN_AnimDetailProxyBool
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Interp, Category = "Bool")
	bool Bool = false;

};

USTRUCT(BlueprintType)
struct FTLLRN_ControlRigEnumControlProxyValue
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_ControlRigEnumControlProxyValue()
	{
		EnumType = nullptr;
		EnumIndex = INDEX_NONE;
	}

	UPROPERTY()
	TObjectPtr<UEnum> EnumType;

	UPROPERTY(EditAnywhere, Category = Enum)
	int32 EnumIndex;
};

class FTLLRN_ControlRigEnumControlProxyValueDetails : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_ControlRigEnumControlProxyValueDetails);
	}

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:

	int32 GetEnumValue() const;
	void OnEnumValueChanged(int32 InValue, ESelectInfo::Type InSelectInfo, TSharedRef<IPropertyHandle> InStructHandle);

	UTLLRN_AnimDetailControlsProxyEnum* ProxyBeingCustomized;
};

USTRUCT(BlueprintType)
struct FTLLRN_AnimDetailProxyInteger
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Interp, Category = "Integer")
	int64 Integer = 0;

};

USTRUCT(BlueprintType)
struct FTLLRN_AnimDetailProxyVector3 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Interp, Category = "Vector")
	double X = 0.0;

	UPROPERTY(EditAnywhere, Interp, Category = "Vector")
	double Y = 0.0;

	UPROPERTY(EditAnywhere, Interp, Category = "Vector")
	double Z = 0.0;

	FVector ToVector() const { return FVector(X, Y, Z); }
};

USTRUCT(BlueprintType)
struct  FTLLRN_AnimDetailProxyLocation 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Interp, Category = "Location", meta = (Delta = "0.5", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double LX = 0.0;

	UPROPERTY(EditAnywhere, Interp, Category = "Location", meta = (Delta = "0.5", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double LY = 0.0;

	UPROPERTY(EditAnywhere, Interp, Category = "Location", meta = (Delta = "0.5", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double LZ = 0.0;

	FTLLRN_AnimDetailVectorState State;

	FTLLRN_AnimDetailProxyLocation() : LX(0.0), LY(0.0), LZ(0.0) {};
	FTLLRN_AnimDetailProxyLocation(const FVector& InVector, const FTLLRN_AnimDetailVectorState &InState) 
	{
		LX = InVector.X; LY = InVector.Y; LZ = InVector.Z; State = InState;
	}
	FTLLRN_AnimDetailProxyLocation(const FVector3f& InVector, const FTLLRN_AnimDetailVectorState& InState) 
	{
		LX = InVector.X; LY = InVector.Y; LZ = InVector.Z; State = InState;
	}
	FVector ToVector() const { return FVector(LX, LY, LZ); }
	FVector3f ToVector3f() const { return FVector3f(LX, LY, LZ); }

};

USTRUCT(BlueprintType)
struct  FTLLRN_AnimDetailProxyRotation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Interp, Category = "Rotation", meta = (Delta = "0.5", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double RX = 0.0;

	UPROPERTY(EditAnywhere, Interp, Category = "Rotation", meta = (Delta = "0.5", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double RY = 0.0;

	UPROPERTY(EditAnywhere, Interp, Category = "Rotation", meta = (Delta = "0.5", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double RZ = 0.0;

	FTLLRN_AnimDetailVectorState State;

	FTLLRN_AnimDetailProxyRotation() : RX(0.0), RY(0.0), RZ(0.0) {};
	FTLLRN_AnimDetailProxyRotation(const FRotator& InRotator, const FTLLRN_AnimDetailVectorState& InState) { FromRotator(InRotator); State = InState; }
	FTLLRN_AnimDetailProxyRotation(const FVector3f& InVector, const FTLLRN_AnimDetailVectorState& InState) 
	{ 
		RX = InVector.X; RY = InVector.Y; RZ = InVector.Z; State = InState;
	}

	FVector ToVector() const { return FVector(RX, RY, RZ); }
	FVector3f ToVector3f() const { return FVector3f(RX, RY, RZ); }
	FRotator ToRotator() const { FRotator Rot; Rot = Rot.MakeFromEuler(ToVector()); return Rot; }
	void FromRotator(const FRotator& InRotator) { FVector Vec(InRotator.Euler()); RX = Vec.X; RY = Vec.Y; RZ = Vec.Z; }
};

USTRUCT(BlueprintType)
struct  FTLLRN_AnimDetailProxyScale 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Interp, Category = "Scale", meta = (Delta = "0.5", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double SX = 1.0;

	UPROPERTY(EditAnywhere, Interp, Category = "Scale", meta = (Delta = "0.5", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double SY = 1.0;

	UPROPERTY(EditAnywhere, Interp, Category = "Scale", meta = (Delta = "0.5", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double SZ = 1.0;

	FTLLRN_AnimDetailVectorState State;

	FTLLRN_AnimDetailProxyScale() : SX(1.0), SY(1.0), SZ(1.0) {};
	FTLLRN_AnimDetailProxyScale(const FVector& InVector, const FTLLRN_AnimDetailVectorState& InState) 
	{
		SX = InVector.X; SY = InVector.Y; SZ = InVector.Z; State = InState;
	}
	FTLLRN_AnimDetailProxyScale(const FVector3f& InVector, const FTLLRN_AnimDetailVectorState& InState)
	{
		SX = InVector.X; SY = InVector.Y; SZ = InVector.Z; State = InState;
	}
	FVector ToVector() const { return FVector(SX, SY, SZ); }
	FVector3f ToVector3f() const { return FVector3f(SX, SX, SZ); }
};

USTRUCT(BlueprintType)
struct  FTLLRN_AnimDetailProxyVector2D
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Interp, Category = "Vector2D", meta = (Delta = "0.05", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double X = 0.0;

	UPROPERTY(EditAnywhere, Interp, Category = "Vector2D", meta = (Delta = "0.05", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	double Y = 0.0;


	FTLLRN_AnimDetailVectorState State;

	FTLLRN_AnimDetailProxyVector2D() : X(0.0), Y(0.0) {};
	FTLLRN_AnimDetailProxyVector2D(const FVector2D& InVector, const FTLLRN_AnimDetailVectorState& InState) 
	{ 
		X = InVector.X; Y = InVector.Y; State = InState;

	}
	FVector2D ToVector2D() const { return FVector2D(X,Y); }
};

UCLASS()
class UTLLRN_AnimDetailControlsKeyedProxy : public UTLLRN_ControlTLLRN_RigControlsProxy
{
	GENERATED_BODY()
public:
	virtual void SetKey(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& KeyedPropertyHandle) override;
	virtual EPropertyKeyedStatus GetPropertyKeyedStatus(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& PropertyHandle) const override;

#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif

};

UCLASS(EditInlineNew, CollapseCategories)
class UTLLRN_AnimDetailControlsProxyFloat : public UTLLRN_AnimDetailControlsKeyedProxy
{
	GENERATED_BODY()
public:

	//UTLLRN_ControlTLLRN_RigControlsProxy
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) override;
	virtual void ValueChanged() override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& InPropertyName) const override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const override;
	virtual TMap<FName, int32> GetPropertyNames() const override;
	virtual bool IsMultiple(const FName& InPropertyName) const override;
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) override;
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) override;
	virtual void UpdatePropertyNames(IDetailLayoutBuilder& DetailBuilder);
	virtual void SetBindingValueFromCurrent(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding, const FTLLRN_RigControlModifiedContext& Context, bool bInteractive = false) override;

	FTLLRN_AnimDetailPropertyState State;

	UPROPERTY(EditAnywhere, Interp, Category = "Float", meta = (Delta = "0.05", SliderExponent = "1", LinearDeltaSensitivity = "1"))
	FTLLRN_AnimDetailProxyFloat Float;
	
private:
	void ClearMultipleFlags();
	void SetMultipleFlags(const float& ValA, const float& ValB);
};

UCLASS(EditInlineNew, CollapseCategories)
class UTLLRN_AnimDetailControlsProxyBool : public UTLLRN_AnimDetailControlsKeyedProxy
{
	GENERATED_BODY()
public:

	//UTLLRN_ControlTLLRN_RigControlsProxy
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) override;
	virtual void ValueChanged() override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& InPropertyName) const override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const override;
	virtual TMap<FName, int32> GetPropertyNames() const override;
	virtual bool IsMultiple(const FName& InPropertyName) const override;
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) override;
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) override;
	virtual void UpdatePropertyNames(IDetailLayoutBuilder& DetailBuilder);
	virtual void SetBindingValueFromCurrent(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding, const FTLLRN_RigControlModifiedContext& Context, bool bInteractive = false) override;

	
	FTLLRN_AnimDetailPropertyState State;

	UPROPERTY(EditAnywhere, Interp, Category = "Bool")
	FTLLRN_AnimDetailProxyBool Bool;

private:
	void ClearMultipleFlags();
	void SetMultipleFlags(const bool& ValA, const bool& ValB);
};

UCLASS(EditInlineNew, CollapseCategories)
class UTLLRN_AnimDetailControlsProxyInteger : public UTLLRN_AnimDetailControlsKeyedProxy
{
	GENERATED_BODY()
public:

	//UTLLRN_ControlTLLRN_RigControlsProxy
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) override;
	virtual void ValueChanged() override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& InPropertyName) const override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const override;
	virtual TMap<FName, int32> GetPropertyNames() const override;
	virtual bool IsMultiple(const FName& InPropertyName) const override;
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) override;
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) override;
	virtual void UpdatePropertyNames(IDetailLayoutBuilder& DetailBuilder);
	virtual void SetBindingValueFromCurrent(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding, const FTLLRN_RigControlModifiedContext& Context, bool bInteractive = false) override;


	FTLLRN_AnimDetailPropertyState State;

	UPROPERTY(EditAnywhere, Interp, Category = "Integer")
	FTLLRN_AnimDetailProxyInteger Integer;

private:
	void ClearMultipleFlags();
	void SetMultipleFlags(const int64& ValA, const int64& ValB);
};

UCLASS(EditInlineNew, CollapseCategories)
class UTLLRN_AnimDetailControlsProxyEnum : public UTLLRN_AnimDetailControlsKeyedProxy
{
	GENERATED_BODY()
public:

	//UTLLRN_ControlTLLRN_RigControlsProxy
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) override;
	virtual void ValueChanged() override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& InPropertyName) const override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const override;
	virtual TMap<FName, int32> GetPropertyNames() const override;
	virtual bool IsMultiple(const FName& InPropertyName) const override;
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) override;
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) override;
	virtual void UpdatePropertyNames(IDetailLayoutBuilder& DetailBuilder);


	FTLLRN_AnimDetailPropertyState State;

	UPROPERTY(EditAnywhere, Interp, Category = "Enum")
	FTLLRN_ControlRigEnumControlProxyValue Enum;

private:
	void ClearMultipleFlags();
	void SetMultipleFlags(const int32& ValA, const int32& ValB);
};


UCLASS(EditInlineNew,CollapseCategories)
class UTLLRN_AnimDetailControlsProxyTransform : public UTLLRN_AnimDetailControlsKeyedProxy
{
	GENERATED_BODY()
public:

	//UTLLRN_ControlTLLRN_RigControlsProxy
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) override;
	virtual void ValueChanged() override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& InPropertyName) const override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const override;
	virtual TMap<FName, int32> GetPropertyNames() const override;
	virtual bool IsMultiple(const FName& InPropertyName) const override;
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) override;
	virtual void SetBindingValueFromCurrent(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding, const FTLLRN_RigControlModifiedContext& Context, bool bInteractive = false) override;
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) override;

	UPROPERTY(EditAnywhere, Interp, Category = Transform)
	FTLLRN_AnimDetailProxyLocation Location;

	UPROPERTY(EditAnywhere, Interp, Category = Transform)
	FTLLRN_AnimDetailProxyRotation Rotation;

	UPROPERTY(EditAnywhere, Interp, Category = Transform)
	FTLLRN_AnimDetailProxyScale Scale;

private:
	void ClearMultipleFlags();
	void SetMultipleFlags(const FEulerTransform& ValA, const FEulerTransform& ValB);
};


UCLASS(EditInlineNew, CollapseCategories)
class UTLLRN_AnimDetailControlsProxyLocation : public UTLLRN_AnimDetailControlsKeyedProxy
{
	GENERATED_BODY()
public:

	//UTLLRN_ControlTLLRN_RigControlsProxy
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) override;
	virtual void ValueChanged() override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& InPropertyName) const override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const override;
	virtual TMap<FName, int32> GetPropertyNames() const override;
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) override;
	virtual bool IsMultiple(const FName& InPropertyName) const override;
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) override;

	UPROPERTY(EditAnywhere, Interp, Category = Location)
	FTLLRN_AnimDetailProxyLocation Location;

	private:
	void ClearMultipleFlags();
	void SetMultipleFlags(const FVector3f& ValA, const FVector3f& ValB);
};

UCLASS(EditInlineNew, CollapseCategories)
class UTLLRN_AnimDetailControlsProxyRotation : public UTLLRN_AnimDetailControlsKeyedProxy
{
	GENERATED_BODY()
public:

	//UTLLRN_ControlTLLRN_RigControlsProxy
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) override;
	virtual void ValueChanged() override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& InPropertyName) const override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const override;
	virtual TMap<FName, int32> GetPropertyNames() const override;
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) override;
	virtual bool IsMultiple(const FName& InPropertyName) const override;
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) override;

	UPROPERTY(EditAnywhere, Interp, Category = Rotation)
	FTLLRN_AnimDetailProxyRotation Rotation;

private:
	void ClearMultipleFlags();
	void SetMultipleFlags(const FVector3f& ValA, const FVector3f& ValB);
};

UCLASS(EditInlineNew, CollapseCategories)
class UTLLRN_AnimDetailControlsProxyScale : public UTLLRN_AnimDetailControlsKeyedProxy
{
	GENERATED_BODY()
public:

	//UTLLRN_ControlTLLRN_RigControlsProxy
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) override;
	virtual void ValueChanged() override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& InPropertyName) const override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const override;
	virtual TMap<FName, int32> GetPropertyNames() const override;
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) override;
	virtual bool IsMultiple(const FName& InPropertyName) const override;
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) override;

	UPROPERTY(EditAnywhere, Interp, Category = Scale)
	FTLLRN_AnimDetailProxyScale Scale;

private:
	void ClearMultipleFlags();
	void SetMultipleFlags(const FVector3f& ValA, const FVector3f& ValB);
};

UCLASS(EditInlineNew, CollapseCategories)
class UTLLRN_AnimDetailControlsProxyVector2D : public UTLLRN_AnimDetailControlsKeyedProxy
{
	GENERATED_BODY()
public:

	//UTLLRN_ControlTLLRN_RigControlsProxy
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty) override;
	virtual void ValueChanged() override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& InPropertyName) const override;
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const override;
	virtual TMap<FName, int32> GetPropertyNames() const override;
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) override;
	virtual bool IsMultiple(const FName& InPropertyName) const override;
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) override;

	UPROPERTY(EditAnywhere, Interp, Category = Vector2D)
	FTLLRN_AnimDetailProxyVector2D Vector2D;

private:
	void ClearMultipleFlags();
	void SetMultipleFlags(const FVector2D& ValA, const FVector2D& ValB);
};


