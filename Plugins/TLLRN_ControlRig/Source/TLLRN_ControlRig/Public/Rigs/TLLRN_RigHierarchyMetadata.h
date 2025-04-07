// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigHierarchyMetadata.generated.h"

struct FTLLRN_RigBaseElement;
class UTLLRN_RigHierarchyController;

#define DECLARE_RIG_METADATA_METHODS(MetadataType) \
template<typename T> \
friend const T* Cast(const MetadataType* InMetadata) \
{ \
   return Cast<T>((const FTLLRN_RigBaseMetadata*) InMetadata); \
} \
template<typename T> \
friend T* Cast(MetadataType* InMetadata) \
{ \
   return Cast<T>((FTLLRN_RigBaseMetadata*) InMetadata); \
} \
template<typename T> \
friend const T* CastChecked(const MetadataType* InMetadata) \
{ \
	return CastChecked<T>((const FTLLRN_RigBaseMetadata*) InMetadata); \
} \
template<typename T> \
friend T* CastChecked(MetadataType* InMetadata) \
{ \
	return CastChecked<T>((FTLLRN_RigBaseMetadata*) InMetadata); \
}

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigBaseMetadata
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	FName Name;

	UPROPERTY()
	ETLLRN_RigMetadataType Type = ETLLRN_RigMetadataType::Invalid;

	mutable const FProperty* ValueProperty = nullptr;

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return true;
	}

public:
	virtual ~FTLLRN_RigBaseMetadata() = default;

	UScriptStruct* GetMetadataStruct() const;
	virtual void Serialize(FArchive& Ar);

	bool IsValid() const { return GetType() != ETLLRN_RigMetadataType::Invalid; }
	const FName& GetName() const { return Name; }
	ETLLRN_RigMetadataType GetType() const { return Type; }
	bool IsArray() const
	{
		return GetValueProperty()->IsA<FArrayProperty>();
	}
	const void* GetValueData() const
	{
		return GetValueProperty()->ContainerPtrToValuePtr<void>(this);
	}
	int32 GetValueSize() const
	{
		return GetValueProperty()->GetElementSize();
	}
	bool SetValueData(const void* InData, int32 InSize)
	{
		if(InData)
		{
			if(InSize == GetValueSize())
			{
				return SetValueDataImpl(InData);
			}
		}
		return false;
	}

	template<typename T>
	bool IsA() const { return T::IsClassOf(this); }

	template<typename T>
    friend const T* Cast(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		if(InMetadata)
		{
			if(InMetadata->IsA<T>())
			{
				return static_cast<const T*>(InMetadata);
			}
		}
		return nullptr;
	}

	template<typename T>
    friend T* Cast(FTLLRN_RigBaseMetadata* InMetadata)
	{
		if(InMetadata)
		{
			if(InMetadata->IsA<T>())
			{
				return static_cast<T*>(InMetadata);
			}
		}
		return nullptr;
	}

	template<typename T>
    friend const T* CastChecked(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		const T* CastElement = Cast<T>(InMetadata);
		check(CastElement);
		return CastElement;
	}

	template<typename T>
    friend T* CastChecked(FTLLRN_RigBaseMetadata* InMetadata)
	{
		T* CastElement = Cast<T>(InMetadata);
		check(CastElement);
		return CastElement;
	}

protected:

	static UScriptStruct* GetMetadataStruct(const ETLLRN_RigMetadataType& InType);
	static FTLLRN_RigBaseMetadata* MakeMetadata(const FName& InName, ETLLRN_RigMetadataType InType);
	static void DestroyMetadata(FTLLRN_RigBaseMetadata** Metadata);

	const FProperty* GetValueProperty() const
	{
		if(ValueProperty == nullptr)
		{
			static constexpr TCHAR ValueName[] = TEXT("Value");
			ValueProperty = GetMetadataStruct()->FindPropertyByName(ValueName);
		}
		return ValueProperty;
	}

	void* GetValueData()
	{
		return GetValueProperty()->ContainerPtrToValuePtr<void>(this);
	}

	virtual bool SetValueDataImpl(const void* InData)
	{
		const FProperty* Property = GetValueProperty();
		if(!Property->Identical(GetValueData(), InData))
		{
			Property->CopyCompleteValue(GetValueData(), InData);
			return true;
		}
		return false;
	}

	friend struct FTLLRN_RigBaseElement;
	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigBoolMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigBoolMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const bool& GetValue() const { return Value; }
	bool& GetValue() { return Value; }
	bool SetValue(const bool& InValue) { return SetValueData(&InValue, sizeof(bool)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::Bool;
	}

protected:

	UPROPERTY()
	bool Value = false;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigBoolArrayMetadata : public FTLLRN_RigBaseMetadata
{
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigBoolArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<bool>& GetValue() const { return Value; }
	TArray<bool>& GetValue() { return Value; }
	bool SetValue(const TArray<bool>& InValue) { return SetValueData(&InValue, sizeof(TArray<bool>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::BoolArray;
	}

protected:

	UPROPERTY()
	TArray<bool> Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigFloatMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigFloatMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const float& GetValue() const { return Value; } 
	float& GetValue() { return Value; }
	bool SetValue(const float& InValue) { return SetValueData(&InValue, sizeof(float)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
    {
    	return InMetadata->GetType() == ETLLRN_RigMetadataType::Float;
    }

protected:

	UPROPERTY()
	float Value = 0.f;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigFloatArrayMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigFloatArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<float>& GetValue() const { return Value; }
	TArray<float>& GetValue() { return Value; }
	bool SetValue(const TArray<float>& InValue) { return SetValueData(&InValue, sizeof(TArray<float>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::FloatArray;
	}

protected:

	UPROPERTY()
	TArray<float> Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigInt32Metadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigInt32Metadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const int32& GetValue() const { return Value; } 
	int32& GetValue() { return Value; }
	bool SetValue(const int32& InValue) { return SetValueData(&InValue, sizeof(int32)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
    {
    	return InMetadata->GetType() == ETLLRN_RigMetadataType::Int32;
    }

protected:

	UPROPERTY()
	int32 Value = 0;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigInt32ArrayMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigInt32ArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<int32>& GetValue() const { return Value; }
	TArray<int32>& GetValue() { return Value; }
	bool SetValue(const TArray<int32>& InValue) { return SetValueData(&InValue, sizeof(TArray<int32>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::Int32Array;
	}

protected:

	UPROPERTY()
	TArray<int32> Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigNameMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigNameMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const FName& GetValue() const { return Value; } 
	FName& GetValue() { return Value; }
	bool SetValue(const FName& InValue) { return SetValueData(&InValue, sizeof(FName)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
    {
    	return InMetadata->GetType() == ETLLRN_RigMetadataType::Name;
    }

protected:

	UPROPERTY()
	FName Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigNameArrayMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigNameArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<FName>& GetValue() const { return Value; }
	TArray<FName>& GetValue() { return Value; }
	bool SetValue(const TArray<FName>& InValue) { return SetValueData(&InValue, sizeof(TArray<FName>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::NameArray;
	}

protected:

	UPROPERTY()
	TArray<FName> Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigVectorMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigVectorMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const FVector& GetValue() const { return Value; } 
	FVector& GetValue() { return Value; }
	bool SetValue(const FVector& InValue) { return SetValueData(&InValue, sizeof(FVector)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
    {
    	return InMetadata->GetType() == ETLLRN_RigMetadataType::Vector;
    }

protected:

	UPROPERTY()
	FVector Value = FVector::ZeroVector;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigVectorArrayMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigVectorArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<FVector>& GetValue() const { return Value; }
	TArray<FVector>& GetValue() { return Value; }
	bool SetValue(const TArray<FVector>& InValue) { return SetValueData(&InValue, sizeof(TArray<FVector>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::VectorArray;
	}

protected:

	UPROPERTY()
	TArray<FVector> Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigRotatorMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigRotatorMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const FRotator& GetValue() const { return Value; } 
	FRotator& GetValue() { return Value; }
	bool SetValue(const FRotator& InValue) { return SetValueData(&InValue, sizeof(FRotator)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
    {
    	return InMetadata->GetType() == ETLLRN_RigMetadataType::Rotator;
    }

protected:

	UPROPERTY()
	FRotator Value = FRotator::ZeroRotator;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigRotatorArrayMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigRotatorArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<FRotator>& GetValue() const { return Value; }
	TArray<FRotator>& GetValue() { return Value; }
	bool SetValue(const TArray<FRotator>& InValue) { return SetValueData(&InValue, sizeof(TArray<FRotator>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::RotatorArray;
	}

protected:

	UPROPERTY()
	TArray<FRotator> Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigQuatMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigQuatMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const FQuat& GetValue() const { return Value; } 
	FQuat& GetValue() { return Value; }
	bool SetValue(const FQuat& InValue) { return SetValueData(&InValue, sizeof(FQuat)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
    {
    	return InMetadata->GetType() == ETLLRN_RigMetadataType::Quat;
    }

protected:

	UPROPERTY()
	FQuat Value = FQuat::Identity;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigQuatArrayMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigQuatArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<FQuat>& GetValue() const { return Value; }
	TArray<FQuat>& GetValue() { return Value; }
	bool SetValue(const TArray<FQuat>& InValue) { return SetValueData(&InValue, sizeof(TArray<FQuat>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::QuatArray;
	}

protected:

	UPROPERTY()
	TArray<FQuat> Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigTransformMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigTransformMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const FTransform& GetValue() const { return Value; } 
	FTransform& GetValue() { return Value; }
	bool SetValue(const FTransform& InValue) { return SetValueData(&InValue, sizeof(FTransform)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
    {
    	return InMetadata->GetType() == ETLLRN_RigMetadataType::Transform;
    }

protected:

	UPROPERTY()
	FTransform Value = FTransform::Identity;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigTransformArrayMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigTransformArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<FTransform>& GetValue() const { return Value; }
	TArray<FTransform>& GetValue() { return Value; }
	bool SetValue(const TArray<FTransform>& InValue) { return SetValueData(&InValue, sizeof(TArray<FTransform>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::TransformArray;
	}

protected:

	UPROPERTY()
	TArray<FTransform> Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigLinearColorMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigLinearColorMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const FLinearColor& GetValue() const { return Value; } 
	FLinearColor& GetValue() { return Value; }
	bool SetValue(const FLinearColor& InValue) { return SetValueData(&InValue, sizeof(FLinearColor)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::LinearColor;
	}

protected:

	UPROPERTY()
	FLinearColor Value = FLinearColor::White;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigLinearColorArrayMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigLinearColorArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<FLinearColor>& GetValue() const { return Value; }
	TArray<FLinearColor>& GetValue() { return Value; }
	bool SetValue(const TArray<FLinearColor>& InValue) { return SetValueData(&InValue, sizeof(TArray<FLinearColor>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::LinearColorArray;
	}

protected:
	UPROPERTY()
	TArray<FLinearColor> Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigElementKeyMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigElementKeyMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const FTLLRN_RigElementKey& GetValue() const { return Value; } 
	FTLLRN_RigElementKey& GetValue() { return Value; }
	bool SetValue(const FTLLRN_RigElementKey& InValue) { return SetValueData(&InValue, sizeof(FTLLRN_RigElementKey)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
    {
    	return InMetadata->GetType() == ETLLRN_RigMetadataType::TLLRN_RigElementKey;
    }

protected:

	UPROPERTY()
	FTLLRN_RigElementKey Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigElementKeyArrayMetadata : public FTLLRN_RigBaseMetadata
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_METADATA_METHODS(FTLLRN_RigElementKeyArrayMetadata)

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);
		Ar << Value;
	}
	const TArray<FTLLRN_RigElementKey>& GetValue() const { return Value; }
	TArray<FTLLRN_RigElementKey>& GetValue() { return Value; }
	bool SetValue(const TArray<FTLLRN_RigElementKey>& InValue) { return SetValueData(&InValue, sizeof(TArray<FTLLRN_RigElementKey>)); }

	static bool IsClassOf(const FTLLRN_RigBaseMetadata* InMetadata)
	{
		return InMetadata->GetType() == ETLLRN_RigMetadataType::TLLRN_RigElementKeyArray;
	}

protected:

	UPROPERTY()
	TArray<FTLLRN_RigElementKey> Value;
};
